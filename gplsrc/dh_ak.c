/* DH_AK.C
 * AK routines (Alternative Key?)
 * Copyright (c) 2007 Ladybridge Systems, All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Ladybridge Systems can be contacted via the www.openqm.com web site.
 *
 * ScarletDME Wiki: https://scarlet.deltasoft.com
 *
 * START-HISTORY (Scarlet DME):
 * 17Oct25 mab Change dyn file prefix to %
 * 03Sep25 gwb Fix for potential buffer overrun due to an insufficiently sized sprintf() target.
 *             git issue #82
 * 06Feb22 gwb Fixed an uninitialized variable warning in ak_read().
 * 
 * 15Jan22 gwb Fixed argument formatting issues (CwE-686) 
 * 
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 * 22Feb20 gwb Turned off a warning via #pragma in delete_ak().
 *             Based on context, the code appears to be correct.
 *             See the comment near the #pragma for more information.
 * 
 * START-HISTORY (OpenQM):
 * 09 Jul 07  2.5-7 0556 ak_read() for big record set node number incorrectly.
 * 01 Jul 07  2.5-7 Extensive changes for PDA merge.
 * 21 Nov 06  2.4-17 Revised interface to dio_open().
 * 05 Oct 06  2.4-14 0523 Significant reworking of index scanning.
 * 25 Aug 06  2.4-12 0516 AK scans mis-handled "Not found" errors.
 * 15 May 06  2.4-4 Added case insensitivity.
 * 04 May 06  2.4-4 0487 Do not rely on @SELECTED being an integer when
 *                  overwriting it.
 * 05 Jan 06  2.3-3 Allow index scanning operations on QMNet.
 * 11 Oct 05  2.2-14 AK creation now requires separate AK directory pathname.
 *                   AK deletion may need to remove relocation directory.
 * 20 Sep 05  2.2-11 Use dynamic AKBUFF structures to minimise stack space.
 * 06 May 05  2.1-13 Initial coding for large file support.
 * 27 Dec 04  2.1-0 Store dictionary record in AK subfile for all types.
 * 26 Oct 04  2.0-7 Separated MAX_AK_NAME_LEN from MAX_ID_LEN.
 * 20 Sep 04  2.0-2 Use central message handler.
 * 20 Sep 04  2.0-2 Use pcode loader.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * op_akenable   AKENABLE   Enable an AK
 * op_akdelete   AKDELETE   Delete an AK record
 * op_akmap      AKMAP      Return internal form collation map
 * op_akread     AKREAD     Read an AK record
 * op_akwrite    AKWRITE    Write an AK record
 * op_akclear    CLEAR.AK   Clear an AK subfile
 * op_createak   CREATE.AK  Create an AK index subfile
 * op_deleteak   DELETE.AK  Delete an AK index subfile
 * op_indices1   INDICES1   Return field mark delimited list of indices
 * op_indices2   INDICES2   Return data about named index
 * op_selindx    SELINDX    SELECTINDEX, indexed values
 * op_selindxv   SELINDXV   SELECTINDEX, keys for given value
 * op_selleft    SELLEFT    SELECTLEFT
 * op_selright   SELRIGHT   SELECTRIGHT
 * op_setleft    SETLEFT    Set scan at left side
 * op_setright   SETRIGHT   Set scan at right side
 *
 * Internally, AKs are numbered from zero and may have gaps in the used
 * numbers.
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include "dh_int.h"
#include "locks.h"
#include "header.h"
#include "syscom.h"

/* Macro to find entry from ak.data matrix */
#define AKData(dh_file, akno, item) (Element(dh_file->ak_data, (akno * AKD_COLS) + item))

typedef union AKBUFF AKBUFF;
union AKBUFF {
  char buff[DH_AK_NODE_SIZE];
  DH_INT_NODE int_node;
  DH_TERM_NODE term_node;
  DH_BIG_NODE big_node;
};

typedef struct NODE NODE;
Private NODE *tail;

struct NODE {
  NODE *prev;         /* Previous buffer pointer */
  int32_t node_num;   /* Node number */
  int16_t ci;         /* Child index of next level... */
  int16_t key_offset; /* ...and offset in node of its key */
  union AKBUFF node;  /* The node buffer */
};

Private u_int32_t ak_upd; /* File's ak_upd at time of ak_read */
Private int32_t ak_node_num;
Private int16_t ak_rec_offset;
Private u_char ak_flags;
Private int16_t ak_lock_slot = 0; /* Pseudo record lock position */

/* Internal functions */

Private int16_t find_ak_by_name(DESCRIPTOR *descr, DH_FILE *dh_file);

Private void setakpos(bool right);

Private void akscan(bool right);

Private int16_t create_ak(char *data_path, char *ak_path, char *ak_name, int16_t fno, u_int16_t flags, STRING_CHUNK *dict_rec, char *collation_map_name, char *collation_map);

Private bool delete_ak(char *pathname, int16_t akno);

Private void ak_delete(DH_FILE *dh_file, int16_t akno, char id[], int16_t id_len);

Private STRING_CHUNK *ak_read(DH_FILE *dh_file, int16_t akno, char id[], int16_t id_len, bool read_data);

Private bool ak_write(DH_FILE *dh_file, int16_t akno, char id[], int16_t id_len, STRING_CHUNK *rec);

Private DH_RECORD *rightmost(DH_TERM_NODE *node);

Private void copy_ak_record(DH_FILE *dh_file, DH_RECORD *rec_ptr, int16_t base_size, char *id, int16_t id_len, int32_t big_rec_head, int32_t data_len, STRING_CHUNK *str, int16_t pad_bytes);

Private int32_t get_ak_node(DH_FILE *dh_file, int16_t subfile);

Private bool free_ak_node(DH_FILE *dh_file, int16_t subfile, int32_t node_num);

Private int16_t compare(char *s1, int16_t s1_len, char *s2, int16_t s2_len, bool right_justified, bool nocase);

Private bool update_internal_node(DH_FILE *dh_file,
                                  int16_t subfile,
                                  NODE *node_ptr,
                                  char *id1,
                                  int16_t id1_len,
                                  int32_t offset1,
                                  char *id2,
                                  int16_t id2_len,
                                  int32_t offset2,
                                  char *id3,
                                  int16_t id3_len,
                                  int32_t offset3);

Private STRING_CHUNK *ak_read_record(DH_FILE *dh_file, int16_t subfile, DH_RECORD *rec_ptr);

Private int32_t write_ak_big_rec(DH_FILE *dh_file, int16_t subfile, STRING_CHUNK *rec, int32_t data_len);

Private bool free_ak_big_rec(DH_FILE *dh_file, int16_t subfile, int32_t head);

/* ======================================================================
   op_akdelete()  -  AKDELETE                                             */

void op_akdelete() {
  /* Stack:

      |=============================|=============================|
      |            BEFORE           |           AFTER             |
      |=============================|=============================|
  top |  Record id                  | Status 0=ok, else error     |
      |-----------------------------|-----------------------------|
      |  AK number (internal)       |                             |
      |-----------------------------|-----------------------------|
      |  File var                   |                             |
      |=============================|=============================|

      If the file var is unassigned, the file to be processed is accessed
      via ak_dh_file which is set by all opcodes that could end up here
      (DELETE and WRITE).
  */

  DESCRIPTOR *descr;
  int16_t akno;
  char id[MAX_KEY_LEN + 1];
  int16_t id_len;

  process.status = 0;

  /* Get record id */

  descr = e_stack - 1;
  id_len = k_get_c_string(descr, id, MAX_KEY_LEN);

  /* Get AK number */

  descr = e_stack - 2;
  GetInt(descr);
  akno = (int16_t)(descr->data.value);

  /* Get file */

  descr = e_stack - 3;
  while (descr->type == ADDR)
    descr = descr->data.d_addr;
  if (descr->type != UNASSIGNED) {
    if (descr->type != FILE_REF)
      k_error("AK_READ not for file\n");
    ak_dh_file = descr->data.fvar->access.dh.dh_file;
  }

  /* Check validity for AK number */

  if (!(((ak_dh_file->ak_map) >> akno) & 1))
    k_error(sysmsg(1001));

  if (id_len < 0) {
    process.status = ER_IID;
  } else /* Delete the record */
  {
    ak_delete(ak_dh_file, akno, id, id_len);
    process.status = dh_err;
  }

  /* Release pseudo group lock that actually locks the AK record */

  if (ak_lock_slot) {
    FreeGroupWriteLock(ak_lock_slot);
    ak_lock_slot = 0;
  }

  k_dismiss(); /* Record id */
  k_pop(1);    /* AK number */
  k_dismiss(); /* file */

  /* Set status code on stack */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = process.status;
}

/* ======================================================================
   op_akrelease()  -  AKRELEASE                                           */

void op_akrelease() {
  /* Release pseudo group lock that actually locks the AK record. Because
     this can only ever be set from _AK which is called as a recursive from
     dh_write and dh_delete, there can only ever be at most one such lock
     for any user. Therefore, we do not need any qualifying information to
     this opcode.                                                          */

  if (ak_lock_slot) {
    FreeGroupWriteLock(ak_lock_slot);
    ak_lock_slot = 0;
  }
}

/* ======================================================================
   op_akread()  -  AKREAD                                                 */

void op_akread() {
  /* Stack:

      |=============================|=============================|
      |            BEFORE           |           AFTER             |
      |=============================|=============================|
  top |  Record id                  | Status 0=ok, else error     |
      |-----------------------------|-----------------------------|
      |  AK number (internal)       |                             |
      |-----------------------------|-----------------------------|
      |  File var                   |                             |
      |-----------------------------|-----------------------------|
      |  ADDR to target             |                             |
      |=============================|=============================|

      If the file var is unassigned, the file to be processed is accessed
      via ak_dh_file which is set by all opcodes that could end up here
      (DELETE and WRITE).
  */

  DESCRIPTOR *descr;
  int16_t akno;
  char id[MAX_KEY_LEN + 1];
  int16_t id_len;
  u_int16_t key;
  int16_t i;

  process.status = 0;

  /* Get record id */

  descr = e_stack - 1;
  id_len = k_get_c_string(descr, id, MAX_KEY_LEN);

  /* Get AK number */

  descr = e_stack - 2;
  GetInt(descr);
  akno = (int16_t)(descr->data.value);

  /* Get file */

  descr = e_stack - 3;
  while (descr->type == ADDR)
    descr = descr->data.d_addr;
  if (descr->type != UNASSIGNED) {
    if (descr->type != FILE_REF)
      k_error("AK_READ not for file\n");
    ak_dh_file = descr->data.fvar->access.dh.dh_file;
  }

  /* Get target descr */

  descr = e_stack - 4;
  do {
    descr = descr->data.d_addr;
  } while (descr->type == ADDR);
  k_release(descr);

  if (!(((ak_dh_file->ak_map) >> akno) & 1))
    k_error(sysmsg(1001)); /* TODO: Magic numbers are bad, mmkay? */

  if (id_len < 0) {
    process.status = ER_IID;
  } else /* Read the record */
  {
    /* Form key value for pseudo group lock which actually locks AK record */

    key = 0;
    for (i = 0; i < id_len; i += 2) {
      key ^= *((u_int16_t *)(id + i));
    }

    /* Lock the AK record */

    ak_lock_slot = GetGroupWriteLock(ak_dh_file, AKRlock(akno, key));

    InitDescr(descr, STRING);
    descr->data.str.saddr = ak_read(ak_dh_file, akno, id, id_len, TRUE);
    process.status = dh_err;
  }

  k_dismiss(); /* Record id */
  k_pop(1);    /* AK number */
  k_dismiss(); /* Dismiss file */
  k_dismiss(); /* Dismiss ADDR */

  /* Set status code on stack */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = process.status;
}

/* ======================================================================
   op_akwrite()  -  Write AK record                                       */

void op_akwrite() {
  /* Stack:

      |=============================|=============================|
      |            BEFORE           |           AFTER             |
      |=============================|=============================|
  top |  Record id                  | Status 0 = ok, else error   |
      |-----------------------------|-----------------------------|
      |  AK number (internal)       |                             |
      |-----------------------------|-----------------------------|
      |  File var                   |                             |
      |-----------------------------|-----------------------------|
      |  Record data to write       |                             |
      |=============================|=============================|

      If the file var is unassigned, the file to be processed is accessed
      via ak_dh_file which is set by all opcodes that could end up here
      (DELETE and WRITE).
  */

  DESCRIPTOR *descr;
  char id[MAX_KEY_LEN + 1];
  int16_t id_len;
  int16_t akno;

  process.status = 0;

  /* Get record id */

  descr = e_stack - 1;
  id_len = k_get_c_string(descr, id, MAX_KEY_LEN);
  if (id_len < 0)
    goto exit_op_akwrite; /* Too long to index */

  /* Get AK number */

  descr = e_stack - 2;
  GetInt(descr);
  akno = (int16_t)(descr->data.value);

  /* Get file */

  descr = e_stack - 3;
  while (descr->type == ADDR)
    descr = descr->data.d_addr;
  if (descr->type != UNASSIGNED) {
    if (descr->type != FILE_REF)
      k_error("AK_WRITE not for file\n");
    ak_dh_file = descr->data.fvar->access.dh.dh_file;
  }

  /* Get string descr */

  descr = e_stack - 4;
  k_get_string(descr);

  if (!(((ak_dh_file->ak_map) >> akno) & 1))
    k_error(sysmsg(1001));

  if (id_len < 0) {
    process.status = ER_IID;
  } else /* Write the record */
  {
    if (!ak_write(ak_dh_file, akno, id, id_len, descr->data.str.saddr)) {
      process.status = dh_err;
    }
  }

exit_op_akwrite:

  /* Release pseudo group lock that actually locks the AK record */

  if (ak_lock_slot) {
    FreeGroupWriteLock(ak_lock_slot);
    ak_lock_slot = 0;
  }

  k_dismiss();
  k_pop(1);
  k_dismiss();
  k_dismiss();

  /* Set status code on stack */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = process.status;
}

/* ====================================================================== */

void op_akclear() {
  /* Stack:

      |=============================|=============================|
      |            BEFORE           |           AFTER             |
      |=============================|=============================|
  top |  AK number (internal)       |                             |
      |-----------------------------|-----------------------------|
      |  File var                   |                             |
      |=============================|=============================|

      This action will wipe out existing AK data. It is intended for use
      by BUILD.INDEX and may mess up anyone else who is using the file.
  */

  DESCRIPTOR *descr;
  int16_t akno;
  FILE_VAR *fvar;
  DH_FILE *dh_file;

  /* AK number */

  descr = e_stack - 1;
  GetInt(descr);
  akno = (int16_t)(descr->data.value);

  /* File */

  descr = e_stack - 2;
  k_get_file(descr);
  fvar = descr->data.fvar;
  if (fvar->type != DYNAMIC_FILE)
    goto exit_op_akclear; /* Not DH file */
  dh_file = fvar->access.dh.dh_file;

  if (!(dh_file->ak_map & (1 << akno)))
    k_error("AK_CLEAR: No such AK");

  if (!ak_clear(dh_file, AK_BASE_SUBFILE + akno))
    goto exit_op_akclear;

exit_op_akclear:
  k_pop(1);    /* AK number */
  k_dismiss(); /* File */
}

/* ====================================================================== */

void op_akenable() {
  /* Stack:

      |=============================|=============================|
      |            BEFORE           |           AFTER             |
      |=============================|=============================|
  top |  AK number (internal)       |                             |
      |-----------------------------|-----------------------------|
      |  File var                   |                             |
      |=============================|=============================|
  */

  DESCRIPTOR *descr;
  int16_t akno;
  FILE_VAR *fvar;
  DH_FILE *dh_file;
  DH_AK_HEADER ak_hdr;
  int16_t subfile;

  /* AK number */

  descr = e_stack - 1;
  GetInt(descr);
  akno = (int16_t)(descr->data.value);

  /* File */

  descr = e_stack - 2;
  k_get_file(descr);
  fvar = descr->data.fvar;

  if (fvar->type == DYNAMIC_FILE) {
    dh_file = fvar->access.dh.dh_file;

    if (dh_file->ak_map & (1 << akno)) {
      subfile = AK_BASE_SUBFILE + akno;

      if (FDS_open(dh_file, subfile)) {
        if (!dh_read_group(dh_file, subfile, 0, (char *)&ak_hdr, DH_AK_HEADER_SIZE)) {
          goto exit_ak_enable;
        }

        ak_hdr.flags |= AK_ENABLED;

        if (!dh_write_group(dh_file, subfile, 0, (char *)&ak_hdr, DH_AK_HEADER_SIZE)) {
          goto exit_ak_enable;
        }
      }
    }
  }

exit_ak_enable:
  k_pop(1);    /* AK number */
  k_dismiss(); /* File */
}

