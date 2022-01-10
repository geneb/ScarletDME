/* OP_MISC.C
 * Miscellaneous opcodes
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
 * ScarletDME Wiki: https://scarlet.deltasoft.com
 * 
 * START-HISTORY (ScarletDME):
 * 09Jan22 gwb Fixed a cast warning. (search for 09Jan22 for details)
 *             Fixed a format specifier warning.
 *
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 *
 * 22Feb20 gwb Corrected an uninitialized variable issue in op_umask().
 *             I'm initializing it to 0, which is going to be better than
 *             the potentially random number that could be there when the
 *             routine is called.
 * 
 * START-HISTORY (OpenQM):
 * 08 Nov 07  2.6-5 Removed op_exch(), relinking to indentical op_swap().
 * 03 Sep 07  2.6-1 0560 In itype(), handle difference between I-type and C-type
 *                  when dismissing object code copy for word alignment.
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 13 May 07  2.5-4 Use 1 Jan 0001 as internal day 0 during date conversion.
 * 01 May 07  2.5-3 0549 Reworked day_to_dmy() to handle dates prior to 1900
 *                  correctly.
 * 29 Jan 07  2.4-20 Copy I-type object code if not word aligned.
 * 02 Nov 06  2.4-15 In op_itype, dictionary record types are now case
 *                   insensitive.
 * 25 Aug 06  2.4-12 Allow use of ITYPE() with non-compiled dictionary items.
 * 30 May 06  2.4-5 0493 op_xtd() should use u_char data.
 * 14 Apr 06  2.4-1 Added op_pause() and op_wake().
 * 07 Apr 06  2.4-1 Added op_rtrans() and associated change to op_trans().
 * 08 Feb 06  2.3-6 0460 Handling of string format sleep time was fundamentally
 *                  wrong.
 * 30 Jan 06  2.3-5 0456 Corrected leap year determination rules.
 * 23 Nov 05  2.2-17 Exported time functions to time.c
 * 19 Oct 05  2.2-15 Moved op_csvdq() to op_str5.c
 * 22 Sep 05  2.2-12 0413 Added recursive call to CHAIN for PROC handling.
 * 01 Sep 05  2.2-9 Added op_getmsg.
 * 13 May 05  2.1-15 Added error arg to s_make_contiguous().
 * 23 Dec 04  2.1-0 Added op_procread().
 * 07 Dec 04  2.1-0 Added support for C-types in op_itype().
 * 28 Oct 04  2.0-8 0276 Rewrote FOR/NEXT opcodes to handle non-integer values.
 * 20 Sep 04  2.0-2 Use message handler.
 * 20 Sep 04  2.0-2 Use dynamic pcode loader.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * Opcodes:
 *    op_abtcause       ABTCAUSE       ABORT.CAUSE() function
 *    op_argct          ARGCT()
 *    op_ascii          ASCII
 *    op_chain          CHAIN
 *    op_clrmode        CLRMODE
 *    op_date()         DATE
 *    op_ebcdic()       EBCDIC
 *    op_pwcrypt()      PWCRYPT()
 *    op_env()          ENV()
 *    op_exch           EXCH
 *    op_forinit        FORINIT
 *    op_getmsg         GETMSG         GET.MESSAGES() function
 *    op_issubr         ISSUBR         IS.SUBR() function
 *    op_itype          ITYPE
 *    op_itype2         ITYPE2         ITYPE(), 2 arg version (query processor)
 *    op_nap            NAP
 *    op_null           NULL
 *    op_ojoin          OJOIN          Outer join
 *    op_onerror        ONERROR
 *    op_oserror        OS.ERROR()
 *    op_precision      PRECISION
 *    op_procread       PROCREAD
 *    op_rtrans()       RTRANS()
 *    op_saveaddr                       Save address descriptor
 *    op_sendmail       SENDMAIL()
 *    op_setmode        SETMODE
 *    op_setstat        SETSTAT
 *    op_sleep          SLEEP
 *    op_status         STATUS          STATUS() function
 *    op_time()         TIME
 *    op_timedate()     TIMEDATE
 *    op_total()        TOTAL
 *    op_trans()        TRANS()
 *    op_vartype()      VARTYPE         VARTYPE() function
 *
 * Other externally callable functions:
 *    day_to_dmy()      Convert day number to day, month, year
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include "header.h"
#include "syscom.h"
#include "tio.h"
#include "config.h"

#include <math.h>
#include <time.h>

#define _timezone timezone
#include <signal.h>

bool qmsendmail(char* sender,
                char* recipients,
                char* cc_recipients,
                char* bcc_recipients,
                char* subject,
                char* text,
                char* attachments);

extern time_t clock_time;
extern char* month_names[];
Private int32_t date_adjustment = 0;
Private int16_t tbase; /* Base of TOTAL() accumulator array,
                                   zero if not to be used              */

Private void for1(bool store_before_test);
Private void forloop(bool store_before_test);
Private void itype(void);

/* ======================================================================
   op_abtcause()  -  ABTCAUSE  -  Return and clear abort cause            */

void op_abtcause() {
  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = process.k_abort_code;
  process.k_abort_code = 0;
}

/* ======================================================================
   op_argct()  -  ARGCT()  -  Return count of passed arguments            */

void op_argct() {
  int16_t arg_ct;

  arg_ct = process.program.arg_ct;
  if (process.program.flags & HDR_IS_FUNCTION)
    arg_ct--; /* Hide return arg */
  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = arg_ct;
}

/* ======================================================================
   op_ascii()  -  ASCII() function                                        */

