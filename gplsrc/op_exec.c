/* OP_EXEC.C
 * Execute and related opcodes
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
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 19 Jun 07  2.5-7 k_call() now has additional argument for stack adjustment.
 * 24 Feb 06  2.3-7 Stack display print unit across capture.
 * 20 Sep 05  2.2-11 0411 Must set IS_CLEXEC when using EXECUTE CURRENT.LEVEL so
 *                   that STOP can recognise that it needs to go back to the
 *                   pseudo command processor.
 * 15 Sep 05  2.2-10 0409 The CURRENT.LEVEL option to EXECUTE must cause the
 *                   command to execute at the current command processor level.
 *                   This is achieved by turning off the HDR.IS.CPROC flag so
 *                   that the command processor doesn't look like a command
 *                   processor at all.
 * 22 Dec 04  2.1-0 Handle nested EXECUTE...CAPTURING constructs.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * Opcodes:
 * op_capture()   CAPTURE   Save data from EXECUTE CAPTURING
 * op_execute()   EXECUTE   Call $CPROC for new command processor level
 * op_passlist()  PASSLIST
 * op_rtnlist()   RTNLIST
 *
 * EXECUTE xxx {TRAPPING ABORTS}
 *             {CAPTURING var1} 
 *             {PASSLIST var2} 
 *             {RTNLIST var3} 
 *             {SETTING | RETURNING var4} 
 *
 * xxx is stored in $SYSCOM XEQ.COMMAND
 * 
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include "header.h"
#include "syscom.h"
#include "tio.h"

typedef struct SAVED_PU SAVED_PU;
struct SAVED_PU
{
 struct SAVED_PU * next;
 PRINT_UNIT print_unit;
};

Private SAVED_PU * saved_pu = NULL;

#ifdef __BORLANDC__
/* Windows Borland compiler bug.
   The optimiser appears to mess up the newline substitution loop in
   op_capture.
*/

   #pragma option -Od
#endif

/* ======================================================================
   op_capture()  -  Save data from EXECUTE CAPTURING                      */

void op_capture()
{
/*
     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  ADDR to target                |                             |
     |================================|=============================|
*/

 DESCRIPTOR * descr;
 STRING_CHUNK * str;
 STRING_CHUNK * last = NULL;
 STRING_CHUNK * prev = NULL;
 short int bytes;
 char * p;
 char * q;


 if (!capturing)
  {
   /* 0117  This is EXECUTE xx TRAPPING ABORTS CAPTURING var where the
      executed command has aborted, wiping our target variable addr from
      the stack.  Clean up the captured data and return. The target will
      have been set to null for programs compiled from this release onwards.
      Older programs will leave the variable unchanged.                     */

   s_free(capture_head);
   capture_head = NULL;
   return;
  }

 descr = e_stack - 1;

 do {
     descr = descr->data.d_addr;
    } while(descr->type == ADDR);

 Release(descr);

 /* Change all newlines to field marks */

 for(str = capture_head; str != NULL; str = str->next)
  {
   last = str;
   bytes = str->bytes;
   p = str->data;
   while((bytes > 0) && ((q = (char *)memchr(p, '\n', bytes)) != NULL))
    {
     *q = FIELD_MARK;
     bytes -= q + 1 - p;
     p = q + 1;
    }
   prev = str;
  }

 /* Remove last character if it is a field mark */

 if ((last != NULL)
    && (last->data[last->bytes - 1] == FIELD_MARK))
  {
   (capture_head->string_len)--;
   if (--(last->bytes) == 0)   /* Remove final chunk */
    {
     if (prev == NULL) capture_head = NULL; /* Was only chunk */
     else prev->next = NULL;
     k_free(last);
    }
  }

 InitDescr(descr, STRING);
 descr->data.str.saddr = capture_head;

 capture_head = NULL;
 capturing = FALSE;
 unstack_display_pu();

 /* Look for a stacked EXECUTE...CAPTURING construct */

 if (process.program.flags & PF_CAPTURING)
  {
   capture_head = process.program.saved_capture_head;
   capture_tail = process.program.saved_capture_tail;
   capturing = TRUE;
   stack_display_pu();
   process.program.flags &= ~PF_CAPTURING;
  }

 k_pop(1);
}

/* ======================================================================
   op_execute()  -  EXECUTE opcode                                        */

