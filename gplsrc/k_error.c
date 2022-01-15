/* K_ERROR.C
 * Kernel error handling
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
 * 15Jan22 gwb Fixed argument formatting issues (CwE-686) 
 * 
 * 11Jan22 gwb Fix for Issue #13 ("Uncontrolled format string")
 * 
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 *
 * 20 Jun 12 gwb Fixed a compiler warning when calling sprintf() without a format
 *               specifier.  (see "20Jun12" mark in k_error() )
 *
 * START-HISTORY (OpenQM):
 * 23 Mar 09 gwb removed two of the "fixes" - done incorrectly.
 * 23 Mar 09 gwb Fixed sprintf() calls that were missing explicit formatting 
 *               strings.  (warnings were triggered by the addition of the 
 *               -Wformat=2 compiler directive.
 *
 * 13 Sep 07  2.6-3 0562 Reset in_sh flag at error.
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 05 Feb 07  2.4-20 Added transparent_newline argument to tio_display_string().
 * 21 Nov 06  2.4-17 Revised interface to dio_open().
 * 27 Jun 06  2.4-5 Allow use of log_printf() from non-user process.
 * 16 Jun 06  2.4-5 Ensure terminal is in appropriate state to display the error
 *                  in k_error().
 * 05 Mar 06  2.3-8 Combined two buffers in log_message().
 * 17 Jan 06  2.3-5 0450 k_var_name was using the wrong pointer when moving from
 *                  one common chunk to the next.
 * 21 Dec 05  2.3-3 Trim front off errlog file when size limit reached.
 * 16 Oct 05  2.2-15 0423 Close the log file in log_message().
 * 30 Jun 05  2.2-3 Added k_not_socket().
 * 29 Jun 05  2.2-1 Added k_illegal_call_name().
 * 09 Mar 05  2.1-8 Added k_strnum_len().
 * 15 Oct 04  2.0-5 Added errlog handling.
 * 20 Sep 04  2.0-2 Use message handler.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * If the message text starts with ! the call is not fatal and returns.
 *
 * op_illegal()            Report "Illegal opcode"
 * k_dh_error()            Report "Needs dynamic file"
 * k_index_error()         Report "Array subscript out of range"
 * k_not_array_error()     Report "Expected array variable"
 * k_value_error()         Report "Value not found where expected"
 * k_non_numeric_error()   Report "Non-numeric where numeric required"
 * k_select_range_error()  Report "Select list number out of range"
 * k_unassigned()          Report "Unassigned variable"
 * k_err_pu()              Report "Invalid print unit"
 * k_data_type()           Report "Incorrect data type"
 * k_deadlock()            Report "Deadlock detected"
 * k_txn_error()           Report "File operation not allowed in transaction"
 * k_illegal_call_name()   Report "Illegal call name"
 * k_error()               Report error using supplied text and abort
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include "header.h"
#include "syscom.h"
#include "options.h"
#include "tio.h"
#include "qmtermlb.h"

#include <setjmp.h>
#include <stdarg.h>
#include <time.h>

extern jmp_buf k_exit;
extern char* month_names[];

extern bool in_sh; /* 0562 Doing SH command? */

Private int32_t line_table_offset;
Private int32_t line_table_bytes;
void null_function(void);

Private void k_descr_error(int16_t err, DESCRIPTOR* descr);

/* ====================================================================== */

void op_illegal() {
  k_error("Illegal opcode x%02X", (unsigned int)(*(pc - 1)));
}

void op_illegal2() {
  k_error("Illegal opcode x%02X%02X", (unsigned int)(*(pc - 2)),
          (unsigned int)(*(pc - 1)));
}

/* ======================================================================
   General descriptor based message handler                               */

#define DescrError(msg_no, name) \
  void name(DESCRIPTOR* descr) { k_descr_error(msg_no, descr); }

