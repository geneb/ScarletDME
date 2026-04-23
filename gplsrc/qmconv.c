/* QMCONV.C
 * File convertor for high/low byte systems.
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
 * 17Oct25 mab Change dyn file prefix to %
 * 09Jan22 gwb Fixups for 64 bit builds.  (see #ifndef __LP64__ blocks)
 *
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 *
 * 24Feb20 gwb Fixed a variable set but never used warning.
 * START-HISTORY (OpenQM):
 * 24 May 06  2.4-5 0491 AK used_bytes field was being converted as a int32_t.
 * 17 Mar 06  2.3-8 Convert record count.
 * 20 Sep 05  2.2-11 Use static buffer to minimise stack space.
 * 10 May 05  2.1-13 Changes for large file support.
 * 01 Feb 05  2.1-5 Fixed various issues with conversion to inverse byte order
 *                  compared to machine on which we are running.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * qmconv {options} pathname(s)
 *
 * Options:
 *   -B   Convert to big-endian format
 *   -L   Convert to little-endian format
 *        If neither option is given, converts to format of machine on
 *        which it is being run.
 *
 *   -D   Debug - Prints detailed progress information.
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#define Public
#define init(a) = a

#include "qm.h"
#include "dh_int.h"
#include "header.h"
#include "revstamp.h"

#define MAX_SUBFILES (AK_BASE_SUBFILE + MAX_INDICES)

static bool file_found = FALSE;
static int fu[MAX_SUBFILES];   /* File variables */
static DH_HEADER header;       /* Primary subfile header */
static DH_HEADER oheader;      /* Overflow subfile header */
static int16_t header_bytes; /* Offset of group 1 */
static int16_t ak_header_size;

static union {
  DH_BLOCK dh_block;
  DH_BIG_BLOCK dh_big_block;
  DH_AK_HEADER dh_ak_header;
  DH_FREE_NODE dh_free_node;
  DH_INT_NODE dh_int_node;
  DH_TERM_NODE dh_term_node;
  DH_ITYPE_NODE dh_itype_node;
  DH_BIG_NODE dh_big_node;
  char data[DH_MAX_GROUP_SIZE_BYTES];
} buffer;

#define TO_CURRENT 0
#define TO_BIG 1
#define TO_LITTLE 2
static int16_t mode = TO_CURRENT;
static bool converting_to_inverse; /* Are we going away from this system? */

static bool debug = FALSE;

void convert_file(char* fn);
bool is_dh_file(char* fn);
bool is_object_file(char* fn);
int process_file(char* fn);
int process_object_file(char* fn);
bool open_subfile(char* fn, int16_t sf);
bool read_block(int16_t subfile, int64 offset, int16_t bytes, char* buff);
bool write_block(int16_t subfile, int64 offset, int16_t bytes, char* buff);
bool write_header(void);
int16_t swap2(int16_t data);
int32_t swap4(int32_t data);
bool convert_object(OBJECT_HEADER* obj, int bytes, bool skip_fields);
void convert_primary_header(void);

#define Reverse2(a) a = swap2(a)
#define Reverse4(a) a = swap4(a)
#define Reverse8(a) a = swap8(a)

#define Seek(fu, offset, whence) lseek(fu, offset, whence)

/* ====================================================================== */

int main(int argc, char* argv[]) {
  int status = 1;
  int arg;
  int argx;
  bool arg_end;

  printf(
      "[ QMCONV %s   Copyright, Ladybridge Systems, %s.  All rights reserved. "
      "]\n\n",
      QM_REV_STAMP, QM_COPYRIGHT_YEAR);

  /* Check command arguments */

  for (arg = 1; arg < argc; arg++) {
    if (argv[arg][0] != '-')
      break;

    argx = 1;
    arg_end = FALSE;
    do {
      switch (toupper(argv[arg][argx])) {
        case '\0':
          arg_end = TRUE;
          break;

        case 'B':
          mode = TO_BIG;
          break;

        case 'D':
          debug = TRUE;
          break;

        case 'L':
          mode = TO_LITTLE;
          break;

        default:
          goto usage;
      }

      argx++;
    } while (!arg_end);
  }

  if (arg == argc)
    goto usage; /* No filenames */

#ifdef BIG_ENDIAN_SYSTEM
  converting_to_inverse = (mode == TO_LITTLE);
#else
  converting_to_inverse = (mode == TO_BIG);
#endif

  while (arg < argc) {
    convert_file(argv[arg]);
    arg++;
  }

  status = 0;

exit_qmconv:
  return status;

usage:
  printf("Usage: QMCONV {options} pathname(s)\n\n");
  printf("Options:\n");
  printf("   -B      Force conversion to big-endian\n");
  printf("   -L      Force conversion to little-endian\n");

  goto exit_qmconv;
}

