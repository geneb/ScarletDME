/* PDUMP.C
 * Process dump.
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
 * 02Jan22 gwb Cleaned up a number of warnings related to using the wrong
 *             format specifier vs the variable type being formatted.
 *
 * 28Feb20 gwb Added indicator of platform word size.
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 *
 * START-HISTORY (OpenQM):
 * 15 Aug 07  2.6-0 Reworked remove pointers.
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 23 Mar 06  2.3-9 Array tracking was counting elements incorrectly.
 * 21 Mar 06  2.3-9 Limit length of dumped string.
 * 01 Dec 05  2.2-18 Added LOCALVARS.
 * 30 Jun 05  2.2-3 Added SOCK data type for sockets.
 * 13 May 05  2.1-15 Added OS.ERROR().
 * 03 May 05  2.1-13 Added DEBUG setting for dump of internal mode programs.
 * 08 Apr 05  2.1-12 Added DUMPDIR parameter.
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
#include "header.h"
#include "syscom.h"
#include "config.h"
#include "revstamp.h"
#include "locks.h"
#include "dh_int.h"

#include <time.h>
#include <stdint.h>

#define MAX_DUMPED_STRING_LEN 10000

/* Common block memory */

struct COM {
  struct COM* next;
  int prog_no;
  char name[1];
};
Private struct COM* com_head = NULL;

Private char* sym_tab;

Private FILE* fu = NULL;
Private int16_t prog_no;
Private void dump_variable(DESCRIPTOR* descr,
                           int16_t var_no,
                           char* symbol,
                           int16_t indent);
Private void print_string(STRING_CHUNK* str);
Private void dump_syscom_var(char* tag, int16_t offset);

/* ====================================================================== */

