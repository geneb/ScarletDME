/* TO_FILE.C
 * File output from terminal i/o
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
 * 09Jan22 gwb Fixed a format specifier warning. (%ld should have been %d)
 *
 * 27Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 * 22Feb20 gwb Converted sprintf() to snprintf() in start_file().
 * 
 * START-HISTORY (OpenQM):
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 05 Feb 07  2.4-20 Added transparent_newline argument to tio_display_string().
 * 21 Nov 06  2.4-17 Revised interface to dio_open().
 * 15 Nov 06  2.4-16 0527 Moved printing of mode 1 on Linux to here.
 * 13 Oct 06  2.4-15 Added PRINT_AND_HOLD handling.
 * 16 Aug 06  2.4-11 Added PRINT_TO_AUX_PORT handling.
 * 11 Apr 06  2.4-1 Allow page length of zero as infinite.
 * 21 Oct 05  2.2-15 Include error number in sysmsg(1716).
 * 18 Oct 05  2.2-15 Added overlay handling.
 * 16 Oct 05  2.2-15 Added PCL duplex printing modes.
 * 06 Sep 05  2.2-9 Emit header and footer once on NFMT output.
 * 01 Aug 05  2.2-6 0382 Introduced PU_OUTPUT flag and used to test whether
 *                  heading needs page throw.
 * 19 Jul 05  2.2-4 Added support for prefix strings.
 * 20 Jan 05  2.1-3 0306 NFMT mode must not interpret any control codes.
 * 20 Sep 04  2.0-2 Use message handler.
 * 20 Sep 04  2.0-2 Use dynamic pcode loader.
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
#include "tio.h"
#include "qmtermlb.h"

#define FILE_BUFF_SIZE 2048

void to_file(PRINT_UNIT* pu, char* str, int16_t bytes);

Private bool printing_footer = FALSE;

Private void start_file(PRINT_UNIT* pu);
Private void print_footer(PRINT_UNIT* pu);
Private void print_header(PRINT_UNIT* pu);
Private void emit(PRINT_UNIT* pu, char* str, int16_t bytes);

/* ======================================================================
   to_file  -  Send text to file                                          */

void to_file(PRINT_UNIT* pu, char* str, int16_t bytes) {
  char* p;
  int16_t n;
  int16_t mode;
#define NL 1
#define FF 2
#define CR 3

  /* If first call, start file */

  if (!ValidFileHandle(pu->file.fu))
    start_file(pu);

  if (pu->flags & PU_NFMT) { /* 0306 */
    /* Print header if at top of page (only ever one page in NFMT) */

    if (pu->flags & PU_HDR_NEXT) {
      pu->flags &= ~PU_HDR_NEXT;
      print_header(pu);
    }

    emit(pu, str, bytes);
    return;
  }

  /* Output this line */

  while (bytes > 0) {
    if (!printing_footer && !(pu->flags & PU_SUPPRESS_HF)) {
      /* Print footer if at end of page */

      if ((pu->data_lines_per_page != 0) &&
          (pu->line == pu->data_lines_per_page)) {
        print_footer(pu);
      }

      /* Print header if at top of page */

      if (pu->flags & PU_HDR_NEXT) {
        pu->flags &= ~PU_HDR_NEXT;
        print_header(pu);
      }
    }

    n = bytes;
    mode = 0;
    if ((p = (char*)memchr(str, '\n', n)) != NULL) {
      n = p - str;
      mode = NL;
    }
    if (n && (p = (char*)memchr(str, '\f', n)) != NULL) {
      n = p - str;
      mode = FF;
    }
    if (n && (p = (char*)memchr(str, '\r', n)) != NULL) {
      n = p - str;
      mode = CR;
    }

    if (n > 0) {
      if (pu->col == 0) /* Emit left margin, if required */
      {
        while (pu->col < pu->left_margin) {
          emit(pu, " ", 1);
          pu->col++;
        }
      }

      emit(pu, str, n);
      pu->col += n;
      str += n;
      bytes -= n;
    }

    switch (mode) {
      case NL:
        emit(pu, Newline, NewlineBytes);
        pu->line++;
        str++;
        bytes--;
        pu->col = 0;
        break;

      case FF:
        print_footer(pu);
        pu->flags |= PU_HDR_NEXT;
        str++;
        bytes--;
        pu->col = 0;
        break;

      case CR:
        emit(pu, "\r", 1);
        str++;
        bytes--;
        pu->col = 0;
        break;
    }
  }
}

