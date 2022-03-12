* ERR.H
* Error numbers and STATUS() values
* Copyright (c) 2006 Ladybridge Systems, All Rights Reserved
*
* START-HISTORY:
* 27 Mar 06  2.3-9 Added ER$TERMINATED.
* 23 Aug 04  1.5-11 Header added to existing module.
* END-HISTORY
*
* START-DESCRIPTION:
*
* END-DESCRIPTION
*
* START-CODE
*

* Commands that set negative error codes into @SYSTEM.RETURN.CODE use the
* arithmetic inverse of the values listed below.

* Errors tagged (os.error) have associated operating system error numbers
* that can be retrieved from OS.ERROR().  The !ERRTEXT() subroutine will
* automatically include this extended error information in the message.

* 0001 - 0999  General command processing errors
$define ER$ARGS           1    ;* Command arguments invalid or incomplete
$define ER$NCOMO          2    ;* Como file not active
$define ER$ICOMP          3    ;* I-type compilation error
$define ER$ACC.EXISTS     4    ;* Account name already in register
$define ER$NO.DIR         5    ;* Directory not found
$define ER$NOT.CREATED    6    ;* Unable to create directory
$define ER$STOPPED        7    ;* Processing terminated by user action
$define ER$INVA.PATH      8    ;* Invalid pathname
$define ER$NOT.CAT        9    ;* Item not in catalogue
$define ER$PROCESS       10    ;* Unable to start new process
$define ER$USER.EXISTS   11    ;* User name already in register
$define ER$UNSUPPORTED   12    ;* This operation is not supported on this platform
$define ER$TERMINFO      13    ;* No terminfo definition for this function
$define ER$NO.ACC        14    ;* Account name not in register
$define ER$TERMINATED    15    ;* Operation terminated by user

* 1000 - 1999  General errors

$define ER$PARAMS      1000    ;* Invalid parameters
$define ER$MEM         1001    ;* Cannot allocate memory
$define ER$LENGTH      1002    ;* Invalid length
$define ER$BAD.NAME    1003    ;* Bad name
$define ER$NOT.FOUND   1004    ;* Item not found
$define ER$IN.USE      1005    ;* Item is in use
$define ER$BAD.KEY     1006    ;* Bad action key
$define ER$PRT.UNIT    1007    ;* Bad print unit
$define ER$FAILED      1008    ;* Action failed (os.error)
$define ER$MODE        1009    ;* Bad mode setting
$define ER$TXN         1010    ;* Operation not allowed in a transaction
$define ER$TIMEOUT     1011    ;* Timeout
$define ER$LIMIT       1012    ;* Limit reached
$define ER$EXPIRED     1013    ;* Expired
$define ER$NO.CONFIG   1014    ;* Cannot find configuration file
$define ER$RDONLY.VAR  1015    ;* Variable is read-only
$define ER$NOT.PHANTOM 1016    ;* Not a phantom process
$define ER$CONNECTED   1017    ;* Device already connected
$define ER$INVA.ITYPE  1018    ;* Invalid I-type

* 20xx  Catalog management errors

$define ER$INVA.OBJ    2000    ;* Invalid object code
$define ER$CFNF        2001    ;* Catalogued function not found

* 21xx  Terminfo errors

$define ER$TI.NAME     2100    ;* Invalid terminal type name
$define ER$TI.NOENT    2101    ;* No terminfo entry for given name
$define ER$TI.MAGIC    2102    ;* Terminfo magic number check failed
$define ER$TI.INVHDR   2103    ;* Invalid terminfo header data
$define ER$TI.STRSZ    2104    ;* Invalid terminfo string length
$define ER$TI.STRMEM   2105    ;* Error allocating terminfo string memory
$define ER$TI.NAMEMEM  2106    ;* Error allocating terminfo name memory
$define ER$TI.BOOLMEM  2107    ;* Error allocating terminfo boolean memory
$define ER$TI.BOOLRD   2108    ;* Error reading terminfo booleans
$define ER$TI.NUMMEM   2109    ;* Error allocating terminfo numbers memory
$define ER$TI.NUMRD    2110    ;* Error reading terminfo numbers
$define ER$TI.STROMEM  2111    ;* Error allocating terminfo string offsets memory
$define ER$TI.STRORD   2112    ;* Error reading terminfo string offsets
$define ER$TI.STRTBL   2113    ;* Error reading terminfo string table

