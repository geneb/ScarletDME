/* OP_DIO2.C
 * Disk i/o opcodes part 2 (Information opcodes)
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
 * 09Jan22 gwb Fixed some format specifier warnings.
 *
 * 29Feb20 gwb Changed LONG_MAX to INT32_MAX.  When building for a 64 bit 
 *             platform, the LONG_MAX constant overflows the size of the
 *             int32_t variable type.  This change needed to be made across
 *             the entire code base.
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 *
 * 22Feb20 gwb Converted some problematic instances of sprintf() to 
 *             snprintf().
 * 
 * START-HISTORY (OpenQM):
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 30 Aug 06  2.4-12 Added FL$NO.RESIZE.
 * 04 Apr 06  2.4-1 Added FL$PRI.BYTES and FL$OVF.BYTES to op_fileinfo().
 * 28 Mar 06  2.3-9 Sequential files can now exceed 2Gb.
 * 17 Mar 06  2.3-8 Added fileinfo(1015) to get record count.
 * 19 Dec 05  2.3-3 Added fileinfo(fl$id).
 * 02 Dec 05  2.2-18 Allow spaces in valid_name().
 * 11 Oct 05  2.2-14 Added FILEINFO(FL$AKPATH).
 * 31 Aug 05  2.2-9 Use local buffer in fullpath() on NIX systems to ensure that
 *                  it is PATH_MAX bytes without having to increase the general
 *                  MAX_PATHNAME_LEN to this value (which is 4096 on Linux,
 *                  causing the file table to be enormous).
 * 30 Aug 05  2.2-9 Added OSPATH(OS$OPEN).
 * 20 Jun 05  2.2-1 Enhanced FILEINFO(FL$EXCLUSIVE) to flush cache of other
 *                  processes, wait, and try again if unsuccessful.
 * 17 Jun 05  2.2-1 Added FILEINFO(FL$JNL.FNO).
 * 30 May 05  2.2-0 0363 FILEINFO(FL$STATS) was not cleaning up stack correctly.
 * 25 May 05  2.2-0 Added FILEINFO(FL$FILENO).
 * 18 May 05  2.2-0 0355 make_path() was not handling leading directory
 *                  delimiter correctly.
 * 29 Oct 04  2.0-9 Added VFS.
 * 11 Oct 04  2.0-5 Added fileinfo(FL$TRG_MODES).
 * 01 Oct 04  2.0-3 Added OSPATH(OS$MKDIR).
 * 30 Sep 04  2.0-3 Handle network files in FILEINFO().
 * 20 Sep 04  2.0-2 Use message handler.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 *  op_fileinfo        FILEINFO
 *  op_sysdir          SYSDIR
 *  op_ospath          OSPATH
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include "keys.h"
#include "dh_int.h"
#include "header.h"

#include <time.h>
#include <stdint.h>

Private bool valid_name(char* p);
Private bool attach(char* path);
Private STRING_CHUNK* get_file_status(FILE_VAR* fvar);

bool make_path(char* tgt);
bool FDS_open(DH_FILE* dh_file, int16_t subfile);

/* ======================================================================
   op_fileinfo()  -  Return information about an open file                */

