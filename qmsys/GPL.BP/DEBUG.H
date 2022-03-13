* DEBUG.H
* Debugger include file
* Copyright (c) 2006 Ladybridge Systems, All Rights Reserved

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
*  START-HISTORY:
* 11 Apr 06  2.4-1 Added program.flags array.
* 16 Mar 05  2.1-10 0325 DEBUG.DATA.AREA is now a dimensioned matrix.
* 28 Jan 05  2.1-4 Added debug.command.stack.
* 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
*  END-HISTORY
*
*  START-DESCRIPTION:

* *************************************************************************
* ******                           WARNING                           ******
* ******                                                             ******
* ******   Changes to this file may need to be reflected by changes  ******
* ******   the DEBUGGER.H C include file.                            ******
* ******                                                             ******
* *************************************************************************

* END-DESCRIPTION

* START-CODE

$define INITIAL.PROGRAMS      10    ;* Initial size of program matrices
$define INITIAL.CHUNKS        50    ;* Initial size of source chunk matrices
$define CHUNK.LOAD           100    ;* Lines per chunk
$define MAX.BREAKPOINTS       20    ;* Also in op_debug.c
$define DATA.AREA.LINES        5

   common /$debug/ debug.initialised,
                   full.screen,                ;* Full screen mode?
                   title.attr,
                   source.attr,
                   highlight.source.attr,
                   breakpoint.source.attr,
                   command.attr,
                   debug.screen.image,
                   debug.command.stack,
                   debug.overlay,
                   debug.data.area(DATA.AREA.LINES), ;* The data area

                   array.name,
                   array.offset,               ;* Position of rolling display
                   rows,
                   cols,
                   last.display,               ;* Last display command
                   src.pos,                    ;* Position for SRC n,m command
                   src.incr,                   ;* ...and line count

                   banned,
                   replacements,

                   chunk(INITIAL.CHUNKS),
                   next.chunk(INITIAL.CHUNKS),
                   free.chunk,                 ;* Free chunk chain head

                   program.id(INITIAL.PROGRAMS),    ;* Id (compile time)
                   program.name(INITIAL.PROGRAMS),  ;* Program name
                   head(INITIAL.PROGRAMS),
                   lines(INITIAL.PROGRAMS),
                   symbols(INITIAL.PROGRAMS),
                   program.flags(INITIAL.PROGRAMS),  ;* Header flags
                   last.program,

                   breakpoints,    ;* Stored as programindex.line, FM delimited
                   watch.var.name,              ;* Name of watched variable

                   system.variables,
                   basic.chars,                 ;* Valid characters in names
                   rebind.string,               ;* Codes to rebind keys
                   mouse.on,                    ;* Enable mouse string
                   mouse.off,                   ;* Disable mouse string

                   debugger.uid                 ;* Other side for phantom debugger

* Event codes (additive)

$define DE$WATCH            1     ;* Watch variable has changed
$define DE$BREAKPOINT       2     ;* At breakpoint

* Debug actions

$define BRK$RUN             0     ;* Free run
$define BRK$STEP            1     ;* Step n debug elements
$define BRK$STEP.LINE       2     ;* Step n lines
$define BRK$LINE            3     ;* Run to line n
$define BRK$PARENT          4     ;* Run to parent
$define BRK$PARENT.PROGRAM  5     ;* Run to parent program
$define BRK$TRACE           6     ;* Return to debugged program in trace mode
$define BRK$ADD.LINE        7     ;* Add a new breakpoint line
$define BRK$CLEAR           8     ;* Clear all breakpoints
$define BRK$CLR.LINE        9     ;* Clear breakpoint line
$define BRK$GOTO.LINE       10    ;* Continue execution at given line

* File types from DESCR.H
$define FVAR.DIRECTORY      1
$define FVAR.DYNAMIC        2
$define FVAR.SEQ            3
$define FVAR.NET            4

* END-CODE