void op_ascii() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  String to convert          | Converted string            |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;
  STRING_CHUNK* str;
  u_char* p;
  int16_t bytes;
  static u_char table[] = {
      /*0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F */
      0x00, 0x01, 0x02, 0x03, 0x3F, 0x09, 0x3F, 0x7F, 0x3F, 0x3F, 0x3F, 0x0B,
      0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x3F, 0x3F, 0x08, 0x3F,
      0x18, 0x19, 0x3F, 0x3F, 0x1C, 0x1D, 0x1E, 0x1F, 0x3F, 0x3F, 0x3F, 0x3F,
      0x3F, 0x0A, 0x17, 0x1B, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x05, 0x06, 0x07,
      0x3F, 0x3F, 0x16, 0x3F, 0x3F, 0x3F, 0x3F, 0x04, 0x3F, 0x3F, 0x3F, 0x3F,
      0x14, 0x15, 0x3F, 0x1A, 0x20, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
      0x3F, 0x3F, 0x3F, 0x2E, 0x3C, 0x28, 0x2B, 0x3F, 0x26, 0x3F, 0x3F, 0x3F,
      0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x21, 0x24, 0x2A, 0x29, 0x3B, 0x5E,
      0x2D, 0x2F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x7C, 0x2C,
      0x25, 0x5F, 0x3E, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
      0x3F, 0x60, 0x3A, 0x23, 0x40, 0x27, 0x3D, 0x22, 0x5B, 0x61, 0x62, 0x63,
      0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
      0x5D, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x3F, 0x3F,
      0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x7E, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
      0x79, 0x7A, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
      0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
      0x7B, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x3F, 0x3F,
      0x3F, 0x3F, 0x3F, 0x3F, 0x7D, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50,
      0x51, 0x52, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x5C, 0x3F, 0x53, 0x54,
      0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
      0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3F, 0x3F,
      0x3F, 0x3F, 0x3F, 0x3F};

  descr = e_stack - 1;
  k_get_string(descr);

  str = descr->data.str.saddr;

  if (str != NULL) {
    if (str->ref_ct > 1) {
      str->ref_ct--;
      str = k_copy_string(descr->data.str.saddr);
      descr->data.str.saddr = str;
    }

    /* Clear any hint */

    str->field = 0;

    descr->flags &= ~DF_CHANGE;

    do {
      p = (u_char*)(str->data);
      bytes = str->bytes;
      while (bytes--) {
        *p = table[*p];
        p++;
      }
    } while ((str = str->next) != NULL);
  }
}

/* ======================================================================
   op_chain()  -  CHAIN                                                   */

void op_chain() {
  k_recurse(pcode_chain, 0);
  if ((e_stack - 1)->data.value)
    k_exit_cause = K_CHAIN_PROC;
  else
    k_exit_cause = K_CHAIN;
  k_pop(1);
}

/* ======================================================================
   op_clrmode()  -  CLRMODE  -  Clear program flag bits (restricted)      */

void op_clrmode() {
  DESCRIPTOR* descr;

  descr = e_stack - 1;
  GetInt(descr);
  process.program.flags &= ~((u_int32_t)(descr->data.value));
  k_pop(1);
}

/* ======================================================================
   op_date()  -  Return date as integer                                   */

void op_date() {
  /* The windows time uses 00:00:00 on 1/1/70 as its datum. We
    must adjust this so that 31/12/67 is day zero.                      */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = ((local_time() + date_adjustment) / 86400L) + 732;
}

/* ======================================================================
   op_dtx()  -  DTX() function                                            */

void op_dtx() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Minimum width              | Hexadecimal string          |
     |-----------------------------|-----------------------------|
     |  Value to convert           |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;
  char value[8 + 1];
  char s[32 + 1];
  int n;
  int min_width;
  char* p;

  /* Get width */

  descr = e_stack - 1;
  GetInt(descr);
  min_width = descr->data.value;
  if (min_width > 32)
    min_width = 32; /* Sanity check */
  k_pop(1);

  /* Get and convert value */

  descr = e_stack - 1;
  GetInt(descr);
  n = sprintf(value, "%d", descr->data.value);

  p = s;
  if (n < min_width) {
    memset(p, '0', min_width - n);
    p += min_width - n;
  }

  strcpy(p, value);
  k_put_c_string(s, descr);
}

/* ======================================================================
   op_ebcdic()  -  EBCDIC() function                                      */

void op_ebcdic() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  String to convert          | Converted string            |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;
  STRING_CHUNK* str;
  u_char* p;
  int16_t bytes;
  register u_char c;
  static u_char table[] = {
      /*0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F */
      0x00, 0x01, 0x02, 0x03, 0x37, 0x2D, 0x2E, 0x2F, 0x16, 0x05, 0x25, 0x0B,
      0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x3C, 0x3D, 0x32, 0x26,
      0x18, 0x19, 0x3F, 0x27, 0x1C, 0x1D, 0x1E, 0x1F, 0x40, 0x5A, 0x7F, 0x7B,
      0x5B, 0x6C, 0x50, 0x7D, 0x4D, 0x5D, 0x5C, 0x4E, 0x6B, 0x60, 0x4B, 0x61,
      0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0x7A, 0x5E,
      0x4C, 0x7E, 0x6E, 0x6F, 0x7C, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
      0xC8, 0xC9, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xE2,
      0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0x80, 0xE0, 0x90, 0x5F, 0x6D,
      0x79, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x91, 0x92,
      0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6,
      0xA7, 0xA8, 0xA9, 0xC0, 0x6A, 0xD0, 0xA1, 0x07};

  descr = e_stack - 1;
  k_get_string(descr);

  str = descr->data.str.saddr;
  if (str != NULL) {
    if (str->ref_ct > 1) {
      str->ref_ct--;
      str = k_copy_string(descr->data.str.saddr);
      descr->data.str.saddr = str;
    }

    /* Clear any hint */

    str->field = 0;

    descr->flags &= ~DF_CHANGE;

    do {
      p = (u_char*)(str->data);
      bytes = str->bytes;
      while (bytes--) {
        if (!((c = *p) & 0x80))
          *p = table[c];
        p++;
      }
    } while ((str = str->next) != NULL);
  }
}

