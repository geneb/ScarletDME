* MENU.H
* Menu record layout
* Copyright (c) 1996 Ladybridge Systems, All Rights Reserved

$define M$TITLE       2  ;* Title line
$define M$TEXT        3  ;* Option prompt texts (multi-valued)
$define M$ACTION      4  ;* Action (multi-valued)
$define M$HELP        5  ;* Help text (multi-valued)
$define M$ACCESS      6  ;* Access key (multi-valued)
$define M$ACCESS.HIDE 7  ;* Hide inaccessible options (bool)
                          * If multi-valued, one per item, else applies to all
$define M$ACCESS.SUB  8  ;* Subroutine for access control
$define M$PROMPT      9  ;* Alternative prompt text
$define M$EXITS      10  ;* Codes to exit to parent menu (multi-valued)
$define M$STOPS      11  ;* Codes to terminate all menus (multi-valued)
