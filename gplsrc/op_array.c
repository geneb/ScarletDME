/* OP_ARRAY.C
 * Array and common handling opcodes
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
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 *
 * START-HISTORY (OpenQM):
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 19 Jun 06  2.4-5 Create persistent variables (public/private for class
 *                  modules) as initially unassigned.
 * 22 Mar 06  2.3-9 Added object pseudo-common (*VARS).
 * 20 Feb 06  2.3-6 Added delete_common()
 * 20 Feb 06  2.3-6 Added self-deleting common.
 * 20 Feb 06  2.3-6 Generalised array header flags.
 * 09 Jan 06  2.3-4 0444 Common matrices were not initialised to zero.
 * 01 Dec 05  2.2-18 Added op_local().
 * 15 Nov 05  2.2-17 0432 Using one subscript when referencing a two dimensional
 *                   matrix (op_indx1) was working across the top row when it
 *                   should work down the leftmost column.
 * 12 Aug 05  2.2-7 0390 Do not allow Pick matrices to be dimensioned with zero
 *                  size.
 * 25 Oct 04  2.0-7 0270 DIMCOM should initialise elements to zero.
 * 29 Sep 04  2.0-3 Added PMATRIX support.
 * 20 Sep 04  2.0-2 Use message handler.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * Opcodes:
 * op_common         COMMON
 * op_delcom         DELCOM
 * op_dimcom         DIMCOM
 * op_dimlcl         DIMLCL
 * op_indx1          INDX1
 * op_indx2          INDX2
 * op_inmat          INMAT
 * op_inmata         INMATA
 * op_matcopy        MATCOPY
 * op_matfill        MATFILL
 * op_pmatrix        PMATRIX
 *
 * Other externally callable functions:
 * a_alloc           Allocate memory for array descriptors
 * free_array        Free memory used by array descriptors
 *
 * Internal functions:
 * a_dimension       Common path for DIMLCL and DIMCOM
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include "header.h"
#include "syscom.h"

Private bool creating_common; /* Most recent COMMON opcode created block? */
Private bool initialise_zero;

Private void a_dimension(bool pick_style, bool set_zero);

/* ======================================================================
   op_common()  -  Link to common block                                   */

void op_common() {
  /*
    OP.COMMON
    Variable number  (2 bytes)
    Number of variables  (2 bytes) - Excludes zero element for name
    Name length  (1 byte). 0x80 = Leave uninitialised
                           0x40 = spare
                           0x3F = actual name length
    Name
    Address to skip DIMCOMs  (3 bytes)

    Note: The unnamed common is actually named $ in this opcode and gets
    the command processor level number appended to this name.
 */

  int16_t var_no;      /* Local var no of common descriptor */
  DESCRIPTOR* com_descr; /* Common block descriptor */
  int16_t dim;         /* Size of common block */
  int16_t name_len;    /* Bytes in block name */
  u_char creation_flags;
  char block_name[32 + 1];
  ARRAY_HEADER* a_hdr;
  ARRAY_CHUNK* a_chunk;
  DESCRIPTOR* descr;
  STRING_CHUNK* str_hdr;
  int16_t actual_size;
  bool add_block_to_common_chain = TRUE;
  OBJDATA* objdata;
  bool is_persistent_vars; /* Persistent variables for object? */

  var_no = *(pc++);
  var_no |= ((int)*(pc++)) << 8;
  com_descr = process.program.vars + var_no;
  k_release(com_descr);

  dim = *(pc++);
  dim |= ((int)*(pc++)) << 8;

  creation_flags = *(pc++);
  name_len = creation_flags & 0x3F;
  if (name_len > 32)
    k_error(sysmsg(1230));

  memcpy(block_name, pc, name_len);
  block_name[name_len] = '\0';
  pc += name_len;

  creating_common = TRUE;

  if ((name_len == 1) && (*block_name == '$')) {
    name_len = sprintf(block_name, "$%d", (int)cproc_level);
  }

  /* Important - The is_persistent_vars and initialise_zero flags must
    be set correctly for all paths that also set creating_common - see
    op_dimcom()                                                         */

  if ((is_persistent_vars = !strcmp(block_name, "*VARS")) != 0) {
    /* Pseudo-common for persistent variables (PUBLIC/PRIVATE) */
    objdata = process.program.objdata;
    creating_common = ((a_hdr = objdata->obj_vars) == NULL);
    initialise_zero = FALSE;
  } else if (*block_name ==
             '~') /* STRUCT block - like common but not on chain */
  {
    add_block_to_common_chain = FALSE;
    initialise_zero = TRUE;
  } else /* Normal common block */
  {
    initialise_zero = !(creation_flags & 0x80);

    /* Search common chain for this block */

    for (a_hdr = process.named_common; a_hdr != NULL;
         a_hdr = a_hdr->next_common) {
      a_chunk = a_hdr->chunk[0];
      str_hdr = a_chunk->descr[0].data.str.saddr;
      if (strcmp(str_hdr->data, block_name) == 0) {
        creating_common = FALSE;
        break;
      }
    }
  }

  if (creating_common) /* Creating a new block */
  {
    a_hdr = a_alloc((int32_t)dim, 0L, initialise_zero);
    a_hdr->ref_ct = 0;

    if (*block_name == '_')
      a_hdr->flags |= AH_AUTO_DELETE;

    if (!strcmp(block_name, "*VARS")) {
      a_hdr->ref_ct = 1;
      objdata->obj_vars = a_hdr;
    } else {
      if (add_block_to_common_chain) {
        if (!strcmp(block_name, "$SYSCOM")) {
          process.syscom =
              a_hdr; /* Remember $SYSCOM location for LDSYS opcode */
        }

        /* Add new block to head of named common chain */

        a_hdr->next_common = process.named_common;
        process.named_common = a_hdr;
      }

      /* Block name is stored as element zero of common block.  The name is
        null terminated to ease string comparisons above but the null is
        not included in the string length.                                 */

      a_chunk = a_hdr->chunk[0];
      descr = &(a_chunk->descr[0]);
      InitDescr(descr, STRING);
      str_hdr = s_alloc((int32_t)name_len + 1, &actual_size);
      descr->data.str.saddr = str_hdr;
      str_hdr->ref_ct = 1;
      str_hdr->string_len = name_len;
      str_hdr->bytes = name_len;
      strcpy(str_hdr->data, block_name);
    }
    pc += 3;
  } else /* Common block already exists */
  {
    if (a_hdr->rows < dim)
      k_error(sysmsg(1231));

    /* Skip any DIMCOMs as this will already have been done */

    pc = c_base + (*pc | (((int32_t)*(pc + 1)) << 8) | (((int32_t)*(pc + 2)) << 16));
  }

  InitDescr(com_descr, (is_persistent_vars) ? PERSISTENT : COMMON);
  com_descr->data.c_addr = a_hdr;
  a_hdr->ref_ct++;
}

