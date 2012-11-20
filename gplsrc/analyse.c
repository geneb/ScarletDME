/* ANALYSE.C
 * Analyze DH and directory files
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
 * 30 Aug 06  2.4-12 Added non_numeric_ids counter.
 * 23 Feb 06  2.3-7 dir_filesize() now returns int64.
 * 19 Sep 05  2.2-11 Use dh_buffer to reduce stack space.
 * 11 May 05  2.1-14 Changes for large file support.
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

Private bool dh_analyse(DH_FILE * dh_file, char * result);
Private bool dir_analyse(FILE_VAR * fvar, char * result);

/* ====================================================================== */

void op_analyse()
{
 DESCRIPTOR * descr;
 FILE_VAR * fvar;
 char result[256];

 descr = e_stack - 1;
 k_get_file(descr);
 fvar = descr->data.fvar;
 k_dismiss();

 switch(fvar->type)
  {
   case DYNAMIC_FILE:
      if (!dh_analyse(fvar->access.dh.dh_file, result)) result[0] = '\0';
      break;

   case DIRECTORY_FILE:
      if (!dir_analyse(fvar, result)) result[0] = '\0';
      break;

   default:
      k_error("Illegal file type for ANALYSE");
  }

 k_put_c_string(result, e_stack++);
}

/* ====================================================================== */

