/* OP_DEBUG.C
 * Debugger opcode
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
 * 15Jan22 gwb Fixed argument formatting issues (CwE-686)
 * 
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 *
 * START-HISTORY:
 * 15 Aug 07  2.6-0 Reworked remove pointers.
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 19 Jun 07  2.5-7 k_call() now has additional argument for stack adjustment.
 * 15 Dec 06  2.4-17 Pass array header flags in op_dbginf().
 * 06 Apr 06  2.4-1 Added PERSISTENT descriptor type for CLASS module variables.
 * 22 Mar 06  2.3-9 Added object programming interface.
 * 02 Mar 06  2.3-8 Inhibit break key while in debugger.
 * 27 Jan 06  2.3-5 DEBUG.INFO(0) now returns VM after program name to allow
 *                  spaces in pathnames.
 * 01 Dec 05  2.2-18 Added LOCALVARS.
 * 30 Jun 05  2.2-3 Added SOCK data type for sockets.
 * 19 May 05  2.2-0 Make os_error available to debugger.
 * 25 Jan 05  2.1-4 Added remove pointer position to op_dbginf() string data.
 * 20 Sep 04  2.0-2 Use message handler.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * Opcodes:
 *    op_dbgbrk         BREAKPOINT
 *    op_dbginf         DEBUG.INFO()
 *    op_dbgoff         DEBUG.OFF
 *    op_dbgon          DEBUG
 *    op_dbgset         DEBUG.SET
 *    op_dbgwatch       WATCH
 *    op_debug          Debugging hook
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include "debug.h"
#include "header.h"
#include "debugger.h"
#include "tio.h"

#define OP_DEBUG 0xFC

void trace_parent(void);

Private ARRAY_HEADER* ahdr;
Private int array_offset;
Private int lo_index;
Private int hi_index;

Private int16_t breakpoint = BRK_RUN;
Private int16_t breakpoint_count;
Private int32_t breakpoint_call_info;
Private int32_t breakpoint_id;
Private int32_t debug_id;

/* The following token is also in BP DEBUG.H */
#define MAX_BREAKPOINTS 20
Private struct BREAKPOINT_LIST {
  u_int16_t line;
  int32_t id;
} breakpoint_list[MAX_BREAKPOINTS];
Private int16_t num_active_breakpoints = 0;

Private void get_var_info(DESCRIPTOR* var_descr, int16_t com_var);

/* ======================================================================
   op_dbgbrk()  -  Debugger breakpoint control functions                  */