/* ======================================================================
   op_delcom()  -  Delete named common block                              */

void op_delcom() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Common name (null for all) |                             |
     |=============================|=============================|
                    
    STATUS() = 0              = OK
             = ER_BAD_NAME    = Illegal name
             = ER_NOT_FOUND   = No such common found
             = ER_IN_USE      = Common is in use
 */

  DESCRIPTOR* descr;
  char block_name[32 + 1];
  ARRAY_HEADER* a_hdr;
  ARRAY_HEADER* prev;
  ARRAY_CHUNK* a_chunk;
  STRING_CHUNK* str_hdr;

  descr = e_stack - 1;
  if (k_get_c_string(descr, block_name, 32) < 0) {
    process.status = ER_BAD_NAME;
    goto exit_op_delcom;
  }

  if (block_name[0] == '\0') /* Delete all named common */
  {
    prev = NULL;
    for (a_hdr = process.named_common; a_hdr != NULL;
         a_hdr = a_hdr->next_common) {
      if (a_hdr->ref_ct == 0) {
        /* Dechain common block */
        if (prev == NULL)
          process.named_common = a_hdr->next_common;
        else
          prev->next_common = a_hdr->next_common;

        free_array(a_hdr);
        process.status = 0;
      } else
        prev = a_hdr;
    }
    process.status = 0;
  } else /* Delete specific block */
  {
    /* Search common chain for this block */

    prev = NULL;
    process.status = ER_NOT_FOUND;
    for (a_hdr = process.named_common; a_hdr != NULL;
         a_hdr = a_hdr->next_common) {
      a_chunk = a_hdr->chunk[0];
      str_hdr = a_chunk->descr[0].data.str.saddr;
      if (strcmp(str_hdr->data, block_name) == 0) {
        if (a_hdr->ref_ct == 0) {
          /* Dechain common block */
          if (prev == NULL)
            process.named_common = a_hdr->next_common;
          else
            prev->next_common = a_hdr->next_common;

          free_array(a_hdr);
          process.status = 0;
        } else
          process.status = ER_IN_USE;
        break;
      }
      prev = a_hdr;
    }
  }

exit_op_delcom:
  k_dismiss();
}

/* ======================================================================
   op_dimcom()  -  DIMCOM   Set common array dimensions                   */

/* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Number of columns          |                             |
     |  Zero if one dimension      |                             |
     |-----------------------------|-----------------------------| 
     |  Number of rows             |                             |
     |-----------------------------|-----------------------------| 
     |  ADDR of array descr        |                             |
     |=============================|=============================|
                    
 */

void op_dimcom() {
  if (creating_common) {
    a_dimension(FALSE, initialise_zero); /* 0270 */
  } else {
    k_pop(3);
    process.inmat = 0;
  }
}

/* ======================================================================
   op_dimlcl()  -  DIMLCL   Set local array dimensions (Information style)
   op_dimlclp() -  DIMLCLP  Set local array dimensions (Pick style)       */

/* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Number of columns          |                             |
     |  Zero if one dimension      |                             |
     |-----------------------------|-----------------------------| 
     |  Number of rows             |                             |
     |-----------------------------|-----------------------------| 
     |  ADDR of array descr        |                             |
     |=============================|=============================|
                    
 */

void op_dimlcl() {
  a_dimension(FALSE, FALSE);
}

void op_dimlclp() {
  a_dimension(TRUE, FALSE);
}

/* ======================================================================
   op_indx1()  -  Resolve one dimension array index                       */

