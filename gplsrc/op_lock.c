/* OP_LOCK.C
 * Lock management opcodes
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
 * START-HISTORY:
 * 15 Oct 07  2.6-5 Added local lock table.
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 26 Jan 07  2.4-20 Track peak number of record locks.
 * 29 Jun 06  2.4-5 Added lock_beep().
 * 12 Apr 06  2.4-1 Added non-transactional file flag to FILE_VAR.
 * 16 Dec 05  2.3-2 Log a message if the lock table becomes full.
 * 20 Sep 05  2.2-11 User map now dynamic.
 * 25 May 05  2.2-0 In op_unlk(), map id to uppercase for case insensitive file.
 * 27 Oct 04  2.0-7 0274 unlock_record() was passing wrong variable into
 *                  net_unlock() for record id.
 * 26 Oct 04  2.0-7 Use dynamic max_id_len.
 * 18 Oct 04  2.0-5 0261 In unlock_record(), set id = u_id after making upper
 *                  case form of id.
 * 20 Sep 04  2.0-2 Use messge handler.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * Opcodes:
 *    op_filelock       FILELOCK
 *    op_flunlock       FLUNLOCK
 *    op_getlocks       GETLOCKS()
 *    op_llock          LLOCK
 *    op_lock           LOCK
 *    op_lockrec        RECORDLOCKL/U
 *    op_nowait         NOWAIT
 *    op_reclckd        RECORDLOCKED()
 *    op_release        RELEASE
 *    op_rlsall         RLSALL
 *    op_rlsfile        RLSFILE
 *    op_testlock       TESTLOCK()
 *    op_ulock          ULOCK
 *    op_unlock         UNLOCK
 *
 *    lock_record
 *    unlock_record
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include "locks.h"
#include "keys.h"
#include "header.h"
#include "tio.h"

#include <time.h>

Private bool unlock_id(short int file_id, char * raw_id, short int id_len);

/* ======================================================================
   op_filelock()  -  Set file lock                                        */

void op_filelock()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  File variable              |  0 = success, <0 = error    |
     |                             |  ER$LCK = Already locked    |
     |=============================|=============================|

  The ON ERROR clause is currently never taken.

  ER$LCK is only returned if LOCKED clause is present. Otherwise the opcode
  waits for the lock to become available.
 */

 DESCRIPTOR * descr;
 unsigned short int op_flags;
 short int status = 0;
 short int lock_owner;
 FILE_ENTRY * fptr;
 RLOCK_ENTRY * lptr;
 short int idx;
 short int file_id;
 FILE_VAR * fvar;

 op_flags = process.op_flags;

 descr = e_stack - 1;
 k_get_file(descr);
 fvar = descr->data.fvar;

 if (fvar->type == NET_FILE)
  {
   process.status = net_filelock(fvar, !(op_flags & P_LOCKED));
   if (process.status) status = ER_LCK;       /* Take the LOCKED clause */
  }
 else
  {
   file_id = fvar->file_id;
   fptr = FPtr(file_id); 

   StartExclusive(REC_LOCK_SEM, 36);

   if ((lock_owner = fptr->file_lock) != 0)
    {
     if (lock_owner == process.user_no)      /* File lock is held by us */
      {
       fptr->txn_id = process.txn_id;
       goto locked_it; /* Already hold the lock */
      }
     else
      {
       goto wait_lock;
      }
    }

   if (fptr->lock_count == 0)           /* No locks in this file */
    {
     fptr->file_lock = process.user_no;
     fptr->fvar_index = fvar->index;
     fptr->txn_id = process.txn_id;
     goto locked_it;
    }

   /* Check that all record locks are held by us */

   for(idx = 1; idx <= sysseg->numlocks; idx++)
    {
     lptr = RLPtr(idx);
     if ((lptr->hash != 0)                   /* Cell used... */
        && (lptr->file_id == file_id)        /* ...for this file... */
        && (lptr->owner != process.user_no)) /* ...by another user */
      {
       lock_owner = lptr->owner;
       goto wait_lock;
      }
    }

   fptr->file_lock = process.user_no;
   fptr->fvar_index = fvar->index;
   fptr->txn_id = process.txn_id;
   goto locked_it;

   /* The lock is in use. Wait for the lock? */

wait_lock:
   if (!(op_flags & P_LOCKED))
    {
     /* Set up lock wait information */

     my_uptr->lockwait_index = -(fvar->file_id);
     EndExclusive(REC_LOCK_SEM);
     if (my_uptr->events) process_events();
     pc--;                   /* Back up to repeat opcode */
     lock_beep();
     Sleep(250);
     return;
    }

   status = ER_LCK;       /* Take the LOCKED clause */
   process.status = lock_owner;

locked_it:
   my_uptr->lockwait_index = 0;
   EndExclusive(REC_LOCK_SEM);
 }

 process.op_flags = 0;
 k_dismiss();


 InitDescr(e_stack, INTEGER);
 (e_stack++)->data.value = status;
}

/* ======================================================================
   op_flunlock()  -  Release file lock                                    */

void op_flunlock()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  File variable              |  0 = success, <0 = error    |
     |=============================|=============================|

  The ON ERROR clause is currently never taken.
 */

 DESCRIPTOR * descr;
 FILE_VAR * fvar;
 FILE_ENTRY * fptr;
 short int lock_owner;
 short int file_id;

 descr = e_stack - 1;
 k_get_file(descr);
 fvar = descr->data.fvar;

 if (fvar->type == NET_FILE)
  {
   process.status = net_fileunlock(fvar);
  }
 else
  {
   file_id = fvar->file_id; 
   fptr = FPtr(file_id); 
   if ((lock_owner = fptr->file_lock) == process.user_no)
    {

     StartExclusive(REC_LOCK_SEM, 56);
     fptr->file_lock = 0;
     clear_waiters(-file_id);
     EndExclusive(REC_LOCK_SEM);
     process.status = 0;
    }
   else if (lock_owner != 0)
    {
     process.status = ER_LCK;
    }
   else
    {
     process.status = ER_NLK;
    }
  }

 k_dismiss();
 InitDescr(e_stack, INTEGER);
 (e_stack++)->data.value = process.status;
}

