/* OP_STOP.C
 * Stop and Abort opcodes
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
 * 27 Sep 05  2.2-13 0415 op_abortmsg() did not allow for the message text
 *                   coming from @ABORT.MESSAGE (Used in CPROC).
 * 14 Jan 05  2.1-1 Suppress abort details for internal mode programs.
 * 20 Sep 04  2.0-2 Use dynamic pcode loader.
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
#include "syscom.h"
#include "header.h"
#include "options.h"

void op_dspnl(void);
void op_abort(void);

Private void pickmsg(bool is_abort);

/* ====================================================================== */

void op_abort()
{
 DESCRIPTOR * msg_descr;

 msg_descr = Element(process.syscom, SYSCOM_ABORT_MESSAGE);
 k_release(msg_descr);
 InitDescr(msg_descr, STRING);
 msg_descr->data.str.saddr = NULL;

 if ((process.program.flags & HDR_INTERNAL) || Option(OptSuppressAbortMsg))
  {
   k_exit_cause = K_ABORT;
  }
 else k_error("Abort");
}

/* ====================================================================== */

void op_abortmsg()
{
 DESCRIPTOR * msg_descr;
 DESCRIPTOR * descr;
 STRING_CHUNK * str = NULL;
 static bool aborting = FALSE;    /* Used to avoid recursion if not string */

 /* 0415 Resequenced to allow "ABORT @ABORT.MESSAGE" (used in CPROC) */

 if (!aborting)
  {
   aborting = TRUE;
   descr = e_stack - 1;
   k_get_string(descr);
   if ((str = descr->data.str.saddr) != NULL)
    {
     str->ref_ct++;
    }
   aborting = FALSE;
  }

 msg_descr = Element(process.syscom, SYSCOM_ABORT_MESSAGE);
 k_release(msg_descr);
 InitDescr(msg_descr, STRING);
 msg_descr->data.str.saddr = str;

 if (str != NULL) op_dspnl();
 else k_dismiss();

 if ((process.program.flags & HDR_INTERNAL) || Option(OptSuppressAbortMsg))
  {
   k_exit_cause = K_ABORT;
  }
 else k_error("Abort");
}

/* ======================================================================
   op_errmsg()  -  Pick style error message (ERRMSG or STOP)              */

void op_errmsg()
{
 pickmsg(FALSE);
}

Private void pickmsg(bool is_abort)
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Arguments                  |                             |
     |-----------------------------|-----------------------------|
     |  Message id                 |                             |
     |=============================|=============================|
 */

 InitDescr(e_stack, INTEGER);
 (e_stack++)->data.value = is_abort;

 k_recurse(pcode_pickmsg, 3); /* Execute recursive code */
}

/* ======================================================================
   op_pabort()  -  Pick style abort                                       */

void op_pabort()
{
 pickmsg(TRUE);
 op_abort();
}

/* ====================================================================== */

void op_stop()
{
 k_exit_cause = K_STOP;
}

/* END-CODE */
