/* OP_DIO4.C
 * Disk i/o opcodes (Select actions for DH and directory files)
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
 * 02 Mar 07  2.5-1 Added select_ftype handling.
 * 11 Dec 06  2.4-17 Added op_rdnxint().
 * 19 Jul 06  2.4-10 Various optimisations in op_readnext().
 * 04 May 06  2.4-4 0487 Do not rely on @SELECTED being an integer when
 *                  overwriting it.
 * 26 Apr 06  2.4-2 0481 Directory file select used short int record count.
 * 19 Dec 05  2.3-3 Added OptSelectKeepCase handling in dir_select().
 * 16 Dec 05  2.3-3 0441 Clear old list at start of op_select().
 * 13 May 05  2.1-15 Added error arg to s_make_contiguous().
 * 27 Dec 04  2.1-0 0293 SELECTV was leaving its operands on the stack.
 * 20 Sep 04  2.0-2 Use message handler.
 * 20 Sep 04  2.0-2 Use dynamic pcode loader.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 *  START-DESCRIPTION:
 *
 *  op_clearall        CLEARALL
 *  op_clrselect       CLEARSEL
 *  op_dellist         DELLIST
 *  op_formlist        FORMLIST
 *  op_getlist         GETLIST
 *  op_readlist        READLIST
 *  op_readnext        READNEXT
 *  op_savelist        SAVELIST
 *  op_select          SELECT
 *  op_selecte         SELECTE
 *  op_selectv         SELECTV
 *  op_sselect         SSELECT
 *  op_slctinfo        SLCTINFO
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */


#include "qm.h"
#include "header.h"
#include "keys.h"
#include "syscom.h"
#include "dh_int.h"
#include "options.h"

void init_record_cache(void);

void op_formlist(void);
void op_rmvf(void);
void op_stor(void);
void op_dcount(void);

Private void readnext(short int mode);
Private bool dir_select(FILE_VAR * fvar, short int list_no);

/* ====================================================================== */

bool dio_init()
{
 short int i;

 for (i = 0; i <= HIGH_SELECT; i++)
  {
   select_ftype[i] = SEL_NONE;
   select_file[i] = NULL;
   select_group[i] = 0;
  }

 init_record_cache();

 if (!dh_init()) return FALSE;

 return TRUE;
}

/* ======================================================================
   op_clearall()  -  Clear all user select lists                          */

void op_clearall()
{
 short int i;

 for (i = 0; i <= HIGH_USER_SELECT; i++)
  {
   clear_select(i);
  }
}

/* ======================================================================
   op_clrselect()  -  Clear select list                                   */

void op_clrselect()
{

 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Select list number         |                             |
     |=============================|=============================|
 */

 DESCRIPTOR * list_descr;
 short int list_no;                 /* Select list number */

 /* Get the select list number */

 list_descr = e_stack - 1;
 GetInt(list_descr);
 list_no = (short int)(list_descr->data.value);
 k_pop(1);

 if (InvalidSelectListNo(list_no)) k_select_range_error();

 clear_select(list_no);
}

/* ======================================================================
   op_dellist()  -  DELETELIST statement                                  */

void op_dellist()
{

 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  List name                  |                             |
     |=============================|=============================|

 */

 k_recurse(pcode_dellist, 1); /* Execute recursive code */
}

/* ======================================================================
   op_getlist()  -  GETLIST statement                                     */

void op_getlist()
{

 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Select list number         | Status 0=ok, >0 = ELSE      |
     |-----------------------------|-----------------------------| 
     |  List name                  |                             |
     |=============================|=============================|

 */

 DESCRIPTOR * descr;
 short int list_no;

 /* Validate select list number, leaving on e-stack */

 descr = e_stack - 1;
 GetInt(descr);
 list_no = (short int)(descr->data.value);

 if (InvalidSelectListNo(list_no)) k_select_range_error();

 k_recurse(pcode_getlist, 2); /* Execute recursive code */
}

/* ======================================================================
   op_formlst()  -  Form select list from dynamic array                   */