/* ======================================================================
   op_getlocks()  -  GETLOCKS() function                                  */

void op_getlocks()
{
 /* Stack:

     |===============================|===============================|
     |            BEFORE             |             AFTER             |
     |===============================|===============================|
 top |  User no, zero if all         |  Result list                  |
     |-------------------------------|-------------------------------|
     |  File var. All if not a file  |                               |
     |===============================|===============================|

 The first field of the result contains record lock statistics:
     numlocks VM current count VM peak locks
 The remainder of the result list is a string formatted as:
     file_id VM file path VM userno VM type VM id VM username FM file path....
     Type = RU, RL, FX or SX
     Id is absent for file lock
     Multiple locks for a single file are not merged
     Locks are only reported for the specified user / file
 */

 DESCRIPTOR * descr;
 short int file_id;            /* File to report, -ve if all */
 short int user;               /* User to report, zero if all */
 STRING_CHUNK * str_head = NULL;
 FILE_ENTRY * fptr;
 RLOCK_ENTRY * rlptr;
 short int lock_owner;
 short int i;
 USER_ENTRY * uptr;
 short int j;
 short int lo;
 short int hi;

 /* Get user number, zero if to report all users */

 descr = e_stack - 1;
 GetInt(descr);
 user = (short int)(descr->data.value);

 /* Get file variable. Report all files if it is not a file var */

 descr = e_stack - 2;
 k_get_value(descr);
 if (descr->type == FILE_REF)
  {
   file_id = descr->data.fvar->file_id;
  }
 else
  {
   file_id = -1;
  }

 /* Generate result string */

 ts_init(&str_head, 64);

 /* We hold the file table lock throughout so that file ids are guaranteed
    not to change during the report. The file_lock entry in the file table
    is actually protected by the REC_LOCK_SEM so we get this up front too. */

 StartExclusive(FILE_TABLE_LOCK, 27);
 StartExclusive(REC_LOCK_SEM, 28);

 /* Counters */

 ts_printf("%d\xfd%d\xfd%d",
           sysseg->numlocks, sysseg->rl_count, sysseg->rl_peak);

 /* File locks */

 for(i = 1, fptr = FPtr(1); i <= sysseg->used_files; i++, fptr++)
  {
   if (fptr->file_lock != 0)
    {
     lock_owner = abs(fptr->file_lock);
     if (((file_id < 0) || (i == file_id))
        && ((user == 0) || (user == abs(lock_owner))))
      {
       ts_copy_byte(FIELD_MARK);
       ts_printf("%d\xfd%s\xfd%d\xfd%s\xfd\xfd%s",
                 (int)i,
                 fptr->pathname,
                 (int)abs(lock_owner),
                 (lock_owner < 0)?"SX":"FX",
                 UserPtr(abs(lock_owner))->username);
      }
    }
  }

 /* Record locks */

 for(i = 1; i <= sysseg->numlocks; i++)
  {
   rlptr = RLPtr(i);
   if (rlptr->hash != 0)
    {
     lock_owner = rlptr->owner;
     if (((file_id < 0) || (rlptr->file_id == file_id))
        && ((user == 0) || (user == lock_owner)))
      {
       fptr = FPtr(rlptr->file_id);
       if (str_head != NULL) ts_copy_byte(FIELD_MARK);

       ts_printf("%d\xfd%s\xfd%d\xfd%s\xfd%.*s\xfd%s",
                 (int)(rlptr->file_id),
                 fptr->pathname,
                 (int)lock_owner,
                 (rlptr->lock_type == L_UPDATE)?"RU":"RL",
                 rlptr->id_len, rlptr->id,
                 UserPtr(abs(lock_owner))->username);
      }
    }
  }

 /* Lock waiters */

 if (user == 0)
  {
   lo = 1;
   hi = sysseg->max_users;
  }
 else
  {
   lo = hi = *(UMap(user));
  }

 for (i = lo; i <= hi; i++)
  {
   uptr = UPtr(i);

   if ((uptr->uid != 0)                        /* User table entry is in use */
       && ((j = uptr->lockwait_index) != 0))   /* User is waiting for a lock */
    {
     if (j > 0)       /* Waiting for record lock */
      {
       rlptr = RLPtr(j);
       if ((file_id < 0) || (rlptr->file_id == file_id))
        {
         fptr = FPtr(rlptr->file_id);
         if (str_head != NULL) ts_copy_byte(FIELD_MARK);

         ts_printf("%d\xfd%s\xfd%d\xfdWAIT\xfd%.*s\xfd",
                   (int)(rlptr->file_id),
                   fptr->pathname,
                   (int)(uptr->uid),
                   rlptr->id_len, rlptr->id);
        }
      }
     else             /* Waiting for file lock */
      {
       if ((file_id < 0) || (-j == file_id))
        {
         fptr = FPtr(-j);
         if (str_head != NULL) ts_copy_byte(FIELD_MARK);

         ts_printf("%d\xfd%s\xfd%d\xfdWAIT\xfd\xfd",
                   (int)(-j),
                   fptr->pathname,
                   (int)(uptr->uid));
        }
      }
    }
  }

 EndExclusive(REC_LOCK_SEM);
 EndExclusive(FILE_TABLE_LOCK);

 (void)ts_terminate(); 

 k_pop(1);
 k_dismiss();

 InitDescr(e_stack, STRING);
 (e_stack++)->data.str.saddr = str_head;
}

/* ======================================================================
   op_llock()  -  Read lock prefix opcode                                 */

