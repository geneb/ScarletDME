/* TELNET.H
 * Telnet parameter negotiation.
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
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

  #define TN_IAC       255    /* FF: Interpret as command: */
  #define TN_DONT      254    /* FE: You are not to use option */
  #define TN_DO        253    /* FD: Please, you use option */
  #define TN_WONT      252    /* FC: I won't use option */
  #define TN_WILL      251    /* FB: I will use option */
  #define TN_SB        250    /* FA: Interpret as subnegotiation */
  #define TN_GA        249    /* F9: Go ahead */
  #define TN_EL        248    /* F8: Erase the current line */
  #define TN_EC        247    /* F7: Erase the current character */
  #define TN_AYT       246    /* F6: Are you there */
  #define TN_AO        245    /* F5: Abort output--but let prog finish */
  #define TN_IP        244    /* F4: Interrupt process--permanently */
  #define TN_BRK       243    /* F3: Break */
  #define TN_DM        242    /* F2: Data mark--for connect. cleaning */
  #define TN_NOP       241    /* F1: Nop */
  #define TN_SE        240    /* F0: End sub negotiation */
  #define TN_EOR       239    /* EF: End of record (transparent mode) */
  #define TN_ABORT     238    /* EE: Abort process */
  #define TN_SUSP      237    /* ED: Suspend process */
  #define TN_XEOF      236    /* EC: End of file: EOF is already used... */


/* END-CODE */
