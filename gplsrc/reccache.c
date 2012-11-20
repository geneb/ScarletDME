/* RECCACHE.C
 * Record cache management.
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
 * START-HISTORY:
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 06 May 05  2.1-13 Removed raw mode argument from keyin().
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
#include "config.h"


typedef struct REC_CACHE_ENTRY REC_CACHE_ENTRY;
struct REC_CACHE_ENTRY
 {
  REC_CACHE_ENTRY * next;
  REC_CACHE_ENTRY * prev;
  short int file_no;             /* File table index */
  unsigned long int upd_ct;      /* File's upd_ct value when cached */
  STRING_CHUNK * data;           /* Record data, NULL if null string */
  short int id_len;              /* Length of record id */
  char id[MAX_ID_LEN];           /* Id, not null terminated */
 };

Private REC_CACHE_ENTRY * rec_cache_head = NULL;
Private REC_CACHE_ENTRY * rec_cache_tail = NULL;
Private short int rec_cache_size = 0;    /* Current size */

/* ======================================================================
   init_record_cache()  -  Initialise record cache                        */

void init_record_cache()
{
 REC_CACHE_ENTRY * p;
 STRING_CHUNK * str;

 /* Expand cache */

 while(rec_cache_size < pcfg.reccache)
  {
   p = (REC_CACHE_ENTRY *)k_alloc(71, sizeof(REC_CACHE_ENTRY));
   p->next = NULL;
   p->prev = rec_cache_tail;
   p->file_no = -1;
   p->id_len = 0;
   p->data = NULL;
   if (rec_cache_head == NULL) rec_cache_head = p;
   else rec_cache_tail->next = p;
   rec_cache_tail = p;
   rec_cache_size++;
  }

 /* Contract cache */

 while(rec_cache_size > pcfg.reccache)
  {
   p = rec_cache_tail;
   rec_cache_tail = p->prev;
   if (rec_cache_tail != NULL) rec_cache_tail->next = NULL;

   str = p->data;
   if ((str != NULL) && (--(str->ref_ct) == 0)) s_free(str);

   k_free(p);

   if (--rec_cache_size == 0) rec_cache_head = NULL;
  }
}

/* ======================================================================
   cache_record()  -  Add record to cache                                 */

void cache_record(
  short int fno,    /* File table index */
  short int id_len,
  char * id,
  STRING_CHUNK * data)
{
 REC_CACHE_ENTRY * p;
 STRING_CHUNK * str;

 if (rec_cache_size)
  {
   /* Release the oldest entry.  It doesn't matter if there is another
      version of this record in the cache as we could never find it.   */

   p = rec_cache_tail;
   str = p->data;
   if ((str != NULL) && (--(str->ref_ct) == 0)) s_free(str);

   if (p != rec_cache_head)   /* Not already at head */
    {
     /* Dechain */

     rec_cache_tail = p->prev;
     rec_cache_tail->next = NULL;

     /* Rechain at head */

     rec_cache_head->prev = p;
     p->next = rec_cache_head;
     p->prev = NULL;
     rec_cache_head = p;
   }

   /* Enter details of new record */

   p->file_no = fno;
   p->upd_ct = FPtr(fno)->upd_ct;
   p->data = data;
   if (data != NULL) data->ref_ct++;
   p->id_len = id_len;
   if (id_len) memcpy(p->id, id, id_len);
  }
}

/* ======================================================================
   scan_record_cache()  -  Search for a record in the cache
   Returns pointer to string via data argument, incrementing ref count    */

bool scan_record_cache(
   short int fno,
   short int id_len,
   char * id,
   STRING_CHUNK ** data)
{
 REC_CACHE_ENTRY * p;
 FILE_ENTRY * fptr;
 unsigned long int upd_ct;

 fptr = FPtr(fno);
 upd_ct = fptr->upd_ct;
 for(p = rec_cache_head; p != NULL; p = p->next)
  {
   if ((p->upd_ct == upd_ct)
      && (p->file_no == fno)
      && (p->id_len == id_len)
      && !(memcmp(p->id, id, id_len)))
    {
     if ((*data = p->data) != NULL) p->data->ref_ct++;

     if (p != rec_cache_head)
      {
       /* Move to head of cache */

       p->prev->next = p->next;

       if (p == rec_cache_tail) rec_cache_tail = p->prev;
       else p->next->prev = p->prev;

       rec_cache_head->prev = p;
       p->next = rec_cache_head;
       p->prev = NULL;
       rec_cache_head = p;
      }

     return TRUE;
    }
  }

 return FALSE;
}


/* END-CODE */
