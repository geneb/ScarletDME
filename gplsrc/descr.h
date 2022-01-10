/* DESCR.H
 * Descriptor definitions
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
 * ScarletDME Wiki: https://scarlet.deltasoft.com
 * 
 * START-HISTORY (ScarletDME):
 * 09Jan22 gwb Changed STRING_CHUNK to be aligned on a 2 byte boundary.
 *
 * 27Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 * 
 * START-HISTORY (OpenQM):
 * 09 Apr 09 gwb Added SKT_RAW
 *               Added "family" to the SOCKVAR struct in order to track whether or 
 *               not we're using IPv4 or IPv6.
 *
 * 06 Apr 09 gwb Increased the size of the ip_addr variable in SOCKVAR to handle
 *               IPv6 addresses.
 *
 * 30 Mar 09 gwb Added missing defines, SKT$STREAM, SKT$DGRAM, SKT$TCP,
 *               SKT$UDP, SKT$ICMP in order to bring the GPL version to parity
 *               with the commercial version.
 *
 * 15 Aug 07  2.6-0 Reworked remove pointers.
 * 01 Jul 07  2.5-7 Extensive changes for PDA merge.
 * 21 Jul 06  2.4-10 Added object inheritance pointers.
 * 06 Apr 06  2.4-1 Added PERSISTANT descriptor type.
 * 28 Mar 06  2.3-9 Adjusted sequential file handling to allow files > 2Gb.
 * 22 Mar 06  2.3-9 Added object programming structures.
 * 24 Feb 06  2.3-7 Added timeout element to SQ_FILE structure.
 * 20 Feb 06  2.3-6 Generalised array header flags.
 * 29 Oct 04  2.0-9 Added VFS.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * Every variable is represented by a DESCRIPTOR structure.  In some cases
 * this holds all the data associated with the variable.  In other cases it
 * contains pointers to other structures.
 *
 * UNASSIGNED   All local variable start out in this state.
 *
 * ADDR         A pointer to another descriptor.
 *
 * INTEGER      A 32 bit integer value.
 *
 * FLOATNUM     A floating point number.
 *
 * SUBR         A pointer to a memory resident QMBasic program.
 *              Created by CALL transforming a character string holding the
 *              name of the subroutine.  This string is still available via
 *              a pointer in the descriptor.
 *
 * STRING       A character string.
 *              Strings are stored as a series of STRING_CHUNK structures. By
 *              holding strings in this way, some operations such as
 *              concatenation are much faster than using contiguous strings.
 *              Conversely, some operations are slower.
 *              The actual string represented by the string chunks may be
 *              referenced by more than one descriptor. This is controlled
 *              using a reference count in the first chunk.
 *              The rmv_saddr and n1 elements of the descriptor are only
 *              significant if the DF_REMOVE flag is set in which case they
 *              point to the string chunk and offset within that chunk of the
 *              current remove pointer position.
 *
 * FILE_REF     A file variable.
 *              Points to a FILE_VAR structure.  This includes a reference
 *              count to allow multiple descriptors to share a FILE_VAR.
 *
 * ARRAY        An array.
 *              Points to an ARRAY_HEADER structure.  Again, this has a
 *              reference count.  The array header contains information about
 *              the array dimensions and points to a list of ARRAY_CHUNK
 *              structures.  This two level approach significantly improves
 *              performance of redimensioning arrays.
 *
 * COMMON       A common block.
 *              Common blocks are actually held as arrays, each element of
 *              which is an entry from the common declaration, perhaps also an
 *              array.  The common blocks are chained together through the
 *              next_common item in the array header. Element 0 of the common
 *              "array" is the name of the block, the remaining elements are
 *              the common itself. The chain has its head in named_common. The
 *              blank common is addressed by blank_common.
 *
 * IMAGE        A screen image.
 *              Points to a SCREEN_IMAGE structure.  On a Windows QMConsole
 *              session, this structure contains the actual screen data. On a
 *              QMTerm session, the data is held locally by QMTerm and a
 *              unique reference id is stored in the SCREEN_IMAGE.
 *             
 * BTREE        A binary tree data item.
 *              Not accessible to user mode programs (because it's a bit
 *              awkward to use), these are used internally by the QMBasic
 *              compiler and the query processor for fast access tables.
 *
 * SELLIST      A select list variable.
 *              Used by QMBasic operations that work with select list
 *              variables as distinct from numbered select lists.  Because this
 *              variable type was introduced late in the life of QM, the
 *              numbered lists are handled differently.
 *
 * PMATRIX      Pick style systems use a different form of matrix than 
 *              Information style systems. Firstly, Pick matrices have no zero
 *              element. Secondly, a Pick matrix in a common block is just a
 *              list of simple data items. Both styles have their advantages
 *              and disadvantages. QM supports both, though the Information
 *              style is the default. The PMATRIX descriptor defines a Pick
 *              style matrix in a common block.
 *
 * SOCK         Socket.
 *
 * LOCALVARS    A local variable pool.
 *              Local variables are held as arrays in exactly the same way as
 *              common. As far as the program using them is concerned, they
 *              are common variables.
 *              To allow for recursion, entry to an internal subroutine that
 *              declares local variables will stack any previous incarnation
 *              of the local variable pool by chaining it on to the next_common
 *              item in the array header.
 *
 * OBJ          Object.
 *
 * OBJCD        Object code pointer for property access.
 *
 * OBJCDX       Undefined object routine reference (variant on OBJCD)
 *
 * PERSISTENT   Persistent public and private variables of a CLASS module.
 *              For most purposes, this is the same as a COMMON block. The
 *              most significant difference is that element 0 is not
 *              reserved for the block name but is available for normal use.
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

/* ====================================================================== */