void op_dbgbrk() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Qualifier                  |                             |
     |-----------------------------|-----------------------------|
     |  Breakpoint type            |                             |
     |=============================|=============================|

     Breakpoint types:
        0 BRK_RUN             Free run
        1 BRK_STEP            Step n debug elements
        2 BRK_STEP_LINE       Step n lines
        3 BRK_LINE            Run to line n
        4 BRK_PARENT          Break in parent
        5 BRK_PARENT_PROGRAM  Break in parent program
        6 BRK_TRACE           Return to debugged program in trace mode
        7 BRK_ADD_LINE        Add a new breakpoint table entry
        8 BRK_CLEAR           Clear breakpoint table
        9 BRK_CLR_LINE        Clear a specific breakpoint line
       10 BRK_GOTO_LINE       Position PC at given line number
 */

  DESCRIPTOR* descr;
  OBJECT_HEADER* obj_hdr;
  int32_t line_table_offset;
  int32_t line_table_end;
  int32_t line_table_bytes;
  int16_t bytes;
  int line;
  int32_t line_pc;
  int16_t i;
  int16_t j;
  int n;
  u_char* p;

  /* Get breakpoint type */

  descr = e_stack - 2;
  GetInt(descr);
  breakpoint = (int16_t)(descr->data.value);

  /* Get qualifier */

  descr = e_stack - 1;

  switch (breakpoint) {
    case BRK_RUN:
      /* ++++ */
      break;

    case BRK_STEP:
      GetInt(descr);
      breakpoint_count = (int16_t)(descr->data.value);
      break;

    case BRK_STEP_LINE:
      GetInt(descr);
      breakpoint_count = (int16_t)(descr->data.value);
      break;

    case BRK_LINE:
      GetInt(descr);
      breakpoint_id = debug_id;
      breakpoint_count = (int16_t)(descr->data.value);
      break;

    case BRK_PARENT:
      breakpoint_call_info = (((int32_t)process.call_depth - 1) << 16) |
                             process.program.prev->gosub_depth;
      break;

    case BRK_PARENT_PROGRAM:
      breakpoint_id = debug_id;
      breakpoint_call_info = ((int32_t)process.call_depth - 1) << 16;
      break;

    case BRK_ADD_LINE:
      if (num_active_breakpoints == MAX_BREAKPOINTS)
        k_error(sysmsg(1300));
      GetInt(descr);
      breakpoint_list[num_active_breakpoints].id = debug_id;
      breakpoint_list[num_active_breakpoints].line =
          (u_int16_t)(descr->data.value);
      num_active_breakpoints++;
      break;

    case BRK_CLEAR:
      num_active_breakpoints = 0;
      break;

    case BRK_CLR_LINE:
      GetInt(descr);
      for (i = 0; i < num_active_breakpoints; i++) {
        if ((breakpoint_list[i].id == debug_id) &&
            (breakpoint_list[i].line == descr->data.value)) {
          for (j = i + 1; j < num_active_breakpoints; i++, j++) {
            breakpoint_list[i].id = breakpoint_list[j].id;
            breakpoint_list[i].line = breakpoint_list[j].line;
          }
          num_active_breakpoints--;
          break;
        }
      }
      break;

    case BRK_GOTO_LINE:
      process.status = 0;
      GetInt(descr);
      n = descr->data.value;

      obj_hdr = (OBJECT_HEADER*)find_object(debug_id);
      line_table_offset = obj_hdr->line_tab_offset;
      if (line_table_offset) {
        /* The first entry in the line table is for line 0, the fixed
           stuff on the front of the program.                         */

        line_table_end = obj_hdr->sym_tab_offset;
        if (line_table_end == 0)
          line_table_end = obj_hdr->object_size;
        line_table_bytes = line_table_end - line_table_offset;

        p = ((u_char*)obj_hdr) + line_table_offset;
        line = 0;
        line_pc = 0;
        while (line_table_bytes-- > 0) {
          bytes = (int16_t)(*(p++));
          if (bytes == 255) {
            bytes = (int16_t)(*(p++));
            bytes |= ((int16_t)(*(p++))) << 8;
            line_table_bytes -= 2;
          }

          if (line == n) {
            p = process.program.prev->saved_c_base + line_pc;
            if (*(p++) == OP_DEBUG) {
              line = ((int)(*p) << 8) | *(p + 1); /* Line number */
              process.status = line;              /* Actual line number */
              line_pc += 4;
              process.program.prev->saved_pc_offset = line_pc;
            }
            break;
          }

          line_pc += bytes;
          line += 1;
        }
      }
      break;

    default:
      breakpoint = BRK_RUN;
      k_error("Illegal debugger breakpoint type %d", breakpoint);
  }

  k_dismiss();
  k_pop(1);
}

/* ======================================================================
   op_dbgoff()  -  Turn off debugging for debug program                   */

void op_dbgoff() {
  process.debugging = FALSE;
  breakpoint = BRK_STEP;
  breakpoint_count = 1;
}

/* ======================================================================
   op_dbgon()  -  Enter debugger                                          */

void op_dbgon() {
  process.debugging = TRUE;
  breakpoint = BRK_STEP;
  breakpoint_count = 1;
}

/* ======================================================================
   op_dbginf()  -  Debugger information functions                         */

