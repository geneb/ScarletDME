/* OP_TIO.C
 * Terminal i/o opcodes and support functions
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
 * 15Jan22 gwb Fixed formatting argument issue (CWE-686) and reformatted
 *             the whole file.
 * 
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 *
 * 22Feb20 gwb Cleared up a variable assigned but never used in
 *             process_client_input().
 * 
 * START-HISTORY (OpenQM):
 * 05 Nov 07  2.6-5 0566 Applied casts to handle keyin() correctly.
 * 31 Aug 07  2.6-1 Set tio.term_type for PDA.
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 19 Jun 07  2.5-7 Terminal type now set by $LOGIN.
 * 09 Jun 07  2.5-7 Added break handler.
 * 15 Feb 07  2.4-20 0542 Do not send IS1 string for phantom or QMClient.
 * 08 Feb 07  2.4-20 0540 Handling of binary mode on output was inverted.
 * 05 Feb 07  2.4-20 Added transparent_newline argument to tio_display_string().
 * 04 Feb 07  2.4-20 tparm() now takes length argument.
 * 18 Dec 06  2.4-18 0531 Break key Q action was handled incorrectly.
 * 15 Dec 06  2.4-17 Honour BELL on/off setting in squeek().
 * 21 Nov 06  2.4-17 Revised interface to dio_open().
 * 15 Oct 06  2.4-15 Added support for print mode 6 (Print and hold).
 * 13 Oct 06  2.4-15 Separated printer name and file name in print unit.
 * 30 Aug 06  2.4-12 0518 emit_header_footer() was using \n, not pu->newline.
 * 16 Aug 06  2.4-11 Added PRINT_TO_AUX_PORT.
 * 11 Aug 06  2.4-11 Changed name of help file.
 * 02 Aug 06  2.4-10 Added terminfo colour mapping.
 * 07 Jul 06  2.4-9 For CN_WINSTDOUT, use TERM environment variable to set
 *                  initial termianl type.
 * 03 Jul 06  2.4-6 0500 printer_on() and printer_off() were only setting or
 *                  clearing the flag on the current program.
 * 30 Jun 06  2.4-5 Added CN_WINSTDOUT.
 * 29 Jun 06  2.4-5 Added lock_beep().
 * 31 May 06  2.4-5 Moved printer on flag into PROGRAM structure. Printer on
 *                  status should apply to current command processor level.
 * 28 Apr 06  2.4-2 Added newline element to print unit.
 * 19 Apr 06  2.4-2 Added CN_PORT handling.
 * 11 Apr 06  2.4-1 Allow page length of zero as infinite.
 * 04 Apr 06  2.4-1 Adapted process_client_input() for byte order independence.
 * 03 Apr 06  2.4-0 0471 Send correct packet length in QMRespond().
 * 20 Mar 06  2.3-8 Added op_in().
 * 17 Mar 06  2.3-8 KEYIN() timeout now in milliseconds.
 * 02 Mar 06  2.3-8 Inhibit break key (not just defer) when in debugger.
 * 10 Feb 06  2.3-6 Added timeout and THEN/ELSE processing to INPUT and INPUT@.
 * 09 Feb 06  2.3-6 Added style to print unit definition.
 * 19 Jan 06  2.3-5 Added GETPU(PU$DEFINED).
 * 30 Dec 05  2.3-3 Added GETPU(PU$PAGENUMBER).
 * 20 Oct 05  2.2-15 Added PCL related GETPU() and SETPU options.
 * 18 Oct 05  2.2-15 Added overlay to print unit structure.
 * 01 Oct 05  2.2-14 0419 Heading P option should take width qualifier and
 *                   default to four characters. Also extended S to take a
 *                   width qualifer (default 1) for left justification.
 * 31 Aug 05  2.2-9 0404 HELP did not work on Windows client from Linux server
 *                  if entered QM from the Linux shell.
 * 30 Aug 05  2.2-9 Added big endian support for op_readpkt() and op_writepkt().
 * 24 Aug 05  2.2-8 Use default date conversion in headings/footings.
 * 24 Aug 05  2.2-8 Use setqmstring().
 * 24 Aug 05  2.2-8 Added handling of spooler name in print unit.
 * 10 Aug 05  2.2-7 0387 Removed como file buffering so that stdout/stderr from
 *                  phantom appear in the right place.
 * 09 Aug 05  2.2-7 0387 como_open() should redirect stdout/stderr to the como
 *                  file.
 * 08 Aug 05  2.2-7 Allow print unit width up to 32767 with corresponding
 *                  changes to emit_header_footer.
 * 01 Aug 05  2.2-6 0382 Fixed page numbering on PAGE.
 * 19 Jul 05  2.2-4 Added prefix element to print unit definition.
 * 18 Jul 05  2.2-4 Added support for IT$E80 and IT$E132.
 * 14 Jul 05  2.2-4 Added options to print unit definition.
 * 01 Jul 05  2.2-3 Added form name to print unit definition.
 * 27 Jun 05  2.2-1 0368 Page with a very long header crashed QM with recursive
 *                  processing of header.
 * 12 May 05  2.1-14 0353 Moved inewline and onewline to allow static
 *                   initialisation.
 * 06 May 05  2.1-13 Removed raw mode argument from keyin().
 * 27 Apr 05  2.1-13 0349 Reinstated pipe output in write_socket() for Linux.
 * 21 Apr 05  2.1-12 Renamed trapping_breaks as trap_break_char.
 * 14 Apr 05  2.1-12 @(-108) and @(-109) now take string as arg2.
 * 14 Apr 05  2.1-12 Added socket output buffering.
 * 12 Apr 05  2.1-12 Separated in/out binary mode.
 * 24 Feb 05  2.1-8 Check for LINES and COLUMNS environment variables.
 * 14 Feb 05  2.1-7 Added IT$SBOLD and IT$EBOLD.
 * 27 Jan 05  2.1-4 Added overrun check for B heading option.
 * 10 Jan 05  2.1-0 Renamed tgetstr() as qmtgetstr().
 * 03 Jan 05  2.1-0 Added support for save/restore screen on non-QMTerm.
 * 03 Jan 05  2.1-0 Moved @(-54) and @(-55) to use new terminfo entries slt and
 *                  rlt. u0 and u1 are now available as @(-100) and @(-101).
 * 17 Dec 04  2.1-0 Added H option to heading/footing.
 * 28 Sep 04  2.0-3 Suppress clear screen for single command execution mode.
 * 28 Sep 04  2.0-3 0254 Reworked process_client_input to handle case where a
 *                  program performs input with no preceding output.
 * 20 Sep 04  2.0-2 Use message handler.
 * 20 Sep 04  2.0-2 Use dynamic pcode loader.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * op_at         AT
 * op_bindkey    BINDKEY        BINDKEY()
 * op_break      BREAK
 * op_breakct    BREAKCT        BREAK.COUNT()
 * op_clrinput   CLEARINPUT
 * op_como       COMO
 * op_data       DATA
 * op_dsp        DSP
 * op_dspnl      DSPNL
 * op_expandhf   EXPANDHF       EXPAND.HF()
 * op_footing    FOOTING
 * op_getprompt  GETPROMPT      PROMPT()
 * op_heading    HEADING
 * op_headingn   HEADINGN
 * op_hush       HUSH
 * op_in         IN
 * op_input      INPUT
 * op_inputat    INPUT@
 * op_keycode    KEYCODE        KEYCODE()
 * op_keyin      KEYIN          KEYIN()
 * op_keyready   KEYRDY         KEYREADY()
 * op_page       PAGE
 * op_prclose    PRCLOSE
 * op_pr_disp    PRDISP
 * op_prfile     PRFILE
 * op_printerr   PRINTERR
 * op_prname     PRNAME
 * op_prnl       PRNL
 * op_prnt       PRNT
 * op_proff      PROFF
 * op_pron       PRON
 * op_prompt     PROMPT
 * op_prreset    PRRESET        PRINTER RESET
 * op_pset       PSET           PRINTER.SETTING(), PRINTER SETTING
 * op_rstscrn    RSTSCRN
 * op_savescrn   SAVESCRN       SAVE.SCREEN()
 *
 * Other externally callable functions
 * como_close
 * emit_header_footer
 * free_print_units
 * tio_display_string
 * tio_printf
 * tio_write                 Write C string at current cursor position
 *
 * Support Functions
 * como_string
 * como_open
 * display_chunked_string
 * display_footer
 * display_header
 * phantom_error           Raise error on phantom terminal input
 * print_chunked_string
 * set_data_lines          Find lines per page for header and footer
 * squeek                  Sound terminal "bell"
 * tio_find_printer
 * tio_handle_break        Break key action
 * tio_new_line
 * tio_print_string
 * tio_set_printer
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include "keys.h"
#include "tio.h"
#include "syscom.h"
#include "header.h"
#include "qmclient.h"
#include "qmtermlb.h"
#include "config.h"
#include "options.h"
#include "debug.h"

#include <stdarg.h>
#include <setjmp.h>
#include <time.h>

Private bool displaying_footer = FALSE;
Private bool error_displayed = FALSE;

/* Cached pointer to last referenced print unit to increase performance
   when an application uses many printers.                               */
#define NO_LAST_PU -2
Private int16_t last_pu = NO_LAST_PU; /* Print unit no (never 0 or -1) */
Private PRINT_UNIT *last_pu_ptr;      /* Pointer to PRINT_UNIT structure */

#define PrinterOn ((process.program.flags & PF_PRINTER_ON) != 0)

/* External functions */
bool check_debug(void);
void op_dbgon(void);
void op_dsp(void);
void to_printer(PRINT_UNIT *, char *q, int16_t bytes);
void to_file(PRINT_UNIT *, char *q, int16_t bytes);
STRING_CHUNK *inblk(int max_bytes);

/* Internal used opcodes */
void op_getprompt(void);

/* Internal functions */
Private int colour_map(int user_colour);
Private void keyedit_trap_exit(int mode);
Private void capture(char *s, int16_t n);
Private void emitn(char *tgt, char *src, int rpt);
Private int como_open(char name[]);
Private void como_string(char *str, int16_t bytes);
Private bool display_chunked_string(bool flush);
Private bool display_footer(void);
Private void display_header(void);
Private void heading(bool eject);
Private void phantom_error(void);
Private bool print_chunked_string(PRINT_UNIT *pu, bool flush);
Private void set_data_lines(PRINT_UNIT *pu);
Private void set_print_mode(PRINT_UNIT *pu, int16_t mode);
Private PRINT_UNIT *tio_find_printer(DESCRIPTOR *descr, bool pr_on);
Private void tio_display_new_page(void);
Private void tio_new_line(void);
Private bool tio_print_string(PRINT_UNIT *pu, char *q, int16_t bytes, bool transparent_newline);
Private void getkey(bool invert, int timeout);
Private void printer_off(void);
Private void printer_on(void);
void csr(int16_t x, int16_t y);
void clr(void);
void cll(void);
Private void process_client_input(void);

/* ======================================================================
   op_at()  -  @ functions                                                */

