/* DH_FILE.C
 * Block level file handling functions
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
 
 * 17Oct25 mab Change dyn file prefix to %
 * 15Jan22 gwb Fixed argument formatting issues (CwE-686) 
 * 
 * 29Feb20 gwb Changed LONG_MAX to INT32_MAX.  When building for a 64 bit 
 *             platform, the LONG_MAX constant overflows the size of the
 *             int32_t variable type.  This change needed to be made across
 *             the entire code base.
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 * 22Feb20 gwb Cleaned up a variable assigned but unused warning.
 * 
 * START-HISTORY (OpenQM):
 * 01 Jul 07  2.5-7 Extensive changes for PDA merge.
 * 21 Nov 06  2.4-17 Revised interface to dio_open() and removed dh_open_path().
 * 04 Apr 06  2.4-1 Added restart_tx_ref() to handle situations where the tx_ref
 *                  value cycles. This would only ever impact a system that has
 *                  hit the FDS limit although the routine will be called for
 *                  all systems where the process does over 2^31 disk accesses.
 * 04 Apr 06  2.4-1 dh_filesize() now processes subfiles separately.
 * 28 Mar 06  2.3-9 Adde dh_fsync().
 * 17 Mar 06  2.3-8 Save record count when flushing header.
 * 23 Feb 06  2.3-7 dh_filesize() now returns int64.
 * 16 Oct 05  2.2-15 Report FDS_open() failures.
 * 11 Oct 05  2.2-14 Moved all FDS handling into dio_open().
 * 10 Oct 05  2.2-14 Implemented akpath element of DH_FILE.
 * 30 Sep 05  2.2-13 Made FDS_Close() public.
 * 20 Sep 05  2.2-11 Use dynamic buffers in dh_get_overflow() and
 *                   dh_free_overflow() to minimise stack space.
 * 31 Aug 05  2.2-9 Modified dio_open() to look for ENFILE and ECHILD as well as
 *                  EMFILE as signs of running out of files. Linux appears to
 *                  return ECHILD in this situation.
 * 12 May 05  2.1-15 Shortened group lock retry sleep period.
 * 04 May 05  2.1-13 Initial implementation of large file support.
 * 06 Apr 05  2.1-12 Extended log messages at fatal errors to include errno.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * dh_fsync            Perform fsync() on file
 * dh_init             Initialise DH subsystem
 * dh_open_subfile     Open/create a subfile to the DH_FILE structure
 * dh_close_subfile    Close a subfile in DH_FILE, handling FDS
 * dh_close_file       Close a file unit, handling FDS
 * dh_shutdown         Close all files at shutdown
 * FDS_open            Open subfile by number using FDS if not already open
 * FDS_close           Close oldest subfile in FDS
 * dio_open            Interlude to sopen() for FDS actions
 * dh_read_group       Read data from DH subfile
 * dh_write_group      Write data to DH subfile
 * dh_get_overflow     Find a free overflow block in a DH file
 * dh_free_overflow    Release an overflow block in a DH file
 * dh_flush_header     Flush the DH file header to disk
 * dh_get_group_lock   Set a group lock
 * dh_free_group_lock  Free a group lock
 * dh_configure        Set new DH file parameters
 * read_at             Read from given address in file unit
 * write_at            Write to given address in file unit
 * dh_set_subfile      Copy a file unit into the DH_FILE/FDS system
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include "dh_int.h"
#include "locks.h"
#include "options.h"
#include "config.h"
#include <sched.h>
#include <stdint.h> 

int OpenFile(char* path, int mode, int rights);

Private int32_t tx_ref = 0; /* Running transfer count for FDS sequence */
Private int16_t FDS_open_count = 0;

Private void restart_tx_ref(void);

bool FDS_open(DH_FILE* dh_file, int16_t subfile);

/* ====================================================================== */

void dh_fsync(DH_FILE* dh_file, int16_t subfile) {
  if (!ValidFileHandle(dh_file->sf[subfile].fu)) {
    if (!FDS_open(dh_file, subfile)) {
      log_printf("DH_FSYNC: FDS open failure %d on %s subfile %d.\n",
                 process.os_error, FPtr(dh_file->file_id)->pathname, subfile);
      return;
    }
  }

  fsync(dh_file->sf[subfile].fu);
}