/* ======================================================================
   op_akmap()  -  Return AK collation map (internal form)                 */

void op_akmap() {
  /* Stack:

      |=============================|=============================|
      |            BEFORE           |           AFTER             |
      |=============================|=============================|
  top |  Index name                 |  Map string                 |
      |-----------------------------|-----------------------------|
      |  File variable              |                             |
      |=============================|=============================|
  */

  DESCRIPTOR *descr;
  FILE_VAR *fvar;
  DH_FILE *dh_file;
  int16_t akno = -1;
  STRING_CHUNK *str;

  /* Find file variable */

  descr = e_stack - 2;
  k_get_file(descr);
  fvar = descr->data.fvar;

  if (fvar->type == DYNAMIC_FILE) {
    dh_file = fvar->access.dh.dh_file;

    /* Find index name */

    descr = e_stack - 1;
    akno = find_ak_by_name(descr, dh_file);
  }

  k_dismiss();
  k_dismiss();

  if (akno < 0) /* No such AK */
  {
    InitDescr(e_stack, STRING);
    (e_stack++)->data.str.saddr = NULL;
  } else {
    /* Extract AK data */

    descr = Element(dh_file->ak_data, (akno * AKD_COLS) + AKD_MAP);
    str = descr->data.str.saddr;
    InitDescr(e_stack, STRING);
    (e_stack++)->data.str.saddr = str;
    if (str != NULL)
      (str->ref_ct)++;
  }
}

/* ====================================================================== */

void op_createak() {
  /* Stack:

      |=============================|=============================|
      |            BEFORE           |           AFTER             |
      |=============================|=============================|
  top |  Collation map              |  0 = ok, else error         |
      |-----------------------------|-----------------------------|
      |  Collation map name         |                             |
      |-----------------------------|-----------------------------|
      |  Dictionary record          |                             |
      |-----------------------------|-----------------------------|
      |  Field number               |                             |
      |-----------------------------|-----------------------------|
      |  Flags                      |                             |
      |-----------------------------|-----------------------------|
      |  AK name                    |                             |
      |-----------------------------|-----------------------------|
      |  AK directory pathname      |                             |
      |-----------------------------|-----------------------------|
      |  File pathname              |                             |
      |=============================|=============================|
  */

  DESCRIPTOR *descr;
  int16_t fno; /* Field number */
  u_int16_t flags;
  STRING_CHUNK *dict_rec;
  char data_path[MAX_PATHNAME_LEN + 1];
  char ak_path[MAX_PATHNAME_LEN + 1];
  char ak_name[MAX_AK_NAME_LEN + 1];
  char collation_map_name[MAX_ID_LEN + 1];
  char collation_map[256 + 1];

  /* Collation map */

  descr = e_stack - 1;
  k_get_c_string(descr, collation_map, 256);

  /* Collation map name */

  descr = e_stack - 2;
  k_get_c_string(descr, collation_map_name, MAX_ID_LEN);

  /* I-type code from dictionary record */

  descr = e_stack - 3;
  k_get_string(descr);
  dict_rec = descr->data.str.saddr;

  /* Field number */

  descr = e_stack - 4;
  GetInt(descr);
  fno = (int16_t)(descr->data.value);

  /* Flags */

  descr = e_stack - 5;
  GetInt(descr);
  flags = (int16_t)(descr->data.value);

  /* AK name */

  descr = e_stack - 6;
  if (k_get_c_string(descr, ak_name, MAX_AK_NAME_LEN) < 0)
    goto exit_op_createak;

  /* AK directory pathname */

  descr = e_stack - 7;
  if (k_get_c_string(descr, ak_path, MAX_PATHNAME_LEN) < 0) {
    goto exit_op_createak;
  }

  /* Data file pathname */

  descr = e_stack - 8;
  if (k_get_c_string(descr, data_path, MAX_PATHNAME_LEN) < 0) {
    goto exit_op_createak;
  }

  if (create_ak(data_path, ak_path, ak_name, fno, flags, dict_rec, collation_map_name, collation_map) == 0) {
    process.status = dh_err;
    goto exit_op_createak;
  }

  process.status = 0;

exit_op_createak:
  k_dismiss(); /* Collation map */
  k_dismiss(); /* Collation map name */
  k_dismiss(); /* Object code */
  k_pop(2);    /* Field number, flags */
  k_dismiss(); /* AK name */
  k_dismiss(); /* AK directory pathname */
  k_dismiss(); /* File pathname */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = process.status;
}

/* ======================================================================
   op_deleteak()  -  DELETE.AK                                            */

void op_deleteak() {
  /* Stack:

      |=============================|=============================|
      |            BEFORE           |           AFTER             |
      |=============================|=============================|
  top |  AK number (internal)       | Status 0=ok, else error     |
      |-----------------------------|-----------------------------|
      |  Pathname                   |                             |
      |=============================|=============================|

  */

  DESCRIPTOR *descr;
  int16_t akno;
  char pathname[MAX_PATHNAME_LEN + 1];

  process.status = 0;

  /* Get AK number */

  descr = e_stack - 1;
  GetInt(descr);
  akno = (int16_t)(descr->data.value);

  /* Pathname */

  descr = e_stack - 2;
  if (k_get_c_string(descr, pathname, MAX_PATHNAME_LEN) < 0) {
    process.status = DHE_INVA_FILE_NAME;
    goto exit_op_deleteak;
  }

  if (!delete_ak(pathname, akno)) {
    process.status = dh_err;
    goto exit_op_deleteak;
  }

exit_op_deleteak:
  k_pop(1);    /* AK number */
  k_dismiss(); /* Pathname */

  /* Set status code on stack */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = process.status;
}

/* ====================================================================== */

void op_indices1() {
  /* Stack:

      |=============================|=============================|
      |            BEFORE           |           AFTER             |
      |=============================|=============================|
  top |  File variable              |  Returned string            |
      |=============================|=============================|
  */

  DESCRIPTOR *descr;
  FILE_VAR *fvar;
  DH_FILE *dh_file;
  ARRAY_HEADER *ak_data;
  int16_t num_aks;
  int16_t akno;
  STRING_CHUNK *str;
  STRING_CHUNK *tgt = NULL;

  /* Find file variable */

  descr = e_stack - 1;
  k_get_file(descr);
  fvar = descr->data.fvar;
  k_dismiss();

  /* Extract AK data */

  switch (fvar->type) {
    case DYNAMIC_FILE:
      dh_file = fvar->access.dh.dh_file;

      if ((ak_data = dh_file->ak_data) != NULL) {
        ts_init(&tgt, 128);

        num_aks = (int16_t)(ak_data->rows);
        for (akno = 0; akno < num_aks; akno++) {
          descr = Element(ak_data, (akno * AKD_COLS) + AKD_NAME);
          if ((str = descr->data.str.saddr) != NULL) {
            if (tgt != NULL)
              ts_copy_byte(FIELD_MARK);
            ts_copy(str->data, str->bytes); /* Always one chunk */
          }
        }

        (void)ts_terminate();
      }

      InitDescr(e_stack, STRING);
      (e_stack++)->data.str.saddr = tgt;
      break;

    case NET_FILE:
      InitDescr(e_stack, STRING);
      (e_stack++)->data.str.saddr = net_indices1(fvar);
      break;

    default:
      InitDescr(e_stack, STRING);
      (e_stack++)->data.str.saddr = NULL;
      break;
  }
}

/* ====================================================================== */

void op_indices2() {
  /* Stack:

      |=============================|=============================|
      |            BEFORE           |           AFTER             |
      |=============================|=============================|
  top |  Index name                 |  Returned string            |
      |-----------------------------|-----------------------------|
      |  File variable              |                             |
      |=============================|=============================|
  */

  DESCRIPTOR *descr;
  FILE_VAR *fvar;
  DH_FILE *dh_file;
  int16_t akno;
  char index_name[MAX_AK_NAME_LEN + 1];

  /* Find file variable */

  descr = e_stack - 2;
  k_get_file(descr);
  fvar = descr->data.fvar;

  if (fvar->type == DYNAMIC_FILE) {
    dh_file = fvar->access.dh.dh_file;
    akno = find_ak_by_name(e_stack - 1, dh_file);
    k_dismiss();
    k_dismiss();

    if (akno >= 0) {
      /* Extract AK data */

      InitDescr(e_stack, ARRAY); /* AK data */
      (e_stack++)->data.ahdr_addr = dh_file->ak_data;
      (dh_file->ak_data->ref_ct)++;

      InitDescr(e_stack, INTEGER); /* AK number (from 1) */
      (e_stack++)->data.value = akno + 1;

      k_recurse(pcode_indices, 2);
    } else {
      InitDescr(e_stack, STRING);
      (e_stack++)->data.str.saddr = NULL;
    }
  } else {
    /* Find index name */

    descr = e_stack - 1;
    k_get_c_string(descr, index_name, MAX_AK_NAME_LEN);

    k_dismiss();
    k_dismiss();

    switch (fvar->type) {
      case NET_FILE:
        InitDescr(e_stack, STRING);
        (e_stack++)->data.str.saddr = net_indices2(fvar, index_name);
        break;

      default:
        InitDescr(e_stack, STRING);
        (e_stack++)->data.str.saddr = NULL;
        break;
    }
  }
}

/* ====================================================================== */

void op_selindx() {
  /* Stack:

      |=============================|=============================|
      |            BEFORE           |           AFTER             |
      |=============================|=============================|
  top |  Target list number         |                             |
      |-----------------------------|-----------------------------|
      |  File variable              |                             |
      |-----------------------------|-----------------------------|
      |  Index name                 |                             |
      |=============================|=============================|

      STATUS() is set to 1 for errors such as unknown index, 0 for success.
  */

  DESCRIPTOR *descr;
  int16_t list_no;
  FILE_VAR *fvar;
  DH_FILE *dh_file;
  int16_t akno;
  int16_t subfile;
  int16_t lock_slot = 0; /* Group lock table index */
  int32_t record_count = 0;
  char *buff = NULL;
  int32_t node_num;
  int16_t used_bytes;
  int16_t rec_offset;
  DH_RECORD *rec_ptr;
  char index_name[MAX_AK_NAME_LEN + 1];
  DESCRIPTOR *list_descr;
  DESCRIPTOR *count_descr;

  process.status = 1; /* Preset for error paths */
  dh_err = 0;

  /* Get list number */

  descr = e_stack - 1;
  GetInt(descr);
  list_no = (int16_t)(descr->data.value);
  if (InvalidSelectListNo(list_no))
    k_select_range_error();

  if (select_ftype[list_no] != SEL_NONE)
    end_select(list_no);

  clear_select(list_no);

  descr = SelectList(list_no);
  ts_init(&descr->data.str.saddr, MAX_STRING_CHUNK_SIZE);

  /* Get file variable */

  descr = e_stack - 2;
  k_get_file(descr);
  fvar = descr->data.fvar;

  if (fvar->type == NET_FILE) {
    /* Find index name */

    descr = e_stack - 3;
    k_get_c_string(descr, index_name, MAX_AK_NAME_LEN);

    list_descr = SelectList(list_no);
    k_release(list_descr);
    InitDescr(list_descr, STRING);
    list_descr->data.str.saddr = NULL;

    record_count = net_selectindex(fvar, index_name, &(list_descr->data.str.saddr));

    count_descr = SelectCount(list_no);
    k_release(count_descr);
    InitDescr(count_descr, INTEGER);
    count_descr->data.value = record_count;
  } else {
    if (fvar->type != DYNAMIC_FILE)
      goto exit_selindx; /* Not DH file */
    dh_file = fvar->access.dh.dh_file;

    /* Get index name */

    descr = e_stack - 3;
    if ((akno = find_ak_by_name(descr, dh_file)) < 0)
      goto exit_selindx;

    process.status = 0;
    subfile = AK_BASE_SUBFILE + akno;

    /* Allocate a buffer for handling AK blocks */

    buff = (char *)k_alloc(52, DH_AK_NODE_SIZE);
    if (buff == NULL)
      goto exit_selindx;

    /* Lock the AK subfile */

    lock_slot = GetGroupReadLock(dh_file, AKGlock(akno));

    /* Construct the index by walking down to the leftmost terminal node and
       then following the chain to the right, collecting AK key values as
       we go.                                                                 */

    node_num = 1;
    do {
      if (!dh_read_group(dh_file, subfile, node_num, buff, DH_AK_NODE_SIZE)) {
        goto exit_selindx;
      }
      if (((DH_INT_NODE *)buff)->node_type == AK_TERM_NODE)
        break;
      node_num = GetAKFwdLink(dh_file, ((DH_INT_NODE *)buff)->child[0]);
    } while (1);

    do {
      used_bytes = ((DH_TERM_NODE *)buff)->used_bytes;
      rec_offset = TERM_NODE_HEADER_SIZE;
      while (rec_offset < used_bytes) {
        rec_ptr = (DH_RECORD *)(buff + rec_offset);
        if (record_count++)
          ts_copy_byte(FIELD_MARK);
        ts_copy(rec_ptr->id, rec_ptr->id_len);
        rec_offset += rec_ptr->next;
      }

      node_num = GetAKFwdLink(dh_file, ((DH_TERM_NODE *)buff)->right);
      if (node_num == 0)
        break;

      if (!dh_read_group(dh_file, subfile, node_num, buff, DH_AK_NODE_SIZE)) {
        goto exit_selindx;
      }
    } while (1);

    (void)ts_terminate();

    /* Set record count for select list */

    SelectCount(list_no)->data.value = record_count;

  exit_selindx:
    if (lock_slot != 0)
      FreeGroupReadLock(lock_slot);

    if (buff != NULL)
      k_free(buff);
  }

  k_pop(1);    /* Index number */
  k_dismiss(); /* File variable */
  k_dismiss(); /* Index name */

  /* Set selected record count to @SELECTED */

  descr = Element(process.syscom, SYSCOM_SELECTED); /* 0487 */
  k_release(descr);
  InitDescr(descr, INTEGER);
  descr->data.value = record_count;
}

/* ====================================================================== */

