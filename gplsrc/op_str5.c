/* OP_STR5.C
 * String handling opcodes
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
 * 15 Aug 07  2.6-0 Reworked remove pointers.
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 09 Feb 06  2.3-6 Added op_ismv().
 * 19 Oct 05  2.2-15 0425 Added revised op_csvdq().
 * 06 May 05  2.1-13 Added case insensitive string handling.
 * 03 May 05  2.1-13 Added support for case insensitive strings.
 * 15 Oct 04  2.0-5 0260 op_listindex() crashed if the item to find was a null
 *                  string.
 * 20 Sep 04  2.0-2 Use message handler.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 *    op_checksum    CHECKSUM() function
 *    op_compare     COMPARE() function
 *    op_convert     CONVERT statement
 *    op_crop        CROP()
 *    op_csvqd()     CSVDQ() function
 *    op_fconvert    CONVERT() function
 *    op_fldstor     FLDSTOR       Delimited substring assignment
 *    op_fldstorf    FLDSTORF      FIELDSTORE() function
 *    op_formcsv     FORMCSV() function
 *    op_listindx    LISTINDEX() function
 *    op_getrem      GETREM()
 *    op_ismv        ISMV() function
 *    op_lower       LOWER()
 *    op_quote       QUOTE() and DQUOTE()
 *    op_raise       RAISE()
 *    op_setrem      SETREM()
 *    op_squote      SQUOTE()
 *    op_subst       SUBSTITUTE() function
 *    op_swapcase    SWAPCASE() function
 *    op_vslice      VSLICE()      Value slice
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include "header.h"


void op_fldstorf(void);

Private void quote(char quote_char);
Private STRING_CHUNK * convert(void);

/* ======================================================================
   op_checksum()  -  CHECKSUM() function                                        */

void op_checksum()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Source string              |  Result value               |
     |=============================|=============================|
 */

 DESCRIPTOR * descr;
 STRING_CHUNK * src_hdr;
 long int src_len;
 char * p;
 unsigned long int checksum = 0;
 
 /* Get source string */
 
 descr = e_stack - 1;
 k_get_string(descr);
 src_hdr = descr->data.str.saddr;

 while(src_hdr != NULL)
  {
   src_len = src_hdr->bytes;
   p = src_hdr->data;

   while(src_len-- > 0)
    {
     checksum = (((checksum & 0x80000000L) != 0) | (checksum << 1)) ^ *(p++);
    }

   src_hdr = src_hdr->next;
  }

 k_dismiss();

 InitDescr(e_stack, INTEGER);
 ((e_stack++)->data.value) = (long int)checksum;
}

/* ======================================================================
   op_compare()  -  COMPARE function                                      */

void op_compare()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Justification              |  -1  string 1 < string 2    |
     |                             |   0  string 1 = string 2    |
     |                             |   1  string 1 > string 2    |
     |-----------------------------|-----------------------------| 
     |  String 2                   |                             |
     |-----------------------------|-----------------------------| 
     |  String 1                   |                             |
     |=============================|=============================|

   The definition of this opcode when right justified is not the same
   as for PI/open.  It does a simple right justified comparison with
   spaces inserted to the left of the shorter string.
 */

 DESCRIPTOR * descr;
 char justification[1+1];
 bool right_justified = FALSE;

 STRING_CHUNK * str1_hdr;
 long int str1_len;
 char * str1;
 short int str1_bytes_remaining;

 STRING_CHUNK * str2_hdr;
 long int str2_len;
 char * str2;
 short int str2_bytes_remaining;
 short int d = 0;
 long int n;

 descr = e_stack - 1;
 k_get_c_string(descr, justification, 1);
 if (justification[0] == 'R') right_justified = TRUE;
 k_dismiss();

 descr = e_stack - 1;
 k_get_string(descr);
 str2_hdr = descr->data.str.saddr;
 if (str2_hdr == NULL) str2_len = 0;
 else
  {
   str2_len = str2_hdr->string_len;
   str2 = str2_hdr->data;
   str2_bytes_remaining = str2_hdr->bytes;
  }

 descr = e_stack - 2;
 k_get_string(descr);
 str1_hdr = descr->data.str.saddr;
 if (str1_hdr == NULL) str1_len = 0;
 else
  {
   str1_len = str1_hdr->string_len;
   str1 = str1_hdr->data;
   str1_bytes_remaining = str1_hdr->bytes;
  }

 if (right_justified && (str1_len != str2_len))
  {
   /* Compare leading characters of longer string against spaces */

   if (str1_len > str2_len)
    {
     n = str1_len - str2_len;
     str1_len -= n;
     while(n--)
      {
       if ((d = (((short int)*(str1++)) - ' ')) != 0) goto mismatch;

       if (--str1_bytes_remaining == 0)
        {
         str1_hdr = str1_hdr->next;
         str1 = str1_hdr->data;
         str1_bytes_remaining = str1_hdr->bytes;
        }
      }
    }
   else
    {
     n = str2_len - str1_len;
     str2_len -= n;
     while(n--)
      {
       if ((d = (' ' - ((short int)*(str2++)))) != 0) goto mismatch;

       if (--str2_bytes_remaining == 0)
        {
         str2_hdr = str2_hdr->next;
         str2 = str2_hdr->data;
         str2_bytes_remaining = str2_hdr->bytes;
        }
      }
    }
  }

 /* Compare to end of shorter string */

 while(str1_len && str2_len)
  {
   if ((d = (((short int)(*((u_char *)str1++))) - *((u_char *)str2++))) != 0) goto mismatch;

   str1_len--;
   str2_len--;

   if (--str1_bytes_remaining == 0)
    {
     str1_hdr = str1_hdr->next;
     if (str1_hdr == NULL) break;
     str1 = str1_hdr->data;
     str1_bytes_remaining = str1_hdr->bytes;
    }

   if (--str2_bytes_remaining == 0)
    {
     str2_hdr = str2_hdr->next;
     if (str2_hdr == NULL) break;
     str2 = str2_hdr->data;
     str2_bytes_remaining = str2_hdr->bytes;
    }
  }

 /* If either string still has unprocessed characters, it is the greater
    string (d must be zero when we arrive here).                         */

 if (str1_len > str2_len) d = 1;      /* Can't do d = str1_len - str2_len... */
 else if (str1_len < str2_len) d = -1; /* ...as lengths are long ints. */
 goto exit_op_compare;

