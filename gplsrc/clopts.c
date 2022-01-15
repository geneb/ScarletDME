/* CLOPTS.C
 * Special command line option processing
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
 * 15Jan22 gwb Fixed argument formatting issues (CwE-686) 
 * 
 * 10Jan22 gwb Fixed a format specifier warning.
 *
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 * 22Feb20 gwb Changed printed references of QM to ScarletDME.
 *             Cleared a warning regarding an unused variable.
 * 
 * START-HISTORY (OpenQM):
 * 01 Jul 07  2.5-7 Extensive changes for PDA merge.
 * 18 Jun 07  2.5-7 Extended qm -u to show ttyname and ip address, not just one.
 * 11 May 07  2.5-3 Log use of EVT_TERMINATE.
 * 26 Jan 07  2.4-20 0538 recover_user() decremented the file reference count
 *                   twice if there was a file lock.
 * 19 Dec 06  2.4-18 Allow kill_user() by login name.
 * 28 Jun 06  2.4-5 The file table file_lock entry holds a negative user number
 *                  during a CLEARFILE operation. The cleanup process must allow
 *                  for this when checking for an active file lock.
 * 27 Jun 06  2.4-5 Added cleanup processing.
 * 26 Jun 06  2.4-5 Use kill() to check for process existence as FreeBSD does
 *                  not use the /proc database.
 * 21 Jun 06  2.4-5 Clear down user if process is dead in qm -k.
 * 23 Nov 05  2.2-17 Exported from sysseg.c
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 * Processes these command line options:
 * -K         Kill user(s)
 * -RECOVER   Recover users
 * -U         Show users
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include <signal.h>
#include "qm.h"
#include "locks.h"


Private void kill_process(USER_ENTRY* uptr);
Private bool process_exists(int pid);
Private void remove_user(USER_ENTRY* uptr);

/* ======================================================================
   recover_users()  -  Recover licence space for vanished users           */

bool recover_users() {
  bool status = FALSE;
  USER_ENTRY* uptr;
  int32_t pid;
  int16_t u;
  int16_t user_no;

  /* Be brutal - Lock everything in sight */

  StartExclusive(FILE_TABLE_LOCK,
                 45); /* TODO: Magic numbers are bad, mmmkay? */
  StartExclusive(REC_LOCK_SEM, 45);
  StartExclusive(GROUP_LOCK_SEM, 45);
  StartExclusive(SHORT_CODE, 45);

  for (u = 1; u <= sysseg->max_users; u++) {
    uptr = UPtr(u);
    user_no = uptr->uid;
    pid = uptr->pid;
    if (uptr->uid) {
      if (!process_exists(pid)) {
        remove_user(uptr);
        tio_printf("Removed user %d (pid %d)\n", (int)user_no, pid);
        status = TRUE;
      }
    }
  }

  EndExclusive(SHORT_CODE);
  EndExclusive(GROUP_LOCK_SEM);
  EndExclusive(REC_LOCK_SEM);
  EndExclusive(FILE_TABLE_LOCK);

  return status;
}

/* ======================================================================
 * show_users()  -  Display user information (QM -U)
 * TODO: this may not show IPv6 addresses properly -gwb 22Feb20
 */

void show_users() {
  int i;
  USER_ENTRY* uptr;
  char origin[50];

  if (!attach_shared_memory()) {
    fprintf(stderr, "ScarletDME is not active.\n");
    return;
  }

  /* Users
     0         1         2         3         4         5         6         7
     01234567890123456789012345678901234567890123456789012345678901234567890123456789
      Uid Pid........ Puid Origin.................
     Username........................ 1234 12345678901 1234 telnet
     123.123.123.123  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
 */

  printf(" Uid Pid........ Puid Origin................. Username\n");
  for (i = 1; i <= sysseg->max_users; i++) {
    uptr = UPtr(i);
    if (uptr->uid != 0) {
      strcpy(origin, (char*)(uptr->ttyname));
      if (origin[0] != '\0')
        strcat(origin, " ");
      strcat(origin, (char*)(uptr->ip_addr));
      printf("%4hd %11d %4d %-23s %-32s\n", uptr->uid, uptr->pid,
             (int)(uptr->puid), origin, uptr->username);
    }
  }

  unbind_sysseg();
}