void op_selindxv() {
  /* Stack:

      |=============================|=============================|
      |            BEFORE           |           AFTER             |
      |=============================|=============================|
  top |  Target list number         |                             |
      |-----------------------------|-----------------------------|
      |  File variable              |                             |
      |-----------------------------|-----------------------------|
      |  Indexed value              |                             |
      |-----------------------------|-----------------------------|
      |  Index name                 |                             |
      |=============================|=============================|

      STATUS() is set to 1 for errors such as unknown index, 0 for success.
  */

  DESCRIPTOR *descr;
  int16_t list_no;
  FILE_VAR *fvar;
  DH_FILE *dh_file;
  AK_CTRL *ak_ctrl;
  char indexed_value[MAX_KEY_LEN + 1];
  int16_t indexed_value_len;
  int16_t akno;
  int32_t record_count = 0;
  STRING_CHUNK *str;
  char *p;
  char *q;
  int16_t bytes;
  char index_name[MAX_AK_NAME_LEN + 1];
  DESCRIPTOR *list_descr;
  DESCRIPTOR *count_descr;

  process.status = 1; /* Preset for error paths */
  dh_err = 0;

  /* Get list number */

  descr = e_stack - 1;
  GetInt(descr);
  list_no = (int16_t)(descr->data.value);
  if (InvalidSelectListNo(list_no))
    k_select_range_error();

  /* Get file variable */

  descr = e_stack - 2;
  k_get_file(descr);
  fvar = descr->data.fvar;

  if (select_ftype[list_no] != SEL_NONE)
    end_select(list_no);

  clear_select(list_no);

  /* Get indexed value */

  descr = e_stack - 3;
  indexed_value_len = k_get_c_string(descr, indexed_value, MAX_KEY_LEN);
  if (indexed_value_len < 0)
    goto exit_selindxv; /* Not indexable, hence empty */

  if (fvar->type == NET_FILE) {
    /* Find index name */

    descr = e_stack - 4;
    k_get_c_string(descr, index_name, MAX_AK_NAME_LEN);

    list_descr = SelectList(list_no);
    k_release(list_descr);
    InitDescr(list_descr, STRING);
    list_descr->data.str.saddr = NULL;

    record_count = net_selectindexv(fvar, index_name, indexed_value, &(list_descr->data.str.saddr));

    count_descr = SelectCount(list_no);
    k_release(count_descr);
    InitDescr(count_descr, INTEGER);
    count_descr->data.value = record_count;
  } else {
    if (fvar->type != DYNAMIC_FILE)
      goto exit_selindxv; /* Not DH file */
    dh_file = fvar->access.dh.dh_file;

    /* Get index name */

    descr = e_stack - 4;
    if ((akno = find_ak_by_name(descr, dh_file)) < 0)
      goto exit_selindxv;

    process.status = 0;

    str = ak_read(dh_file, akno, indexed_value, indexed_value_len, TRUE);

    SelectList(list_no)->data.str.saddr = str;

    /* Count items */

    if (str != NULL) {
      record_count = 1;
      do {
        bytes = str->bytes;
        p = str->data;
        do {
          q = (char *)memchr(p, FIELD_MARK, bytes);
          if (q == NULL)
            break;
          record_count++;
          bytes -= (q - p) + 1;
          p = q + 1;
        } while (bytes);
      } while ((str = str->next) != NULL);
    }

    /* Save position for scan operations */

    ak_ctrl = fvar->access.dh.ak_ctrl;
    ak_ctrl->ak_scan[akno].upd = ak_upd;
    ak_ctrl->ak_scan[akno].node_num = ak_node_num;
    ak_ctrl->ak_scan[akno].rec_offset = ak_rec_offset;
    ak_ctrl->ak_scan[akno].flags = ak_flags;
    ak_ctrl->ak_scan[akno].key_len = (u_char)indexed_value_len;
    memcpy(ak_ctrl->ak_scan[akno].key, indexed_value, indexed_value_len);

    /* Set record count for select list */

    SelectCount(list_no)->data.value = record_count;
  }

exit_selindxv:
  k_pop(1);    /* Index number */
  k_dismiss(); /* File variable */
  k_dismiss(); /* Indexed value */
  k_dismiss(); /* Index name */

  /* Set selected record count to @SELECTED */

  descr = Element(process.syscom, SYSCOM_SELECTED); /* 0487 */
  k_release(descr);
  InitDescr(descr, INTEGER);
  descr->data.value = record_count;
}

/* ====================================================================== */

void op_selright() {
  akscan(TRUE);
}

void op_selleft() {
  akscan(FALSE);
}

Private void akscan(bool right) {
  /* Stack:

      |=============================|=============================|
      |            BEFORE           |           AFTER             |
      |=============================|=============================|
  top |  Target list number         |                             |
      |-----------------------------|-----------------------------|
      |  ADDR to key var            |                             |
      |-----------------------------|-----------------------------|
      |  File variable              |                             |
      |-----------------------------|-----------------------------|
      |  Index name                 |                             |
      |=============================|=============================|

  */

  DESCRIPTOR *descr;
  DESCRIPTOR *key_descr;
  int16_t list_no;
  FILE_VAR *fvar;
  DH_FILE *dh_file;
  int16_t subfile;
  DH_RECORD *rec_ptr;
  AK_CTRL *ak_ctrl;
  int16_t lock_slot = 0;
  union AKBUFF *node = NULL;
  int16_t akno;
  int32_t record_count = 0;
  STRING_CHUNK *str;
  char *p;
  char *q;
  int16_t bytes;
  int16_t n;
  bool found;
  bool return_key = FALSE; /* Return key value? (SETTING clause) */
  u_char old_flags;
  char akname[MAX_AK_NAME_LEN + 1];

  process.status = 1; /* Preset for error paths */
  dh_err = 0;

  /* Get list number */

  descr = e_stack - 1;
  GetInt(descr);
  list_no = (int16_t)(descr->data.value);
  if (InvalidSelectListNo(list_no))
    k_select_range_error();

  if (select_ftype[list_no] != SEL_NONE)
    end_select(list_no);

  clear_select(list_no);

  /* Get key variable */

  key_descr = e_stack - 2;
  if (key_descr->type != UNASSIGNED) {
    return_key = TRUE;
    while (key_descr->type == ADDR)
      key_descr = key_descr->data.d_addr;
    k_release(key_descr); /* 0098 */
    InitDescr(key_descr, STRING);
    key_descr->data.str.saddr = NULL;
  }

  /* Get file variable */

  descr = e_stack - 3;
  k_get_file(descr);
  fvar = descr->data.fvar;

  if (fvar->type == NET_FILE) {
    descr = e_stack - 4;
    k_get_c_string(descr, akname, MAX_AK_NAME_LEN);
    process.status = net_scanindex(fvar, akname, list_no, (return_key) ? key_descr : NULL, right);
    goto exit_akscan;
  }

  if (fvar->type != DYNAMIC_FILE)
    goto exit_akscan; /* Not DH file */
  dh_file = fvar->access.dh.dh_file;

  /* Get index name */

  descr = e_stack - 4;
  if ((akno = find_ak_by_name(descr, dh_file)) < 0)
    goto exit_akscan;

  subfile = AK_BASE_SUBFILE + akno;

  /* If the AK transaction counter has changed, we must read to the last
     key extracted because the node may have moved.                       */

  ak_ctrl = fvar->access.dh.ak_ctrl;

  /* Lock the AK subfile */

  lock_slot = GetGroupReadLock(dh_file, AKGlock(akno));

  /* If there have been no writes to this AK since the last search, we
     can use the cached position information.  Otherwise we must repeat
     the search for the previous key first.                              */

  old_flags = ak_ctrl->ak_scan[akno].flags;
  if ((old_flags & AKS_FOUND) && (ak_ctrl->ak_scan[akno].upd == FPtr(dh_file->file_id)->ak_upd)) {
    found = ((old_flags & AKS_FOUND) != 0);
    ak_node_num = ak_ctrl->ak_scan[akno].node_num;
    ak_rec_offset = ak_ctrl->ak_scan[akno].rec_offset;
    ak_flags = old_flags;
  } else /* Must re-read */
  {
    found = (ak_read(dh_file, akno, ak_ctrl->ak_scan[akno].key, ak_ctrl->ak_scan[akno].key_len, FALSE) != NULL);
  }

  // 0516   /* If the tree is empty, go no further */
  // 0516  if ((ak_flags & AKS_LEFT) && (ak_flags & AKS_RIGHT)) goto no_record;

  /* 0516 If we are at edge towards which we are scanning, go no further */

  if (ak_flags & ((right) ? AKS_RIGHT : AKS_LEFT)) {
    ak_flags &= ~AKS_FOUND;
    goto no_record;
  }

  if (old_flags & (AKS_RIGHT | AKS_LEFT)) /* Examine old flags */
  {
    found = FALSE; /* Not really found - we located the edge record */
  }

  /* Read the node buffer */

  node = (AKBUFF *)k_alloc(104, sizeof(AKBUFF));
  if (!dh_read_group(dh_file, subfile, ak_node_num, node->buff, DH_AK_NODE_SIZE)) {
    goto exit_akscan;
  }

  rec_ptr = (DH_RECORD *)(node->buff + ak_rec_offset);

  if (found) /* Previous scan position was found */
  {
    /* We found the record last time. To get the next record we need to
       move one step across the tree.                                    */

    if (right) {
      /* Step one record to the right */

      ak_rec_offset += rec_ptr->next;
      if (ak_rec_offset == node->term_node.used_bytes) {
        ak_node_num = GetAKFwdLink(dh_file, node->term_node.right);
        ak_rec_offset = TERM_NODE_HEADER_SIZE;
        if (ak_node_num == 0) /* No more records */
        {
          ak_flags = AKS_RIGHT;
          goto no_record;
        }

        if (!dh_read_group(dh_file, subfile, ak_node_num, node->buff, DH_AK_NODE_SIZE)) {
          goto exit_akscan;
        }
      }
    } else {
      /* Step one record to the left */

      if (ak_rec_offset == TERM_NODE_HEADER_SIZE) /* At first record */
      {
        ak_node_num = GetAKFwdLink(dh_file, node->term_node.left);
        ak_rec_offset = TERM_NODE_HEADER_SIZE;
        if (ak_node_num == 0) /* No more records */
        {
          ak_flags = AKS_LEFT;
          goto no_record;
        }

        if (!dh_read_group(dh_file, subfile, ak_node_num, node->buff, DH_AK_NODE_SIZE)) {
          goto exit_akscan;
        }

        rec_ptr = rightmost(&node->term_node);
        ak_rec_offset = (char *)rec_ptr - node->buff;
      } else {
        p = node->buff + TERM_NODE_HEADER_SIZE;
        while ((p - node->buff + ((DH_RECORD *)p)->next) < ak_rec_offset) {
          p += ((DH_RECORD *)p)->next;
        }
        ak_rec_offset = p - node->buff;
      }
    }
  } else /* Previous scan position was not found */
  {
    /* We did not find the record last time.  Possibilities are:
       Action      Previously         To find next record
       SelectRight AKS_LEFT           Want current record
       SelectRight AKS_RIGHT          Move one position to right
       SelectRight Neither            Want current record
       SelectLeft  AKS_LEFT           Move one position to left
       SelectLeft  AKS_RIGHT          Want current record
       SelectLeft  Neither            Move one position to left
    */
    if (right) /* SelectRight */
    {
      if (old_flags & AKS_RIGHT) {
        /* Step one record to the right */

        ak_rec_offset += rec_ptr->next;
        if (ak_rec_offset == node->term_node.used_bytes) {
          ak_node_num = GetAKFwdLink(dh_file, node->term_node.right);
          ak_rec_offset = TERM_NODE_HEADER_SIZE;
          if (ak_node_num == 0) /* No more records */
          {
            ak_flags = AKS_RIGHT;
            goto no_record;
          }

          if (!dh_read_group(dh_file, subfile, ak_node_num, node->buff, DH_AK_NODE_SIZE)) {
            goto exit_akscan;
          }
        }
      }
    } else /* SelectLeft */
    {
      if (!(old_flags & AKS_RIGHT)) {
        /* Step one record to the left */

        if (ak_rec_offset == TERM_NODE_HEADER_SIZE) /* At first record */
        {
          ak_node_num = GetAKFwdLink(dh_file, node->term_node.left);
          ak_rec_offset = TERM_NODE_HEADER_SIZE;
          if (ak_node_num == 0) /* No more records */
          {
            ak_flags |= AKS_LEFT;
            goto no_record;
          }

          if (!dh_read_group(dh_file, subfile, ak_node_num, node->buff, DH_AK_NODE_SIZE)) {
            goto exit_akscan;
          }

          rec_ptr = rightmost(&node->term_node);
          ak_rec_offset = (char *)rec_ptr - node->buff;
        } else {
          p = node->buff + TERM_NODE_HEADER_SIZE;
          while ((p - node->buff + ((DH_RECORD *)p)->next) < ak_rec_offset) {
            p += ((DH_RECORD *)p)->next;
          }
          ak_rec_offset = p - node->buff;
        }
      } else /* 0523 Moving left from rightmost unfound record */
      {
        if (!dh_read_group(dh_file, subfile, ak_node_num, node->buff, DH_AK_NODE_SIZE)) {
          goto exit_akscan;
        }

        if (ak_rec_offset == node->term_node.used_bytes) {
          rec_ptr = rightmost(&node->term_node);
          ak_rec_offset = (char *)rec_ptr - node->buff;
        }
      }
    }
  }

  rec_ptr = (DH_RECORD *)(node->buff + ak_rec_offset);

  /* Save the key value and position data */

  ak_ctrl->ak_scan[akno].key_len = rec_ptr->id_len;
  memcpy(ak_ctrl->ak_scan[akno].key, rec_ptr->id, rec_ptr->id_len);
  ak_ctrl->ak_scan[akno].node_num = ak_node_num;
  ak_ctrl->ak_scan[akno].rec_offset = ak_rec_offset;
  ak_flags = AKS_FOUND;

  /* Return key value to caller */

  if (return_key && (rec_ptr->id_len != 0)) {
    n = rec_ptr->id_len;
    str = s_alloc(n, &bytes);
    key_descr->data.str.saddr = str;
    memcpy(str->data, rec_ptr->id, n);
    str->bytes = n;
    str->string_len = n;
    str->ref_ct = 1;
  }

  /* Extract the data from this record into the target select list */

  str = ak_read_record(dh_file, subfile, rec_ptr);
  SelectList(list_no)->data.str.saddr = str;

  /* Count items */

  if (str == NULL) {
    record_count = 1; /* Null id only */
  } else {
    record_count = 1;
    do {
      bytes = str->bytes;
      p = str->data;
      do {
        q = (char *)memchr(p, FIELD_MARK, bytes);
        if (q == NULL)
          break;
        record_count++;
        bytes -= (q - p) + 1;
        p = q + 1;
      } while (bytes);
    } while ((str = str->next) != NULL);
  }

  /* Set record count for select list */

  SelectCount(list_no)->data.value = record_count;

  process.status = 0;

no_record:
  ak_ctrl->ak_scan[akno].flags = ak_flags;

exit_akscan:
  if (lock_slot != 0)
    FreeGroupReadLock(lock_slot);

  k_pop(1);    /* Index number */
  k_dismiss(); /* Key variable */
  k_dismiss(); /* File variable */
  k_dismiss(); /* Index name */

  /* Set selected record count to @SELECTED */

  descr = Element(process.syscom, SYSCOM_SELECTED); /* 0487 */
  k_release(descr);
  InitDescr(descr, INTEGER);
  descr->data.value = record_count;

  if (node != NULL)
    k_free(node);
}

/* ====================================================================== */

void op_setright() {
  setakpos(TRUE);
}

void op_setleft() {
  setakpos(FALSE);
}

