/* QMIDX.C
 * Index relocation tool
 * Copyright (c) 2005 Ladybridge Systems, All Rights Reserved
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
 * 03Sep25 gwb Fix for potential buffer overrun due to an insufficiently sized sprintf() target.
 *             git issue #82
 * 15Jan22 gwb Reformatted code.
 * 
 * 27Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 *
 * 25Feb20 gwb Converted sprintf() to snprintf(). 
 *             Updated K&R function declarations to ANSI.
 *        
 * START-HISTORY (OpenQM):
 * 11 Oct 05  2.2-14 New program.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * qmidx -d datapath           Delete indices
 * qmidx -m datapath {akpath}  Move indices to new location
 * qmidx -p datapath {akpath}  Set index path
 * qmidx -q datapath           Report path   [ Default action if no option ]
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
#include "revstamp.h"

#include <unistd.h>
#include <sys/shm.h>

#define Seek(fu, offset, whence) lseek(fu, offset, whence)

#define SF(ak) (ak + AK_BASE_SUBFILE)

char data_path[MAX_PATHNAME_LEN + 1] = "";
char ak_path[MAX_PATHNAME_LEN + 1] = "";

int pfu;          /* Primary subfile file handle */
DH_HEADER header; /* Primary subfile header */

#define BUFFER_SIZE 32768
char buffer[BUFFER_SIZE];

bool open_primary(void);
bool write_primary_header(void);
bool cross_check(char *path);
bool copy_indices(void);
bool yesno(void);
bool delete_path(char *path);
bool make_path(char *tgt);
bool attach_shared_memory(void);
void unbind_sysseg(void);
bool fullpath(char *path, char *name);

/* ====================================================================== */