/* ======================================================================
   convert_file()                                                         */

void convert_file(char* fn) {
  // int status; value set but never used.
  char filename[MAX_PATHNAME_LEN + 1];

  strcpy(filename, fn);

  if (is_dh_file(filename)) {
    if (file_found)
      printf("\n\n");
    file_found = TRUE;
    //status = process_file(filename); value is never used, so don't assign it.
    process_object_file(filename);
  } else if (is_object_file(filename)) {
    if (file_found)
      printf("\n\n");
    file_found = TRUE;
    // status = process_object_file(filename);
    // value is never used, so don't assign it.
    process_object_file(filename);
  }
}

/* ======================================================================
   is_dh_file()                                                           */

bool is_dh_file(char* filename) {
  bool status = FALSE;
  int dhfu = -1;
  char pathname[MAX_PATHNAME_LEN + 1];
  struct stat statbuf;
  
  //* 17Oct25 mab Change dyn file prefix to %
  sprintf(pathname, "%s%c%%0", filename, DS);

  if (stat(pathname, &statbuf) != 0)
    goto exit_is_dh_file;

  if (!(statbuf.st_mode & S_IFREG))
    goto exit_is_dh_file;

  dhfu = open(pathname, (int)(O_RDWR | O_BINARY), default_access);
  if (dhfu < 0)
    goto exit_is_dh_file;

  if (read(dhfu, (u_char*)&header, DH_HEADER_SIZE) != DH_HEADER_SIZE) {
    goto exit_is_dh_file;
  }

  /* Check magic number in primary subfile */

  if ((header.magic != DH_PRIMARY) && (swap2(header.magic) != DH_PRIMARY)) {
    goto exit_is_dh_file;
  }

  status = TRUE;

exit_is_dh_file:
  if (dhfu >= 0)
    close(dhfu);
  return status;
}

/* ======================================================================
   is_object_file()                                                      */

bool is_object_file(char* filename) {
  bool status = FALSE;
  int objfu = -1;
  struct stat statbuf;
  OBJECT_HEADER obj_header;

  if (stat(filename, &statbuf) != 0)
    goto exit_is_object_file;

  if (!(statbuf.st_mode & S_IFREG))
    goto exit_is_object_file;

  objfu = open(filename, (int)(O_RDWR | O_BINARY), default_access);
  if (objfu < 0)
    goto exit_is_object_file;

  if (read(objfu, (u_char*)&obj_header, OBJECT_HEADER_SIZE) !=
      OBJECT_HEADER_SIZE) {
    goto exit_is_object_file;
  }

  /* Check magic number */

  if ((obj_header.magic != HDR_MAGIC) &&
      (obj_header.magic != HDR_MAGIC_INVERSE)) {
    goto exit_is_object_file;
  }

  status = TRUE;

exit_is_object_file:
  if (objfu >= 0)
    close(objfu);
  return status;
}

/* ======================================================================
   process_file()  -  Process a file                                      */

