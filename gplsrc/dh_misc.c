/* DH_MISC.C
 * Miscellaneous DH file opcodes.
 * Copyright (c) 2006 Ladybridge Systems, All Rights Reserved
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
 * START-HISTORY (ScarletDME):
 * 10Jan22 gwb Fixed some format specifier warnings.
 *
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 * 22Feb20 gwb Fixed two 'variable assigned but unused' warnings in
 *             op_fcontrol()
 * 
 * START-HISTORY (OpenQM):
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 30 Aug 06  2.4-12 Added fcontrol mode 5 to force file rehash.
 * 19 Sep 05  2.2-11 Use dh_buffer to reduce stack space.
 * 04 May 05  2.1-13 Initial implementation of large file support.
 * 11 Oct 04  2.0-5 Added modes to op_settrig.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 *  op_fcontrol     FCONTROL    Miscellaneous file control functions
 *  op_grpstat      GRPSTAT     Return information about file group
 *  op_settrig      SETTRIG     Set trigger function
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include "dh_int.h"
#include "keys.h"

/* ======================================================================
   op_fcontrol()  -  Miscellaneous file control functions                 */

void op_fcontrol() {
  /* Stack:

      |================================|=============================|
      |            BEFORE              |           AFTER             |
      |================================|=============================|
  top |  Qualifier                     | Returned data               |
      |--------------------------------|-----------------------------|
      |  Action key                    |                             |
      |--------------------------------|-----------------------------|
      |  File variable                 |                             |
      |================================|=============================|

  This function is restricted for internal use only and therefore assumes
  that the caller knows what they are doing. There is minimal validation.

  Key   Action                                      Qualifier
    1   Set journalling file no in file header      File number
    2   Disable journalling for an open file
    3   Set akpath element of file header           Path name
    4   Set file as non-transactional
    5   Force resize
    6   Set/clear DHF_NO_RESIZE flag                New setting
 */

  DESCRIPTOR* descr;
  int action;
  FILE_VAR* fvar;
  DESCRIPTOR result;
  DH_FILE* dh_file;
  DH_HEADER header;
  int32_t modulus;
  int32_t load;
  FILE_ENTRY* fptr;
  int16_t header_lock;
  /* u_char ftype; variable assigned but not used */
  /* bool dynamic; variable assigned but not used */

  process.status = 0;

  InitDescr(&result, INTEGER); /* Assume integer return value by default */
  result.data.value = 0;

  /* Get action key */

  descr = e_stack - 2;
  GetInt(descr);
  action = descr->data.value;

  /* Get file variable */

  descr = e_stack - 3;
  k_get_file(descr);
  fvar = descr->data.fvar;
  fptr = FPtr(fvar->file_id);
  /* ftype = fvar->type; variable assigned but not used */
  /* dynamic = (ftype == DYNAMIC_FILE); variable assigned but never used */
  dh_file = fvar->access.dh.dh_file; /* May be irrelevant */

  /* Resolve value of qualifier */

  descr = e_stack - 1;
  k_get_value(descr);

  switch (action) {
    case FC_SET_JNL_FNO: /* Set jnl_fno field of file header */
      GetInt(descr);

      /* Read file header - We have exclusive access so needn't lock */

      if (!dh_read_group(dh_file, PRIMARY_SUBFILE, 0, (char*)&header,
                         DH_HEADER_SIZE)) {
        process.status = ER_IOE;
        goto exit_op_fcontrol;
      }

      header.jnl_fno = descr->data.value;
      dh_file->jnl_fno = descr->data.value;

      if (!dh_write_group(dh_file, PRIMARY_SUBFILE, 0, (char*)&header,
                          DH_HEADER_SIZE)) {
        process.status = ER_IOE;
        goto exit_op_fcontrol;
      }

      result.data.value = 1;
      break;

    case FC_KILL_JNL:
      dh_file->jnl_fno = 0; /* Kill journalling */
      break;

    case FC_SET_AKPATH:
      /* Read file header - We have exclusive access so needn't lock */

      if (!dh_read_group(dh_file, PRIMARY_SUBFILE, 0, (char*)&header,
                         DH_HEADER_SIZE)) {
        process.status = ER_IOE;
        goto exit_op_fcontrol;
      }

      k_get_c_string(descr, header.akpath, MAX_PATHNAME_LEN);
#ifdef CASE_INSENSITIVE_FILE_SYSTEM
      UpperCaseString(header.akpath);
#endif

      if (header.akpath[0] == '\0')
        dh_file->akpath = NULL;
      else {
        dh_file->akpath = (char*)k_alloc(107, strlen(header.akpath) + 1);
        strcpy(dh_file->akpath, header.akpath);
      }

      if (!dh_write_group(dh_file, PRIMARY_SUBFILE, 0, (char*)&header,
                          DH_HEADER_SIZE)) {
        process.status = ER_IOE;
        goto exit_op_fcontrol;
      }
      break;

    case FC_NON_TXN: /* Set file as non-transactional */
      fvar->flags |= FV_NON_TXN;
      break;

    case FC_SPLIT_MERGE: /* Force split/merge */
      if (fvar->type == DYNAMIC_FILE) {
        modulus = fptr->params.modulus;
        load = DHLoad(fptr->params.load_bytes, dh_file->group_size, modulus);

        if ((load > fptr->params.split_load) ||
            (modulus < fptr->params.min_modulus)) {
          dh_split(dh_file);
          result.data.value = TRUE;
        } else if ((load < fptr->params.merge_load) &&
                   (modulus > fptr->params.min_modulus)) {
          /* Looks like we need to merge but check won't immediately split */
          load =
              DHLoad(fptr->params.load_bytes, dh_file->group_size, modulus - 1);
          if (load < fptr->params.split_load) /* Would not immediately split */
          {
            dh_merge(dh_file);
            result.data.value = TRUE;
          }
        }
      }
      break;

    case FC_NO_RESIZE: /* Set/clear DHF_NO_RESIZE flag */
      if (fvar->type == DYNAMIC_FILE) {
        GetInt(descr);
        header_lock = GetGroupWriteLock(dh_file, 0);

        if (!dh_read_group(dh_file, PRIMARY_SUBFILE, 0, (char*)&header,
                           DH_HEADER_SIZE)) {
          process.status = ER_IOE;
          goto exit_op_fcontrol;
        }

        if (descr->data.value) {
          fptr->flags |= DHF_NO_RESIZE;
          header.flags |= DHF_NO_RESIZE;
        } else {
          fptr->flags &= ~DHF_NO_RESIZE;
          header.flags &= ~DHF_NO_RESIZE;
        }

        if (!dh_write_group(dh_file, PRIMARY_SUBFILE, 0, (char*)&header,
                            DH_HEADER_SIZE)) {
          process.status = ER_IOE;
          goto exit_op_fcontrol;
        }

        FreeGroupWriteLock(header_lock);
      }
      break;
  }

exit_op_fcontrol:
  k_dismiss();
  k_dismiss();
  k_dismiss();

  *(e_stack++) = result; /* Move result descriptor to stack */
}