void op_formlist()
{

 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Select list number         |                             |
     |-----------------------------|-----------------------------| 
     |  Source list                |                             |
     |=============================|=============================|

 */

 DESCRIPTOR * descr;
 short int list_no;


 /* Validate select list number, leaving on e-stack */

 descr = e_stack - 1;
 GetInt(descr);
 list_no = (short int)(descr->data.value);
 if (InvalidSelectListNo(list_no)) k_select_range_error();

 end_select(list_no);   /* Terminate any partial select */

 k_recurse(pcode_formlst, 2);
}

/* ======================================================================
   op_readlist()  -  Read select list to dynamic array                    */

void op_readlist()
{

 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Select list number         | Status 0=ok, >0 = ELSE      |
     |-----------------------------|-----------------------------| 
     |  ADDR to target             |                             |
     |=============================|=============================|

 */

 DESCRIPTOR * descr;
 short int list_no;
 DESCRIPTOR status_descr;


 /* Validate select list number, leaving on e-stack */

 descr = e_stack - 1;
 GetInt(descr);
 list_no = (short int)(descr->data.value);

 if (InvalidSelectListNo(list_no)) k_select_range_error();

 complete_select(list_no);  /* Complete any partial select */

 /* Push ADDR to status-descr onto e-stack */

 InitDescr(&status_descr, UNASSIGNED);
 InitDescr(e_stack, ADDR);
 (e_stack++)->data.d_addr = &status_descr;

 k_recurse(pcode_readlst, 3); /* Execute recursive code */

 /* Set status code on stack */

 *(e_stack++) = status_descr;
}

/* ======================================================================
   op_rdnxpos()   Standard READNEXT, returning composite exploded list
   op_readnext()  Standard READNEXT, returning id only in exploded list
   op_rdnxexp()   Extended READNEXT, returning positional data            */

void op_rdnxpos()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Select list number         | Status 0=ok, >0 = ELSE      |
     |                             |        <0 = ON ERROR        |
     |-----------------------------|-----------------------------| 
     |  ADDR to id target          |                             |
     |=============================|=============================|
                    
    ON ERROR codes are only returned if ONERROR opcode executed prior to
    call to READNEXT.
 */

 readnext(0);
}

void op_readnext()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Select list number         | Status 0=ok, >0 = ELSE      |
     |                             |        <0 = ON ERROR        |
     |-----------------------------|-----------------------------| 
     |  ADDR to id target          |                             |
     |=============================|=============================|
                    
    ON ERROR codes are only returned if ONERROR opcode executed prior to
    call to READNEXT.
 */

 readnext(1);
}

void op_rdnxexp()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Select list number or      | Status 0=ok, >0 = ELSE      |
     |  Pick style list variable   |        <0 = ON ERROR        |
     |-----------------------------|-----------------------------| 
     |  ADDR to subvalue target    |                             |
     |-----------------------------|-----------------------------| 
     |  ADDR to value target       |                             |
     |-----------------------------|-----------------------------| 
     |  ADDR to id target          |                             |
     |=============================|=============================|
                    
    ON ERROR codes are only returned if ONERROR opcode executed prior to
    call to READNEXT.
 */

 readnext(2);
}

void op_rdnxint()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Select list number or      | Status 0=ok, >0 = ELSE      |
     |  Pick style list variable   |        <0 = ON ERROR        |
     |-----------------------------|-----------------------------| 
     |  ADDR to additional target  |                             |
     |-----------------------------|-----------------------------| 
     |  ADDR to subvalue target    |                             |
     |-----------------------------|-----------------------------| 
     |  ADDR to value target       |                             |
     |-----------------------------|-----------------------------| 
     |  ADDR to id target          |                             |
     |=============================|=============================|
                    
    ON ERROR codes are only returned if ONERROR opcode executed prior to
    call to READNEXT.
 */

 readnext(3);
}