Private void setakpos(bool right) {
  /* Stack:

      |=============================|=============================|
      |            BEFORE           |           AFTER             |
      |=============================|=============================|
  top |  File variable              |                             |
      |-----------------------------|-----------------------------|
      |  Index name                 |                             |
      |=============================|=============================|

  */

  DESCRIPTOR *descr;
  FILE_VAR *fvar;
  DH_FILE *dh_file;
  AK_CTRL *ak_ctrl;
  int16_t akno;
  char *buff = NULL;
  int16_t lock_slot = 0; /* Group lock table index */
  int16_t subfile;
  int32_t node_num;
  int16_t rec_offset;
  DH_RECORD *rec_ptr;
  DH_INT_NODE *node_ptr;
  char akname[MAX_AK_NAME_LEN + 1];

  process.status = 1; /* Preset for error paths */
  dh_err = 0;

  /* Get file variable */

  descr = e_stack - 1;
  k_get_file(descr);
  fvar = descr->data.fvar;

  if (fvar->type == NET_FILE) {
    descr = e_stack - 2;
    k_get_c_string(descr, akname, MAX_AK_NAME_LEN);
    process.status = net_setindex(fvar, akname, right);
    goto exit_setakpos;
  }

  if (fvar->type != DYNAMIC_FILE)
    goto exit_setakpos; /* Not DH file */
  dh_file = fvar->access.dh.dh_file;
  ak_ctrl = fvar->access.dh.ak_ctrl;

  /* Get index name */

  descr = e_stack - 2;
  if ((akno = find_ak_by_name(descr, dh_file)) < 0)
    goto exit_setakpos;
  subfile = AK_BASE_SUBFILE + akno;

  /* Allocate a buffer for handling AK blocks */

  buff = (char *)k_alloc(73, DH_AK_NODE_SIZE);
  if (buff == NULL)
    goto exit_setakpos;
  node_ptr = ((DH_INT_NODE *)buff);

  /* Lock the AK subfile */

  lock_slot = GetGroupReadLock(dh_file, AKGlock(akno));

  /* Find key at this edge and record it as the last key processed to
     allow us to walk further when new data is added.                   */

  ak_ctrl->ak_scan[akno].key_len = 0; /* For empty tree */

  /* Walk down appropriate edge of tree */

  node_num = 1;
  do {
    if (!dh_read_group(dh_file, subfile, node_num, buff, DH_AK_NODE_SIZE)) {
      goto exit_setakpos;
    }

    if (node_ptr->node_type == AK_TERM_NODE)
      break;

    if (right)
      node_num = GetAKFwdLink(dh_file, node_ptr->child[node_ptr->child_count - 1]);
    else
      node_num = GetAKFwdLink(dh_file, node_ptr->child[0]);
  } while (1);

  if ((node_num == 1) && (node_ptr->used_bytes == TERM_NODE_HEADER_SIZE)) {
    /* Tree is empty */
    ak_ctrl->ak_scan[akno].key_len = 0;
    ak_ctrl->ak_scan[akno].flags = AKS_RIGHT | AKS_LEFT;
    goto exit_setakpos_ok;
  }

  if (right) {
    rec_ptr = rightmost((DH_TERM_NODE *)buff);
    rec_offset = (char *)rec_ptr - buff;
  } else {
    rec_offset = TERM_NODE_HEADER_SIZE;
    if (rec_offset < node_ptr->used_bytes) {
      rec_ptr = (DH_RECORD *)(buff + rec_offset);
    } else
      rec_ptr = NULL;
  }

  if (rec_ptr != NULL) {
    ak_ctrl->ak_scan[akno].node_num = node_num;
    ak_ctrl->ak_scan[akno].rec_offset = rec_offset;
    ak_ctrl->ak_scan[akno].key_len = rec_ptr->id_len;
    memcpy(ak_ctrl->ak_scan[akno].key, rec_ptr->id, rec_ptr->id_len);
    ak_ctrl->ak_scan[akno].upd = FPtr(dh_file->file_id)->ak_upd;
  }

  ak_ctrl->ak_scan[akno].flags = (right) ? AKS_RIGHT : AKS_LEFT;

exit_setakpos_ok:
  process.status = 0;

exit_setakpos:
  if (lock_slot != 0)
    FreeGroupReadLock(lock_slot);

  if (buff != NULL)
    k_free(buff);

  k_dismiss(); /* File variable */
  k_dismiss(); /* Index name */
}

/* ======================================================================
   create_ak() - Create an AK subfile                                     */

Private int16_t create_ak(char *data_path,          /* Data file path name */
                          char *ak_path,            /* AK directory path, null if to use data path */
                          char *ak_name,            /* AK field name */
                          int16_t fno,              /* Field number, -ve if I-type index */
                          u_int16_t flags,          /* AK flags */
                          STRING_CHUNK *dict_rec,   /* Dictionary record */
                          char *collation_map_name, /* Collation map name (null if none)... */
                          char *collation_map)      /* ...and the actual map (null if none) */
{
  int16_t subfile = 0;
  char path[MAX_PATHNAME_LEN + 1];
  OSFILE fu = INVALID_FILE_HANDLE;
  OSFILE akfu = INVALID_FILE_HANDLE;
  DH_HEADER header;
  DH_AK_HEADER ak_header;
  u_int32_t ak_map;
  char *buff = NULL;
  int16_t i;
  STRING_CHUNK *str;
  int32_t dict_rec_len;
  bool big_dict_rec = FALSE;
  int32_t node_num;
  int16_t bytes;
  int16_t n;
  int16_t x;
  char *p;
  char *q;
  int ak_header_size;
  u_char *obj;

  /* Open primary subfile.  We do this outside of the file sharing mechanism
     to guarantee that we are the only user of the file.                     */

// 17Oct25 mab Change dyn file prefix to %
  sprintf(path, "%s%c%%0", data_path, DS);
  fu = dio_open(path, DIO_UPDATE);
  if (!ValidFileHandle(fu)) {
    dh_err = DHE_FILE_NOT_FOUND;
    process.os_error = OSError;
    goto exit_create_ak;
  }

  /* Read the file header */

  if (Read(fu, (char *)&header, DH_HEADER_SIZE) < 0) {
    dh_err = DHE_READ_ERROR;
    process.os_error = OSError;
    goto exit_create_ak;
  }

  ak_header_size = AKHeaderSize(header.file_version);

  /* Search map for a spare subfile */

  for (i = 0, ak_map = header.ak_map; i < MAX_INDICES; i++, ak_map = ak_map >> 1) {
    if ((ak_map & 1) == 0) {
      header.ak_map |= 1 << i; /* Update ready to write back */
      subfile = i + 2;
      break;
    }
  }

  if (subfile == 0) {
    dh_err = DHE_AK_TOO_MANY;
    goto exit_create_ak;
  }

  /* Allocate a buffer for creation of AK blocks */

  buff = (char *)k_alloc(47, DH_MAX_GROUP_SIZE_BYTES);
  if (buff == NULL) {
    dh_err = DHE_NO_MEM;
    goto exit_create_ak;
  }

  /* Create an empty AK subfile */

  memset((char *)&ak_header, 0, DH_AK_HEADER_SIZE);
  ak_header.magic = DH_INDEX;
  ak_header.flags = flags;
  ak_header.fno = fno;
  strcpy((char *)ak_header.ak_name, ak_name);
  ak_header.data_creation_timestamp = header.creation_timestamp;

  if (ak_path[0] == '\0') /* Not relocated */
  {
    strcpy(ak_path, data_path); /* Use data path for AKs */
  } else                        /* Relocated */
  {
    strcpy(header.akpath, ak_path); /* Only relevant on first index */
  }

// 17Oct25 mab Change dyn file prefix to %
  sprintf(path, "%s%c%%%d", ak_path, DS, subfile);
  akfu = dio_open(path, DIO_NEW);
  if (!ValidFileHandle(akfu)) {
    dh_err = DHE_AK_CREATE_ERR;
    process.os_error = OSError;
    subfile = 0;
    goto exit_create_ak;
  }

  dict_rec_len = dict_rec->string_len;
  ak_header.itype_len = dict_rec_len;
  big_dict_rec = (dict_rec_len > AK_CODE_BYTES);
  if (big_dict_rec) {
    ak_header.itype_ptr = ak_header_size + DH_AK_NODE_SIZE;
  } else /* Short enough to store in AK header */
  {
    for (obj = ak_header.itype, str = dict_rec; str != NULL; str = str->next) {
      memcpy(obj, str->data, str->bytes);
      obj += str->bytes;
    }
  }

  /* Set collation map */

  if (collation_map_name[0] != '\0') {
    strcpy(ak_header.collation_map_name, collation_map_name);
    memcpy(ak_header.collation_map, collation_map, 256);
  }

  /* Write AK header */

  memset(buff, 0, ak_header_size);
  memcpy(buff, (char *)(&ak_header), DH_AK_HEADER_SIZE);

  if (Write(akfu, buff, ak_header_size) < 0) {
    dh_err = DHE_AK_HDR_WRITE_ERROR;
    process.os_error = OSError;
    subfile = 0;
    goto exit_create_ak;
  }

  /* Write empty terminal node */

  memset(buff, 0, DH_AK_NODE_SIZE);
  ((DH_TERM_NODE *)buff)->used_bytes = TERM_NODE_HEADER_SIZE;
  ((DH_TERM_NODE *)buff)->node_type = AK_TERM_NODE;

  if (Write(akfu, buff, DH_AK_NODE_SIZE) < 0) {
    dh_err = DHE_AK_WRITE_ERROR;
    process.os_error = OSError;
    subfile = 0;
    goto exit_create_ak;
  }

  /* If we have a "big" I-type, we must store it in a chain of nodes
     immediately after the root node. Although this chain could technically
     go anywhere, the akclear opcode relies on this being contiguous after
     the header.                                                            */

  if (big_dict_rec) {
    node_num = 2;
    str = dict_rec;
    bytes = str->bytes;
    q = str->data;

    do {
      memset(buff, '\0', DH_AK_NODE_SIZE);
      ((DH_ITYPE_NODE *)buff)->node_type = AK_ITYPE_NODE;

      /* Calculate number of bytes to write into this node */

      n = (int16_t)min(dict_rec_len, DH_AK_NODE_SIZE - DH_ITYPE_NODE_DATA_OFFSET);
      ((DH_ITYPE_NODE *)buff)->used_bytes = n + DH_ITYPE_NODE_DATA_OFFSET;

      p = (char *)(((DH_ITYPE_NODE *)buff)->data);
      dict_rec_len -= n;
      while (n != 0) {
        x = min(n, bytes);
        memcpy(p, q, x);
        p += x;
        q += x;
        n -= x;
        bytes -= x;
        if (bytes == 0) {
          if ((str = str->next) != NULL) {
            bytes = str->bytes;
            q = str->data;
          }
        }
      }

      if (dict_rec_len) /* More */
      {
        node_num++;
        if (header.file_version < 2) {
          ((DH_ITYPE_NODE *)buff)->next = ((node_num - 1) * DH_AK_NODE_SIZE + ak_header_size);
        } else {
          ((DH_ITYPE_NODE *)buff)->next = node_num;
        }
      }

      if (Write(akfu, buff, DH_AK_NODE_SIZE) < 0) {
        dh_err = DHE_WRITE_ERROR;
        process.os_error = OSError;
        goto exit_create_ak;
      }
    } while (dict_rec_len != 0);
  }

  /* Write modified primary header */

  if (!write_at(fu, (int64)0, (char *)&header, DH_HEADER_SIZE)) {
    goto exit_create_ak;
  }

exit_create_ak:
  if (ValidFileHandle(fu))
    CloseFile(fu);
  if (ValidFileHandle(akfu))
    CloseFile(akfu);

  if (buff != NULL)
    k_free(buff);

  return subfile;
}

/* ======================================================================
   delete_ak() - Delete an AK subfile                                     */

Private bool delete_ak(char *pathname, /* File path name */
                       int16_t akno)   /* AK number */
{
  char path[MAX_PATHNAME_LEN + 1 + 256]; /* git issue #82 */
  bool relocated;
  OSFILE fu = INVALID_FILE_HANDLE;
  DH_HEADER header;

  /* Open primary subfile.  We do this outside of the file sharing mechanism
     to guarantee that we are the only user of the file.                     */
	 
//  17Oct25 mab Change dyn file prefix to %
  sprintf(path, "%s%c%%0", pathname, DS);
  fu = dio_open(path, DIO_UPDATE);
  if (!ValidFileHandle(fu)) {
    dh_err = DHE_FILE_NOT_FOUND;
    process.os_error = OSError;
    goto exit_delete_ak;
  }

  /* Read the file header */

  if (!read_at(fu, (int64)0, (char *)&header, DH_HEADER_SIZE)) {
    goto exit_delete_ak;
  }

  /* Check this AK exists */
  /* the (1 << akno) code in the if statement below is triggering a warning
   * "warning: ‘<<’ in boolean context, did you mean ‘<’ ? [-Wint-in-bool-context]"
   * Based on how it's used later (header.ak_map ^= (1 << akno)) in the code, it's
   * my belief that the code is correct as written.  As such, I'm going to turn off
   * the warning for this single instance.  -gwb 22Feb20
   */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-in-bool-context"
  if ((header.ak_map && (1 << akno)) == 0) {
#pragma GCC diagnostic pop
    dh_err = DHE_NO_SUCH_AK;
    goto exit_delete_ak;
  }

  /* Clear AK map flag */

  header.ak_map ^= (1 << akno);

  /* Does this file use relocated indices? */

  relocated = (header.akpath[0] != '\0');

  /* Delete the AK subfile */
  
// 17Oct25 mab Change dyn file prefix to %
  sprintf(path, "%s%c%%%d", (relocated) ? header.akpath : pathname, DS, (int)(akno + AK_BASE_SUBFILE));
  if (remove(path) != 0) {
    dh_err = DHE_AK_DELETE_ERROR;
    process.os_error = OSError;
    goto exit_delete_ak;
  }

  /* If we have just deleted the final index in a file that uses relocated
     indices, we need to clear the pathname in the data file header and
     delete the index directory.                                            */

  if (relocated && (header.ak_map == 0)) {
    delete_path(header.akpath);
    header.akpath[0] = '\0';
  }

  /* Write primary subfile header */

  if (!write_at(fu, (int64)0, (char *)&header, DH_HEADER_SIZE)) {
    goto exit_delete_ak;
  }

exit_delete_ak:
  if (ValidFileHandle(fu))
    CloseFile(fu);

  return (dh_err == 0);
}

/* ======================================================================
   ak_write()  -  Write a record to an AK subfile

   Unlike a DH file, an AK subfile must accept a null record id.          */