* 3000 - 3999  Generic file system errors
$define ER$IID         3000    ;* Illegal record id
$define ER$SFNF        3001    ;* Subfile not found
$define ER$NAM         3002    ;* Bad file name
$define ER$FNF         3003    ;* File not found
$define ER$NDIR        3004    ;* Not a directory file
$define ER$NDYN        3005    ;* Not a dynamic file
$define ER$RNF         3006    ;* Record not found
$define ER$NVR         3007    ;* No VOC record
$define ER$NPN         3008    ;* No pathname in VOC record
$define ER$VNF         3009    ;* VOC file record not F type
$define ER$IOE         3010    ;* I/O error (os.error)
$define ER$LCK         3011    ;* Lock is held by another process
$define ER$NLK         3012    ;* Lock is not held by this process
$define ER$NSEQ        3013    ;* Not a sequential file
$define ER$NEOF        3014    ;* Not at end of file
$define ER$SQRD        3015    ;* Sequential file record read before creation
$define ER$SQNC        3016    ;* Sequential record not created due to error (os.error)
$define ER$SQEX        3017    ;* Sequential record already exists (CREATE)
$define ER$RDONLY      3018    ;* Update to read only file
$define ER$AKNF        3019    ;* AK index not found
$define ER$INVAPATH    3020    ;* Invalid pathname
$define ER$EXCLUSIVE   3021    ;* Cannot gain exclusive access to file
$define ER$TRIGGER     3022    ;* Trigger function error
$define ER$NOLOCK      3023    ;* Attempt to write/delete record with no lock
$define ER$REMOTE      3024    ;* Open of remote file not allowed
$define ER$NOTNOW      3025    ;* Action cannot be performed now
$define ER$PORT        3026    ;* File is a port
$define ER$NPORT       3027    ;* File is not a port
$define ER$SQSEEK      3028    ;* Seek to invalid offset in sequential file
$define ER$SQREL       3029    ;* Invalid seek relto in sequential file
$define ER$EOF         3030    ;* End of file
$define ER$CNF         3031    ;* Multifile component not found
$define ER$MFC         3032    ;* Multifile reference with no component name
$define ER$PNF         3033    ;* Port not found (os.error)
$define ER$BAD.DICT    3034    ;* Bad dictionary entry
$define ER$PERM        3035    ;* Permissions error (os.errno)
$define ER$SEEK.ERROR  3036    ;* Seek error
$define ER$WRITE.ERROR 3037    ;* Write error (os.errno)
$define ER$VFS.NAME    3038    ;* Bad class name in VFS entry
$define ER$VFS.CLASS   3039    ;* VFS class routine not found
$define ER$VFS.NGLBL   3040    ;* VFS class routine is not globally catalogued
$define ER$ENCRYPTED   3041    ;* Access denied to encrypted file

* 4000 - 4999   QMClient errors
$define ER$SRVRMEM     4000    ;* Insufficient memory for packet buffer

* 5000 - 5999   Operating system related issues
$define ER$NO.DLL      5000    ;* DLL not found
$define ER$NO.API      5001    ;* API not found
$define ER$NO.TEMP     5002    ;* Cannot open temporary file (os.error)

* 6000 - 6999
$define ER$NO.EXIST    6031    ;* Item does not exist
$define ER$EXISTS      6032    ;* Item already exists
$define ER$NO.SPACE    6033    ;* No space for entry
$define ER$INVALID     6034    ;* Validation error

* 7000 - 7999   Network errors
$define ER$NETWORK     7000    ;* Networked file not allowed for this operation
$define ER$SERVER      7001    ;* Unknown server name
$define ER$WSA.ERR     7002    ;* Failed to start Window socket library (os.error)
$define ER$HOSTNAME    7003    ;* Invalid host name
$define ER$NOSOCKET    7004    ;* Cannot open socket (os.error)
$define ER$CONNECT     7005    ;* Cannot connect socket (os.error)
$define ER$RECV.ERR    7006    ;* Error receiving socket data (os.error)
$define ER$RESOLVE     7007    ;* Cannot resolve server name (os.error)
$define ER$LOGIN       7008    ;* Login rejected
$define ER$XREMOTE     7009    ;* Remote server disallowed access
$define ER$ACCOUNT     7010    ;* Cannot connect to account
$define ER$HOST.TABLE  7011    ;* Host table is full
$define ER$BIND        7012    ;* Error binding socket (os.error)
$define ER$SKT.CLOSED  7013    ;* Socket has been closed