/* ======================================================================
   op_pwcrypt()  -  Encrypt a string (One way)                            */

void op_pwcrypt() {
  DESCRIPTOR* descr;
  STRING_CHUNK* str;
  int32_t a = 1;
  double n;
  int i = 1;
  u_char* p;
  u_char result[16 + 1];

  descr = e_stack - 1;
  k_get_string(descr);
  str = descr->data.str.saddr;

  if (str != NULL) {
    do {
      n = str->bytes;
      p = (u_char*)(str->data);
      while (n--)
        a = (int32_t)sqrt(a * i++ * *(p++));
    } while ((str = str->next) != NULL);

    srand((int32_t)a);

    for (i = 1, p = result; i <= 16; i++)
      *(p++) = 33 + (rand() % 94);
    *p = '\0';

    k_dismiss();
    InitDescr(e_stack, STRING);
    k_put_c_string((char*)result, e_stack++);
  }
}

/* ======================================================================
   op_env()  -  Get environment variable                                  */

void op_env() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Variable name              | Variable value              |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;
  char env_var_name[64 + 1];
  char* p;

  descr = e_stack - 1;
  if (k_get_c_string(descr, env_var_name, 64) < 0)
    p = NULL;
  else
    p = getenv(env_var_name);

  k_dismiss();
  k_put_c_string(p, e_stack++);
}

/* ======================================================================
   op_forinit()  -  Initialise FOR loop                                   */

void op_forinit() {
  process.for_init = TRUE;
}

/* ======================================================================
   op_forloop()  -  Increment and test FOR loop control variable
   op_forloops() -  As OP_FORLOOP but store value before test             */

void op_forloop() {
  forloop(FALSE);
}

void op_forloops() {
  forloop(TRUE);
}

Private void forloop(bool store_before_test) {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Step value                 |                             |
     |-----------------------------|-----------------------------|
     |  Limit value                |                             |
     |-----------------------------|-----------------------------|
     |  ADDR to control var        |                             |
     |-----------------------------|-----------------------------|
     |  Initial value. Only        |                             |
     |  present if for_init set    |                             |
     |=============================|=============================|

     Opcode is followed by exit jump address
 */

  DESCRIPTOR* descr;
  DESCRIPTOR* control_descr;
  int32_t control_value;
  double fcontrol_value;
  int32_t limit;
  double flimit;
  int32_t step;
  double fstep;
  bool is_float = FALSE;
  bool again;

  /* Find control var */

  control_descr = e_stack - 3;
  do {
    control_descr = control_descr->data.d_addr;
  } while (control_descr->type == ADDR);

  /* Get step value */

  descr = e_stack - 1;
  GetNum(descr);
  if (descr->type == INTEGER) {
    step = descr->data.value;
  } else /* Must be FLOATNUM */
  {
    is_float = TRUE;
    fstep = descr->data.float_value;
  }

  /* Get limit value */

  descr = e_stack - 2;
  GetNum(descr);
  if (descr->type == INTEGER) {
    if (is_float)
      flimit = descr->data.value;
    else
      limit = descr->data.value;
  } else /* Must be FLOATNUM */
  {
    if (!is_float)
      fstep = step;
    is_float = TRUE;
    flimit = descr->data.float_value;
  }

  if (process.for_init) /* Initialising first iteration */
  {
    descr = e_stack - 4;
    GetNum(descr);
    /* Note that we do not release this descriptor until we have determined
      the type rules as a user could write "FOR X = X TO Y"                */

    if (descr->type == INTEGER) {
      if (is_float) /* Step or limit is float */
      {
        fcontrol_value = descr->data.value;
        Release(control_descr);
        InitDescr(control_descr, FLOATNUM);
        control_descr->data.float_value = fcontrol_value;
      } else /* Everything is integer */
      {
        control_value = descr->data.value;
        Release(control_descr);
        InitDescr(control_descr, INTEGER);
        control_descr->data.value = control_value;
      }
    } else /* Must be FLOATNUM */
    {
      fcontrol_value = descr->data.float_value;
      Release(control_descr);
      InitDescr(control_descr, FLOATNUM);
      control_descr->data.float_value = fcontrol_value;

      if (!is_float) /* Step and limit are both integer */
      {
        fstep = step;
        flimit = limit;
        is_float = TRUE;
      }
    }

    k_pop(4);

    process.for_init = FALSE;
  } else /* Not first iteration */
  {
    GetNum(control_descr);
    if (control_descr->type == INTEGER) {
      if (is_float) {
        fcontrol_value = ((double)(control_descr->data.value)) + fstep;
      } else {
        control_value = control_descr->data.value + step;
      }
    } else /* Must be FLOATNUM */
    {
      if (is_float) {
        fcontrol_value = control_descr->data.float_value + fstep;
      } else /* Step is integer */
      {
        fstep = step;
        flimit = limit;
        fcontrol_value = control_descr->data.float_value + fstep;
        is_float = TRUE;
      }
    }
    k_pop(3);
  }

  /* Test limit condition */

  if (is_float) {
    if (fstep > 0)
      again = (fcontrol_value - pcfg.fltdiff <= flimit);
    else
      again = (fcontrol_value + pcfg.fltdiff >= flimit);
    if (store_before_test || again) {
      Release(control_descr); /* Might have changed type inside loop! */
      InitDescr(control_descr, FLOATNUM);
      control_descr->data.float_value = fcontrol_value;
    }
  } else {
    if (step > 0)
      again = (control_value <= limit);
    else
      again = (control_value >= limit);
    if (store_before_test || again) {
      Release(control_descr); /* Might have changed type inside loop! */
      InitDescr(control_descr, INTEGER);
      control_descr->data.value = control_value;
    }
  }

  if (again) {
    pc += 3;
  } else {
    pc = c_base + (*pc | (((int32_t)*(pc + 1)) << 8) | (((int32_t)*(pc + 2)) << 16));
  }
}

