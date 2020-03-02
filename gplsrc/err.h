/* ERR.H
 * Error numbers and STATUS() values
 * Copyright (c) 2006 Ladybridge Systems, All Rights Reserved
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
 * ScarletDME Wiki: https://scarlet.deltasoft.com
 * 
 * START-HISTORY (ScarletDME):
 * 
 * START-HISTORY (OpenQM):
 * 09 Apr 09 gwb Added ER_INCOMP_PROTO
 *               Added ER_NONBLOCK_FAIL
 *
 * 01 Apr 09 gwb Updated with additional error constants for socket operations.
 *
 * 27 Mar 06  2.3-9 Added ER$TERMINATED.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

// ==================================================================
// All updates need to be reflected in the qmh.rtf help file.
// Use ERRGEN to generate SYSCOM ERR.H and ERRTEXT from this record
// ==================================================================
//
/* Commands that set negative error codes into @SYSTEM.RETURN.CODE use the
   arithmetic inverse of the values listed below.

   Errors tagged (os.error) have associated operating system error numbers
   that can be retrieved from OS.ERROR().  The !ERRTEXT() subroutine will
   automatically include this extended error information in the message. */

/* 0001 - 0999  General command processing errors */
#define ER_ARGS           1    /* Command arguments invalid or incomplete */
#define ER_NCOMO          2    /* Como file not active */
#define ER_ICOMP          3    /* I-type compilation error */
#define ER_ACC_EXISTS     4    /* Account name already in register */
#define ER_NO_DIR         5    /* Directory not found */
#define ER_NOT_CREATED    6    /* Unable to create directory */
#define ER_STOPPED        7    /* Processing terminated by user action */
#define ER_INVA_PATH      8    /* Invalid pathname */
#define ER_NOT_CAT        9    /* Item not in catalogue */
#define ER_PROCESS       10    /* Unable to start new process */
#define ER_USER_EXISTS   11    /* User name already in register */
#define ER_UNSUPPORTED   12    /* This operation is not supported on this platform */
#define ER_TERMINFO      13    /* No terminfo definition for this function */
#define ER_NO_ACC        14    /* Account name not in register */
#define ER_TERMINATED    15    /* Operation terminated by user */

/* 1000 - 1999  General errors */

#define ER_PARAMS      1000    /* Invalid parameters */
#define ER_MEM         1001    /* Cannot allocate memory */
#define ER_LENGTH      1002    /* Invalid length */
#define ER_BAD_NAME    1003    /* Bad name */
#define ER_NOT_FOUND   1004    /* Item not found */
#define ER_IN_USE      1005    /* Item is in use */
#define ER_BAD_KEY     1006    /* Bad action key */
#define ER_PRT_UNIT    1007    /* Bad print unit */
#define ER_FAILED      1008    /* Action failed (os.error) */
#define ER_MODE        1009    /* Bad mode setting */
#define ER_TXN         1010    /* Operation not allowed in a transaction */
#define ER_TIMEOUT     1011    /* Timeout */
#define ER_LIMIT       1012    /* Limit reached */
#define ER_EXPIRED     1013    /* Expired */
#define ER_NO_CONFIG   1014    /* Cannot find configuration file */
#define ER_RDONLY_VAR  1015    /* Variable is read-only */
#define ER_NOT_PHANTOM 1016    /* Not a phantom process */
#define ER_CONNECTED   1017    /* Device already connected */
#define ER_INVA_ITYPE  1018    /* Invalid I-type */

/* 20xx  Catalog management errors */

#define ER_INVA_OBJ    2000    /* Invalid object code */
#define ER_CFNF        2001    /* Catalogued function not found */

/* 21xx  Terminfo errors */

#define ER_TI_NAME     2100    /* Invalid terminal type name */
#define ER_TI_NOENT    2101    /* No terminfo entry for given name */
#define ER_TI_MAGIC    2102    /* Terminfo magic number check failed */
#define ER_TI_INVHDR   2103    /* Invalid terminfo header data */
#define ER_TI_STRSZ    2104    /* Invalid terminfo string length */
#define ER_TI_STRMEM   2105    /* Error allocating terminfo string memory */
#define ER_TI_NAMEMEM  2106    /* Error allocating terminfo name memory */
#define ER_TI_BOOLMEM  2107    /* Error allocating terminfo boolean memory */
#define ER_TI_BOOLRD   2108    /* Error reading terminfo booleans */
#define ER_TI_NUMMEM   2109    /* Error allocating terminfo numbers memory */
#define ER_TI_NUMRD    2110    /* Error reading terminfo numbers */
#define ER_TI_STROMEM  2111    /* Error allocating terminfo string offsets memory */
#define ER_TI_STRORD   2112    /* Error reading terminfo string offsets */
#define ER_TI_STRTBL   2113    /* Error reading terminfo string table */

