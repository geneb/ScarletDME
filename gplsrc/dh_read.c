/* DH_READ.C
 * Read Record
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
 * 14 Apr 06  2.4-1 Replaced use of the ts_xxx routines with an internal string
 *                  assembly subroutine. Previously, calls to dh_read() while
 *                  the ts_xxx functions were in use elsewhere caused problems.
 *                  Although this may only affect diagnostic code, one example
 *                  was that the "Press return to continue" prompt uses dh_read
 *                  to fetch the message text.
 * 19 Dec 05  2.3-3 Return actual id to allow caller to determine real id in a
 *                  file with case insensitive ids.
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
#include "config.h"

Private void copy(char* data,
                  int16_t bytes,
                  STRING_CHUNK** head,
                  STRING_CHUNK** tail);

/* ====================================================================== */

STRING_CHUNK* dh_read(
    DH_FILE* dh_file, /* File descriptor */
    char id[],        /* Record id */
    int16_t id_len, /* Record id length */
    char* actual_id)  /* Returned. Casing may differ from id. May be NULL */
{
  int32_t group;
  int16_t group_bytes;
  int16_t lock_slot = 0;
  DH_BLOCK* buff;
  DH_RECORD* rec_ptr;
  STRING_CHUNK* head = NULL;
  int16_t fno;
  FILE_ENTRY* fptr;
  int16_t subfile;
  int16_t rec_offset;
  int16_t used_bytes;
  int32_t grp;

  dh_err = DHE_RECORD_NOT_FOUND;
  process.os_error = 0;

  buff = (DH_BLOCK*)(&dh_buffer);

  fno = dh_file->file_id;
  fptr = FPtr(fno);
  while (fptr->file_lock < 0)
    Sleep(1000); /* Clearfile in progress */

  /* Lock group */

  StartExclusive(FILE_TABLE_LOCK, 11);
  group = dh_hash_group(fptr, id, id_len);
  lock_slot = GetGroupReadLock(dh_file, group);
  fptr->stats.reads++;
  sysseg->global_stats.reads++;
  EndExclusive(FILE_TABLE_LOCK);

  if ((pcfg.reccache != 0) && scan_record_cache(fno, id_len, id, &head)) {
    FreeGroupReadLock(lock_slot);
    dh_err = 0;
    return head;
  }

  group_bytes = (int16_t)(dh_file->group_size);

  subfile = PRIMARY_SUBFILE;
  grp = group;

  do {
    /* Read group */

    if (!dh_read_group(dh_file, subfile, grp, (char*)buff, group_bytes)) {
      goto exit_dh_read;
    }

    /* Scan group buffer for record */

    used_bytes = buff->used_bytes;
    if ((used_bytes == 0) || (used_bytes > group_bytes)) {
      log_printf(
          "DH_READ: Invalid byte count (x%04X) in subfile %d, group %d\nof "
          "file %s\n",
          used_bytes, (int)subfile, grp, fptr->pathname);
      dh_err = DHE_POINTER_ERROR;
      goto exit_dh_read;
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

  goto exit_dh_read; /* Record not found */

  /* Record found */

found:
  if (actual_id != NULL)
    memcpy(actual_id, rec_ptr->id, id_len);
  dh_err = 0;

  head = dh_read_record(dh_file, rec_ptr);

exit_dh_read:
  if (lock_slot != 0)
    FreeGroupReadLock(lock_slot);

  if (dh_err) {
    if (head != NULL) {
      s_free(head);
      head = NULL;
    }
  } else {
    if (pcfg.reccache != 0)
      cache_record(fno, id_len, id, head);
  }

  return head;
}

/* ====================================================================== */

STRING_CHUNK* dh_read_record(DH_FILE* dh_file, DH_RECORD* rec_ptr) {
  STRING_CHUNK* str = NULL;
  STRING_CHUNK* tail = NULL;
  int16_t group_bytes;
  int32_t data_len;
  int16_t n;
  char* buff = NULL;
  int32_t grp;

  if (rec_ptr->flags & DH_BIG_REC) /* Found a large record */
  {
    group_bytes = (int16_t)(dh_file->group_size);
    buff = (char*)k_alloc(60, group_bytes);

    grp = GetFwdLink(dh_file, rec_ptr->data.big_rec);
    while (grp != 0) {
      if (!dh_read_group(dh_file, OVERFLOW_SUBFILE, grp, buff, group_bytes)) {
        goto exit_dh_read_record;
      }

      if (str == NULL) /* First block */
      {
        data_len = ((DH_BIG_BLOCK*)buff)->data_len;
      }

      n = (int16_t)min(group_bytes - DH_BIG_BLOCK_SIZE, data_len);
      data_len -= n;

      copy(((DH_BIG_BLOCK*)buff)->data, n, &str, &tail);

      grp = GetFwdLink(dh_file, ((DH_BIG_BLOCK*)buff)->next);
    }
  } else /* Not a large record */
  {
    data_len = rec_ptr->data.data_len;
    if (data_len != 0) {
      copy(rec_ptr->id + rec_ptr->id_len, (int16_t)data_len, &str, &tail);
    }
  }

exit_dh_read_record:
  if (buff != NULL)
    k_free(buff);

  return str;
}

/* ====================================================================== */

Private void copy(char* data,
                  int16_t bytes,
                  STRING_CHUNK** head,
                  STRING_CHUNK** tail) {
  STRING_CHUNK* str;
  int16_t actual;

  str = s_alloc(bytes, &actual);
  if (*head == NULL) /* First chunk */
  {
    *head = str;
    *tail = str;
    str->ref_ct = 1;
    str->string_len = 0;
  } else {
    (*tail)->next = str;
    *tail = str;
  }

  memcpy(str->data, data, bytes);
  str->bytes = bytes;
  (*head)->string_len += bytes;
}

/* END-CODE */
