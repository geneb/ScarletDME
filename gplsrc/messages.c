/* MESSAGES.C
 * Message handler.
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
 * 01Feb23 njs sysmsg replace new line characters with @FM in message
 *             remove trailing new line if present
 *             remove problematic use of strcpy in sysmsg
 * 31Mar22 rdm Updated sysmsg to close the msg_rec with returning to 
 *             prevent max open files error
 * 11Jan22 gwb Removed goto calls.
 * 
 * 09Jan22 gwb Fixed a format specifier warning.
 *
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 * 
 * 22Feb20 gwb Converted sprintf() to snprintf() and removed some unused
 *             variables in sysmsg()
 * 07Aug09 dat Changed to using Directory file version of MESSAGES
 *
 * START-HISTORY (OpenQM):
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 30 May 06  2.4-5 0492 Transfer of the message from the chunked string to the
 *                  message buffer did not walk the chunks correctly.
 * 27 Dec 05  2.3-3 0442 Memory leak caused by destruction of string pointer
 *                  before using it to release the string memory in sysmsg().
 * 08 Sep 05  2.2-10 Validate day/month names as having right number of items.
 * 13 Jul 05  2.2-4 0371 Changed conditioning of recursive call in op_sysmsg()
 *                  so that erroneously supplied arguments don't leave e_stack
 *                  items unprocessed.
 * 04 Dec 04  2.0-12 0289 Set default day and month names if cannot find.
 * 11 Nov 04  2.0-10 Include error codes in "cannot open message file".
 * 19 Oct 04  2.0-5 Tidied up load_language()..
 * 20 Sep 04  2.0-2 New module.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * The message library (QMSYS MESSAGES file) uses numbers to identify
 * messages. For non-English texts, the message number is prefixed by a
 * language code of up to three letters.
 *
 * Message numbers are groups according to their role. Open source
 * developers should use numbers in the range 10000 to 19999.
 *
 * Messages that are called from QMBasic using the sysmsg() function
 * can include up to four arguments referenced as %1 to %4. These tokens
 * may appear in any order.
 *
 * Messages that are called for C, use conventional printf style tokens
 * and are therefore both type and order sensitive.
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"

Private char prefix[3 + 1] = ""; /* Language prefix */

char* month_names[12] = {"January",   "February", "March",    "April",
                         "May",       "June",     "July",     "August",
                         "September", "October",  "November", "December"};
char* day_names[7] = {"Monday", "Tuesday",  "Wednesday", "Thursday",
                      "Friday", "Saturday", "Sunday"};

Private char* message = NULL;
Private int message_len;
Private int msg_file = -1;

/* ======================================================================
   Select a language                                                      */

bool load_language(char* language_prefix) {
  static bool loaded = FALSE;
  static char* default_months =
      "January,February,March,April,May,June,July,August,September,October,"
      "November,December";
  static char* default_days =
      "Monday,Tuesday,Wednesday,Thursday,Friday,Saturday,Sunday";
  char* p;
  int16_t i;

  if (strlen(language_prefix) > 3)
    return FALSE;

  strcpy(prefix, language_prefix);

  if (loaded) { /* Free old memory */
    k_free(month_names[0]);
    k_free(day_names[0]);
  }

  /* Month names */

  p = sysmsg(1500); /* TODO: Magic numbers are bad, mmkay? */
  if ((*p == '[') || (strdcount(p, ',') != 12))
    p = default_months; /* 0289 */

  month_names[0] = (char*)k_alloc(83, strlen(p) + 1);
  strcpy(month_names[0], p);
  (void)strtok(month_names[0], ",");
  for (i = 1; i < 12; i++)
    month_names[i] = strtok(NULL, ",");

  /* Day names */

  p = sysmsg(1501); /* TODO: Magic numbers are bad, mmkay? */
  if ((*p == '[') || (strdcount(p, ',') != 7))
    p = default_days; /* 0289 */
  day_names[0] = (char*)k_alloc(84, strlen(p) + 1);
  strcpy(day_names[0], p);
  (void)strtok(day_names[0], ",");
  for (i = 1; i < 7; i++)
    day_names[i] = strtok(NULL, ",");

  loaded = TRUE;

  return TRUE;
}

/* ======================================================================
   sysmsg()  -  Return message text                                       */