/* 3000 - 3999  Generic file system errors */
#define ER_IID         3000    /* Illegal record id */
#define ER_SFNF        3001    /* Subfile not found */
#define ER_NAM         3002    /* Bad file name */
#define ER_FNF         3003    /* File not found */
#define ER_NDIR        3004    /* Not a directory file */
#define ER_NDYN        3005    /* Not a dynamic file */
#define ER_RNF         3006    /* Record not found */
#define ER_NVR         3007    /* No VOC record */
#define ER_NPN         3008    /* No pathname in VOC record */
#define ER_VNF         3009    /* VOC file record not F type */
#define ER_IOE         3010    /* I/O error (os.error) */
#define ER_LCK         3011    /* Lock is held by another process */
#define ER_NLK         3012    /* Lock is not held by this process */
#define ER_NSEQ        3013    /* Not a sequential file */
#define ER_NEOF        3014    /* Not at end of file */
#define ER_SQRD        3015    /* Sequential file record read before creation */
#define ER_SQNC        3016    /* Sequential record not created due to error (os.error) */
#define ER_SQEX        3017    /* Sequential record already exists (CREATE) */
#define ER_RDONLY      3018    /* Update to read only file */
#define ER_AKNF        3019    /* AK index not found */
#define ER_INVAPATH    3020    /* Invalid pathname */
#define ER_EXCLUSIVE   3021    /* Cannot gain exclusive access to file */
#define ER_TRIGGER     3022    /* Trigger function error */
#define ER_NOLOCK      3023    /* Attempt to write/delete record with no lock */
#define ER_REMOTE      3024    /* Open of remote file not allowed */
#define ER_NOTNOW      3025    /* Action cannot be performed now */
#define ER_PORT        3026    /* File is a port */
#define ER_NPORT       3027    /* File is not a port */
#define ER_SQSEEK      3028    /* Seek to invalid offset in sequential file */
#define ER_SQREL       3029    /* Invalid seek relto in sequential file */
#define ER_EOF         3030    /* End of file */
#define ER_CNF         3031    /* Multifile component not found */
#define ER_MFC         3032    /* Multifile reference with no component name */
#define ER_PNF         3033    /* Port not found (os.error) */
#define ER_BAD_DICT    3034    /* Bad dictionary entry */
#define ER_PERM        3035    /* Permissions error (os.errno) */
#define ER_SEEK_ERROR  3036    /* Seek error */
#define ER_WRITE_ERROR 3037    /* Write error (os.errno) */
#define ER_VFS_NAME    3038    /* Bad class name in VFS entry */
#define ER_VFS_CLASS   3039    /* VFS class routine not found */
#define ER_VFS_NGLBL   3040    /* VFS class routine is not globally catalogued */
#define ER_ENCRYPTED   3041    /* Access denied to encrypted file */

/* 4000 - 4999   QMClient errors */
#define ER_SRVRMEM     4000    /* Insufficient memory for packet buffer */

/* 5000 - 5999   Operating system related issues */
#define ER_NO_DLL      5000    /* DLL not found */
#define ER_NO_API      5001    /* API not found */
#define ER_NO_TEMP     5002    /* Cannot open temporary file (os.error) */

/* 6000 - 6999 */
#define ER_NO_EXIST    6031    /* Item does not exist */
#define ER_EXISTS      6032    /* Item already exists */
#define ER_NO_SPACE    6033    /* No space for entry */
#define ER_INVALID     6034    /* Validation error */