void op_at() {
  DESCRIPTOR *arg1;
  DESCRIPTOR *arg2;
  int at1;
  int at2;
  char s[1024 + 1] = "";
  int n = -1;
  char arg2_str[MAX_PATHNAME_LEN + 1];
  char *p;
  char *q;

  arg1 = e_stack - 2;
  GetInt(arg1);
  at1 = arg1->data.value;

  arg2 = e_stack - 1;                 /* -ve for single arg @ functions */
  if ((at1 == -108) || (at1 == -109)) /* Special case for strings */
  {
    GetString(arg2);
    k_get_c_string(arg2, arg2_str, MAX_PATHNAME_LEN);
  } else {
    GetInt(arg2);
    at2 = arg2->data.value;
  }

  InitDescr(arg1, STRING);
  arg1->data.str.saddr = NULL;

  p = s;
  if (at1 >= 0) {
    if (at1 < tio.dsp.width) {
      if ((at2 >= 0) && (at2 < tio.dsp.lines_per_page)) {
        p = tparm(&n, qmtgetstr("cup"), at2, at1);
      } else /* Column address only */
      {
        /* Not all terminals have the hpa capability. If this is not
          available on this terminal, try for the cuf capability.  If this
          is also not available fall back on repeated cuf1 sequences. The
          tio.terminfo_hpa flag is set to zero whenever we change terminal
          type and subsequently optimises the path through the following
          code so that we do not have to do repeated qmtgetstr() searches on
          later use of this style of cursor move.                           */

        s[0] = '\r';
        if (at1 == 0) {
          s[1] = '\0';
          p = s;
        } else {
          switch (tio.terminfo_hpa) {
            case 0:
              p = qmtgetstr("hpa");
              if (*p != '\0') {
                p = tparm(&n, p, at1);
                break;
              }
              tio.terminfo_hpa++;
            case 1:
              p = qmtgetstr("cuf");
              if (*p != '\0') {
                q = tparm(&n, p, at1);
                memcpy(s + 1, q, n);
                n++; /* Allow for CR at start */
                p = s;
                break;
              }
              tio.terminfo_hpa++;
            default:
              emitn(s + 1, qmtgetstr("cuf1"), at1);
              p = s;
          }
        }
      }
    }
  } else {
    switch (at1) {
      case IT_CS: /* @(-1)  -  Clear screen */
        p = qmtgetstr("clear");
        tio.dsp.line = 0;
        break;

      case IT_CAH: /* @(-2)  -  Cursor home */
        p = qmtgetstr("home");
        tio.dsp.line = 0;
        break;

      case IT_CLEOS: /* @(-3)  -  Clear to end of screen */
        p = qmtgetstr("ed");
        break;

      case IT_CLEOL: /* @(-4)  -  Clear to end of line */
        p = qmtgetstr("el");
        break;

      case IT_SBLINK: /* @(-5)  -  Start flashing text */
        p = qmtgetstr("blink");
        break;

      case IT_EBLINK: /* @(-6)  -  End flashing text */
        p = qmtgetstr("sgr0");
        break;

      case IT_CUB: /* @(-9,n)  -  Cursor backspace */
        if (at2 < 1)
          at2 = 1;
        if (at2 > tio.dsp.width)
          at2 = tio.dsp.width;
        emitn(s, qmtgetstr("cub1"), at2);
        p = s;
        break;

      case IT_CUU: /* @(-10, n)  -  Cursor up line */
        if (at2 < 1)
          at2 = 1;
        if (at2 > tio.dsp.lines_per_page)
          at2 = tio.dsp.lines_per_page;
        emitn(s, qmtgetstr("cuu1"), at2);
        p = s;
        break;

      case IT_SHALF: /* @(-11)  -  Start half intensity mode */
        p = qmtgetstr("dim");
        break;

      case IT_EHALF: /* @(-12)  -  End half intensity mode */
        p = qmtgetstr("sgr0");
        break;

      case IT_SREV: /* @(-13)  -  Start reverse video */
        p = qmtgetstr("rev");
        break;

      case IT_EREV: /* @(-14)  -  End reverse video */
        p = qmtgetstr("sgr0");
        break;

      case IT_SUL: /* @(-15)  -  Start underline */
        p = qmtgetstr("smul");
        break;

      case IT_EUL: /* @(-16)  -  End underline */
        p = qmtgetstr("rmul");
        break;

      case IT_IL: /* @(-17)  -  Insert line */
        p = (at2 < 1) ? qmtgetstr("il1") : tparm(&n, qmtgetstr("il"), at2);
        break;

      case IT_DL: /* @(-18)  -  Delete line */
        p = (at2 < 1) ? qmtgetstr("dl1") : tparm(&n, qmtgetstr("dl"), at2);
        break;

      case IT_ICH: /* @(-19)  -  Insert character */
        if (at2 < 1)
          at2 = 1;
        emitn(s, qmtgetstr("ich1"), at2);
        p = s;
        break;

      case IT_DCH: /* @(-22)  -  Delete character */
        if (at2 < 1)
          at2 = 1;
        emitn(s, qmtgetstr("dch1"), at2);
        p = s;
        break;

      case IT_AUXON: /* @(-23)  -  Printer on */
        p = qmtgetstr("mc5");
        break;

      case IT_AUXOFF: /* @(-24)  -  Printer off */
        p = qmtgetstr("mc4");
        break;

      case IT_E80: /* @(-29)  -  Set 80 columns */
        if (is_QMTerm) {
          sprintf(s, "\x1B[29Q");
          p = s;
        }
        break;

      case IT_E132: /* @(-30)  -  Set 132 columns */
        if (is_QMTerm) {
          sprintf(s, "\x1B[30Q");
          p = s;
        }
        break;

      case IT_RIC: /* @(-31)  -  Reset inhibit cursor */
        p = qmtgetstr("cvvis");
        break;

      case IT_SIC: /* @(-32)  -  Set inhibit cursor */
        p = qmtgetstr("civis");
        break;

      case IT_CUD: /* @(-33, n)  -  Cursor down */
        if (at2 < 1)
          at2 = 1;
        if (at2 > tio.dsp.lines_per_page)
          at2 = tio.dsp.lines_per_page;
        emitn(s, qmtgetstr("cud1"), at2);
        break;

      case IT_CUF: /* @(-34, n)  -  Cursor forward */
        if (at2 < 1)
          at2 = 1;
        if (at2 > tio.dsp.width)
          at2 = tio.dsp.width;
        emitn(s, qmtgetstr("cuf1"), at2);
        p = s;
        break;

      case IT_FGC: /* @(-37)  -  Set foreground colour */
        p = tparm(&n, qmtgetstr("setf"), colour_map(at2));
        break;

      case IT_BGC: /* @(-38)  -  Set background colour */
        p = tparm(&n, qmtgetstr("setb"), colour_map(at2));
        break;

      case IT_SLT: /* @(-54)  -  Set line truncate */
        p = qmtgetstr("slt");
        break;

      case IT_RLT: /* @(-55)  -  Reset line truncate */
        p = qmtgetstr("rlt");
        break;

      case IT_SBOLD: /* @(-58)  -  Start bold mode */
        p = qmtgetstr("bold");
        break;

      case IT_EBOLD: /* @(-59)  -  End bold mode */
        p = qmtgetstr("sgr0");
        break;

      case IT_SCREEN: /* @(-256, n)  -  Set screen shape */
        if (is_QMTerm) {
          sprintf(s, "\x1B[256;%dQ", at2);
          p = s;
        }
        break;

      case IT_ACMD: /* @(-108)  -  Execute asynchronous command */
        p = tparm(&n, qmtgetstr("u8"), arg2_str);
        break;

      case IT_SCMD: /* @(-109)  -  Execute synchronous command */
        p = tparm(&n, qmtgetstr("u9"), arg2_str);
        break;

      default:
        if ((at1 <= -100) && (at1 >= -107)) /* 0233 */
        {
          sprintf(s, "u%d", -(at1 + 100));
          p = qmtgetstr(s);
        }
        break;
    }
  }

  k_dismiss(); /* Remove arg2 */
  if (n < 0)
    n = strlen(p);
  k_put_string(p, n, arg1); /* Replace arg1 by result */

  /* Use of @ cursor positioning functions turns off screen pagination even
    if the result is never used in a PRINT statement. For example
        DUMMY = @(0,0)
    will turn off pagination. Test for this here.                           */

  if (at1 >= 0)
    tio.dsp.flags &= ~PU_PAGINATE;
}

/* ------------------------------------------------ */
Private void emitn(char *tgt, char *src, int rpt) {
  int n;

  n = strlen(src);
  while (rpt--) {
    memcpy(tgt, src, n);
    tgt += n;
  }
  *tgt = '\0';
}

/* ------------------------------------ */
Private int colour_map(int user_colour) {
  char *p;
  int i;

  if (tio.terminfo_colour_map == NULL) {
    tio.terminfo_colour_map = qmtgetstr("colourmap");
    if (tio.terminfo_colour_map == NULL)
      tio.terminfo_colour_map = null_string;
  }

  for (p = tio.terminfo_colour_map, i = 0; i < user_colour && (*p != '\0'); p++, i++) {
    p = strchr(p, '|');
    if (p == NULL)
      return user_colour;
  }

  if ((*p == '\0') || (*p == '|'))
    return user_colour;

  return atoi(p);
}

/* ======================================================================
   op_bindkey()  -  BINDKEY()                                             */

void op_bindkey() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top | Action code                    | Success = 1, failure = 0    |
     |--------------------------------|-----------------------------|
     | Key string                     |                             |
     |================================|=============================|
*/

  k_recurse(pcode_bindkey, 2);
}

/* ======================================================================
   op_break()  -  BREAK opcode                                            */

void op_break() {
  DESCRIPTOR *arg;

  arg = e_stack - 1;
  GetInt(arg);

  if (arg->data.value < 0) /* BREAK CLEAR */
  {
    break_pending = FALSE;
  }
  if (arg->data.value) /* BREAK ON */
  {
    if (process.break_inhibits)
      process.break_inhibits--;
  } else /* BREAK OFF */
  {
    process.break_inhibits++;
  }

  k_pop(1);

  /* Now that we have dismissed the control variable, test to see if we have
    re-enabled breaks with a break noted whilst they were inhibited.       */

  if ((process.break_inhibits == 0) && !recursion_depth && break_pending) {
    k_exit_cause = K_QUIT;
  }
}

/* ======================================================================
   op_breakct()  -  BREAKCT opcode  -  Return break counter               */

void op_breakct() {
  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = process.break_inhibits;
}

/* ======================================================================
   op_clrinput()  -  CLRINPUT opcode  -  Clear terminal input queue       */

void op_clrinput() {
  while (keyready())
    keyin(0);
}

/* ======================================================================
   op_como()  -  Activate como file                                       */

void op_como() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  Como file path name           | Status 0=ok, <0 = ON ERROR  |
     |================================|=============================|

    ON ERROR codes are only returned if ONERROR opcode executed prior to
    call to COMO.
 */

  DESCRIPTOR *descr;
  char path_name[MAX_PATHNAME_LEN + 1];
  u_int16_t op_flags;

  op_flags = process.op_flags;
  process.op_flags = 0;

  /* Get path name */

  descr = e_stack - 1;
  if (k_get_c_string(descr, path_name, MAX_PATHNAME_LEN) < 0) {
    process.status = ER_NAM;
    goto exit_como;
  }

  if (path_name[0] == '\0') {
    como_close();
    process.status = 0;
  } else {
    process.status = (int32_t)como_open(path_name);
  }

exit_como:
  k_dismiss();

  /* Set status code on stack */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = -process.status;

  if ((process.status != 0) && !(op_flags & P_ON_ERROR)) {
    k_error(sysmsg(1700));
  }
}

/* ======================================================================
   op_data()  -  DATA statement                                           */

void op_data() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  String to add to data queue   |                             |
     |================================|=============================|
*/

  k_recurse(pcode_data, 1);
}

/* ======================================================================
   op_dsp()  -  Display without newline                                   */

void op_dsp() {
  display_chunked_string(TRUE);
}

/* ======================================================================
   op_dspnl()  -  Display with newline                                    */

void op_dspnl() {
  if (display_chunked_string(FALSE)) {
    tio_display_string("\n", 1, TRUE, FALSE);
  }
}

/* ======================================================================
   op_expandhf()  -  Expand heading/footing                               */

void op_expandhf() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  Heading/footing template      | Expansion                   |
     |--------------------------------|-----------------------------|
     |  Page number                   |                             |
     |--------------------------------|-----------------------------|
     |  Print unit number             |                             |
     |================================|=============================|
 */

  k_recurse(pcode_hf, 3);
}

/* ======================================================================
   op_footing()  -  FOOTING opcode                                        */

void op_footing() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  Footing string                |                             |
     |--------------------------------|-----------------------------|
     |  Print unit                    |                             |
     |================================|=============================|

 */

  DESCRIPTOR *descr;
  PRINT_UNIT *pu;

  pu = tio_find_printer(e_stack - 2, PrinterOn);
  if (pu == NULL)
    k_err_pu();

  //0134 if (cproc_level > pu->level) pu->level = cproc_level;

  descr = e_stack - 1;
  setqmstring(&(pu->footing), descr);

  k_dismiss();
  k_pop(1);

  /* We paginate if either the heading or the footing is not null */

  if ((pu->heading != NULL) || (pu->footing != NULL))
    pu->flags |= PU_PAGINATE;
  else
    pu->flags &= ~PU_PAGINATE;

  set_data_lines(pu);
}

/* ======================================================================
   op_getprompt()  -  Get prompt character (restricted)                   */

void op_getprompt() {
  STRING_CHUNK *str;
  int16_t n;

  InitDescr(e_stack, STRING);

  if (tio.prompt_char == '\0') {
    e_stack->data.str.saddr = NULL;
  } else {
    str = e_stack->data.str.saddr = s_alloc(1L, &n);
    str->ref_ct = 1;
    str->string_len = 1;
    str->bytes = 1;
    str->data[0] = tio.prompt_char;
  }
  e_stack++;
}

/* ======================================================================
   op_heading()   -  HEADING opcode
   op_headingn()  -  HEADINGN opcode                                      */

void op_heading() {
  heading(TRUE);
}

void op_headingn() {
  heading(FALSE);
}

Private void heading(bool eject) {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  Heading string                |                             |
     |--------------------------------|-----------------------------|
     |  Print unit                    |                             |
     |================================|=============================|

 */

  DESCRIPTOR *descr;
  PRINT_UNIT *pu;

  pu = tio_find_printer(e_stack - 2, PrinterOn);
  if (pu == NULL)
    k_err_pu();

  if (!(pu->flags & PU_OUTPUT))
    eject = TRUE; /* 0382 Force page start if first output */

  //0134 if (cproc_level > pu->level) pu->level = cproc_level;

  descr = e_stack - 1;
  setqmstring(&(pu->heading), descr);
  k_dismiss();
  k_pop(1);

  /* We paginate if either the heading or the footing is not null */

  if ((pu->heading != NULL) || (pu->footing != NULL))
    pu->flags |= PU_PAGINATE;
  else
    pu->flags &= ~PU_PAGINATE;

  if (eject) /* 0203 */
  {
    if ((pu->heading != NULL) && (pu->line != 0)) {
      switch (pu->mode) {
        case PRINT_TO_DISPLAY:
          tio_display_new_page();
          break;

        case PRINT_TO_PRINTER:
        case PRINT_TO_FILE:
        case PRINT_TO_AUX_PORT:
        case PRINT_AND_HOLD:
          tio_print_string(pu, "\f", 1, FALSE);
          break;
      }
    }

    pu->line = 0;
    pu->col = 0;
    pu->flags |= PU_HDR_NEXT; /* Force heading on next print */
  }
  set_data_lines(pu);
}