/* ======================================================================
   kill_user()  -  Kill user process command line option                  */

void kill_user(char* user) {
  USER_ENTRY* uptr;
  int16_t u;
  char errmsg[80 + 1];
  int16_t uid;
  char* p;

  if (!attach_shared_memory()) {
    fprintf(stderr, "ScarletDME is not active.\n");
    return;
  }

  if (!get_semaphores(FALSE, errmsg)) {
    fprintf(stderr, "Cannot access semaphores.\n");
    return;
  }

  StartExclusive(FILE_TABLE_LOCK,
                 68); /* TODO: Magic numbers are bad, mmmkay? */
  StartExclusive(REC_LOCK_SEM, 68);
  StartExclusive(GROUP_LOCK_SEM, 68);
  StartExclusive(SHORT_CODE, 68);

  if (user == NULL) { /* Kill all users */
    log_printf("External request to terminate all ScarletDME users.\n");
    for (u = 1; u <= sysseg->max_users; u++) {
      uptr = UPtr(u);
      if (uptr->uid != 0) {
        uptr->events |= (uptr->flags & USR_LOGOUT) ? EVT_LOGOUT : EVT_TERMINATE;
        uptr->flags |= USR_LOGOUT;
      }
    }
  } else {              /* Kill specific user */
    if (IsDigit(*user)) { /* Kill user by user number */
      for (p = user, uid = 0; IsDigit(*p); p++)
        uid = (uid * 10) + (*p - '0');
      if ((*p != '\0') || (uid < 1) || (uid > sysseg->hi_user_no)) {
        fprintf(stderr, "Invalid user number.\n");
      } else {
        log_printf("External request to terminate ScarletDME user %d.\n", uid);
        uptr = UserPtr(uid);
        if (uptr != NULL)
          kill_process(uptr);
        else
          fprintf(stderr, "User %d is not active.\n", uid);
      }
    } else { /* Kill user by login name */
      log_printf(
          "External request to terminate ScarletDME sessions for user %2.\n",
          user);
      for (u = 1; u <= sysseg->max_users; u++) {
        uptr = UPtr(u);
        if ((uptr->uid != 0) && !stricmp((char*)(uptr->username), user)) {
          kill_process(uptr);
        }
      }
    }
  }

  EndExclusive(SHORT_CODE);
  EndExclusive(GROUP_LOCK_SEM);
  EndExclusive(REC_LOCK_SEM);
  EndExclusive(FILE_TABLE_LOCK);

  unbind_sysseg();
}

/* ======================================================================
   kill_process()  -  Kill a ScarletDME process */

Private void kill_process(USER_ENTRY* uptr) {
  int16_t user_no;
  int pid;

  user_no = uptr->uid;
  pid = uptr->pid;

  /* Check that the process actually exists */

  if (process_exists(pid)) {
    uptr->events |= (uptr->flags & USR_LOGOUT) ? EVT_LOGOUT : EVT_TERMINATE;
    uptr->flags |= USR_LOGOUT;
  } else {
    remove_user(uptr);
    printf("Removed user %d (pid %d).\n", (int)user_no, pid);
  }
}

/* ======================================================================
   cleanup()  -  Clean up user tables from lost processes                 */