Private void readnext(short int mode)
                   /* 0 = Standard READNEXT, returns composite exploded data */
                   /* 1 = Standard READNEXT, returns id only */
                   /* 2 = Extended READNEXT, returns id and  positional data */
                   /* 3 = Extended READNEXT, Mode 2 + internal data */
{
 DESCRIPTOR * descr;
 DESCRIPTOR * d;
 DESCRIPTOR * select_list_descr;
 DESCRIPTOR * select_count_descr;
 DESCRIPTOR value_descr;
 DESCRIPTOR subvalue_descr;
 DESCRIPTOR int_descr;
 short int list_no;
 short int status;
 unsigned short int op_flags;
 long int count;
 char * vpos;
 short int id_bytes;
 short int bytes_remaining;
 int value = 0;
 int subvalue = 0;
 STRING_CHUNK * str;
 int saved_status;
 short int errnum;


 op_flags = process.op_flags;
 process.op_flags = 0;

 /* Save copies of positional descriptors for exploded list format. We
    need to do this so that these descriptors are not in the way when
    we store the id string by calling op_stor().
    The code below removes the two positional descriptors, leaving us with

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Select list number or      | Status 0=ok, >0 = ELSE      |
     |  Pick style list variable   |        <0 = ON ERROR        |
     |-----------------------------|-----------------------------| 
     |  ADDR to id target          |                             |
     |=============================|=============================|
 */

 switch(mode)
  {
   case 2:
      subvalue_descr = *(e_stack - 2);
      value_descr = *(e_stack - 3);
      *(e_stack - 3) = *(e_stack - 1);
      k_pop(2);
      break;

   case 3:
      int_descr = *(e_stack - 2);
      subvalue_descr = *(e_stack - 3);
      value_descr = *(e_stack - 4);
      *(e_stack - 4) = *(e_stack - 1);
      k_pop(3);
      break;
  }

 /* Get list number or Pick style list variable */

 descr = e_stack - 1;
 for(d = descr; d->type == ADDR; d = d->data.d_addr) {}
 if (d->type == SELLIST)
  {
   saved_status = process.status;

   /* Remove next field from the string */
   InitDescr(e_stack, INTEGER);
   (e_stack++)->data.value = 1;
   op_rmvf();
   status = (process.status != 0);
   process.status = saved_status;
  }
 else
  {
   GetInt(descr);
   list_no = (short int)(descr->data.value);
   k_pop(1);
   if (InvalidSelectListNo(list_no)) k_select_range_error();

   /* Find SELECT.COUNT(LIST.NO) in SYSCOM */

   select_count_descr = SelectCount(list_no);
   GetInt(select_count_descr);

   /* Find SELECT.LIST(LIST.NO) in SYSCOM */

   select_list_descr = SelectList(list_no);
   if (select_list_descr->type != STRING) k_get_string(select_list_descr);

   count = select_count_descr->data.value;

   if (count < 0) /* List is not active */
    {
     status = 1;                     /* Take ELSE clause */
     goto exit_readnext_null;
    }

   if (count == 0)  /* List is empty */
    {
     switch(select_ftype[list_no])
      {
       case SEL_DH:
          if (dh_select_group(((DH_FILE *)select_file[list_no]), list_no)) goto list_extended;
          break;
      }

     /* Nothing added to list. Release memory from previous list */

     k_release(select_list_descr);
     InitDescr(select_list_descr, STRING);
     select_list_descr->data.str.saddr = NULL;
     select_count_descr->data.value = -1;
     status = 1;           /* Take ELSE clause */
     goto exit_readnext_null;
    }

list_extended:

   /* List is active */

   (select_count_descr->data.value)--;

   InitDescr(e_stack, ADDR);
   (e_stack++)->data.d_addr = select_list_descr;

   /* Extract item from list */

   InitDescr(e_stack, INTEGER);
   (e_stack++)->data.value = 1;
   op_rmvf();
   status = 0;     /* Take THEN clause */
  }


 if (mode)
  {
   descr = e_stack - 1;
   str = descr->data.str.saddr;

   if (str != NULL)   /* 0162 */
    {
     /* The top item on the stack is a string. If we are going to do
        anything clever with it, we need it to be contiguous.         */

     if (str->next != NULL)
      {
       errnum = 0;
       str = s_make_contiguous(str, &errnum);
       descr->data.str.saddr = str;

       if (errnum)
        {
         process.status = errnum;
         status = -errnum;
         goto exit_readnext;
        }
      }

     vpos = (char *)memchr(str->data, VALUE_MARK, str->bytes);
     if (vpos)  /* Looks like an exploded list */
      {
       /* We need to chop off positional data. Rather than copying the
          id string in a truncated form, we simply adjust the byte count
          so that the positional stuff is never seen.                   */

       id_bytes = vpos - str->data;
       bytes_remaining = str->bytes - (id_bytes + 1);
       str->bytes = id_bytes;
       str->string_len = id_bytes;
      }
     else id_bytes = (short int)(str->string_len);     /* 0239 */

     if (mode >= 2)  /* Return positional data */
      {
       if (vpos++)
        {
         /* Extract value position */

         while(bytes_remaining && IsDigit(*vpos))
          {
           value = value * 10 + (*(vpos++) - '0');
           bytes_remaining--;
          }

         /* Extract subvalue position */

         if (*(vpos++) == VALUE_MARK)
          {
           bytes_remaining--;
           while(bytes_remaining && IsDigit(*vpos))
            {
             subvalue = subvalue * 10 + (*(vpos++) - '0');
             bytes_remaining--;
            }
          }

         /* Extract internal data */

         if (mode == 3)
          {
           if (*(vpos++) == VALUE_MARK)
            {
             bytes_remaining--;
             descr = &int_descr;
             while(descr->type == ADDR) descr = descr->data.d_addr;
             Release(descr);
             k_put_string(vpos, bytes_remaining, descr);
            }
          }
        }
      }

     if (id_bytes == 0)    /* 0239 */
      {
       /* This is clearly a duff select list item as QM doesn't support
          null record ids.  It can happen if a user writes a record with
          an id that starts with a value mark (naughty!). We need to
          dispose of the STRING_CHUNK and write a null string.          */

       s_free(descr->data.str.saddr);
       descr->data.str.saddr = NULL;
      }
    }
  }

 op_stor();      /* Store the id string */

 goto exit_readnext;

exit_readnext_null:

 /* Set target as null string */

 descr = e_stack - 1;
 do {
     descr = descr->data.d_addr;
    } while(descr->type == ADDR);
 k_release(descr);
 InitDescr(descr, STRING);
 descr->data.str.saddr = NULL;
 k_pop(1);


exit_readnext:
 if (mode >= 2)
  {
   /* Set value and subvalue positions to zero */

   descr = &subvalue_descr;    /* Subvalue position (optional) */
   if (descr->type != UNASSIGNED)
    {
     while(descr->type == ADDR) descr = descr->data.d_addr;
     Release(descr);
     InitDescr(descr, INTEGER);
     descr->data.value = subvalue;
    }

   descr = &value_descr;    /* Value position (mandatory) */
   while(descr->type == ADDR) descr = descr->data.d_addr;
   Release(descr);
   InitDescr(descr, INTEGER);
   descr->data.value = value;
  }

 /* Set status code on stack */

 InitDescr(e_stack, INTEGER);
 (e_stack++)->data.value = status;

 if ((status < 0) && !(op_flags & P_ON_ERROR))
  {
   k_error(sysmsg(1412), -status);
  }
}