/* Data descriptor */

struct DESCRIPTOR {
  u_char type; /* Item type */
  u_char flags;
#ifdef BIG_ENDIAN_SYSTEM
#define InitDescr(d, t) *((int16_t*)(d)) = ((t) << 8)
#else
#define InitDescr(d, t) *((int16_t*)(d)) = t
#endif
  int16_t n1;
  /* OBJCDX:
         Key value for OBJCODE reference.
     STRING:
         Remove pointer offset into chunk pointed to by
         rmv_saddr element of the descriptor. Using QMBasic
         character position numbers, this points to the
         character before the next one to be extracted.
         When the end of the string is reached, it points
         one character off the end of the string.
   */
  union {
    /* ADDR */
    DESCRIPTOR* d_addr; /* Ptr to descriptor (ADDR) */

    /* INTEGER */
    int32_t value; /* Value */

    /* FLOATNUM */
    double float_value; /* Value */

    /* SUBR */
    struct {
      void* object;
      STRING_CHUNK* saddr;
    } subr;

    /* STRING, SELLIST */
    struct {
      STRING_CHUNK* saddr;
      STRING_CHUNK* rmv_saddr; /* Ptr to current chunk */
    } str;

    /* FILE_REF */
    FILE_VAR* fvar; /* File var */

    /* ARRAY */
    ARRAY_HEADER* ahdr_addr;

    /* COMMON and PERSISTENT */
    ARRAY_HEADER* c_addr;

    /* IMAGE */
    SCREEN_IMAGE* i_addr;

    /* BTREE */
    BTREE_HEADER* btree;

    /* PMATRIX */
    PMATRIX_HEADER* pmatrix;

    /* SOCK */
    SOCKVAR* sock;

    /* LOCALVARS */
    ARRAY_HEADER* lv_addr;

    /* OBJ */
    OBJDATA* objdata;

    /* OBJCD */
    struct {
      OBJDATA* objdata; /* Object associated with this data */
      u_int16_t key;    /* Key value */
      u_char arg_ct;    /* Arg count + var args flag */
    } objcode;

    /* OBJCDX */
    struct {
      OBJDATA* objdata; /* Object associated with this data */
      char* undefined_name;
    } objundef;

    /* Debugging */
    struct {
      int32_t w1;
      int32_t w2;
    } dbg;
  } data;
};

/* Descriptor types (Also in BP INT$KEYS.H) */
#define UNASSIGNED 0
#define ADDR 1
#define INTEGER 2
#define FLOATNUM 3
#define COMPLEX_DESCR 4 /* Descriptors with types greater than or equal
                         * to this value have subordinate data which
                         * must be handled by k_release().              
                         */