/* ======================================================================
   op_grpstat()  -  Return information about a file group                 */

void op_grpstat() {
  /* Stack:

      |================================|=============================|
      |            BEFORE              |           AFTER             |
      |================================|=============================|
  top |  Group number                  | Information                 |
      |--------------------------------|-----------------------------|
      |  ADDR to file variable         |                             |
      |================================|=============================|

  Returns:
     F1    Bytes used (excluding large records but including group headers)
     F2    Group buffer chain length
     F3    Record count (including large records)
     F4    Large record count
  */

  DESCRIPTOR* descr;
  FILE_VAR* fvar;
  DH_FILE* dh_file;
  FILE_ENTRY* fptr;
  int32_t group;
  int16_t group_bytes;
  int16_t used_bytes;
  char result[50] = "";
  int16_t lock_slot = 0;
  int16_t subfile;
  int32_t grp;
  int16_t rec_offset;
  DH_RECORD* rec_ptr;
  int32_t byte_count = 0;
  int16_t buffer_count = 0;
  int16_t record_count = 0;
  int32_t large_record_count = 0;

  /* Get group number */

  descr = e_stack - 1;
  GetInt(descr);
  group = descr->data.value;

  /* Get file variable */

  descr = e_stack - 2;
  k_get_file(descr);
  fvar = descr->data.fvar;
  if (fvar->type != DYNAMIC_FILE)
    goto exit_op_grpstat;

  dh_file = fvar->access.dh.dh_file;

  fptr = FPtr(dh_file->file_id);
  while (fptr->file_lock < 0)
    Sleep(1000); /* Clearfile in progress */

  /* Lock group */

  StartExclusive(FILE_TABLE_LOCK, 39);
  lock_slot = GetGroupReadLock(dh_file, group);
  EndExclusive(FILE_TABLE_LOCK);

  group_bytes = (int16_t)(dh_file->group_size);

  subfile = PRIMARY_SUBFILE;
  grp = group;
  do {
    /* Read group */

    if (!dh_read_group(dh_file, subfile, grp, dh_buffer, group_bytes)) {
      goto exit_op_grpstat;
    }

    /* Scan group buffer for records */

    used_bytes = ((DH_BLOCK*)dh_buffer)->used_bytes;
    if ((used_bytes == 0) || (used_bytes > group_bytes)) {
      goto exit_op_grpstat;
    }

    buffer_count++;
    byte_count += used_bytes;

    rec_offset = offsetof(DH_BLOCK, record);
    while (rec_offset < used_bytes) {
      rec_ptr = (DH_RECORD*)(dh_buffer + rec_offset);
      record_count++;
      if (rec_ptr->flags & DH_BIG_REC)
        large_record_count++;

      rec_offset += rec_ptr->next;
    }

    /* Move to next group buffer */

    subfile = OVERFLOW_SUBFILE;
    grp = GetFwdLink(dh_file, ((DH_BLOCK*)dh_buffer)->next);
  } while (grp != 0);

  /* Construct return data */

  sprintf(result, "%d%c%d%c%d%c%d", byte_count, FIELD_MARK, (int)buffer_count,
          FIELD_MARK, (int)record_count, FIELD_MARK, large_record_count);

exit_op_grpstat:
  if (lock_slot != 0)
    FreeGroupReadLock(lock_slot);

  k_pop(1);
  k_dismiss();

  /* Set string return value on stack */

  k_put_c_string(result, e_stack++);
}