void op_dbginf() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Qualifier                  |  Information                |
     |-----------------------------|-----------------------------|
     |  Information code           |                             |
     |=============================|=============================|

     Information codes:
        0   Return symbols (Program.name var1 var2 var3...)
        1   Return information about variable
            Qualifier is var number. If common, can include offset << 16.
        2   Return element from last array referenced
        3   Return source pathname
        4   Return system variables
        5   Returns program header flags
 */

  DESCRIPTOR* key_descr;
  DESCRIPTOR* qual_descr;
  int16_t key;
  struct OBJECT_HEADER* obj_hdr;
  char* p;
  char* q;
  int16_t n;
  int32_t sym_tab_offset;
  DESCRIPTOR result;
  DESCRIPTOR* var_descr;

  key_descr = e_stack - 2;
  GetInt(key_descr);
  key = (int16_t)(key_descr->data.value);

  qual_descr = e_stack - 1;

  InitDescr(&result, STRING);
  result.data.str.saddr = NULL;
  ts_init(&(result.data.str.saddr), 128);

  switch (key) {
    case 0: /* Get symbols */
      obj_hdr = (OBJECT_HEADER*)find_object(debug_id);
      ts_copy_c_string(obj_hdr->ext_hdr.prog.program_name);
      ts_copy_byte(VALUE_MARK);

      sym_tab_offset = obj_hdr->sym_tab_offset;
      if (sym_tab_offset) {
        p = ((char*)obj_hdr) + sym_tab_offset;
        n = (int16_t)(obj_hdr->object_size - sym_tab_offset);

        q = (char*)memchr(p, '\0', n);
        if (q != NULL)
          n = q - p;
        ts_copy(p, n);
      }
      break;

    case 1: /* Get variable information */
      obj_hdr = (OBJECT_HEADER*)find_object(debug_id);
      GetInt(qual_descr);
      n = (int16_t)(qual_descr->data.value & 0xFFFFL); /* Variable number */
      if ((n >= 0) && (n <= obj_hdr->no_vars)) {
        var_descr = process.program.prev->vars + n;
        n = (int16_t)((qual_descr->data.value >> 16) &
                        0xFFFFL); /* Common offset */
        get_var_info(var_descr, n);
      }
      break;

    case 2: /* Get information for element of last referenced array */
      GetInt(qual_descr);
      n = (int16_t)(qual_descr->data.value); /* Element offset */
      if ((n >= lo_index) && (n <= hi_index)) {
        var_descr = Element(ahdr, n + array_offset);
        get_var_info(var_descr, 0);
      }
      break;

    case 3: /* Get source pathname */
      obj_hdr = (OBJECT_HEADER*)find_object(debug_id);
      p = (char*)obj_hdr + OBJECT_HEADER_SIZE;
      ts_copy_c_string(p);
      break;

    case 4: /* Return system variables */
      ts_printf("%d%c%d%c%d%c%d%c%d", debug_status, FIELD_MARK,
                debug_inmat, FIELD_MARK, process.program.prev->col1, FIELD_MARK,
                process.program.prev->col2, FIELD_MARK, debug_os_error);
      break;

    case 5:
      InitDescr(&result, INTEGER);
      obj_hdr = (OBJECT_HEADER*)find_object(debug_id);
      result.data.value = obj_hdr->flags;
      goto return_integer;
  }

  ts_terminate();

return_integer:
  k_dismiss();         /* Dismiss qualifier */
  *key_descr = result; /* Replace (integer) key by result */
}

/* ----------------------------------------------------------------------
   Get information about variable                                         */

