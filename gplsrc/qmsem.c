/* QMSEM.C
 * Semaphore functions.
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
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 19 Mar 07  2.5-1 Handle errors from semop() when locking semaphore.
 * 26 Jun 06  2.4-5 Extracted from kernel.c
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"


   #include <sys/sem.h>


/* ======================================================================
   get_semaphores()  -  Get inter-process semaphores                      */

bool get_semaphores(create, errmsg)
   bool create;     /* Linux only - Create rather than attach to existing? */
   char * errmsg;
{
 short int i;


 union semun
  {
   int val;                     /* Value for SETVAL */
   struct semid_ds * buf;       /* Buffer for IPC_STAT, IPC_SET */
   unsigned short int * array;  /* Array for GETALL, SETALL */
   struct seminfo * __buf;      /* Buffer for IPC_INFO */
  } seminit;

 seminit.val = 1;

 if ((semid = semget(QM_SEM_KEY, 0, 0666)) != -1)
  {
   /* Semaphores already exist */

   if (!create) return TRUE;

   strcpy(errmsg, "QM is already started");
   return FALSE;
  }

 if (errno != ENOENT)
  {
   sprintf(errmsg, "Error %d getting semaphores", errno);
   return FALSE;
  }

 /* Semaphores do not already exist */

 if (!create)
  {
   strcpy(errmsg, "QM is not started");
   return FALSE;
  }

 /* Create new semaphores */

 if ((semid = semget(QM_SEM_KEY, NUM_SEMAPHORES, IPC_CREAT | IPC_EXCL | 0666)) != -1)
  {
   /* Initialise the semaphores */

   for (i = 0; i < NUM_SEMAPHORES; i++) semctl(semid, i, SETVAL, seminit);
  }
 else
  {
   sprintf(errmsg, "Error %d allocating semaphores", errno);
   return FALSE;
  }

 return TRUE;
}

/* ====================================================================== */

void delete_semaphores()
{
 if ((semid = semget(QM_SEM_KEY, 0, 0666)) != -1)
  {
   semctl(semid, 0, IPC_RMID);
  }
}

/* ======================================================================
   Variants on StartExclusive and EndExclusive for use with no shared mem */

void LockSemaphore(int semno)
{

 static struct sembuf sem_lock = {0,-1,0};
 sem_lock.sem_num = semno;
 while(semop(semid, &sem_lock, 1)) {}
}

void UnlockSemaphore(int semno)
{

 static struct sembuf sem_unlock = {0,1,0};
 sem_unlock.sem_num = semno;
 semop(semid, &sem_unlock, 1);
}

/* ====================================================================== */

void StartExclusive(int semno, short int where)
{
 register SEMAPHORE_ENTRY * semptr;

 LockSemaphore(semno);
 semptr = (((SEMAPHORE_ENTRY *)(((char *)sysseg) + sysseg->semaphore_table)) + (semno));
 semptr->owner = process.user_no;
 semptr->where = where;
}

void EndExclusive(int semno)
{
 register SEMAPHORE_ENTRY * semptr;

 semptr = (((SEMAPHORE_ENTRY *)(((char *)sysseg) + sysseg->semaphore_table)) + (semno));
 semptr->owner = 0;
 UnlockSemaphore(semno);
}

/* END-CODE */
