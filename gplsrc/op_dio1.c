/* OP_DIO1.C
 * Disk i/o opcodes (Create, open and close actions for DH and type1 files)
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
 * 17Oct25 mab Change dyn file prefix to %
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 *
 * 22Feb20 gwb Replaced a pair of sprintf() calls to snprintf() calls in
 *             open_file().
 *             Fixed a variable assigned but never used in open_file()
 * START-HISTORY (OpenQM):
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 07 Dec 06  2.4-17 Added clearance of TRANS file cache to flush_dh_cache().
 * 27 Jun 06  2.4-5 Maintain user/file map table.
 * 30 Jan 06  2.3-5 0458 Pathname longer than MAX_ID_LEN failed in OPENPATH.
 * 30 Aug 05  2.2-9 Track device and inode on NIX.
 * 25 May 05  2.2-0 Set DHF_NOCASE on file table entry for Windows dir file.
 * 06 May 05  2.1-13 Removed raw mode argument from keyin().
 * 11 Mar 05  2.1-8 0323 Moved call to txn_close() so that it happens for
 *                  implicit file closure, not just use of CLOSE.
 * 10 Mar 05  2.1-8 Added support for READONLY flag on open.
 * 29 Oct 04  2.0-9 Added VFS.
 * 26 Oct 04  2.0-7 Use dynamic max_id_len.
 * 20 Sep 04  2.0-2 Use message handler.
 * 20 Sep 04  2.0-2 Use dynamic pcode.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 *  op_close           CLOSE
 *  op_configfl        CONFIGFL
 *  op_create_dh       CREATEDH
 *  op_create_t1       CREATET1
 *  op_open            OPEN
 *  op_openpath        OPENPATH
 *
 * Other externally callable functions
 *  dio_close
 *  get_voc_file_reference
 *  open_file          Common path for op_open and op_openpath
 *  flush_dh_cache     Flush DH file cache (eg prior to file delete)
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include "dh_int.h"
#include "header.h"
#include "locks.h"
#include "syscom.h"

#define MAX_DH_CACHE_SIZE 10
struct DH_CACHE {
  char* pathname;
  DH_FILE* dh_file;
};
Private struct DH_CACHE dh_cache[MAX_DH_CACHE_SIZE];
Private int16_t dh_cache_size = 0;

Private void open_file(bool map_name);

/* ======================================================================
   op_close()  -  Close a file                                            */

void op_close() {
  /* Stack:

      |================================|=============================|
      |            BEFORE              |           AFTER             |
      |================================|=============================|
  top |  File var                      | Status 0=ok, <0 = ON ERROR  |
      |================================|=============================|

     ON ERROR codes are only returned if ONERROR opcode executed prior to
     call to CLOSE.
  */

  DESCRIPTOR* descr;
  /* FILE_VAR* fvar; variable set but never used */

  process.op_flags = 0; /* Currently not used - ON ERROR never happens */

  descr = e_stack - 1;
  while (descr->type == ADDR)
    descr = descr->data.d_addr;
  k_get_file(descr);
  /* fvar = descr->data.fvar; variable set but never used */

  k_release(descr); /* This will close the file */
  k_pop(1);

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = 0; /* Always successful */
}

/* ======================================================================
   op_configfl()  -  Configure DH file                                    */

void op_configfl() {
  /* Stack:

      |================================|=============================|
      |            BEFORE              |           AFTER             |
      |================================|=============================|
  top |  Merge load                    | Status 0=ok, <0 = ON ERROR  |
      |--------------------------------|-----------------------------|
      |  Split load                    |                             |
      |--------------------------------|-----------------------------|
      |  Minimum modulus               |                             |
      |--------------------------------|-----------------------------|
      |  Big record size               |                             |
      |--------------------------------|-----------------------------|
      |  File variable                 |                             |
      |================================|=============================|
 */

  DESCRIPTOR* descr;
  int32_t min_modulus;
  int32_t big_rec_size;
  int16_t merge_load;
  int16_t split_load;
  FILE_VAR* fvar;
  DH_FILE* dh_file;
  int32_t status;

  /* Get DH file parameters */

  descr = e_stack - 1;
  GetInt(descr);
  merge_load = (int16_t)(descr->data.value);

  descr = e_stack - 2;
  GetInt(descr);
  split_load = (int16_t)(descr->data.value);

  descr = e_stack - 3;
  GetInt(descr);
  min_modulus = descr->data.value;

  descr = e_stack - 4;
  GetInt(descr);
  big_rec_size = descr->data.value;

  descr = e_stack - 5;
  k_get_file(descr);
  fvar = descr->data.fvar;

  if (fvar->flags & FV_RDONLY)
    k_error(sysmsg(1400));

  if (fvar->type == DYNAMIC_FILE) {
    dh_file = fvar->access.dh.dh_file;
    dh_configure(dh_file, min_modulus, split_load, merge_load, big_rec_size);
    status = 0;
  } else {
    status = 1;
  }

  k_pop(4);
  k_dismiss();

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = status;
}