/* ======================================================================
   op_for1()  -  Increment and test FOR loop control variable
   op_for1s   -  As FOR1 but store value before test                      */

void op_for1() {
  for1(FALSE);
}

void op_for1s() {
  for1(TRUE);
}

Private void for1(bool store_before_test) {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Limit value                |                             |
     |-----------------------------|-----------------------------|
     |  ADDR to control var        |                             |
     |-----------------------------|-----------------------------|
     |  Initial value. Only        |                             |
     |  present if for_init set    |                             |
     |=============================|=============================|

     Opcode is followed by exit jump address
 */

  DESCRIPTOR* descr;
  DESCRIPTOR* control_descr;
  int32_t control_value;
  double fcontrol_value;
  int32_t limit;
  double flimit;
  bool is_float = FALSE;
  bool again;

  /* Find control var */

  control_descr = e_stack - 2;
  do {
    control_descr = control_descr->data.d_addr;
  } while (control_descr->type == ADDR);

  /* Get limit value */

  descr = e_stack - 1;
  GetNum(descr);
  if (descr->type == INTEGER) {
    limit = descr->data.value;
  } else /* Must be FLOATNUM */
  {
    is_float = TRUE;
    flimit = descr->data.float_value;
  }

  if (process.for_init) /* Initialising first iteration */
  {
    descr = e_stack - 3;
    GetNum(descr);

    /* Note that we do not release this descriptor until we have determined
      the type rules as a user could write "FOR X = X TO Y"                */

    if (descr->type == INTEGER) {
      if (is_float) /* Limit is float */
      {
        fcontrol_value = descr->data.value;
        Release(control_descr);
        InitDescr(control_descr, FLOATNUM);
        control_descr->data.float_value = fcontrol_value;
      } else /* Everything is integer */
      {
        control_value = descr->data.value;
        Release(control_descr);
        InitDescr(control_descr, INTEGER);
        control_descr->data.value = control_value;
      }
    } else /* Must be FLOATNUM */
    {
      fcontrol_value = descr->data.float_value;
      Release(control_descr);
      InitDescr(control_descr, FLOATNUM);
      control_descr->data.float_value = fcontrol_value;

      if (!is_float) /* Limit is integer */
      {
        flimit = limit;
        is_float = TRUE;
      }
    }

    k_pop(3);
    process.for_init = FALSE;
  } else /* Not first iteration */
  {
    GetNum(control_descr);
    if (control_descr->type == INTEGER) {
      if (is_float) {
        fcontrol_value = ((double)(control_descr->data.value)) + 1.0;
      } else {
        control_value = control_descr->data.value + 1;
      }
    } else /* Must be FLOATNUM */
    {
      if (is_float) {
        fcontrol_value = control_descr->data.float_value + 1.0;
      } else /* Limit is integer */
      {
        flimit = limit;
        fcontrol_value = control_descr->data.float_value + 1.0;
        is_float = TRUE;
      }
    }

    k_pop(2);
  }

  /* Test limit condition */

  if (is_float) {
    again = (fcontrol_value - pcfg.fltdiff <= flimit);

    if (store_before_test || again) {
      Release(control_descr); /* Might have changed type inside loop! */
      InitDescr(control_descr, FLOATNUM);
      control_descr->data.float_value = fcontrol_value;
    }
  } else {
    again = (control_value <= limit);

    if (store_before_test || again) {
      Release(control_descr); /* Might have changed type inside loop! */
      InitDescr(control_descr, INTEGER);
      control_descr->data.value = control_value;
    }
  }

  if (again) {
    pc += 3;
  } else {
    pc = c_base + (*pc | (((int32_t)*(pc + 1)) << 8) | (((int32_t)*(pc + 2)) << 16));
  }
}

/* ======================================================================
   op_getmsg()  -  GETMSG opcode                                          */

void op_getmsg() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |                             |  Messages                   |
     |=============================|=============================|
 */

  k_recurse(pcode_getmsg, 0);
}

/* ======================================================================
   op_issubr()  -  ISSUBR opcode                                          */

void op_issubr() {
  DESCRIPTOR* descr;
  bool is_subr;

  descr = e_stack - 1;
  k_get_value(descr);
  is_subr = (descr->type == SUBR);
  k_dismiss();

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = is_subr;
}

/* ======================================================================
   op_itype()  -  ITYPE opcode                                            */

void op_itype() {
  tbase = 0;
  itype();
}