/* ======================================================================
   op_hush()  -  HUSH opcode                                              */

void op_hush() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  HUSH flag setting             |                             |
     |================================|=============================|

 */

  DESCRIPTOR *descr;

  descr = e_stack - 1;
  GetInt(descr);

  process.status = tio.hush;
  tio.hush = (descr->data.value != 0);
  k_pop(1);
}

/* ======================================================================
   op_in()  -  opcode for the IN statement                                */

void op_in() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  Timeout (tenths of second)    | Charactre value             |
     |================================|=============================|
*/

  k_recurse(pcode_in, 1);
}

/* ======================================================================
   op_inputblk()  -  INPUTBLK                                             */

void op_inputblk() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  Maximum length                | String                      |
     |================================|=============================|
*/

  DESCRIPTOR *descr;
  int max_bytes;

  descr = e_stack - 1;
  GetInt(descr);
  max_bytes = descr->data.value;

  InitDescr(descr, STRING);
  descr->data.str.saddr = inblk(max_bytes);
}

/* ======================================================================
   op_input()  -  INPUT                                                   */

void op_input() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  Flags                         |                             |
     |--------------------------------|-----------------------------|
     |  Timeout (may be absent)       |                             |
     |--------------------------------|-----------------------------|
     |  Max input length              |                             |
     |--------------------------------|-----------------------------|
     |  ADDR to string (updated)      |                             |
     |================================|=============================|

     The timeout element is only present if the IN$TIMEOUT flag is set
*/

  DESCRIPTOR *descr;
  DESCRIPTOR *syscom_descr;
  bool status;
  int flags;

  /* Examine flags */

  descr = e_stack - 1;
  GetInt(descr);
  flags = descr->data.value;
  if (!(flags & IN_TIMEOUT)) {
    /* We do not have a timeout element. We need to insert this so that the
      stack matches the arguments for _INPUT.                              */

    *(e_stack++) = *descr; /* Relocated flags */
    descr->data.value = 0; /* Zero timeout */
  }

  descr = e_stack - 3;
  GetInt(descr);
  if (descr->data.value < 0) /* INPUT var, -1 */
  {
    k_dismiss();
    k_dismiss();
    k_dismiss();

    descr = e_stack - 1;
    while (descr->type == ADDR)
      descr = descr->data.d_addr;
    k_release(descr);

    status = (connection_type != CN_NONE) && keyready();
    if (!status) {
      /* Find DATA in SYSCOM */

      syscom_descr = Element(process.syscom, SYSCOM_DATA_QUEUE);
      status = syscom_descr->data.str.saddr != NULL;
    }

    InitDescr(descr, INTEGER);
    descr->data.value = status;
    k_dismiss();
  } else {
    if (is_QMVbSrvr) {
      /* Dismiss flags, timeout and input length */

      k_dismiss();
      k_dismiss();
      k_dismiss();

      process_client_input();
    } else {
      k_recurse(pcode_input, 4);
    }
  }
}

/* ======================================================================
   op_inputat()  -  INPUT@                                                */

void op_inputat() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  Flags                         |                             |
     |--------------------------------|-----------------------------|
     |  Timeout (optional)            |                             |
     |--------------------------------|-----------------------------|
     |  Mask (optional)               |                             |
     |--------------------------------|-----------------------------|
     |  Max input length              |                             |
     |--------------------------------|-----------------------------|
     |  ADDR to string (updated)      |                             |
     |--------------------------------|-----------------------------|
     |  Line position                 |                             |
     |--------------------------------|-----------------------------|
     |  Column position               |                             |
     |================================|=============================|

     The timeout element is only present if the IN$TIMEOUT flag is set
     The format mask is only present if the IN_MASK flag is set.
*/

  DESCRIPTOR *descr;
  int flags;
  DESCRIPTOR timeout_descr;
  DESCRIPTOR mask_descr;

  /* Examine flags and rebuild top of stack for optional components */

  descr = e_stack - 1;
  GetInt(descr);
  flags = descr->data.value;
  k_pop(1);

  if (flags & IN_TIMEOUT)
    timeout_descr = *(--e_stack);
  else {
    InitDescr(&timeout_descr, INTEGER);
    timeout_descr.data.value = 0;
  }

  if (flags & IN_MASK)
    mask_descr = *(--e_stack);
  else
    InitDescr(&mask_descr, UNASSIGNED);

  *(e_stack++) = mask_descr;
  *(e_stack++) = timeout_descr;
  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = flags;

  if (is_QMVbSrvr) {
    /* Dismiss flags, timeout, mask and input length */

    k_dismiss();
    k_dismiss();
    k_dismiss();
    k_dismiss();

    process_client_input();

    /* Dismiss screen coordinates */

    k_dismiss();
    k_dismiss();
  } else {
    if (error_displayed) {
      descr = e_stack - 1;
      k_get_value(descr);
      descr->data.value |= IN_ERR_DISPLAYED;
    }
    k_recurse(pcode_inputat, 7);
  }
}

/* ======================================================================
   op_keycode()  -  KEYCODE()                                             */

void op_keycode() {
  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = 0;
  k_recurse(pcode_keycode, 1);
}

void op_keycodet() {
  DESCRIPTOR *descr;

  descr = e_stack - 1;
  GetInt(descr);
  k_recurse(pcode_keycode, 1);
}

/* ======================================================================
   op_keyedit()  -  KEYEDIT()                                             */

void op_keyedit() {
  keyedit_trap_exit(0);
}
void op_keyexit() {
  keyedit_trap_exit(1);
}
void op_keytrap() {
  keyedit_trap_exit(2);
}

Private void keyedit_trap_exit(int mode) {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top | Key string value               |                             |
     |--------------------------------|-----------------------------|
     | Key code                       |                             |
     |================================|=============================|
*/

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = mode;

  k_recurse(pcode_keyedit, 3);
}

/* ======================================================================
   op_keyin()  -  opcode for the KEYIN() function                         */

void op_keyin() {
  getkey(FALSE, -1);
}

void op_keyinc() {
  getkey(TRUE, -1);
}

void op_keyint() {
  DESCRIPTOR *descr;
  int timeout;

  descr = e_stack - 1;
  GetNum(descr);
  if (descr->type == INTEGER)
    timeout = descr->data.value * 1000;
  else
    timeout = (int)(descr->data.float_value * 1000);

  if (timeout < 0)
    timeout = 0; /* 0145 */
  getkey(FALSE, timeout);
}

void op_keyinct() {
  DESCRIPTOR *descr;
  int timeout;

  descr = e_stack - 1;
  GetNum(descr);
  if (descr->type == INTEGER)
    timeout = descr->data.value * 1000;
  else
    timeout = (int)(descr->data.float_value * 1000);

  if (timeout < 0)
    timeout = 0; /* 0145 */
  getkey(TRUE, timeout);
}

Private void getkey(bool invert, int timeout) /* -ve = none */
{
  STRING_CHUNK *str;
  int16_t n;
  int16_t k;
  char c;

  if (connection_type == CN_NONE)
    k_error(sysmsg(1701));
  if (is_QMVbSrvr)
    k_error(sysmsg(1702));

  k = keyin((timeout < 0) ? 0 : timeout);
  if ((k == 0) && (k_exit_cause & K_INTERRUPT)) { /* Break key, etc.  Process and then repeat */
    pc--;
    return;
  }

  if (timeout >= 0)
    k_pop(1); /* 0145 Remove timeout from the stack */

  InitDescr(e_stack, STRING);

  if (k < 0) /* Exception condition (timeout or pipe eof) */
  {
    e_stack->data.str.saddr = NULL;
  } else {
    c = (char)k;
    if (invert && case_inversion && IsAlpha(c))
      c ^= 0x20;

    str = e_stack->data.str.saddr = s_alloc(1L, &n);
    str->ref_ct = 1;
    str->string_len = 1;
    str->bytes = 1;
    str->data[0] = c;
  }

  e_stack++;
  return;
}

/* ======================================================================
   op_keyready()  -  opcode for the KEYREADY() function                   */

void op_keyready() {
  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = (connection_type != CN_NONE) && keyready();
}

/* ======================================================================
   op_page()  -  PAGE opcode                                              */

void op_page() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  New page number (-ve if none) |                             |
     |--------------------------------|-----------------------------|
     |  Print unit                    |                             |
     |================================|=============================|

 */

  DESCRIPTOR *descr;
  PRINT_UNIT *pu;

  pu = tio_find_printer(e_stack - 2, PrinterOn);
  if (pu == NULL)
    k_err_pu();

  descr = e_stack - 1;
  GetInt(descr);

  // 0184 pu->paginate = TRUE;

  switch (pu->mode) {
    case PRINT_TO_DISPLAY:
      tio_display_new_page();
      break;

    case PRINT_TO_PRINTER:
    case PRINT_TO_FILE:
    case PRINT_TO_AUX_PORT:
    case PRINT_AND_HOLD:
      tio_print_string(pu, "\f", 1, FALSE);
      break;
  }

  if (descr->data.value >= 0)
    pu->page_no = (int16_t)(descr->data.value); /* 0382 moved */

  k_pop(2);
}

/* ======================================================================
   op_pset()  -  PRINTER.SETTING() function                               */

void op_pset() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  New value, -1 = default       |  New value                  |
     |  -2 = report value             |                             |
     |--------------------------------|-----------------------------|
     |  Parameter number              |                             |
     |--------------------------------|-----------------------------|
     |  Print unit                    |                             |
     |================================|=============================|

 */

  DESCRIPTOR *descr;
  PRINT_UNIT *pu;
  int32_t new_value;
  int16_t param;

  /* Get new value */

  descr = e_stack - 1;
  GetInt(descr);
  new_value = descr->data.value;

  /* Get parameter number */

  descr = e_stack - 2;
  GetInt(descr);
  param = (int16_t)(descr->data.value);
  if (descr->data.value != param) /* Overflow - set illegal param no */
  {
    param = -1;
  }

  /* Get print unit */

  pu = tio_find_printer(e_stack - 3, TRUE);
  if (pu == NULL)
    k_err_pu();

  //0134 if (new_value >= -1)
  //0134  {
  //0134   if (cproc_level > pu->level) pu->level = cproc_level;
  //0134  }

  switch (param) {
    case LPTR_WIDTH:
      if (new_value == -1)
        pu->width = pcfg.lptrwide;
      else if (new_value >= 0)
        pu->width = (int16_t)new_value;
      new_value = pu->width;
      break;

    case LPTR_LINES:
      if (new_value == -1)
        pu->lines_per_page = pcfg.lptrhigh;
      else if (new_value >= 0)
        pu->lines_per_page = (int16_t)new_value;
      new_value = pu->lines_per_page;
      set_data_lines(pu);
      break;

    case LPTR_TOP_MARGIN:
      if (new_value == -1)
        pu->top_margin = TIO_DEFAULT_TMGN;
      else if (new_value >= 0)
        pu->top_margin = (int16_t)new_value;
      new_value = pu->top_margin;
      set_data_lines(pu);
      break;

    case LPTR_BOTTOM_MARGIN:
      if (new_value == -1)
        pu->bottom_margin = TIO_DEFAULT_BMGN;
      else if (new_value >= 0)
        pu->bottom_margin = (int16_t)new_value;
      new_value = pu->bottom_margin;
      set_data_lines(pu);
      break;

    case LPTR_LEFT_MARGIN:
      if (new_value == -1)
        pu->left_margin = TIO_DEFAULT_LMGN;
      else if (new_value >= 0)
        pu->left_margin = (int16_t)new_value;
      new_value = pu->left_margin;
      break;

    case LPTR_DATA_LINES: /* Query only */
      new_value = pu->data_lines_per_page;
      break;

    case LPTR_HEADING_LINES: /* Query only */
      new_value = pu->heading_lines;
      break;

    case LPTR_FOOTING_LINES: /* Query only */
      new_value = pu->footing_lines;
      break;

    case LPTR_MODE: /* Query only */
      new_value = pu->mode;
      break;

    case LPTR_NAME: /* Query only */
      k_pop(3);
      k_put_c_string((pu->mode == PRINT_TO_FILE) ? pu->file_name : pu->printer_name, e_stack);
      e_stack++;
      return;

    case LPTR_FLAGS:
      if (new_value == -1)
        pu->flags &= ~PU_USER_SETABLE;
      else if (new_value >= 0) {
        pu->flags &= ~PU_USER_SETABLE;
        pu->flags |= new_value & PU_USER_SETABLE;
      }
      new_value = pu->flags & PU_USER_SETABLE;
      break;

    case LPTR_LINE_NO: /* Query only */
      new_value = pu->line + 1;
      if (new_value > pu->data_lines_per_page) /* Page throw pending */
      {
        new_value = 1;
      }
      break;

    case LPTR_PAGE_NO: /* Query only */
      new_value = pu->page_no + (pu->line >= pu->data_lines_per_page);
      break;

    case LPTR_LINES_LEFT: /* Query only - Lines remaining on page */
      new_value = pu->data_lines_per_page - pu->line;
      if (pu->unit < 0)
        new_value--; /* Space for pagination prompt */
      break;

    case LPTR_COPIES: /* Number of copies */
      if (new_value == -1)
        pu->copies = 1;
      else if (new_value >= 0)
        pu->copies = (int16_t)new_value;
      new_value = pu->copies;
      break;

    default:
      k_error(sysmsg(1703));
      break;
  }

  k_pop(2);
  (e_stack - 1)->data.value = new_value;
}

