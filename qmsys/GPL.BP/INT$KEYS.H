      * INT$KEYS.H
      * Keys for internal use only
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
      *  START-HISTORY:
      * 29 Mar 09 gwb Added IN$NO.ECHO.DATA
      *
      * 17 Jan 07  2.4-19 Added OPT.NO.DATE.WRAPPING
      * 15 Mar 06  2.3-8 Added OPT.INHERIT.
      * 01 Sep 05  2.2-9 Added EVT$MESSAGE.
      * 09 Aug 05  2.2-7 Added OPT.ED.NO.QUERY.FD
      * 14 Jan 05  2.1-1 Added OPT.SUPPRESS.ABORT.MSG
      * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
      *  END-HISTORY
      *
      *  START-DESCRIPTION:
      *
      *  END-DESCRIPTION
      *
      *  START-CODE

      $define MAX.CALL.NAME.LEN     63
      $define MAX.TRIGGER.NAME.LEN  32
      $define MAX.USERNAME.LEN      32
      $define MAX.PATHNAME.LEN     255
      $define MAX.ACCOUNT.NAME.LEN  32

      * Screen parameters, also defined in csrc tio.h
      $define MIN.WIDTH           20
      $define DEFAULT.WIDTH       80
      $define MAX.WIDTH        32767
      $define MIN.DEPTH           10
      $define DEFAULT.DEPTH       24
      $define MAX.DEPTH        32767


      $define DH.VERSION             2  ;* Current file version

      * FLAGS argument to $INPUT
      $define IN$FIELD.MODE      0x00000001  ;* INPUT.FIELD
      $define IN$UPCASE          0x00000002  ;* Force input to uppercase
      $define IN$DISPLAY         0x00000010  ;* Display existing field value
      $define IN$NOCRLF          0x00000020  ;* Suppress CR/LF (underscore used)
      $define IN$NOLF            0x00000040  ;* Suppress LF after CR (colon used)
      $define IN$NOT.DATA        0x00000080  ;* Do not use DATA queue
      $define IN$MASK            0x00000100  ;* INPUT @ with format mask
      $define IN$ERR.DISPLAYED   0x00000200  ;* Error is currently displayed (see op_tio.c)
      $define IN$PASSWORD        0x00000400  ;* Do password style masking of input
      $define IN$TIMEOUT         0x00000800  ;* Timeout present on e-stack
      $define IN$THENELSE        0x00001000  ;* THEN/ELSE clause(s) present
      $define IN$OVERLAY         0x00002000  ;* Start in overlay mode
      $define IN$EDIT            0x00004000  ;* Start in edit mode, don't clear on data
      $define IN$APPEND          0x00008000  ;* Append (used with IN$EDIT)
      $define IN$PANNING         0x00010000  ;* Panning input field
      $define IN$NO.ECHO.DATA    0x00020000  ;* Supress echo on input from DATA statements.
      $define IN$NO.CASE.INVERT  0x00040000  ;* Disable PTERM CASE inversion for the input session

      * KERNEL() function action keys
      $define K$INTERNAL         0       ;* Set/clear internal mode
      $define K$SECURE           1       ;* Secure system (login required)?
      $define K$LPTRHIGH         2       ;* @LPTRHIGH
      $define K$LPTRWIDE         3       ;* @LPTRWIDE
      $define K$PAGINATE         4       ;* Pagination active?
      $define K$FLAGS            5       ;* Check/return header flags
      $define K$DATE.FORMAT      6       ;* European date format?
      $define K$CRTHIGH          7       ;* @CRTHIGH
      $define K$CRTWIDE          8       ;* @CRTWIDE
      $define K$SET.DATE         9       ;* Set current date
      $define K$IS.PHANTOM      10       ;* Phantom process?
      $define K$TERM.TYPE       11       ;* Terminal type name
      $define K$USERNAME        12       ;* User name
      $define K$DATE.CONV       13       ;* Set default date conversion
      $define K$PPID            14       ;* Get parent process id
      $define K$USERS           15       ;* User id/ip_addr list
      $define K$USERS.UID     1       ;* User id (value position in field)
      $define K$USERS.PID     2       ;* Process id
      $define K$USERS.IP      3       ;* IP address
      $define K$USERS.FLAGS   4       ;* Process flags
      $define K$USERS.PUID    5       ;* Parent process id for phantom
      $define K$USERS.UNAME   6       ;* User login name
      $define K$USERS.DEV     7       ;* TTY name
      $define K$USERS.LOGIN   8       ;* Login time
      $define K$CONFIG.DATA     16       ;* Configuration data
      $define K$LICENCE         17       ;* Validate licence data
      $define K$INIPATH         18       ;* Get ini file pathname
      $define K$MONITOR         19       ;* Report/Set/Clear pc monitor mode
      $define K$FORCED.ACCOUNT  20       ;* Force entry to named account unless set in $LOGINS
      $define K$QMNET           21       ;* Get/set QMNet status flag
      $define K$CPROC.LEVEL     22       ;* Get/set command processor level
      $define K$HELP            23       ;* Invoke help system
      $define K$SUPPRESS.COMO   24       ;* Suppress como file output
      $define K$IS.QMVBSRVR     25       ;* QMVbSrvr process?
      $define K$ADMINISTRATOR   26       ;* Get/set administrator rights
      $define K$FILESTATS       27       ;* Global file statistics
      $define K$TTY             28       ;* Get terminal device name
      $define K$GET.OPTIONS     29       ;* Get option flags
      $define K$SET.OPTIONS     30       ;* Set option flags
      $define K$PRIVATE.CATALOGUE 31     ;* Set private catalogue directory
      $define K$SYS.ID          32       ;* Get system id
      $define K$CLEANUP         33       ;* Remove defunct users
      $define K$OBJKEY          34       ;* Object action key
      $define K$COMMAND.OPTIONS 35       ;* QM command line option flag bit positions
      $define CMD.APPLY.LICENCE  0x0001  ;* -L Apply new licnce
      $define CMD.QUERY.ACCOUNT  0x0002  ;* -A Query account on entry
      $define CMD.INSTALL        0x0004  ;* -I Completing install
      $define CMD.QUIET          0x0008  ;* -QUIET - Suppress copyright/licence
      $define CMD.PERSONAL       0x0010  ;* -IP Completing personal install
      $define CMD.STDOUT         0x0020  ;* -STDOUT
      $define CMD.FLASH          0x0040  ;* -F option
      $define K$CASE.SENSITIVE  36       ;* REMOVE.TOKEN() case sensitive in TKN.NAME
      $define K$PACKAGE.DATA    37       ;* Get package licence data
      $define K$SET.LANGUAGE    38       ;* Set language for message system
      $define K$HSM             39       ;* Hot spot monitor
      $define K$COLLATION       40       ;* Set collation map
      $define K$GET.QMNET.CONNECTIONS  41 ;* Get details of open QMNet connections
      $define K$JNL             42       ;* Journalling functions
      $define K$INVALIDATE.OBJECT  43    ;* Invalidate object cache
      $define K$MESSAGE         44       ;* Enable/disable message reception
      $define K$SET.EXIT.CAUSE  45       ;* Set k_exit_cause value
      $define K$COLLATION.NAME  46       ;* Set collation name
      $define K$AK.COLLATION    47       ;* Select/deselect AK collation map
      $define K$EXIT.STATUS     48       ;* Set/retrieve exit status
      $define K$CASE.MAP        49       ;* Set upper/lower case pairing
      $define K$AUTOLOGOUT      50       ;* Set/retrieve autologout period
      $define K$MAP.DIR.IDS     51       ;* Enable/disable dir file id mapping
      $define K$IN.GROUP        52       ;* Is user in named group?
      $define K$BREAK.HANDLER   53       ;* Define break handler
      $define K$SETUID          54       ;* NIX authorisation
      $define K$SETGID          55       ;* NIX authorisation
      $define K$RUNEXE          56       ;* Run executable

      * PTERM() action keys
      $define PT$BREAK           1       ;* Trap break character as break?
      $define PT$INVERT          2       ;* Case inversion
      $define PT$BRKCH           3       ;* Set break character
      $define PT$ONEWLINE        4       ;* Set output newline mode
      $define PT$INEWLINE        5       ;* Set input newline mode
      $define PT$BINARY.IN       6       ;* Telnet binary mode, client -> server
      $define PT$BINARY.OUT      7       ;* Telnet binary mode, server -> client
      $define PT$TELNET          8       ;* Recognise TN_IAC on input?

      * OSPATH() action keys
      $define OS$PATHNAME        0       ;* Test if valid filename
      $define OS$FILENAME        1       ;* Test if valid pathname
      $define OS$EXISTS          2       ;* Test if file exists
      $define OS$UNIQUE          3       ;* Make a unique file name
      $define OS$FULLPATH        4       ;* Return full DOS file name
      $define OS$DELETE          5       ;* Delete file
      $define OS$CWD             6       ;* Get current working directory
      $define OS$DTM             7       ;* Get date/time modified
      $define OS$FLUSH.CACHE     8       ;* Flush DH file cache
      $define OS$CD              9       ;* Change working directory
      $define OS$MAPPED.NAME    10       ;* Map a directory file name
      #define OS$OPEN           11       ;* Check if file is open by pathanme
      $define OS$DIR            12       ;* Return content of directory
      $define OS$MKDIR          13       ;* Create a directory
      $define OS$MKPATH         14       ;* Create a directory path

      * DH library errors
      $define DHE.RECORD.NOT.FOUND     401       ;* Record not found (read, select, etc)

      * @ABORT.CODE
      $define K$LOGOUT           3               ;* LOGOUT of phantom process

      * Kernel k_exit_cause values (from csrc kernel.h)
      $define K$EXIT.ABORT       3

      * USER_ENTRY flag word from kernel(K$USERS,n)
      $define USR$PHANTOM      0x0001    ;* Process is a phantom
      $define USR$LOGOUT       0x0002    ;* Logout in progress
      $define USR$QMVBSRVR     0x0004    ;* QMVbSrvr process
      $define USR$ADMIN        0x0008    ;* Administrator privileges
      $define USR$QMNET        0x0010    ;* QMNet process (USR$QMVBSRVR also set)
      $define USR$CHGPHANT     0x0020    ;* Chargeable phantom
      $define USR$MSG.OFF      0x0040    ;* Messge reception disabled

      * USER_ENTRY events word
      $define EVT$LOGOUT       0x0001    ;* Forced logout - immediate termination
      $define EVT$STATUS       0x0002    ;* Return status dump
      $define EVT$UNLOAD       0x0004    ;* Unload inactive cached object code
      $define EVT$BREAK        0x0008    ;* Clear break inhibit counter
      $define EVT$HSM.ON       0x0010    ;* Enable HSM
      $define EVT$HSM.DUMP     0x0020    ;* Return HSM data
      $define EVT$PDUMP        0x0040    ;* Force process dump
      $define EVT$FLUSH.CACHE  0x0080    ;* Flush DH cache
      $define EVT$JNL.SWITCH   0x0100    ;* Switch journal file
      $define EVT$TERMINATE    0x0200    ;* Forced logout - graceful termination
      $define EVT$MESSAGE      0x0400    ;* Send immediate message

      * Opcode prefix flags as defined in kernel.h.  These are used by the READV
      * recursive function (See op_dio3.c).                                       */
      $define P$LOCKED      2
      $define P$LLOCK       4
      $define P$ULOCK       8
      $define P$READONLY   16
      $define P$PICKREAD   32

      * BTree flag keys
      $define BT.RIGHT.ALIGNED 1
      $define BT.DESCENDING    2
      $define BT.UNIQUE        4
      $define BT.DATA          8

      * Undocumented FILEINFO() keys
      $define FL$EXCLUSIVE   10000    ;* Set exclusive file access
      $define FL$FLAGS       10001    ;* Fetch file flags
      * LS 16 bits are from file header...
      $define FL$FLAGS.NORESIZE   0x00000008   ;* Case insensitive ids
      $define FL$FLAGS.NOCASE     0x00000010   ;* Case insensitive ids
      $define FL$FLAGS.TRUSTED    0x00000020   ;* Access requires trusted program
      * MS 16 bits are internal...
      $define FL$FLAGS.TRIGGER    0x00010000   ;* File has trigger
      $define FL$FLAGS.AK         0x00020000   ;* File has AK indices
      $define FL$FLAGS.RDONLY     0x00040000   ;* Read only
      $define FL$FLAGS.UPDATED    0x00080000   ;* Written since opened
      $define FL$STATS.ON    10002
      $define FL$STATS.OFF   10003
      $define FL$STATS       10004
      $define FL$STATS.RESET           1
      $define FL$STATS.OPENS           2
      $define FL$STATS.READS           3
      $define FL$STATS.WRITES          4
      $define FL$STATS.DELETES         5
      $define FL$STATS.CLEARS          6
      $define FL$STATS.SELECTS         7
      $define FL$STATS.SPLITS          8
      $define FL$STATS.MERGES          9
      $define FL$STATS.AKREADS        10
      $define FL$STATS.AKWRITES       11
      $define FL$STATS.AKDELETES      12
      $define FL$SETRDONLY   10005           ;* Make file read only (cannot reverse)

      * $LOGINS file
      * Id = Uppercase user name or "Console". Name cannot start with $ sign.
      $define LGN$LAST.ACCOUNT     1    ;* Last account name
      $define LGN$DATE             2    ;* Date of last login
      $define LGN$TIME             3    ;* Time of last login
      $define LGN$PASSWORD         4    ;* Encrypted password
      $define LGN$FORCE.ACCOUNT    5    ;* Forced account name
      $define LGN$ADMIN            6    ;* Administrator?
      $define LGN$MIN              7    ;* Minimum password length
      $define LGN$OWNER            8    ;* Owner's name etc
      $define LGN$VALID.ACCOUNTS   9    ;* Valid accounts, blank = unrestricted
      $define LGN$BANNED.ACCOUNTS 10    ;* Banned accounts

      * ID = $SECURE
      *    F1 = Secure system? (boolean)


      * Variable types ++ALLTYPES++

      $define V$UNASSIGNED    0
      $define V$ADDR          1
      $define V$INTEGER       2
      $define V$FLOATNUM      3
      $define V$SUBR          4
      $define V$STRING        5
      $define V$FILE.REF      6
      $define V$ARRAY         7
      ;* These tokens are from csrc descr.h
      $define AH$PICK.STYLE  0x0001  ;* Pick style matrix
      $define AH$AUTO.DELETE 0x0002  ;* Self-deleting common block
      $define V$COMMON        8
      $define V$IMAGE         9
      $define V$BTREE        10
      $define V$SELLIST      11
      $define V$PMATRIX      12
      $define V$SOCK         13
      $define V$LOCALVARS    14
      $define V$OBJ          15
      $define V$OBJCD        16
      $define V$OBJCDX       17
      $define V$PERSISTENT   18

      * Option flags (see also csrc options.h)
      $define OPT.UNASS.WARNING        0 ;* Treat unassigned variable as warning
      $define OPT.QUERY.DEBUG          1 ;* QPROC debugging (restricted)
      $define OPT.PICK.WILDCARD        2 ;* QPROC supports Pick wildcards
      $define OPT.QUAL.DISPLAY         3 ;* QPROC supports qualified display clauses
      $define OPT.PICK.BREAKPOINT      4 ;* QPROC Pick placement of breakpoint text
      $define OPT.WITH.IMPLIES.OR      5 ;* QPROC Pick WITH ... (OR) WITH ...
      $define OPT.DUMP.ON.ERROR        6 ;* Dump process on error
      $define OPT.DIV.ZERO.WARNING     7 ;* Treat divide by zero as warning
      $define OPT.NON.NUMERIC.WARNING  8 ;* Treat non-numeric as warning
      $define OPT.ASSOC.UNASSOC.MV     9 ;* Associate unassociated MV fields
      $define OPT.PICK.BREAKPOINT.U   10 ;* QPROC Use Pick meaning of U breakpoint
      $define OPT.NO.USER.ABORTS      11 ;* Suppress user options to abort
      $define OPT.RUN.NO.PAGE         12 ;* RUN/DEBUG defaults to NO.PAGE
      $define OPT.SHOW.STACK.ON.ERROR 13 ;* Show stack on error
      $define OPT.CRDB.UPCASE         14 ;* Show CR/DB in OCONV MD in uppercase?
      $define OPT.AMPM.UPCASE         15 ;* Show AM/PM in OCONV MTH in uppercase?
      $define OPT.PICK.NULL           16 ;* Return null for null data in ML, MR and FMT()
      $define OPT.PICK.GRAND.TOTAL    17 ;* GRAND.TOTAL text on same line as values
      $define OPT.SUPPRESS.ABORT.MSG  18 ;* Suppress program details on abort
      $define OPT.DEBUG.REBIND.KEYS   19 ;* Rebind AccuTerm keys on entry to debugger
      $define OPT.ED.NO.QUERY.FD      20 ;* Don't ask for confirmation on FD command in ED
      $define OPT.NO.SEL.LIST.QUERY   21 ;* Don't query use of select list
      $define OPT.CHAIN.KEEP.COMMON   22 ;* Keep unnamed common on CHAIN
      $define OPT.SELECT.KEEP.CASE    23 ;* Keep case in Windows dir file select
      $define OPT.CREATE.FILE.CASE    24 ;* Keep operating system file name case in CREATE.FILE
      $define OPT.PICK.EXPLODE        25 ;* Pick style processing of single value item in BY.EXP
      $define OPT.INHERIT             26 ;* Phantom inherits options
      $define OPT.QUERY.NO.CASE       27 ;* Case insensitive query processor selection operators
      $define OPT.LOCKBEEP            28 ;* Beep on wait for record lock
      $define OPT.PICK.IMPLIED.EQ     29 ;* Implied EQ between field and literal in query
      $define OPT.QUERY.PRIORITY.AND  30 ;* AND has priority over OR operator in query processor
      $define OPT.NO.DATE.WRAPPING    31 ;* Don't roll overlarge day number into next month
      $define OPT.PROC.A              32 ;* Treat Proc A command as in D3

      * MKINDEX flag values (additive)
      $define MKI$CREATE               1 ;* Create indices
      $define MKI$BUILD                2 ;* Build indices
      $define MKI$NO.NULLS             4 ;* NO.NULLS (for create mode)
      $define MKI$NO.CASE              8 ;* NO.CASE (for create mode)

      * FCONTROL() keys
      $define FC$SET.JNL.FNO           1 ;* Set jnl_fno in file header
      $define FC$KILL.JNL              2 ;* Disable journalling on open file
      $define FC$SET.AKPATH            3 ;* Set akpath element of file header
      $define FC$NON.TXN               4 ;* Set open file as non-transactional
      $define FC$SPLIT.MERGE           5 ;* Force split/merge
      $define FC$NO.RESIZE             6 ;* Set DHF_NO_RESIZE flag



      * !!ATVAR!!
      $define READ.ONLY.AT.VARS 'ABORT.CODE,ANS,CRTHIGH,CRTWIDE,DATE,DAY,GID,HOSTNAME,IP.ADDR,LEVEL,LOGNAME,LPTRHIGH,LPTRWIDE,MONTH,PARASENTENCE,PATH,PIB,POB,QMSYS,SELECTED,SIB,SOB,TERM.TYPE,TIME,TTY,UID,USER,USERNO,USER.NO,WHO,YEAR,YEAR4'

      * END-CODE
