* SCREENS.H
* Screen definition record structure.
* Copyright (c) 2001 Ladybridge Systems, All Rights Reserved

equate SCR.NAME        to    1 ;*  Step name
equate SCR.TYPE        to    2 ;*  Step type
equate SCR.CLEAR       to    3 ;*  Clear screen
equate SCR.TEXT        to    4 ;*  Prompt text
equate SCR.DISP.STEP   to    5 ;*  Display these steps first
equate SCR.TEXT.ROW    to    6 ;*  Row for text display
equate SCR.TEXT.COL    to    7 ;*  Col for text display
equate SCR.TEXT.MODE   to    8 ;*  Mode for text display
equate SCR.FIELD       to    9 ;*  Data field
equate SCR.VALUE       to   10 ;*  Data value
equate SCR.SUBVALUE    to   11 ;*  Data subvalue
equate SCR.PROMPT.CHAR to   12 ;*  Prompt character
equate SCR.FILL.CHAR   to   13 ;*  Fill character
equate SCR.DATA.ROW    to   14 ;*  Row for data display
equate SCR.DATA.COL    to   15 ;*  Col for data display
equate SCR.DATA.MODE   to   16 ;*  Mode for data display
equate SCR.OUTPUT.LEN  to   17 ;*  Data output length
equate SCR.OUTPUT.CONV to   18 ;*  Output conversion
equate SCR.JUSTIFY     to   19 ;*  Justification
equate SCR.END.MARK    to   20 ;*  Field end marker
equate SCR.INPUT.LEN   to   21 ;*  Input length
equate SCR.REQUIRED    to   22 ;*  Data required
equate SCR.VAL.1       to   23 ;*  Validation before conversion
equate SCR.INPUT.CONV  to   24 ;*  Input conversion
equate SCR.VAL.2       to   25 ;*  Validation after conversion
equate SCR.BACKSTEP    to   26 ;*  Backstep key actions
equate SCR.NEXT.STEP   to   27 ;*  Next step
equate SCR.HELP.MSG    to   28 ;*  F1 help message key
equate SCR.ERROR.MSG   to   29 ;*  Error message key
equate SCR.EXIT.KEY    to   30 ;*  Exit key actions
equate SCR.F2          to   31 ;*  F2 key action
equate SCR.FKEYS       to   32 ;*  Function keys allowed (Y/N)
equate SCR.FIELD.NAME  to   33 ;*  Field name
equate SCR.HEADER      to   34 ;*  Header
equate SCR.FILE.NAME   to   35 ;*  Include file name
equate SCR.KEY.VAL     to   36 ;*  Key validation subroutine and error text
equate SCR.ATTR        to   37 ;*  Default attributes