char* sysmsg(int msg_no) {
  /* STRING_CHUNK* str = NULL; /x redundant x/ unused variable */
  char id[16];              /* Holds the msg id */
  char path[MAX_PATHNAME_LEN + 1]; /* Pathstring of the MESSAGES file or Record */
  int n;            /* A random tmp var by Ladybridge */
  int msg_rec = -1; /* The message record FileHandle */
  char* p;
  char* q;
  /* STRING_CHUNK* q; unused variable */
  struct stat msg_stat; /* Holds Dir records files stats */
  int status;

  if (msg_file == -1) {
    message_len = 128;
    if (message == NULL)
      message = (char*)k_alloc(82, message_len);
    /* converted to snprintf() -gwb 22Feb20 */
    if (snprintf(path, MAX_PATHNAME_LEN + 1, "%s%cMESSAGES", sysseg->sysdir, 
               DS) >= (MAX_PATHNAME_LEN + 1)) {
      /* TODO: this should be sent to the system log. */
      k_error("Overflowed directory/filename path length in sysmsg()!");
      message = "";
      //goto exit_sysmsg;  /* I died inside adding this. -gwb */
      return message;
    }
    msg_file = open(path, O_RDONLY);
    if (msg_file < 0) {
      sprintf(message, "[%d] Message file not found(%d %d).", msg_no, dh_err,
              process.os_error);
      return message;
    }
    /* close(msg_file); Don't need to keep it open, just checking its there */
  }

  /* Open language specific msg */
  if (prefix[0] != '\0') {
    n = sprintf(id, "%s%d", prefix, msg_no);
    /* converted to snprintf() -gwb 22Feb20 */
    if (snprintf(path, MAX_PATHNAME_LEN + 1, "%s%cMESSAGES%c%s", sysseg->sysdir, 
            DS, DS, id) >= (MAX_PATHNAME_LEN + 1)) {
      /* TODO: this should be sent to the system log. */
      k_error("Overflowed directory/filename path length in sysmsg()!");
      message = "";
      // goto exit_sysmsg;  /* I died inside adding this. -gwb */
      return message; /* ...and un-died! */
    }
    msg_rec = open(path, O_RDONLY);
  }

  /* Try English messages */
  if (msg_rec < 0) {
    n = sprintf(id, "%d", msg_no);
    /* converted to snprintf() -gwb 22Feb20 */
    if (snprintf(path, MAX_PATHNAME_LEN + 1, "%s%cMESSAGES%c%s", sysseg->sysdir, 
            DS, DS, id) >= (MAX_PATHNAME_LEN + 1)) {
      /* TODO: this should be sent to the system log. */
      k_error("Overflowed directory/filename path length in sysmsg()!");
      message = "";
      // goto exit_sysmsg;  /* I died inside adding this. -gwb */
      return message;  /* and un-died. */
    }
    msg_rec = open(path, O_RDONLY);
  }

  if (msg_rec >= 0) {
    /* Get size of record to come */
    status = fstat(msg_rec, &msg_stat);
    int msg_size = msg_stat.st_size;

    /* Check buffer size */
    if (msg_size > message_len) { /* Must increase buffer size */
      k_free(message); /* Release old buffer */

      n = (msg_size & ~127) +
          ((msg_size & 127) ? 128 : 0); /* Round to multiple of 128 bytes */
      message = (char*)k_alloc(82, n);
      message_len = n;
    }

    /* Read message rec */
    status = read(msg_rec, message, msg_size);
    message[msg_size] = '\0';
    /*printf("FILE (%s) %d\n", path, msg_size);*/
  }

  if (msg_rec < 0 || status < 0) { /* Either open or read failed */
    sprintf(message, "[%s] Message not found", id);
  }
  /* njs - 01Feb23 mimic how basic program would read DIRECTORY_FILE via VM */
  /* consult op_dio3.2 read_record() */
  /* first remove trailing new line  */
  message_len = strlen(message);
   if (message[message_len-1] == '\n')
    message[message_len-1] = '\0';
  /* njs - 01Feb23 Walk through and replace newlines by field marks. */
  p = message;
  n = strlen(message);
  do{
    q = memchr(p, '\n', n);
    if (q == NULL)
       break;
    *q = FIELD_MARK;
    n -= (q + 1 - p);  /* bytes remaining */
    p = q + 1;         /* next byte to start memchr at */
    } while (n);

  /* Replace any embedded newline and tab codes */
  /* njs 01Feb23 note old code violates:       */
  /*  char *strcpy(char *restrict s1, const char *restrict s2) */
  /* restrict in this case indicates that the caller promises  */
  /* that the two buffers in question do not overlap.          */
  /* And it does fail on 64bit linux! (atleast ubuntu 22.04    */
  p = message;
  while ((p = strchr(p, '\\')) != NULL) {
    switch (*(p + 1)) {
      case 'n':
        *p = '\n';
      /*  strcpy(p + 1, p + 2); */
        memmove(p + 1, p + 2, strlen(p+2));
      /* get ride of dublicate last character */
        p[strlen(p)-1] = '\0';
        break;
      case 't':
        *p = '\t';
        /*  strcpy(p + 1, p + 2); */
        memmove(p + 1, p + 2, strlen(p+2));
        /* get ride of dublicate last character */
        p[strlen(p)-1] = '\0';
        break;
    }
    p++;
  }

   /* rdm - 03/31/22
   *If you don't close the message recs at this point they will stay open forever
   *and give you greif causing the system into run into max open files*/
  close(msg_rec);

  return message;
}

/* ======================================================================
   op_sysmsg()  -  Return message text to QMBasic program                 */

void op_sysmsg() {
  /* Stack:

      |================================|=============================|
      |            BEFORE              |           AFTER             |
      |================================|=============================|
  top |  Arguments (perhaps)           | Message text                |
      |--------------------------------|-----------------------------|
      |  Key                           |                             |
      |================================|=============================|

      Opcode is followed by single byte argument count
 */

  DESCRIPTOR* descr;
  int16_t arg_ct;
  int saved_process_status;
  int saved_os_error;
  char* msg;

  saved_process_status = process.status;
  saved_os_error = process.os_error;

  arg_ct = *(pc++);

  /* Replace stack entry for key with skeleton message text */

  descr = e_stack - (1 + arg_ct);
  GetNum(descr);
  msg = sysmsg(descr->data.value);
  k_put_c_string(msg, descr);

  if ((strchr(msg, '%') != NULL) || arg_ct) /* Need to substitute arguments */
  {
    /* Add null strings to take us to four arguments */

    while (arg_ct++ < 4) {
      InitDescr(e_stack, STRING);
      (e_stack++)->data.str.saddr = NULL;
    }

    InitDescr(e_stack, INTEGER);
    (e_stack++)->data.value = saved_process_status;

    InitDescr(e_stack, INTEGER);
    (e_stack++)->data.value = saved_os_error;

    k_recurse(pcode_msgargs, 7); /* Execute recursive to do substitution */
  }
}

/* END-CODE */