void pdump() {
  char path[MAX_PATHNAME_LEN + 1];
  struct PROGRAM* pgm;
  int32_t offset;
  char* sym;
  int16_t sym_bytes;
  char symbol[64 + 1];
  bool first;
  FILE_ENTRY* fptr;
  RLOCK_ENTRY* rlptr;
  int16_t lock_owner;
  struct COM* com_ptr;
  struct COM* com_next;
  DESCRIPTOR* descr;
  time_t tm;
  char c;
  int i;
  int n;

  /* Symbol table sorting data */

  struct SYM {
    struct SYM* next;
    int var_no;
    char name[1];
  };
  struct SYM* sym_head;
  struct SYM* sym_ptr;
  struct SYM* sym_prev;
  struct SYM* sym_new;

  sprintf(path, "%s%cqmdump.%d",
          (pcfg.dumpdir[0] != '\0') ? pcfg.dumpdir : (sysseg->sysdir), DS,
          process.user_no);
  tio_printf("Dumping process state as %s\n", path);

  /* Open dump file */

  FDS_close(); /* Ensure free handle without even checking if needed */

  fu = fopen(path, FOPEN_WRITE_MODE);
  if (fu == NULL) {
    tio_printf("Cannot open dump file\n");
    goto exit_pdump;
  }

  /* Write dump file header */

  time(&tm);

// shows how we were compiled.
// The method below is cpu-agnostic in theory.
#if INTPTR_MAX == INT64_MAX // __x86_64__
  fprintf(fu, "64 Bit ");
#elif INTPTR_MAX == INT32_MAX
  fprintf(fu, "32 Bit ");
#else
  fprintf(fu, "?? Bit ");
#endif

  fprintf(fu, "ScarletDME Process Dump    %s", ctime(&tm));
  fprintf(fu, "==========================================================\n\n");
  /* Own user table entry */

  fprintf(fu, "User %d. Process id %d. Parent user %d.\n", process.user_no,
          my_uptr->pid, my_uptr->puid);
  fprintf(fu, "User name '%s'\n", my_uptr->username);

  /* Global system variables */

  fprintf(fu, "\nSTATUS() = %d, OS.ERROR() = %d\n", process.status,
          process.os_error);

  /* SYSCOM data */

  fprintf(fu, "\n\n@-variables\n");
  dump_syscom_var("@ABORT.CODE", SYSCOM_ABORT_CODE);
  dump_syscom_var("@ABORT.MESSAGE", SYSCOM_ABORT_MESSAGE);
  dump_syscom_var("@COMMAND", SYSCOM_COMMAND);
  dump_syscom_var("@DATA.PENDING", SYSCOM_DATA_QUEUE);
  dump_syscom_var("@ID", SYSCOM_QPROC_ID);
  dump_syscom_var("@PATH", SYSCOM_ACCOUNT_PATH);
  dump_syscom_var("@RECORD", SYSCOM_QPROC_RECORD);
  dump_syscom_var("@SELECTED", SYSCOM_SELECTED);
  dump_syscom_var("@SENTENCE", SYSCOM_SENTENCE);
  dump_syscom_var("@SYSTEM.RETURN.CODE", SYSCOM_SYSTEM_RETURN_CODE);
  dump_syscom_var("@USER.RETURN.CODE", SYSCOM_USER_RETURN_CODE);
  dump_syscom_var("@USER0", SYSCOM_USER0);
  dump_syscom_var("@USER1", SYSCOM_USER1);
  dump_syscom_var("@USER2", SYSCOM_USER2);
  dump_syscom_var("@USER3", SYSCOM_USER3);
  dump_syscom_var("@USER4", SYSCOM_USER4);

  /* Task locks */

  first = TRUE;
  for (i = 0; i < 64; i++) {
    if (sysseg->task_locks[i] == process.user_no) {
      if (first) {
        fprintf(fu, "\nTask Locks\n ");
        first = FALSE;
      }
      fprintf(fu, "%2d ", i);
    }
  }

  /* File locks */

  first = TRUE;
  for (i = 1, fptr = FPtr(1); i <= sysseg->used_files; i++, fptr++) {
    lock_owner = abs(fptr->file_lock);
    if (lock_owner == process.user_no) {
      if (first) {
        fprintf(fu, "\nFile locks\n");
        first = FALSE;
      }
      fprintf(fu, "  %s (%s)\n", fptr->pathname,
              (lock_owner < 0) ? "SX" : "FX");
    }
  }

  /* Record locks */

  first = TRUE;
  for (i = 1; i <= sysseg->numlocks; i++) {
    rlptr = RLPtr(i);
    if ((rlptr->hash != 0) && (rlptr->owner == process.user_no)) {
      if (first) {
        fprintf(fu, "\nRecord locks\n");
        first = FALSE;
      }
      fptr = FPtr(rlptr->file_id);
      fprintf(fu, "  %s %.*s (%s)\n", fptr->pathname, rlptr->id_len, rlptr->id,
              (rlptr->lock_type == L_UPDATE) ? "RU" : "RL");
    }
  }

  /* Dump programs, top down */

  fprintf(fu, "\n\n======== PROGRAM STACK (Most recent first) ========\n\n");
  pgm = &(process.program);
  offset = pc - c_base;
  prog_no = 0;
  do {
    fprintf(fu, "===== [%d] Program %s at 0x%08X (line %d)\n", ++prog_no,
            ProgramName(pgm->saved_c_base), offset,
            k_line_no(offset, pgm->saved_c_base));

    if (pgm->flags) {
      fprintf(fu, "  Program status flags\n");
      if (pgm->flags & IS_EXECUTE)
        fprintf(fu, "    Started via EXECUTE\n");
      if (pgm->flags & IGNORE_ABORTS)
        fprintf(fu, "    Ignore aborts\n");
      if (pgm->flags & PF_IS_TRIGGER)
        fprintf(fu, "    Is trigger\n");
      if (pgm->flags & SORT_ACTIVE)
        fprintf(fu, "    Sort in progress\n");
      if (pgm->flags & PF_IS_VFS)
        fprintf(fu, "    VFS handler\n");
      if (pgm->flags & PF_CAPTURING)
        fprintf(fu, "    Capturing\n");
      if (pgm->flags & HDR_IS_CPROC)
        fprintf(fu, "    Command processor\n");
      if (pgm->flags & HDR_INTERNAL)
        fprintf(fu, "    Internal mode\n");
      if (pgm->flags & HDR_DEBUG)
        fprintf(fu, "    Debug mode\n");
      if (pgm->flags & HDR_IS_DEBUGGER)
        fprintf(fu, "    Is debugger\n");
      if (pgm->flags & HDR_IS_FUNCTION)
        fprintf(fu, "    Is function\n");
      if (pgm->flags & HDR_RECURSIVE)
        fprintf(fu, "    Recursive\n");
      if (pgm->flags & HDR_ITYPE)
        fprintf(fu, "    I or C-type\n");
      if (pgm->flags & HDR_ALLOW_BREAK)
        fprintf(fu, "    Allow break in recursive\n");
      if (pgm->flags & HDR_IS_TRUSTED)
        fprintf(fu, "    Trusted\n");
      if (pgm->flags & HDR_NETFILES)
        fprintf(fu, "    Allow NFS\n");
      if (pgm->flags & HDR_CASE_SENSITIVE)
        fprintf(fu, "    Case sensitive names\n");
      if (pgm->flags & HDR_QMCALL_ALLOWED)
        fprintf(fu, "    QMCall allowed\n");
      if (pgm->flags & HDR_CTYPE)
        fprintf(fu, "    C-type\n");
      if (pgm->flags & HDR_IS_CLASS)
        fprintf(fu, "    Class module\n");
      fprintf(fu, "\n");
    }

    fprintf(fu, "  Precision = %d\n", process.program.precision);
    fprintf(fu, "  COL1() = %d, COL2() = %d\n\n", process.program.col1,
            process.program.col2);
    n = pgm->gosub_depth;
    if (n) {
      fprintf(fu, "  Gosub stack\n");
      for (i = n - 1; i >= 0; i--) {
        offset = pgm->gosub_stack[i] - 1; /* Back up to GOSUB (etc) */
        fprintf(fu, "    0x%08X, line %d\n", offset,
                k_line_no(offset, pgm->saved_c_base));
      }
      fprintf(fu, "\n");
    }

    /* Dump variables */

    n = ((OBJECT_HEADER*)(pgm->saved_c_base))->sym_tab_offset;
    if (n == 0) {
      fprintf(fu, "  No symbols\n");

      /* Even though we have no symbols, we dump this program if the
          internal mode debug flag is set.                            */

      if (sysseg->flags & SSF_INT_PDUMP) {
        for (i = 0; i < pgm->no_vars; i++) {
          sprintf(symbol, "%d", i);
          dump_variable(pgm->vars + i, i, symbol, 4);
        }
      }
    } else {
      fprintf(fu, "  Variables\n");

      /* Build sorted list of variable names */

      sym_head = NULL;
      sym_tab = (char*)(pgm->saved_c_base) + n;

      for (i = 0, sym = sym_tab; i < pgm->no_vars; i++) {
        /* Extract symbol name */

        sym_bytes = 1;   /* Char 0 inserted below... */
        symbol[0] = '?'; /* ...but must not be null for now */
        while ((c = *(sym++)) != VALUE_MARK) {
          if (sym_bytes < 64)
            symbol[sym_bytes++] = c;
        }
        symbol[sym_bytes++] = '\0';

        if (!internal_mode) /* Ignore hidden variables if not internal mode */
        {
          if ((symbol[0] == '_') || (symbol[0] == '~'))
            continue;
        }

        sym_new = (struct SYM*)malloc(sizeof(struct SYM) + strlen(symbol));
        if (sym_new != NULL) {
          /* Determine variable type and prefix name with sort code */

          descr = pgm->vars + i;
          while (descr->type == ADDR)
            descr = descr->data.d_addr;
          switch (descr->type) {
            case COMMON:
              symbol[0] = '1';
              break;
            case PERSISTENT:
              symbol[0] = '2';
              break;
            case LOCALVARS:
              symbol[0] = '4';
              break;
            default:
              symbol[0] = '3';
              break;
          }

          /* Make SYM structure */

          strcpy(sym_new->name, symbol);
          sym_new->var_no = i;

          /* Find position for symbol insertion */

          sym_prev = NULL;
          for (sym_ptr = sym_head; sym_ptr != NULL; sym_ptr = sym_ptr->next) {
            if (strcmp(symbol, sym_ptr->name) < 0)
              break;
            sym_prev = sym_ptr;
          }

          if (sym_prev == NULL) /* Goes at head of list */
          {
            sym_new->next = sym_head;
            sym_head = sym_new;
          } else /* Goes after sym_prev item */
          {
            sym_new->next = sym_prev->next;
            sym_prev->next = sym_new;
          }
        }
      }

      /* Now dump the variables in sorted order */

      for (sym_ptr = sym_head; sym_ptr != NULL; sym_ptr = sym_new) {
        dump_variable(pgm->vars + sym_ptr->var_no, sym_ptr->var_no,
                      sym_ptr->name + 1, 4);
        sym_new = sym_ptr->next;
        free(sym_ptr);
      }
    }

    fprintf(fu, "\n");

    if ((pgm = pgm->prev) == NULL)
      break;

    offset = pgm->saved_pc_offset - 1; /* Back up into CALL */
  } while (1);

exit_pdump:

  /* Release any common block memory */

  for (com_ptr = com_head; com_ptr != NULL; com_ptr = com_next) {
    com_next = com_ptr->next;
    free(com_ptr);
  }
  com_head = NULL;

  if (fu != NULL) {
    fclose(fu);
    fu = NULL;
  }
}

