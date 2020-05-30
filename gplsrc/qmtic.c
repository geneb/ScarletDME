/* QMTIC.C
 * Terminfo compiler.
 * Copyright (c) 2004 Ladybridge Systems, All Rights Reserved
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
 * 23Feb20 gwb Bumped MAX_PATHNAME_LEN to 250 from 160.  250 is within the
 *             max filename limit of both Linux and Windows.
 *             Converted a few problematic sprintf() calls to snprintf() and
 *             converted existing K&R function declarations to ANSI.
 * 
 * START-HISTORY (OpenQM):
 * 28 Sep 04  2.0-3 Replaced GetSysPaths() with GetConfigPath().
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 * Terminfo Compiler.
 *   qmtic {options} src
 *
 *      -tname   Compile only specified terminal
 *      -d       Decompile to stdout
 *      -dall    Decompile all definitions to stdout
 *      -i       Display index of terminal names
 *      -ppath   Alternative terminfo library pathname
 *      -v       Verbose mode
 *      -x       Do not overwrite existing entries
 *
 * Return codes:
 *   0    Success
 *   1    Command error
 *   2    Cannot open source file
 *
 *
 * QMTIC supports a limited subset of terminfo definitions, sufficient to
 * populate our private terminfo system.
 *
 *
 * Layout of terminfo database file:
 *
 *     ======================================
 *     | Magic number (0x011A)              | All numbers are stored
 *     |------------------------------------| in little endian format
 *     | Name size (2 bytes)                |
 *     |------------------------------------|
 *     | Boolean count (2 bytes)            |
 *     |------------------------------------|
 *     | Numeric count (2 bytes)            |
 *     |------------------------------------|
 *     | String count (2 bytes)             |
 *     |------------------------------------|
 *     | String size (2 bytes)              |
 *     |====================================|
 *     | Name                               |
 *     |------------------------------------|
 *     | Booleans                           |
 *     |------------------------------------|
 *     | {Pad to even byte boundary}        |
 *     |------------------------------------|
 *     | Numbers (NumCount short ints)      |   -1 = absent, -2 = cancelled
 *     |------------------------------------|
 *     | Str offsets (NumString short ints) |   -1 = absent, -2 = cancelled
 *     |------------------------------------|
 *     | String table (String size bytes)   |
 *     ======================================
 *
 * QM does not use the concept of cancelled strings but must retain
 * recognition of them to allow use of compiled Linux terminfo items.
 *
 * The underlying concepts of terminfo are taken from postings on various
 * web sites. This module is believed to be totally proprietary.
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>

#include "ti_names.h"
#include "revstamp.h"

#define NIX
#ifdef __APPLE__
#include <machine/endian.h>
#else
#include <endian.h>
#endif
#if __BYTE_ORDER__ == __BIG_ENDIAN__
#define BIG_ENDIAN_SYSTEM
#endif

#include <unistd.h>
#define MakeDirectory(name) mkdir(name, 0777)
#define stricmp(a, b) strcasecmp(a, b)
#define O_BINARY 0
#define TEXTREAD "r"
#define default_access 0777
#define DS '/'

#ifndef DS
#error No environment set
#endif

#define MAX_PATHNAME_LEN 250
// was 160.  I bumped this to 250 because it falls within both
// the filename limits of Linux & Windows. -gwb 23Feb20
#define MAX_REC_LEN 256
#define MAX_DEFN_LEN 4096
#define MAX_ID_LEN 32

#ifdef BIG_ENDIAN_SYSTEM
#define Reverse2(a) a = swap2(a)
int16_t swap2(int16_t data);
#endif

/* Type definitions */

typedef int16_t bool;
#define FALSE 0
#define TRUE 1

char sysdir[MAX_PATHNAME_LEN + 1] = "";
char inipath[MAX_PATHNAME_LEN + 1] = "";

/* Source file */
char src[MAX_PATHNAME_LEN + 1];
FILE* src_fu = NULL;
int src_lno;
char term_name[MAX_REC_LEN + 1];
u_char id[MAX_ID_LEN + 1];

/* Terminfo data */