/* ====================================================================== */

bool dh_init() {
  /* Currently none */
  return TRUE;
}

/* ====================================================================== */

bool dh_open_subfile(DH_FILE* dh_file,
                     char* pathname,
                     int16_t subfile,
                     bool create) {
  bool status = FALSE;
  char filename[MAX_PATHNAME_LEN + 1];
  int mode;

  if (dh_file->flags & DHF_RDONLY)
    mode = DIO_READ;
  else if (create)
    mode = DIO_REPLACE;
  else
    mode = DIO_UPDATE;

  if ((subfile >= AK_BASE_SUBFILE) && (dh_file->akpath != NULL)) {
// 17Oct25 mab Change dyn file prefix to %
    sprintf(filename, "%s%c%%%d", dh_file->akpath, DS, (int)subfile);
  } else {
    sprintf(filename, "%s%c%%%d", pathname, DS, (int)subfile);
  }

  dh_file->sf[subfile].fu = dio_open(filename, mode);
  if (!ValidFileHandle(dh_file->sf[subfile].fu))
    goto exit_dh_open_subfile;

  FDS_open_count++;
  dh_file->sf[subfile].tx_ref = tx_ref++; /* Prevent immediate close */
  if (tx_ref < 0)
    restart_tx_ref();

  status = TRUE;

exit_dh_open_subfile:
  return status;
}

/* ====================================================================== */

void dh_close_subfile(DH_FILE* dh_file, int16_t subfile) {
  if (ValidFileHandle(dh_file->sf[subfile].fu)) {
    CloseFile(dh_file->sf[subfile].fu);
    dh_file->sf[subfile].fu = INVALID_FILE_HANDLE;
    FDS_open_count--;
  }
}

/* ====================================================================== */

void dh_close_file(OSFILE fu) {
  CloseFile(fu);
  FDS_open_count--;
}

/* ====================================================================== */

void dh_shutdown() {
  DH_FILE* p;
  DH_FILE* next;

  for (p = dh_file_head; p != NULL; p = next) {
    next = p->next_file;
    p->open_count = 1; /* Force close */
    dh_close(p);
  }
}

/* ====================================================================== */

int64 dh_filesize(DH_FILE* dh_file, int16_t subfile) {
  if (!FDS_open(dh_file, subfile))
    return -1;

  return filelength64(dh_file->sf[subfile].fu);
}

/* ====================================================================== */

bool FDS_open(DH_FILE* dh_file, int16_t subfile) {
  bool status = FALSE;

  if (!ValidFileHandle(dh_file->sf[subfile].fu)) {
    if (!dh_open_subfile(dh_file, (char*)(FPtr(dh_file->file_id)->pathname),
                         subfile, FALSE)) {
      dh_err = DHE_FDS_OPEN_ERR;
      process.os_error = OSError;
      goto exit_fds_open;
    }
  }

  status = TRUE;

exit_fds_open:
  return status;
}

/* ====================================================================== */

void FDS_close() {
  DH_FILE* p;
  int16_t i;
  int32_t low_ref;
  DH_FILE* dh_file = NULL;
  int16_t subfile;

  /* Close the file with the lowest transfer reference number */

  for (p = dh_file_head; p != NULL; p = p->next_file) {
    low_ref = INT32_MAX;  // was LONG_MAX.  See 29Feb20 comment at the top.
    for (i = 0; i < p->no_of_subfiles; i++) {
      if ((ValidFileHandle(p->sf[i].fu)) && (p->sf[i].tx_ref < low_ref)) {
        dh_file = p;
        subfile = i;
        low_ref = p->sf[i].tx_ref;
      }
    }
  }

  if (dh_file != NULL)
    dh_close_subfile(dh_file, subfile);
}

/* ======================================================================
   dio_open()  -  Open a file with FDS actions                            */