/* ======================================================================
   op_savelist()  -  SAVELIST statement                                   */

void op_savelist()
{

 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Select list number         |  Status 0=ok, >0 = ELSE     |
     |-----------------------------|-----------------------------| 
     |  List name                  |                             |
     |=============================|=============================|

 */

 DESCRIPTOR * descr;
 short int list_no;

 /* Validate select list number, leaving on e-stack */

 descr = e_stack - 1;
 GetInt(descr);
 list_no = (short int)(descr->data.value);
 if (InvalidSelectListNo(list_no)) k_select_range_error();

 k_recurse(pcode_savelst, 2); /* Execute recursive code */
}

/* ======================================================================
   op_select()  -  Select records from file                               */

void op_select()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Select list number         | Status 0=ok, >0 = ELSE      |
     |                             |        <0 = ON ERROR        |
     |-----------------------------|-----------------------------| 
     |  File variable              |                             |
     |=============================|=============================|
                    
    ON ERROR codes are only returned if ONERROR opcode executed prior to
    call to SELECT.
 */

 unsigned short int op_flags;
 DESCRIPTOR * descr;
 DESCRIPTOR * list_descr;
 DESCRIPTOR * count_descr;
 short int list_no;
 FILE_VAR * fvar;
 long int record_count;


 op_flags = process.op_flags;
 process.op_flags = 0;

 process.status = 0;

 /* Validate select list number */

 descr = e_stack - 1;
 GetInt(descr);
 list_no = (short int)(descr->data.value);

 if (InvalidSelectListNo(list_no)) k_select_range_error();

 descr = e_stack - 2;
 k_get_value(descr);

 if (descr->type != FILE_REF)    /* 0208 */
  {
   op_formlist();
  }
 else
  {
   fvar = descr->data.fvar;

   end_select(list_no);
   clear_select(list_no);  /* 0441 */

   /* Do the select */

   switch(fvar->type)
    {
     case DYNAMIC_FILE:
        if (!dh_select(fvar->access.dh.dh_file, list_no))
         {
          process.status = ER_RNF;
         }
        break;

     case DIRECTORY_FILE:
        if (!dir_select(fvar, list_no)) process.status = ER_RNF;
        break;

     case NET_FILE:
        list_descr = SelectList(list_no);
        k_release(list_descr);
        InitDescr(list_descr, STRING);
        list_descr->data.str.saddr = NULL;

        count_descr = SelectCount(list_no);
        k_release(count_descr);
        InitDescr(count_descr, INTEGER);
        count_descr->data.value = 0;

        if (!net_select(fvar, &(list_descr->data.str.saddr),
                        &(count_descr->data.value)))
         {
          process.status = ER_RNF;
         }
        break;


     default:
        k_error(sysmsg(1413));
    }

   k_pop(1);
   k_dismiss();

   /* Copy selected record count to @SELECTED */

   /* Find SELECT.COUNT(LIST.NO) in SYSCOM */

   record_count = SelectCount(list_no)->data.value;

   /* Find @SELECTED in SYSCOM */

   descr = Element(process.syscom, SYSCOM_SELECTED);
   k_release(descr);              /* 0487 */
   InitDescr(descr, INTEGER);
   descr->data.value = record_count;
  }

 /* Set status code on stack */

 InitDescr(e_stack, INTEGER);
 (e_stack++)->data.value = process.status;

 if ((process.status < 0) && !(op_flags & P_ON_ERROR))
  {
   k_error(sysmsg(1414), -process.status);
  }
}

