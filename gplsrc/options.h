/* OPTIONS.H
 * Option flags.
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
 * 13 Sep 07  2.6-3 Added OptSpaceMCT.
 * 30 Aug 07  2.6-0 Added OptProcA.
 * 17 Jan 07  2.4-19 Added OptNoDateWrapping.
 * 03 Nov 06  2.4-15 Added QUERY.PRIORITY.AND option.
 * 07 Apr 06  2.4-1 Added OptQueryNoCase.
 * 15 Mar 06  2.3-8 Added OptInherit.
 * 09 Feb 06  2.3-6 Added OptPickExplode.
 * 10 Aug 05  2.2-7 Added OptNoSelListQuery.
 * 09 Aug 05  2.2-7 Added OptEDNoQueryFD.
 * 01 Jul 05  2.2-3 Added OptDebugRebindKeys.
 * 14 Jan 05  2.1-1 Added OptSuppressAbortMsg.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

/* Option flags
   All references are by macro so that the implementation can change to
   accommodate larger numbers of option flags.
   NOTE: This list is known by the BP OPTION program and BP INT$KEYS.H.   */

#define OptUnassignedWarning  0
#define OptQueryDebug         1
#define OptPickWildcard       2
#define OptQualDisplay        3
#define OptPickBreakpoint     4
#define OptWithImpliesOr      5
#define OptDumpOnError        6
#define OptDivZeroWarning     7
#define OptNonNumericWarning  8
#define OptAssocUnassocMV     9
#define OptPickBreakpointU   10
#define OptNoUserAborts      11
#define OptRunNoPage         12
#define OptShowStackOnError  13
#define OptCRDBUpcase        14
#define OptAMPMUpcase        15
#define OptPickNull          16
#define OptPickGrandTotal    17
#define OptSuppressAbortMsg  18
#define OptDebugRebindKeys   19
#define OptEDNoQueryFD       20
#define OptNoSelListQuery    21
#define OptChainKeepCommon   22
#define OptSelectKeepCase    23
#define OptCreateFileCase    24
#define OptPickExplode       25
#define OptInherit           26
#define OptQueryNoCase       27
#define OptLockBeep          28
#define OptPickImpliedEQ     29
#define OptQueryPriorityAnd  30
#define OptNoDateWrapping    31
#define OptProcA             32
#define OptSpaceMCT          33
#define NumOptions           34

Public char option_flags[NumOptions];

#define Option(opt) (option_flags[(opt)])
#define SetOption(opt, val) option_flags[(opt)] = (val);

/* END-CODE */
