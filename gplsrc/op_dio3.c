/* OP_DIO3.C
 * Disk i/o opcodes (Read, write and delete actions for DH and type1 files)
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
 * START-HISTORY (ScarletDME):
 * 06Feb22 gwb Initialized a char array in read_record() in order to clear a warning
 *             reported by valgrind.  Reformatted code.
 * 
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 *
 * 22Feb20 gwb Converted sprintf() to snprintf() in op_delete(), op_readv(),
 *             read_record(), and dir_write().
 * 
 * START-HISTORY (OpenQM):
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 19 Jun 07  2.5-7 k_call() now has additional argument for stack adjustment.
 * 02 Jan 07  2.4-19 Added trigger name to message 1409.
 * 28 Nov 06  2.4-17 Added o/s error to messages 1406 and 1407.
 * 21 Nov 06  2.4-17 Revised interface to dio_open().
 * 29 Jun 06  2.4-5 Added lock_beep() calls.
 * 20 Apr 06  2.4-2 0480 Reworked QMNet interface to use SV_xxx status flags.
 * 12 Apr 06  2.4-1 Added non-transactional flag to FILE_VAR.
 * 04 Apr 06  2.4-1 Trap use of char(0) in ids in valid_id().
 * 29 Mar 06  2.3-9 Added pre/post clearfile triggers.
 * 09 Mar 06  2.3-8 Added ability to disable directory file id mapping.
 * 09 Mar 06  2.3-8 Added mark mapping control for QMNet files.
 * 08 Mar 06  2.3-8 Added check for SSF_SUSPEND.
 * 08 Mar 06  2.3-8 Set PF_IN_TRIGGER too when starting trigger.
 * 12 Dec 05  2.3-2 0439 READV was taking THEN clause if record did not exist
 *                  when a non-zero field number was used.
 * 01 Dec 05  2.2-18 READVU for field 0 was returning ER$LCK instead of user
 *                   number in STATUS().
 * 26 Oct 05  2.2-15 0427 Trap permissions related errors when writing or
 *                   deleting a record in a directory file.
 * 14 Sep 05  2.2-10 0408 Use of reserved names CON, COMn, LPTn as directory
 *                   file record ids on Windows crashes QM.
 * 25 Aug 05  2.2-8 Reject writes of records with mark characters in their id.
 * 29 Jun 05  2.2-1 0369 read_record() was not accepting PMATRIX for MATREAD.
 * 16 Mar 05  2.1-10 Added support for PICKREAD mode.
 * 04 Jan 05  2.1-0 0295 Set string pointer correctly for net_write().
 * 02 Nov 04  2.0-9 Added VFS.
 * 26 Oct 04  2.0-7 Allow pre-write trigger to update data.
 * 26 Oct 04  2.0-7 Use dynamic max_id_len.
 * 21 Oct 04  2.0-6 0266 op_readv() was not working for QMNet connections.
 * 11 Oct 04  2.0-5 Added post-delete and post-write triggers.
 * 20 Sep 04  2.0-2 Use message handler.
 * 20 Sep 04  2.0-2 Use dynamic pcode loader.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 *  op_clrfile         CLRFILE
 *  op_delete          DELETE
 *  op_mapmarks        MAPMARKS
 *  op_matread         MATREAD
 *  op_read            READ
 *  op_readv           READV
 *  op_write           WRITE
 *  op_writev          WRITEV
 *
 *  dir_write          Write record to directory file
 *
 * Private functions
 *  t1_buffer_alloc    Allocate working buffer for directory file handling
 *  t1_write           Write data to directory file
 *  t1_flush           Flush directory file working buffer
 *  t1_buffer_free     Free working buffer for directory file handling
 *  read_record        Read/Matread common path
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include "dh_int.h"
#include "syscom.h"
#include "header.h"
#include "config.h"
#include "locks.h"
#include "qmclient.h"

#define MAX_T1_BUFFER_SIZE 31744
Private char *t1_buffer = NULL;
Private int t1_buffer_size;
Private int16_t t1_space;
Private char *t1_ptr;
Private OSFILE t1_fu;

void t1in(char *src, char *dst, int16_t inct, int16_t *outct);

void op_write(void); /* Necessary for internal call */
void op_dspnl(void);
void op_matparse(void);

Private void t1_buffer_alloc(int32_t size);
Private bool t1_write(char *p, int16_t bytes);
Private bool t1_flush(void);
Private void t1_buffer_free(void);
Private void read_record(bool matread);
Private bool valid_id(char *id, int16_t id_len);

/* ======================================================================
   op_clrfile()  -  Clear File                                            */