char terminfodir[MAX_PATHNAME_LEN + 1] = "";

union {
  struct {
    int16_t magic;
    int16_t name_size;
    int16_t bool_count;
    int16_t num_count;
    int16_t str_count;
    int16_t str_size;
  } header;
  char buff[4096];
} tinfo;
#define MAGIC 0x011A

int tinfo_bytes;
char booleans[NumBoolNames];
int16_t numerics[NumNumNames];
int16_t string_offsets[NumStrNames];
char strings[4096];

int errors;

struct NAME {
  struct NAME* next;
  char name[1];
};
struct NAME* name_head = NULL;

/* Command options */

bool decompile = FALSE;     /* -D Decompile existing entries */
bool decompile_all = FALSE; /* -DALL */
bool do_index = FALSE;      /* -I Display index of terminal names */
bool overwrite = TRUE;      /* -X */
bool verbose = FALSE;       /* -V Detailed progress report */

char text[512 + 1];
int16_t text_len;
bool flush_text;

bool more;

/* External routines */

bool GetConfigPath(char* inipath);

/* Internal routines */

bool read_config(void);
bool add_name(char* name);
void process_file(void);
char* get_token(void);
void process_escapes(u_char* p);
void reset_buffers(void);
bool in_list(char* name);
int16_t lookup(char* id, char* names[], int16_t ct);
void err(char* template, ...);
bool write_entry(char* name);
void decompile_entry(void);
void emit(char* template, ...);
void build_index(bool decomp);

/* ====================================================================== */

int main(int argc, char* argv[]) {
  int status = 1;
  int arg;
  struct stat statbuf;
  bool name_present = FALSE;

  printf(
      "#[ QMTIC %s   Copyright, Ladybridge Systems, %s.  All rights reserved. "
      "]\n\n",
      QM_REV_STAMP, QM_COPYRIGHT_YEAR);

  /* First pass  -  Pick out options */

  for (arg = 1; arg < argc; arg++) {
    if (argv[arg][0] == '-') {
      switch (toupper(argv[arg][1])) {
        case 'D': /* Decompile */
          decompile = TRUE;
          decompile_all = (stricmp(argv[arg] + 2, "ALL") == 0);
          break;

        case 'I': /* Index */
          do_index = TRUE;
          break;

        case 'P':
          strcpy(terminfodir, argv[arg] + 2);
          break;

        case 'T': /* Compile specific terminal only */
          if (!add_name(argv[arg] + 2))
            goto usage;
          break;

        case 'V': /* Verbose mode */
          verbose = TRUE;
          break;

        case 'X': /* Do not overwrite existing entries */
          overwrite = FALSE;
          break;

        default:
          goto usage;
      }
    } else
      name_present = TRUE;
  }

  /* Check for incompatible options */

  if (decompile && (do_index || (name_head != NULL))) {
    printf("Incompatible options given\n");
    goto usage;
  }

  /* Find the terminfo directory */

  if (terminfodir[0] == '\0') {
    /* Read config file to find terminfo directory */

    if (!read_config())
      goto abort;

    if (sysdir[0] == '\0') {
      printf("Unable to locate QMSYS directory\n");
      goto abort;
    }

    if (terminfodir[0] == '\0')
      // converted to snprintf() -gwb 23Feb20
      if (snprintf(terminfodir, MAX_PATHNAME_LEN + 1, "%s%cterminfo", sysdir,
                   DS) >= (MAX_PATHNAME_LEN + 1)) {
        printf("Overflowed max file/path length. Truncated to:\n\"%s\"\n",
               terminfodir);
      }
  }

  if (verbose)
    printf("Terminfo directory = %s\n", terminfodir);

  /* Here we go... */

  if (do_index) /* Index terminals */
  {
    if (name_present)
      goto usage;
    build_index(FALSE);
  } else if (decompile_all) {
    build_index(TRUE);
  } else if (decompile) /* Decompile mode */
  {
    if (!name_present)
      goto usage;

    /* Second pass  -  Pick out terminal names */

    for (arg = 1; arg < argc; arg++) {
      if (argv[arg][0] != '-') {
        // converted to snprintf() -gwb 23Feb20
        if (snprintf(src, MAX_PATHNAME_LEN + 1, "%s%c%c%c%s", terminfodir, DS,
                     argv[arg][0], DS, argv[arg]) >= (MAX_PATHNAME_LEN + 1)) {
          printf("Overflowed max file/path size. Truncated to :\n\"%s\"\n",
                 src);
        }
        decompile_entry();
      }
    }
  } else /* Compile mode */
  {
    if (!name_present)
      goto usage;

    umask(0);

    /* Create terminfo directory if it does not already exist */

    if (stat(terminfodir, &statbuf) == 0) /* Exists */
    {
      if (!(statbuf.st_mode & S_IFDIR)) /* But not as a directory */
      {
        printf("%s exists but is not a directory\n", terminfodir);
        goto abort;
      }
    } else /* Does not exist */
    {
      if (MakeDirectory(terminfodir) != 0) {
        printf("[%d] Unable to create %s\n", errno, terminfodir);
        goto abort;
      }
    }

    /* Second pass  -  Pick out source names */

    for (arg = 1; arg < argc; arg++) {
      if (argv[arg][0] != '-') {
        strcpy(src, argv[arg]);
        src_fu = fopen(src, TEXTREAD);

        if (src_fu == NULL) {
          printf("[%d] Cannot open %s\n", errno, src);
          status = 2;
          goto abort;
        }

        process_file();

        fclose(src_fu);
        src_fu = NULL;
      }
    }
  }

  status = 0;

abort:
  return status;

usage:
  printf("Usage:\n");
  printf("  qmtic {options} src...\n");
  printf("\n");
  printf("Options:\n");
  printf("  -d       Decompile named terminal entry\n");
  printf("  -i       Display an index of terminal names\n");
  printf("  -ppath   Use terminfo library at given path\n");
  printf("  -txxx    Compile only terminal xxx (may be repeated)\n");
  printf("  -v       Verbose mode\n");
  printf("  -x       Do not overwrite existing entries\n");
  printf("\n");
  printf("Without -d, QMTIC compiles items from the given source file(s).\n");
  printf("With -d, src should be a terminal name to be decompiled.\n");

  return status;
}