DescrError(1100, k_index_error) DescrError(1101, k_dh_error)
    DescrError(1102, k_not_array_error) DescrError(1103, k_value_error)
        DescrError(1104, k_non_numeric_error) DescrError(1105, k_unassigned)
            DescrError(1106, k_null_id) DescrError(1107, k_illegal_id)
                DescrError(1108, k_div_zero_error)
                    DescrError(1109, k_unassigned_zero)
                        DescrError(1110, k_unassigned_null)
                            DescrError(1111, k_div_zero_warning)
                                DescrError(1112, k_non_numeric_warning)
                                    DescrError(1113, k_data_type)
                                        DescrError(1137, k_strnum_len)
                                            DescrError(1138, k_not_socket)

                                                Private
    void k_descr_error(int16_t msg_no, DESCRIPTOR* descr) {
  char* var;
  char message[80 + 1];
  char* p;

  strcpy(message, sysmsg(msg_no));
  p = strchr(message, '|');
  if ((var = k_var_name(descr)) != NULL) {
    *p = ' ';
    k_error(message, var);
  } else {
    *p = '\0';
    k_error(message);
  }
}

/* ======================================================================
   Simple messages                                                        */

void k_deadlock() {
  k_error(sysmsg(1114));
}

void k_err_pu() {
  k_error(sysmsg(1115));
}

void k_inva_task_lock_error() {
  k_error(sysmsg(1116));
}

void k_nary_length_error() {
  k_error(sysmsg(1117));
}

void k_select_range_error() {
  k_error(sysmsg(1118));
}

void k_txn_error() {
  k_error(sysmsg(1119));
}

void k_illegal_call_name() {
  k_error(sysmsg(1130));
}

/* ====================================================================== */

void k_error(char* message, ...) {
  int32_t failing_offset;
  char s[(MAX_ERROR_LINES * MAX_EMSG_LEN) + 1]; /* Max 3 lines */
  va_list arg_ptr;
  int16_t n;
  int line;
  struct PROGRAM* pgm;
  int32_t xpc_offset;
  u_char* xcbase;
  DESCRIPTOR* msg_descr;
  bool fatal;

  /* Ensure terminal is in an appropriate mode to receive the error message */

  tio.hush = FALSE;
  tio.suppress_como = TRUE;
  tio_write(qmtgetstr("rlt"));
  tio.suppress_como = FALSE;

  in_sh = FALSE; /* 0562 */

  fatal = (*message != '!');
  if (!fatal)
    message++;

  process.numeric_array_allowed = FALSE;

  /* PC may now point anywhere from one byte after the start of the failing
    opcode to the first byte of the next opcode. Back up by one byte to
    place us within the current opcode.                                    */

  if (c_base != NULL) {
    /* Track back through any recursives to report error as belonging to
      parent program.                                                    */

    xpc_offset = pc - c_base;
    xcbase = c_base;

    pgm = &process.program;
    while (pgm->flags & HDR_RECURSIVE) {
      if (internal_mode) {
        tio_printf("%08X in %s \n", xpc_offset - 1,
                   ((OBJECT_HEADER*)xcbase)->ext_hdr.prog.program_name);
      }

      pgm = pgm->prev;
      xpc_offset = pgm->saved_pc_offset;
      xcbase = pgm->saved_c_base;
    }

    failing_offset = xpc_offset - 1;
    n = sprintf(s, "%08X: ", failing_offset);
  } else {
    n = 0;
  }

  va_start(arg_ptr, message);
  /* Fix for Issue #13.  Converted a vsprintf() to vsnprintf(). -gwb */
  vsnprintf(&(s[n]), (MAX_ERROR_LINES + MAX_EMSG_LEN) + 1,  message, arg_ptr);
  va_end(arg_ptr);

  if (c_base == NULL) /* No object currently loaded */
  {
    tio_write(s);
    tio_write("\n");
    k_exit_cause = K_LOGOUT;
    longjmp(k_exit, k_exit_cause);
  }

  n = strlen(s);
  if (process.program.flags & HDR_ITYPE) {
    // sprintf(s + n, sysmsg(1120)); /* in dictionary expression */
    sprintf(s + n, "%s", sysmsg(1120)); /* 20Jun12 gwb #1 */

  } else {
    line = k_line_no(failing_offset, xcbase);
    if (line >= 0) {
      sprintf(s + n, sysmsg(1121), (int)line,
              ((OBJECT_HEADER*)xcbase)->ext_hdr.prog.program_name);
    } else {
      sprintf(s + n, sysmsg(1122),
              ((OBJECT_HEADER*)xcbase)->ext_hdr.prog.program_name);
    }
  }
  tio_write(s);
  tio_write("\n");

  log_message(s);

  if (fatal) {
    /* Save abort message in SYSCOM */

    msg_descr = Element(process.syscom, SYSCOM_ABORT_MESSAGE);
    k_release(msg_descr);
    k_put_c_string(s, msg_descr);

    if (Option(OptShowStackOnError))
      show_stack();

    if (Option(OptDumpOnError))
      pdump();

    k_exit_cause = K_ABORT;
    longjmp(k_exit, k_exit_cause);
  }
}