void op_fileinfo() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  Key                           | Information                 |
     |--------------------------------|-----------------------------| 
     |  ADDR to file variable         |                             |
     |================================|=============================|

 Key values:           Action                         Returns
     0 FL_OPEN         Test if is open file variable  True/False
     1 FL_VOCNAME      Get VOC name of file           VOC name
     2 FL_PATH         Get file pathname              Pathname
     3 FL_TYPE         Check file type                DH:         FL_TYPE_DH  (3)
                                                      Directory:  FL_TYPE_DIR (4)
                                                      Sequential: FL_TYPE_SEQ (5)
     5 FL_MODULUS      File modulus                   Modulus value
     6 FL_MINMOD       Minimum modulus                Minimum modulus value
     7 FL_GRPSIZE      Group size                     Group size
     8 FL_LARGEREC     Large record size              Large record size
     9 FL_MERGE        Merge load percentage          Merge load
    10 FL_SPLIT        Split load percentage          Split load
    11 FL_LOAD         Current load percentage        Current load
    13 FL_AK           File has AK indices?           Boolean
    14 FL_LINE         Number of next line            Line number
  1000 FL_LOADBYTES    Current load in bytes          Current load bytes
  1001 FL_READONLY     Read only file?                Boolean
  1002 FL_TRIGGER      Get trigger function name      Call name
  1003 FL_PHYSBYTES    Physical file size             Size in bytes, excl indices
  1004 FL_VERSION      File version
  1005 FL_STATS_QUERY  Query file stats status        Boolean
  1006 FL_SEQPOS       File position                  File offset
  1007 FL_TRG_MODES    Get trigger modes              Mode mask
  1008 FL_NOCASE       File uses case insensitive ids?  Boolean
  1009 FL_FILENO       Return internal file number    File number
  1010 FL_JNL_FNO      Return journalling file no     File no, zero if not journalling
  1011 FL_AKPATH       Returns AK subfile location    Pathname of directory
  1012 FL_ID           Id of last record read         Id
  1013 FL_STATUS       As STATUS statement            Dynamic array
  1014 FL_MARK_MAPPING Is mark mapping enabled?       Boolean
  1015 FL_RECORD_COUNT Approximate record count
  1016 FL_PRI_BYTES    Primary subfile size in bytes
  1017 FL_OVF_BYTES    Overflow subfile size in bytes
  1018 FL_NO_RESIZE    Resizing inhibited?
  1019 FL_UPDATE       Update counter
  1020 FL_ENCRYPTED    File uses encryption?          Boolean
 10000 FL_EXCLUSIVE    Set exclusive access           Successful?
 10001 FL_FLAGS        Fetch file flags               File flags
 10002 FL_STATS_ON     Turn on file statistics
 10003 FL_STATS_OFF    Turn off file statistics
 10004 FL_STATS        Return file statistics
 10005 FL_SETRDONLY    Set file as read only
 */

  int16_t key;
  DESCRIPTOR* descr;
  FILE_VAR* fvar;
  DH_FILE* dh_file;
  char* p = NULL;
  int32_t n = 0;
  FILE_ENTRY* fptr;
  OSFILE fu;
  bool dynamic;
  bool internal;
  int32_t* q;
  STRING_CHUNK* str;
  int16_t i;
  double floatnum;
  u_char ftype;
  int64 n64;

  /* Get action key */

  descr = e_stack - 1;
  GetInt(descr);
  key = (int16_t)(descr->data.value);
  k_pop(1);

  /* Get file variable */

  descr = e_stack - 1;
  while (descr->type == ADDR) {
    descr = descr->data.d_addr;
  }

  if (key == FL_OPEN) /* Test if file is open */
  {
    n = (descr->type == FILE_REF);
  } else {
    if (descr->type != FILE_REF)
      k_error(sysmsg(1200));

    fvar = descr->data.fvar;
    ftype = fvar->type;
    if (ftype == NET_FILE) /* Network file */
    {
      str = net_fileinfo(fvar, key);
      k_dismiss();
      InitDescr(e_stack, STRING);
      (e_stack++)->data.str.saddr = str;
      return;
    }

    fptr = FPtr(fvar->file_id);

    dynamic = (ftype == DYNAMIC_FILE);
    if (dynamic)
      dh_file = fvar->access.dh.dh_file;

    internal = ((process.program.flags & HDR_INTERNAL) != 0);
    switch (key) {
      case FL_VOCNAME: /* 1  VOC name of file */
        if (fvar->voc_name != NULL)
          p = fvar->voc_name;
        else
          p = "";
        goto set_string;

      case FL_PATH: /* 2  File pathname */
        p = (char*)(fptr->pathname);
        goto set_string;

      case FL_TYPE: /* 3  File type */
        /* !!FVAR_TYPES!! */
        switch (ftype) {
          case DYNAMIC_FILE:
            n = FL_TYPE_DH;
            break;
          case DIRECTORY_FILE:
            n = FL_TYPE_DIR;
            break;
          case SEQ_FILE:
            n = FL_TYPE_SEQ;
            break;
        }
        break;

      case FL_MODULUS: /* 5  Modulus of file */
        if (dynamic)
          n = fptr->params.modulus;
        break;

      case FL_MINMOD: /* 6  Minimum modulus of file */
        if (dynamic)
          n = fptr->params.min_modulus;
        break;

      case FL_GRPSIZE: /* 7  Group size of file */
        if (dynamic)
          n = dh_file->group_size / DH_GROUP_MULTIPLIER;
        break;

      case FL_LARGEREC: /* 8  Large record size */
        if (dynamic)
          n = fptr->params.big_rec_size;
        break;

      case FL_MERGE: /* 9  Merge load percentage */
        if (dynamic)
          n = fptr->params.merge_load;
        break;

      case FL_SPLIT: /* 10  Split load percentage */
        if (dynamic)
          n = fptr->params.split_load;
        break;

      case FL_LOAD: /* 11  Load percentage */
        if (dynamic) {
          n = DHLoad(fptr->params.load_bytes, dh_file->group_size,
                     fptr->params.modulus);
        }
        break;

      case FL_AK: /* 13  File has AKs? */
        if (dynamic)
          n = (dh_file->ak_map != 0);
        break;

      case FL_LINE: /* 14  Sequential file line position */
        if (ftype == SEQ_FILE) {
          n64 = fvar->access.seq.sq_file->line;
          if (n64 > INT32_MAX) {
            floatnum = (double)n64;
            goto set_float;
          }
          n = (int32_t)n64;
        }
        break;

      case FL_LOADBYTES: /* 1000  Load bytes */
        if (dynamic) {
          floatnum = (double)(fptr->params.load_bytes);
          goto set_float;
        }
        break;

      case FL_READONLY: /* 1001  Read-only? */
        n = ((fvar->flags & FV_RDONLY) != 0);
        break;

      case FL_TRIGGER: /* 1002  Trigger function name */
        if (dynamic) {
          p = dh_file->trigger_name;
          goto set_string;
        }
        break;

      case FL_PHYSBYTES: /* 1003  Physical file size */
        switch (ftype) {
          case DIRECTORY_FILE:
            floatnum = (double)dir_filesize(fvar);
            break;
          case DYNAMIC_FILE:
            floatnum = (double)dh_filesize(dh_file, PRIMARY_SUBFILE) +
                       dh_filesize(dh_file, OVERFLOW_SUBFILE);
            break;
          case SEQ_FILE:
            fu = fvar->access.seq.sq_file->fu;
            floatnum = (double)(ValidFileHandle(fu) ? filelength64(fu) : -1);
            break;
        }
        goto set_float;

      case FL_VERSION: /* 1004  File version */
        if (dynamic)
          n = dh_file->file_version;
        break;

      case FL_STATS_QUERY: /* 1005  File statistics enabled? */
        if (dynamic)
          n = (fptr->stats.reset != 0);
        break;

      case FL_SEQPOS: /* 1006  Sequential file offset */
        if (ftype == SEQ_FILE) {
          n64 = fvar->access.seq.sq_file->posn;
          if (n64 > INT32_MAX) {
            floatnum = (double)n64;
            goto set_float;
          }
          n = (int32_t)n64;
        }
        break;

      case FL_TRG_MODES: /* 1007  Trigger modes */
        if (dynamic)
          n = dh_file->trigger_modes;
        break;

      case FL_NOCASE: /* 1008  Case insensitive ids? */
        switch (ftype) {
          case DIRECTORY_FILE:
          case DYNAMIC_FILE:
            n = (fptr->flags & DHF_NOCASE) != 0;
            break;
        }
        break;

      case FL_FILENO: /* 1009  Internal file number */
        n = fvar->file_id;
        break;

      case FL_JNL_FNO: /* 1010  Journalling file number */
        break;

      case FL_AKPATH: /* 1011  AK subfile pathname */
        if (dynamic) {
          p = dh_file->akpath;
          goto set_string;
        }
        break;

      case FL_ID: /* 1012  Id of last record read */
        k_dismiss();
        k_put_string(fvar->id, fvar->id_len, e_stack);
        e_stack++;
        return;

      case FL_STATUS: /* 1013  STATUS array */
        str = get_file_status(fvar);
        k_dismiss();
        InitDescr(e_stack, STRING);
        (e_stack++)->data.str.saddr = str;
        return;

      case FL_MARK_MAPPING: /* 1014  Mark mapping enabled? */
        if (ftype == DIRECTORY_FILE)
          n = fvar->access.dir.mark_mapping;
        break;

      case FL_RECORD_COUNT: /* 1015  Approximate record count */
        if (dynamic) {
          floatnum = (double)(fptr->record_count);
          goto set_float;
        } else
          n = -1;

      case FL_PRI_BYTES: /* 1016  Physical size of primary subfile */
        if (dynamic) {
          floatnum = (double)dh_filesize(dh_file, PRIMARY_SUBFILE);
          goto set_float;
        }
        break;

      case FL_OVF_BYTES: /* 1017  Physical size of overflow subfile */
        if (dynamic) {
          floatnum = (double)dh_filesize(dh_file, OVERFLOW_SUBFILE);
          goto set_float;
        }
        break;

      case FL_NO_RESIZE: /* 1018  Resizing inhibited? */
        if (dynamic)
          n = ((fptr->flags & DHF_NO_RESIZE) != 0);
        break;

      case FL_UPDATE: /* 1019  File update counter */
        n = (int32_t)(fptr->upd_ct);
        break;

      case FL_ENCRYPTED: /* 1020  File uses encryption? */
        /* Recognised but returns default zero */
        break;

      case FL_EXCLUSIVE: /* 10000  Set exclusive access mode */
        if (internal) {
          /* To gain exclusive access to a file it must be open only to this
             process (fptr->ref_ct = 1) and must not be open more than once
             in this process.  The latter condition only affects dynamic files
             as other types produce multiply referenced file table entries.
             We need to ensure a dynamic file is only open once so that when
             we close the file we really are going to kill off the DH_FILE
             structure. This is essential, for example, in AK creation where
             the DH_FILE structure has to change its size.                   */

          flush_dh_cache(); /* Ensure we are not stopped by a cached
                                  reference from our own process.       */
          n = FALSE;
          for (i = 0; i < 6; i++) {
            StartExclusive(FILE_TABLE_LOCK, 37);
            if ((fptr->ref_ct == 1) &&
                ((ftype != DYNAMIC_FILE) || (dh_file->open_count == 1))) {
              fptr->ref_ct = -1;
              fptr->fvar_index = fvar->index;
              n = TRUE;
            }
            EndExclusive(FILE_TABLE_LOCK);

            if (n)
              break;

            if (i == 0) /* First attempt */
            {
              /* Cannot gain exclusive access. Maybe some other process has
                 the file in its DH cache. Fire an EVT_FLUSH_CACHE event to
                 all processes to see if this clears the problem. We then
                 continue trying for a short time until either we get the
                 required access or we reach our retry count.              */
              raise_event(EVT_FLUSH_CACHE, -1);
            }

            Sleep(500); /* Pause for something to happen */
          }
        }
        break;

      case FL_FLAGS: /* 10001  File flags */
        if (dynamic && internal)
          n = (int32_t)(dh_file->flags);
        break;

      case FL_STATS_ON: /* 10002  Enable file statistics */
        if (dynamic && internal) {
          memset((char*)&(fptr->stats), 0, sizeof(struct FILESTATS));
          fptr->stats.reset = qmtime();
        }
        break;

      case FL_STATS_OFF: /* 10003  Disable file statistics */
        if (dynamic && internal)
          fptr->stats.reset = 0;
        break;

      case FL_STATS: /* 10004  Return file statistics data */
        if (dynamic && internal) {
          str = NULL;
          ts_init(&str, 5 * FILESTATS_COUNTERS);
          for (i = 0, q = (int32_t*)&(fptr->stats.reset);
               i < FILESTATS_COUNTERS; i++, q++) {
            ts_printf("%d\xfe", *q);
          }
          (void)ts_terminate();
          k_dismiss(); /* 0363 */
          InitDescr(e_stack, STRING);
          (e_stack++)->data.str.saddr = str;
          return;
        }
        break;

      case FL_SETRDONLY: /* 10005  Set read-only */
        if (internal) {
          fvar->flags |= FV_RDONLY;
          if (dynamic)
            dh_file->flags |= DHF_RDONLY;
        }
        break;

      default:
        k_error(sysmsg(1010));
    }
  }

  /* Set integer return value on stack */