OSFILE dio_open(char* fn, int mode) {
  OSFILE fu;

  int open_modes;

  switch (mode) {
    case DIO_NEW: /* Create new file, fail if exists */
      open_modes = O_RDWR | O_CREAT | O_EXCL | O_BINARY;
      break;
    case DIO_REPLACE: /* Create new file, overwriting if exists */
      open_modes = O_RDWR | O_CREAT | O_TRUNC | O_BINARY;
      break;
    case DIO_READ: /* Open existing file, read only */
      open_modes = O_RDONLY | O_BINARY;
      break;
    case DIO_UPDATE: /* Open existing file, read/write */
      open_modes = O_RDWR | O_BINARY;
      break;
    case DIO_OVERWRITE: /* Open file, creating if doesn't exist */
      open_modes = O_RDWR | O_CREAT | O_BINARY;
      break;
  }

  /* Check if we are about to hit the FDS limit */

  if (FDS_open_count == sysseg->fds_limit)
    FDS_close();

  fu = OpenFile(fn, open_modes, default_access);
  if (!ValidFileHandle(fu) &&
      ((errno == EMFILE) || (errno == ENFILE) || (errno == ECHILD))) {
    sysseg->fds_rotate++;
    FDS_close();
    fu = OpenFile(fn, open_modes, default_access);
  }

  if (!ValidFileHandle(fu))
    process.os_error = OSError;

  return fu;
}

/* ======================================================================
   dh_read_group()  -  Read a group buffer                                */

bool dh_read_group(DH_FILE* dh_file,
                   int16_t subfile,
                   int32_t group,
                   char* buff,
                   int16_t bytes) {
  FILE_ENTRY* fptr;
  int64 offset;

  if (group) {
    if (subfile < AK_BASE_SUBFILE) {
      offset = GroupOffset(dh_file, group);
    } else {
      offset = ((int64)group - 1) * DH_AK_NODE_SIZE + dh_file->ak_header_bytes;
    }
  } else
    offset = 0;

  if (!ValidFileHandle(dh_file->sf[subfile].fu)) {
    if (!FDS_open(dh_file, subfile)) {
      fptr = FPtr(dh_file->file_id);
      log_printf("DH_READ_GROUP: FDS open failure %d on %s subfile %d.\n",
                 process.os_error, fptr->pathname, (int)subfile);
      return FALSE;
    }
  }

  dh_file->sf[subfile].tx_ref = tx_ref++;
  if (tx_ref < 0)
    restart_tx_ref();

  if (Seek(dh_file->sf[subfile].fu, offset, SEEK_SET) < 0) {
    process.os_error = OSError;
    fptr = FPtr(dh_file->file_id);
    log_printf("DH_READ_GROUP: Seek error %d on %s subfile %d, group %d.\n",
               process.os_error, fptr->pathname, (int)subfile, group);
    dh_err = DHE_SEEK_ERROR;
    return FALSE;
  }

  if (Read(dh_file->sf[subfile].fu, buff, bytes) < 0) {
    process.os_error = OSError;
    fptr = FPtr(dh_file->file_id);
    log_printf("DH_READ_GROUP: Read error %d on %s subfile %d, group %d.\n",
               process.os_error, fptr->pathname, (int)subfile, group);
    dh_err = DHE_READ_ERROR;
    return FALSE;
  }

  return TRUE;
}

/* ======================================================================
   dh_write_group()  -  Write a group buffer                              */