/* ======================================================================
   op_create_dh()  -  Create a DH file                                    */

void op_create_dh() {
  /* Stack:

      |================================|=============================|
      |            BEFORE              |           AFTER             |
      |================================|=============================|
  top |  Version                       | Status 0=ok, <0 = ON ERROR  |
      |--------------------------------|-----------------------------|
      |  Flags                         |                             |
      |--------------------------------|-----------------------------|
      |  Merge load                    |                             |
      |--------------------------------|-----------------------------|
      |  Split load                    |                             |
      |--------------------------------|-----------------------------|
      |  Minimum modulus               |                             |
      |--------------------------------|-----------------------------|
      |  Big record size               |                             |
      |--------------------------------|-----------------------------|
      |  Group size                    |                             |
      |--------------------------------|-----------------------------|
      |  Path name                     |                             |
      |================================|=============================|

     ON ERROR codes are only returned if ONERROR opcode executed prior to
     call to CREATE_DH.
  */

  u_int16_t op_flags;

  DESCRIPTOR* descr;
  char path_name[MAX_PATHNAME_LEN + 1];
  int16_t name_len;
  int16_t group_size;
  int32_t min_modulus;
  int32_t big_rec_size;
  int16_t merge_load;
  int16_t split_load;
  u_int16_t creation_flags;
  int16_t version;

  op_flags = process.op_flags;
  process.op_flags = 0;

  process.status = 0;

  /* Get file version */

  descr = e_stack - 1;
  GetInt(descr);
  version = (int16_t)(descr->data.value);

  /* Get file creation flags */

  descr = e_stack - 2;
  GetInt(descr);
  creation_flags = (u_int16_t)((descr->data.value < 0)
                                        ? 0
                                        : (descr->data.value & DHF_CREATE));

  /* Get DH file parameters */

  descr = e_stack - 3;
  GetInt(descr);
  merge_load = (int16_t)(descr->data.value);

  descr = e_stack - 4;
  GetInt(descr);
  split_load = (int16_t)(descr->data.value);

  descr = e_stack - 5;
  GetInt(descr);
  min_modulus = descr->data.value;

  descr = e_stack - 6;
  GetInt(descr);
  big_rec_size = descr->data.value;

  descr = e_stack - 7;
  GetInt(descr);
  group_size = (int16_t)(descr->data.value);

  k_pop(7);

  /* Get path name */

  descr = e_stack - 1;
  name_len = k_get_c_string(descr, path_name, MAX_PATHNAME_LEN);
  k_dismiss();
  if (name_len < 1) {
    process.status = -ER_NAM;
    goto exit_op_create_dh;
  }

  if (!dh_create_file(path_name, group_size, min_modulus, big_rec_size,
                      merge_load, split_load, creation_flags, version)) {
    process.status = -dh_err;
  }

exit_op_create_dh:

  /* Set status code on stack */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = process.status;

  if ((process.status < 0) && !(op_flags & P_ON_ERROR)) {
    k_error(sysmsg(1401), -process.status);
  }
}

/* ======================================================================
   op_create_sh()  -  Create a SH file                                    */

void op_create_sh() {}

/* ======================================================================
   op_create_t1()  -  Create a directory file                             */