/* 7000 - 7999   Network errors */
#define ER_NETWORK     7000    /* Networked file not allowed for this operation */
#define ER_SERVER      7001    /* Unknown server name */
#define ER_WSA_ERR     7002    /* Failed to start Window socket library (os.error) */
#define ER_HOSTNAME    7003    /* Invalid host name */
#define ER_NOSOCKET    7004    /* Cannot open socket (os.error) */
#define ER_CONNECT     7005    /* Cannot connect socket (os.error) */
#define ER_RECV_ERR    7006    /* Error receiving socket data (os.error) */
#define ER_RESOLVE     7007    /* Cannot resolve server name (os.error) */
#define ER_LOGIN       7008    /* Login rejected */
#define ER_XREMOTE     7009    /* Remote server disallowed access */
#define ER_ACCOUNT     7010    /* Cannot connect to account */
#define ER_HOST_TABLE  7011    /* Host table is full */
#define ER_BIND        7012    /* Error binding socket (os.error) */
#define ER_SKT_CLOSED  7013    /* Socket has been closed */
// getaddrinfo errors 
#define ERAI_ADDRFAMILY 7014    /* Address family for nodename not supported */
#define ERAI_AGAIN      7015    /* Temporary failure in name resolution. */
#define ERAI_BADFLAGS   7016    /* Invalid value for ai_flags */
#define ERAI_FAIL       7017    /* Non-recoverable failure in name resolution. */
#define ERAI_FAMILY     7018    /* Requested ai_family not supported */
#define ERAI_MEMORY     7019    /* Memory allocation failure */
#define ERAI_NODATA     7020    /* No address associated with nodename */
#define ERAI_NONAME     7021    /* nodename nor servname provided, or not known. */
#define ERAI_SERVICE    7022    /* servname not supported for ai_socktype */
#define ERAI_SOCKTYPE   7023    /* Requested ai_socktype not supported */
#define ERAI_SYSTEM     7024    /* System error returned in errno */

#define ER_INCOMP_PROTO 7025    /* Incompatible protocol/transport combination */
#define ER_NONBLOCK_FAIL 7026   /* Attempt to set O_NON_BLOCK via fctnl() has failed */
#define ER_BADADDR      7027    /* Incorrectly formed hostname or IP Address */