int main(int argc, char *argv[]) {
  int status = 1;
  int arg;
  char mode;
  int16_t ak;
  char path[MAX_PATHNAME_LEN + 1];
  char old_akpath[MAX_PATHNAME_LEN + 1];
  struct stat stat_buf;

  printf(
      "[ QMIDX %s   Copyright, Ladybridge Systems, %s.  All rights reserved. "
      "]\n\n",
      QM_REV_STAMP, QM_COPYRIGHT_YEAR);

  /* Check command arguments */

  mode = 0;
  for (arg = 1; arg < argc; arg++) {
    if (argv[arg][0] != '-')
      break;

    /* Although there are currently no options other than the mode, the
      code below allows for future introduction of non-mode options.   */

    switch (toupper(argv[arg][1])) {
      case 'D': /* Delete indices */
        if (mode)
          goto usage;
        mode = 'D';
        break;

      case 'M': /* Move indices */
        if (mode)
          goto usage;
        mode = 'M';
        break;

      case 'P': /* Set index path */
        if (mode)
          goto usage;
        mode = 'P';
        break;

      case 'Q': /* Query path settings */
        if (mode)
          goto usage;
        mode = 'Q';
        break;

      default:
        goto usage;
    }
  }

  /* Select path by mode */

  switch (mode) {
    case 'D': /* Delete indices */
      if (arg >= argc)
        goto usage; /* Data pathname missing */
      fullpath(data_path, argv[arg++]);
      if (arg != argc)
        goto usage; /* Unexpected argument */

      if (!open_primary())
        goto exit_qmidx;

      if ((header.akpath[0] != '\0')               /* Has relocated indices */
          && (stat(header.akpath, &stat_buf) == 0) /* Path exists */
          && (stat_buf.st_mode & S_IFDIR))         /* It's a directory */
      {
        printf("Delete %s (Y/N)", header.akpath);
        if (yesno()) {
          delete_path(header.akpath);
        }
        header.akpath[0] = '\0'; /* Wipe out reference to indices */
      } else                     /* Delete all "local" index subfiles */
      {
        for (ak = 0; ak < MAX_INDICES; ak++) {
          if (header.ak_map & (1 << ak)) {
            // converted to snprintf() -gwb 25Feb20
            if (snprintf(path, MAX_ACCOUNT_NAME_LEN + 1, "%s%c~%d", data_path, DS, SF(ak)) >= (MAX_ACCOUNT_NAME_LEN + 1)) {
              printf("Overflowed path/filename buffer.  Truncated to:\n%s%c~%d", data_path, DS, SF(ak));
            }
            remove(path);
          }
        }
      }

      header.ak_map = 0; /* Clear AK map */
      write_primary_header();
      printf("All indices deleted\n");
      break;

    case 'M': /* Move indices */
      if (arg >= argc)
        goto usage; /* Data pathname missing */
      fullpath(data_path, argv[arg++]);
      if (arg < argc)
        strcpy(ak_path, argv[arg++]);
      if (arg != argc)
        goto usage; /* Unexpected argument */

      if (!open_primary())
        goto exit_qmidx;

      /* Check we are not moving to the current index location */

      if (strcmp(ak_path, header.akpath) == 0) {
        printf("Indices are already at this location\n");
        goto exit_qmidx;
      }

      /* Cross-check that these indices really belong to this file */

      if (!cross_check(header.akpath))
        goto exit_qmidx;

      /* Confirm target directory does not exist */

      if (ak_path[0] != '\0') {
        /* Check target location does not already exist */

        if (stat(ak_path, &stat_buf) == 0) {
          printf("%s already exists\n", ak_path);
          goto exit_qmidx;
        }

        /* Create the directory */

        if (!make_path(ak_path))
          goto exit_qmidx;
      }

      /* Copy indices */

      if (!copy_indices())
        goto exit_qmidx;

      /* Update primary header */

      strcpy(old_akpath, header.akpath);
      strcpy(header.akpath, ak_path);
      if (!write_primary_header())
        goto exit_qmidx;

      /* Remove old indices */

      if (old_akpath[0] != '\0') /* Had relocated indices */
      {
        delete_path(old_akpath);
      } else /* Delete all "local" index subfiles */
      {
        for (ak = 0; ak < MAX_INDICES; ak++) {
          if (header.ak_map & (1 << ak)) {
            // converted to snprintf() -gwb 25Feb20
            if (snprintf(path, MAX_ACCOUNT_NAME_LEN + 1, "%s%c~%d", data_path, DS, SF(ak)) >= (MAX_ACCOUNT_NAME_LEN + 1)) {
              printf("Overflowed path/filename buffer.  Truncated to:\n%s%c~%d", data_path, DS, SF(ak));
            }
            remove(path);
          }
        }
      }

      printf("Indices have been moved\n");
      break;

    case 'P': /* Set index path */
      if (arg >= argc)
        goto usage; /* Data pathname missing */
      fullpath(data_path, argv[arg++]);
      if (arg < argc)
        strcpy(ak_path, argv[arg++]);
      if (arg != argc)
        goto usage; /* Unexpected argument */

      if (!open_primary())
        goto exit_qmidx;

      if (strcmp(ak_path, header.akpath) == 0) {
        printf("Indices are already at this location\n");
        goto exit_qmidx;
      }

      if (ak_path[0] != '\0') {
        /* Check target location exists */

        if (stat(ak_path, &stat_buf) != 0) {
          printf("%s does not exist\n", ak_path);
          goto exit_qmidx;
        }

        if (!(stat_buf.st_mode & S_IFDIR)) {
          printf("%s is not a directory\n", ak_path);
          goto exit_qmidx;
        }
      }

      /* Cross-check that indices are for this file */

      if (!cross_check(ak_path))
        goto exit_qmidx;

      /* Update primary header */

      strcpy(header.akpath, ak_path);
      write_primary_header();
      break;

    case 'Q': /* Query path settings */
    case '\0':
      if (arg >= argc)
        goto usage; /* Data pathname missing */
      fullpath(data_path, argv[arg++]);
      if (arg != argc)
        goto usage; /* Unexpected argument */

      if (!open_primary())
        goto exit_qmidx;

      if (header.ak_map == 0) {
        printf("File has no indices\n");
      } else if (header.akpath[0] == '\0') {
        printf("Indices are not relocated\n");
      } else {
        printf("Index directory is %s\n", header.akpath);
      }
      break;

    default:
      goto usage;
  }

  status = 0;

exit_qmidx:
  return status;

usage:
  printf("qmidx -d datapath           Delete indices\n");
  printf("qmidx -m datapath {akpath}  Move indices to new location\n");
  printf("qmidx -p datapath {akpath}  Set index path\n");
  printf("qmidx -q datapath           Report path\n");
  printf("qmidx datapath              Report path\n");

  return 1;
}

/* ======================================================================
   open_primary()  -  Open primary subfile                                */

