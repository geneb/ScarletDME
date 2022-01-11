/* OP_JUMPS.C
 * Jump opcodes
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
 * ScarletDME Wiki: https://scarlet.deltasoft.com
 * 
 * START-HISTORY (ScarletDME):
 * 11Jan22 gwb Fix for Issue #12
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 *
 * 22Feb20 gwb Converted sprintf() to snprintf() in op_chkcat().
 *             More error handling needs to be implemented due to this change.
 * 
 * START-HISTORY (OpenQM):
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 19 Jun 07  2.5-7 k_call() now has additional argument for stack adjustment.
 * 07 Mar 06  2.3-8 Adapted computed_jump() to allow for over 255 labels.
 * 09 Sep 05  2.2-10 0406 Re-evaluate address of name descriptor before turning
 *                   it into a SUBR in op_call() as k_call() may have moved it
 *                   when expanding the e_stack.
 * 29 Jun 05  2.2-1 Added op_enter().
 * 13 May 05  2.1-15 Added error arg to s_make_contiguous().
 * 16 Mar 05  2.1-10 0326 Preserve flags when converting a descriptor to a SUBR.
 * 09 Mar 05  2.1-8 Added Pick style ON GOTO and ON GOSUB.
 * 20 Sep 04  2.0-2 Use message handler.
 * 20 Sep 04  2.0-2 Use dynamic pcode loader.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * Opcodes:
 * op_call()      CALL      Call catalogued subroutine
 * op_callv()     CALLV     Call catalogued subroutine, variable arg list
 * op_chkcat()    CHKCAT    Check catalogue for named subroutine
 * op_enter()     ENTER     Pick style ENTER
 * op_gosub()     GOSUB     Local subroutine call
 * op_jmp()       JMP       Unconditional jump
 * op_jng()       JNG       Jump if negative
 * op_jnz()       JNZ       Jump if non-zero
 * op_jpg()       JPG       Jump to start of next page
 * op_jpo()       JPO       Jump if strictly positive
 * op_jpz()       JPZ       Jump if positive or zero
 * op_jze()       JZE       Jump if zero
 * op_ongosub()   ONGOSUB   Computed local subroutine call
 * op_ongosubp()  ONGOSUBP  Computed local subroutine call (Pick style)
 * op_ongoto()    ONGOTO    Computed jump
 * op_ongotop()   ONGOTOP   Computed jump (Pick style)
 * op_return()    RETURN    Return from GOSUB or CALL
 * op_returnto()  RETURNTO  Jump to label if from GOSUB, return if from CALL
 * op_run()       RUN
 *
 * Local functions:
 * computed_jump()          Common path for ONGOTO and ONGOSUB
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include "header.h"
#include "syscom.h"
#include "tio.h"
#include "locks.h"

#include <time.h>

extern time_t clock_time;

Private void computed_jump(bool save_link, bool pick_style);

/* ======================================================================
   op_call()  -  Call catalogued routine                                  */

void op_call() {
  DESCRIPTOR* descr;
  int16_t num_args;
  char call_name[MAX_PROGRAM_NAME_LEN + 1];

  num_args = *(pc++);

  descr = e_stack - (num_args + 1);
  while (descr->type == ADDR)
    descr = descr->data.d_addr;

  switch (descr->type) {
    case STRING:
      if (k_get_c_string(descr, call_name, MAX_PROGRAM_NAME_LEN) <= 0) {
        k_illegal_call_name();
      }

      UpperCaseString(call_name);

      if (!valid_call_name(call_name))
        k_illegal_call_name();

      if (call_name[0] == '$') /* Only callable by internal mode code */
      {
        if (!(process.program.flags & HDR_INTERNAL))
          k_illegal_call_name();
      }

      k_call(call_name, num_args, NULL, 1);

      /* 0407 We need to find the name descriptor again as k_call() may
         have moved it while expanding the e_stack. The arguments have
         been moved into the subroutine's own variable space so the
         desciptor we need is at the top of the stack.                   */

      descr = e_stack - 1;
      while (descr->type == ADDR)
        descr = descr->data.d_addr;

      /* Convert descriptor to become a SUBR.  Convert type, copy string
         pointer, set object address.                                     */

      descr->type = SUBR; /* Preserve flags (e.g. DF_SYSTEM) */
      descr->data.subr.saddr = s_make_contiguous(descr->data.str.saddr, NULL);

      descr->data.subr.object = c_base;
      ((OBJECT_HEADER*)c_base)->ext_hdr.prog.refs++;
      break;

    case SUBR:
      k_call("", num_args, (u_char*)(descr->data.subr.object), 1);
      ((OBJECT_HEADER*)c_base)->ext_hdr.prog.refs++;
      break;

    default:
      k_error(sysmsg(1129));
  }

  k_dismiss(); /* Dismiss call name */

  return;
}