/* 8000 - 8999  DH errors */
// 80xx  General errors 
#define DHE_FILE_NOT_OPEN        8001 /* DH_FILE pointer is NULL */
#define DHE_NOT_A_FILE           8002 /* DH_FILE does not point to a file descriptor */
#define DHE_ID_LEN_ERR           8003 /* Invalid record id length */
#define DHE_SEEK_ERROR           8004 /* Error seeking in DH file (os.error) */
#define DHE_READ_ERROR           8005 /* Error reading DH file (os.error) */
#define DHE_WRITE_ERROR          8006 /* Error writing DH file (os.error) */
#define DHE_NAME_TOO_LONG        8007 /* File name is too long */
#define DHE_SIZE                 8008 /* File exceeds maximum allowable size */
#define DHE_STAT_ERR             8009 /* Error from STAT() (os.error) */
// 81xx  dh_open()
#define DHE_OPEN_NO_MEMORY       8100 /* No memory for DH_FILE structure */
#define DHE_FILE_NOT_FOUND       8101 /* Cannot open primary subfile (os.error) */
#define DHE_OPEN1_ERR            8102 /* Cannot open overflow subfile (os.error) */
#define DHE_PSFH_FAULT           8103 /* Primary subfile header format error */
#define DHE_OSFH_FAULT           8104 /* Overflow subfile header format error */
#define DHE_NO_BUFFERS           8105 /* Unable to allocate file buffers */
#define DHE_INVA_FILE_NAME       8106 /* Invalid file name */
#define DHE_TOO_MANY_FILES       8107 /* Too many files (NUMFILES config param) */
#define DHE_NO_MEM               8108 /* No memory to allocate group buffer */
#define DHE_AK_NOT_FOUND         8109 /* Cannot open AK subfile */
#define DHE_AK_HDR_READ_ERROR    8110 /* Error reading AK header */
#define DHE_AK_HDR_FAULT         8111 /* AK subfile header format error */
#define DHE_AK_ITYPE_ERROR       8112 /* Format error in AK I-type code */
#define DHE_AK_NODE_ERROR        8113 /* Unrecognised node type */
#define DHE_NO_SUCH_AK           8114 /* Reference to non-existant AK */
#define DHE_AK_DELETE_ERROR      8115 /* Error deleting AK subfile */
#define DHE_EXCLUSIVE            8116 /* File is open for exclusive access */
#define DHE_TRUSTED              8117 /* Requires trusted program to open */
#define DHE_VERSION              8118 /* Incompatible file version */
#define DHE_ID_LEN               8119 /* File may contain id longer than MAX_ID */
#define DHE_AK_CROSS_CHECK       8120 /* Relocated index pathname cross-check failed */
#define DHE_HASH_TYPE            8121 /* Unsupported hash type */
// 82xx  dh_create_file()
#define DHE_ILLEGAL_GROUP_SIZE   8201 /* Group size out of range */
#define DHE_ILLEGAL_MIN_MODULUS  8202 /* Minimum modulus < 1 */
#define DHE_ILLEGAL_BIG_REC_SIZE 8203 /* Big record size invalid */
#define DHE_ILLEGAL_MERGE_LOAD   8204 /* Merge load invalid */
#define DHE_ILLEGAL_SPLIT_LOAD   8205 /* Split load invalid */
#define DHE_FILE_EXISTS          8206 /* File exists on create */
#define DHE_CREATE_DIR_ERR       8207 /* Cannot create directory (os.error) */
#define DHE_CREATE0_ERR          8208 /* Cannot create primary subfile (os.error) */
#define DHE_CREATE1_ERR          8209 /* Cannot create overflow subfile (os.error) */
#define DHE_PSFH_WRITE_ERROR     8210 /* Failure writing primary subfile header (os.error) */
#define DHE_INIT_DATA_ERROR      8211 /* Failure initialising data bucket (os.error) */
#define DHE_ILLEGAL_HASH         8212 /* Invalid hashing algorithm */
#define DHE_OSFH_WRITE_ERROR     8213 /* Failure writing overflow subfile header (os.error) */
// 83xx  dh_read()
#define DHE_RECORD_NOT_FOUND     8301 /* Record not in file */
#define DHE_BIG_CHAIN_END        8302 /* Found end of big record chain early */
#define DHE_NOT_BIG_REC          8303 /* Big record pointer does not point to big record block */
// 84xx  dh_select(), dh_readkey(), dh_readnext(), dh_generate_select()
#define DHE_NO_SELECT            8401 /* No select is active */
#define DHE_OPEN2_ERR            8402 /* Cannot open select list */
#define DHE_GSL_WRITE_ERR        8403 /* Error from write() (os.error) */
#define DHE_GSL_TRUNCATE_ERR     8404 /* Error from chsize() (os.error) */
// 85xx  index errors
#define DHE_AK_NAME_LEN          8501 /* Index name too long */
#define DHE_AK_EXISTS            8502 /* AK already exists */
#define DHE_AK_TOO_MANY          8503 /* Too many AKs to create a new one */
#define DHE_AK_CREATE_ERR        8504 /* Unable to create AK subfile */
#define DHE_AK_HDR_WRITE_ERROR   8505 /* Error writing SK subfile header */
#define DHE_AK_WRITE_ERROR       8506 /* Error writing AK node */
// 86xx  file pack errors
#define DHE_PSF_CHSIZE_ERR       8601 /* Error compacting primary subfile (os.error) */
// 87xx internal errors
#define DHE_ALL_LOCKED           8701 /* All buffers are locked */
#define DHE_SPLIT_HASH_ERROR     8702 /* Record does not hash to either group in split */
#define DHE_WRONG_BIG_REC        8703 /* Big record chain error */
#define DHE_FREE_COUNT_ZERO      8704 /* Overflow free count zero in dh_new_overflow() */
#define DHE_FDS_OPEN_ERR         8705 /* Cannot reopen subfile (os.error) */
#define DHE_POINTER_ERROR        8706 /* Internal file pointer fault */
#define DHE_NO_INDICES           8707 /* File has no AKs */
//88xx journalling
#define DHE_JNL_OPEN_ERR         8801 /* Cannot open journal file (os.error) */
#define DHE_JNL_CTL_ERR          8802 /* Cannot open QMSYS $JNLCTRL (os.error) */
#define DHE_JNL_CTL_READ         8803 /* Cannot read QMSYS $JNLCTRL (os.error) */
#define DHE_JNL_XCHK             8804 /* $JNL cross-check error */
//89xx encryption
#define DHE_ECB_TYPE             8900 /* ECB has incorrect type */

/* END-CODE */