/* ======================================================================
   op_selecte()  -  Transfer list 0 to a select list variable             */

void op_selecte()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Select list variable       |                             |
     |=============================|=============================|
                    
 */

 DESCRIPTOR * src_descr;
 DESCRIPTOR * tgt_descr;
 STRING_CHUNK * src_hdr;
 STRING_CHUNK * src_str;
 STRING_CHUNK * tgt_str;
 short int offset;              /* Offset into current source chunk */
 short int bytes_remaining;


 /* Get target address */

 tgt_descr = e_stack - 1;
 while(tgt_descr->type == ADDR) tgt_descr = tgt_descr->data.d_addr;
 k_release(tgt_descr);
 k_pop(1);

 /* Transfer unprocessed items in select list 0 to the target variable */

 src_descr = SelectList(0);

 src_hdr = src_descr->data.str.saddr;
 if (src_hdr != NULL)
  {
   if (src_descr->flags & DF_REMOVE)  /* Remove pointer is active */
    {
     offset = src_descr->n1;
     src_str = src_descr->data.str.rmv_saddr;

     ts_init(&tgt_str, 128);
     do {
         bytes_remaining = src_str->bytes - offset;
         if (bytes_remaining > 0 ) ts_copy(src_str->data + offset, bytes_remaining);
         src_str = src_str->next;
         offset = 0;
        } while(src_str != NULL);

     (void)ts_terminate();

     if (--(src_hdr->ref_ct) == 0) s_free(src_hdr);
    }
   else /* No remove pointer - Simply reference whole string */
    {
     tgt_str = src_hdr;
    }
  }
 else   /* Null source list */
  {
   tgt_str = NULL;
  }

 InitDescr(tgt_descr, SELLIST);
 tgt_descr->data.str.saddr = tgt_str;

 /* Wipe out the original list 0 */

 src_descr->data.str.saddr = NULL;

 /* Copy remaining item count to offset field of SELLIST header and
    clear on list 0.                                                */

 src_descr = SelectCount(0);
 if (tgt_str != NULL) tgt_str->offset = src_descr->data.value;
 src_descr->data.value = 0;
}

