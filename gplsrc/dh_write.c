/* DH_WRITE.C
 * Write record.
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
 * 30 Aug 06  2.4-12 Added DHF_NO_RESIZE flag handling.
 * 17 Mar 06  2.3-8 Maintain record count in file table.
 * 17 Jun 05  2.2-1 Added dj_jnl() interface.
 * 12 May 05  2.1-15 Moved freeing of old big record space until after we give
 *                   away the group lock on the group we are updating.
 * 04 May 05  2.1-13 Initial implementation of large file support.
 * 15 Dec 04  2.1-0 Limit file size on personal licences.
 * 23 Sep 04  2.0-2 Added support for case insensitive ids.
 * 20 Sep 04  2.0-2 Use pcode loader.
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

Private int32_t dh_write_big_rec(DH_FILE* dh_file,
                                  STRING_CHUNK* rec,
                                  int32_t data_len);

/* ====================================================================== */

bool dh_write(DH_FILE* dh_file,       /* File descriptor */
              char id[],              /* Record id... */
              int16_t id_len,       /* ...and length */
              STRING_CHUNK* new_data) /* Data buffer, may be null */
{
  int32_t data_len;  /* Data length */
  int32_t base_size; /* Bytes required excluding big_rec chain */
  int16_t pad_bytes;
  int32_t grp;
  FILE_ENTRY* fptr;              /* File table entry */
  int32_t group;                /* Group number */
  int16_t group_bytes;         /* Group size in bytes */
  int16_t lock_slot = 0;       /* Lock table index */
  DH_BLOCK* buff = NULL;         /* Active buffer */
  int16_t subfile;             /* Current subfile */
  int rec_offset;                /* Offset of record in group buffer */
  DH_RECORD* rec_ptr;            /* Record pointer */
  int16_t used_bytes;          /* Number of bytes used in this group buffer */
  int16_t rec_size;            /* Size of current record */
  int32_t old_big_rec_head = 0; /* Head of old big record chain */
  int32_t new_big_rec_head;     /* Head of new big record chain */
  int32_t ogrp;
  char* data_ptr; /* Data */
  char* p;
  char* q;
  int n;
  int32_t modulus;
  int32_t load;
  DH_BLOCK* obuff = NULL;
  int16_t space;
  int16_t orec_offset;
  DH_RECORD* orec_ptr;
  int16_t orec_bytes;
  bool big_rec;         /* Is this a big record? */
  int32_t load_change; /* Change in file load for this write */
  STRING_CHUNK* str;
  bool ak;
  bool jnl;
  bool save_old_data;
  int16_t ak_mode = AK_ADD;
  STRING_CHUNK* old_data = NULL;
  STRING_CHUNK* rec; /* Data as written to file (possibly encrypted) */
  char u_id[MAX_ID_LEN];
  bool found = FALSE;

  dh_err = 0;
  process.os_error = 0;

  /* new_data is the data to be written. If we are using encryption, we may
    need access to the original plain text data and also to the encrypted
    form. At this stage, set rec as a pointer to the new data but this may
    be updated later to point to the encrypted version of this data.        */

  rec = new_data;
  if (rec != NULL)
    (rec->ref_ct)++;

  ak = ((dh_file->flags & DHF_AK) != 0); /* AK indices present? */
  jnl = FALSE;
  save_old_data = ak || jnl;

  group_bytes = (int16_t)(dh_file->group_size);

  buff = (DH_BLOCK*)k_alloc(16, group_bytes);
  if (buff == NULL) {
    dh_err = DHE_NO_MEM;
    goto exit_dh_write;
  }

  fptr = FPtr(dh_file->file_id);

  while (fptr->file_lock < 0)
    Sleep(1000); /* Clearfile in progress */

  /* We establish the record size here so that we can use it in the personal
    licence size check. If this record uses field level encryption, we will
    need to recalculate this later.                                         */

  data_len = (rec == NULL) ? 0 : rec->string_len;
  base_size = RECORD_HEADER_SIZE + id_len;
  load_change = 0;

  dh_file->flags |= FILE_UPDATED;

  /* Write big rec data
      We do this before we lock the group so that we do not end up holding
      the group lock over a potentially long series of writes.             */

  big_rec = ((base_size + data_len) >= fptr->params.big_rec_size);
  if (big_rec) {
    new_big_rec_head = dh_write_big_rec(dh_file, rec, data_len);
    if (new_big_rec_head == 0)
      goto exit_dh_write;
  }

  /* Calculate non-big-rec space required for this record */

  if (!big_rec)
    base_size += data_len;
  pad_bytes = (int16_t)((4 - (base_size & 3)) & 3);
  base_size += pad_bytes; /* Round to four byte boundary */

  /* Set load change applicable if this is a new record */

  load_change += base_size;

  /* Lock group */

  StartExclusive(FILE_TABLE_LOCK, 20);
  fptr->stats.writes++;
  sysseg->global_stats.writes++;
  group = dh_hash_group(fptr, id, id_len);
  lock_slot = GetGroupWriteLock(dh_file, group);
  fptr->upd_ct++;
  EndExclusive(FILE_TABLE_LOCK);

  /* Now search for old record of same id */

  subfile = PRIMARY_SUBFILE;
  grp = group;

  do {
    /* Read group */

    if (!dh_read_group(dh_file, subfile, grp, (char*)buff, group_bytes)) {
      goto exit_dh_write;
    }

    /* Scan group buffer for record */

    used_bytes = buff->used_bytes;
    if ((used_bytes == 0) || (used_bytes > group_bytes)) {
      log_printf(
          "DH_WRITE: Invalid byte count (x%04X) in subfile %d, group %d\nof "
          "file %s\n",
          used_bytes, (int)subfile, grp, fptr->pathname);
      dh_err = DHE_POINTER_ERROR;
      goto exit_dh_write;
    }

    rec_offset = offsetof(DH_BLOCK, record);

    while (rec_offset < used_bytes) {
      rec_ptr = (DH_RECORD*)(((char*)buff) + rec_offset);
      rec_size = rec_ptr->next;

      if (id_len == rec_ptr->id_len) {
        if (fptr->flags & DHF_NOCASE) {
          found = !MemCompareNoCase(id, rec_ptr->id, id_len);
        } else {
          found = !memcmp(id, rec_ptr->id, id_len);
        }
      }

      if (found) {
        ak_mode = AK_MOD;
        data_ptr = rec_ptr->id + id_len; /* (Meaningless for big record) */

        if (rec_ptr->flags & DH_BIG_REC) /* Old record is big */
        {
          old_big_rec_head = GetFwdLink(dh_file, rec_ptr->data.big_rec);
        } else /* Old record is not big */
        {
          if (save_old_data) /* Copy out old record data */
          {
            ts_init(&old_data, rec_ptr->data.data_len);
            ts_copy(data_ptr, rec_ptr->data.data_len);
            (void)ts_terminate();
          }
        }

        load_change -= rec_size;

        /* If the new and old records require the same space, simply
            replace the original record                                */

        if (base_size == rec_size) {
          rec_ptr->flags = 0;

          if (big_rec) /* New record is a big record (old will be too) */
          {
            rec_ptr->flags |= DH_BIG_REC;
            rec_ptr->data.big_rec = SetFwdLink(dh_file, new_big_rec_head);
          } else {
            rec_ptr->data.data_len = data_len; /* Set new data length */

            if (data_len != 0) {
              str = rec;
              do {
                memcpy(data_ptr, str->data, str->bytes);
                data_ptr += str->bytes;
                str = str->next;
              } while (str != NULL);
            }
          }

          goto record_replaced;
        }

        /* Delete old record */

        p = (char*)rec_ptr;
        q = p + rec_size;
        n = used_bytes - (rec_size + rec_offset);
        if (n > 0)
          memmove(p, q, n);

        used_bytes -= rec_size;
        buff->used_bytes = used_bytes;

        /* Clear the new slack space */

        memset(((char*)buff) + used_bytes, '\0', rec_size);

        /* Perform buffer compaction if there is a further overflow block */

        if ((ogrp = GetFwdLink(dh_file, buff->next)) != 0) {
          if ((obuff = (DH_BLOCK*)k_alloc(17, group_bytes)) != NULL) {
            do {
              if (!dh_read_group(dh_file, OVERFLOW_SUBFILE, ogrp, (char*)obuff,
                                 group_bytes)) {
                goto exit_dh_write;
              }

              /* Move as much as will fit of this block */

              space = group_bytes - buff->used_bytes;
              orec_offset = BLOCK_HEADER_SIZE;
              while (orec_offset < obuff->used_bytes) {
                orec_ptr = (DH_RECORD*)(((char*)obuff) + orec_offset);
                if ((orec_bytes = orec_ptr->next) > space)
                  break;

                /* Move this record */

                memcpy(((char*)buff) + buff->used_bytes, (char*)orec_ptr,
                       orec_bytes);
                buff->used_bytes += orec_bytes;
                space -= orec_bytes;
                orec_offset += orec_bytes;
              }

              /* Remove moved records from source buffer */

              if (orec_offset != BLOCK_HEADER_SIZE) {
                p = ((char*)obuff) + BLOCK_HEADER_SIZE;
                q = ((char*)obuff) + orec_offset;
                n = obuff->used_bytes - orec_offset;
                memmove(p, q, n);

                /* Clear newly released space */

                p += n;
                n = orec_offset - BLOCK_HEADER_SIZE;
                memset(p, '\0', n);
                obuff->used_bytes -= n;
              }

              /* If the source block is now empty, dechain it and move
                    it to the free chain.                                 */

              if (obuff->used_bytes == BLOCK_HEADER_SIZE) {
                buff->next = obuff->next;
                dh_free_overflow(dh_file, ogrp);
              } else {
                /* Write the target block and make this source block current */

                if (!dh_write_group(dh_file, subfile, grp, (char*)buff,
                                    group_bytes)) {
                  goto exit_dh_write;
                }

                memcpy((char*)buff, (char*)obuff, group_bytes);

                subfile = OVERFLOW_SUBFILE;
                grp = ogrp;
              }
            } while ((ogrp = GetFwdLink(dh_file, obuff->next)) != 0);
          }
        }

        goto add_record;
      }

      rec_offset += rec_ptr->next;
    }

    /* Move to next group buffer */

    if (buff->next == 0)
      break;

    subfile = OVERFLOW_SUBFILE;
    grp = GetFwdLink(dh_file, buff->next);
  } while (1);

  /* Add new record */

add_record:

  if (base_size > (group_bytes - buff->used_bytes)) /* Need overflow */
  {
    if ((ogrp = dh_get_overflow(dh_file, FALSE)) == 0) {
      /* Cannot allocate overflow block. At this stage we haven't done
        anything to update the main body of the group though we may have
        written a large record chain.                                    */

      if (big_rec) /* Give away big record space */
      {
        (void)dh_free_big_rec(dh_file, new_big_rec_head, NULL);
      }
      goto exit_dh_write;
    }

    buff->next = SetFwdLink(dh_file, ogrp);

    if (!dh_write_group(dh_file, subfile, grp, (char*)buff, group_bytes)) {
      goto exit_dh_write;
    }

    memset((char*)buff, '\0', group_bytes);
    buff->used_bytes = BLOCK_HEADER_SIZE;

    subfile = OVERFLOW_SUBFILE;
    grp = ogrp;
  }

  rec_ptr = (DH_RECORD*)(((char*)buff) + buff->used_bytes);
  rec_ptr->next = (int16_t)base_size;
  rec_ptr->flags = 0;
  rec_ptr->id_len = (u_char)id_len;
  memcpy(rec_ptr->id, id, id_len);
  if (big_rec) {
    rec_ptr->flags |= DH_BIG_REC;
    rec_ptr->data.big_rec = SetFwdLink(dh_file, new_big_rec_head);
  } else {
    rec_ptr->data.data_len = data_len;

    if (data_len != 0) {
      data_ptr = rec_ptr->id + id_len;
      str = rec;
      do {
        memcpy(data_ptr, str->data, str->bytes);
        data_ptr += str->bytes;
        str = str->next;
      } while (str != NULL);
    }
  }

  buff->used_bytes += (int16_t)base_size;

record_replaced:
  /* Write last affected block */

  if (!dh_write_group(dh_file, subfile, grp, (char*)buff, group_bytes)) {
    goto exit_dh_write;
  }

  FreeGroupWriteLock(lock_slot);
  lock_slot = 0;

  /* Give away any big rec space from old record */

  if (old_big_rec_head) {
    if (!dh_free_big_rec(dh_file, old_big_rec_head,
                         (ak || jnl) ? (&old_data) : NULL)) {
      goto exit_dh_write;
    }
  }

  /* Update AK index */

  if (ak) {
    InitDescr(e_stack, INTEGER); /* Mode */
    (e_stack++)->data.value = ak_mode;

    InitDescr(e_stack, ARRAY); /* AK data */
    (e_stack++)->data.ahdr_addr = dh_file->ak_data;
    (dh_file->ak_data->ref_ct)++;

    if (fptr->flags & DHF_NOCASE) {
      memucpy(u_id, id, id_len);
      k_put_string(u_id, id_len, e_stack++); /* Record id */
    } else {
      k_put_string(id, id_len, e_stack++); /* Record id */
    }

    InitDescr(e_stack, STRING); /* Old record */
    (e_stack++)->data.str.saddr = old_data;

    InitDescr(e_stack, STRING); /* New record */
    (e_stack++)->data.str.saddr = new_data;
    if (new_data != NULL)
      new_data->ref_ct++;

    ak_dh_file = dh_file;
    k_recurse(pcode_ak, 5);
    old_data = NULL; /* Will have been released on exit */
  }

exit_dh_write:
  if (old_data != NULL)
    s_free(old_data);
  k_deref_string(rec);

  dh_file->flags |= FILE_UPDATED;

  if (lock_slot != 0)
    FreeGroupWriteLock(lock_slot);

  /* Adjust load value and record count */

  if (dh_err == 0) {
    StartExclusive(FILE_TABLE_LOCK, 21);
    if ((load_change >= 0) ||
        ((int64)(-load_change) <= fptr->params.load_bytes)) {
      fptr->params.load_bytes += load_change;
    } else {
      fptr->params.load_bytes = 0;
    }
    if (id_len > fptr->params.longest_id)
      fptr->params.longest_id = id_len;

    if (!found && fptr->record_count >= 0)
      fptr->record_count += 1;

    EndExclusive(FILE_TABLE_LOCK);
  }

  if (buff != NULL)
    k_free(buff);
  if (obuff != NULL)
    k_free(obuff);

  /* Check if to do split or merge */

  if (fptr->inhibit_count == 0) /* Not held off by active select etc */
  {
    if (!(fptr->flags & DHF_NO_RESIZE)) {
      modulus = fptr->params.modulus;
      load = DHLoad(fptr->params.load_bytes, group_bytes, modulus);

      if ((load > fptr->params.split_load)         /* Load has grown */
          || (modulus < fptr->params.min_modulus)) /* From reconfig */
      {
        dh_split(dh_file);
      } else if ((load < fptr->params.merge_load) &&
                 (modulus > fptr->params.min_modulus)) {
        /* Looks like we need to merge but check won't immediately split */
        load = DHLoad(fptr->params.load_bytes, group_bytes, modulus - 1);
        if (load < fptr->params.split_load) /* Would not immediately split */
        {
          dh_merge(dh_file);
        }
      }
    }
  }

  return (dh_err == 0);
}

