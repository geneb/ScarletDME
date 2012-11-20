/* DH_SELCT.C
 * Handle select list functions
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
 * 02 Mar 07  2.5-1 Added select_ftype handling.
 * 20 Mar 06  2.3-8 Correct file header record count after a select during which
 *                  no updates were applied.
 * 19 Sep 05  2.2-11 Use dh_buffer to reduce stack space.
 * 29 Jun 05  2.2-2 0370 ts_init() call got lost from dh_complete_select().
 * 08 Jun 05  2.2-1 0366 Stack the target string in dh_complete_select() before
 *                  handling external events.
 * 04 May 05  2.1-13 Initial implementation of large file support.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * dh_select()         Set up select list operation
 * dh_readkey()        Get next key
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include "dh_int.h"
#include "syscom.h"

Private int64 rec_ct[HIGH_SELECT+1];
Private int64 load_bytes[HIGH_SELECT+1];
Private unsigned long int upd_ct[HIGH_SELECT+1];

/* ======================================================================
   Start select on given file                                             */

bool dh_select(
   DH_FILE * dh_file,         /* File descriptor */
   short int list_no)
{
 FILE_ENTRY * fptr;

 dh_err = 0;
 process.os_error = 0;

 if (select_file[list_no] != NULL)  /* Clear down partial select */
  {
   fptr = FPtr(((DH_FILE *)select_file[list_no])->file_id);
   while(fptr->file_lock < 0) Sleep(1000);  /* Clear file in progress */
   (fptr->inhibit_count)--;
  }

 select_ftype[list_no] = SEL_DH;
 select_file[list_no] = dh_file;

 fptr = FPtr(dh_file->file_id);

 StartExclusive(FILE_TABLE_LOCK, 12);

 fptr->inhibit_count++;
 fptr->stats.selects++;
 sysseg->global_stats.selects++;

 /* We can use the select as a way to correct any error in the file header
    record count and load bytes so long as the file is not updated during
    the select. Save the current update count and initialise the record
    counter and load bytes for this list to zero. When the select completes,
    we can check if there have been any updates and, if not, we can save
    the new values in the file header.                                       */

 upd_ct[list_no] = fptr->upd_ct;
 rec_ct[list_no] = 0;
 load_bytes[list_no] = 0;

 EndExclusive(FILE_TABLE_LOCK);

 select_group[list_no] = 1;
 return dh_select_group(dh_file, list_no);
}

/* ======================================================================
   Complete any partial DH select                                         */

