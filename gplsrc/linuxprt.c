/* LINUXPRT.C
 * Printer i/o (Linux)
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
 * 03Sep25 gwb Remove K&R-isms.
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 *
 * START-HISTORY (OpenQM):
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 15 Nov 06  2.4-16 0527 Moved printing of mode 1 job to to_file.c
 * 13 Oct 06  2.4-15 Separated printer name and file name in print unit.
 * 25 Nov 05  2.2-17 pcfg.spooler is now a static string.
 * 24 Aug 05  2.2-8 Added handling of spooler name in print unit.
 * 19 Jul 05  2.2-4 Added support for the LANDSCAPE option of SETPTR.
 * 14 Jul 05  2.2-4 Added use of print unit options parameter.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * Linux printers work by diverting the output to the prt subdirectory of
 * the QMSYS account and then feeding this file to the Linux spooler.
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include "tio.h"
#include "config.h"

#define FILE_BUFF_SIZE 1024

void to_file(PRINT_UNIT* pu, char* str, int16_t bytes);

/* ======================================================================
   to_printer  -  Send text to printer                                    */

void to_printer(PRINT_UNIT *pu, char *str, int16_t bytes) { 
  to_file(pu, str, bytes); 
}

/* ======================================================================
   validate_printer()  -  Check printer name is valid                     */

bool validate_printer(char *printer_name) { 
  return TRUE; 
}

/* ======================================================================
   end_printer()  -  End access to printer                                */

void end_printer(PRINT_UNIT *pu) {
  if (!(pu->flags & PU_KEEP_OPEN)) {
    end_file(pu);
  }
}

/* ======================================================================
   spool_print_job()                                                      */

void spool_print_job(PRINT_UNIT* pu) {
  char cmd[300];
  char* p;

  if (pu->spooler != NULL) {
    p = cmd + sprintf(cmd, "%s ", pu->spooler);
  } else if (pcfg.spooler[0] != '\0') {
    p = cmd + sprintf(cmd, "%s ", pcfg.spooler);
  } else {
    p = cmd + sprintf(cmd, "lp ");
  }

  if (pu->copies > 1)
    p += sprintf(p, " -n %d", pu->copies); /* Copies */
  if (pu->printer_name != NULL)
    p += sprintf(p, " -d %s", pu->printer_name); /* Printer */
  if (pu->banner != NULL)
    p += sprintf(p, " -t \"%s\"", pu->banner); /* Banner */
  if (pu->options != NULL)
    p += sprintf(p, " -o \"%s\"", pu->options); /* Options */
  if (pu->flags & PU_LAND)
    p += sprintf(p, " -o \"landscape\"");

  p += sprintf(p, " '%s' > /dev/null", pu->file.pathname); /* File to print */

  system(cmd);
}

/* END-CODE */
