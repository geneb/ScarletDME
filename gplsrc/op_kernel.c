/* OP_KERNEL.C
 * Kernel opcodes.
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
 * 15Jan22 gwb Fixed argument formatting issues (CwE-686), reformmated
 *             entire source file.
 * 
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 *
 * 22Feb20 gwb Cleared a pair of variables set but not used in op_cnctport and
 *             converted an sprintf() to snprintf() in op_phantom().
 * START-HISTORY (OpenQM):
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 28 Jun 07  2.5-7 Extracted from kernel.c
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include "revstamp.h"
#include "header.h"
#include "tio.h"
#include "debug.h"
#include "keys.h"
#include "syscom.h"
#include "config.h"
#include "options.h"
#include "dh_int.h"
#include "locks.h"

#include <sys/wait.h>

Public bool case_sensitive;

int help(char *key);
bool IsAdmin(void);
bool recover_users(void);
void set_date(int32_t);

Private bool run_exe(char *exe_name, char *cmd_line);

/* ======================================================================
   op_kernel()  -  KERNEL()  -  Miscellaneous kernel functions            */

void op_kernel() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  Qualifier                     |  Result                     |
     |--------------------------------|-----------------------------|
     |  Action key                    |                             |
     |================================|=============================|
     
   Keys:
     K$INTERNAL           Set or clear internal mode
     K$INTERNAL.QUERY     Query internal mode
     K$PAGINATE           Test or modify pagination flag
     K$FLAGS              Test/return program header flags
     K$DATE.FORMAT        European date format?
     K$CRTWIDE            Return display width
     K$CRTHIGH            Return display lines per page
     K$SET.DATE           Set current date
     K$IS.PHANTOM         Is this a phantom process
     K$TERM.TYPE          Terminal type name
     K$USERNAME           User name
     K$DATE.CONV          Set default date conversion
     K$PPID               Get parent process id
     K$USERS              Get user list
     K$INIPATH            Get ini file pathname
     K$FORCED.ACCOUNT     Force entry to named account unless set in $LOGINS
     K$QMNET              Get/set QMNet status flag
     K$CPROC.LEVEL        Get/set command processor level
     K$HELP               Invoke help system
     K$SUPPRESS.COMO      Supress/resume como file output
     K$ADMINISTRATOR      Get/set administrator rights
     K$SECURE             Secure system?
     K$GET.OPTIONS        Get options flags
     K$SET.OPTIONS        Set options flags
     K$PRIVATE.CATALOGUE  Set private catalogue pathname
     K$CLEANUP            Clean up defunct users
     K$COMMAND.OPTIONS    Get command line option flags
     K$CASE.SENSITIVE     REMOVE.TOKEN() cases sensitivity
     K$SET.LANGUAGE       Set language for message handler
     K$COLLATION          Set/clear sort collation data
     K$GET.QMNET.CONNECTIONS  Get details of open QMNet connections
     K$INVALIDATE.OBJECT  Invalidate object cache
     K$MESSAGE            Enable/disable message reception
     K$SET.EXIT.CAUSE     Set k_exit_cause
     K$COLLATION.NAME     Set primary collation map name
     K$AK.COLLATION       Select AK collation map
     K$EXIT.STATUS        Set exit status
     K$AUTOLOGOUT         Set/retrieve autologout period
     K$MAP.DIR.IDS        Enable/disable dir file id mapping
 */

  DESCRIPTOR *descr;
  int action;
  DESCRIPTOR result;
  int32_t n;
  char s[(MAX_PATHNAME_LEN * 2) + 1];
  int16_t i;
  int16_t j;
  char *p;
  int32_t *q;
  USER_ENTRY *uptr;
  STRING_CHUNK *str;

  InitDescr(&result, INTEGER);
  result.data.value = 0;

  descr = e_stack - 2;
  GetInt(descr);
  action = descr->data.value;

  descr = e_stack - 1;
  k_get_value(descr);

  switch (action) {
    case K_INTERNAL:
      GetInt(descr);
      if (descr->data.value < 0)
        result.data.value = internal_mode;
      else
        internal_mode = (descr->data.value != 0);

      break;

    case K_SECURE:
      GetInt(descr);
      if (descr->data.value < 0) {
        if (is_nt)
          result.data.value = TRUE;
        else
          result.data.value = ((sysseg->flags & SSF_SECURE) != 0);
      } else {
        if (descr->data.value)
          sysseg->flags |= SSF_SECURE;
        else
          sysseg->flags &= ~SSF_SECURE;
      }

      break;

    case K_LPTRHIGH: /* Same as SYSTEM(3) */
      if (process.program.flags & PF_PRINTER_ON)
        result.data.value = tio.lptr_0.lines_per_page;
      else
        result.data.value = tio.dsp.lines_per_page;

      break;

    case K_LPTRWIDE: /* Same as SYSTEM(2) */
      if (process.program.flags & PF_PRINTER_ON)
        result.data.value = tio.lptr_0.width;
      else
        result.data.value = tio.dsp.width;

      break;

    case K_PAGINATE:
      if ((n = descr->data.value) < 0) { /* Enquiry */
        result.data.value = (tio.dsp.flags & PU_PAGINATE) != 0;
      } else { /* Set pagination state */
        if (n) {
          tio.dsp.flags |= PU_PAGINATE;
          tio.dsp.line = 0;
        } else {
          tio.dsp.flags &= ~PU_PAGINATE;
        }
      }
      break;

    case K_FLAGS:
      GetInt(descr);
      n = descr->data.value;
      if (n)
        result.data.value = ((process.program.flags & n) != 0);
      else
        result.data.value = process.program.flags;
      break;

    case K_DATE_FORMAT:
      GetInt(descr);
      n = descr->data.value;
      if (n >= 0)
        european_dates = (n != 0);
      result.data.value = european_dates;
      break;

    case K_CRTHIGH:
      result.data.value = tio.dsp.lines_per_page;
      break;

    case K_CRTWIDE:
      result.data.value = tio.dsp.width;
      break;

    case K_SET_DATE:
      GetInt(descr);
      set_date(descr->data.value);
      break;

    case K_IS_PHANTOM:
      result.data.value = is_phantom;
      break;

    case K_TERM_TYPE:
      k_get_string(descr);
      if (descr->data.str.saddr != NULL) {
        k_get_c_string(descr, s, 32);
        settermtype(s);
      }
      k_put_c_string(tio.term_type, &result);
      break;

    case K_USERNAME:
      k_put_c_string(process.username, &result);
      break;

    case K_DATE_CONV:
      if ((result.data.value = (k_get_c_string(descr, s, 32))) > 0) {
        strcpy(default_date_conversion, s);
      }
      k_put_c_string(default_date_conversion, &result);
      break;

    case K_PPID:
      result.data.value = my_uptr->puid;
      break;

    case K_USERS:
      InitDescr(&result, STRING);
      result.data.str.saddr = NULL;
      ts_init(&(result.data.str.saddr), 128);
      GetInt(descr);
      n = descr->data.value; /* User number or zero for all */
      for (i = 1; i <= sysseg->max_users; i++) {
        uptr = UPtr(i);
        if (((n == 0) && (uptr->uid > 0)) || ((n != 0) && (uptr->uid == n))) {
          if (result.data.str.saddr != NULL)
            ts_copy_byte(FIELD_MARK);

          ts_printf("%d%c%d%c%s%c%d%c%d%c%s%c%s%c%d", (int)(uptr->uid), VALUE_MARK, (int)(uptr->pid), VALUE_MARK, 
                    uptr->ip_addr, VALUE_MARK, (int)(uptr->flags), VALUE_MARK, uptr->puid, VALUE_MARK,
                    uptr->username, VALUE_MARK, uptr->ttyname, VALUE_MARK, uptr->login_time);
          if (n)
            break;
        }
      }
      ts_terminate();
      break;

    case K_INIPATH:
      k_put_c_string(config_path, &result);
      break;

    case K_FORCED_ACCOUNT:
      k_put_c_string(forced_account, &result);
      break;

    case K_QMNET:
      if (descr->data.value < 0)
        result.data.value = ((my_uptr->flags & USR_QMNET) != 0);
      else if (descr->data.value)
        my_uptr->flags |= USR_QMNET;
      else
        my_uptr->flags &= ~USR_QMNET;
      break;

    case K_CPROC_LEVEL:
      if (descr->data.value > 0)
        cproc_level = (int16_t)(descr->data.value);
      result.data.value = cproc_level;
      break;

    case K_HELP:
      if (k_get_c_string(descr, s, 200) >= 0)
        result.data.value = help(s);
      else
        result.data.value = -1; /* Key too long */
      break;

    case K_SUPPRESS_COMO:
      if ((n = descr->data.value) < 0) { /* Enquiry */
        result.data.value = (tio.suppress_como);
      } else { /* Set suppression state */
        tio.suppress_como = (n != 0);
      }
      break;

    case K_IS_QMVBSRVR:
      result.data.value = is_QMVbSrvr;
      break;

    case K_ADMINISTRATOR:
      GetInt(descr);
      n = descr->data.value;
      if (n >= 0) { /* Setting / clearing */
        if ((n > 0) || IsAdmin())
          my_uptr->flags |= USR_ADMIN;
        else
          my_uptr->flags &= ~USR_ADMIN;
      }
      result.data.value = (my_uptr->flags & USR_ADMIN) != 0;
      break;

    case K_FILESTATS:
      GetInt(descr);
      if (descr->data.value) { /* Reset counters */
        memset((char *)&(sysseg->global_stats), 0, sizeof(struct FILESTATS));
        sysseg->global_stats.reset = qmtime();
      } else {
        InitDescr(&result, STRING);
        result.data.str.saddr = NULL;
        ts_init(&(result.data.str.saddr), 5 * FILESTATS_COUNTERS);
        for (i = 0, q = (int32_t *)&(sysseg->global_stats.reset); i < FILESTATS_COUNTERS; i++, q++) {
          ts_printf("%d\xfe", *q);
        }
        (void)ts_terminate();
      }
      break;

    case K_TTY:
      k_put_c_string((char *)(my_uptr->ttyname), &result);
      break;

    case K_GET_OPTIONS:
      for (i = 0; i < NumOptions; i++)
        s[i] = option_flags[i] + '0';

      s[NumOptions] = '\0';
      k_put_c_string(s, &result);
      break;

    case K_SET_OPTIONS:
      j = k_get_c_string(descr, s, 200);
      for (i = 0; (i < j) && (i < NumOptions); i++)
        SetOption(i, s[i] == '1');

      break;

    case K_PRIVATE_CATALOGUE:
      j = k_get_c_string(descr, private_catalogue, MAX_PATHNAME_LEN);
      break;

    case K_CLEANUP:
      result.data.value = recover_users();
      break;

    case K_OBJKEY:
      result.data.value = object_key;
      break;

    case K_COMMAND_OPTIONS:
      result.data.value = command_options;
      break;

    case K_CASE_SENSITIVE:
      GetInt(descr);
      if (descr->data.value < 0)
        result.data.value = case_sensitive;
      else
        case_sensitive = (descr->data.value != 0);
      break;

    case K_SET_LANGUAGE:
      k_get_c_string(descr, s, 3);
      result.data.value = load_language(s);
      break;

    case K_HSM:
      GetInt(descr);
      switch (descr->data.value) {
        case 0: /* Disable */
          hsm = FALSE;
          break;
        case 1: /* Enable */
          hsm_on();
          break;
        case 2: /* Return data */
          InitDescr(&result, STRING);
          result.data.str.saddr = hsm_dump();
          break;
      }
      break;

    case K_COLLATION:
      k_get_string(descr);
      if (descr->data.str.saddr == NULL) {
        primary_collation = NULL;
        collation = NULL;
      } else {
        str = s_make_contiguous(descr->data.str.saddr, NULL);
        descr->data.str.saddr = str;
        memcpy(primary_collation_map, str->data, 256);
        primary_collation = primary_collation_map;
        collation = primary_collation_map;
      }
      break;

    case K_GET_QMNET_CONNECTIONS:
      InitDescr(&result, STRING);
      result.data.str.saddr = get_qmnet_connections();
      break;

    case K_INVALIDATE_OBJECT:
      invalidate_object();
      break;

    case K_MESSAGE:
      GetInt(descr);
      n = descr->data.value;
      if (n == 0)
        my_uptr->flags |= USR_MSG_OFF;
      else if (n > 0)
        my_uptr->flags &= ~USR_MSG_OFF;
      result.data.value = (my_uptr->flags & USR_MSG_OFF) == 0;
      break;

    case K_SET_EXIT_CAUSE:
      GetInt(descr);
      k_exit_cause = descr->data.value;
      break;

    case K_COLLATION_NAME:
      setqmstring(&collation_map_name, descr);
      break;

    case K_AK_COLLATION:
      if (descr->type == STRING) {
        str = descr->data.str.saddr;
        collation = (str == NULL) ? NULL : str->data;
      } else
        collation = primary_collation;
      break;

    case K_EXIT_STATUS:
      GetInt(descr);
      exit_status = descr->data.value;
      break;

    case K_CASE_MAP:
      GetString(descr);
      if ((str = descr->data.str.saddr) == NULL)
        set_default_character_maps();
      else {
        uc_chars[(u_char)(str->data[1])] = str->data[0];
        lc_chars[(u_char)(str->data[0])] = str->data[1];
        char_types[(u_char)(str->data[0])] |= CT_ALPHA | CT_GRAPH;
        char_types[(u_char)(str->data[1])] |= CT_ALPHA | CT_GRAPH;
      }
      break;

    case K_AUTOLOGOUT:
      GetInt(descr);
      if (descr->data.value >= 0)
        autologout = descr->data.value;
      result.data.value = autologout;
      break;

    case K_MAP_DIR_IDS:
      GetInt(descr);
      if (descr->data.value >= 0)
        map_dir_ids = (descr->data.value != 0);
      result.data.value = map_dir_ids;
      break;

    case K_IN_GROUP:
      k_get_c_string(descr, s, 64);
      result.data.value = in_group(s);
      break;

    case K_BREAK_HANDLER:
      k_get_c_string(descr, s, 64);
      setstring(&process.program.break_handler, s);
      break;

    case K_RUNEXE:
      k_get_c_string(descr, s, sizeof(s) - 1);
      p = strchr(s, ' ');
      if (p != NULL) {
        *(p++) = '\0';
      }
      result.data.value = run_exe(s, p);
      break;

    default:
      k_error("Illegal KERNEL() action key (%d)", action);
  }

  k_dismiss();
  *(e_stack - 1) = result;
}