void op_clrfile() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  File variable              | Status 0=ok, <0 = ON ERROR  |
     |=============================|=============================|
                    
    ON ERROR codes are only returned if ONERROR opcode executed prior to
    call to CLRFILE.
 */

  DESCRIPTOR *fvar_descr;
  FILE_VAR *fvar;
  char pathname[MAX_PATHNAME_LEN + 1];
  int path_len;
  char subfilename[MAX_PATHNAME_LEN + 1];
  DIR *dfu;
  struct dirent *dp;
  u_int16_t op_flags;
  FILE_ENTRY *fptr;
  DH_FILE *dh_file;
  bool took_lock = FALSE; /* Did we aquire lock here? */

  op_flags = process.op_flags;
  process.op_flags = 0;

  process.status = 0;

  if (sysseg->flags & SSF_SUSPEND)
    suspend_updates();

  /* Find the file variable descriptor */

  fvar_descr = e_stack - 1;
  k_get_file(fvar_descr);
  fvar = fvar_descr->data.fvar;

  if ((process.txn_id != 0) && !(fvar->flags & FV_NON_TXN))
    k_txn_error();

  if (fvar->flags & FV_RDONLY)
    k_error(sysmsg(1403));

  if (fvar->type == NET_FILE) {
    process.status = net_clearfile(fvar);
  } else {
    /* Get exclusive access to the file_lock entry in the file table. Because
      acquisition of a file lock may require scanning the record lock table,
      the file_lock entry is protected by the REC_LOCK_SEM. The file table
      entry cannot go away while we are looking at it because we have the
      file open.                                                              */

    StartExclusive(REC_LOCK_SEM, 3);

    /* Wait if file lock held elsewhere. */

    fptr = FPtr(fvar->file_id);
    if (fptr->file_lock != process.user_no) {
      if (fptr->file_lock != 0) {
        EndExclusive(REC_LOCK_SEM);
        pc--; /* Back up to repeat opcode */
        if (my_uptr->events)
          process_events();
        Sleep(500);
        return;
      }
      took_lock = TRUE;
    }

    fptr->file_lock = -process.user_no; /* Mark clearfile in progress */
    if (fptr->ref_ct > 1)
      Sleep(1000); /* Ensure all activity finished */
    EndExclusive(REC_LOCK_SEM);

    /* Clear the file */

    (fptr->upd_ct)++;
    switch (fvar->type) /* !!FVAR_TYPES!! */
    {
      case DYNAMIC_FILE:
        dh_file = fvar->access.dh.dh_file;

        /* Call pre-clearfile trigger function if required */

        if (dh_file->trigger_modes & TRG_PRE_CLEAR) {
          if (!call_trigger(fvar_descr, TRG_PRE_CLEAR, NULL, NULL, op_flags & P_ON_ERROR, FALSE)) {
            process.status = -ER_TRIGGER;
            goto exit_op_clrfile;
          }
        }

        if (!dh_clear(dh_file)) {
          process.status = -dh_err;
          goto exit_op_clrfile;
        }

        /* Call post-clearfile trigger function if required */

        if (dh_file->trigger_modes & TRG_POST_CLEAR) {
          if (!call_trigger(fvar_descr, TRG_POST_CLEAR, NULL, NULL, op_flags & P_ON_ERROR, FALSE)) {
            process.status = -ER_TRIGGER;
            goto exit_op_clrfile;
          }
        }
        break;

      case DIRECTORY_FILE:
        strcpy(pathname, (char *)(fptr->pathname));
        if ((dfu = opendir(pathname)) == NULL) {
          process.status = -ER_RNF;
          goto exit_op_clrfile;
        }

        path_len = strlen(pathname);
        if (pathname[path_len - 1] == DS)
          pathname[path_len - 1] = '\0';

        while ((dp = readdir(dfu)) != NULL) {
          if (strcmp(dp->d_name, ".") == 0)
            continue;
          if (strcmp(dp->d_name, "..") == 0)
            continue;
          /* converted to snprintf() -gwb 22Feb20 */
          if (snprintf(subfilename, MAX_PATHNAME_LEN + 1, "%s%c%s", pathname, DS, dp->d_name) >= (MAX_PATHNAME_LEN + 1)) {
            /* TODO: this should also be logged with more detail */
            k_error("Overflowed path/filename length in op_clrfile()!");
            goto exit_op_clrfile;
          }
          remove(subfilename);
        }
        break;
    }

  exit_op_clrfile:

    if (took_lock) {
      StartExclusive(REC_LOCK_SEM, 55);
      fptr->file_lock = 0; /* Release special lock */
      clear_waiters(-(fvar->file_id));
      EndExclusive(REC_LOCK_SEM);
    } else {
      fptr->file_lock = process.user_no; /* Revert to normal lock */
    }
  }

  k_dismiss();

  /* Set status code on stack */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = process.status;

  if (process.status && !(op_flags & P_ON_ERROR)) {
    k_error(sysmsg(1404), -process.status);
  }
}

/* ======================================================================
   op_delete()  -  Delete record                                          */

void op_delete() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Record id                  | Status 0=ok, <0 = ON ERROR  |
     |-----------------------------|-----------------------------| 
     |  File variable              |                             |
     |=============================|=============================|
                    
    ON ERROR codes are only returned if ONERROR opcode executed prior to
    call to DELETE.
 */

  DESCRIPTOR *fvar_descr;
  DESCRIPTOR *rid_descr;
  char id[MAX_ID_LEN + 1];
  char mapped_id[2 * MAX_ID_LEN + 1];
  int16_t id_len;
  FILE_VAR *fvar;
  DH_FILE *dh_file;
  u_int16_t op_flags;
  char subfilename[MAX_PATHNAME_LEN + 1];
  char pathname[MAX_PATHNAME_LEN + 1];
  int16_t path_len;
  FILE_ENTRY *fptr;
  bool keep_lock;
  u_int32_t txn_id;
  struct stat statbuf;

  op_flags = process.op_flags;
  process.op_flags = 0;
  keep_lock = (op_flags & P_ULOCK) != 0;

  process.status = 0;

  /* Get record id */

  rid_descr = e_stack - 1;
  id_len = k_get_c_string(rid_descr, id, sysseg->maxidlen);
  if (id_len <= 0) {
    process.status = -ER_IID;
    goto exit_op_delete; /* We failed to extract the record id */
  }

  /* Find the file variable descriptor */

  fvar_descr = e_stack - 2;
  k_get_file(fvar_descr);
  fvar = fvar_descr->data.fvar;

  txn_id = (fvar->flags & FV_NON_TXN) ? 0 : process.txn_id;

  if ((sysseg->flags & SSF_SUSPEND) && (txn_id == 0))
    suspend_updates();

  /* Check for read only file */

  if (fvar->flags & FV_RDONLY) {
    process.status = -ER_RDONLY;
    log_permissions_error(fvar);
    goto exit_op_delete;
  }

  if (pcfg.must_lock || txn_id) {
    if (!check_lock(fvar, id, id_len)) {
      process.status = -ER_NOLOCK;
      goto exit_op_delete;
    }
  }

  /* Delete the record */

  switch (fvar->type) /* !!FVAR_TYPES!! */
  {
    case DYNAMIC_FILE:
      dh_file = fvar->access.dh.dh_file;

      /* Call pre-delete trigger function if required */

      if (dh_file->trigger_modes & TRG_PRE_DELETE) {
        if (!call_trigger(fvar_descr, TRG_PRE_DELETE, rid_descr, NULL, op_flags & P_ON_ERROR, FALSE)) {
          process.status = -ER_TRIGGER;
          goto exit_op_delete;
        }
      }

      if (txn_id != 0) {
        txn_delete(fvar, id, id_len);
      } else {
        if ((!dh_delete(dh_file, id, id_len)) && (dh_err != DHE_RECORD_NOT_FOUND)) {
          process.status = -dh_err;
        }
      }

      /* Call post-delete trigger function if required */

      if (dh_file->trigger_modes & TRG_POST_DELETE) {
        if (!call_trigger(fvar_descr, TRG_POST_DELETE, rid_descr, NULL, op_flags & P_ON_ERROR, FALSE)) {
          process.status = -ER_TRIGGER;
          goto exit_op_delete;
        }
      }

      break;

    case DIRECTORY_FILE:
      fptr = FPtr(fvar->file_id);
      if (!map_t1_id(id, id_len, mapped_id)) {
        process.status = -ER_IID;
        goto exit_op_delete; /* Illegal record id */
      }

      if (txn_id != 0) {
        txn_delete(fvar, id, id_len);
      } else {
        /* Increment statistics and transaction counters */

        StartExclusive(FILE_TABLE_LOCK, 50);
        sysseg->global_stats.deletes++;
        fptr->upd_ct++;
        EndExclusive(FILE_TABLE_LOCK);

        strcpy(pathname, (char *)(fptr->pathname));
        path_len = strlen(pathname);
        if (pathname[path_len - 1] == DS)
          pathname[path_len - 1] = '\0'; /* 0214 */
        /* converted to snprintf() -gwb 22Feb20 */
        if (snprintf(subfilename, MAX_PATHNAME_LEN + 1, "%s%c%s", pathname, DS, mapped_id) >= (MAX_PATHNAME_LEN + 1)) {
          /* TODO: this should also be logged with more detail */
          k_error("Overflowed path/filename length in op_delete()!");
          goto exit_op_delete;
        }

        /* 0408 Check that this really is a file, not CON, COMn, LPTn */

        if (!stat(subfilename, &statbuf) && !(statbuf.st_mode & S_IFREG)) {
          process.status = -ER_IID;
          goto exit_op_delete;
        }

        if (remove(subfilename) < 0) /* 0427 */
        {
          process.os_error = errno;
          if (process.os_error != ENOENT) {
            process.status = -ER_PERM;
            log_permissions_error(fvar);
            goto exit_op_delete;
          }
        }
      }
      break;

    case NET_FILE:
      net_delete(fvar, id, id_len, keep_lock);
      break;
  }