Private bool dh_analyse(
   DH_FILE * dh_file,    /* File descriptor */
   char * result)        /* Result string */
{
 bool status = FALSE;
 long int group;
 short int group_bytes;
 DH_BLOCK * buff;
 DH_RECORD * rec_ptr;
 DH_BIG_BLOCK big_buff;
 long int grp;
 FILE_ENTRY * fptr;
 short int subfile;
 short int lock_slot = 0;
 short int rec_offset;
 short int used_bytes;
 short int rec_bytes;
 long int record_len;
 long int recs_in_group;      /* Records in group being processed */
 long int blocks_in_group;    /* Blocks in group being processed */
 long int used_bytes_in_group;
 long int non_numeric_ids = 0;

 /* Group statistics */
 long int empty_groups = 0;               /* Empty groups */
 long int overflowed_groups = 0;          /* Single overflow block */
 long int badly_overflowed_groups = 0;    /* Multiple overflow blocks */
 long int smallest_group = LONG_MAX;      /* Blocks in smallest group */
 long int largest_group = 0;              /* Blocks in largest group */
 long int total_blocks = 0;               /* Total blocks in all groups */

 /* Per group statistics */
 long int min_recs_per_group = LONG_MAX;  /* Minimum records per group */
 long int max_recs_per_group = 0;         /* Maximum records per group */
 long int min_bytes_per_group = LONG_MAX; /* Min used bytes per group */
 long int max_bytes_per_group = 0;        /* Max used bytes per group */

 /* Record statistics for normal records */
 long int record_count = 0;               /* Number of records */
 long int smallest_record = LONG_MAX;     /* Smallest record size */
 long int largest_record = 0;             /* Largest record size */
 int64 total_record_bytes = 0;            /* Total used space */

 /* Record statistics for large records */
 long int large_record_count = 0;         /* Number of records */
 long int smallest_lrg_record = LONG_MAX; /* Smallest record size */
 long int largest_lrg_record = 0;         /* Largest record size */
 int64 total_lrg_record_bytes = 0;        /* Total used space */

 /* Record length statistics */
 long int histogram[11] = {0,0,0,0,0,0,0,0,0,0,0};
                                          /* 16 bytes - 8k bytes, over 8k */

 short int i;
 long int n;
 char * p;

 dh_err = 0;
 process.os_error = 0;

 buff = (DH_BLOCK *)(&dh_buffer);

 fptr = FPtr(dh_file->file_id);
 while(fptr->file_lock < 0) Sleep(1000); /* Clearfile in progress */

 /* Use inhibit count to hold off splits and merges while we do the analysis */

 StartExclusive(FILE_TABLE_LOCK, 1);
 fptr->inhibit_count++;
 EndExclusive(FILE_TABLE_LOCK);


 group_bytes = (short int)(dh_file->group_size);
 for (group = 1; group <= fptr->params.modulus; group++)
  {
   if (my_uptr->events) process_events();

   if (k_exit_cause & K_INTERRUPT)   /* User interrupt or logout*/
    {
     goto exit_dh_analyse;
    }

   /* Lock group */

   lock_slot = GetGroupReadLock(dh_file, group);

   /* Process this group */

   subfile = PRIMARY_SUBFILE;
   grp = group;

   recs_in_group = 0;
   blocks_in_group = 0;

   do {
       blocks_in_group++;
       used_bytes_in_group = 0;

       /* Read group */

       if (!dh_read_group(dh_file, subfile, grp, (char *)buff, group_bytes))
        {
         goto exit_dh_analyse;
        }

       /* Scan group */

       used_bytes = buff->used_bytes;
       used_bytes_in_group += used_bytes;

       rec_offset = offsetof(DH_BLOCK, record);
       while(rec_offset < used_bytes)
        {
         rec_ptr = (DH_RECORD *)(((char *)buff) + rec_offset);
         recs_in_group++;
         rec_bytes = rec_ptr->next;

         for(i = 0, p = rec_ptr->id; i < rec_ptr->id_len; i++, p++)
          {
           if (!IsDigit(*p))
            {
             non_numeric_ids++;
             break;
            }
          }

         if (rec_ptr->flags & DH_BIG_REC)    /* Large record */
          {
           if (!dh_read_group(dh_file, OVERFLOW_SUBFILE,
                              GetFwdLink(dh_file, rec_ptr->data.big_rec),
                              (char *)(&big_buff), DH_BIG_BLOCK_SIZE))
            {
             goto exit_dh_analyse;
            }

           record_len = rec_bytes + big_buff.data_len;
           large_record_count++;
           if (record_len < smallest_lrg_record) smallest_lrg_record = record_len;
           if (record_len > largest_lrg_record) largest_lrg_record = record_len;
           total_lrg_record_bytes += record_len;
          }
         else                                /* Not large record */
          {
           record_count++;
           record_len = rec_bytes;
           if (rec_bytes < smallest_record) smallest_record = rec_bytes;
           if (rec_bytes > largest_record) largest_record = rec_bytes;
           total_record_bytes += rec_bytes;
          }

         for(i = 0, n = 16; i < 10; n <<= 1, i++)
          {
           if (record_len <= n)
            {
             histogram[i] += 1;
             goto histogram_done;
            }
          }
         histogram[10] += 1;
histogram_done:

         rec_offset += rec_bytes;
        }

       /* Move to next group buffer */

       subfile = OVERFLOW_SUBFILE;
       grp = GetFwdLink(dh_file, buff->next);
      } while(grp != 0);

   /* Unlock group */
   
   FreeGroupReadLock(lock_slot);
   lock_slot = 0;

   /* Accumulate figures from this group */

   switch(blocks_in_group)
    {
     case 0:
        empty_groups++;
        break;
     case 1:
        break;
     case 2:
        overflowed_groups++;
        break;
     default:
        badly_overflowed_groups++;
        break;
    }

   if (recs_in_group > max_recs_per_group) max_recs_per_group = recs_in_group;
   if (recs_in_group < min_recs_per_group) min_recs_per_group = recs_in_group;

   if (blocks_in_group > largest_group) largest_group = blocks_in_group;
   if (blocks_in_group < smallest_group) smallest_group = blocks_in_group;

   if (used_bytes_in_group > max_bytes_per_group) max_bytes_per_group = used_bytes_in_group;
   if (used_bytes_in_group < min_bytes_per_group) min_bytes_per_group = used_bytes_in_group;

   total_blocks += blocks_in_group;
  }

 status = TRUE;

exit_dh_analyse: 
 if (lock_slot) FreeGroupReadLock(lock_slot);

 /* Decrement inhibit count now that we have finished */

 StartExclusive(FILE_TABLE_LOCK, 2);
 fptr->inhibit_count--;
 EndExclusive(FILE_TABLE_LOCK);

 /* Construct return string */

 if (largest_group == 0) smallest_group = 0;
 if (max_recs_per_group == 0) min_recs_per_group = 0;
 if (max_bytes_per_group == 0) min_bytes_per_group = 0;
 if (largest_record == 0) smallest_record = 0;
 if (largest_lrg_record == 0) smallest_lrg_record = 0;

/*                 1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31*/
 sprintf(result, "%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%Ld,%ld,%ld,%Ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld",
         fptr->params.modulus,     /*  1 Modulus */
         empty_groups,             /*  2 Empty groups */
         overflowed_groups,        /*  3 Single overflow groups */
         badly_overflowed_groups,  /*  4 Badly overflowed groups */
         min_bytes_per_group,      /*  5 Minimum used bytes per group */
         max_bytes_per_group,      /*  6 Maximum used bytes per group */
         smallest_group,           /*  7 Minimum blocks per group */
         largest_group,            /*  8 Maximum blocks per group */
         total_blocks,             /*  9 Total used blocks (excl large recs) */
         min_recs_per_group,       /* 10 Minimum records per group */
         max_recs_per_group,       /* 11 Maximum records per group */
         record_count,             /* 12 Number of non-large records */
         large_record_count,       /* 13 Number of large records */
         smallest_record,          /* 14 Minimum bytes per non-large record */
         largest_record,           /* 15 Maximum bytes per non-large record */
         total_record_bytes,       /* 16 Total non-large record space */
         smallest_lrg_record,      /* 17 Minimum bytes per large record */
         largest_lrg_record,       /* 18 Maximum bytes per large record */
         total_lrg_record_bytes,   /* 19 Total large record space */
         histogram[0],             /* 20 Records up to 16 bytes */
         histogram[1],             /* 21 Records up to 32 bytes */
         histogram[2],             /* 22 Records up to 64 bytes */
         histogram[3],             /* 23 Records up to 128 bytes */
         histogram[4],             /* 24 Records up to 256 bytes */
         histogram[5],             /* 25 Records up to 512 bytes */
         histogram[6],             /* 26 Records up to 1k bytes */
         histogram[7],             /* 27 Records up to 2k bytes */
         histogram[8],             /* 28 Records up to 4k bytes */
         histogram[9],             /* 29 Records up to 8k bytes */
         histogram[10],            /* 30 Records over 8k bytes */
         non_numeric_ids);         /* 31 Records with non-numeric ids */
 return status;
}

