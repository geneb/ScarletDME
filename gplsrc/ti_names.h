/* TI_NAMES.H
 * Terminfo capability names.
 * Copyright (c) 2007 Ladybridge Systems, All Rights Reserved
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
 * 26 Oct 07  2.6-5 Added xmc attribute.
 * 10 Jan 05  2.1-0 Added QM extensions.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

char * boolnames[] = {
"bw",              /* cub1 wraps from column 0 to last column */
"am",              /* Terminal has automatic margins */
"xsb",             /* Beehive (f1=escape, f2=ctrl C) */
"xhp",             /* Standout not erased by overwriting (hp) */
"xenl",            /* Newline ignored after 80 cols (concept) */
"eo",              /* Can erase overstrikes with a blank */
"gn",              /* Generic line type */
"hc",              /* Hardcopy terminal */
"km",              /* Has a meta key (i.e., sets 8th-bit) */
"hs",              /* Has extra status line */
"in",              /* Insert mode distinguishes nulls */
"da",              /* Display may be retained above the screen */
"db",              /* Display may be retained below the screen */
"mir",             /* Safe to move while in insert mode */
"msgr",            /* Safe to move while in standout mode */
"os",              /* Terminal can overstrike */
"eslok",           /* Escape can be used on the status line */
"xt",              /* Tabs destructive, magic so char (t1061) */
"hz",              /* Cannot print ~'s (hazeltine) */
"ul",              /* Underline character overstrikes */
"xon",             /* Terminal uses xon/xoff handshaking */
"nxon",            /* Padding will not work, xon/xoff required */
"mc5i",            /* Printer will not echo on screen */
"chts",            /* Cursor is hard to see */
"nrrmc",           /* Smcup does not reverse rmcup */
"npc",             /* Pad character does not exist */
"ndscr",           /* Scrolling region is non-destructive */
"ccc",             /* Terminal can re-define existing colors */
"bce",             /* Screen erased with background color */
"hls",             /* Terminal uses only HLS color notation (Tektronix) */
"xhpa",            /* Only positive motion for hpa/mhpa caps */
"crxm",            /* Using cr turns off micro mode */
"daisy",           /* Printer needs operator to change character set */
"xvpa",            /* Only positive motion for vpa/mvpa caps */
"sam",             /* Printing in last column causes cr */
"cpix",            /* Changing character pitch changes resolution */
"lpix"};           /* Changing line pitch changes resolution */
#define NumBoolNames (sizeof(boolnames) / sizeof(char *))

char * numnames[] = {
"cols",            /* Number of columns in a line */
"it",              /* Tabs initially every # spaces */
"lines",           /* Number of lines on screen or page */
"lm",              /* Lines of memory if > line. 0 means varies */
"xmc",             /* Number of blank characters left by smso or rmso */
"pb",              /* Lowest baud rate where padding needed */
"vt",              /* Virtual terminal number (CB/unix) */
"wsl",             /* Number of columns in status line */
"nlab",            /* Number of labels on screen */
"lh",              /* Rows in each label */
"lw",              /* Columns in each label */
"ma",              /* Maximum combined attributes terminal can handle */
"wnum",            /* Maximum number of defineable windows */
"colors",          /*  */
"pairs",           /*  */
"ncv",             /*  */
"xmc"};            /* Space occupied by attribute */
#define NumNumNames (sizeof(numnames) / sizeof(char *))

