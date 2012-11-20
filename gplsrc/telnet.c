/* TELNET.C
 * Telnet parameter negotiation.
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
 * START-HISTORY:
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 19 Jun 07  2.5-7 Handle terminal type negotiation. 
 * 21 Apr 05  2.1-12 Always honour telnet break parameter regardless of setting
 *                   of trap_break_char.
 * 14 Apr 05  2.1-12 Added socket output buffering.
 * 12 Apr 05  2.1-12 Handle input and output binary mode separately.
 * 06 Mar 05  2.1-8 Honour telnet binary mode negotiation.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * **********************************************************************
 * NOTE: THIS CODE IS REPLICATED BY A SIMILAR FUNCTION IN QMSVC.C
 * CHANGES MAY NEED TO BE APPLIED IN BOTH MODULES
 * **********************************************************************
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include <qm.h>
#include <tio.h>
#include <telnet.h>

char socket_byte(void);

/* ======================================================================
   Negotiate telnet parameter

   Returns true if parameter processed, false if was real IAC data char  */

bool negotiate_telnet_parameter()
{
 unsigned char c;
 unsigned char last;
#define MAX_SUBNEG 100
 char subneg_buff[MAX_SUBNEG];   /* Maximum subnegotiation length */
 int subneg_bytes;

 c = (unsigned char)socket_byte();
 if (c == TN_IAC) return FALSE;     /* Treat as real IAC character */

 switch((unsigned char)c)   /* Process negotiation text */
  {
   case TN_IP:     /* Interrupt process */
   case TN_BRK:    /* Break key */
      break_key();
      break;

   case TN_WILL:   /* WILL: Acknowledgement of DO */
      c = (unsigned char)socket_byte();
      switch(c)
       {
        case 0x00:   /* WILL Binary mode */
           telnet_binary_mode_in = TRUE;
           to_outbuf("\xFF\xFD\x00", 3); /* DO Binary mode */
           break;

        case 0x18:  /* WILL TERMTYPE */
           to_outbuf("\xFF\xFA\x18\x01\xFF\xF0", 6); /* SB TERMTYPE SEND SE */
           break;
       }
      break;

   case TN_DO:     /* DO: Sender wants to enable a feature */
      c = (unsigned char)socket_byte();
      switch(c)
       {
        case 0x00:   /* DO Binary mode */
           telnet_binary_mode_out = TRUE;
           to_outbuf("\xFF\xFB\x00", 3); /* WILL Binary mode */
           break;
       }
      break;

   case TN_WONT:   /* WONT: Rejection of DO, acknowledgement of DONT */
      c = (unsigned char)socket_byte();
      switch(c)
       {
        case 0x00:   /* DONT Binary mode */
           telnet_binary_mode_in = FALSE;
           to_outbuf("\xFF\xFE\x00", 3); /* DONT Binary mode */
           break;
       }
      break;

   case TN_DONT:   /* DONT: Sender wants to disable a feature */
      c = (unsigned char)socket_byte();
      switch(c)
       {
        case 0x00:   /* DONT Binary mode */
           telnet_binary_mode_out = FALSE;
           to_outbuf("\xFF\xFC\x00", 3); /* WONT Binary mode */
           break;
       }
      break;

   case TN_SB:     /* SB: Subnegotiation */
      /* Gather data to SE */

      subneg_bytes = 0;
      last = 0;
      do {
          c = (unsigned char)socket_byte();

          if ((last == TN_IAC) && (c == TN_SE))
           {
            subneg_bytes--;  /* Discard SE pair */
            break;
           }

          if (subneg_bytes < MAX_SUBNEG)
           {
            subneg_buff[subneg_bytes++] = c;
           }

          last = c;
         } while(1);

      /* What was it? */

      switch(subneg_buff[0])
       {
        case 0x18:    /* TERMTYPE */
           if (subneg_buff[1] == 0x00)  /* IS */
            {
             if (tio.term_type[0] == '\0')
              {
               sprintf(tio.term_type, "%.*s", subneg_bytes - 2, subneg_buff+2);
              }
            }
           break;
       }
      break;
  }

 flush_outbuf();

 return TRUE;
}


/* END-CODE */
