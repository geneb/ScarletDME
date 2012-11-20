* TERMINFO.H
* TERMINFO() function dynamic array structure
* Copyright (c) 2003 Ladybridge Systems, All Rights Reserved

* The names used for these terminal attributes are mostly the standard terminfo
* full name for the corresponding capability though some of the longer or less
* obvious ones have been changed.
*
* The first item in the comment text is the actual terminfo capability name.
* This can be used as the argument to TERMINFO() to retrieve a specific
* capability.

* Terminal characteristics
$define T$TERM.TYPE               1 ;* Terminal device type name
$define T$COLUMNS                 2 ;* cols   : Default number of columns
$define T$LINES                   3 ;* lines  : Default number of lines

* Control codes
$define T$CARRIAGE.RETURN         4 ;* cr     : Carriage return code
$define T$FORM.FEED               5 ;* ff     : Form feed code
$define T$NEWLINE                 6 ;* nel    : CR followed by LF
$define T$BELL                    7 ;* bel    : Audible alarm
$define T$TAB                     8 ;* ht     : Tab
$define T$BACK.TAB                9 ;* cbt    : Back tab
$define T$CLEAR.SCREEN           10 ;* clear  : Clear screen, cursor to top left [@(-1)]
$define T$CLR.EOL                11 ;* el     : Clear to end of line [@(-4)]
$define T$CLR.EOS                12 ;* ed     : Clear to end of screen [@(-3)]
$define T$COLUMN.ADDRESS         13 ;* hpa    : Horizontal cursor position
$define T$CURSOR.ADDRESS         14 ;* cup    : Cursor position
$define T$CURSOR.LEFT            15 ;* cub1   : Cursor left one column [@(-9)]
$define T$CURSOR.LEFT.N          16 ;* cub    : Cursor left n columns
$define T$CURSOR.RIGHT           17 ;* cuf1   : Cursor right one column [@(-34)]
$define T$CURSOR.RIGHT.N         18 ;* cuf    : Cursor right n columns
$define T$CURSOR.UP              19 ;* cuu1   : Cursor up one line [@(-10)]
$define T$CURSOR.UP.N            20 ;* cuu    : Cursor up n lines
$define T$CURSOR.DOWN            21 ;* cud1   : Cursor down one line [@(-33)]
$define T$CURSOR.DOWN.N          22 ;* cud    : Cursor down n lines
$define T$CURSOR.HOME            23 ;* home   : Cursor home [@(-2)]
$define T$DELETE.CHARACTER       24 ;* dch1   : Delete one character [@(-22)]
$define T$DELETE.LINE            25 ;* dl1    : Delete one line [@(-18)]
$define T$DELETE.LINES           26 ;* dl     : Delete n lines [@(-18)]
$define T$INSERT.CHARACTER       27 ;* ich1   : Insert one character [@(-19)]
$define T$INSERT.LINE            28 ;* il1    : Insert one line [@(-17)]
$define T$INSERT.LINES           29 ;* il     : Insert n lines [@(-17)]
$define T$ENTER.BLINK.MODE       30 ;* blink  : Start flashing text [@(-5)]
$define T$ENTER.BOLD.MODE        31 ;* bold   : Start highlighted text [@(-58)]
$define T$ENTER.DIM.MODE         32 ;* dim    : Start half brightness text [@(-11)]
$define T$ENTER.REVERSE.MODE     33 ;* rev    : Start reverse video [@(-13)]
$define T$ENTER.UNDERLINE.MODE   34 ;* smul   : Start underlined text [@(-15)]
$define T$EXIT.ATTRIBUTE.MODE    35 ;* sgr0   : Clear all text attributes [@(-6), @(-12), @(-14), @(-59)]
$define T$EXIT.UNDERLINE.MODE    36 ;* rmul   : End underlined text [@(-16)]
$define T$PRTR.OFF               37 ;* mc4    : Turn printer off [@(-24)]
$define T$PRTR.ON                38 ;* mc5    : Turn printer on [@(-23)]
$define T$SCROLL.FORWARD         39 ;* ind    : Scroll forward
$define T$SCROLL.BACK            40 ;* ri     : Scroll backward
$define T$SET.FOREGROUND         41 ;* setf   : Set foreground colour attribute [@(-37)]
$define T$SET.BACKGROUND         42 ;* setb   : Set background colour attribute [@(-38)]
$define T$USER0                  43 ;* u0     : User 0
$define T$USER1                  44 ;* u1     : User 1
$define T$USER2                  45 ;* u2     : User 2
$define T$USER3                  46 ;* u3     : User 3
$define T$USER4                  47 ;* u4     : User 4
$define T$USER5                  48 ;* u5     : User 5
$define T$USER6                  49 ;* u6     : User 6
$define T$USER7                  50 ;* u7     : User 7
$define T$USER8                  51 ;* u8     : User 8
$define T$USER9                  52 ;* u9     : User 9