int process_file(char* filename) {
  int status = 1;
  int16_t tgt_magic;
  int16_t otgt_magic;
  int16_t sf;     /* Subfile index */
  int32_t grp;     /* Group number... */
  int64 grp_offset; /* ...and file offset */
  int16_t group_bytes;
  int16_t rec_offset;
  DH_RECORD* rec_ptr;
  int32_t ak_itype_ptr = 0;
  int32_t ak_itype_len;
  int16_t ak_fno;
  u_char* itype_object;
  u_int32_t bytes_remaining;
  int16_t next;
  int16_t used_bytes;
  int32_t next_node;
  int16_t i;
  int32_t n;
  u_char* p;

  printf("Processing %s\n", filename);

  if (debug)
    printf("Converting to inverse = %d\n", converting_to_inverse);

  /* Open primary and overflow subfiles */

  if (!open_subfile(filename, PRIMARY_SUBFILE)) {
    perror("Cannot open primary subfile");
    goto exit_process_file;
  }

  if (!open_subfile(filename, OVERFLOW_SUBFILE)) {
    perror("Cannot open overflow subfile");
    goto exit_process_file;
  }

  /* Read primary subfile header */

  if (!read_block(PRIMARY_SUBFILE, 0, DH_HEADER_SIZE, (char*)&header)) {
    perror("Cannot read primary subfile header");
    goto exit_process_file;
  }

  /* Read overflow subfile header */

  if (!read_block(OVERFLOW_SUBFILE, 0, DH_HEADER_SIZE, (char*)&oheader)) {
    perror("Cannot read overflow subfile header");
    goto exit_process_file;
  }

  /* Check magic number in primary subfile */

  if (converting_to_inverse) {
    tgt_magic = swap2(DH_PRIMARY);
    otgt_magic = swap2(DH_OVERFLOW);
  } else {
    tgt_magic = DH_PRIMARY;
    otgt_magic = DH_OVERFLOW;
  }

  if (header.magic == tgt_magic) /* Already in right format */
  {
    printf("No conversion required\n");
    goto exit_process_file_ok;
  }

  if ((header.magic == DH_CONVERTING) /* Left from aborted conversion */
      || (swap2(header.magic) == DH_CONVERTING)) {
    printf(
        "This file has previously been partially converted and is unusable\n");
    goto exit_process_file;
  }

  if (header.magic != swap2(tgt_magic)) {
    printf("Primary subfile has incorrect magic number (%04X)\n",
           (int)(header.magic));
    goto exit_process_file;
  }

  /* Check magic number in overflow subfile */

  if (oheader.magic != swap2(otgt_magic)) {
    printf("Overflow subfile has incorrect magic number (%04X)\n",
           (int)(oheader.magic));
    goto exit_process_file;
  }

  group_bytes = header.group_size;

  /* Check for each AK subfile and open */

  for (i = 0; i < MAX_INDICES; i++) {
    (void)open_subfile(filename, AK_BASE_SUBFILE + i);
  }

  /* Check file version is supported */

  /* !!FILE_VERSION!! */
  if (header.file_version > DH_VERSION) {
    printf("Unrecognised file version (%d)\n", (int)header.file_version);
    goto exit_process_file;
  }

  /* Do the conversion.
    We update the header in memory first to make it easy to use the pointers
    in it, however, we write it back with a special magic number so that we
    can detect a file that was partially converted and is probably totally
    destroyed.  The corrected header will be written when we finish.
    Note that the header is left uncoverted at this stage if we are
    converting to the inverse of the byte ordering of the system on which
    we are running so that we can use the header data internally.           */

  /* ========== Primary subfile header */

  header.magic = DH_CONVERTING;
  if (!converting_to_inverse)
    convert_primary_header();

  if (!write_block(PRIMARY_SUBFILE, 0, DH_HEADER_SIZE, (char*)&header)) {
    printf("Write error in primary subfile header\n");
    goto exit_process_file;
  }

  /* Now that it is all the right way round, pick up some useful data */

  header_bytes = DHHeaderSize(header.file_version, header.group_size);
  ak_header_size = AKHeaderSize(header.file_version);

  /* ========== Overflow subfile header */

  Reverse2(oheader.magic);
  Reverse4(oheader.group_size);

  if (!write_block(OVERFLOW_SUBFILE, 0, DH_HEADER_SIZE, (char*)&oheader)) {
    printf("Write error in overflow subfile header\n");
    goto exit_process_file;
  }

  /* ========== Data blocks.
    For best performance, we walk through block by block, ignoring overflow
    chains. We will end up also converting any dead space at the end of the
    subfiles but there shouldn't be much.                                    */

  for (sf = PRIMARY_SUBFILE; sf < AK_BASE_SUBFILE; sf++) {
    if (fu[sf] < 0)
      continue; /* This subfile doesn't exist */

    if (debug)
      printf("Subfile %d\n", sf);

    grp = 1;
    grp_offset = header_bytes;
    group_bytes = header.group_size;

    while (read_block(sf, grp_offset, group_bytes, buffer.data)) {
      if (debug)
#ifndef __LP64__      
        printf("%012LX: Group %d, type %d\n", grp_offset, grp, 
               buffer.dh_block.block_type);
#else
        printf("%012lu: Group %d, type %d\n", grp_offset, grp, 
              buffer.dh_block.block_type);
#endif
      /* What is this group? */

      switch (buffer.dh_block.block_type) {
        case DHT_DATA:
          used_bytes = buffer.dh_block.used_bytes;
          Reverse4(buffer.dh_block.next);
          Reverse2(buffer.dh_block.used_bytes);
          if (!converting_to_inverse)
            used_bytes = buffer.dh_block.used_bytes;

          rec_offset = BLOCK_HEADER_SIZE;
          while (rec_offset < used_bytes) {
            if (debug)
#ifndef __LP64__            
              printf("%012LX.%04X\n", grp_offset, rec_offset); 
#else
              printf("%012lu.%04X\n", grp_offset, rec_offset); 
#endif
            rec_ptr = (DH_RECORD*)(buffer.data + rec_offset);
            next = rec_ptr->next; /* Pick up as is... */
            Reverse2(rec_ptr->next);
            Reverse4(rec_ptr->data.data_len);
            if (!converting_to_inverse)
              next = rec_ptr->next; /* ...update */
            rec_offset += next;
          }
          break;

        case DHT_BIG_REC:
          Reverse4(buffer.dh_block.next);
          Reverse2(buffer.dh_block.used_bytes);
          Reverse4(buffer.dh_big_block.data_len);

          break;

        default:
          printf("Subfile %d, group %d: Unknown block type (%d)\n", sf, grp,
                 buffer.dh_block.block_type);
          continue;
      }

      if (!write_block(sf, grp_offset, group_bytes, buffer.data)) {
        printf("Subfile %d, group %d: Write error\n", sf, grp);
      }

      grp++;
      grp_offset += group_bytes;
    }
  }

  /* ========== AK subfiles */

  for (sf = AK_BASE_SUBFILE; sf < MAX_SUBFILES; sf++) {
    if (fu[sf] < 0)
      continue; /* This subfile doesn't exist */

    if (debug)
      printf("Subfile %d\n", sf);

    /* Convert AK subfile header */

    if (!read_block(sf, 0, ak_header_size, buffer.data)) {
      printf("Error reading AK header in subfile %d", sf);
      continue;
    }

    ak_itype_ptr = buffer.dh_ak_header.itype_ptr;
    ak_itype_len = buffer.dh_ak_header.itype_len;
    ak_fno = buffer.dh_ak_header.fno;

    Reverse2(buffer.dh_ak_header.magic);
    Reverse2(buffer.dh_ak_header.flags);
    Reverse2(buffer.dh_ak_header.fno);
    Reverse2(buffer.dh_ak_header.spare);
    Reverse4(buffer.dh_ak_header.free_chain);
    Reverse4(buffer.dh_ak_header.itype_len);
    Reverse4(buffer.dh_ak_header.itype_ptr);
    Reverse4(buffer.dh_ak_header.data_creation_timestamp);

    if (!converting_to_inverse) {
      ak_itype_ptr = buffer.dh_ak_header.itype_ptr;
      ak_itype_len = buffer.dh_ak_header.itype_len;
      ak_fno = buffer.dh_ak_header.fno;
    }

    if ((ak_fno < 0) && (ak_itype_ptr == 0)) {
      if (debug)
        printf("Converting short AK I-type object code\n");
      convert_object((OBJECT_HEADER*)&buffer.dh_ak_header.itype, ak_itype_len,
                     TRUE);
    }

    if (!write_block(sf, 0, ak_header_size, buffer.data)) {
      printf("Subfile %d, AK header: Write error\n", sf);
    }

    grp = 1;
    grp_offset = ak_header_size;
    group_bytes = DH_AK_NODE_SIZE;

    while (read_block(sf, grp_offset, group_bytes, buffer.data)) {
      if (debug)
#ifndef __LP64__      
        printf("%012LX: Group %d, type %d\n", grp_offset, grp,  
               buffer.dh_free_node.node_type);
#else
        printf("%012lu: Group %d, type %d\n", grp_offset, grp,
               buffer.dh_free_node.node_type);
#endif
      /* What is this node? */

      switch (buffer.dh_free_node.node_type) {
        case AK_FREE_NODE:
          Reverse2(buffer.dh_free_node.used_bytes); /* 0491 */
          Reverse4(buffer.dh_free_node.next);
          break;

        case AK_INT_NODE:
          Reverse2(buffer.dh_int_node.used_bytes); /* 0491 */
          for (i = 0; i < MAX_CHILD; i++)
            Reverse4(buffer.dh_int_node.child[i]);
          break;

        case AK_TERM_NODE:
          used_bytes = buffer.dh_term_node.used_bytes;
          Reverse2(buffer.dh_term_node.used_bytes); /* 0491 */
          Reverse4(buffer.dh_term_node.left);
          Reverse4(buffer.dh_term_node.right);
          if (!converting_to_inverse)
            used_bytes = buffer.dh_term_node.used_bytes;
          rec_offset = TERM_NODE_HEADER_SIZE;
          while (rec_offset < used_bytes) {
            rec_ptr = (DH_RECORD*)(buffer.data + rec_offset);
            next = rec_ptr->next;
            Reverse2(rec_ptr->next);
            Reverse4(rec_ptr->data.data_len);
            if (!converting_to_inverse)
              next = rec_ptr->next;
            rec_offset += next;
          }
          break;

        case AK_ITYPE_NODE:
          Reverse2(buffer.dh_itype_node.used_bytes); /* 0491 */
          Reverse4(buffer.dh_itype_node.next);
          break;

        case AK_BIGREC_NODE:
          Reverse2(buffer.dh_big_node.used_bytes); /* 0491 */
          Reverse4(buffer.dh_big_node.next);
          Reverse4(buffer.dh_big_node.data_len);
          break;

        default:
          printf("Subfile %d, node %d: Unknown node type (%d)\n", sf, grp,
                 buffer.dh_free_node.node_type);
          continue;
      }

      if (!write_block(sf, grp_offset, group_bytes, buffer.data)) {
        printf("Subfile %d, node %d: Write error\n", sf, grp);
      }

      grp++;
      grp_offset += group_bytes;
    }
  }

  /* If this subfile includes a large AK I-type, the block headers have all
    now been processed so we can follow the chain and convert the object
    code.                                                                   */

  if ((ak_fno < 0) && (ak_itype_ptr != 0)) {
    if (debug)
      printf("Converting extended AK I-type object code\n");

    /* This may span multiple blocks. Allocate a temporary buffer, read
         the object code, perform the conversion and then write it back.    */

    itype_object = malloc(ak_itype_len);

    grp_offset = ak_itype_ptr;
    p = itype_object;
    bytes_remaining = ak_itype_len;
    do {
      if (debug)
#ifndef __LP64__      
        printf("Read block at %012LX\n", grp_offset);
#else
        printf("Read block at %012lu\n", grp_offset);
#endif        

      if (!read_block(sf, grp_offset, group_bytes, (char*)p)) {
        printf("Read error converting AK I-type\n");
        goto exit_process_file;
      }

      used_bytes = buffer.dh_itype_node.used_bytes;
      next_node = buffer.dh_itype_node.next;
      Reverse2(used_bytes);
      Reverse4(next_node);
      if (!converting_to_inverse) {
        used_bytes = buffer.dh_itype_node.used_bytes;
        next_node = buffer.dh_itype_node.next;
      }

      n = min(bytes_remaining, used_bytes - offsetof(DH_ITYPE_NODE, data));
      memcpy(p, buffer.dh_itype_node.data, n);
      p += n;
      bytes_remaining -= n;
    } while ((grp_offset = next_node) != 0);

    convert_object((OBJECT_HEADER*)itype_object, ak_itype_len, TRUE);

    grp_offset = ak_itype_ptr;
    p = itype_object;
    bytes_remaining = ak_itype_len;
    do {
      if (debug)
#ifndef __LP64__      
        printf("Update block at %012LX\n", grp_offset); 
#else
        printf("Update block at %012lu\n", grp_offset); 
#endif
      if (!read_block(sf, grp_offset, group_bytes, (char*)p)) {
        printf("Read error replacing converted AK I-type\n");
        goto exit_process_file;
      }

      used_bytes = buffer.dh_itype_node.used_bytes;
      next_node = buffer.dh_itype_node.next;
      Reverse2(used_bytes);
      Reverse4(next_node);
      if (!converting_to_inverse) {
        used_bytes = buffer.dh_itype_node.used_bytes;
        next_node = buffer.dh_itype_node.next;
      }

      n = min(bytes_remaining, used_bytes - offsetof(DH_ITYPE_NODE, data));
      memcpy(buffer.dh_itype_node.data, p, n);
      p += n;
      bytes_remaining -= n;

      if (!write_block(sf, grp_offset, group_bytes, (char*)p)) {
        printf("Write error replacing converted AK I-type\n");
        goto exit_process_file;
      }
    } while ((grp_offset = next_node) != 0);
  }

  /* Rewrite primary subfile header with corrected magic number */

  if (converting_to_inverse)
    convert_primary_header();
  header.magic = tgt_magic;

  if (!write_block(PRIMARY_SUBFILE, 0, DH_HEADER_SIZE, (char*)&header)) {
    printf("Write error in primary subfile header\n");
    goto exit_process_file;
  }

exit_process_file_ok:
  status = 0;

exit_process_file:
  for (sf = 0; sf < MAX_SUBFILES; sf++) {
    if (fu[sf] >= 0) {
      close(fu[sf]);
      fu[sf] = -1;
    }
  }

  return status;
}