/* ======================================================================
   op_settrig()  - Set trigger name                                       */

void op_settrig() {
  /* Stack:

      |================================|=============================|
      |            BEFORE              |           AFTER             |
      |================================|=============================|
  top |  Mode flags                    |                             |
      |--------------------------------|-----------------------------|
      |  Function name                 |                             |
      |--------------------------------|-----------------------------|
      |  File variable                 |                             |
      |================================|=============================|
 */

  DESCRIPTOR* descr;
  char call_name[MAX_TRIGGER_NAME_LEN + 1];
  int16_t name_len;
  FILE_VAR* fvar;
  DH_FILE* dh_file;
  DH_HEADER header;
  int modes;

  process.status = 0;

  /* Get mode flags */

  descr = e_stack - 1;
  GetNum(descr);
  modes = descr->data.value;

  /* Get function name */

  descr = e_stack - 2;
  name_len = k_get_c_string(descr, call_name, MAX_TRIGGER_NAME_LEN);
  if ((name_len < 0) || (name_len && !valid_call_name(call_name))) {
    process.status = ER_IID;
    goto exit_op_settrig;
  }

  /* Get file variable */

  descr = e_stack - 3;
  k_get_file(descr);
  fvar = descr->data.fvar;
  if (fvar->type != DYNAMIC_FILE) {
    process.status = ER_NDYN;
    goto exit_op_settrig;
  }

  dh_file = fvar->access.dh.dh_file;

  /* Read file header - We have exclusive access so needn't lock */

  if (!dh_read_group(dh_file, PRIMARY_SUBFILE, 0, (char*)&header,
                     DH_HEADER_SIZE)) {
    process.status = ER_IOE;
    goto exit_op_settrig;
  }

  memset(header.trigger_name, 0,
         MAX_TRIGGER_NAME_LEN); /* Wipe out completely */
  strcpy(header.trigger_name, call_name);
  header.trigger_modes = modes;

  if (!dh_write_group(dh_file, PRIMARY_SUBFILE, 0, (char*)&header,
                      DH_HEADER_SIZE)) {
    process.status = ER_IOE;
    goto exit_op_settrig;
  }

exit_op_settrig:
  k_dismiss();
  k_dismiss();
  k_dismiss();
}

/* END-CODE */
