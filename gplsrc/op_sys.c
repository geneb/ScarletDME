/* OP_SYS.C
 * op_system()  -  SYSTEM() function.
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
 * 13Feb23 njs Add SYSTEM(1050) is administrator
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 *
 * START-HISTORY (OpenQM):
 * 07 Aug 07  2.5-7 Added keys 27 - 30 for UID/GID.
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 18 Jun 07  2.5-7 SYSTEM(1016) can make use of separated user type counts.
 * 08 Jun 07  2.5-7 Added SYSTEM(1032) to retrieve and clear break pending flag.
 * 21 May 07  2.5-5 Added SYSTEM(1031) for operating system process id.
 * 27 Apr 07  2.5-3 Added SYSTEM(38) as temporary directory pathname and
 *                  SYSTEM(1030) as login time.
 * 18 Jan 07  2.4-19 Added SYSTEM(1029) for internal subroutine depth.
 * 31 May 06  2.4-5 Moved printer on flag into PROGRAM structure.
 * 17 May 06  2.4-4 Added SYSTEM(1027) for serial port name.
 * 21 Feb 06  2.3-6 Added SYSTEM(1026) for access to command line command.
 * 02 Feb 06  2.3-6 Added SYSTEM(1025) to retrieve all environment variables.
 * 20 Dec 05  2.3-3 Added system(1024) to get current directory on entry to QM.
 * 14 Dec 05  2.3-2 Added collation map handling.
 * 07 Dec 05  2.2-18 Added system(1020) to get time of day in milliseconds.
 * 27 May 05  2.2-0 Added device licensing items.
 * 30 Mar 05  2.1-11 Added SYSTEM(1017) for server port number.
 * 02 Jan 05  2.1-0 Added SYSTEMS(1015).
 * 15 Dec 04  2.1-0 Added SYSTEM(1013) and SYSTEM(1014) for user limits.
 * 28 Sep 04  2.0-3 Replaced GetSysPaths() with config_path.
 * 20 Sep 04  2.0-2 Use dynamic pcode loader.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 *     1   Printer on?
 *     2   Default print unit page width
 *     3   Default print unit page depth
 *     4   Default print unit lines remaining on page
 *     5   Default print unit page number
 *     6   Default print unit line number
 *     7   @TERM.TYPE
 *     9   CPU time (mS)
 *    10   DATA queue active?
 *    11   Select list 0 active?
 *    12   Time (same as TIME())
 *    18   User number
 *    23   Break key enabled?
 *    24   Echo enabled?
 *    25   Phantom process?
 *    26   Prompt character
 *    27   UID   (not Windows)
 *    28   EUID  (not Windows)
 *    29   GID   (not Windows)
 *    30   EGID  (not Windows)
 *    31   Licence number
 *    32   System directory pathname
 *    38   Temporary directory pathname
 *    42   Client IP address
 *    91   Windows? (see also 1006)
 *  1000   CAPTURING in effect?
 *  1001   Case inversion on?
 *  1002   Call stack
 *  1003   Return list of open files
 *  1004   Peak number of open files
 *  1005   Internal time equivalent to DATE() * 86400 + TIME()
 *  1006   Windows NT style (NT, 2000, XP)?
 *  1007   Transaction number, zero if none
 *  1008   Transaction level, zero if none
 *  1009   Returns 1 on big endian system, 0 on little endian system
 *  1010   Platform name
 *  1011   qm.ini or qmconfig file pathname
 *  1012   QM version number
 *  1013   User limit (excluding phantom pool)
 *  1014   User limit (including phantom pool)
 *  1015   Computer name
 *  1016   Number of available users
 *  1017   Port number for tcp/ip connection
 *  1018   Device licensing ip address limit
 *  1019   Device licensing current ip address count
 *  1020   Time of day in milliseconds
 *  1021   Get collation map name
 *  1022   Get collation map (user format)
 *  1023   Get collation map (internal format)
 *  1024   Current directory on entry to QM
 *  1025   Environment variables
 *  1026   Command text from "qm xxx"
 *  1027   Port name for serial connection
 *  1028   System id code for system specific licences
 *  1029   Internal subroutine depth
 *  1030   Login time as date * 86400 + time
 *  1031   Operating system process id
 *  1032   Test and clear break pending flag
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include "header.h"
#include "tio.h"
#include "syscom.h"
#include "dh_int.h"
#include "revstamp.h"
#include "config.h"

#include <time.h>
#include <sys/time.h>

extern int txn_depth;

void op_getprompt(void);
void op_sysdir(void);

/* ======================================================================
   op_system()  -  SYSTEM() function                                      */