/* ======================================================================
   open_subfile()  -  Open a subfile                                      */

bool open_subfile(char* filename, int16_t sf) {
  char path[160 + 1];
  
  // 17Oct25 mab Change dyn file prefix to %
  sprintf(path, "%s%c%%%d", filename, DS, (int)sf);
  fu[sf] = open(path, (int)(O_RDWR | O_BINARY), default_access);
  return (fu[sf] >= 0);
}

/* ======================================================================
   read_block()  -  Read a data block                                     */

bool read_block(int16_t sf, int64 offset, int16_t bytes, char* buff) {
  if (Seek(fu[sf], offset, SEEK_SET) < 0) {
    return FALSE;
  }

  if (read(fu[sf], buff, bytes) != bytes) {
    return FALSE;
  }

  return TRUE;
}

/* ======================================================================
   write_block()  -  Write a data block                                   */

bool write_block(int16_t sf, int64 offset, int16_t bytes, char* buff) {
  if (Seek(fu[sf], offset, SEEK_SET) < 0) {
    return FALSE;
  }

  if (write(fu[sf], buff, bytes) != bytes) {
    return FALSE;
  }

  return TRUE;
}

/* ======================================================================
   write_header()  -  Write primary subfile header                        */

bool write_header() {
  if (!write_block(PRIMARY_SUBFILE, 0, DH_HEADER_SIZE, (char*)&header)) {
    perror("Error writing primary subfile header");
    return FALSE;
  }

  return TRUE;
}