void op_llock()
{
 process.op_flags |= P_LLOCK;

 /* Set the lock_beep_timer back to zero so that we always wait one
    second before the first beep. This ensures that "normal" short
    waits do not beep.                                              */
    
 lock_beep_timer = 0;
}

/* ======================================================================
   op_lock()  -  Acquire task lock                                        */

void op_lock()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Lock number                |  0 = success, >0 = failed   |
     |=============================|=============================|
 */

 DESCRIPTOR * descr;
 short int lock_no;
 short int lock_owner;

 descr = e_stack -1;
 GetInt(descr);
 lock_no = (short int)(descr->data.value);

 if ((lock_no < 0) || (lock_no > 63)) k_inva_task_lock_error();

 StartExclusive(SHORT_CODE,41);

 lock_owner = sysseg->task_locks[lock_no];
 process.status = lock_owner;
 if ((lock_owner == 0) || (lock_owner == process.user_no))
  {
   descr->data.value = 0;
   sysseg->task_locks[lock_no] = process.user_no;
  }
 else
  {
   descr->data.value = lock_owner;
  }

 EndExclusive(SHORT_CODE);
}

/* ======================================================================
   op_testlock  -  Test a task lock                                       */

void op_testlock()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Lock number                |  Owner                      |
     |=============================|=============================|
 */

 DESCRIPTOR * descr;
 short int lock_no;

 descr = e_stack -1;
 GetInt(descr);
 lock_no = (short int)(descr->data.value);

 if ((lock_no < 0) || (lock_no > 63)) k_inva_task_lock_error();

 descr->data.value = sysseg->task_locks[lock_no];
}

/* ======================================================================
   op_lockrec()  -  Set read or update lock on record                     */

void op_lockrec()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Record id                  | Status 0=ok, <0 = ON ERROR  |
     |-----------------------------|-----------------------------| 
     |  File variable              |                             |
     |=============================|=============================|
                    
    ON ERROR codes are only returned if ONERROR opcode executed prior to
    call to LOCKREC.
 */

 DESCRIPTOR * descr;
 DESCRIPTOR * id_descr;
 short int id_len;
 char id[MAX_ID_LEN+1];
 FILE_VAR * fvar;
 unsigned short int op_flags;
 short int status;
 bool update;
 bool no_wait;
 unsigned long int txn_id;

 op_flags = process.op_flags;
 update = (op_flags & P_ULOCK) != 0;
 no_wait = (op_flags & P_LOCKED) != 0;

 /* Get record id */

 id_descr = e_stack - 1;
 if ((id_len = k_get_c_string(id_descr, id, sysseg->maxidlen)) <= 0)
  {
   k_illegal_id(id_descr);
  }

 /* Find the file variable descriptor */

 descr = e_stack - 2;
 k_get_file(descr);
 fvar = descr->data.fvar;
 txn_id = (fvar->flags & FV_NON_TXN)?0:process.txn_id;

 switch(status = lock_record(fvar, id, id_len, update, txn_id, no_wait))
  {
   case 0:    /* Got the lock */
      break;

   case -2:   /* Deadlock detected */
      if (sysseg->deadlock) k_deadlock();
      /* **** FALL THROUGH **** */

   case -1:   /* Lock table is full */
   default:   /* Conflicting lock is already held by another user */
      if (!no_wait) /* Wait for the lock */
       {
        if (my_uptr->events) process_events();
        pc--;
        lock_beep();
        Sleep(250);
        return;
       }
      process.status = status;
      status = ER_LCK;       /* Take the LOCKED clause */
      break;
  }
 
 process.op_flags = 0;


 k_dismiss();
 k_dismiss();

 InitDescr(e_stack, INTEGER);
 (e_stack++)->data.value = status;
}

/* ======================================================================
   op_nowait()  -  Activate LOCKED clause for various opcodes             */

void op_nowait()
{
 process.op_flags |= P_LOCKED;
}

/* ======================================================================
   op_reclckd()  -  RECORDLOCKED function                                 */

void op_reclckd()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Record id                  |  Lock state code            |
     |-----------------------------|-----------------------------|
     |  File variable              |                             |
     |=============================|=============================|
 */

 DESCRIPTOR * descr;
 char id[MAX_ID_LEN+1];
 short int id_len;
 short int status = 0;
 FILE_VAR * fvar;
 FILE_ENTRY * fptr;
 short int idx;
 long int hash_value;
 short int scan_idx;     /* Scanning index */
 RLOCK_ENTRY * lptr;
 short int active_locks;
 short int file_id;


 /* Get record id */

 descr = e_stack - 1;
 if ((id_len = k_get_c_string(descr, id, sysseg->maxidlen)) <= 0)
  {
   k_illegal_id(descr);
  }

 /* Find the file variable descriptor */

 descr = e_stack - 2;
 k_get_file(descr);
 fvar = descr->data.fvar;

 if (fvar->type == NET_FILE)
  {
   status = net_recordlocked(fvar, id, id_len);
  }
 else
  {
   file_id = fvar->file_id;
   fptr = FPtr(file_id);

   if ((process.status = fptr->file_lock) != 0)  /* File is locked */
    {
     if (process.status == process.user_no) status = LOCK_MY_FILELOCK;
     else status = LOCK_OTHER_FILELOCK;
    }
   else if (fptr->lock_count != 0)
    {
     StartExclusive(REC_LOCK_SEM, 29);

     if (fptr->flags & DHF_NOCASE) UpperCaseMem(id, id_len);

     hash_value = hash(id, id_len);
     idx = (short int)RLockHash(file_id, hash_value);
     scan_idx = idx;
     lptr = RLPtr(scan_idx);

     active_locks = lptr->count;   /* There are this many locks somewhere */
     while(active_locks)
      {
       if (lptr->hash == idx)
        {
         if ((lptr->file_id == file_id)        /* Right file... */
           && (lptr->id_hash == hash_value)    /* ...right hash... */
           && (lptr->id_len == id_len)         /* ...right id_len... */
           && (!memcmp(lptr->id, id, id_len))) /* ...right id */
          {
           /* We have found the lock */
         
           if ((process.status = lptr->owner) == process.user_no)
            {
             status = (lptr->lock_type == L_UPDATE)?LOCK_MY_READU:LOCK_MY_READL;
             break;
            }
           else
            {
             status = (lptr->lock_type == L_UPDATE)?LOCK_OTHER_READU:LOCK_OTHER_READL;
            }
          }

         active_locks--;
        }

       if (++scan_idx > sysseg->numlocks) scan_idx = 1;
       lptr = RLPtr(scan_idx);
      }

     EndExclusive(REC_LOCK_SEM);
    }
  }

 k_dismiss();
 k_dismiss();

 InitDescr(e_stack, INTEGER);
 (e_stack++)->data.value = status;
}