/* ======================================================================
   add_name()  -  Add terminal name to list to compile                    */

bool add_name(char* name) {
  int n;
  struct NAME* p;

  n = strlen(name);
  if ((*name == '\0') || (n > 32)) {
    printf("Invalid or missing terminal name\n");
    return FALSE;
  }

  p = malloc(offsetof(struct NAME, name) + n + 1);
  p->next = name_head;
  strcpy(p->name, name);
  name_head = p;

  return TRUE;
}

/* ======================================================================
   read_config()  -  Read config file                                     */

bool read_config() {
  char rec[200 + 1];
  FILE* ini_file;
  char section[32 + 1];
  char* p;

  if (inipath[0] == '\0')
    (void)GetConfigPath(inipath);

  if ((ini_file = fopen(inipath, "rt")) == NULL) {
    printf("[%d] Cannot open %s\n", errno, inipath);
    return FALSE;
  }

  section[0] = '\0';
  while (fgets(rec, 200, ini_file) != NULL) {
    if ((p = strchr(rec, '\n')) != NULL)
      *p = '\0';

    if ((rec[0] == '#') || (rec[0] == '\0'))
      continue;

    if (rec[0] == '[') {
      if ((p = strchr(rec, ']')) != NULL)
        *p = '\0';
      strcpy(section, rec + 1);
      for (p = section; *p != '\0'; p++)
        *p = toupper(*p);
      continue;
    }

    if (strcmp(section, "QM") == 0) /* [qm] items */
    {
      if (strncmp(rec, "QMSYS=", 6) == 0) {
        if (sysdir[0] == '\0')
          strcpy(sysdir, rec + 6);
      } else if (strncmp(rec, "TERMINFO=", 9) == 0)
        strcpy(terminfodir, rec + 9);
    }
  }

  fclose(ini_file);
  return TRUE;
}

/* ======================================================================
   process_file()  -  Process a source file                               */