void op_itype2() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  TOTAL() accumulator base   |  Result                     |
     |-----------------------------|-----------------------------|
     |  ADDR to object code        |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;

  descr = e_stack - 1;
  GetInt(descr);
  tbase = (int16_t)(descr->data.value);
  k_pop(1);

  itype();
}

Private void itype() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  ADDR to object code        |  Result                     |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;
  STRING_CHUNK* str;
  char* p;
  int16_t i;
  int n;
  u_char type;
  u_int16_t header_flags;
  OBJECT_HEADER* obj;

  /* Find I-type code.  An I-type must be referenced via an ADDR as we are
    going to pop the pointer and use the actual string.  Anything else would
    require that we dereference the string on leaving the I-type program.
    This implies that an I-type must be a variable or an array reference, not
    a field extraction, etc.                                                  */

  descr = e_stack - 1;
  if (descr->type != ADDR)
    k_error(sysmsg(1470));
  do {
    descr = descr->data.d_addr;
  } while (descr->type == ADDR);
  k_get_string(descr);
  descr->data.str.saddr = s_make_contiguous(descr->data.str.saddr, NULL);

  str = descr->data.str.saddr;
  if (str == NULL)
    goto inva_i_type;

  p = str->data;
  type = UpperCase(*p);

  if ((type == 'I') || (type == 'C') || (type == 'A') || (type == 'S')) {
    /* It's a complete dictionary record */
    for (i = 15; i--;) /* Skip 15 field marks */
    {
      p = strchr(p, FIELD_MARK);
      if (p == NULL)
        goto inva_i_type;
      p++;
    }
  }

  if (*p != HDR_MAGIC) {
    if (*p == HDR_MAGIC_INVERSE)
      convert_object_header((OBJECT_HEADER*)p);
    if (*p != HDR_MAGIC)
      goto inva_i_type;
  }

  k_pop(1);
  
  //if (((int)p) & 0x00000003) {
    if ((p-(char*)NULL) & 0x3) { /* 09Jan22 gwb - 64 bit fix.
                                  * I'm not sure what the origin of the 
                                  * if (((int)p).... code is, but the below
                                  * version of the if clears the "cast from pointer to
                                  * integer of different size" warning.
                                  */
    // if ((p-(char*)NULL) & 0x3) {  (this fixes the warning the above line triggers)
    /* Not word aligned - must make a copy. To ensure that this gets
      released at an abort, we push a string descriptor onto the stack */

    n = str->bytes - (p - str->data);
    k_put_string(p, n, e_stack);
    obj = (OBJECT_HEADER*)((e_stack++)->data.str.saddr->data);
    header_flags = obj->flags;
    k_recurse((u_char*)obj, 0);
    if (header_flags & HDR_CTYPE) /* 0560 */
    {
      k_release(e_stack - 1);
    } else {
      k_release(e_stack - 2);
      *(e_stack - 2) = *(e_stack - 1);
    }
    e_stack--;
  } else {
    header_flags = ((OBJECT_HEADER*)p)->flags;
    k_recurse((u_char*)p, 0);
  }

  /* If this is a C-type, load @ANS on to the stack */

  if (header_flags & HDR_CTYPE) {
    *e_stack = *(Element(process.syscom, SYSCOM_AT_ANS));
    if (e_stack->type >= COMPLEX_DESCR)
      k_incr_refct(e_stack);
    e_stack++;
  }

  return;

inva_i_type:
  k_recurse(pcode_itype, 1);
  if (process.status == ER_INVA_ITYPE)
    k_error(sysmsg(1471));
}

/* ======================================================================
   op_saveaddr()  -  Save address descriptor                              */

void op_saveaddr() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Descriptor to save (ADDR)  |                             |
     |-----------------------------|-----------------------------|
     |  Where to save (ADDR)       |                             |
     |=============================|=============================|
 */

  *((e_stack - 2)->data.d_addr) = *(e_stack - 1);
  k_pop(2);
}

/* ======================================================================
   op_sendmail()  -  SENDMAIL()  Send email                               */

