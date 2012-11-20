/* OP_LOCAT.C
 * LOCATE statement opcodes
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
 * 19 Jul 05  2.2-4 0373 Use of AR with numeric data mis-sequenced data if the
 *                  value in the dynamic array spanned chunks.
 * 03 May 05  2.1-13 Added support for case insensitive strings.
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
#include "header.h"

Private void locate(bool numeric_mode);

/* ======================================================================
   op_locate()  -  Locate item in dynamic array                           */

void op_locates()  /* Numeric data not treated as special case */
{
 locate(FALSE);
}

void op_locate()    /* Integer numeric data treated as special case */
{
 locate(TRUE);
}

void op_locatef()   /* LOCATE() function */
{
 DESCRIPTOR result;
 DESCRIPTOR * descr;

 InitDescr(&result, UNASSIGNED);
 InitDescr(e_stack, ADDR);
 (e_stack++)->data.d_addr = &result;
 locate(TRUE);
 descr = e_stack - 1;
 if (descr->data.value)   /* Returned TRUE, replace with value of result */
  {
   descr->data.value = result.data.value;
  }
}

/* ====================================================================== */

Private void locate(bool numeric_mode)
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  ADDR to result var         | Status 1=found, 0=not found |
     |-----------------------------|-----------------------------| 
     |  Order string               |                             |
     |-----------------------------|-----------------------------| 
     |  Subvalue number            |                             |
     |-----------------------------|-----------------------------| 
     |  Value number               |                             |
     |-----------------------------|-----------------------------| 
     |  Field number               |                             |
     |-----------------------------|-----------------------------| 
     |  Source string              |                             |
     |-----------------------------|-----------------------------| 
     |  String to find             |                             |
     |=============================|=============================|
 */

 DESCRIPTOR * result_descr;     /* Result index descriptor */
 DESCRIPTOR * descr;            /* Various descriptors */
 DESCRIPTOR * src_descr;        /* Source string to search */
 DESCRIPTOR * search_descr;     /* String to locate */
 long int field;                /* Start field for search */
 long int value;                /* Start value for search */
 long int subvalue;             /* Start subvalue for search */
 long int index;                /* Index position to return (working) */
 long int result;               /* Index position to return (actual) */
 STRING_CHUNK * first_chunk;
 short int offset;              /* Offset into current source chunk */
 STRING_CHUNK * src_hdr;
 short int src_bytes_remaining;
 STRING_CHUNK * search_hdr;
 short int search_bytes_remaining;
 long int search_string_len;
 char order_string[2+1];        /* Ordering code */
 short int order_string_len;
 short int status = 0;          /* Assume failed for now */
 bool ordered = FALSE;
 bool ascending = FALSE;
 bool right = FALSE;
 bool nocase;
 short int search_mode;
#define SEARCH_FOR_FIELD    0
#define SEARCH_FOR_VALUE    1
#define SEARCH_FOR_SUBVALUE 2