mismatch:
 if (d > 0) d = 1;
 else if (d < 0) d = -1;
 else d = 0;

exit_op_compare:
 k_dismiss();
 k_dismiss();

 InitDescr(e_stack, INTEGER);
 (e_stack++)->data.value = d;
}

/* ======================================================================
   op_convert()  -  CONVERT statement                                     */

void op_convert()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  String to modify           |                             |
     |-----------------------------|-----------------------------| 
     |  Characters to replace      |                             |
     |-----------------------------|-----------------------------| 
     |  Replacement characters     |                             |
     |=============================|=============================|
 */

 STRING_CHUNK * tgt;
 DESCRIPTOR * tgt_descr;

 /* The convert() function will do a k_get_string() on the source item
    and hence lose our reference to the original descriptor.  Find it
    now and remember its location.                                      */

 tgt_descr = e_stack - 1;
 while(tgt_descr->type == ADDR) tgt_descr = tgt_descr->data.d_addr;

 /* Do the conversion */

 tgt = convert();
 k_dismiss();
 k_dismiss();
 k_dismiss();

 /* Now save the result in the original data item */

 k_release(tgt_descr);
 InitDescr(tgt_descr, STRING);
 tgt_descr->data.str.saddr = tgt;
}

/* ======================================================================
   op_fconvert()  -  CONVERT() function                                   */

void op_fconvert()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Source string              |  Result string              |
     |-----------------------------|-----------------------------| 
     |  Replacement characters     |                             |
     |-----------------------------|-----------------------------| 
     |  Characters to replace      |                             |
     |=============================|=============================|
 */

 STRING_CHUNK * tgt;

 tgt = convert();
 k_dismiss();
 k_dismiss();
 k_dismiss();

 InitDescr(e_stack, STRING);
 (e_stack++)->data.str.saddr = tgt;
}

/* ======================================================================
   op_crop()  -  Remove redundant mark characters                         */

void op_crop()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Source string              | Result string               |
     |=============================|=============================|
 */
 
 DESCRIPTOR * src_descr;        /* Source string descriptor */
 DESCRIPTOR result;             /* Result string descriptor */
 STRING_CHUNK * src_str;        /* Current source chunk */
 char * src;                    /* Ptr to current byte */
 short int src_bytes_remaining; /* Remaining bytes in this chunk */

 long int src_f, src_v, src_s;
 long int tgt_f, tgt_v, tgt_s;
 char c;

 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Source string              |  Result string              |
     |=============================|=============================|
 */

 /* Get source string */

 src_descr = e_stack - 1;
 k_get_string(src_descr);
 src_str = src_descr->data.str.saddr;
 if (src_str == NULL) goto exit_crop;    /* Null source */
 
 /* Create a result string descriptor in our C stack space */

 result.type = STRING;
 result.flags = 0;
 result.data.str.saddr = NULL;

 ts_init(&result.data.str.saddr, min(64, src_str->string_len));

 /* Copy string, removing redundant mark characters */

 src_f = 1;
 src_v = 1;
 src_s = 1;
 tgt_f = 1;
 tgt_v = 1;
 tgt_s = 1;

 while(src_str != NULL)
  {
   src_bytes_remaining = src_str->bytes;
   src = src_str->data;

   /* Process all data in this chunk */

   while(src_bytes_remaining-- > 0)
    {
     c = *(src++);
     switch(c)
      {
       case FIELD_MARK:
          src_f++;
          src_v = 1;
          src_s = 1;
          break;

       case VALUE_MARK:
          src_v++;
          src_s = 1;
          break;

       case SUBVALUE_MARK:
          src_s++;
          break;

       default:
          if (tgt_f < src_f)
           {
            while(tgt_f < src_f) {ts_copy_byte(FIELD_MARK); tgt_f++;}
            tgt_v = 1; tgt_s = 1;
           }

          if (tgt_v < src_v)
           {
            while(tgt_v < src_v) {ts_copy_byte(VALUE_MARK); tgt_v++;}
            tgt_s = 1;
           }

          if (tgt_s < src_s)
           {
            while(tgt_s < src_s) {ts_copy_byte(SUBVALUE_MARK); tgt_s++;}
           }

          ts_copy_byte(c);
      }
    }

   /* Advance to next source chunk */

   src_str = src_str->next;
  }

 ts_terminate();

 k_dismiss();               /* Dismiss source string */

 /* Copy result to e-stack */

 *(e_stack++) = result;