#define SUBR 4
#define STRING 5
#define FILE_REF 6
#define ARRAY 7
#define COMMON 8
#define IMAGE 9
#define BTREE 10
#define SELLIST 11
#define PMATRIX 12
#define SOCK 13
#define LOCALVARS 14
#define OBJ 15
#define OBJCD 16
#define OBJCDX 17
#define PERSISTENT 18

/* When adding new entry, all places that include a comment containing
   ++ALLTYPES++ must be amended.                                       */
/* This list is also in BP DEBUG.H */

/* Flags */
#define DF_REUSE 0x01  /* Set by reuse function */
#define DF_REMOVE 0x02 /* Remove pointer active */
#define DF_ARG 0x04    /* Argument pointer */
#define DF_WATCH 0x08  /* Set by debug watch, cleared by data change */
#define DF_ARGSET 0x10 /* Set on callv arg vars, cleared by data change */
#define DF_SYSTEM 0x20 /* Set on system variables (STORSYS opcode) */
#define DF_MOVE 0x40   /* Move descr in object function/subr call */
#define DF_CHANGE (DF_WATCH | DF_ARGSET) /* Flags to clear on change */

/* ====================================================================== */

/* --------------- String descriptor STRING --------------- */

struct STRING_CHUNK {
  STRING_CHUNK* next; /* Ptr to next chunk */
  int16_t alloc_size; /* Allocated space excluding header */
  int16_t bytes;      /* Actual used bytes */
  /* The following are only valid in the first chunk */
  int32_t string_len; /* Total length of all chunks */
  int32_t field;      /* Hint field number and... */
  int32_t offset;     /* ...offset. In SELLIST this is item count */
  int16_t ref_ct;     /* Reference count */
  char data[1];
} ALIGN2; /* Making this struct align on a 2 byte boundary cleared up a warning when 
           * a struct of DH_RECORD was being cast to STRING_CHUNK in dh_ak.c, around
           * line #3414. 09Jan22 gwb
           */

#define STRING_CHUNK_HEADER_SIZE (offsetof(STRING_CHUNK, data))
#define MAX_STRING_CHUNK_SIZE ((signed int)(16384 - STRING_CHUNK_HEADER_SIZE))

/* ------------------ File descriptor FILE_REF ------------------ */

typedef struct SQ_FILE SQ_FILE;
struct SQ_FILE {
  OSFILE fu;         /* -ve if open to create */
  char* buff;        /* Buffer */
  int64 posn;        /* File: File position
                        Port: Offset into buffer */
  int64 line;        /* Current line no */
  int64 base;        /* Base address of block */
  char* record_name; /* Record id */
  int16_t record_name_len;
  int16_t bytes;   /* Bytes in buffer */
  char* pathname;  /* File pathname */
  int32_t timeout; /* Timeout, -ve if none */
  u_int16_t flags;
#define SQ_NOBUF 0x0001 /* Unbuffered */
#define SQ_DIRTY 0x0002 /* Buffer needs writing */
#define SQ_PORT 0x0004  /* Is a port */
#define SQ_CHDEV 0x0008 /* Character mode device */
#define SQ_FIFO 0x0010  /* FIFO */
#define SQ_NOTFL 0x001C /* Mask for "files" that aren't files */
};

/* File variable structure
   See !!FVAR_CREATE!! and !!FVAR_DESTROY!! */
struct FILE_VAR {
  int16_t ref_ct;  /* Reference count */
  int16_t file_id; /* File table index */
  int32_t index;   /* Sequential reference counter */
  u_char type;
/* !!FVAR_TYPES!! */
#define INITIAL_FVAR 0
#define DIRECTORY_FILE 1
#define DYNAMIC_FILE 2
#define SEQ_FILE 3
#define NET_FILE 4
#define VFS_FILE 5
  /* Tokens also in BP DEBUG.H */
  u_char flags;
#define FV_RDONLY 0x01  /* Read only file */
#define FV_NON_TXN 0x02 /* Updates are non-transactional */
  int16_t id_len;       /* Length of id and... */
  char* id;             /* ...actual id of last record read */
  char* voc_name;