/* ======================================================================
   op_callv()  -  Call catalogued routine with variable argument list     */

void op_callv() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  Addr to args matrix           |                             |
     |--------------------------------|-----------------------------|
     |  Argument count                |                             |
     |--------------------------------|-----------------------------|
     |  Subroutine name               |                             |
     |================================|=============================|
 */

  DESCRIPTOR* subr_descr;
  DESCRIPTOR* descr;
  int16_t num_args;
  char call_name[MAX_PROGRAM_NAME_LEN + 1];
  ARRAY_HEADER* ahdr;
  int16_t i;

  /* Get array header address for argument matrix */

  descr = e_stack - 1;
  while (descr->type == ADDR)
    descr = descr->data.d_addr;
  ahdr = descr->data.ahdr_addr;

  /* Get argument count */

  descr = e_stack - 2;
  GetInt(descr);
  num_args = (int16_t)(descr->data.value);
  if ((num_args < 0) || (num_args > 255))
    k_error(sysmsg(1131));

  if (num_args > ahdr->rows * ((ahdr->cols) ? (ahdr->cols) : 1)) {
    k_error(sysmsg(1132));
  }

  /* Get subroutine name descriptor before we dismiss it. This is always
    an ADDR so dismissing it does no harm.                              */

  subr_descr = e_stack - 3;
  while (subr_descr->type == ADDR)
    subr_descr = subr_descr->data.d_addr;

  /* Dismiss stack items */

  k_pop(2);    /* Dismiss arg matrix pointer and arg count */
  k_dismiss(); /* Dismiss call name */

  /* Put arguments onto stack */

  for (i = 1; i <= num_args; i++) {
    descr = Element(ahdr, i);
    InitDescr(e_stack, ADDR);
    (e_stack++)->data.d_addr = descr;
    while (descr->type == ADDR)
      descr = descr->data.d_addr;
    descr->flags |= DF_ARGSET; /* Monitor change */
  }

  switch (subr_descr->type) {
    case STRING:
      if (k_get_c_string(subr_descr, call_name, MAX_PROGRAM_NAME_LEN) <= 0) {
        k_illegal_call_name();
      }

      UpperCaseString(call_name);

      if (!valid_call_name(call_name))
        k_illegal_call_name();

      if (call_name[0] == '$') /* Only callable by internal mode code */
      {
        if (!(process.program.flags & HDR_INTERNAL))
          k_illegal_call_name();
      }

      k_call(call_name, num_args, NULL, 0);

      /* Convert descriptor to become a SUBR.  Convert type, copy string
         pointer, set object address.                                     */

      subr_descr->type = SUBR; /* Preserve flags (e.g. DF_SYSTEM) */
      subr_descr->data.subr.saddr = subr_descr->data.str.saddr;

      subr_descr->data.subr.object = c_base;
      ((OBJECT_HEADER*)c_base)->ext_hdr.prog.refs++;
      break;

    case SUBR:
      k_call("", num_args, (u_char*)(subr_descr->data.subr.object), 0);
      ((OBJECT_HEADER*)c_base)->ext_hdr.prog.refs++;
      break;

    default:
      k_error(sysmsg(1129));
  }

  return;
}

/* ======================================================================
   op_chkcat()  -  Check catalogue for named subroutine                   */

