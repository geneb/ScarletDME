/* LNXPORT.C
 * Port i/o opcodes for Linux/FreeBSD
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
 * ScarletDME Wiki: https://scarlet.deltasoft.com
 * 
 * START-HISTORY (ScarletDME):
 * 23Jan22 gwb macOS requires sys/ioctl.h to be included.
 *             Fixed #include references to use "" instead of <>
 * 
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 *
 * START-HISTORY (OpenQM):
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 28 Apr 05  2.1-13 In is_port(), check for /dev/ prefix to improve performance
 *                   for other pathnames.
 * 21 Apr 05  2.1-12 Trap errno of openport() failure.
 * 04 Jan 05  2.1-0 Completed implementation.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * closeport()       Close a port
 * is_port()         Does name reference a port?
 * openport()        Open a port
 * readport()        Read data from a port
 * writeport()       Write data to a port
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include "qmnet.h"

#ifdef __APPLE__
  #include <sys/ioctl.h>
#endif

/* ======================================================================
   closeport()  -  Close a port                                           */

void closeport(int hPort) {
  close(hPort);
}

/* ======================================================================
   is_port()  -  Check if name references a port                          */

bool is_port(char* name) {
  int fu;
  bool result = FALSE;

  if (!memcmp(name, "/dev/", 5)) {
    fu = open(name, O_RDONLY | O_NONBLOCK);
    if (fu >= 0) {
      result = isatty(fu);
      close(fu);
    }
  }

  return result;
}

/* ======================================================================
   openport()  -  Open a port                                             */

int openport(char* name) {
  int hPort;

  /* Open the port */

  hPort = open(name, O_RDWR | O_NONBLOCK, S_IREAD | S_IWRITE);
  if (hPort < 0)
    process.os_error = errno;
  return hPort;
}

/* ======================================================================
   readport()  -  Read data from port                                     */

int readport(int hPort, char* str, int16_t bytes) {
  return read(hPort, str, bytes);
}

/* ======================================================================
   writeport()  -  Write data to port                                     */

bool writeport(int hPort, char* str, int16_t bytes) {
  return (write(hPort, str, bytes) == bytes);
}

/* ======================================================================
   op_getport()  -  GET.PORT.PARAMS()                                     */

void op_getport() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  ADDR to file variable      | Dynamic array               |
     |=============================|=============================|
                    
 */

  DESCRIPTOR* descr;
  FILE_VAR* fvar;
  SQ_FILE* sq_file;
  STRING_CHUNK* str = NULL;
  int n1, n2, n3, n4, n5;

  struct termios tty_settings;

  descr = e_stack - 1;
  k_get_file(descr);
  fvar = descr->data.fvar;

  ts_init(&str, 128);

  if (fvar->type != SEQ_FILE) {
    process.status = ER_NSEQ;
    goto exit_op_getport;
  }

  sq_file = fvar->access.seq.sq_file;
  if (!(sq_file->flags & SQ_PORT)) {
    process.status = ER_NPORT;
    goto exit_op_getport;
  }

  tcgetattr(sq_file->fu, &tty_settings);

  /* Baud rate */

  switch (cfgetospeed(&tty_settings)) {
    case B110:
      n1 = 110;
      break;
    case B300:
      n1 = 300;
      break;
    case B600:
      n1 = 600;
      break;
    case B1200:
      n1 = 1200;
      break;
    case B2400:
      n1 = 2400;
      break;
    case B4800:
      n1 = 4800;
      break;
    case B9600:
      n1 = 9600;
      break;
    case B19200:
      n1 = 19200;
      break;
    case B38400:
      n1 = 38400;
      break;
#ifdef TTY_50_75_134_150_200_1800
    case B50:
      n1 = 50;
      break;
    case B75:
      n1 = 75;
      break;
    case B134:
      n1 = 134;
      break;
    case B150:
      n1 = 150;
      break;
    case B200:
      n1 = 200;
      break;
    case B1800:
      n1 = 1800;
      break;
#endif
#ifdef TTY_57600_115200_230400
    case B57600:
      n1 = 57600;
      break;
    case B115200:
      n1 = 115200;
      break;
    case B230400:
      n1 = 230400;
      break;
#endif
  }

  /* Parity */

  if ((tty_settings.c_cflag & PARENB) == 0)
    n2 = 0;
  else
    n2 = (tty_settings.c_cflag & PARODD) ? 1 : 2;

  /* Byte size */

  switch (tty_settings.c_cflag & CSIZE) {
    case CS5:
      n3 = 5;
      break;
    case CS6:
      n3 = 6;
      break;
    case CS7:
      n3 = 7;
      break;
    case CS8:
      n3 = 8;
      break;
  }

  /* Stop bits */

  n4 = (tty_settings.c_cflag & CSTOPB) ? 2 : 1;

  /* Status bits */

  ioctl(sq_file->fu, TIOCMGET, &n5);

  ts_printf("%s\xfe%d\xfe%d\xfe%d\xfe%d", sq_file->pathname, /* Port name */
            n1,                                              /* Baud rate */
            n2,                                              /* Parity type */
            n3,                                              /* Bits per byte */
            n4,                                              /* Stop bits */
            (n5 & TIOCM_CTS) != 0,                           /* CTS */
            (n5 & TIOCM_DSR) != 0,                           /* DSR */
            (n5 & TIOCM_RNG) != 0,                           /* RING */
            (n5 & TIOCM_CAR) != 0);                          /* DCD */

  process.status = 0;

