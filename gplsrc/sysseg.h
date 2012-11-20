/* SYSSEG.H
 * Shared memory definition.
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
 * START-HISTORY:
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 28 Aug 06  2.4-12 Added FIXUSERS parameter.
 * 27 Jun 06  2.4-5 Added file mapping table to user entry.
 * 05 Dec 05  2.2-18 Extracted from kernel.h
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */


/* System shared segment. */

typedef volatile struct SYSSEG SYSSEG;
struct SYSSEG {
   unsigned long int revstamp;   /* QM revstamp that create the memory */
   long int shmem_size;          /* Shared memory size */
   short int task_locks[64];     /* User number of lock owner. Zero if free.
                                    Protected by SHORT_CODE semaphore. */
   short int numfiles;           /* NUMFILES: Number of file table entries */
   short int used_files;         /* Number of used cells including embedded
                                    free cells. Protected by FILE_TABLE_LOCK */
   short int numlocks;           /* NUMLOCKS: Number of record lock table entries */
   short int num_glocks;         /* Number of group lock table entries */
   short int max_users;          /* User limit (all processes) */
   short int last_user;          /* Last user number allocated (cyclic) */
   short int hi_user_no;         /* Highest valid user number... */
     #define MIN_HI_USER_NO 1023 /* ...and its minimum value */
   int qmlnxd_pid;               /* PID of qmlnxd daemon */
   char sysdir[MAX_PATHNAME_LEN+1];
   short int cmdstack;           /* CMDSTACK: Command stack depth */
   bool deadlock;                /* DEADLOCK: Trap deadlocks? */
   int errlog;                   /* ERRLOG: Max size of errlog in bytes */
   short int fds_limit;
   long int fds_rotate;
   short int fixusers_base;      /* FIXUSERS: First user number and... */
   short int fixusers_range;     /*          ...Number of ports/users */
   short int maxidlen;           /* MAXIDLEN: Max record id length */
   short int netfiles;           /* NETFILES: 0x0001   Allow outgoing NFS
                                              0x0002   Allow incoming QMNet */
   short int pdump;              /* PDUMP:    0x0001   Ban dump of other username */
   short int portmap_base_port;  /* PORTMAP: First port number ... */
   short int portmap_base_user;  /*          ...First user number... */
   short int portmap_range;      /*          ...Number of ports/users */
   unsigned long int flags;
     #define SSF_SECURE     0x00000001   /* Secure mode? */
     #define SSF_SUSPEND    0x00000020   /* Suspend writes */
// MS 16 bits from from DEBUG configuration parameter
     #define SSF_PRTDEBUG   0x00010000   /* Debug printer actions */
     #define SSF_QMFIX      0x00020000   /* Allow QMFix in interactive mode */
     #define SSF_MONITOR    0x00040000   /* Run in monitor mode */
     #define SSF_INT_PDUMP  0x00080000   /* Allow PDUMP to dump internal mode */
     #define SSF_NO_FILE_CLEANUP 0x00100000 /* Inhibit cleanup of file table */
     #define SSF_VBSRVR_LOG 0x00200000   /* VBSRVR logs traffic */

   struct FILESTATS global_stats; /* Global file stats (see DH_STAT.H) */
   unsigned long next_txn_id;    /* Next transaction id */
   long int prtjob;              /* Print job number (SHORT_CODE semaphore) */
   short int jnlmode;            /* JNLMODE: Journal mode */
   int jnlseq;                   /* Journal file sequence no. 0 = inactive.
                                    Protected by JNL_SEM */
   char jnldir[MAX_PATHNAME_LEN+1]; /* JNLDIR: Journal file directory */
   char startup[80+1];           /* STARTUP: Startup command */
   /* Group lock counters (Protected by GROUP_LOCK_SEM) */
   unsigned long int gl_count;   /* Number of group locks obtained */
   unsigned long int gl_wait;    /* Group locks blocked on first attempt */
   unsigned long int gl_retry;   /* Group locks blocked on subsequent attempt */
   unsigned long int gl_scan;    /* Number of steps to obtain group lock */
   /* Record lock counters (Protected by REC_LOCK_SEM) */
   unsigned long int rl_count;   /* Current number of record locks */
   unsigned long int rl_peak;    /* Peak number of record locks */
   long int file_table;          /* Offset of file table */
   long int rlock_table;         /* Offset of record lock table... */
   short int rlock_entry_size;   /* ...and size of each entry */
   long int glock_table;         /* Offset of group lock table */
   long int semaphore_table;     /* Offset of semaphore owner table */
   long int user_table;          /* Offset of user table */
   short int user_entry_size;    /* Size of user table entry */
   long int user_map;            /* Offset of user map */
   long int pcode_offset;        /* Pcode */
   long int pcfg_offset;         /* Offset to template pcfg structure */
   int pcode_len;
};

Public SYSSEG * sysseg init(NULL);   /* 0234 */