* Keys
$define T$KEY.BACKSPACE          53 ;* kbs    : Backspace key
$define T$KEY.BTAB               54 ;* kcbt   : Back tab key
$define T$KEY.LEFT               55 ;* kcub1  : Cursor left key
$define T$KEY.RIGHT              56 ;* kcuf1  : Cursor right key
$define T$KEY.UP                 57 ;* kcuu1  : Cursor up key
$define T$KEY.DOWN               58 ;* kcud1  : Cursor down key
$define T$KEY.NPAGE              59 ;* knp    : Next page key
$define T$KEY.PPAGE              60 ;* kpp    : Previous page key
$define T$KEY.SF                 61 ;* kind   : Scroll forward key
$define T$KEY.SR                 62 ;* kri    : Scroll back key
$define T$KEY.HOME               63 ;* khome  : Home key
$define T$KEY.END                64 ;* kend   : End key
$define T$KEY.ENTER              65 ;* kent   : Enter key
$define T$KEY.HELP               66 ;* khlp   : Help key
$define T$KEY.PRINT              67 ;* kprt   : Print key
$define T$KEY.CLEAR              68 ;* kclr   : Clear key
$define T$KEY.IC                 69 ;* kich1  : Insert character key
$define T$KEY.IL                 70 ;* kil1   : Insert line key
$define T$KEY.DC                 71 ;* kdch1  : Delete character key
$define T$KEY.DL                 72 ;* kdl1   : Delete line key
$define T$KEY.EOL                73 ;* kel    : Delete to end of line key
$define T$KEY.EOS                74 ;* ked    : Delete to end of screen key
$define T$KEY.F0                 75 ;* kf0    : F0 key
$define T$KEY.F1                 76 ;* kf1    : F1 key
$define T$KEY.F2                 77 ;* kf2    : F2 key
$define T$KEY.F3                 78 ;* kf3    : F3 key
$define T$KEY.F4                 79 ;* kf4    : F4 key
$define T$KEY.F5                 80 ;* kf5    : F5 key
$define T$KEY.F6                 81 ;* kf6    : F6 key
$define T$KEY.F7                 82 ;* kf7    : F7 key
$define T$KEY.F8                 83 ;* kf8    : F8 key
$define T$KEY.F9                 84 ;* kf9    : F9 key
$define T$KEY.F10                85 ;* kf10   : F10 key
$define T$KEY.F11                86 ;* kf11   : F11 key
$define T$KEY.F12                87 ;* kf12   : F12 key
$define T$KEY.SLEFT              88 ;* kLFT   : Shifted left arrow key
$define T$KEY.SRIGHT             89 ;* kRIT   : Shifted right arrow key
$define T$KEY.SIC                90 ;* kIC    : Shifted insert character key
$define T$KEY.SDC                91 ;* kDC    : Shifted delete character key
$define T$KEY.SDL                92 ;* kDL    : Shifted delete line key
$define T$KEY.SEND               93 ;* kEND   : Shifted end key
$define T$KEY.SEOL               94 ;* kEOL   : Shifted clear to end of line key
$define T$KEY.SHOME              95 ;* kHOM   : Shifted home key
$define T$KEY.MOUSE              96 ;* kmous  : Mouse input

* QM extensions to the standard terminfo database
$define T$SLT                    97 ;* slt    : Set line truncate mode [@(-54)]
$define T$RLT                    98 ;* rlt    : Reset line truncate mode [@(-55)]
$define T$KEY.USER0              99 ;* ku0    : User key 0
$define T$KEY.USER1             100 ;* ku1    : User key 1
$define T$KEY.USER2             101 ;* ku2    : User key 2
$define T$KEY.USER3             102 ;* ku3    : User key 3
$define T$KEY.USER4             103 ;* ku4    : User key 4
$define T$KEY.USER5             104 ;* ku5    : User key 5
$define T$KEY.USER6             105 ;* ku6    : User key 6
$define T$KEY.USER7             106 ;* ku7    : User key 7
$define T$KEY.USER8             107 ;* ku8    : User key 8
$define T$KEY.USER9             108 ;* ku9    : User key 9
$define T$SREG                  109 ;* sreg   : Save screen region
$define T$RREG                  110 ;* rreg   : Restore screen region
$define T$DREG                  111 ;* dreg   : Delete saved screen region
* Other standard terminfo items
$define T$CVVIS                 112 ;* cvvis  : Set cursor visible
$define T$CIVIS                 113 ;* cvvis  : Set cursor invisible
$define T$XMC                   114 ;* xmc    : Space occupied by attribute