exit_crop:
 return;
}

/* ======================================================================
   op_csvdq()  -  CSVDQ  -  Dequote a CSV string into a dynamic array     */

void op_csvdq()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Delimiter                  | Dequoted string             |
     |-----------------------------|-----------------------------|
     |  Source string              |                             |
     |=============================|=============================|
 */

 DESCRIPTOR * descr;
 STRING_CHUNK * src;
 STRING_CHUNK * result = NULL;
 short int bytes;
 char * p;
 char c;
 char delimiter;
 bool first;
 bool quoted;
 bool last_char_was_quote;

 descr = e_stack - 1;
 GetString(descr);
 if (descr->data.str.saddr == NULL)
  {
   k_dismiss();   /* Leave source as result */
  }
 else
  {
   delimiter = descr->data.str.saddr->data[0];
   k_dismiss();

   descr = e_stack - 1;
   GetString(descr);

   if ((src = descr->data.str.saddr) != NULL)
    {
     ts_init(&result, src->string_len); /* A good guess on the length */

     first = TRUE;
     quoted = FALSE;
     last_char_was_quote = FALSE;
     do {
         bytes = src->bytes;
         p = src->data;
         while(bytes--)
          {
           c = *(p++);
           if (c == '"')
            {
             if (first)
              {
               quoted = TRUE;
              }
             else if (last_char_was_quote)
              {
               ts_copy_byte(c);             /* "" becomes " */
               last_char_was_quote = FALSE;
              }
             else
              {
               last_char_was_quote = TRUE; /* Remember but do not copy */
              }
             first = FALSE;
            }
           else if (c == delimiter)
            {
             if (quoted && !last_char_was_quote)
              {
               ts_copy_byte(c);
              }
             else
              {
               ts_copy_byte(FIELD_MARK);
               first = TRUE;
               last_char_was_quote = FALSE;
               quoted = FALSE;
              }
            }
           else
            {
             last_char_was_quote = FALSE;  /* Format error (e.g. "ab"cd") */
             ts_copy_byte(c);
             first = FALSE;
            }
          }
        } while((src = src->next) != NULL);

     ts_terminate();

     k_dismiss();
     InitDescr(e_stack, STRING);
     (e_stack++)->data.str.saddr = result;
    }
  }
}

/* ======================================================================
   op_fldstor()  -  Store delimited substring in a dynamic array          */

void op_fldstor()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Replacement substring      |                             |
     |-----------------------------|-----------------------------| 
     |  Number of substrings       |                             |
     |-----------------------------|-----------------------------| 
     |  Occurrence number          |                             |
     |-----------------------------|-----------------------------| 
     |  Delimiter                  |                             |
     |-----------------------------|-----------------------------| 
     |  String to modify           |                             |
     |=============================|=============================|
 */
 
 DESCRIPTOR * descr;

 descr = e_stack - 5;
 do {descr = descr->data.d_addr;} while(descr->type == ADDR);
 op_fldstorf();
 k_release(descr);
 *descr = *(--e_stack);
}
/* ======================================================================
   op_fldstorf()  -  Store delimited substring in a dynamic array          */

