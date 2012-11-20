/* STRINGS.C
 * String manipulation functions
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
 * 08 Jun 05  2.2-1 Added ts_stack() and ts_unstack().
 * 03 Jun 05  2.2-1 0365 s_free() was caching strings incorrectly.
 * 13 May 05  2.1-15 Added error arg to s_make_contiguous().
 * 20 Sep 04  2.0-2 Use message handler.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * alloc_c_string()      Make a C string copy of a chunked string
 * dupstring()           Duplicate a possibly null C string
 * s_alloc()             Allocate string chunk in virtual memory
 * s_free()              Free string chunk chain
 * s_make_contiguous()   Make a string contiguous
 * setstring()           Copy a string to a dynamically allocated area
 * ts_copy()             Copy string of given length to target
 * ts_copy_c_string()    Copy C string to target
 * ts_copy_byte()        Copy byte to target
 * ts_fill()             Insert repeated character
 * ts_init()             Initialise target string chain control
 * ts_new_chunk()        Allocate new target chunk
 * ts_printf()           Target string version of printf
 * ts_terminate()        Terminate target string
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include <stdarg.h>

/* #define MEMDIAG */

/* Small memory block allocation cache */

Private STRING_CHUNK * pool[3] = {NULL,NULL,NULL}; /* Chain heads */
Private long int pool_bytes[3] = {2, 8, 32};       /* Largest block on chain */
Private long int pool_count[3] = {0,0,0};          /* No of blocks on chain */
Private long int pool_max[3] = {20,20,64};         /* Max blocks on chain */
#define SMALL_BLOCK_SIZE 32                        /* Largest "small" block */
#define POOL_RANGES (sizeof(pool_bytes) / sizeof(long int))

#ifdef MEMDIAG
Private long int pool_alloc_hit[3] = {0,0,0};      /* Allocated from pool */
Private long int pool_alloc_miss[3] = {0,0,0};     /* No block in pool */
Private long int pool_free_hit[3] = {0,0,0};       /* Freed to pool */
Private long int pool_free_miss[3] = {0,0,0};      /* Freed but pool full */
Private long int cts[18] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; /* Allocation sizes */
#endif

/* Data for ts_init(), ts_new_chunk(), ts_terminate() */
Private STRING_CHUNK ** ts_tgt_head;   /* Ptr to chain head ptr */
Private STRING_CHUNK * ts_tgt_str; /* Current chunk */
Private long int ts_tgt_bytes;     /* Total bytes in string */
Private long int ts_tgt_alloc_len; /* Size to allocate next (may be big) */
Private short int ts_tgt_bytes_remaining; /* Bytes available */
Private char * ts_tgt;             /* Next byte to write */

Private STRING_CHUNK ** stacked_ts_tgt_head;
Private STRING_CHUNK * stacked_ts_tgt_str;
Private long int stacked_ts_tgt_bytes;
Private long int stacked_ts_tgt_alloc_len;
Private short int stacked_ts_tgt_bytes_remaining;
Private char * stacked_ts_tgt;

/* ======================================================================
   alloc_c_string()  -  Make a C string copy of a chunked string          */

char * alloc_c_string(DESCRIPTOR * descr)
{
 char * p;
 char * q;
 STRING_CHUNK * str;

 if ((str = descr->data.str.saddr) == NULL)
  {
   p = (char *)k_alloc(39,1);
   *p = '\0';
  }
 else
  {
   p = (char *)k_alloc(40,(str->string_len) + 1);
   if (p != NULL)
    {
     q = p;
     if ((str = descr->data.str.saddr) != NULL)
      {
       do {
           memcpy(q, str->data, str->bytes);
           q += str->bytes;
          } while((str = str->next) != NULL);
      }
     *q = '\0';
    }
  }

 return p;
}

/* ======================================================================
   dupstring()  -  Duplicate a possibly null C string                     */

char * dupstring(char * str)
{
 char * s;

 if (str != NULL)
  {
   s = (char *)k_alloc(108, strlen(str) + 1);
   strcpy(s, str);
   return s;
  }
 else return NULL;
}

/* ====================================================================== */

STRING_CHUNK * s_alloc(
   long int size,           /* Desired minimum chunk size (data part) */
   short int * actual_size) /* Actual size allocated (data part) */
{
 unsigned short int reqd_size;
 STRING_CHUNK * p;
 short int i;

#ifdef MEMDIAG
 {
  /* Record allocation size histogram data */

  long int hi;
  short int x;
  for(x = 0, hi = 1; x < 18; x++, hi = hi << 1)
   {
    if (size <= hi)
     {
      cts[x]++;
      break;
     }
   }
 }
#endif

 if (size <= SMALL_BLOCK_SIZE)
  {
   /* May be able to use small block cache */

   for(i = 0; i < POOL_RANGES; i++)
    {
     if (size <= pool_bytes[i])
      {
       size = pool_bytes[i];    /* Round up to cached size for small strings */
       if (pool_count[i])
        {
         p = pool[i];
         pool[i] = p->next;
         pool_count[i]--;

#ifdef MEMDIAG
         pool_alloc_hit[i]++;
#endif
         goto allocated_from_cache;
        }

#ifdef MEMDIAG
       pool_alloc_miss[i]++;
#endif
       break;
      }
    }
  }
 else if (size > MAX_STRING_CHUNK_SIZE)
  {
   size = MAX_STRING_CHUNK_SIZE;
  }

 reqd_size = ((unsigned short int)size) + sizeof(struct STRING_CHUNK) - 1;

 p = (STRING_CHUNK *)k_alloc(2, reqd_size);

allocated_from_cache:

 p->next = NULL;
 p->alloc_size = (short int)size;
 p->bytes = 0;
 p->field = 0;                  /* No active hint */

 *actual_size = (short int)size;

 return p;
}

