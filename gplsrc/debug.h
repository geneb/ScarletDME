/* DEBUG.H
 * Debugger include file
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
 * START-HISTORY:
 * 01 Jul 07  2.5-7 Extensive changes for PDA merge.
 * 19 May 05  2.2-0 Added saved_os_error.
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

Public bool in_debugger init(FALSE);

Public long int debug_status;      /* Saved process.status */
Public long int debug_inmat;       /* Saved process.inmat */
Public bool debug_suppress_como;   /* Saved tio.suppress_como */
Public bool debug_hush;            /* Saved tio.hush */
Public bool debug_capturing;       /* Saved capturing */
Public char debug_prompt_char;     /* Saved tio.prompt_char */
Public long int debug_dsp_line;    /* Saved tio.dsp.line */
Public bool debug_dsp_paginate;    /* Saved tio.dsp.paginate */
Public long int debug_os_error;    /* Saved process.os_error */

/* Event codes (additive) */

#define DE_WATCH            1     /* Watch variable has changed */
#define DE_BREAKPOINT       2     /* At breakpoint */

/* END-CODE */