void op_chkcat() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  Name to locate                | 0 = Not found               |
     |                                | 1 = Locally catalogued      |
     |                                | 2 = Privately catalogued    |
     |                                | 3 = Gloablly catalogued     |
     |================================|=============================|

 */

  DESCRIPTOR* descr;
  char call_name[MAX_PROGRAM_NAME_LEN + 1];
  char mapped_name[MAX_PROGRAM_NAME_LEN * 2 + 1];
  char pathname[MAX_PATHNAME_LEN + MAX_PROGRAM_NAME_LEN + 1];
  int mode = 0;
  DESCRIPTOR pathname_descr;

  descr = e_stack - 1;
  while (descr->type == ADDR)
    descr = descr->data.d_addr;

  descr = e_stack - 1; /* Replace e-stack item, not final descr */
  if (k_get_c_string(descr, call_name, MAX_PROGRAM_NAME_LEN) > 0) {
    UpperCaseString(call_name);
    (void)map_t1_id(call_name, strlen(call_name), mapped_name);

    if (strchr("*$_!", call_name[0]) == NULL) { /* No prefix */
      /* Try local catalogue (mode 1) */

      /* Push arguments onto e-stack */

      k_put_c_string(call_name, e_stack++);
      InitDescr(e_stack, ADDR); /* File path name (output) */
      (e_stack++)->data.d_addr = &pathname_descr;
      pathname_descr.type = UNASSIGNED;

      k_recurse(pcode_voc_cat, 2); /* Execute recursive code */

      /* Extract result string */
      if (pathname_descr.data.str.saddr != NULL)
        mode = 1;
      k_release(&pathname_descr);

      if (mode == 0) {
        /* Try private catalogue */
        /* converted to snprintf() -gwb 22Feb20 */
        /* Fix for Issue #12 - 11Jan22 */
        if (snprintf(pathname, (MAX_PATHNAME_LEN + MAX_PROGRAM_NAME_LEN) + 1, "%s%c%s",
                     private_catalogue, DS,
                     mapped_name) >= (MAX_PATHNAME_LEN + 1)) {
          /* TODO: this should also be logged with more detail */
          k_error("Overflowed path/filename length in op_chkcat())!");
          /* TODO: this should trigger some kind of error condition */
        }
        if (access(pathname, 0) == 0)
          mode = 2;
      }
    }

    if (mode == 0) {
      /* Try global catalog */
      /* converted to snprintf() -gwb 22Feb20 */
      /* Fix for Issue #12 - 11Jan22*/
      if (snprintf(pathname, (MAX_PATHNAME_LEN + MAX_PROGRAM_NAME_LEN) + 1, "%s%cgcat%c%s",
                   sysseg->sysdir, DS, DS,
                   mapped_name) >= (MAX_PATHNAME_LEN + 1)) {
        /* TODO: this should also be logged with more detail */
        k_error("Overflowed path/filename length in op_chkcat()!");
        /* TODO: this should trigger some kind of error condition. */
      }
      if (access(pathname, 0) == 0)
        mode = 3;
    }
  }

  k_dismiss(); /* Dismiss call name */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = mode;
}

/* ======================================================================
   op_enter()  -  Pick style ENTER                                        */

void op_enter() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  Program name or SUBR link     |                             |
     |================================|=============================|
 */

  DESCRIPTOR* descr;
  char call_name[MAX_PROGRAM_NAME_LEN + 1];

  descr = e_stack - 1;
  k_get_value(descr);

  switch (descr->type) {
    case SUBR:
      /* Convert SUBR reference back to a STRING */
      InitDescr(descr, STRING);
      --(((OBJECT_HEADER*)(descr->data.subr.object))->ext_hdr.prog.refs);
      descr->data.str.saddr = descr->data.subr.saddr;

      /* **** FALL THROUGH **** */

    case STRING:
      if (k_get_c_string(descr, call_name, MAX_PROGRAM_NAME_LEN) <= 0) {
        k_illegal_call_name();
      }
      break;

    default:
      k_error(sysmsg(1129));
  }

  UpperCaseString(call_name);

  if (!valid_call_name(call_name))
    k_illegal_call_name();
  if (call_name[0] == '$') /* Only callable by internal mode code */
  {
    if (!(process.program.flags & HDR_INTERNAL))
      k_illegal_call_name();
  }

  k_dismiss(); /* Dismiss call name */

  if (my_uptr->lockwait_index)
    clear_lockwait();

  /* Cast off all programs down to but not including one with
    the HDR_IS_CPROC flag set.                               */

  while (!(process.program.flags & HDR_IS_CPROC)) {
    k_return();
  }

  k_call(call_name, 0, NULL, 0);

  return;
}

