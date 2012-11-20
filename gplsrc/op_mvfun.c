/* OP_MVFUN.C
 * Multi-value function opcodes
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
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 18 Dec 06  2.4-18 0532 All MV opcodes should do one cycle for null data.
 * 08 Nov 04  2.0-10 0283 RewroteIFS() to handle REUSE().
 * 20 Sep 04  2.0-2 Use dynamic pcode loader.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * Opcodes:
 *    op_ifs          IFS
 *    op_mvd          MVFUN(dyn.arr)
 *    op_mvdd         MVFUN(dyn.arr, dyn.arr)
 *    op_mvds         MVFUN(dyn.arr, scalar)
 *    op_mvdss        MVFUN(dyn.arr, scalar, scalar)
 *    op_mvdss        MVFUN(dyn.arr, scalar, scalar, scalar)
 *    op_substrng()   SUBSTRNG
 *
 *  END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include "header.h"

extern void (*dispatch[])(void);

void op_cat(void);

/* ======================================================================
   op_ifs()  -  IFS opcode                                                */

void op_ifs()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  String 2       (arg 3)     | Result                      |
     |-----------------------------|-----------------------------|
     |  String 1       (arg 2)     |                             |
     |-----------------------------|-----------------------------|
     |  Boolean array  (arg 1)     |                             |
     |=============================|=============================|
 */

 DESCRIPTOR * arg1;        /* arg1 source descriptor */
 bool reuse1;
 short int delimiter1 = 0; /* Last mark character */
 STRING_CHUNK * src1_str;  /* Current chunk */
 short int src1_offset = 0;
 char * src1;
 bool need1;
 
 DESCRIPTOR * arg2;       /* arg2 source descriptor */
 bool reuse2;
 short int delimiter2 = 0;      /* Last mark character */
 STRING_CHUNK * src2_str; /* Current chunk */
 short int src2_offset = 0;
 char * src2;
 bool need2;

 DESCRIPTOR * arg3;       /* arg3 source descriptor */
 bool reuse3;
 short int delimiter3 = 0;      /* Last mark character */
 STRING_CHUNK * src3_str; /* Current chunk */
 short int src3_offset = 0;
 char * src3;
 bool need3;

 DESCRIPTOR op_arg1;
 DESCRIPTOR op_arg2;
 DESCRIPTOR op_arg3;

 STRING_CHUNK * result_str;
 char result_delimiter;
 bool first = TRUE;

 register char c;
 short int n;

 /* Ensure that all three arguments are strings */

 arg3 = e_stack - 1;
 reuse3 = arg3->flags & DF_REUSE;
 k_get_string(arg3);
 src3_str = arg3->data.str.saddr;

 arg2 = e_stack - 2;
 reuse2 = arg2->flags & DF_REUSE;
 k_get_string(arg2);
 src2_str = arg2->data.str.saddr;

 arg1 = e_stack - 3;
 reuse1 = arg1->flags & DF_REUSE;
 k_get_string(arg1);
 src1_str = arg1->data.str.saddr;

 /* Make descriptors to store substrings */

 InitDescr(&op_arg1, UNASSIGNED);
 InitDescr(&op_arg2, UNASSIGNED);
 InitDescr(&op_arg3, UNASSIGNED);

 /* Place a null string on the stack to accumulate the result */

 InitDescr(e_stack, STRING);
 (e_stack++)->data.str.saddr = NULL;

 while((src1_str != NULL) || (src2_str != NULL) || (src3_str != NULL))
  {
   /* Determine which items we need to fetch from the source strings */

   need1 = (delimiter1 <= delimiter2) && (delimiter1 <= delimiter3);
   need2 = (delimiter2 <= delimiter1) && (delimiter2 <= delimiter3);
   need3 = (delimiter3 <= delimiter1) && (delimiter3 <= delimiter2);

   if (need1)   
    {
     /* Fetch an element of arg1 */

     k_release(&op_arg1);
     InitDescr(&op_arg1, STRING);
     op_arg1.data.str.saddr = NULL;

     delimiter1 = 256;  /* For end of string paths */
     ts_init(&op_arg1.data.str.saddr, 100);

     while(src1_str != NULL)
      {
       src1 = src1_str->data + src1_offset;
       while(src1_offset++ < src1_str->bytes)
        {
         c = *(src1++);
         if (IsMark(c))
          {
           delimiter1 = (u_char)c;
           goto end_item1;
          }
         ts_copy_byte(c);
        }
       src1_str = src1_str->next;
       src1_offset = 0;
      }
end_item1:
     ts_terminate();
    }
   else /* Reuse or insert default value for item 1 */
    {
     if (!reuse1)
      {
       k_release(&op_arg1);
       InitDescr(&op_arg1, STRING);
       op_arg1.data.str.saddr = NULL;
      }
    }

   if (need2)
    {
     /* Fetch an element of arg2 */

     k_release(&op_arg2);
     InitDescr(&op_arg2, STRING);
     op_arg2.data.str.saddr = NULL;

     delimiter2 = 256;   /* For end of string paths */
     ts_init(&op_arg2.data.str.saddr, 100);

     while(src2_str != NULL)
      {
       src2 = src2_str->data + src2_offset;
       while(src2_offset++ < src2_str->bytes)
        {
         c = *(src2++);
         if (IsMark(c) && ((u_char)c >= delimiter1))
          {
           delimiter2 = (u_char)c;
           goto end_item2;
          }
         ts_copy_byte(c);
        }
       src2_str = src2_str->next;
       src2_offset = 0;
      }
end_item2:
     ts_terminate();
    }
   else /* Reuse or insert default value for item 2 */
    {
     if (!reuse2)
      {
       k_release(&op_arg2);
       InitDescr(&op_arg2, STRING);
       op_arg2.data.str.saddr = NULL;
      }
    }

   if (need3)
    {
     /* Fetch an element of arg3 */

     k_release(&op_arg3);
     InitDescr(&op_arg3, STRING);
     op_arg3.data.str.saddr = NULL;

     delimiter3 = 256;   /* For end of string paths */
     ts_init(&op_arg3.data.str.saddr, 100);

     while(src3_str != NULL)
      {
       src3 = src3_str->data + src3_offset;
       while(src3_offset++ < src3_str->bytes)
        {
         c = *(src3++);
         if (IsMark(c) && ((u_char)c >= delimiter1))
          {
           delimiter3 = (u_char)c;
           goto end_item3;
          }
         ts_copy_byte(c);
        }
       src3_str = src3_str->next;
       src3_offset = 0;
      }
end_item3:
     ts_terminate();
    }
   else /* Reuse or insert default value for item 3 */
    {
     if (!reuse3)
      {
       k_release(&op_arg3);
       InitDescr(&op_arg3, STRING);
       op_arg3.data.str.saddr = NULL;
      }
    }

   /* Insert delimiter */

   if (!first)
    {
     InitDescr(e_stack, STRING);
     result_str = e_stack->data.str.saddr = s_alloc(1, &n);
     result_str->ref_ct = 1;
     result_str->string_len = 1;
     result_str->bytes = 1;
     result_str->data[0] = result_delimiter;
     e_stack++;

     op_cat();
    }

   result_delimiter = (char)min(min(delimiter1, delimiter2), delimiter3);

   /* Perform operation */

   k_get_bool(&op_arg1);
   InitDescr(e_stack, ADDR);
   (e_stack++)->data.d_addr = (op_arg1.data.value)?(&op_arg2):(&op_arg3);

   /* Save result */

   op_cat();

   first = FALSE;
  }

 k_release(&op_arg1);
 k_release(&op_arg2);
 k_release(&op_arg3);

 /* The stack currently has:
     Result
     String 3
     String 2
     String 1
   We need to remove the three source strings */

 k_release(arg1);
 k_release(arg2);
 k_release(arg3);
 *(e_stack - 4) = *(e_stack - 1);
 e_stack -= 3;
}