/* ======================================================================
   s_free()  -  Free a string chunk chain                                 */

void s_free(STRING_CHUNK * str)
{
 STRING_CHUNK * next_str;
 short int bytes;
 short int i;


 while(str != NULL)
  {
   next_str = str->next;

   if ((bytes = str->alloc_size) <= SMALL_BLOCK_SIZE)
    {
     /* May be able to cache this block for the future */

     for(i = 0; i < POOL_RANGES; i++)
      {
       if (bytes == pool_bytes[i])   /* 0365 was <= */
        {
         if (pool_count[i] < pool_max[i])
          {
           str->next = pool[i];
           pool[i] = str;
           pool_count[i]++;
#ifdef MEMDIAG
           pool_free_hit[i]++;
#endif
           goto added_to_pool;
          }

#ifdef MEMDIAG
         pool_free_miss[i]++;
#endif
         break;
        }
      }
    }

   /* We did not manage to put this block in a pool */

   k_free(str);

added_to_pool:
   str = next_str;
  }
}

/* ======================================================================
   s_make_continguous()  -  Make string contiguous                        */

STRING_CHUNK * s_make_contiguous(
   STRING_CHUNK * old_str,
   short int * errnum)
{
 STRING_CHUNK * str;
 STRING_CHUNK * new_str;
 short int reqd_size;
 char * p;

 if ((old_str == NULL) || (old_str->next == NULL))
  {
   return old_str;      /* Nothing to do */
  }

 if (old_str->string_len > 32767)
  {
   if (errnum != NULL)
    {
     *errnum = ER_LENGTH;
     return old_str;
    }
   k_error(sysmsg(1224));
  }

 reqd_size = (short int)(old_str->string_len + sizeof(struct STRING_CHUNK) - 1);

 new_str = (STRING_CHUNK *)k_alloc(2, reqd_size);
 new_str->next = NULL;
 new_str->field = 0;             /* No active hint */
 new_str->alloc_size = (short int)(old_str->string_len);
 new_str->string_len = old_str->string_len;
 new_str->bytes = (short int)(old_str->string_len);
 new_str->ref_ct = 1;

 p = new_str->data;

 for(str = old_str; str != NULL; str = str->next)
  {
   memcpy(p, str->data, str->bytes);
   p += str->bytes;
  }

 k_deref_string(old_str);          /* Sort out ref count */

 return new_str;
}

/* ======================================================================
   setqmstring()  -  Make a dynamically allocated string from a descriptor */

void setqmstring(char ** strptr, DESCRIPTOR * descr)
{
 k_get_string(descr);
 if (*strptr != NULL)
  {
   k_free(*strptr);
   *strptr = NULL;
  }
 if (descr->data.str.saddr) *strptr = alloc_c_string(descr);
}

/* ======================================================================
   setstring()  -  Copy a C string to a dynamically allocated area        */

void setstring(char ** strptr, char * string)
{
 if (*strptr != NULL)
  {
   k_free(*strptr);
   *strptr = NULL;
  }

 if (string != NULL)
  {
   *strptr = (char *)k_alloc(36, strlen(string) + 1);
   strcpy(*strptr, string);
  }
}

/* ======================================================================
  ts_copy()  -  Copy string to string chunk chain                         */

void ts_copy(
   char * src,           /* Ptr to data to append */
   int len)              /* Length of data to append */
{
 short int bytes_to_move;

 while (len > 0)
  {
   /* Allocate new chunk if the current one is full */

   if (ts_tgt_bytes_remaining == 0) ts_new_chunk();

   /* Copy what will fit into current chunk */

   bytes_to_move = min(ts_tgt_bytes_remaining, len);
   memcpy(ts_tgt, src, bytes_to_move);
   src += bytes_to_move;
   ts_tgt += bytes_to_move;
   len -= bytes_to_move;
   ts_tgt_bytes_remaining -= bytes_to_move;
  }
 
 return;
}

/* ======================================================================
  ts_copy_byte()  -  Copy one byte to string chunk chain                  */

void ts_copy_byte(char c) /* Byte to append */
{

 /* Allocate new chunk if the current one is full */

 if (ts_tgt_bytes_remaining == 0) ts_new_chunk();

 /* Copy byte */

 *(ts_tgt++) = c;
 ts_tgt_bytes_remaining--;
 
 return;
}

/* ======================================================================
  ts_copy_c_string()  -  Copy C string to string chunk chain                         */