/* ======================================================================
   op_lgnport()  -  Login a serial port process                           */

void op_lgnport() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top | Account name                   |  Successful? (true/false)   |
     |--------------------------------|-----------------------------|
     | Port name{:params}             |                             |
     |================================|=============================|
 */

  bool status = FALSE;

  process.status = ER_UNSUPPORTED;

  k_dismiss();
  k_dismiss();

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = status;
}

/* ======================================================================
   op_option()  -  OPTION() function                                      */

void op_option() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  Option number                 |  Option state               |
     |================================|=============================|
 */

  DESCRIPTOR *descr;
  int opt;

  descr = e_stack - 1;
  GetInt(descr);
  opt = descr->data.value;
  if ((opt >= 0) && (opt < NumOptions))
    descr->data.value = Option(opt);
  else
    descr->data.value = 0;
}

/* ======================================================================
   op_phantom()  -  PHANTOM  -  Start new process                         */

void op_phantom() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |                                |  QM user id, zero if fails  |
     |================================|=============================|
 */

  int16_t i;
  USER_ENTRY *uptr;
  int16_t phantom_user_index;
  int16_t phantom_uid = 0;
  char path[MAX_PATHNAME_LEN + 1];
  char option[15 + 1];
  int cpid;

  /* Reserve a user table entry for the phantom process */

  StartExclusive(SHORT_CODE, 38);
  phantom_user_index = 0;
  for (i = 1; i <= sysseg->max_users; i++) {
    uptr = UPtr(i);
    if (uptr->uid == 0) /* Spare cell */
    {
      phantom_uid = assign_user_no(i);
      uptr->uid = phantom_uid;
      uptr->puid = process.user_no;
      strcpy((char *)(uptr->username), (char *)(my_uptr->username));
      phantom_user_index = i;
      break;
    }
  }
  EndExclusive(SHORT_CODE);

  if (phantom_user_index == 0)
    goto exit_op_phantom;

  /* Construct command for CreateProcess */

  cpid = fork();
  if (cpid == 0) { /* Child process */
    //0387   close(0);
    //0387   close(1);
    //0387   close(2);
    for (i = 3; i < 1024; i++)
      close(i); /* 0401 */

    daemon(1, 1);
    /* converted to snprintf() -gwb 22Feb20 */
    if (snprintf(path, MAX_PATHNAME_LEN + 1, "%s/bin/qm", sysseg->sysdir) >= (MAX_PATHNAME_LEN + 1)) {
      /* TODO: this should also be logged with more detail */
      k_error("Overflowed path/filename length in op_phantom()!");
      goto exit_op_phantom;
    }
    sprintf(option, "-p%d", phantom_user_index);
    execl(path, path, option, NULL);
  } else if (cpid == -1) { /* Error */
    *(UMap(uptr->uid)) = 0;
    uptr->uid = 0; /* Release reserved user cell */
    uptr->puid = 0;
    phantom_uid = 0;
  } else { /* Parent process */
    waitpid(cpid, NULL, WNOHANG);
  }

