/* QMFIX.C
 * QM file fix tool
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
 * 17Oct25 mab Change dyn file prefix to %
 * 12Sep25 gwb Git Issue #86: Uncomment out code that was commented out in error...in 2020.
 * 03Sep25 gwb Remove K&R-isms.
 * 15Jan22 gwb Fixed dozens of instances of "Wrong type of arguments to fomatting
 *             function" that were reported by CodeQL. Tagged as CWE-686 by CodeQL.
 * 
 * 09Jan22 gwb Various fixes for sscanf() and printf() format codes that needed
 *             adjusting in order to stop throwing warnings under 32 and 64 bit
 *             target builds.
 *
 * 27Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 * 
 * 24Feb20 gwb Changed a number of sprintf() to snprintf() and converted
 *             any K&R function declarations to ANSI.
 *             The K&R conversion exposed a bug where the function prototype
 *             for delete_subfile() had the parameter declared as an int, but
 *             the actual function used a int16_t.  Based on how it was called,
 *             the int16_t version was deemed to be the correct one.
 * START-HISTORY (OpenQM):
 * 07 Nov 07  2.6-5 Extended to handle intermediate unused groups when doing
 *                  rebuild operation.
 * 12 Apr 07  2.5-2 Added remapping of ECB to recover_space().
 * 13 Dec 06  2.4-17 0529 recover_space() used wrong subfile index when moving
 *                   overflow block with large record.
 * 06 Nov 06  2.4-16 0526 -R option miscalculated forward link for large record
 *                   chain.
 * 31 Oct 06  2.4-15 0524 Calculation of new primary file size in -R option did
 *                   not cast values to int64 and could overflow.
 * 01 Aug 06  2.4-10 Added C option.
 * 20 Jul 06  2.4-10 Major rework of recover option.
 * 05 Apr 06  2.4-1 Set default character maps on entry.
 * 17 Mar 06  2.3-8 Fix record count.
 * 11 Oct 05  2.2-14 Added w="xxx".
 * 10 Oct 05  2.2-14 Implemented akpath element of file header.
 * 20 Sep 05  2.2-11 Use dynamic buffer in show_group() to minimise stack size.
 * 23 Aug 05  2.2-8 0396 Big record movement for -R used incorrect address.
 * 23 Aug 05  2.2-8 0395 Windows wildcard processing was incorrect.
 * 04 Aug 05  2.2-7 0386 Various mapping errors that caused QMFix to report
 *                  errors in healthy files. Going on to fix the file would then
 *                  corrupt it as it used the incorrect map.
 * 16 May 05  2.1-15 Use Seek() macro to improve portability.
 * 13 May 05  2.1-15 Introduced I64() to work around vararg inability to handle
 *                   int64.
 * 11 May 05  2.1-14 Extensive changes for large files.
 * 11 Oct 04  2.0-5 Added trigger modes to header report.
 * 28 Sep 04  2.0-3 Replaced GetSysPaths() with GetConfigPath().
 * 27 Sep 04  2.0-3 Added support for case insensitive files.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 *   QMFIX {options} pathnames
 *
 *   Options:
 *      -B      Suppress progress bars
 *      -C      Check file (implied if no other mode options)
 *      -F      Fix errors unconditionally
 *      -I      Interactive mode (restricted, needs DEBUG & 2 in config file)
 *      -L      Log output to qmfix.log
 *      -Lxxx   Log output to named file
 *      -Q      Fix errors after querying
 *      -R      Recover disk space (primary and overflow)
 *      -V      Verbose (not documented but not restricted)
 *
 *
 * The action of the dump and rebuild operation on finding that a mis-hashed
 * record is also present in its correct position is determined by the
 * REPLACE_DUPLICATE define token.  If this is defined, the correctly hashed
 * record is replaced by the mis-hashed one.  If this is not defined, the
 * mis-hashed record is discarded.
 *
 * Some parts of the utility may not behave perfectly if record ids contain
 * ASCII nul characters.
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
#include <setjmp.h>
#include <ctype.h>

#define Public
#define init(a) = a

#include "qm.h"
#include "dh_int.h"
#include "revstamp.h"

#include <unistd.h>

#define Seek(fu, offset, whence) lseek(fu, offset, whence)

#include <signal.h>
#include "linuxlb.h"
#define strnicmp(a, b, n) strncasecmp(a, b, n)

#define MAX_SUBFILES (AK_BASE_SUBFILE + MAX_INDICES)

/* GetLink() / SetLink() - Internal form = offset, file form by version */
#define GetLink(x) ((int64)((((x) == 0) || (header.file_version < 2)) ? (x) : (((x)-1) * (int64)group_bytes + header_bytes)))
#define SetLink(x) (((x) == 0) || (header.file_version < 2)) ? (x) : (((x)-header_bytes) / group_bytes + 1)

/* GetAKLink() / SetAKLink() - Internal form = offset, file form by version */
#define GetAKLink(x) ((int64)((((x) == 0) || (header.file_version < 2)) ? (x) : (((x)-1) * (int64)DH_AK_NODE_SIZE + ak_header_size)))
#define SetAKLink(x) (((x) == 0) || (header.file_version < 2)) ? (x) : (((x)-ak_header_size) / DH_AK_NODE_SIZE + 1)

/* GetAKNodeNum() - Get node number from version dependent file link */
#define GetAKNodeNum(x) ((int32_t)(((x) != 0) && (header.file_version >= 2)) ? (x) : (((x)-ak_header_size) / DH_AK_NODE_SIZE + 1))

/* OffsetToNode() - Translate node offset to node number */
#define OffsetToNode(x) ((int32_t)(((x)-ak_header_size) / DH_AK_NODE_SIZE + 1))

static u_char cfg_debug = 0; /* DEBUG config parameter */

static bool suppress_bar = FALSE; /* -B */
static bool check = FALSE;        /* -C */
static bool interactive = FALSE;  /* -I */
static bool logging = FALSE;      /* -L */
static int16_t fix_level = 0;     /* 1 = -Q, 2 = -F */
static bool recover = FALSE;      /* -R */
static bool verbose = FALSE;      /* -V */

static FILE *log = NULL;

static bool file_found = FALSE;
static char filename[MAX_PATHNAME_LEN + 1] = ""; /* File being processed */
static int fu[MAX_SUBFILES];                     /* File variables */
static int16_t subfile = 0;                      /* Currently selected subfile */
static DH_HEADER header;                         /* Primary subfile header */
static DH_HEADER oheader;                        /* Overflow subfile header */
static int16_t header_bytes;                     /* Offset of group 1 */
static int16_t ak_header_size;
static int16_t group_bytes;
static int64 load_bytes;

int16_t display_lines; /* Available lines for display */
int16_t emitted_lines; /* Lines emitted */
static jmp_buf quit_command;
static int64 display_addr = 0; /* Display address */
int16_t window_size = 256;
static bool quit = FALSE;
static bool bar_displayed = FALSE;

#define HASH_ERROR_LIMIT 50 /* Maximum reported hashing errors */

/* Automatic group dump and reload */
#define MAX_GROUPS_TO_DUMP 50
int16_t num_groups_to_dump = 0;
int32_t groups_to_dump[MAX_GROUPS_TO_DUMP];
char dump_file_name[20];
int dump_fu;

int32_t *map = NULL;
int32_t map_offset = 0;
int32_t num_overflow_blocks; /* Size of map */

void G_command(int32_t grp);
void GA_command(void);
void H_command(void);
void L_command(char *id, int16_t id_len);
void S_command(int32_t n);
void W_command(char *cmnd);
void Z_command(void);

void fix_file(char *fn);
bool is_dh_file(void);
int process_file(void);
void show_window(int64 addr);
void show_group(int32_t group);
Private void show_node(int32_t group);
bool check_file(void);
bool open_dump_file(void);
bool rebuild_group(int32_t grp);
bool dump_group(int32_t grp);
bool write_dumped_data(void);
bool write_record(DH_RECORD *rec);
bool release_chain(int64 offset);
int32_t get_overflow(void);
bool clear_group(int32_t grp);
bool check_and_fix(void);
bool open_subfile(int16_t subfile);
bool read_block(int16_t subfile, int64 offset, int16_t bytes, char *buff);
bool write_block(int16_t subfile, int64 offset, int16_t bytes, char *buff);
bool read_header(void);
bool write_header(void);
bool delete_subfile(int16_t subfile);
bool read_qmconfig(void);
bool map_big_rec(int16_t rec_hdr_sf, int64 rec_hdr_offset, DH_RECORD *rec_ptr, int32_t *map);
void tag_block(int64 offset, int16_t ref_subfile, int64 ref_offset, int16_t usage);
void map_node(int16_t sf, int32_t node, char *akmap, int32_t num_nodes, int32_t parent);
void map_ak_free_chain(int16_t sf, char *map, int32_t num_nodes);
bool recover_space(void);
bool fix(void);
bool yesno(char *prompt);
void emit(char msg[], ...);
char *I64(int64 x);
void memupr(char *str, int16_t len);
char *strupr(char *s);
void progress_bar(int32_t grp);

void event_handler(int signum);

/* ====================================================================== */

int main(int argc, char *argv[]) {
  int status = 1;
  int arg;
  int argx;
  bool arg_end;
  int16_t sf;

  for (sf = 0; sf < MAX_SUBFILES; sf++)
    fu[sf] = -1;

  set_default_character_maps();

  display_lines = 24;

  emitted_lines = 0;

  emit(
      "[ QMFIX %s   Copyright, Ladybridge Systems, %s.  All rights reserved. "
      "]\n\n",
      QM_REV_STAMP, QM_COPYRIGHT_YEAR);

  filename[0] = '\0';

  if (!read_qmconfig())
    goto exit_qmfix;

  signal(SIGINT, event_handler);

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
          suppress_bar = TRUE;
          break;

        case 'C':
          check = TRUE;
          break;

        case 'F': /* Fix */
          if (fix_level)
            goto usage;
          fix_level = 2;
          break;

        case 'I': /* Interactive (if enabled) */
          if (!(cfg_debug & 0x02))
            goto usage;
          interactive = TRUE;
          break;

        case 'L': /* Logging */
          if (argv[arg][++argx] == '\0')
            log = fopen("qmfix.log", "wt");
          else
            log = fopen(argv[arg] + argx, "wt");
          if (log == NULL) {
            perror("Unable to open log file");
            goto exit_qmfix;
          }
          logging = TRUE;
          arg_end = TRUE;
          break;

        case 'Q': /* Query fix */
          if (fix_level)
            goto usage;
          fix_level = 1;
          break;

        case 'R':
          recover = TRUE;
          break;

        case 'V':
          verbose = TRUE;
          break;

        default:
          goto usage;
      }

      argx++;
    } while (!arg_end);
  }

  if (!(check || fix_level || interactive || recover))
    check = TRUE;

  if (fix_level && !(interactive || recover))
    check = TRUE;

  if (arg == argc)
    goto usage; /* No filenames */

  while (arg < argc) {
    fix_file(argv[arg]);
    arg++;
  }

  if (!file_found)
    printf("No QM hashed files found matching given name(s)\n");

exit_qmfix:
  return status;

usage:
  printf("Usage: QMFIX {options} pathname(s)\n\n");
  printf("Options:\n");
  printf("   -F      Fix errors unconditionally\n");
  printf("   -L      Log output to qmfix.log\n");
  printf("   -Lxxx   Log output to named file\n");
  printf("   -Q      Fix errors after querying\n");
  printf("   -R      Recover disk space\n");

  return status;
}

/* ======================================================================
   fix_file()                                                             */

void fix_file(char *fn) {
  //int status;

  strcpy(filename, fn);

  if (is_dh_file()) {
    if (file_found)
      emit("\n\n");
    file_found = TRUE;
    //status = process_file(); /* git issue #86 */
    process_file();

  }
}

/* ======================================================================
   is_dh_file()                                                           */