/* ======================================================================
   op_setpu()  -  SETPU statement                                         */

void op_setpu() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Value                      |                             |
     |-----------------------------|-----------------------------|
     |  Print unit                 |                             |
     |-----------------------------|-----------------------------|
     |  Action key                 |                             |
     |=============================|=============================|
 */

  DESCRIPTOR *descr;
  PRINT_UNIT *pu;
  int16_t key;
  int16_t n;
  char *printer_name = NULL;
  char s[10];

  process.status = 0; /* Assume success */

  /* Get action code */

  descr = e_stack - 3;
  GetInt(descr);
  key = (int16_t)(descr->data.value);

  /* Get print unit */

  pu = tio_find_printer(e_stack - 2, TRUE);
  if (pu == NULL) {
    process.status = ER_PRT_UNIT;
    goto exit_setpu;
  }

  /* Set descr to point to value item */

  descr = e_stack - 1;
  switch (key) /* !!PU!! */
  {
    case PU_MODE:
      GetInt(descr);
      switch (n = (int16_t)(descr->data.value)) {
        case PRINT_TO_DISPLAY:
        case PRINT_TO_PRINTER:
        case PRINT_TO_FILE:
        case PRINT_TO_STDERR:
        case PRINT_TO_AUX_PORT:
        case PRINT_AND_HOLD:
          //0134          if (cproc_level > pu->level) pu->level = cproc_level;
          set_print_mode(pu, n);
          break;

        default:
          process.status = ER_MODE;
          break;
      }
      break;

    case PU_WIDTH:
      GetInt(descr);
      if ((n = (int16_t)(descr->data.value)) < 0)
        n = pcfg.lptrwide;
      pu->width = n;
      break;

    case PU_LENGTH:
      GetInt(descr);
      if ((n = (int16_t)(descr->data.value)) < 0)
        n = pcfg.lptrhigh;
      pu->lines_per_page = n;
      set_data_lines(pu);
      break;

    case PU_TOPMARGIN:
      GetInt(descr);
      if ((n = (int16_t)(descr->data.value)) < 0)
        n = TIO_DEFAULT_TMGN;
      pu->top_margin = n;
      set_data_lines(pu);
      break;

    case PU_BOTMARGIN:
      GetInt(descr);
      if ((n = (int16_t)(descr->data.value)) < 0)
        n = TIO_DEFAULT_BMGN;
      pu->bottom_margin = n;
      set_data_lines(pu);
      break;

    case PU_LEFTMARGIN:
      GetInt(descr);
      if ((n = (int16_t)(descr->data.value)) < 0)
        n = TIO_DEFAULT_LMGN;
      pu->left_margin = n;
      break;

    case PU_SPOOLFLAGS:
      GetInt(descr);
      pu->flags &= ~PU_USER_SETABLE;
      pu->flags |= descr->data.value & PU_USER_SETABLE;
      break;

    case PU_BANNER:
      setqmstring(&(pu->banner), descr);
      break;

    case PU_FORM:
      setqmstring(&(pu->form), descr);
      break;

    case PU_LOCATION:
      k_get_string(descr);
      if (descr->data.str.saddr != NULL) {
        printer_name = alloc_c_string(descr);
      }

      switch (pu->mode) {
        case PRINT_TO_PRINTER:
        case PRINT_AND_HOLD:
          if (!validate_printer(printer_name)) {
            if (printer_name != NULL)
              k_free(printer_name);
            process.status = ER_NOT_FOUND;
            break;
          }
          if (pu->printer_name != NULL)
            k_free(pu->printer_name);
          pu->printer_name = printer_name;
          break;

        case PRINT_TO_FILE:
          if (pu->file_name != NULL)
            k_free(pu->file_name);
          pu->file_name = printer_name;
          break;

        default:
          process.status = ER_MODE;
          break;
      }
      break;

    case PU_COPIES:
      GetInt(descr);
      if ((n = (int16_t)(descr->data.value)) < 1)
        n = 1;
      pu->copies = n;
      break;

    case PU_PAGENUMBER:
      GetInt(descr);
      if ((n = (int16_t)(descr->data.value)) < 1)
        n = 1;
      pu->page_no = n;
      break;

    case PU_OPTIONS:
      setqmstring(&(pu->options), descr);
      break;

    case PU_PREFIX:
      setqmstring(&(pu->prefix), descr);
      break;

    case PU_SPOOLER:
      setqmstring(&(pu->spooler), descr);
      break;

    case PU_OVERLAY:
      setqmstring(&(pu->overlay), descr);
      break;

    case PU_CPI:
      k_get_float(descr);
      pu->cpi = (int16_t)(descr->data.float_value * 100); /* Stored scaled by 100 */
      break;

    case PU_PAPER_SIZE:
      GetInt(descr);
      if ((n = (int16_t)(descr->data.value)) < 1)
        n = 1;
      pu->paper_size = n;
      break;

    case PU_LPI:
      GetInt(descr);
      pu->lpi = (int16_t)(descr->data.value);
      break;

    case PU_WEIGHT:
      GetInt(descr);
      pu->weight = (int16_t)(descr->data.value);
      break;

    case PU_SYMBOL_SET:
      k_get_c_string(descr, pu->symbol_set, 7);
      break;

    case PU_STYLE:
      setqmstring(&(pu->style), descr);
      break;

    case PU_NEWLINE:
      k_get_c_string(descr, s, 2);
      if (!strcmp(s, "\r") || !strcmp(s, "\r\n"))
        strcpy(pu->newline, s);
      else
        strcpy(pu->newline, "\n");
      break;

    case PU_PRINTER_NAME:
      k_get_string(descr);
      if (descr->data.str.saddr != NULL) {
        printer_name = alloc_c_string(descr);
      }

      if (!validate_printer(printer_name)) {
        if (printer_name != NULL)
          k_free(printer_name);
        process.status = ER_NOT_FOUND;
      }

      if (pu->printer_name != NULL)
        k_free(pu->printer_name);
      pu->printer_name = printer_name;
      break;

    case PU_FILE_NAME:
      k_get_string(descr);
      if (descr->data.str.saddr != NULL) {
        printer_name = alloc_c_string(descr);
      }

      if (pu->file_name != NULL)
        k_free(pu->file_name);
      pu->file_name = printer_name;
      break;

    default:
      process.status = ER_BAD_KEY;
      break;
  }

exit_setpu:
  k_dismiss(); /* Value */
  k_pop(2);    /* Print unit, action key */
}

/* ======================================================================
   op_getpu()  -  GETPU() function                                        */

void op_getpu() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Print unit                 |  Result                     |
     |-----------------------------|-----------------------------|
     |  Action key                 |                             |
     |=============================|=============================|
 */

  DESCRIPTOR *descr;
  PRINT_UNIT *pu;
  int16_t key;
  int16_t n;
  DESCRIPTOR result;

  process.status = 0;

  /* Initialise result as integer zero for error paths */

  InitDescr(&result, INTEGER);
  result.data.value = 0;

  /* Get action code */

  descr = e_stack - 2;
  GetInt(descr);
  key = (int16_t)(descr->data.value);

  descr = e_stack - 1;

  if (key == PU_DEFINED) {
    GetInt(descr);
    n = (int16_t)(descr->data.value);
    for (pu = &tio.dsp; pu != NULL; pu = pu->next) {
      if (pu->unit == n) {
        result.data.value = TRUE;
        break;
      }
    }
  } else {
    /* Get print unit */

    pu = tio_find_printer(descr, TRUE);
    if (pu == NULL) {
      process.status = ER_PRT_UNIT;
      goto exit_getpu;
    }

    switch (key) /* !!PU!! */
    {
      case PU_MODE:
        result.data.value = pu->mode;
        break;

      case PU_WIDTH:
        result.data.value = pu->width;
        break;

      case PU_LENGTH:
        result.data.value = pu->lines_per_page;
        break;

      case PU_TOPMARGIN:
        result.data.value = pu->top_margin;
        break;

      case PU_BOTMARGIN:
        result.data.value = pu->bottom_margin;
        break;

      case PU_LEFTMARGIN:
        result.data.value = pu->left_margin;
        break;

      case PU_SPOOLFLAGS:
        result.data.value = pu->flags;
        break;

      case PU_BANNER:
        k_put_c_string(pu->banner, &result);
        break;

      case PU_FORM:
        k_put_c_string(pu->form, &result);
        break;

      case PU_LOCATION:
        k_put_c_string((pu->mode == PRINT_TO_FILE) ? pu->file_name : pu->printer_name, &result);
        break;

      case PU_COPIES:
        result.data.value = pu->copies;
        break;

      case PU_PAGENUMBER:
        result.data.value = pu->page_no;
        break;

      case PU_LINESLEFT:
        n = (int16_t)(pu->data_lines_per_page - pu->line);
        if (pu->unit < 0)
          n--; /* Space for pagination prompt */
        result.data.value = n;
        break;

      case PU_HEADERLINES:
        result.data.value = pu->heading_lines;
        break;

      case PU_FOOTERLINES:
        result.data.value = pu->footing_lines;
        break;

      case PU_DATALINES:
        result.data.value = pu->data_lines_per_page;
        break;

      case PU_OPTIONS:
        k_put_c_string(pu->options, &result);
        break;

      case PU_PREFIX:
        k_put_c_string(pu->prefix, &result);
        break;

      case PU_SPOOLER:
        k_put_c_string(pu->spooler, &result);
        break;

      case PU_OVERLAY:
        k_put_c_string(pu->overlay, &result);
        break;

      case PU_CPI:
        InitDescr(&result, FLOATNUM);
        result.data.float_value = pu->cpi / 100;
        break;

      case PU_PAPER_SIZE:
        result.data.value = pu->paper_size;
        break;

      case PU_LPI:
        result.data.value = pu->lpi;
        break;

      case PU_WEIGHT:
        result.data.value = pu->weight;
        break;

      case PU_SYMBOL_SET:
        k_put_c_string(pu->symbol_set, &result);
        break;

      case PU_STYLE:
        k_put_c_string(pu->style, &result);
        break;

      case PU_NEWLINE:
        k_put_c_string(pu->newline, &result);
        break;

      case PU_LINENO:
        n = (int16_t)(pu->line + 1);
        if (n > pu->data_lines_per_page)
          n = 1; /* Page throw pending */
        result.data.value = n;
        break;

      case PU_PRINTER_NAME:
        k_put_c_string(pu->printer_name, &result);
        break;

      case PU_FILE_NAME:
        k_put_c_string(pu->file_name, &result);
        break;

      default:
        process.status = ER_BAD_KEY;
        break;
    }
  }

exit_getpu:
  k_pop(2); /* Print unit, action key */

  /* Set result on stack */

  *(e_stack++) = result;
}

/* ======================================================================
   op_printerr()  -  PRINTERR opcode                                      */

void op_printerr() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Error string to display    |                             |
     |=============================|=============================|
 */

  DESCRIPTOR *descr;
  char errmsg[80 + 1];

  descr = e_stack - 1;
  k_get_string(descr);

  csr(0, tio.dsp.lines_per_page - 1);
  cll();

  if (descr->data.str.saddr == NULL) /* Clearing error */
  {
    error_displayed = FALSE;
  } else {
    (void)k_get_c_string(descr, errmsg, 80);
    tio_write(errmsg);
    error_displayed = TRUE;
  }

  flush_outbuf();

  k_dismiss();
}

/* ======================================================================
   op_prclose()  -  PRINTER CLOSE                                         */

void op_prclose() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Print unit (-2 = all)      |                             |
     |=============================|=============================|
 */

  DESCRIPTOR *descr;
  int16_t low_unit;
  int16_t high_unit;
  PRINT_UNIT *pu;
  int16_t level = 0;

  descr = e_stack - 1;
  GetInt(descr);
  low_unit = (int16_t)(descr->data.value);

  switch (low_unit) {
    case -3: /* Close all units at this or higher level */
      if (!(process.program.flags & HDR_INTERNAL))
        goto exit_op_prclose;
      high_unit = HIGH_PRINT_UNIT;
      level = cproc_level;
      break;

    case -2: /* Close all units */
      high_unit = HIGH_PRINT_UNIT;
      break;

    default:
      high_unit = low_unit;
      break;
  }

  for (pu = &tio.dsp; pu != NULL; pu = pu->next) {
    if ((pu->unit >= low_unit) && (pu->unit <= high_unit) && (pu->level >= level)) {
      if (!(pu->flags & PU_KEEP_OPEN))
        pu->flags &= ~PU_PART_CLOSED;

      if ((pu->flags & PU_ACTIVE) && !(pu->flags & PU_PART_CLOSED)) {
        switch (pu->mode) {
          case PRINT_TO_DISPLAY:
            if ((pu->footing != NULL) && (pu->line != 0)) {
              display_footer();
            }
            break;

          case PRINT_TO_PRINTER:
            end_printer(pu);
            break;

          case PRINT_TO_FILE:
          case PRINT_AND_HOLD:
            end_file(pu);
            break;

          case PRINT_TO_AUX_PORT:
            end_file(pu);
            break;
        }

        if ((pu->flags & PU_KEEP_OPEN)) {
          pu->flags |= PU_PART_CLOSED;
        } else {
          pu->level = 0;
          pu->flags &= ~(PU_ACTIVE | PU_OUTPUT);
        }
      }

      if (pu->unit == 0)
        printer_off();

      k_free_ptr(pu->heading);
      k_free_ptr(pu->footing);

      set_data_lines(pu); /* 0141 */

      pu->page_no = 1;
      pu->line = 0;
    }
  }

