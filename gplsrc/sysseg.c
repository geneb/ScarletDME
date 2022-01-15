/* SYSSEG.C
 * System shared segment management
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
 * 13Jan22 gwb Changed bind_sysseg() so that it returns a full error message if 
 *             the pcode load fails.  A numeric error doesn't help anyone.
 * 
 * 09Feb20 gwb Format specifier changes to clear out warnings.
 *
 * 27Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 * 27Feb20 gwb Changed some instances of "QM" to "ScarletDME"
 * 22Feb20 gwb Function declarations converted to ANSI style from K&R.
 *             Converted sprintf() in bind_sysseg() and start_qm() to
 *             snprintf().
 * 
 * START-HISTORY (OpenQM):
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 21 Mar 07  2.5-1 Use daemon() on NIX platforms to detach qmlnxd and startup
 *                  command process from parent.
 * 11 Mar 07  2.5-1 Ensure sysseg unbound at revstamp mismatch.
 * 18 Aug 06  2.4-11 Added STARTUP command handling.
 * 27 Jun 06  2.4-5 Added user/file mapping table to user entry.
 * 19 Jan 06  2.3-5 Moved Windows memory map file into sys subdirectory of
 *                  QMSYS account to allow permission settings.
 * 23 Nov 05  2.2-17 Exported dump_sysseg() to sysdump.c and special command
 *                   line option routines to clopts.c
 * 20 Oct 05  2.2-15 Load pcode into shared memory to reduce overheads.
 * 20 Sep 05  2.2-11 User map now dynamic.
 * 04 Aug 05  2.2-7 First use of -k tries graceful termination. Second use
 *                  forces immediate termination.
 * 21 Jun 05  2.2-1 Added journalling data.
 * 26 May 05  2.2-0 Added device licensing support.
 * 13 May 05  2.1-15 Linux portablility improved with change to get_semaphores()
 *                   as submitted by Jon A Kristofferson.
 * 31 Mar 05  2.1-11 Added PORTMAP variables.
 * 15 Oct 04  2.0-5 Added ERRLOG configuration parameter.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include <signal.h>
#include <time.h>

#include "qm.h"
#include "locks.h"
#include "config.h"
#include "revstamp.h"

/* Special interfaces in qmsem.c for use before shared memory created */

void LockSemaphore(int semno);
void UnlockSemaphore(int semno);

#define SYSSEG_REVSTAMP \
  (((u_int32_t)MAJOR_REV << 16) | ((u_int32_t)MINOR_REV << 8) | BUILD)

#include <sys/ipc.h>
#include <sys/shm.h>

void dump_config(void);

Private bool create_shared_segment(int32_t bytes,
                                   struct CONFIG* cfg,
                                   char* errmsg);

/* ====================================================================== */

