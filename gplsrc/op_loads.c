/* OP_LOADS.C
 * Load and store opcodes
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
 * 05 Oct 06  2.4-14 Added op_ldlstr().
 * 03 Jul 06  2.4-6 op_arg() now aborts if illegal argument number given.
 * 30 Jun 06  2.4-6 op_arg() now returns ADDR to allow use as lvalue.
 * 27 Mar 06  2.3-9 Added op_arg().
 * 24 Mar 06  2.3-9 Added op_me().
 * 15 Feb 06  2.3-6 Replaced set_zero() with set_descr() and added op_setunass.
 * 23 Jan 06  2.3-5 0451 set_zero() was not releasing complex descriptor.
 * 01 Dec 05  2.2-18 Added LOCALVARS.
 * 16 Mar 05  2.1-10 0326 Added op_storsys() to set DF_SYSTEM.
 * 24 Dec 04  2.1-0 Added op_deref() for dereferencing arguments.
 * 20 Sep 04  2.0-2 Use message handler.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 *
 * END-DESCRIPTION
 *
 * op_arg          ARG()
 * op_ass          ASSIGNED()
 * op_changed      CHANGED()
 * op_clear        CLEAR
 * op_deref        DEREF
 * op_dup          DUP
 * op_ld0          LD0
 * op_ld1          LD1
 * op_ldcom        LDCOM
 * op_ldfloat      LDFLOAT
 * op_ldlcl        LDLCL
 * op_ldlint       LDLINT
 * op_ldlstr       LDLSTR
 * op_ldsint       LDSINT
 * op_ldslcl       LDSLCL
 * op_ldnull       LDNULL
 * op_ldstr        LDSTR
 * op_ldsys        LDSYS
 * op_ldsysv       LDSYSV
 * op_ldunass      LDUNASS
 * op_me           ME
 * op_pop          POP
 * op_reuse        REUSE
 * op_setunass     SETUNASS
 * op_stor         STOR
 * op_swap         SWAP
 * op_unass        UNASSIGNED()
 * op_value        VALUE
 *
 * START-CODE
 */

#include "qm.h"
#include "options.h"

Private void set_descr(DESCRIPTOR * p, u_char type);
void op_ldsys(void);


/* ======================================================================
   op_arg()  -  Return argument n from the current subroutine             */

void op_arg()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top | Argument number             | ADDR                        |
     |=============================|=============================|
 */

 DESCRIPTOR * descr;
 int arg;

 descr = e_stack - 1;
 GetInt(descr);
 arg = descr->data.value;
 k_dismiss();

 if ((arg < 1) || (arg > process.program.arg_ct))
  {
   k_error("Illegal argument reference");
  }

 InitDescr(e_stack, ADDR);
 (e_stack++)->data.d_addr = process.program.vars + arg - 1;
}

/* ======================================================================
   op_ass()  -  ASSIGNED() function                                       */

void op_ass()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Any                        | 1 if assigned, else 0       |
     |=============================|=============================|
 */

 DESCRIPTOR * descr;
 DESCRIPTOR * test_descr;

 descr = e_stack - 1;

 test_descr = descr;
 while(test_descr->type == ADDR) test_descr = test_descr->data.d_addr;

 Release(descr);
 InitDescr(descr, INTEGER);
 descr->data.value = (test_descr->type != UNASSIGNED);
}

/* ======================================================================
   op_changed()  -  CHANGED() function (restricted)                       */

void op_changed()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  ADDR to query variable     | 1 if changed, else 0        |
     |=============================|=============================|
 */

 DESCRIPTOR * descr;
 DESCRIPTOR * test_descr;

 descr = e_stack - 1;

 test_descr = descr;
 while(test_descr->type == ADDR) test_descr = test_descr->data.d_addr;

 Release(descr);
 InitDescr(descr, INTEGER);
 descr->data.value = ((test_descr->flags & DF_ARGSET) == 0);
}

/* ======================================================================
   op_clear()  -  CLEAR statement  -  Set local variables to zero         */

void op_clear()
{
 DESCRIPTOR * p;            /* Local variable descriptor */
 short int i;               /* Local variable index */

 for (i = 0, p = process.program.vars; i < process.program.no_vars; i++, p++)
  {
   if ((p->type != COMMON) && (!(p->flags & DF_SYSTEM))) set_descr(p, INTEGER);
  }
}

/* ======================================================================
   op_clrcom()  -  CLEARCOMMON : Set unnamed common variables to zero     */