/* ======================================================================
   start_file()  -  Open print file                                       */

Private void start_file(PRINT_UNIT* pu) {
  char fn[MAX_PATHNAME_LEN + 1];
  DESCRIPTOR* descr;
  char seqno[4 + 1];

  if (!ValidFileHandle(pu->file.fu)) /* First reference - create hold file */
  {
    /* Non-Windows printing comes through here to generate a temporary
      file that will subsequently be passed to the spooler.           */

    if (pu->mode == PRINT_TO_PRINTER) {
      StartExclusive(SHORT_CODE, 43);
      pu->file.jobno = (sysseg->prtjob)++;
      EndExclusive(SHORT_CODE);
      /* convert to snprintf() -gwb 22Feb20 */
      if (snprintf(fn, MAX_PATHNAME_LEN + 1, "%s/prt/p%d", sysseg->sysdir,
                   pu->file.jobno) >= (MAX_PATHNAME_LEN + 1)) {
         /* TODO this should be logged to a file with more info */
         k_error("Overflowed path/filename buffer in start_file()!");
      }
      goto open_file;
    }

    if (pu->mode == PRINT_TO_AUX_PORT) {
      sprintf(fn, "$HOLD%c__Aux%d.%d", DS, (int)process.user_no,
              (int)(pu->unit));
      goto open_file;
    }

    /* If we get here, this is either PRINT_TO_FILE or PRINT_AND_HOLD */

    if (pu->file_name == NULL) /* Use default name */
    {
      sprintf(fn, "$HOLD%cP%d", DS, (int)(pu->unit));
    } else if (memcmp(pu->file_name, "$HOLD ", 6) == 0) {
      sprintf(fn, "$HOLD%c%s", DS, pu->file_name + 6);
    } else {
      strcpy(fn, pu->file_name);
    }

    if (pu->flags & PU_NEXT) /* Add suffix */
    {
      k_recurse(pcode_nextptr, 0); /* Execute recursive code */
      descr = e_stack - 1;
      k_get_c_string(descr, seqno, 4);
      strcat(fn, "_");
      strcat(fn, seqno);
    }

  open_file:
    pu->file.pathname = dupstring(fn);
    pu->file.fu = dio_open(fn, DIO_REPLACE);
    if (!ValidFileHandle(pu->file.fu)) {
      k_error(sysmsg(1716), pu->unit, fn, process.os_error);
    }
  }

  /* Allocate buffer */

  pu->buff = (char*)k_alloc(41, FILE_BUFF_SIZE);
  pu->bytes = 0;
  pu->flags |= PU_ACTIVE;
  pu->flags |= PU_HDR_NEXT;
  pu->col = 0;

  if (pu->flags & PU_PCL) {
    pu->flags |= PU_SUPPRESS_HF;
    pu->col = 1; /* Suppress left margin insertion */
    InitDescr(e_stack, INTEGER);
    (e_stack++)->data.value = pu->unit;
    k_recurse(pcode_pclstart, 1); /* Execute recursive code */
    pu->col = 0;
    pu->flags &= ~PU_SUPPRESS_HF;
  }

  /* Emit prefix string, if present */

  if (pu->prefix != NULL) {
    pu->flags |= PU_SUPPRESS_HF;
    pu->col = 1; /* Suppress left margin insertion */
    InitDescr(e_stack, INTEGER);
    (e_stack++)->data.value = pu->unit;
    k_put_c_string(pu->prefix, e_stack++);
    k_recurse(pcode_prefix, 2); /* Execute recursive code */
    pu->col = 0;
    pu->flags &= ~PU_SUPPRESS_HF;
  }

  pu->flags &= ~PU_OUTPUT; /* Startup and prefix don't count */
}

/* ======================================================================
   end_file()  -  End output to file                                      */