* 8000 - 8999  DH errors
$define DHE$FILE.NOT.OPEN        8001 ;* DH.FILE pointer is NULL
$define DHE$NOT.A.FILE           8002 ;* DH.FILE does not point to a file descriptor
$define DHE$ID.LEN.ERR           8003 ;* Invalid record id length
$define DHE$SEEK.ERROR           8004 ;* Error seeking in DH file (os.error)
$define DHE$READ.ERROR           8005 ;* Error reading DH file (os.error)
$define DHE$WRITE.ERROR          8006 ;* Error writing DH file (os.error)
$define DHE$NAME.TOO.LONG        8007 ;* File name is too long
$define DHE$SIZE                 8008 ;* File exceeds maximum allowable size
$define DHE$STAT.ERR             8009 ;* Error from STAT() (os.error)
$define DHE$OPEN.NO.MEMORY       8100 ;* No memory for DH.FILE structure
$define DHE$FILE.NOT.FOUND       8101 ;* Cannot open primary subfile (os.error)
$define DHE$OPEN1.ERR            8102 ;* Cannot open overflow subfile (os.error)
$define DHE$PSFH.FAULT           8103 ;* Primary subfile header format error
$define DHE$OSFH.FAULT           8104 ;* Overflow subfile header format error
$define DHE$NO.BUFFERS           8105 ;* Unable to allocate file buffers
$define DHE$INVA.FILE.NAME       8106 ;* Invalid file name
$define DHE$TOO.MANY.FILES       8107 ;* Too many files (NUMFILES config param)
$define DHE$NO.MEM               8108 ;* No memory to allocate group buffer
$define DHE$AK.NOT.FOUND         8109 ;* Cannot open AK subfile
$define DHE$AK.HDR.READ.ERROR    8110 ;* Error reading AK header
$define DHE$AK.HDR.FAULT         8111 ;* AK subfile header format error
$define DHE$AK.ITYPE.ERROR       8112 ;* Format error in AK I-type code
$define DHE$AK.NODE.ERROR        8113 ;* Unrecognised node type
$define DHE$NO.SUCH.AK           8114 ;* Reference to non-existant AK
$define DHE$AK.DELETE.ERROR      8115 ;* Error deleting AK subfile
$define DHE$EXCLUSIVE            8116 ;* File is open for exclusive access
$define DHE$TRUSTED              8117 ;* Requires trusted program to open
$define DHE$VERSION              8118 ;* Incompatible file version
$define DHE$ID.LEN               8119 ;* File may contain id longer than MAX.ID
$define DHE$AK.CROSS.CHECK       8120 ;* Relocated index pathname cross-check failed
$define DHE$HASH.TYPE            8121 ;* Unsupported hash type
$define DHE$ILLEGAL.GROUP.SIZE   8201 ;* Group size out of range
$define DHE$ILLEGAL.MIN.MODULUS  8202 ;* Minimum modulus < 1
$define DHE$ILLEGAL.BIG.REC.SIZE 8203 ;* Big record size invalid
$define DHE$ILLEGAL.MERGE.LOAD   8204 ;* Merge load invalid
$define DHE$ILLEGAL.SPLIT.LOAD   8205 ;* Split load invalid
$define DHE$FILE.EXISTS          8206 ;* File exists on create
$define DHE$CREATE.DIR.ERR       8207 ;* Cannot create directory (os.error)
$define DHE$CREATE0.ERR          8208 ;* Cannot create primary subfile (os.error)
$define DHE$CREATE1.ERR          8209 ;* Cannot create overflow subfile (os.error)
$define DHE$PSFH.WRITE.ERROR     8210 ;* Failure writing primary subfile header (os.error)
$define DHE$INIT.DATA.ERROR      8211 ;* Failure initialising data bucket (os.error)
$define DHE$ILLEGAL.HASH         8212 ;* Invalid hashing algorithm
$define DHE$OSFH.WRITE.ERROR     8213 ;* Failure writing overflow subfile header (os.error)
$define DHE$RECORD.NOT.FOUND     8301 ;* Record not in file
$define DHE$BIG.CHAIN.END        8302 ;* Found end of big record chain early
$define DHE$NOT.BIG.REC          8303 ;* Big record pointer does not point to big record block
$define DHE$NO.SELECT            8401 ;* No select is active
$define DHE$OPEN2.ERR            8402 ;* Cannot open select list
$define DHE$GSL.WRITE.ERR        8403 ;* Error from write() (os.error)
$define DHE$GSL.TRUNCATE.ERR     8404 ;* Error from chsize() (os.error)
$define DHE$AK.NAME.LEN          8501 ;* Index name too long
$define DHE$AK.EXISTS            8502 ;* AK already exists
$define DHE$AK.TOO.MANY          8503 ;* Too many AKs to create a new one
$define DHE$AK.CREATE.ERR        8504 ;* Unable to create AK subfile
$define DHE$AK.HDR.WRITE.ERROR   8505 ;* Error writing SK subfile header
$define DHE$AK.WRITE.ERROR       8506 ;* Error writing AK node
$define DHE$PSF.CHSIZE.ERR       8601 ;* Error compacting primary subfile (os.error)
$define DHE$ALL.LOCKED           8701 ;* All buffers are locked
$define DHE$SPLIT.HASH.ERROR     8702 ;* Record does not hash to either group in split
$define DHE$WRONG.BIG.REC        8703 ;* Big record chain error
$define DHE$FREE.COUNT.ZERO      8704 ;* Overflow free count zero in dh.new.overflow()
$define DHE$FDS.OPEN.ERR         8705 ;* Cannot reopen subfile (os.error)
$define DHE$POINTER.ERROR        8706 ;* Internal file pointer fault
$define DHE$NO.INDICES           8707 ;* File has no AKs
$define DHE$JNL.OPEN.ERR         8801 ;* Cannot open journal file (os.error)
$define DHE$JNL.CTL.ERR          8802 ;* Cannot open QMSYS $JNLCTRL (os.error)
$define DHE$JNL.CTL.READ         8803 ;* Cannot read QMSYS $JNLCTRL (os.error)
$define DHE$JNL.XCHK             8804 ;* $JNL cross-check error
$define DHE$ECB.TYPE             8900 ;* ECB has incorrect type

* END-CODE