void op_execute()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Addr to RTNLIST var        |                             |
     |  (only if RTNLIST set)      |                             |
     |-----------------------------|-----------------------------| 
     |  Addr to capture var        |                             |
     |  (only if CAPTURING set)    |                             |
     |=============================|=============================|

 The command to be executed has already been stored in SYSCOM XEQ.COMMAND

 The RTNLIST and CAPTURING elements will be handled by inline code on
 return from the executed command.

 The EXECUTE opcode is followed by a single byte flag word:
    0x01   TRAPPING ABORTS
    0x02   CAPTURING
    0x04   RTNLIST
    0x08   PASSLIST  (set by compiler but not used in run machine)
    0x10   RETURNING (set by compiler but not used in run machine)
    0x20   CURRENT.LEVEL  (restricted)
 */

 u_char flags;

 flags = *(pc++);
     
 if (flags & 0x02)   /* CAPTURING */
  {
   if (capturing) /* This is a nested EXECUTE...CAPTURING structure */
    {
     process.program.flags |= PF_CAPTURING;

     if (capture_head != NULL)  /* Stack data already captured */
      {
       process.program.saved_capture_head = capture_head;
       process.program.saved_capture_tail = capture_tail;
      }
    }
   else
    {
    }

   capture_head = NULL;
   capturing = TRUE;
   stack_display_pu();
  }

 /* The EXECUTE opcode is simply a CALL to $CPROC.  The compiler cannot
    plant the actual CALL opcode as the called name is illegal for user
    mode programs.                                                      */

 k_call("$CPROC", 0, NULL, 0);
 process.program.flags |= IS_EXECUTE;
 if (!(flags & 0x01)) process.program.flags |= IGNORE_ABORTS;

 if (flags & 0x20) /* CURRENT.LEVEL */
  {
   /* Make this look like a normal program, not a command processor but
      tag it as a using EXECUTE CURRENT.LEVEL for handling STOP */
   process.program.flags &= ~HDR_IS_CPROC;
   process.program.flags |= IS_CLEXEC;
  }

 if (flags & 0x04)   /* RTNLIST */
  {
   rtnlist = TRUE;
  }

 return;
}

/* ======================================================================
   op_passlist()  -  Move select list variable to list 0                  */

void op_passlist()
{
/*
     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  ADDR to list                  |                             |
     |================================|=============================|
*/

 DESCRIPTOR * src_descr;
 DESCRIPTOR * tgt_descr;
 STRING_CHUNK * tgt_str;

 /* Find source list without dereferencing stack entry */

 src_descr = e_stack - 1;
 while(src_descr->type == ADDR) src_descr = src_descr->data.d_addr;
 if (src_descr->type != SELLIST) k_data_type(src_descr);

 /* Find target list */

 tgt_descr = SelectList(0);
 k_release(tgt_descr);
 
 /* Move data across, taking the remove pointer with it */

 *tgt_descr = *src_descr;
 tgt_descr->type = STRING;
 tgt_str = tgt_descr->data.str.saddr;

 /* Set count on list 0 */

 tgt_descr = SelectCount(0);
 tgt_descr->data.value = (tgt_str != NULL)?(tgt_str->offset):0;

 /* Set source as unassigned variable. Do not release as we have moved the
    string to the new descriptor.                                          */

 InitDescr(src_descr, UNASSIGNED);

 k_dismiss();
}

/* ======================================================================
   op_rtnlist()  -  Save data from EXECUTE RTNLIST                        */

void op_rtnlist()
{
/*
     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  ADDR to target                |                             |
     |================================|=============================|
*/

 DESCRIPTOR * src_descr;
 DESCRIPTOR * tgt_descr;
 STRING_CHUNK * tgt_str;

 if (rtnlist)  /* Abort hasn't wiped out our target variable */
  {
   src_descr = SelectList(0);
   if (src_descr->type == STRING)
    {
     for(tgt_descr = e_stack - 1; tgt_descr->type == ADDR;
         tgt_descr = tgt_descr->data.d_addr) {}
     k_release(tgt_descr);

     *tgt_descr = *src_descr;
     tgt_descr->type = SELLIST;
     tgt_str = tgt_descr->data.str.saddr;

     InitDescr(src_descr, STRING);
     src_descr->data.str.saddr = NULL;

     src_descr = SelectCount(0);
     if (tgt_str != NULL) tgt_str->offset = src_descr->data.value;
     src_descr->data.value = 0;
   }

   rtnlist = FALSE;
   k_pop(1);
  }
}

/* ======================================================================
   stack_display_pu()  -  Stack display print unit on capture             */

void stack_display_pu()
{
 SAVED_PU * p;

 /* Save the print unit definition for the display and reset it */

 p = (SAVED_PU *)k_alloc(114,sizeof(struct SAVED_PU));
 if (p == NULL) k_error(sysmsg(1712));

 copy_print_unit(&tio.dsp, &(p->print_unit));

 p->next = saved_pu;
 saved_pu = p;
}

/* ======================================================================
   unstack_display_pu()  -  Recover saved display print unit              */

void unstack_display_pu()
{
 SAVED_PU * next_saved_pu;

 /* Recover the previously saved display print unit definition */

 if (saved_pu != NULL)  /* Sanity check */
  {
   free_print_unit(&tio.dsp);
   copy_print_unit(&(saved_pu->print_unit), &tio.dsp);
   free_print_unit(&(saved_pu->print_unit));
   next_saved_pu = saved_pu->next;
   k_free(saved_pu);
   saved_pu = next_saved_pu;
  }
}

/* END-CODE */