void op_create_t1() {
  /* Stack:

      |================================|=============================|
      |            BEFORE              |           AFTER             |
      |================================|=============================|
  top |  Path name                     | Status 0=ok, <0 = ON ERROR  |
      |================================|=============================|

     ON ERROR codes are only returned if ONERROR opcode executed prior to
     call to CREATET1.
  */

  u_int16_t op_flags;

  DESCRIPTOR* descr;
  char path_name[MAX_PATHNAME_LEN + 1];
  int16_t name_len;

  op_flags = process.op_flags;
  process.op_flags = 0;

  process.status = 0;

  /* Get path name */

  descr = e_stack - 1;
  name_len = k_get_c_string(descr, path_name, MAX_PATHNAME_LEN);
  k_dismiss();

  if (name_len <= 0)
    process.status = -ER_NAM;

  if (!make_path(path_name)) {
    process.os_error = OSError;
    process.status = -ER_NOT_CREATED;
  }

  /* Set status code on stack */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = process.status;

  if ((process.status < 0) && !(op_flags & P_ON_ERROR)) {
    k_error(sysmsg(1401), -process.status);
  }
}

/* ======================================================================
   op_open()  -  Open file                                                */

void op_open() {
  /* Stack:

      |================================|=============================|
      |            BEFORE              |           AFTER             |
      |================================|=============================|
  top |  ADDR to file variable         | Status 0=ok, >0 = ELSE      |
      |                                |        <0 = ON ERROR        |
      |--------------------------------|-----------------------------|
      |  VOC name of file              |                             |
      |--------------------------------|-----------------------------|
      |  Dict specifier ("DICT" / "" ) |                             |
      |================================|=============================|

     ON ERROR codes are only returned if ONERROR opcode executed prior to
     call to OPEN.
  */

  open_file(TRUE);
}

/* ======================================================================
   op_openpath()  -  Open file by path name                               */

void op_openpath() {
  /* Stack:

      |================================|=============================|
      |            BEFORE              |           AFTER             |
      |================================|=============================|
  top |  ADDR to file variable         | Status 0=ok, >0 = ELSE      |
      |                                |        <0 = ON ERROR        |
      |--------------------------------|-----------------------------|
      |  Path name of file             |                             |
      |================================|=============================|

     ON ERROR codes are only returned if ONERROR opcode executed prior to
     call to OPEN.
  */

  open_file(FALSE);
}

/* ======================================================================
   dio_close()  -  Close dio file                                         */

void dio_close(FILE_VAR* fvar) {
  int16_t fno;
  FILE_ENTRY* fptr;
  DH_FILE* dh_file;

  fno = fvar->file_id;

  if (fno > 0) {
    fptr = FPtr(fno);

    /* 0323 Handle close in a transaction */

    if ((process.txn_id != 0) && !(fvar->flags & FV_NON_TXN) &&
        (fvar->type != NET_FILE)) {
      /* Do not really close in mid-transaction */
      (void)txn_close(fvar);
      return;
    }

    /* Release all locks on this file */

    if ((abs(fptr->file_lock) == process.user_no) /* We own the file lock */
        && (fptr->fvar_index == fvar->index))     /* For this file var */
    {
      StartExclusive(REC_LOCK_SEM, 54);
      fptr->file_lock = 0;
      clear_waiters(-(fvar->file_id));
      EndExclusive(REC_LOCK_SEM);
    }

    unlock_record(fvar, "", 0);

    /* If file is opened for exclusive access by this process, revert to
       normal shared access                                                */

    if ((fptr->ref_ct < 0) && (fptr->fvar_index == fvar->index)) {
      fptr->ref_ct = 1;
    }
  }

  /* !!FVAR_TYPES!! */
  switch (fvar->type) {
    case DYNAMIC_FILE:
      dh_file = fvar->access.dh.dh_file;
      dh_close(dh_file);

      if (fvar->access.dh.ak_ctrl != NULL)
        k_free(fvar->access.dh.ak_ctrl);
      break;

    case DIRECTORY_FILE:
      StartExclusive(FILE_TABLE_LOCK, 48);
      (fptr->ref_ct)--;
      (*UFMPtr(my_uptr, fno))--;
      EndExclusive(FILE_TABLE_LOCK);
      break;

    case SEQ_FILE:
      close_seq(fvar);
      break;

    case NET_FILE:
      net_close(fvar);
      break;
  }

  if (fvar->voc_name != NULL)
    k_free(fvar->voc_name);

  if (fvar->id != NULL)
    k_free(fvar->id);

  k_free((void*)fvar); /* !!FVAR_DESTROY!! */
}

/* ======================================================================
   Look up file in VOC                                                    */