Private bool ak_write(DH_FILE *dh_file, /* File descriptor */
                      int16_t akno,     /* AK index number */
                      char id[],        /* Record id... */
                      int16_t id_len,   /* ...and length */
                      STRING_CHUNK *rec) {
  FILE_ENTRY *fptr;        /* File table entry pointer */
  int32_t data_len;        /* Data length */
  bool big_rec;            /* Stored as big record? */
  int16_t subfile;         /* Subfile number */
  int16_t lock_slot = 0;   /* Group lock table index */
  int16_t header_lock = 0; /* File header group lock table index */
  int32_t node_num;
  int16_t flags;    /* AK flags from ak.data matrix */
  bool rj;          /* Right justified? */
  bool nocase;      /* Case insensitive? */
  int16_t child_ct; /* Child node count */
  int16_t ci;       /* Child index for node scan */
  bool status = FALSE;
  int16_t rec_offset; /* Offset of current record... */
  DH_RECORD *rec_ptr; /* ...its DH_RECORD pointer... */
  int16_t rec_size;   /* ...and its size */
  NODE *node_ptr;
  bool found;
  int32_t new_big_rec_head = 0;
  int32_t base_size; /* Bytes required excluding big_rec chain */
  int16_t pad_bytes; /* Padding bytes to bring to multiple of 4 bytes */
  char *key;         /* Key being examined in tree scan... */
  int16_t key_len;   /* ...and its length */
  int16_t used_bytes;
  int32_t old_big_rec_head = 0; /* Head of old big record chain */
  int16_t gap_required;
  int16_t next_offset; /* Offset of next record after insert/replace */
  DH_TERM_NODE *new_node1 = NULL;
  DH_TERM_NODE *new_node2 = NULL;
  DH_TERM_NODE *new_node3 = NULL;
  int32_t new_node1_num;
  int32_t new_node2_num;
  int32_t new_node3_num;
  DH_TERM_NODE *link_node = NULL;
  DH_TERM_NODE *link_node_ptr;
  char *key1;
  char *key2;
  char *key3;
  int16_t key1_len;
  int16_t key2_len;
  int16_t key3_len;
  int16_t x;
  char *p;
  int n;

  tail = NULL;

  /* Get basic information */

  fptr = FPtr(dh_file->file_id);
  subfile = AK_BASE_SUBFILE + akno;
  flags = (int16_t)(AKData(dh_file, akno, AKD_FLGS)->data.value);
  rj = (flags & AK_RIGHT) != 0;
  nocase = (flags & AK_NOCASE) != 0;
  fptr->stats.ak_writes++;
  sysseg->global_stats.ak_writes++;

  /* Examine record to determine space requirements */

  data_len = (rec == NULL) ? 0 : rec->string_len;
  base_size = RECORD_HEADER_SIZE + id_len;

  /* Lock the AK subfile */

  lock_slot = GetGroupWriteLock(dh_file, AKGlock(akno));
  (fptr->ak_upd)++;

  /* Write big rec data if this is a big record */

  big_rec = ((base_size + data_len) >= AK_BIG_REC_SIZE);
  if (big_rec) {
    new_big_rec_head = write_ak_big_rec(dh_file, subfile, rec, data_len);
    if (new_big_rec_head == 0)
      goto exit_ak_write;
  }

  /* Calculate non-big-rec space required for this record */

  if (!big_rec)
    base_size += data_len;
  pad_bytes = (int16_t)((4 - (base_size & 3)) & 3);
  base_size += pad_bytes; /* Round to four byte boundary */

  /* Find the position for this record.
     Each internal node traversed on the way to the terminal node is
     retained along with the terminal node in the node chain.          */

  node_num = 1;

  do {
    /* Allocate a new buffer area */

    node_ptr = (NODE *)k_alloc(43, sizeof(struct NODE));
    node_ptr->prev = tail;
    tail = node_ptr;
    tail->node_num = node_num;

    /* Read the node buffer */

    if (!dh_read_group(dh_file, subfile, node_num, tail->node.buff, DH_AK_NODE_SIZE)) {
      goto exit_ak_write;
    }

    if (tail->node.term_node.node_type == AK_TERM_NODE)
      break;

    if (tail->node.int_node.node_type != AK_INT_NODE) {
      log_printf(
          "DH_AK: Invalid node type (x%04X) at subfile %d node %d\nof file "
          "%s.\n",
          (int)(tail->node.int_node.node_type), (int)subfile, node_num, fptr->pathname);
      dh_err = DHE_AK_NODE_ERROR;
      goto exit_ak_write;
    }

    /* Search internal node for first key greater than or equal to the
       key we are looking for.  This search follows the normal rules for
       numeric/character sequencing.                                     */

    child_ct = tail->node.int_node.child_count;
    found = FALSE;
    for (ci = 0, key = tail->node.int_node.keys; ci < child_ct; ci++, key += key_len) {
      key_len = tail->node.int_node.key_len[ci];
      if (compare(id, id_len, key, key_len, rj, nocase) <= 0) {
        found = TRUE;
        break;
      }
    }

    if (!found) /* New key is to right of existing values */
    {
      /* This can only happen in the root node as we would otherwise be
         processing a node for keys up to the desired value.
         Walk down to rightmost terminal node and add the record in the
         usual way.                                                     */

      ci = child_ct - 1;
      key -= tail->node.int_node.key_len[ci];
    }

    /* Move down into child node */

    tail->ci = ci;
    tail->key_offset = key - tail->node.buff;
    node_num = GetAKFwdLink(dh_file, tail->node.int_node.child[ci]);
  } while (1);

  /* We have found the terminal node that should contain this key.
     Scan group buffer for record.                                  */

  used_bytes = tail->node.term_node.used_bytes;

  if ((used_bytes == 0) || (used_bytes > DH_AK_NODE_SIZE)) {
    log_printf(
        "AK_WRITE: Invalid byte count (x%04X) at subfile %d node %d\nof file "
        "%s.\n",
        used_bytes, (int)subfile, node_num, fptr->pathname);
    dh_err = DHE_POINTER_ERROR;
    goto exit_ak_write;
  }

  rec_offset = TERM_NODE_HEADER_SIZE;
  found = FALSE;
  while (rec_offset < used_bytes) {
    rec_ptr = (DH_RECORD *)(((char *)&(tail->node.term_node)) + rec_offset);
    rec_size = rec_ptr->next;

    x = compare(id, id_len, (rec_ptr->id), rec_ptr->id_len, rj, nocase);
    if (x <= 0) /* Record goes here */
    {
      if (x == 0) /* Found key  -  Replace record */
      {
        if (rec_ptr->flags & DH_BIG_REC) /* Old record is big */
        {
          old_big_rec_head = GetAKFwdLink(dh_file, rec_ptr->data.big_rec);
        }

        gap_required = (int16_t)(base_size - rec_size); /* Adjust space to overwrite */
        next_offset = rec_offset + rec_size;
      } else /* Gone past key value */
      {
        gap_required = (int16_t)base_size; /* Open a gap to hold this record */
        next_offset = rec_offset;
      }

      found = TRUE; /* Found position for record */
      break;
    }

    rec_offset += rec_ptr->next;
  }

  if (!found) {
    /* Record not found and is to right of final record. This can only
       happen when we have followed the chain down to the rightmost node
       at each step.
       Add the new record to the right and adjust the key in the parent
       internal nodes (if any).                                          */

    rec_offset = used_bytes;
    next_offset = used_bytes; /* Just to keep everything happy later */
    rec_ptr = (DH_RECORD *)(((char *)&(tail->node.term_node)) + rec_offset);
    gap_required = (int16_t)base_size;
  }

  /* Adjust the gap size as appropriate */

  if (gap_required <= 0) {
    /* We are replacing an existing record with one that is of the same
       size or smaller.  This is easy as it cannot cause a split.       */

    if ((gap_required != 0)                      /* Not an exact fit */
        && (rec_offset + rec_size < used_bytes)) /* There is something after */
    {
      p = (char *)rec_ptr + base_size;
      n = used_bytes - (rec_size + rec_offset);
      memmove(p, p - gap_required, n);

      /* Clear the new slack space */

      memset(p + n, '\0', -gap_required);
    }

    used_bytes += gap_required;
    tail->node.term_node.used_bytes = used_bytes;

    /* Add the new record at rec_ptr */

    copy_ak_record(dh_file, rec_ptr, (int16_t)base_size, id, id_len, new_big_rec_head, data_len, rec, pad_bytes);

    /* Rewrite this terminal node */

    if (!dh_write_group(dh_file, subfile, node_num, tail->node.buff, DH_AK_NODE_SIZE)) {
      goto exit_ak_write;
    }

    goto ak_written; /* All done */
  }

  /* Open a gap for a new or larger record */

  if (used_bytes + gap_required <= DH_AK_NODE_SIZE) {
    /* Record will fit without splitting terminal node */

    if (rec_offset < used_bytes) /* Something to move */
    {
      p = (char *)rec_ptr;
      n = used_bytes - rec_offset;
      memmove(p + gap_required, p, n);
    } else {
      /* We have added a new rightmost record.
         Walk back up tree changing the rightmost key value.    */

      if (tail->prev != NULL) /* Not updating the root terminal node */
      {
        if (!update_internal_node(dh_file, subfile, tail->prev, /* Node structure */
                                  id, id_len, node_num,         /* Child 1 */
                                  NULL, 0, 0,                   /* Child 2 absent */
                                  NULL, 0, 0))                  /* Child 3 absent */
        {
          goto exit_ak_write;
        }
      }
    }

    used_bytes += gap_required;
    tail->node.term_node.used_bytes = used_bytes;

    /* Add the new record at rec_ptr */

    copy_ak_record(dh_file, rec_ptr, (int16_t)base_size, id, id_len, new_big_rec_head, data_len, rec, pad_bytes);

    /* Rewrite this terminal node */

    if (!dh_write_group(dh_file, subfile, node_num, tail->node.buff, DH_AK_NODE_SIZE)) {
      goto exit_ak_write;
    }

    goto ak_written; /* All done */
  }

  /* The new record will not fit in the terminal node so we must split it.
     At the least, this requires two new node buffers.  We may need three
     because the new record may be too long to fit in the same node as the
     data on either side of it.                                            */

  /* We are splitting node MS to insert record P.  Nodes AL and TZ might not
     exist as MS could be at one (or both) edges of the tree.

            ----      ------      ----
            AL |  ->  | MS |  ->  | TZ
               |      |    |      |
               |  <-  |    |  <-  |
            ----      ------      ----
  */

  /* Create leftmost new node */

  new_node1_num = get_ak_node(dh_file, subfile);
  new_node1 = (DH_TERM_NODE *)k_alloc(50, DH_AK_NODE_SIZE);
  memset((char *)new_node1, '\0', DH_AK_NODE_SIZE);
  new_node1->node_type = AK_TERM_NODE;

  /* We now have a new (empty) node as node 1.

            ----      ------      ----
            AL |  ->  | MS |  ->  | TZ
               |      |    |      |
               |  <-  |    |  <-  |
            ----      ------      ----
                ------
                |    |
                |    |
                | 1  |
                ------
  */

  /* Create new node to right */

  new_node2_num = get_ak_node(dh_file, subfile);
  new_node2 = (DH_TERM_NODE *)k_alloc(50, DH_AK_NODE_SIZE);
  memset((char *)new_node2, '\0', DH_AK_NODE_SIZE);
  new_node2->node_type = AK_TERM_NODE;
  new_node2->used_bytes = TERM_NODE_HEADER_SIZE;

  /* We now have another new (empty) node as node 2.

            ----      ------      ----
            AL |  ->  | MS |  ->  | TZ
               |      |    |      |
               |  <-  |    |  <-  |
            ----      ------      ----
                ------      ------
                |    |      |    |
                |    |      |    |
                | 1  |      | 2  |
                ------      ------
  */

  /* Link terminal nodes */

  new_node1->left = tail->node.term_node.left;
  new_node1->right = SetAKFwdLink(dh_file, new_node2_num);
  new_node2->left = SetAKFwdLink(dh_file, new_node1_num);
  new_node2->right = tail->node.term_node.right;

  /* The tail pointer addresses the node we are splitting (MS). So now we
     have the new nodes linked back to the old AL and TZ nodes though the
     links from AL and TZ still point to the node we are splitting.

            ----      ------      ----
            AL |  ->  | MS |  ->  | TZ
               |      |    |      |
               |  <-  |    |  <-  |
            ----      ------      ----
                ------      ------
            AL<-|    |  ->  |    |->TZ
                |    |      |    |
                | 1  |  <-  | 2  |
                ------      ------
  */

  /* Populate leftmost new node by copying all before insertion point */

  new_node1->used_bytes = rec_offset;

  memcpy((char *)&(new_node1->record), (char *)&(tail->node.term_node.record), rec_offset - TERM_NODE_HEADER_SIZE);

  /*
            ----      ------      ----
            AL |  ->  | MS |  ->  | TZ
               |      |    |      |
               |  <-  |    |  <-  |
            ----      ------      ----
                ------      ------
            AL<-| MO |  ->  |    |->TZ
                |    |      |    |
                | 1  |  <-  | 2  |
                ------      ------
  */

  /* Check if new record will fit in this node */

  if (new_node1->used_bytes + base_size <= DH_AK_NODE_SIZE) /* Yes */
  {
    rec_ptr = (DH_RECORD *)(((char *)new_node1) + rec_offset);
    copy_ak_record(dh_file, rec_ptr, (int16_t)base_size, id, id_len, new_big_rec_head, data_len, rec, pad_bytes);
    new_node1->used_bytes += (int16_t)base_size;
  } else /* No. Must put new record in right node */
  {
    rec_ptr = &(new_node2->record);
    copy_ak_record(dh_file, rec_ptr, (int16_t)base_size, id, id_len, new_big_rec_head, data_len, rec, pad_bytes);
    new_node2->used_bytes += (int16_t)base_size;
  }

  /* Record P will be in the MO node if it will fit. Here we show it in
     the new node 2 because it would not fit in new node 1.
            ----      ------      ----
            AL |  ->  | MS |  ->  | TZ
               |      |    |      |
               |  <-  |    |  <-  |
            ----      ------      ----
                ------      ------
            AL<-| MO |  ->  | P  |->TZ
                |    |      |    |
                | 1  |  <-  | 2  |
                ------      ------
  */

  /* Now move as much as possible of the remaining data from the original
     node into the right node. At this point, rec_offset, rec_size and
     used_bytes still reference the original node buffer.                 */

  rec_offset = next_offset;
  while (rec_offset < used_bytes) {
    rec_ptr = (DH_RECORD *)(((char *)&(tail->node.term_node)) + rec_offset);
    rec_size = rec_ptr->next;

    if (new_node2->used_bytes + rec_size > DH_AK_NODE_SIZE)
      break;

    memcpy(((char *)new_node2) + new_node2->used_bytes, (char *)rec_ptr, rec_size);
    new_node2->used_bytes += rec_size;

    rec_offset += rec_ptr->next;
  }

  /* Here we show node 2 as holding all the remaining data from the old MS
     node. This is the most likely situation but we might find that not all
     of it will fit.
            ----      ------      ----
            AL |  ->  | MS |  ->  | TZ
               |      |    |      |
               |  <-  |    |  <-  |
            ----      ------      ----
                ------      ------
            AL<-| MO |  ->  | PS |->TZ
                |    |      |    |
                | 1  |  <-  | 2  |
                ------      ------
  */

  /* If there is anything left we need to create a third node buffer. The
     one we have so far considered as the right node becomes the centre
     node and we add a new right node.                                    */

  if (rec_offset < used_bytes) {
    new_node3_num = get_ak_node(dh_file, subfile);
    new_node3 = (DH_TERM_NODE *)k_alloc(50, DH_AK_NODE_SIZE);
    memset((char *)new_node3, '\0', DH_AK_NODE_SIZE);
    new_node3->node_type = AK_TERM_NODE;
    new_node3->used_bytes = TERM_NODE_HEADER_SIZE;
    new_node3->right = new_node2->right;
    new_node2->right = SetAKFwdLink(dh_file, new_node3_num);
    new_node3->left = SetAKFwdLink(dh_file, new_node2_num);

    /* We can guarantee that all the data fits so simply copy all that is
       left in one go.                                                    */

    memcpy(((char *)new_node3) + new_node3->used_bytes, ((char *)&(tail->node.term_node)) + rec_offset, used_bytes - rec_offset);
    new_node3->used_bytes += used_bytes - rec_offset;
  }

  /* If we needed to create a third node, the situation is getting a bit
     more complex.
            ----      ------      ----
            AL |  ->  | MS |  ->  | TZ
               |      |    |      |
               |  <-  |    |  <-  |
            ----      ------      ----
                ------      ------      ------
            AL<-| MO |  ->  | PQ |  ->  | RS |->TZ
                |    |      |    |      |    |
                | 1  |  <-  | 2  |  <-  | 3  |
                ------      ------      ------
  */

  /* Now write the two (or perhaps three) new nodes */

  if (!dh_write_group(dh_file, subfile, new_node1_num, (char *)new_node1, DH_AK_NODE_SIZE)) {
    goto exit_ak_write;
  }

  if (!dh_write_group(dh_file, subfile, new_node2_num, (char *)new_node2, DH_AK_NODE_SIZE)) {
    goto exit_ak_write;
  }

  if (new_node3 != NULL) {
    if (!dh_write_group(dh_file, subfile, new_node3_num, (char *)new_node3, DH_AK_NODE_SIZE)) {
      goto exit_ak_write;
    }
  }

  /* Adjust links in terminal nodes either side of the one to split */

  if (new_node1->left) {
    if (link_node == NULL) {
      link_node = (DH_TERM_NODE *)k_alloc(50, DH_AK_NODE_SIZE);
    }

    if (!dh_read_group(dh_file, subfile, GetAKFwdLink(dh_file, new_node1->left), (char *)link_node, DH_AK_NODE_SIZE)) {
      goto exit_ak_write;
    }

    link_node->right = SetAKFwdLink(dh_file, new_node1_num);

    if (!dh_write_group(dh_file, subfile, GetAKFwdLink(dh_file, new_node1->left), (char *)link_node, DH_AK_NODE_SIZE)) {
      goto exit_ak_write;
    }
  }

  /* The link from the left sibling node is now fixed.
                      ------      ----
                  AL<-| MS |  ->  | TZ
                      |    |      |
                      |    |  <-  |
                      ------      ----
        ----    ------      ------      ------
        AL | <- | MO |  ->  | PQ |  ->  | RS | -> TZ
           |    |    |      |    |      |    |
           | -> | 1  |  <-  | 2  |  <-  | 3  |
        ----    ------      ------      ------
  */

  /* Set link_node_ptr and offset to belong to the rightmost new node */

  if (new_node3 != NULL) {
    link_node_ptr = new_node3;
    node_num = new_node3_num;
  } else {
    link_node_ptr = new_node2;
    node_num = new_node2_num;
  }

  if (link_node_ptr->right) {
    if (link_node == NULL) {
      link_node = (DH_TERM_NODE *)k_alloc(50, DH_AK_NODE_SIZE);
    }

    if (!dh_read_group(dh_file, subfile, GetAKFwdLink(dh_file, link_node_ptr->right), (char *)link_node, DH_AK_NODE_SIZE)) {
      goto exit_ak_write;
    }

    link_node->left = SetAKFwdLink(dh_file, node_num);

    if (!dh_write_group(dh_file, subfile, GetAKFwdLink(dh_file, link_node_ptr->right), (char *)link_node, DH_AK_NODE_SIZE)) {
      goto exit_ak_write;
    }
  }

  /* The links are now all sorted out. The old MS node will get removed
     later.
                      ------
                  AL<-| MS |->TZ
                      |    |
                      |    |
                      ------
        ----    ------      ------      ------    ----
        AL | <- | MO |  ->  | PQ |  ->  | RS | -> | TZ
           |    |    |      |    |      |    |    |
           | -> | 1  |  <-  | 2  |  <-  | 3  | <- |
        ----    ------      ------      ------    ----
  */

  /* The terminal nodes are now correct. Now sort out parent nodes */

  if ((node_ptr = tail->prev) == NULL) {
    /* We just split the root (terminal) node.
       Reuse the original terminal root node as an internal root node.
       Because this was the root node, it follows that there cannot be
       any siblings to the left or right of the new nodes. Thus AL and TZ
       cannot exist.                                                       */

    memset(tail->node.buff, 0, DH_AK_NODE_SIZE);
    tail->node.int_node.node_type = AK_INT_NODE;
    tail->node.int_node.used_bytes = INT_NODE_HEADER_SIZE;

    /* Add leftmost child node */

    rec_ptr = rightmost(new_node1);
    memcpy(tail->node.buff + tail->node.int_node.used_bytes, rec_ptr->id, rec_ptr->id_len);
    tail->node.int_node.used_bytes += rec_ptr->id_len;
    x = (tail->node.int_node.child_count)++;
    tail->node.int_node.key_len[x] = rec_ptr->id_len;
    tail->node.int_node.child[x] = SetAKFwdLink(dh_file, new_node1_num);

    /* Add second child node */

    rec_ptr = rightmost(new_node2);
    memcpy(tail->node.buff + tail->node.int_node.used_bytes, rec_ptr->id, rec_ptr->id_len);
    tail->node.int_node.used_bytes += rec_ptr->id_len;
    x = (tail->node.int_node.child_count)++;
    tail->node.int_node.key_len[x] = rec_ptr->id_len;
    tail->node.int_node.child[x] = SetAKFwdLink(dh_file, new_node2_num);

    if (new_node3 != NULL) {
      /* Add rightmost child node */

      rec_ptr = rightmost(new_node3);
      memcpy(tail->node.buff + tail->node.int_node.used_bytes, rec_ptr->id, rec_ptr->id_len);
      tail->node.int_node.used_bytes += rec_ptr->id_len;
      x = (tail->node.int_node.child_count)++;
      tail->node.int_node.key_len[x] = rec_ptr->id_len;
      tail->node.int_node.child[x] = SetAKFwdLink(dh_file, new_node3_num);
    }

    /*
                              ------
                              |Root|
                      --------|node|--------
                     |        |    |        |
                     |        ------        |
                     v          v           v
                  ------      ------      ------
                  | MO |  ->  | PQ |  ->  | RS |
                  |    |      |    |      |    |
                  | 1  |  <-  | 2  |  <-  | 3  |
                  ------      ------      ------
    */

    /* Write new node (internal) node back to disk */

    if (!dh_write_group(dh_file, subfile, tail->node_num, (char *)(tail->node.buff), DH_AK_NODE_SIZE)) {
      goto exit_ak_write;
    }

    goto ak_written; /* All done */
  }

  /* The terminal node that we just split was not the root node.
     Work our way up the tree as far as necessary replacing child pointers */

  /* Release the old terminal node (MS) */

  if (!free_ak_node(dh_file, subfile, tail->node_num)) {
    goto exit_ak_write;
  }

  /* Find keys for new child nodes */

  rec_ptr = rightmost(new_node1);
  key1 = rec_ptr->id;
  key1_len = rec_ptr->id_len;

  rec_ptr = rightmost(new_node2);
  key2 = rec_ptr->id;
  key2_len = rec_ptr->id_len;

  if (new_node3 != NULL) {
    rec_ptr = rightmost(new_node3);
    key3 = rec_ptr->id;
    key3_len = rec_ptr->id_len;
  } else {
    key3 = NULL;
    key3_len = 0;
  }

  if (tail->prev != NULL) /* Not updating the root terminal node */
  {
    if (!update_internal_node(dh_file, subfile, tail->prev,   /* Node structure */
                              key1, key1_len, new_node1_num,  /* Child 1 */
                              key2, key2_len, new_node2_num,  /* Child 2 */
                              key3, key3_len, new_node3_num)) /* Child 3 */
    {
      goto exit_ak_write;
    }
  }

ak_written:

  /* Give away any big rec space from old record */

  if (old_big_rec_head) {
    if (!free_ak_big_rec(dh_file, subfile, old_big_rec_head)) {
      goto exit_ak_write;
    }
  }

  status = TRUE;

exit_ak_write:
  if (lock_slot != 0)
    FreeGroupWriteLock(lock_slot);
  if (header_lock != 0)
    FreeGroupWriteLock(header_lock);

  /* Release memory */

  if (tail != NULL) {
    do {
      node_ptr = tail->prev;
      k_free(tail);
    } while ((tail = node_ptr) != NULL);
  }

  if (new_node1 != NULL)
    k_free(new_node1);
  if (new_node2 != NULL)
    k_free(new_node2);
  if (new_node3 != NULL)
    k_free(new_node3);
  if (link_node != NULL)
    k_free(link_node);

  return status;
}