/* ======================================================================
   k_quit  -  raise quit event. (Also used as opcode)

   BEWARE: This function can be called in mid-opcode (eg during a long
   terminal display).                                                     */

void k_quit() {
  k_exit_cause = K_QUIT;
  longjmp(k_exit, k_exit_cause);
}

/* ======================================================================
   k_line_no()
   Return line number for given PC into program, -1 if cannot locate      */

int k_line_no(int32_t offset, u_char* xcbase) {
  OBJECT_HEADER* obj_header;
  int32_t line_table_end;
  int line;
  int32_t line_pc;
  int16_t bytes;
  u_char* p;

  obj_header = (OBJECT_HEADER*)xcbase;

  line_table_offset = obj_header->line_tab_offset;
  if (line_table_offset) {
    /* The first entry in the line table is for line 0, the fixed stuff on
      the front of the program.                                           */

    line_table_end = obj_header->sym_tab_offset;
    if (line_table_end == 0)
      line_table_end = obj_header->object_size;
    line_table_bytes = line_table_end - line_table_offset;

    p = xcbase + line_table_offset;
    line = 0;
    line_pc = 0;
    while (line_table_bytes-- > 0) {
      bytes = (int16_t)(*(p++));
      if (bytes == 255) {
        bytes = (int16_t)(*(p++));
        bytes |= ((int16_t)(*(p++))) << 8;
        line_table_bytes -= 2;
      }
      line_pc += bytes;

      if (offset < line_pc)
        return line;

      line += 1;
    }
  }

  return -1;
}

/* ======================================================================
   k_var_name()  - Identify variable by name                              */

