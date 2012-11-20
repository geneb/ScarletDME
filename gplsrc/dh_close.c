/* DH_CLOSE.C
 * Close file
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
 * 24 Jul 06  2.4-10 0505 Closing message file after failed login tried to
 *                   access user file map table.
 * 27 Jun 06  2.4-5 Maintain user/file map table.
 * 10 Oct 05  2.2-14 Implemented akpath element of DH_FILE.
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
#include "header.h"

/* ====================================================================== */

bool dh_close(DH_FILE * dh_file)
{
 DH_FILE * p;
 DH_FILE * prev;
 FILE_ENTRY * fptr;

 dh_err = 0;
 process.os_error = 0;


 (void)dh_flush_header(dh_file);

 if (--(dh_file->open_count) == 0)
  {
   dh_end_select_file(dh_file);   /* Clear down partially completed selects */

   StartExclusive(FILE_TABLE_LOCK, 42);
   fptr = FPtr(dh_file->file_id);
   (fptr->ref_ct)--;
   if (my_uptr != NULL) (*UFMPtr(my_uptr, dh_file->file_id))--;  /* 0505 */
   EndExclusive(FILE_TABLE_LOCK);

   /* Remove from DH_FILE chain */

   prev = NULL;
   for (p = dh_file_head; p != NULL; p = p->next_file)
    {
     if (p == dh_file)
      {
       if (prev == NULL) /* Removing at head */
        {
         dh_file_head = p->next_file;
        }
       else
        {
         prev->next_file = p->next_file;
        }
       break;
      }
     prev = p;
    }

   deallocate_dh_file(dh_file);
  }

 return (dh_err == 0);
}

/* ======================================================================
   deallocate_dh_file()  -  Release DH_FILE and sub-structures            */

void deallocate_dh_file(DH_FILE* dh_file)
{
 short int i;

 /* Close subfiles */

 for (i = 0; i < dh_file->no_of_subfiles; i++)
  {
   if (ValidFileHandle(dh_file->sf[i].fu)) dh_close_subfile(dh_file, i);
  }

 /* Remove AK data array */

 if (dh_file->ak_data != NULL) free_array(dh_file->ak_data);

 /* Release trigger function name */

 if (dh_file->trigger_name != NULL) k_free(dh_file->trigger_name);

 /* Decrement reference count on trigger function */

 if (dh_file->trigger != NULL)
  {
   --(((OBJECT_HEADER *)(dh_file->trigger))->ext_hdr.prog.refs);
   dh_file->trigger = NULL;
  }

 /* Release akpath */

 if (dh_file->akpath != NULL) k_free(dh_file->akpath);


 /* Release memory */

 k_free(dh_file);
}

/* END-CODE */