exit_op_delete:
  k_dismiss();
  k_dismiss();

  /* Set status code on stack */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = process.status;

  if (process.status >= 0) {
    /* Release record lock if non-transactional DELETE */
    if (!keep_lock && (txn_id == 0) && (id_len != 0) && (fvar->type != NET_FILE)) {
      unlock_record(fvar, id, id_len);
    }
  } else if (!(op_flags & P_ON_ERROR))
    k_error(sysmsg(1405), process.status);
}

/* ======================================================================
   op_matread()  -  Read record to matrix                                 */

void op_matread() {
  read_record(TRUE);
}

/* ======================================================================
   op_read()  -  Read record                                              */

void op_read() {
  read_record(FALSE);
}

/* ======================================================================
   op_readv()  -  Read specific field from a record                       */

void op_readv() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Field number               | Status 0=ok, >0 = ELSE      |
     |                             |        <0 = ON ERROR        |
     |-----------------------------|-----------------------------| 
     |  Record id                  |                             |
     |-----------------------------|-----------------------------| 
     |  File variable              |                             |
     |-----------------------------|-----------------------------| 
     |  ADDR to target             |                             |
     |=============================|=============================|
                    
    ON ERROR codes are only returned if ONERROR opcode executed prior to
    call to READV.
 */

  DESCRIPTOR *descr;
  DESCRIPTOR *rid_descr;
  char id[MAX_ID_LEN + 1];
  char mapped_id[2 * MAX_ID_LEN + 1];
  char actual_id[MAX_ID_LEN + 1];
  int32_t field_no;
  char subfilename[MAX_PATHNAME_LEN + 1];
  char pathname[MAX_PATHNAME_LEN + 1];
  int16_t path_len;
  int16_t id_len;
  FILE_VAR *fvar;
  DH_FILE *dh_file;
  u_int16_t op_flags;
  STRING_CHUNK *str;
  int32_t status = 0;
  u_int32_t txn_id;

  process.status = 0;

  op_flags = process.op_flags;
  process.op_flags = 0;

  /* Get field number */

  descr = e_stack - 1;
  GetInt(descr);
  field_no = descr->data.value;

  /* Find the file variable descriptor */

  descr = e_stack - 3;
  k_get_file(descr);
  fvar = descr->data.fvar;
  txn_id = (fvar->flags & FV_NON_TXN) ? 0 : process.txn_id;

  if ((field_no != 0) && (fvar->type != NET_FILE) && (fvar->type != VFS_FILE)) {
    /* Reading a field of a local file */

    /* Push lock flag onto e-stack. This corresponds to the lock bits of
      the op_flags opcode prefix value.                                 */

    InitDescr(e_stack, INTEGER);
    (e_stack++)->data.value = op_flags;

    k_recurse(pcode_readv, 5);        /* Execute recursive code */
    status = (--e_stack)->data.value; /* 0439 */
  } else {
    /* Get record id */

    rid_descr = e_stack - 2;
    id_len = k_get_c_string(rid_descr, id, sysseg->maxidlen);

    /* Get target descr */

    descr = e_stack - 4;
    do {
      descr = descr->data.d_addr;
    } while (descr->type == ADDR);

    if (!(op_flags & P_PICKREAD)) {
      k_release(descr);
      InitDescr(descr, STRING);
      descr->data.str.saddr = NULL;
    }

    if (id_len <= 0) {
      status = process.status = ER_IID;
    } else {
      if (fvar->type == NET_FILE) {
        switch (net_readv(fvar, id, id_len, field_no, op_flags, &str)) {
          case SV_OK: /* Record exists. Take THEN clause */
            k_release(descr);
            InitDescr(descr, STRING);
            descr->data.str.saddr = str;
            status = 0;
            break;
          case SV_LOCKED:
            if (my_uptr->events)
              process_events();
            process.op_flags = op_flags;
            pc = op_pc;
            lock_beep();
            Sleep(250);
            return;
          case SV_ON_ERROR: /* Fatal error */
            status = -1;
            break;
          default: /* Take ELSE clause */
            status = 1;
            break;
        }
      } else {
        if (fvar->type == DIRECTORY_FILE) {
          if (!map_t1_id(id, id_len, mapped_id)) {
            status = process.status = ER_IID;
            goto exit_op_readv; /* Illegal record id */
          }
        }

        /* Aquire lock if necessary */

        if (op_flags & P_REC_LOCKS) /* READL or READU */
        {
          process.status = lock_record(fvar, id, id_len, (op_flags & P_ULOCK) != 0, txn_id, (op_flags & P_LOCKED) != 0);
          switch (process.status) {
            case 0: /* Got the lock */
              break;

            case -2: /* Deadlock detected */
              if (sysseg->deadlock)
                k_deadlock();
              /* **** FALL THROUGH **** */

            case -1:                   /* Lock table is full */
            default:                   /* Conflicting lock is held by another user */
              if (op_flags & P_LOCKED) /* LOCKED clause present */
              {
                status = ER_LCK;
                goto exit_op_readv;
              }
              if (my_uptr->events)
                process_events();
              process.op_flags = op_flags;
              pc = op_pc;
              lock_beep();
              Sleep(250);
              return;
          }
        }

        /* Read the record */

        if (txn_id && (fvar->type != NET_FILE)) {
          switch (txn_read(fvar, id, id_len, actual_id, &str)) {
            case TXC_FOUND: /* Found the record */
              goto exit_op_readv;

            case TXC_DELETED: /* Reference found as deleted record */
              status = process.status = ER_RNF;
              goto exit_op_readv;
          }
        }

        switch (fvar->type) /* !!FVAR_TYPES!! */
        {
          case DYNAMIC_FILE:
            dh_file = fvar->access.dh.dh_file;
            if (!dh_exists(dh_file, id, id_len)) {
              if (dh_err == DHE_RECORD_NOT_FOUND)
                process.status = ER_RNF;
              else
                process.status = -dh_err;
              status = process.status;
            }
            break;

          case DIRECTORY_FILE:
            strcpy(pathname, (char *)(FPtr(fvar->file_id)->pathname));
            path_len = strlen(pathname);
            if (pathname[path_len - 1] == DS)
              pathname[path_len - 1] = '\0'; /* 0214 */
            /* converted to snprintf() -gwb 22Feb20 */
            if (snprintf(subfilename, MAX_PATHNAME_LEN + 1, "%s%c%s", pathname, DS, mapped_id) >= (MAX_PATHNAME_LEN + 1)) {
              /* TODO: this should also be logged with more detail */
              k_error("Overflowed path/filename length in op_readv()!");
              goto exit_op_readv;
            }
            if ((mapped_id[0] == '\0') || access(subfilename, 0)) {
              status = process.status = ER_RNF;
            }
            break;
        }
      }
    }

    if ((status == 0) && (field_no == 0)) {
      k_release(descr);
      k_put_string(id, id_len, descr);
    }

  exit_op_readv:
    k_pop(1);    /* Dismiss field number */
    k_dismiss(); /* Record id */
    k_dismiss(); /* File var */
    k_dismiss(); /* Target */
  }

  /* Set status code on stack */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = status;

  if ((status < 0) && !(op_flags & P_ON_ERROR)) {
    k_error(sysmsg(1406), -process.status, process.os_error);
  }
}

