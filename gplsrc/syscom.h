/* SYSCOM.H
 * C definitions from QMBasic SYSCOM.H
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

/* SYSCOM layout from BP SYSCOM.H */

#define SYSCOM_ITYPE_MODE            1
#define SYSCOM_COMMAND               2
#define SYSCOM_SENTENCE              3
#define SYSCOM_SYSTEM_RETURN_CODE    8
#define SYSCOM_DATA_QUEUE            9
#define SYSCOM_SELECT_LIST          12
#define SYSCOM_SELECT_COUNT         13
#define SYSCOM_ECHO_INPUT           15
#define SYSCOM_CPROC_DATE           17
#define SYSCOM_CPROC_TIME           18
#define SYSCOM_QPROC_ID             20
#define SYSCOM_QPROC_RECORD         21
#define SYSCOM_BREAK_LEVEL          23
#define SYSCOM_BELL                 24
#define SYSCOM_USER_RETURN_CODE     25
#define SYSCOM_ACCOUNT_PATH         26
#define SYSCOM_WHO                  27
#define SYSCOM_IPC                  29
#define SYSCOM_BREAK_VALUE          30
#define SYSCOM_LAST_COMMAND         31
#define SYSCOM_ABORT_CODE           32
#define SYSCOM_SELECTED             39
#define SYSCOM_TTY                  41
#define SYSCOM_FPROC_ND             46
#define SYSCOM_USER0                49
#define SYSCOM_USER1                50
#define SYSCOM_USER2                51
#define SYSCOM_USER3                51
#define SYSCOM_USER4                53
#define SYSCOM_QPROC_TOTALS         54
#define SYSCOM_TRIGGER_RETURN_CODE  55
#define SYSCOM_ABORT_MESSAGE        56
#define SYSCOM_PROC_IBUF            67
#define SYSCOM_AT_ANS               75
#define SYSCOM_SYS0                 76
#define SYSCOM_TRANS_FILES          78
#define SYSCOM_TRANS_FVARS          79

/* END-CODE */