void op_clrcom()
{
 DESCRIPTOR descr;
 DESCRIPTOR * p;
 ARRAY_HEADER * a_hdr;
 ARRAY_CHUNK * a_chunk;
 STRING_CHUNK * str_hdr;
 char block_name[3+1];

 /* Find blank common */

 sprintf(block_name, "$%d", (int)cproc_level);

 for (a_hdr = process.named_common; a_hdr != NULL;
      a_hdr = a_hdr->next_common)
  {
   a_chunk = a_hdr->chunk[0];
   
   str_hdr = a_chunk->descr[0].data.str.saddr;
   if (strcmp(str_hdr->data, block_name) == 0)
    {
     p = &descr;
     InitDescr(p, COMMON);
     descr.data.c_addr = a_hdr;
     set_descr(p, INTEGER);
     break;
    }
  }
}

/* ======================================================================
   op_deref()  -  DEREF opcode. Resolve item as actual value.             */

void op_deref()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  ADDR to item to resolve    |                             |
     |=============================|=============================|
 */

 DESCRIPTOR * descr;

 descr = (e_stack - 1)->data.d_addr;   /* One step only */
 if (descr->type == ADDR) k_get_value(descr);
 k_pop(1);  /* Always ADDR */
}

/* ======================================================================
   op_dup()  -  Duplicate item at top of stack                            */

void op_dup()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Any                        | Copy of any                 |
     |-----------------------------|-----------------------------|
     |                             | Original any                |
     |=============================|=============================|
 */

 register STRING_CHUNK * str;

 *e_stack = *(e_stack - 1);

 /* ++ALLTYPES++ */

 switch(e_stack->type)
  {
   case SUBR:
      str = e_stack->data.subr.saddr;
      if (str != NULL) str->ref_ct++;
      break;

   case STRING:
   case SELLIST:
      str = e_stack->data.str.saddr;
      if (str != NULL) str->ref_ct++;
      break;

   case FILE_REF:
      e_stack->data.fvar->ref_ct++;
      break;

   case ARRAY:
      e_stack->data.ahdr_addr->ref_ct++;
      break;

   case COMMON:
   case PERSISTENT:
      e_stack->data.c_addr->ref_ct++;
      break;

   case IMAGE:
      e_stack->data.i_addr->ref_ct++;
      break;

   case BTREE:
      e_stack->data.btree->ref_ct++;
      break;

   case SOCK:
      e_stack->data.sock->ref_ct++;
      break;

   case OBJ:
      e_stack->data.objdata->ref_ct++;
      break;
  }

 e_stack++;
}

/* ======================================================================
   op_ld0()  -  Load integer zero                                         */

void op_ld0()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |                             | Integer 0                   |
     |=============================|=============================|
 */

 InitDescr(e_stack, INTEGER);
 (e_stack++)->data.value = 0;
}


/* ======================================================================
   op_ld1()  -  Load integer one                                          */

void op_ld1()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |                             | Integer 1                   |
     |=============================|=============================|
 */

 InitDescr(e_stack, INTEGER);
 (e_stack++)->data.value = 1;
}

/* ======================================================================
   op_ldcom()  -  Load variable from common                               */

void op_ldcom()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |                             | Addr to common var          |
     |=============================|=============================|
 */

 unsigned short int com_no;
 DESCRIPTOR * com_descr;    /* Common block descriptor */
 unsigned long int var_no;
 ARRAY_HEADER * a_hdr;
 ARRAY_CHUNK * a_chunk;

 /* Fetch common block number and locate block */

 com_no = *pc | (unsigned short int)(*(pc + 1) << 8);
 pc += 2;

 com_descr = process.program.vars + com_no;

 while(com_descr->type == ADDR) com_descr = com_descr->data.d_addr;
 if ((com_descr->type != COMMON)
     && (com_descr->type != PERSISTENT)
     && (com_descr->type != LOCALVARS))
  {
   k_error(sysmsg(1233));
  }

 a_hdr = com_descr->data.ahdr_addr;

 /* Fetch variable number within common block */

 var_no = *pc | (((long int)*(pc + 1)) << 8);
 pc += 2;

 if ((var_no < 1) || (var_no > (unsigned long int)(a_hdr->rows)))
  {
   k_error(sysmsg(1234));
  }


 a_chunk = a_hdr->chunk[var_no / MAX_ARRAY_CHUNK_SIZE];

 InitDescr(e_stack, ADDR);
 (e_stack++)->data.d_addr = a_chunk->descr + (var_no % MAX_ARRAY_CHUNK_SIZE);
}