void op_sendmail() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Attachments                |                             |
     |-----------------------------|-----------------------------|
     |  Text                       |                             |
     |-----------------------------|-----------------------------|
     |  Subject                    |                             |
     |-----------------------------|-----------------------------|
     |  Bcc recipients             |                             |
     |-----------------------------|-----------------------------|
     |  Cc recipients              |                             |
     |-----------------------------|-----------------------------|
     |  Recipients                 |                             |
     |-----------------------------|-----------------------------|
     |  Sender                     |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;
  char* text;
  int32_t text_bytes;
  char* recipients = NULL;
  char* cc_recipients = NULL;
  char* bcc_recipients = NULL;
  int32_t bytes;
  char sender[100 + 1];
  char subject[255 + 1];
  char* attachments = NULL;

  process.status = 0;

  /* Attachments */

  descr = e_stack - 1;
  k_get_string(descr);
  if (descr->data.str.saddr != NULL) {
    bytes = descr->data.str.saddr->string_len;
    attachments = k_alloc(77, bytes + 1);
    k_get_c_string(descr, attachments, bytes);
  }

  /* Text */

  descr = e_stack - 2;
  k_get_string(descr);
  if (descr->data.str.saddr == NULL) {
    text_bytes = 0;
    text = null_string;
  } else {
    text_bytes = descr->data.str.saddr->string_len;
    text = k_alloc(76, text_bytes + 1);
    k_get_c_string(descr, text, text_bytes);
  }

  /* Subject */

  descr = e_stack - 3;
  if (k_get_c_string(descr, subject, 255) < 0) {
    process.status = ER_LENGTH;
    goto exit_op_sendmail;
  }

  /* Bcc recipients */

  descr = e_stack - 4;
  k_get_string(descr);
  if (descr->data.str.saddr != NULL) {
    bytes = descr->data.str.saddr->string_len;
    bcc_recipients = k_alloc(78, bytes + 1);
    k_get_c_string(descr, bcc_recipients, bytes);
  }

  /* Cc recipients */

  descr = e_stack - 5;
  k_get_string(descr);
  if (descr->data.str.saddr != NULL) {
    bytes = descr->data.str.saddr->string_len;
    cc_recipients = k_alloc(78, bytes + 1);
    k_get_c_string(descr, cc_recipients, bytes);
  }

  /* Recipients */

  descr = e_stack - 6;
  k_get_string(descr);
  if (descr->data.str.saddr != NULL) {
    bytes = descr->data.str.saddr->string_len;
    recipients = k_alloc(78, bytes + 1);
    k_get_c_string(descr, recipients, bytes);
  }

  /* Sender */

  descr = e_stack - 7;
  if (k_get_c_string(descr, sender, 100) < 0) {
    process.status = ER_LENGTH;
    goto exit_op_sendmail;
  }

  if (!qmsendmail(sender, recipients, cc_recipients, bcc_recipients, subject,
                  text, attachments)) {
    /* process.status and possibly process.os_error set by qmsendmail */
  }

exit_op_sendmail:
  if (text_bytes)
    k_free(text);
  if (recipients != NULL)
    k_free(recipients);
  if (cc_recipients != NULL)
    k_free(cc_recipients);
  if (bcc_recipients != NULL)
    k_free(bcc_recipients);
  if (attachments != NULL)
    k_free(attachments);

  k_dismiss(); /* Attachments */
  k_dismiss(); /* Text */
  k_dismiss(); /* Subject */
  k_dismiss(); /* Bcc recipients */
  k_dismiss(); /* Cc recipients */
  k_dismiss(); /* Recipients */
  k_dismiss(); /* Sender */
}

/* ======================================================================
   op_nap()  -  NAP opcode                                                */

void op_nap() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Interval (milliseconds)    |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;
  time_t wake_time;
  int32_t tm;

  descr = e_stack - 1;
  GetInt(descr);
  tm = descr->data.value;
  k_pop(1);

  if (tm > 0) {
    if (tm < 5000) {
      Sleep(tm);
    } else {
      clock_time = time(NULL);
      wake_time = clock_time + (tm / 1000);

      while ((clock_time = time(NULL)) < wake_time) {
        if (my_uptr->events)
          process_events();
        Sleep(1000);
        if ((process.break_inhibits == 0) && !recursion_depth && break_pending)
          break;
      }
    }
  }

  return;
}

/* ======================================================================
   op_null()  -  Null opcode                                              */

void op_null() {}

/* ======================================================================
   op_ojoin()  -  OUTERJOIN function                                      */

void op_ojoin() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Indexed value              |  Result                     |
     |-----------------------------|-----------------------------|
     |  Index name                 |                             |
     |-----------------------------|-----------------------------|
     |  VOC name of file (may have |                             |
     |  DICT prefix                |                             |
     |=============================|=============================|
 */

  k_recurse(pcode_ojoin, 3); /* Execute recursive code */
}

/* ======================================================================
   op_onerror()  -  Activate ON ERROR clause for various opcodes          */

void op_onerror() {
  process.op_flags |= P_ON_ERROR;
}

/* ======================================================================
   op_oserror()  -  OS.ERROR() function                                   */

void op_oserror() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |                             |  OS.ERROR() value           |
     |=============================|=============================|
 */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = process.os_error;
}

/* ======================================================================
   op_pause()  -  Pause until awoken                                      */

void op_pause() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Timeout (seconds)          |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;
  int32_t timeout;

  descr = e_stack - 1;
  GetInt(descr);
  timeout = descr->data.value;
  timeout = (timeout > 0) ? (local_time() + timeout) : 0;
  k_dismiss();

  while (!(my_uptr->flags & USR_WAKE)) {
    if (my_uptr->events)
      process_events();
    if (k_exit_cause & K_INTERRUPT)
      break;
    if ((process.break_inhibits == 0) && break_pending)
      break;
    sleep(1);
    if ((timeout) && (local_time() >= timeout)) {
      process.status = ER_TIMEOUT;
      goto exit_op_pause;
    }
  }
  process.status = 0;
  my_uptr->flags &= ~USR_WAKE;

exit_op_pause:
  return;
}

/* ======================================================================
   op_precision()  -  Set numeric to string precision                     */

void op_precision() {
  DESCRIPTOR* descr;
  int32_t precision;

  descr = e_stack - 1;
  GetInt(descr);
  precision = descr->data.value;

  if (precision < 0)
    precision = 0;
  else if (precision > 14)
    precision = 14;

  process.program.precision = (int16_t)precision;

  k_pop(1);
}

/* ======================================================================
   op_procread()  -  Read data from PROC primary input buffer             */