void op_fldstorf()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Replacement substring      | Modified string             |
     |-----------------------------|-----------------------------| 
     |  Number of substrings       |                             |
     |-----------------------------|-----------------------------| 
     |  Occurrence number          |                             |
     |-----------------------------|-----------------------------| 
     |  Delimiter                  |                             |
     |-----------------------------|-----------------------------| 
     |  String to modify           |                             |
     |=============================|=============================|
 */

 DESCRIPTOR * descr;

 /* String to modify */
 STRING_CHUNK * src_str;            /* Current chunk */
 char * src;                        /* Character pointer into chunk */
 short int src_bytes_remaining;     /* Bytes left to process in current chunk */

 char delimiter;                   /* Substring delimiter character */

 long int occurrence;               /* Substring occurrence to replace */

 long int no_of_substrings;         /* Number of substrings to replace */

 /* Replacement string */
 STRING_CHUNK * rep_str;            /* Current chunk */
 char * rep;                        /* Character pointer into chunk */
 short int rep_bytes_remaining;     /* Bytes left to process in current chunk */

 DESCRIPTOR tgt_descr;              /* Target string */

 bool insert_all = FALSE;           /* No of substrings negative? */
 long int skip_count;               /* Number of substrings to skip */
 bool nocase;
 short int n;
 char s[2];
 char * p;

 nocase = (process.program.flags & HDR_NOCASE) != 0;

 /* Get replacement substring */

 descr = e_stack - 1;
 k_get_string(descr);
 rep_str = descr->data.str.saddr;
 if (rep_str == NULL) rep_bytes_remaining = 0;
 else
  {
   rep_bytes_remaining = rep_str->bytes;
   rep = rep_str->data;
  }

 /* Get number of substrings */

 descr = e_stack - 2;
 GetInt(descr);
 no_of_substrings = descr->data.value;
 if (no_of_substrings <= 0)
  {
   no_of_substrings = -no_of_substrings;
   insert_all = TRUE;
  }

 /* Get occurrence */

 descr = e_stack - 3;
 GetInt(descr);
 occurrence = descr->data.value;
 if (occurrence < 1) occurrence = 1;

 /* Get delimiter */

 descr = e_stack - 4;
 if (k_get_c_string(descr, s, 1) == 0) k_error(sysmsg(1605));
 delimiter = s[0];

 /* Get string to be modified */

 descr = e_stack - 5;
 k_get_string(descr);
 src_str = descr->data.str.saddr;
 if (src_str == NULL) src_bytes_remaining = 0;
 else
  {
   src_bytes_remaining = src_str->bytes;
   src = src_str->data;
  }

 /* Create target string using descriptor in our C stack */

 tgt_descr.type = STRING;
 tgt_descr.flags = 0;
 tgt_descr.data.str.saddr = NULL;
 ts_init(&(tgt_descr.data.str.saddr), 64);

 /* Copy source string to target string up to desired position */

 skip_count = occurrence - 1;

 if (src_str != NULL)
  {
   while(skip_count)
    {
     if (src_bytes_remaining == 0) /* Need new source chunk */
      {
       if (src_str->next == NULL) break; /* End of source string */

       src_str = src_str->next;
       src_bytes_remaining = src_str->bytes;
       src = src_str->data;
      }

     if (nocase)
      {
       p = (char *)memichr(src, delimiter, src_bytes_remaining);
      }
     else
      {
       p = (char *)memchr(src, delimiter, src_bytes_remaining);
      }
     if (p == NULL)
      {
       /* No delimiter in this chunk */
       n = src_bytes_remaining;
      }
     else     /* Delimiter found */
      {
       n = p - src + 1;
       skip_count--;
      }

     ts_copy(src, n);
     src += n;
     src_bytes_remaining -= n;
    }
  }

 /* Add trailing delimiters to target string */

 if (skip_count) ts_fill(delimiter, skip_count);

 /* Remove source substrings, if necessary */

 if (src_str != NULL)
  {
   skip_count = no_of_substrings;
   while(skip_count)
    {
     if (src_bytes_remaining == 0) /* Need new source chunk */
      {
       if (src_str->next == NULL) break; /* End of source string */

       src_str = src_str->next;
       src_bytes_remaining = src_str->bytes;
       src = src_str->data;
      }

     if (nocase)
      {
       p = (char *)memichr(src, delimiter, src_bytes_remaining);
      }
     else
      {
       p = (char *)memchr(src, delimiter, src_bytes_remaining);
      }

     if (p == NULL)
      {
       /* No delimiter in this chunk */
       src_bytes_remaining = 0;
      }
     else     /* Delimiter found */
      {
       n = p - src + 1;
       src += n;
       src_bytes_remaining -= n;
       skip_count--;
      }
    }
  }

 /* Insert replacement substring(s) */
 
 if (insert_all)  /* Insert entire replacement regardless of substring count */
  {
   while(rep_str != NULL)
    {
     ts_copy(rep_str->data, rep_str->bytes);
     rep_str = rep_str->next;
    }
  }
 else /* Insert no_of_substrings components of replacement substring */
  {
   if (rep_str != NULL)
    {
     while(no_of_substrings)
      {
       if (rep_bytes_remaining == 0) /* Need new source chunk */
        {
         rep_str = rep_str->next;
         if (rep_str == NULL) break;  /* End of replacement string */

         rep_bytes_remaining = rep_str->bytes;
         rep = rep_str->data;
        }

       if (nocase)
        {
         p = (char *)memichr(rep, delimiter, rep_bytes_remaining);
        }
       else
        {
         p = (char *)memchr(rep, delimiter, rep_bytes_remaining);
        }

       if (p == NULL)
        {
         /* No delimiter in this chunk */
         n = rep_bytes_remaining;
        }
       else     /* Delimiter found */
        {
         n = p - rep;
         if (--no_of_substrings) n++; /* Copy delimiter except in final substring */
        }

       ts_copy(rep, n);
       rep += n;
       rep_bytes_remaining -= n;
      }
    }

   /* Add delimiters to target string */
  
   if (--no_of_substrings > 0) ts_fill(delimiter, no_of_substrings);
  }

 /* Copy remainder of source string */

 if (src_str != NULL)
  {
   if (src_bytes_remaining || (src_str->next != NULL))
    {
     ts_copy_byte(delimiter);

     do {
         ts_copy(src, src_bytes_remaining);

         if ((src_str = src_str->next) == NULL) break;

         src_bytes_remaining = src_str->bytes;
         src = src_str->data;
        } while(1);
    }
  }

 /* Dismiss e_stack items */

 k_dismiss();       /* Replacement substring */
 k_pop(2);          /* Occurrence, number of substrings */
 k_dismiss();       /* Delimiter */
 k_dismiss();       /* String to modify */

 /* Terminate target string and place on e-stack */

 ts_terminate();
 *(e_stack++) = tgt_descr;
}