set_integer:
  k_dismiss();
  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = n;
  return;

  /* Set string return value on stack */

set_string:
  k_dismiss();
  k_put_c_string(p, e_stack);
  e_stack++;
  return;

set_float:
  if (floatnum <= (double)INT32_MAX) {
    n = (int32_t)floatnum;
    goto set_integer;
  }

  k_dismiss();
  InitDescr(e_stack, FLOATNUM);
  (e_stack++)->data.float_value = floatnum;
  return;
}

/* ======================================================================
   op_sysdir()  -  Returns system directory name                          */

void op_sysdir() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |                                | System directory name       |
     |================================|=============================|
*/

  k_put_c_string((char*)(sysseg->sysdir), e_stack);
  e_stack++;
}

/* ======================================================================
   op_ospath()  -  OS file system actions                                 */

void op_ospath() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  Key                           | Information                 |
     |--------------------------------|-----------------------------| 
     |  Pathname string               |                             |
     |================================|=============================|

 Key values:           Action                             Returns
     0  OS_PATHNAME    Test if valid pathname             True/False
     1  OS_FILENAME    Test if valid filename             True/False
                       or directory file record name
     2  OS_EXISTS      Test if file exists                True/False
     3  OS_UNIQUE      Make a unique file name            Name
     4  OS_FULLPATH    Return full pathname               Name
     5  OS_DELETE      Delete file                        Success/Failure
     6  OS_CWD         Get current working directory      Pathname
     7  OS_DTM         Return date/time modified          DTM value
     8  OS_FLUSH_CACHE Flush DH file cache                -
     9  OS_CD          Change working directory           Success/Failure
    10  OS_MAPPED_NAME Map a directory file name          Mapped name
    11  OS_OPEN        Check if path is an open file      True/False
    12  OS_DIR         Return content of directory        Filenames
    13  OS_MKDIR       Make a directory                   True/False
    14  OS_MKPATH      Make a directory path              True/False

 Pathnames with lengths outside the range 1 to MAX_PATHNAME_LEN return
 0 regardless of the action key.

 */

  int32_t status = 0;
  int16_t key;
  DESCRIPTOR* descr;
  char path[MAX_PATHNAME_LEN + 1];
  int16_t path_len;
  char name[MAX_PATHNAME_LEN + 1];
  char* p;
  char* q;
  STRING_CHUNK* head;
  int file_id;
  FILE_ENTRY* fptr;
  struct stat stat_buff;
  DIR* dfu;
  struct dirent* dp;
  int32_t n;

  /* Get action key */

  descr = e_stack - 1;
  GetInt(descr);
  key = (int16_t)(descr->data.value);
  k_pop(1);

  /* Get pathname */

  descr = e_stack - 1;
  path_len = k_get_c_string(descr, path, MAX_PATHNAME_LEN);
  k_dismiss();
  if (path_len < 0)
    goto set_status;
