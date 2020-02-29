/* LOCKS.H
 * Lock structure definitions
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
 * 27Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 * 
 * START-HISTORY (OpenQM):
 * 15 Oct 07  2.6-5 Added local lock table structure.
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 24 Feb 06  2.3-7 Padded lock table entries to be four byte multiples.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

/* ==================== RECORD LOCKS ==================== */

/* Shared segment cyclic hash lock table
   For internal purposes, cells are numbered from 1.
   Cyclic hash index = ((hash ^ file_id) % num_locks) + 1
*/

typedef struct RLOCK_ENTRY RLOCK_ENTRY;
struct RLOCK_ENTRY {
  int16_t hash;    /* Table hash index of entry using this cell.
                              Zero if cell is free */
  int16_t count;   /* Number of locks hashing to this cell */
  int16_t owner;   /* User id of owner */
  int16_t waiters; /* Count of users waiting for lock. */
  int16_t lock_type;
#define L_UPDATE 1
#define L_SHARED 2
  int16_t file_id;    /* Index to file table */
  int32_t id_hash;    /* Record's hash value */
  int32_t fvar_index; /* Cross-ref to FILE_VAR */
  u_int32_t txn_id;   /* Transaction id (0 if outside transaction) */
  int16_t id_len;
  int16_t pad; /* Pad to 4 byte multiple */
  char id[MAX_ID_LEN];
};

#define RLPtr(n)                                            \
  ((RLOCK_ENTRY*)((((char*)sysseg) + sysseg->rlock_table) + \
                  ((n - 1) * sysseg->rlock_entry_size)))

#define RLockHash(f, h) ((((f) ^ (h)) % sysseg->numlocks) + 1)

/* =================== LOCAL LOCK TABLE ==================== */

/* The local lock table holds a private memory reference to all record
   locks owned by the process. Using this table removes the need to scan
   the entire record lock table when closing a file or using the forms
   of RELEASE that do not include a record id.
   The table is maintained by lock_record() and unlock_record() in
   op_lock.c. There is an event (EVT_REBUILD_LLT) that is raised by the
   UNLOCK command to rebuild the local lock table if a lock is released
   in another process by an administrator.                                */

typedef struct LLT_ENTRY LLT_ENTRY;
struct LLT_ENTRY {
  LLT_ENTRY* next;    /* Next local lock table entry */
  int16_t fno;        /* File table index */
  int16_t id_len;     /* Length of record id */
  int32_t fvar_index; /* Links lock to specific instance of file */
  char id[1];         /* The record id */
};
Public LLT_ENTRY* llt_head init(NULL);

/* ==================== GROUP LOCKS ==================== */

/* Shared segment cyclic hash lock table
   For internal purposes, cells are numbered from 1.
   Cyclic hash index = ((group ^ file_id) % num_locks) + 1
*/

typedef struct GLOCK_ENTRY GLOCK_ENTRY;
struct GLOCK_ENTRY {
  int16_t hash;      /* Table hash index of entry using this cell.
                                Zero if cell is free */
  int16_t count;     /* Number of locks hashing to this cell */
  int16_t owner;     /* User id of owner */
  int16_t file_id;   /* Index to file table */
  int32_t group;     /* Group number. For AK subfiles, see below */
  int16_t grp_count; /* +ve = read count, -ve = write lock */
  int16_t pad;       /* Pad to 4 byte multiple */
};

#define GLPtr(n) \
  (((GLOCK_ENTRY*)(((char*)sysseg) + sysseg->glock_table)) + (n - 1))
#define GLockHash(f, g) ((((f) ^ (g)) % sysseg->num_glocks) + 1)

/* Pseudo group locks for AK subfile */

/* AKGlock() defines a pseudo group number used to lock the entire AK
   subfile during a read or write action.  This is constructed from the
   AK number (from zero) or'ed with 0x40000000.                         */

#define AKGlock(akno) (akno | 0x40000000L)

/* AKRlock() defines a pseudo group number used as a record update lock
   by _AK while updating an AK.  The key item is formed by xor'ing together
   all byte pairs within the key, including the trailing \0 if the key is
   of odd length.                                                         */

#define AKRlock(akno, key) ((((int32_t)akno) << 16) | key | 0x20000000L)

void clear_waiters(int16_t idx);
void clear_lockwait(void);

/* END-CODE */