/* ======================================================================
   op_formcsv()  -  FORMCSV  -  Convert a string to CSV item format       */

void op_formcsv()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Source item                | CSV compliant string        |
     |=============================|=============================|
 */

 k_recurse(pcode_formcsv, 1);
}

/* ======================================================================
   op_getrem()  -  Return remove pointer offset                           */

void op_getrem()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  ADDR to string             | Offset                      |
     |=============================|=============================|
 */

 DESCRIPTOR * descr;
 long int offset = 0;
 STRING_CHUNK * src;
 STRING_CHUNK * current_chunk;

 descr = e_stack - 1;
 while(descr->type == ADDR) descr = descr->data.d_addr;

 switch(descr->type)
  {
   case STRING:
      if (descr->flags & DF_REMOVE)
       {
        current_chunk = descr->data.str.rmv_saddr;
        for(src = descr->data.str.saddr; src != current_chunk;
            src = src->next)
         {
          if (src == NULL)
           {
            k_error("Internal error : Remove pointer not in string");
           }
          offset += src->bytes;
         }
        offset += descr->n1;
       }
      break;

   case INTEGER:
   case FLOATNUM:
   case UNASSIGNED:
      break;

   default:
      k_error(sysmsg(1203));
  }

 k_dismiss();
 InitDescr(e_stack, INTEGER);
 (e_stack++)->data.value = offset;
}

/* ======================================================================
   op_listindx()  -  LISTINDEX() function                                 */

void op_listindx()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Item to find               |  Index, zero if not found   |
     |-----------------------------|-----------------------------|
     |  Delimiter                  |                             |
     |-----------------------------|-----------------------------|
     |  String to search           |                             |
     |=============================|=============================|
 */

 DESCRIPTOR * descr;

 STRING_CHUNK * list_str;    /* List to search */
 short int list_bytes;       /* Byte left in current chunk */

 STRING_CHUNK * item_string; /* Item to find - Start pointer*/
 STRING_CHUNK * item_str;    /* Item to find - Running pointer */
 short int item_bytes;       /* Byte left in current chunk */

 char delim;                 /* Delimiter */

 STRING_CHUNK * str;
 int pos = 0;
 char * p;
 char * q;
 char ci;
 char cl;

 descr = e_stack - 1;
 k_get_string(descr);
 item_string = descr->data.str.saddr;

 descr = e_stack - 2;
 k_get_string(descr);
 str = descr->data.str.saddr;
 if (str == NULL) goto exit_op_listindx;
 delim = str->data[0];

 descr = e_stack - 3;
 k_get_string(descr);
 list_str = descr->data.str.saddr;
 if (list_str == NULL)
  {
   pos = (item_string == NULL);  /* 1 if finding null in null, else 0 */
   goto exit_op_listindx;
  }

 pos = 1;

 item_str = item_string;
 if (item_str == NULL)   /* 0260 */
  {
   item_bytes = 0;
  }
 else
  {
   q = item_str->data;
   item_bytes = item_str->bytes;
  }

 while(list_str != NULL)
  {
   list_bytes = list_str->bytes;
   p = list_str->data;

   while(list_bytes--)
    {
     /* Get char from list */

     cl = *(p++);

     /* Get char from item */

     if (item_bytes)
      {
       ci = *(q++);

       if (item_bytes-- == 0)
        {
         item_str = item_str->next;
         if (item_str != NULL)
          {
           q = item_str->data;
           item_bytes = item_str->bytes - 1;
          }
        }
      }
     else ci = delim;

     if (cl == delim)   /* Delimiter */
      {
       if (ci == delim) goto exit_op_listindx; /* Found it */

       pos++;
      }
     else if (cl == ci)     /* Match */
      {
       continue;
      }
     else                   /* Mismatch */
      {
       /* Skip to delimiter in list */

       do {
           while(list_bytes--)
            {
             if (*(p++) == delim) goto realigned;
            }

           list_str = list_str->next;
           if (list_str == NULL)
            {
             pos = 0;
             goto exit_op_listindx;
            }

           list_bytes = list_str->bytes;
           p = list_str->data;
          } while(1);
realigned:
       pos++;
      }

     /* Reset item string */

     item_str = item_string;
     if (item_str == NULL) item_bytes = 0;   /* 0260 */
     else
      {
       q = item_str->data;
       item_bytes = item_str->bytes;
      }
    }

   list_str = list_str->next;
  }

 if (item_bytes == 0) goto exit_op_listindx; /* Found it */

 pos = 0;   /* Not found */