bool is_dh_file() {
  bool status = FALSE;
  int dhfu = -1;
  char pathname[MAX_PATHNAME_LEN + 1];
  struct stat statbuf;
  // converted to snprintf() -gwb 23Feb20
  // 17Oct25 mab Change dyn file prefix to %
  if (snprintf(pathname, MAX_PATHNAME_LEN + 1, "%s%c%%0", filename, DS) >= (MAX_PATHNAME_LEN + 1)) {
    emit("Overflow of max file/pathname size. Truncated to:\n\"%s\"\n", pathname);
  }

  if (stat(pathname, &statbuf) != 0)
    goto exit_is_dh_file;

  if (!(statbuf.st_mode & S_IFREG))
    goto exit_is_dh_file;

  dhfu = open(pathname, (int)(O_RDWR | O_BINARY), default_access);
  if (dhfu < 0)
    goto exit_is_dh_file;

  if (read(dhfu, (u_char *)&header, DH_HEADER_SIZE) != DH_HEADER_SIZE) {
    goto exit_is_dh_file;
  }

  /* Check magic number in primary subfile */

  if (header.magic != DH_PRIMARY)
    goto exit_is_dh_file;

  status = TRUE;

exit_is_dh_file:
  if (dhfu >= 0)
    close(dhfu);
  return status;
}

/* ======================================================================
   process_file()  -  Process a file                                      */

