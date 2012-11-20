      * KEYS.H
      * Keys for BASIC functions
      * Copyright (c) 2004, Ladybridge Systems, All Rights Reserved
      
      $list off
      
      * 30Mar09 gwb Added missing defines, SKT$STREAM, SKT$DGRAM, SKT$TCP,
      *             SKT$UDP, SKT$ICMP
      *
      * System limits
      $define MAX.ID.LEN            63
      
      * FILEINFO() function keys
      $define FL$OPEN           0  ;* Check if variable is an open file
      $define FL$VOCNAME        1  ;* Get VOC name by which file was opened
      $define FL$PATH           2  ;* Get pathname of file
      $define FL$TYPE           3  ;* Return file type
      $define FL$TYPE.SH        1  ;* SH file
      $define FL$TYPE.DH        3  ;* DH file
      $define FL$TYPE.DIR       4  ;* Directory file
      $define FL$TYPE.SEQ       5  ;* Sequential file
      $define FL$TYPE.VFS       6  ;* Virtual file system
      $define FL$MODULUS        5  ;* Get file modulus
      $define FL$MINMOD         6  ;* Get mimimum modulus value for file
      $define FL$GRPSIZE        7  ;* Get group size
      $define FL$LARGEREC       8  ;* Get large record size
      $define FL$MERGE          9  ;* Get merge load percentage
      $define FL$SPLIT         10  ;* Get split load percentage
      $define FL$LOAD          11  ;* Get current load percentage
      $define FL$AK            13  ;* File has AK indices?
      $define FL$LINE          14  ;* Get number of next line to read or write
      $define FL$LOADBYTES   1000  ;* Get current load bytes
      $define FL$READONLY    1001  ;* Read only file?
      $define FL$TRIGGER     1002  ;* Get trigger function name
      $define FL$PHYSBYTES   1003  ;* File size, excluding indices
      $define FL$VERSION     1004  ;* File version
      $define FL$STATS.QUERY 1005  ;* File statistics enabled?
      $define FL$SEQPOS      1006  ;* File position for sequential file
      $define FL$TRG.MODES   1007  ;* Trigger modes
      $define FL$TRG.PRE.WRITE    1
      $define FL$TRG.PRE.DELETE   2
      $define FL$TRG.POST.WRITE   4
      $define FL$TRG.POST.DELETE  8
      $define FL$TRG.READ        16
      $define FL$TRG.PRE.CLEAR   32
      $define FL$TRG.POST.CLEAR  64
      $define FL$NOCASE      1008  ;* File uses case insensitive ids?
      $define FL$FILENO      1009  ;* Get internal file number
      $define FL$JNL.FNO     1010  ;* Get journalling file no, zero if none
      $define FL$AKPATH      1011  ;* Get AK subfile relocation pathname
      $define FL$ID          1012  ;* Id of last record read */
      $define FL$STATUS      1013  ;* STATUS dynamic array
      $define FL$MARK.MAPPING 1014 ;* Mark mapping enabled?
      $define FL$RECORD.COUNT 1015 ;* Record count (approximate)
      $define FL$PRI.BYTES   1016  ;* Physical size of primary subfile
      $define FL$OVF.BYTES   1017  ;* Physical size of overflow subfile
      $define FL$NO.RESIZE   1018  ;* Resizing inhibited?
      $define FL$UPDATE      1019  ;* File update counter
      $define FL$ENCRYPTED   1020  ;* File uses encryption
      
      * PRINTER.SETTING() function keys
      * This function is obsolete - Use GETPU() and SETPU()
      $define LPTR$WIDTH               1
      $define LPTR$LINES               2
      $define LPTR$TOP.MARGIN          3
      $define LPTR$BOTTOM.MARGIN       4
      $define LPTR$LEFT.MARGIN         5
      $define LPTR$DATA.LINES          6
      $define LPTR$HEADING.LINES       7
      $define LPTR$FOOTING.LINES       8
      $define LPTR$MODE                9
      $define LPTR$NAME               10
      $define LPTR$FLAGS              11
      * Also defined below as PU$...
      $define LPTR$FLG.PS         0x0002   ;* Emulated PostScript printer
      $define LPTR$FLG.NFMT       0x0004   ;* Suppress all formatting
      $define LPTR$FLG.NEXT       0x0008   ;* SETPTR NEXT options
      $define LPTR$FLG.RAW        0x0010   ;* Use raw mode APIs (Windows)
      $define LPTR$FLG.KEEP.OPEN  0x0020   ;* Do not close on PRINTER CLOSE
      $define LPTR$FLG.LAND       0x0040   ;* Landscape mode
      $define LPTR$LINE.NO            12
      $define LPTR$PAGE.NO            13
      $define LPTR$LINES.LEFT         14
      $define LPTR$COPIES             15
      $define LPTR$BANNER             16
      $define LPTR$HIGH.PRINT.UNIT   255     ;* Highest available print unit number
      
      * RECORDLOCKED() function return values
      $define LOCK$OTHER.FILELOCK  -3
      $define LOCK$OTHER.READU     -2
      $define LOCK$OTHER.READL     -1
      $define LOCK$NO.LOCK          0
      $define LOCK$MY.READL         1
      $define LOCK$MY.READU         2
      $define LOCK$MY.FILELOCK      3
      
      * SELECTINFO() function keys
      $define SL$ACTIVE       1  ;* Check if select list is active
      $define IK$SLACTIVE     1  ;* Check if select list is active
      $define SL$COUNT        3  ;* Fetch number of entries in select list
      $define IK$SLCOUNT      3  ;* Fetch number of entries in select list
      
      * @functions
      $define IT$CS          -1  ;* Clear screen
      $define IT$CAH         -2  ;* Cursor home
      $define IT$CLEOS       -3  ;* Clear to end of screen
      $define IT$CLEOL       -4  ;* Clear to end of line
      $define IT$SBLINK      -5  ;* Start flashing text
      $define IT$EBLINK      -6  ;* End flashing text
      $define IT$CUB         -9  ;* Backspace one char (or count given in arg 2)
      $define IT$CUU        -10  ;* Cursor up one line (or count given in arg 2)
      $define IT$SHALF      -11  ;* Start half brightness
      $define IT$EHALF      -12  ;* End half brightness
      $define IT$SREV       -13  ;* Start reverse video
      $define IT$EREV       -14  ;* End reverse video
      $define IT$SUL        -15  ;* Start underline
      $define IT$EUL        -16  ;* End underline
      $define IT$IL         -17  ;* Insert line (or as count in arg 2)
      $define IT$DL         -18  ;* Delete line (or as count in arg 2)
      $define IT$ICH        -19  ;* Insert character (or as count in arg 2)
      $define IT$DCH        -22  ;* Delete character (or as count in arg 2)
      $define IT$AUXON      -23  ;* Auxillary print mode on
      $define IT$AUXOFF     -24  ;* Auxillary print mode off
      $define IT$E80        -29  ;* Set 80 column mode
      $define IT$E132       -30  ;* Set 132 column mode
      $define IT$RIC        -31  ;* Reset inhibit cursor
      $define IT$SIC        -32  ;* Set inhibit cursor
      $define IT$CUD        -33  ;* Cursor down one line (or as count in arg 2)
      $define IT$CUF        -34  ;* Cursor right one column (or as count in arg 2)
      $define IT$FGC        -37  ;* Set foreground colour
      $define IT$FCOLOR     -37  ;* Set foreground colour
      $define IT$BGC        -38  ;* Set background colour
      $define IT$BCOLOR     -38  ;* Set background colour
      $define IT$SLT        -54  ;* Set line truncate
      $define IT$RLT        -55  ;* Reset line truncate
      $define IT$SBOLD      -58  ;* Start bold
      $define IT$EBOLD      -59  ;* End bold
      $define IT$ACMD       -108 ;* Asynchronous command prefix
      $define IT$SCMD       -109 ;* Synchronous command prefix
      $define IT$STYLUS     -250 ;* Enable/disable PDA stylus
      $define IT$KEYS       -251 ;* Enable/disable PDA keyboard display
      
      * Colours for IT$FGC and IT$BGC
      $define IT$BLACK                 0
      $define IT$BLUE                  1
      $define IT$GREEN                 2
      $define IT$CYAN                  3
      $define IT$RED                   4
      $define IT$MAGENTA               5
      $define IT$BROWN                 6
      $define IT$WHITE                 7
      $define IT$GREY                  8
      $define IT$BRIGHT.BLUE           9
      $define IT$BRIGHT.GREEN         10
      $define IT$BRIGHT.CYAN          11
      $define IT$BRIGHT.RED           12
      $define IT$BRIGHT.MAGENTA       13
      $define IT$YELLOW               14
      $define IT$BRIGHT.WHITE         15
      
      * GETPU(), SETPU, !GETPU() and !SETPU() mode keys
      $define PU$DEFINED               0
      $define PU$MODE                  1
      $define PU$WIDTH                 2
      $define PU$LENGTH                3
      $define PU$TOPMARGIN             4
      $define PU$BOTMARGIN             5
      $define PU$LEFTMARGIN            6
      $define PU$SPOOLFLAGS            7
      * Older flags are also defined above as LPTR$...
      $define PU$FLG.PS             0x00000002   ;* Emulated PostScript printer
      $define PU$FLG.NFMT           0x00000004   ;* Suppress all formatting
      $define PU$FLG.NEXT           0x00000008   ;* SETPTR NEXT options
      $define PU$FLG.RAW            0x00000010   ;* Use raw mode APIs (Windows)
      $define PU$FLG.KEEP.OPEN      0x00000020   ;* Do not close on PRINTER CLOSE
      $define PU$FLG.LAND           0x00000040   ;* Landscape mode
      $define PU$FLG.PCL            0x00002000   ;* PCL printer
      $define PU$FLG.DUPLEX.LONG    0x00004000   ;* Duplex, long edge binding
      $define PU$FLG.DUPLEX.SHORT   0x00008000   ;* Duplex, short edge binding
      $define PU$FORM                  9
      $define PU$BANNER               10
      $define PU$LOCATION             11
      $define PU$COPIES               12
      $define PU$PAGENUMBER           15
      $define PU$LINESLEFT          1002
      $define PU$HEADERLINES        1003
      $define PU$FOOTERLINES        1004
      $define PU$DATALINES          1005
      $define PU$OPTIONS            1006
      $define PU$PREFIX             1007
      $define PU$SPOOLER            1008
      $define PU$OVERLAY            1009
      $define PU$CPI                1010
      $define PU$PAPER.SIZE         1011
      $define PU$LPI                1012
      $define PU$WEIGHT             1013
      $define PU$SYMBOL.SET         1014
      $define PU$STYLE              1015
      $define PU$NEWLINE            1016
      $define PU$PRINTER.NAME       1017
      $define PU$FILE.NAME          1018
      $define PU$LINENO             2000
      
      * SETNLS key values
      
      $define NLS$CURRENCY             1
      $define NLS$THOUSANDS            2
      $define NLS$DECIMAL              3
      
      * GET.PORT.PARAMS(), SET.PORT.PARAMS() dynamic array fields
      $define PP$PORT                  1    ;* Port number
      $define PP$BAUD.RATE             2    ;* Baud rate
      $define PP$CHECK.PARITY          3    ;* 1 = enable, 0 = disable
      $define PP$PARITY.MODE           4    ;* 0=None, 1=Odd, 2=Even, 3=Mark, 4=Space */
      $define PP$BYTE.SIZE             5    :* Bits per byte
      $define PP$STOP.BITS             6    ;* 0=One, 1= One and a half, 2 = Two
      
      * Socket interface tokens
      $define SKT$BLOCKING             1    ;* Blocking mode
      $define SKT$NON.BLOCKING         2    ;* Non-blocking mode
      * Socket Types
      $define SKT$STREAM               0x00000000  
      $define SKT$DGRM                 0x00000100
      * Protocols
      $define SKT$TCP                  0x00000000
      $define SKT$UDP                  0x00010000
      $define SKT$ICMP                 0x00020000
      
      * SOCKET.INFO() and SET.SOCKET.MODE() keys
      $define SKT$INFO.OPEN            0    ;* Is this a socket variable?
      $define SKT$INFO.TYPE            1    ;* What sort of socket is this?
      $define SKT$INFO.TYPE.SERVER     1    ;* From CREATE.SERVER.SOCKET()
      $define SKT$INFO.TYPE.INCOMING   2    ;* From ACCEPT.SOCKET.CONNECTION()
      $define SKT$INFO.TYPE.OUTGOING   3    ;* From OPEN.SOCKET()
      $define SKT$INFO.PORT            2    ;* Port number
      $define SKT$INFO.IP.ADDR         3    ;* IP address
      $define SKT$INFO.BLOCKING        4    ;* Blocking mode?
      $define SKT$INFO.NO.DELAY        5    ;* Nagle algorithm disabled?
      $define SKT$INFO.KEEP.ALIVE      6    ;* Keep alive enabled?
      $define SKT$INFO.FAMILY          7    ;* Socket address family
      $define SKT$INFO.FAMILY.IPV4     1    ;* Socket address family IPV4
      $define SKT$INFO.FAMILY.IPV6     2    ;* Socket address family IPV6     
      
      * OBJINFO() keys
      $define OI$ISOBJ                 0    ;* Is descriptor an object
      $define OI$CLASS                 1    ;* Class name
      
      * ACCOUNTS file
      * Id = account name (forced to uppercase)
      $define ACC$PATH         1     ;* Account pathname
      $define ACC$DESCR        2     ;* Account description
      deffun parse.pathname.tokens(path) calling "!PATHTKN"
      