void process_file() {
  bool skip;
  int16_t id_len;
  char* tptr;
  char* p;
  int16_t n;
#ifdef BIG_ENDIAN_SYSTEM
  int16_t i;
#endif

  if (verbose)
    printf("Processing %s\n", src);

  src_lno = 0;
  errors = 0;
  reset_buffers();

  while ((p = get_token()) != NULL) {
    strcpy(term_name, p); /* First token is terminal name(s) */

    /* Construct tinfo buffer */

    tinfo.header.magic = MAGIC;
    tptr = tinfo.buff + sizeof(tinfo.header);

    /* ----- Terminal name */

    tinfo.header.name_size = strlen(term_name) + 1;
    memcpy(tptr, term_name, tinfo.header.name_size);
    tptr += tinfo.header.name_size;

    /* Check for selective compilation */

    if ((p = strrchr(term_name, '|')) == NULL) {
      err("Invalid terminal name token : %s\n", term_name);
      skip = TRUE;
    } else {
      *p = '\0'; /* Remove description element */
      skip = ((name_head != NULL) && !in_list(term_name));
    }

    /* Remaining tokens are capabilities */

    while (more) {
      if ((p = get_token()) == NULL) {
        err("Unexpected end of source file\n");
        goto exit_process_file;
      }

      if (!skip) {
        /* Extract id */

        for (id_len = 0;; p++) {
          if ((*p == '#') || (*p == '=') || (*p == '\0'))
            break;

          if (id_len == MAX_ID_LEN) {
            err("Capability name too long\n");
            goto skip_token;
          }
          id[id_len++] = *p;
        }

        id[id_len] = '\0';

        /* Process this id */

        switch (*p) {
          case '#': /* Numeric parameter */
            if ((n = lookup((char*)id, numnames, NumNumNames)) < 0)
              continue;
            numerics[n] = atoi(++p);
            if (n >= tinfo.header.num_count)
              tinfo.header.num_count = n + 1;
            break;

          case '=': /* String parameter */
            if ((n = lookup((char*)id, strnames, NumStrNames)) < 0)
              continue;
            p++;
            process_escapes((unsigned char*)p);
            string_offsets[n] = tinfo.header.str_size;
            strcpy(strings + tinfo.header.str_size, p);
            tinfo.header.str_size += strlen(p) + 1;
            if (n >= tinfo.header.str_count)
              tinfo.header.str_count = n + 1;
            break;

          case '\0': /* Boolean parameter */
            if ((n = lookup((char*)id, boolnames, NumBoolNames)) < 0)
              continue;
            booleans[n] = 1;
            if (n >= tinfo.header.bool_count)
              tinfo.header.bool_count = n + 1;
            break;
        }
      }
    }
  skip_token:

    if ((errors == 0) && !skip) {
      /* ----- Booleans */

      memcpy(tptr, booleans, tinfo.header.bool_count);
      tptr += tinfo.header.bool_count;

      /* ----- Padding */

      if ((tptr - tinfo.buff) & 1)
        tptr++;

        /* ----- Numbers */

#ifdef BIG_ENDIAN_SYSTEM
      for (i = 0; i < NumNumNames; i++)
        Reverse2(numerics[i]);
#endif
      memcpy(tptr, numerics, tinfo.header.num_count * 2);
      tptr += tinfo.header.num_count * 2;

      /* ----- String offsets */

#ifdef BIG_ENDIAN_SYSTEM
      for (i = 0; i < NumStrNames; i++)
        Reverse2(string_offsets[i]);
#endif
      memcpy(tptr, string_offsets, tinfo.header.str_count * 2);
      tptr += tinfo.header.str_count * 2;

      /* ----- String table */

      memcpy(tptr, strings, tinfo.header.str_size);
      tptr += tinfo.header.str_size;

      tinfo_bytes = tptr - tinfo.buff;

#ifdef BIG_ENDIAN_SYSTEM
      /* This is a big endian system. Turn all numbers around as terminfo
        files are always stored in little endian format.                 */

      Reverse2(tinfo.header.magic);
      Reverse2(tinfo.header.name_size);
      Reverse2(tinfo.header.bool_count);
      Reverse2(tinfo.header.num_count);
      Reverse2(tinfo.header.str_count);
      Reverse2(tinfo.header.str_size);
#endif

      /* Write to terminfo database */

      p = strtok(term_name, "|");
      do {
        if ((name_head == NULL) || in_list(p))
          (void)write_entry(p);
      } while ((p = strtok(NULL, "|")) != NULL);

      errors = 0;
      reset_buffers();
    }
  }

exit_process_file:
  return;
}

