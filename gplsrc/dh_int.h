/* DH_INT.H
 * Internal include file
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
 * 27Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 * 
 * START-HISTORY (OpenQM):
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 04 Apr 06  2.4-1 dh_filesize() now processes subfiles separately.
 * 23 Feb 06  2.3-7 dh_filesize() now returns int64.
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

#include "dh_fmt.h"

/* Chain of fully open DH files */

Public DH_FILE* dh_file_head init(NULL);

/* Chain of cached closed files */

Public DH_FILE* dh_cache_head init(NULL);
Public DH_FILE* dh_cache_tail init(NULL);
Public int16_t dh_cache_count init(0);

Public DH_FILE* ak_dh_file; /* For dh_write, dh_del interactions with AKs */

#define DEFAULT_GROUP_SIZE 1
#define DEFAULT_BIG_REC_SIZE ((int)(DH_BLOCK_SIZE * 0.8))
#define DEFAULT_MIN_MODULUS 1
#define DEFAULT_MERGE_LOAD 50
#define DEFAULT_SPLIT_LOAD 80

/* Public buffer for DH file processing functions that cannot be called
   recursively.                                                          */

Public char dh_buffer[DH_MAX_GROUP_SIZE_BYTES];

/* Modes for AK update subroutine calls (Also in BP AK_INFO.H) */

#define AK_ADD 1
#define AK_DEL 2
#define AK_MOD 3

/* AK data column index values */

#define AKD_NAME 1    /* AK name. Null string if unused (gap) AK entry. */
#define AKD_FNO 2     /* Field number, -ve if I-type */
#define AKD_FLGS 3    /* Flags */
#define AKD_OBJ 4     /* I-type object code */
#define AKD_MAPNAME 5 /* Collation map name (null string if none)... */
#define AKD_MAP 6     /* ...and the map (contiguous string or null) */
#define AKD_COLS 6    /* No of columns */

/* Trigger functions */

#define TRIGGER_ARGS 4

/* Group lock management macros */

#define GetGroupReadLock(dhf, grp) dh_get_group_lock(dhf, grp, FALSE)
#define FreeGroupReadLock(slot) dh_free_group_lock(slot)
#define GetGroupWriteLock(dhf, grp) dh_get_group_lock(dhf, grp, TRUE)
#define FreeGroupWriteLock(slot) dh_free_group_lock(slot)

/* ======================================================================
   Internal function definitions                                          */

int32_t dh_hash_group(FILE_ENTRY* dh_file, char* id, int16_t id_len);

/* DH_AK.C */
bool ak_clear(DH_FILE* dh_file, int16_t subfile);

/* DH_CLEAR.C */
bool dh_clear(DH_FILE* dh_file);

/* DH_CLOSE.C */
void deallocate_dh_file(DH_FILE* dh_file);

/* DH_CREAT.C */
bool dh_create_file(char path[],
                    int16_t group_size,
                    int32_t min_modulus,
                    int32_t big_rec_size,
                    int16_t merge_load,
                    int16_t split_load,
                    u_int32_t flags,
                    int16_t version);

/* DH_DEL.C */
bool dh_delete(DH_FILE* dh_file, char id[], int16_t id_len);

/* DH_FILE.C */
void dh_fsync(DH_FILE* dh_file, int16_t subfile);
bool dh_init(void);
void dh_close_file(OSFILE fu);
void dh_close_subfile(DH_FILE* dh_file, int16_t subfile);
bool dh_open_subfile(DH_FILE* dh_file,
                     char* pathname,
                     int16_t subfile,
                     bool create);
bool dh_read_group(DH_FILE* dh_file,
                   int16_t subfile,
                   int32_t group,
                   char* buff,
                   int16_t bytes);
bool dh_write_group(DH_FILE* dh_file,
                    int16_t subfile,
                    int32_t group,
                    char* buff,
                    int16_t bytes);
int32_t dh_get_overflow(DH_FILE* dh_file, bool have_group_lock);
void dh_free_overflow(DH_FILE* dh_file, int32_t ogrp);
bool dh_flush_header(DH_FILE* dh_file);
int16_t dh_get_group_lock(DH_FILE* dh_file, int32_t group, bool write_lock);
void dh_free_group_lock(int16_t slot);
bool FDS_open(DH_FILE* dh_file, int16_t subfile);
void FDS_close(void);
void dh_set_subfile(DH_FILE* dh_file, int16_t subfile, OSFILE fu);
int64 dh_filesize(DH_FILE* dh_file, int16_t subfile);
bool SetFileSize(OSFILE fu, int64 bytes);

/* DH_OPEN.C */
int16_t get_file_entry(char* filename,
                       u_int32_t device,
                       u_int32_t inode,
                       struct DH_HEADER* header);

/* DH_READ.C */
STRING_CHUNK* dh_read_record(DH_FILE* dh_file, DH_RECORD* rec_ptr);

/* DH_WRITE.C */
bool dh_free_big_rec(DH_FILE* dh_file, int32_t head, STRING_CHUNK** ak_data);

/* TXN.C */
void txn_abort(void);
bool txn_close(FILE_VAR* fvar);
bool txn_delete(FILE_VAR* fvar, char* id, int16_t id_len);
FILE_VAR* txn_open(char* pathname);
int16_t txn_read(FILE_VAR* fvar,
                 char* id,
                 int16_t id_len,
                 char* actual_id,
                 STRING_CHUNK** str);
#define TXC_FOUND 1   /* Record found in cache */
#define TXC_DELETED 2 /* Record deleted in cache */
bool txn_write(FILE_VAR* fvar, char* id, int16_t id_len, STRING_CHUNK* str);

/* END-CODE */