/* ======================================================================
   op_mvd()  -  MVD opcode                                                */

void op_mvd()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  String                     | Result                      |
     |=============================|=============================|
 */

 u_char opcode;

 DESCRIPTOR * arg;         /* Source descriptor */
 short int delimiter = 0;  /* Last mark character */
 STRING_CHUNK * src_str;   /* Current chunk */
 short int src_offset = 0;
 char * src;
 
 DESCRIPTOR op_arg;

 STRING_CHUNK * result_str;
 char result_delimiter;
 bool first = TRUE;

 register char c;
 short int n;


 /* Get opcode to execute for each element */

 opcode = *(pc++);

 /* Ensure that argument is a  string */

 arg = e_stack - 1;
 k_get_string(arg);
 src_str = arg->data.str.saddr;

 /* Make descriptor to store substrings */

 InitDescr(&op_arg, UNASSIGNED);

 /* Place a null string on the stack to accumulate the result */

 InitDescr(e_stack, STRING);
 (e_stack++)->data.str.saddr = NULL;

 do {
     /* Fetch an element of source string */

     k_release(&op_arg);
     InitDescr(&op_arg, STRING);
     op_arg.data.str.saddr = NULL;

     delimiter = 256;  /* For end of string paths */
     ts_init(&op_arg.data.str.saddr, 100);

     while(src_str != NULL)
      {
       src = src_str->data + src_offset;
       while(src_offset++ < src_str->bytes)
        {
         c = *(src++);
         if (IsMark(c))
          {
           delimiter = (u_char)c;
           goto end_item;
          }
         ts_copy_byte(c);
        }
       src_str = src_str->next;
       src_offset = 0;
      }
end_item:
     ts_terminate();

     /* Insert delimiter */

     if (!first)
      {
       InitDescr(e_stack, STRING);
       result_str = e_stack->data.str.saddr = s_alloc(1, &n);
       result_str->ref_ct = 1;
       result_str->string_len = 1;
       result_str->bytes = 1;
       result_str->data[0] = result_delimiter;
       e_stack++;

       op_cat();
      }

     result_delimiter = (char)delimiter;

     /* Perform operation */

     InitDescr(e_stack, ADDR);
     (e_stack++)->data.d_addr = &op_arg;

     dispatch[opcode]();

     /* Save result */

     op_cat();

     first = FALSE;
    } while(src_str != NULL);


 k_release(&op_arg);

 /* The stack currently has:
     Result
     String
   We need to remove the source string */

 k_release(arg);
 *(e_stack - 2) = *(e_stack - 1);
 e_stack--;
}

