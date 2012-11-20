/* DEBUG.H
 * Debugger include file
 * Copyright (c) 1994, Ladybridge Systems, All Rights Reserved
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
 * *************************************************************************
 * ******                           WARNING                           ******
 * ******                                                             ******
 * ******   Changes to this file may need to be reflected by changes  ******
 * ******   the BP DEBUG.H include file.                              ******
 * ******                                                             ******
 * *************************************************************************
 * END-DESCRIPTION
 *
 * START-CODE
 */

#define BRK_RUN             0     /* Free run */
#define BRK_STEP            1     /* Step n debug elements */
#define BRK_STEP_LINE       2     /* Step n lines */
#define BRK_LINE            3     /* Run to line n */
#define BRK_PARENT          4     /* Run to parent */
#define BRK_PARENT_PROGRAM  5     /* Run to parent program */
#define BRK_TRACE           6     /* Return to debugged program in trace mode */
#define BRK_ADD_LINE        7     /* Add new breakpoint */
#define BRK_CLEAR           8     /* Clear all breakpoints */
#define BRK_CLR_LINE        9     /* Clear a specific breakpoint */
#define BRK_GOTO_LINE      10     /* Continue at given line */

/* END-CODE */