/* ======================================================================
   get_token()                                                            */

char* get_token() {
  static char rec[MAX_REC_LEN + 1] = "";
  static int idx = 0;
  char* token;
  char* p;
  int16_t n;

  while (rec[idx] == '\0') {
    do {
      if (fgets(rec, MAX_REC_LEN, src_fu) == NULL)
        return NULL;
      src_lno++;

      if ((p = strchr(rec, '\n')) != NULL)
        *p = '\0'; /* Remove newline */

      /* Strip off trailing spaces */

      for (n = strlen(rec) - 1; (n >= 0) && isspace((unsigned int)(rec[n]));
           n--)
        rec[n] = '\0';

      /* Skip leading spaces */

      idx = 0;
      while (isspace((unsigned int)(rec[idx])))
        idx++;
    } while ((rec[idx] == '\0') || (rec[idx] == '#'));
  }

  token = rec + idx;
  while (*token == ' ')
    token++;

  /* Find end of token */

  while (rec[idx] != '\0') {
    if (rec[idx] == '\\')
      idx++; /* Ignore literal character */
    else if (rec[idx] == ',') {
      rec[idx] = '\0';
      while (isspace((unsigned int)(rec[++idx]))) {
      }
      more = TRUE;
      return token;
    }
    idx++;
  }

  /* This is the final token in the line with no trailing comma */

  more = FALSE;
  return token;
}

/* ====================================================================== */

void process_escapes(u_char* p) {
  int16_t i;
  int16_t n;
  u_char* q;
  u_char* r;

  for (q = p, r = p; *q != '\0'; q++) {
    switch (*q) {
      case '\\':
        q++;
        switch (toupper(*q)) {
          case 'B':
            *(r++) = '\b';
            break;
          case 'E':
            *(r++) = 0x1B;
            break;
          case 'F':
            *(r++) = '\f';
            break;
          case 'L':
            *(r++) = '\n';
            break;
          case 'N':
            *(r++) = '\n';
            break;
          case 'R':
            *(r++) = '\r';
            break;
          case 'S':
            *(r++) = ' ';
            break;
          case 'T':
            *(r++) = '\t';
            break;
          case '^':
            *(r++) = '^';
            break;
          case '\\':
            *(r++) = '\\';
            break;
          default:
            if ((*q >= '0') && (*q <= '7')) {
              for (i = 0, n = 0; i < 3; i++) {
                n = (n << 3) + (*q - '0');
                if ((*(q + 1) < '0') || (*(q + 1) > '7'))
                  break;
                q++;
              }

              *(r++) = n;

              if ((n == 0) || (n > 255)) {
                err("\nnn construct not in range \001 to \377 in %s\n", id);
              }
            } else
              *(r++) = *q;
            break;
        }
        break;

      case '^':
        *(r++) = *(++q) & 0x1F;
        break;

      default:
        *(r++) = *q;
        break;
    }
  }
  *r = '\0';
}

/* ======================================================================
   reset_buffers()  -  Set up buffer areas                                */

void reset_buffers() {
  memset(tinfo.buff, 0, sizeof(tinfo.buff));
  memset(booleans, 0, sizeof(booleans));
  memset(numerics, -1, sizeof(numerics));
  memset(string_offsets, -1, sizeof(string_offsets));
}

/* ======================================================================
   in_list()  -  Check conditional compilation                            */