Private void get_var_info(
    DESCRIPTOR* var_descr,
    int16_t com_var) /* Element (from one) if common block */
{
  int type;
  STRING_CHUNK* str;
  FILE_VAR* fvar;
  struct OBJECT_HEADER* obj_hdr;
  PMATRIX_HEADER* pm_hdr;
  DESCRIPTOR* com_descr;
  int32_t offset;
  STRING_CHUNK* current_chunk;
  int n;

  while (var_descr->type == ADDR)
    var_descr = var_descr->data.d_addr;
  type = var_descr->type;

  if (com_var && (type == COMMON || type == LOCALVARS)) /* Bound common */
  {
    var_descr = Element(var_descr->data.c_addr, com_var);
    type = var_descr->type;
  }

  /* Copy variable type to result string */

  ts_printf("%d", type);
  ts_copy_byte(FIELD_MARK);

  switch (type) /* ++ALLTYPES++ */
  {
    case INTEGER:
      ts_printf("%d", var_descr->data.value);
      break;

    case FLOATNUM:
      ts_printf("%lf", var_descr->data.float_value);
      break;

    case SUBR:
      obj_hdr = (struct OBJECT_HEADER*)(var_descr->data.subr.object);
      ts_copy_c_string(obj_hdr->ext_hdr.prog.program_name);
      break;

    case STRING:
    case SELLIST:
      if (var_descr->data.str.saddr == NULL) {
        ts_copy_byte('0');
      } else {
        /* String length */

        ts_printf("%d", var_descr->data.str.saddr->string_len);

        /* Remove pointer position */

        if (var_descr->flags & DF_REMOVE) {
          offset = var_descr->n1;
          current_chunk = var_descr->data.str.rmv_saddr;
          for (str = var_descr->data.str.saddr; str != NULL; str = str->next) {
            if (str == current_chunk)
              break;
            offset += str->bytes;
          }

          ts_copy_byte(VALUE_MARK);
          ts_printf("%d", offset);
        }

        ts_copy_byte(FIELD_MARK);
        for (str = var_descr->data.str.saddr; str != NULL; str = str->next) {
          ts_copy(str->data, str->bytes);
        }
      }
      break;

    case FILE_REF:
      fvar = var_descr->data.fvar;

      ts_printf("%d", fvar->type);
      ts_copy_byte(FIELD_MARK);

      if (fvar->voc_name != NULL) {
        ts_copy_c_string(fvar->voc_name);
      }
      break;

    case ARRAY:
      ahdr = var_descr->data.ahdr_addr;
      array_offset = 0;
      lo_index = 0;
      hi_index = ahdr->used_elements;
      ts_printf("%d%c%d%c%d", ahdr->rows, FIELD_MARK, ahdr->cols, FIELD_MARK,
                ahdr->flags);
      break;

    case COMMON:
      break;

    case IMAGE:
      break;

    case BTREE:
      break;

    case PMATRIX:
      pm_hdr = var_descr->data.pmatrix;
      com_descr = pm_hdr->com_descr;
      array_offset = pm_hdr->base_offset - 1;
      lo_index = 1;
      n = pm_hdr->cols;
      hi_index = ((n) ? n : 1) * pm_hdr->rows;
      ahdr = com_descr->data.ahdr_addr;
      ts_printf("%d%c%d", pm_hdr->rows, FIELD_MARK, pm_hdr->cols);
      break;

    case SOCK:
      break;

    case LOCALVARS:
      break;

    case OBJ:
      obj_hdr = (struct OBJECT_HEADER*)(var_descr->data.objdata->objprog);
      ts_copy_c_string(obj_hdr->ext_hdr.prog.program_name);
      break;

    case OBJCD: /* Never stored as a variable */
      break;

    case OBJCDX: /* Never stored as a variable */
      break;

    case PERSISTENT:
      break;
  }
}

/* ======================================================================
   op_dbgwatch()  -  Set watch point                                      */

