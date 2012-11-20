/* DH.H
 * Public DH include file
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
 * START-HISTORY:
 * 01 Jul 07  2.5-7 Extensive changes for PDA merge.
 * 30 Aug 06  2.4-12 Added DHF_NO_RESIZE flag.
 * 17 Mar 06  2.3-8 Added record_count to file table.
 * 10 Oct 05  2.2-14 Added akpath element to DH_FILE structure.
 * 09 May 05  2.1-13 Initial changes for large file support.
 * 04 Nov 04  2.0-10 Added DHF_VFS flag.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#define PRIMARY_SUBFILE  0
#define OVERFLOW_SUBFILE 1
#define AK_BASE_SUBFILE  2

#define MAX_GROUP_SIZE   8

#include <dh_stat.h>

/* ========================= FILE_ENTRY ========================= */

typedef volatile struct FILE_ENTRY FILE_ENTRY;
struct FILE_ENTRY
 {
   short int ref_ct;          /* Reference count. Zero in free cell.
                                 -1 if file is open for exclusive access */
   short int lock_count;      /* Count of record locks */
   short int file_lock;       /* User holding file lock, -ve during clearfile.
                                 This entry is protected by REC_LOCK_SEM */
   short int inhibit_count;   /* Count of reasons to hold off split/merge */
   long int fvar_index;       /* Cross-ref to FILE_VAR for file lock or for
                                 exclusive access.                          */
   unsigned long int upd_ct;  /* Updated on write/delete/clear */
   unsigned long int ak_upd;  /* Updated on AK write */
   unsigned long txn_id;      /* Transaction id for file lock (0 if outside
                                 transaction). Applies to owner. */
   unsigned long device;
   unsigned long inode;
   char pathname[MAX_PATHNAME_LEN+1];   /* Null terminated */
   struct
    {
     long int modulus;        /* Current modulus */
     long int min_modulus;
     long int big_rec_size;
     short int split_load;    /* Percent */
     short int merge_load;    /* Percent */
     int64 load_bytes;        /* Bytes */
     long int mod_value;      /* Imaginary file size for hashing */
     short int longest_id;    /* Longest record id in file */
     long int free_chain;     /* Stored as "group" number */
    } params;
   struct FILESTATS stats;
   int64 record_count;        /* Approximate record count. -ve = not set. */
   unsigned short int flags;  /* File specific flags (from DH file header
                                 or as appropriate for DIR file) */
 };

/* Find file table entry for file n, numbered from 1 */
#define FPtr(n) (((FILE_ENTRY *)(((char *)sysseg) + sysseg->file_table)) + ((n)-1))



/* ========================== DH_FILE =========================== */

struct SUBFILE_INFO {
                     OSFILE fu;         /* Operating system file handle */
                     long int tx_ref;   /* For all accesses */
                    };

typedef struct DH_FILE DH_FILE;
struct DH_FILE {
                struct DH_FILE * next_file;
                short int file_id;         /* File table index */

/* File configuration details */
                u_char file_version;      /* File version */
                long int group_size;      /* Group size (bytes) */
                long int header_bytes;    /* Offset of group 1 */
                long int ak_header_bytes; /* Offset of node 1 */
                unsigned long int flags;
/* Flags also defined in BP INT$KEYS.H */
/* LS 16 bits come from file header and are duplicated in file entry... */
#define DHF_NO_RESIZE       0x00000008   /* No splits/merges (Unreliable in
                                            DH_FILE, as CONFIGURE.FILE may
                                            change. Test only in FILE_ENTRY) */
#define DHF_NOCASE          0x00000010   /* Case insensitive ids */
#define DHF_TRUSTED         0x00000020   /* Access requires trusted program */
#define DHF_VFS             0x00000040   /* Not DH at all - It's a VFS */
#define DHF_CREATE          0x000000B8   /* Bits that can be set on creation */
/* MS 16 bits are internal... */
#define DHF_TRIGGER         0x00010000   /* File has trigger */
#define DHF_AK              0x00020000   /* File has AK indices */
#define DHF_RDONLY          0x00040000   /* Read only */
#define FILE_UPDATED        0x00080000   /* Written since opened */

#define DHF_FSYNC           0x01000000   /* fsync pending - see txn.c */
/* File information */
                short int open_count;
                short int no_of_subfiles;
                unsigned long int ak_map;
                ARRAY_HEADER * ak_data;
                char * trigger_name;       /* Trigger function name */
                u_char * trigger;          /* Trigger function code pointer */
                u_char trigger_modes;      /* From file header */
                char * akpath;             /* From file header */
                long int jnl_fno;          /* Journalling file number */
                struct SUBFILE_INFO sf[1];
               };


/* ========================== AK_CTRL =========================== */

struct AK_SCAN
 {
  unsigned long int upd;     /* Copy of file's ak_upd value at time when AK
                                details saved (SELECTINDEX, SETLEFT/RIGHT) */
  long int node_num;
  short int rec_offset;
  u_char flags;
     #define AKS_FOUND 0x01   /* Record was found */
     #define AKS_LEFT  0x02   /* To left of cached record */
     #define AKS_RIGHT 0x04   /* To right of cached record */
  u_char key_len;
  char key[255+1];
 };

typedef struct AK_CTRL AK_CTRL;
struct AK_CTRL
 {
  struct AK_SCAN ak_scan[1];
 };
#define AkCtrlSize(n) \
  ((sizeof(struct AK_CTRL)) + ((n - 1) * sizeof(struct AK_SCAN)))


/* ================================================================ */

Public int select_ftype[HIGH_SELECT+1];       /* Select list file type */
   #define SEL_NONE 0
   #define SEL_DH   1
   #define SEL_VFS  2
Public void * select_file[HIGH_SELECT+1];     /* DH_FILE (DH), FILE_VAR (VFS) */
Public long int select_group[HIGH_SELECT+1];  /* Group for active DH select */

/* ======================================================================
   Public data                                                            */

Public short int dh_err;

/* ======================================================================
   User callable routines                                                 */

/* DH.CLOSE.C */
bool dh_close(DH_FILE * dh_file);

/* DH_EXIST.C */
bool dh_exists(DH_FILE * dh_file, char id[], short int id_len);

/* DH.FILE.C */
bool read_at(OSFILE fu, int64 offset, char * buff, int bytes);
bool write_at(OSFILE fu, int64 offset, char * buff, int bytes);
void dh_shutdown(void);
void dh_configure(DH_FILE * dh_file, long int min_modulus, short int split_load,
                  short int merge_load, long int large_rec_size);

/* DH_HASH.C */
long int hash(char * id, short int id_len);

/* DH_OPEN.C */
DH_FILE * dh_open(char path[]);
long int dh_modulus(DH_FILE * dh_file);

/* DH_READ.C */
STRING_CHUNK * dh_read(DH_FILE * dh_file, char id[], short int id_len, char * actual_id);
char * dh_read_contiguous(DH_FILE * dh_file, char id[], short int id_len);

/* DH_SELCT.C */
bool dh_select(DH_FILE * dh_file, short int list_no);
bool dh_select_group(DH_FILE * dh_file, short int list_no);
void dh_complete_select(short int list_no);
void dh_end_select(short int list_no);
void dh_end_select_file(DH_FILE * dh_file);

/* DH_SPLIT.C */
void dh_split(DH_FILE * dh_file);
void dh_merge(DH_FILE * dh_file);

/* DH_WRITE.C */
bool dh_write(DH_FILE * dh_file, char id[], short int id_len, STRING_CHUNK * rec);

/* END-CODE */