/* ======================================================================
   op_release()  -  Release locks                                         */

void op_release()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Record id                  |  0 = ok; <0 = ON ERROR      |
     |-----------------------------|-----------------------------|
     |  File variable              |                             |
     |=============================|=============================|

    ON ERROR codes are only returned if ONERROR opcode executed prior to
    call to RELEASE.
 */

 DESCRIPTOR * id_descr;
 DESCRIPTOR * descr;
 FILE_VAR * fvar;
 char id[MAX_ID_LEN+1];
 short int id_len;
 unsigned long int txn_id;


 process.op_flags = 0;

 /* Get record id */

 id_descr = e_stack - 1;
 if ((id_len = k_get_c_string(id_descr, id, sysseg->maxidlen)) < 0)
  {
   k_illegal_id(id_descr);
  }

 /* Find the file variable descriptor */

 descr = e_stack - 2;
 k_get_file(descr);
 fvar = descr->data.fvar;
 txn_id = (fvar->flags & FV_NON_TXN)?0:process.txn_id;
 
 if (txn_id == 0)
  {
   if (id_len) (void)unlock_record(fvar, id, id_len);

  }

 k_dismiss();
 k_dismiss();

 InitDescr(e_stack, INTEGER);
 (e_stack++)->data.value = 0;
}

/* ======================================================================
   op_rlsfile()  -  Release all locks for given file                      */

void op_rlsfile()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  File variable              |  0 = ok; <0 = ON ERROR      |
     |=============================|=============================|

    ON ERROR codes are only returned if ONERROR opcode executed prior to
    call to RELEASE.
 */

 DESCRIPTOR * descr;
 FILE_VAR * fvar;
 unsigned long int txn_id;

 process.op_flags = 0;

 /* Find the file variable descriptor */

 descr = e_stack - 1;
 k_get_file(descr);
 fvar = descr->data.fvar;
 txn_id = (fvar->flags & FV_NON_TXN)?0:process.txn_id;

 if (txn_id == 0)
  {
   unlock_record(fvar, NULL, 0);
  }

 k_dismiss();

 InitDescr(e_stack, INTEGER);
 (e_stack++)->data.value = 0;
}

/* ======================================================================
   op_rlsall()  -  Release all record locks                               */

void op_rlsall()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |                             |  0 = ok; <0 = ON ERROR      |
     |=============================|=============================|

    ON ERROR codes are only returned if ONERROR opcode executed prior to
    call to RLSALL.
 */

 process.op_flags = 0;

 net_unlock_all();

 if (process.txn_id == 0)
  {
   (void)unlock_record(NULL, "", 0);

  }

 InitDescr(e_stack, INTEGER);
 (e_stack++)->data.value = 0;
}

/* ======================================================================
   op_ulock()  -  Update lock prefix opcode                               */

void op_ulock()
{
 process.op_flags |= P_ULOCK;

 /* Set the lock_beep_timer back to zero so that we always wait one
    second before the first beep. This ensures that "normal" short
    waits do not beep.                                              */
    
 lock_beep_timer = 0;
}

/* ======================================================================
 op_unlk()  -  Release a specific record lock, possibly not ours          */

void op_unlk()
{
 /* Stack:

     |===============================|===============================|
     |            BEFORE             |             AFTER             |
     |===============================|===============================|
 top |  Record id, null if all       |                               |
     |-------------------------------|-------------------------------|
     |  User no, -ve if all          |                               |
     |-------------------------------|-------------------------------|
     |  File id, -ve if all          |                               |
     |===============================|===============================|

*/

 DESCRIPTOR * descr;
 short int id_len;
 char id[MAX_ID_LEN + 1];
 short int file_id;
 short int user_no;
 RLOCK_ENTRY * lptr;
 short int lno;


 /* Get record id */
 
 descr = e_stack - 1;
 if ((id_len = k_get_c_string(descr, id, sysseg->maxidlen)) < 0)
  {
   k_illegal_id(descr);
  }
 k_dismiss();

 /* Get user number */

 descr = e_stack - 1;
 GetInt(descr);
 user_no = (short int)(descr->data.value);
 k_pop(1);

 /* Get file id */

 descr = e_stack - 1;
 GetInt(descr);
 file_id = (short int)(descr->data.value);
 k_pop(1);

 if (file_id >= 0)
  {
   if (FPtr(file_id)->flags & DHF_NOCASE) UpperCaseMem(id, id_len);
  }

 StartExclusive(REC_LOCK_SEM, 30);

 for (lno = 1; lno <= sysseg->numlocks; lno++)
  {
   lptr = RLPtr(lno);

   if ((lptr->hash != 0)                                /* Cell in use */
      && ((file_id < 0) || (lptr->file_id == file_id))  /* Right file... */
      && ((user_no < 0) || (lptr->owner == user_no))    /* ...right user... */
      && ((id_len == 0)
          || ((lptr->id_len == id_len)                  /* ...id_len... */
              && (!memcmp(lptr->id, id, id_len)))))     /* ...right id */
    {
     /* We have found a lock to release */
     (RLPtr(lptr->hash)->count)--;
     (sysseg->rl_count)--;
     (FPtr(lptr->file_id)->lock_count)--;
     if (lptr->waiters) clear_waiters(lno);
     lptr->hash = 0;    /* Free this cell */

     /* Raise an EVT_REBUILD_LLT event in the target user. We do this,
        even if it is ourself, as the code above will not have released
        the local lock table entry.                                     */

     raise_event(EVT_REBUILD_LLT, lptr->owner);
    }
  }

 EndExclusive(REC_LOCK_SEM);
} 