bool bind_sysseg(bool create, char* errmsg) {
  /* 13Jan22 gwb A bit of refactoring.
   *         renamed "pcode_fu" to "pcode_fd"  It holds a file descriptor, 
   *         not whatever the hell "fu" is. :)
   */

  /* Linux only - Create rather than attach to existing? */

  bool status = FALSE;
  int32_t sharedMemSize;
  int32_t offset;
  int16_t num_glocks;
  struct CONFIG* cfg = NULL;
  int16_t max_users;
  int16_t rlock_entry_size;
  int16_t hi_user_no;
  int user_map_size;
  char path[MAX_PATHNAME_LEN + 1];
  int pcode_fd;
  struct stat statBuf;
  int pcode_len;
  int16_t user_entry_size;

  /* Get the semaphores. If this fails, return directly rather than jumping
    to the normal error exit as we do not need to release the semaphores.   */

  if (!get_semaphores(create, errmsg))
    return FALSE;

  /* 
   * Try to open existing shared segment.
   * Use SHORT_CODE semaphore to prevent two users trying to create the
   * shared segment at once.  We cannot use StartExclusive as this uses
   * the shared segment.                      
   */

  LockSemaphore(SHORT_CODE);

  if (attach_shared_memory()) {
    /* The shared memory is already initialised */

    UnlockSemaphore(SHORT_CODE);

    if (sysseg->revstamp != SYSSEG_REVSTAMP) {
      sprintf(errmsg, "Shared memory revstamp mismatch (%08X %08X)",
              sysseg->revstamp, SYSSEG_REVSTAMP);
      unbind_sysseg();
      goto exit_bind_sysseg;
    }

    if (create) {
      strcpy(errmsg, "ScarletDME is already started.");
      goto exit_bind_sysseg;
    }

    /* Copy the template pcfg structure to our private version */

    memcpy(&pcfg, ((char*)sysseg) + sysseg->pcfg_offset, sizeof(struct PCFG));
    status = TRUE;
    goto exit_bind_sysseg;
  }

  /* If we arrive here, the shared segment does not already exist.
    Create a new shared segment and populate it.                   */

  if (!create) {
    strcpy(errmsg, "ScarletDME has not been started.");
    goto exit_bind_sysseg;
  }

  /* Read the config file */

  if ((cfg = read_config(errmsg)) == NULL)
    goto exit_bind_sysseg;

  max_users = cfg->max_users;

  num_glocks = max_users * 3; /* Worst case in split, src, tgt, grp 0 */

  sharedMemSize = sizeof(SYSSEG);
  sharedMemSize += cfg->numfiles * sizeof(struct FILE_ENTRY);

  rlock_entry_size = (offsetof(RLOCK_ENTRY, id) + MAX_ID_LEN + 3) & ~3;
  sharedMemSize += cfg->numlocks * rlock_entry_size;

  sharedMemSize += num_glocks * sizeof(struct GLOCK_ENTRY);

  sharedMemSize += NUM_SEMAPHORES * sizeof(SEMAPHORE_ENTRY);

  user_entry_size =
      sizeof(struct USER_ENTRY) + (cfg->numfiles * sizeof(int16_t));
  sharedMemSize += max_users * user_entry_size;

  /* Work out size of user map based on highest user number. Users are
    allocated user numbers in a cycle from 1 to hi_user_no. This upper
    limit has a minimum value of MIN_HI_USER_NO but must be expanded if
    the licence results in more than this number of simultaneous processes. */

  hi_user_no = max(max_users, MIN_HI_USER_NO);
  user_map_size = (hi_user_no + 1) * sizeof(int16_t);
  sharedMemSize += user_map_size;

  /* Find size of pcode */
  /* converted to snprintf() -gwb 22Feb20 */
  if (snprintf(path, MAX_PATHNAME_LEN + 1, "%s%cbin%cpcode", cfg->sysdir, DS, DS) >= (MAX_PATHNAME_LEN + 1)) {
    fprintf(stderr, "Overflowed file/pathname length in bind_sysseg())!\n");
    goto exit_bind_sysseg;
  }
  if (((pcode_fd = open(path, O_RDONLY | O_BINARY)) < 0) || fstat(pcode_fd, &statBuf)) {
    /* return an error message, not a number that has to be looked up! */
    sprintf(errmsg, "Cannot read pcode file - %s.", strerror(errno)); 
    goto exit_bind_sysseg;
  }

  pcode_len = statBuf.st_size;
  sharedMemSize += pcode_len;

  /* Make space for template pcfg structure */

  sharedMemSize += sizeof(struct PCFG);

  /* Create the shared segment */

  if (!create_shared_segment(sharedMemSize, cfg, errmsg)) {
    goto exit_bind_sysseg;
  }

  memset((char*)sysseg, '\0', sharedMemSize);

  sysseg->revstamp = SYSSEG_REVSTAMP;
  sysseg->shmem_size = sharedMemSize;

  sysseg->flags |= SSF_SECURE; /* Secure until we know otherwise */
  sysseg->prtjob = 1;
  sysseg->next_txn_id = 1;

  /* Licence data */

  sysseg->max_users = max_users; /* Total, all process types */

  /* Copy global configuration parameters to sysseg */

  /* !!CONFIG!! */
  sysseg->cmdstack = cfg->cmdstack;                   /* CMDSTACK */
  sysseg->deadlock = cfg->deadlock;                   /* DEADLOCK */
  sysseg->errlog = cfg->errlog;                       /* ERRLOG */
  sysseg->fds_limit = cfg->fds_limit;                 /* FDS */
  sysseg->fixusers_base = cfg->fixusers_base;         /* FIXUSERS */
  sysseg->fixusers_range = cfg->fixusers_range;       /* FIXUSERS */
  sysseg->jnlmode = cfg->jnlmode;                     /* JNLMODE */
  strcpy((char*)(sysseg->jnldir), cfg->jnldir);       /* JNLDIR */
  sysseg->maxidlen = cfg->maxidlen;                   /* MAXIDLEN */
  sysseg->netfiles = cfg->netfiles;                   /* NETFILES */
  sysseg->pdump = cfg->pdump;                         /* PDUMP */
  sysseg->portmap_base_port = cfg->portmap_base_port; /* PORTMAP */
  sysseg->portmap_base_user = cfg->portmap_base_user; /* PORTMAP */
  sysseg->portmap_range = cfg->portmap_range;         /* PORTMAP */
  strcpy((char*)(sysseg->sysdir), cfg->sysdir);       /* QMSYS */
  strcpy((char*)(sysseg->startup), cfg->startup);     /* STARTUP */

  /* Create dynamically sized parts of segment */

  offset = sizeof(SYSSEG);
  sysseg->numfiles = cfg->numfiles;
  sysseg->file_table = offset;
  offset += (cfg->numfiles * sizeof(struct FILE_ENTRY));

  sysseg->numlocks = cfg->numlocks;
  sysseg->rlock_table = offset;
  sysseg->rlock_entry_size = rlock_entry_size;
  offset += (cfg->numlocks * rlock_entry_size);

  sysseg->num_glocks = num_glocks;
  sysseg->glock_table = offset;
  offset += (num_glocks * sizeof(struct GLOCK_ENTRY));

  sysseg->semaphore_table = offset;
  offset += NUM_SEMAPHORES * sizeof(SEMAPHORE_ENTRY);

  sysseg->user_table = offset;
  sysseg->user_entry_size = user_entry_size;
  offset += max_users * user_entry_size;

  sysseg->hi_user_no = hi_user_no;
  sysseg->user_map = offset;
  offset += user_map_size;

  /* Load pcode */

  sysseg->pcode_offset = offset;
  sysseg->pcode_len = pcode_len;

  if (read(pcode_fd, ((char*)sysseg) + offset, pcode_len) != pcode_len) {
    sprintf(errmsg, "Unable to load pcode [%d].\n", errno);
    goto exit_bind_sysseg;
  }

  close(pcode_fd);
  offset += pcode_len;

  /* Copy our pcfg to become the template for new processes */

  sysseg->pcfg_offset = offset;
  memcpy(((char*)sysseg) + offset, &pcfg, sizeof(struct PCFG));
  offset += sizeof(struct PCFG);

  UnlockSemaphore(SHORT_CODE);

  /* Reset file stats timer. This must be done after creating the dynamic
    tables as qmtime() may reference user table. Furthermore, because
    qmtime() may end up calling raise_event(), which requires the short
    code semaphore, we need to be very sinful and do this operation after
    releasing the semaphore that we already hold. Given that we are
    creating the shared memory, the risk of collision is extremely low
    and the effect would be unimportant anyway.                          */

  sysseg->global_stats.reset = qmtime();

  status = TRUE;

  if (cfg != NULL)
    k_free(cfg);

exit_bind_sysseg:

  if (!status)
    stop_qm();

  return status;
}