/* ======================================================================
   op_selectv()  -  Select records from file to a SELLIST variable        */

void op_selectv()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Select list variable       | Status 0=ok, >0 = ELSE      |
     |                             |        <0 = ON ERROR        |
     |-----------------------------|-----------------------------| 
     |  File variable              |                             |
     |=============================|=============================|
                    
    ON ERROR codes are only returned if ONERROR opcode executed prior to
    call to SELECTV.

    NOTE: This opcode uses list 11 internally.
 */

 unsigned short int op_flags;
 DESCRIPTOR * descr;
 DESCRIPTOR * tgt_descr;
 FILE_VAR * fvar;
 STRING_CHUNK * tgt_str;
 STRING_CHUNK * str;
 long int record_count;
 DESCRIPTOR * list_descr;
 DESCRIPTOR * count_descr;
 char * p;
 short int bytes;

 op_flags = process.op_flags;
 process.op_flags = 0;

 process.status = 0;

 /* Get target address */

 tgt_descr = e_stack - 1;
 while(tgt_descr->type == ADDR) tgt_descr = tgt_descr->data.d_addr;
 k_release(tgt_descr);

 /* Get file variable / dynamic array */

 descr = e_stack - 2;
 k_get_value(descr);
 if (descr->type != FILE_REF)    /* 0208 */
  {
   k_get_string(descr);
   InitDescr(tgt_descr, SELLIST);

   /* Must copy the string rather than just incrementing the reference
      count as we are going to the use offset field of the header for
      the item count.                                                  */

   tgt_descr->data.str.saddr = k_copy_string(descr->data.str.saddr);
   tgt_str = tgt_descr->data.str.saddr;
   if ((str = tgt_str) != NULL)
    {
     record_count = 1;
     do {
         for(p = str->data, bytes = str->bytes; bytes--; p++)
          {
           if (*p == FIELD_MARK) record_count++;
          }
        } while ((str = str->next) != NULL);
     tgt_str->offset = record_count;
    }
  }
 else
  {
   fvar = descr->data.fvar;

   /* Do the select */

   switch(fvar->type)
    {
     case DYNAMIC_FILE:
        if (!dh_select(fvar->access.dh.dh_file, 11))
         {
          process.status = ER_RNF;
         }
        complete_select(11);  /* Complete select */
        break;

     case DIRECTORY_FILE:
        if (!dir_select(fvar, 11)) process.status = ER_RNF;
        break;

     case NET_FILE:
        list_descr = SelectList(11);
        k_release(list_descr);
        InitDescr(list_descr, STRING);
        list_descr->data.str.saddr = NULL;

        count_descr = SelectCount(11);
        k_release(count_descr);
        InitDescr(count_descr, INTEGER);
        count_descr->data.value = 0;

        if (!net_select(fvar, &(list_descr->data.str.saddr),
                        &(count_descr->data.value)))
         {
          process.status = ER_RNF;
         }
        break;

     default:
        k_error(sysmsg(1415));
    }

   /* Transfer the select list to the target variable */

   descr = SelectList(11);
   *tgt_descr = *descr;
   tgt_descr->type = SELLIST;
   tgt_str = tgt_descr->data.str.saddr;
   InitDescr(descr, STRING);
   descr->data.str.saddr = NULL;

   /* Copy selected record count to @SELECTED and SELLIST, then clear it */

   descr = SelectCount(11);
   record_count = descr->data.value;
   descr->data.value = 0;

   if (tgt_str != NULL) tgt_str->offset = record_count;

   descr = Element(process.syscom, SYSCOM_SELECTED);
   descr->data.value = record_count;
  }

 k_pop(1);      /* 0293 Moved to be in both processing paths */
 k_dismiss();

 /* Set status code on stack */

 InitDescr(e_stack, INTEGER);
 (e_stack++)->data.value = process.status;

 if ((process.status < 0) && !(op_flags & P_ON_ERROR))
  {
   k_error(sysmsg(1414), -process.status);
  }
}

/* ======================================================================
   op_sselect()  -  Select records from file, sorted by record id         */