void op_indx1() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Index                      | ADDR of array element       |
     |-----------------------------|-----------------------------| 
     |  ADDR of array descr        |                             |
     |=============================|=============================|
                    
 */

  DESCRIPTOR* descr;
  DESCRIPTOR* com_descr;
  int32_t indx;
  ARRAY_HEADER* a_hdr;
  ARRAY_CHUNK* a_chunk;
  PMATRIX_HEADER* pm_hdr;

  /* Get index */

  descr = e_stack - 1;
  GetInt(descr);
  indx = descr->data.value;

  /* Find array */

  descr = e_stack - 2;
  do {
    descr = descr->data.d_addr;
  } while (descr->type == ADDR);

  switch (descr->type) {
    case ARRAY:
      a_hdr = descr->data.ahdr_addr;
      if ((indx < 0) || (indx > a_hdr->rows) ||
          (indx == 0 && (a_hdr->flags & AH_PICK_STYLE)))
        k_index_error(descr); /* 0207 */

      if (a_hdr->cols && (indx != 0)) /* 0432 It's a two dimensional matrix */
      {
        indx = (indx - 1) * a_hdr->cols + 1;
      }
      break;

    case PMATRIX:
      pm_hdr = descr->data.pmatrix;
      if ((indx < 1) || (indx > pm_hdr->rows))
        k_index_error(descr);
      if (pm_hdr->cols && (indx != 0)) /* 0432 It's a two dimensional matrix */
      {
        indx = (indx - 1) * pm_hdr->cols + 1;
      }
      com_descr = pm_hdr->com_descr;
      indx += pm_hdr->base_offset - 1;
      a_hdr = com_descr->data.ahdr_addr;
      break;

    default:
      k_not_array_error(descr);
  }

  k_pop(2);

  a_chunk = a_hdr->chunk[indx / MAX_ARRAY_CHUNK_SIZE];
  InitDescr(e_stack, ADDR);
  (e_stack++)->data.d_addr = a_chunk->descr + (indx % MAX_ARRAY_CHUNK_SIZE);
}

/* ======================================================================
   op_indx2()  -  Resolve two dimension array index                       */

void op_indx2() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Column index               | ADDR of array element       |
     |-----------------------------|-----------------------------| 
     |  Row index                  |                             |
     |-----------------------------|-----------------------------| 
     |  ADDR of array descr        |                             |
     |=============================|=============================|
                    
 */

  DESCRIPTOR* descr;
  DESCRIPTOR* com_descr;
  int32_t row;
  int32_t col;
  int32_t cols;
  int32_t indx;
  ARRAY_HEADER* a_hdr;
  ARRAY_CHUNK* a_chunk;
  PMATRIX_HEADER* pm_hdr;

  /* Get column index */

  descr = e_stack - 1;
  GetInt(descr);
  col = descr->data.value;

  /* Get row index */

  descr = e_stack - 2;
  GetInt(descr);
  row = descr->data.value;

  /* Find array */

  descr = e_stack - 3;
  while (descr->type == ADDR)
    descr = descr->data.d_addr;

  switch (descr->type) {
    case ARRAY:
      a_hdr = descr->data.ahdr_addr;
      cols = a_hdr->cols; /* 0207 */
      if (cols == 0)
        cols = 1; /* 0207 Treat as two dim */

      if ((row == 0) || (col == 0)) {
        if (a_hdr->flags & AH_PICK_STYLE)
          k_index_error(descr);
        indx = 0;
      } else {
        if ((row <= 0) || (row > a_hdr->rows) /* 0207 */
            || (col <= 0) || (col > cols))    /* 0207 */
        {
          k_index_error(descr);
        }

        indx = ((row - 1) * cols) + col;
      }
      break;

    case PMATRIX:
      pm_hdr = descr->data.pmatrix;
      cols = pm_hdr->cols;
      if (cols == 0)
        cols = 1;

      if ((row <= 0) || (row > pm_hdr->rows) || (col <= 0) || (col > cols)) {
        k_index_error(descr);
      }

      indx = pm_hdr->base_offset + ((row - 1) * cols) + col - 1;

      com_descr = pm_hdr->com_descr;
      a_hdr = com_descr->data.ahdr_addr;
      break;

    default:
      k_not_array_error(descr);
  }

  a_chunk = a_hdr->chunk[indx / MAX_ARRAY_CHUNK_SIZE];

  k_dismiss();
  k_pop(2);

  InitDescr(e_stack, ADDR);
  (e_stack++)->data.d_addr = a_chunk->descr + (indx % MAX_ARRAY_CHUNK_SIZE);
}

/* ======================================================================
   op_inmat()  -  Return inmat value                                      */

void op_inmat() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |                             | INMAT value                 |
     |=============================|=============================|
                    
 */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = process.inmat;
}

/* ======================================================================
   op_inmata()  -  Return dimensions of array                             */