/* ======================================================================
   create_shared_segment()                                                */

Private bool create_shared_segment(int32_t bytes,
                                   struct CONFIG* cfg,
                                   char* errmsg) {
  int shmid;

  if ((shmid = shmget(QM_SHM_KEY, bytes, IPC_CREAT | 0666)) == -1) {
    sprintf(errmsg, "Error %d creating shared segment.", errno);
    return FALSE;
  }

  if ((sysseg = (SYSSEG*)shmat(shmid, NULL, 0)) == (void*)(-1)) {
    sprintf(errmsg, "Error %d attaching to new shared segment.", errno);
    return FALSE;
  }

  return TRUE;
}

/* ======================================================================
   Attach to shared memory segment                                        */

bool attach_shared_memory() {
  int shmid;

  if ((shmid = shmget(QM_SHM_KEY, 0, 0666)) != -1) {
    if ((sysseg = (SYSSEG*)shmat(shmid, NULL, 0)) == (void*)(-1)) {
      fprintf(stderr, "Error %d attaching to shared segment.\n", errno);
      return FALSE;
    }
    return TRUE;
  }

  return FALSE;
}

/* ====================================================================== */

void unbind_sysseg() {
  shmdt((void*)sysseg);
}

/* ======================================================================
   start_qm()                                                             */