bool dh_write_group(DH_FILE* dh_file,
                    int16_t subfile,
                    int32_t group,
                    char* buff,
                    int16_t bytes) {
  int64 offset;
  FILE_ENTRY* fptr;

  if (group) {
    if (subfile < AK_BASE_SUBFILE) {
      offset = GroupOffset(dh_file, group);
    } else {
      offset = ((int64)group - 1) * DH_AK_NODE_SIZE + dh_file->ak_header_bytes;
    }
  } else
    offset = 0;

  if (!ValidFileHandle(dh_file->sf[subfile].fu)) {
    if (!FDS_open(dh_file, subfile)) {
      fptr = FPtr(dh_file->file_id);
      log_printf("DH_WRITE_GROUP: FDS open failure %d on %s subfile %d.\n",
                 process.os_error, fptr->pathname, (int)subfile);
      return FALSE;
    }
  }

  dh_file->sf[subfile].tx_ref = tx_ref++;
  if (tx_ref < 0)
    restart_tx_ref();

  if (Seek(dh_file->sf[subfile].fu, offset, SEEK_SET) < 0) {
    process.os_error = OSError;
    log_printf("DH_WRITE_GROUP: Seek error %d on %s subfile %d, group %d.\n",
               process.os_error, FPtr(dh_file->file_id)->pathname, (int)subfile,
               group);
    dh_err = DHE_SEEK_ERROR;
    return FALSE;
  }

  if (Write(dh_file->sf[subfile].fu, buff, bytes) < 0) {
    process.os_error = OSError;
    log_printf("DH_WRITE_GROUP: Write error %d on %s subfile %d, group %d.\n",
               process.os_error, FPtr(dh_file->file_id)->pathname, (int)subfile,
               group);
    dh_err = DHE_WRITE_ERROR;
    return FALSE;
  }

  return TRUE;
}

/* ======================================================================
   dh_get_overflow()  -  Get overflow block(s)
   Returns zero if unable to allocate block.                              */

int32_t dh_get_overflow(
    DH_FILE* dh_file,
    bool have_group_lock) /* Process already owns header group lock. Also
                             we do not flush the header if we are keeping
                             an existing lock.                            */
{
  int16_t header_lock;
  DH_BLOCK overflow_block;
  int32_t ogrp;
  int64 offset;
  FILE_ENTRY* fptr;
  char* buff = NULL;
  int16_t group_bytes;
  int16_t subfile = OVERFLOW_SUBFILE;

  fptr = FPtr(dh_file->file_id);

  /* The file table free chain item is protected by the group lock on group 0 */

  if (!have_group_lock)
    header_lock = GetGroupWriteLock(dh_file, 0);

  if ((ogrp = fptr->params.free_chain) != 0) {
    if (!dh_read_group(dh_file, subfile, ogrp, (char*)(&overflow_block),
                       BLOCK_HEADER_SIZE)) {
      ogrp = 0;
      goto exit_get_overflow;
    }

    fptr->params.free_chain = GetFwdLink(dh_file, overflow_block.next);
  } else {
    /* Must make new overflow block(s) */

    group_bytes = (int16_t)(dh_file->group_size);
    buff = (char*)k_alloc(74, group_bytes);

    if (!ValidFileHandle(dh_file->sf[subfile].fu)) {
      if (!FDS_open(dh_file, subfile)) {
        log_printf("DH_GET_OVERFLOW: FDS open failure %d on %s subfile %d.\n",
                   process.os_error, fptr->pathname, subfile);
        return FALSE;
      }
    }

    dh_file->sf[subfile].tx_ref = tx_ref++;
    if (tx_ref < 0)
      restart_tx_ref();

    if ((offset = Seek(dh_file->sf[subfile].fu, (int64)0, SEEK_END)) == -1) {
      dh_err = DHE_SEEK_ERROR;
      process.os_error = OSError;
      ogrp = 0;
      goto exit_get_overflow;
    }

    if (dh_file->file_version < 2) {
      /* Check not about to go over 2Gb */

      if ((((u_int32_t)offset) + group_bytes) > 0x80000000L) {
        dh_err = DHE_SIZE;
        ogrp = 0;
        goto exit_get_overflow;
      }
    }

    memset(buff, '\0', group_bytes);

    if (Write(dh_file->sf[subfile].fu, buff, group_bytes) < 0) {
      process.os_error = OSError;
      log_printf("DH_GET_OVERFLOW: Write error %d on %s subfile %d at %ld.\n",
                 process.os_error, FPtr(dh_file->file_id)->pathname, subfile,
                 offset);
      dh_err = DHE_WRITE_ERROR;
      ogrp = 0;
      goto exit_get_overflow;
    }

    ogrp = (int32_t)(((offset - dh_file->header_bytes) / group_bytes) + 1);
  }

exit_get_overflow:
  if (!have_group_lock) {
    dh_flush_header(dh_file);
    FreeGroupWriteLock(header_lock);
  }

  if (buff != NULL)
    k_free(buff);
  dh_file->flags |= FILE_UPDATED;

  return ogrp;
}