void op_inmata() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  ADDR of array descr        | INMAT string                |
     |=============================|=============================|
                    
 */

  DESCRIPTOR* descr;
  ARRAY_HEADER* a_hdr;
  STRING_CHUNK* s_hdr;
  PMATRIX_HEADER* pm_hdr;
  int16_t s_len;
  char s[30];
  int16_t actual_size;
  int32_t rows;
  int32_t cols;

  descr = e_stack - 1;
  while (descr->type == ADDR)
    descr = descr->data.d_addr;

  switch (descr->type) {
    case ARRAY:
      a_hdr = descr->data.ahdr_addr;
      rows = a_hdr->rows;
      cols = a_hdr->cols;
      break;

    case PMATRIX:
      pm_hdr = descr->data.pmatrix;
      rows = pm_hdr->rows;
      cols = pm_hdr->cols;
      break;

    default:
      k_not_array_error(descr);
  }

  k_dismiss(); /* Safe as always ADDR on stack */

  if (cols == 0) /* Single dimension */
  {
    InitDescr(e_stack, INTEGER);
    (e_stack++)->data.value = rows;
  } else /* Two dimension */
  {
    s_len = sprintf(s, "%d%c%d", rows, (char)VALUE_MARK, cols);

    s_hdr = s_alloc((int32_t)s_len, &actual_size);

    InitDescr(e_stack, STRING);
    (e_stack++)->data.str.saddr = s_hdr;

    s_hdr->string_len = s_len;
    s_hdr->bytes = s_len;
    s_hdr->ref_ct = 1;
    memcpy(s_hdr->data, s, s_len);
  }
}

/* ======================================================================
   op_listcom()  -  List common blocks                                    */

void op_listcom() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |                             | List of common blocks       |
     |=============================|=============================|
                    
 */

  ARRAY_HEADER* a_hdr;
  ARRAY_CHUNK* a_chunk;
  STRING_CHUNK* str_hdr;
  STRING_CHUNK* result = NULL;

  ts_init(&result, 128);

  for (a_hdr = process.named_common; a_hdr != NULL;
       a_hdr = a_hdr->next_common) {
    a_chunk = a_hdr->chunk[0];
    str_hdr = a_chunk->descr[0].data.str.saddr;
    if (result != NULL)
      ts_copy_byte(FIELD_MARK);
    ts_copy_c_string(str_hdr->data);
  }

  (void)ts_terminate();
  InitDescr(e_stack, STRING);
  (e_stack++)->data.str.saddr = result;
}

/* ======================================================================
   op_local()  -  Create local variable pool                              */

void op_local() {
  /*
    OP.LOCAL
    Reference variable number  (2 bytes)
    Number of local variables  (2 bytes)
    Number of args (1 byte)
    Number of matrices (2 bytes)
       Variable number (2 bytes)   } Repeated for
       Rows (2 bytes)              } each matrix to
       Cols (2 bytes)              } be dimensioned
 */

  int16_t var_no;     /* Var no of LOCALVARS descriptor */
  DESCRIPTOR* lv_descr; /* Common block descriptor */
  int16_t lv_count;   /* Number of local variables */
  int16_t arg_ct;     /* Argument count */
  int16_t mat_ct;     /* Matrix count */
  int16_t mat_vno;    /* Matrix local variable number */
  int16_t mat_rows;   /* Matrix rows */
  int16_t mat_cols;   /* Matrix columns */
  DESCRIPTOR* p;
  DESCRIPTOR* q;
  int16_t i;
  ARRAY_HEADER* a_hdr;

  /* Get reference variable number */

  var_no = *(pc++);
  var_no |= ((int)*(pc++)) << 8;
  lv_descr = process.program.vars + var_no;

  /* Get total number of local variables are create descriptors */

  lv_count = *(pc++);
  lv_count |= ((int)*(pc++)) << 8;

  a_hdr = a_alloc((int32_t)lv_count, 0L, FALSE);
  a_hdr->ref_ct = 1;

  /* If this is a recursive call, stack the previous incarnation of
    this local variable pool.                                      */

  if (lv_descr->type == LOCALVARS) {
    a_hdr->next_common = lv_descr->data.lv_addr;
  } else /* Must currently be unassigned */
  {
    InitDescr(lv_descr, LOCALVARS);
    a_hdr->next_common = NULL;
  }
  lv_descr->data.lv_addr = a_hdr;

  /* Transfer argument variables from e_stack */

  arg_ct = *(pc++);
  if (arg_ct) {
    for (i = 1, p = e_stack - arg_ct; i <= arg_ct; i++) {
      q = Element(a_hdr, i);
      *q = *(p++);
      q->flags |= DF_ARG;
    }
    e_stack -= arg_ct; /* Remove from e-stack. Do not release as copied */
  }

  /* Dimension any matrices */

  mat_ct = *(pc++);
  mat_ct |= ((int)*(pc++)) << 8;

  while (mat_ct--) {
    mat_vno = *(pc++);
    mat_vno |= ((int)*(pc++)) << 8;

    mat_rows = *(pc++);
    mat_rows |= ((int)*(pc++)) << 8;

    mat_cols = *(pc++);
    mat_cols |= ((int)*(pc++)) << 8;

    q = Element(a_hdr, mat_vno);
    InitDescr(q, ARRAY);
    q->data.ahdr_addr = a_alloc((int32_t)mat_rows, (int32_t)mat_cols, FALSE);
    a_hdr->ref_ct = 1;
  }
}

/* ======================================================================
   op_matcopy()  -  Copy one array to another                             */