void op_dbgwatch() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Qualifier                  |                             |
     |-----------------------------|-----------------------------|
     |  Variable number            |                             |
     |=============================|=============================|

 */

  DESCRIPTOR* descr;
  int16_t var_no;
  struct OBJECT_HEADER* obj_hdr;
  int32_t vn;
  int32_t qual;
  DESCRIPTOR* var_descr;
  int16_t n;
  ARRAY_HEADER* a_hdr;
  PMATRIX_HEADER* pm_hdr;

  descr = e_stack - 1;
  GetInt(descr);
  qual = descr->data.value;

  descr = e_stack - 2;
  GetInt(descr);
  vn = descr->data.value;
  k_pop(2);

  if (vn == -1) /* Cancel watch */
  {
    watch_descr = NULL;
  } else {
    var_no = (int16_t)(vn & 0xFFFFL);

    obj_hdr = (OBJECT_HEADER*)find_object(debug_id);
    if ((var_no >= 0) && (var_no <= obj_hdr->no_vars)) {
      var_descr = process.program.prev->vars + var_no;
      n = (int16_t)((vn >> 16) & 0xFFFFL); /* Common offset */
      if (n != 0)                            /* Common variable */
      {
        if (var_descr->type == COMMON) {
          var_descr = Element(var_descr->data.c_addr, n);
        } else {
          return;
        }
      }

      switch (var_descr->type) {
        case ARRAY:
          a_hdr = var_descr->data.ahdr_addr;
          if ((qual >= 0) && (qual <= a_hdr->used_elements)) {
            var_descr = Element(a_hdr, qual);
          } else
            k_error(sysmsg(1301));
          break;

        case PMATRIX:
          pm_hdr = var_descr->data.pmatrix;
          n = max(pm_hdr->cols, 1) * pm_hdr->rows;
          if ((qual > 0) && (qual <= n)) {
            var_descr = Element(pm_hdr->com_descr->data.ahdr_addr,
                                qual + pm_hdr->base_offset - 1);
          } else
            k_error(sysmsg(1301));
          break;
      }

      var_descr->flags |= DF_WATCH;
      watch_descr = var_descr;
    }
  }
}

/* ======================================================================
   op_dbgset()  -  Set variable                                           */

void op_dbgset() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Value                      |                             |
     |-----------------------------|-----------------------------|
     |  Qualifier                  |                             |
     |-----------------------------|-----------------------------|
     |  Variable number            |                             |
     |=============================|=============================|

 */

  DESCRIPTOR* descr;
  int16_t var_no;
  struct OBJECT_HEADER* obj_hdr;
  int32_t vn;
  int32_t qual;
  DESCRIPTOR* var_descr;
  int16_t n;
  ARRAY_HEADER* a_hdr;
  PMATRIX_HEADER* pm_hdr;

  descr = e_stack - 2;
  GetInt(descr);
  qual = descr->data.value;

  descr = e_stack - 3;
  GetInt(descr);
  vn = descr->data.value;

  var_no = (int16_t)(vn & 0xFFFFL);

  obj_hdr = (OBJECT_HEADER*)find_object(debug_id);
  if ((var_no >= 0) && (var_no <= obj_hdr->no_vars)) {
    var_descr = process.program.prev->vars + var_no;
    n = (int16_t)((vn >> 16) & 0xFFFFL); /* Common offset */
    if (n != 0)                            /* Common variable */
    {
      if (var_descr->type != COMMON)
        goto exit_dbgset;

      var_descr = Element(var_descr->data.c_addr, n);
    }

    switch (var_descr->type) {
      case ARRAY:
        a_hdr = var_descr->data.ahdr_addr;
        if ((qual >= 0) && (qual <= a_hdr->used_elements)) {
          var_descr = Element(a_hdr, qual);
        } else
          k_error(sysmsg(1302));
        break;

      case PMATRIX:
        pm_hdr = var_descr->data.pmatrix;
        n = max(pm_hdr->cols, 1) * pm_hdr->rows;
        if ((qual > 0) && (qual <= n)) {
          var_descr = Element(pm_hdr->com_descr->data.ahdr_addr,
                              qual + pm_hdr->base_offset - 1);
        } else
          k_error(sysmsg(1302));
        break;
    }

    k_release(var_descr);
    descr = e_stack - 1;
    k_get_value(descr);
    *var_descr = *descr;
  }