/* ======================================================================
   process_object_file()  -  Process an object file                       */

int process_object_file(char* filename) {
  int status = 1;
  int objfu = -1;
  int fsize;
  OBJECT_HEADER* obj = NULL;

  objfu = open(filename, (int)(O_RDWR | O_BINARY), default_access);
  if (objfu < 0)
    goto exit_process_object_file;

  fsize = filelength(objfu);
  obj = malloc(fsize);
  if (obj == NULL) {
    printf("Cannot allocate memory for %s\n", filename);
    goto exit_process_object_file;
  }

  if (read(objfu, (u_char*)obj, fsize) != fsize) {
    printf("Cannot read %s\n", filename);
    goto exit_process_object_file;
  }

  if (!convert_object(obj, fsize, FALSE)) {
    printf("Error converting %s\n", filename);
    goto exit_process_object_file;
  }

  if (Seek(objfu, 0, SEEK_SET) < 0) {
    printf("Seek error in %s\n", filename);
    goto exit_process_object_file;
  }

  if (write(objfu, (u_char*)obj, fsize) != fsize) {
    printf("Write error in %s\n", filename);
    goto exit_process_object_file;
  }

  status = 0;

exit_process_object_file:
  if (objfu >= 0)
    close(objfu);
  if (obj != NULL)
    free(obj);
  return status;
}