/* ======================================================================
   op_gosub()  -  Enter local subroutine                                  */

void op_gosub() {
  if (my_uptr->events)
    process_events();

  if (process.program.gosub_depth == MAX_GOSUB_DEPTH) {
    k_error(sysmsg(1133));
  }
  process.program.gosub_stack[process.program.gosub_depth++] = pc - c_base + 3;
  pc = c_base +
       (*pc | (((int32_t) * (pc + 1)) << 8) | (((int32_t) * (pc + 2)) << 16));
}

/* ======================================================================
   op_jfalse()  -  Jump if false                                          */

void op_jfalse() {
  DESCRIPTOR* descr;
  register bool jumping;

  if (my_uptr->events)
    process_events();

  descr = e_stack - 1;
  GetBool(descr);
  jumping = (descr->data.value == 0);
  k_pop(1);

  if (jumping) {
    pc = c_base +
         (*pc | (((int32_t) * (pc + 1)) << 8) | (((int32_t) * (pc + 2)) << 16));
  } else
    pc += 3;
}

/* ======================================================================
   op_jmp()  -  Unconditional jump                                        */

void op_jmp() {
  if (my_uptr->events)
    process_events();

  pc = c_base +
       (*pc | (((int32_t) * (pc + 1)) << 8) | (((int32_t) * (pc + 2)) << 16));
}

/* ======================================================================
   op_jng()  -  Jump if negative                                          */

void op_jng() {
  DESCRIPTOR* descr;
  register bool jumping;

  if (my_uptr->events)
    process_events();

  descr = e_stack - 1;

again:
  switch (descr->type) {
    case INTEGER:
      jumping = (descr->data.value < 0);
      break;

    case FLOATNUM:
      jumping = (descr->data.float_value < 0.0);
      break;

    default:
      GetNum(descr);
      goto again;
  }

  k_pop(1);

  if (jumping) {
    pc = c_base +
         (*pc | (((int32_t) * (pc + 1)) << 8) | (((int32_t) * (pc + 2)) << 16));
  } else
    pc += 3;
}

/* ======================================================================
   op_jnz()  -  Jump if non-zero                                          */

void op_jnz() {
  DESCRIPTOR* descr;
  register bool jumping;

  if (my_uptr->events)
    process_events();

  descr = e_stack - 1;

again:
  switch (descr->type) {
    case INTEGER:
      jumping = (descr->data.value != 0);
      break;

    case FLOATNUM:
      jumping = (descr->data.float_value != 0.0);
      break;

    default:
      GetNum(descr);
      goto again;
  }

  k_pop(1);

  if (jumping) {
    pc = c_base +
         (*pc | (((int32_t) * (pc + 1)) << 8) | (((int32_t) * (pc + 2)) << 16));
  } else
    pc += 3;
}

/* ======================================================================
   op_jpo()  -  Jump if strictly positive                                 */

void op_jpo() {
  DESCRIPTOR* descr;
  register bool jumping;

  if (my_uptr->events)
    process_events();

  descr = e_stack - 1;

again:
  switch (descr->type) {
    case INTEGER:
      jumping = (descr->data.value > 0);
      break;

    case FLOATNUM:
      jumping = (descr->data.float_value > 0.0);
      break;

    default:
      GetNum(descr);
      goto again;
  }

  k_pop(1);

  if (jumping) {
    pc = c_base +
         (*pc | (((int32_t) * (pc + 1)) << 8) | (((int32_t) * (pc + 2)) << 16));
  } else
    pc += 3;
}