#ifdef CASE_INSENSITIVE_FILE_SYSTEM
  UpperCaseString(path);
#endif

  switch (key) {
    case OS_PATHNAME: /* Test if valid pathname */
      p = path;
      if (*p == '/')
        p++;

      do {
        q = strchr(p, '/');
        if (q != NULL)
          *q = '\0';
        if (!valid_name(p))
          goto set_status;
        p = q + 1;
      } while (q != NULL);
      status = 1;

      break;

    case OS_FILENAME: /* Test if valid pathname */
      status = (int32_t)valid_name(path);
      break;

    case OS_EXISTS: /* Test if file exists */
      status = !access(path, 0);
      break;

    case OS_UNIQUE: /* Make unique file name. Path variable holds directory name */
      n = (time(NULL) * 10) & 0xFFFFFFFL;
      do {
         /* converted to snprintf() -gwb 22Feb20 */
        if (snprintf(name, MAX_PATHNAME_LEN + 1,"%s\\D%07d", path, n) >= (MAX_PATHNAME_LEN +1 )) {
            /* TODO: this should be logged to a file with more details */
            k_error("Overflowed path/filename length in delete_path()!");
            goto exit_op_pathinfo;
        }
        n--;
      } while (!access(name, 0));
      sprintf(name, "D%07d", n);
      k_put_c_string(name, e_stack);
      e_stack++;
      goto exit_op_pathinfo;

    case OS_FULLPATH: /* Expand path to full OS pathname */
      fullpath(name, path);
      k_put_c_string(name, e_stack);
      e_stack++;
      goto exit_op_pathinfo;

    case OS_DELETE:
      flush_dh_cache();
      status = (int32_t)delete_path(path);
      break;

    case OS_CWD:
      (void)getcwd(name, MAX_PATHNAME_LEN);
#ifdef CASE_INSENSITIVE_FILE_SYSTEM
      UpperCaseString(name);
#endif
      k_put_c_string(name, e_stack);
      e_stack++;
      goto exit_op_pathinfo;

    case OS_DTM:
      if (stat(path, &stat_buff) == 0)
        status = stat_buff.st_mtime;
      break;

    case OS_FLUSH_CACHE:
      flush_dh_cache();
      break;

    case OS_CD:
      status = attach(path);
      break;

    case OS_MAPPED_NAME: /* Map a directory file record name */
      (void)map_t1_id(path, strlen(path), name);
      k_put_c_string(name, e_stack);
      e_stack++;
      goto exit_op_pathinfo;

    case OS_OPEN:
      fullpath(name, path);
      for (file_id = 1; file_id <= sysseg->used_files; file_id++) {
        fptr = FPtr(file_id);
        if ((fptr->ref_ct != 0) &&
            (strcmp((char*)(fptr->pathname), name) == 0)) {
          status = TRUE;
          break;
        }
      }
      break;

    case OS_DIR:
      head = NULL;
      ts_init(&head, 1024);
      if ((dfu = opendir(path)) != NULL) {
        if (path[path_len - 1] == DS)
          path[path_len - 1] = '\0';

        while ((dp = readdir(dfu)) != NULL) {
          if (strcmp(dp->d_name, ".") == 0)
            continue;
          if (strcmp(dp->d_name, "..") == 0)
            continue;
          /* converted to snprintf() -gwb 22Feb20 */
          if (snprintf(name, MAX_PATHNAME_LEN + 1,"%s%c%s", path, DS, 
                  dp->d_name) >= (MAX_PATHNAME_LEN + 1)) {
            /* TODO: this should be logged to a file with more details */
            k_error("Overflowed path/filename length in delete_path()!");
            goto exit_op_pathinfo;

          }
          if (stat(name, &stat_buff))
            continue;

          strcpy(name + 1, dp->d_name);
#ifdef CASE_INSENSITIVE_FILE_SYSTEM
          UpperCaseString(name + 1);
#endif
          if (stat_buff.st_mode & S_IFDIR) {
            name[0] = 'D';
            if (head != NULL)
              ts_copy_byte(FIELD_MARK);
            ts_copy_c_string(name);
          } else if (stat_buff.st_mode & S_IFREG) {
            name[0] = 'F';
            if (head != NULL)
              ts_copy_byte(FIELD_MARK);
            ts_copy_c_string(name);
          }
        }

        closedir(dfu);
      }
      ts_terminate();
      InitDescr(e_stack, STRING);
      (e_stack++)->data.str.saddr = head;
      goto exit_op_pathinfo;

    case OS_MKDIR:
      status = !MakeDirectory(path);
      break;

    case OS_MKPATH:
      status = make_path(path);
      break;

    default:
      k_error(sysmsg(1010));
  }