/* ======================================================================
   op_mvdd()  -  MVDD opcode                                              */

void op_mvdd()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  String 2                   | Result                      |
     |-----------------------------|-----------------------------|
     |  String 1                   |                             |
     |=============================|=============================|
 */

 u_char opcode;

 DESCRIPTOR * arg1;        /* arg1 source descriptor */
 bool reuse1;
 short int delimiter1 = 0; /* Last mark character */
 STRING_CHUNK * src1_str;  /* Current chunk */
 short int src1_offset = 0;
 char * src1;
 bool need1;
 
 DESCRIPTOR * arg2;       /* arg2 source descriptor */
 bool reuse2;
 short int delimiter2 = 0;      /* Last mark character */
 STRING_CHUNK * src2_str; /* Current chunk */
 short int src2_offset = 0;
 char * src2;
 bool need2;

 DESCRIPTOR op_arg1;
 DESCRIPTOR op_arg2;

 STRING_CHUNK * result_str;
 char result_delimiter;
 bool first = TRUE;

 register char c;
 short int n;


 /* Get opcode to execute for each element */

 opcode = *(pc++);

 /* Ensure that both arguments are strings */

 arg2 = e_stack - 1;
 reuse2 = arg2->flags & DF_REUSE;
 k_get_string(arg2);
 src2_str = arg2->data.str.saddr;

 arg1 = e_stack - 2;
 reuse1 = arg1->flags & DF_REUSE;
 k_get_string(arg1);
 src1_str = arg1->data.str.saddr;

 /* Make descriptors to store substrings */

 InitDescr(&op_arg1, UNASSIGNED);
 InitDescr(&op_arg2, UNASSIGNED);

 /* Place a null string on the stack to accumulate the result */

 InitDescr(e_stack, STRING);
 (e_stack++)->data.str.saddr = NULL;

 do {
     /* Determine which items we need to fetch from the source strings */

     need1 = (delimiter1 <= delimiter2);
     need2 = (delimiter1 >= delimiter2);

     if (need1)   
      {
       /* Fetch an element of arg1 */

       k_release(&op_arg1);
       InitDescr(&op_arg1, STRING);
       op_arg1.data.str.saddr = NULL;

       delimiter1 = 256;  /* For end of string paths */
       ts_init(&op_arg1.data.str.saddr, 100);

       while(src1_str != NULL)
        {
         src1 = src1_str->data + src1_offset;
         while(src1_offset++ < src1_str->bytes)
          {
           c = *(src1++);
           if (IsMark(c))
            {
             delimiter1 = (u_char)c;
             goto end_item1;
            }
           ts_copy_byte(c);
          }
         src1_str = src1_str->next;
         src1_offset = 0;
        }
end_item1:
       ts_terminate();
      }
     else /* Reuse or insert default value for item 1 */
      {
       if (!reuse1)
        {
         k_release(&op_arg1);
         InitDescr(&op_arg1, STRING);
         op_arg1.data.str.saddr = NULL;
        }
      }

     if (need2)
      {
       /* Fetch an element of arg2 */

       k_release(&op_arg2);
       InitDescr(&op_arg2, STRING);
       op_arg2.data.str.saddr = NULL;

       delimiter2 = 256;   /* For end of string paths */
       ts_init(&op_arg2.data.str.saddr, 100);

       while(src2_str != NULL)
        {
         src2 = src2_str->data + src2_offset;
         while(src2_offset++ < src2_str->bytes)
          {
           c = *(src2++);
           if (IsMark(c))
            {
             delimiter2 = (u_char)c;
             goto end_item2;
            }
           ts_copy_byte(c);
          }
         src2_str = src2_str->next;
         src2_offset = 0;
        }
end_item2:
       ts_terminate();
      }
     else /* Reuse or insert default value for item 2 */
      {
       if (!reuse2)
        {
         k_release(&op_arg2);
         InitDescr(&op_arg2, STRING);
         op_arg2.data.str.saddr = NULL;
        }
      }

     /* Insert delimiter */

     if (!first)
      {
       InitDescr(e_stack, STRING);
       result_str = e_stack->data.str.saddr = s_alloc(1, &n);
       result_str->ref_ct = 1;
       result_str->string_len = 1;
       result_str->bytes = 1;
       result_str->data[0] = result_delimiter;
       e_stack++;

       op_cat();
      }

     result_delimiter = (char)min(delimiter1, delimiter2);

     /* Perform operation */

     InitDescr(e_stack, ADDR);
     (e_stack++)->data.d_addr = &op_arg1;
     InitDescr(e_stack, ADDR);
     (e_stack++)->data.d_addr = &op_arg2;

     dispatch[opcode]();

     /* Save result */

     op_cat();

     first = FALSE;
    } while((src1_str != NULL) || (src2_str != NULL));

 k_release(&op_arg1);
 k_release(&op_arg2);

 /* The stack currently has:
     Result
     String 2
     String 1
   We need to remove the two source strings */

 k_release(arg1);
 k_release(arg2);
 *(e_stack - 3) = *(e_stack - 1);
 e_stack -= 2;
}