void dh_complete_select(short int list_no)
{
 DH_FILE * dh_file;
 long int group;
 short int group_bytes;
 short int lock_slot;
 DH_BLOCK * buff;
 DH_RECORD * rec_ptr;
 long int grp;
 FILE_ENTRY * fptr;
 short int subfile;
 short int rec_offset;
 short int used_bytes;
 long int record_count;
 STRING_CHUNK * head = NULL;
 DESCRIPTOR * count_descr;
 DESCRIPTOR * list_descr;
 STRING_CHUNK * str;
 bool do_break;


 if (select_group[list_no] != 0)
  {
   /* Find SELECT.COUNT(LIST.NO) in SYSCOM */

   count_descr = SelectCount(list_no);

   if (count_descr->type != INTEGER)  /* Set as inactive list */
    {
     Release(count_descr);
     InitDescr(count_descr, INTEGER);
     count_descr->data.value = -1;
    }

   record_count = count_descr->data.value;
   if (record_count < 0) record_count = 0;

   /* Find SELECT.LIST(LIST.NO) in SYSCOM */

   list_descr = SelectList(list_no);

   if (list_descr->type != STRING)   /* Set as null string */     
    {
     Release(list_descr);
     InitDescr(list_descr, STRING);
     list_descr->data.str.saddr = NULL;
    }

   buff = (DH_BLOCK *)(&dh_buffer);

   dh_file = (DH_FILE *)select_file[list_no];
   fptr = FPtr(dh_file->file_id);

   while((group = (select_group[list_no])++) <= fptr->params.modulus)
    {
     /* Check for events that must be processed in this loop */

     if (k_exit_cause == K_QUIT)
      {
       ts_stack();             /* 0366 */
       do_break = !tio_handle_break();
       ts_unstack();
       if (do_break) break;
      }

     if (my_uptr->events)
      {
       ts_stack();             /* 0366 */
       process_events();
       ts_unstack();
      }

     if (k_exit_cause == K_TERMINATE) break;

     /* Lock group */

     StartExclusive(FILE_TABLE_LOCK, 13);
     lock_slot = GetGroupReadLock(dh_file, group);
     EndExclusive(FILE_TABLE_LOCK);

     group_bytes = (short int)(dh_file->group_size);

     subfile = PRIMARY_SUBFILE;
     grp = group;

     do {
         /* Read group */

         if (!dh_read_group(dh_file, subfile, grp, (char *)buff, group_bytes))
          {
           FreeGroupReadLock(lock_slot);
           goto exit_dh_complete_select;
          }

         /* Scan group buffer for records */

         used_bytes = buff->used_bytes;
         rec_offset = offsetof(DH_BLOCK, record);
         while(rec_offset < used_bytes)
          {
           rec_ptr = (DH_RECORD *)(((char *)buff) + rec_offset);

           /* Add this record to the list */

           if (head == NULL) ts_init(&head, 256);  /* 0370 */

           if (record_count != 0) ts_copy_byte(FIELD_MARK);

           ts_copy(rec_ptr->id, rec_ptr->id_len);

           /* Maintain the record counts. These two variables are not
              necessarily equal. Record_count is a count of items in the
              select list. Rec_ct is a count of records in the file. It
              is possible that the calling program statrted a select,
              did a few READNEXT operations (which call dh-select_group),
              and then decided to do a READLIST for the rest. Many of the
              standard QM command do something of this sort when they
              query the user about use of an active list.                  */

           record_count++;
           rec_ct[list_no]++;
           load_bytes[list_no] += rec_ptr->next;
           rec_offset += rec_ptr->next;
          }

         /* Move to next group buffer */

         subfile = OVERFLOW_SUBFILE;
         grp = GetFwdLink(dh_file, buff->next);
        } while(grp != 0);

     FreeGroupReadLock(lock_slot);
    }

   /* Copy the record list (if any) to the select list */

   if (head != NULL)
    {
     ts_terminate();

     /* Append new items to list */

     if (list_descr->data.str.saddr == NULL)
      {
       list_descr->data.str.saddr = head;
      }
     else
      {
       for (str = list_descr->data.str.saddr; str->next != NULL;
            str = str->next)
        {
        }
       str->next = head;
       list_descr->data.str.saddr->string_len += head->string_len;
      }

     count_descr->data.value = record_count;

     /* We can use the accumulated record count to update the file header */

     if ((upd_ct[list_no] == fptr->upd_ct)   /* File has not changed... */
         && !(dh_file->flags & DHF_RDONLY))  /* ...and is not read-only */
      {
       dh_file->flags |= FILE_UPDATED;
       fptr->record_count = rec_ct[list_no];
       fptr->params.load_bytes = load_bytes[list_no];
      }
    }

exit_dh_complete_select:

   /* Terminate the select */

   StartExclusive(FILE_TABLE_LOCK, 14);
   (fptr->inhibit_count)--;
   EndExclusive(FILE_TABLE_LOCK);

   select_ftype[list_no] = SEL_NONE;
   select_file[list_no] = NULL;
   select_group[list_no] = 0;
  }
}

/* ======================================================================
   Terminate a DH select                                                  */

void dh_end_select(short int list_no)
{
 FILE_ENTRY * fptr;

 if (select_group[list_no] != 0)
  {
   fptr = FPtr(((DH_FILE *)select_file[list_no])->file_id);
   StartExclusive(FILE_TABLE_LOCK, 15);
   (fptr->inhibit_count)--;
   EndExclusive(FILE_TABLE_LOCK);

   select_ftype[list_no] = SEL_NONE;
   select_group[list_no] = 0;
   select_file[list_no] = NULL;
  }
}

/* ======================================================================
   Terminate a DH select for a given file                                 */