exit_op_getport:
  ts_terminate();

  k_release(descr);
  InitDescr(descr, STRING);
  descr->data.str.saddr = str;
}

/* ======================================================================
   op_setport()  -  SET.PORT.PARAMS()                                     */

void op_setport() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Dynamic array              | 1 = OK, 0  = Error          |
     |-----------------------------|-----------------------------|
     |  ADDR to file variable      |                             |
     |=============================|=============================|

     If an error is reported, STATUS() contains the o/s error number.
 */

  DESCRIPTOR* descr;
  FILE_VAR* fvar;
  SQ_FILE* sq_file;
  char params[256 + 1];
  struct termios tty_settings;
  int n1, n2, n3, n4;
  char* p;

  descr = e_stack - 1; /* New parameters */
  if (k_get_c_string(descr, params, 256) < 0) {
    process.status = ER_LENGTH;
    goto exit_op_setport;
  }

  descr = e_stack - 2; /* File variable */
  k_get_file(descr);
  fvar = descr->data.fvar;

  if (fvar->type != SEQ_FILE) {
    process.status = ER_NSEQ;
    goto exit_op_setport;
  }

  sq_file = fvar->access.seq.sq_file;
  if (!(sq_file->flags & SQ_PORT)) {
    process.status = ER_NPORT;
    goto exit_op_setport;
  }

  p = strchr(params, '\xfe');
  if ((p == NULL) ||
      (sscanf(p + 1, "%d\xfe%d\xfe%d\xfe%d", &n1, &n2, &n3, &n4) != 4)) {
    process.status = ER_PARAMS;
    goto exit_op_setport;
  }

  tcgetattr(sq_file->fu, &tty_settings);

  /* Baud rate */

#define SetSpeed(x)                 \
  cfsetispeed(&tty_settings, B##x); \
  cfsetispeed(&tty_settings, B##x);
  switch (n1) {
    case 110:
      SetSpeed(110);
      break;
    case 300:
      SetSpeed(300);
      break;
    case 600:
      SetSpeed(600);
      break;
    case 1200:
      SetSpeed(1200);
      break;
    case 2400:
      SetSpeed(2400);
      break;
    case 4800:
      SetSpeed(4800);
      break;
    case 9600:
      SetSpeed(9600);
      break;
    case 19200:
      SetSpeed(19200);
      break;
    case 38400:
      SetSpeed(38400);
      break;
#ifdef TTY_50_75_134_150_200_1800
    case 50:
      SetSpeed(50);
      break;
    case 75:
      SetSpeed(75);
      break;
    case 134:
      SetSpeed(134);
      break;
    case 150:
      SetSpeed(150);
      break;
    case 200:
      SetSpeed(200);
      break;
    case 1800:
      SetSpeed(1800);
      break;
#endif
#ifdef TTY_57600_115200_230400
    case 57600:
      SetSpeed(57600);
      break;
    case 115200:
      SetSpeed(115200);
      break;
    case 230400:
      SetSpeed(230400);
      break;
#endif
  }

  /* Parity */

  if (n2) {
    tty_settings.c_cflag |= PARENB; /* Enabled */
    if (n2 == 1)
      tty_settings.c_cflag |= PARODD;
    else
      tty_settings.c_cflag &= ~PARODD;
  } else
    tty_settings.c_cflag &= ~PARENB; /* Disabled */

  /* Byte size */

  tty_settings.c_cflag &= ~CSIZE;
  switch (n3) {
    case 5:
      tty_settings.c_cflag |= CS5;
      break;
    case 6:
      tty_settings.c_cflag |= CS6;
      break;
    case 7:
      tty_settings.c_cflag |= CS7;
      break;
    case 8:
      tty_settings.c_cflag |= CS8;
      break;
  }

  /* Stop bits */

  if (n4 == 2)
    tty_settings.c_cflag |= CSTOPB;
  else
    tty_settings.c_cflag &= ~CSTOPB;

  process.status = (tcsetattr(sq_file->fu, TCSANOW, &tty_settings) != 0);

exit_op_setport:
  k_dismiss();
  k_dismiss();

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = (process.status == 0);
}

/* END-CODE */