/* ======================================================================
   op_jpz()  -  Jump if positive or zero                                  */

void op_jpz() {
  DESCRIPTOR* descr;
  register bool jumping;

  if (my_uptr->events)
    process_events();

  descr = e_stack - 1;

again:
  switch (descr->type) {
    case INTEGER:
      jumping = (descr->data.value >= 0);
      break;

    case FLOATNUM:
      jumping = (descr->data.float_value >= 0.0);
      break;

    default:
      GetNum(descr);
      goto again;
  }

  k_pop(1);

  if (jumping) {
    pc = c_base +
         (*pc | (((int32_t) * (pc + 1)) << 8) | (((int32_t) * (pc + 2)) << 16));
  } else
    pc += 3;
}

/* ======================================================================
   op_jtrue()  -  Jump if true                                            */

void op_jtrue() {
  DESCRIPTOR* descr;
  register bool jumping;

  if (my_uptr->events)
    process_events();

  descr = e_stack - 1;
  GetBool(descr);
  jumping = (descr->data.value != 0);
  k_pop(1);

  if (jumping) {
    pc = c_base +
         (*pc | (((int32_t) * (pc + 1)) << 8) | (((int32_t) * (pc + 2)) << 16));
  } else
    pc += 3;
}

/* ======================================================================
   op_jze()  -  Jump if zero                                              */

void op_jze() {
  DESCRIPTOR* descr;
  register bool jumping;

  if (my_uptr->events)
    process_events();

  descr = e_stack - 1;

again:
  switch (descr->type) {
    case INTEGER:
      jumping = (descr->data.value == 0);
      break;

    case FLOATNUM:
      jumping = (descr->data.float_value == 0.0);
      break;

    default:
      GetNum(descr);
      goto again;
  }

  k_pop(1);

  if (jumping) {
    pc = c_base +
         (*pc | (((int32_t) * (pc + 1)) << 8) | (((int32_t) * (pc + 2)) << 16));
  } else
    pc += 3;
}

/* ======================================================================
   op_loadobj()  -  Load object code but do not execute                   */

void op_loadobj() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  Addr to subr name             | Program flags               |
     |================================|=============================|
 */

  DESCRIPTOR* descr;
  char call_name[MAX_PROGRAM_NAME_LEN + 1];
  struct OBJECT_HEADER* hdr;

  /* Find subroutine name - Always an ADDR */

  descr = e_stack - 1;
  while (descr->type == ADDR)
    descr = descr->data.d_addr;

  switch (descr->type) {
    case STRING:
      if (k_get_c_string(descr, call_name, MAX_PROGRAM_NAME_LEN) <= 0) {
        k_illegal_call_name();
      }

      UpperCaseString(call_name);

      if (!valid_call_name(call_name))
        k_illegal_call_name();

      if (call_name[0] == '$') /* Only callable by internal mode code */
      {
        if (!(process.program.flags & HDR_INTERNAL))
          k_illegal_call_name();
      }

      hdr = (OBJECT_HEADER*)load_object(call_name, FALSE);
      if (hdr == NULL)
        k_error(sysmsg(1134), call_name);
      hdr->ext_hdr.prog.refs++;

      /* Convert descriptor to become a SUBR.  Convert type, copy string
         pointer, set object address. We need to re-evaluate the descriptor
         address as the call to load_object() could move the e-stack.      */

      descr = e_stack - 1;
      while (descr->type == ADDR)
        descr = descr->data.d_addr;

      descr->type = SUBR; /* Preserve flags (e.g. DF_SYSTEM) */
      descr->data.subr.saddr = s_make_contiguous(descr->data.str.saddr, NULL);
      descr->data.subr.object = (u_char*)hdr;
      break;

    case SUBR:
      hdr = (OBJECT_HEADER*)(descr->data.subr.object);
      break;

    default:
      k_error(sysmsg(1129));
  }

  k_dismiss(); /* Dismiss call name */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = hdr->flags;
}