/* ====================================================================== */

Private void dump_variable(DESCRIPTOR* descr, /* Descriptor to dump */
                           int16_t var_no,    /* Variable number */
                           char* symbol,      /* Symbol name */
                           int16_t indent)    /* Indentation */
{
  ARRAY_HEADER* ahdr;
  FILE_VAR* fvar;
  ARRAY_CHUNK* achnk;
  int16_t chnk;
  int16_t el;
  bool two_d;
  char element[16];
  char* com_name;
  struct COM* com_ptr;
  int16_t num_com_vars;
  int16_t com_var_no;
  char com_var_name[100];
  STRING_CHUNK* str;
  STRING_CHUNK* rmv_str;
  int32_t rmv_offset;
  int lsub_level;
  char s[80 + 1];
  int16_t n;
  char* p;
  char* q;
  int32_t base;

  while (descr->type == ADDR)
    descr = descr->data.d_addr;

  /* Emit indent */

  memset(s, ' ', indent);
  sprintf(s + indent, "%s", symbol);

  /* ++ALLTYPES++ */
  switch (descr->type) {
    case UNASSIGNED:
      fprintf(fu, "%s: Unassigned\n", s);
      break;

    case INTEGER:
      fprintf(fu, "%s: Int: %d\n", s, descr->data.value);
      break;

    case FLOATNUM:
      fprintf(fu, "%s: Flt: %lf\n", s, descr->data.float_value);
      break;

    case SUBR:
      fprintf(fu, "%s: Subr: %.*s\n", s,
              descr->data.subr.saddr->bytes, /* Always continguous */
              descr->data.subr.saddr->data);
      break;

    case STRING:
      if (descr->flags & DF_REMOVE) {
        rmv_offset = descr->n1;
        rmv_str = descr->data.str.rmv_saddr;
        for (str = descr->data.str.saddr; str != NULL; str = str->next) {
          if (str == rmv_str)
            break;
          rmv_offset += str->bytes;
        }
        fprintf(fu, "%s: String (rmv=%d): \"", s, rmv_offset);
      } else {
        fprintf(fu, "%s: String: \"", s);
      }
      print_string(descr->data.str.saddr);
      break;

    case IMAGE:
      fprintf(fu, "%s: Image\n", s);
      break;

    case FILE_REF:
      fvar = descr->data.fvar;
      fprintf(fu, "%s: File: %s (%s)\n", s,
              (fvar->voc_name != NULL) ? (fvar->voc_name) : "",
              FPtr(fvar->file_id)->pathname);
      break;

    case ARRAY:
      ahdr = descr->data.ahdr_addr;
      two_d = (ahdr->cols != 0);
      if (two_d)
        fprintf(fu, "%s: Array (%d,%d)\n", s, ahdr->rows, ahdr->cols);
      else
        fprintf(fu, "%s: Array (%d)\n", s, ahdr->rows);

      /* Dump array elements */

      for (chnk = 0, base = 0; chnk < ahdr->num_chunks;
           base += achnk->num_descr, chnk++) {
        achnk = ahdr->chunk[chnk];
        for (el = (ahdr->flags & AH_PICK_STYLE) ? 1 : 0; el < achnk->num_descr;
             el++) {
          if (el == 0) {
            strcpy(element, (two_d) ? "(0,0)" : "(0)");
          } else if (two_d) {
            sprintf(element, "(%d,%d)", (el + base - 1) / ahdr->cols,
                    ((el + base - 1) % ahdr->cols) + 1);
          } else {
            sprintf(element, "(%d)", el);
          }
          dump_variable(&(achnk->descr[el]), -1, element, indent + 2);
        }
      }
      break;

    case COMMON:
      com_name = descr->data.ahdr_addr->chunk[0]->descr[0].data.str.saddr->data;

      /* Have we seen this common block already? */

      for (com_ptr = com_head; com_ptr != NULL; com_ptr = com_ptr->next) {
        if (!strcmp(com_ptr->name, com_name))
          break;
      }

      if (com_ptr == NULL) /* Not seen it before */
      {
        fprintf(fu, "%s: Common /%s/\n", s, com_name);

        /* Find start of symbol table entry for this common block */

        if ((p = sym_tab) != NULL) {
          while ((p = strchr(p, FIELD_MARK)) != NULL) {
            p++; /* Skip field mark */

            /* Extract common block number */

            n = 0;
            while (IsDigit(*p))
              n = (n * 10) + (*(p++) - '0');

            if (n == var_no)
              break; /* Found this common */
          }
        }

        num_com_vars = (int16_t)(descr->data.ahdr_addr->rows);
        for (com_var_no = 1; com_var_no <= num_com_vars; com_var_no++) {
          /* Copy variable name */

          q = com_var_name;
          if (p != NULL) {
            p++; /* Skip VM before name */
            while ((!IsMark(*p)) && (*p != '\0'))
              *(q++) = *(p++);
            *q = '\0';
          } else {
            sprintf(com_var_name, "%d", com_var_no);
          }

          dump_variable(Element(descr->data.ahdr_addr, com_var_no), var_no,
                        com_var_name, indent + 2);
        }

        /* Add to chain of dumped commons so we don't dump it again */

        com_ptr = (struct COM*)malloc(sizeof(struct COM) + strlen(com_name));
        if (com_ptr != NULL) {
          strcpy(com_ptr->name, com_name);
          com_ptr->prog_no = prog_no;
          com_ptr->next = com_head;
          com_head = com_ptr;
        }
      } else /* Seen it before */
      {
        fprintf(fu, "%s: Common /%s/ (Already dumped in program %d)\n", s,
                com_name, com_ptr->prog_no);
      }
      break;

    case BTREE:
      fprintf(fu, "%s: BTree\n", s);
      break;

    case SELLIST:
      fprintf(fu, "%s: SelList: \"", s);
      print_string(descr->data.str.saddr);
      break;

    case PMATRIX:
      break;

    case SOCK:
      fprintf(fu, "%s: Socket\n", s);
      break;

    case LOCALVARS:
      ahdr = descr->data.ahdr_addr;
      lsub_level = 0;
      do {
        memset(s, ' ', indent); /* Discard pre-assembled line */
        sprintf(s + indent, "Local variables for LSUB %s", symbol);
        if (lsub_level == 0) {
          strcat(s, " (current invocation)");
        } else {
          sprintf(s + strlen(s), " (previous invocation %d)", lsub_level);
        }

        fprintf(fu, "%s\n", s);

        /* Find start of symbol table entry for this "common block" */

        if ((p = sym_tab) != NULL) {
          while ((p = strchr(p, FIELD_MARK)) != NULL) {
            p++; /* Skip field mark */

            /* Extract common block number */

            n = 0;
            while (IsDigit(*p))
              n = (n * 10) + (*(p++) - '0');

            if (n == var_no)
              break; /* Found this local variable pool */
          }
        }

        num_com_vars = (int16_t)(ahdr->rows);
        for (com_var_no = 1; com_var_no <= num_com_vars; com_var_no++) {
          /* Copy variable name */

          q = com_var_name;
          if (p != NULL) {
            p++; /* Skip VM before name */
            while ((!IsMark(*p)) && (*p != '\0'))
              *(q++) = *(p++);
            *q = '\0';
          } else {
            sprintf(com_var_name, "%d", com_var_no);
          }

          q = strchr(com_var_name, ':');
          if (q++ == NULL)
            q = com_var_name; /* Should never happen */
          dump_variable(Element(ahdr, com_var_no), var_no, q, indent + 2);
        }
        lsub_level++;
      } while ((ahdr = ahdr->next_common) != NULL);
      break;

    case OBJ:
      fprintf(fu, "%s: Object\n", s);
      break;

    case PERSISTENT:
      fprintf(fu, "%s: Persistent variables\n", s);

      /* Find start of symbol table entry for this "common block" */

      if ((p = sym_tab) != NULL) {
        while ((p = strchr(p, FIELD_MARK)) != NULL) {
          p++; /* Skip field mark */

          /* Extract "common block" number */

          n = 0;
          while (IsDigit(*p))
            n = (n * 10) + (*(p++) - '0');

          if (n == var_no)
            break; /* Found it */
        }
      }

      num_com_vars = (int16_t)(descr->data.ahdr_addr->rows);
      for (com_var_no = 1; com_var_no <= num_com_vars; com_var_no++) {
        /* Copy variable name */

        q = com_var_name;
        if (p != NULL) {
          p++; /* Skip VM before name */
          while ((!IsMark(*p)) && (*p != '\0'))
            *(q++) = *(p++);
          *q = '\0';
        } else {
          sprintf(com_var_name, "%d", com_var_no);
        }

        dump_variable(Element(descr->data.ahdr_addr, com_var_no), var_no,
                      com_var_name, indent + 2);
      }
      break;
  }
}

/* ====================================================================== */

Private void print_string(STRING_CHUNK* str) {
  int n;
  char* p;
  char* q;
  u_char c;
  int total_bytes = 0;

  while ((str != NULL) && (total_bytes < MAX_DUMPED_STRING_LEN)) {
    p = str->data;
    n = str->bytes;
    total_bytes += n;

    q = p;
    while (n--) {
      c = (u_char) * (q++);
      if ((c < 32) || (c >= 251)) {
        if (fprintf(fu, "%.*s\\%02x", (int)(q - p - 1), p, c) < 0)
          return;
        p = q;
      } else if (c == '\\') {
        if (fprintf(fu, "%.*s\\\\", (int)(q - p - 1), p) < 0)
          return;
        p = q;
      }
    }

    if (q != p) {
      if (fprintf(fu, "%.*s", (int)(q - p), p) < 0)
        return;
    }

    str = str->next;
  }
  fprintf(fu, "\"%s\n", (str == NULL) ? "" : "+");
}

/* ====================================================================== */

Private void dump_syscom_var(char* tag, int16_t offset) {
  DESCRIPTOR* descr;

  descr = Element(process.syscom, offset);
  dump_variable(descr, -1, tag, 2);
}

/* END-CODE */
