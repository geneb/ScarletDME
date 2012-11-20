/* LNX.C
 * Linux specific functions
 * Copyright (c) 2003 Ladybridge Systems, All Rights Reserved
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

#include <qm.h>

/* ======================================================================
   qmsendmail()  -  Send email                                            */

bool qmsendmail(sender, recipients, cc_recipients, bcc_recipients,
                subject, text, attachments)
   char * sender;         /* Sender's address:  fred@acme.com */
   char * recipients;     /* Comma separated list of recipient addresses */
   char * cc_recipients;  /* Comma separated list of recipient addresses */
   char * bcc_recipients; /* Comma separated list of recipient addresses */
   char * subject;        /* Subject line */
   char * text;           /* Text of email */
   char * attachments;    /* Comma separated list of attachment files */
{
 bool status = FALSE;
 char tempname[12+1];  /* .qm_mailnnnn */
 char command[1024+1];
 int tfu;
 int n;


 /* Write mail text to a temporary file */

 sprintf(tempname, ".qm_mail%d", my_uptr->uid);
 tfu = open(tempname, O_RDWR | O_CREAT | O_TRUNC, default_access);
 if (tfu < 0)
  {
   process.status = ER_NO_TEMP;
   process.os_error = errno;
   goto exit_sendmail;
  }

 n = strlen(text);
 if (write(tfu, text, n) != n)
  {
   process.status = ER_NO_TEMP;
   goto exit_sendmail;
  }

 close(tfu);

 /* Construct mail command */

 n = sprintf(command, "mail -s \"%s\"", subject);

 if (cc_recipients != NULL) n += sprintf(command + n, " -c %s", cc_recipients);
 if (bcc_recipients != NULL) n += sprintf(command + n, " -b %s", bcc_recipients);
 if (recipients != NULL) n += sprintf(command + n, " %s", recipients);

 sprintf(command + n, " <%s", tempname);

 system(command);

 /* Delete temporary file */

 remove(tempname);


 status = TRUE;

exit_sendmail:
 return status;
}

/* END-CODE */