/* ======================================================================
   op_mvds()  -  MVDS opcode                                              */

void op_mvds()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  String 2                   | Result                      |
     |-----------------------------|-----------------------------|
     |  Dynamic array              |                             |
     |=============================|=============================|
 */

 u_char opcode;

 DESCRIPTOR * arg1;        /* arg1 source descriptor */
 short int delimiter = 0;  /* Last mark character */
 STRING_CHUNK * src1_str;  /* Current chunk */
 short int src1_offset = 0;
 char * src1;
 
 DESCRIPTOR * arg2;       /* arg2 source descriptor */

 DESCRIPTOR op_arg1;

 STRING_CHUNK * result_str;
 char result_delimiter;
 bool first = TRUE;

 register char c;
 short int n;


 /* Get opcode to execute for each element */

 opcode = *(pc++);

 /* Ensure that both arguments are strings */

 arg2 = e_stack - 1;
 k_get_string(arg2);

 arg1 = e_stack - 2;
 k_get_string(arg1);
 src1_str = arg1->data.str.saddr;

 /* Make descriptors to store substring */

 InitDescr(&op_arg1, UNASSIGNED);

 /* Place a null string on the stack to accumulate the result */

 InitDescr(e_stack, STRING);
 (e_stack++)->data.str.saddr = NULL;

 do {
     k_release(&op_arg1);
     InitDescr(&op_arg1, STRING);
     op_arg1.data.str.saddr = NULL;

     delimiter = 256;  /* For end of string paths */
     ts_init(&op_arg1.data.str.saddr, 100);

     while(src1_str != NULL)
      {
       src1 = src1_str->data + src1_offset;
       while(src1_offset++ < src1_str->bytes)
        {
         c = *(src1++);
         if (IsMark(c))
          {
           delimiter = (u_char)c;
           goto end_item;
          }
         ts_copy_byte(c);
        }
       src1_str = src1_str->next;
       src1_offset = 0;
      }
end_item:
     ts_terminate();

     /* Insert delimiter */

     if (!first)
      {
       InitDescr(e_stack, STRING);
       result_str = e_stack->data.str.saddr = s_alloc(1, &n);
       result_str->ref_ct = 1;
       result_str->string_len = 1;
       result_str->bytes = 1;
       result_str->data[0] = result_delimiter;
       e_stack++;

       op_cat();
      }

     result_delimiter = (char)delimiter;

     /* Perform operation */

     InitDescr(e_stack, ADDR);         /* Substring */
     (e_stack++)->data.d_addr = &op_arg1;

     InitDescr(e_stack, ADDR);         /* Copy of second argument */
     (e_stack++)->data.d_addr = arg2;

     dispatch[opcode]();

     /* Save result */

     op_cat();

     first = FALSE;
    } while(src1_str != NULL);

 k_release(&op_arg1);

 /* The stack currently has:
     Result
     String 2
     String 1
   We need to remove the two source strings */

 k_release(arg1);
 k_release(arg2);
 *(e_stack - 3) = *(e_stack - 1);
 e_stack -= 2;
}