bool in_list(char* defn_names) { /* Names in definition */
  char name_list[512];
  char* p;
  char* q;
  struct NAME* r;

  /* Copy the names so that we can modify the list without upsetting the
    calling routine.                                                    */

  strcpy(name_list, defn_names);
  p = name_list;
  do {
    q = strchr(p, '|');
    if (q != NULL)
      *(q++) = '\0';
    for (r = name_head; r != NULL; r = r->next) {
      if (strcmp(r->name, p) == 0)
        return TRUE; /* Found it */
    }
  } while ((p = q) != NULL);

  return FALSE;
}

/* ======================================================================
   lookup()  -  Lookup capability name                                    */

int16_t lookup(char* id, char* names[], int16_t ct) {
  int16_t i;

  for (i = 0; i < ct; i++)
    if (strcmp(id, names[i]) == 0)
      return i;
  err("Unrecognised capability name '%s'\n", id);
  errors++;
  return -1;
}

/* ====================================================================== */

void err(char* template, ...) {
  va_list arg_ptr;

  if (errors == 0)
    printf("%s:\n", term_name);

  printf("%d: ", src_lno);
  va_start(arg_ptr, template);
  vprintf(template, arg_ptr);
  va_end(arg_ptr);

  errors++;
}

/* ======================================================================
   write_entry()  -  Write terminfo database entry                        */

bool write_entry(char* name) {
  int tgt_fu;
  char path[MAX_PATHNAME_LEN + 1];
  char fullpath[MAX_PATHNAME_LEN + 1];
  struct stat statbuf;
  // converted to snprintf() -gwb 23Feb20
  if (snprintf(fullpath, MAX_PATHNAME_LEN + 1, "%s%c%c%c%s", terminfodir, DS,
               name[0], DS, name) >= (MAX_PATHNAME_LEN + 1)) {
    printf("Overflowed max file/pathname.  Truncated to:\n\"%s\"\n", fullpath);
  }
  if ((!overwrite) && !access(fullpath, 0))
    return TRUE;

  /* Check terminfo subdirectory exists, creating if necessary */
  // converted to snprintf() -gwb 23Feb20
  if (snprintf(path, MAX_PATHNAME_LEN + 1, "%s%c%c", terminfodir, DS,
               name[0]) >= (MAX_PATHNAME_LEN + 1)) {
    printf("Overflowed max file/pathname.  Truncated to:\n\"%s\"\n", path);
  }
  if (stat(path, &statbuf) == 0) /* Exists */
  {
    if (!(statbuf.st_mode & S_IFDIR)) /* But not as a directory */
    {
      printf("%s exists but is not a directory\n", path);
      return FALSE;
    }
  } else /* Does not exist */
  {
    if (MakeDirectory(path) != 0) {
      printf("[%d] Unable to create %s\n", errno, path);
      return FALSE;
    }
  }

  /* Write entry */

  if ((tgt_fu = open(fullpath, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY,
                     default_access)) < 0) {
    printf("[%d] Unable to open %s\n", errno, fullpath);
  } else {
    if (write(tgt_fu, tinfo.buff, tinfo_bytes) != tinfo_bytes) {
      printf("[%d] Write error in %s\n", errno, fullpath);
      close(tgt_fu);
      return FALSE;
    }

    if (verbose)
      printf("   %s\n", name);
    close(tgt_fu);
  }

  return TRUE;
}

/* ======================================================================
   decompile_entry()  -  Decompile a terminfo entry                       */

