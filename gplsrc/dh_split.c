/* DH_SPLIT.C
 * Split or merge
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
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 * 
 * START-HISTORY (OpenQM):
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 04 May 05  2.1-13 Initial implementation of large file support.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include "dh_int.h"

/* ====================================================================== */

void dh_split(DH_FILE* dh_file) {
  FILE_ENTRY* fptr;
  int32_t modulus;
  int32_t load;
  int16_t group_bytes;

  /* Record being processed... */
  char id[MAX_ID_LEN + 1];

  /* Group to split... */
  DH_BLOCK* src_buff = NULL; /* Source buffer */
  int16_t src_group_lock = 0;
  int32_t group;    /* Group number */
  int16_t subfile; /* File part and... */
  int32_t sgrp;     /* ...position in source group */
  int16_t used_bytes;
  int16_t rec_offset;
  DH_RECORD* rec_ptr;

  /* Group to be created... */
  int16_t new_group_lock = 0;
  int32_t new_group; /* Group number */

  /* Target groups... */
  int16_t tgt; /* Index to following items */
  DH_BLOCK* tgt_buff[2] = {NULL, NULL};
  int16_t tgt_subfile[2];
  int32_t tgt_group[2];
  int16_t reqd_bytes;
  int16_t space;
  u_int32_t file_size;
  int32_t next_tgt_group;

  fptr = FPtr(dh_file->file_id);
  group_bytes = (int16_t)(dh_file->group_size);

  StartExclusive(FILE_TABLE_LOCK, 18);
  if (fptr->inhibit_count != 0) {
    EndExclusive(FILE_TABLE_LOCK);
    return;
  }

  /* Check we still need to do the split. We may have been beaten to it by
    another user.                                                         */

  modulus = fptr->params.modulus;
  load = DHLoad(fptr->params.load_bytes, group_bytes, modulus);
  if ((load <= fptr->params.split_load) &&
      (modulus >= fptr->params.min_modulus)) {
    EndExclusive(FILE_TABLE_LOCK);
    return;
  }

  if (dh_file->file_version < 2) {
    /* Check that we are not about to go over 2Gb */

    file_size = DHHeaderSize(dh_file->file_version, group_bytes);
    file_size += (u_int32_t)(fptr->params.modulus + 1) * group_bytes;
    if (file_size > (u_int32_t)0x80000000L) /* Do not do split */
    {
      EndExclusive(FILE_TABLE_LOCK);
      return;
    }
  }

  /* Calculate group to split */

  new_group = ++(fptr->params.modulus);
  if (new_group > fptr->params.mod_value)
    fptr->params.mod_value <<= 1;
  group = (new_group - 1) - (fptr->params.mod_value >> 1) + 1;

  /* Lock group to be created while we own the file table lock.  If we unlock
    the file table before doing this, another user could attempt to access
    the group before we have created it.                                     */

  new_group_lock = GetGroupWriteLock(dh_file, new_group);

  /* Increment the inhibit count to hold off other users starting a further
    split/merge in this file.                                              */

  (fptr->inhibit_count)++;
  fptr->stats.splits++;
  sysseg->global_stats.splits++;
  EndExclusive(FILE_TABLE_LOCK);

  /* Lock group to split now that we have released the file table lock.  If
    we do this with the file table lock, there may be some other user who
    cannot unlock this group without getting that lock first.              */

  src_group_lock = GetGroupWriteLock(dh_file, group);

  /* Set up buffers */

  dh_err = DHE_NO_MEM;
  if ((src_buff = (DH_BLOCK*)k_alloc(12, group_bytes)) == NULL)
    goto exit_dh_split;

  for (tgt = 0; tgt < 2; tgt++) {
    if ((tgt_buff[tgt] = (DH_BLOCK*)k_alloc(13, group_bytes)) == NULL) {
      goto exit_dh_split;
    }

    tgt_buff[tgt]->next = 0;
    tgt_buff[tgt]->used_bytes = BLOCK_HEADER_SIZE;
    tgt_buff[tgt]->block_type = DHT_DATA;
  }
  dh_err = 0;

  /* Process source group */

  subfile = PRIMARY_SUBFILE;
  sgrp = group;

  tgt_subfile[0] = PRIMARY_SUBFILE;
  tgt_group[0] = group;

  tgt_subfile[1] = PRIMARY_SUBFILE;
  tgt_group[1] = new_group;

  do {
    /* Read group */

    if (!dh_read_group(dh_file, subfile, sgrp, (char*)src_buff, group_bytes)) {
      goto exit_dh_split;
    }

    /* Scan group buffer for records */

    used_bytes = src_buff->used_bytes;
    if ((used_bytes == 0) || (used_bytes > group_bytes)) {
      log_printf(
          "DH_SPLIT: Invalid byte count (x%04X) in subfile %d, group %d\nof "
          "file %s\n",
          used_bytes, (int)subfile, sgrp, fptr->pathname);
      dh_err = DHE_POINTER_ERROR;
      goto exit_dh_split;
    }

    rec_offset = offsetof(DH_BLOCK, record);
    while (rec_offset < used_bytes) {
      rec_ptr = (DH_RECORD*)(((char*)src_buff) + rec_offset);

      memcpy(id, rec_ptr->id, rec_ptr->id_len);

      /* Write record to appropriate new group buffer */

      tgt = (dh_hash_group(fptr, id, rec_ptr->id_len) == new_group) ? 1 : 0;

      reqd_bytes = rec_ptr->next;
      space = group_bytes - tgt_buff[tgt]->used_bytes;
      if (reqd_bytes > space) /* Flush buffer and make an overflow block */
      {
        next_tgt_group = dh_get_overflow(dh_file, FALSE);

        if (next_tgt_group == 0) {
          /* Cannot allocate overflow block */
          goto exit_dh_split;
        }

        tgt_buff[tgt]->next = SetFwdLink(dh_file, next_tgt_group);

        if (!dh_write_group(dh_file, tgt_subfile[tgt], tgt_group[tgt],
                            (char*)tgt_buff[tgt], group_bytes)) {
          goto exit_dh_split;
        }

        tgt_subfile[tgt] = OVERFLOW_SUBFILE;
        tgt_group[tgt] = next_tgt_group;
        memset((char*)(tgt_buff[tgt]), '\0', group_bytes);
        tgt_buff[tgt]->used_bytes = BLOCK_HEADER_SIZE;
        tgt_buff[tgt]->block_type = DHT_DATA;
      }

      memcpy(((char*)(tgt_buff[tgt])) + tgt_buff[tgt]->used_bytes,
             (char*)rec_ptr, reqd_bytes);
      tgt_buff[tgt]->used_bytes += reqd_bytes;
      rec_offset += rec_ptr->next;
    }

    /* If this was an overflow block, give it away */

    if (subfile == OVERFLOW_SUBFILE) {
      dh_free_overflow(dh_file, sgrp);
    }

    /* Move to next group buffer */

    subfile = OVERFLOW_SUBFILE;
    sgrp = GetFwdLink(dh_file, src_buff->next);
  } while (sgrp != 0);

  /* All done. Flush both output buffers */

  for (tgt = 0; tgt < 2; tgt++) {
    if (!dh_write_group(dh_file, tgt_subfile[tgt], tgt_group[tgt],
                        (char*)tgt_buff[tgt], group_bytes)) {
      goto exit_dh_split;
    }
  }

  /* Update file header */

  dh_file->flags |= FILE_UPDATED;
  dh_flush_header(dh_file);

exit_dh_split:
  if (src_group_lock != 0)
    FreeGroupWriteLock(src_group_lock);
  if (new_group_lock != 0)
    FreeGroupWriteLock(new_group_lock);

  /* Decrement the inhibit count now that we have finished */

  StartExclusive(FILE_TABLE_LOCK, 34);
  (fptr->inhibit_count)--;
  EndExclusive(FILE_TABLE_LOCK);

  if (src_buff != NULL)
    k_free(src_buff);
  if (tgt_buff[0] != NULL)
    k_free(tgt_buff[0]);
  if (tgt_buff[1] != NULL)
    k_free(tgt_buff[1]);
}