char * strnames[] = {
"cbt",             /* Back tab */
"bel",             /* Bell character */
"cr",              /* Carriage return */
"csr",             /* Change scroll region */
"tbc",             /* Clear all tabs */
"clear",           /* Clear screen */
"el",              /* Clear to end of line */
"ed",              /* Clear to end of screen */
"hpa",             /* Horizontal position */
"cmdch",           /* Command character */
"cup",             /* Cursor address */
"cud1",            /* Cursor down one line */
"home",            /* Cursor home */
"civis",           /* Cursor invisible */
"cub1",            /* Cursor left one column */
"mrcup",           /* Cursor memory address */
"cnorm",           /* Cursor normal */
"cuf1",            /* Cursor right one column */
"ll",              /* Cursor to last line */
"cuu1",            /* Cursor up one line */
"cvvis",           /* Cursor visible */
"dch1",            /* Delete one character */
"dl1",             /* Delete one line */
"dsl",             /* Disable status line */
"hd",              /* Down half line */
"smacs",           /* Enter alt charset mode */
"blink",           /* Enter blink mode */
"bold",            /* Enter bold mode */
"smcup",           /* String to start programs using cup */
"smdc",            /* Enter delete mode */
"dim",             /* Enter half brightness mode */
"smir",            /* Enter insert mode */
"invis",           /* Turn on blank mode */
"prot",            /* Turn on protected mode */
"rev",             /* Enter reverse video mode */
"smso",            /* Begin standout mode */
"smul",            /* Enter underline mode */
"ech",             /* Erase n characters */
"rmacs",           /* End alternate character set */
"sgr0",            /* Turn off all attributes */
"rmcup",           /* Strings to end programs using cup */
"rmdc",            /* End delete mode */
"rmir",            /* Exit insert mode */
"rmso",            /* Exit standout mode */
"rmul",            /* Exit underline mode */
"flash",           /* Visible bell */
"ff",              /* Hardcopy terminal page eject */
"fsl",             /* Return from status line */
"is1",             /* Initialisation string */
"is2",             /* Initialisation string */
"is3",             /* Initialisation string */
"if",              /* Name of initialisation file */
"ich1",            /* Insert one character */
"il1",             /* Insert one line */
"ip",              /* Insert padding after inserted character */
"kbs",             /* Backspace key */
"ktbc",            /* Clear all tabs key */
"kclr",            /* Clear screen key */
"kctab",           /* Clear tab key */
"kdch1",           /* Delete character key */
"kdl1",            /* Delete line key */
"kcud1",           /* Down arrow key */
"krmir",           /* Sent by rmir or smir in insert mode */
"kel",             /* Clear to end of line key */
"ked",             /* Clear to end of screen key */
"kf0",             /* Function key F0 */
"kf1",             /* Function key F1 */
"kf10",            /* Function key F10 */
"kf2",             /* Function key F2 */
"kf3",             /* Function key F3 */
"kf4",             /* Function key F4 */
"kf5",             /* Function key F5 */
"kf6",             /* Function key F6 */
"kf7",             /* Function key F7 */
"kf8",             /* Function key F8 */
"kf9",             /* Function key F9 */
"khome",           /* Home key */
"kich1",           /* Insert character key */
"kil1",            /* Insert line key */
"kcub1",           /* Left arrow key */
"kll",             /* Lower left key */
"knp",             /* Next page key */
"kpp",             /* Previous page key */
"kcuf1",           /* Right arrow key */
"kind",            /* Scroll forward key */
"kri",             /* Scroll backward key */
"khts",            /* Set tab key */
"kcuu1",           /* Up arrow key */
"rmkx",            /* Enter keyboard transmit mode */
"smkx",            /* Leave keyboard transmit mode */
"lf0",             /* Label on function key 0 if not F0 */                     
"lf1",             /* Label on function key 1 if not F1 */                     
"lf10",            /* Label on function key 10 if not F10 */                      
"lf2",             /* Label on function key 2 if not F2 */                     
"lf3",             /* Label on function key 3 if not F3 */                     
"lf4",             /* Label on function key 4 if not F4 */                     
"lf5",             /* Label on function key 5 if not F5 */                     
"lf6",             /* Label on function key 6 if not F6 */                     
"lf7",             /* Label on function key 7 if not F7 */                     
"lf8",             /* Label on function key 8 if not F8 */                     
"lf9",             /* Label on function key 9 if not F9 */                     
"rmm",             /* Turn off meta mode */
"smm",             /* Turn on meta mode */
"nel",             /* Newline (behave like cr followed by lf) */
"pad",             /* Padding character */
"dch",             /* Delete n characters */
"dl",              /* Delete n lines */
"cud",             /* Down n lines */
"ich",             /* Insert n characters */
"indn",            /* Scroll forward n lines */
"il",              /* Insert n lines */
"cub",             /* Cursor left n */
"cuf",             /* Cursor right n */
"rin",             /* Scroll back n lines */
"cuu",             /* Cursor up n lines */
"pfkey",           /* Program function key #1 to type string #2 */
"pfloc",           /* Program function key #1 to execute string #2 */
"pfx",             /* Program function key #1 to transmit string #2 */
"mc0",             /* Print contents of screen */
"mc4",             /* Turn off printer */
"mc5",             /* Turn on printer */
"rep",             /* Repeat char #1 #2 times */
"rs1",             /* Reset string */
"rs2",             /* Reset string */
"rs3",             /* Reset string */
"rf",              /* Name of reset file */
"rc",              /* Restore cursor to position of last save cursor */
"vpa",             /* Vertical position #1 absolute */
"sc",              /* Save current cursor position */
"ind",             /* Scroll text up */
"ri",              /* Scroll text down */
"sgr",             /* Define video attributes #1 = #9 */
"hts",             /* Set tab in every row, current columns */
"wind",            /* Current window is lines #1-#2 cols #3-#4 */
"ht",              /* Tab to next 8 space hardware tab stop */
"tsl",             /* Move to status line column 1 */
"uc",              /* Underline character and move past it */
"hu",              /* Half a line up */
"iprog",           /* Path name of program for initialisation */
"ka1",             /* Upper left of keypad */
"ka3",             /* Upper right of keypad */
"kb2",             /* Centre of keypad */
"kc1",             /* Lower left of keypad */
"kc3",             /* Lower right of keypad */
"mc5p",            /* Turn on printer for #1 bytes */
"rmp",             /* Like ip but when in insert mode */
"acsc",            /* Graphics charset pairs based on vt100 */
"pln",             /* Program label #1 to show string #2 */
"kcbt",            /* Back tab key */
"smxon",           /* Turn on xon/xoff handshaking */
"rmxon",           /* Turn off xon/xoff handshaking */
"smam",            /* Turn on automatic margins */
"rmam",            /* Turn off automatic margins */
"xonc",            /* XON character */
"xoffc",           /* XOFF character */
"enacs",           /* Enable alternate character set */
"smln",            /* Turn on soft labels */
"rmln",            /* Turn off soft labels */
"kbeg",            /* Begin key */
"kcan",            /* Cancel key */
"kclo",            /* Close key */
"kcmd",            /* Command key */
"kcpy",            /* Copy key */
"kcrt",            /* Create key */
"kend",            /* End key */
"kent",            /* Enter/send key */
"kext",            /* Exit key */
"kfnd",            /* Find key */
"khlp",            /* Help key */
"kmrk",            /* Mark key */
"kmsg",            /* Message key */
"kmov",            /* Move key */
"knxt",            /* Next key */
"kopn",            /* Open key */
"kopt",            /* Options key */
"kprv",            /* Previous key */
"kprt",            /* Print key */
"krdo",            /* Redo key */
"kref",            /* Reference key */
"krfr",            /* Refresh key */
"krpl",            /* Replace key */
"krst",            /* Restart key */
"kres",            /* Resume key */
"ksav",            /* Save key */
"kspd",            /* Suspend key */
"kund",            /* Undo key */
"kBEG",            /* Shifted begin key */
"kCAN",            /* Shifted cancel key */
"kCMD",            /* Shifted command key */
"kCPY",            /* Shifted copy key */
"kCRT",            /* Shifted create key */
"kDC",             /* Shifted delete character key */
"kDL",             /* Shifted delete line key */
"kslt",            /* Select key */
"kEND",            /* Shifted end key */
"kEOL",            /* Shifted clear to end of line key */
"kEXT",            /* Shifted exit key */
"kFND",            /* Shifted find key */
"kHLP",            /* Shifted help key */
"kHOM",            /* Shifted home key */
"kIC",             /* Shifted insert character key */
"kLFT",            /* Shifted left arrow key */
"kMSG",            /* Shifted message key */
"kMOV",            /* Shifted move key */
"kNXT",            /* Shifted next key */
"kOPT",            /* Shifted options key */
"kPRV",            /* Shifted previous key */
"kPRT",            /* Shifted print key */
"kRDO",            /* Shifted redo key */
"kRPL",            /* Shifted replace key */
"kRIT",            /* Shifted right arrow key */
"kRES",            /* Shifted resume key */
"kSAV",            /* Shifted save key */
"kSPD",            /* Shifted suspend key */
"kUND",            /* Shifted undo key */
"rfi",             /* Send next input char */
"kf11",            /* Function key F11 */
"kf12",            /* Function key F12 */
"kf13",            /* Function key F13 */
"kf14",            /* Function key F14 */
"kf15",            /* Function key F15 */
"kf16",            /* Function key F16 */
"kf17",            /* Function key F17 */
"kf18",            /* Function key F18 */
"kf19",            /* Function key F19 */
"kf20",            /* Function key F20 */
"kf21",            /* Function key F21 */
"kf22",            /* Function key F22 */
"kf23",            /* Function key F23 */
"kf24",            /* Function key F24 */
"kf25",            /* Function key F25 */
"kf26",            /* Function key F26 */
"kf27",            /* Function key F27 */
"kf28",            /* Function key F28 */
"kf29",            /* Function key F29 */
"kf30",            /* Function key F30 */
"kf31",            /* Function key F31 */
"kf32",            /* Function key F32 */
"kf33",            /* Function key F33 */
"kf34",            /* Function key F34 */
"kf35",            /* Function key F35 */
"kf36",            /* Function key F36 */
"kf37",            /* Function key F37 */
"kf38",            /* Function key F38 */
"kf39",            /* Function key F39 */
"kf40",            /* Function key F40 */
"kf41",            /* Function key F41 */
"kf42",            /* Function key F42 */
"kf43",            /* Function key F43 */
"kf44",            /* Function key F44 */
"kf45",            /* Function key F45 */
"kf46",            /* Function key F46 */
"kf47",            /* Function key F47 */
"kf48",            /* Function key F48 */
"kf49",            /* Function key F49 */
"kf50",            /* Function key F50 */
"kf51",            /* Function key F51 */
"kf52",            /* Function key F52 */
"kf53",            /* Function key F53 */
"kf54",            /* Function key F54 */
"kf55",            /* Function key F55 */
"kf56",            /* Function key F56 */
"kf57",            /* Function key F57 */
"kf58",            /* Function key F58 */
"kf59",            /* Function key F59 */
"kf60",            /* Function key F60 */
"kf61",            /* Function key F61 */
"kf62",            /* Function key F62 */
"kf63",            /* Function key F63 */
"el1",             /* Clear to beginning of line */
"mgc",             /* Clear right and left soft margins */
"smgl",            /* Set left soft margin at current column */
"smgr",            /* Set right soft margin at current column */
"fln",             /* Label format */
"sclk",            /* Set clock, #1 hours #2 minutes #3 seconds */
"dclk",            /* Display clock */
"rmclk",           /* Remove clock */
"cwin",            /* Define window #1 from #2,#3 to #4,#5 */
"wingo",           /* Goto window #1 */
"hup",             /* Hang up phone */
"dial",            /* Dial number #1 */
"qdial",           /* Dial number #1 without checking */
"tone",            /* Select touch tone dialing */
"pulse",           /* Select pulse dialing */
"hook",            /* Flash switch hook */
"pause",           /* Pause for 2 - 3 seconds */
"wait",            /* Wait for dial tone */
"u0",              /* User function 0 */
"u1",              /* User function 1 */
"u2",              /* User function 2 */
"u3",              /* User function 3 */
"u4",              /* User function 4 */
"u5",              /* User function 5 */
"u6",              /* User function 6 */
"u7",              /* User function 7 */
"u8",              /* User function 8 */
"u9",              /* User function 9 */
"op",              /* Set default pair to its original value */
"oc",              /* Set all colour pairs to the original ones */
"initc",           /* Initialise colour #1 to (#2,#3,#4) */
"initp",           /* Initialise colour pair #1 to fg=(#2,#3,#4), bg=(#5,#6,#7) */
"scp",             /* Set current colour pair to #1 */
"setf",            /* Set foreground colour #1 */
"setb",            /* Set background colour #1 */
"cpi",             /* Change number of characters per inch to #1 */
"lpi",             /* Change number of lines per inch to #1 */
"chr",             /* Change horizontal resolution to #1 */
"cvr",             /* Change vertical resolution to #1 */
"defc",            /* Define a character #1, #2 dots wide, descender #3 */
"swidm",           /* Enter double-wide mode */
"sdrfq",           /* Enter draft-quality mode */
"sitm",            /* Enter italic mode */
"slm",             /* Start leftward carriage motion */
"smicm",           /* Start micro-motion mode */
"snlq",            /* Enter NLQ mode */
"snrmq",           /* Enter normal-quality mode */
"sshm",            /* Enter shadow-print mode */
"ssubm",           /* Enter subscript mode */
"ssupm",           /* Enter superscript mode */
"sum",             /* Start upward carriage motion */
"rwidm",           /* End double-wide mode */
"ritm",            /* End italic mode */
"rlm",             /* End left-motion mode */
"rmicm",           /* End micro-motion mode */
"rshm",            /* End shadow-print mode */
"rsubm",           /* End subscript mode */
"rsupm",           /* End superscript mode */
"rum",             /* End reverse character motion */
"mhpa",            /* Like column_address in micro mode */
"mcud1",           /* Like cursor_down in micro mode */
"mcub1",           /* Like cursor_left in micro mode */
"mcuf1",           /* Like cursor_right in micro mode */
"mvpa",            /* Like row_address #1 in micro mode */
"mcuu1",           /* Like cursor_up in micro mode */
"porder",          /* Match software bits to print-head pins */
"mcud",            /* Like parm_down_cursor in micro mode */
"mcub",            /* Like parm_left_cursor in micro mode */
"mcuf",            /* Like parm_right_cursor in micro mode */
"mcuu",            /* Like parm_up_cursor in micro mode */
"scs",             /* Select character set, #1 */
"smgb",            /* Set bottom margin at current line */
"smgbp",           /* Set bottom margin at line #1 or (if smgtp is not given) #2 lines from bottom */
"smglp",           /* Set left (right) margin at column #1 */
"smgrp",           /* Set right margin at column #1 */
"smgt",            /* Set top margin at current line */
"smgtp",           /* Set top (bottom) margin at row #1 */
"sbim",            /* Start printing bit image graphics */
"scsd",            /* Start character set definition #1, with #2 characters in the set */
"rbim",            /* Stop printing bit image graphics */
"rcsd",            /* End definition of character set #1 */
"subcs",           /* List of subscriptable characters */
"supcs",           /* List of superscriptable characters */
"docr",            /* Printing any of these characters causes CR */
"zerom",           /* No motion for subsequent character */
"csnm",            /* Produce #1'th item from list of character set names */
"kmous",           /* Mouse event has occurred */
"minfo",           /* Mouse status information */
"reqmp",           /* Request mouse position */
"getm",            /* Curses should get button events, parameter #1 not documented. */
"setaf",           /* Set foreground color to #1, using ANSI escape */
"setab",           /* Set background color to #1, using ANSI escape */
"pfxl",            /* Program function key #1 to type string #2 and show string #3 */
"devt",            /* Indicate language/codeset support */
"csin",            /* Init sequence for multiple codesets */
"s0ds",            /* Shift to code set 0 (EUC set 0, ASCII) */
"s1ds",            /* Shift to code set 1 */
"s2ds",            /* Shift to code set 2 */
"s3ds",            /* Shift to code set 3 */
"smglr",           /* Set both left and right margins to #1, #2.  (ML is not in BSD termcap). */
"smgtb",           /* Sets both top and bottom margins to #1, #2 */
"birep",           /* Repeat bit image cell #1 #2 times */
"binel",           /* Move to next row of the bit image */
"bicr",            /* Move to beginning of same row */
"colornm",         /* Give name for color #1 */
"defbi",           /* Define rectangualar bit image region */
"endbi",           /* End a bit-image region */
"setcolor",        /* Change to ribbon color #1 */
"slines",          /* Set page length to #1 lines */
"dispc",           /* Display PC character #1 */
"smpch",           /* Enter PC character display mode */
"rmpch",           /* Exit PC character display mode */
"smsc",            /* Enter PC scancode mode */
"rmsc",            /* Exit PC scancode mode */
"pctrm",           /* PC terminal options */
"scesc",           /* Escape for scancode emulation */
"scesa",           /* Alternate escape for scancode emulation */
/* QM extensions start here */
"slt",             /* Set line truncate mode */
"rlt",             /* Reset line truncate mode */
"sreg",            /* Save screen region */
"rreg",            /* Restore screen region */
"dreg",            /* Delete saved screen region */
"ku0",             /* User key 0 */
"ku1",             /* User key 1 */
"ku2",             /* User key 2 */
"ku3",             /* User key 3 */
"ku4",             /* User key 4 */
"ku5",             /* User key 5 */
"ku6",             /* User key 6 */
"ku7",             /* User key 7 */
"ku8",             /* User key 8 */
"ku9",             /* User key 9 */
"colourmap"};      /* Colour translation map */

#define NumStrNames (sizeof(strnames) / sizeof(char *))

/* END-CODE */
