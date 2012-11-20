* TOKENS.H
* Compiler parser token definitions.
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
* 07 Dec 04  2.1-0 Added TKN.LCBR and TKN.RCBR.
* 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
* END-HISTORY
*
* START-DESCRIPTION:
*
* END-DESCRIPTION
*
* START-CODE

$define TKN.END         0           ;* MUST BE ZERO (Assumed by compiler)
$define TKN.NAME        1  
$define TKN.NUM         2  
$define TKN.LABEL       3  
$define TKN.STRING      4  
$define TKN.LSQBR       5  
$define TKN.RSQBR       6  
$define TKN.LBR         7  
$define TKN.RBR         8  

* The tokens below don't really exist but are used in the compiler to handle
* special cases. They must be less than TKN.HIGH.OPERATOR so that there is an
* entry in the compiler operator tables for them.

$define TKN.FIELD.REF   9 ;* < symbol identified as a field reference
$define TKN.COMMA      10 ;* Comma may be used in field reference
$define TKN.FMT        11 ;* SMA format expression

$define TKN.LOW.OPERATOR 12
$define TKN.LT         12  ;* <
$define TKN.LTX        13  ;* LT
$define TKN.GT         14  ;* >
$define TKN.GTX        15  ;* GT
$define TKN.EQ         16  ;* =     EQ
$define TKN.NE         17  ;* #     <>     NE
$define TKN.NEX        18  ;* ><
$define TKN.LE         19  ;* <=    =<     #>     LE
$define TKN.GE         20  ;* >=
$define TKN.GEX        21  ;* =>     #<     GE
$define TKN.AND        22  ;* &     AND
$define TKN.OR         23  ;* !     OR
$define TKN.PLUS       24  ;* +
$define TKN.MINUS      25  ;* -
$define TKN.DIV        26  ;* /
$define TKN.IDIV       27  ;* /
$define TKN.MULT       28  ;* *
$define TKN.PWR        29  ;* **    ^
$define TKN.COLON      30  ;* :     CAT
$define TKN.MATCHES    31  ;* MATCHES
$define TKN.HIGH.OPERATOR 31

$define TKN.ADDEQ      32
$define TKN.SUBEQ      33
$define TKN.SEMICOLON  34
$define TKN.NAME.LBR   35
$define TKN.CATEQ      36
$define TKN.DOLLAR     37
$define TKN.AT         38
$define TKN.UNDERSCORE 39
$define TKN.MULTEQ     40
$define TKN.DIVEQ      41
$define TKN.FLOAT      42
$define TKN.AT.NAME    43     ;* @ followed by letter with no intervening space
$define TKN.LCBR       44  ;* {
$define TKN.RCBR       45  ;* }
$define TKN.HEXNUM     46  ;* Hexadecimal number
$define TKN.OBJREF     47  ;* -> (Object reference)

$define TKN.UNCLOSED   62  ;* Unclosed string
$define TKN.UNKNOWN    63
$define TKN.END.FIELD  99  ;* Pseudo-token for ">" of dynamic array reference

* END-CODE
