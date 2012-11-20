/* DH_DEL.C
 * Delete record.
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
 * 01 Jul 07  2.5-7 Extensive changes for PDA merge.
 * 30 Aug 06  2.4-12 Added DHF_NO_RESIZE flag handling.
 * 17 Mar 06  2.3-8 Maintain record count in file table.
 * 17 Jun 05  2.2-1 Added journalling interface.
 * 12 May 05  2.1-15 Moved freeing of big record space until after group lock is
 *                   released.
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

/* SUBROUTINE AK(MODE, AK.DATA, ID, OLD.REC, NEW.REC) */
extern u_char ak_code [];

/* ====================================================================== */

bool dh_delete(
   DH_FILE * dh_file,      /* File descriptor */
   char id[],              /* Record id... */
   short int id_len)       /* ...and length */
{
 long int grp;
 FILE_ENTRY * fptr;        /* File table entry */
 long int group;           /* Group number */
 short int group_bytes;    /* Group size in bytes */
 short int lock_slot = 0;  /* Lock table index */
 DH_BLOCK * buff;          /* Active buffer */
 short int subfile;        /* Current subfile */
 int rec_offset;           /* Offset of record in group buffer */
 DH_RECORD * rec_ptr;      /* Record pointer */
 short int used_bytes;     /* Number of bytes used in this group buffer */
 short int rec_size;       /* Size of current record */
 long int big_rec_head = 0; /* Head of big record chain */
 long int ogrp;
 char * p;
 char * q;
 int n;
 long int modulus;
 long int load;
 DH_BLOCK * obuff = NULL;
 short int space;
 short int orec_offset;
 DH_RECORD * orec_ptr;
 short int orec_bytes;
 long int load_change = 0;
 bool ak;
 bool jnl;
 STRING_CHUNK * old_data = NULL;
 bool found = FALSE;
 long int prev_grp = 0;
 short int prev_subfile;
 long int fwd;
 char u_id[MAX_ID_LEN];

 dh_err = 0;
 process.os_error = 0;

 ak = ((dh_file->flags & DHF_AK) != 0);   /* AK indices present? */
 jnl = FALSE;

 group_bytes = (short int)(dh_file->group_size);

 buff = (DH_BLOCK *)k_alloc(5,group_bytes);
 if (buff == NULL)
  {
   dh_err = DHE_NO_MEM;
   goto exit_dh_delete;
  }

 fptr = FPtr(dh_file->file_id);
 while(fptr->file_lock < 0) Sleep(1000); /* Clearfile in progress */

 dh_file->flags |= FILE_UPDATED;

 /* Lock group */

 StartExclusive(FILE_TABLE_LOCK, 4);
 fptr->stats.deletes++;
 sysseg->global_stats.deletes++;
 group = dh_hash_group(fptr, id, id_len);
 lock_slot = GetGroupWriteLock(dh_file, group);
 fptr->upd_ct++;
 EndExclusive(FILE_TABLE_LOCK);

 subfile = PRIMARY_SUBFILE;
 grp = group;

 do {
     /* Read group */

     if (!dh_read_group(dh_file, subfile, grp, (char *)buff, group_bytes))
      {
       goto exit_dh_delete;
      }

     /* Scan group buffer for record */

     used_bytes = buff->used_bytes;
     rec_offset = offsetof(DH_BLOCK, record);

     while(rec_offset < used_bytes)
      {
       rec_ptr = (DH_RECORD *)(((char *)buff) + rec_offset);
       rec_size = rec_ptr->next;

       if (id_len == rec_ptr->id_len)
        {
         if (fptr->flags & DHF_NOCASE)
          {
           found = !MemCompareNoCase(id, rec_ptr->id, id_len);
          }
         else
          {
           found = !memcmp(id, rec_ptr->id, id_len);
          }
        }

       if (found)
        {
         if (rec_ptr->flags & DH_BIG_REC)
          {
           big_rec_head = GetFwdLink(dh_file, rec_ptr->data.big_rec);
          }
         else
          {
           if (ak || jnl)    /* Copy out old record data */
            {
             ts_init(&old_data, rec_ptr->data.data_len);
             ts_copy(rec_ptr->id + id_len, rec_ptr->data.data_len);
             (void)ts_terminate();
            }
          }

         load_change = -rec_size;

         /* Delete record */

         p = (char *)rec_ptr;
         q = p + rec_size;
         n = used_bytes - (rec_size + rec_offset);
         if (n > 0) memmove(p, q, n);

         used_bytes -= rec_size;
         buff->used_bytes = used_bytes;

         /* If this block is now empty and it was an overflow block,
            dechain it from the group.                               */

         if ((used_bytes == BLOCK_HEADER_SIZE) && (prev_grp != 0))
          {
           fwd = buff->next;   /* Hold on to forward pointer */
           dh_free_overflow(dh_file, grp);  /* Free overflow block */

           /* Re-read previous block */
           subfile = prev_subfile;
           grp = prev_grp;
           if (!dh_read_group(dh_file, subfile, grp, (char *)buff, group_bytes))
            {
             goto exit_dh_delete;
            }

           buff->next = fwd;   /* Chain to next (if any) overflow block */
          }
         else           /* Not freeing an empty overflow block */
          {
           /* Clear the new slack space */

           memset(((char *)buff) + used_bytes, '\0', rec_size);

           /* Perform buffer compaction if there is a further overflow block */

           if ((ogrp = GetFwdLink(dh_file, buff->next)) != 0)
            {
             if ((obuff = (DH_BLOCK *)k_alloc(11,group_bytes)) != NULL)
              {
               do {
                   if (!dh_read_group(dh_file, OVERFLOW_SUBFILE, ogrp,
                                      (char *)obuff, group_bytes))
                    {
                     goto exit_dh_delete;
                    }

                   /* Move as much as will fit of this block */

                   space = group_bytes - buff->used_bytes;
                   orec_offset = BLOCK_HEADER_SIZE;
                   while(orec_offset < obuff->used_bytes)
                    {
                     orec_ptr = (DH_RECORD *)(((char *)obuff) + orec_offset);
                     if (orec_ptr->next > space) break;
  
                     /* Move this record */

                     orec_bytes = orec_ptr->next;
                     memcpy(((char *)buff) + buff->used_bytes,
                            (char *)orec_ptr, orec_bytes);
                     buff->used_bytes += orec_bytes;
                     space -= orec_bytes;
                     orec_offset += orec_bytes;
                    }

                   /* Remove moved records from source buffer */

                   if (orec_offset != BLOCK_HEADER_SIZE)
                    {
                     p = ((char *)obuff) + BLOCK_HEADER_SIZE;
                     q = ((char *)obuff) + orec_offset;
                     n = obuff->used_bytes - orec_offset;
                     memmove(p, q, n);
                     p += n;

                     n = orec_offset - BLOCK_HEADER_SIZE;
                     memset(p, '\0', n);
                     obuff->used_bytes -= n;
                    }

                   /* If the source block is now empty, dechain it and move
                      it to the free chain.                                 */

                   if (obuff->used_bytes == BLOCK_HEADER_SIZE)
                    {
                     buff->next = obuff->next;
                     dh_free_overflow(dh_file, ogrp);
                    }
                   else
                    {
                     /* Write the target block and make this source block current */

                     if (!dh_write_group(dh_file, subfile, grp, (char *)buff,
                                         group_bytes))
                      {
                       goto exit_dh_delete;
                      }

                     memcpy((char *)buff, (char *)obuff, group_bytes);

                     subfile = OVERFLOW_SUBFILE;
                     grp = ogrp;
                    }
                  } while((ogrp = GetFwdLink(dh_file, obuff->next)) != 0);
              }
            }
          }

         /* Write last affected block */

         if (!dh_write_group(dh_file, subfile, grp, (char *)buff, group_bytes))
          {
           goto exit_dh_delete;
          }

         goto exit_dh_delete_ok;
        }

       rec_offset += rec_ptr->next;
      }

     /* Move to next group buffer */

     prev_subfile = subfile;
     prev_grp = grp;
     subfile = OVERFLOW_SUBFILE;
    } while((grp = GetFwdLink(dh_file, buff->next)) != 0);

exit_dh_delete_ok:

 FreeGroupWriteLock(lock_slot);
 lock_slot = 0;

 /* Give away any big rec space */

 if (big_rec_head)
  {
   if (!dh_free_big_rec(dh_file, big_rec_head, (ak || jnl)?(&old_data):NULL))
    {
     goto exit_dh_delete;
    }
  }


 /* Update AK index */

 if (ak && found)
  {
   InitDescr(e_stack, INTEGER);                   /* Mode */
   (e_stack++)->data.value = AK_DEL;

   InitDescr(e_stack, ARRAY);                     /* AK data */
   (e_stack++)->data.ahdr_addr = dh_file->ak_data;
   (dh_file->ak_data->ref_ct)++;

   if (fptr->flags & DHF_NOCASE)
    {
     memucpy(u_id, id, id_len);
     k_put_string(u_id, id_len, e_stack++);       /* Record id */
    }
   else
    {
     k_put_string(id, id_len, e_stack++);         /* Record id */
    }

   InitDescr(e_stack, STRING);                    /* Old record */
   (e_stack++)->data.str.saddr = old_data;

   InitDescr(e_stack, STRING);                    /* New record */
   (e_stack++)->data.str.saddr = NULL;

   ak_dh_file = dh_file;
   k_recurse(pcode_ak, 5);
   old_data = NULL;           /* Will have been released on exit */
  }

exit_dh_delete:
 dh_file->flags |= FILE_UPDATED;

 if (lock_slot != 0) FreeGroupWriteLock(lock_slot);

 StartExclusive(FILE_TABLE_LOCK, 5);

 if (load_change != 0)         /* Adjust load value */
  {
   if (fptr->params.load_bytes >= (int64)rec_size) fptr->params.load_bytes -= rec_size;
   else fptr->params.load_bytes = 0;
  }

 if (found)
  {
   if (fptr->record_count >= 0)
    {
     fptr->record_count -= 1;
     if (fptr->record_count < 0) fptr->record_count = 0; /* Survive crashes */
    }
  }

 EndExclusive(FILE_TABLE_LOCK);

 if (buff != NULL) k_free(buff);
 if (obuff != NULL) k_free(obuff);

 /* Check if to do split or merge */

 if (fptr->inhibit_count == 0)   /* Not held off by active select etc */
  {
   if (!(fptr->flags & DHF_NO_RESIZE))
    {
     modulus = fptr->params.modulus;
     load = DHLoad(fptr->params.load_bytes, group_bytes, modulus);

     if ((load > fptr->params.split_load)        /* Load has grown */
       || (modulus < fptr->params.min_modulus))  /* From reconfig */
      {
       dh_split(dh_file);
      }
     else if ((load < fptr->params.merge_load) 
             && (modulus > fptr->params.min_modulus))
      {
       /* Looks like we need to split but check won't immediately merge */
       load = DHLoad(fptr->params.load_bytes, group_bytes, modulus - 1);
       if (load < fptr->params.split_load)  /* Would not immediately split */
        {
         dh_merge(dh_file);
        }
      }
    }
  }

 return (dh_err == 0);
}

/* END-CODE */