void decompile_entry() {
  int fu;
  int16_t i;
  int16_t n;
  char* p;
  char* q;
  u_char* r;
  u_char* s;
  u_char str[512];

  struct TN {
    struct TN* next;
    char name[1];
  };
  static struct TN* tn_head = NULL;
  struct TN* tn;

  if ((fu = open(src, O_RDONLY | O_BINARY)) < 0) {
    printf("[%d] Unable to open %s\n", errno, src);
    goto exit_decompile_entry;
  }

  if (read(fu, tinfo.buff, sizeof(tinfo.buff)) <= 0) {
    printf("[%d] Error reading %s\n", errno, src);
    goto exit_decompile_entry;
  }

#ifdef BIG_ENDIAN_SYSTEM
  /* This is a big endian system. Turn all numbers around as terminfo
    files are always stored in little endian format.                 */

  Reverse2(tinfo.header.magic);
  Reverse2(tinfo.header.name_size);
  Reverse2(tinfo.header.bool_count);
  Reverse2(tinfo.header.num_count);
  Reverse2(tinfo.header.str_count);
  Reverse2(tinfo.header.str_size);
#endif

  if (tinfo.header.magic != MAGIC) {
    printf("%s is not a valid terminfo file\n", src);
    goto exit_decompile_entry;
  }

  p = tinfo.buff + sizeof(tinfo.header);

  /* If we are decompiling all entries, check for duplicates */

  if (decompile_all) {
    for (tn = tn_head; tn != NULL; tn = tn->next) {
      if (strcmp(tn->name, p) == 0)
        goto exit_decompile_entry;
    }

    tn = malloc(offsetof(struct TN, name) + strlen(p) + 1);
    strcpy(tn->name, p);
    tn->next = tn_head;
    tn_head = tn;
  }

  /* ----- Terminal name(s) */

  printf("%.*s", tinfo.header.name_size, p);
  p += tinfo.header.name_size;
  flush_text = TRUE;
  text_len = 0;

  /* ----- Booleans */

  for (i = 0; i < tinfo.header.bool_count; i++, p++) {
    if (i < NumBoolNames) {
      n = *p;
      if (n == -1)
        continue; /* Absent string */
      if (n == -2)
        continue; /* Cancelled string */
      if (n)
        emit("%s", boolnames[i]);
    }
  }
  flush_text = TRUE;

  if ((p - tinfo.buff) & 1)
    p++; /* Align on even byte boundary */

  /* ----- Numbers */

  for (i = 0; i < tinfo.header.num_count; i++, p += 2) {
    n = *((int16_t*)p);
#ifdef BIG_ENDIAN_SYSTEM
    Reverse2(n);
#endif
    if (n == -1)
      continue; /* Absent number */
    if (n == -2)
      continue; /* Cancelled number */
    if (i < NumNumNames) {
      emit("%s#%d", numnames[i], (int)n);
    }
  }
  flush_text = TRUE;

  /* ----- Strings */

  q = p + tinfo.header.str_count * 2;
  for (i = 0; i < tinfo.header.str_count; i++, p += 2) {
    n = *((int16_t*)p);
#ifdef BIG_ENDIAN_SYSTEM
    Reverse2(n);
#endif
    if (n == -1)
      continue; /* Absent string */
    if (n == -2)
      continue; /* Cancelled string */
    if (i < NumStrNames) {
      if (strcmp(strnames[i], "acsc") == 0)
        continue; /* Omit acsc */

      for (r = (u_char*)(q + n), s = str; *r != '\0'; r++) {
        switch (*r) {
          case '\b':
            *(s++) = '\\';
            *(s++) = 'b';
            break;
          case '\f':
            *(s++) = '\\';
            *(s++) = 'f';
            break;
          case '\n':
            *(s++) = '\\';
            *(s++) = 'n';
            break;
          case '\r':
            *(s++) = '\\';
            *(s++) = 'r';
            break;
          case '\t':
            *(s++) = '\\';
            *(s++) = 't';
            break;
          case ' ':
            *(s++) = '\\';
            *(s++) = ' ';
            break;
          case ',':
            *(s++) = '\\';
            *(s++) = ',';
            break;
          case '^':
            *(s++) = '\\';
            *(s++) = '^';
            break;
          case '\\':
            *(s++) = '\\';
            *(s++) = '\\';
            break;
          case 27:
            *(s++) = '\\';
            *(s++) = 'E';
            break;
          default:
            if (*r < 32) {
              *(s++) = '^';
              *(s++) = *r + 64;
            } else if (*r >= 128) {
              *(s++) = '\\';
              *(s++) = (*r >> 6) + '0';
              *(s++) = ((*r >> 3) & 7) + '0';
              *(s++) = (*r & 7) + '0';
            } else
              *(s++) = *r;
            break;
        }
      }
      *s = '\0';
      emit("%s=%s", strnames[i], str);
    }
  }

  text[text_len] = '\0';
  printf("%s\n\n", text);
  text_len = 0;

exit_decompile_entry:
  if (fu >= 0)
    close(fu);
}