void op_matcopy() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  ADDR of source array       |                             |
     |-----------------------------|-----------------------------|
     |  ADDR of target array       |                             |
     |=============================|=============================|
                    
 */

  PMATRIX_HEADER* pm_hdr;
  DESCRIPTOR* com_descr; /* Common descriptor for PMATRIX */

  DESCRIPTOR* src_descr;   /* Source array or element descriptor */
  ARRAY_HEADER* src_a_hdr; /* Source array header */
  int32_t src_elements;
  int src_indx;

  DESCRIPTOR* tgt_descr;   /* Target array or element descriptor */
  ARRAY_HEADER* tgt_a_hdr; /* Target array header */
  int32_t tgt_elements;
  int tgt_indx;

  int32_t elements; /* No of elements to copy */

  src_descr = e_stack - 1;
  while (src_descr->type == ADDR)
    src_descr = src_descr->data.d_addr;
  switch (src_descr->type) {
    case ARRAY:
      src_a_hdr = src_descr->data.ahdr_addr;
      src_elements = src_a_hdr->used_elements;
      src_indx = 0;
      break;

    case PMATRIX:
      pm_hdr = src_descr->data.pmatrix;
      com_descr = pm_hdr->com_descr;
      if (pm_hdr->cols == 0)
        src_elements = pm_hdr->rows;
      else
        src_elements = pm_hdr->cols * pm_hdr->rows;
      src_indx = pm_hdr->base_offset;
      src_a_hdr = com_descr->data.ahdr_addr;
      break;

    default:
      k_not_array_error(src_descr);
  }

  tgt_descr = e_stack - 2;
  while (tgt_descr->type == ADDR)
    tgt_descr = tgt_descr->data.d_addr;
  switch (tgt_descr->type) {
    case ARRAY:
      tgt_a_hdr = tgt_descr->data.ahdr_addr;
      tgt_elements = tgt_a_hdr->used_elements;
      tgt_indx = 0;

      /* If the source is a PMATRIX rather than an ARRAY, advance the
         target pointer to forget about the zero element.             */
      if (src_descr->type == PMATRIX) {
        tgt_indx++;
        tgt_elements--;
      }
      break;

    case PMATRIX:
      pm_hdr = tgt_descr->data.pmatrix;
      com_descr = pm_hdr->com_descr;
      if (pm_hdr->cols == 0)
        tgt_elements = pm_hdr->rows;
      else
        tgt_elements = pm_hdr->cols * pm_hdr->rows;
      tgt_indx = pm_hdr->base_offset;
      tgt_a_hdr = com_descr->data.ahdr_addr;

      /* If the source is an ARRAY rather than a PMATRIX, advance the
         source pointer to forget about the zero element.             */
      if (src_descr->type == ARRAY) {
        src_indx++;
        src_elements--;
      }
      break;

    default:
      k_not_array_error(tgt_descr);
  }

  k_pop(2); /* Both are ADDRs */

  elements = min(src_elements, tgt_elements);
  while (elements--) {
    src_descr = Element(src_a_hdr, src_indx);
    tgt_descr = Element(tgt_a_hdr, tgt_indx);

    Release(tgt_descr);
    *tgt_descr = *src_descr;
    tgt_descr->flags &= ~DF_CHANGE;
    IncrRefCt(tgt_descr);

    src_indx++;
    tgt_indx++;
  }
}

/* ======================================================================
   op_matfill()  -  Copy a value to all elements of an array             */

void op_matfill() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Source value               |                             |
     |-----------------------------|-----------------------------|
     |  ADDR of target array       |                             |
     |=============================|=============================|
                    
 */

  DESCRIPTOR* src_descr; /* Source descriptor */
  DESCRIPTOR* tgt_descr; /* Target array descriptor */
  DESCRIPTOR* com_descr; /* Common descriptor for PMATRIX */
  ARRAY_HEADER* a_hdr;   /* Target array header */
  ARRAY_CHUNK* a_chunk;  /* Current chunk pointer */
  PMATRIX_HEADER* pm_hdr;
  int16_t chunk;         /* Current chunk number */
  int32_t elements;       /* No of elements to set */
  int16_t start_element; /* First element to set in this chunk */
  DESCRIPTOR* p;           /* Ptr to current element to set */
  int16_t n;

  src_descr = e_stack - 1;
  k_get_value(src_descr);

  tgt_descr = e_stack - 2;
  while (tgt_descr->type == ADDR)
    tgt_descr = tgt_descr->data.d_addr;

  switch (tgt_descr->type) {
    case ARRAY:
      a_hdr = tgt_descr->data.ahdr_addr;

      if (!(a_hdr->flags & AH_PICK_STYLE)) {
        /* Zero element is set to a null string */

        p = &(a_hdr->chunk[0]->descr[0]);
        Release(p);
        InitDescr(p, STRING);
        p->data.str.saddr = NULL;
      }

      /* Remaining elements are copies of the source item */

      elements = a_hdr->used_elements - 1;

      for (chunk = 0, start_element = 1; elements > 0;
           chunk++, start_element = 0) {
        a_chunk = a_hdr->chunk[chunk];
        p = &(a_chunk->descr[start_element]);
        n = (int16_t)min(a_chunk->num_descr - start_element, elements);
        elements -= n;
        while (n--) {
          Release(p);
          *p = *src_descr;
          p->flags &= ~DF_CHANGE;
          IncrRefCt(p);
          p++;
        }
      }
      break;

    case PMATRIX:
      pm_hdr = tgt_descr->data.pmatrix;
      com_descr = pm_hdr->com_descr;
      if (pm_hdr->cols == 0)
        elements = pm_hdr->rows;
      else
        elements = pm_hdr->cols * pm_hdr->rows;
      n = pm_hdr->base_offset;

      a_hdr = com_descr->data.ahdr_addr;
      while (elements--) {
        p = Element(a_hdr, n);
        Release(p);

        *p = *src_descr;
        p->flags &= ~DF_CHANGE;
        IncrRefCt(p);
        n++;
      }
      break;

    default:
      k_not_array_error(tgt_descr);
  }

  k_dismiss(); /* Source item value */
  k_pop(1);    /* ADDR to target */
}