void ts_copy_c_string(char * str)  /* Ptr to data to append */
{
 ts_copy(str, strlen(str));
}

/* ======================================================================
  ts_fill()  -  Insert repeated character into string chunk chain         */

void ts_fill(
   char c,               /* Character to append */
   long int len)         /* Length of data to append */
{
 short int n;

 while (len > 0)
  {
   /* Allocate new chunk if the current one is full */

   if (ts_tgt_bytes_remaining == 0) ts_new_chunk();

   n = (short int)min(ts_tgt_bytes_remaining, len);
   memset(ts_tgt, c, n);
   ts_tgt += n;
   len -= n;
   ts_tgt_bytes_remaining -= n;
  }
 
 return;
}

/* ======================================================================
   ts_printf()  -  ts version of printf()                                 */

int ts_printf(char * template_string, ...)
{
 char s[500];
 va_list arg_ptr;
 int n;

 va_start(arg_ptr, template_string);

 n = vsprintf(s, template_string, arg_ptr);

 ts_copy(s, n);

 va_end(arg_ptr);
 return n;
}

/* ======================================================================
   ts_init()  -  Initialise target string chain control                   */

void ts_init(
   STRING_CHUNK ** head,
   long int base_size)    /* Desired size for first chunk */
{
 ts_tgt_head = head;             /* Remember chain head address... */
 ts_tgt_alloc_len = base_size;   /* ...and desired size */
 ts_tgt_bytes = 0;               /* Total bytes in string */
 ts_tgt_str = NULL;              /* No chunks yet */
 ts_tgt_bytes_remaining = 0;
}

/* ======================================================================
   ts_new_chunk()  -  Add a chunk to the target string chain              */

void ts_new_chunk()
{
 STRING_CHUNK * tgt;


 tgt = s_alloc(ts_tgt_alloc_len, &ts_tgt_bytes_remaining);
 ts_tgt_alloc_len = ts_tgt_bytes_remaining * 2;

 if (ts_tgt_str == NULL)           /* Creating first chunk */
  {
   *ts_tgt_head = tgt;
  }
 else                           /* Appending new chunk */
  {
   ts_tgt_str->bytes = ts_tgt - ts_tgt_str->data; /* Length of this chunk */
   ts_tgt_bytes += ts_tgt_str->bytes; /* Accumulate total length */
   ts_tgt_str->next = tgt;
  }

 ts_tgt_str = tgt;
 ts_tgt = ts_tgt_str->data;

 return;
}

/* ======================================================================
   ts_terminate()  -  Terminate target string                             */

long int ts_terminate()
{
 if (ts_tgt_str != NULL)
  {
   ts_tgt_str->bytes = ts_tgt - ts_tgt_str->data; /* Length of this chunk */
   ts_tgt_bytes += ts_tgt_str->bytes;     /* Accumulate total length */

   /* Go back to first chunk and fill in reference count and total length */

   ts_tgt_str = *ts_tgt_head;
   ts_tgt_str->ref_ct = 1;
   ts_tgt_str->string_len = ts_tgt_bytes;
  }
 else
  {
   *ts_tgt_head = NULL;
  }

 ts_tgt_head = NULL;

 return ts_tgt_bytes;
}

/* ====================================================================== */

void s_free_all()
{
 STRING_CHUNK * p;
 short int i;

 /* Free all pool memory. This is primarilly for the memory leak check
    of MEMTRACE.                                                        */

 for(i = 0; i < POOL_RANGES; i++)
  {
   while(pool[i] != NULL)
    {
     p = pool[i];
     pool[i] = p->next;
     pool_count[i]--;
     k_free(p);
    }
  }

#ifdef MEMDIAG

 tio_printf("Size   A-Hit  A-Miss   F-Hit  F-Miss\n");
 for(i = 0; i < POOL_RANGES; i++)
  {
   tio_printf("%4ld  %6ld  %6ld  %6ld  %6ld\n",
              pool_bytes[i], pool_alloc_hit[i], pool_alloc_miss[i],
              pool_free_hit[i], pool_free_miss[i]);
  }

 {
  long int hi;

  tio_printf("  Size  Allocations\n");
  for(i = 0; i < 18; i++)
   {
    tio_printf("%6ld  %ld\n", 1L << i, cts[i]);
   }
 }
#endif
}

void ts_stack()
{
 stacked_ts_tgt_head = ts_tgt_head;
 stacked_ts_tgt_str = ts_tgt_str;
 stacked_ts_tgt_bytes = ts_tgt_bytes;
 stacked_ts_tgt_alloc_len = ts_tgt_alloc_len;
 stacked_ts_tgt_bytes_remaining = ts_tgt_bytes_remaining;
 stacked_ts_tgt = ts_tgt;
}

void ts_unstack()
{
 ts_tgt_head = stacked_ts_tgt_head;
 ts_tgt_str = stacked_ts_tgt_str;
 ts_tgt_bytes = stacked_ts_tgt_bytes;
 ts_tgt_alloc_len = stacked_ts_tgt_alloc_len;
 ts_tgt_bytes_remaining = stacked_ts_tgt_bytes_remaining;
 ts_tgt = stacked_ts_tgt;
}

/* END-CODE */