/* ======================================================================
   dh_write_big_rec()  -  Write big record overflow blocks                */

Private int32_t dh_write_big_rec(DH_FILE* dh_file,
                                  STRING_CHUNK* rec,
                                  int32_t data_len) {
  int32_t head;         /* Head of new big record chain */
  char* obuff = NULL;    /* Buffer */
  int16_t group_bytes; /* Group size */
  int32_t ogrp;             /* Overflow "group" of block being processed */
  int32_t next_ogrp;        /* Offset overflow "group" of next block to write */
  STRING_CHUNK* str;
  int16_t bytes;
  int16_t header_lock;
  int n;
  char* p;
  char* q;
  int16_t x;

  /* Get the group lock on the file header and hold it over the whole big
    record write.                                                        */

  header_lock = GetGroupWriteLock(dh_file, 0);

  /* Allocate overflow buffer */

  group_bytes = (int16_t)(dh_file->group_size);
  obuff = (char*)k_alloc(44, group_bytes);
  if (obuff == NULL) {
    /* Cannot allocate memory for buffer. We haven't done anything to the
      file so there is nothing to tidy up.                               */
    dh_err = DHE_NO_MEM;
    head = 0;
    goto exit_write_big_rec;
  }

  if ((head = (dh_get_overflow(dh_file, TRUE))) == 0) {
    /* Cannot allocate overflow block. We haven't done anything to the
      file so there is nothing to tidy up. dh_err will have been set by
      dh_get_overflow() or its subsidiaries.                             */
    goto exit_write_big_rec;
  }

  ogrp = head;

  memset(obuff, '\0', group_bytes);
  ((DH_BIG_BLOCK*)obuff)->used_bytes = DH_BIG_BLOCK_SIZE;
  ((DH_BIG_BLOCK*)obuff)->block_type = DHT_BIG_REC;
  ((DH_BIG_BLOCK*)obuff)->data_len = data_len;

  str = rec;
  bytes = str->bytes;
  q = str->data;

  do {
    p = ((DH_BIG_BLOCK*)obuff)->data;
    n = min(group_bytes - DH_BIG_BLOCK_SIZE, data_len);
    data_len -= n;
    while (n != 0) {
      x = min(n, bytes);
      memcpy(p, q, x);
      p += x;
      q += x;
      n -= x;
      ((DH_BIG_BLOCK*)obuff)->used_bytes += x;
      bytes -= x;
      if (bytes == 0) {
        if ((str = str->next) != NULL) {
          bytes = str->bytes;
          q = str->data;
        }
      }
    }

    if (data_len != 0) /* More */
    {
      if ((next_ogrp = dh_get_overflow(dh_file, TRUE)) == 0) {
        /* Cannot allocate overflow block. The dh_err code will have been
            set by dh_get_overflow or its subsidiaries. We need to give
            away any blocks we have already written.                        */

        for (next_ogrp = head; next_ogrp != ogrp;
             next_ogrp = GetFwdLink(dh_file, ((DH_BIG_BLOCK*)obuff)->next)) {
          if (!dh_read_group(dh_file, OVERFLOW_SUBFILE, next_ogrp, obuff,
                             group_bytes)) {
            break; /* Give up trying to be tidy if we cannot read it */
          }

          dh_free_overflow(dh_file, next_ogrp);
        }

        head = 0;
        goto exit_write_big_rec;
      }

      ((DH_BIG_BLOCK*)obuff)->next = SetFwdLink(dh_file, next_ogrp);
    }

    if (!dh_write_group(dh_file, OVERFLOW_SUBFILE, ogrp, obuff, group_bytes)) {
      /* Much as it would be nice to give away the space used by the part
          written record, things have gone seriously wrong.  The write is
          using previously allocated space so the chances are that any
          failure is going to cause the tidy up to fail too.               */

      head = 0;
      goto exit_write_big_rec;
    }

    if (data_len == 0)
      break;

    memset(obuff, '\0', group_bytes);
    ((DH_BIG_BLOCK*)obuff)->used_bytes = DH_BIG_BLOCK_SIZE;
    ((DH_BIG_BLOCK*)obuff)->block_type = DHT_BIG_REC;
    ogrp = next_ogrp;
  } while (1);

exit_write_big_rec:
  FreeGroupWriteLock(header_lock);
  if (obuff != NULL)
    k_free(obuff);

  dh_flush_header(dh_file);

  return head;
}

