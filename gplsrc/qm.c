/* QM.C
 * Main module of ScarletDME (QM)
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
 * 14Jan22 gwb Created a routine named dump_pcode_file() that will dump
 *             the contents of /usr/qmsys/bin/pcode into /home/geneb/pcode_files.
 *             This only works when DEBUG has been defined.  If you want to
 *             generate the pcode files yourself, just change the hardcoded
 *             output directory.
 *             The output of the routine is a dump of the data stored in the 
 *             OBJECT_HEADER structure as well as the object code itself as
 *             clipped from the pcode file.  At this time I don't know if the
 *             object code that's been output is valid in that it's what the
 *             system actually tries to "run".
 * 
 * 13Jan22 gwb Refactored main() to remove "goto" calls.
 * 
 * 11Jan22 gwb Fix for Issue #15
 * 
 * 10Jan22 gwb Added BUILD_TARGET macro in order to display build target size.
 *
 * 29Feb20 gwb Fixed issues related to format specifiers in ?printf()
 *             statements.  The warnings are triggered due to the conversion
 *             platform agnostic variable types (int32_t, etc)
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 *
 * 27Feb20 gwb Updated a few K&R function declarations to ANSI style.
 * 20Jun12 gwb Rebranded to Scarlet (poorly)
 * 
 * START-HISTORY (OpenQM):
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 18 Jun 07  2.5-7 Added -TERM option.
 * 24 Jan 07  2.4-20 Added -F option.
 * 19 Dec 06  2.4-18 Allog -K to take a user login name.
 * 19 Dec 06  2.4-18 -K option should require administrative access.
 * 30 Jun 06  2.4-5 Added CN_WINSTDOUT.
 * 27 Jun 06  2.4-5 Added -cleanup.
 * 05 May 06  2.4-4 Extended serial port command line to handle port settings.
 * 19 Apr 06  2.4-2 Added port connection (Windows).
 * 08 Mar 06  2.3-8 Added -SUSPEND and -RESUME.
 * 20 Dec 05  2.3-3 Set entry_dir as current directory on entry.
 * 20 Dec 05  2.3-3 Changed exit() values so that errors return 1.
 * 19 Dec 05  2.3-3 Modified single command processing to allow use with other
 *                  options. Also, quotes are now often not needed.
 * 20 Oct 05  2.2-15 Pcode now in shared segment.
 * 15 Sep 05  2.2-10 Added -QUIET.
 * 13 Jul 05  2.2-4 Added --HELP and --VERSION.
 * 21 Apr 05  2.1-12 Extended -B to specify which directions.
 * 08 Apr 05  2.1-12 Added -Aname option to force entry to given account.
 * 31 Mar 05  2.1-11 0335 Break statement omitted.
 * 23 Mar 05  2.1-10 Set console window title for QMConsole sessions.
 * 11 Mar 05  2.1-9 Set binary mode for QMClient connections.
 * 11 Mar 05  2.1-9 Set binary mode on QMClient connections.
 * 06 Mar 05  2.1-8 Honour telnet binary mode negotiation via -B option.
 * 26 Oct 04  2.0-7 Allow qm -a on all platforms.
 * 28 Sep 04  2.0-3 Set config_path.
 * 22 Sep 04  2.0-2 Added dynamic pcode loader.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * Available single letter options: EFGHJORVWY
 *
 * QM      -A            Query account name
 *         -Aname        Force entry to named account unless set in $LOGINS
 *         -Bn           Telnet binary mode? Additive: 1=input, 2=output,
 *                                                     4 = suppress telnet
 *         -D            Diagnostic dump
 *         -K n          Kill user n
 *         -K ALL        Kill all users
 *         -L            Apply new licence
 *         -M            Dump shared memory
 *         -N            Network connection (QMClient or direct telnet)
 *         -Pn           Execute phantom command (command in $IPC)
 *         -Q            QMClient
 *         -U            List current users
 *
 * "Word" options
 *    -CLEANUP      Clean up lost processes
 *    -INTERNAL     Run in internal mode
 *    -QUIET        Suppress copyright/licence display on entry
 *    -RESUME       Resume updates
 *    -SUSPEND      Suspend updates
 *    -TERM xx      Set default terminal type
 *
 * Doubly prefixed word options
 *    --HELP        Display usage help
 *    --VERSION     Display revision stamp
 *
 *
 * Options applicable to Linux only:
 *    -Cs.r         Local client connection (s = send pipe, r = receive pipe)
 *    -N            Network connection
 *    -RESTART      Restart system
 *    -START        Start system
 *    -STOP         Stop system
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include <setjmp.h>
#include <time.h>
#include <stdarg.h>

// #define DEBUG /* enables harcoded diagnostic output */