bool open_primary() {
  bool status = FALSE;
  char pathname[MAX_PATHNAME_LEN + 1];
  int fno;
  FILE_ENTRY *fptr;
  // converted to snprintf() -gwb 25Feb20
  if (snprintf(pathname, MAX_PATHNAME_LEN + 1, "%s%c~0", data_path, DS) >= (MAX_PATHNAME_LEN + 1)) {
    printf("Overflowed path/filename buffer.  Truncated to:\n%s%c~0\n", data_path, DS);
  }

  /* Check that this file is not currently in use within QM */

  if (attach_shared_memory()) {
    for (fno = 1; fno <= sysseg->used_files; fno++) {
      fptr = FPtr(fno);
      if (fptr->ref_ct != 0) {
        if (strcmp((char *)(fptr->pathname), data_path) == 0) {
          printf("File is currently open within QM\n");
          goto exit_open_primary;
        }
      }
    }
    unbind_sysseg();
  }

  pfu = open(pathname, (int)(O_RDWR | O_BINARY), default_access);
  if (pfu < 0) {
    printf("Cannot open data file [%d]\n", errno);
    goto exit_open_primary;
  }

  if (read(pfu, (u_char *)&header, DH_HEADER_SIZE) != DH_HEADER_SIZE) {
    printf("Read error in primary subfile [%d]\n", errno);
    goto exit_open_primary;
  }

  /* Check magic number in primary subfile */

  if (header.magic != DH_PRIMARY) {
    printf("Incorrect magic number in primary subfile\n");
    goto exit_open_primary;
  }

  status = TRUE;

exit_open_primary:
  return status;
}

/* ======================================================================
   write_primary_header()  -  Write primary subfile header                */

bool write_primary_header() {
  if (Seek(pfu, 0, SEEK_SET) < 0) {
    printf("Seek error positioning to write primary subfile header [%d]\n", errno);
    return FALSE;
  }

  if (write(pfu, (char *)&header, DH_HEADER_SIZE) != DH_HEADER_SIZE) {
    printf("Error writing primary subfile header [%d]\n", errno);
    return FALSE;
  }

  return TRUE;
}

/* ======================================================================
   cross_check()  -  Cross-check index subfile                            */

bool cross_check(char *path) {
  bool status = FALSE;
  char pathname[MAX_PATHNAME_LEN + 1 + 256]; /* git issue #82*/
  int16_t ak;
  DH_AK_HEADER ak_header;
  int akfu = -1;

  for (ak = 0; ak < MAX_INDICES; ak++) {
    if (header.ak_map & (1 << ak)) {
      sprintf(pathname, "%s%c~%d", (path[0] != '\0') ? path : data_path, DS, SF(ak));
      akfu = open(pathname, (int)(O_RDONLY | O_BINARY), default_access);
      if (akfu < 0) {
        printf("Cannot open index subfile %d [%d]\n", SF(ak), errno);
        goto exit_cross_check;
      }

      if (read(akfu, (u_char *)&ak_header, DH_AK_HEADER_SIZE) != DH_AK_HEADER_SIZE) {
        printf("Read error in index subfile %d [%d]\n", SF(ak), errno);
        goto exit_cross_check;
      }

      /* Check magic number in primary subfile */

      if (ak_header.magic != DH_INDEX) {
        printf("Incorrect magic number in index subfile %d\n", SF(ak));
        goto exit_cross_check;
      }

      /* Cross-check with primary subfile */

      if (ak_header.data_creation_timestamp != header.creation_timestamp) {
        printf("Index subfile %d does not appear to belong to the data file\n", SF(ak));
        goto exit_cross_check;
      }

      close(akfu);
      akfu = -1;
    }
  }

  status = TRUE;

exit_cross_check:
  if (akfu >= 0)
    close(akfu);

  return status;
}

/* ======================================================================
   copy_indices()  -  Copy indices to new location                        */

