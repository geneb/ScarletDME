/* DH_EXIST.C
 * Check if record exists on file.
 * Copyright (c) 2005 Ladybridge Systems, All Rights Reserved
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
 * 01 Jul 07  2.5-7 Extensive changes for PDA merge.
 * 19 Sep 05  2.2-11 Use dh_buffer to reduce stack space.
 * 11 May 05  2.1-14 Removed use of GroupOffset() in log_printf() call.
 * 04 May 05  2.1-13 Initial implementation of large file support.
 * 23 Sep 04  2.0-2 Added support for case insensitive ids.
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

bool dh_exists(DH_FILE* dh_file, /* File descriptor */
               char id[],        /* Record id... */
               int16_t id_len) /* ...and length */
{
  int32_t group;
  int16_t group_bytes;
  int lock_slot;
  DH_BLOCK* buff;
  DH_RECORD* rec_ptr;
  FILE_ENTRY* fptr;
  int16_t subfile;
  bool group_locked = FALSE;
  int16_t rec_offset;
  int16_t used_bytes;
  int32_t grp;

  dh_err = DHE_RECORD_NOT_FOUND;
  process.os_error = 0;

  buff = (DH_BLOCK*)(&dh_buffer);

  fptr = FPtr(dh_file->file_id);
  while (fptr->file_lock < 0)
    Sleep(1000); /* Clearfile in progress */

  /* Lock group */

  StartExclusive(FILE_TABLE_LOCK, 6);

  group = dh_hash_group(fptr, id, id_len);
  lock_slot = GetGroupReadLock(dh_file, group);
  group_locked = TRUE;

  fptr->stats.reads++;
  sysseg->global_stats.reads++;

  EndExclusive(FILE_TABLE_LOCK);

  group_bytes = (int16_t)(dh_file->group_size);

  subfile = PRIMARY_SUBFILE;
  grp = group;

  do {
    /* Read group */

    if (!dh_read_group(dh_file, subfile, grp, (char*)buff, group_bytes)) {
      goto exit_dh_exist;
    }

    /* Scan group buffer for record */

    used_bytes = buff->used_bytes;
    if ((used_bytes == 0) || (used_bytes > group_bytes)) {
      log_printf(
          "DH_EXIST: Invalid byte count (x%04X) in subfile %d, group %d\nof "
          "file %s\n",
          used_bytes, (int)subfile, grp, fptr->pathname);
      dh_err = DHE_POINTER_ERROR;
      goto exit_dh_exist;
    }

    rec_offset = offsetof(DH_BLOCK, record);
    while (rec_offset < used_bytes) {
      rec_ptr = (DH_RECORD*)(((char*)buff) + rec_offset);

      if (id_len == rec_ptr->id_len) {
        if (fptr->flags & DHF_NOCASE) {
          if (!MemCompareNoCase(id, rec_ptr->id, id_len))
            goto found;
        } else {
          if (!memcmp(id, rec_ptr->id, id_len))
            goto found;
        }
      }

      rec_offset += rec_ptr->next;
    }

    /* Move to next group buffer */

    subfile = OVERFLOW_SUBFILE;
    grp = GetFwdLink(dh_file, buff->next);
  } while (grp != 0);

  goto exit_dh_exist; /* Record not found */

  /* Record found */

found:
  dh_err = 0;

exit_dh_exist:
  if (group_locked)
    FreeGroupReadLock(lock_slot);

  return (dh_err == 0);
}

/* END-CODE */
