* KEYCODE.H
* COmmon definition for KEYCODE() function.
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

   common /$keycode/ kc.term.type,  ;* Used to check if bound correctly
                     kc.keys,       ;* Key binding string
                     kc.codes,      ;* Action number. Considered as a two byte
                                    ;* value, the high order byte indicates the
                                    ;* key binding type:
                                    ;*   0    A normal key code
                                    ;*   1    A KEYEXIT code. LS byte = action
                                    ;*   2    A KEYTRAP code. LS byte = action
                     kc.disable.esc ;* Disable lone Esc handling in KEYCODE()

* END-CODE
