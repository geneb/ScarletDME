* HEADER.H
* Object code header
* Copyright (c) 2004 Ladybridge Systems, All Rights Reserved
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
* 08 Dec 04  2.1-0 Added HDR.CTYPE.
* 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
* END-HISTORY
*
* START-DESCRIPTION:
*
* *************************************************************************
* ******                           WARNING                           ******
* ******                                                             ******
* ******   All changes to this file must be reflected by changes to  ******
* ******   the HEADER.H C include file.                              ******
* ******                                                             ******
* *************************************************************************
*
* END-DESCRIPTION
*
* START-CODE

$define INCLUDE.SUFFIX           "H"
$define SCREEN.DEFINITION.SUFFIX "SCR"
$define LISTING.SUFFIX           ".LIS"
$define RECURSIVE.SUFFIX         ".rcr"

* Header field offsets (from 1)
$define hdr.magic                  1
$define hdr.rev                    2
$define hdr.id                     3    ;* Used after loading to memory
$define hdr.start.offset           7
$define hdr.args                  11
$define hdr.no.vars               13
$define hdr.stack.depth           15
$define hdr.sym.tab.offset        17
$define hdr.lin.tab.offset        21
$define hdr.object.size           25
$define hdr.flags                 29
$define hdr.compile.time          31    ;* 4 bytes: = SYSTEM(1005)

* Later items differ depending on object type

* Programs and subroutines
$define hdr.refs                  35
$define hdr.program.name          37
$define hdr.program.name.len     128
$define object.header.size       165

* I-types
$define hdr.totals                35    ;* Number of TOTAL() functions (1 byte)
$define itype.object.header.size  36

* Inline code
$define inline.object.header.size 35

* Magic number and revision
$define hdr.magic.no.l        100    ;* For little endian machines
$define hdr.magic.no.b        101    ;* For big endian machines
$define hdr.revision            0

* Header flags bit values
$define hdr.is.cproc         0x0001  ;* Command processor
$define hdr.internal         0x0002  ;* Internal mode program
$define hdr.debug            0x0004  ;* Compiled in debug mode
$define hdr.is.debugger      0x0008  ;* Debugger
$define hdr.nocase           0x0010  ;* Case insensitive string operations
$define hdr.is.function      0x0020  ;* Basic function
$define hdr.var.args         0x0040  ;* Variable arg count (hdr.args = max)
$define hdr.recursive        0x0080  ;* Is a recursive program
$define hdr.itype            0x0100  ;* Is an A/S/C/I-type
$define hdr.allow.break      0x0200  ;* Allow break in recursive
$define hdr.is.trusted       0x0400  ;* Trusted program
$define hdr.netfiles         0x0800  ;* Allow remote files by NFS
$define hdr.case.sensitive   0x1000  ;* Program uses case sensitive names
$define hdr.qmcall.allowed   0x2000  ;* Can be called using QMCall()
$define hdr.ctype            0x4000  ;* Is a C-type
$define hdr.is.class         0x8000  ;* Is CLASS module

* Flag bits not in header but in process.program.flags
* (see kernel.h for full list)
$define hdr.is.execute    0x00010000 ;* Started from an EXECUTE
$define hdr.ignore.aborts 0x00040000 ;* Ignore aborts from EXECUTEd sentence

* END-CODE
