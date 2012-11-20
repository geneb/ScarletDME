/* OP_FIND.C
 * FIND and FINDSTR opcodes
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

/* ======================================================================
   op_find()  -  Locate item in dynamic array                             */

void op_find()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  ADDR to s or int zero      | Status 1=found, 0=not found |
     |-----------------------------|-----------------------------| 
     |  ADDR to v or int zero      |                             |
     |-----------------------------|-----------------------------| 
     |  ADDR to f                  |                             |
     |-----------------------------|-----------------------------| 
     |  Occurrence                 |                             |
     |-----------------------------|-----------------------------| 
     |  Source string              |                             |
     |-----------------------------|-----------------------------| 
     |  String to find             |                             |
     |=============================|=============================|

     Because the value and subvalue return variables are optional, the
     compiler loads either an ADDR to the variable or integer 0 if none.
 */

 DESCRIPTOR * src_descr;        /* Source string to search */
 STRING_CHUNK * src_hdr;
 short int src_bytes_remaining;
 char * src;

 DESCRIPTOR * srch_descr;       /* String to locate */
 STRING_CHUNK * srch_hdr;
 short int srch_bytes_remaining;
 char * srch;

 DESCRIPTOR * descr;
 long int field;                /* Current field... */
 long int value;                /* ...value... */
 long int subvalue;             /* ...and subvalue */

 long int occurrence;

 bool nocase;
 bool skipping = FALSE;
 char c;

 long int status = 0;


 nocase = (process.program.flags & HDR_NOCASE) != 0;

 /* Get occurrence */

 descr = e_stack - 4;
 GetInt(descr);
 occurrence = descr->data.value;

 /* Fetch source string to be searched */
 
 src_descr = e_stack - 5;
 k_get_string(src_descr);

 /* Fetch string to be located */
 
 srch_descr = e_stack - 6;
 k_get_string(srch_descr);

 if ((srch_hdr = srch_descr->data.str.saddr) != NULL)
  {
   srch_bytes_remaining = srch_hdr->bytes;
   srch = srch_hdr->data;
  }

 src_hdr = src_descr->data.str.saddr;
 if (src_hdr != NULL)
  {
   status = 1;
   field = 1; value = 1; subvalue = 1;  /* Current position */
   do {
       src_bytes_remaining = src_hdr->bytes;
       src = src_hdr->data;
       while(src_bytes_remaining--)
        {
         c = *(src++);
         if (IsDelim(c))
          {
           if (!skipping && (srch_hdr == NULL) && (--occurrence == 0)) goto found;
           skipping = FALSE;
           switch(c)
            {
             case FIELD_MARK:    field++;    value = 1; subvalue = 1; break;
             case VALUE_MARK:                value++;   subvalue = 1; break;
             case SUBVALUE_MARK:                        subvalue++;   break;
            }

           /* Reset search string pointer */

           if ((srch_hdr = srch_descr->data.str.saddr) != NULL)
            {
             srch_bytes_remaining = srch_hdr->bytes;
             srch = srch_hdr->data;
            }

           continue;
          }

         if (!skipping)
          {
           if ((srch_hdr != NULL)
               && ((nocase)?(UpperCase(c) == UpperCase(*(srch++)))
                           :(c == *(srch++))))
            {
             if (--srch_bytes_remaining == 0)
              {
               srch_hdr = srch_hdr->next;
               if (srch_hdr != NULL)
                {
                 srch_bytes_remaining = srch_hdr->bytes;
                 srch = srch_hdr->data;
                }
              }
            }
           else
            {
             skipping = TRUE;
             if ((srch_hdr = srch_descr->data.str.saddr) != NULL)
              {
               srch_bytes_remaining = srch_hdr->bytes;
               srch = srch_hdr->data;
              }
            }
          }
        }
      } while((src_hdr = src_hdr->next) != NULL);

   if (skipping || (srch_hdr != NULL) || --occurrence) status = 0;  /* Not found */

found:

   if (status)
    {
     /* Set result field, value and subvalue */

     descr = e_stack - 3;
     do {descr = descr->data.d_addr;} while(descr->type == ADDR);
     k_release(descr);
     InitDescr(descr, INTEGER);
     descr->data.value = field;

     descr = e_stack - 2;
     if (descr->type == ADDR)
      {
       do {descr = descr->data.d_addr;} while(descr->type == ADDR);
       k_release(descr);
       InitDescr(descr, INTEGER);
       descr->data.value = value;
      }

     descr = e_stack - 1;
     if (descr->type == ADDR)
      {
       do {descr = descr->data.d_addr;} while(descr->type == ADDR);
       k_release(descr);
       InitDescr(descr, INTEGER);
       descr->data.value = subvalue;
      }
    }
  }

 /* Remove arguments from stack */

 k_pop(4);
 k_dismiss();
 k_dismiss();

 /* Set status code on stack */

 InitDescr(e_stack, INTEGER);
 (e_stack++)->data.value = status;
}