/* ======================================================================
   rightmost  - Return pointer to rightmost record in a terminal AK node  */

Private DH_RECORD *rightmost(DH_TERM_NODE *node) {
  int16_t rec_offset;
  int16_t used_bytes;
  DH_RECORD *rec_ptr;

  used_bytes = node->used_bytes;
  rec_offset = TERM_NODE_HEADER_SIZE;
  if (rec_offset == used_bytes)
    return NULL; /* Node is empty */

  do {
    rec_ptr = (DH_RECORD *)(((char *)node) + rec_offset);
    rec_offset += rec_ptr->next;
  } while (rec_offset < used_bytes);

  return rec_ptr;
}

/* ======================================================================
   copy_ak_record  - Copy new record into an AK node buffer               */

Private void copy_ak_record(DH_FILE *dh_file,
                            DH_RECORD *rec_ptr,
                            int16_t base_size,
                            char *id,
                            int16_t id_len,
                            int32_t big_rec_head, /* Head of big record chain or zero if not big */
                            int32_t data_len,     /* Data bytes */
                            STRING_CHUNK *str,    /* Record data string chunk head */
                            int16_t pad_bytes) {
  char *data_ptr; /* Data */

  rec_ptr->next = base_size;
  rec_ptr->flags = 0;
  rec_ptr->id_len = (u_char)id_len;
  memcpy(rec_ptr->id, id, id_len);

  if (big_rec_head) {
    rec_ptr->flags |= DH_BIG_REC;
    rec_ptr->data.big_rec = SetAKFwdLink(dh_file, big_rec_head);
  } else {
    rec_ptr->data.data_len = data_len;
    data_ptr = rec_ptr->id + id_len;
    if (data_len != 0) {
      do {
        memcpy(data_ptr, str->data, str->bytes);
        data_ptr += str->bytes;
        str = str->next;
      } while (str != NULL);
    }
    while (pad_bytes--)
      *(data_ptr++) = '\0';
  }
}

/* ======================================================================
   Read AK header to get free chain and create new node                   */

Private int32_t get_ak_node(DH_FILE *dh_file, int16_t subfile) {
  int32_t new_node_num = 0;
  DH_AK_HEADER *ak_header = NULL;
  DH_FREE_NODE ak_node;
  int64 file_bytes;

  ak_header = (DH_AK_HEADER *)k_alloc(51, DH_AK_HEADER_SIZE);

  if (!dh_read_group(dh_file, subfile, 0, (char *)ak_header, DH_AK_HEADER_SIZE)) {
    goto exit_get_ak_node;
  }

  if (ak_header->free_chain == 0) {
    file_bytes = filelength64(dh_file->sf[subfile].fu);
    new_node_num = (int32_t)((file_bytes - dh_file->ak_header_bytes) / DH_AK_NODE_SIZE + 1);

    chsize64(dh_file->sf[subfile].fu, file_bytes + DH_AK_NODE_SIZE);
  } else {
    new_node_num = GetAKFwdLink(dh_file, ak_header->free_chain);
    if (!dh_read_group(dh_file, subfile, new_node_num, (char *)&ak_node, DH_FREE_NODE_SIZE)) {
      goto exit_get_ak_node;
    }

    ak_header->free_chain = ak_node.next;
    if (!dh_write_group(dh_file, subfile, 0, (char *)ak_header, DH_AK_HEADER_SIZE)) {
      new_node_num = 0;
      goto exit_get_ak_node;
    }
  }

exit_get_ak_node:
  if (ak_header != NULL)
    k_free(ak_header);

  return new_node_num;
}

/* ======================================================================
   Free an AK node                                                        */

Private bool free_ak_node(DH_FILE *dh_file, int16_t subfile, int32_t node_num) {
  bool status = FALSE;
  DH_AK_HEADER *ak_header = NULL;
  DH_FREE_NODE ak_node;

  ak_header = (DH_AK_HEADER *)k_alloc(54, DH_AK_HEADER_SIZE);

  if (!dh_read_group(dh_file, subfile, 0, (char *)ak_header, DH_AK_HEADER_SIZE)) {
    goto exit_free_ak_node;
  }

  if (!dh_read_group(dh_file, subfile, node_num, (char *)&ak_node, DH_FREE_NODE_SIZE)) {
    goto exit_free_ak_node;
  }

  ak_node.node_type = AK_FREE_NODE;
  ak_node.next = ak_header->free_chain;
  if (!dh_write_group(dh_file, subfile, node_num, (char *)&ak_node, DH_FREE_NODE_SIZE)) {
    goto exit_free_ak_node;
  }

  ak_header->free_chain = SetAKFwdLink(dh_file, node_num);
  if (!dh_write_group(dh_file, subfile, 0, (char *)ak_header, DH_AK_HEADER_SIZE)) {
    goto exit_free_ak_node;
  }

  status = TRUE;

exit_free_ak_node:
  if (ak_header != NULL)
    k_free(ak_header);
  return status;
}

/* ======================================================================
   ak_delete()  -  Delete a record from an AK subfile                     */

Private void ak_delete(DH_FILE *dh_file, /* File descriptor */
                       int16_t akno,     /* AK index number */
                       char id[],        /* Record id... */
                       int16_t id_len)   /* ...and length */
{
  FILE_ENTRY *fptr;        /* File table entry pointer */
  int16_t subfile;         /* Subfile number */
  int16_t lock_slot = 0;   /* Group lock table index */
  int16_t header_lock = 0; /* File header group lock table index */
  int32_t node_num;
  int32_t sibling_node_num;
  int16_t flags;      /* AK flags from ak.data matrix */
  bool rj;            /* Right justified? */
  bool nocase;        /* Case insensitive? */
  int16_t child_ct;   /* Child node count */
  int16_t ci;         /* Child index for node scan */
  int16_t rec_offset; /* Offset of current record... */
  DH_RECORD *rec_ptr; /* ...its DH_RECORD pointer... */
  int16_t rec_size;   /* ...and its size */
  NODE *node_ptr;
  bool found;
  int16_t x;
  int n;
  char *key;       /* Key being examined in tree scan... */
  int16_t key_len; /* ...and its length */
  int16_t used_bytes;
  int32_t old_big_rec_head = 0; /* Head of old big record chain */
  char *prev_key;
  int16_t prev_len;
  DH_TERM_NODE *sibling = NULL;
  bool rightmost_record;

  tail = NULL;

  /* Get basic information */

  fptr = FPtr(dh_file->file_id);
  subfile = AK_BASE_SUBFILE + akno;
  flags = (int16_t)AKData(dh_file, akno, AKD_FLGS)->data.value;
  rj = (flags & AK_RIGHT) != 0;
  nocase = (flags & AK_NOCASE) != 0;
  fptr->stats.ak_deletes++;
  sysseg->global_stats.ak_deletes++;

  /* Lock the AK subfile */

  lock_slot = GetGroupWriteLock(dh_file, AKGlock(akno));
  (fptr->ak_upd)++;

  /* Find this record */

  node_num = 1;

  do {
    /* Allocate a new buffer area */

    node_ptr = (NODE *)k_alloc(43, sizeof(struct NODE));
    node_ptr->prev = tail;
    tail = node_ptr;
    tail->node_num = node_num;

    /* Read the node buffer */

    if (!dh_read_group(dh_file, subfile, node_num, tail->node.buff, DH_AK_NODE_SIZE)) {
      goto exit_ak_delete;
    }

    if (tail->node.term_node.node_type == AK_TERM_NODE)
      break;

    if (tail->node.int_node.node_type != AK_INT_NODE) {
      log_printf(
          "AK_DELETE: Invalid node type (x%04X) at subfile %d node %d\nof "
          "file %s.\n",
          (int)(tail->node.int_node.node_type), (int)subfile, node_num, fptr->pathname);
      dh_err = DHE_AK_NODE_ERROR;
      goto exit_ak_delete;
    }

    /* Search internal node for first key greater than or equal to the
       key we are looking for.  This search follows the normal rules for
       numeric/character sequencing.                                     */

    child_ct = tail->node.int_node.child_count;
    found = FALSE;
    for (ci = 0, key = tail->node.int_node.keys; ci < child_ct; ci++, key += key_len) {
      key_len = tail->node.int_node.key_len[ci];
      if (compare(id, id_len, key, key_len, rj, nocase) <= 0) {
        found = TRUE;
        break;
      }
    }

    if (!found) /* Key is to right of existing values */
    {
      goto exit_ak_delete;
    }

    /* Move down into child node */

    tail->ci = ci;
    tail->key_offset = key - tail->node.buff;
    node_num = GetAKFwdLink(dh_file, tail->node.int_node.child[ci]);
  } while (1);

  /* We have found the terminal node that should contain this key.
     Scan group buffer for record.                                  */

  used_bytes = tail->node.term_node.used_bytes;
  if ((used_bytes == 0) || (used_bytes > DH_AK_NODE_SIZE)) {
    log_printf(
        "AK_DELETE: Invalid byte count (x%04X) at subfile %d node %d\nof file "
        "%s.\n",
        used_bytes, (int)subfile, node_num, fptr->pathname);
    dh_err = DHE_POINTER_ERROR;
    goto exit_ak_delete;
  }

  rec_offset = TERM_NODE_HEADER_SIZE;
  found = FALSE;
  prev_key = NULL;
  while (rec_offset < used_bytes) {
    rec_ptr = (DH_RECORD *)(((char *)&(tail->node.term_node)) + rec_offset);
    rec_size = rec_ptr->next;

    x = compare(id, id_len, rec_ptr->id, rec_ptr->id_len, rj, nocase);
    if (x <= 0) /* Record goes here */
    {
      if (x == 0) /* Found key  -  Delete record */
      {
        rightmost_record = ((rec_offset + rec_ptr->next) == used_bytes);

        if (rec_ptr->flags & DH_BIG_REC) /* Old record is big */
        {
          old_big_rec_head = GetAKFwdLink(dh_file, rec_ptr->data.big_rec);
        }

        n = used_bytes - (rec_size + rec_offset);
        if (n > 0) /* There is something after */
        {
          memmove((char *)rec_ptr, ((char *)rec_ptr) + rec_size, n);
        }

        /* Clear the new slack space */

        memset(((char *)rec_ptr) + n, '\0', rec_size);

        used_bytes -= rec_size;
        tail->node.term_node.used_bytes = used_bytes;

        if (rightmost_record) {
          /* We have just deleted the rightmost record. */

          if (tail->prev != NULL) /* Not the root node */
          {
            /*  If we have deleted the only record in this terminal node
                we need to delete the node.                              */

            if (prev_key == NULL) /* Was also leftmost record */
            {
              /* Adjust right pointer from left sibling */

              sibling = (DH_TERM_NODE *)k_alloc(57, DH_AK_NODE_SIZE);

              if (tail->node.term_node.left != 0) {
                sibling_node_num = GetAKFwdLink(dh_file, tail->node.term_node.left);

                if (!dh_read_group(dh_file, subfile, sibling_node_num, (char *)sibling, DH_AK_NODE_SIZE)) {
                  goto exit_ak_delete;
                }

                sibling->right = tail->node.term_node.right;

                if (!dh_write_group(dh_file, subfile, sibling_node_num, (char *)sibling, DH_AK_NODE_SIZE)) {
                  goto exit_ak_delete;
                }
              }

              /* Adjust left pointer from right sibling */

              if (tail->node.term_node.right != 0) {
                sibling_node_num = GetAKFwdLink(dh_file, tail->node.term_node.right);

                if (!dh_read_group(dh_file, subfile, sibling_node_num, (char *)sibling, DH_AK_NODE_SIZE)) {
                  goto exit_ak_delete;
                }

                sibling->left = tail->node.term_node.left;

                if (!dh_write_group(dh_file, subfile, sibling_node_num, (char *)sibling, DH_AK_NODE_SIZE)) {
                  goto exit_ak_delete;
                }
              }

              /* Free node buffer */

              if (!free_ak_node(dh_file, subfile, tail->node_num)) {
                goto exit_ak_delete;
              }

              /* 0097 Remove reference from parent node */

              if (!update_internal_node(dh_file, subfile, tail->prev, NULL, 0, 0, NULL, 0, 0, NULL, 0, 0)) {
                goto exit_ak_delete;
              }

              goto node_deleted;
            }

            /* We have deleted the rightmost record in this node but there
               is at least one record left. Adjust parent internal node.    */

            if (!update_internal_node(dh_file, subfile, tail->prev, prev_key, prev_len, tail->node_num, NULL, 0, 0, NULL, 0, 0)) {
              goto exit_ak_delete;
            }
          }
        }
      }
      goto record_deleted;
    }

    prev_key = rec_ptr->id;
    prev_len = rec_ptr->id_len;
    rec_offset += rec_ptr->next;
  }

record_deleted:
  /* Rewrite this terminal node */

  if (!dh_write_group(dh_file, subfile, tail->node_num, tail->node.buff, DH_AK_NODE_SIZE)) {
    goto exit_ak_delete;
  }

node_deleted:
  /* Give away any big rec space from old record */

  if (old_big_rec_head) {
    if (!free_ak_big_rec(dh_file, subfile, old_big_rec_head)) {
      goto exit_ak_delete;
    }
  }

exit_ak_delete:
  if (lock_slot != 0)
    FreeGroupWriteLock(lock_slot);
  if (header_lock != 0)
    FreeGroupWriteLock(header_lock);

  /* Release memory */

  if (sibling != NULL)
    k_free(sibling);

  if (tail != NULL) {
    do {
      node_ptr = tail->prev;
      k_free(tail);
    } while ((tail = node_ptr) != NULL);
  }
}