/* ======================================================================
   op_mapmarks()  -  Perform mark mapping in directory files?             */

void op_mapmarks() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  New status                 |                             |
     |-----------------------------|-----------------------------| 
     |  File variable              |                             |
     |=============================|=============================|

     This operation is ignored if not a directory file.                    
 */

  DESCRIPTOR *descr;
  FILE_VAR *fvar;
  bool state;

  /* Get file variable */

  descr = e_stack - 2;
  k_get_file(descr);
  fvar = descr->data.fvar;

  /* Get new setting */

  descr = e_stack - 1;
  GetInt(descr);
  state = (descr->data.value != 0);

  switch (fvar->type) {
    case DIRECTORY_FILE:
      fvar->access.dir.mark_mapping = state;
      break;

    case NET_FILE:
      net_mark_mapping(fvar, state);
      break;
  }

  k_pop(1);
  k_dismiss();
}

/* ======================================================================
   op_write()  -  Write record                                            */

void op_write() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Record id                  | Status 0=ok, <0 = ON ERROR  |
     |-----------------------------|-----------------------------| 
     |  File variable              |                             |
     |-----------------------------|-----------------------------| 
     |  Record data to write       |                             |
     |=============================|=============================|
                    
    ON ERROR codes are only returned if ONERROR opcode executed prior to
    call to WRITE.
 */

  DESCRIPTOR *fvar_descr;
  DESCRIPTOR *rid_descr;
  DESCRIPTOR *src_descr;
  char id[MAX_ID_LEN + 1];
  char lock_id[MAX_ID_LEN + 1];
  char mapped_id[2 * MAX_ID_LEN + 1];
  int16_t id_len;
  FILE_VAR *fvar;
  DH_FILE *dh_file;
  STRING_CHUNK *str;
  u_int16_t op_flags;
  bool keep_lock;
  u_int32_t txn_id;

  op_flags = process.op_flags;
  process.op_flags = 0;
  keep_lock = (op_flags & P_ULOCK) != 0;

  process.status = 0;

  /* Get record id */

  rid_descr = e_stack - 1;
  id_len = k_get_c_string(rid_descr, id, sysseg->maxidlen);

  /* Find the file variable descriptor */

  fvar_descr = e_stack - 2;
  k_get_file(fvar_descr);
  fvar = fvar_descr->data.fvar;

  txn_id = (fvar->flags & FV_NON_TXN) ? 0 : process.txn_id;

  if ((sysseg->flags & SSF_SUSPEND) && (txn_id == 0))
    suspend_updates();

  /* Get string descr */

  src_descr = e_stack - 3;
  k_get_string(src_descr);

  if (!valid_id(id, id_len)) {
    process.status = -ER_IID;
    goto exit_op_write; /* We failed to extract the record id */
  }

  /* Check for read only file */

  if (fvar->flags & FV_RDONLY) {
    process.status = -ER_RDONLY;
    log_permissions_error(fvar);
    goto exit_op_write;
  }

  memcpy(lock_id, id, id_len);

  if (pcfg.must_lock || (txn_id != 0)) {
    if (!check_lock(fvar, lock_id, id_len)) {
      process.status = -ER_NOLOCK;
      goto exit_op_write;
    }
  }

  /* Write the record */

  switch (fvar->type) /* !!FVAR_TYPES!! */
  {
    case DYNAMIC_FILE:
      dh_file = fvar->access.dh.dh_file;

      /* Call pre-write trigger function if required */

      if (dh_file->trigger_modes & TRG_PRE_WRITE) {
        if (!call_trigger(fvar_descr, TRG_PRE_WRITE, rid_descr, src_descr, (op_flags & P_ON_ERROR), TRUE)) {
          process.status = -ER_TRIGGER;
          goto exit_op_write;
        }
      }

      str = src_descr->data.str.saddr;
      if (txn_id != 0) {
        if (!txn_write(fvar, id, id_len, str))
          goto exit_op_write;
      } else {
        if (!dh_write(dh_file, id, id_len, str)) {
          process.status = -dh_err;
          goto exit_op_write;
        }
      }

      /* Call post-write trigger function if required */

      if (dh_file->trigger_modes & TRG_POST_WRITE) {
        if (!call_trigger(fvar_descr, TRG_POST_WRITE, rid_descr, src_descr, (op_flags & P_ON_ERROR), FALSE)) {
          process.status = -ER_TRIGGER;
          goto exit_op_write;
        }
      }
      break;

    case DIRECTORY_FILE:
      str = src_descr->data.str.saddr;
      if (!map_t1_id(id, id_len, mapped_id)) {
        process.status = -ER_IID;
        if (!(op_flags & P_ON_ERROR))
          k_illegal_id(rid_descr);
        goto exit_op_write; /* Illegal record id */
      }

      if (txn_id != 0) {
        if (!txn_write(fvar, id, id_len, str))
          goto exit_op_write;
      } else {
        if (!dir_write(fvar, mapped_id, str))
          goto exit_op_write;
      }

      break;

    case NET_FILE:
      net_write(fvar, id, id_len, src_descr->data.str.saddr, keep_lock); /* 0295 */
      break;
  }