/* ======================================================================
   op_ongosub()  -  Computed gosub                                        */

void op_ongosub() {
  computed_jump(TRUE, FALSE);
}

/* ======================================================================
   op_ongosubp()  -  Computed gosub (Pick style)                          */

void op_ongosubp() {
  computed_jump(TRUE, TRUE);
}

/* ======================================================================
   op_ongoto()  -  Computed goto                                          */

void op_ongoto() {
  computed_jump(FALSE, FALSE);
}

/* ======================================================================
   op_ongotop()  -  Computed goto (Pick style)                            */

void op_ongotop() {
  computed_jump(FALSE, TRUE);
}

/* ======================================================================
   op_return()  -  Return from CALL or GOSUB                              */

void op_return() {
  if (my_uptr->events)
    process_events();

  if (process.program.gosub_depth) /* Returning from local call (GOSUB) */
  {
    pc = c_base + process.program.gosub_stack[--process.program.gosub_depth];
  } else /* Returning from a CALL */
  {
    k_return();
  }
}

/* ======================================================================
   op_returnto()  -  Return from GOSUB to specific address                */

void op_returnto() {
  if (my_uptr->events)
    process_events();

  if (process.program.gosub_depth) /* Returning from local call (GOSUB) */
  {
    --process.program.gosub_depth;
    pc = c_base +
         (*pc | (((int32_t) * (pc + 1)) << 8) | (((int32_t) * (pc + 2)) << 16));
  } else /* Returning from a CALL */
  {
    k_return();
  }
}

/* ======================================================================
   op_run()  -  Enter runfile                                             */

void op_run() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  Runfile name                  |                             |
     |================================|=============================|
 */

  DESCRIPTOR* descr;
  char runfile_name[MAX_PROGRAM_NAME_LEN + 1];

  descr = e_stack - 1;
  if (k_get_c_string(descr, runfile_name, MAX_PROGRAM_NAME_LEN) < 1) {
    k_error(sysmsg(1135));
  }
  k_dismiss();

  k_call(runfile_name, 0, NULL, 0);

  return;
}

/* ======================================================================
   computed_jump()  -  Common path for op_ongoto() and op_ongosub()       */

Private void computed_jump(
    bool save_link,
    bool pick_style) /* Affects action of out of range index value */
{
  DESCRIPTOR* descr;
  int16_t num_labels;
  int32_t n;

  num_labels = *(pc++);
  if (num_labels == 0) {
    num_labels = (int16_t)(*pc | (((int32_t) * (pc + 1)) << 8));
    pc += 2;
  }

  descr = e_stack - 1;
  GetInt(descr);
  n = descr->data.value;
  k_dismiss();

  if (pick_style) {
    if ((n < 1) || (n > num_labels)) {
      pc += num_labels * 3;
      return;
    }
  } else {
    if (n < 1)
      n = 1;
    else if (n > num_labels)
      n = num_labels;
  }

  if (save_link) {
    if (process.program.gosub_depth == MAX_GOSUB_DEPTH)
      k_error(sysmsg(1133));

    process.program.gosub_stack[process.program.gosub_depth++] =
        pc - c_base + (num_labels * 3);
  }

  pc += (n - 1) * 3;
  pc = c_base +
       (*pc | (((int32_t) * (pc + 1)) << 8) | (((int32_t) * (pc + 2)) << 16));
}

/* ======================================================================
   valid_call_name()  -  Validate a subroutine name                       */

bool valid_call_name(char* p) {
  char c;
  char valid_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.%$-_";
  char leading_chars[] = "*$!_";

  /* !!CALLNAME!! */

  if (strlen(p) > MAX_CALL_NAME_LEN)
    return FALSE;

  c = *(p++);
  if (c == '\0')
    return FALSE;

  if ((strchr(valid_chars, c) == NULL) && (strchr(leading_chars, c) == NULL)) {
    return FALSE;
  }

  while ((c = *(p++)) != '\0') {
    if (strchr(valid_chars, c) == NULL)
      return FALSE;
  }

  return TRUE;
}

/* END-CODE */
