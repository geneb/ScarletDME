/* PREFIX.C
 * convert dynamic (hashed) files from ~ prefix to %
 *  assume usage: prefix path_to_account/wildcard* 
 * based on QM program QMFIX
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
 * 24Oct25 mab Initial release
 
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

#include <signal.h>
#include "linuxlb.h"

#define MAX_SUBFILES (AK_BASE_SUBFILE + MAX_INDICES)


static u_char cfg_debug = 0; /* DEBUG config parameter */


static FILE *log = NULL;
static bool logging = FALSE;      
static char filename[MAX_PATHNAME_LEN + 1] = ""; /* File being processed */

int16_t display_lines; /* Available lines for display */
int16_t emitted_lines; /* Lines emitted */
static jmp_buf quit_command;
static bool quit = FALSE;

bool read_qmconfig(void);
bool is_dh_file(void);
bool rename_subfile(int16_t subfile);
void process_file(void);
void fix_file(char *fn);
bool yesno(char *prompt);
void emit(char msg[], ...);
void event_handler(int signum);
char *strupr(char *s);

/* ====================================================================== 
 // assume usage: prefix path_to_account/wildcard* 
 * Where command is issued from within account root directory
 * As displayed when sorting accounts file from qmsys
 * note -
 * The linux shell will replace the * with a list of full qulified file 
 * names (at least that is what i am told.....)                                                 
 * ====================================================================== */
 int main(int argc, char *argv[]) {
  int status = 1;
  int arg;

  set_default_character_maps();

  display_lines = 24;

  emitted_lines = 0;

  emit(
      "[ PREFIX %s   The ScarletDME project. "
      "]\n\n",
      QM_REV_STAMP);

  filename[0] = '\0';

  if (!read_qmconfig())
    goto exit_qmfix;

  signal(SIGINT, event_handler);

  /* Check command arguments */

  if (argc == 1)
    goto usage; /* No filenames */

  arg = 1;
  while (arg < argc) {
    printf("File: %s\n",argv[arg]);
    fix_file(argv[arg]);
    arg++;
  }

exit_qmfix:
  return status;

usage:
  printf("Usage: PREFIX path_to_account\\*\n\n");
    return status;
}

/* ======================================================================
   fix_file()                                                             */

void fix_file(char *fn) {

  strcpy(filename, fn);

  if (is_dh_file()) {
    process_file();
  }

  return;
}

/* ======================================================================
   is_dh_file()  by looking for ~0 file                                 */

bool is_dh_file() {
  bool status = FALSE;
  char pathname[MAX_PATHNAME_LEN + 1];
  struct stat statbuf;
  // converted to snprintf() -gwb 23Feb20
  // rem looking for old style blob name (~0)
  if (snprintf(pathname, MAX_PATHNAME_LEN + 1, "%s%c~0", filename, DS) >= (MAX_PATHNAME_LEN + 1)) {
    emit("Overflow of max file/pathname size. Truncated to:\n\"%s\"\n", pathname);
  }

   /* file exist?, if not skip */
  if (stat(pathname, &statbuf) != 0){
    goto exit_is_dh_file;
  }  

  /* regular file?, if not skip */
  if (!(statbuf.st_mode & S_IFREG)){
    goto exit_is_dh_file;
  }  

  status = TRUE;

exit_is_dh_file:
  return status;
}

/* ======================================================================
   process_file()  -  Process a file                                      */

void process_file() {

  int16_t i;

  emit("Processing %s\n", filename);

  if (setjmp(quit_command))
    goto exit_process_file;

  /* look for primary and overflow subfiles, renmae here? */

  if (rename_subfile(PRIMARY_SUBFILE) != 0) {
    perror("Cannot rename primary subfile");
    goto exit_process_file;
  }

  if (rename_subfile(OVERFLOW_SUBFILE) != 0) {
    perror("Cannot rename overflow subfile");
    goto exit_process_file;
  }

  for (i = 0; i < MAX_INDICES; i++) {
    rename_subfile(AK_BASE_SUBFILE + i);
  }

exit_process_file:
  return;
}

/* ======================================================================
   rename_subfile()  -  rename a subfile from ~ to %                   */

bool rename_subfile(int16_t sf) {
  char new_path[MAX_PATHNAME_LEN + 1];  // was hardcoded to 160.
  char old_path[MAX_PATHNAME_LEN + 1];  // was hardcoded to 160.
  struct stat statbuf; 
  int status;

  if (snprintf(old_path, MAX_PATHNAME_LEN + 1, "%s%c~%d", filename, DS, (int)sf) >= (MAX_PATHNAME_LEN + 1)) {
    emit("Overflow of max file/pathname size. Truncated to:\n\"%s\"\n", old_path);
  }

  if (snprintf(new_path, MAX_PATHNAME_LEN + 1, "%s%c%%%d", filename, DS, (int)sf) >= (MAX_PATHNAME_LEN + 1)) {
    emit("Overflow of max file/pathname size. Truncated to:\n\"%s\"\n", new_path);
  }

  /* file exist?, if not skip */
  if (stat(old_path, &statbuf) != 0){
    status = 1;  // file does not exist or is not accessable
  } else {
    emit("Renaming %s to %s\n", old_path,new_path);
    status = rename(old_path,new_path);
  }
  return status;
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

/* ======================================================================
   strupr()  -  Convert string to upper case                              */

char *strupr(char *s) {
  char *p;
  p = s;
  while ((*(p++) = UpperCase(*p)) != '\0') {
  }
  return s;
}

/* END-CODE */