exit_op_prclose:
  k_pop(1);
}

/* ======================================================================
   op_prdisp()  -  PRINTER DISP                                           */

void op_prdisp() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Print unit                 |  Status 0 = ok, >0 = ELSE   |
     |                             |         <0 = ON ERROR       |
     |=============================|=============================|
 */

  PRINT_UNIT *pu;
  u_int16_t op_flags;
  int16_t status = 0;

  op_flags = process.op_flags;
  process.op_flags = 0;

  pu = tio_find_printer(e_stack - 1, TRUE);

  if (pu == NULL)
    status = -ER_PRT_UNIT;
  else {
    if (pu->unit < 0)
      k_error(sysmsg(1704));

    //0134   if (cproc_level > pu->level) pu->level = cproc_level;

    set_print_mode(pu, PRINT_TO_DISPLAY);
  }

  if ((status < 0) && !(op_flags & P_ON_ERROR)) {
    k_error(sysmsg(1705));
  }

  (e_stack - 1)->data.value = status;
}

/* ======================================================================
   op_prfile()  -  PRINTER FILE                                           */

void op_prfile() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Record name                |  Status 0 = ok, >0 = ELSE   |
     |                             |         <0 = ON ERROR       |
     |-----------------------------|-----------------------------|
     |  File name                  |                             |
     |-----------------------------|-----------------------------|
     |  Print unit                 |                             |
     |=============================|=============================|
 */

  DESCRIPTOR pathname;
  DESCRIPTOR status;
  PRINT_UNIT *pu;
  u_int16_t op_flags;

  op_flags = process.op_flags;
  process.op_flags = 0;

  /* Push PATHNAME and STATUS.CODE arguments onto stack */

  InitDescr(&pathname, UNASSIGNED);
  InitDescr(e_stack, ADDR);
  (e_stack++)->data.d_addr = &pathname;

  InitDescr(&status, UNASSIGNED);
  InitDescr(e_stack, ADDR);
  (e_stack++)->data.d_addr = &status;

  k_recurse(pcode_prfile, 4); /* Execute recursive code */

  if (status.data.value == 0) {
    pu = tio_find_printer(e_stack - 1, TRUE);
    if (pu == NULL)
      k_err_pu();

    k_pop(1);
    if (pu->unit < 0)
      k_error(sysmsg(1704));

    //0134 if (cproc_level > pu->level) pu->level = cproc_level;

    set_print_mode(pu, PRINT_TO_FILE);

    k_free_ptr(pu->file_name);

    if (pathname.data.str.saddr != NULL) {
      pu->file_name = alloc_c_string(&pathname);
      k_release(&pathname); /* 0148 */
    }

    if ((status.data.value < 0) && !(op_flags & P_ON_ERROR)) {
      k_error(sysmsg(1706));
    }
  }

  /* Transfer status onto e-stack */

  *(e_stack++) = status;
}

/* ======================================================================
   op_prname()  -  PRINTER NAME                                           */

void op_prname() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Printer name               |  Status 0 = ok, >0 = ELSE   |
     |                             |         <0 = ON ERROR       |
     |-----------------------------|-----------------------------|
     |  Print unit                 |                             |
     |=============================|=============================|
 */

  DESCRIPTOR *descr;
  PRINT_UNIT *pu;
  u_int16_t op_flags;
  int16_t status = 0;
  char *printer_name = NULL;

  op_flags = process.op_flags;
  process.op_flags = 0;

  pu = tio_find_printer(e_stack - 2, TRUE);
  if (pu == NULL)
    k_err_pu();

  if (pu->unit < 0)
    k_error(sysmsg(1704));

  //0134 if (cproc_level > pu->level) pu->level = cproc_level;

  descr = e_stack - 1;
  k_get_string(descr);
  if (descr->data.str.saddr != NULL) {
    printer_name = alloc_c_string(descr);
  }

  if (validate_printer(printer_name)) {
    set_print_mode(pu, PRINT_TO_PRINTER);

    if (pu->printer_name != NULL)
      k_free(pu->printer_name);
    pu->printer_name = printer_name;
  } else {
    if (printer_name != NULL)
      k_free_ptr(printer_name);
    status = 1;
  }

  if ((status < 0) && !(op_flags & P_ON_ERROR)) {
    k_error(sysmsg(1705));
  }

  k_dismiss();
  (e_stack - 1)->data.value = status;
}

/* ======================================================================
   op_prnl()  -  Print with newline                                       */

void op_prnl() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  String to print               |                             |
     |--------------------------------|-----------------------------|
     |  Print unit                    |                             |
     |================================|=============================|

 */

  PRINT_UNIT *pu;

  pu = tio_find_printer(e_stack - 2, PrinterOn);
  if (pu == NULL)
    k_err_pu();

  if (pu->level == 0)
    pu->level = cproc_level;

  if (print_chunked_string(pu, FALSE)) {
    switch (pu->mode) {
      case PRINT_TO_DISPLAY:
        tio_display_string("\n", 1, TRUE, FALSE);
        break;

      case PRINT_TO_PRINTER:
      case PRINT_TO_FILE:
      case PRINT_TO_AUX_PORT:
      case PRINT_AND_HOLD:
        tio_print_string(pu, pu->newline, strlen(pu->newline), FALSE);
        break;

      case PRINT_TO_STDERR:
        fprintf(stderr, "\n");
        break;
    }
  }

  k_pop(1);
}

/* ======================================================================
   op_prnt()  -  Print without newline                                    */

void op_prnt() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  String to print               |                             |
     |--------------------------------|-----------------------------|
     |  Print unit                    |                             |
     |================================|=============================|

 */

  PRINT_UNIT *pu;

  pu = tio_find_printer(e_stack - 2, PrinterOn);
  if (pu == NULL)
    k_err_pu();

  if (pu->level == 0)
    pu->level = cproc_level;

  print_chunked_string(pu, TRUE); /* String has been popped from stack */
  k_pop(1);
}

/* ======================================================================
   op_proff()  -  PRINTER OFF                                             */

void op_proff() {
  printer_off();
}

/* ======================================================================
   op_pron()  -  PRINTER ON                                               */

void op_pron() {
  printer_on();
}

/* ======================================================================
   op_prompt()  -  Set prompt character                                   */

void op_prompt() {
  DESCRIPTOR *string;
  STRING_CHUNK *str;

  string = e_stack - 1;
  k_get_string(string);
  str = string->data.str.saddr;

  if (str != NULL)
    tio.prompt_char = str->data[0];
  else
    tio.prompt_char = '\0';

  k_dismiss();
}

/* ======================================================================
   op_prreset()  -  PRINTER RESET                                         */

void op_prreset() {
  printer_off();
  tio.dsp.flags |= PU_PAGINATE;
  tio.dsp.page_no = 1;
  tio.dsp.line = 0;

  /* !!PU!! */
  k_free_ptr(tio.dsp.heading);
  k_free_ptr(tio.dsp.footing);
  k_free_ptr(tio.dsp.banner);
  k_free_ptr(tio.dsp.form);
  k_free_ptr(tio.dsp.options);
  k_free_ptr(tio.dsp.prefix);
  k_free_ptr(tio.dsp.overlay);
  k_free_ptr(tio.dsp.spooler);
  k_free_ptr(tio.dsp.style);
}

/* ======================================================================
   op_pterm()  -  PTERM() function                                        */

void op_pterm() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  New value                     |  New value                  |
     |  -ve = report value where      |                             |
     |        allowed                 |                             |
     |--------------------------------|-----------------------------|
     |  Action                        |                             |
     |================================|=============================|

 */

  DESCRIPTOR *descr;
  int32_t new_value;
  int16_t action;
  STRING_CHUNK *str;
  DESCRIPTOR result;

  InitDescr(&result, INTEGER);

  /* Get action */

  descr = e_stack - 2;
  GetInt(descr);
  action = (int16_t)(descr->data.value);

  descr = e_stack - 1;
  switch (action) {
    case PT_BREAK: /* Trap ctrl-c as break? */
      GetInt(descr);
      new_value = descr->data.value;
      if (new_value >= 0)
        set_term((new_value != 0));
      result.data.value = trap_break_char;
      break;

    case PT_INVERT: /* Case inversion */
      GetInt(descr);
      new_value = descr->data.value;
      if (new_value >= 0)
        case_inversion = (new_value != 0);
      result.data.value = case_inversion;
      break;

    case PT_BRKCH: /* Set break character */
      GetInt(descr);
      new_value = descr->data.value;
      if (new_value != 0)
        tio.break_char = (char)new_value;
      result.data.value = tio.break_char;
      break;

    case PT_ONEWLINE: /* Set output newline sequence */
      k_get_string(descr);
      str = descr->data.str.saddr;
      if (str != NULL)
        k_get_c_string(descr, onewline, 10);
      k_put_c_string(onewline, &result);
      break;

    case PT_INEWLINE: /* Set input newline character */
      k_get_string(descr);
      str = descr->data.str.saddr;
      if (str != NULL)
        inewline = str->data[0];
      k_put_string(&inewline, 1, &result);
      break;

    case PT_BINARY_IN: /* Telnet binary mode, client -> server */
      GetInt(descr);
      new_value = descr->data.value;
      if (new_value >= 0)
        telnet_binary_mode_in = (new_value != 0);
      result.data.value = telnet_binary_mode_in;
      break;

    case PT_BINARY_OUT: /* Telnet binary mode, server -> client */
      GetInt(descr);
      new_value = descr->data.value;
      if (new_value >= 0)
        telnet_binary_mode_out = (new_value != 0);
      result.data.value = telnet_binary_mode_out;
      break;

    case PT_TELNET: /* Recognise TN_IAC on input */
      GetInt(descr);
      new_value = descr->data.value;
      if (new_value >= 0)
        telnet_negotiation = (new_value != 0);
      result.data.value = telnet_negotiation;
      break;

    default:
      k_error(sysmsg(1707));
  }

  k_dismiss();
  k_dismiss();
  *(e_stack++) = result;
}

/* ======================================================================
   op_rstscrn()  -  RSTSCRN opcode                                        */

void op_rstscrn() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  Restore cursor, attributes,   |                             |
     |  pagination, etc?              |                             |
     |--------------------------------|-----------------------------|
     |  Screen image string           |                             |
     |================================|=============================|

 */

  DESCRIPTOR *descr;
  SCREEN_IMAGE *screen;
  bool restore_all;

  if ((connection_type == CN_NONE) || is_QMVbSrvr)
    phantom_error();

  descr = e_stack - 1;
  GetInt(descr);
  restore_all = (descr->data.value != 0);

  descr = e_stack - 2;
  k_get_value(descr);
  if (descr->type != IMAGE)
    k_error(sysmsg(1708));

  if (!tio.hush) {
    screen = descr->data.i_addr;
    restore_screen(screen, restore_all);
    if (restore_all) {
      if (screen->pagination)
        tio.dsp.flags |= PU_PAGINATE;
      else
        tio.dsp.flags &= ~PU_PAGINATE;
    }
  }

  k_pop(1);
  k_dismiss();
}

/* ======================================================================
   op_savescrn()  -  SAVESCRN opcode                                      */

void op_savescrn() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  Image area height             | Screen image string         |
     |--------------------------------|-----------------------------|
     |  Image area width              |                             |
     |--------------------------------|-----------------------------|
     |  Image area top line no        |                             |
     |--------------------------------|-----------------------------|
     |  Image area left col no        |                             |
     |================================|=============================|

 */

  DESCRIPTOR *descr;
  int16_t x;
  int16_t y;
  int16_t width;
  int16_t height;
  SCREEN_IMAGE *p;

  if ((connection_type == CN_NONE) || is_QMVbSrvr)
    phantom_error();

  descr = e_stack - 1;
  GetInt(descr);
  height = (int16_t)(descr->data.value);

  descr = e_stack - 2;
  GetInt(descr);
  width = (int16_t)(descr->data.value);

  descr = e_stack - 3;
  GetInt(descr);
  y = (int16_t)(descr->data.value);

  descr = e_stack - 4;
  GetInt(descr);
  x = (int16_t)(descr->data.value);

  /* Validate dimensions */

  if ((x < 0) || (width < 1) || ((x + width) > tio.dsp.width) || (y < 0) || (height < 1) || ((y + height) > tio.dsp.lines_per_page)) {
    k_error(sysmsg(1709));
  }

  p = (SCREEN_IMAGE *)k_alloc(7, sizeof(struct SCREEN_IMAGE));
  if (p == NULL)
    k_error(sysmsg(1710));

  p->ref_ct = 1;
  p->buffer = NULL;
  if (!tio.hush) {
    if (!save_screen(p, x, y, width, height)) {
      k_free(p);
      k_error(sysmsg(1710));
    }
  }

  p->pagination = (tio.dsp.flags & PU_PAGINATE) != 0;

  k_pop(4);

  InitDescr(e_stack, IMAGE);
  (e_stack++)->data.i_addr = p;
}