/* ======================================================================
   op_mvdss()  -  MVDSS opcode                                            */

void op_mvdss()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  String 3                   | Result                      |
     |-----------------------------|-----------------------------|
     |  String 2                   |                             |
     |-----------------------------|-----------------------------|
     |  Dynamic array              |                             |
     |=============================|=============================|
 */

 u_char opcode;

 DESCRIPTOR * arg1;        /* arg1 source descriptor */
 short int delimiter = 0;  /* Last mark character */
 STRING_CHUNK * src1_str;  /* Current chunk */
 short int src1_offset = 0;
 char * src1;
 
 DESCRIPTOR * arg2;       /* arg2 source descriptor */
 DESCRIPTOR * arg3;       /* arg3 source descriptor */

 DESCRIPTOR op_arg1;

 STRING_CHUNK * result_str;
 char result_delimiter;
 bool first = TRUE;

 register char c;
 short int n;


 /* Get opcode to execute for each element */

 opcode = *(pc++);

 /* Ensure that all arguments are strings */

 arg3 = e_stack - 1;
 k_get_string(arg3);

 arg2 = e_stack - 2;
 k_get_string(arg2);

 arg1 = e_stack - 3;
 k_get_string(arg1);
 src1_str = arg1->data.str.saddr;

 /* Make descriptors to store substring */

 InitDescr(&op_arg1, UNASSIGNED);

 /* Place a null string on the stack to accumulate the result */

 InitDescr(e_stack, STRING);
 (e_stack++)->data.str.saddr = NULL;

 do {
     k_release(&op_arg1);
     InitDescr(&op_arg1, STRING);
     op_arg1.data.str.saddr = NULL;

     delimiter = 256;  /* For end of string paths */
     ts_init(&op_arg1.data.str.saddr, 100);

     while(src1_str != NULL)
      {
       src1 = src1_str->data + src1_offset;
       while(src1_offset++ < src1_str->bytes)
        {
         c = *(src1++);
         if (IsMark(c))
          {
           delimiter = (u_char)c;
           goto end_item;
          }
         ts_copy_byte(c);
        }
       src1_str = src1_str->next;
       src1_offset = 0;
      }
end_item:
     ts_terminate();

     /* Insert delimiter */

     if (!first)
      {
       InitDescr(e_stack, STRING);
       result_str = e_stack->data.str.saddr = s_alloc(1, &n);
       result_str->ref_ct = 1;
       result_str->string_len = 1;
       result_str->bytes = 1;
       result_str->data[0] = result_delimiter;
       e_stack++;

       op_cat();
      }

     result_delimiter = (char)delimiter;

     /* Perform operation */

     InitDescr(e_stack, ADDR);         /* Substring */
     (e_stack++)->data.d_addr = &op_arg1;

     InitDescr(e_stack, ADDR);         /* Copy of second argument */
     (e_stack++)->data.d_addr = arg2;

     InitDescr(e_stack, ADDR);         /* Copy of third argument */
     (e_stack++)->data.d_addr = arg3;

     dispatch[opcode]();

     /* Save result */

     op_cat();

     first = FALSE;
    } while(src1_str != NULL);

 k_release(&op_arg1);

 /* The stack currently has:
     Result
     String 3
     String 2
     String 1
   We need to remove the three source strings */

 k_release(arg1);
 k_release(arg2);
 k_release(arg3);
 *(e_stack - 4) = *(e_stack - 1);
 e_stack -= 3;
}