void op_procread() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  ADDR to target variable    | 0 = success, 1 = error      |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;
  DESCRIPTOR* ibuf_descr;
  struct PROGRAM* pgm;
  bool is_proc = FALSE;

  /* The semantics of this statement require that we check that we are
    in a PROC. The simplest thing to do is to wander down the call stack
    looking for $PROC.                                                   */

  for (pgm = process.program.prev; pgm != NULL; pgm = pgm->prev) {
    if (!strcmp(
            ((OBJECT_HEADER*)(pgm->saved_c_base))->ext_hdr.prog.program_name,
            "$PROC")) {
      is_proc = TRUE;
      break;
    }
  }

  descr = e_stack - 1;
  while (descr->type == ADDR)
    descr = descr->data.d_addr;
  Release(descr);

  if (is_proc) {
    ibuf_descr = Element(process.syscom, SYSCOM_PROC_IBUF);
    *descr = *(Element(ibuf_descr->data.ahdr_addr, 0));
    k_incr_refct(descr);
  } else {
    InitDescr(descr, STRING);
    descr->data.str.saddr = NULL;
  }

  k_dismiss();
  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = !is_proc;
}

/* ======================================================================
   op_rtrans()  -  RTRANS function (Revelation style, no mark lowering)   */

void op_rtrans() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Action code expression     |  Result                     |
     |-----------------------------|-----------------------------|
     |  Field number               |                             |
     |-----------------------------|-----------------------------|
     |  Record ID                  |                             |
     |-----------------------------|-----------------------------|
     |  VOC name of file (may have |                             |
     |  DICT prefix                |                             |
     |=============================|=============================|
 */

  /* Push flag controlling lowering of marks */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = FALSE;

  k_recurse(pcode_trans, 5); /* Execute recursive code */
}

/* ======================================================================
   op_setmode()  -  SETMODE  -  Set program flag bits (restricted)        */

void op_setmode() {
  DESCRIPTOR* descr;

  descr = e_stack - 1;
  GetInt(descr);
  process.program.flags |= descr->data.value;
  k_pop(1);
}

/* ======================================================================
   op_setstat()  -  SETSTAT opcode                                        */

void op_setstat() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Value to set               |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;

  descr = e_stack - 1;
  GetInt(descr);
  process.status = descr->data.value;
  k_pop(1);
}

/* ======================================================================
   op_sleep()  -  SLEEP opcode                                            */

void op_sleep() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Interval or time of day    |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;
  int32_t time_now;
  int32_t wake_time;

  time_now = local_time();

  descr = e_stack - 1;
  k_get_value(descr); /* 0460 */
  if (descr->type == STRING)
    (void)k_str_to_num(descr);

  if (descr->type == STRING) {
    if (!iconv_time_conversion()) /* Converted to internal format time of day */
    {
      GetInt(descr);
      wake_time = time_now - (time_now % 86400) + descr->data.value; /* 0460 */
      if (wake_time < time_now)
        wake_time += 86400; /* Tomorrow */
    }
  } else /* Sleep specified number of seconds */
  {
    GetInt(descr);
    wake_time = time_now + descr->data.value;
  }
  k_dismiss();

  while (local_time() < wake_time) {
    if (my_uptr->events)
      process_events();
    if (k_exit_cause & K_INTERRUPT)
      break;
    Sleep(1000);
    if ((process.break_inhibits == 0) && break_pending)
      break;
  }

  return;
}

/* ======================================================================
   op_status()  -  STATUS() function                                      */

void op_status() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |                             |  STATUS() value             |
     |=============================|=============================|
 */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = abs(process.status);
}

/* ======================================================================
   op_time()  -  Return time as integer                                   */

void op_time() {
  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = local_time() % 86400L;
}

/* ======================================================================
   op_timedate()  -  Return time and date as string                       */

void op_timedate() {
  char s[20 + 1];
  int32_t timenow;
  int16_t hour;
  int16_t min;
  int16_t sec;
  STRING_CHUNK* p;
  int32_t n;
  int16_t i;

  timenow = local_time() + date_adjustment;

  /* Form time of day values */

  n = timenow % 86400L;
  hour = (int16_t)(n / 3600);
  n = n % 3600;
  min = (int16_t)(n / 60);
  sec = (int16_t)(n % 60);

  /* Build result string */

  sprintf(s, "%02d:%02d:%02d %s", (int)hour, (int)min, (int)sec,
          day_to_ddmmmyyyy((timenow / 86400L) + 732));
  UpperCaseString(s);

  InitDescr(e_stack, STRING);
  p = e_stack->data.str.saddr = s_alloc(20L, &i);
  p->string_len = 20;
  p->bytes = 20;
  p->ref_ct = 1;
  memcpy(p->data, s, 20);

  e_stack++;
}

/* ======================================================================
   op_total()  -  TOTAL() function                                        */

void op_total() {
  int16_t index;
  DESCRIPTOR* descr;
  DESCRIPTOR* total_descr;

  index = *(pc++) + tbase - 1; /* 0205 */

  if (tbase) /* Doing query processor listing phase I-type */
  {
    /* Find totals array element to accumulate */

    total_descr = Element(process.syscom, SYSCOM_QPROC_TOTALS);
    total_descr = Element(total_descr->data.ahdr_addr, index);

    if (Element(process.syscom, SYSCOM_BREAK_LEVEL)->data.value == 0) {
      /* Detail line  -  Accumulate totals */

      descr = e_stack - 1;
      GetNum(descr);
      *total_descr = *descr;
    } else {
      /* Total line  -  Retrieve totals */

      *(e_stack++) = *total_descr;
    }
  }
}

/* ======================================================================
   op_trans()  -  TRANS function                                          */