/* ======================================================================
 op_unlkfl()  -  Release a specific file lock, possibly not ours          */

void op_unlkfl()
{
 /* Stack:

     |===============================|===============================|
     |            BEFORE             |             AFTER             |
     |===============================|===============================|
 top |  User no, -ve if all          |                               |
     |-------------------------------|-------------------------------|
     |  File id, -ve if all          |                               |
     |===============================|===============================|

*/

 DESCRIPTOR * descr;
 short int user_no;
 FILE_ENTRY * fptr;
 short int fno;


 /* Get user number */

 descr = e_stack - 1;
 GetInt(descr);
 user_no = (short int)(descr->data.value);
 k_pop(1);

 /* Get file id */

 descr = e_stack - 1;
 GetInt(descr);
 fno = (short int)(descr->data.value);
 k_pop(1);

 StartExclusive(REC_LOCK_SEM, 31);
 if (fno >= 0)    /* Specific file */
  {
   fptr = FPtr(fno);
   if (fptr->ref_ct != 0)
    {
     if ((user_no < 0) || (abs(fptr->file_lock) == user_no))
      {
       clear_waiters(-fno);
       fptr->file_lock = 0;
      }
    }
  }
 else                 /* All files */
  {
   for (fno = 1; fno <= sysseg->used_files; fno++)
    { 
     fptr = FPtr(fno);
     if (fptr->ref_ct != 0)
      {
       if ((user_no < 0) || (abs(fptr->file_lock) == user_no))
        {
         clear_waiters(-fno);
         fptr->file_lock = 0;
        }
      }
    }
  }

 EndExclusive(REC_LOCK_SEM);
} 

/* ======================================================================
   op_unlock()  -  Release task lock                                      */

void op_unlock()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Lock number                |  0 = ok; >0 = error         |
     |=============================|=============================|

     Lock number may have 1024 additive to release locks owned by
     other users (only valid from internal mode programs).

     Errors are:
        ER$LCK    Lock is held by another process
        ER$NLK    Lock is not held by any process
 */

 DESCRIPTOR * descr;
 short int lock_no;
 bool allow_other_users = FALSE;

 descr = e_stack -1;
 GetInt(descr);
 lock_no = (short int)(descr->data.value);

 if (process.program.flags & HDR_INTERNAL)
  {
   allow_other_users = (lock_no & 1024) != 0;
   lock_no &= ~1024;
  }

 if ((lock_no < 0) || (lock_no > 63)) k_inva_task_lock_error();

 if ((sysseg->task_locks[lock_no] == process.user_no) /* Own this lock */
     || allow_other_users)
  {
   process.status = 0;
   sysseg->task_locks[lock_no] = 0;
  }
 else if (sysseg->task_locks[lock_no]) /* Lock owned elsewhere */
  {
   process.status = ER_LCK;
  }
 else
  {
   process.status = ER_NLK;
  }
  
 descr->data.value = process.status;
}

/* ======================================================================
   check_lock()  -  Check if record is covered by update or file lock     */

bool check_lock(
   FILE_VAR * fvar,
   char * raw_id,              /* Record id */
   short int id_len)
{
 bool status = FALSE;
 FILE_ENTRY * fptr;
 short int idx;
 long int hash_value;
 short int scan_idx;     /* Scanning index */
 RLOCK_ENTRY * lptr;
 short int active_locks;
 short int file_id;
 char u_id[MAX_ID_LEN];
 char * id;

 file_id = fvar->file_id;
 fptr = FPtr(file_id);

 if (fptr->flags & DHF_NOCASE)             /* 0313 */
  {
   memucpy(u_id, raw_id, id_len);
   id = u_id;
  }
 else
  {
   id = raw_id;
  }

 if (fptr->file_lock != 0)  /* File is locked */
  {
   if (fptr->file_lock == process.user_no) status = TRUE;  /* It's us */
  }
 else if (fptr->lock_count != 0)  /* File not locked but records are locked */
  {
   StartExclusive(REC_LOCK_SEM, 47);

   hash_value = hash(id, id_len);
   idx = (short int)RLockHash(file_id, hash_value);
   scan_idx = idx;
   lptr = RLPtr(scan_idx);

   active_locks = lptr->count;   /* There are this many locks somewhere */
   while(active_locks)
    {
     if (lptr->hash == idx)
      {
       if ((lptr->file_id == file_id)         /* Right file... */
         && (lptr->id_hash == hash_value)     /* ...right hash... */
         && (lptr->id_len == id_len)          /* ...right id_len... */
         && (!memcmp(lptr->id, id, id_len))   /* ...right id... */
         && (lptr->owner == process.user_no)) /* ...right user */
        {
         /* We have found the lock */

         status = (lptr->lock_type == L_UPDATE);
         break;
        }

       active_locks--;
      }

     if (++scan_idx > sysseg->numlocks) scan_idx = 1;
     lptr = RLPtr(scan_idx);
    }

   EndExclusive(REC_LOCK_SEM);
  }

 return status;
}

/* ======================================================================
   lock_record()  -  Set a read or update lock on a record.
   Note that a record can be locked if the file lock is held by the user
   locking the record.                                                    */