/* ======================================================================
   dh_free_overflow()  -  Add a block to the free overflow chain          */

void dh_free_overflow(DH_FILE* dh_file, int32_t ogrp) {
  int16_t header_lock;
  char* buff;
  int16_t group_bytes;
  FILE_ENTRY* fptr;

  fptr = FPtr(dh_file->file_id);
  group_bytes = (int16_t)(dh_file->group_size);

  buff = (char*)k_alloc(103, group_bytes);
  memset(buff, '\0', group_bytes);

  header_lock = GetGroupWriteLock(dh_file, 0);
  ((DH_BIG_BLOCK*)buff)->next = SetFwdLink(dh_file, fptr->params.free_chain);
  if (dh_write_group(dh_file, OVERFLOW_SUBFILE, ogrp, buff, group_bytes)) {
    fptr->params.free_chain = ogrp;
  }
  FreeGroupWriteLock(header_lock);

  k_free(buff);
}

/* ======================================================================
   dh_flush_header()  -  Write file header                                */

bool dh_flush_header(DH_FILE* dh_file) {
  DH_HEADER header;
  FILE_ENTRY* fptr;

  /* Save if we have updated the file or statistics counting is enabled */

  fptr = FPtr(dh_file->file_id);
  if (((dh_file->flags & FILE_UPDATED) || fptr->stats.reset) &&
      !(dh_file->flags & DHF_RDONLY)) {
    dh_file->flags &= ~FILE_UPDATED;

    if (!ValidFileHandle(dh_file->sf[PRIMARY_SUBFILE].fu)) {
      if (!FDS_open(dh_file, PRIMARY_SUBFILE)) {
        log_printf("DH_FLUSH_HEADER: FDS open failure %d on %s subfile %d.\n",
                   process.os_error, fptr->pathname, PRIMARY_SUBFILE);
        return FALSE;
      }
    }

    dh_file->sf[PRIMARY_SUBFILE].tx_ref = tx_ref++;
    if (tx_ref < 0)
      restart_tx_ref();

    if (!read_at(dh_file->sf[PRIMARY_SUBFILE].fu, (int64)0, (char*)(&header),
                 DH_HEADER_SIZE)) {
      return FALSE;
    }

    header.params.modulus = fptr->params.modulus;
    header.params.min_modulus = fptr->params.min_modulus;
    header.params.big_rec_size = fptr->params.big_rec_size;
    header.params.split_load = fptr->params.split_load;
    header.params.merge_load = fptr->params.merge_load;
    header.params.load_bytes =
        (u_int32_t)(fptr->params.load_bytes & 0xFFFFFFFF);
    header.params.extended_load_bytes =
        (int16_t)(fptr->params.load_bytes >> 32);
    header.params.mod_value = fptr->params.mod_value;
    header.params.longest_id = fptr->params.longest_id;
    header.params.free_chain = SetFwdLink(dh_file, fptr->params.free_chain);
    header.record_count = fptr->record_count + 1; /* See dh_fmt.h */

    header.stats = fptr->stats;

    if (!write_at(dh_file->sf[PRIMARY_SUBFILE].fu, (int64)0, (char*)(&header),
                  DH_HEADER_SIZE)) {
      return FALSE;
    }

    if (pcfg.fsync & 0x0001) {
      dh_fsync(dh_file, PRIMARY_SUBFILE);
      dh_fsync(dh_file, OVERFLOW_SUBFILE);
    }
  }

  dh_file->flags |= FILE_UPDATED;

  return TRUE;
}

/* ======================================================================
   dh_get_group_lock()  -  Set a group lock                             */