/* ======================================================================
   dir_analyse()  -  Analyse a directory file                             */

Private bool dir_analyse(
   FILE_VAR * fvar,
   char * result)
{
 bool status = FALSE;
 FILE_ENTRY * fptr;
 char name[MAX_PATHNAME_LEN+1];
 long int record_count = 0;               /* Number of records */
 int64 total_record_bytes = 0;
 int64 smallest_record = LONG_MAX;     /* Smallest record size */
 int64 largest_record = 0;             /* Largest record size */
 char parent_name[MAX_PATHNAME_LEN+1];
 int parent_len;
 DIR * dfu;
 struct dirent * dp;
 struct stat statbuf;


 fptr = FPtr(fvar->file_id);

 if ((dfu = opendir((char *)(fptr->pathname))) != NULL)
  {
   strcpy(parent_name, (char *)(fptr->pathname));
   parent_len = strlen(parent_name);
   if (parent_name[parent_len-1] == DS) parent_name[parent_len-1] = '\0';

   while((dp = readdir(dfu)) != NULL)
    {
     if (my_uptr->events) process_events();

     if (k_exit_cause & K_INTERRUPT)  /* User interrupt or logout */
      {
       goto exit_dir_analyse;
      }

     sprintf(name, "%s%c%s", parent_name, DS, dp->d_name);
     if (stat(name, &statbuf)) break;

     if (statbuf.st_mode & S_IFREG)
      {
       record_count++;
       total_record_bytes += statbuf.st_size;
       if (statbuf.st_size < smallest_record) smallest_record = statbuf.st_size;
       if (statbuf.st_size > largest_record) largest_record = statbuf.st_size;
      }
    }

   closedir(dfu);
   status = TRUE;
  }

 if (largest_record == 0) smallest_record = 0;
 sprintf(result,
         "%ld,%lld,%lld,%lld",
         record_count,
         total_record_bytes,
         smallest_record,
         largest_record);

exit_dir_analyse:
 return status;
}

/* ====================================================================== */

int64 dir_filesize(FILE_VAR * fvar)
{
 FILE_ENTRY * fptr;
 char name[MAX_PATHNAME_LEN+1];
 char parent_name[MAX_PATHNAME_LEN+1];
 int parent_len;
 int64 total_bytes = 0;

 DIR * dfu;
 struct dirent * dp;
 struct stat statbuf;


 fptr = FPtr(fvar->file_id);

 if ((dfu = opendir((char *)(fptr->pathname))) != NULL)
  {
   strcpy(parent_name, (char *)(fptr->pathname));
   parent_len = strlen(parent_name);
   if (parent_name[parent_len-1] == DS) parent_name[parent_len-1] = '\0';

   while((dp = readdir(dfu)) != NULL)
    {
     if (my_uptr->events) process_events();

     if (k_exit_cause & K_INTERRUPT)  /* User interrupt or logout */
      {
       total_bytes = -1;
       goto exit_dir_filesize;
      }

     sprintf(name, "%s%c%s", parent_name, DS, dp->d_name);
     if (stat(name, &statbuf)) break;

     if (statbuf.st_mode & S_IFREG)
      {
       total_bytes += statbuf.st_size;
      }
    }

   closedir(dfu);
  }

exit_dir_filesize:
 return total_bytes;
}

/* END-CODE */