int process_file() {
  int status = 1;
  char cmnd[80 + 1];
  char u_cmnd[80 + 1];
  bool done = FALSE;
  int16_t sf; /* Subfile index */
  int16_t i;
  int32_t n;
  int64 n64;
  char *p;

  emit("Processing %s\n", filename);

  /* Open primary and overflow subfiles */

  if (!open_subfile(PRIMARY_SUBFILE)) {
    perror("Cannot open primary subfile");
    goto exit_process_file;
  }

  /* Read primary subfile header */

  if (!read_header()) {
    perror("Cannot read primary subfile header");
    goto exit_process_file;
  }

  /* Check magic number in primary subfile */

  switch (header.magic) {
    case DH_PRIMARY:
      break;
    default:
      emit("Primary subfile has incorrect magic number (%04X)\n", (int)(header.magic));
      if (!interactive)
        goto exit_process_file;
      break;
  }

  if (!open_subfile(OVERFLOW_SUBFILE)) {
    perror("Cannot open overflow subfile");
    goto exit_process_file;
  }

  /* Read overflow subfile header */

  if (!read_block(OVERFLOW_SUBFILE, 0, DH_HEADER_SIZE, (char *)&oheader)) {
    perror("Cannot read overflow subfile header");
    goto exit_process_file;
  }

  /* Check magic number in overflow subfile */

  if (oheader.magic != DH_OVERFLOW) {
    emit("Overflow subfile has incorrect magic number (%04X)\n", (int)(oheader.magic));
    if (!interactive)
      goto exit_process_file;
  }

  group_bytes = header.group_size;

  /* Check for each AK subfile and open */

  for (i = 0; i < MAX_INDICES; i++) {
    (void)open_subfile(AK_BASE_SUBFILE + i);
  }

  /* Check file version is supported */

  /* !!FILE_VERSION!! */
  if (header.file_version > DH_VERSION) {
    emit("Unrecognised file version (%d)\n", (int)header.file_version);
    if (!interactive)
      goto exit_process_file;
  }

  header_bytes = DHHeaderSize(header.file_version, header.group_size);
  ak_header_size = AKHeaderSize(header.file_version);

  if (interactive) {
    if (setjmp(quit_command)) {
    }

    do {
      emit("%d?", (int)subfile);
      *cmnd = '\0'; /* For quit key path which aborts gets() */
      fgets(cmnd, 80, stdin);
      if ((p = strchr(cmnd, '\n')) != NULL)
        *p = '\0';
      if (logging)
        emit("%s\n", cmnd);
      emitted_lines = 0;
      quit = FALSE;

      strcpy(u_cmnd, cmnd);
      strupr(u_cmnd);

      if (cmnd[0] == '\0') {
      } else if (stricmp(cmnd, "CHECK") == 0)
        check_file();
      else if (stricmp(cmnd, "FIX") == 0)
        check_and_fix();
      else if (sscanf(u_cmnd, "G%d", &n) == 1)
        G_command(n);
      else if (stricmp(cmnd, "GA") == 0)
        GA_command();
      else if (stricmp(cmnd, "H") == 0)
        H_command();
      else if (u_cmnd[0] == 'L')
        L_command(cmnd + 1, strlen(cmnd + 1));
      else if (stricmp(cmnd, "N") == 0)
        show_window(display_addr);
      else if (stricmp(cmnd, "P") == 0)
        show_window(display_addr - 2 * window_size);
      else if (stricmp(cmnd, "Q") == 0)
        done = TRUE;
      else if (sscanf(u_cmnd, "REBUILD %d", &n) == 1)
        rebuild_group(n);
      else if (stricmp(cmnd, "RECOVER") == 0)
        recover_space();
      else if (sscanf(u_cmnd, "S%d", &n) == 1)
        S_command(n);
      else if (strnicmp(cmnd, "W", 1) == 0)
        W_command(cmnd + 1);
#ifndef __LP64__
      else if (sscanf(u_cmnd, "%lld", &n64) == 1)
#else
      else if (sscanf(u_cmnd, "%ld", &n64) == 1)
#endif
        show_window(n64);
      else if (stricmp(cmnd, "Z") == 0)
        Z_command();
      else if (strcmp(cmnd, "?") == 0) {
        emit("CHECK      check file integrity\n");
        emit("FIX        check file integrity, fixing errors\n");
        emit("Gn         display group n\n");
        emit("GA         display all groups\n");
        emit("H          display header\n");
        emit("Lxxx       locate record xxx by hashing\n");
        emit("N          show next page of data window\n");
        emit("P          show previous page of data window\n");
        emit("Q          quit\n");
        emit("REBUILD n  Dump and rebuild group n\n");
        emit("RECOVER    recover disk space\n");
        emit("Sn         select subfile\n");
        emit(
            "Wn=x       write hex value (1, 2 or 4 bytes in file byte "
            "order)\n");
        emit("Z          show statistic counters\n");
        emit("n          show window from given hex address\n");
      } else {
        emit("Unrecognised command. Enter ? for help\n");
      }
    } while (!done);
  } else /* Automatic mode */
  {
    if (setjmp(quit_command))
      goto exit_process_file;

    /* Perform automatic checks */

    if (check) {
      if (!check_file())
        goto exit_process_file;
    }

    if (recover)
      recover_space();
  }

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
   G_command()  -  Gn command                                             */

void G_command(int32_t grp) {
  if (grp > 0) {
    if (subfile < 2)
      show_group(grp);
    else
      show_node(grp);
  } else {
    emit("Invalid group number\n");
  }
}

/* ======================================================================
  GA_command  -  Show all groups                                          */

void GA_command() {
  int32_t grp;

  for (grp = 1; grp <= header.params.modulus; grp++) {
    show_group(grp);
    if (quit)
      break;
  }
}

/* ======================================================================
   H_command()  -  Show header                                            */

void H_command() {
  int32_t load;
  DH_AK_HEADER ak_header;
  int64 ak_node_offset;

  load = DHLoad(load_bytes, header.group_size, header.params.modulus);

  emit("%04X: File version  %d\n", offsetof(DH_HEADER, file_version), (int)header.file_version);
  emit("      Header size   %d (x%04X)\n", header_bytes, header_bytes);
  emit("%04X: Group size    %d (x%04X)\n", offsetof(DH_HEADER, group_size), (int)(header.group_size), (int)(header.group_size));
  emit("%04X: Modulus       %d\n", offsetof(DH_HEADER, params.modulus), header.params.modulus);
  emit("%04X: Min modulus   %d\n", offsetof(DH_HEADER, params.min_modulus), header.params.min_modulus);
  emit("%04X: Big record    %d\n", offsetof(DH_HEADER, params.big_rec_size), header.params.big_rec_size);
  emit("%04X: Split load    %d\n", offsetof(DH_HEADER, params.split_load), (int)header.params.split_load);
  emit("%04X: Merge load    %d\n", offsetof(DH_HEADER, params.merge_load), (int)header.params.merge_load);
  emit("      Load bytes    %s (%d%%)\n", I64(load_bytes), load);
  emit("%04X: Mod value     %d\n", offsetof(DH_HEADER, params.mod_value), header.params.mod_value);
  emit("%04X: Longest id    %d\n", offsetof(DH_HEADER, params.longest_id), header.params.longest_id);
  emit("%04X: Free chain    %08X  x%s\n", offsetof(DH_HEADER, params.free_chain), header.params.free_chain, I64(GetLink(header.params.free_chain)));
  emit("%04X: Flags         x%04X\n", offsetof(DH_HEADER, flags), (int)header.flags);
  emit("%04X: AK map        %08X  x%s\n", offsetof(DH_HEADER, ak_map), header.ak_map, I64(GetLink(header.ak_map)));
  emit("%04X: Trigger name  '%s'\n", offsetof(DH_HEADER, trigger_name), header.trigger_name);
  emit("%04X: Trigger modes x%02X\n", offsetof(DH_HEADER, trigger_modes), header.trigger_modes);
  emit("%04X: Journal file  %d\n", offsetof(DH_HEADER, jnl_fno), header.jnl_fno);
  emit("%04X: AK path       '%s'\n", offsetof(DH_HEADER, akpath), (header.akpath[0] == '\0') ? "" : header.akpath);
  emit("%04X: Creation time %d\n", offsetof(DH_HEADER, creation_timestamp), header.creation_timestamp);
  emit("%04X: Record count  x%s (actual count is one less)\n", offsetof(DH_HEADER, record_count), I64(header.record_count));
  emit("\n");

  if (subfile >= AK_BASE_SUBFILE) {
    emit("AK %d header:\n", (int)(subfile - AK_BASE_SUBFILE));
    if (!read_block(subfile, 0, DH_AK_HEADER_SIZE, (char *)&ak_header)) {
      emit("Read error\n");
      return;
    }

    emit("%04X: Flags        x%04X\n", offsetof(DH_AK_HEADER, flags), (int)(ak_header.flags));
    emit("%04X: Field number %d\n", offsetof(DH_AK_HEADER, fno), (int)ak_header.fno);
    ak_node_offset = GetAKLink(ak_header.free_chain);
    emit("%04X: Free chain   x%s, Node %ld\n", offsetof(DH_AK_HEADER, free_chain), I64(ak_node_offset), OffsetToNode(ak_node_offset));
    emit("%04X: I-type bytes %d\n", offsetof(DH_AK_HEADER, itype_len), ak_header.itype_len);
    ak_node_offset = GetAKLink(ak_header.itype_ptr);
    emit("%04X: I-type ptr   x%s, Node %ld\n", offsetof(DH_AK_HEADER, itype_ptr), I64(ak_node_offset), OffsetToNode(ak_node_offset));
    emit("      AK name      '%s'\n", ak_header.ak_name);
    emit("%04X: Data created %d\n", offsetof(DH_AK_HEADER, data_creation_timestamp), ak_header.data_creation_timestamp);
    emit("%04x: Map name     %s\n", offsetof(DH_AK_HEADER, collation_map_name), ak_header.collation_map_name);
  }
}

/* ======================================================================
   L_command()  -  Locate record by hashing                               */

void L_command(char *id, int16_t id_len) {
  char u_id[MAX_ID_LEN];
  int32_t grp;
  int32_t hash_value;

  if (id_len == 0)
    emit("Illegal record id\n");
  else {
    if (header.flags & DHF_NOCASE) {
      memucpy(u_id, id, id_len);
      hash_value = hash(u_id, id_len);
    } else {
      hash_value = hash(id, id_len);
    }

    grp = (hash_value % header.params.mod_value) + 1;
    if (grp > header.params.modulus) {
      grp = (hash_value % (header.params.mod_value >> 1)) + 1;
    }

    emit("Hash value %08X, Group %d, Address x%s\n", hash_value, grp, I64(((int64)(grp - 1) * header.group_size) + header_bytes));
  }
}

/* ======================================================================
   S_command  -  Select subfile                                           */

void S_command(int32_t n) {
  int64 bytes;

  if ((n >= 0) && (n < MAX_SUBFILES)) {
    if (fu[n] >= 0) {
      subfile = n;
      bytes = filelength64(fu[n]);
      emit("Subfile %d: x%s bytes, ", (int)n, I64(bytes));
      switch (subfile) {
        case PRIMARY_SUBFILE:
          printf("%d group buffers\n", (int32_t)((bytes - header_bytes) / header.group_size));
          break;

        case OVERFLOW_SUBFILE:
          printf("%d overflow buffers\n", (int32_t)((bytes - header_bytes) / header.group_size));
          break;

        default:
          printf("%d node buffers\n", (int32_t)((bytes - ak_header_size) / DH_AK_NODE_SIZE));
          break;
      }
    } else
      emit("Subfile not open\n");
  } else
    emit("Invalid subfile number\n");
}

/* ======================================================================
   W_command()  -  Write data                                             */

void W_command(char *cmnd) {
  u_int32_t w_addr;
  u_int32_t w_data;
  u_int32_t old_data;
  int16_t n;
  int16_t bytes;
  char *p;
  char c;

  n = sscanf(cmnd, "%d=%d", &w_addr, &w_data); /* was %lx for both - 64bit fix -gwb*/

  if (n < 1)
    goto w_format_error; /* Address component not present */

  if (n == 2) /* Hex assignment */
  {
    n = strlen(cmnd) - (strchr(cmnd, '=') - cmnd) - 1; /* Chars after '=' */
    switch (n) {
      case 2:
        break;
      case 4:
        w_data = ((w_data & 0x0000FF00L) >> 8) | ((w_data & 0x000000FFL) << 8);
        break;
      case 8:
        w_data = (w_data >> 24) | ((w_data & 0x00FF0000L) >> 8) | ((w_data & 0x0000FF00L) << 8) | ((w_data & 0x000000FFL) << 24);
        break;
      default:
        emit("Illegal data length\n");
        return;
    }

    bytes = n >> 1; /* Convert to bytes */

    if ((w_addr < 0) || (w_addr + bytes > filelength64(fu[subfile]))) {
      emit("Illegal address\n");
      return;
    }

    if (Seek(fu[subfile], w_addr, SEEK_SET) < 0) {
      emit("Seek error\n");
      return;
    }

    old_data = 0;
    read(fu[subfile], &old_data, bytes);

    switch (n) {
      case 2:
        emit("Was %02X\n", old_data);
        break;
      case 4:
        old_data = ((old_data & 0x0000FF00L) >> 8) | ((old_data & 0x000000FFL) << 8);
        emit("Was %04X\n", old_data);
        break;
      case 8:
        old_data = (old_data >> 24) | ((old_data & 0x00FF0000L) >> 8) | ((old_data & 0x0000FF00L) << 8) | ((old_data & 0x000000FFL) << 24);
        emit("Was %08X\n", old_data);
        break;
    }

    if (Seek(fu[subfile], w_addr, SEEK_SET) < 0) {
      emit("Seek error\n");
      return;
    }

    write(fu[subfile], &w_data, bytes);

    if (subfile == 0) {
      if (!read_header()) {
        emit("Error re-reading primary subfile header\n");
      }
    }
  } else if ((n == 1) && ((p = strchr(cmnd, '=')) != NULL) && (((c = *(++p)) == '"') || (c == '\'')) && ((n = strlen(p)) >= 2) && (p[n - 1] == c)) {
    if ((w_addr < 0) || (w_addr + n - 2 > filelength64(fu[subfile]))) {
      emit("Illegal address\n");
      return;
    }

    if (Seek(fu[subfile], w_addr, SEEK_SET) < 0) {
      emit("Seek error\n");
      return;
    }

    write(fu[subfile], p + 1, n - 2);
  } else
    goto w_format_error;

  return;

w_format_error:
  emit("Format error in W command\n");
}

/* ======================================================================
   Z_command()  -  Show file statistics                                   */

void Z_command() {
  emit("Reset time       %d\n", header.stats.reset);
  emit("Open count       %d\n", header.stats.opens);
  emit("Read count       %d\n", header.stats.reads);
  emit("Write count      %d\n", header.stats.writes);
  emit("Delete count     %d\n", header.stats.deletes);
  emit("Clear count      %d\n", header.stats.clears);
  emit("Select count     %d\n", header.stats.selects);
  emit("Split count      %d\n", header.stats.splits);
  emit("Merge count      %d\n", header.stats.merges);
  emit("AK read count    %d\n", header.stats.ak_reads);
  emit("AK write count   %d\n", header.stats.ak_writes);
  emit("AK delete count  %d\n", header.stats.ak_deletes);
}

/* ====================================================================== */

void show_window(int64 addr) {
  unsigned char buff[1024];
  int64 buff_addr = -1;
  int64 base_addr;
  int buff_offset;
  int16_t i;
  unsigned char c;
  int16_t bytes;

  addr &= ~0xF; /* Align on 16 byte boundary */
  bytes = window_size;

  if ((addr < 0) || (addr > filelength64(fu[subfile]))) {
    emit("Invalid address\n");
    return;
  }

  while (bytes > 0) {
    base_addr = addr & ~1023;
    if (base_addr != buff_addr) {
      buff_addr = base_addr;

      if (!read_block(subfile, buff_addr, 1024, (char *)buff)) {
        emit(("Read error\n"));
        break;
      }
    }

    /* 0.012345678901: 12345678  12345678  12345678  12345678  | ................ */

    buff_offset = addr - buff_addr;

    emit("%d.%s: ", (int)subfile, I64(addr));
    for (i = 0; i < 16; i++) {
      emit("%02X", (int)(*(buff + buff_offset + i)));
      if (i % 4 == 3)
        emit("  ");
    }

    emit("| ");
    for (i = 0; i < 16; i++) {
      c = *(buff + buff_offset + i);
      if ((c < 32) || (c > 126))
        c = '.';
      emit("%c", c);
    }
    emit("\n");

    addr += 16;
    bytes -= 16;
  }

  display_addr = addr;
}

/* ======================================================================
   check_and_fix()  -  Perform automatic checks , fixing errors           */

bool check_and_fix() {
  bool status;

  fix_level = 2;
  status = check_file();
  fix_level = 0;
  return status;
}

/* ======================================================================
   check_file()  -  Perform automatic checks                              */

bool check_file() {
  bool status = FALSE;
  int16_t akno;
  DH_AK_HEADER ak_header;
  int64 scan_load_bytes = 0;   /* Actual load bytes from scan */
  int64 scan_record_count = 0; /* Actual record count from scan */
  int32_t apparent_modulus;    /* Calculated by examining groups */
  int32_t grp;                 /* Group number during scan */
  int16_t sf;                  /* Subfile index */
  int64 offset;                /* Buffer offset into subfile */
  DH_BLOCK *buff = NULL;
  DH_BLOCK *ebuff = NULL;
  int16_t id_len;
  char id[MAX_ID_LEN + 1];
  int32_t hash_value;
  int32_t hgroup;
  int16_t hash_errors = 0;
  int16_t used_bytes;
  int16_t rec_offset;
  int16_t rec_size;
  DH_RECORD *rec_ptr;
  int32_t released_group = 0;
  int32_t last_offset;
  int32_t free_blocks;
  int16_t ak_subfile;
  int16_t longest_id = 0;
  int64 next_buffer;
  int64 overflow_space;

  int32_t num_nodes;
  char *ak_map = NULL;

#define CHK_FREE 0
#define CHK_OVERFLOW 1
#define CHK_BIG_REC 2
#define CHK_ECB 3

  int32_t n;
  int64 n64;
  char *p;
  char *q;
  int16_t i;

  num_groups_to_dump = 0;

  buff = (DH_BLOCK *)malloc(DH_MAX_GROUP_SIZE_BYTES);

  emit("Checking file header...\n");

  /* Check group size is valid */

  if ((group_bytes & (DH_GROUP_MULTIPLIER - 1)) || (group_bytes < DH_GROUP_MULTIPLIER) || (group_bytes > DH_MAX_GROUP_SIZE_BYTES)) {
    emit("Invalid group size (%d bytes)\n", group_bytes);
    goto exit_check;
  }

  /* Allocate space map */

  overflow_space = filelength64(fu[OVERFLOW_SUBFILE]) - header_bytes;

  num_overflow_blocks = overflow_space / group_bytes;

  n = (num_overflow_blocks + 1) * sizeof(int32_t);
  map = malloc(n);
  if (map == NULL) {
    emit("Unable to allocate space map\n");
    goto exit_check;
  }
  memset(map, 0, n);

  /* Check big record size */

  if ((header.params.big_rec_size < 0) || (header.params.big_rec_size > (header.group_size - BLOCK_HEADER_SIZE))) {
    emit("Large record size is invalid (%d)\n", header.params.big_rec_size);
    if (fix()) {
      header.params.big_rec_size = header.group_size - BLOCK_HEADER_SIZE;
      if (!write_header())
        goto exit_check;
      emit("   Corrected by resetting to default\n");
    }
  }

  /* Check split and merge loads */

  if ((header.params.split_load < 0) || (header.params.merge_load < 0) || (header.params.split_load <= header.params.merge_load)) {
    emit("Invalid split and/or merge loads (%d, %d)\n", (int)(header.params.split_load), (int)(header.params.merge_load));
    if (fix()) {
      header.params.split_load = 50;
      header.params.merge_load = 80;
      if (!write_header())
        goto exit_check;
      emit("   Corrected by resetting to defaults (50, 80)\n");
    }
  }

  /* Check for each AK subfile and open */

  for (akno = 0; akno < MAX_INDICES; akno++) {
    sf = AK_BASE_SUBFILE + akno;
    if (fu[sf] >= 0) {
      if (header.ak_map & (1 << akno)) /* Expected this AK subfile */
      {
        /* Read AK subfile header to check magic number */

        if (!read_block(sf, 0, DH_AK_HEADER_SIZE, (char *)&ak_header)) {
          emit("Cannot read AK%d subfile header", (int)akno);
          if (fix()) {
            delete_subfile(sf);
            header.ak_map &= ~(1 << akno);
            if (!write_header())
              goto exit_check;
            emit("   Corrected by deleting affected AK index subfile\n");
            emit("   AK must be recreated and rebuilt\n");
          }
        }

        if (ak_header.magic != DH_INDEX) {
          emit("AK%d subfile has incorrect magic number (%04X)\n", (int)akno, (int)(ak_header.magic));
          if (fix()) {
            delete_subfile(sf);
            header.ak_map &= ~(1 << akno);
            if (!write_header())
              goto exit_check;
            emit("   Corrected by deleting affected AK index subfile\n");
            emit("   AK must be recreated and rebuilt\n");
          }
        }
      } else /* Did not expect this AK subfile */
      {
        emit("AK%d subfile found but not expected\n", (int)akno);
        if (fix()) {
          delete_subfile(sf);
          emit("   Corrected by deleting affected AK index subfile\n");
        }
      }
    } else {
      if (header.ak_map & (1 << akno)) /* Expected this AK subfile */
      {
        emit("AK%d subfile not found\n", (int)akno);
        if (fix()) {
          header.ak_map &= ~(1 << akno);
          if (!write_header())
            goto exit_check;
          emit(
              "   Corrected by removing reference to missing AK index "
              "subfile\n");
        }
      } else /* Did not expect this AK subfile */
      {
      }
    }
  }

  /* Walk through primary groups */

  emit("Checking data groups...\n");

  apparent_modulus = 0;
  grp = 1;
  while (1) {
    if (quit)
      goto exit_check;

    progress_bar(grp);

    sf = PRIMARY_SUBFILE;
    offset = (((int64)(grp - 1)) * group_bytes) + header_bytes;

    if (!read_block(sf, offset, group_bytes, (char *)buff))
      break;

    used_bytes = buff->used_bytes;
    if (used_bytes == 0) /* Looks like a released primary group */
    {
      if (released_group == 0)
        released_group = grp;
    } else /* Looks like an active group */
    {
      if (released_group != 0) {
        emit("Active group %d found after released group(s)!\n", grp);
        if (fix()) {
          if (ebuff == NULL) {
            ebuff = (DH_BLOCK *)malloc(group_bytes);
            memset(ebuff, 0, group_bytes);
            ebuff->next = 0;
            ebuff->used_bytes = BLOCK_HEADER_SIZE;
            ebuff->block_type = DHT_DATA;
          }

          emit("   Corrected by initialising %d groups starting at %d\n", grp - released_group, released_group);

          while (released_group < grp) {
            write_block(PRIMARY_SUBFILE, (((int64)(released_group - 1)) * group_bytes) + header_bytes, group_bytes, (char *)ebuff);
            released_group++;
          }
          released_group = 0;
        }
      }

      if (buff->block_type != DHT_DATA) {
        emit("Group %d has incorrect block type (%d)\n", grp, (int)(buff->block_type));
        if (fix()) {
          buff->block_type = DHT_DATA;
          write_block(sf, offset, group_bytes, (char *)buff);
          emit("   Corrected by setting block type\n");
        }
      }

      apparent_modulus = grp;

      /* Scan group */

      do {
        if (quit)
          goto exit_check;

        rec_offset = offsetof(DH_BLOCK, record);

        while (rec_offset < used_bytes) {
          scan_record_count++;

          rec_ptr = (DH_RECORD *)(((char *)buff) + rec_offset);
          rec_size = rec_ptr->next;
          scan_load_bytes += rec_size;

          id_len = rec_ptr->id_len;
          if (id_len > longest_id)
            longest_id = id_len;

          memcpy(id, rec_ptr->id, id_len);
          id[id_len] = '\0';
          if (header.flags & DHF_NOCASE)
            memupr(id, id_len);
          hash_value = hash(id, id_len);
          hgroup = (hash_value % header.params.mod_value) + 1;
          if (hgroup > header.params.modulus) {
            hgroup = (hash_value % (header.params.mod_value >> 1)) + 1;
          }

          if ((hgroup != grp) && (hash_errors < HASH_ERROR_LIMIT)) {
            hash_errors++;
            emit(
                "Record '%s' mis-hashed into group %d. Should be in group "
                "%d\n",
                id, grp, hgroup);

            /* Add group to list of groups to dump */

            if ((fix_level == 2) && ((num_groups_to_dump == 0) || (groups_to_dump[num_groups_to_dump - 1] != grp)) && (num_groups_to_dump < MAX_GROUPS_TO_DUMP)) {
              groups_to_dump[num_groups_to_dump++] = grp;
            }

            if (hash_errors == HASH_ERROR_LIMIT) {
              emit("Further hash errors will not be reported\n");
            }
          }

          if (rec_ptr->flags & DH_BIG_REC) {
            if (!map_big_rec(sf, offset, rec_ptr, map)) {
              emit(
                  "Error in large record. Group %d, %d.%s, id '%s'.\nRecord "
                  "cannot be recovered.\n",
                  grp, (int)sf, I64(offset + rec_offset), id);
              if (fix()) {
                /* Remove header part of faulty big record by sliding back
                    any further records.                                    */

                p = (char *)rec_ptr;
                q = p + rec_size;
                n = used_bytes - (rec_size + rec_offset);
                if (n > 0)
                  memmove(p, q, n);

                used_bytes -= rec_size;
                buff->used_bytes = used_bytes;

                if (!write_block(sf, offset, group_bytes, (char *)buff)) {
                  goto exit_check;
                }

                /* Adjust load */

                scan_load_bytes -= rec_size;
                load_bytes -= rec_size;
                if (!write_header())
                  goto exit_check;

                emit("   Corrected by deleting record\n");

                rec_size = 0; /* To reprocess this offset */
              }
            }

            if (quit)
              goto exit_check;
          }
          rec_offset += rec_size;
        }

        if ((next_buffer = GetLink(buff->next)) == 0)
          break;

        tag_block(next_buffer, sf, offset, CHK_OVERFLOW);

        sf = OVERFLOW_SUBFILE;
        offset = next_buffer;
        if (!read_block(sf, offset, group_bytes, (char *)buff))
          break;

        used_bytes = buff->used_bytes;
        if (used_bytes == 0)
          break; /* Looks like a released primary group */
      } while (1);
    }

    grp++;
  }
  progress_bar(-1);

  /* Check modulus */

  if (apparent_modulus != header.params.modulus) {
    emit("Apparent modulus %d differs from recorded modulus %d\n", apparent_modulus, header.params.modulus);
    if (fix()) {
      header.params.modulus = apparent_modulus;
      if (!write_header())
        goto exit_check;
      emit("   Corrected by updating header\n");
    }
  }

  /* Check mod_value */

  for (n = 1; n < header.params.modulus; n = n << 1) {
  }
  if (header.params.mod_value != n) {
    emit("Incorrect mod_value (%d), expected %d\n", header.params.mod_value, n);
    if (fix()) {
      header.params.mod_value = n;
      if (!write_header())
        goto exit_check;
      emit("   Corrected by setting to %d\n", n);
    }
  }

  /* Check minimum modulus */

  if ((header.params.min_modulus < 1) || (header.params.min_modulus > header.params.modulus)) {
    emit("Minimum modulus is invalid (%d)\n", header.params.min_modulus);
    if (fix()) {
      header.params.min_modulus = 1;
      if (!write_header())
        goto exit_check;
      emit("   Corrected by resetting to 1\n");
    }
  }

  /* Check load */

  if (scan_load_bytes != load_bytes) {
    emit("Load error: Actual %s, file header %s\n", I64(scan_load_bytes), I64(load_bytes));
    if (fix()) {
      load_bytes = scan_load_bytes;
      if (!write_header())
        goto exit_check;
      emit("   Corrected by updating header\n");
    }
  }

  /* Check record count */

  if (scan_record_count != header.record_count - 1) {
    if (header.record_count == 0) {
      emit("Header record count has not been set\n");
    } else {
      emit("Record count error: Actual %s, file header (adjusted) %s\n", I64(scan_record_count), I64(header.record_count - 1));
    }
    if (fix()) {
      header.record_count = scan_record_count + 1;
      if (!write_header())
        goto exit_check;
      emit("   Corrected by updating header\n");
    }
  }

  /* Correct the longest_id field. This is a silent fix as it is likely
    to be "wrong".  It isn't really an error at all but would prevent
    us opening the file if the current value is greater than the id
    length limit of the QM system we are using.                         */

  if (longest_id < header.params.longest_id) {
    if (fix()) {
      header.params.longest_id = longest_id;
      if (!write_header())
        goto exit_check;
    }
  }

  /* Now scan the free chain */

  emit("Checking free chain...\n");
  free_blocks = 0;
  last_offset = 0;
  for (offset = GetLink(header.params.free_chain); offset != 0; offset = GetLink(buff->next)) {
    if (quit)
      goto exit_check;

    if (!read_block(OVERFLOW_SUBFILE, offset, group_bytes, (char *)buff)) {
      emit("\nRead error at 1.%s\n", I64(offset));
      break;
    }

    if (buff->used_bytes) {
      emit("Overflow block x%s has non-zero used byte count\n", I64(offset));
      if (fix()) {
        n = buff->next;
        memset((char *)buff, 0, group_bytes);
        buff->next = n;
        write_block(OVERFLOW_SUBFILE, offset, group_bytes, (char *)buff);
        emit("   Corrected by clearing block\n");
      }
    }

    free_blocks++;

    tag_block(offset, OVERFLOW_SUBFILE, last_offset, CHK_FREE);

    if (header.file_version < 2) {
      if (buff->next & ((int32_t)(group_bytes - 1))) {
        emit("Faulty next block pointer x%08X at 1.%s\n", buff->next, I64(offset));
        break;
      }
    }

    last_offset = offset;
  }

  /* Process AK subfiles */

  for (akno = 0; akno < MAX_INDICES; akno++) {
    if (quit)
      goto exit_check;

    ak_subfile = AK_BASE_SUBFILE + akno;
    if (fu[ak_subfile] >= 0) {
      emit("Checking AK %d\n", (int)akno);

      /* Create a map for the AK subfile. Elements of this map are
        set to the node type for each node we process. Other values
        are:  -1  Unreferenced node
              -2  Seek error
              -3  Read error
              -4  Illegal node type                                 */

      n64 = filelength64(fu[ak_subfile]);
      num_nodes = (n64 - ak_header_size) / DH_AK_NODE_SIZE;
      ak_map = malloc(num_nodes + 1);
      if (ak_map == NULL) {
        emit("Unable to allocate AK map\n");
        goto exit_check;
      }
      memset(ak_map, -1, num_nodes + 1);

      map_node(ak_subfile, 1, ak_map, num_nodes, 0);
      if (quit)
        goto exit_check;

      /* Process AK subfile free chain */

      map_ak_free_chain(ak_subfile, ak_map, num_nodes);

      /* Scan map for unreferenced nodes */

      for (n = 1; n <= num_nodes; n++) {
        if (quit)
          goto exit_check;

        if (ak_map[n] == (char)-1) {
          emit("AK %d node %d unreferenced\n", (int)akno, n);
          if (fix()) {
            /* Add to AK free node chain */

            if (read_block(ak_subfile, 0, DH_AK_HEADER_SIZE, (char *)&ak_header)) {
              offset = ((int64)(n - 1)) * DH_AK_NODE_SIZE + ak_header_size;
              memset((char *)buff, 0, DH_AK_NODE_SIZE);
              ((DH_FREE_NODE *)buff)->node_type = AK_FREE_NODE;
              ((DH_FREE_NODE *)buff)->next = ak_header.free_chain;
              write_block(ak_subfile, offset, DH_AK_NODE_SIZE, (char *)buff);
              ak_header.free_chain = offset;
              write_block(ak_subfile, 0, DH_AK_HEADER_SIZE, (char *)&ak_header);
              emit("   Corrected by adding to free node chain\n");
            }
          }
        }
      }

      free(ak_map);
      ak_map = NULL;
    }
  }

  /* Finally, look for unreferenced blocks */

  emit("Checking for unreferenced blocks...\n");
  for (n = 1; n <= num_overflow_blocks; n++) {
    if (quit)
      goto exit_check;

    if (map[n] == 0) {
      offset = (((int64)(n - 1 + map_offset)) * group_bytes) + header_bytes;
      emit("Overflow block %d (x%s) unreferenced\n", n + map_offset, I64(offset));
      if (fix()) {
        /* Add to free chain */

        memset((char *)buff, 0, group_bytes);
        buff->next = header.params.free_chain;
        write_block(OVERFLOW_SUBFILE, offset, group_bytes, (char *)buff);
        header.params.free_chain = SetLink(offset);
        write_header();
        emit("   Corrected by adding to free chain\n");
      }
    }
  }

  emit("%d overflow blocks allocated, %d on free chain\n", num_overflow_blocks, free_blocks);

  /* Perform automatic dump and rebuild of faulty groups */

  if (num_groups_to_dump && yesno("Dump and rebuild groups with mis-hashed records")) {
    /* Open temporary file */

    if (!open_dump_file())
      goto omit_dump;

    /* Copy groups to temporary file */

    for (i = 0; i < num_groups_to_dump; i++) {
      grp = groups_to_dump[i];
      if (!dump_group(grp))
        goto omit_dump;
    }
    emit("Data copied to temporary file\n");

    /* Clear original groups */

    for (i = 0; i < num_groups_to_dump; i++) {
      grp = groups_to_dump[i];
      if (!clear_group(grp))
        goto omit_dump;
    }
    emit("Original groups cleared\n");

    /* Write dumped data back to file */

    if (!write_dumped_data())
      goto omit_dump;
    emit("Data rewritten\n");

    /* Delete temporary file */

    close(dump_fu);
    remove(dump_file_name);
    emit("Temporary file deleted\n");
  }

omit_dump:

  status = TRUE;

exit_check:
  if (map != NULL)
    free(map);
  if (ak_map != NULL)
    free(ak_map);
  if (buff != NULL)
    free(buff);
  if (ebuff != NULL)
    free(ebuff);

  return status;
}

/* ======================================================================
   rebuild_group()  -  Dump and rebuild a specific group                  */

bool rebuild_group(int32_t grp) {
  if ((grp < 1) || (grp > header.params.modulus)) {
    emit("Invalid group number\n");
    return FALSE;
  }

  /* Open temporary file */

  if (!open_dump_file())
    return FALSE;

  /* Copy group to temporary file */

  if (!dump_group(grp))
    return FALSE;
  emit("Data copied to temporary file\n");

  /* Clear original group */

  if (!clear_group(grp))
    return FALSE;
  emit("Original group cleared\n");

  /* Write dumped data back to file */

  if (!write_dumped_data())
    return FALSE;
  emit("Data rewritten\n");

  /* Delete temporary file */

  close(dump_fu);
  remove(dump_file_name);
  emit("Temporary file deleted\n");

  return TRUE;
}

/* ======================================================================
   open_dump_file()  -  Open temporary file for dump and rebuild          */

bool open_dump_file() {
  emit("Creating temporary file\n");

  sprintf(dump_file_name, "~QMFix.%d", (int32_t)getuid()); /* changed to %d from %lu */

  dump_fu = open(dump_file_name, (int)(O_RDWR | O_CREAT | O_TRUNC | O_BINARY), default_access);
  if (dump_fu < 0) {
    emit("Unable to open temporary file for group dump and rebuild\n");
    return FALSE;
  }

  return TRUE;
}

/* ======================================================================
   write_dumped_data()  -  Rewrite dumped data to correct locations       */

bool write_dumped_data() {
  bool status = FALSE;
  DH_BLOCK *buff;
  int16_t rec_offset;
  DH_RECORD *rec_ptr;

  buff = malloc(group_bytes);
  if (Seek(dump_fu, 0, SEEK_SET) < 0) {
    emit("Seek error in temporary file\n");
    goto exit_write_dumped_data;
  }

  while (read(dump_fu, buff, group_bytes) == group_bytes) {
    /* Scan group */

    rec_offset = offsetof(DH_BLOCK, record);
    while (rec_offset < buff->used_bytes) {
      rec_ptr = (DH_RECORD *)(((char *)buff) + rec_offset);
      if (!write_record(rec_ptr))
        goto exit_write_dumped_data;
      rec_offset += rec_ptr->next;
    }
  }

  status = TRUE;

exit_write_dumped_data:
  free(buff);
  return status;
}

/* ======================================================================
   dump_group()  -  Dump content of group for rebuild                     */

bool dump_group(int32_t grp) {
  bool status = FALSE;
  int16_t sf;   /* Subfile index */
  int64 offset; /* Buffer offset into subfile */
  DH_BLOCK *buff;

  buff = (DH_BLOCK *)malloc(DH_MAX_GROUP_SIZE_BYTES);

  /* Walk through the group, dumping each group buffer into the work file
    but ignoring any large record chains as these will sort themselves
    out later.                                                            */

  sf = PRIMARY_SUBFILE;
  offset = (((int64)(grp - 1)) * group_bytes) + header_bytes;
  do {
    if (!read_block(sf, offset, group_bytes, (char *)buff)) {
      emit("Error reading group %d for dump and rebuild\n", grp);
      goto exit_dump_group;
    }

    if (write(dump_fu, (char *)buff, group_bytes) != group_bytes) {
      emit("Error writing group %d to temporary file for dump and rebuild\n", grp);
      goto exit_dump_group;
    }

    sf = OVERFLOW_SUBFILE;
    offset = GetLink(buff->next);
  } while (offset != 0);

  status = TRUE;

exit_dump_group:
  free(buff);
  return status;
}

/* ======================================================================
   clear_group()  -  Clear group during rebuild                           */

bool clear_group(int32_t grp) {
  bool status = FALSE;
  int64 offset; /* Buffer offset into subfile */
  int64 new_offset;
  DH_BLOCK *buff;

  buff = (DH_BLOCK *)malloc(DH_MAX_GROUP_SIZE_BYTES);

  /* Read primary group */

  offset = (((int64)(grp - 1)) * group_bytes) + header_bytes;
  if (!read_block(PRIMARY_SUBFILE, offset, group_bytes, (char *)buff)) {
    emit("Error reading group %d for dump and rebuild\n", grp);
    goto exit_clear_group;
  }

  new_offset = GetLink(buff->next);

  /* Wipe out old content */

  memset(buff, 0, group_bytes);
  buff->next = 0;
  buff->used_bytes = BLOCK_HEADER_SIZE;
  buff->block_type = DHT_DATA;
  write_block(PRIMARY_SUBFILE, offset, group_bytes, (char *)buff);

  /* Give away any overflow blocks (but not large record space) */

  if (!release_chain(new_offset))
    goto exit_clear_group;

  status = TRUE;

exit_clear_group:
  free(buff);
  return status;
}

/* ======================================================================
   release_chain()  -  Free a chain of overflow blocks                    */

bool release_chain(int64 offset) {
  bool status = FALSE;
  DH_BLOCK *buff;
  int64 new_offset;

  buff = malloc(group_bytes);

  while (offset) {
    if (!read_block(OVERFLOW_SUBFILE, offset, group_bytes, (char *)buff)) {
      emit("Error reading overflow block at %s\n", I64(offset));
      goto exit_release_chain;
    }

    new_offset = GetLink(buff->next);

    /* Add to free chain */

    memset((char *)buff, 0, group_bytes);
    buff->next = GetLink(header.params.free_chain);
    write_block(OVERFLOW_SUBFILE, offset, group_bytes, (char *)buff);
    header.params.free_chain = SetLink(offset);
    write_header();
    offset = new_offset;
  }

  status = TRUE;

exit_release_chain:
  free(buff);

  return status;
}

/* ======================================================================
   get_overflow()  -  Allocate a new overflow block                       */

int32_t get_overflow() {
  DH_BLOCK *buff;
  int64 offset;

  buff = malloc(group_bytes);

  if ((offset = GetLink(header.params.free_chain)) != 0) {
    if (!read_block(OVERFLOW_SUBFILE, offset, BLOCK_HEADER_SIZE, (char *)buff)) {
      offset = 0;
      goto exit_get_overflow;
    }

    header.params.free_chain = SetLink(buff->next);
    if (!write_header()) {
      offset = 0;
      goto exit_get_overflow;
    }
  } else {
    /* Must make new overflow block */

    if ((offset = Seek(fu[OVERFLOW_SUBFILE], 0, SEEK_END)) == -1) {
      offset = 0;
      goto exit_get_overflow;
    }

    if (header.file_version < 2) {
      /* Check not about to go over 2Gb */

      if ((((u_int32_t)offset) + group_bytes) > 0x80000000L) {
        emit("Recovery would take file over 2Gb\n");
        offset = 0;
        goto exit_get_overflow;
      }
    }

    memset(buff, '\0', group_bytes);

    if (write(fu[OVERFLOW_SUBFILE], (char *)buff, group_bytes) != group_bytes) {
      emit("Write error extending overflow space\n");
      offset = 0;
      goto exit_get_overflow;
    }
  }

exit_get_overflow:
  free(buff);
  return offset;
}

/* ======================================================================
   show_group()  -  Display a group                                       */

void show_group(int32_t group) {
  int16_t sf = 0;
  int64 offset;
  int16_t used_bytes;
  int16_t rec_offset;
  int id_len;
  int64 big_offset;
  char *buffer = NULL;
  DH_BLOCK *buff;
  DH_RECORD *rec_ptr;
  DH_BIG_BLOCK big_block;
  bool first_big;
  char id[MAX_ID_LEN + 1];
  int32_t hash_value;
  int32_t hgroup;
  char block_type[20 + 1];

  /*
Addr          Next Fl LR Chain Data len Id
000000000400: 0030 00 ........ 00000016 nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn
000000000430: 0035 01 00002000 ........ nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn
000000002000> .... .. 00002400 00000884
000000002400> .... .. 00000000 ........
000000000465: 1234 00 ........ 00000018 nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn
*/

  buffer = malloc(DH_MAX_GROUP_SIZE_BYTES);
  buff = (DH_BLOCK *)buffer;

  offset = (((int64)(group - 1)) * group_bytes) + header_bytes;

  do {
    if (quit)
      goto exit_show_group;

    if (sf == OVERFLOW_SUBFILE) {
      emit("Overflow block x%s", I64(offset));
    } else {
      emit("Group %d (x%s)", group, I64(offset));
    }

    if (!read_block(sf, offset, group_bytes, buffer)) {
      emit("Read error at %d.%s\n", (int)sf, I64(offset));
      break;
    }

    used_bytes = buff->used_bytes;

    if (used_bytes == 0)
      strcpy(block_type, "free");
    else if (buff->block_type == DHT_DATA)
      strcpy(block_type, "data");
    else if (buff->block_type == DHT_BIG_REC)
      strcpy(block_type, "big");
    else
      sprintf(block_type, "unknown (%d)", buff->block_type);

    emit("   Type: %s, Used space %04X, Spare space %04X\n", block_type, (int)used_bytes, (int)(group_bytes - used_bytes));

    rec_offset = offsetof(DH_BLOCK, record);

    emit("Addr          Next Fl LR Chain Data len Id  [Next %s]\n", I64(GetLink(buff->next)));

    while (rec_offset < used_bytes) {
      if (quit)
        goto exit_show_group;

      rec_ptr = (DH_RECORD *)(buffer + rec_offset);

      id_len = rec_ptr->id_len;
      if (rec_ptr->flags & DH_BIG_REC) {
        emit("%s: %04X %02X %08X ........ %.*s\n", I64(offset + rec_offset), (int)(rec_ptr->next), (int)(rec_ptr->flags), rec_ptr->data.big_rec, id_len, rec_ptr->id);

        big_offset = GetLink(rec_ptr->data.big_rec);
        first_big = TRUE;
        while (big_offset != 0) {
          if (quit)
            goto exit_show_group;

          emit("%s> ", I64(big_offset));
          if (!read_block(OVERFLOW_SUBFILE, big_offset, DH_BIG_BLOCK_SIZE, (char *)(&big_block))) {
            emit("Read error at %d.%s\n", OVERFLOW_SUBFILE, I64(big_offset));
            break;
          }

          if (first_big) {
            emit(".... .. %08X %08X\n", big_block.next, big_block.data_len);
            first_big = FALSE;
          } else {
            emit(".... .. %08X\n", big_block.next);
            //           emit(".... .. %s\n", I64(big_block.next));
          }

          big_offset = GetLink(big_block.next);
        }
      } else {
        emit("%s: %04X %02X ........ %08X %.*s\n", I64(offset + rec_offset), (int)(rec_ptr->next), (int)(rec_ptr->flags), rec_ptr->data.data_len, id_len, rec_ptr->id);
      }

      memcpy(id, rec_ptr->id, id_len);
      id[id_len] = '\0';
      if (header.flags & DHF_NOCASE)
        memupr(id, id_len);
      hash_value = hash(id, id_len);
      hgroup = (hash_value % header.params.mod_value) + 1;
      if (hgroup > header.params.modulus) {
        hgroup = (hash_value % (header.params.mod_value >> 1)) + 1;
      }

      if (hgroup != group) {
        emit("  ** Above record should be in group %d **\n", hgroup);
      }

      rec_offset += rec_ptr->next;
    }

    /* Move to next group buffer */

    sf = OVERFLOW_SUBFILE;
    offset = GetLink(buff->next);
  } while (offset != 0);

exit_show_group:
  emit("\n");
  if (buffer != NULL)
    free(buffer);
}

/* ======================================================================
   show_node()  -  Show AK node                                           */

Private void show_node(int32_t group) {
  char buffer[DH_AK_NODE_SIZE];
  int64 offset;
  int16_t used_bytes;
  int16_t rec_offset;
  DH_RECORD *rec_ptr;
  int id_len;
  int32_t big_offset;
  bool first_big;
  DH_BIG_NODE big_node;
  int16_t child_count;
  int64 child_offset;
  char *p;
  int16_t i;
  int key_len;

  offset = (((int64)(group - 1)) * DH_AK_NODE_SIZE) + ak_header_size;

  if (!read_block(subfile, offset, DH_AK_NODE_SIZE, buffer)) {
    emit("\nRead error at %d.%s\n", (int)subfile, I64(offset));
    return;
  }

  switch (((DH_INT_NODE *)&buffer)->node_type) {
    case AK_FREE_NODE:
      emit("Free node %d (x%s): Free space.  Next %ld (x%s).\n", group, I64(offset), GetAKNodeNum(((DH_FREE_NODE *)&buffer)->next), I64(GetAKLink(((DH_FREE_NODE *)&buffer)->next)));
      break;

    case AK_INT_NODE:
      used_bytes = ((DH_INT_NODE *)&buffer)->used_bytes;
      child_count = ((DH_INT_NODE *)&buffer)->child_count;
      emit("Internal node %d (x%s): Child count %d.\n", group, I64(offset), (int)child_count);
      emit("Used space %04X, Spare space %04X\n", (int)used_bytes, (int)(DH_AK_NODE_SIZE - used_bytes));

      /*
       * Chld   Node NodeAddr     Len Key
       * 123: 123456 000000000000 123 50xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
       */
      emit("Chld   Node NodeAddr     Len Key\n");
      for (i = 0, p = ((DH_INT_NODE *)&buffer)->keys; i < child_count; i++, p += key_len) {
        key_len = ((DH_INT_NODE *)&buffer)->key_len[i];
        child_offset = GetAKLink(((DH_INT_NODE *)&buffer)->child[i]);
        emit("%3d: %6d %s %3d %.*s\n", (int)i, OffsetToNode(child_offset), I64(child_offset), key_len, min(50, key_len), p);
      }
      break;

    case AK_TERM_NODE:
      used_bytes = ((DH_TERM_NODE *)&buffer)->used_bytes;
      emit("Terminal node %d (x%s):\n", group, I64(offset));
      emit("Left = %d (x%s), Right = %d (x%s).\n", GetAKNodeNum(((DH_TERM_NODE *)&buffer)->left), I64(GetAKLink(((DH_TERM_NODE *)&buffer)->left)), GetAKNodeNum(((DH_TERM_NODE *)&buffer)->right),
           I64(GetAKLink(((DH_TERM_NODE *)&buffer)->right)));
      emit("Used space %04X, Spare space %04X\n", (int)used_bytes, (int)(DH_AK_NODE_SIZE - used_bytes));

      /*
       * Addr          Next Fl LR Chain Data len Id
       * 000000000400: 0030 00 ........ 00000016 nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn
       * 000000000430: 0035 01 00002000 ........ nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn
       * 000000002000> .... .. 00002400 00000884 (Node 4)
       * 000000002400> .... .. 00000000 ........ (Node 7)
       * 000000000465: 1234 00 ........ 00000018 nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn
       */

      rec_offset = offsetof(DH_TERM_NODE, record);

      emit("Addr          Next Fl LR Chain Data len Id\n");
      while (rec_offset < used_bytes) {
        rec_ptr = (DH_RECORD *)(buffer + rec_offset);

        id_len = rec_ptr->id_len;
        if (rec_ptr->flags & DH_BIG_REC) {
          emit("%s: %04X %02X %08X ........ %.*s\n", I64(offset + rec_offset), (int)(rec_ptr->next), (int)(rec_ptr->flags), GetAKNodeNum(rec_ptr->data.big_rec), min(id_len, 39), rec_ptr->id);

          big_offset = GetAKLink(rec_ptr->data.big_rec);
          first_big = TRUE;
          while (big_offset != 0) {
            emit("%s> ", I64(big_offset));
            if (!read_block(subfile, big_offset, DH_AK_BIG_NODE_SIZE, (char *)(&big_node))) {
              emit("Read error at %d.%s\n", (int)subfile, I64(big_offset));
              break;
            }

            if (first_big) {
              emit(".... .. %08X %08X (Node %d)\n", big_node.next, big_node.data_len, OffsetToNode(big_offset));
              first_big = FALSE;
            } else {
              emit(".... .. %08X          (Node %d)\n", big_node.next, OffsetToNode(big_offset));
            }

            big_offset = GetAKLink(big_node.next);
          }
        } else {
          emit("%s: %04X %02X ........ %08X %.*s\n", I64(offset + rec_offset), (int)(rec_ptr->next), (int)(rec_ptr->flags), rec_ptr->data.data_len, min(id_len, 40), rec_ptr->id);
        }

        rec_offset += rec_ptr->next;
      }
      break;

    case AK_ITYPE_NODE:
      emit("Node %d (x%s): I-type buffer.  Used bytes %04X.  Next %d (x%s).\n", group, I64(offset), (int)(((DH_ITYPE_NODE *)&buffer)->used_bytes), GetAKNodeNum(((DH_ITYPE_NODE *)&buffer)->next),
           I64(GetAKLink(((DH_ITYPE_NODE *)&buffer)->next)));
      break;

    case AK_BIGREC_NODE:
      emit("Node %d (x%s): Big record.  Used bytes %04X.  Next %d (x%s).\n", group, I64(offset), (int)(((DH_BIG_NODE *)&buffer)->used_bytes), GetAKNodeNum(((DH_ITYPE_NODE *)&buffer)->next),
           I64(GetAKLink(((DH_BIG_NODE *)&buffer)->next)));
      break;

    default:
      emit("Unrecognised node type (%d)\n", (int)((DH_INT_NODE *)&buffer)->node_type);
  }

  emit("\n");
}

/* ======================================================================
   open_subfile()  -  Open a subfile                                      */

bool open_subfile(int16_t sf) {
  char path[MAX_PATHNAME_LEN + 1];  // was hardcoded to 160.
  // converted to snprintf() -gwb 23Feb20
  if ((sf >= AK_BASE_SUBFILE) && (header.akpath[0] != '\0')) {
    // 17Oct25 mab Change dyn file prefix to %
    if (snprintf(path, MAX_PATHNAME_LEN + 1, "%s%c%%%d", header.akpath, DS, (int)sf) >= (MAX_PATHNAME_LEN + 1)) {
      emit("Overflow of max file/pathname size. Truncated to:\n\"%s\"\n", path);
    }
  } else {
    // 17Oct25 mab Change dyn file prefix to %
	if (snprintf(path, MAX_PATHNAME_LEN + 1, "%s%c%%%d", filename, DS, (int)sf) >= (MAX_PATHNAME_LEN + 1)) {
      emit("Overflow of max file/pathname size. Truncated to:\n\"%s\"\n", path);
    }
  }

  fu[sf] = open(path, (int)(O_RDWR | O_BINARY), default_access);
  return (fu[sf] >= 0);
}

/* ======================================================================
   read_block()  -  Read a data block                                     */

bool read_block(int16_t sf, int64 offset, int16_t bytes, char *buff) {
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

bool write_block(int16_t sf, int64 offset, int16_t bytes, char *buff) {
  if (Seek(fu[sf], offset, SEEK_SET) < 0) {
    return FALSE;
  }

  if (write(fu[sf], buff, bytes) != bytes) {
    return FALSE;
  }

  return TRUE;
}

/* ======================================================================
   read_header()  -  Read primary subfile header                          */

bool read_header() {
  if (!read_block(PRIMARY_SUBFILE, 0, DH_HEADER_SIZE, (char *)&header)) {
    return FALSE;
  }

  load_bytes = HeaderLoadBytes(&header);

  return TRUE;
}

/* ======================================================================
   write_header()  -  Write primary subfile header                        */

bool write_header() {
  header.params.load_bytes = load_bytes & 0xFFFFFFFF;
  header.params.extended_load_bytes = load_bytes >> 32;

  if (!write_block(PRIMARY_SUBFILE, 0, DH_HEADER_SIZE, (char *)&header)) {
    perror("Error writing primary subfile header");
    return FALSE;
  }

  return TRUE;
}

/* ======================================================================
   delete_subfile()  -  Delete a subfile                                  */

bool delete_subfile(int16_t sf) {
  char path[MAX_PATHNAME_LEN + 1];  // was hardcoded 160
  // converted to snprintf() -gwb 23Feb20
  if ((sf >= AK_BASE_SUBFILE) && (header.akpath[0] != '0')) {
    // 17Oct25 mab Change dyn file prefix to %
    if (snprintf(path, MAX_PATHNAME_LEN + 1, "%s%c%%%d", header.akpath, DS, (int)sf) >= (MAX_PATHNAME_LEN + 1)) {
      emit("Overflow of max file/pathname size. Truncated to:\n\"%s\"\n", path);
    }
  } else {
    // 17Oct25 mab Change dyn file prefix to %
    if (snprintf(path, MAX_PATHNAME_LEN + 1, "%s%c%%%d", filename, DS, (int)sf) >= (MAX_PATHNAME_LEN + 1)) {
      emit("Overflow of max file/pathname size. Truncated to:\n\"%s\"\n", path);
    }
  }

  if (fu[sf] >= 0) {
    close(fu[sf]);
    fu[sf] = -1;
  }

  return (remove(path) == 0);
}

/* ======================================================================
   read_qmconfig()  -  Read config file                                     */

bool read_qmconfig() {
  char path[200 + 1];
  char rec[200 + 1];
  FILE *ini_file;
  char section[32 + 1];
  char *p;
  int n;

  if (!GetConfigPath(path))
    return FALSE;

  if ((ini_file = fopen(path, "rt")) == NULL) {
    fprintf(stderr, "%s not found\n", path);
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
      strupr(section);
      continue;
    }

    if (strcmp(section, "QM") == 0) /* [qm] items */
    {
      if (sscanf(rec, "DEBUG=%d", &n) == 1)
        cfg_debug = n;
    }
  }

  fclose(ini_file);
  return TRUE;
}

/* ======================================================================
   Map a big record                                                       */

bool map_big_rec(int16_t rec_hdr_sf, int64 rec_hdr_offset, DH_RECORD *rec_ptr, int32_t *map) {
  int64 offset;
  int64 next_offset;
  DH_BIG_BLOCK big_block;

  offset = GetLink(rec_ptr->data.big_rec);
  tag_block(offset, rec_hdr_sf, rec_hdr_offset, CHK_BIG_REC); /* 0386 */

  while (offset != 0) {
    if (quit)
      return TRUE;

    if (!read_block(OVERFLOW_SUBFILE, offset, DH_BIG_BLOCK_SIZE, (char *)(&big_block))) {
      emit("Read error at %d.%s\n", OVERFLOW_SUBFILE, I64(offset));
      return FALSE;
    }

    if (big_block.block_type != DHT_BIG_REC) {
      emit("Big record overflow block x%s has incorrect block type (%d)\n", I64(offset), (int)(big_block.block_type));

      if (fix()) {
        big_block.block_type = DHT_BIG_REC;
        write_block(OVERFLOW_SUBFILE, offset, DH_BIG_BLOCK_SIZE, (char *)(&big_block));
        emit("   Corrected by setting block type\n");
      }
    }

    next_offset = GetLink(big_block.next);
    /* 0386 Map correct addresses */
    if (next_offset)
      tag_block(next_offset, OVERFLOW_SUBFILE, offset, CHK_BIG_REC);
    offset = next_offset;
  }

  return TRUE;
}

/* ======================================================================
   tag_block()  -  Construct space map                                    */

void tag_block(int64 offset, int16_t ref_subfile, int64 ref_offset, int16_t usage) {
  int32_t ogrp;
  int32_t ref_grp;
  int32_t n;
  int64 other_offset;
  int32_t other_group;
  static char *uses[] = {"free block", "overflow block", "big record block", "ECB"};
  static char *sf[] = {"primary", "overflow"};

  /* Map cell format:
   uusooooooooooooooooooooooooooooo
   u = use (2 bits)
   s = subfile (1 bit)        Subfile from which referenced...
   o = group (29 bits)        ...and group number of reference
*/

  ogrp = ((offset - header_bytes) / group_bytes) + 1;
  ref_grp = ((ref_offset - header_bytes) / group_bytes) + 1;

  if ((ogrp - map_offset > num_overflow_blocks) || (ogrp < 0))
    return;

  /* 0386 ref_grp range check is different for each subfile */

  if (ref_subfile) /* Overflow */
  {
    if (ref_grp - map_offset < 0)
      return;
    if (ref_grp - map_offset > num_overflow_blocks)
      return;
  } else /* Primary */
  {
    if (ref_grp < 0)
      return;
    if (ref_grp > header.params.modulus)
      return;
  }

  n = map[ogrp - map_offset];
  if (n == 0) {
    map[ogrp - map_offset] = (((int32_t)usage) << 30) | ((int32_t)ref_subfile << 29) | ref_grp;
  } else {
    other_group = n & 0x1FFFFFFF;
    other_offset = ((int64)n) * group_bytes + header_bytes;
    emit("%s %d (x%s) also referenced as %s from %s group %d (x%s)\n", uses[usage], /* Block type from this call */
         ogrp,                                                                        /* Group number */
         I64(offset),                                                                 /* Group offset */
         uses[n >> 30],                                                               /* Block type already recorded */
         sf[(n >> 29) & 0x1],                                                         /* Subfile of other reference */
         other_group,                                                                 /* Group number of other reference */
         I64(other_offset));                                                          /* Group offset of other reference */
  }
}

/* ======================================================================
   map_node()  -  Map an AK node                                          */

void map_node(int16_t sf, int32_t node, char *ak_map, int32_t num_nodes, int32_t parent) {
  char *buff = NULL;
  int64 offset;
  u_char node_type;
  int16_t i;
  int32_t child;
  int16_t used_bytes;
  int16_t rec_offset;
  DH_BIG_NODE big_node;
  DH_RECORD *rec_ptr;
  int64 big_offset;
  int32_t big_node_no;

  if (node > num_nodes) {
    emit("Non-existant node %d referenced from node %d\n", node, parent);
    goto exit_map_node;
  }

  buff = malloc(DH_AK_NODE_SIZE);

  offset = ((int64)(node - 1)) * DH_AK_NODE_SIZE + ak_header_size;

  if (!read_block(sf, offset, DH_AK_NODE_SIZE, buff)) {
    ak_map[node] = -3;
    emit("Read error on node %d referenced from node %d\n", node, parent);
    goto exit_map_node;
  }

  node_type = ((DH_INT_NODE *)buff)->node_type;

  if (ak_map[node] != (char)-1) {
    emit("Type %d node %d also referenced from node %d\n", (int)node_type, node, parent);
    goto exit_map_node;
  }

  ak_map[node] = node_type;

  switch (node_type) {
    case AK_FREE_NODE:
      emit("Free node %d referenced from node %d\n", node, parent);
      break;

    case AK_INT_NODE:
      /* Process all child nodes */

      for (i = 0; i < ((DH_INT_NODE *)buff)->child_count; i++) {
        child = GetAKNodeNum(((DH_INT_NODE *)buff)->child[i]);
        map_node(sf, child, ak_map, num_nodes, node);
        if (quit)
          return;
      }
      break;

    case AK_TERM_NODE:
      used_bytes = ((DH_TERM_NODE *)buff)->used_bytes;
      rec_offset = TERM_NODE_HEADER_SIZE;
      while (rec_offset < used_bytes) {
        if (quit)
          return;

        rec_ptr = (DH_RECORD *)(buff + rec_offset);
        if (rec_ptr->flags & DH_BIG_REC) {
          big_offset = GetAKLink(rec_ptr->data.big_rec);
          while (big_offset != 0) {
            if (quit)
              break;

            big_node_no = OffsetToNode(big_offset);

            if (ak_map[big_node_no] != (char)-1) {
              emit(
                  "Type %d node %d also referenced as big record from node "
                  "%d\n",
                  (int)node_type, node, parent);
              goto exit_map_node;
            }

            if (!read_block(sf, big_offset, DH_BIG_BLOCK_SIZE, (char *)(&big_node))) {
              emit("Read error\n");
              break;
            }

            if (big_node.node_type != AK_BIGREC_NODE) {
              emit("Big rec node %d referenced from node %d has type %d\n", big_node_no, parent, big_node.node_type);
              goto exit_map_node;
            }

            ak_map[big_node_no] = AK_BIGREC_NODE;

            big_offset = GetAKLink(big_node.next);
          }
        }
        rec_offset += rec_ptr->next;
      }
      break;

    case AK_ITYPE_NODE:
      emit("I-type node %d referenced from node %d\n", node, parent);
      break;

    case AK_BIGREC_NODE:
      emit("Big record node %d referenced from node %d\n", node, parent);
      break;

    default:
      ak_map[node] = -4;
      emit("Unexpected node type %d in node %d referenced from node %d\n", (int)(((DH_INT_NODE *)buff)->node_type), node, parent);
      break;
  }

exit_map_node:
  if (buff != NULL)
    free(buff);
}

/* ======================================================================
   map_ak_free_chain()  -  Add AK free chain to space map                 */

void map_ak_free_chain(int16_t sf, char *map, int32_t num_nodes) {
  char buff[DH_AK_NODE_SIZE];
  DH_AK_HEADER ak_header;
  int64 offset;
  int32_t node;
  int32_t last_node;

  read_block(sf, 0, DH_AK_HEADER_SIZE, (char *)&ak_header);

  emit("Checking AK free chain\n");
  last_node = 0;
  for (offset = GetAKLink(ak_header.free_chain); offset != 0; offset = GetAKLink(((DH_FREE_NODE *)buff)->next)) {
    if (quit)
      return;

    node = OffsetToNode(offset);
    if ((node < 1) || (node > num_nodes)) {
      emit("Invalid forward pointer in free node %d\n", last_node);
      break;
    }

    if (!read_block(sf, offset, DH_AK_NODE_SIZE, buff)) {
      map[node] = -3;
      emit("Read error on node %d referenced from node %d\n", node, last_node);
      goto exit_map_ak_free_chain;
    }

    map[node] = AK_FREE_NODE;
  }

  /* Follow the AK i-type chain if there is one */

  emit("Checking I-type chain\n");
  last_node = 0;
  for (offset = GetAKLink(ak_header.itype_ptr); offset != 0; offset = GetAKLink(((DH_ITYPE_NODE *)buff)->next)) {
    if (quit)
      return;

    node = OffsetToNode(offset);
    if ((node < 1) || (node > num_nodes)) {
      emit("Invalid forward pointer in itype node %d\n", last_node);
      break;
    }

    if (!read_block(sf, offset, DH_AK_NODE_SIZE, buff)) {
      map[node] = -3;
      emit("Read error on node %d referenced from node %d\n", node, last_node);
      goto exit_map_ak_free_chain;
    }

    map[node] = AK_ITYPE_NODE;
  }

exit_map_ak_free_chain:
  return;
}

/* ======================================================================
   recover_space()  -  Recover space from primary and overflow subfiles   */

bool recover_space() {
  bool status = FALSE;
  int16_t sf;
  char oldpath[MAX_PATHNAME_LEN + 1];
  char newpath[MAX_PATHNAME_LEN + 1];
  int ofu;                   /* New overflow subfile */
  DH_BLOCK *buff = NULL;     /* Main buffer for group scan */
  DH_BLOCK *big_buff = NULL; /* Big record buffer */
  int64 overflow_offset;
  int64 offset;
  int64 big_offset;
  int64 new_big_offset;
  int64 next_big_offset;
  int64 next_offset;
  int64 rewrite_offset;
  int16_t used_bytes;
  int16_t rec_offset;
  DH_RECORD *rec_ptr;
  bool rewrite;
  int32_t grp;
  int64 n;
  bool damaged = FALSE;

  buff = (DH_BLOCK *)malloc(group_bytes);
  big_buff = (DH_BLOCK *)malloc(group_bytes);

  emit("Recovering space\n");

  /* ---------- Primary subfile  -  Contract to modulus value */

  n = (((int64)header.params.modulus) * group_bytes) + header_bytes; /* 0524 */
  if (n < filelength64(fu[PRIMARY_SUBFILE])) {
    if (chsize64(fu[PRIMARY_SUBFILE], n)) {
      emit("Error %d releasing primary subfile space\n", errno);
      goto exit_recover_space;
    }

    if (verbose) {
      emit("Primary subfile contracted to %ld bytes\n", n);
    }
  }

  /* ---------- Overflow subfile  -  Create a new overflow subfile */
  // converted to snprintf() -gwb 23Feb20
  // 17Oct25 mab Change dyn file prefix to %
  if (snprintf(oldpath, MAX_PATHNAME_LEN + 1, "%s%c%%1", filename, DS) >= (MAX_PATHNAME_LEN + 1)) {
    emit("Overflow of max file/pathname size. Truncated to:\n\"%s\"\n", oldpath);
  }
  // 17Oct25 mab Change dyn file prefix to %
  if (snprintf(newpath, MAX_PATHNAME_LEN + 1, "%s%c%%%%1", filename, DS) >= (MAX_PATHNAME_LEN + 1)) {
    emit("Overflow of max file/pathname size. Truncated to:\n\"%s\"\n", newpath);
  }
  ofu = open(newpath, O_RDWR | O_BINARY | O_CREAT, default_access);
  if (ofu < 0) {
    emit("Error %d creating new overflow subfile\n", errno);
    goto exit_recover_space;
  }

  /* Clear header free chain */

  header.params.free_chain = 0;
  if (!write_header())
    goto exit_recover_space;

  /* Copy overflow header */

  if ((!read_block(OVERFLOW_SUBFILE, 0, header_bytes, (char *)buff)) || (write(ofu, buff, header_bytes) != header_bytes)) {
    goto exit_recover_space;
  }
  overflow_offset = header_bytes;

  /* Scan primary subfile, transfering overflow to the new overflow subfile
    in the order in which we find it.                                      */

  for (grp = 1; grp <= header.params.modulus; grp++) {
    progress_bar(grp);

    /* Scan group */

    rewrite = FALSE;
    sf = PRIMARY_SUBFILE;
    offset = (((int64)(grp - 1)) * group_bytes) + header_bytes;
    do {
      if (!read_block(sf, offset, group_bytes, (char *)buff)) {
        goto exit_recover_space;
      }

      if (verbose)
        emit("Processing %d.%s (group %d)\n", sf, I64(offset), grp);

      rewrite_offset = 0;
      used_bytes = buff->used_bytes;

      for (rec_offset = offsetof(DH_BLOCK, record); rec_offset < used_bytes; rec_offset += rec_ptr->next) {
        rec_ptr = (DH_RECORD *)(((char *)buff) + rec_offset);

        if (rec_ptr->flags & DH_BIG_REC) {
          rewrite = TRUE;                              /* Must rewrite this block */
          big_offset = GetLink(rec_ptr->data.big_rec); /* Old link */

          if (sf == OVERFLOW_SUBFILE) /* 0529 */
          {
            /* This group buffer came from the overflow subfile.
                If this is the first big record linked to this group
                buffer, we need to leave a gap for the group buffer.  */

            if (rewrite_offset == 0) {
              rewrite_offset = overflow_offset;
              memset(big_buff, 0, group_bytes);

              if (verbose) {
                emit("Block 1.%s moved to 1.%s. Rewrite offset %s\n", I64(offset), I64(overflow_offset), I64(rewrite_offset));
              }

              if (write(ofu, big_buff, group_bytes) != group_bytes) {
                goto exit_recover_space;
              }
              overflow_offset += group_bytes;
            }
          }

          new_big_offset = overflow_offset;
          rec_ptr->data.big_rec = SetLink(new_big_offset); /* New link */

          /* Transfer big record to new overflow subfile */

          if (verbose)
            emit("Processing big record at 1.%s\n", I64(big_offset));

          do {
            if (!read_block(OVERFLOW_SUBFILE, big_offset, group_bytes, (char *)big_buff)) {
              goto exit_recover_space;
            }

            next_big_offset = GetLink(big_buff->next);
            if (next_big_offset != 0) {
              big_buff->next = SetLink(overflow_offset + group_bytes); /* 0526 */
            }

            if (verbose) {
              emit("   Block 1.%s moved to 1.%s\n", I64(big_offset), I64(overflow_offset));
            }

            if (write(ofu, big_buff, group_bytes) != group_bytes) {
              goto exit_recover_space;
            }

            overflow_offset += group_bytes;
          } while ((big_offset = next_big_offset) != 0);
        }
      }

      /* If there is a further block in this group, update the link */

      if ((next_offset = GetLink(buff->next)) != 0) {
        rewrite = TRUE;
        if (sf == PRIMARY_SUBFILE) {
          buff->next = SetLink(overflow_offset);
        } else {
          buff->next = SetLink(overflow_offset + group_bytes);
        }
      }

      if (rewrite) /* Something has been changed */
      {
        if (sf == PRIMARY_SUBFILE) /* Write back to original location */
        {
          damaged = TRUE; /* Old file is no longer usable */

          if (verbose) {
            emit("Block 0.%s updated. Next = %s\n", I64(offset), I64(GetLink(buff->next)));
          }

          if (!write_block(sf, offset, group_bytes, (char *)buff)) {
            goto exit_recover_space;
          }
        } else /* Copy to new overflow subfile */
        {
          if (rewrite_offset != 0) {
            if (Seek(ofu, rewrite_offset, SEEK_SET) < 0) {
              goto exit_recover_space;
            }
          }

          if (verbose) {
            if (rewrite_offset != 0) {
              emit("Block 1.%s rewritten at 1.%s\n", I64(offset), I64(rewrite_offset));
            } else {
              emit("Block 1.%s rewritten at 1.%s\n", I64(offset), I64(overflow_offset));
            }
          }

          if (write(ofu, buff, group_bytes) != group_bytes) {
            goto exit_recover_space;
          }

          if (rewrite_offset != 0) {
            if (Seek(ofu, overflow_offset, SEEK_SET) < 0) {
              goto exit_recover_space;
            }
          } else {
            overflow_offset += group_bytes;
          }
        }
      }

      sf = OVERFLOW_SUBFILE;
      rewrite = TRUE; /* Overflow blocks must all be written */
    } while ((offset = next_offset) != 0);
  }

  progress_bar(-1);

  /* Now replace original overflow subfile with the new one */

  close(fu[OVERFLOW_SUBFILE]);
  close(ofu);

  if (remove(oldpath)) {
    emit("Error %d removing old overflow subfile\n", errno);
    goto exit_recover_space;
  }

  if (rename(newpath, oldpath)) {
    emit("Error %d renaming new overflow subfile\n", errno);
    goto exit_recover_space;
  }

  if (!open_subfile(OVERFLOW_SUBFILE)) {
    emit("Error %d opening new overflow subfile\n", errno);
    goto exit_recover_space;
  }

  emit("Overflow subfile contracted to %ld bytes\n", overflow_offset);

  /* ---------- AK subfiles  -  Too difficult !!!! */
  /* ....why? -gwb */

  status = TRUE;

exit_recover_space:
  if (!status && damaged) {
    emit("The original file is no longer usable\n");
  }

  if (buff != NULL)
    free(buff);
  if (big_buff != NULL)
    free(big_buff);

  return status;
}

/* ======================================================================
   write_record()  -  Write a record to the file during group reload      */

bool write_record(DH_RECORD *rec) {
  bool status = FALSE;
  char id[MAX_ID_LEN + 1];
  int16_t id_len; /* Record id length */
  int32_t hash_value;
  int64 offset;                 /* Offset of group buffer in file */
  int32_t group;                /* Group number */
  DH_BLOCK *buff = NULL;        /* Active buffer */
  int16_t subfile;              /* Current subfile */
  int rec_offset;               /* Offset of record in group buffer */
  DH_RECORD *rec_ptr;           /* Record pointer */
  int16_t used_bytes;           /* Number of bytes used in this group buffer */
  int32_t old_big_rec_head = 0; /* Head of old big record chain */
  int64 overflow_offset;
  DH_BLOCK *obuff = NULL;
#ifdef REPLACE_DUPLICATE
  // I moved the rec_size declaration here because it's only used within the
  // REPLACE_DUPLICATE code block below.  Otherwise it throws a variable set but
  // never used warning. -gwb 24Feb20
  int16_t rec_size; /* Size of current record */
  int16_t space;
  int16_t orec_offset;
  DH_RECORD *orec_ptr;
  int16_t orec_bytes;
  char *p;
  char *q;
  int n;
#endif

  buff = (DH_BLOCK *)malloc(group_bytes);
  if (buff == NULL) {
    emit("Unable to allocate memory for write buffer\n");
    goto exit_write_record;
  }

  /* Work out where this record should go */

  id_len = rec->id_len;
  memcpy(id, rec->id, id_len);
  id[id_len] = '\0';
  if (header.flags & DHF_NOCASE)
    memupr(id, id_len);

  hash_value = hash(id, id_len);
  group = (hash_value % header.params.mod_value) + 1;
  if (group > header.params.modulus) {
    group = (hash_value % (header.params.mod_value >> 1)) + 1;
  }

  /* Now search for old record of same id */

  offset = (((int64)(group - 1)) * group_bytes) + header_bytes;
  subfile = PRIMARY_SUBFILE;

  do {
    /* Read group */

    if (!read_block(subfile, offset, group_bytes, (char *)buff)) {
      emit("Read error at %d.%s\n", subfile, I64(offset));
      goto exit_write_record;
    }

    /* If this has occured because of a system failure prior to flushing
        a newly created group, the group may exist but be marked as unused.
        In this case, set it as an empty group.                             */

    used_bytes = buff->used_bytes;
    if (used_bytes == 0) {
      buff->next = 0;
      buff->used_bytes = BLOCK_HEADER_SIZE;
      buff->block_type = DHT_DATA;
    }

    /* Scan group buffer for record */

    if (used_bytes > group_bytes) {
      emit("Invalid byte count at %d.%s\n", subfile, I64(offset));
      goto exit_write_record;
    }

    rec_offset = offsetof(DH_BLOCK, record);

    while (rec_offset < used_bytes) {
      rec_ptr = (DH_RECORD *)(((char *)buff) + rec_offset);
#ifdef REPLACE_DUPLICATE
      rec_size = rec_ptr->next;
#endif
      if ((id_len == rec_ptr->id_len) && (memcmp(id, rec_ptr->id, id_len) == 0)) /* Found this record */
      {
#ifdef REPLACE_DUPLICATE
        if (rec_ptr->flags & DH_BIG_REC) /* Old record is big */
        {
          old_big_rec_head = rec_ptr->data.big_rec;
        }

        /* If the new and old records require the same space, simply
            replace the original record                                */

        if (rec->next == rec_size) {
          memcpy(rec_ptr, rec, rec_size);
          goto record_replaced;
        }

        /* Delete old record */

        p = (char *)rec_ptr;
        q = p + rec_size;
        n = used_bytes - (rec_size + rec_offset);
        if (n > 0)
          memmove(p, q, n);

        used_bytes -= rec_size;
        buff->used_bytes = used_bytes;

        /* Clear the new slack space */

        memset(((char *)buff) + used_bytes, '\0', rec_size);

        /* Perform buffer compaction if there is a further overflow block */

        if ((overflow_offset = GetLink(buff->next)) != 0) {
          if ((obuff = (DH_BLOCK *)malloc(group_bytes)) != NULL)
            ;
          {
            do {
              if (!read_block(OVERFLOW_SUBFILE, overflow_offset, group_bytes, (char *)obuff)) {
                goto exit_write_record;
              }

              /* Move as much as will fit of this block */

              space = group_bytes - buff->used_bytes;
              orec_offset = BLOCK_HEADER_SIZE;
              while (orec_offset < obuff->used_bytes) {
                orec_ptr = (DH_RECORD *)(((char *)obuff) + orec_offset);
                if ((orec_bytes = orec_ptr->next) > space)
                  break;

                /* Move this record */

                memcpy(((char *)buff) + buff->used_bytes, (char *)orec_ptr, orec_bytes);
                buff->used_bytes += orec_bytes;
                space -= orec_bytes;
                orec_offset += orec_bytes;
              }

              /* Remove moved records from source buffer */

              if (orec_offset != BLOCK_HEADER_SIZE) {
                p = ((char *)obuff) + BLOCK_HEADER_SIZE;
                q = ((char *)obuff) + orec_offset;
                n = obuff->used_bytes - orec_offset;
                memmove(p, q, n);

                /* Clear newly released space */

                p += n;
                n = orec_offset - BLOCK_HEADER_SIZE;
                memset(p, '\0', n);
                obuff->used_bytes -= n;
              }

              /* If the source block is now empty, dechain it and move
                    it to the free chain.                                 */

              if (obuff->used_bytes == BLOCK_HEADER_SIZE) {
                buff->next = obuff->next;
                if (!release_chain(overflow_offset))
                  goto exit_write_record;
              } else {
                /* Write the target block and make this source block current */

                if (!write_block(subfile, offset, group_bytes, (char *)buff)) {
                  goto exit_write_record;
                }

                memcpy((char *)buff, (char *)obuff, group_bytes);

                subfile = OVERFLOW_SUBFILE;
                offset = overflow_offset;
              }
            } while ((overflow_offset = GetLink(obuff->next)) != 0);
          }
        }

        goto add_record;
#else
        emit(
            "   Record '%s' not moved as a correctly hashed version also "
            "exists\n",
            id);
        load_bytes -= rec_ptr->next;
#endif
      }
      rec_offset += rec_ptr->next;
    }

    /* Move to next group buffer */

    if (buff->next == 0)
      break;

    subfile = OVERFLOW_SUBFILE;
    offset = GetLink(buff->next);
  } while (1);

#ifdef REPLACE_DUPLICATE
add_record:
#endif

  /* Add new record */

  if (rec->next > (group_bytes - buff->used_bytes)) /* Need overflow */
  {
    if ((overflow_offset = get_overflow()) == 0) {
      goto exit_write_record;
    }

    buff->next = SetLink(overflow_offset);

    if (!write_block(subfile, offset, group_bytes, (char *)buff)) {
      goto exit_write_record;
    }

    memset((char *)buff, '\0', group_bytes);
    buff->used_bytes = BLOCK_HEADER_SIZE;

    subfile = OVERFLOW_SUBFILE;
    offset = overflow_offset;
  }

  rec_ptr = (DH_RECORD *)(((char *)buff) + buff->used_bytes);
  memcpy(rec_ptr, rec, rec->next);
  buff->used_bytes += rec->next;

#ifdef REPLACE_DUPLICATE
record_replaced:
#endif

  /* Write last affected block */

  if (!write_block(subfile, offset, group_bytes, (char *)buff)) {
    goto exit_write_record;
  }

  /* Give away any big rec space from old record */

  if (old_big_rec_head) {
    if (!release_chain(old_big_rec_head)) {
      goto exit_write_record;
    }
  }

  status = TRUE;

exit_write_record:
  if (buff != NULL)
    free(buff);
  if (obuff != NULL)
    free(obuff);

  return status;
}

/* ======================================================================
   emit()  -  Paginated printf()                                          */

void emit(char msg[], ...) {
  va_list arg_ptr;
  char c;
  char text[500];
  char *p;
  char *q;

  va_start(arg_ptr, msg);
  vsprintf(text, msg, arg_ptr);
  va_end(arg_ptr);

  if (bar_displayed) {
    printf("\n");
    bar_displayed = FALSE;
  }

  q = text;
  do {
    if (emitted_lines == display_lines) {
      printf("Press return to continue...");
      fflush(stdout);
      read(0, &c, 1);
      printf("\n");

      switch (toupper(c)) {
        case 'Q':
          emitted_lines = 0;
          longjmp(quit_command, 1);
          break;

        case 'S':
          emitted_lines = -1; /* Suppress pagination */
          break;

        default:
          emitted_lines = 0;
          break;
      }
    }

    p = strchr(q, '\n');
    if (p == NULL) {
      printf("%s", q);
      if (logging)
        fprintf(log, "%s", q);
    } else {
      *p = '\0';
      puts(q);
      if (logging)
        fprintf(log, "%s\n", q);
      if (emitted_lines >= 0)
        emitted_lines++;
    }

    q = p + 1;
  } while (p);
}

/* ======================================================================
   fix()  -  Fix fault?                                                   */

bool fix() {
  switch (fix_level) {
    case 1: /* Query fix */
      return yesno("Fix");

    case 2: /* Unconditional fix */
      return TRUE;

    default:
      return FALSE;
  }
}

/* ====================================================================== */

bool yesno(char *prompt) {
  char c;

  printf("%s? ", prompt);
  fflush(stdout);
  do {
    read(0, &c, 1);
    c = toupper(c);
    if (c >= 32)
      printf("%c", c);
  } while ((c != 'Y') && (c != 'N'));
  printf("\n");
  return (c == 'Y');
}

/* ======================================================================
   Console event handler                                                  */

void event_handler(int signum) {
  emit("\nQuit\n");
  quit = TRUE;
}

/* ====================================================================== */

char *I64(int64 x) {
  static char s[8][20];
  static int16_t i = 0;

  i = (i + 1) % 8;
#ifndef __LP64__
  sprintf(s[i], "%.12llX", x);
#else
  sprintf(s[i], "%.12lu", x);
#endif
  return s[i];
}

/* ======================================================================
   memupr()  -  Uppercase specified number of bytes                       */

void memupr(char *str, int16_t len) {
  register char c;

  while (len--) {
    c = UpperCase(*str);
    *(str++) = c;
  }
}

/* ======================================================================
   strupr()  -  Convert string to upper case                              */

char *strupr(char *s) {
  char *p;
  p = s;
  while ((*(p++) = UpperCase(*p)) != '\0') {
  }
  return s;
}

/* ======================================================================
   progress_bar()  -  Draw a progress bar                                 */

void progress_bar(int32_t grp) {
#define MIN_BAR_INTERVAL 100
  int16_t pct_done; /* Actually 50ths */
  static int16_t last_pct_reported = -1;
  static int32_t last_grp_reported = -MIN_BAR_INTERVAL;
  char s[100];

  if (suppress_bar)
    return;

  if (grp >= 0) /* Scanning */
  {
    if (grp > header.params.modulus)
      return; /* Scanning released groups */

    pct_done = (int16_t)((((double)grp) * 50) / header.params.modulus);
    if ((pct_done != last_pct_reported) && (grp - last_grp_reported >= MIN_BAR_INTERVAL)) {
      memset(s, '-', 50);
      memset(s, '*', pct_done);
      sprintf(s + 50, "| %d/%d", grp, header.params.modulus);
      printf("\r%s", s);
      last_pct_reported = pct_done;
      last_grp_reported = grp;
      bar_displayed = TRUE;
    }
  } else /* End of scan */
  {
    printf("\r**************************************************| %d/%d\n", header.params.modulus, header.params.modulus);

    last_pct_reported = -1;
    last_grp_reported = -MIN_BAR_INTERVAL;
    bar_displayed = FALSE;
  }
}

/* END-CODE */