set_status:

  /* Set status value on stack */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = status;

exit_op_pathinfo:
  return;
}

/* ======================================================================
   op_osrename()  -  Rename a file                                        */

void op_osrename() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  New pathname                  | 1 = ok, 0 = error           |
     |--------------------------------|-----------------------------| 
     |  Old pathname                  |                             |
     |================================|=============================|

 */

  DESCRIPTOR* descr;
  char old_path[MAX_PATHNAME_LEN + 1];
  char new_path[MAX_PATHNAME_LEN + 1];
  int16_t path_len;

  process.status = 0;
  process.os_error = 0;

  /* Get new pathname */

  descr = e_stack - 1;
  path_len = k_get_c_string(descr, new_path, MAX_PATHNAME_LEN);
  if (path_len < 0) {
    process.status = ER_INVAPATH;
    goto exit_osrename;
  }
#ifdef CASE_INSENSITIVE_FILE_SYSTEM
  UpperCaseString(new_path);
#endif

  /* Get old pathname */

  descr = e_stack - 2;
  path_len = k_get_c_string(descr, old_path, MAX_PATHNAME_LEN);
  if (path_len < 0) {
    process.status = ER_INVAPATH;
    goto exit_osrename;
  }
#ifdef CASE_INSENSITIVE_FILE_SYSTEM
  UpperCaseString(old_path);