/* ======================================================================
   op_dellcl()  -  Delete local variable pool                             */

void op_dellcl() {
  /*
    OP.DELLCL
    Reference variable number  (2 bytes)
 */

  int16_t var_no;     /* Var no of LOCALVARS descriptor */
  DESCRIPTOR* lv_descr; /* Common block descriptor */
  ARRAY_HEADER* a_hdr;

  /* Get reference variable number */

  var_no = *(pc++);
  var_no |= ((int)*(pc++)) << 8;
  lv_descr = process.program.vars + var_no;

  a_hdr = lv_descr->data.lv_addr;
  if ((lv_descr->data.lv_addr = a_hdr->next_common) == NULL) {
    InitDescr(lv_descr, UNASSIGNED);
  }

  free_array(a_hdr);
}

/* ======================================================================
   free_common()  -  Free a common area chain                             */

void free_common(ARRAY_HEADER* addr) /* Head of chain to free */
{
  ARRAY_HEADER* next_addr;

  while (addr != NULL) {
    next_addr = addr->next_common;
    free_array(addr);
    addr = next_addr;
  }
}

/* ======================================================================
   free_array()  -  Free memory used by array descriptors                 */

void free_array(ARRAY_HEADER* a_hdr) {
  ARRAY_CHUNK* chunk;
  int16_t i;
  int16_t j;
  DESCRIPTOR* d;

  for (i = 0; i < a_hdr->num_chunks; i++) {
    chunk = a_hdr->chunk[i];
    if (chunk != NULL) /* Was allocated successfully */
    {
      for (j = 0; j < chunk->num_descr; j++) {
        d = &(chunk->descr[j]);
        if (d->flags & DF_WATCH)
          watch_descr = NULL;
        k_release(d);
      }
      k_free((void*)chunk);
    }
  }

  k_free(a_hdr);
}

/* ======================================================================
   a_alloc()  -  Allocate memory for array descriptors                    */

ARRAY_HEADER* a_alloc(
    int32_t rows,
    int32_t cols,
    bool init_zero) /* TRUE: initialise to zero, FALSE leave as UNASSIGNED */
{
  int32_t elements;
  int16_t num_chunks;
  int32_t n;
  u_int16_t mem_reqd;
  ARRAY_HEADER* hdr;
  ARRAY_CHUNK* chunk;
  DESCRIPTOR* p;
  ARRAY_CHUNK** q;
  int16_t i;
  bool allocation_error = FALSE;

  if (cols == 0)
    elements = rows + 1; /* One dimension */
  else
    elements = (rows * cols) + 1; /* Two dimensions */

  num_chunks =
      (int16_t)((elements + MAX_ARRAY_CHUNK_SIZE - 1) / MAX_ARRAY_CHUNK_SIZE);

  /* Allocate array header */

  mem_reqd =
      sizeof(struct ARRAY_HEADER) + ((num_chunks - 1) * sizeof(ARRAY_CHUNK*));
  hdr = (ARRAY_HEADER*)k_alloc(3, mem_reqd);
  if (hdr == NULL)
    goto exit_a_alloc;

  hdr->ref_ct = 1;
  hdr->flags = 0;
  hdr->rows = rows;
  hdr->cols = cols;
  hdr->alloc_elements = elements;
  hdr->used_elements = elements;
  hdr->num_chunks = num_chunks;

  /* Allocate array chunks */

  q = &(hdr->chunk[0]);

  while (elements > 0) {
    n = min(elements, MAX_ARRAY_CHUNK_SIZE);

    if (!allocation_error) {
      mem_reqd = (u_int16_t)(sizeof(struct ARRAY_CHUNK) +
                                      ((n - 1) * sizeof(struct DESCRIPTOR)));
      chunk = (ARRAY_CHUNK*)k_alloc(23, mem_reqd);
    }

    if (chunk == NULL) {
      allocation_error = TRUE; /* Do not allocate further chunks */
    } else {
      chunk->num_descr = (int16_t)n;
      for (i = 0, p = &(chunk->descr[0]); i < n; i++, p++) {
        if (init_zero) {
          InitDescr(p, INTEGER);
          p->data.value = 0;
        } else {
          InitDescr(p, UNASSIGNED);
        }
      }
    }

    *(q++) = chunk;
    elements -= n;
  }

  if (allocation_error) /* Release any successfully allocated chunks */
  {
    free_array(hdr);
    hdr = NULL;
    goto exit_a_alloc;
  }

exit_a_alloc:
  return hdr;
}

/* ======================================================================
   a_dimension()  -  Common path for DIMLCL & DIMCOM                      */