/* ======================================================================
   op_ldfloat()  -  Load floating point value                             */

void op_ldfloat()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |                             | Float value                 |
     |=============================|=============================|
 */

 InitDescr(e_stack, FLOATNUM);

#ifdef BIG_ENDIAN_SYSTEM
 {
  u_char * p;
  u_char * q;
  short int n;
  for(n = sizeof(double), p = (u_char *)&(e_stack->data.float_value), q = pc + n; n--;)
   {
    *(p++) = *(--q);
   }
 }
#else
 memcpy(&(e_stack->data.float_value), pc, sizeof(double));
#endif
 pc += sizeof(double);
 e_stack++;
}

/* ======================================================================
   op_ldlcl()  -  Load local variable address                             */

void op_ldlcl()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |                             | Addr to var                 |
     |=============================|=============================|
 */

 unsigned long int var_no;

 var_no = *pc | (((long int)*(pc + 1)) << 8);
 pc += 2;

 InitDescr(e_stack, ADDR);
 (e_stack++)->data.d_addr = process.program.vars + var_no;
}

/* ======================================================================
   op_ldlint()  -  Load long integer                                      */

void op_ldlint()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |                             | Integer value               |
     |=============================|=============================|
 */

 unsigned long int value;

 value = *pc | (((long int)*(pc+1)) << 8)
             | (((long int)(*(pc+2))) << 16)
             | (((long int)*(pc+3)) << 24);
 pc += 4;

 InitDescr(e_stack, INTEGER);
 (e_stack++)->data.value = (long int)value;
}

/* ======================================================================
   op_ldlstr()  -  Load long string                                        */

void op_ldlstr()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |                             | String                      |
     |=============================|=============================|
 */

 long int string_length;

 string_length = *pc | (((long int)*(pc+1)) << 8)
                     | (((long int)(*(pc+2))) << 16)
                     | (((long int)(*(pc+3))) << 24);
 pc += 4;

 k_put_string((char *)pc, string_length, e_stack);
 e_stack++;
 pc += string_length;
}

/* ======================================================================
   op_ldsint()  -  Load short integer                                     */

void op_ldsint()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |                             | Integer value               |
     |=============================|=============================|
 */

 InitDescr(e_stack, INTEGER);
// (e_stack++)->data.value = *(((signed char *)pc)++);
 (e_stack++)->data.value = (signed char)(*(pc++));
}

/* ======================================================================
   op_ldslcl()  -  Load short local variable address                      */

void op_ldslcl()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |                             | Addr to var                 |
     |=============================|=============================|
 */

 InitDescr(e_stack, ADDR);
 (e_stack++)->data.d_addr = process.program.vars + *(pc++);
}

/* ======================================================================
   op_ldnull()  -  Load null string                                       */

void op_ldnull()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |                             | Null string                 |
     |=============================|=============================|
 */

 InitDescr(e_stack, STRING);
 (e_stack++)->data.str.saddr = NULL;
}

/* ======================================================================
   op_ldstr()  -  Load string                                             */

void op_ldstr()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |                             | String                      |
     |=============================|=============================|
 */

 short int string_length;
 short int actual_length;
 STRING_CHUNK * str;

 string_length = *(pc++);     /* Always fits a single chunk */

 InitDescr(e_stack, STRING);

 if (string_length == 0)
  {
   e_stack->data.str.saddr = NULL;
  }
 else
  {
   str = s_alloc((long)string_length, &actual_length);
   e_stack->data.str.saddr = str;
  
   str->ref_ct = 1;
   str->string_len = string_length;
   str->bytes = string_length;
   memcpy(str->data, pc, string_length);
  
   pc += string_length;
  }

 e_stack++;
}

/* ======================================================================
   op_ldsys()  -  Load system variable from $SYSCOM common block          */

void op_ldsys()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |                             | Addr to SYSCOM var          |
     |=============================|=============================|
 */

 unsigned long int var_no;
 ARRAY_HEADER * a_hdr;
 ARRAY_CHUNK * a_chunk;

 /* Fetch common block number and locate block */

 if (process.syscom == NULL) k_error(sysmsg(1235));

 a_hdr = process.syscom;

 /* Fetch variable number within common block */

 var_no = *(pc++);

 if ((var_no < 1) || (var_no > (unsigned long int)(a_hdr->rows)))
  {
   k_error(sysmsg(1234));
  }

 a_chunk = a_hdr->chunk[var_no / MAX_ARRAY_CHUNK_SIZE];

 InitDescr(e_stack, ADDR);
 (e_stack++)->data.d_addr = &(a_chunk->descr[var_no % MAX_ARRAY_CHUNK_SIZE]);
}

