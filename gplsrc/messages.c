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
 * START-HISTORY:
 * 07 Aug 09        dat: Changed to using Directory file version of MESSAGES
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

Private char prefix[3+1] = "";   /* Language prefix */

char * month_names[12] = {"January","February","March","April","May","June","July","August","September","October","November","December"};
char * day_names[7] = {"Monday","Tuesday","Wednesday","Thursday","Friday","Saturday","Sunday"};

Private char * message = NULL;
Private int message_len;
Private int msg_file = -1;

/* ======================================================================
   Select a language                                                      */

bool load_language(char * language_prefix)
{
 static bool loaded = FALSE;
 static char * default_months = "January,February,March,April,May,June,July,August,September,October,November,December";
 static char * default_days = "Monday,Tuesday,Wednesday,Thursday,Friday,Saturday,Sunday";
 char * p;
 short int i;

 if (strlen(language_prefix) > 3) return FALSE;

 strcpy(prefix, language_prefix);

 if (loaded)  /* Free old memory */
  {
   k_free(month_names[0]);
   k_free(day_names[0]);
  }

 /* Month names */

 p = sysmsg(1500);
 if ((*p == '[') || (strdcount(p, ',') != 12)) p = default_months;  /* 0289 */
 month_names[0] = (char *)k_alloc(83, strlen(p) + 1);
 strcpy(month_names[0], p);
 (void)strtok(month_names[0], ",");
 for (i = 1; i < 12; i++) month_names[i] = strtok(NULL, ",");

 /* Day names */

 p = sysmsg(1501);
 if ((*p == '[') || (strdcount(p, ',') != 7)) p = default_days;    /* 0289 */
 day_names[0] = (char *)k_alloc(84, strlen(p) + 1);
 strcpy(day_names[0], p);
 (void)strtok(day_names[0], ",");
 for (i = 1; i < 7; i++) day_names[i] = strtok(NULL, ",");

 loaded = TRUE;

 return TRUE;
}

/* ======================================================================
   sysmsg()  -  Return message text                                       */

char * sysmsg(int msg_no)
{
 STRING_CHUNK * str = NULL; /* redundant */
 char id[16];           /* Holds the msg id */
 char path[MAX_PATHNAME_LEN+1]; /* Pathstring of the MESSAGES file or Record */
 int n;                 /* A random tmp var by Ladybridge */
 int msg_rec = -1;           /* The message record FileHandle */
 char * p;
 STRING_CHUNK * q;
 struct stat msg_stat; /* Holds Dir records files stats */
 int status;

 if (msg_file == -1)
  {
   message_len = 128;
   if (message == NULL) message = (char *)k_alloc(82, message_len);

   sprintf(path, "%s%cMESSAGES", sysseg->sysdir, DS);
   msg_file = open(path, O_RDONLY);
   if (msg_file < 0)
    {
     sprintf(message, "[%d] Message file not found(%d %ld)", 
             msg_no, dh_err, process.os_error);
     return message;
    }
   /* close(msg_file); Don't need to keep it open, just checking its there */
  }

 /* Open language specific msg */ 
 if (prefix[0] != '\0')
  {
   n = sprintf(id, "%s%d", prefix, msg_no);
   sprintf(path, "%s%cMESSAGES%c%s", sysseg->sysdir, DS, DS, id);
   msg_rec = open(path, O_RDONLY);
 }

 /* Try English messages */
 if( msg_rec < 0 )  
  {
   n = sprintf(id, "%d", msg_no);
   sprintf(path, "%s%cMESSAGES%c%s", sysseg->sysdir, DS, DS, id);
   msg_rec = open(path, O_RDONLY);
  }

 if( msg_rec >= 0)
  {
   /* Get size of record to come */   
   status = fstat(msg_rec, &msg_stat);
   int msg_size = msg_stat.st_size;

   /* Check buffer size */
   if (msg_size > message_len)         /* Must increase buffer size */
    {
     k_free(message);                 /* Release old buffer */

     n = (msg_size & ~127) + ((msg_size & 127)?128:0);  /* Round to multiple of 128 bytes */
     message = (char *)k_alloc(82, n);
     message_len = n;
    }

    /* Read message rec */ 
    status = read(msg_rec, message, msg_size);
    message[msg_size] = '\0';
   /*printf("FILE (%s) %d\n", path, msg_size);*/
  }
 
 if (msg_rec < 0 || status < 0) /* Either open or read failed */
  {
   sprintf(message, "[%s] Message not found", id);
  }
  

 /* Replace any embedded newline and tab codes */
 p = message;
 while((p = strchr(p, '\\')) != NULL)
  {
   switch(*(p+1))
    {
     case 'n':
        *p = '\n';
        strcpy(p+1, p+2);
        break;
     case 't':
        *p = '\t';
        strcpy(p+1, p+2);
        break;
    }
   p++;
  }

 return message;
}

/* ======================================================================
   op_sysmsg()  -  Return message text to QMBasic program                 */

void op_sysmsg()
{
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

 DESCRIPTOR * descr;
 short int arg_ct;
 int saved_process_status;
 int saved_os_error;
 char * msg;

 saved_process_status = process.status;
 saved_os_error = process.os_error;

 arg_ct = *(pc++);

 /* Replace stack entry for key with skeleton message text */

 descr = e_stack - (1 + arg_ct);
 GetNum(descr);
 msg = sysmsg(descr->data.value);
 k_put_c_string(msg, descr);

 if ((strchr(msg, '%') != NULL) || arg_ct)  /* Need to substitute arguments */
  {
   /* Add null strings to take us to four arguments */

   while(arg_ct++ < 4)
    {
     InitDescr(e_stack, STRING);
     (e_stack++)->data.str.saddr = NULL;
    }

   InitDescr(e_stack, INTEGER);
   (e_stack++)->data.value = saved_process_status;

   InitDescr(e_stack, INTEGER);
   (e_stack++)->data.value = saved_os_error;

   k_recurse(pcode_msgargs,7);   /* Execute recursive to do substitution */
  }
}

/* END-CODE */