int16_t dh_get_group_lock(DH_FILE* dh_file, int32_t group, bool write_lock) {
  int16_t file_id;
  int16_t idx;      /* Index of hash cell */
  int16_t scan_idx; /* Scanning index */
  GLOCK_ENTRY* lptr;
  int16_t active_locks;
  int16_t free_cell;
  static int16_t pause_ct = 5;
  bool retry = FALSE;
  // FILE_ENTRY* fptr; variable set but not used.
  int steps = 0;

  file_id = dh_file->file_id;
  idx = GLockHash(file_id, group);
  // fptr = FPtr(file_id);  //variable set but not used.

again:
  scan_idx = idx;
  lptr = GLPtr(scan_idx);
  free_cell = 0;

  StartExclusive(GROUP_LOCK_SEM, 7);

  /* Scan to see if this lock is already owned, remembering any blank cell */

  active_locks = lptr->count; /* There are this many locks somewhere */
  while (active_locks) {
    steps++;
    if (lptr->hash == idx) {
      if ((lptr->file_id == file_id) && (lptr->group == group)) {
        if (!write_lock) {
          if (lptr->grp_count >= 0) /* Read lock - can increment count */
          {
            lptr->grp_count++;
            lptr->owner = process.user_no; /* Use newest owner */

            sysseg->gl_count++;
            sysseg->gl_scan += steps;
            EndExclusive(GROUP_LOCK_SEM);
            return scan_idx;
          }
        }

        goto wait_for_lock; /* Must wait */
      } else {
        /* Not the right lock */
      }
      active_locks--;
    } else if (lptr->hash == 0) { /* Unused cell */
      if (free_cell == 0)
        free_cell = scan_idx;
    }

    if (++scan_idx > sysseg->num_glocks) {
      scan_idx = 1;
      lptr = GLPtr(1);
    } else {
      lptr++;
    }

    if (scan_idx == idx)
      goto wait_for_lock; /* Table full */
  }

  /* Continue looking if we have yet to find a spare cell */

  if (free_cell == 0) {
    do {
      steps++;

      if (lptr->hash == 0) {
        free_cell = scan_idx;
        break;
      }

      if (++scan_idx > sysseg->num_glocks) {
        scan_idx = 1;
        lptr = GLPtr(1);
      } else {
        lptr++;
      }

      if (scan_idx == idx)
        goto wait_for_lock; /* Table full */
    } while (1);
  }

  /* Set up a new lock in the free cell */

  lptr = GLPtr(free_cell);
  lptr->hash = idx;
  lptr->owner = process.user_no;
  lptr->file_id = file_id;
  lptr->group = group;
  lptr->grp_count = (write_lock) ? -1 : 1;

  GLPtr(idx)->count += 1;

  EndExclusive(GROUP_LOCK_SEM);

  sysseg->gl_count++;
  sysseg->gl_scan += steps;
  return free_cell;

wait_for_lock:
  /* We need to wait for another user to give away this (or another) group
     lock.  Release the lock we hold on the group lock table and then pause
     briefly before trying again.
     It is not good enough simply to give away our timeslice as a lower
     priority process might own the lock and we would end up spinning
     through the scheduler without giving it a chance to give the lock away.
     A timed wait would potentially result in unacceptable performance. As
     a compromise, we give away our time slice but on every tenth attempt we
     pause for one millisecond.                                             */

  if (retry)
    sysseg->gl_retry++;
  else
    sysseg->gl_wait++;

  EndExclusive(GROUP_LOCK_SEM);

  if (--pause_ct) {
    RelinquishTimeslice;
  } else {
    Sleep(1);
    pause_ct = 10;
  }

  retry = TRUE;
  goto again;
}

/* ======================================================================
   dh_free_group_lock()  -  Free a group lock                             */

void dh_free_group_lock(int16_t slot) {
  GLOCK_ENTRY* lptr;
  GLOCK_ENTRY* hash_lptr;

  lptr = GLPtr(slot);
  StartExclusive(GROUP_LOCK_SEM, 8);

  if (lptr->grp_count < 0) /* Write lock */
  {
    hash_lptr = GLPtr(lptr->hash);
    lptr->hash = 0;
    (hash_lptr->count)--;
  } else /* Read lock */
  {
    if (--(lptr->grp_count) == 0) {
      hash_lptr = GLPtr(lptr->hash);
      lptr->hash = 0;
      (hash_lptr->count)--;
    }
  }

  EndExclusive(GROUP_LOCK_SEM);
}