#endif

  /* Check old path exists */

  if (access(old_path, 0)) {
    process.os_error = OSError;
    process.status = ER_FNF;
    goto exit_osrename;
  }

  if (rename(old_path, new_path)) {
    process.os_error = OSError;
    process.status = ER_FAILED;
    goto exit_osrename;
  }

exit_osrename:
  k_dismiss();
  k_dismiss();

  /* Set status value on stack */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = (process.status == 0);

  return;
}

/* ======================================================================
   valid_name()  -  Test for valid OS file name                           */

Private bool valid_name(char* p) {
  bool status = FALSE;

  if ((strcmp(p, ".") == 0) || (strcmp(p, "..") == 0))
    return TRUE;

  if (strlen(p) > 0) {
    while (((*p == ' ') || IsGraph(*p)) &&
           (strchr(df_restricted_chars, *p) == NULL)) {
      p++;
    }
    status = (*p == '\0');
  }

  return status;
}

/* ======================================================================
   delete_path()  -  Delete DOS file/directory path                       */

bool delete_path(char* path) {
  bool status = FALSE;
  DIR* dfu;
  struct dirent* dp;
  struct stat stat_buf;
  char parent_path[MAX_PATHNAME_LEN + 1];
  int parent_len;
  char sub_path[MAX_PATHNAME_LEN + 1];

  /* Check path exists and get type information */

  if (stat(path, &stat_buf) != 0)
    goto exit_delete_path;

  if (stat_buf.st_mode & S_IFDIR) /* It's a directory */
  {
    dfu = opendir(path);
    if (dfu == NULL)
      goto exit_delete_path;

    strcpy(parent_path, path);
    parent_len = strlen(parent_path);
    if (parent_path[parent_len - 1] == DS)
      parent_path[parent_len - 1] = '\0';

    while ((dp = readdir(dfu)) != NULL) {
      if (strcmp(dp->d_name, ".") == 0)
        continue;
      if (strcmp(dp->d_name, "..") == 0)
        continue;
      /* converted to snprintf() -gwb 22Feb20 */
      if (snprintf(sub_path, MAX_PATHNAME_LEN + 1,"%s%c%s", parent_path, DS, 
            dp->d_name) >= (MAX_PATHNAME_LEN + 1)) {
            /* TODO: this should be logged to a file with more details */
            k_error("Overflowed path/filename length in delete_path()!");
            goto exit_delete_path;
      }
      if (!delete_path(sub_path))
        goto exit_delete_path;
      ;
    }
    closedir(dfu);

    if (rmdir(path) != 0)
      goto exit_delete_path;             /* Delete the directory */
  } else if (stat_buf.st_mode & S_IFREG) /* It's a file */
  {
    if (remove(path) != 0)
      goto exit_delete_path;
  }

  status = TRUE;

exit_delete_path:
  return status;
}