  union {
    struct {
      DH_FILE* dh_file; /* DH_FILE structure */
      AK_CTRL* ak_ctrl; /* AK control data */
    } dh;
    struct {
      SQ_FILE* sq_file; /* SQ_FILE structure */
    } seq;
    struct {
      u_int16_t mark_mapping : 1;
    } dir;
    struct {
      int file_no;
      int16_t host_index;
    } net;
  } access;
};

/* ------------------ Array descriptor ARRAY ------------------ */

#define MAX_ARRAY_CHUNK_SIZE 200 /* Max elements in a single chunk */

struct ARRAY_CHUNK {
  int16_t num_descr;
  int16_t pad;
  DESCRIPTOR descr[1];
};

struct ARRAY_HEADER {
  int16_t ref_ct;
  u_int16_t flags;
/* These tokens are also in BP INT$KEYS.H */
#define AH_PICK_STYLE 0x0001  /* Pick style matrix */
#define AH_AUTO_DELETE 0x0002 /* Self-deleting common block */
  int32_t rows;
  int32_t cols;              /* Zero if one dimension */
  int32_t alloc_elements;    /* Allocated descriptors and... */
  int32_t used_elements;     /* ...number actually used (could have
                                       redimensioned to smaller size) */
  ARRAY_HEADER* next_common; /* Chain linking named common */
  int16_t num_chunks;        /* Entries in chunk array below. There may
                                       be unused entries at the end of the
                                       array which will be set to NULL.      */
  int16_t pad;
  ARRAY_CHUNK* chunk[1];
};

/* Macro to find an array element given its header and element offset */
#define Element(hdr, n) \
  (&(hdr->chunk[(n) / MAX_ARRAY_CHUNK_SIZE]->descr[(n) % MAX_ARRAY_CHUNK_SIZE]))

/* ------------------ Screen image IMAGE ------------------ */

struct SCREEN_IMAGE {
  int16_t ref_ct;
  int16_t pad;
  void* buffer; /* Screen image data */
  int16_t x;    /* Left edge of block */
  int16_t y;    /* Top edge of block */
  int16_t w;    /* Width of block */
  int16_t h;    /* Height of block */
  int16_t line; /* Cursor line position */
  int16_t col;  /* Cursor column position */
  int16_t pagination;
  int16_t attr; /* ms8 = Inverse, ls8 = attr */
  int32_t id;
};

/* ------------------ Binary Tree BTREE ------------------ */

typedef struct BTREE_ELEMENT BTREE_ELEMENT;
struct BTREE_ELEMENT {
  BTREE_ELEMENT* parent;
  BTREE_ELEMENT* left;
  BTREE_ELEMENT* right;
  char* data;
  char* key[1];
};

struct BTREE_HEADER {
  int16_t ref_ct;
  int16_t pad;
  BTREE_ELEMENT* head;
  BTREE_ELEMENT* current;
  u_char keys;
  u_char flags[1];
/* Additive flags values: */
#define BT_RIGHT_ALIGNED 0x01
#define BT_DESCENDING 0x02
#define BT_UNIQUE 0x04
#define BT_DATA 0x08 /* Has data element */
};

/* ---------------------- PMATRIX ------------------------ */

struct PMATRIX_HEADER {
  DESCRIPTOR* com_descr; /* Associated COMMON */
  u_int16_t base_offset; /* Where is element 1 */
  u_int16_t rows;
  u_int16_t cols; /* Zero if one dimension */
};

/* ------------------------ SOCK ------------------------- */

struct SOCKVAR {
  int ref_ct;
  int socket_handle;
  int port;     /* Port number */
  int protocol; /* protocol to use... */
  int family;   /* PF_INET or PF_INET6 */

#define SKT_STREAM 0x00000000
#define SKT_DGRM 0x00000100
#define SKT_RAW 0x00000200

#define SKT_TCP 0x00000000
#define SKT_UDP 0x00010000
#define SKT_ICMP 0x00020000
  u_int16_t flags;
#define SKT_BLOCKING 0x0001     /* Blocking mode */
#define SKT_NON_BLOCKING 0x0002 /* Non-blocking (Irrelevant here) */
#define SKT_SERVER 0x0004       /* Opened with CREATE.SERVER.SOCKET */
#define SKT_INCOMING 0x0008     /* Opened with ACCEPT.SOCKET.CONNECTION */
#define SKT_USER_MASK 0x0001    /* Flags settable by user */
  char ip_addr[40];             /* IP address,IPv4 or IPv6 */
};