/* ======================================================================
   op_ldsysv()  -  Load value of system variable from $SYSCOM common block */

void op_ldsysv()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |                             | SYSCOM var value            |
     |=============================|=============================|
 */

 op_ldsys();
 k_get_value(e_stack - 1);
}

/* ======================================================================
   op_ldunass()  -  Load unassigned                                       */

void op_ldunass()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |                             | Unassigned                  |
     |=============================|=============================|
 */

 InitDescr(e_stack++, UNASSIGNED);
}

/* ======================================================================
   op_me()  -  Create OBJ descriptor for self                             */

void op_me()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |                             | OBJ descriptor              |
     |=============================|=============================|
 */

 if (process.program.objdata == NULL) k_error("ME reference in non-object code");

 InitDescr(e_stack, OBJ);
 process.program.objdata->ref_ct++;
 (e_stack++)->data.objdata = process.program.objdata;
}

/* ======================================================================
   op_pop()  -  Pop evaluation stack                                      */

void op_pop()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top | Any                         |                             |
     |=============================|=============================|
 */

 k_dismiss();
}

/* ======================================================================
   op_reuse()  -  REUSE opcode. Resolve stack top as actual value with
                  reuse flag set.                                         */

void op_reuse()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top | Any                         | Value with reuse flag set   |
     |=============================|=============================|
 */

 DESCRIPTOR * descr;

 descr = e_stack - 1;
 k_get_value(descr);       /* Would normally expect a string */
 descr->flags |= DF_REUSE;
}

/* ======================================================================
   op_stnull()  -  Store a null string in variable                        */

void op_stnull()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  ADDR to target variable    |                             |
     |=============================|=============================|
 */

 DESCRIPTOR * descr;

 descr = e_stack - 1;

 do {
     descr = descr->data.d_addr;
    } while(descr->type == ADDR);

 Release(descr);
 InitDescr(descr, STRING);
 descr->data.str.saddr = NULL;
 k_pop(1);      /* Dismiss ADDR */
}

/* ======================================================================
   op_setunass()  -  Set variable to unassigned (restricted)              */

void op_setunass()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top | Addr                        |                             |
     |=============================|=============================|
 */

 DESCRIPTOR * descr;

 descr = e_stack - 1;
 while(descr->type == ADDR) descr = descr->data.d_addr;
 set_descr(descr, UNASSIGNED);
 k_pop(1);
}

/* ======================================================================
   op_stor()  -  Store value in variable                                  */

void op_stor()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top | Any resolvable to value     |                             |
     |-----------------------------|-----------------------------|
     | Addr                        |                             |
     |=============================|=============================|
 */

 DESCRIPTOR * value_descr;
 DESCRIPTOR * var_descr;

 var_descr = e_stack - 2;
 value_descr = e_stack - 1;

 /* For best performance, test the type before calling k_get_value().  The
    item at the top of the e-stack is quite likely to be a value already.  */

 switch(value_descr->type)     /* ++ALLTYPES++ Check if affected */
  {
   case ADDR:
      k_get_value(value_descr);
      break;

   case UNASSIGNED:
      if (!Option(OptUnassignedWarning)) k_unassigned(value_descr);
      k_unass_zero(value_descr, value_descr);
      break;
  }

 do {
     var_descr = var_descr->data.d_addr;
    } while(var_descr->type == ADDR);

 Release(var_descr);

 *var_descr = *value_descr;
 var_descr->flags &= ~(DF_REUSE | DF_CHANGE); /* Ensure clear; eg A = REUSE(B) */
  

 /* Remove both items from stack. The data item was copied so we must not
    use k_dismiss() whatever its type. The address item is known to be a
    ADDR so a k_pop() is adequate.                                      */

 k_pop(2);      /* Dismiss ADDR */
}

/* ======================================================================
   op_storsys()  -  Store value in system variable                        */