/* ======================================================================
   freescrn()  -  Free a screen image descriptor                          */

void freescrn(SCREEN_IMAGE *image) {
  char *p;
  char *q;
  int n;

  if ((connection_type == CN_SOCKET) || (connection_type == CN_PORT)) {
    p = qmtgetstr("dreg");
    if (p != NULL) {
      q = tparm(&n, p, (int)(image->id));
      write_socket(q, n, TRUE);
    }
  }

  if (image->buffer != NULL)
    k_free(image->buffer);
  k_free(image);
}

/* ======================================================================
   op_ttyget()  -  Get terminal settings                                  */

void op_ttyget() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |                                | Returned dynamic array      |
     |================================|=============================|

 */

  k_recurse(pcode_ttyget, 0);
}

/* ======================================================================
   op_ttyset()  -  Set terminal settings                                  */

void op_ttyset() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top | Modes to set                   |                             |
     |================================|=============================|

 */

  k_recurse(pcode_ttyset, 1);
}

/* ======================================================================
   como_string()  -  Write string to como file                            */

Private void como_string(char *str, int16_t bytes) {
  if (!tio.suppress_como) {
    Write(tio.como_file, str, bytes);
  }
}

/* ======================================================================
   como_close()  -  Flush and close como file                             */

void como_close() {
  OSFILE handle;

  if (ValidFileHandle(tio.como_file)) {
    CloseFile(tio.como_file);
    tio.como_file = INVALID_FILE_HANDLE;

    if (is_phantom) {
      handle = open(NULL_DEVICE, O_WRONLY);
      dup2(handle, 1);
      dup2(handle, 2);
      close(handle);
    }
  }
}

/* ======================================================================
   settermtype()  -  Set terminal type and send initialisation string     */

bool settermtype(char *name) {
  char *p;
  int n;

  if (!tsettermtype(name))
    return FALSE;

  if (!is_phantom && !is_QMVbSrvr) /* 0542 */
  {
    p = qmtgetstr("is1");
    n = strlen(p);

    if (n) {
      switch (connection_type) {
        case CN_CONSOLE:
          write_console(p, n);
          break;
        case CN_SOCKET:
        case CN_PORT:
          write_socket(p, n, TRUE);
          break;
        case CN_WINSTDOUT:
          fwrite(p, 1, n, stdout);
          break;
      }
    }
  }

  return TRUE;
}

/* ======================================================================
   como_open()  -  Open como file                                         */

Private int como_open(char name[]) {
  if (ValidFileHandle(tio.como_file))
    como_close();

  tio.como_file = dio_open(name, DIO_REPLACE);
  if (!ValidFileHandle(tio.como_file))
    return ER_FNF;

  if (is_phantom) /* 0387 */
  {
    dup2(tio.como_file, 1);
    dup2(tio.como_file, 2);
    close(tio.como_file);
    tio.como_file = 1;
  }

  tio.suppress_como = FALSE;

  return 0;
}

/* ======================================================================
   display_chunked_string()                                               */

Private bool display_chunked_string(bool flush) {
  bool status = TRUE;
  STRING_CHUNK *str;
  DESCRIPTOR *string;

  /* Top of stack must be a string */

  string = e_stack - 1;
  k_get_string(string);

  for (str = string->data.str.saddr; str != NULL; str = str->next) {
    if (!tio_display_string(str->data, str->bytes, FALSE, TRUE)) {
      status = FALSE;
      break;
    }
  }

  if (flush)
    flush_outbuf();

  k_dismiss(); /* Release descriptor at top of stack */

  return status;
}

/* ======================================================================
   emit_header_footer()  -  Emit heading or footing                       */

void emit_header_footer(PRINT_UNIT *pu, bool is_heading) {
  STRING_CHUNK *str;
  DESCRIPTOR *descr;
  int16_t bytes;
  int16_t n;
  char *p;
  char *q;
  bool text_emitted = FALSE;
  bool to_display;

  to_display = (pu->mode == PRINT_TO_DISPLAY);

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = pu->unit;

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = pu->page_no;

  k_put_c_string((is_heading) ? (pu->heading) : (pu->footing), e_stack++);

  k_recurse(pcode_hf, 3); /* SUBROUTINE HF(PU, PGNO, HF.IN) */

  if (to_display) {
    descr = Element(process.syscom, SYSCOM_SYS0);
    if (descr->data.value & 0x0001)
      tio.dsp.flags &= ~PU_PAGINATE;
  }

  /* Now emit the text from the stack */

  descr = e_stack - 1;
  for (str = descr->data.str.saddr; str != NULL; str = str->next) {
    text_emitted = TRUE;
    p = str->data;
    bytes = str->bytes;
    while (bytes > 0) {
      q = (char *)memchr(p, FIELD_MARK, bytes);
      if (q != NULL) {
        n = q - p;
        tio_print_string(pu, p, n, FALSE);
        tio_print_string(pu, pu->newline, strlen(pu->newline), FALSE); /* 0518 */
        pu->line = 0;                                                  /* 0368 */
        p = q + 1;
        bytes -= n + 1;
      } else {
        n = bytes;
        tio_print_string(pu, p, n, FALSE);
        bytes = 0;
      }
    }
  }

  /* Emit trailing newline if we have output anything and this is going to
    the screen or it is a heading.                                        */

  if (text_emitted && (to_display || is_heading)) {
    tio_print_string(pu, pu->newline, strlen(pu->newline), FALSE); /* 0518 */
  }

  k_dismiss();
}

/* ======================================================================
   free_print_units()  -  Release all print unit memory                   */

void free_print_units() {
  PRINT_UNIT *pu;
  PRINT_UNIT *next_pu;

  for (pu = &tio.dsp; pu != NULL; pu = next_pu) {
    next_pu = pu->next;

    free_print_unit(pu);
  }
}

/* ====================================================================== */

Private void phantom_error() {
  k_error(sysmsg(1711));
}

/* ======================================================================
   print_chunked_string()                                                 */

Private bool print_chunked_string(PRINT_UNIT *pu, bool flush) {
  bool status = TRUE;
  STRING_CHUNK *str;
  DESCRIPTOR *string;

  string = e_stack - 1;
  k_get_string(string);

  for (str = string->data.str.saddr; str != NULL; str = str->next) {
    if (!tio_print_string(pu, str->data, str->bytes, TRUE)) {
      status = FALSE;
      break;
    }
  }

  if (flush)
    flush_outbuf();

  k_dismiss(); /* Release descriptor at top of stack */

  return status;
}

/* ======================================================================
   set_data_lines()  -  Find lines per page excluding header and footer   */

Private void set_data_lines(PRINT_UNIT *pu) {
  int data_lines;
  bool control_mode;
  char *q;

  if (pu->lines_per_page == 0)
    data_lines = 0;
  else
    data_lines = pu->lines_per_page - (pu->top_margin + pu->bottom_margin);

  pu->heading_lines = 0;
  pu->footing_lines = 0;

  if (pu->heading != NULL) {
    data_lines--;
    pu->heading_lines++;
    for (q = pu->heading, control_mode = FALSE; *q != '\0'; q++) {
      if (*q == '\'')
        control_mode = !control_mode;
      else if (control_mode && (*q == 'L')) {
        data_lines--;
        pu->heading_lines++;
      }
    }
  }

  if (pu->footing != NULL) {
    data_lines--;
    pu->footing_lines++;
    for (q = pu->footing, control_mode = FALSE; *q != '\0'; q++) {
      if (*q == '\'')
        control_mode = !control_mode;
      else if (control_mode && (*q == 'L')) {
        data_lines--;
        pu->footing_lines++;
      }
    }
  }

  if (data_lines < 0)
    data_lines = 0;
  pu->data_lines_per_page = data_lines;
}

/* ======================================================================
   set_print_mode()  -  Clear down and reset printer mode at change       */

Private void set_print_mode(PRINT_UNIT *pu, int16_t mode) {
  if (pu->mode != mode) {
    /* Clear down old settings */

    if (pu->flags & PU_ACTIVE) {
      switch (pu->mode) {
        case PRINT_TO_PRINTER:
          end_printer(pu);
          break;

        case PRINT_TO_FILE:
        case PRINT_TO_AUX_PORT:
        case PRINT_AND_HOLD:
          end_file(pu);
      }
      if (pu->unit != -1)
        pu->flags &= ~(PU_ACTIVE | PU_OUTPUT);
    }

    pu->mode = mode;
    pu->file.fu = INVALID_FILE_HANDLE;
  }
}

/* ======================================================================
   squeek()  -  Sound terminal "bell"                                     */

void squeek() {
  DESCRIPTOR *descr;

  descr = Element(process.syscom, SYSCOM_BELL);
  if (descr->data.str.saddr != NULL)
    tio_printf("\x07");
}

/* ======================================================================
   tio_display_string()  -  Output a string to the display

   Although any print unit can be redirected to the display, we always
   use the unit -1 (dsp) structure so that we can maintain correct
   pagination, etc.                                                       */

bool tio_display_string(char *q, int16_t bytes, bool flush, bool transparent_newline) {
  int16_t n;
  char *p;

  tio.dsp.flags |= PU_ACTIVE | PU_OUTPUT;

  if (tio.dsp.flags & PU_HDR_NEXT) /* Force page heading */
  {
    tio.dsp.flags &= ~PU_HDR_NEXT;
    /* 1.1-11  The next line was previously conditioned "if (tio.dsp.paginate)" */
    if (tio.dsp.heading != NULL)
      display_header();
  }

  if (tio.hush)
    return TRUE; /* Suppress screen and como output */

  if (!capturing) {
    if (tio.como_file >= 0)
      como_string(q, bytes);

    if (is_QMVbSrvr)
      return TRUE; /* Ignore output from QMServer */

    if (connection_type == CN_NONE)
      return TRUE;
  }

  /* Do terminal output */
  /* 0083  Moved all the capturing stuff into the loop below so that page
    headings get handled correctly.                                      */

  while (bytes) {
    if (!transparent_newline) {
      /* Find first newline in data stream */

      p = (char *)memchr(q, '\n', bytes);
      n = (p != NULL) ? (p - q) : bytes;
    } else {
      n = bytes;
    }

    /* Emit data before newline */

    if (n != 0) {
      if (capturing)
        capture(q, n);
      else {
        switch (connection_type) {
          case CN_CONSOLE:
            write_console(q, n);
            break;
          case CN_SOCKET:
          case CN_PORT:
            write_socket(q, n, FALSE);
            break;
          case CN_WINSTDOUT:
            fwrite(q, 1, n, stdout);
            break;
        }
      }
      q += n;
      bytes -= n;
    }

    if (bytes) /* Terminated by newline */
    {
      tio_new_line();
      q++;
      bytes--;
    }
  }

  if (flush)
    flush_outbuf();

  return TRUE;
}

Private void tio_display_new_page() {
  if (tio.dsp.line && (tio.dsp.flags & PU_PAGINATE))
    display_footer();

  if (!capturing) {
    clr();
    flush_outbuf();
  }
}

Private void capture(char *q, int16_t n) {
  int16_t cn;
  STRING_CHUNK *str;
  int16_t space;

  if (capture_head == NULL) /* Allocate first chunk */
  {
    capture_tail = s_alloc(MAX_STRING_CHUNK_SIZE, &cn);
    capture_tail->string_len = 0;
    capture_tail->ref_ct = 1;
    capture_head = capture_tail;
  }

  /* Copy data */

  while (n) {
    space = capture_tail->alloc_size - capture_tail->bytes;
    if (space == 0) {
      str = s_alloc(MAX_STRING_CHUNK_SIZE, &cn);
      capture_tail->next = str;
      capture_tail = str;
      space = cn;
    }

    cn = min(n, space);
    memcpy(capture_tail->data + capture_tail->bytes, q, cn);
    capture_tail->bytes += cn;
    capture_head->string_len += cn;
    q += cn;
    n -= cn;
  }
}

/* ======================================================================
   display_footer()  -  Display page footer                               */

Private bool display_footer() {
  bool status = TRUE;
  DESCRIPTOR *msg_descr;
  char c;

  displaying_footer = TRUE;

  if (tio.dsp.footing != NULL) {
    while (tio.dsp.line < (tio.dsp.data_lines_per_page - 1)) {
      tio_new_line();
    }
    emit_header_footer(&tio.dsp, FALSE);
  }

  if (!capturing) {
    csr(0, tio.dsp.lines_per_page - 1);
    tio_write(sysmsg(1750)); /* Press RETURN */
    if (!Option(OptNoUserAborts))
      tio_printf(", %s", sysmsg(1751)); /* A option */
    tio_printf(", %s", sysmsg(1752));   /* Q option */
    tio_printf(", %s", sysmsg(1753));   /* S option */

    c = UpperCase((char)keyin(0));

    switch (c) {
      case 'A':
        if (!Option(OptNoUserAborts)) {
          /* Save pseudo abort message in SYSCOM */

          msg_descr = Element(process.syscom, SYSCOM_ABORT_MESSAGE);
          k_release(msg_descr);
          k_put_c_string(sysmsg(1760), msg_descr);

          k_exit_cause = K_ABORT;
        }
        break;

      case 'S':
        tio.dsp.flags &= ~PU_PAGINATE;
        break;

      case 'Q':
        clear_select(0);
        k_exit_cause = K_STOP;
        status = FALSE;
        break;
    }

    if (ValidFileHandle(tio.como_file))
      como_string(Newline, NewlineBytes);
    tio_new_line();
  }

  flush_outbuf();

  displaying_footer = FALSE;
  tio.dsp.page_no++;
  tio.dsp.flags |= PU_HDR_NEXT; /* Header required on next output */
  tio.dsp.line = 0;

  return status;
}