/* ======================================================================
   attach()  -  Change working directory and, perhaps, drive.             */

Private bool attach(char* path) {
  char cwd[MAX_PATHNAME_LEN + 1];

  (void)getcwd(cwd, MAX_PATHNAME_LEN); /* Hang on to current dir */

  if (!chdir(path))
    return TRUE; /* Success */

  (void)chdir(cwd);

  return FALSE;
}

/* ======================================================================
   fullpath()  -  Map a pathname to its absolute form                     */

bool fullpath(char* path, /* Out (can be same as input path buffer) */
              char* name) /* In */
{
  bool ok;

  char buff[PATH_MAX + 1];

  /* realpath() requires that the buffer is PATH_MAX bytes even for short
    pathnames. Use a temporary buffer and then copy the data back to the
    caller's buffer.                                                      */

  ok = (qmrealpath(name, buff) != NULL);

  strcpy(path, buff);

  return ok;
}

/* ======================================================================
   make_path()  -  Recursive mkdir to make directory path                 */

bool make_path(char* tgt) {
  char path[MAX_PATHNAME_LEN + 1];
  char new_path[MAX_PATHNAME_LEN + 1];
  char* p;
  char* q;
  struct stat statbuf;

  strcpy(path, tgt); /* Make local copy as we will use strtok() */

  p = path;

  q = new_path;

  if (*p == DS) /* 0355 */
  {
    *(q++) = DS;
    p++;
  }
  *q = '\0';

  while ((q = strtok(p, DSS)) != NULL) {
    strcat(new_path, q);

    if (stat(new_path, &statbuf)) /* Directory does not exist */
    {
      if (MakeDirectory(new_path) != 0)
        return FALSE;
    } else if (!(statbuf.st_mode & S_IFDIR)) /* Exists but not as a directory */
    {
      return FALSE;
    }

    strcat(new_path, DSS);
    p = NULL;
  }

  return TRUE;
}