void op_sselect()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Select list number         | Status 0=ok, >0 = ELSE      |
     |                             |        <0 = ON ERROR        |
     |-----------------------------|-----------------------------| 
     |  File variable              |                             |
     |=============================|=============================|
                    
    ON ERROR codes are only returned if ONERROR opcode executed prior to
    call to SELECT.
 */

 DESCRIPTOR * descr;
 short int list_no;


 process.op_flags = 0;  /* ON ERROR not used */

 process.status = 0;

 /* Validate select list number */

 descr = e_stack - 1;
 GetInt(descr);
 list_no = (short int)(descr->data.value);

 if (InvalidSelectListNo(list_no)) k_select_range_error();

 descr = e_stack - 2;
 k_get_value(descr);    /* 0208 */

 k_recurse(pcode_sselct, 2);
}

/* ======================================================================
   op_slctinfo()  -  SELECTINFO() function                                */

void op_slctinfo()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Key value                  | Result                      |
     |-----------------------------|-----------------------------| 
     |  Select list number         |                             |
     |=============================|=============================|

     Key values:       Function                      Returns       
     1    SL_ACTIVE    Test if list is active        True/False 
     3    SL_COUNT     Find size of list             No of items
 */  

 DESCRIPTOR * descr;
 DESCRIPTOR * select_count_descr;
 short int list_no;                 /* Select list number */
 long int key;


 /* Fetch key value */

 descr = e_stack - 1;
 GetInt(descr);
 key = descr->data.value;
 k_pop(1);

 /* Validate select list number, leaving on e-stack */

 descr = e_stack - 1;
 GetInt(descr);
 list_no = (short int)(descr->data.value);

 if (InvalidSelectListNo(list_no)) k_select_range_error();
 
 /* Find SELECT.COUNT(LIST.NO) in SYSCOM */

 select_count_descr = SelectCount(list_no);
 GetInt(select_count_descr);

 /* Process according to key value */

 descr = e_stack - 1;
 switch(key)
  {
   case 1:
      descr->data.value = (select_count_descr->data.value > 0);
      break;

   case 3:
      complete_select(list_no);  /* Complete any partial select */
      descr->data.value = max(0, select_count_descr->data.value);
      break;

   default:
      descr->data.value = 0;
      break;
  }
}

/* ======================================================================
   clear_select()  -  Clear a select list                                 */

void clear_select(short int list_no)
{
 DESCRIPTOR * tgt_descr;        /* Target select list descriptor */

 /* Find SELECT.LIST array in SYSCOM */

 tgt_descr = SelectList(list_no);
 
 /* Release current content and replace by null string */

 k_release(tgt_descr);
 InitDescr(tgt_descr, STRING);
 tgt_descr->data.str.saddr = NULL;

 /* Find SELECT.COUNT array in SYSCOM */

 tgt_descr = SelectCount(list_no);

 /* Check if this is an active partial select */

 end_select(list_no);

 /* Release current content and replace by -1 */

 k_release(tgt_descr);
 InitDescr(tgt_descr, INTEGER);
 tgt_descr->data.value = -1;
}

/* ======================================================================
   Perform select on directory file                                       */