exit_op_listindx:
 k_dismiss();
 k_dismiss();
 k_dismiss();

 InitDescr(e_stack, INTEGER);
 (e_stack++)->data.value = pos;
}

/* ======================================================================
   op_ismv()  -  ISMV() function                                          */

void op_ismv()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Source string              |  Boolean                    |
     |=============================|=============================|
 */

 DESCRIPTOR * descr;

 STRING_CHUNK * str;
 short int bytes;
 char * p;
 int result = FALSE;


 /* Get string */
 
 descr = e_stack - 1;
 k_get_string(descr);

 for(str = descr->data.str.saddr; str != NULL; str = str->next)
  {
   for (p = str->data, bytes = str->bytes; bytes--; p++)
    {
     if (IsDelim(*p))
      {
       result = TRUE;
       goto exit_op_ismv;
      }
    }
  }

exit_op_ismv:
 k_dismiss();

 InitDescr(e_stack, INTEGER);
 (e_stack++)->data.value = result;
}

/* ======================================================================
   op_lower()  -  LOWER() function                                        */

void op_lower()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Source string              |  Result string              |
     |=============================|=============================|
 */

 DESCRIPTOR * descr;

 STRING_CHUNK * src_hdr;
 long int src_len;

 STRING_CHUNK * tgt_hdr;
 STRING_CHUNK * new_tgt;
 short int tgt_bytes_remaining;

 char * p;
 char * q;
 register char c;

 DESCRIPTOR result;
 long int total_len;


 /* Get source string */
 
 descr = e_stack - 1;
 k_get_string(descr);
 src_hdr = descr->data.str.saddr;

 result.type = STRING;
 result.flags = 0;
 result.data.str.saddr = NULL;

 if (src_hdr == NULL) goto done;
 
 total_len = src_hdr->string_len;

 /* Allocate initial target chunk */

 tgt_hdr = s_alloc(total_len, &tgt_bytes_remaining);
 result.data.str.saddr = tgt_hdr;
 tgt_hdr->ref_ct = 1;
 tgt_hdr->string_len = total_len;

 while(src_hdr != NULL)
  {
   src_len = src_hdr->bytes;
   p = src_hdr->data;
   q = tgt_hdr->data + tgt_hdr->bytes;

   if (src_len > tgt_bytes_remaining) /* Overflows current target chunk */
    {
     tgt_hdr->bytes += tgt_bytes_remaining;
     total_len -= tgt_bytes_remaining;
     src_len -= tgt_bytes_remaining;

     while(tgt_bytes_remaining-- > 0)
      {
       c = *(p++);
       if (IsMark(c) && (c != TEXT_MARK)) c--;
       *(q++) = c;
      }

     /* Allocate a new chunk */

     new_tgt = s_alloc(total_len, &tgt_bytes_remaining);
     tgt_hdr->next = new_tgt;
     tgt_hdr = new_tgt;
     q = tgt_hdr->data;
    }

   tgt_hdr->bytes += (short int)src_len;
   total_len -= src_len;
   tgt_bytes_remaining -= (short int)src_len;

   while(src_len-- > 0)
    {
     c = *(p++);
     if (IsMark(c) && (c != TEXT_MARK)) c--;
     *(q++) = c;
    }

   src_hdr = src_hdr->next;
  }

done:
 k_dismiss();

 *(e_stack++) = result;
 return;
}

/* ======================================================================
   op_quote()  -  QUOTE(), DQUOTE() and SQUOTE() functions                */

void op_quote() {quote('"');}
void op_squote() {quote('\'');}

Private void quote(char quote_char)
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Source string              |  Result string              |
     |=============================|=============================|
 */

 DESCRIPTOR * descr;
 STRING_CHUNK * src_hdr;
 DESCRIPTOR result;

 /* Get source string */
 
 descr = e_stack - 1;
 k_get_string(descr);
 src_hdr = descr->data.str.saddr;

 /* Create target string */

 result.type = STRING;
 result.flags = 0;
 result.data.str.saddr = NULL;

 if (src_hdr == NULL)
  {
   ts_init(&result.data.str.saddr, 2);
  }
 else
  {
   ts_init(&result.data.str.saddr, src_hdr->string_len + 2);
  }

 ts_copy_byte(quote_char);

 while(src_hdr != NULL)
  {
   ts_copy(src_hdr->data, src_hdr->bytes);
   src_hdr = src_hdr->next;
  }

 ts_copy_byte(quote_char);
 ts_terminate();


 k_dismiss();
 *(e_stack++) = result;
 return;
}
/* ======================================================================
   op_raise()  -  RAISE() function                                        */