exit_op_phantom:
  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = phantom_uid;
}

/* ======================================================================
   op_chgphant()  -  Make process a "chargeable" phantom                  */

void op_chgphant() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |                                |  1 = ok, 0 = error          |
     |================================|=============================|
 */

  bool status = TRUE;

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = status;
}

/* ======================================================================
   op_cnctport()  -  CONNECT.PORT()                                        */

void op_cnctport() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top | Stop bits                      |  1 = ok, 0 = error          |
     |--------------------------------|-----------------------------|
     | Bits per byte                  |                             |
     |--------------------------------|-----------------------------|
     | Parity mode                    |                             |
     |--------------------------------|-----------------------------|
     | Baud rate                      |                             |
     |--------------------------------|-----------------------------|
     | Port name                      |                             |
     |================================|=============================|
 */

  DESCRIPTOR *descr;
  int stop_bits;
  int bits_per_byte;
  /* int parity; variable set but not used */
  /* int baud_rate; variable set but not used */
  char portname[20 + 1];

  process.status = ER_PARAMS;

  descr = e_stack - 1;
  GetInt(descr);
  stop_bits = descr->data.value;
  if ((stop_bits < 1) || (stop_bits > 20))
    goto exit_op_cnctport;

  descr = e_stack - 2;
  GetInt(descr);
  bits_per_byte = descr->data.value;
  if ((bits_per_byte < 7) || (bits_per_byte > 8))
    goto exit_op_cnctport;

  descr = e_stack - 3;
  GetInt(descr);
  /* parity = descr->data.value; set but never used */

  descr = e_stack - 4;
  GetInt(descr);
  /* baud_rate = descr->data.value; set but never used */

  descr = e_stack - 5;
  if (k_get_c_string(descr, portname, 20) < 1) {
    process.status = ER_LENGTH;
    goto exit_op_cnctport;
  }

  process.status = 0;

  /* This operation is only valid if we are a phantom process with a
    connection type of CN_NONE.                                      */

  if (!is_phantom) {
    process.status = ER_NOT_PHANTOM;
    goto exit_op_cnctport;
  }

  if (connection_type != CN_NONE) {
    process.status = ER_CONNECTED;
    goto exit_op_cnctport;
  }

  process.status = ER_UNSUPPORTED;