#define Public
#define init(a) = a

#include "qm.h"
#include "revstamp.h"
#include "header.h"
#include "debug.h"
#include "dh_int.h"
#include "tio.h"
#include "config.h"
#include "options.h"
#include "locks.h"

#ifdef __LP64__ /* 10Jan22 gwb */
#define BUILD_TARGET "64 Bit"
#else
#define BUILD_TARGET "32 Bit"
#endif

extern char *x_option; /* -x option */

bool bind_sysseg(bool create, char *errmsg);
void unbind_sysseg(void);
void dump_sysseg(bool dump_config);
void show_users(void);
void kill_user(char *user);

Private jmp_buf qm_exit;

Private void qm_init(int argc, char *argv[]);
Private void check_admin(void);
Private bool comlin(int argc, char *argv[]);
Private bool load_pcode(char *pname, u_char **ptr);

void suspend_resume(bool suspend);
void cleanup(void);
void clean_stop(void);
void dump_pcode_file(void);

/* ====================================================================== */

int main(int argc, char *argv[]) {
  /* 13Jan22 gwb Refactored to remove "goto" calls. */

  int status = 1;
  char errmsg[80 + 1];

  tio.term_type[0] = '\0';

  set_default_character_maps();
  qm_init(argc, argv);

  if (!(command_options & CMD_FLASH)) {
    /* Get config file path */
    if (!GetConfigPath(config_path)) {
      clean_stop();
      return status; /* TODO: add a custom return value for this failure. */
    }

    fullpath(config_path, config_path);
  }

  /* Process the command line */
  if (!comlin(argc, argv)) {
    clean_stop();
    return status; /* TODO: add a custom return value for this failure. */
  }

  if (!bind_sysseg(FALSE, errmsg)) {
    fprintf(stderr, "%s\n", errmsg);
    clean_stop();
    return status; /* TODO: add a custom return value for this failure. */
  }

  if (sysseg->flags & SSF_SUSPEND) {
    fprintf(stderr, "ScarletDME is suspended\n");
    clean_stop();
    return status; /* TODO: add a custom return value for this failure. */
  }

  /* Disaster exit */
  if (setjmp(qm_exit)) {
    clean_stop();
    return status; /* TODO: add a custom return value for this failure. */
  }

  /* Set pcode pointers */

#undef Pcode
/* 
 * The block below is kind of interesting.  The #define functions as a function call that works like
 * this:
 * Given a call like Pcode(chain), the preprocessor is going to emit this:
 * if (!load_pcode("chain", &pcode_chain)) {
 *   clean_stop();
 *   return status;
 * }
  * 
 * Now the line below where pcode.h is included is going to trigger a call to load_pcode() for each
 * line in the pcode.h file that references the Pcode() macro.  It's clever in that you don't need to 
 * somehow specify a static list of pcode value names, you just add them to the include file and they'll
 * get pulled in automatically since the compiler preprocessor will have "unrolled" all of the entries
 * in the pcode.h include file, resulting in them being loaded at run time.
 * 
 * A similar method is used in kernel.h to declare all of the pcode variables via this define:
 * #define Pcode(a) Public u_char* pcode_##a;
 * Public has been defined in qmdefs.h as "extern".  Given a call of Pcode(chain), the pre-processor
 * is going to expand that as:
 * extern u_char* pcode_chain;
 * 
 * -gwb
 * 
 */
#define Pcode(a)                     \
  if (!load_pcode(#a, &pcode_##a)) { \
    clean_stop();                    \
    return status;                   \
  }

#include "pcode.h" /* this loads up all the pcode object code from the "pcode" file. */

  /* Go run the system */
  if (!init_kernel()) {
    clean_stop();
    return status; /* TODO: add a custom return value for this failure. */
  }

  /* Initialize English messages */
  if (!load_language("")) {
    clean_stop();
    return status; /* TODO: add a custom return value for this failure. */
  }

#ifdef DEBUG
  dump_pcode_file();
#endif

  kernel(); /* Run the command processor */

  s_free_all(); /* Only really needed for MEMTRACE */

  status = exit_status;

  // abort:
  //   dh_shutdown();
  //   unbind_sysseg();
  //   shut_console();
  clean_stop();
  return status;
}

void clean_stop(void) {
  /* these functions were originally called at the end of main().
   * I've moved them to their own function in order to remove all of the
   * instances of "goto abort" that were in main().
   * 13Jan22 gwb */

  dh_shutdown();
  unbind_sysseg();
  shut_console();
}

/* ======================================================================
   Initialialisation tasks that need to be done very early                */

Private void qm_init(int argc, char *argv[]) {
  char cwd[MAX_PATHNAME_LEN + 1];

  /* Save the current working directory for use by SYSTEM(1024) */

  (void)getcwd(cwd, MAX_PATHNAME_LEN);
  entry_dir = k_alloc(MAX_PATHNAME_LEN, strlen(cwd) + 1); /* was hard coded at 110 -gwb */
  strcpy(entry_dir, cwd);
}

/* ====================================================================== */

Private bool comlin(int argc, char *argv[]) {
  int arg;
  int socket_handle = 0;
  char c;
  int16_t bytes;
  int n; /* Fix for Issue #15 - 11Jan22 gwb */
  int RxPipe;
  int TxPipe;

  for (arg = 1; (arg < argc) && (argv[arg][0] == '-'); arg++) {
    if (IsDigit(*(argv[arg] + 1))) {
      forced_user_no = atoi(argv[arg] + 1);
    } else if (!stricmp(argv[arg], "-CLEANUP")) {
      cleanup();
      exit(0);
    } else if (!stricmp(argv[arg], "-INTERNAL")) {
      internal_mode = TRUE;
    } else if (!stricmp(argv[arg], "-QUIET")) {
      command_options |= CMD_QUIET;
    } else if (!stricmp(argv[arg], "-TERM")) {
      if (++arg < argc)
        strcpy(tio.term_type, argv[arg]);
    } else {
      switch (UpperCase(argv[arg][1])) {
        case 'A': /* Query account */
          if (argv[arg][2] == '\0') {
            command_options |= CMD_QUERY_ACCOUNT;
          } else {
            forced_account = argv[arg] + 2;
          }
          break;

        case 'B': /* Enable telnet binary mode */
          c = argv[arg][2];
          telnet_binary_mode_in = c & 1;
          telnet_binary_mode_out = c & 2;
          if (c & 4)
            telnet_negotiation = FALSE;
          break;

        case 'D': /* Diagnostic report */
          dump_sysseg(TRUE);
          exit(0);

        case 'K': /* Kill user */
          check_admin();
          if (++arg < argc) {
            if (!stricmp(argv[arg], "ALL"))
              kill_user(NULL);
            else
              kill_user(argv[arg]);
            exit(0);
          }
          fprintf(stderr, "User number, login name or ALL required\n");
          exit(1);

        case 'L': /* Apply new licence */
          command_options |= CMD_APPLY_LICENCE;
          break;

        case 'M': /* Dump memory */
          dump_sysseg(FALSE);
          exit(0);

        case 'P': /* Execute phantom command */
          phantom_user_index = atoi(argv[arg] + 2);
          is_phantom = TRUE;
          connection_type = CN_NONE;
          break;

        case 'Q': /* Start QMClient session (NT style login) */
          is_QMVbSrvr = TRUE;
          telnet_binary_mode_in = TRUE;
          telnet_binary_mode_out = TRUE;
          break;

        case 'U': /* Show users */
          show_users();
          exit(0);

        case 'C': /* QMLocal client connection */
          connection_type = CN_PIPE;
          if (sscanf(argv[arg], "-C%d!%d", &TxPipe, &RxPipe) != 2) {
            exit(1);
          }
          dup2(RxPipe, 0);
          dup2(TxPipe, 1);
          break;

        case 'N': /* Network server */
          connection_type = CN_SOCKET;
          break;

        case 'R':
          if (!stricmp(argv[arg], "-RESUME")) {
            suspend_resume(FALSE);
            exit(0);
          }

          if (stricmp(argv[arg], "-RESTART") == 0) {
            check_admin();
            if (stop_qm() && start_qm()) {
              printf("ScarletDME has been restarted\n");
              exit(0);
            }
            exit(1);
          }

          goto unrecognised;

        case 'S':
          if (!stricmp(argv[arg], "-SUSPEND")) {
            suspend_resume(TRUE);
            exit(0);
          }

          if (stricmp(argv[arg], "-START") == 0) {
            check_admin();
            if (start_qm()) {
              printf("ScarletDME (%s) has been started\n", BUILD_TARGET);
              exit(0);
            }
            exit(1);
          }

          if (stricmp(argv[arg], "-STOP") == 0) {
            check_admin();
            if (stop_qm()) {
              printf("ScarletDME (%s) has been shut down\n", BUILD_TARGET);
              exit(0);
            }
            exit(1);
          }

        case '-':
          if (!stricmp(argv[arg], "--HELP")) {
            goto help;
          } else if (!stricmp(argv[arg], "--VERSION")) {
            printf("%s (%s)\n", QM_REV_STAMP, BUILD_TARGET);
            exit(0);
          } else
            goto unrecognised;
          break;

        default:
          goto unrecognised;
      }
    }
  }

  /* Anything else on the command line is considered to be a command
    to be executed.                                                  */

  if (arg < argc) {
    bytes = 0;
    for (n = arg; n < argc; n++) {
      bytes += strlen(argv[n]) + 1;
    }

    single_command = k_alloc(109, bytes);
    n = 0;
    while (1) {
      strcpy(single_command + n, argv[arg]);
      n += strlen(argv[arg]);
      if (++arg == argc)
        break;
      single_command[n++] = ' ';
    }
  }

  /* Start connection */

  switch (connection_type) {
    case CN_SOCKET:
      if (!start_connection(socket_handle))
        exit(1);
      break;
    case CN_PIPE:
    case CN_PORT:
      if (!start_connection(0))
        exit(1);
      break;
    case CN_WINSTDOUT:
      break;
  }

  if (connection_type != CN_SOCKET)
    telnet_negotiation = FALSE;

  return TRUE;

unrecognised:
  fprintf(stderr, "Unrecognised argument '%s'\n", argv[arg]);
help:
  fprintf(stderr, "\nUsage:\n");
  fprintf(stderr, "   qm xxx\n");
  fprintf(stderr, "      Execute QM command xxx\n\n");
  fprintf(stderr, "   qm {options}\n");
  fprintf(stderr, "      -a          Prompt for account unless forced elsewhere\n");
  fprintf(stderr,
          "      -axxx       Enter ScarletDME in account xxx unless forced "
          "elsewhere\n");
  fprintf(stderr, "      -k n        Kill (logout) user n\n");
  fprintf(stderr, "      -k all      Kill (logout) all users n\n");
  fprintf(stderr, "      -l          Apply new licence\n");
  fprintf(stderr, "      -u          List current users\n");
  fprintf(stderr, "      -quiet      Suppress all displays on entry\n");
  fprintf(stderr, "      --help      Show this summary\n");
  fprintf(stderr, "      --version   Report version number\n");

  fprintf(stderr, "      -start      Start system\n");
  fprintf(stderr, "      -stop       Stop system\n");
  return FALSE;
}

/* ======================================================================
   Fatal error handler                                                    */

void fatal() {
  longjmp(qm_exit, 1);
}

/* ======================================================================
   dump()  -  General purpose memory dump function                        */

void dump(u_char *addr, int32_t bytes) {
  int32_t i;
  int16_t j;
  u_char c;

  for (i = 0; i < bytes; i += 16) {
    /* Offset */

    printf("%08X: ", i);  // was lX -Wformat=2 issue

    /* Hex */

    for (j = 0; j < 16; j++) {
      if (i + j < bytes)
        printf("%02X", addr[i + j]);
      else
        printf("  ");
      if ((j % 4) == 3)
        printf(" ");
    }

    printf(" | ");

    /* Character */

    for (j = 0; (j < 16) && (i + j < bytes); j++) {
      c = *(addr + i + j);
      printf("%c", (c < 32) ? '.' : c);
    }

    printf("\n");
  }

  if (bytes % 16 != 0)
    printf("\n");
}

/* ======================================================================
   check_admin()  -  Check user has admin rights                          */

void check_admin() {
  int16_t in_group(char *group_name);

  if ((geteuid() != 0) && !in_group("admin")) {
    fprintf(stderr, "Command requires administrator privileges\n");
    exit(1);
  }
}

void dump_pcode_file(void) {
  OBJECT_HEADER *obj;
  int i;
  u_char *pcode;
  u_char **ptr;
  bool okMagicCheck = TRUE;
  pcode = ((u_char *)sysseg) + sysseg->pcode_offset;
  printf("Dumping pcode file.\n\r");

  /* iterate through the entire pcode library */
  for (i = 0; i < sysseg->pcode_len; i += (obj->object_size + 3) & ~3) {
    obj = (OBJECT_HEADER *)(pcode + i);
    if (obj->magic == HDR_MAGIC_INVERSE) {
      convert_object_header(obj);
    } else if (obj->magic != HDR_MAGIC) {
      fprintf(stderr, "Pcode is corrupt (%s)\n", obj->ext_hdr.prog.program_name);
      okMagicCheck = FALSE;
    }
    if (okMagicCheck) {
      ptr = malloc(4096);
      *ptr = pcode + i;
      printf("Dumping object & info for %s\r\n", obj->ext_hdr.prog.program_name);
      time_t epochTime = obj->compile_time;
      struct tm ts;
      char timeBuf[90];
      ts = *localtime(&epochTime);
      strftime(timeBuf, sizeof(timeBuf), "%a %Y-%m-%d %H:%M:%S %z", &ts);
      int pcode_fd;
      FILE *info_fd;

      char path[MAX_PATHNAME_LEN];
      sprintf(path, "/home/geneb/pcode_files/%s", obj->ext_hdr.prog.program_name);
      pcode_fd = open(path, O_CREAT | O_RDWR, S_IWUSR | S_IRUSR);
      write(pcode_fd, ptr, obj->object_size);
      close(pcode_fd);
      sprintf(path, "/home/geneb/pcode_files/%s.txt", obj->ext_hdr.prog.program_name);
      info_fd = fopen(path, "w+");
      fprintf(info_fd, "Object Header for %s\n", obj->ext_hdr.prog.program_name);
      fprintf(info_fd, "---------------------------------------\n");
      fprintf(info_fd, "Magic #...............%x (%d)\n", obj->magic, obj->magic);
      fprintf(info_fd, "Revision..............%x (%d)\n", obj->rev, obj->rev);
      fprintf(info_fd, "Start Offset..........%x (%d)\n", obj->start_offset, obj->start_offset);
      fprintf(info_fd, "# of Arguments........%d\n", obj->args);
      fprintf(info_fd, "# of variables........%d\n", obj->no_vars);
      fprintf(info_fd, "Stack Depth...........%d\n", obj->stack_depth);
      fprintf(info_fd, "Symbol Table Offset...%x (%d)\n", obj->sym_tab_offset, obj->sym_tab_offset);
      fprintf(info_fd, "Line Table Offset.....%x (%d)\n", obj->line_tab_offset, obj->line_tab_offset);
      fprintf(info_fd, "Object Size (bytes)...%d\n", obj->object_size);
      fprintf(info_fd, "Compile timestamp.....%s\n", timeBuf);
      fprintf(info_fd, "Flags: \n");
      if (obj->flags & HDR_IS_CPROC)
        fprintf(info_fd, "  CPROC Command Processor.\n");
      if (obj->flags & HDR_INTERNAL)
        fprintf(info_fd, "  Compiled in Internal Mode.\n");
      if (obj->flags & HDR_DEBUG)
        fprintf(info_fd, "  Compiled in Debug Mode.\n");
      if (obj->flags & HDR_IS_DEBUGGER)
        fprintf(info_fd, "  Debugger.\n");
      if (obj->flags & HDR_NOCASE)
        fprintf(info_fd, "  Case Insensitive String Operations.\n");
      if (obj->flags & HDR_IS_FUNCTION)
        fprintf(info_fd, "  Is a Basic Function.\n");
      if (obj->flags & HDR_VAR_ARGS)
        fprintf(info_fd, "  Variable argument count (args = max).\n");
      if (obj->flags & HDR_RECURSIVE)
        fprintf(info_fd, "  Is a recursive program.\n");
      if (obj->flags & HDR_ITYPE)
        fprintf(info_fd, "  Is an A/S/C/I-Type.\n");
      if (obj->flags & HDR_ALLOW_BREAK)
        fprintf(info_fd, "  Allow Break key in recursive.\n");
      if (obj->flags & HDR_IS_TRUSTED)
        fprintf(info_fd, "  Trusted program.\n");
      if (obj->flags & HDR_NETFILES)
        fprintf(info_fd, "  Allow remote files by NFS.\n");
      if (obj->flags & HDR_CASE_SENSITIVE)
        fprintf(info_fd, "  Program uses case sensitive names.\n");
      if (obj->flags & HDR_QMCALL_ALLOWED)
        fprintf(info_fd, "  Can be called via QMCall().\n");
      if (obj->flags & HDR_CTYPE)
        fprintf(info_fd, "  Is a C-Type.\n");
      if (obj->flags & HDR_IS_CLASS)
        fprintf(info_fd, "  Is a Class module.\n");
      fflush(info_fd);
      fclose(info_fd);
      free(ptr);
    }
  }
}
/* ====================================================================== */

Private bool load_pcode(char *pname, u_char **ptr) {
  char u_pname[MAX_PROGRAM_NAME_LEN + 1];
  OBJECT_HEADER *obj;
  int i;
  u_char *pcode;

  pcode = ((u_char *)sysseg) + sysseg->pcode_offset;

  /* Take a local copy of the pcode name and force it to uppercase */

  strcpy(u_pname, pname);
  UpperCaseString(u_pname);

  /* Search for this item in the pcode library */
  for (i = 0; i < sysseg->pcode_len; i += (obj->object_size + 3) & ~3) {
    obj = (OBJECT_HEADER *)(pcode + i);
    if (obj->magic == HDR_MAGIC_INVERSE) {
      convert_object_header(obj);
    } else if (obj->magic != HDR_MAGIC) {
      fprintf(stderr, "Pcode is corrupt (%s)\n", u_pname);
      return FALSE;
    }

    if (!strcmp(obj->ext_hdr.prog.program_name, u_pname)) { /* Found it */
      *ptr = pcode + i;
      return TRUE;
    }
  }

  fprintf(stderr, "Pcode item %s not found\n", u_pname);
  return FALSE;
}

/* END-CODE */