#define MAX_NUMERIC_STRING 20
 char numeric_string[MAX_NUMERIC_STRING+1];   /* Search item */
 short int numeric_len;
 long int search_value;           /* Search item as a number */
 bool numeric_compare = FALSE;  /* Try numeric compare? */
 char item_string[MAX_NUMERIC_STRING+1];
 short int item_len;
 long int item_value;
 bool skipping;
 char * srch;
 char c;
 char mark;                    /* Mark which ends item */
 short int n;
 short int x;
 char * p;
 char * q;
 char * r;
 long int search_offset;
 long int src_len;
 long int chunk_offset;
 long int new_hint_offset;
 STRING_CHUNK * str;

 nocase = (process.program.flags & HDR_NOCASE) != 0;

 /* Get pointer to descriptor to receive item index */

 result_descr = e_stack - 1;
 do {
     result_descr = result_descr->data.d_addr;
    } while(result_descr->type == ADDR);
 k_pop(1);          /* Dismiss ADDR */

 result = 0;

 /* Fetch ordering string */

 descr = e_stack - 1;
 order_string_len = k_get_c_string(descr, order_string, 2);
 k_dismiss();

 /* Fetch field, value and subvalue of search start */

 descr = e_stack - 1;
 GetInt(descr);
 subvalue = descr->data.value;

 descr = e_stack - 2;
 GetInt(descr);
 value = descr->data.value;

 descr = e_stack - 3;
 GetInt(descr);
 field = descr->data.value;

 k_pop(3);         /* Dismiss subvalue, value and field positions */

 /* Sort out defaults etc for start position information */

 if (field <= 0) field = 1;

 if (value <= 0) /* Locating field */
  {
   search_mode = SEARCH_FOR_FIELD;
   index = field;
   mark = FIELD_MARK;
   value = 1;
   subvalue = 1;
  }
 else if (subvalue <= 0) /* Locating value */
  {
   search_mode = SEARCH_FOR_VALUE;
   index = value;
   mark = VALUE_MARK;
   subvalue = 1;
  }
 else /* Locating subvalue */
  {
   search_mode = SEARCH_FOR_SUBVALUE;
   index = subvalue;
   mark = SUBVALUE_MARK;
  }

 /* Fetch source string to be searched */
 
 src_descr = e_stack - 1;
 k_get_string(src_descr);
 src_hdr = src_descr->data.str.saddr;

 /* Fetch string to be located */
 
 search_descr = e_stack - 2;
 k_get_string(search_descr);

 search_hdr = search_descr->data.str.saddr;
 if (search_hdr == NULL)
  {
   search_string_len = 0;
   search_bytes_remaining = 0;
  }
 else
  {
   search_string_len = search_hdr->string_len;
   search_bytes_remaining = search_hdr->bytes;
   srch = search_hdr->data;
  }
 search_offset = 0;

 /* Process the ordering string */

 if (order_string_len < 0) goto exit_locate; /* Failed to extract ordering string */

 switch(UpperCase(order_string[0]))
  {
   case 'A':
      ordered = TRUE;
      ascending = TRUE;
      break;

   case 'D':
      ordered = TRUE;
      break;
  }

 if (ordered)
  {
   switch(UpperCase(order_string[1]))
    {
     case 'R':
        right = TRUE;

        if (numeric_mode)
         {
          /* Try to convert search string to a numeric value. */

          if (search_hdr != NULL)
           {
            if ((n = (short int)(search_hdr->string_len)) <= MAX_NUMERIC_STRING)
             {
              /* Extract potential numeric search string */

              str = search_hdr;
              q = numeric_string;
              while(n)
               {
                x = str->bytes;
                memcpy(q, str->data, x);
                q += x;
                n -= x;
                str = str->next;
               }

              numeric_len = (short int)(search_hdr->string_len);
              numeric_compare = strnint(numeric_string, numeric_len, &search_value);
             }
           }
         }
        break;
    }
  }

 /* Find start position of search */

 first_chunk = src_hdr;

 if (src_hdr == NULL)   /* Source string is null */
  {
   if (search_hdr == NULL)    /* Finding null in null */
    {
     status = (field == 1) && (value == 1) && (subvalue == 1);
     index = 1;
    }
   else
    {
     status = FALSE;
    }
   goto found;
  }

 if (!find_item(src_hdr, field, value, subvalue, &src_hdr, &offset))
  {
   goto found; /* Start position is off end of string */
  }

 /* Now examine each item of the appropriate type in turn until we meet
    the search condition.                                                */

 /* Point to source item. Note that find_item() will have left us pointing
    one byte beyond the string chunk if the mark was in the final byte.    */

 chunk_offset = src_hdr->offset - offset;
 new_hint_offset = src_hdr->offset;
 p = src_hdr->data + offset;
 src_bytes_remaining = src_hdr->bytes - offset;

 if (src_bytes_remaining == 0)
  {
   if (src_hdr->next == NULL) goto found;

   chunk_offset += src_hdr->bytes;

   src_hdr = src_hdr->next;
   p = src_hdr->data;
   src_bytes_remaining = src_hdr->bytes;
  }

 /* Check for special case of higher value mark at start of item, for
    example
        abc FM FM def
    when doing 
        LOCATE "xyz" IN S<2,1> 
    In this case we want to return status 0 with index value 1.
    If we had been looking for a null string, status should be 1.       */

 c = *p; 
 if (IsDelim(c) && ((u_char)c > (u_char)mark))
  {
   status = (search_string_len == 0); /* Found a null? */
   goto found;
  }

 /* Special case for right justifed search where the search string can be
    treated as a number.                                                  */

 if (numeric_compare)
  {
   /* Extract up to MAX_NUMERIC_STRING characters from each item.
      If the item is no longer than this and is numeric, perform a
      numeric comparison.
      If the item is longer than the search item is is greater.
      If the item is shorter than the search item is is less.
      If the lengths are equal, perform a character based comparison. */

   skipping = FALSE;
   item_len = 0;    /* 0373 Moved out of loop */
   while(1)  /* For each source chunk */
    {
     while(src_bytes_remaining-- > 0)
      {
       c = *(p++);

       /* Check if this is the end of an item. This is when we find a mark
          character equal to or higher than the level we are searching */

       if (IsDelim(c) && ((u_char)c >= (u_char)mark))
        {
         if (skipping)
          {
           skipping = FALSE;
          }
         else
          {
           if (strnint(item_string, item_len, &item_value)) /* Is numeric */
            {
             if (item_value == search_value)
              {
               status = TRUE;
               goto found;
              }
             if ((item_value < search_value) != ascending) goto found;
            }
           else                                             /* Not numeric */
            {
             if (item_len == numeric_len)
              {
               if ((SortCompare(item_string, numeric_string, item_len, nocase) < 0) != ascending) goto found;
              }
             else if ((item_len < numeric_len) != ascending) goto found;
            }
          }

         if (c != mark) goto not_found;    /* Higher mark */

         index++;
         new_hint_offset = chunk_offset + (p - src_hdr->data);

         item_len = 0;
        }
       else if (!skipping)  /* Still in item */
        {
         if (item_len < MAX_NUMERIC_STRING)
          {
           item_string[item_len++] = c;
          }
         else   /* Item is longer than valid for a numeric string */
          {
           if (ascending) goto not_found;   /* Gone past desired item */

           /* Skip remainder of this item */

           skipping = TRUE;
          }
        }
      }

     /* End of source chunk */

     if (src_hdr->next == NULL) break;

     chunk_offset += src_hdr->bytes;
     src_hdr = src_hdr->next;
     p = src_hdr->data;
     src_bytes_remaining = src_hdr->bytes;
    }

   /* Check final item */

   if (!skipping)
    {
     if (strnint(item_string, item_len, &item_value)) /* Is numeric */
      {
       if (item_value == search_value)
        {
         status = TRUE;
         goto found;
        }

       if ((item_value < search_value) != ascending) goto found;
      }
     else                                             /* Not numeric */
      {
       if (item_len == numeric_len)
        {
         if ((SortCompare(item_string, numeric_string, item_len, nocase) < 0) != ascending) goto not_found;
        }
       else if ((item_len < numeric_len) != ascending) goto not_found;
      }
    }
   goto not_found;
  }

 /* Perform a standard string based search */

 while(1)
  {
   while(src_bytes_remaining-- > 0)
    {
     c = *(p++);
     if (c == mark) /* End of an item */
      {
       if (search_bytes_remaining == 0) /* Perfect match */
        {
         status = 1;      /* Found a match */
         goto found;
        }
       else /* Item to find is longer than substring */
        {
         if (right && !ascending) goto found;
        }

       index++;

       new_hint_offset = chunk_offset + (p - src_hdr->data);

       /* Reset position in search string */
       
       search_hdr = search_descr->data.str.saddr;
       if (search_hdr == NULL)
        {
         search_bytes_remaining = 0;
        }
       else
        {
         search_bytes_remaining = search_hdr->bytes;
         srch = search_hdr->data;
        }
       search_offset = 0;
      }
     else /* Still in item */
      {
       if (IsDelim(c))
        {
         if ((c == FIELD_MARK)
            || (( c == VALUE_MARK) && (search_mode == SEARCH_FOR_SUBVALUE)))
          {
           if (search_bytes_remaining != 0) goto not_found;
           status = 1;
           goto found;
          }
        }

       if (search_bytes_remaining == 0)
        {
         if (search_hdr == NULL) /* Null search string */
          {
           /* Treat as source string item > search string item */
           if (ascending) goto found;
           goto skip_item;
          }

         if (search_hdr->next == NULL)
          {
           /* source string item > search string item */
           if (ascending) goto found; /* Position found */
           goto skip_item;
          }

         search_offset += search_hdr->bytes;

         search_hdr = search_hdr->next;
         search_bytes_remaining = search_hdr->bytes;
         srch = search_hdr->data;
        }

       search_bytes_remaining--;

       if (nocase) x = UpperCase((short int)c) - UpperCase(*(srch++));
       else x = ((short int)c) - *(srch++);

       if (x != 0) /* Found a difference */
        {
         if (ordered)
          {
           if (right)
            {
             /* For a right aligned locate, scan to the end of the source
                string to compare its length with the search string.
                If they are both of the same length, the value of x determines
                the outcome.
                If the source item is shorter, goto found if doing DR
                If the search item is shorter, goto found if doing AR        */

             src_len = search_offset + search_hdr->bytes - search_bytes_remaining;

             do {
                 while(src_bytes_remaining > 0)
                  {
                   c = *p;
                   if (IsDelim(c))
                    {
                     if ((u_char)c >= (u_char)mark) /* Found end of item */
                      {
                       if (src_len == search_string_len) goto compare;
                       if ((search_string_len < src_len) == ascending) goto found;
                       goto skip_item;
                      }
                    }
                   p++;
                   src_len++;
                   src_bytes_remaining--;
                  }

                 if (src_hdr->next == NULL) /* End of source */
                  {
                   if (src_len == search_string_len) goto compare;
                   if ((search_string_len < src_len) == ascending) goto found;
                   goto not_found;
                  }

                 chunk_offset += src_hdr->bytes;

                 src_hdr = src_hdr->next;
                 src_bytes_remaining = src_hdr->bytes;
                 p = src_hdr->data;
                } while(TRUE);
            }

compare:
           /* Examine the value of x
              x > 0  =>  source string item > search string item
              x < 0  =>  source string item < search string item
              x cannot be zero 

              We jump to found if
                  x > 0 and ascending
                  x < 0 and descending                             */

           if ((x > 0) == ascending) goto found; /* Position found */
         }

skip_item:
         /* Skip to next mark */

        do {
            if (search_mode == SEARCH_FOR_FIELD)
             {
              if (src_bytes_remaining)
               {
                r = (char *)memchr(p, FIELD_MARK, src_bytes_remaining);
                if (r != NULL)
                 {
                  r++;
                  src_bytes_remaining -= r - p;
                  p = r;
                  goto skipped;
                 }
               }
             }
            else
             {
              while(src_bytes_remaining-- > 0)
               {
                c = *(p++);
                if (IsDelim(c))
                 {
                  if (c == mark) goto skipped;
                  if ((u_char)c > (u_char)mark) goto not_found;
                 }
               }
             }

            if (src_hdr->next == NULL) /* End of source */
             {
              goto not_found;
             }

            chunk_offset += src_hdr->bytes;

            src_hdr = src_hdr->next;
            src_bytes_remaining = src_hdr->bytes;
            p = src_hdr->data;
           } while(TRUE);

skipped:
         index++;

         new_hint_offset = chunk_offset + (p - src_hdr->data);

         /* Reset position in search string */
        
         search_hdr = search_descr->data.str.saddr;
         if (search_hdr == NULL)
          {
           search_bytes_remaining = 0;
          }
         else
          {
           search_bytes_remaining = search_hdr->bytes;
           srch = search_hdr->data;
          }
         search_offset = 0;
        }
      }
    }

   if (src_hdr->next == NULL) break;

   chunk_offset += src_hdr->bytes;
   src_hdr = src_hdr->next;
   p = src_hdr->data;
   src_bytes_remaining = src_hdr->bytes;
  }

 if (search_bytes_remaining == 0) /* Found as final item in source */
  {
   status = 1;
   goto found;
  }

 if ((search_offset == 0)
     && ((search_hdr == NULL)
         || (search_bytes_remaining == search_hdr->bytes)))
  {
   /* Starting a new field at the end of the source string */
   goto found;
  }

not_found:

 index++;

found:

 result = index;

 if (status)       /* Found item - set new hint */
  {
   if (first_chunk != NULL)
    {
     if (search_mode == SEARCH_FOR_FIELD)
      {
       first_chunk->field = index;
       first_chunk->offset = new_hint_offset;
      }
     else
      {
       first_chunk->field = 0;
      }
    }
  }

exit_locate:

 k_dismiss(); /* Dismiss source string */
 k_dismiss(); /* Dismiss search string */

 /* Set result index */

 k_release(result_descr);
 InitDescr(result_descr, INTEGER);
 result_descr->data.value = result;

 /* Set status code on stack */

 InitDescr(e_stack, INTEGER);
 (e_stack++)->data.value = status;
}

/* ======================================================================
   op_maximum()  -  MAXIMUM() function                                    */

void op_maximum()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Dynamic array              | Result                      |
     |=============================|=============================|
 */

 k_recurse(pcode_maximum, 1); /* Execute recursive code */
}


/* END-CODE */