Private void a_dimension(bool pick_style, bool set_zero) {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Number of columns          |                             |
     |  Zero if one dimension      |                             |
     |-----------------------------|-----------------------------| 
     |  Number of rows             |                             |
     |-----------------------------|-----------------------------| 
     |  ADDR of array descr        |                             |
     |=============================|=============================|
                    
 */

  DESCRIPTOR* descr;
  int32_t rows;
  int32_t cols;
  int32_t elements; /* Elements required in array */
  int32_t diff;     /* Difference from current allocation */
  int32_t slack;    /* Spare elements in last chunk */
  int mem_reqd;
  int old_size;
  ARRAY_HEADER* new_ahdr;
  ARRAY_HEADER* a_hdr;
  ARRAY_CHUNK* a_chnk;
  ARRAY_CHUNK* new_chunk;
  int32_t n_elements;
  int16_t i;
  int16_t j;
  int16_t n;
  DESCRIPTOR* d;
  u_char creation_type;

  process.inmat = 1;

  creation_type = (set_zero) ? INTEGER : UNASSIGNED; /* 0270 */

  /* Get number of columns, zero if one dimension */

  descr = e_stack - 1;
  GetInt(descr);
  cols = descr->data.value;
  if (cols < 0)
    cols = 0;

  /* Get number of rows */

  descr = e_stack - 2;
  GetInt(descr);
  rows = descr->data.value;
  if (rows < 0)
    rows = 0;

  k_pop(2);

  /* Find array descriptor */

  descr = e_stack - 1;
  while (descr->type == ADDR)
    descr = descr->data.d_addr;
  k_dismiss();

  switch (descr->type) {
    case ARRAY: /* Redimensioning existing array */
      a_hdr = descr->data.ahdr_addr;
      if ((cols != a_hdr->cols) || (rows != a_hdr->rows)) {
        if (cols == 0)
          elements = rows + 1; /* One dimension */
        else
          elements = (rows * cols) + 1; /* Two dimensions */

        if ((a_hdr->flags & AH_PICK_STYLE) &&
            elements == 1) /* 0390 Only a pseudo zero element */
        {
          k_error(sysmsg(
              1139)); /* Pick style matrices must have at least one element */
        }

        diff = elements - a_hdr->used_elements;
        if (diff < 0) /* Redimensioning smaller */
        {
          /* Find chunk holding last descriptor that we need to keep */

          for (i = 0, n_elements = elements;
               n_elements > (a_chnk = a_hdr->chunk[i])->num_descr; i++) {
            n_elements -= a_chnk->num_descr;
          }

          /* Release current values of any descriptors that we no longer need. */

          while (n_elements < a_chnk->num_descr) {
            d = &(a_chnk->descr[n_elements++]);
            if (d->flags & DF_WATCH)
              watch_descr = NULL;
            k_release(d);
          }

          /* Release all later chunks. Last required chunk is in i. */

          while (++i < a_hdr->num_chunks) {
            a_chnk = a_hdr->chunk[i];
            if (a_chnk != NULL) /* Has not already been released */
            {
              a_hdr->alloc_elements -= a_chnk->num_descr;
              for (j = 0; j < a_chnk->num_descr; j++) {
                d = &(a_chnk->descr[j]);
                if (d->flags & DF_WATCH)
                  watch_descr = NULL;
                k_release(d);
              }
              k_free((void*)a_chnk);
              a_hdr->chunk[i] = NULL;
            }
          }
        } else if (diff > 0) /* Redimensioning larger */
        {
          /* Calculate the number of chunks that will be required and
             reallocate the array header if necessary.                       */

          n = (int16_t)((elements + MAX_ARRAY_CHUNK_SIZE - 1) /
                          MAX_ARRAY_CHUNK_SIZE);

          if (n > a_hdr->num_chunks) {
            mem_reqd =
                sizeof(struct ARRAY_HEADER) + ((n - 1) * sizeof(int16_t*));
            new_ahdr = (ARRAY_HEADER*)k_alloc(3, mem_reqd);
            if (new_ahdr == NULL)
              goto exit_a_dimension;

            memcpy((char*)new_ahdr, (char*)a_hdr, mem_reqd);

            k_free(descr->data.ahdr_addr); /* Free old header */

            descr->data.ahdr_addr = new_ahdr;
            a_hdr = new_ahdr;

            /* Set chunk pointers to null to preserve integrity */

            while (a_hdr->num_chunks < n) {
              a_hdr->chunk[a_hdr->num_chunks++] = NULL;
            }
          }

          /* Allocate any spare elements from final chunk. The final chunk
             may not be of full size. In this case, we must expand it before
             we go on to allocate additional chunks.                         */

          slack = a_hdr->alloc_elements - a_hdr->used_elements;

          if ((slack < diff) &&
              (a_hdr->alloc_elements % MAX_ARRAY_CHUNK_SIZE)) {
            /* Last used chunk is not full and we need to expand it */

            j = (int16_t)((a_hdr->alloc_elements - 1) / MAX_ARRAY_CHUNK_SIZE);
            a_chnk = a_hdr->chunk[j];

            /* Calculate the number of elements required in final chunk */

            n = (int16_t)(a_chnk->num_descr + diff - slack);
            if (n > MAX_ARRAY_CHUNK_SIZE)
              n = MAX_ARRAY_CHUNK_SIZE;

            mem_reqd = sizeof(struct ARRAY_CHUNK) +
                       ((n - 1) * sizeof(struct DESCRIPTOR));
            new_chunk = (struct ARRAY_CHUNK*)k_alloc(24, mem_reqd);
            if (new_chunk == NULL)
              k_error(sysmsg(1008));

            /* 0167 Calculate old size rather than using mem_reqd for copy */

            old_size = sizeof(struct ARRAY_CHUNK) +
                       ((a_chnk->num_descr - 1) * sizeof(struct DESCRIPTOR));
            memcpy((char*)new_chunk, (char*)a_chnk, old_size);

            k_free((void*)a_chnk);
            a_chnk = new_chunk;
            a_hdr->chunk[j] = new_chunk;

            a_hdr->alloc_elements += n - a_chnk->num_descr;
            slack += n - a_chnk->num_descr;

            while (a_chnk->num_descr < n) {
              a_chnk->descr[a_chnk->num_descr].type = creation_type;
              a_chnk->descr[a_chnk->num_descr].flags = 0;
              a_chnk->descr[a_chnk->num_descr].data.value = 0;
              a_chnk->num_descr++;
            }
          }

          diff -= (slack > diff) ? diff : slack;

          /* If we still need further space, allocate new chunks */

          while (diff) {
            /* Calculate index of next chunk to add. At this point, the
               alloc_elements value will be an exact multiple of the maximum
               chunk size.                                                   */

            j = (int16_t)(a_hdr->alloc_elements / MAX_ARRAY_CHUNK_SIZE);

            n = (int16_t)min(diff, MAX_ARRAY_CHUNK_SIZE);
            mem_reqd = sizeof(struct ARRAY_CHUNK) +
                       ((n - 1) * sizeof(struct DESCRIPTOR));

            new_chunk = (struct ARRAY_CHUNK*)k_alloc(25, mem_reqd);
            if (new_chunk == NULL)
              k_error(sysmsg(1009));

            diff -= n;

            /* Add new chunk to array header */

            a_hdr->chunk[j] = new_chunk;
            a_hdr->alloc_elements += n;

            new_chunk->num_descr = n;
            while (n-- > 0) {
              new_chunk->descr[n].type = creation_type;
              new_chunk->descr[n].flags = 0;
              new_chunk->descr[n].data.value = 0;
            }
          }
        }

        a_hdr->rows = rows;
        a_hdr->cols = cols;
        a_hdr->used_elements = elements;
      }
      break;

    case PMATRIX:
      k_error(sysmsg(1136)); /* Cannot redimension a Pick style matrix */

    default: /* Unassigned or converting some other type */
      if (pick_style && (rows == 0) && (cols == 0)) /* 0390 */
      {
        k_error(sysmsg(
            1139)); /* Pick style matrices must have at least one element */
      }

      if ((a_hdr = a_alloc(rows, cols, set_zero)) != NULL) /* 0444 */
      { /* Only change descriptor after successful allocation */
        k_release(descr);
        descr->data.ahdr_addr = a_hdr;
        descr->type = ARRAY;
        if (pick_style)
          a_hdr->flags |= AH_PICK_STYLE;
      }
      break;
  }

  process.inmat = 0;

exit_a_dimension:
  return;
}