void op_raise()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Source string              |  Result string              |
     |=============================|=============================|
 */

 DESCRIPTOR * descr;

 STRING_CHUNK * src_hdr;
 long int src_len;

 STRING_CHUNK * tgt_hdr;
 STRING_CHUNK * new_tgt;
 short int tgt_bytes_remaining;

 char * p;
 char * q;
 register char c;

 DESCRIPTOR result;
 long int total_len;


 /* Get source string */
 
 descr = e_stack - 1;
 k_get_string(descr);
 src_hdr = descr->data.str.saddr;

 result.type = STRING;
 result.flags = 0;
 result.data.str.saddr = NULL;

 if (src_hdr == NULL) goto done;

 total_len = src_hdr->string_len;

 /* Allocate initial target chunk */

 tgt_hdr = s_alloc(total_len, &tgt_bytes_remaining);
 result.data.str.saddr = tgt_hdr;
 tgt_hdr->ref_ct = 1;
 tgt_hdr->string_len = total_len;

 while(src_hdr != NULL)
  {
   src_len = src_hdr->bytes;
   p = src_hdr->data;
   q = tgt_hdr->data + tgt_hdr->bytes;

   if (src_len > tgt_bytes_remaining) /* Overflows current target chunk */
    {
     tgt_hdr->bytes += tgt_bytes_remaining;
     total_len -= tgt_bytes_remaining;
     src_len -= tgt_bytes_remaining;

     while(tgt_bytes_remaining-- > 0)
      {
       c = *(p++);
       if (IsMark(c) && (c != ITEM_MARK)) c++;
       *(q++) = c;
      }

     /* Allocate a new chunk */

     new_tgt = s_alloc(total_len, &tgt_bytes_remaining);
     tgt_hdr->next = new_tgt;
     tgt_hdr = new_tgt;
     q = tgt_hdr->data;
    }

   tgt_hdr->bytes += (short int)src_len;
   total_len -= src_len;
   tgt_bytes_remaining -= (short int)src_len;

   while(src_len-- > 0)
    {
     c = *(p++);
     if (IsMark(c) && (c != ITEM_MARK)) c++;
     *(q++) = c;
    }

   src_hdr = src_hdr->next;
  }

done:
 k_dismiss();

 *(e_stack++) = result;
 return;
}

/* ======================================================================
   op_setrem()  -  Set remove pointer                                     */

void op_setrem()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  ADDR to string             |                             |
     |-----------------------------|-----------------------------| 
     |  Remove offset              |                             |
     |=============================|=============================|
 */

 DESCRIPTOR * descr;
 long int offset;
 STRING_CHUNK * str;

 process.status = 0;
 
 descr = e_stack - 2;
 GetInt(descr);
 offset = descr->data.value;

 descr = e_stack - 1;
 while(descr->type == ADDR) descr = descr->data.d_addr;

 if (offset == 0)
  {
   descr->flags &= ~DF_REMOVE;
   goto exit_setrem;
  }

 k_get_string(descr); /* Ensure that it is a string */

 str = descr->data.str.saddr;
 if (str == NULL)
  {
   process.status = ER_LENGTH;
   goto exit_setrem;
  }

 /* Find chunk holding offset position */

 while(offset > str->bytes)
  {
   offset -= str->bytes;
   str = str->next;
   if (str == NULL)
    {
     process.status = ER_LENGTH;
     goto exit_setrem;
    }
  } 

 /* Set remove pointer */

 descr->flags |= DF_REMOVE;
 descr->n1 = (short int)offset;
 descr->data.str.rmv_saddr = str;

exit_setrem:
 k_dismiss();
 k_pop(1);
}

/* ======================================================================
   op_swapcase()  -  SWAPCASE() function                                  */

void op_swapcase()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Source string              | Result string               |
     |=============================|=============================|
 */
 
 DESCRIPTOR * descr;
 STRING_CHUNK * str;
 char * p;
 short int bytes;
 char c;

 descr = e_stack - 1;
 k_get_string(descr);

 str = descr->data.str.saddr;
 
 if (str != NULL)
  {
   if (str->ref_ct > 1)
    {
     str->ref_ct--;
     str = k_copy_string(descr->data.str.saddr);
     descr->data.str.saddr = str;
    } 

   /* Clear any hint */

   str->field = 0;

   descr->flags &= ~DF_CHANGE;

   do {
       p = str->data;
       bytes = str->bytes;
       while(bytes--)
        {
         c = *p;
         if (IsAlpha(c)) *(p++) = c ^ 0x20;
        }
      } while((str = str->next) != NULL);
 }
}

/* ======================================================================
   op_vslice()  -  VSLICE() function                                        */