exit_op_cnctport:
  k_pop(4);
  k_dismiss();
  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = (process.status == 0);
}

/* ======================================================================
   op_login()  -  LOGIN()  -  Perform login for socket based process      */

void op_login() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  Password                      |  1 = ok, 0 = error          |
     |--------------------------------|-----------------------------|
     |  User name                     |                             |
     |================================|=============================|
 */

  bool ok;
  DESCRIPTOR *descr;
  char username[32 + 1];
  char password[32 + 1];

  /* Get password */

  descr = e_stack - 1;
  (void)k_get_c_string(descr, password, 32);
  k_dismiss();

  /* Get username */

  descr = e_stack - 1;
  (void)k_get_c_string(descr, username, 32);
  k_dismiss();

  InitDescr(e_stack, INTEGER);
  ok = login_user(username, password);
  if (ok) {
    strcpy((char *)(my_uptr->username), username);
    strcpy(process.username, username);
  }
  (e_stack++)->data.value = ok;
}

/* ======================================================================
   op_logout()  -  LOGOUT()  -  Logout phantom process                    */

void op_logout() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  Immediate flag                |  1 = ok, 0 = error          |
     |--------------------------------|-----------------------------|
     |  User number                   |                             |
     |================================|=============================|
 */

  DESCRIPTOR *descr;
  int user;
  int status = 0;
  bool immediate;

  /* Get immediate flag */

  descr = e_stack - 1;
  GetBool(descr);
  immediate = (descr->data.value != 0);
  k_pop(1);

  /* Get user number */

  descr = e_stack - 1;
  GetInt(descr);
  user = descr->data.value;

  if (user == 0) {
    k_exit_cause = (immediate) ? K_LOGOUT : K_TERMINATE;
    status = 1;
  } else {
    log_printf(sysmsg(1027), user); /* Force logout initiated for user %d */
    status = raise_event((immediate) ? EVT_LOGOUT : EVT_TERMINATE, user);
  }

  descr->data.value = status;
  return;
}