bool copy_indices() {
  bool status = FALSE;
  char src[MAX_PATHNAME_LEN + 1 + 256]; /* git issue #82*/
  char tgt[MAX_PATHNAME_LEN + 1 + 256]; /* git issue #82*/
  int16_t ak;
  int src_fu = -1;
  int tgt_fu = -1;
  int bytes;

  for (ak = 0; ak < MAX_INDICES; ak++) {
    if (header.ak_map & (1 << ak)) {
      printf("Moving index subfile %d\n", SF(ak));

      /* Form source path and open subfile */

      sprintf(src, "%s%c~%d", (header.akpath[0] != '\0') ? header.akpath : data_path, DS, SF(ak));

      src_fu = open(src, (int)(O_RDONLY | O_BINARY), default_access);
      if (src_fu < 0) {
        printf("Cannot open old index subfile %d [%d]\n", SF(ak), errno);
        goto exit_copy_indices;
      }

      /* Form target path and open subfile */

      sprintf(tgt, "%s%c~%d", (ak_path[0] != '\0') ? ak_path : data_path, DS, SF(ak));

      tgt_fu = open(tgt, (int)(O_RDWR | O_CREAT | O_EXCL | O_BINARY), default_access);
      if (tgt_fu < 0) {
        printf("Cannot create new index subfile %d [%d]\n", SF(ak), errno);
        goto exit_copy_indices;
      }

      while ((bytes = read(src_fu, buffer, BUFFER_SIZE)) > 0) {
        if (write(tgt_fu, buffer, bytes) != bytes) {
          printf("Write error in new index subfile %d [%d]\n", SF(ak), errno);
          goto exit_copy_indices;
        }
      }

      close(src_fu);
      src_fu = -1;
      close(tgt_fu);
      tgt_fu = -1;
    }
  }

  status = TRUE;

exit_copy_indices:
  if (src_fu >= 0)
    close(src_fu);
  if (tgt_fu >= 0)
    close(tgt_fu);

  return status;
}

/* ====================================================================== */

bool yesno() {
  char c;
  char s[10 + 1];

  do {
    printf("? ");
    fflush(stdout);
    fgets(s, 10, stdin);
    c = toupper(s[0]);
  } while ((c != 'Y') && (c != 'N'));
  return (c == 'Y');
}

/* ======================================================================
   delete_path()  -  Delete DOS file/directory path                       */

bool delete_path(char *path) {
  DIR *dfu;
  struct dirent *dp;
  struct stat stat_buf;
  char parent_path[MAX_PATHNAME_LEN + 1];
  int parent_len;
  char sub_path[MAX_PATHNAME_LEN + 1];

  /* Check path exists and get type information */

  if (stat(path, &stat_buf) != 0)
    return FALSE;

  if (stat_buf.st_mode & S_IFDIR) /* It's a directory */
  {
    dfu = opendir(path);
    if (dfu == NULL)
      return FALSE;

    strcpy(parent_path, (char *)path);
    parent_len = strlen(parent_path);
    if (parent_path[parent_len - 1] == DS)
      parent_path[parent_len - 1] = '\0';

    while ((dp = readdir(dfu)) != NULL) {
      if (strcmp(dp->d_name, ".") == 0)
        continue;
      if (strcmp(dp->d_name, "..") == 0)
        continue;
      // converted to snprintf() -gwb 25Feb20
      if (snprintf(sub_path, MAX_PATHNAME_LEN + 1, "%s%c%s", parent_path, DS, dp->d_name) >= (MAX_PATHNAME_LEN + 1)) {
        printf("Overflow of path/filename buffer.  Truncated to:\n%s%c%s\n", parent_path, DS, dp->d_name);
      }
      if (!delete_path(sub_path))
        return FALSE;
    }
    closedir(dfu);

    if (rmdir(path) != 0)
      return FALSE;                        /* Delete the directory */
  } else if (stat_buf.st_mode & S_IFREG) { /* It's a file */
    if (remove(path) != 0)
      return FALSE;
  }

  return TRUE;
}

/* ======================================================================
   make_path()  -  Recursive mkdir to make directory path                 */

bool make_path(char *tgt) {
  char path[MAX_PATHNAME_LEN + 1];
  char new_path[MAX_PATHNAME_LEN + 1];
  struct stat statbuf;
  char *p;
  char *q;

  strcpy(path, tgt);

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

/* ======================================================================
   Attach to shared memory segment                                        */

bool attach_shared_memory() {
  int shmid;

  if ((shmid = shmget(QM_SHM_KEY, 0, 0666)) != -1) {
    if ((sysseg = (SYSSEG *)shmat(shmid, NULL, 0)) == (void *)(-1)) {
      printf("Error %d attaching to shared segment\n", errno);
      return FALSE;
    }
    return TRUE;
  }

  return FALSE;
}

/* ====================================================================== */

void unbind_sysseg() {
  shmdt((void *)sysseg);
}

/* ======================================================================
   fullpath()  -  Map a pathname to its absolute form                     */

bool fullpath(char *path, char *name) {
  //char* path; /* Out */
  // char* name; /* In */

  bool ok;

  /* realpath() requires that the buffer is PATH_MAX bytes even for short
    pathnames. Use a temporary buffer and then copy the data back to the
    caller's buffer.                                                      */

  char buff[PATH_MAX + 1];
  ok = (realpath(name, buff) != NULL);
  strcpy(path, buff);

  return ok;
}

/* END-CODE */