exit_dbgset:
  k_pop(3);
}

/* ======================================================================
   op_debug()  -  DEBUG  -  Debugger interface                            */

void op_debug() {
  u_int16_t line;
  int16_t sub_ref;
  int16_t event = 0;
  struct OBJECT_HEADER* obj_hdr;
  int16_t i;

  if (process.debugging) {
    line = ((int)(*pc) << 8) | *(pc + 1); /* Line number */
    sub_ref = *(pc + 2);
    pc += 3;

    /* Check watch descriptor */

    if ((watch_descr != NULL) && ((watch_descr->flags & DF_WATCH) == 0)) {
      watch_descr->flags |= DF_WATCH;
      event |= DE_WATCH;
    }

    /* Scan breakpoint list */

    for (i = 0; i < num_active_breakpoints; i++) {
      if ((line == breakpoint_list[i].line) &&
          (debug_id == breakpoint_list[i].id)) {
        event |= DE_BREAKPOINT;
        break;
      }
    }

    if (event)
      goto break_now;

    switch (breakpoint) {
      case BRK_RUN:
        goto exit_op_debug;

      case BRK_STEP:
        if (--breakpoint_count)
          goto exit_op_debug;
        break;

      case BRK_STEP_LINE:
        if (sub_ref || --breakpoint_count)
          goto exit_op_debug;
        break;

      case BRK_LINE:
        if ((debug_id != breakpoint_id) || (line != breakpoint_count)) {
          goto exit_op_debug;
        }
        break;

      case BRK_PARENT:
      case BRK_PARENT_PROGRAM:
        if (breakpoint_call_info <= ((((int32_t)process.call_depth) << 16) |
                                     process.program.gosub_depth)) {
          goto exit_op_debug;
        }
        break;
    }

  break_now:
    obj_hdr = (struct OBJECT_HEADER*)c_base;
    debug_id = obj_hdr->id; /* Program we are debugging */

    /* Save items from process structure that must not be trampled on by
      the debugger. These will be restored by k_restore_state().        */

    debug_status = process.status;
    debug_os_error = process.os_error;
    debug_inmat = process.inmat;
    debug_suppress_como = tio.suppress_como;
    tio.suppress_como = TRUE;
    debug_capturing = capturing;
    capturing = FALSE;
    debug_hush = tio.hush;
    tio.hush = FALSE;
    debug_prompt_char = tio.prompt_char;
    debug_dsp_line = tio.dsp.line;
    debug_dsp_paginate = (tio.dsp.flags & PU_PAGINATE) != 0;
    tio.dsp.flags &= ~PU_PAGINATE;

    /* Push arguments onto e-stack */

    InitDescr(e_stack, INTEGER); /* 0230 */
    (e_stack++)->data.value = debug_id;

    InitDescr(e_stack, INTEGER);
    (e_stack++)->data.value = obj_hdr->compile_time;

    InitDescr(e_stack, INTEGER);
    (e_stack++)->data.value = line;

    InitDescr(e_stack, INTEGER);
    (e_stack++)->data.value = sub_ref;

    InitDescr(e_stack, INTEGER);
    (e_stack++)->data.value = event;

    /* Execute debugger */

    in_debugger = TRUE;
    k_call((is_phantom || is_QMVbSrvr) ? "$PDBG" : "$DEBUG", 5, NULL, 0);
  } else
    pc += 3;

exit_op_debug:
  return;
}

/* ======================================================================
   check_debug()  -  Look back down program stack for debugable item      */

bool check_debug() {
  struct PROGRAM* prg;

  if (process.program.flags & HDR_DEBUG)
    return TRUE;

  for (prg = process.program.prev; prg != NULL; prg = prg->prev) {
    if (prg->flags & HDR_DEBUG)
      return TRUE;
  }

  return FALSE;
}

/* END-CODE */