/* ======================================================================
   op_pmatrix()  -  Create a reference to a Pick matrix in common         */

void op_pmatrix() {
  /* The opcode is followed by
    2 byte local var number to use as reference
    2 byte common variable number
    2 byte offset into common for base of matrix
    2 byte row count
    2 byte column count
    All values are low byte first
 */

  DESCRIPTOR* descr;
  PMATRIX_HEADER* pm_hdr;

  /* The local variable to become the PMATRIX descriptor is always an
    uninitialised local variable so we can just initialise it without
    needing to worry about releasing any previous value.               */

  descr = process.program.vars + (*pc + (*(pc + 1) << 8));
  pc += 2;

  pm_hdr = (PMATRIX_HEADER*)k_alloc(85, sizeof(PMATRIX_HEADER));
  InitDescr(descr, PMATRIX);
  descr->data.pmatrix = pm_hdr;

  /* Find corresponding COMMON descriptor */

  /* Get common variable number */

  pm_hdr->com_descr = process.program.vars + (*pc + (*(pc + 1) << 8));
  pc += 2;

  /* Get offset of first matrix element */

  pm_hdr->base_offset = *pc + (*(pc + 1) << 8);
  pc += 2;

  /* Get row count */

  pm_hdr->rows = *pc + (*(pc + 1) << 8);
  pc += 2;

  /* Get offset of column count */

  pm_hdr->cols = *pc + (*(pc + 1) << 8);
  pc += 2;
}

/* ======================================================================
   delete_common()  -  Delete a specific named common block               */

void delete_common(ARRAY_HEADER* ahdr) {
  ARRAY_HEADER* p;
  ARRAY_HEADER* prev;

  prev = NULL;
  for (p = process.named_common; p != NULL; p = p->next_common) {
    if (p == ahdr) {
      /* Dechain common block */

      if (prev == NULL)
        process.named_common = p->next_common;
      else
        prev->next_common = p->next_common;

      free_array(p);
      break;
    }
    prev = p;
  }
}

/* END-CODE */