/* ======================================================================
   ak_read()  -  Read a record from an AK subfile                         */

Private STRING_CHUNK *ak_read(DH_FILE *dh_file, /* File descriptor */
                              int16_t akno,     /* AK index number */
                              char id[],        /* Record id... */
                              int16_t id_len,   /* ...and length */
                              bool read_data)   /* Read data record? If false, returns NULL if
                         record does not exist, non-NULL if it does.  */
{
  FILE_ENTRY *fptr;      /* File table entry pointer */
  int32_t data_len;      /* Data length */
  int16_t subfile;       /* Subfile number */
  int16_t lock_slot = 0; /* Group lock table index */
  int32_t node_num;
  int32_t big_node_num;   /* 0556 */
  int16_t flags;          /* AK flags from ak.data matrix */
  bool rj;                /* Right justified? */
  bool nocase;            /* Case insensitive? */
  int16_t child_ct;       /* Child node count */
  int16_t ci;             /* Child index for node scan */
  int16_t rec_offset = 0; /* Offset of current record and... */ /* initialized to clear a clang warning -gwb */
  DH_RECORD *rec_ptr;     /* ...its DH_RECORD pointer */
  union AKBUFF *node = NULL;
  bool found;
  STRING_CHUNK *str = NULL;
  int16_t x;
  int n;
  char *key;       /* Key being examined in tree scan... */
  int16_t key_len; /* ...and its length */
  int16_t used_bytes;
  int32_t child_node;

  ak_flags = 0;

  /* Allocate node buffer */

  node = (AKBUFF *)k_alloc(105, sizeof(AKBUFF));

  /* Get basic information */

  fptr = FPtr(dh_file->file_id);
  subfile = AK_BASE_SUBFILE + akno;
  flags = (int16_t)(AKData(dh_file, akno, AKD_FLGS)->data.value);
  rj = (flags & AK_RIGHT) != 0;
  nocase = (flags & AK_NOCASE) != 0;
  ak_upd = fptr->ak_upd;
  fptr->stats.ak_reads++;
  sysseg->global_stats.ak_reads++;

  /* Lock the AK subfile */

  lock_slot = GetGroupReadLock(dh_file, AKGlock(akno));

  /* Find the position for this record */

  node_num = 1;

  do {
    /* Read the node buffer */

    if (!dh_read_group(dh_file, subfile, node_num, node->buff, DH_AK_NODE_SIZE)) {
      goto exit_ak_read;
    }

    if (node->term_node.node_type == AK_TERM_NODE)
      break;

    if (node->int_node.node_type != AK_INT_NODE) {
      log_printf(
          "AK_READ: Invalid node type (x%04X) at subfile %d node %d\nof file "
          "%s.\n",
          (int)(node->int_node.node_type), (int)subfile, node_num, fptr->pathname);
      dh_err = DHE_AK_NODE_ERROR;
      goto exit_ak_read;
    }

    /* Search internal node for first key greater than or equal to the
       key we are looking for.  This search follows the normal rules for
       numeric/character sequencing.                                     */

    child_ct = node->int_node.child_count;
    found = FALSE;
    for (ci = 0, key = (char *)(node->int_node.keys); ci < child_ct; ci++, key += key_len) {
      key_len = node->int_node.key_len[ci];
      if (compare(id, id_len, key, key_len, rj, nocase) <= 0) {
        found = TRUE;
        break;
      }
    }

    if (!found) /* New key is to right of existing values */
    {
      /* Descend to rightmost terminal node so that we can return the
         correct position information.                                */

      do {
        child_node = GetAKFwdLink(dh_file, node->int_node.child[node->int_node.child_count - 1]);
        node_num = child_node;
        if (!dh_read_group(dh_file, subfile, node_num, node->buff, DH_AK_NODE_SIZE)) {
          goto exit_ak_read;
        }
      } while (node->int_node.node_type == AK_INT_NODE);
      rec_offset = node->term_node.used_bytes;
      ak_flags |= AKS_RIGHT;
      goto exit_ak_read;
    }

    /* Move down into child node */

    node_num = GetAKFwdLink(dh_file, node->int_node.child[ci]);
  } while (1);

  /* We have found the terminal node that should contain this key.
     Scan group buffer for record.                                  */

  used_bytes = node->term_node.used_bytes;
  if ((used_bytes == 0) || (used_bytes > DH_AK_NODE_SIZE)) {
    log_printf(
        "AK_READ: Invalid byte count (x%04X) at subfile %d node %d\nof file "
        "%s.\n",
        used_bytes, (int)subfile, node_num, fptr->pathname);
    dh_err = DHE_POINTER_ERROR;
    goto exit_ak_read;
  }

  /* If the terminal node is empty, this must be an empty tree. The
     record we are seeking is, therefore, off the left and the right
     of the tree.                                                    */

  if (used_bytes == TERM_NODE_HEADER_SIZE) {
    ak_flags |= (AKS_LEFT | AKS_RIGHT);
    goto exit_ak_read;
  }

  rec_offset = TERM_NODE_HEADER_SIZE;
  found = FALSE;
  while (rec_offset < used_bytes) {
    rec_ptr = (DH_RECORD *)(((char *)&(node->term_node)) + rec_offset);

    x = compare(id, id_len, rec_ptr->id, rec_ptr->id_len, rj, nocase);
    if (x <= 0) /* Found or gone past position */
    {
      ak_node_num = node_num;
      ak_rec_offset = rec_offset;

      if (x == 0) /* Found key  -  Extract record */
      {
        ak_flags |= AKS_FOUND;

        if (read_data) {
          if (rec_ptr->flags & DH_BIG_REC) /* Found a large record */
          {
            big_node_num = GetAKFwdLink(dh_file, rec_ptr->data.big_rec);
            while (big_node_num != 0) {
              if (!dh_read_group(dh_file, subfile, big_node_num, node->buff, DH_AK_NODE_SIZE)) {
                goto exit_ak_read;
              }

              if (str == NULL) {
                data_len = node->big_node.data_len;
                ts_init(&str, data_len);
              }

              n = min(DH_AK_NODE_SIZE - DH_AK_BIG_NODE_SIZE, data_len);
              ts_copy(node->big_node.data, n);
              data_len -= n;

              big_node_num = GetAKFwdLink(dh_file, node->big_node.next);
            }

            (void)ts_terminate();
          } else /* Not a large record */
          {
            data_len = rec_ptr->data.data_len;
            if (data_len != 0) {
              ts_init(&str, data_len);
              ts_copy(rec_ptr->id + rec_ptr->id_len, data_len);
              (void)ts_terminate();
            }
          }
        } else /* Just searching for record */
        {
          str = (STRING_CHUNK *)rec_ptr; /* Anything non-null */
        }
      } else /* Gone past required position */
      {
        /* If we are still at the first record in the node and there is
           no left sibling, the record we are seeking is to the left of
           the leftmost record in the tree.                             */

        if ((rec_offset == TERM_NODE_HEADER_SIZE) && (node->term_node.left == 0)) {
          ak_flags |= AKS_LEFT;
        }
      }
      goto exit_ak_read;
    }

    rec_offset += rec_ptr->next;
  }

  /* If we arrive here, we have gone off the right side of the terminal node.
     This can only happen when we are searching for a key greater than the
     last key in a tree that has a terminal root node.                        */

  ak_flags |= AKS_RIGHT;

exit_ak_read:
  ak_node_num = node_num; /* 0516 Moved */
  ak_rec_offset = rec_offset;

  if (lock_slot != 0)
    FreeGroupReadLock(lock_slot);
  if (node != NULL)
    k_free(node);

  return str;
}

/* ======================================================================
   Compare strings
   Returns -1   s2 < s1
            0   Strings are equal
            1   s1 > s2                                                   */

Private int16_t compare(char *s1, int16_t s1_len, char *s2, int16_t s2_len, bool right_justified, bool nocase) {
  int16_t n;
  int16_t d = 0;
  int32_t value1;
  int32_t value2;

  if (right_justified) {
    if (strnint(s1, s1_len, &value1) && strnint(s2, s2_len, &value2)) {
      if (value1 > value2)
        d = 1;
      else if (value1 == value2)
        d = 0;
      else
        d = -1;
      return d;
    }

    if (s1_len != s2_len) {
      /* Compare leading characters of longer string against spaces */

      if (s1_len > s2_len) {
        n = s1_len - s2_len;
        s1_len -= n;
        while (n--) {
          if ((d = (((int16_t) * ((u_char *)s1++)) - ' ')) != 0)
            goto mismatch;
        }
      } else {
        n = s2_len - s1_len;
        s2_len -= n;
        while (n--) {
          if ((d = (' ' - ((int16_t) * ((u_char *)s2++)))) != 0)
            goto mismatch;
        }
      }
    }
  }

  /* Compare to end of shorter string */

  while (s1_len && s2_len) {
    if (nocase) {
      if ((d = (((int16_t)(UpperCase(*(s1++)))) - UpperCase(*(s2++)))) != 0)
        goto mismatch;
    } else {
      if ((d = (((int16_t)(*((u_char *)s1++))) - *((u_char *)s2++))) != 0)
        goto mismatch;
    }

    s1_len--;
    s2_len--;
  }

  /* If either string still has unprocessed characters, it is the greater
     string (d must be zero when we arrive here).                         */

  d = s1_len - s2_len;
  goto exit_compare;

mismatch:
  if (d > 0)
    d = 1;
  else if (d < 0)
    d = -1;
  else
    d = 0;

exit_compare:
  return d;
}

/* ======================================================================
   update_internal_node()  -  Replace an internal key child pointer       */

