* QDISP.H
* Tokens for query processor display handler.
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
* 18 Jan 06  2.3-5 New module.
* 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
* END-HISTORY
*
* START-DESCRIPTION:
*
* END-DESCRIPTION
*
* START-CODE

$define QD$INIT  1 ;* Initialise for new report
$define QD$BOX   2 ;* Set boxed mode
$define QD$LPTR  3 ;* Set destination print unit : Returns usable width
$define QD$PHDR  4 ;* Set page heading
$define QD$CHDR  5 ;* Set column headings
$define QD$PFTR  6 ;* Set page footing
$define QD$SCRL  7 ;* Set scroll mode
$define QD$PAN   8 ;* Set pan columns
$define QD$LBL   9 ;* Set label parameters
$define QD$STYL 10 ;* Set style parameters
$define QD$NPAG 11 ;* Disable pagination
$define QD$NOHF 12 ;* Kill headings/footings
$define QD$EMIT 13 ;* Emit a line
$define QD$PAGE 14 ;* Start new page
$define QD$NLBL 15 ;* Start new label
$define QD$NEED 16 ;* Start new page if less than "text" lines left
$define QD$END  17 ;* End of report
$define QD$TERM 18 ;* Terminate display handler
$define QD$SETP 19 ;* Set page number
$define QD$GETP 20 ;* Get page number at end of report

deffun qdisp(mode, text, class) calling '$qdisp' var.args

* END-CODE