/* ======================================================================
   op_mvdsss()  -  MVDSSS opcode                                          */

void op_mvdsss()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  String 4                   | Result                      |
     |-----------------------------|-----------------------------|
     |  String 3                   |                             |
     |-----------------------------|-----------------------------|
     |  String 2                   |                             |
     |-----------------------------|-----------------------------|
     |  Dynamic array              |                             |
     |=============================|=============================|
 */

 u_char opcode;

 DESCRIPTOR * arg1;        /* arg1 source descriptor */
 short int delimiter = 0;  /* Last mark character */
 STRING_CHUNK * src1_str;  /* Current chunk */
 short int src1_offset = 0;
 char * src1;
 
 DESCRIPTOR * arg2;       /* arg2 source descriptor */
 DESCRIPTOR * arg3;       /* arg3 source descriptor */
 DESCRIPTOR * arg4;       /* arg4 source descriptor */

 DESCRIPTOR op_arg1;

 STRING_CHUNK * result_str;
 char result_delimiter;
 bool first = TRUE;

 register char c;
 short int n;


 /* Get opcode to execute for each element */

 opcode = *(pc++);

 /* Ensure that all arguments are strings */

 arg4 = e_stack - 1;
 k_get_string(arg4);

 arg3 = e_stack - 2;
 k_get_string(arg3);

 arg2 = e_stack - 3;
 k_get_string(arg2);

 arg1 = e_stack - 4;
 k_get_string(arg1);
 src1_str = arg1->data.str.saddr;

 /* Make descriptors to store substring */

 InitDescr(&op_arg1, UNASSIGNED);

 /* Place a null string on the stack to accumulate the result */

 InitDescr(e_stack, STRING);
 (e_stack++)->data.str.saddr = NULL;

 do {
     k_release(&op_arg1);
     InitDescr(&op_arg1, STRING);
     op_arg1.data.str.saddr = NULL;

     delimiter = 256;  /* For end of string paths */
     ts_init(&op_arg1.data.str.saddr, 100);

     while(src1_str != NULL)
      {
       src1 = src1_str->data + src1_offset;
       while(src1_offset++ < src1_str->bytes)
        {
         c = *(src1++);
         if (IsMark(c))
          {
           delimiter = (u_char)c;
           goto end_item;
          }
         ts_copy_byte(c);
        }
       src1_str = src1_str->next;
       src1_offset = 0;
      }
end_item:
     ts_terminate();

     /* Insert delimiter */

     if (!first)
      {
       InitDescr(e_stack, STRING);
       result_str = e_stack->data.str.saddr = s_alloc(1, &n);
       result_str->ref_ct = 1;
       result_str->string_len = 1;
       result_str->bytes = 1;
       result_str->data[0] = result_delimiter;
       e_stack++;

       op_cat();
      }

     result_delimiter = (char)delimiter;

     /* Perform operation */

     InitDescr(e_stack, ADDR);         /* Substring */
     (e_stack++)->data.d_addr = &op_arg1;

     InitDescr(e_stack, ADDR);         /* Copy of second argument */
     (e_stack++)->data.d_addr = arg2;

     InitDescr(e_stack, ADDR);         /* Copy of third argument */
     (e_stack++)->data.d_addr = arg3;

     InitDescr(e_stack, ADDR);         /* Copy of fourth argument */
     (e_stack++)->data.d_addr = arg4;

     dispatch[opcode]();

     /* Save result */

     op_cat();

     first = FALSE;
    } while(src1_str != NULL);

 k_release(&op_arg1);

 /* The stack currently has:
     Result
     String 4
     String 3
     String 2
     String 1
   We need to remove the three source strings */

 k_release(arg1);
 k_release(arg2);
 k_release(arg3);
 k_release(arg4);
 *(e_stack - 5) = *(e_stack - 1);
 e_stack -= 4;
}