exit_op_write:

  k_dismiss();
  k_dismiss();
  k_dismiss();

  /* Set status code on stack */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = process.status;

  if (process.status >= 0) {
    /* Release record lock if non-transactional WRITE */

    if (!keep_lock && (txn_id == 0) && (fvar->type != NET_FILE)) {
      unlock_record(fvar, lock_id, id_len);
    }
  } else if (!(op_flags & P_ON_ERROR)) {
    k_error(sysmsg(1407), -process.status, process.os_error);
  }
}

/* ======================================================================
   op_writev()  -  Write specific field to a record                       */

void op_writev() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Field number               | Status 0=ok, >0 = ELSE      |
     |                             |        <0 = ON ERROR        |
     |-----------------------------|-----------------------------| 
     |  Record id                  |                             |
     |-----------------------------|-----------------------------| 
     |  File variable              |                             |
     |-----------------------------|-----------------------------| 
     |  Field value                |                             |
     |=============================|=============================|
                    
    ON ERROR codes are only returned if ONERROR opcode executed prior to
    call to WRITEV.
 */

  u_int16_t op_flags;

  op_flags = process.op_flags;
  process.op_flags = 0;

  process.status = 0;

  /* Push lock flag onto e-stack; zero for WRITEV, non-zero for WRITEVU */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = op_flags & P_ULOCK;

  k_recurse(pcode_writev, 5); /* Execute recursive code */

  /* Set status code on stack */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = process.status;

  if ((process.status < 0) && !(op_flags & P_ON_ERROR)) {
    k_error(sysmsg(1408), -process.status);
  }
}

/* ======================================================================
   read_record()  -  Read a record                                        */