void op_trans() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Action code expression     |  Result                     |
     |-----------------------------|-----------------------------|
     |  Field number               |                             |
     |-----------------------------|-----------------------------|
     |  Record ID                  |                             |
     |-----------------------------|-----------------------------|
     |  VOC name of file (may have |                             |
     |  DICT prefix                |                             |
     |=============================|=============================|
 */

  /* Push flag controlling lowering of marks */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = TRUE;

  k_recurse(pcode_trans, 5); /* Execute recursive code */
}

/* ======================================================================
   op_umask()  -  Apply UMASK value                                       */

void op_umask() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  New value. -ve if query only  | Previous value              |
     |================================|=============================|
 */

  DESCRIPTOR* descr;
  int n = 0;  /* this variable was never initialized and could potentially
               * produce unwanted results via the umask() call if a weird
               * value was passed in to it.
               * -gwb 22Feb20
               */

  descr = e_stack - 1;
  GetInt(descr);

  if (descr->data.value < 0) { /* Query only */
    descr->data.value = umask(n) & 0777;
    (void)umask(descr->data.value);
  } else { /* Query and set */
    n = descr->data.value & 0777;
    descr->data.value = umask(n);
  }
}

/* ======================================================================
   op_vartype()  -  Return variable type                                  */

void op_vartype() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  ADDR to variable           |  Variable type code         |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;
  u_char type;

  descr = e_stack - 1;
  while (descr->type == ADDR)
    descr = descr->data.d_addr;
  type = descr->type;
  k_dismiss();

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = type;
}

/* ======================================================================
   op_wake()  -  Wake up paused process                                   */

void op_wake() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  QM user number of target   |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;
  int16_t uid;
  USER_ENTRY* uptr;

  descr = e_stack - 1;
  GetInt(descr);
  uid = (int16_t)(descr->data.value);
  k_dismiss();

  if ((uid > 0) && (uid <= sysseg->hi_user_no)) {
    uptr = UserPtr(uid);
    if (uptr != NULL) {
      uptr->flags |= USR_WAKE;

      kill(uptr->pid, SIGUSR1);
    }
  }
}

/* ======================================================================
   op_xtd()  -  XTD() function                                            */

void op_xtd() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Input string               | Converted value             |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;
  char s[20 + 1]; /* 0493 */
  char* p;        /* 0493 */
  int n;
  register u_char c;

  /* Get and convert value */

  descr = e_stack - 1;
  if (k_get_c_string(descr, s, 20) <= 0)
    goto exit_op_xtd;

  n = 0;
  for (p = s; (c = UpperCase(*p)) != '\0'; p++) {
    if ((c >= '0') && (c <= '9'))
      n = (n * 16) + (c - '0');
    else if ((c >= 'A') && (c <= 'F'))
      n = (n * 16) + (c - 'A' + 10);
    else
      goto exit_op_xtd;
  }

  k_dismiss();
  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = n;

exit_op_xtd:
  return;
}

/* ======================================================================
   day_to_dmy()  -  Convert Pick date to day, mon, year                   */

Private char days_in_month[12] = {31, 28, 31, 30, 31, 30,
                                  31, 31, 30, 31, 30, 31};

void day_to_dmy(int32_t day_no,
                int16_t* day,    /* 1 - 31 */
                int16_t* mon,    /* 1 - 12 */
                int16_t* year,   /* Absolute */
                int16_t* julian) /* Day of year */
{
  int32_t n;
  int16_t i;

  /* The Pick date has 31/12/67 as day zero. For convenience in conversion,
    adjust the date value so that the datum becomes 1/1/0001 as day zero.  */

  day_no += 718430;

  /* Calculate number of 400 year cycles since datum point */

  n = day_no / 146097;

  *year = (int16_t)(1 + (n * 400));

  day_no -= n * 146097;

  /* Calculate number of 100 year cycles within this 400 year cycle */

  n = min(day_no / 36524, 3);
  *year += (int16_t)(n * 100);
  day_no -= n * 36524;

  /* Calculate number of 4 year cycles within this 100 year cycle */

  n = day_no / 1461;
  *year += (int16_t)(n * 4);
  day_no -= n * 1461;

  /* Calculate whole years into this four year group */

  if (day_no == 1460)
    n = 3; /* Final day of 4 year group */
  else
    n = day_no / 365;
  *year += (int16_t)n;
  day_no -= n * 365;

  *julian = (int16_t)(day_no + 1);

  /* Is this a leap year? */

  if ((n == 3) && (((*year % 100) != 0) || ((*year % 400) == 0))) /* 0456 */
  {
    days_in_month[1] = 29;
  } else {
    days_in_month[1] = 28;
  }

  /* Calculate month */

  for (i = 0; day_no >= days_in_month[i]; i++) {
    day_no -= days_in_month[i];
  }

  *mon = i + 1;
  *day = (int16_t)(day_no + 1);
}

/* ======================================================================
   day_to_ddmmmyyyy()  -  Convert date to DD MMM YYYY format              */

char* day_to_ddmmmyyyy(int32_t day_no) /* QM format day number */
{
  int16_t year;
  int16_t mon;
  int16_t day;
  int16_t day_of_year;
  static char date_string[11 + 1];

  day_to_dmy(day_no, &day, &mon, &year, &day_of_year);

  /* Build result string */

  sprintf(date_string, "%2d %.3s %4d", (int)day, month_names[mon - 1],
          (int)year);
  UpperCaseString(date_string);
  return date_string;
}

/* ======================================================================
   set_date()  -  Set current date                                        */

void set_date(int32_t new_date) {
  date_adjustment = (new_date - ((local_time() / 86400L) + 732)) * 86400;
}

/* END-CODE */