Private bool dir_select(FILE_VAR * fvar, short int list_no)
{
 bool status = FALSE;
 FILE_ENTRY * fptr;
 char pathname[MAX_PATHNAME_LEN+1];
 int path_len;
 char name[MAX_PATHNAME_LEN+1];
 STRING_CHUNK * head = NULL;
 DESCRIPTOR * descr;
 int record_count = 0;    /* 0481 */
 char * p;
 char * q;
 char * r;
 char c;
 DIR * dfu;
 struct dirent * dp;
 struct stat statbuf;

 fptr = FPtr(fvar->file_id);
 strcpy(pathname, (char *)(fptr->pathname));
 path_len = strlen(pathname);

 if ((dfu = opendir(pathname)) != NULL)
  {
   if (pathname[path_len-1] == DS) pathname[path_len-1] = '\0';

   while((dp = readdir(dfu)) != NULL)
    {
     sprintf(name, "%s%c%s", pathname, DS, dp->d_name);
     if (stat(name, &statbuf)) continue;  /* 1.3-8  Carry on rather than end */

     if (statbuf.st_mode & S_IFREG)
      {
       strcpy(name, dp->d_name);
#ifdef CASE_INSENSITIVE_FILE_SYSTEM
       if (!Option(OptSelectKeepCase)) UpperCaseString(name);
#endif

       if (map_dir_ids && (strchr(name, '%') != NULL))   /* Mapped name */
        {
         p = name;
         q = name;

         if ((*p == '%') && (*(p+1) == 'D'))  /* Special case - first char only */
          {
           *(q++) = '.';
           p += 2;
          }
         else if ((*p == '%') && (*(p+1) == 'T'))  /* Special case - first char only */
          {
           *(q++) = '~';
           p += 2;
          }

         while((c = *(p++)) != '\0')
          {
           if (c == '%')
            {
             r = strchr(df_substitute_chars, *(p++));
             if (r != NULL)
              {
               *(q++) = df_restricted_chars[r - df_substitute_chars];
              }
            }
           else
            {
             *(q++) = c;
            }
          }
         *q = '\0';
        }

       if (head == NULL) ts_init(&head, 256);
       else ts_copy_byte(FIELD_MARK);

       ts_copy_c_string(name);
       record_count++;
      }
    }

   closedir(dfu);
  }

 /* Copy the record list (if any) to the select list */

 if (head != NULL)
  {
   ts_terminate();

   /* Find SELECT.COUNT(LIST.NO) in SYSCOM */

   descr = Element(process.syscom, SYSCOM_SELECT_COUNT);
   descr = Element(descr->data.ahdr_addr, list_no);
   k_release(descr);
   InitDescr(descr, INTEGER);
   descr->data.value = record_count;

   /* Find SELECT.LIST(LIST.NO) in SYSCOM */

   descr = Element(process.syscom, SYSCOM_SELECT_LIST);
   descr = Element(descr->data.ahdr_addr, list_no);
   k_release(descr);
   InitDescr(descr, STRING);
   descr->data.str.saddr = head;
  }

 status = TRUE;

 return status;
}

/* ======================================================================
   op_dir()  -  Return directory data                                     */

void op_dir()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Directory pathname         | Returned data               |
     |=============================|=============================|
                    
 */

 DESCRIPTOR * descr;
 char pathname[MAX_PATHNAME_LEN+1];
 short int path_len;
 char name[MAX_PATHNAME_LEN+1];
 STRING_CHUNK * result = NULL;
 char mode;
 DIR * dfu;
 struct dirent * dp;
 struct stat statbuf;

 ts_init(&result, 1024);

 descr = e_stack - 1;
 path_len = k_get_c_string(descr, pathname, MAX_PATHNAME_LEN);
 if (path_len > 0)
  {
   if ((dfu = opendir(pathname)) != NULL)
    {
     if (pathname[path_len-1] == DS) pathname[path_len-1] = '\0';

     while((dp = readdir(dfu)) != NULL)
      {
       if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) continue;

       sprintf(name, "%s%c%s", pathname, DS, dp->d_name);
       if (stat(name, &statbuf)) continue;

       strcpy(name, dp->d_name);
#ifdef CASE_INSENSITIVE_FILE_SYSTEM
       UpperCaseString(name);
#endif
       if (statbuf.st_mode & S_IFREG) mode = 'F';
       else if (statbuf.st_mode & S_IFDIR) mode = 'D';
       else continue;

       if (result != NULL) ts_copy_byte(FIELD_MARK);

       ts_copy_c_string(name);
       ts_copy_byte(VALUE_MARK);
       ts_copy_byte(mode);
      }
     closedir(dfu);
    }
  }

 ts_terminate();

 k_release(descr);
 InitDescr(descr, STRING);
 descr->data.str.saddr = result;
}

/* ======================================================================
   complete_select()  -  Complete a partial select                        */

void complete_select(short int list_no)
{
 switch(select_ftype[list_no])
  {
   case SEL_DH:
      dh_complete_select(list_no);
      break;
  }
}

/* ======================================================================
   end_select()  -  Terminate a partial select                           */

void end_select(short int list_no)
{
 switch(select_ftype[list_no])
  {
   case SEL_DH:
      dh_end_select(list_no);
      break;
  }
}

/* END-CODE */