/* ====================================================================== */

Private STRING_CHUNK* get_file_status(FILE_VAR* fvar) {
  STRING_CHUNK* str = NULL;
  u_char ftype;
  DH_FILE* dh_file;
  SQ_FILE* sq_file;
  int64 file_size = 0;
  int file_type_num = 0;
  bool is_seq = FALSE;
  char* path;
  int64 n64;
  struct stat statbuf;

  memset(&statbuf, 0, sizeof(statbuf));

  ftype = fvar->type;
  path = (char*)(FPtr(fvar->file_id)->pathname);

  /* !!FVAR_TYPES!! */
  switch (ftype) {
    case DYNAMIC_FILE:
      file_type_num = FL_TYPE_DH;
      dh_file = fvar->access.dh.dh_file;
      FDS_open(dh_file, PRIMARY_SUBFILE);
      fstat(dh_file->sf[PRIMARY_SUBFILE].fu, &statbuf);
      file_size = dh_filesize(dh_file, PRIMARY_SUBFILE) +
                  dh_filesize(dh_file, OVERFLOW_SUBFILE);
      break;

    case DIRECTORY_FILE:
      file_type_num = FL_TYPE_DIR;
      stat((char*)(FPtr(fvar->file_id)->pathname), &statbuf);
      file_size = dir_filesize(fvar);
      break;

    case SEQ_FILE:
      file_type_num = FL_TYPE_SEQ;
      is_seq = TRUE;
      sq_file = fvar->access.seq.sq_file;
      if (!(sq_file->flags & SQ_NOTFL))
        fstat(sq_file->fu, &statbuf);
      file_size = statbuf.st_size;
      path = sq_file->pathname;
      break;
  }

  ts_init(&str, 128);

  /* 1  File position */
  n64 = (is_seq) ? sq_file->posn : 0;
  ts_printf("%lld\xfe", n64);

  /* 2  At EOF? */
  ts_printf("%d\xfe", (is_seq) ? sq_file->posn == file_size : 0);

  /* 3  Not used */
  ts_printf("\xfe");

  /* 4  Bytes available to read */
  n64 = (is_seq) ? (file_size - sq_file->posn) : 0;
  ts_printf("%lld\xfe", n64);

  /* 5  File mode */
  ts_printf("%u\xfe", statbuf.st_mode & 0777);

  /* 6  File size */
  ts_printf("%lld\xfe", file_size);

  /*  7  Hard links */
  ts_printf("%u\xfe", statbuf.st_nlink);

  /*  8  UID of owner */
  ts_printf("%d\xfe", statbuf.st_uid);

  /*  9  GID of owner */
  ts_printf("%d\xfe", statbuf.st_gid);

  /* 10  Inode number */
  ts_printf("%u\xfe", statbuf.st_ino);

  /* 11  Device number */
  ts_printf("%u\xfe", statbuf.st_dev);

  /* 12  Not used */
  ts_printf("\xfe");

  /* 13  Time of last access */
  ts_printf("%d\xfe", statbuf.st_atime % 86400);

  /* 14  Date of last access */
  ts_printf("%d\xfe", (statbuf.st_atime / 86400) + 732);

  /* 15  Time of last modification */
  ts_printf("%d\xfe", statbuf.st_mtime % 86400);

  /* 16  Date of last modification */
  ts_printf("%d\xfe", (statbuf.st_mtime / 86400) + 732);

  /* 17 - 19 unused */
  ts_printf("\xfe\xfe\xfe");

  /* 20  Operating system file name */
  ts_printf("%s\xfe", path);

  /* 21  File type */
  ts_printf("%d", file_type_num);

  (void)ts_terminate();

  return str;
}

/* END-CODE */