/* ======================================================================
   display_header()  -  Display page header                               */

Private void display_header() {
  int16_t i;

  clr();

  for (i = 0; i < tio.dsp.top_margin; i++) {
    tio_display_string("\n", 1, FALSE, FALSE);
  }

  if (tio.dsp.heading_lines)
    emit_header_footer(&tio.dsp, TRUE);
  flush_outbuf();

  tio.dsp.line = 0;
}

/* ======================================================================
   tio_find_printer()  -  Find print unit structure                       */

Private PRINT_UNIT *tio_find_printer(DESCRIPTOR *descr, /* Print unit descriptor */
                                     bool pr_on)        /* Printer on/off flag.
                              If not set, interprets unit 0 as unit -1 */
{
  PRINT_UNIT *pu;
  int16_t u;

  GetInt(descr);
  u = (int16_t)(descr->data.value);

  if (u == last_pu)
    return last_pu_ptr;

  switch (u) {
    case 0:
      return (pr_on) ? &tio.lptr_0 : &tio.dsp;

    case -1:
      return &tio.dsp;

    default:
      if ((u < LOW_PRINT_UNIT) || (u > HIGH_PRINT_UNIT)) {
        if (u != TEMPLATE_PRINT_UNIT)
          return NULL;
      }

      for (pu = &tio.dsp; pu != NULL; pu = pu->next) {
        if (pu->unit == u) {
          last_pu = u;
          last_pu_ptr = pu;
          return pu;
        }
      }
      break;
  }

  /* New print unit - create as default settings */

  pu = tio_set_printer(u, PRINT_TO_FILE, pcfg.lptrhigh, pcfg.lptrwide, TIO_DEFAULT_TMGN, TIO_DEFAULT_BMGN, TIO_DEFAULT_LMGN);
  last_pu = u;
  last_pu_ptr = pu;
  return pu;
}

/* ======================================================================
   tio_handle_break()  -  Called from kernel on detecting break key

   Returns true if continuing processing, false if not                    */

bool tio_handle_break() {
  SCREEN_IMAGE *image;
  bool status = TRUE;
  bool saved_capturing;
  char c;
  bool ok;
  int32_t offset;
  struct PROGRAM *pgm;
  int handler_return;

  k_exit_cause = 0;

  /* Save screen image */

  image = (SCREEN_IMAGE *)k_alloc(70, sizeof(struct SCREEN_IMAGE));
  if (image == NULL)
    k_error(sysmsg(1710));

  image->ref_ct = 1;
  image->buffer = NULL;

  saved_capturing = capturing;
  capturing = FALSE;

  if (!save_screen(image, 0, 0, tio.dsp.width, tio.dsp.lines_per_page)) {
    k_free(image);
    k_error(sysmsg(1710));
  }

  /* Walk back down stack looking for a break handler */

  for (pgm = &process.program; pgm != NULL; pgm = pgm->prev) {
    if (pgm->break_handler != NULL) {
      k_put_c_string(pgm->break_handler, e_stack++);
      k_recurse(pcode_break, 1); /* Execute recursive code */
      k_exit_cause = 0;
      handler_return = e_stack->data.value;
      k_pop(1);
      switch (handler_return) {
        case 1: /* Go */
          k_exit_cause = 0;
          restore_screen(image, TRUE);
          goto exit_handle_break;

        case 2: /* Quit */
          k_exit_cause = K_STOP;
          status = FALSE;
          goto exit_handle_break;

        case 3: /* Abort */
          k_exit_cause = K_ABORT;
          status = FALSE;
          goto exit_handle_break;

        case 4: /* Logout */
          k_exit_cause = K_LOGOUT;
          status = FALSE;
          goto exit_handle_break;
      }
    }
  }

  tio_printf(sysmsg(1761));

  /* Clear input buffer */

  while (keyready())
    (void)keyin(0);

  /* Get next action */

  do {
    ok = TRUE;

    /* A to abort, Q to quit, G to continue, D to debug, X to exit, ? for help */

    if (!Option(OptNoUserAborts))
      tio_printf("%s, ", sysmsg(1751)); /* A option */
    tio_printf("%s, ", sysmsg(1752));   /* Q option */
    tio_printf("%s, ", sysmsg(1754));   /* G option */
    if (check_debug())
      tio_printf("%s, ", sysmsg(1755)); /* D option */
    tio_printf("%s, ", sysmsg(1756));   /* X option */
    tio_write(sysmsg(1757));            /* ? option */

    c = (char)keyin(0);

    tio_write("\n");

    switch (UpperCase(c)) {
      case 'A':
        if (!Option(OptNoUserAborts)) {
          k_exit_cause = K_ABORT;
          status = FALSE;
        } else {
          ok = FALSE;
          squeek();
        }
        break;

      case 'D':
        if (check_debug()) {
          restore_screen(image, TRUE);
          op_dbgon();
          k_exit_cause = 0;
        } else {
          ok = FALSE;
          squeek();
        }
        break;

      case 'G':
        restore_screen(image, TRUE);
        k_exit_cause = 0;
        break;

      case 'P':
        pdump();
        break;

      case 'Q':
        k_exit_cause = K_STOP;
        /* 0531          status = FALSE; */
        break;

      case 'S':
        show_stack();
        ok = FALSE;
        break;

      case 'W':
        offset = pc - c_base;
        tio_printf("%s %d (%08X)\n", ProgramName(c_base), k_line_no(offset, c_base), offset);
        ok = FALSE;
        break;

      case 'X':
        k_exit_cause = K_LOGOUT;
        status = FALSE;
        break;

      case '?':
        tio_printf(sysmsg(1758));
        ok = FALSE;
        break;

      case 0: /* PSTAT while on input */
        break;

      default:
        squeek();
        ok = FALSE;
        break;
    }
  } while (!ok);

exit_handle_break:
  k_deref_image(image);
  capturing = saved_capturing;

  return status;
}

/* ======================================================================
   tio_init()  -  Initialise tio subsystem                                */

bool tio_init() {
  trap_break_char = TRUE;

  tio.break_char = 3;
  tio.hush = FALSE;

  /* Link static print unit structures */

  tio.dsp.next = &tio.lptr_0;
  tio.dsp.unit = DISPLAY;
  tio.lptr_0.next = NULL;
  tio.lptr_0.unit = 0;

  tio_set_printer(DISPLAY, PRINT_TO_DISPLAY, 25, 80, 0, 0, 0);

  tio_set_printer(0, PRINT_TO_PRINTER, pcfg.lptrhigh, pcfg.lptrwide, 0, 0, 0);

  tio.como_file = INVALID_FILE_HANDLE;

  if ((connection_type != CN_NONE) && !is_QMVbSrvr) {
    if ((connection_type == CN_CONSOLE) || (connection_type == CN_WINSTDOUT)) {
      if (!init_console())
        return FALSE;
    }
  }

  return TRUE;
}

/* ======================================================================
   tio_printf()  -  tio version of printf()                               */

int tio_printf(char *template_string, ...) {
  char s[500];
  va_list arg_ptr;
  int16_t n;

  va_start(arg_ptr, template_string);

  n = vsprintf(s, template_string, arg_ptr);

  tio_display_string(s, strlen(s), TRUE, FALSE);

  va_end(arg_ptr);

  return n;
}

/* ====================================================================== */

Private void tio_new_line() {
  /* 0083 Do newline here rather than in tio_display_string */

  if (capturing)
    capture("\n", 1);
  else {
    switch (connection_type) {
      case CN_CONSOLE:
        write_console(onewline, strlen(onewline));
        break;
      case CN_SOCKET:
      case CN_PORT:
        write_socket(onewline, strlen(onewline), FALSE);
        break;
      case CN_WINSTDOUT:
        fwrite(onewline, 1, strlen(onewline), stdout);
        break;
    }
  }

  tio.dsp.line++;
  if (tio.dsp.flags & PU_PAGINATE) {
    if (tio.dsp.line == (tio.dsp.data_lines_per_page - 1)) {
      if (!displaying_footer)
        display_footer();
    }
  }
}

/* ======================================================================
   tio_print_string()  -  Output a string to a print unit                 */

Private bool tio_print_string(PRINT_UNIT *pu, char *q, int16_t bytes, bool transparent_newline) {
  bool status = TRUE;

  switch (pu->mode) {
    case PRINT_TO_DISPLAY:
      status = tio_display_string(q, bytes, FALSE, transparent_newline);
      break;

    case PRINT_TO_PRINTER:
      to_printer(pu, q, bytes);
      break;

    case PRINT_TO_FILE:
    case PRINT_TO_AUX_PORT:
    case PRINT_AND_HOLD:
      to_file(pu, q, bytes);
      break;

    case PRINT_TO_STDERR:
      fprintf(stderr, "%.*s", bytes, q);
      break;
  }

  return status;
}

/* ======================================================================
   tio_write()  -  Write C string to current screen position              */

void tio_write(char *s) {
  tio_display_string(s, strlen(s), TRUE, FALSE);
}

/* ======================================================================
   tio_set_printer()  -  Set up print unit structure                      */

PRINT_UNIT *tio_set_printer(int16_t unit, int16_t mode, int16_t lines_per_page, int16_t width, int16_t top_margin, int16_t bottom_margin, int16_t left_margin) {
  PRINT_UNIT *p;
  PRINT_UNIT *pu = NULL;

  for (p = &tio.dsp; p != NULL; p = p->next) {
    if (p->unit == unit)
      break;
  }

  if (p == NULL) /* Did not find required printer - make a new print unit */
  {
    p = (PRINT_UNIT *)k_alloc(37, sizeof(struct PRINT_UNIT));
    if (p == NULL)
      k_error(sysmsg(1712));

    p->next = tio.lptr_0.next;
    tio.lptr_0.next = p;

    p->unit = unit;
  }

  /* Do we have a template print unit set up? */

  if (unit != TEMPLATE_PRINT_UNIT) {
    for (pu = &tio.dsp; pu != NULL; pu = pu->next) {
      if (pu->unit == TEMPLATE_PRINT_UNIT)
        break;
    }
  }

  if (pu != NULL) /* Copy template */
  {
    copy_print_unit(pu, p);
    p->flags &= PU_TEMPLATE_MASK;
  } else /* Use default defaults */
  {
    /* !!PU!! */
    p->mode = mode;
    p->flags = 0;
    if (!(pcfg.gdi))
      p->flags |= PU_RAW;
    p->width = width;
    p->lines_per_page = lines_per_page;
    p->top_margin = top_margin;
    p->bottom_margin = bottom_margin;
    p->left_margin = left_margin;
    p->cpi = 1000; /* 10 cpi */
    p->lpi = 6;
    p->paper_size = 26; /* A4 */
    p->copies = 1;
    p->weight = 0;               /* Medium */
    strcpy(p->symbol_set, "8U"); /* Roman-8 */
    strcpy(p->newline, "\n");
    p->printer_name = NULL;
    p->file_name = NULL;
    p->banner = NULL;
    p->heading = NULL;
    p->footing = NULL;
    p->spooler = NULL;
    p->form = NULL;
    p->options = NULL;
    p->prefix = NULL;
    p->overlay = NULL;
    p->style = NULL;
    p->heading_lines = 0;
    p->footing_lines = 0;
    p->file.pathname = NULL;
    set_data_lines(p); /* 0141 */
  }

  p->level = cproc_level;
  p->page_no = 1;
  p->line = 0;
  p->col = 0;
  p->buff = NULL;

  p->file.fu = INVALID_FILE_HANDLE;

  return p;
}

/* ======================================================================
   copy_print_unit()  -  Copy a print unit definition                     */

void copy_print_unit(PRINT_UNIT *old_pu, PRINT_UNIT *new_pu) {
  /* !!PU!! */
  memcpy(new_pu, old_pu, sizeof(PRINT_UNIT));

  new_pu->printer_name = dupstring(old_pu->printer_name);
  new_pu->file_name = dupstring(old_pu->file_name);
  new_pu->banner = dupstring(old_pu->banner);
  new_pu->heading = dupstring(old_pu->heading);
  new_pu->footing = dupstring(old_pu->footing);
  new_pu->spooler = dupstring(old_pu->spooler);
  new_pu->form = dupstring(old_pu->form);
  new_pu->options = dupstring(old_pu->options);
  new_pu->prefix = dupstring(old_pu->prefix);
  new_pu->overlay = dupstring(old_pu->overlay);
  new_pu->style = dupstring(old_pu->style);
  new_pu->file.pathname = dupstring(old_pu->file.pathname);

  set_data_lines(new_pu);
}

/* ======================================================================
   free_print_unit()  -  Release specific print unit memory               */