Private bool update_internal_node(DH_FILE *dh_file,  /* DH file affected and... */
                                  int16_t subfile,   /* ...subfile number */
                                  NODE *node_ptr,    /* Node structure for change */
                                  char *id1,         /* Key for child 1 */
                                  int16_t id1_len,   /* Length of key for child 1 */
                                  int32_t node_num1, /* Child node pointer for child 1 */
                                  char *id2,         /* Key for child 2 (May be NULL) */
                                  int16_t id2_len,   /* Length of key for child 2 */
                                  int32_t node_num2, /* Child node pointer for child 2 */
                                  char *id3,         /* Key for child 3 (May be NULL) */
                                  int16_t id3_len,   /* Length of key for child 3 */
                                  int32_t node_num3) /* Child node pointer for child 3 */
{
  bool status = FALSE;
  int16_t num_child;            /* Number of child nodes required */
  int16_t key_len;              /* Space required for key data */
  int16_t key_diff;             /* Difference from previous space requirement */
  int32_t new_node_num;         /* Offset of new node if we split */
  DH_INT_NODE *new_node = NULL; /* Pointer to new node buffer */
  NODE *root_node;              /* Pointer to new root NODE structure */
  int16_t moved_children;
  int16_t remaining_children;
  int16_t uncopied_bytes;
  char *last_key_in_left_node;
  char *last_key_in_right_node;
  char *pkey;       /* Parent key */
  int16_t pkey_len; /* Parent key length */
  bool update_parent;
  int32_t node_num;
  int16_t i;
  int16_t j;
  int16_t n;
  char *p;
  char *q;
  NODE *r;

  /* Work out how many child nodes we are setting up and the total space
     required for the key strings.                                        */

  if (id1 == NULL) /* No children (deleted a child node) */
  {
    num_child = 0;
    key_len = 0;
    pkey = NULL;
    pkey_len = 0;
  } else if (id2 == NULL) /* One child */
  {
    num_child = 1;
    key_len = id1_len;
    pkey = id1;
    pkey_len = id1_len;
  } else if (id3 == NULL) /* Two children */
  {
    num_child = 2;
    key_len = id1_len + id2_len;
    pkey = id2;
    pkey_len = id2_len;
  } else /* Three children */
  {
    num_child = 3;
    key_len = id1_len + id2_len + id3_len;
    pkey = id3;
    pkey_len = id3_len;
  }

  update_parent = (node_ptr->ci == node_ptr->node.int_node.child_count - 1);

  key_diff = key_len - node_ptr->node.int_node.key_len[node_ptr->ci];
  /* Difference in key data length from old to new node */

  if ((node_ptr->node.int_node.child_count + num_child - 1 > MAX_CHILD) || ((node_ptr->node.int_node.used_bytes + key_diff) > DH_AK_NODE_SIZE)) {
    /* This internal node must be split before we can do the replacement
       either because it already holds the maximum number of keys or
       because there is insufficient space for the new key data.
       We split the internal node into two equal halves. The original node
       buffer becomes the left half, a new node buffer is allocated for
       the right half.                                                     */

    new_node_num = get_ak_node(dh_file, subfile);
    new_node = (DH_INT_NODE *)k_alloc(55, DH_AK_NODE_SIZE);
    memset((char *)new_node, '\0', DH_AK_NODE_SIZE);

    new_node->node_type = AK_INT_NODE;

    /* Copy the second half of the child data from the old node to the
       new node.                                                       */

    remaining_children = node_ptr->node.int_node.child_count / 2;
    moved_children = node_ptr->node.int_node.child_count - remaining_children;

    p = node_ptr->node.int_node.keys;
    for (i = 0; i < remaining_children; i++) /* Skip over first half */
    {
      last_key_in_left_node = (char *)p;
      p += node_ptr->node.int_node.key_len[i];
    }
    node_ptr->node.int_node.used_bytes = p - node_ptr->node.buff;
    node_ptr->node.int_node.child_count = (u_char)remaining_children;
    uncopied_bytes = p - node_ptr->node.int_node.keys;

    q = (char *)(new_node->keys);
    for (j = 0; j < moved_children; j++, i++) /* Copy second half */
    {
      new_node->child[j] = node_ptr->node.int_node.child[i];
      n = node_ptr->node.int_node.key_len[i];
      new_node->key_len[j] = (u_char)n;
      memcpy(q, p, n);
      last_key_in_right_node = q;

      /* Clear data from old node */
      memset(p, 0, n);
      node_ptr->node.int_node.child[i] = 0;
      node_ptr->node.int_node.key_len[i] = 0;
      p += n;
      q += n;
    }
    new_node->child_count = (u_char)moved_children;
    new_node->used_bytes = q - (char *)new_node;

    /* If the old node was the root node, we need to do a bit of moving.
       This is the only situation in which we add a new level to the tree. */

    if (node_ptr->prev == NULL) /* Was the root node */
    {
      /* Write out the old root internal node into a new position */

      node_ptr->node_num = get_ak_node(dh_file, subfile);

      /* Create a new root internal node to point to the old and new
         child nodes.                                                 */

      root_node = (NODE *)k_alloc(56, sizeof(struct NODE));
      node_ptr->prev = root_node;
      root_node->prev = NULL;
      root_node->node_num = 1;
      root_node->ci = 0;
      root_node->key_offset = INT_NODE_HEADER_SIZE;

      /* Set up left child */
      memset(root_node->node.buff, 0, DH_AK_NODE_SIZE);
      root_node->node.int_node.node_type = AK_INT_NODE;
      root_node->node.int_node.used_bytes = INT_NODE_HEADER_SIZE;

      n = node_ptr->node.int_node.key_len[node_ptr->node.int_node.child_count - 1];
      memcpy(root_node->node.buff + root_node->node.int_node.used_bytes, last_key_in_left_node, n);
      root_node->node.int_node.used_bytes += n;
      root_node->node.int_node.key_len[0] = (u_char)n;
      root_node->node.int_node.child[0] = SetAKFwdLink(dh_file, node_ptr->node_num);

      /* Set up right child */

      if (node_ptr->ci >= moved_children) /* New key is in right child */
      {
        root_node->ci = 1;
        root_node->key_offset += n;
      }

      n = new_node->key_len[new_node->child_count - 1];
      memcpy(root_node->node.buff + root_node->node.int_node.used_bytes, last_key_in_right_node, n);
      root_node->node.int_node.used_bytes += n;
      root_node->node.int_node.key_len[1] = (u_char)n;
      root_node->node.int_node.child[1] = SetAKFwdLink(dh_file, new_node_num);

      root_node->node.int_node.child_count = 2;

      if (!dh_write_group(dh_file, subfile, root_node->node_num, (char *)(root_node->node.buff), DH_AK_NODE_SIZE)) {
        goto exit_update_internal_node;
      }
    } else {
      /* Call update_internal_node recursively to update the parent
         internal node (if any).                                    */

      if (!update_internal_node(dh_file, subfile, node_ptr->prev, last_key_in_left_node, node_ptr->node.int_node.key_len[node_ptr->node.int_node.child_count - 1], node_ptr->node_num,
                                last_key_in_right_node, new_node->key_len[new_node->child_count - 1], new_node_num, NULL, 0, 0)) {
        goto exit_update_internal_node;
      }
    }

    /* Write out the old internal node, moved if it was the root node */

    if (!dh_write_group(dh_file, subfile, node_ptr->node_num, node_ptr->node.buff, DH_AK_NODE_SIZE)) {
      goto exit_update_internal_node;
    }

    /* Write out the new internal node */

    if (!dh_write_group(dh_file, subfile, new_node_num, (char *)new_node, DH_AK_NODE_SIZE)) {
      goto exit_update_internal_node;
    }

    /* Adjust the node cache to reflect the changes we have made */

    if (node_ptr->ci >= remaining_children) {
      /* The child pointer to replace is in the new node. Update the cache. */

      node_ptr->node_num = new_node_num;
      node_ptr->ci -= remaining_children;
      node_ptr->key_offset -= uncopied_bytes;
      memcpy(node_ptr->node.buff, (char *)new_node, DH_AK_NODE_SIZE);

      if (node_ptr->prev != NULL) {
        node_ptr->prev->key_offset += node_ptr->prev->node.int_node.key_len[node_ptr->prev->ci];
        (node_ptr->prev->ci)++;
      }
    }
  }

  /* The new child pointers can now be added to the internal node. */

  p = node_ptr->node.buff + node_ptr->key_offset;
  if (key_diff > 0) /* Open a gap */
  {
    memmove(p + key_diff, p, node_ptr->node.int_node.used_bytes - node_ptr->key_offset);
  } else if (key_diff < 0) /* Close the gap */
  {
    n = node_ptr->node.int_node.used_bytes - node_ptr->key_offset + key_diff;
    memmove(p, p - key_diff, n);
    memset(p + n, 0, -key_diff); /* Clear the free space */
  }

  node_ptr->node.int_node.used_bytes += key_diff;

  /* Adjust the pointer and key length tables */

  if (num_child > 1) {
    for (j = node_ptr->node.int_node.child_count - 1; j >= node_ptr->ci; j--) {
      node_ptr->node.int_node.child[j + num_child - 1] = node_ptr->node.int_node.child[j];
      node_ptr->node.int_node.key_len[j + num_child - 1] = node_ptr->node.int_node.key_len[j];
    }
  } else if (num_child == 0) /* Removing a child pointer */
  {
    for (j = node_ptr->ci; j < node_ptr->node.int_node.child_count - 1; j++) {
      node_ptr->node.int_node.child[j] = node_ptr->node.int_node.child[j + 1];
      node_ptr->node.int_node.key_len[j] = node_ptr->node.int_node.key_len[j + 1];
    }
    node_ptr->node.int_node.child[j] = 0;
    node_ptr->node.int_node.key_len[j] = 0;
  }

  /* Insert the new/modified keys and pointers */

  if (id1 != NULL) {
    j = node_ptr->ci;
    node_ptr->node.int_node.child[j] = SetAKFwdLink(dh_file, node_num1);
    node_ptr->node.int_node.key_len[j] = (u_char)id1_len;
    memcpy(p, id1, id1_len);
    p += id1_len;

    if (id2 != NULL) {
      j++;
      node_ptr->node.int_node.child[j] = SetAKFwdLink(dh_file, node_num2);
      node_ptr->node.int_node.key_len[j] = (u_char)id2_len;
      memcpy(p, id2, id2_len);
      p += id2_len;

      if (id3 != NULL) {
        j++;
        node_ptr->node.int_node.child[j] = SetAKFwdLink(dh_file, node_num3);
        node_ptr->node.int_node.key_len[j] = (u_char)id3_len;
        memcpy(p, id3, id3_len);
      }
    }
  }

  node_ptr->node.int_node.child_count += num_child - 1;

  /* Check if we can compress the tree */

  /* !!!! Could also do something to merge adjacent internal nodes */

  if (node_ptr->node.int_node.child_count == 1) {
    /* This node has only one child. Simply copy the child node in its
       place, freeing the space from the old child node.                */

    node_num = GetAKFwdLink(dh_file, node_ptr->node.int_node.child[0]);
    if (!dh_read_group(dh_file, subfile, node_num, node_ptr->node.buff, DH_AK_NODE_SIZE)) {
      goto exit_update_internal_node;
    }

    if (!free_ak_node(dh_file, subfile, node_num)) {
      goto exit_update_internal_node;
    }

    /* Scan the node buffer chain. Change the offset of the moved block,
       remove the reallocated block.                                     */

    for (r = tail; r != NULL; r = r->prev) {
      if (r->node_num == node_num) /* This is the one we have moved */
      {
        r->node_num = node_ptr->node_num;
        r->prev = node_ptr->prev;
        k_free(node_ptr);
        node_ptr = r;
        break;
      }
    }
  }

  if (!dh_write_group(dh_file, subfile, node_ptr->node_num, node_ptr->node.buff, DH_AK_NODE_SIZE)) {
    goto exit_update_internal_node;
  }

  /* If we are updating/deleting/expanding the final child of this node,
     we also need to update its parent.                                  */

  if (update_parent && (node_ptr->prev != NULL)) {
    if (!update_internal_node(dh_file, subfile, node_ptr->prev, pkey, pkey_len, node_ptr->node_num, NULL, 0, 0, NULL, 0, 0)) {
      goto exit_update_internal_node;
    }
  }

  status = TRUE;

exit_update_internal_node:
  if (new_node != NULL)
    k_free(new_node);

  return status;
}

/* ======================================================================
   Clear an AK index                                                      */

bool ak_clear(DH_FILE *dh_file, int16_t subfile) {
  bool status = FALSE;
  int64 eof;
  int32_t node_num;
  char *buff = NULL;

  buff = (char *)k_alloc(59, DH_AK_NODE_SIZE);

  if (FDS_open(dh_file, subfile)) {
    if (!dh_read_group(dh_file, subfile, 0, buff, DH_AK_HEADER_SIZE)) {
      goto exit_ak_clear;
    }

    /* Clear free chain and rewrite AK header if necessry */

    if (((DH_AK_HEADER *)buff)->free_chain) {
      ((DH_AK_HEADER *)buff)->free_chain = 0;
      if (!dh_write_group(dh_file, subfile, 0, buff, DH_AK_HEADER_SIZE)) {
        goto exit_ak_clear;
      }
    }

    /* If this AK has a "big" I-type, we must find the end of it. The
       I-type is always written as a contiguous set of nodes after
       the AK header.                                                 */

    eof = AKHeaderSize(dh_file->file_version) + DH_AK_NODE_SIZE;
    node_num = GetAKFwdLink(dh_file, ((DH_AK_HEADER *)buff)->itype_ptr);
    while (node_num) {
      if (!dh_read_group(dh_file, subfile, node_num, buff, DH_AK_NODE_SIZE)) {
        goto exit_ak_clear;
      }

      eof = ((int64)node_num) * DH_AK_NODE_SIZE + dh_file->ak_header_bytes;

      node_num = GetAKFwdLink(dh_file, ((DH_ITYPE_NODE *)buff)->next);
    }

    chsize64(dh_file->sf[subfile].fu, eof);

    /* Write an empty terminal node at the root node position */

    memset(buff, 0, DH_AK_NODE_SIZE);
    ((DH_TERM_NODE *)buff)->used_bytes = TERM_NODE_HEADER_SIZE;
    ((DH_TERM_NODE *)buff)->node_type = AK_TERM_NODE;
    if (!dh_write_group(dh_file, subfile, 1, buff, DH_AK_NODE_SIZE)) {
      goto exit_ak_clear;
    }
  }

  status = TRUE;

exit_ak_clear:
  if (buff != NULL)
    k_free(buff);

  return status;
}

/* ======================================================================
   find_ak_by_name()  -  Find AK number from its name
   Returns -1 if no such AK                                               */

Private int16_t find_ak_by_name(DESCRIPTOR *name_descr, DH_FILE *dh_file) {
  char name[MAX_AK_NAME_LEN + 1];
  int16_t name_len;
  int16_t num_aks;
  int16_t akno;
  DESCRIPTOR *descr;
  ARRAY_HEADER *ak_data;
  STRING_CHUNK *str;

  name_len = k_get_c_string(name_descr, name, MAX_AK_NAME_LEN);
  if (name_len > 0) {
    if ((ak_data = dh_file->ak_data) != NULL) {
      num_aks = (int16_t)(ak_data->rows);
      for (akno = 0; akno < num_aks; akno++) {
        descr = Element(ak_data, (akno * AKD_COLS) + AKD_NAME);
        if (((str = descr->data.str.saddr) != NULL) && (str->string_len == name_len) && (!memcmp(str->data, name, name_len))) /* Always one chunk */
        {
          return akno;
        }
      }
    }
  }

  return -1;
}

/* ====================================================================== */

Private STRING_CHUNK *ak_read_record(DH_FILE *dh_file, int16_t subfile, DH_RECORD *rec_ptr) {
  STRING_CHUNK *str = NULL;
  int32_t node_num;
  int32_t data_len;
  int16_t n;
  DH_BIG_NODE *buff = NULL;

  if (rec_ptr->flags & DH_BIG_REC) /* Found a large record */
  {
    buff = (DH_BIG_NODE *)k_alloc(72, DH_AK_NODE_SIZE);

    node_num = GetAKFwdLink(dh_file, rec_ptr->data.big_rec);
    while (node_num != 0) {
      if (!dh_read_group(dh_file, subfile, node_num, (char *)buff, DH_AK_NODE_SIZE)) {
        goto exit_ak_read_record;
      }

      if (str == NULL) {
        data_len = buff->data_len;
        ts_init(&str, data_len);
      }

      n = (int16_t)min(DH_AK_NODE_SIZE - DH_AK_BIG_NODE_SIZE, data_len);
      ts_copy(buff->data, n);
      data_len -= n;

      node_num = GetAKFwdLink(dh_file, buff->next);
    }

    (void)ts_terminate();
  } else /* Not a large record */
  {
    data_len = rec_ptr->data.data_len;
    if (data_len != 0) {
      ts_init(&str, data_len);
      ts_copy(rec_ptr->id + rec_ptr->id_len, data_len);
      (void)ts_terminate();
    }
  }

exit_ak_read_record:
  if (buff != NULL)
    k_free(buff);

  return str;
}

/* ======================================================================
   write_ak_big_rec()  -  Write big record blocks                         */

Private int32_t write_ak_big_rec(DH_FILE *dh_file, int16_t subfile, STRING_CHUNK *rec, int32_t data_len) {
  int32_t head;             /* Head of new big record chain */
  DH_BIG_NODE *buff = NULL; /* Buffer */
  int32_t node_num;         /* Node being processed */
  int32_t next_node_num;    /* Next node to write */
  STRING_CHUNK *str;
  int16_t bytes;
  int n;
  char *p;
  char *q;
  int16_t x;

  /* Allocate overflow buffer */

  buff = (DH_BIG_NODE *)k_alloc(48, DH_AK_NODE_SIZE);
  if (buff == NULL) {
    dh_err = DHE_NO_MEM;
    head = 0;
    goto exit_write_ak_big_rec;
  }

  head = get_ak_node(dh_file, subfile);
  node_num = head;

  memset((char *)buff, '\0', DH_AK_NODE_SIZE);
  buff->node_type = AK_BIGREC_NODE;
  buff->data_len = data_len;

  str = rec;
  bytes = str->bytes;
  q = str->data;

  do {
    n = min(DH_AK_NODE_SIZE - DH_AK_BIG_NODE_SIZE, data_len);
    buff->used_bytes = DH_AK_BIG_NODE_SIZE + n;

    p = buff->data;
    data_len -= n;
    while (n != 0) {
      x = min(n, bytes);
      memcpy(p, q, x);
      p += x;
      q += x;
      n -= x;
      bytes -= x;
      if (bytes == 0) {
        if ((str = str->next) != NULL) {
          bytes = str->bytes;
          q = str->data;
        }
      }
    }

    if (data_len != 0) /* More */
    {
      next_node_num = get_ak_node(dh_file, subfile);
      buff->next = SetAKFwdLink(dh_file, next_node_num);
    }

    if (!dh_write_group(dh_file, subfile, node_num, (char *)buff, DH_AK_NODE_SIZE)) {
      head = 0;
      goto exit_write_ak_big_rec;
    }

    if (data_len == 0)
      break;

    memset(buff, '\0', DH_AK_NODE_SIZE);
    buff->node_type = AK_BIGREC_NODE;
    node_num = next_node_num;
  } while (1);

exit_write_ak_big_rec:
  if (buff != NULL)
    k_free(buff);

  return head;
}

/* ======================================================================
   free_ak_big_rec()  -  Release space from a big record                  */

Private bool free_ak_big_rec(DH_FILE *dh_file, int16_t subfile, int32_t head) {
  bool status = FALSE;
  DH_BIG_NODE *buff = NULL;
  int32_t next_node_num;
  int32_t node_num;

  buff = (DH_BIG_NODE *)k_alloc(49, DH_AK_NODE_SIZE);
  if (buff == NULL) {
    dh_err = DHE_NO_MEM;
    goto exit_free_ak_big_rec;
  }

  next_node_num = head;
  do {
    node_num = next_node_num;
    if (!dh_read_group(dh_file, subfile, node_num, (char *)buff, DH_AK_NODE_SIZE)) {
      goto exit_free_ak_big_rec;
    }

    next_node_num = GetAKFwdLink(dh_file, buff->next);

    free_ak_node(dh_file, subfile, node_num);
  } while (next_node_num);

  status = TRUE;

exit_free_ak_big_rec:
  if (buff != NULL)
    k_free(buff);

  return status;
}

/* END-CODE */