/* ======================================================================
   dh_configure()  - Change file parameters                               */

void dh_configure(DH_FILE* dh_file,
                  int32_t min_modulus,
                  int16_t split_load,
                  int16_t merge_load,
                  int32_t large_rec_size) {
  FILE_ENTRY* fptr;
  fptr = FPtr(dh_file->file_id);

  StartExclusive(FILE_TABLE_LOCK, 9);
  if (min_modulus >= 0)
    fptr->params.min_modulus = min_modulus;
  if (split_load >= 0)
    fptr->params.split_load = split_load;
  if (merge_load >= 0)
    fptr->params.merge_load = merge_load;
  if (large_rec_size >= 0)
    fptr->params.big_rec_size = large_rec_size;
  dh_file->flags |= FILE_UPDATED;
  EndExclusive(FILE_TABLE_LOCK);

  dh_flush_header(dh_file);
}

/* ======================================================================
   read_at()  -  Read from given file position                            */

bool read_at(OSFILE fu, int64 offset, char* buff, int bytes) {
  if (Seek(fu, offset, SEEK_SET) < 0) {
    dh_err = DHE_SEEK_ERROR;
    process.os_error = OSError;
    return FALSE;
  }

  if (Read(fu, buff, bytes) < 0) {
    dh_err = DHE_READ_ERROR;
    process.os_error = OSError;
    return FALSE;
  }

  return TRUE;
}

/* ======================================================================
   write_at()  -  Write at given file position                            */

bool write_at(OSFILE fu, int64 offset, char* buff, int bytes) {
  if (Seek(fu, offset, SEEK_SET) < 0) {
    dh_err = DHE_SEEK_ERROR;
    process.os_error = OSError;
    return FALSE;
  }

  if (Write(fu, buff, bytes) < 0) {
    process.os_error = OSError;
    log_printf("DH_WRITE_AT: Write error %d\n", process.os_error);
    dh_err = DHE_WRITE_ERROR;
    return FALSE;
  }

  return TRUE;
}

/* ======================================================================
   dh_set_subfile  -  Copy a file unit number into the DH_FILE system,
   adjusting the FDS open count.                                          */

void dh_set_subfile(DH_FILE* dh_file, int16_t subfile, OSFILE fu) {
  dh_file->sf[subfile].fu = fu;
  FDS_open_count++;
}

/* ======================================================================
   restart_tx_ref()  -  Recover from tx_ref cycling

   A busy system might cycle the value of tx_ref such that it becomes
   negative. The effect will be that the FDS algorithm will close the
   wrong file and continue to do so for a long time with a serious impact
   on performance. This routine is called if the tx_ref value goes
   negative. We then reset the tx_ref figure of every DH subfile back
   to one. Files may get closed once in error but things will quickly
   sort themselves out.                                                   */

Private void restart_tx_ref() {
  DH_FILE* p;
  int16_t sf;

  for (p = dh_file_head; p != NULL; p = p->next_file) {
    for (sf = 0; sf < p->no_of_subfiles; sf++) {
      p->sf[sf].tx_ref = 1;
    }
  }
  tx_ref = 2;
}

/* ======================================================================
   OpenFile()  -  Open a file, marking as not inheritable                 */

int OpenFile(char* path, int mode, int rights) {
  OSFILE fu;
  int flags;

  fu = open(path, mode, rights);
  if (fu >= 0) {
    flags = fcntl(fu, F_GETFD);
    flags |= FD_CLOEXEC;
    fcntl(fu, F_SETFD, flags);
  }
  return fu;
}

/* ======================================================================
   SetFileSize()  -  Change file size                                     */

bool SetFileSize(OSFILE fu, int64 bytes) {
  chsize64(fu, bytes);
  return TRUE;
}

/* END-CODE */