void free_print_unit(PRINT_UNIT *pu) {
  if (pu->unit == last_pu) {
    last_pu = NO_LAST_PU; /* Kill cache of last referenced print unit */
  }

  /* !!PU!! */
  k_free_ptr(pu->printer_name);
  k_free_ptr(pu->file_name);
  k_free_ptr(pu->heading);
  k_free_ptr(pu->footing);

  set_data_lines(pu); /* 0141 May be needed for static units */

  k_free_ptr(pu->banner);
  k_free_ptr(pu->form);
  k_free_ptr(pu->options);
  k_free_ptr(pu->prefix);
  k_free_ptr(pu->overlay);
  k_free_ptr(pu->spooler);
  k_free_ptr(pu->style);
  k_free_ptr(pu->file.pathname);

  if (pu->unit > 0) /* Dynamically allocated unit */
  {
    k_free(pu);
  }
}

/* ====================================================================== */

void csr(int16_t x, int16_t y) {
  char *p;
  int n;

  if (!is_QMVbSrvr) {
    p = tparm(&n, qmtgetstr("cup"), y, x);

    switch (connection_type) {
      case CN_CONSOLE:
        write_console(p, n);
        break;
      case CN_SOCKET:
      case CN_PORT:
        write_socket(p, n, FALSE);
        break;
      case CN_WINSTDOUT:
        fwrite(p, 1, n, stdout);
        break;
    }
  }
}

/* ====================================================================== */

void clr() {
  char *p;
  int n;

  if (!capturing && !is_QMVbSrvr) /* 0139 */
  {
    p = qmtgetstr("clear");
    n = strlen(p);

    switch (connection_type) {
      case CN_CONSOLE:
        write_console(p, n);
        break;
      case CN_SOCKET:
      case CN_PORT:
        write_socket(p, n, FALSE);
        break;
      case CN_WINSTDOUT:
        fwrite(p, 1, n, stdout);
        break;
    }
  }

  tio.dsp.line = 0;
}

/* ====================================================================== */

void cll() {
  char *p;
  int n;

  if (!is_QMVbSrvr) {
    p = qmtgetstr("el");
    n = strlen(p);

    switch (connection_type) {
      case CN_CONSOLE:
        write_console(p, n);
        break;
      case CN_SOCKET:
      case CN_PORT:
        write_socket(p, n, FALSE);
        break;
      case CN_WINSTDOUT:
        fwrite(p, 1, n, stdout);
        break;
    }
  }
}

/* ======================================================================
   break_key()  -  Action break key                                       */

void break_key() {
  if (!in_debugger) {
    if (process.break_inhibits || (recursion_depth && !(process.program.flags & HDR_ALLOW_BREAK))) {
      break_pending = TRUE;
    } else
      k_exit_cause = K_QUIT;
  }
}

/* ======================================================================
   op_readpkt()  - Read QMClient data packet                              */

void op_readpkt() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |                                | Received string             |
     |================================|=============================|

 */

  int32_t packet_bytes; /* Total packet length including header */
  STRING_CHUNK *str_hdr = NULL;
  STRING_CHUNK *str = NULL;
  STRING_CHUNK *tail;
  int16_t chunk_bytes;
  int n;

  /* Read packet length from header */

  if (!read_socket((char *)&packet_bytes, 4))
    goto exit_op_readpkt;
  if (process.status == ER_EOF)
    goto exit_op_readpkt;

#ifdef BIG_ENDIAN_SYSTEM
  packet_bytes = swap4(packet_bytes);
#endif

  /* Read remainder of packet into user string */

  packet_bytes -= 4; /* Remove length header from packet size */
  while (packet_bytes > 0) {
    str = s_alloc(packet_bytes, &chunk_bytes);
    if (str_hdr == NULL) {
      str_hdr = str;
      str_hdr->string_len = 0;
      str_hdr->ref_ct = 1;
    } else {
      tail->next = str;
    }
    tail = str;

    n = min(chunk_bytes, packet_bytes);
    if (!read_socket(str->data, n))
      goto exit_op_readpkt;

    str->bytes = n;
    str_hdr->string_len += n;
    packet_bytes -= n;
  }

exit_op_readpkt:
  InitDescr(e_stack, STRING);
  (e_stack++)->data.str.saddr = str_hdr;
}

/* ======================================================================
   op_writepkt()  -  Write QMClient data packet                           */

void op_writepkt() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top | String to send                 |                             |
     |================================|=============================|

 */

  DESCRIPTOR *descr;
  STRING_CHUNK *str;
  int32_t packet_length;

  descr = e_stack - 1;
  k_get_string(descr);
  str = descr->data.str.saddr;

  /* Send packet length header */

  packet_length = 4;
  if (str != NULL)
    packet_length += str->string_len;

#ifdef BIG_ENDIAN_SYSTEM
  packet_length = swap4(packet_length);
#endif

  if (!write_socket((char *)&packet_length, 4, FALSE))
    goto exit_op_writepkt;

  /* Send packet data */

  while (str != NULL) {
    if (!write_socket(str->data, str->bytes, FALSE))
      goto exit_op_writepkt;

    str = str->next;
  }

  flush_outbuf();

exit_op_writepkt:
  k_dismiss();
}

/* ======================================================================
   process_client_input()  -  Process input from client (INPUT, INPUT@)

   The descriptor at the top of the stack must be an ADDR to the target
   variable. This will be dismissed before returning to the caller.       */

Private void process_client_input() {
  DESCRIPTOR *descr;
  struct {
    int32_t packet_len;
    int16_t server_error;
  } header_packet;
#define HDR_PKT_SIZE 6

  struct {
    int32_t packet_len;
    int16_t function;
  } in_packet;
#define IN_PKT_SIZE 6

  STRING_CHUNK *str;
  STRING_CHUNK *str_hdr;
  STRING_CHUNK *tail;
  int32_t packet_bytes;
  int16_t chunk_bytes;
  int32_t n;
  /* int16_t function; variable set but never used */

  /* Send any output */

  if (capturing) /* 0254 Reworked */
  {
    /* Send packet header */
    /* 0272 This must be done in two steps to resolve alignment
      differences between compilers.                           */

    header_packet.packet_len = HDR_PKT_SIZE + 4; /* 0471 */
    if (capture_head != NULL)
      header_packet.packet_len += capture_head->string_len;
    header_packet.server_error = SV_PROMPT;

#ifdef BIG_ENDIAN_SYSTEM
    header_packet.packet_len = swap4(header_packet.packet_len);
    header_packet.server_error = swap2(header_packet.server_error);
#endif

    write_socket((char *)&header_packet, HDR_PKT_SIZE, FALSE);

#ifdef BIG_ENDIAN_SYSTEM
    n = swap4(process.status);
    write_socket((char *)&n, 4, FALSE);
#else
    write_socket((char *)&process.status, 4, FALSE);
#endif

    /* Send data chunks */

    for (str = capture_head; str != NULL; str = str->next) {
      write_socket(str->data, str->bytes, FALSE);
    }

    flush_outbuf();

    if (capture_head != NULL) {
      s_free(capture_head);
      capture_tail = NULL;
      capture_head = NULL;
    }
  }

  /* Read incoming packet */

  descr = e_stack - 1;
  while (descr->type == ADDR)
    descr = descr->data.d_addr;
  k_release(descr);
  InitDescr(descr, STRING);
  descr->data.str.saddr = NULL;
  str_hdr = NULL;

  /* Read header */

  if (!read_socket((char *)&in_packet, IN_PKT_SIZE))
    return; /* Socket closed */

#ifdef BIG_ENDIAN_SYSTEM
  packet_bytes = swap4(in_packet.packet_len);
  /* function = swap2(in_packet.function); variable set but never used */
#else
  packet_bytes = in_packet.packet_len;
  /* function = in_packet.function; variable set but never used */
#endif

  /* Read remainder of packet into user string */

  packet_bytes -= IN_PKT_SIZE; /* Remaining bytes to read */
  while (packet_bytes > 0) {
    str = s_alloc(packet_bytes, &chunk_bytes);
    if (str_hdr == NULL) {
      str_hdr = str;
      str_hdr->string_len = 0;
      str_hdr->ref_ct = 1;
    } else {
      tail->next = str;
    }
    tail = str;

    n = min(chunk_bytes, packet_bytes);
    if (!read_socket(str->data, n))
      break;

    str->bytes = (int16_t)n;
    str_hdr->string_len += n;
    packet_bytes -= n;
  }

  /* Check action code */

  switch (in_packet.function) {
    case SrvrRespond:
      descr->data.str.saddr = str_hdr;
      k_dismiss();
      break;

    case SrvrEndCommand:
      s_free(str_hdr);
      k_exit_cause = K_STOP;
      break;

    default:
      s_free(str_hdr);
      k_error(sysmsg(1800), in_packet.function);
      break;
  }
}

/* ======================================================================
   pkt_debug()  -  Debug function                                         */

/* ======================================================================
   write_socket()  -  Write to socket / pipe                              */

bool write_socket(char *str, int bytes, bool flush) {
  int n;
  char *p;
  int16_t mode;
#define WS_CHAR_FF 1
#define WS_CR 2

  if (bytes < 0)
    bytes = strlen(str);

  switch (connection_type) {
    case CN_SOCKET:
      if (is_QMVbSrvr) /* Do not do TN parameter mangling */
      {
        if (!to_outbuf(str, bytes))
          return FALSE;
      } else {
        while (bytes) {
          /* Find first character requiring special treatment */

          n = bytes;
          mode = 0;
          if ((p = (char *)memchr(str, (unsigned char)0xFF, n)) != NULL) {
            n = p - str;
            mode = WS_CHAR_FF;
          }

          if (!telnet_binary_mode_out) /* 0540 */
          {
            if (n && (p = (char *)memchr(str, '\r', n)) != NULL) {
              n = p - str;
              mode = WS_CR;
            }
          }

          if (n) {
            if (!to_outbuf(str, n))
              return FALSE;
            bytes -= n;
            str += n;
          }

          switch (mode) {
            case WS_CHAR_FF:
              if (!to_outbuf("\xff\xff", 2))
                return FALSE; /* 0264 */
              bytes--;
              str++;
              break;

            case WS_CR:
              if (!to_outbuf("\r\x00", 2))
                return FALSE;
              bytes--;
              str++;
              break;
          }
        }
      }

      if (flush) {
        if (!flush_outbuf())
          return FALSE;
      }
      break;

    case CN_PIPE:
    case CN_PORT:
      if (write(1, str, bytes) != bytes)
        return FALSE; /* 0349 */
      break;
  }

  return TRUE;
}

/* ======================================================================
   to_outbuf()  -  Copy data to socket output buffer                      */

bool to_outbuf(char *str, int bytes) {
  int bytes_available;
  int n;

  while (bytes) {
    bytes_available = OUTBUF_SIZE - outbuf_bytes;
    n = min(bytes_available, bytes);
    memcpy(outbuf + outbuf_bytes, str, n);
    outbuf_bytes += n;
    bytes -= n;
    str += n;
    if (bytes) /* Buffer must be full */
    {
      if (!flush_outbuf())
        return FALSE;
    }
  }

  return TRUE;
}

/* ======================================================================
   help()  -  Invoke Windows help system for given key                    */

int help(char *key) {
  char cmd[MAX_PATHNAME_LEN * 2 + 1];
  char *p;
  int n;

  while (*key == ' ')
    key++;

  if (*key != '\0') {
    sprintf(cmd, "hh.exe c:\\qmsys\\qm.chm::%s.htm", key);
  } else {
    sprintf(cmd, "hh.exe c:\\qmsys\\qm.chm");
  }

  if (is_QMTerm) {
    /* Although QMTerm now supports use of u8, this code is retained
      to ensure that old versions of QMTerm still work.              */

    sprintf(cmd, "\x1B[H%s\n", key);
    write_socket(cmd, strlen(cmd), TRUE);
  } else {
    p = qmtgetstr("u8");
    if (*p == '\0')
      return ER_TERMINFO; /* No command code binding */
    strcpy(cmd, tparm(&n, p, cmd));
  }

  if (connection_type == CN_CONSOLE)
    tio_write(cmd); /* 0404 */
  else
    write_socket(cmd, strlen(cmd), TRUE);

  return 0;
}

/* ====================================================================== */

Private void printer_off() {
  struct PROGRAM *prg;

  for (prg = &process.program; prg != NULL; prg = prg->prev) {
    prg->flags &= ~PF_PRINTER_ON; /* 0500 */
    if (prg->flags & HDR_IS_CPROC)
      break;
  }
}

/* ====================================================================== */

Private void printer_on() {
  struct PROGRAM *prg;

  for (prg = &process.program; prg != NULL; prg = prg->prev) {
    prg->flags |= PF_PRINTER_ON; /* 0500 */
    if (prg->flags & HDR_IS_CPROC)
      break;
  }
}

/* ======================================================================
   lock_beep()  -  Beep on lock wait                                      */

void lock_beep() {
  bool saved_suppress_como;

  if (!is_phantom && !is_QMVbSrvr) {
    if (Option(OptLockBeep)) {
      if ((++lock_beep_timer % 4) == 0) {
        saved_suppress_como = tio.suppress_como;
        tio.suppress_como = TRUE;
        squeek();
        tio.suppress_como = saved_suppress_como;
      }
    }
  }
}

/* END-CODE */