/* Semaphores
   Aquisition sequence must always be decreasing semaphore number
   First: JNL_SEM          Single threads journal file updates
          FILE_TABLE_LOCK  Protects updates to file table.
          REC_LOCK_SEM     Protects updates to lock table and to lock_state in
                           file table entry.
          GROUP_LOCK_SEM   Protects group lock table.
   Last:  SHORT_CODE       Protects short in-line code sequences.
*/

/* ======================================================================
   Semaphore Table                                                        */

#define SHORT_CODE       0
#define ERRLOG_SEM       1
#define GROUP_LOCK_SEM   2
#define REC_LOCK_SEM     3
#define FILE_TABLE_LOCK  4
#define JNL_SEM          5
#define NUM_SEMAPHORES   6

typedef struct SEMAPHORE_ENTRY SEMAPHORE_ENTRY;
struct SEMAPHORE_ENTRY
 {
  short int owner;         /* QM user number of owner, zero if free, -ve for system processes */
  short int where;         /* Where was this last taken? (See MEM_TAGS) */
 };

Public char * sem_tags init("SHCLOGGLTRLTFLTJNL"); /* 3 chars per semaphore */

   Public int semid;

/* ======================================================================
   User Table                                                             */

typedef volatile struct USER_ENTRY USER_ENTRY;
struct USER_ENTRY
 {
  short int uid;                   /* Internal user id. Zero if spare cell.
                                      -1 = reserved for new phantom */
  long int pid;                    /* OS process id */
  short int puid;                  /* Parent user id. Zero if not phantom */
  long int login_time;             /* qmtime() at login */
  char username[MAX_USERNAME_LEN+1];  /* Login user name */
  char ip_addr[15+1];
  #define MAX_TTYNAME_LEN 15
  char ttyname[MAX_TTYNAME_LEN+1];
  short int flags;                 /* Also defined in INT$KEYS.H */
     #define USR_PHANTOM     0x0001 /* Is a phantom */
     #define USR_LOGOUT      0x0002 /* Logout in progress */
     #define USR_QMVBSRVR    0x0004 /* Is QMVbSrvr process */
     #define USR_ADMIN       0x0008 /* Administrator privileges */
     #define USR_QMNET       0x0010 /* Is QMNet (USR_QMVBSRVR also set) */
     #define USR_CHGPHANT    0x0020 /* "Chargeable" phantom; counts as licensed user */
     #define USR_MSG_OFF     0x0040 /* Message reception disabled */
     #define USR_WAKE        0x0080 /* Set by op_wake, cleared by op_pause */
  unsigned short int events;        /* Any bit set causes processing interrupt */
     #define EVT_LOGOUT      0x0001 /* Forced logout - immediate termination */
     #define EVT_STATUS      0x0002 /* Return status dump */
     #define EVT_UNLOAD      0x0004 /* Unload inactive cached object code */
     #define EVT_BREAK       0x0008 /* Set break inhibit to zero */
     #define EVT_HSM_ON      0x0010 /* Start HSM */
     #define EVT_HSM_DUMP    0x0020 /* Return HSM data */
     #define EVT_PDUMP       0x0040 /* Force process dump */
     #define EVT_FLUSH_CACHE 0x0080 /* Flush DH cache */
     #define EVT_JNL_SWITCH  0x0100 /* Switch journal file */
     #define EVT_TERMINATE   0x0200 /* Forced logout - graceful termination */
     #define EVT_MESSAGE     0x0400 /* Send immediate message */
     #define EVT_LICENCE     0x0800 /* Logout from licence expiry */
     #define EVT_REBUILD_LLT 0x1000 /* Rebuild local lock table */
  /* Lock wait data (protected by REC_LOCK_SEM) */

   short int lockwait_index;       /* 0 = not waiting,
                                      +ve = rec lock table index (record lock),
                                      -ve = file table index (file lock) */
   unsigned short int file_map[1];          /* Count of opens by file.
                                               Protected by FILE_TABLE_LOCK */
 };

/* UMap(n)    Returns pointer to user map entry for user n */
#define UMap(n) (((short int *)(((char *)sysseg) + sysseg->user_map)) + (n))

/* UserPtr(n) Returns pointer for table entry for user n */
#define UserPtr(n) ((*(UMap(n))!=0)?UPtr(*(UMap(n))):NULL)

/* UPtr(n)    Returns pointer for table index n, not user n (from 1) */
#define UPtr(n) ((USER_ENTRY *)(((char *)sysseg) + sysseg->user_table + (((long int)((n) - 1)) * sysseg->user_entry_size)))

/* UFMPtr(uptr,fno)  Pointer to user/file map table entry. Fno from 1 */
#define UFMPtr(uptr,fno) ((unsigned short int *)&((uptr)->file_map[(fno)-1]))

Public USER_ENTRY * my_uptr init(NULL);

/* END-CODE */