char* k_var_name(DESCRIPTOR* descr) {
  struct PROGRAM* pgm;
  OBJECT_HEADER* obj_header;
  int32_t sym_tab_offset;
  int16_t d_no; /* Descriptor number within program */
  DESCRIPTOR* d;
  DESCRIPTOR* cd;
  int16_t cd_no;
  ARRAY_HEADER* ahdr;
  ARRAY_CHUNK* achnk;
  int16_t c_no = -1; /* Common block variable index */
  ARRAY_HEADER* chdr;  /* Common block header... */
  ARRAY_CHUNK* cchnk;  /* ...and chunk */
  int32_t cols;
  int32_t element = -1;
  static char name[80 + 1]; /* Extracted variable name */
  u_char* p;
  u_char* q;
  int16_t i;
  int16_t j;
  int16_t n;
  u_char c;
  u_char* xcbase;

  if (descr == NULL)
    return NULL;

  /* Track back through any recursives to report variable error as
    belonging to parent program.                                   */

  xcbase = c_base;
  pgm = &process.program;
  while (pgm->flags & HDR_RECURSIVE) {
    pgm = pgm->prev;
    xcbase = pgm->saved_c_base;
  }

  obj_header = (OBJECT_HEADER*)xcbase;
  if ((sym_tab_offset = obj_header->sym_tab_offset) == 0)
    return NULL;

  /* Follow ADDR chain to base descriptor, stopping if we find an ADDR
    that represents a subroutine argument.                                */

  while (descr->type == ADDR) {
    if (descr->flags & DF_ARG)
      break;
    descr = descr->data.d_addr;
  }

  /* Check decriptor is in variable area */

  d_no = descr - pgm->vars;
  if ((d_no >= 0) && (d_no < pgm->no_vars))
    goto found;

  null_function(); /* Just to stop the compiler optimiser */

  /* No, is it a common variable or an array element? */

  for (d_no = 0, d = pgm->vars; d_no < pgm->no_vars; d_no++, d++) {
    switch (d->type) {
      case ARRAY:
        element = 0;
        ahdr = d->data.ahdr_addr;
        cols = ahdr->cols;
        for (i = 0; i < ahdr->num_chunks; i++) {
          achnk = ahdr->chunk[i];
          if ((descr >= achnk->descr) &&
              (descr < (achnk->descr + achnk->num_descr))) {
            element += descr - achnk->descr;
            goto found;
          }

          element += achnk->num_descr;
        }
        break;

      case COMMON:
        c_no = 0;
        chdr = d->data.ahdr_addr;
        for (j = 0; j < chdr->num_chunks; j++) {
          cchnk = chdr->chunk[j];
          if ((descr >= cchnk->descr) &&
              (descr < (cchnk->descr + cchnk->num_descr))) {
            /* Variable is a simple common block entry */
            c_no += descr - cchnk->descr - 1; /* -1 to skip block name */
            goto found;
          }

          /* Check for arrays in this common chunk */

          for (cd_no = 0, cd = cchnk->descr; cd_no < cchnk->num_descr;
               cd_no++, cd++) {
            if (cd->type == ARRAY) {
              element = 0;
              ahdr = cd->data.ahdr_addr;
              cols = ahdr->cols;
              for (i = 0; i < ahdr->num_chunks; i++) {
                achnk = ahdr->chunk[i];
                if ((descr >= achnk->descr) &&
                    (descr < (achnk->descr + achnk->num_descr))) {
                  c_no += cd - cchnk->descr - 1; /* -1 to skip block name */
                  element += descr - achnk->descr;
                  goto found;
                }

                element += achnk->num_descr;
              }
            }
          }

          c_no += cchnk->num_descr; /* 0450 */
        }
        c_no = -1; /* Didn' find it - make sure we don't treat as common */
        break;
    }
  }

  return NULL;

found:
  /* Identify the variable at offset d_no */

  p = xcbase + sym_tab_offset;

  for (i = 0;
       (i < d_no) && ((q = (u_char*)strchr((char*)p, VALUE_MARK)) != NULL);
       i++) {
    p = q + 1;
  }

  if (i == d_no) /* Found start of symbol */
  {
    q = (u_char*)name;
    if (c_no >= 0) /* Common block reference */
    {
      *(q++) = '/';
    }

    for (i = 0; i < 80; i++) {
      if ((p == NULL) || IsMark(*p) || (*p == '\0'))
        break;
      *(q++) = *(p++);
    }

    if (c_no >= 0) /* Add common block variable name */
    {
      *(q++) = '/';

      p = xcbase + sym_tab_offset;
      while ((p = (u_char*)strchr((char*)p, (char)FIELD_MARK)) != NULL) {
        p++; /* Skip field mark */

        /* Extract common block number */

        n = 0;
        while (IsDigit(*p))
          n = (n * 10) + (*(p++) - '0');

        if (n == d_no) /* Found this common */
        {
          p++; /* Skip VM before first name */

          /* Step through to required variable name */

          for (j = 0; (j < c_no) && (p != NULL); j++) {
            while ((((char)(c = *(p++))) != SUBVALUE_MARK) && (c != '\0')) {
            }
            if (c == '\0')
              p = NULL;
          }

          /* Copy variable name */

          if (p != NULL) {
            while ((!IsMark(*p)) && (*p != '\0'))
              *(q++) = *(p++);
            break;
          }
        }
      }
    }

    switch (element) {
      case -1: /* Not an array */
        *q = '\0';
        break;

      case 0: /* Zero element */
        sprintf((char*)q, (cols == 0) ? "(0)" : "(0,0)");
        break;

      default: /* Some other element */
        if (cols == 0)
          sprintf((char*)q, "(%d)", element);
        else
          sprintf((char*)q, "(%d,%d)", ((element - 1) / cols) + 1,
                  ((element - 1) % cols) + 1);
        break;
    }
  }

  return name;
}

void null_function() {}

/* ======================================================================
   log_message()  -  Add message to error log                             */