Private void read_record(bool matread) {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Record id                  | Status 0=ok, >0 = ELSE      |
     |                             |        <0 = ON ERROR        |
     |-----------------------------|-----------------------------| 
     |  File variable              |                             |
     |-----------------------------|-----------------------------| 
     |  ADDR to target             |                             |
     |=============================|=============================|
                    
    ON ERROR codes are only returned if ONERROR opcode executed prior to
    call to READ.
 */

  DESCRIPTOR *fvar_descr;
  DESCRIPTOR *rid_descr;
  DESCRIPTOR *tgt_descr;
  char id[MAX_ID_LEN + 1];
  char mapped_id[2 * MAX_ID_LEN + 1];
  /* initializing actual_id to clear a valgrind warning. 06feb22 -gwb */
  char actual_id[MAX_ID_LEN + 1] = {0}; /* Returned as recorded in file */
  int16_t id_len;
  char pathname[MAX_PATHNAME_LEN + 1];
  int16_t path_len;
  char record_path[MAX_PATHNAME_LEN + 1];
  FILE_VAR *fvar;
  DH_FILE *dh_file;
  int32_t bytes;
  u_int16_t op_flags;
  OSFILE t1_fu;
  int32_t remaining_bytes;
  int16_t n;
  int16_t status;
  DESCRIPTOR temp_descr; /* Temporary string used by MATREAD */
  DESCRIPTOR *str_descr; /* Descriptor into which to perform string read */
  STRING_CHUNK *str;
  bool is_net_file;
  u_int32_t txn_id;
  char *p;
  char *q;
  struct stat statbuf;

  op_flags = process.op_flags;
  process.op_flags = 0;

  status = 0;

  t1_fu = INVALID_FILE_HANDLE;

  /* Get record id */

  rid_descr = e_stack - 1;
  id_len = k_get_c_string(rid_descr, id, sysseg->maxidlen);

  /* Find the file variable descriptor */

  fvar_descr = e_stack - 2;
  k_get_file(fvar_descr);
  fvar = fvar_descr->data.fvar;
  is_net_file = (fvar->type == NET_FILE);
  txn_id = (fvar->flags & FV_NON_TXN) ? 0 : process.txn_id;

  /* Get target descr */

  tgt_descr = e_stack - 3;
  do {
    tgt_descr = tgt_descr->data.d_addr;
  } while (tgt_descr->type == ADDR);

  if (matread) {
    InitDescr(&temp_descr, UNASSIGNED);
    str_descr = &temp_descr;

    /* Check target is a matrix */
    if ((tgt_descr->type != ARRAY) && (tgt_descr->type != PMATRIX)) /* 0369 */
    {
      k_error(sysmsg(1232));
    }
  } else {
    str_descr = tgt_descr;
  }

  /* 1.4-3  Moved to before all possible exit paths */

  if (!(op_flags & P_PICKREAD)) {
    k_release(str_descr);
    InitDescr(str_descr, STRING);
    str_descr->data.str.saddr = NULL;
  }

  if (id_len <= 0) {
    process.status = ER_IID;
    status = (int16_t)process.status;
    goto exit_op_read; /* We failed to extract the record id */
  }

  if (!is_net_file) {
    /* Aquire lock if necessary */

    if (op_flags & P_REC_LOCKS) /* READL or READU */
    {
      process.status = lock_record(fvar, id, id_len, (op_flags & P_ULOCK) != 0, txn_id, (op_flags & P_LOCKED) != 0);
      switch (process.status) {
        case 0: /* Got the lock */
          break;

        case -2: /* Deadlock detected */
          if (sysseg->deadlock)
            k_deadlock();
          /* **** FALL THROUGH **** */

        case -1:                   /* Lock table is full */
        default:                   /* Conflicting lock is held by another user */
          if (op_flags & P_LOCKED) /* LOCKED clause present */
          {
            status = ER_LCK;
            goto exit_op_read;
          }
          pc = op_pc;
          process.op_flags = op_flags;
          if (my_uptr->events)
            process_events();
          lock_beep();
          Sleep(250);
          return;
      }
    }

    memcpy(actual_id, id, id_len); /* For all paths that don't update it */

    /* If we're in a transaction, we must scan the transaction cache for a
      possible reference to this record.                                   */

    if (txn_id && (fvar->type != NET_FILE)) {
      switch (txn_read(fvar, id, id_len, actual_id, &str)) {
        case TXC_FOUND: /* Found the record */
          /* For a directory file, the cached record is in internal format 
             so we do not need to apply mark mapping.                      */

          if ((op_flags & P_PICKREAD)) {
            k_release(str_descr);
            InitDescr(str_descr, STRING);
          }

          str_descr->data.str.saddr = str;
          if (str != NULL)
            str->ref_ct++;
          goto exit_op_read;

        case TXC_DELETED: /* Reference found as deleted record */
          status = (int16_t)(process.status = ER_RNF);
          goto exit_op_read;
      }
    }
  }

  switch (fvar->type) /* !!FVAR_TYPES!! */
  {
    case DYNAMIC_FILE:
      dh_file = fvar->access.dh.dh_file;
      str = dh_read(dh_file, id, id_len, actual_id);
      switch (dh_err) {
        case 0:
          if ((op_flags & P_PICKREAD)) {
            k_release(str_descr);
            InitDescr(str_descr, STRING);
          }
          str_descr->data.str.saddr = str;

          /* Call read trigger function if required */

          if (dh_file->trigger_modes & TRG_READ) {
            if (!call_trigger(fvar_descr, TRG_READ, rid_descr, str_descr, (op_flags & P_ON_ERROR), TRUE)) {
              process.status = -ER_TRIGGER;
              goto exit_op_read;
            }
          }
          break;

        case DHE_RECORD_NOT_FOUND:
          status = (int16_t)(process.status = ER_RNF);
          break;

        default:
          status = (int16_t)(-(process.status = dh_err));
          goto exit_op_read;
      }
      break;

    case DIRECTORY_FILE:
      if (!map_t1_id(id, id_len, mapped_id)) {
        process.status = ER_IID;
        goto exit_op_read; /* Illegal record id */
      }

      /* Increment statistics counter */

      StartExclusive(FILE_TABLE_LOCK, 49);
      sysseg->global_stats.reads++;
      EndExclusive(FILE_TABLE_LOCK);

      strcpy(pathname, (char *)(FPtr(fvar->file_id)->pathname));
      path_len = strlen(pathname);
      if (pathname[path_len - 1] == DS)
        pathname[path_len - 1] = '\0'; /* 0214 */
      /* converted to snprintf() -gwb 22Feb20 */
      if (snprintf(record_path, MAX_PATHNAME_LEN + 1, "%s%c%s", pathname, DS, mapped_id) >= (MAX_PATHNAME_LEN + 1)) {
        /* TODO: this should also be logged with more detail */
        k_error("Overflowed path/filename length in read_record()!");
        goto exit_op_read;
      }

      t1_fu = dio_open(record_path, DIO_READ);

      if (!ValidFileHandle(t1_fu)) {
        status = (int16_t)(process.status = ER_RNF);
        goto exit_op_read;
      }

      /* 0408 Check that this really is a file, not CON, COMn, LPTn */

      if (fstat(t1_fu, &statbuf) || !(statbuf.st_mode & S_IFREG)) {
        status = process.status = ER_IID;
        goto exit_op_read;
      }

      /* Find file size and initialise target string */

      remaining_bytes = (int32_t)filelength64(t1_fu);

      if ((op_flags & P_PICKREAD)) {
        k_release(str_descr);
        InitDescr(str_descr, STRING);
      }
      str_descr->data.str.saddr = NULL;
      ts_init(&(str_descr->data.str.saddr), remaining_bytes);

      /* Create a buffer in the virtual memory area */

      t1_buffer_alloc(remaining_bytes);

      while (remaining_bytes > 0) {
        bytes = min(remaining_bytes, t1_buffer_size);

        if (Read(t1_fu, t1_buffer, bytes) < 0) {
          status = (int16_t)(-(process.status = ER_IOE));
          process.os_error = OSError;
          goto exit_op_read;
        }

        remaining_bytes -= bytes;

        if (fvar->access.dir.mark_mapping) /* Non-image mode read */
        {
          if (remaining_bytes == 0) {
            /* This chunk contains the final byte of the file. This is
               probably a newline which we do not want to convert into
               a field mark. If so, decrement the byte count.          */

            if (t1_buffer[bytes - 1] == '\n')
              bytes--;
          }

          if (bytes) {
            /* Walk through the buffer replacing newlines by field marks. */

            p = t1_buffer;
            n = bytes;
            do {
              q = memchr(p, '\n', n);
              if (q == NULL)
                break;
              *q = FIELD_MARK;
              n -= (q + 1 - p);
              p = q + 1;
            } while (n);
          }
        }
        if (bytes)
          ts_copy(t1_buffer, bytes);
      }

      (void)ts_terminate();
      break;

    case NET_FILE:
      switch (net_read(fvar, id, id_len, op_flags, &str)) {
        case SV_OK: /* Take THEN clause */
          if ((op_flags & P_PICKREAD)) {
            k_release(str_descr);
            InitDescr(str_descr, STRING);
          }
          str_descr->data.str.saddr = str;
          break;
        case SV_ON_ERROR: /* Fatal error */
          status = -1;
          break;
        case SV_LOCKED:
          if (my_uptr->events)
            process_events();
          process.op_flags = op_flags;
          pc = op_pc;
          lock_beep();
          Sleep(250);
          return;
        default: /* Take ELSE clause */
          status = 1;
          break;
      }
      break;
  }

exit_op_read:

  if (ValidFileHandle(t1_fu))
    CloseFile(t1_fu);
  if (t1_buffer != NULL)
    t1_buffer_free();

  k_dismiss(); /* Record id */
  k_dismiss(); /* File var */

  if (matread) {
    if (status) {
      k_release(str_descr); /* Cast off string */
      k_dismiss();          /* ADDR to target */
    } else {
      *(e_stack++) = temp_descr; /* Move temporary string to e-stack */
      InitDescr(e_stack, STRING);
      str = e_stack->data.str.saddr = s_alloc(1L, &n);
      str->ref_ct = 1;
      str->string_len = 1;
      str->bytes = 1;
      str->data[0] = FIELD_MARK;
      (e_stack++)->data.str.saddr = str;
      op_matparse();
    }
  } else {
    k_dismiss(); /* Dismiss ADDR */
  }

  process.op_flags = 0; /* For error paths */

  /* Set status code on stack */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = status;

  if (status == 0) /* No errors */
  {
    /* Save the record id as the "last record read" using the actual_id
      so that files with case insensitive ids record the "real" id.    */

    fvar->id_len = id_len;
    setstring(&(fvar->id), actual_id);
  }

  if ((status < 0) && !(op_flags & P_ON_ERROR)) {
    k_error(sysmsg(1406), process.status, process.os_error);
  }
}