short int lock_record(
               /* Returns 0 if ok else user no of process preventing lock,
                  -1 = lock table full
                  -2 = deadlock detected */
   FILE_VAR * fvar,
   char * raw_id,            /* Record id... */
   short int id_len,         /* ...and length */
   bool update,              /* Update lock? */
   unsigned long int txn_id, /* Locked as part of this transaction */
   bool no_wait)             /* Caller is not going to wait for lock */
{
 short int status = 0;
 short int file_id;
 FILE_ENTRY * fptr;
 char u_id[MAX_ID_LEN];
 char * id;
 short int idx;
 long int hash_value;
 short int scan_idx;     /* Scanning index */
 short int upgrade_idx = 0;
 RLOCK_ENTRY * lptr;
 short int active_locks;
 short int free_cell = 0;
 short int lock_owner;
 short int blocking_idx;
 bool table_full = FALSE;
 short int u;
 short int lwi;
 FILE_ENTRY * d_fptr;
 RLOCK_ENTRY * d_lptr;
 LLT_ENTRY * llt;

 if (fvar->type == NET_FILE)
  {
   /* All locking for network files is handled at the remote end.
      Send a request to get this lock.                            */

   return net_lock(fvar, raw_id, id_len, update, no_wait);
  }

 file_id = fvar->file_id;
 fptr = FPtr(file_id);

 if (fptr->flags & DHF_NOCASE)
  {
   memucpy(u_id, raw_id, id_len);
   id = u_id;
  }
 else
  {
   id = raw_id;
  }

 StartExclusive(REC_LOCK_SEM, 32);

 /* Check file lock */

 lock_owner = abs(fptr->file_lock);                 /* 0194 */
 if (lock_owner && (lock_owner != process.user_no))
  {
   status = lock_owner;  /* File is locked elsewhere */
   blocking_idx = -file_id;
   goto exit_lock_record;
  }

 /* Check record lock */

 hash_value = hash(id, id_len);
 idx = (short int)RLockHash(file_id, hash_value);

 scan_idx = idx;
 lptr = RLPtr(scan_idx);

 /* Scan to see if this lock is already owned, remembering any blank cell */

 active_locks = lptr->count;   /* There are this many locks somewhere */
 while(active_locks)
  {
   if (lptr->hash == idx)
    {
     if ((lptr->file_id == file_id)          /* Right file... */
       && (lptr->id_hash == hash_value)      /* ...right hash... */
       && (lptr->id_len == id_len)           /* ...right length... */
       && (!memcmp(lptr->id, id, id_len)))   /* ...right id */
      {
       /* We have found this lock already in use. Is it ours? */

       if (lptr->owner == process.user_no)  /* Already own this lock */
        {
         if (lptr->lock_type == L_UPDATE)
          {
           if (!update) lptr->lock_type = L_SHARED;
           lptr->fvar_index = fvar->index;  /* Now for this fvar */
           lptr->txn_id = txn_id;
           goto exit_lock_record;
          }

         /* Existing lock is a shared lock. Are we upgrading? */

         if (!update)  /* No - just exit */
          {
           lptr->fvar_index = fvar->index;  /* Now for this fvar */
           lptr->txn_id = txn_id;
           goto exit_lock_record;
          }
         upgrade_idx = scan_idx;              /* Remember this cell */
        }
       else                                 /* Lock is owned by another user */
        {
         if (update || (lptr->lock_type == L_UPDATE))
          {
           status = lptr->owner;
           blocking_idx = scan_idx;

           if (sysseg->deadlock)
            {
             /* Check for a deadlock */

             u = lptr->owner;
             while((lwi = UserPtr(u)->lockwait_index) != 0)
              {
               if (lwi > 0)   /* Waiting for record lock */
                {
                 u = RLPtr(lwi)->owner;
                }
               else           /* Waiting for file lock */
                {
                 u = abs(FPtr(-lwi)->file_lock);
                }

               if (u == process.user_no)  /* Found ourself - It's a deadlock */
                {
                 u = lptr->owner;
                 tio_printf("\n");
                 tio_printf(sysmsg(1450), u, file_id, id_len, id);
                 while((lwi = UserPtr(u)->lockwait_index) != 0)
                  {
                   if (lwi > 0)   /* Waiting for record lock */
                    {
                     d_lptr = RLPtr(lwi);
                     tio_printf(sysmsg(1451),
                                d_lptr->file_id, d_lptr->id_len, d_lptr->id,
                                d_lptr->owner);
                     u = RLPtr(lwi)->owner;
                    }
                   else           /* Waiting for file lock */
                    {
                     d_fptr = FPtr(-lwi);
                     tio_printf(sysmsg(1452), -lwi, abs(d_fptr->file_lock));
                     u = abs(FPtr(-lwi)->file_lock);
                    }

                   if (u == process.user_no) break;
                  }

                 status = -2;  /* Deadlock */
                 break;
                }
              }
            }

           goto exit_lock_record;
          }
        }
      }
     else   /* Not the right lock */
      {
      }
     active_locks--;
    }
   else if (lptr->hash == 0)    /* Unused cell */
    {
     if (free_cell == 0) free_cell = scan_idx;
    }

   if (++scan_idx > sysseg->numlocks) scan_idx = 1;
   lptr = RLPtr(scan_idx);
  }

 /* We have now scanned all locks for this hash position.  If we are upgrading
    an existing shared lock, we did not find a conflict so do the upgrade. */

 if (upgrade_idx > 0)
  {
   lptr = RLPtr(upgrade_idx);
   lptr->lock_type = L_UPDATE;
   goto exit_lock_record;
  }

 /* Continue looking if we have yet to find a spare cell */

 if (free_cell == 0)
  {
   do {
       if (lptr->hash == 0)
        {
         free_cell = scan_idx;
         break;
        }

       if (++scan_idx > sysseg->numlocks) scan_idx = 1;
       lptr = RLPtr(scan_idx);

       if (scan_idx == idx)  /* Table full */
        {
         table_full = TRUE;
         status = -1;
         goto exit_lock_record;
        }
      } while(1);
  }

 /* Set up a new lock in the free cell */

 lptr = RLPtr(free_cell);
 lptr->hash = idx;
 lptr->owner = process.user_no;
 lptr->waiters = 0;
 lptr->lock_type = (update)?L_UPDATE:L_SHARED;
 lptr->file_id = file_id;
 lptr->id_hash = hash_value;
 lptr->fvar_index = fvar->index;
 lptr->txn_id = txn_id;
 lptr->id_len = id_len;
 memcpy(lptr->id, id, id_len);
 RLPtr(idx)->count += 1;
 if (++(sysseg->rl_count) > sysseg->rl_peak) sysseg->rl_peak = sysseg->rl_count;
 (fptr->lock_count)++;

 /* Create local lock table entry */

 llt = (LLT_ENTRY *)k_alloc(122, sizeof(LLT_ENTRY) + id_len - 1);
 if (llt != NULL)
  {
   llt->fno = file_id;
   llt->fvar_index = fvar->index;
   llt->id_len = id_len;
   memcpy(llt->id, id, id_len);
   llt->next = llt_head;
   llt_head = llt;
  }

exit_lock_record: 
 switch(status)
  {
   case 0:   /* Got the lock */
      my_uptr->lockwait_index = 0;
      break;

   case -1:  /* Lock table full */
      my_uptr->lockwait_index = 0;
      break;

   case -2:  /* Deadlock detected */
      if (sysseg->deadlock && !no_wait) break; /* Let deadlock system handle */
      /* *** FALL THROUGH *** */

   default:  /* Blocked by another user */
      if (!no_wait)
       {
        /* Note that we are waiting for this lock.  If lockwait_index is
           already non-zero, it must be for a subsequent attempt to get
           this lock so we must not increment the waiters counter.      */
        if (my_uptr->lockwait_index == 0)
         {
          if (blocking_idx >= 0) lptr->waiters++;   /* 0193 */
          my_uptr->lockwait_index = blocking_idx;
         }
       }
      break;
  }

 EndExclusive(REC_LOCK_SEM);

 /* Now that we have released the semaphore, log a message if the lock
    action failed because the lock table is full.                      */

 if (table_full) log_message("Lock table full");

 return status;
}