/* ======================================================================
   op_findstr()  -  Locate item in dynamic array                             */

void op_findstr()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  ADDR to s or int zero      | Status 1=found, 0=not found |
     |-----------------------------|-----------------------------| 
     |  ADDR to v or int zero      |                             |
     |-----------------------------|-----------------------------| 
     |  ADDR to f                  |                             |
     |-----------------------------|-----------------------------| 
     |  Occurrence                 |                             |
     |-----------------------------|-----------------------------| 
     |  Source string              |                             |
     |-----------------------------|-----------------------------| 
     |  String to find             |                             |
     |=============================|=============================|

     Because the value and subvalue return variables are optional, the
     compiler loads either an ADDR to the variable or integer 0 if none.
 */

 DESCRIPTOR * src_descr;        /* Source string to search */
 STRING_CHUNK * src_hdr;
 short int src_bytes_remaining;
 char * src;

 DESCRIPTOR * srch_descr;       /* String to locate */
 STRING_CHUNK * srch_hdr;
 short int srch_bytes_remaining;
 char * srch;

 DESCRIPTOR * descr;
 long int field;                /* Current field... */
 long int value;                /* ...value... */
 long int subvalue;             /* ...and subvalue */

 bool nocase;
 long int occurrence;
 char c;
 bool m;

 long int status = 0;


 nocase = (process.program.flags & HDR_NOCASE) != 0;

 /* Get occurrence */

 descr = e_stack - 4;
 GetInt(descr);
 occurrence = descr->data.value;

 /* Fetch source string to be searched */
 
 src_descr = e_stack - 5;
 k_get_string(src_descr);
 if (src_descr->data.str.saddr == NULL) goto not_found;   /* 0247 */

 /* Fetch string to be located */
 
 srch_descr = e_stack - 6;
 k_get_string(srch_descr);

 status = 1;
 field = 1; value = 1; subvalue = 1;  /* Current position */

 if ((srch_hdr = srch_descr->data.str.saddr) == NULL)
  {
   goto found;
  }

 srch_bytes_remaining = srch_hdr->bytes;
 srch = srch_hdr->data;

 src_hdr = src_descr->data.str.saddr;
 if (src_hdr != NULL)
  {
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
             case FIELD_MARK:    field++;    value = 1; subvalue = 1; break;
             case VALUE_MARK:                value++;   subvalue = 1; break;
             case SUBVALUE_MARK:                        subvalue++;   break;
            }

           /* Reset search string pointer */

           srch_hdr = srch_descr->data.str.saddr;
           srch_bytes_remaining = srch_hdr->bytes;
           srch = srch_hdr->data;
          }
         else   /* Compare character */
          {
           if (nocase) m = (UpperCase(c) == UpperCase(*(srch++)));
           else m = (c == *(srch++));

           if (m)   /* Match */
            {
             if (--srch_bytes_remaining == 0)  /* End of search string chunk */
              {
               srch_hdr = srch_hdr->next;
               if (srch_hdr == NULL)           /* End of search string */
                {
                 if (--occurrence == 0) goto found;

                 srch_hdr = srch_descr->data.str.saddr;
                 srch_bytes_remaining = srch_hdr->bytes;
                 srch = srch_hdr->data;
                }
               else   /* Move to next chunk of string to find */
                {
                 srch_bytes_remaining = srch_hdr->bytes;
                 srch = srch_hdr->data;
                }
              }
            }
           else                   /* No match */
            {
             srch_hdr = srch_descr->data.str.saddr;
             srch_bytes_remaining = srch_hdr->bytes;
             srch = srch_hdr->data;
            }
          }
        }
      } while((src_hdr = src_hdr->next) != NULL);

not_found:
   status = 0;  /* Not found */

found:

   if (status)
    {
     /* Set result field, value and subvalue */

     descr = e_stack - 3;
     do {descr = descr->data.d_addr;} while(descr->type == ADDR);
     k_release(descr);
     InitDescr(descr, INTEGER);
     descr->data.value = field;

     descr = e_stack - 2;
     if (descr->type == ADDR)
      {
       do {descr = descr->data.d_addr;} while(descr->type == ADDR);
       k_release(descr);
       InitDescr(descr, INTEGER);
       descr->data.value = value;
      }

     descr = e_stack - 1;
     if (descr->type == ADDR)
      {
       do {descr = descr->data.d_addr;} while(descr->type == ADDR);
       k_release(descr);
       InitDescr(descr, INTEGER);
       descr->data.value = subvalue;
      }
    }
  }

 /* Remove arguments from stack */

 k_pop(4);
 k_dismiss();
 k_dismiss();

 /* Set status code on stack */

 InitDescr(e_stack, INTEGER);
 (e_stack++)->data.value = status;
}

/* END-CODE */

