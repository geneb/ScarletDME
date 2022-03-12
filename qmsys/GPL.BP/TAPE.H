* TAPE.H
* Include record for tape functions.
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
* 03 Apr 06  2.4-0 New items for enhanced tape utilities.
* 27 Jun 05  2.2-1 New module.
* 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
* END-HISTORY
*
* START-DESCRIPTION:
*
* Copyright (c) 2005-2006 Tom deLombarde
* This module was submitted by Tom deLombarde <tomd@blackflute.com>.
*
* END-DESCRIPTION
*
* START-CODE

common /$tapecom/ tp$name,      ;* Device name, null or zero if not open
                  tp$fh,        ;* File handle
                  tp$fname,     ;* Name of first reel (file)
                  tp$blksz,     ;* Logical block size
                  tp$pad,       ;* Padding byte count to make physical block
                  tp$type,      ;* Type of tape (FS = FILE-SAVE)
                  tp$capacity,  ;* Capacity
                  tp$ptr,       ;* Pointer in this reel
                  tp$tptr,      ;* Pointer of total bytes read
                  tp$lastblk,   ;* Type of block last read
                  tp$lasttype,  ;* 
                  tp$eof,       ;* At EOF?
                  tp$eot        ;* At EOT?

* END-CODE