/* ======================================================================
   op_substrng()  -  SUBSTRNG opcode                                      */

void op_substrng()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Length                     |  Result                     |
     |-----------------------------|-----------------------------|
     |  Start                      |                             |
     |-----------------------------|-----------------------------|
     |  Dynamic array              |                             |
     |=============================|=============================|
 */

 k_recurse(pcode_substrn, 3);
}


/* ======================================================================
   op_splice()  -  SPLCIE opcode                                          */

void op_splice()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Array 2                    | Result                      |
     |-----------------------------|-----------------------------|
     |  String                     |                             |
     |-----------------------------|-----------------------------|
     |  Array 1                    |                             |
     |=============================|=============================|
 */

 DESCRIPTOR * array1;      /* array1 source descriptor */
 bool reuse1;
 short int delimiter1 = 0; /* Last mark character */
 STRING_CHUNK * src1_str;  /* Current chunk */
 short int src1_offset = 0;
 char * src1;
 bool need1;
 
 DESCRIPTOR * array2;     /* array2 source descriptor */
 bool reuse2;
 short int delimiter2 = 0;      /* Last mark character */
 STRING_CHUNK * src2_str; /* Current chunk */
 short int src2_offset = 0;
 char * src2;
 bool need2;

 DESCRIPTOR * str_descr;

 DESCRIPTOR op_array1;
 DESCRIPTOR op_array2;

 STRING_CHUNK * result_str;
 char result_delimiter;
 bool first = TRUE;

 register char c;
 short int n;


 /* Ensure that both arrays are strings */

 array2 = e_stack - 1;
 reuse2 = array2->flags & DF_REUSE;
 k_get_string(array2);
 src2_str = array2->data.str.saddr;

 array1 = e_stack - 3;
 reuse1 = array1->flags & DF_REUSE;
 k_get_string(array1);
 src1_str = array1->data.str.saddr;

 /* Get interleaved string */

 str_descr = e_stack - 2;
 k_get_string(str_descr);

 /* Make descriptors to store substrings */

 InitDescr(&op_array1, UNASSIGNED);
 InitDescr(&op_array2, UNASSIGNED);

 /* Place a null string on the stack to accumulate the result */

 InitDescr(e_stack, STRING);
 (e_stack++)->data.str.saddr = NULL;

 while((src1_str != NULL) || (src2_str != NULL))
  {
   /* Determine which items we need to fetch from the source strings */

   need1 = (delimiter1 <= delimiter2);
   need2 = (delimiter1 >= delimiter2);

   if (need1)   
    {
     /* Fetch an element of array1 */

     k_release(&op_array1);
     InitDescr(&op_array1, STRING);
     op_array1.data.str.saddr = NULL;

     delimiter1 = 256;  /* For end of string paths */
     ts_init(&op_array1.data.str.saddr, 100);

     while(src1_str != NULL)
      {
       src1 = src1_str->data + src1_offset;
       while(src1_offset++ < src1_str->bytes)
        {
         c = *(src1++);
         if (IsMark(c))
          {
           delimiter1 = (u_char)c;
           goto end_item1;
          }
         ts_copy_byte(c);
        }
       src1_str = src1_str->next;
       src1_offset = 0;
      }
end_item1:
     ts_terminate();
    }
   else /* Reuse or insert default value for item 1 */
    {
     if (!reuse1)
      {
       k_release(&op_array1);
       InitDescr(&op_array1, STRING);
       op_array1.data.str.saddr = NULL;
      }
    }

   if (need2)
    {
     /* Fetch an element of array2 */

     k_release(&op_array2);
     InitDescr(&op_array2, STRING);
     op_array2.data.str.saddr = NULL;

     delimiter2 = 256;   /* For end of string paths */
     ts_init(&op_array2.data.str.saddr, 100);

     while(src2_str != NULL)
      {
       src2 = src2_str->data + src2_offset;
       while(src2_offset++ < src2_str->bytes)
        {
         c = *(src2++);
         if (IsMark(c))
          {
           delimiter2 = (u_char)c;
           goto end_item2;
          }
         ts_copy_byte(c);
        }
       src2_str = src2_str->next;
       src2_offset = 0;
      }
end_item2:
     ts_terminate();
    }
   else /* Reuse or insert default value for item 2 */
    {
     if (!reuse2)
      {
       k_release(&op_array2);
       InitDescr(&op_array2, STRING);
       op_array2.data.str.saddr = NULL;
      }
    }

   /* Insert delimiter */

   if (!first)
    {
     InitDescr(e_stack, STRING);
     result_str = e_stack->data.str.saddr = s_alloc(1, &n);
     result_str->ref_ct = 1;
     result_str->string_len = 1;
     result_str->bytes = 1;
     result_str->data[0] = result_delimiter;
     e_stack++;

     op_cat();
    }

   result_delimiter = (char)min(delimiter1, delimiter2);

   /* Perform operation */

   InitDescr(e_stack, ADDR);
   (e_stack++)->data.d_addr = &op_array1;
   InitDescr(e_stack, ADDR);
   (e_stack++)->data.d_addr = str_descr;
   InitDescr(e_stack, ADDR);
   (e_stack++)->data.d_addr = &op_array2;

   op_cat();
   op_cat();

   /* Save result */

   op_cat();

   first = FALSE;
  }

 k_release(&op_array1);
 k_release(&op_array2);

 /* The stack currently has:
     Result
     Array 2
     String
     Array 1
   We need to remove the three source strings */

 k_release(array1);
 k_release(str_descr);
 k_release(array2);
 *(e_stack - 4) = *(e_stack - 1);
 e_stack -= 3;
}

/* END-CODE */