/* ======================================================================
   map_t1_id()  -  Perform name mapping for directory files               */

bool map_t1_id(char *id, int16_t id_len, char *mapped_id) {
  char *p;
  char *q;
  char *r;
  char c;

  p = id;
  q = mapped_id;

  if (*p == '.') {
    *(q++) = '%';
    *(q++) = 'd';
    p++;
    id_len--;
  } else if (*p == '~') {
    *(q++) = '%';
    *(q++) = 't';
    p++;
    id_len--;
  }

  while (id_len--) {
    c = *(p++);
    if ((c < 32) || (c > 126)) {
      mapped_id = '\0';
      return FALSE;
    } else if (map_dir_ids && ((r = strchr(df_restricted_chars, c)) != NULL)) {
      *(q++) = '%';
      *(q++) = df_substitute_chars[r - df_restricted_chars];
    } else {
      *(q++) = c;
    }
  }

  *q = '\0';
  return TRUE;
}

/* ======================================================================
   dir_write()  -  Write record to directory file                         */

bool dir_write(FILE_VAR *fvar, char *mapped_id, STRING_CHUNK *str) {
  char pathname[MAX_PATHNAME_LEN + 1];
  int16_t path_len;
  char record_path[MAX_PATHNAME_LEN + 1];
  char temp_path[MAX_PATHNAME_LEN + 1];
  int16_t bytes_remaining;
  char *p;
  char *q;
  int16_t n;
  struct stat statbuf;

  t1_fu = INVALID_FILE_HANDLE;
  process.status = 0;

  /* Increment statistics and transactions counters */

  StartExclusive(FILE_TABLE_LOCK, 51);
  sysseg->global_stats.writes++;
  FPtr(fvar->file_id)->upd_ct++;
  EndExclusive(FILE_TABLE_LOCK);

  /* Open and truncate the file */

  strcpy(pathname, (char *)(FPtr(fvar->file_id)->pathname));
  path_len = strlen(pathname);
  if (pathname[path_len - 1] == DS)
    pathname[path_len - 1] = '\0'; /* 0214 */
  /* converted to snprintf() -gwb 22Feb20 */
  if (snprintf(record_path, MAX_PATHNAME_LEN + 1, "%s%c%s", pathname, DS, mapped_id) >= (MAX_PATHNAME_LEN + 1)) {
    /* TODO: this should also be logged with more detail */
    k_error("Overflowed path/filename length in dir_write()!");
    goto exit_dir_write;
  }

  if (pcfg.safedir) {
    /* converted to snprintf() -gwb 22Feb20 */
    if (snprintf(temp_path, MAX_PATHNAME_LEN + 1, "%s%c~~%d", pathname, DS, my_uptr->uid) >= (MAX_PATHNAME_LEN + 1)) {
      /* TODO: this should also be logged with more detail */
      k_error("Overflowed path/filename length in dir_write()!");
      goto exit_dir_write;
    }
    t1_fu = dio_open(temp_path, DIO_REPLACE);
  } else {
    t1_fu = dio_open(record_path, DIO_REPLACE);
  }

  if (!ValidFileHandle(t1_fu)) {
    if (errno == ENOENT) {
      process.status = -ER_RNF;
    } else {
      process.status = -ER_PERM;
      log_permissions_error(fvar);
    }
    goto exit_dir_write;
  }

  /* 0408 Check that this really is a file, not CON, COMn, LPTn, AUX */

  if (fstat(t1_fu, &statbuf) || !(statbuf.st_mode & S_IFREG)) {
    process.status = -ER_IID;
    goto exit_dir_write;
  }

  /* Create a buffer in the virtual memory area */

  if (str != NULL) {
    t1_buffer_alloc(str->string_len);

    do {
      if (!fvar->access.dir.mark_mapping) {
        if (!t1_write(str->data, str->bytes))
          goto exit_dir_write;
      } else /* Convert field marks to newlines */
      {
        p = str->data;
        bytes_remaining = str->bytes;
        while (bytes_remaining) {
          q = (char *)memchr(p, FIELD_MARK, bytes_remaining);
          if (q == NULL) {
            if (!t1_write(p, bytes_remaining))
              goto exit_dir_write;
            break;
          } else {
            n = q - p;
            if ((n > 0) && (!t1_write(p, n)))
              goto exit_dir_write;
            if (!t1_write(Newline, NewlineBytes))
              goto exit_dir_write;
            p = q + 1;
            bytes_remaining -= n + 1;
          }
        }
      }
      str = str->next;
    } while (str != NULL);

    /* If we are not writing in image mode add a newline to the end
      of the file.                                                   */

    if (fvar->access.dir.mark_mapping) {
      if (!t1_write(Newline, NewlineBytes))
        goto exit_dir_write;
    }

    if (!t1_flush())
      goto exit_dir_write;
  }

  if (pcfg.safedir) {
    CloseFile(t1_fu);
    t1_fu = INVALID_FILE_HANDLE;
    remove(record_path);
    if (rename(temp_path, record_path)) {
      goto exit_dir_write;
    }
  }

exit_dir_write:
  if (ValidFileHandle(t1_fu))
    CloseFile(t1_fu);
  if (t1_buffer != NULL)
    t1_buffer_free();

  return process.status == 0;
}