/* ======================================================================
   unlock_record()  -  Release a read or update lock on a record          */

bool unlock_record(
   FILE_VAR * fvar,        /* NULL if for all files */
   char * raw_id,          /* Record id. Null to release all on file */
   short int id_len)
{
 FILE_ENTRY * fptr;
 bool status = 0;
 short int file_id;
 short int i;
 LLT_ENTRY * llt;
 LLT_ENTRY * next_llt;


 if (fvar != NULL)
  {
   if (fvar->type == NET_FILE)
    {
     return net_unlock(fvar, raw_id, id_len) != 0;    /* 0274 */
    }

   file_id = fvar->file_id;
  }

 StartExclusive(REC_LOCK_SEM, 33);

 if (id_len == 0)   /* Unlock all records */
  {
   for(llt = llt_head; llt != NULL ; llt = next_llt)
    {
     next_llt = llt->next;
     if ((fvar == NULL)                      /* All files or... */
         || ((llt->fno == file_id)           /* ...correct file */
             && (llt->fvar_index == fvar->index)))
      {
       unlock_id(llt->fno, llt->id, llt->id_len);
       /* Note: on return, the original LLT entry will have been removed */
      }
    }

   /* Also unlock file lock(s) */

   if (fvar == NULL)  /* Unlock all file locks for this user */
    {
     for(i = 1, fptr = FPtr(1); i <= sysseg->used_files; i++, fptr++)
      {
       if (fptr->file_lock == process.user_no)
        {
         clear_waiters(-i);
         fptr->file_lock = 0;
        }
      }
    }
   else               /* Release specific file lock */
    {
     fptr = FPtr(file_id);
     if ((fptr->file_lock == process.user_no)
         && (fptr->fvar_index == fvar->index))
      {
       clear_waiters(-file_id);
       fptr->file_lock = 0;
      }
    }
   goto exit_unlock_record;
  }


 /* Unlock specific record */

 status = unlock_id(file_id, raw_id, id_len);

exit_unlock_record: 
 EndExclusive(REC_LOCK_SEM);

 return status;
}

/* ======================================================================
   Unlock specific file number / id pair                                  */

Private bool unlock_id(
   short int file_id,
   char * raw_id,
   short int id_len)
{
 bool status = FALSE;
 FILE_ENTRY * fptr;
 char u_id[MAX_ID_LEN];
 char * id;
 long int hash_value;
 short int scan_idx;     /* Scanning index */
 short int idx;
 RLOCK_ENTRY * lptr;
 short int active_locks;
 LLT_ENTRY * llt;
 LLT_ENTRY * prev_llt;

 fptr = FPtr(file_id);
 if (fptr->lock_count != 0)
  {
   if (fptr->flags & DHF_NOCASE)
    {
     memucpy(u_id, raw_id, id_len);
     id = u_id;                              /* 0261 */
    }
   else
    {
     id = raw_id;
    }

   hash_value = hash(id, id_len);
   idx = (short int)RLockHash(file_id, hash_value);

   scan_idx = idx;
   lptr = RLPtr(scan_idx);

   active_locks = lptr->count;   /* There are this many locks somewhere */
   while(active_locks)
    {
     if (lptr->hash == idx)
      {
       if ((lptr->file_id == file_id)          /* Right file... */
         && (lptr->owner == process.user_no)   /* ...right user... */
         && (lptr->id_hash == hash_value)      /* ...right hash... */
         && (lptr->id_len == id_len)           /* ...right id_len */
         && (!memcmp(lptr->id, id, id_len)))   /* ...right id */
        {
         /* We have found the lock */

         if (lptr->waiters) clear_waiters(scan_idx);
         lptr->hash = 0;    /* Free this cell */
         (RLPtr(idx)->count)--;
         (sysseg->rl_count)--;
         (fptr->lock_count)--;

         /* Remove from the local lock table too */

         for (llt = llt_head, prev_llt = NULL; llt != NULL; llt = llt->next)
          {
           if ((llt->fno == file_id)
               && (llt->id_len == id_len)
               && (!memcmp(llt->id, id, id_len)))
            {
             if (prev_llt == NULL) llt_head = llt->next;
             else prev_llt->next = llt->next;
             k_free(llt);
             break;
            }
           prev_llt = llt;
          }

         status = TRUE;
         goto exit_unlock_id;
        }
       else   /* Not the right lock */
        {
        }
       active_locks--;
      }

     if (++scan_idx > sysseg->numlocks) scan_idx = 1;
     lptr = RLPtr(scan_idx);
    }
  }

exit_unlock_id:
 return status;
}