void op_storsys()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top | Any resolvable to value     |                             |
     |-----------------------------|-----------------------------|
     | Addr                        |                             |
     |=============================|=============================|

     System variables are those created by the compiler for its own
     purposes. These variables must not be cleared by the CLEAR opcode.
 */

 DESCRIPTOR * value_descr;
 DESCRIPTOR * var_descr;

 var_descr = e_stack - 2;
 value_descr = e_stack - 1;

 /* For best performance, test the type before calling k_get_value().  The
    item at the top of the e-stack is quite likely to be a value already.  */

 switch(value_descr->type)     /* ++ALLTYPES++ Check if affected */
  {
   case ADDR:
      k_get_value(value_descr);
      break;

   case UNASSIGNED:
      if (!Option(OptUnassignedWarning)) k_unassigned(value_descr);
      k_unass_zero(value_descr, value_descr);
      break;
  }

 do {
     var_descr = var_descr->data.d_addr;
    } while(var_descr->type == ADDR);

 Release(var_descr);

 *var_descr = *value_descr;
 var_descr->flags &= ~(DF_REUSE | DF_CHANGE); /* Ensure clear; eg A = REUSE(B) */
 var_descr->flags |= DF_SYSTEM;

 /* Remove both items from stack. The data item was copied so we must not
    use k_dismiss() whatever its type. The address item is known to be a
    ADDR so a k_pop() is adequate.                                      */

 k_pop(2);      /* Dismiss ADDR */
}

/* ======================================================================
   op_stz()  -  Store zero in variable                                    */

void op_stz()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  ADDR to target variable    |                             |
     |=============================|=============================|
 */

 DESCRIPTOR * descr;

 descr = e_stack - 1;

 do {
     descr = descr->data.d_addr;
    } while(descr->type == ADDR);

 Release(descr);

 InitDescr(descr, INTEGER);
 descr->data.value = 0;
 k_pop(1);      /* Dismiss ADDR */
}

/* ======================================================================
   op_swap()  -  Swap top two items on stack                              */

void op_swap()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Any A                      | Any B                       |
     |-----------------------------|-----------------------------|
     |  Any B                      | Any A                       |
     |=============================|=============================|
 */

 DESCRIPTOR descr;

 descr = *(e_stack - 1);

 *(e_stack - 1) = *(e_stack - 2);
 *(e_stack - 2) = descr;
}

/* ======================================================================
   op_unass()  -  UNASSIGNED() function                                   */

void op_unass()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Any                        | 1 if unassigned, else 0     |
     |=============================|=============================|
 */

 DESCRIPTOR * descr;
 DESCRIPTOR * test_descr;

 descr = e_stack - 1;

 test_descr = descr;
 while(test_descr->type == ADDR) test_descr = test_descr->data.d_addr;

 Release(descr);
 InitDescr(descr, INTEGER);
 descr->data.value = (test_descr->type == UNASSIGNED);
}

/* ======================================================================
   op_value()  -  VALUE opcode. Resolve stack top as actual value.        */

void op_value()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Any                        | Value                       |
     |=============================|=============================|
 */

 k_get_value(e_stack - 1);
}

/* ======================================================================
   set_descr()  -  Set descriptor(s) to integer zero or unassigned        */

Private void set_descr(
   DESCRIPTOR * p,
   u_char type)
{
 ARRAY_HEADER * a_hdr;      /* Array header pointer */
 ARRAY_CHUNK * chunk;       /* Array chunk pointer */
 short int j;               /* Array chunk index */
 short int k;               /* Array element index */
 short int lo;

 /* ++ALLTYPES++ */

 switch(p->type)
  {
   case STRING:
   case FILE_REF:
   case SUBR:
   case SELLIST:
   case SOCK:
   case OBJ:
   case OBJCD:
   case OBJCDX:
      k_release(p);
      /* *** FALL THROUGH *** */
   case UNASSIGNED:
   case ADDR:
   case FLOATNUM:
   case INTEGER:
      InitDescr(p, type);
      p->data.value = 0;
      break;

   case ARRAY:
   case COMMON:
   case LOCALVARS:
   case PERSISTENT:
      a_hdr = p->data.ahdr_addr;  /* Same as c_addr for common */
      do {
          lo = (p->type == COMMON)?1:0;  /* Don't clear name in common */
          for(j = 0; j < a_hdr->num_chunks; j++)
           {
            chunk = a_hdr->chunk[j];
            if (chunk != NULL) /* Was allocated successfully */
             {
              for(k = lo; k < chunk->num_descr; k++) set_descr(&(chunk->descr[k]), type);
             }
            lo = 0;
           }
          if (p->type != LOCALVARS) break;
          a_hdr = a_hdr->next_common;  /* Links to stacked instance */
         } while(a_hdr != NULL);
      break;

   case PMATRIX:
      break;
  }
}

/* END-CODE */