bool get_voc_file_reference(
    char* voc_name,
    bool get_dict_name,
    char* path) /* Can be same buffer as voc_name argument */
{
  DESCRIPTOR pathname_descr;

  /* Push arguments onto e-stack */

  k_put_c_string(voc_name, e_stack++); /* VOC name of file */

  InitDescr(e_stack, INTEGER); /* Dictionary? */
  (e_stack++)->data.value = get_dict_name;

  InitDescr(e_stack, ADDR); /* File path name (output) */
  (e_stack++)->data.d_addr = &pathname_descr;

  pathname_descr.type = UNASSIGNED;

  k_recurse(pcode_voc_ref, 3); /* Execute recursive code */

  /* Extract result string */
  k_get_c_string(&pathname_descr, path, MAX_PATHNAME_LEN);
  k_release(&pathname_descr);

  return process.status == 0;
}

/* ======================================================================
   open_file()  -  Open file                                              */

Private void open_file(bool map_name) /* Map file name via VOC entry */
{
  /* Stack:

      |================================|=============================|
      |            BEFORE              |           AFTER             |
      |================================|=============================|
  top |  ADDR to file variable         | Status 0=ok, >0 = ELSE      |
      |                                |        <0 = ON ERROR        |
      |--------------------------------|-----------------------------|
      |  Name of file                  |                             |
      |--------------------------------|-----------------------------|
      |  Dict specifier ("DICT" / "" ) |                             |
      |================================|=============================|

     Dictionary specifier is only present for OPEN opcode (map_name = TRUE).

     ON ERROR codes are only returned if ONERROR opcode executed prior to
     call to OPEN.
  */

  DESCRIPTOR* fvar_descr;
  DESCRIPTOR* dict_descr;
  DESCRIPTOR* filename_descr;
  bool open_dict;
  char voc_name[MAX_ID_LEN + 1];
  char mapped_name[MAX_PATHNAME_LEN + 1];
  char pathname[MAX_PATHNAME_LEN + 1];
  int16_t name_len;
  FILE_VAR* fvar = NULL;
  FILE_VAR* cached_fvar;
  DH_FILE* dh_file;
  /* FILE_ENTRY* fptr; variable set but never used */
  u_int16_t op_flags;
  char s[MAX_PATHNAME_LEN + 1];
  int16_t i;
  struct DH_CACHE cache_copy;
  AK_CTRL* ak_ctrl;
  u_int32_t ak_map;
  struct stat statbuf;
  char* server;
  char* remote_file;
  u_int32_t device = 0;
  u_int32_t inode = 0;

  op_flags = process.op_flags;
  process.op_flags = 0;

  process.status = 0;

  /* Find the file variable descriptor */

  fvar_descr = e_stack - 1;
  do {
    fvar_descr = fvar_descr->data.d_addr;
  } while (fvar_descr->type == ADDR);

  k_dismiss(); /* Dismiss ADDR */
  k_release(fvar_descr);

  /* Get file name */

  filename_descr = e_stack - 1;

  if (map_name) {
    name_len = k_get_c_string(filename_descr, voc_name, sysseg->maxidlen);
    k_dismiss();

    /* Get DICT specifier */

    dict_descr = e_stack - 1;
    (void)k_get_c_string(dict_descr, s, 4);
    k_dismiss();
    open_dict = (stricmp(s, "DICT") == 0);
  } else {
    name_len = k_get_c_string(filename_descr, mapped_name,
                              MAX_PATHNAME_LEN); /* 0458 */
    k_dismiss();
  }

  if (name_len < 1) {
    process.status = ER_NAM;
    goto exit_op_open; /* We failed to extract the name */
  }

  /* Create a FILE_VAR structure for this file. We will remove it later if
     the open fails for any reason.                                        */

  fvar = (FILE_VAR*)k_alloc(30, sizeof(struct FILE_VAR)); /* !!FVAR_CREATE!! */
  if (fvar == NULL) {
    process.status = -ER_MEM;
    goto exit_op_open;
  }

  fvar->type = INITIAL_FVAR;
  fvar->ref_ct = 1;
  fvar->index = next_fvar_index++;
  fvar->voc_name = NULL;
  fvar->id_len = 0;
  fvar->id = NULL;
  fvar->file_id = -1;
  fvar->flags = 0;

  if (map_name) {
    /* Insert VOC name of file (remains NULL if not mapping) */

    if ((fvar->voc_name = (char*)k_alloc(6, strlen(voc_name) + 1)) == NULL) {
      process.status = -ER_MEM;
      goto exit_op_open;
    }
    strcpy(fvar->voc_name, voc_name);

    /* Map name via the VOC */

    if (!get_voc_file_reference(voc_name, open_dict, mapped_name)) {
      /* process.status will have been set by recursive code */
      goto exit_op_open;
    }

    if (strchr(mapped_name, ';') != NULL) /* This is a network file reference */
    {
      server = strtok(mapped_name, ";");
      remote_file = strtok(NULL, "\0");

      if (!net_open(server, remote_file, fvar)) {
        /* process.status will have been set by open_networked_file() */
        goto exit_op_open;
      }
      goto opened_network_file;
    }
  }

  fullpath(pathname, mapped_name);

  if (process.txn_id != 0) {
    /* Try to re-open a file previously closed during this transaction */

    if ((cached_fvar = txn_open(pathname)) != NULL) {
      if (cached_fvar->voc_name != NULL)
        k_free(cached_fvar->voc_name);
      cached_fvar->voc_name = fvar->voc_name; /* Possibly change name */
      k_free(fvar);                           /* Release new fvar... */
      fvar = cached_fvar;                     /* ...replacing by cached one */
      if (fvar->type == DYNAMIC_FILE) {
        dh_file = fvar->access.dh.dh_file;
        process.inmat =
            dh_modulus(dh_file); /* Set INMAT() to current modulus */
      }
      goto opened_via_txn_cache;
    }
  }

  /* Is this a cached DH file? */

  for (i = 0; i < dh_cache_size; i++) {
    if (strcmp(pathname, dh_cache[i].pathname) == 0) {
      /* File is available via the DH file cache */

      dh_file = dh_cache[i].dh_file;
      /* fptr = FPtr(dh_file->file_id); assigned but never used */
      dh_file->open_count++;
      process.inmat = dh_modulus(dh_file); /* Set INMAT() to current modulus */
      fvar->type = DYNAMIC_FILE;
      fvar->access.dh.dh_file = dh_file;

      /* Move entry to head of cache */

      if (i > 0) {
        cache_copy = dh_cache[i];
        while (i > 0) {
          dh_cache[i] = dh_cache[i - 1];
          i--;
        }
        dh_cache[0] = cache_copy;
      }

      goto opened_cached_file;
    }
  }

  /* Check that the file exists (ie the directory is present) */

  if ((stat(pathname, &statbuf) != 0) /* No such directory */
      || (!(statbuf.st_mode & S_IFDIR))) {
    process.os_error = OSError;
    process.status = ER_FNF;
    goto exit_op_open;
  }

  device = statbuf.st_dev;
  inode = statbuf.st_ino;

  /* Check if it is a remote file */

  /* !LINUX! There appears to be no reliable way to do this */

  /* Determine file type by examination of the directory. This should contain
     a file named %0 if it is a DH file. Otherwise assume it to be directory. */

  if (pathname[strlen(pathname) - 1] == DS) {
     /* converted to snprintf() -gwb 22Feb20 */
	 // 17Oct25 mab Change dyn file prefix to %
    if (snprintf(s, MAX_PATHNAME_LEN + 1, ""%s%%0"", pathname) >= (MAX_PATHNAME_LEN + 1)) {
       /* TODO: this error should be sent out to a log file with more info */
       k_error("Overflow of path/filename max lengthn in open_file()");
       goto exit_op_open;
    }
  } else
     /* converted to snprintf() - gwb 22Feb20 */
     // 17Oct25 mab Change dyn file prefix to %
    if (snprintf(s, MAX_PATHNAME_LEN + 1,"%s%c%%0", pathname, DS) >= (MAX_PATHNAME_LEN + 1)) {
       /* TODO: this error should be sent out to a log file with more info */
       k_error("Overflow of path/filename max lengthn in open_file()");
       goto exit_op_open;
    }

  if (access(s, 0) == 0) /* It's a DH file */
  {
    /* Open DH file */

    dh_file = dh_open(pathname);
    if (dh_file == NULL) {
      switch (dh_err) {
        case DHE_EXCLUSIVE: /* Cannot gain exclusive access */
          process.status = ER_EXCLUSIVE;
          break;
        case DHE_OPEN_NO_MEMORY: /* No memory for DH_FILE structure */
          process.status = -ER_MEM;
          break;
        case DHE_FILE_NOT_FOUND: /* Cannot open primary subfile */
          process.status = -ER_SFNF;
          break;
        case DHE_INVA_FILE_NAME: /* Invalid file name */
          process.status = -ER_NAM;
          break;
        default:
          process.status = -dh_err;
          break;
      }

      goto exit_op_open;
    }

    /* Set up file variable */

    fvar->type = DYNAMIC_FILE;
    fvar->access.dh.dh_file = dh_file;

    if (map_name) {
      /* Add file to DH cache */

      if (dh_cache_size == MAX_DH_CACHE_SIZE) {
        /* Replace oldest entry */

        i = dh_cache_size - 1;
        dh_close(dh_cache[i].dh_file);
        k_free(dh_cache[i].pathname);
      } else /* Add new entry to end of cache */
      {
        i = dh_cache_size++;
      }

      if ((dh_cache[i].pathname = (char*)k_alloc(31, strlen(pathname) + 1)) ==
          NULL) {
        dh_cache_size--;
        process.status = -ER_MEM;
        goto exit_op_open;
      }

      strcpy(dh_cache[i].pathname, pathname);
      dh_cache[i].dh_file = dh_file;
      dh_file->open_count++;
    }

  opened_cached_file:

    fvar->file_id = dh_file->file_id;
    if ((op_flags & P_READONLY) || (dh_file->flags & DHF_RDONLY)) {
      fvar->flags |= FV_RDONLY;
    }

    if ((ak_map = dh_file->ak_map) == 0) {
      fvar->access.dh.ak_ctrl = NULL;
    } else {
      /* Find highest AK number to determine size of AK_INFO table */

      for (i = 31; i > 0; i--) {
        if ((ak_map >> i) & 1)
          break;
      }
      i++;

      ak_ctrl = (AK_CTRL*)k_alloc(58, AkCtrlSize(i));
      fvar->access.dh.ak_ctrl = ak_ctrl;
      while (i-- > 0) {
        ak_ctrl->ak_scan[i].upd = 0;
        ak_ctrl->ak_scan[i].key_len = 0; /* Just for tracer */
      }
    }

    /* Set INMAT() to current modulus */

    process.inmat = dh_modulus(dh_file);
  } else /* Open as a directory file */
  {
    fvar->type = DIRECTORY_FILE;
    fvar->access.dir.mark_mapping = TRUE;
    fvar->file_id = get_file_entry(pathname, device, inode, NULL);
    if (dh_err) /* Failed to allocate entry */
    {
      process.status = -dh_err;
      goto exit_op_open;
    }
    if (op_flags & P_READONLY)
      fvar->flags |= FV_RDONLY;
  }

opened_via_txn_cache:
opened_network_file:

  /* Set file variable */

  InitDescr(fvar_descr, FILE_REF);
  fvar_descr->data.fvar = fvar;

exit_op_open:

  if (process.status) /* Failed to open */
  {
    if (fvar != NULL)
      dio_close(fvar);
  }

  /* Set status code on stack */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = process.status;

  if ((process.status < 0) && !(op_flags & P_ON_ERROR)) {
    k_error(sysmsg(1402), -process.status);
  }
}

/* ======================================================================
   op_readonly()  -  READONLY prefix opcode                               */

void op_readonly() {
  process.op_flags |= P_READONLY;
}

/* ======================================================================
   flush_dh_cache()  -  Flush the DH cache                                */

void flush_dh_cache() {
  int i;
  int n;
  DESCRIPTOR* descr;
  ARRAY_HEADER* ahdr;

  for (i = 0; i < dh_cache_size; i++) {
    dh_close(dh_cache[i].dh_file);
    k_free(dh_cache[i].pathname);
  }

  dh_cache_size = 0;

  /* Also clear TRANS file cache */

  descr = Element(process.syscom, SYSCOM_TRANS_FILES);
  k_release(descr);
  InitDescr(descr, STRING);
  descr->data.str.saddr = NULL;

  descr = Element(process.syscom, SYSCOM_TRANS_FVARS);
  if (descr->type == ARRAY) {
    ahdr = descr->data.ahdr_addr;
    n = ahdr->used_elements;
    for (i = 0; i < n; i++) {
      k_release(Element(ahdr, i));
    }
  }
}

/* END-CODE */