void end_file(PRINT_UNIT* pu) {
#define AUX_BUFF_SIZE 1024
  char buffer[AUX_BUFF_SIZE];
  int bytes;

  if (pu->buff != NULL) {
    print_footer(pu);

    if ((pu->flags & PU_PCL) &&
        (pu->flags & (PU_DUPLEX_LONG | PU_DUPLEX_LONG))) {
      emit(pu, "\x1b&l0S", 5);
    }

    /* Flush buffer */

    if (pu->bytes != 0) {
      if (Write(pu->file.fu, pu->buff, pu->bytes) != pu->bytes) {
        k_error(sysmsg(1717), pu->unit);
      }
      pu->bytes = 0;
    }

    if (pu->flags & PU_KEEP_OPEN)
      goto exit_end_file;

    k_free(pu->buff);
    pu->buff = NULL;
  }

  if (pu->mode == PRINT_TO_AUX_PORT) {
    tio_write(qmtgetstr("mc5"));
    lseek(pu->file.fu, 0, SEEK_SET);
    while ((bytes = read(pu->file.fu, buffer, AUX_BUFF_SIZE)) > 0) {
      tio_display_string(buffer, bytes, TRUE, FALSE);
    }
    tio_write(qmtgetstr("mc4"));
  }

  CloseFile(pu->file.fu);
  pu->file.fu = INVALID_FILE_HANDLE;
  pu->flags &= ~(PU_ACTIVE | PU_OUTPUT);

  if (pu->mode == PRINT_TO_PRINTER) /* 0527 Never happens on Windows */
  {
    spool_print_job(pu);
    remove(pu->file.pathname);
  } else if (pu->mode == PRINT_TO_AUX_PORT) {
    remove(pu->file.pathname);
  } else if (pu->mode == PRINT_AND_HOLD) {
    /* We have generated the file, now print it */

    spool_print_job(pu);
  }

  k_free_ptr(pu->file.pathname);

exit_end_file:
  return;
}

/* ======================================================================
   print_footer()  -  Print page footer                                   */

Private void print_footer(PRINT_UNIT* pu) {
  if (!(pu->flags & PU_HDR_NEXT) && (pu->footing != NULL)) {
    printing_footer = TRUE;
    if (!(pu->flags & PU_NFMT)) {
      while (pu->line < pu->data_lines_per_page) {
        emit(pu, Newline, NewlineBytes);
        pu->line++;
      }
    }
    emit_header_footer(pu, FALSE);
    printing_footer = FALSE;
  }

  pu->page_no++;
  pu->line = 0;
  pu->flags |= PU_HDR_NEXT; /* Header required on next output */
}

/* ======================================================================
   print_header()  -  Print page header                                   */

Private void print_header(PRINT_UNIT* pu) {
  int16_t i;

  if (pu->flags & PU_OUTPUT) /* 0382 */
  {
    emit(pu, "\r\f", 2);
  }

  if (pu->overlay != NULL) {
    pu->flags |= PU_SUPPRESS_HF;
    InitDescr(e_stack, INTEGER);
    (e_stack++)->data.value = pu->unit;
    k_recurse(pcode_overlay, 1); /* Execute recursive code */
    pu->flags &= ~PU_SUPPRESS_HF;
  }

  if (!(pu->flags & PU_NFMT)) {
    for (i = 0; i < pu->top_margin; i++) {
      to_file(pu, "\n", 1);
    }
  }
  pu->col = 0;

  if (pu->heading_lines)
    emit_header_footer(pu, TRUE);
  pu->line = 0;
}

/* ======================================================================
   emit()  - Emit data to file buffer                                     */

Private void emit(PRINT_UNIT* pu, char* str, int16_t bytes) {
  int16_t n;

  while (bytes) {
    if (pu->bytes == FILE_BUFF_SIZE) /* Buffer is full but more to write */
    {
      if (Write(pu->file.fu, pu->buff, FILE_BUFF_SIZE) != FILE_BUFF_SIZE) {
        k_error(sysmsg(1717), pu->unit);
      }
      pu->bytes = 0;
    }

    n = min(bytes, FILE_BUFF_SIZE - pu->bytes);
    memcpy(pu->buff + pu->bytes, str, n);
    pu->bytes += n;
    bytes -= n;
    str += n;
  }

  pu->flags |= PU_OUTPUT; /* 0382 */
}

/* END-CODE */