void cleanup() {
  USER_ENTRY* uptr;
  int16_t u;
  int16_t user_no;
  int pid;
  char username[MAX_USERNAME_LEN + 1]; /* Login user name */
  char errmsg[80 + 1];

  if (!attach_shared_memory()) {
    fprintf(stderr, "ScarletDME is not active.\n");
    return;
  }

  if (!get_semaphores(FALSE, errmsg)) {
    fprintf(stderr, "Cannot access semaphores.\n");
    return;
  }

  StartExclusive(FILE_TABLE_LOCK, 59); /* TODO: Magic numbers are bad, mmkay? */
  StartExclusive(REC_LOCK_SEM, 59);
  StartExclusive(GROUP_LOCK_SEM, 59);
  StartExclusive(SHORT_CODE, 59);

  for (u = 1; u <= sysseg->max_users; u++) {
    uptr = UPtr(u);
    if (uptr->uid != 0) {
      pid = uptr->pid;
      if (!process_exists(pid)) {
        user_no = uptr->uid;
        strcpy(username, (char*)(uptr->username));
        remove_user(uptr);
        log_printf("Cleanup removed user %d (pid %d, %s).\n", (int)user_no, pid,
                   username);
      }
    }
  }

  EndExclusive(SHORT_CODE);
  EndExclusive(GROUP_LOCK_SEM);
  EndExclusive(REC_LOCK_SEM);
  EndExclusive(FILE_TABLE_LOCK);

  unbind_sysseg();
}

/* ======================================================================
   suspend_resume()                                                       */

void suspend_resume(bool suspend) {
  if (!attach_shared_memory()) {
    fprintf(stderr, "ScarletDME is not active.\n");
    return;
  }

  if (suspend)
    sysseg->flags |= SSF_SUSPEND;
  else
    sysseg->flags &= ~SSF_SUSPEND;

  unbind_sysseg();
}

/* ====================================================================== */

Private bool process_exists(int pid) {
  return (!kill(pid, 0) || (errno == EPERM));
}

/* ====================================================================== */

Private void remove_user(USER_ENTRY* uptr) {
  int16_t i;
  int16_t user_no;
  /* int32_t pid; value set but never used. */
  FILE_ENTRY* fptr;
  RLOCK_ENTRY* lptr;
  GLOCK_ENTRY* gptr;
  u_int16_t* ufm;

  user_no = uptr->uid;
  /* pid = uptr->pid; value set but never used. */

  /* Give away process locks */

  for (i = 0; i < 64; i++) {
    if (sysseg->task_locks[i] == process.user_no)
      sysseg->task_locks[i] = 0;
  }

  /* Give away file locks */

  for (i = 1; i <= sysseg->used_files; i++) {
    fptr = FPtr(i);
    if (fptr->ref_ct != 0) /* File entry is in use */
    {
      if (abs(fptr->file_lock) == user_no) {
        fptr->file_lock = 0;
        clear_waiters(-i);
        // 0538       (fptr->ref_ct)--;   /* Must have been open to us */
      }
    }
  }

  /* Give away record locks */

  for (i = 1; i <= sysseg->numlocks; i++) {
    lptr = RLPtr(i);

    if ((lptr->hash != 0) && (lptr->owner == user_no)) {
      /* We have found a lock to release */
      (RLPtr(lptr->hash)->count)--;
      (sysseg->rl_count)--;
      (FPtr(lptr->file_id)->lock_count)--;
      lptr->hash = 0; /* Free this cell */
      if (lptr->waiters)
        clear_waiters(i);
    }
  }

  /* Give away group locks */

  for (i = 1; i <= sysseg->num_glocks; i++) {
    gptr = GLPtr(i);

    if ((gptr->hash != 0) && (gptr->owner == user_no)) {
      /* We have found a lock to release */
      (GLPtr(gptr->hash)->count)--;
      gptr->hash = 0; /* Free this cell */
    }
  }

  /* Give away file table entries */

  if (!(sysseg->flags & SSF_NO_FILE_CLEANUP)) {
    for (i = 1; i <= sysseg->numfiles; i++) {
      ufm = UFMPtr(uptr, i);
      if (*ufm) {
        /* The following must allow for a reference count of -1 which
           indicates exclusive access to the file.                    */

        fptr = FPtr(i);
        fptr->ref_ct = abs(fptr->ref_ct) - *ufm;
      }
    }
  }

  /* Release user table entry */

  ReleaseLicence(uptr);
}

/* END-CODE */