/* ======================================================================
   dh_free_big_rec()  -  Release space from a big record                  */

bool dh_free_big_rec(DH_FILE* dh_file,
                     int32_t head,          /* Overflow "group number" */
                     STRING_CHUNK** ak_data) /* Ptr to AK data. NULL if no AK */
{
  bool status = FALSE;
  FILE_ENTRY* fptr; /* File table entry */
  char* obuff = NULL;
  int16_t group_bytes;
  int32_t next_ogrp;
  int32_t grp;
  int16_t header_lock = 0;
  int32_t big_bytes = 0;
  int16_t n;

  fptr = FPtr(dh_file->file_id);

  group_bytes = (int16_t)(dh_file->group_size); /* For overflow sizing */
  obuff = (char*)k_alloc(45, group_bytes);
  if (obuff == NULL) {
    dh_err = DHE_NO_MEM;
    goto exit_free_big_rec;
  }

  if (ak_data != NULL)
    ts_init(ak_data, group_bytes);

  next_ogrp = head;
  do {
    grp = next_ogrp;
    if (!dh_read_group(dh_file, OVERFLOW_SUBFILE, grp, obuff, group_bytes)) {
      goto exit_free_big_rec;
    }

    if (ak_data != NULL) /* Gather data from deleted record */
    {
      if (big_bytes == 0) /* First block */
      {
        big_bytes = ((DH_BIG_BLOCK*)obuff)->data_len;
      }

      n = (int16_t)min(group_bytes - DH_BIG_BLOCK_SIZE, big_bytes);
      ts_copy(((DH_BIG_BLOCK*)obuff)->data, n);
      big_bytes -= n;
    }

    next_ogrp = GetFwdLink(dh_file, ((DH_BIG_BLOCK*)obuff)->next);

    /* Clear overflow block and write back, retaining chain */

    memset(obuff, 0, group_bytes);
    if (next_ogrp) /* Not final block */
    {
      ((DH_BIG_BLOCK*)obuff)->next = SetFwdLink(dh_file, next_ogrp);
      if (!dh_write_group(dh_file, OVERFLOW_SUBFILE, grp, obuff, group_bytes)) {
        goto exit_free_big_rec;
      }
    }
  } while (next_ogrp);

  if (ak_data != NULL)
    (void)ts_terminate();

  header_lock = GetGroupWriteLock(dh_file, 0);

  ((DH_BIG_BLOCK*)obuff)->next = SetFwdLink(dh_file, fptr->params.free_chain);
  if (!dh_write_group(dh_file, OVERFLOW_SUBFILE, grp, obuff, group_bytes)) {
    goto exit_free_big_rec;
  }

  fptr->params.free_chain = head;
  dh_file->flags |= FILE_UPDATED;
  status = TRUE;

exit_free_big_rec:
  if (header_lock)
    FreeGroupWriteLock(header_lock);
  if (obuff != NULL)
    k_free(obuff);

  return status;
}

/* END-CODE */