void log_message(char* msg) {
  OSFILE errlog;
  time_t timenow;
  struct tm* ltime;
  int bytes;
  int src;
  int dst;
#define BUFF_SIZE 4096
  char buff[BUFF_SIZE];
  char* p;

  if (sysseg->errlog) {
    StartExclusive(ERRLOG_SEM, 66);

    sprintf(buff, "%s%cerrlog", sysseg->sysdir, DS);
    errlog = dio_open(buff, DIO_OVERWRITE);

    if (ValidFileHandle(errlog)) {
      if (filelength64(errlog) >= sysseg->errlog) {
        /* Must trim front from log file */

        src = sysseg->errlog / 2; /* Move from here... */
        dst = 0;                  /* ...to here */

        Seek(errlog, src, SEEK_SET);
        bytes = Read(errlog, buff, BUFF_SIZE);
        src += bytes;

        /* Find the first newline (there must be one) */

        p = ((char*)memchr(buff, '\n', bytes)) + 1;
        bytes -= (p - buff); /* Bytes to copy */

        do {
          Seek(errlog, dst, SEEK_SET);
          dst += Write(errlog, p, bytes);

          Seek(errlog, src, SEEK_SET);
          bytes = Read(errlog, buff, BUFF_SIZE);

          src += bytes;
          p = buff;
        } while (bytes > 0);

        /* Truncate the file */

        chsize64(errlog, dst);
      }

      Seek(errlog, 0, SEEK_END);

      timenow = time(NULL);
      ltime = localtime(&timenow);

      /* The buffer trimming mechanism above requires that the file is
        opened in binary mode on Windows systems otherwise the destination
        pointer loses track because Windows is busy modifying the data
        written to the file. Therefore, we must do our own handling of
        the CRLF newline pair on Windows. To do this in a platform
        independent way, we use the Newline macro in the sprintf() below
        instead of the more obvious use of \n.                            */

      if (my_uptr != NULL) {
        bytes = sprintf(
            buff,
            "%02d %.3s %02d %02d:%02d:%02d User %d (pid %d, %s):%s   %s%s",
            ltime->tm_mday, month_names[ltime->tm_mon], ltime->tm_year % 100,
            ltime->tm_hour, ltime->tm_min, ltime->tm_sec, my_uptr->uid,
            my_uptr->pid, my_uptr->username, Newline, msg, Newline);
      } else {
        bytes = sprintf(buff, "%02d %.3s %02d %02d:%02d:%02d:%s   %s%s",
                        ltime->tm_mday, month_names[ltime->tm_mon],
                        ltime->tm_year % 100, ltime->tm_hour, ltime->tm_min,
                        ltime->tm_sec, Newline, msg, Newline);
      }

      Write(errlog, buff, bytes);

      CloseFile(errlog); /* 0423 */
    }

    EndExclusive(ERRLOG_SEM);
  }
}

/* ======================================================================
   op_logmsg()  -  Log a message in the errlog file                       */

void op_logmsg() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Message to log             |                             |
     |=============================|=============================|
 */

#define MAX_LOG_MSG_LEN 500

  DESCRIPTOR* descr;
  char msg[MAX_LOG_MSG_LEN + 1];

  descr = e_stack - 1;
  k_get_c_string(descr, msg, MAX_LOG_MSG_LEN);

  log_message(msg);

  k_dismiss();
}

/* ======================================================================
   log_printf()  -  tio version of printf() with logging to errlog        */

int log_printf(char* template_string, ...) {
  char s[MAX_LOG_MSG_LEN];
  va_list arg_ptr;
  int16_t n;
  int bytes;

  va_start(arg_ptr, template_string);

  n = vsprintf(s, template_string, arg_ptr);

  bytes = strlen(s);

  /* If this is a logged in user, display the message too */

  if (my_uptr != NULL)
    tio_display_string(s, bytes, TRUE, FALSE);

  /* Log message, removing possible trailing newline */

  if (s[bytes - 1] == '\n')
    s[bytes - 1] = '\0';
  log_message(s);

  va_end(arg_ptr);

  return n;
}

/* ======================================================================
   log_permissions_error()                                                */

void log_permissions_error(FILE_VAR* fvar) {
  char msg[MAX_PATHNAME_LEN + 50];

  if (abs(process.status) == ER_PERM) {
    sprintf(msg, "Permissions error on %s",
            (char*)(FPtr(fvar->file_id)->pathname));
  } else {
    sprintf(msg, "Update to read-only file %s",
            (char*)(FPtr(fvar->file_id)->pathname));
  }
  log_message(msg);
}

/* END-CODE */