/* ======================================================================
   unlock_txn()  -  Release all locks for a given transaction             */

void unlock_txn(unsigned long int txn)
{
 FILE_ENTRY * fptr;
 short int idx;
 RLOCK_ENTRY * lptr;
 short int i;


 StartExclusive(REC_LOCK_SEM, 33);

 for(idx = 1; idx <= sysseg->numlocks; idx++)
  {
   lptr = RLPtr(idx);
   if ((lptr->hash != 0)                   /* Cell used... */
      && (lptr->owner == process.user_no)  /* ...by this user... */
      && (lptr->txn_id == txn))            /* ...for this transaction */
    {
     (RLPtr(lptr->hash)->count)--;
     (sysseg->rl_count)--;
     (FPtr(lptr->file_id)->lock_count)--;
     if (lptr->waiters) clear_waiters(idx);
     lptr->hash = 0;    /* Free this cell */
    }
  }

 /* Also unlock file lock(s) */

 for(i = 1, fptr = FPtr(1); i <= sysseg->used_files; i++, fptr++)
  {
   if ((fptr->file_lock == process.user_no)
       && (fptr->txn_id == txn))
    {
     clear_waiters(-i);
     fptr->file_lock = 0;
     fptr->txn_id = 0;
    }
  }

 EndExclusive(REC_LOCK_SEM);
}

/* ======================================================================
   clear_waiters()  -  Clear lock wait entries                            */

void clear_waiters(short int idx)      /* Lock cell index */
{
 short int i;
 USER_ENTRY * uptr;
 RLOCK_ENTRY * lptr;

 /* We must clear the user table lock wait entry for any users that are
    waiting for this lock.  If we do not do so, the deadlock detection
    system can see what looks like a deadlock where a process was waiting
    for a lock that has just been released but has not yet managed to wake
    up and claim it.  If more than one user is waiting for the same lock,
    the unlucky ones will reinstate themselves when they next try to get
    the lock.  The only downside to this is that the deadlock system won't
    always abort the last process to wait.

    If the lock we are releasing is a shared read lock and there are other
    users also holding this lock, a number of situations may exist if a
    further user is waiting for an update lock:

    1. All the waiters are recorded against this lock cell.
       We will clear down the wait information and each user will
       re-establish it against the another entry for the same shared lock.

    2. All the waiters are recorded against another lock cell.
       We will not arrive in this routine.  This doesn't matter as the
       users for which we would otherwise clear the wait information are
       still waiting.

    3. Some of the waiters are recorded against this lock cell.
       We will clear some of the waiters.  All of them will subsequently
       retry the lock.  Those that we clear will move their waiter count
       to another lock entry.                                            */

 if (idx > 0) lptr = RLPtr(idx);

 for (i = 1; i <= sysseg->max_users; i++)
  {
   uptr = UPtr(i);
   if ((uptr->uid != 0)                                     /* Live user */
      && (uptr->lockwait_index == idx))                     /* This lock */
    {
     uptr->lockwait_index = 0;
     if (idx > 0)       /* Clearing wait on record lock */
      {
       if (--(lptr->waiters) == 0) break;
      }
    }
  }
}

/* ======================================================================
   clear_lockwait()  -  Clear lock wait for user                          */

void clear_lockwait()
{
 short int i;

 StartExclusive(REC_LOCK_SEM, 53);

 if ((i = my_uptr->lockwait_index) > 0)   /* Waiting for record lock */
  {
   (RLPtr(i)->waiters)--;
  }
 
 my_uptr->lockwait_index = 0;

 EndExclusive(REC_LOCK_SEM);
}

/* ======================================================================
   rebuild_llt()  -  Rebuild local lock table after UNLOCK                */

void rebuild_llt()
{
 /* The UNLOCK command allows a system administrator to release locks in
    another process. Because the local lock table is in our private
    memory, use of UNLOCK raises an event in the process that owns the
    lock being released. This event then calls this routine to rebuild
    the local lock table completely.                                    */

 LLT_ENTRY * llt;
 LLT_ENTRY * next_llt;
 short int idx;
 RLOCK_ENTRY * lptr;

 /* Give away all existing LLT entries */

 for (llt = llt_head; llt != NULL; llt = next_llt)
  {
   next_llt = llt->next;
   k_free(llt);
  }
 llt_head = NULL;

 /* Create new LLT chain */

 for(idx = 1; idx <= sysseg->numlocks; idx++)
  {
   lptr = RLPtr(idx);
   if ((lptr->hash != 0)                   /* Cell used... */
      && (lptr->owner == process.user_no)) /* ...by this user */
    {
     /* Create new local lock table entry */

     llt = (LLT_ENTRY *)k_alloc(123, sizeof(LLT_ENTRY) + lptr->id_len - 1);
     if (llt != NULL)
      {
       llt->fno = lptr->file_id;
       llt->fvar_index = lptr->fvar_index;
       llt->id_len = lptr->id_len;
       memcpy(llt->id, lptr->id, lptr->id_len);
       llt->next = llt_head;
       llt_head = llt;
      }
    }
  }
}

/* END-CODE */