void op_vslice()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Value position             |  Result string              |
     |-----------------------------|-----------------------------|
     |  Source string              |                             |
     |=============================|=============================|
 */

 DESCRIPTOR * descr;

 long int value;             /* Value to extract */
 long int v;                 /* Current value position */
 bool want_this_value;
 long int marks_pending = 0;

 STRING_CHUNK * src_hdr;
 int src_bytes_remaining;
 char * src;

 register char c;

 DESCRIPTOR result;


 /* Get value number */

 descr = e_stack - 1;
 GetInt(descr);
 value = descr->data.value;
 k_pop(1);

 if (value > 0)   /* If < 1, leave source as result */
  {
   /* Get source string */
 
   descr = e_stack - 1;
   k_get_string(descr);
   src_hdr = descr->data.str.saddr;

   if (src_hdr != NULL)    /* If null source, leave as result */
    {
     result.type = STRING;
     result.flags = 0;
     result.data.str.saddr = NULL;
     v = 1;
     want_this_value = (value == 1);

     ts_init(&result.data.str.saddr, 64);

     do {
         src_bytes_remaining = src_hdr->bytes;
         src = src_hdr->data;

         while(src_bytes_remaining--)
          {
           c = *(src++);
           if (IsDelim(c))
            {
             switch(c)
              {
               case FIELD_MARK:
                  marks_pending++;
                  v = 1;
                  want_this_value = (value == 1);
                  break;

               case VALUE_MARK:
                  want_this_value = (++v == value);
                  break;

               default:
                  if (want_this_value) ts_copy_byte(c);
                  break;
              }
            }
           else
            {
             if (want_this_value)
              {
               while(marks_pending)
                {
                 ts_copy_byte(FIELD_MARK);
                 marks_pending--;
                }
               ts_copy_byte(c);
              }
            }
          }

         src_hdr = src_hdr->next;
        } while(src_hdr != NULL);

     ts_terminate();

     k_dismiss();
     *(e_stack++) = result;
    }
  }

 return;
}

/* ======================================================================
   convert()  -  Common path for op_convert() and op_fconvert()           */

Private STRING_CHUNK * convert()
{
 /* Stack:

     |=============================|
     |            BEFORE           |
     |=============================|
 top |  String to modify           |      Stack is unchanged
     |-----------------------------|
     |  Characters to replace      |
     |-----------------------------|
     |  Replacement characters     |
     |=============================|
 */

 DESCRIPTOR * descr;

 u_char map[256];               /* Translation table */
 u_char retain[256];            /* Retain this character? */

 u_char from_str[256+1];        /* Translation string... */
 short int from_len;            /* ...and length */
 u_char to_str[256+1];          /* Translation string... */
 short int to_len;              /* ...and length */

 STRING_CHUNK * src_str;
 short int src_bytes_remaining;
 u_char * src;

 STRING_CHUNK * tgt_str = NULL;

 register u_char c;
 short int i;
 short int j;
 bool nocase;


 nocase = (process.program.flags & HDR_NOCASE) != 0;

 /* Get source string */

 descr = e_stack - 1;
 k_get_string(descr);
 src_str = descr->data.str.saddr;

 if (src_str == NULL) return NULL;

 /* Build translation tables.
    The map table contains the character to substituted for each character
    of the ASCII set. We initialise it to perform a null substitution and
    then insert mapped characters.
    The retain table is zero if the character is to be removed from the
    result string and non-zero to retain it. Because we also need to
    track multiple definitions of a single mapped character, this table is
    initialised to 2 to mean "not seen this one yet".
    We cannot simplify this by walking backwards down the translation
    strings as this would cause problems with the length matching.         */

 for (i = 0; i < 256; i++) map[i] = (u_char)i;
 memset(retain, 2, 256);

 descr = e_stack - 2;
 to_len = k_get_c_string(descr, (char *)to_str, 256);

 descr = e_stack - 3;
 from_len = k_get_c_string(descr, (char *)from_str, 256);
 if (nocase) LowerCaseString((char *)from_str);

 for (j = 0; j < 2; j++)
  {
   for (i = 0; i < from_len; i++)
    {
     c = from_str[i];
     if (retain[c] == 2)    /* Not seen this character yet */
      {
       if ((retain[c] = (i < to_len)) != 0) map[c] = to_str[i];
      }
    }

   if (!nocase) break;

   UpperCaseString((char *)from_str);
  }

 /* Set up target string */

 ts_init(&tgt_str, src_str->string_len);

 /* Perform conversion */

 do {
     src = (u_char *)(src_str->data);
     src_bytes_remaining = src_str->bytes;

     while(src_bytes_remaining--)
      {
       c = *(src++);
       if (retain[c]) ts_copy_byte(map[c]);
      }

     /* Advance to next source chunk */

    } while((src_str = src_str->next) != NULL);

 ts_terminate();

 return tgt_str;
}

/* ======================================================================
   op_subst()  -  Change substrings                                      */

void op_subst()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Delimiter                  | Result string               |
     |-----------------------------|-----------------------------|
     |  New substring              |                             |
     |-----------------------------|-----------------------------|
     |  Old substring              |                             |
     |-----------------------------|-----------------------------|
     |  Source string              |                             |
     |=============================|=============================|
 */

 k_recurse(pcode_subst, 4); /* Execute recursive code */
}

/* END-CODE */