void dh_end_select_file(DH_FILE * dh_file)
{
 FILE_ENTRY * fptr;
 short int i;

 fptr = FPtr(dh_file->file_id);

 if (fptr->inhibit_count > 0)  /* There's a select (etc) somewhere - Is it us? */
  {
   for (i = 0; i <= HIGH_SELECT; i++)
    {
     if ((select_ftype[i] == SEL_DH) && (select_file[i] == dh_file))
      {
       StartExclusive(FILE_TABLE_LOCK, 16);
       (fptr->inhibit_count)--;
       EndExclusive(FILE_TABLE_LOCK);

       select_ftype[i] = SEL_NONE;
       select_file[i] = NULL;
       select_group[i] = 0;
      }
    }
  }
}

/* ====================================================================== */

bool dh_select_group(
   DH_FILE * dh_file, /* File descriptor */
   short int list_no)
{
 bool status = FALSE;
 long int group;
 short int group_bytes;
 short int lock_slot;
 DH_BLOCK * buff;
 DH_RECORD * rec_ptr;
 long int grp;
 FILE_ENTRY * fptr;
 short int subfile;
 short int rec_offset;
 short int used_bytes;
 short int record_count = 0;
 STRING_CHUNK * head = NULL;
 DESCRIPTOR * descr;

 buff = (DH_BLOCK *)(&dh_buffer);

 fptr = FPtr(dh_file->file_id);

 while((record_count == 0)
       && ((group = (select_group[list_no])++) <= fptr->params.modulus))
  {
   /* Lock group */

   StartExclusive(FILE_TABLE_LOCK, 17);
   lock_slot = GetGroupReadLock(dh_file, group);
   EndExclusive(FILE_TABLE_LOCK);

   group_bytes = (short int)(dh_file->group_size);

   subfile = PRIMARY_SUBFILE;
   grp = group;

   do {
       /* Read group */

       if (!dh_read_group(dh_file, subfile, grp, (char *)buff, group_bytes))
        {
         FreeGroupReadLock(lock_slot);
         goto exit_dh_select_group;
        }

       /* Scan group buffer for records */

       used_bytes = buff->used_bytes;
       rec_offset = offsetof(DH_BLOCK, record);
       while(rec_offset < used_bytes)
        {
         rec_ptr = (DH_RECORD *)(((char *)buff) + rec_offset);

         /* Add this record to the list */

         if (head == NULL) ts_init(&head, 256);
         else ts_copy_byte(FIELD_MARK);

         ts_copy(rec_ptr->id, rec_ptr->id_len);
         record_count++;
         rec_ct[list_no]++;

         rec_offset += rec_ptr->next;
         load_bytes[list_no] += rec_ptr->next;
        }

       /* Move to next group buffer */

       subfile = OVERFLOW_SUBFILE;
       grp = GetFwdLink(dh_file, buff->next);
      } while(grp != 0);

   FreeGroupReadLock(lock_slot);
  }

 /* Copy the record list (if any) to the select list */

 if (head != NULL)  /* Found at least one record */
  {
   ts_terminate();

   /* Find SELECT.COUNT(LIST.NO) in SYSCOM */

   descr = SelectCount(list_no);
   Release(descr);
   InitDescr(descr, INTEGER);
   descr->data.value = record_count;

   /* Find SELECT.LIST(LIST.NO) in SYSCOM */

   descr = SelectList(list_no);
   Release(descr);
   InitDescr(descr, STRING);
   descr->data.str.saddr = head;

   status = TRUE;
  }


/* Terminate the select if we have now processed the last group */

 if (select_group[list_no] > fptr->params.modulus)
  {
   StartExclusive(FILE_TABLE_LOCK, 67);

   if ((upd_ct[list_no] == fptr->upd_ct)   /* File has not changed... */
       && !(dh_file->flags & DHF_RDONLY))  /* ...and is not read-only */
    {
     dh_file->flags |= FILE_UPDATED;
     fptr->record_count = rec_ct[list_no];
     fptr->params.load_bytes = load_bytes[list_no];
    }

   (fptr->inhibit_count)--;
   EndExclusive(FILE_TABLE_LOCK);

   select_ftype[list_no] = SEL_NONE;
   select_group[list_no] = 0;
   select_file[list_no] = NULL;
  }

exit_dh_select_group:
 return status;
}

/* END-CODE */