/* ======================================================================
   Directory file buffering functions                                     */

Private void t1_buffer_alloc(int32_t size) {
  if (size > MAX_T1_BUFFER_SIZE)
    size = MAX_T1_BUFFER_SIZE;
  t1_buffer = (char *)k_alloc(8, size);
  t1_ptr = t1_buffer;
  t1_buffer_size = size;
  t1_space = (int16_t)size;
}

Private bool t1_write(char *p, int16_t bytes) {
  while (bytes) {
    if (bytes < t1_space) /* Fits with at least one byte to spare */
    {
      memcpy(t1_ptr, p, bytes);
      t1_ptr += bytes;
      t1_space -= bytes;
      break;
    } else /* Fills buffer */
    {
      memcpy(t1_ptr, p, t1_space);
      p += t1_space;
      bytes -= t1_space;
      t1_space = 0;

      if (!t1_flush())
        return FALSE;

      t1_ptr = t1_buffer;
      t1_space = t1_buffer_size;
    }
  }

  return TRUE;
}

Private bool t1_flush() {
  int16_t len;
  len = t1_buffer_size - t1_space;
  if (len == 0)
    return TRUE;
  if (Write(t1_fu, t1_buffer, len) < 0) {
    process.status = -ER_IOE;
    process.os_error = OSError;
    return FALSE;
  }

  return TRUE;
}

Private void t1_buffer_free() {
  k_free(t1_buffer);
  t1_buffer = NULL;
}

/* ======================================================================
   call_trigger()  -  Call trigger function                               */

bool call_trigger(DESCRIPTOR *fvar_descr, int16_t mode, DESCRIPTOR *id_descr, DESCRIPTOR *data_descr, bool on_error, bool updatable) /* Allow data to be modified? */
{
  FILE_VAR *fvar;
  DESCRIPTOR *trg_descr; /* @TRIGGER.RETURN.CODE */
  STRING_CHUNK *str;
  DH_FILE *dh_file;
  bool error;
  int16_t args;

  fvar = fvar_descr->data.fvar;
  dh_file = fvar->access.dh.dh_file;

  /* Check trigger function was loaded on open */

  if (dh_file->trigger == NULL) {
    k_error(sysmsg(1409), dh_file->trigger_name, FPtr(dh_file->file_id)->pathname);
  }

  /* Check argument count */

  args = ((OBJECT_HEADER *)(dh_file->trigger))->args;
  if ((args < 4) || (args > 5))
    k_error(sysmsg(1410));

  /* Set @TRIGGER.RETURN.CODE to 0 */

  trg_descr = Element(process.syscom, SYSCOM_TRIGGER_RETURN_CODE);
  k_release(trg_descr);
  InitDescr(trg_descr, INTEGER);
  trg_descr->data.value = 0;

  /* Push action onto stack */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = mode;

  /* Push record id onto stack */

  InitDescr(e_stack, STRING);
  str = (id_descr == NULL) ? NULL : (id_descr->data.str.saddr);
  (e_stack++)->data.str.saddr = str;
  if (str != NULL)
    (str->ref_ct)++;

  /* Push record data onto stack */

  if (updatable) {
    InitDescr(e_stack, ADDR);
    (e_stack++)->data.d_addr = data_descr;
  } else {
    InitDescr(e_stack, STRING);
    str = (data_descr == NULL) ? NULL : (data_descr->data.str.saddr);
    (e_stack++)->data.str.saddr = str;
    if (str != NULL)
      (str->ref_ct)++;
  }

  /* Push ON_ERROR flag onto stack */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = on_error;

  /* Push trigger file variable if five args */

  if (args == 5) {
    InitDescr(e_stack, ADDR);
    (e_stack++)->data.d_addr = fvar_descr;
  }

  /* Call trigger function using an adaptation of k_recurse() */

  k_call("", args, dh_file->trigger, 0);
  process.program.flags |= PF_IS_TRIGGER | PF_IN_TRIGGER;
  k_run_program();
  k_exit_cause = 0;

  error = ((trg_descr->type != INTEGER) || (trg_descr->data.value != 0));
  if (error && !on_error) {
    tio_printf("\n");
    tio_printf(sysmsg(1411)); /* Data validation error: */

    /* Push ADDR to @TRIGGER.RETURN.CODE onto stack */

    InitDescr(e_stack, ADDR);
    (e_stack++)->data.d_addr = trg_descr;
    op_dspnl();
  }

  return !error;
}

/* ======================================================================
   op_pickread()  -  PICKREAD  Set Pick style read behaviour              */

void op_pickread() {
  process.op_flags |= P_PICKREAD;
}

/* ======================================================================
   valid_id()  -  Validate record id                                      */

Private bool valid_id(char *id, int16_t id_len) {
  register char c;

  if ((id_len <= 0) /* Handle k_get_c_string() failure here */
      || (id_len > sysseg->maxidlen)) {
    return FALSE;
  }

  while (id_len--) {
    c = *(id++);
    if (c == 0)
      return FALSE;
    if (IsMark(c))
      return FALSE;
  }

  return TRUE;
}

/* END-CODE */
