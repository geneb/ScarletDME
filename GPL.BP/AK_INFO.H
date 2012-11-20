* AK_INFO.H
* AK related data.
* Copyright (c) 2001 Ladybridge Systems, All Rights Reserved
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

$define MAX$INDICES  32        ;* Maximum number of indices per file

* Modes for AK update subroutine calls (Also in DH_INT.H)

$define AK$ADD  1
$define AK$DEL  2
$define AK$MOD  3

* AK data matrix column values passed into _AK subroutine.
* This is NOT the ak.data matrix used by the CREATE.INDEX command.

$define AKD$NAME      1        ;* AK field name. Null string if unused (gap) AK
$define AKD$FNO       2        ;* Field number, -ve if I-type
$define AKD$FLGS      3        ;* Flags
$define AKD$OBJ       4        ;* I-type object code
$define AKD$MAPNAME   5        ;* Collation map name (null string if none)...
$define AKD$MAP       6        ;* ...and the map (null if none)
$define AKD$COLS      6        ;* No of columns

* Flag bit values (Also in DH_FMT.H) 
$define AK$ENABLED    1        ;* Index update and usage is enabled
$define AK$RIGHT      2        ;* Right justified
$define AK$NULLS      4        ;* Includes null values
$define AK$MV         8        ;* Multi-valued
$define AK$LSORT     16        ;* Keys sorted left aligned  } Mutually
$define AK$RSORT     32        ;* Keys sorted right aligned } exclusive
$define AK$NOCASE    64        ;* Case insensitive

* END-CODE