/* ====================================================================== */

void dh_merge(DH_FILE* dh_file) {
  FILE_ENTRY* fptr;
  int32_t modulus;
  int32_t load;
  int16_t group_bytes;
  int16_t rec_bytes;

  /* Source group... */
  int32_t src_group;
  int16_t src_lock = 0;
  DH_BLOCK* src_buff = NULL;
  int16_t src_subfile;
  int32_t sgrp;
  int16_t src_used_bytes;
  int16_t src_rec_offset;
  DH_RECORD* src_rec_ptr;

  /* Target group... */
  int32_t tgt_group;
  int16_t tgt_lock = 0;
  DH_BLOCK* tgt_buff = NULL;
  int16_t tgt_subfile;
  int32_t tgrp;
  int32_t next_tgt_group;
  int32_t n;

  fptr = FPtr(dh_file->file_id);
  group_bytes = (int16_t)(dh_file->group_size);

  StartExclusive(FILE_TABLE_LOCK, 19);
  if (fptr->inhibit_count != 0) {
    EndExclusive(FILE_TABLE_LOCK);
    return;
  }

  /* Check we still need to do the merge. We may have been beaten to it by
    another user.                                                         */

  modulus = fptr->params.modulus;
  load = DHLoad(fptr->params.load_bytes, group_bytes, modulus);
  if ((load >= fptr->params.merge_load) ||
      (modulus <= fptr->params.min_modulus)) {
    EndExclusive(FILE_TABLE_LOCK);
    return;
  }

  src_group = modulus;
  tgt_group = (src_group - 1) - (fptr->params.mod_value >> 1) + 1;

  /* Lock both groups, target group first */

  tgt_lock = GetGroupWriteLock(dh_file, tgt_group);
  src_lock = GetGroupWriteLock(dh_file, src_group);

  /* Increment the inhibit count to hold off other users starting a further
    split/merge in this file.                                             */

  (fptr->inhibit_count)++;
  fptr->stats.merges++;
  sysseg->global_stats.merges++;

  /* Adjust file parameters
    0081 Moved this code from much later. We must adjust the parameters
    before we release the file table lock otherwise the following scenario
    can occur:
       DH_MERGE                DH_READ
       Lock file table
                               Wait for file table
       Lock group
       Unlock file table
                               Lock file table
                               Calculate group
                               Wait group lock
       Do merge
       Unlock group
                               Lock group (wrong group)
 */

  n = fptr->params.mod_value >> 1;
  if ((--(fptr->params.modulus)) == n)
    fptr->params.mod_value = n;

  EndExclusive(FILE_TABLE_LOCK);

  /* Set up buffers */

  dh_err = DHE_NO_MEM;
  if ((src_buff = (DH_BLOCK*)k_alloc(14, group_bytes)) == NULL)
    goto exit_dh_merge;
  if ((tgt_buff = (DH_BLOCK*)k_alloc(15, group_bytes)) == NULL)
    goto exit_dh_merge;
  dh_err = 0;

  /* Find end of target group */

  tgt_subfile = PRIMARY_SUBFILE;
  tgrp = tgt_group;
  while (1) {
    if (!dh_read_group(dh_file, tgt_subfile, tgrp, (char*)tgt_buff,
                       group_bytes)) {
      goto exit_dh_merge;
    }

    if (tgt_buff->next == 0)
      break;

    tgt_subfile = OVERFLOW_SUBFILE;
    tgrp = GetFwdLink(dh_file, tgt_buff->next);
  }

  /* Do the merge */

  src_subfile = PRIMARY_SUBFILE;
  sgrp = src_group;
  while (sgrp != 0) {
    if (!dh_read_group(dh_file, src_subfile, sgrp, (char*)src_buff,
                       group_bytes)) {
      goto exit_dh_merge;
    }

    /* Copy records to target group */

    src_used_bytes = src_buff->used_bytes;
    if ((src_used_bytes == 0) || (src_used_bytes > group_bytes)) {
      log_printf(
          "DH_MERGE: Invalid byte count (x%04X) in subfile %d group %d\nof "
          "file %s\n",
          src_used_bytes, (int)src_subfile, sgrp, fptr->pathname);
      dh_err = DHE_POINTER_ERROR;
      goto exit_dh_merge;
    }

    src_rec_offset = offsetof(DH_BLOCK, record);
    while (src_rec_offset < src_used_bytes) {
      src_rec_ptr = (DH_RECORD*)(((char*)src_buff) + src_rec_offset);
      rec_bytes = src_rec_ptr->next;

      if (rec_bytes > group_bytes - tgt_buff->used_bytes) /* Need overflow */
      {
        next_tgt_group = dh_get_overflow(dh_file, FALSE);
        if (next_tgt_group == 0) {
          /* Cannot allocate overflow block */
          goto exit_dh_merge;
        }

        tgt_buff->next = SetFwdLink(dh_file, next_tgt_group);

        if (!dh_write_group(dh_file, tgt_subfile, tgrp, (char*)tgt_buff,
                            group_bytes)) {
          goto exit_dh_merge;
        }

        tgt_subfile = OVERFLOW_SUBFILE;
        tgrp = next_tgt_group;
        memset((char*)tgt_buff, '\0', group_bytes);
        tgt_buff->used_bytes = BLOCK_HEADER_SIZE;
        tgt_buff->block_type = DHT_DATA;
      }

      memcpy(((char*)tgt_buff) + tgt_buff->used_bytes, (char*)src_rec_ptr,
             rec_bytes);
      tgt_buff->used_bytes += rec_bytes;

      src_rec_offset += rec_bytes;
      ;
    }

    /* If this was an overflow block, release it */

    if (src_subfile == OVERFLOW_SUBFILE) {
      dh_free_overflow(dh_file, sgrp);
    }

    src_subfile = OVERFLOW_SUBFILE;
    sgrp = GetFwdLink(dh_file, src_buff->next);
  }

  /* Flush final target buffer */

  if (!dh_write_group(dh_file, tgt_subfile, tgrp, (char*)tgt_buff,
                      group_bytes)) {
    goto exit_dh_merge;
  }

  /* Clear and write back the released primary group to mark it as free space */

  memset((char*)src_buff, 0, group_bytes);
  if (!dh_write_group(dh_file, PRIMARY_SUBFILE, src_group, (char*)src_buff,
                      group_bytes)) {
    goto exit_dh_merge;
  }

  /* Update file header */

  dh_file->flags |= FILE_UPDATED;
  dh_flush_header(dh_file);

exit_dh_merge:
  if (src_lock != 0)
    FreeGroupWriteLock(src_lock);
  if (tgt_lock != 0)
    FreeGroupWriteLock(tgt_lock);

  /* Decrement the inhibit count now that we have finished */

  StartExclusive(FILE_TABLE_LOCK, 35);
  (fptr->inhibit_count)--;
  EndExclusive(FILE_TABLE_LOCK);

  if (src_buff != NULL)
    k_free(src_buff);
  if (tgt_buff != NULL)
    k_free(tgt_buff);
}

/* END-CODE */