/* ======================================================================
   emit()  -  Emit capability                                             */

void emit(char* template, ...) {
  va_list arg_ptr;
  char s[512];
  int16_t n;

  va_start(arg_ptr, template);
  vsprintf(s, template, arg_ptr);
  va_end(arg_ptr);
  n = strlen(s);

  text[text_len++] = ',';

  if ((text_len + n) > 77)
    flush_text = TRUE;

  if (flush_text) {
    text[text_len] = '\0';
    printf("%s\n", text);
    text_len = 0;
    flush_text = FALSE;
  } else
    text[text_len++] = ' ';

  /* Emit margin if starting a new line */

  while (text_len < 4)
    text[text_len++] = ' ';

  /* Append this item */

  memcpy(text + text_len, s, n);
  text_len += n;
}

/* ======================================================================
   build_index()  -  List terminals in database                           */

void build_index(bool decomp) {
  DIR* dfu1;
  struct dirent* dp1;
  char subdirpath[MAX_PATHNAME_LEN + 1];
  char defpath[MAX_PATHNAME_LEN + 1];
  DIR* dfu2;
  struct dirent* dp2;
  struct stat statbuf;
  int n;
  struct NAME* p;
  struct NAME* q;
  struct NAME* prev;

  /* Level 1 scan - The terminfo directory */

  if ((dfu1 = opendir(terminfodir)) != NULL) {
    while ((dp1 = readdir(dfu1)) != NULL) {
      if ((strcmp(dp1->d_name, ".") == 0) || (strcmp(dp1->d_name, "..") == 0)) {
        continue;
      }
      // converted to snprintf() -gwb 23Feb20
      if (snprintf(subdirpath, MAX_PATHNAME_LEN + 1, "%s%c%s", terminfodir, DS,
                   dp1->d_name) >= (MAX_PATHNAME_LEN + 1)) {
        printf("Overflowed max file/pathname. Truncated to:\n\"%s\"\n",
               subdirpath);
      }
      if (stat(subdirpath, &statbuf))
        break;
      if (statbuf.st_mode & S_IFDIR) {
        /* Level 2 scan - The prefix directories */

        if ((dfu2 = opendir(subdirpath)) != NULL) {
          while ((dp2 = readdir(dfu2)) != NULL) {
            // converted to snprintf() -gwb 23Feb20
            if (snprintf(defpath, MAX_PATHNAME_LEN + 1, "%s%c%s", subdirpath,
                         DS, dp2->d_name) >= (MAX_PATHNAME_LEN + 1)) {
              printf("Overflowed max file/pathname. Truncated to:\n\"%s\"\n",
                     defpath);
            }
            if (stat(defpath, &statbuf))
              break;

            if (statbuf.st_mode & S_IFREG) {
              /* Add to sorted list of names */

              n = strlen(dp2->d_name);
              p = malloc(offsetof(struct NAME, name) + n + 1);
              strcpy(p->name, dp2->d_name);

              for (q = name_head, prev = NULL; q != NULL; q = q->next) {
                if (stricmp(q->name, dp2->d_name) > 0)
                  break;
                prev = q;
              }

              if (prev == NULL) /* Add at head */
              {
                p->next = name_head;
                name_head = p;
              } else {
                p->next = prev->next;
                prev->next = p;
              }
            }
          }

          closedir(dfu2);
        }
      }
    }

    closedir(dfu1);
  }

  for (p = name_head; p != NULL; p = p->next) {
    if (decomp) {
      // converted to snprintf() -gwb 23Feb20
      if (snprintf(src, MAX_PATHNAME_LEN + 1, "%s%c%c%c%s", terminfodir, DS,
                   p->name[0], DS, p->name) >= (MAX_PATHNAME_LEN + 1)) {
        printf("Overflow of max file/pathname.  Trucated to \n\"%s\"\n", src);
      }
      decompile_entry();
    } else
      printf("%s\n", p->name);
  }
}

#ifdef BIG_ENDIAN_SYSTEM

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

#endif

/* END-CODE */