bool start_qm() {
  char errmsg[80 + 1];
  int cpid;
  int i;
  char path[MAX_PATHNAME_LEN + 1];

  if (!bind_sysseg(TRUE, errmsg)) {
    fprintf(stderr, "%s\n", errmsg);
    return FALSE;
  }

  /* Start qmlnxd dameon */

  sysseg->qmlnxd_pid = -1; /* Stays -ve if fails to start */
  cpid = fork();
  if (cpid == 0) { /* Child process */
    for (i = 3; i < 1024; i++)
      close(i);
    daemon(1, 1);
    /* converted to snprintf() -gwb 22Feb20 */
    if (snprintf(path, MAX_PATHNAME_LEN + 1, "%s/bin/qmlnxd", sysseg->sysdir) >=
        (MAX_PATHNAME_LEN + 1)) {
      fprintf(stderr, "Overflowed file/pathname length in start_qm()!\n");
      return FALSE;
    } else
      execl(path, path, NULL);
  } else /* Parent process */
      // {
      // Moved to qmlnxd:   sysseg->qmlnxd_pid = cpid;   /* -ve if failed to start */
      // }

      /* Run startup command, if defined */

      if (sysseg->startup[0] != '\0') {
    cpid = fork();
    if (cpid == 0) { /* Child process */
      for (i = 3; i < 1024; i++)
        close(i);
      daemon(1, 1);
      /* converted to snprintf() -gwb 22Feb20 */
      if (snprintf(path, MAX_PATHNAME_LEN + 1, "%s/bin/qm", sysseg->sysdir) >=
          (MAX_PATHNAME_LEN + 1)) {
        fprintf(stderr, "Overflowed file/pathname length in start_qm()!\n");
        return FALSE;
      } else
        execl(path, path, "-aQMSYS", sysseg->startup, NULL);
    }
  }

  return TRUE;
}

/* ======================================================================
   stop_qm()                                                              */

bool stop_qm() {
  int shmid;
  struct shmid_ds shm;
  int16_t i;
  USER_ENTRY* uptr;

  if ((shmid = shmget(QM_SHM_KEY, 0, 0666)) != -1) {
    if (shmctl(shmid, IPC_STAT, &shm)) {
      fprintf(stderr, "Error %d getting shm status\n", errno);
      return FALSE;
    }

    if (shm.shm_nattch) {
      if ((sysseg = (SYSSEG*)shmat(shmid, NULL, 0)) != (void*)(-1)) {
        /* Send all QM processes the SIGTERM signal */

        for (i = 1; i <= sysseg->max_users; i++) {
          uptr = UPtr(i);
          if (uptr->uid) {
            kill(uptr->pid, SIGTERM);
          }
        }

        /* Shutdown the qmlnxd daemon if it is running */

        if (sysseg->qmlnxd_pid)
          kill(sysseg->qmlnxd_pid, SIGTERM);

        /* Dettach the shared memory */

        shmdt((void*)sysseg);
        sysseg = NULL;
      }

      for (i = 10; i; i--) {
        if (shmctl(shmid, IPC_STAT, &shm))
          break; /* Error getting data */
        if (shm.shm_nattch == 0)
          break; /* Everyone has gone */
        sleep(1);
      }
    }
  }

  if (shmid != -1) {
    if (shmctl(shmid, IPC_RMID, &shm)) {
      fprintf(stderr, "Error %d deleting shared memory\n", errno);
      return FALSE;
    }
  }

  delete_semaphores();

  return TRUE;
}

/* END-CODE */