/* ======================================================================
   swap2()                                                                */

int16_t swap2(int16_t data) {
  union {
    int16_t val;
    unsigned char chr[2];
  } in, out;

  in.val = data;
  out.chr[0] = in.chr[1];
  out.chr[1] = in.chr[0];
  return out.val;
}

/* ======================================================================
   swap4()                                                                */

int32_t swap4(int32_t data) {
  union {
    int32_t val;
    unsigned char chr[4];
  } in, out;

  in.val = data;
  out.chr[0] = in.chr[3];
  out.chr[1] = in.chr[2];
  out.chr[2] = in.chr[1];
  out.chr[3] = in.chr[0];
  return out.val;
}

/* ======================================================================
   swap8()                                                                */

int32_t swap8(int64 data) {
  union {
    int64 val;
    unsigned char chr[8];
  } in, out;

  in.val = data;
  out.chr[0] = in.chr[7];
  out.chr[1] = in.chr[6];
  out.chr[2] = in.chr[5];
  out.chr[3] = in.chr[4];
  out.chr[4] = in.chr[3];
  out.chr[5] = in.chr[2];
  out.chr[6] = in.chr[1];
  out.chr[7] = in.chr[0];
  return out.val;
}

/* ======================================================================
   convert_object()                                                       */

bool convert_object(OBJECT_HEADER* obj, int bytes, bool skip_fields) {
  u_char tgt_magic;
  u_char expected_magic;
  int16_t i;
  u_char* p;

  if (skip_fields) /* AK has dictionary fields on front of object code */
  {
    p = (u_char*)obj;
    for (i = 15; i--;) /* Skip 15 field marks */
    {
      p = (u_char*)strchr((char*)p, (char)FIELD_MARK);
      if (p == NULL) {
        printf("Invalid AK object code\n");
        return FALSE;
      }
      p++;
    }
    obj = (OBJECT_HEADER*)p;
  }

  tgt_magic = (converting_to_inverse) ? HDR_MAGIC_INVERSE : HDR_MAGIC;
  if (obj->magic == tgt_magic)
    return TRUE;

  expected_magic = (converting_to_inverse) ? HDR_MAGIC : HDR_MAGIC_INVERSE;
  if (obj->magic != expected_magic) {
    printf("Invalid magic number (%d) in object code\n", obj->magic);
    return FALSE;
  }

  /* ========== Object header */

  Reverse4(obj->id);
  Reverse4(obj->start_offset);
  Reverse2(obj->args);
  Reverse2(obj->no_vars);
  Reverse2(obj->stack_depth);
  Reverse4(obj->sym_tab_offset);
  Reverse4(obj->line_tab_offset);
  Reverse4(obj->object_size);
  Reverse2(obj->flags);
  Reverse4(obj->compile_time);
  obj->magic = tgt_magic;

  return TRUE;
}

/* ======================================================================
   convert_primary_header()  -  Convert in-memory file header             */

void convert_primary_header() {
  if (debug)
    printf("Converting primary subfile header\n");
  Reverse4(header.group_size);
  Reverse4(header.params.modulus);
  Reverse4(header.params.min_modulus);
  Reverse4(header.params.big_rec_size);
  Reverse2(header.params.split_load);
  Reverse2(header.params.merge_load);
  Reverse4(header.params.load_bytes);
  Reverse4(header.params.mod_value);
  Reverse2(header.params.longest_id);
  Reverse2(header.params.extended_load_bytes);
  Reverse4(header.params.free_chain);
  Reverse2(header.flags);
  Reverse4(header.ak_map);
  memset(&(header.stats), 0, sizeof(struct FILESTATS));
  Reverse4(header.jnl_fno);
  Reverse4(header.creation_timestamp);
  Reverse8(header.record_count);
  Reverse4(header.user_hash);
}

/* END-CODE */