/* -------------------- OBJECT NAME MAP ------------------ */

typedef struct OBJECT_NAME_MAP OBJECT_NAME_MAP;
struct OBJECT_NAME_MAP /* This is embedded in the object code */
{
  /* Multi-byte items are always low byte first because the object stream
    is machine architecture independent                                  */
  u_char next;       /* Offset of next entry, zero in last entry */
  u_char lo_var;     /* Public var no, ls 8 bits */
  u_char hi_var;     /* Public var no, ms 8 bits */
  u_char lo_set;     /* SET routine / method subroutine key, ls 8 bits */
  u_char hi_set;     /* SET routine / method subroutine key, ms 8 bits */
  u_char lo_get;     /* GET routine / method subroutine key, ls 8 bits */
  u_char hi_get;     /* GET routine / method function key, ms 8 bits */
  u_char set_arg_ct; /* SET routine / method subroutine arg count */
  u_char get_arg_ct; /* GET routine / method function arg count */
  char name[1];      /* Item name, upper case, null terminated */
};
/* Notes:
   The public variable number is negative if it is read-only.
   The two arg count items use the top bit to indicate that the
   routine is declared as VAR.ARGS.                                   */

#define SetKey(p) ((p)->lo_set + ((int16_t)((p)->hi_set) << 8))
#define GetKey(p) ((p)->lo_get + ((int16_t)((p)->hi_get) << 8))
#define PublicVar(p) ((p)->lo_var + ((int16_t)((p)->hi_var) << 8))
#define NextNameMapEntry(p) \
  (((p)->next == 0) ? NULL : ((OBJECT_NAME_MAP*)(((char*)(p)) + ((p)->next))))

/* ----------------------- OBJUNDEF ---------------------- */

struct OBJUNDEF /* An undefined object handler reference */
{
  int ref_ct;
  OBJDATA* objdata;     /* Object associated with this data */
  char* undefined_name; /* The handler name we could not find */
  u_int16_t key;        /* Key value */
};

/* ------------------------- OBJ -------------------------

   Object A inherits B and C
   B inherits D
   C inherits E

   -----   -----   -----        In this diagram, the link to the right
   | A |---| B |---| D |        is the "inherits" pointer and the link
   -----   -----   -----        below is the "next_inherited" pointer.
             |
           -----   -----
           | C |---| E |
           -----   -----
*/

struct OBJDATA /* The object itself */
{
  int ref_ct;                /* Reference count */
  void* objprog;             /* Object program pointer */
  OBJECT_NAME_MAP* name_map; /* Start of name map */
  ARRAY_HEADER* obj_vars;    /* PUBLIC and GLOBAL data array */
  OBJDATA* inherits;         /* Head of inherited objects list */
  OBJDATA* next_inherited;   /* Links further items in inheritance chain */
  bool destructor_run;       /* Set on calling destructor to avoid loops */
};

/* ====================================================================== */

#define Release(d)                \
  if ((d)->type >= COMPLEX_DESCR) \
  k_release(d)
#define IncrRefCt(d)              \
  if ((d)->type >= COMPLEX_DESCR) \
  k_incr_refct(d)

#define GetBool(d)          \
  if ((d)->type != INTEGER) \
  k_get_bool(d)
#define GetInt(d)           \
  if ((d)->type != INTEGER) \
  k_get_int(d)
#define GetInt32(d)         \
  if ((d)->type != INTEGER) \
  k_get_int32(d)
#define GetNum(d)                                        \
  if (((d)->type != INTEGER) && ((d)->type != FLOATNUM)) \
  k_get_num(d)
#define GetString(d)       \
  if ((d)->type != STRING) \
  k_get_string(d)

/* END-CODE */