void op_system() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Key                        |  SYSTEM() value             |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;
  DESCRIPTOR* syscom_descr;
  int16_t key;
  PRINT_UNIT* pu;
  STRING_CHUNK* str;
  struct PROGRAM* prg;
  int32_t offset;
  FILE_ENTRY* fptr;
  int16_t i;
  char s[256 + 1] = "";
  char* p;
  char* q;

  struct timeval tv;

  descr = e_stack - 1;
  GetInt(descr);
  key = (int16_t)(descr->data.value);

  switch (key) {
    case 1: /* Printer on? */
      descr->data.value = (process.program.flags & PF_PRINTER_ON) != 0;
      break;

    case 2: /* Default print unit page width */
      pu = (process.program.flags & PF_PRINTER_ON) ? (&tio.lptr_0) : (&tio.dsp);
      descr->data.value = pu->width;
      break;

    case 3: /* Default print unit page depth */
      pu = (process.program.flags & PF_PRINTER_ON) ? (&tio.lptr_0) : (&tio.dsp);
      descr->data.value = pu->lines_per_page;
      break;

    case 4: /* Default print unit lines remaining on page */
      if (process.program.flags & PF_PRINTER_ON) {
        descr->data.value = tio.lptr_0.data_lines_per_page - tio.lptr_0.line;
      } else {
        descr->data.value = tio.dsp.data_lines_per_page - tio.dsp.line - 1;
        /* Allow one line for pagination prompt */
      }
      break;

    case 5: /* Default print unit page number */
      pu = (process.program.flags & PF_PRINTER_ON) ? (&tio.lptr_0) : (&tio.dsp);
      descr->data.value = pu->page_no;
      break;

    case 6: /* Default print unit line number */
      pu = (process.program.flags & PF_PRINTER_ON) ? (&tio.lptr_0) : (&tio.dsp);
      descr->data.value =
          (pu->line >= pu->data_lines_per_page) ? 1 : (pu->line + 1);
      break;

    case 7: /* Terminal type - Same as @TERM.TYPE */
      k_put_c_string(tio.term_type, descr);
      break;

    case 9: /* Process CPU time (mS) */

      descr->data.value = (clock() * 1000) / CLOCKS_PER_SEC;
      break;

    case 10: /* DATA queue active? */
      syscom_descr = Element(process.syscom, SYSCOM_DATA_QUEUE);
      descr->data.value = (syscom_descr->data.str.saddr != NULL);
      break;

    case 11: /* Select list 0 active? */
      syscom_descr = SelectList(0);
      descr->data.value = (syscom_descr->data.str.saddr != NULL);
      break;

    case 12: /* Time - Same as TIME() */
      descr->data.value = local_time() % 86400L;
      break;

    case 18: /* User number */
      descr->data.value = my_uptr->uid;
      break;

    case 23: /* Break key enabled? */
      descr->data.value = (process.break_inhibits == 0);
      break;

    case 24: /* Echo enabled? */
      syscom_descr = Element(process.syscom, SYSCOM_ECHO_INPUT);
      descr->data.value = syscom_descr->data.value;
      break;

    case 25: /* Phantom process? */
      descr->data.value = is_phantom;
      break;

    case 26: /* Prompt character */
      k_dismiss();
      op_getprompt();
      break;

    case 27: /* UID */
      descr->data.value = getuid();
      break;

    case 28: /* Effective UID */
      descr->data.value = geteuid();
      break;

    case 29: /* GID */
      descr->data.value = getgid();
      break;

    case 30: /* Effective GID */
      descr->data.value = getegid();
      break;

    case 31: /* Licence number */
      descr->data.value = 0;
      /* If there is ever a need, programs can use this to determine whether
         they are running on the GPL version of the product.
         DO NOT ALTER THE RETURNED VALUE                                     */
      break;

    case 32: /* System directory pathname */
      k_dismiss();
      op_sysdir();
      break;

    case 38: /* Temporary directory pathname */
      k_put_c_string(pcfg.tempdir, descr);
      break;

    case 42: /* IP address */
      k_put_c_string(ip_addr, descr);
      break;

    case 91: /* Windows? */
      descr->data.value = 0;
      break;

    case 1000: /* CAPTURING in effect? */
      descr->data.value = capturing;
      break;

    case 1001: /* Case inversion on? */
      descr->data.value = case_inversion;
      break;

    case 1002: /* Call stack */
      str = NULL;
      ts_init(&str, 1024);
      if (c_base != NULL) {
        prg = &(process.program);
        offset = pc - c_base;
        do {
          ts_printf("%s%c%d%c%d", ProgramName(prg->saved_c_base), VALUE_MARK,
                    offset, SUBVALUE_MARK,
                    k_line_no(offset, prg->saved_c_base));

          /* Local gosub return pointers
               Back up to point to GOSUB rather than to return address */

          for (i = prg->gosub_depth - 1; i >= 0; i--) {
            offset = prg->gosub_stack[i] - 1; /* Back up to GOSUB (etc) */
            ts_printf("%c%d%c%d", VALUE_MARK, offset, SUBVALUE_MARK,
                      k_line_no(offset, prg->saved_c_base));
          }

          if ((prg = prg->prev) == NULL)
            break;

          offset = prg->saved_pc_offset - 1; /* Back up into CALL */
          ts_printf("%c", FIELD_MARK);
        } while (1);
      }
      ts_terminate();
      InitDescr(descr, STRING);
      descr->data.str.saddr = str;
      break;

    case 1003: /* Return list of open files */
      str = NULL;
      ts_init(&str, 1024);

      for (i = 1, fptr = FPtr(1); i <= sysseg->used_files; i++, fptr++) {
        if (fptr->ref_ct) {
          if (str != NULL)
            ts_copy_byte(FIELD_MARK);
          ts_printf("%d\xfd%s", (int)i, fptr->pathname);
        }
      }

      ts_terminate();
      InitDescr(descr, STRING);
      descr->data.str.saddr = str;
      break;

    case 1004: /* Return peak number of open files */
      descr->data.value = sysseg->used_files;
      break;

    case 1005: /* Internal time - Equivalent to DATE() * 86400 + TIME() */
      descr->data.value = qmtime();
      break;

    case 1006: /* Windows NT style? */
      descr->data.value = is_nt;
      break;

    case 1007: /* Transaction number */
      descr->data.value = process.txn_id;
      break;

    case 1008: /* Transaction level */
      descr->data.value = txn_depth;
      break;

    case 1009: /* Endian */
#ifdef BIG_ENDIAN_SYSTEM
      descr->data.value = 1;
#else
      descr->data.value = 0;
#endif
      break;

    case 1010: /* Platform name */
      k_put_c_string(PLATFORM_NAME, descr);
      break;

    case 1011: /* Full path of qm.ini or qmconfig file */
      k_put_c_string(config_path, descr);
      break;

    case 1012: /* QM version number */
      k_put_c_string(QM_REV_STAMP, descr);
      break;

    case 1013: /* User limit, excluding phantom pool */
    case 1014: /* User limit, including phantom pool */
      descr->data.value = sysseg->max_users;
      break;

    case 1015: /* Computer name */
      gethostname(s, 100);
      k_put_c_string(s, descr);
      break;

    case 1017: /* Port number for tcp/ip connection */
      descr->data.value = port_no;
      break;

    case 1020: /* Time of day in milliseconds */
      gettimeofday(&tv, NULL);
      descr->data.value = (tv.tv_sec % 86400) * 1000 + (tv.tv_usec / 1000);
      break;

    case 1021: /* Get collation map name */
      k_put_c_string(collation_map_name, descr);
      break;

    case 1022: /* Get collation map (user format) */
      if (collation != NULL) {
        /* Transform map back to "source" form */
        for (i = 0; i < 256; i++)
          s[(u_char)collation[i]] = i;
        k_put_string(s, 256, descr);
      } else
        k_put_string("", 0, descr);
      break;

    case 1023: /* Get collation map (internal format) */
      k_put_string(collation, 256, descr);
      break;

    case 1024: /* Current directory on entry to QM */
      k_put_c_string(entry_dir, descr);
      break;

    case 1025: /* Environment variables */
      str = NULL;
      ts_init(&str, 1024);
      for (i = 0; (p = environ[i]) != NULL; i++) {
        if (i)
          ts_copy_byte(VALUE_MARK);
        q = strchr(p, '=');
        if (q != NULL)
          ts_copy(p, q - p);
      }

      ts_copy_byte(FIELD_MARK);

      for (i = 0; (p = environ[i]) != NULL; i++) {
        if (i)
          ts_copy_byte(VALUE_MARK);
        q = strchr(p, '=');
        if (q != NULL)
          ts_copy_c_string(q + 1);
      }
      ts_terminate();
      InitDescr(descr, STRING);
      descr->data.str.saddr = str;
      break;

    case 1026:
      k_put_c_string(single_command, descr);
      break;

    case 1027: /* Serial port name */
      k_put_c_string(port_name, descr);
      break;

    case 1028: /* System id */
      /* Recognised but returns zero on GPL source */
      break;

    case 1029: /* Internal subroutine depth */
      descr->data.value = process.program.gosub_depth;
      break;

    case 1030: /* Login time */
      descr->data.value = my_uptr->login_time;
      break;

    case 1031: /* Operating system process id */
      descr->data.value = my_uptr->pid;
      break;

    case 1032: /* Test and clear break pending flag */
      descr->data.value = break_pending;
      break_pending = FALSE;
      break;
    /* njs 13Feb23 */  
    case 1050: /* admin flag user */
      descr->data.value = (my_uptr->flags & USR_ADMIN) != 0;
      break;   

    default:
      k_recurse(pcode_system, 1); /* Execute recursive code */
      break;
  }
}

/* END-CODE */