/* ======================================================================
   op_events()  -  EVENTS()                                               */

void op_events() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  Event flag values, 0 = query  | Event flag values           |
     |  -ve = unset event             |                             |
     |--------------------------------|-----------------------------|
     |  User number (-ve = all)       |                             |
     |================================|=============================|

 Negative user number is not meaningful for query.
 STATUS() = 0 if user found, non-zero if user not found
 */

  DESCRIPTOR *descr;
  int32_t flags;
  int user;
  USER_ENTRY *uptr;
  int16_t i;

  /* Get flag values */

  descr = e_stack - 1;
  GetInt(descr);
  flags = descr->data.value;

  /* Get user number */

  descr = e_stack - 2;
  GetInt(descr);
  user = descr->data.value;
  k_pop(1);

  process.status = 1;

  StartExclusive(SHORT_CODE, 46);
  for (i = 1; i <= sysseg->max_users; i++) {
    uptr = UPtr(i);
    if ((uptr->uid == user) || (user < 0)) {
      if (flags > 0)
        uptr->events |= flags;
      else if (flags < 0)
        uptr->events &= ~-flags;
      descr->data.value = uptr->events;
      process.status = 0;
      if (user > 0)
        break;
    }
  }
  EndExclusive(SHORT_CODE);
}

/* ======================================================================
   op_setflags()  -  SETFLAGS opcode  - Set opcode_flags                  */

void op_setflags() {
  register u_int16_t flags;

  flags = *(pc++);
  flags |= *(pc++) << 8;

  process.op_flags |= flags;
}

/* ======================================================================
   op_userno()  -  USERNO  -  Get user number                             */

void op_userno() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |                                |  User no                    |
     |================================|=============================|
 */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = process.user_no;
}

/* ======================================================================
   run_exe()  -  Run executable from QM session                           */

Private bool run_exe(char *exe_name, char *cmd_line) {
  //* NIX implementation to follow
  process.status = ER_FAILED;
  return FALSE;
}

/* END-CODE */
