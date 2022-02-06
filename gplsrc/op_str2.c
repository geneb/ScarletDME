/* OP_STR2.C
 * String handling opcodes
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
 * 06Feb20 gwb Initialized a variable in rdi() that was triggering a warning in valigrind.
 *             Reformatted code.
 * 
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 *
 * START-HISTORY:
 * 15 Aug 07  2.6-0 Reworked remove pointers.
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 13 Sep 05  2.2-10 0407 Only add 1 to the string length for the mark character
 *                   if we will be adding the mark when using COMPATIBLE.APPEND
 *                   mode.
 * 15 Jun 05  2.2-1 Added "compatible" style append.
 * 03 May 05  2.1-13 Added case insensitive string support.
 * 20 Sep 04  2.0-2 Use dynamic pcode loader.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 *    op_col1        COL1
 *    op_col2        COL2
 *    op_del         DEL
 *    op_extract     EXTRACT
 *    op_field       FIELD
 *    op_ins         INS        (INS statement)
 *    op_insert      INSERT     (INSERT function)
 *    op_rep         S<f,v,s> = xx
 *    op_replace     REPLACE
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include "header.h"

#define DYN_REPLACE 0
#define DYN_INSERT 1
#define DYN_DELETE 2

Private void ins(bool compatible);
Private void insert(bool compatible);
Private void rep(bool compatible);
Private void replace(bool compatible);
Private void rdi(DESCRIPTOR *src_descr, int32_t field, int32_t value, int32_t subvalue, int16_t mode, DESCRIPTOR *new_descr, DESCRIPTOR *result_descr, bool compatible);

/* ======================================================================
   op_col1()  -  Fetch COL1 value                                         */

void op_col1() {
  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = process.program.col1;
}

/* ======================================================================
   op_col2()  -  Fetch COL2 value                                         */

void op_col2() {
  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = process.program.col2;
}

/* ======================================================================
   op_del()  -  Delete item from a string                                 */

void op_del() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Subvalue number            | Modified string             |
     |-----------------------------|-----------------------------| 
     |  Value number               |                             |
     |-----------------------------|-----------------------------| 
     |  Field number               |                             |
     |-----------------------------|-----------------------------| 
     |  Source string              |                             |
     |=============================|=============================|
 */

  DESCRIPTOR *descr;       /* Position descriptors */
  DESCRIPTOR *src_descr;   /* Source string */
  DESCRIPTOR result_descr; /* Result string */
  int32_t field;
  int32_t value;
  int32_t subvalue;

  descr = e_stack - 1;
  GetInt(descr);
  subvalue = descr->data.value;

  descr = e_stack - 2;
  GetInt(descr);
  value = descr->data.value;

  descr = e_stack - 3;
  GetInt(descr);
  field = descr->data.value;

  src_descr = e_stack - 4;
  k_get_string(src_descr);

  if ((field < 0) || (value < 0) || (subvalue < 0)) {
    /* Leave source string as the result */

    k_dismiss();
    k_dismiss();
    k_dismiss();
  } else {
    rdi(src_descr, field, value, subvalue, DYN_DELETE, NULL, &result_descr, FALSE);

    k_dismiss();
    k_dismiss();
    k_dismiss();
    k_dismiss();

    *(e_stack++) = result_descr;
  }
}

/* ======================================================================
   op_extract()  -  Extract item from a string                            */

void op_extract() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Subvalue number            | Substring                   |
     |-----------------------------|-----------------------------| 
     |  Value number               |                             |
     |-----------------------------|-----------------------------| 
     |  Field number               |                             |
     |-----------------------------|-----------------------------| 
     |  Source string              |                             |
     |=============================|=============================|
 */

  DESCRIPTOR *field_descr;    /* Position descriptor */
  DESCRIPTOR *value_descr;    /* Position descriptor */
  DESCRIPTOR *subvalue_descr; /* Position descriptor */
  DESCRIPTOR *src_descr;      /* Source string */
  DESCRIPTOR result_descr;    /* Result string */
  int32_t field;
  int32_t value;
  int32_t subvalue;
  int16_t offset;
  STRING_CHUNK *str_hdr;
  int16_t bytes_remaining;
  int16_t len;
  char *p;
  char *q;
  bool end_on_value_mark;
  bool end_on_subvalue_mark;
  register char c;

  subvalue_descr = e_stack - 1;
  value_descr = e_stack - 2;
  field_descr = e_stack - 3;

  GetInt(subvalue_descr);
  subvalue = subvalue_descr->data.value;

  GetInt(value_descr);
  value = value_descr->data.value;

  GetInt(field_descr);
  field = field_descr->data.value;

  k_pop(3);

  /* Set up a string descriptor on our C stack to receive the result */

  InitDescr(&result_descr, STRING);
  result_descr.data.str.saddr = NULL;
  ts_init(&result_descr.data.str.saddr, 32);

  if (field <= 0) /* Field zero or negative  -  Return null string */
  {
    goto done;
  } else if (value == 0) /* Extracting field */
  {
    end_on_value_mark = FALSE;
    end_on_subvalue_mark = FALSE;
    value = 1;
    subvalue = 1;
  } else if (subvalue == 0) /* Extracting value */
  {
    end_on_value_mark = TRUE;
    end_on_subvalue_mark = FALSE;
    subvalue = 1;
  } else /* Extracting subvalue */
  {
    end_on_value_mark = TRUE;
    end_on_subvalue_mark = TRUE;
  }

  src_descr = e_stack - 1;
  k_get_string(src_descr);
  str_hdr = src_descr->data.str.saddr;

  if (find_item(str_hdr, field, value, subvalue, &str_hdr, &offset)) {
    if (str_hdr == NULL)
      goto done;

    p = str_hdr->data + offset;
    bytes_remaining = str_hdr->bytes - offset;

    while (1) {
      /* Is there any data left in this chunk? */

      if (bytes_remaining) {
        /* Look for delimiter in the current chunk */

        for (q = p; bytes_remaining-- > 0; q++) {
          c = *q;
          if ((IsDelim(c)) && ((c == FIELD_MARK) || ((c == VALUE_MARK) && end_on_value_mark) || ((c == SUBVALUE_MARK) && end_on_subvalue_mark))) {
            /* Copy up to mark */

            len = q - p;
            if (len > 0)
              ts_copy(p, len);

            goto done;
          }
        }

        /* Copy remainder of this chunk (must be at least one byte) */

        ts_copy(p, q - p);
      }

      if ((str_hdr = str_hdr->next) == NULL)
        break;

      p = str_hdr->data;
      bytes_remaining = str_hdr->bytes;
    }
  }

done:
  ts_terminate();

  k_dismiss(); /* Dismiss source string ADDR */

  *(e_stack++) = result_descr;
}

/* ======================================================================
   op_field()  -  Extract delimited substring from a dynamic array        */

void op_field() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Number of fields           | Substring                   |
     |-----------------------------|-----------------------------| 
     |  Occurrence number          |                             |
     |-----------------------------|-----------------------------| 
     |  Delimiter                  |                             |
     |-----------------------------|-----------------------------| 
     |  Source string              |                             |
     |=============================|=============================|
 */

  DESCRIPTOR *descr; /* Various descriptors */
  int32_t num_fields;
  int32_t occurrence;
  char delimiter;
  STRING_CHUNK *delim_str;
  DESCRIPTOR result_descr; /* Result string */
  STRING_CHUNK *src_str;
  int16_t src_bytes_remaining;
  char *src;
  char *p;
  int16_t len;
  bool nocase;

  nocase = (process.program.flags & HDR_NOCASE) != 0;

  process.program.col1 = 0;
  process.program.col2 = 0;

  /* Get number of fields to return */

  descr = e_stack - 1;
  GetInt(descr);
  num_fields = descr->data.value;
  if (num_fields < 1)
    num_fields = 1;

  /* Get start field position */

  descr = e_stack - 2;
  GetInt(descr);
  occurrence = descr->data.value;
  if (occurrence < 1)
    occurrence = 1;
  k_pop(2);

  /* Get delimiter character */

  descr = e_stack - 1;
  k_get_string(descr);
  if ((delim_str = descr->data.str.saddr) != NULL) {
    delimiter = delim_str->data[0];
    if (nocase)
      delimiter = UpperCase(delimiter);
  }
  k_dismiss();

  /* Get source string */

  descr = e_stack - 1;
  k_get_string(descr);
  src_str = descr->data.str.saddr;
  if (src_str == NULL)
    goto exit_op_field; /* Return source string */

  if (delim_str == NULL) /* Delimiter is null string */
  {
    process.program.col2 = src_str->string_len + 1;
    goto exit_op_field; /* Return source string */
  }

  /* Set up a string descriptor to receive the result */

  InitDescr(&result_descr, STRING);
  result_descr.data.str.saddr = NULL;
  ts_init(&(result_descr.data.str.saddr), 32);

  /* Walk the string looking for the start of this item */

  src = src_str->data;
  src_bytes_remaining = src_str->bytes;

  if (occurrence-- == 1)
    goto found_start;

  do {
    process.program.col1 += src_str->bytes;
    while (src_bytes_remaining) {
      if (nocase) {
        p = (char *)memichr(src, delimiter, src_bytes_remaining);
      } else {
        p = (char *)memchr(src, delimiter, src_bytes_remaining);
      }
      if (p == NULL)
        break; /* No delimiter in this chunk */

      src_bytes_remaining -= (p - src) + 1;
      src = p + 1;

      if (--occurrence == 0) {
        process.program.col1 -= src_bytes_remaining;
        goto found_start;
      }
    }

    if ((src_str = src_str->next) == NULL) {
      process.program.col1 = 0;
      goto return_null_string; /* We did not find the desired field */
    }

    src = src_str->data;
    src_bytes_remaining = src_str->bytes;
  } while (TRUE);

found_start:
  /* We have found the start of the field and its offset is in col1. Now
     copy the specified number of fields to the result string.            */

  do {
    /* Look for delimiter in the current chunk */

    while (src_bytes_remaining) {
      if (nocase) {
        p = (char *)memichr(src, delimiter, src_bytes_remaining);
      } else {
        p = (char *)memchr(src, delimiter, src_bytes_remaining);
      }
      if (p != NULL) /* Found delimiter */
      {
        len = p - src;
        if (len)
          ts_copy(src, len); /* Copy up to delimiter */

        src_bytes_remaining -= len + 1;
        src = p + 1;
        if (--num_fields == 0)
          goto found_end;

        ts_copy_byte(*p); /* Copy delimiter */
      } else              /* No delimiter - copy all to target */
      {
        ts_copy(src, src_bytes_remaining);
        break;
      }
    }

    if ((src_str = src_str->next) == NULL)
      break;

    src = src_str->data;
    src_bytes_remaining = src_str->bytes;
  } while (1);

found_end:
  process.program.col2 = process.program.col1 + ts_terminate() + 1;

return_null_string:
  k_dismiss(); /* Dismiss source string */

  *(e_stack++) = result_descr;

exit_op_field:
  return;
}

/* ======================================================================
   op_ins()  -  Insert item in a string                                   */

void op_ins() {
  ins(FALSE);
}

void op_compins() {
  ins(TRUE);
}

Private void ins(bool compatible) {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Subvalue number            |                             |
     |-----------------------------|-----------------------------| 
     |  Value number               |                             |
     |-----------------------------|-----------------------------| 
     |  Field number               |                             |
     |-----------------------------|-----------------------------| 
     |  ADDR to target string      |                             |
     |-----------------------------|-----------------------------| 
     |  Insertion string           |                             |
     |=============================|=============================|
 */

  DESCRIPTOR *descr;       /* Position descriptors */
  DESCRIPTOR *tgt_descr;   /* Target string */
  DESCRIPTOR *new_descr;   /* Replacement string */
  DESCRIPTOR result_descr; /* Result string */
  int32_t field;
  int32_t value;
  int32_t subvalue;

  descr = e_stack - 1;
  GetInt(descr);
  subvalue = descr->data.value;
  k_pop(1);

  descr = e_stack - 1;
  GetInt(descr);
  value = descr->data.value;
  k_pop(1);

  descr = e_stack - 1;
  GetInt(descr);
  field = descr->data.value;
  k_pop(1);

  /* Find target and ensure that it is a string */

  tgt_descr = e_stack - 1;
  while (tgt_descr->type == ADDR)
    tgt_descr = tgt_descr->data.d_addr;
  k_get_string(tgt_descr);
  k_pop(1);

  new_descr = e_stack - 1;
  k_get_string(new_descr);

  rdi(tgt_descr, field, value, subvalue, DYN_INSERT, new_descr, &result_descr, compatible);

  k_release(tgt_descr);
  *tgt_descr = result_descr;

  k_dismiss();
}

/* ======================================================================
   op_insert()  -  Insert item in a string leaving result on stack        */

void op_insert() {
  insert(FALSE);
}

void op_compinsrt() {
  insert(TRUE);
}

Private void insert(bool compatible) {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Insertion string           | Modified string             |
     |-----------------------------|-----------------------------| 
     |  Subvalue number            |                             |
     |-----------------------------|-----------------------------| 
     |  Value number               |                             |
     |-----------------------------|-----------------------------| 
     |  Field number               |                             |
     |-----------------------------|-----------------------------| 
     |  Source string              |                             |
     |=============================|=============================|
 */

  DESCRIPTOR *descr;       /* Position descriptors */
  DESCRIPTOR *src_descr;   /* Source string */
  DESCRIPTOR *new_descr;   /* Replacement string */
  DESCRIPTOR result_descr; /* Result string */
  int32_t field;
  int32_t value;
  int32_t subvalue;

  new_descr = e_stack - 1;
  k_get_string(new_descr);

  descr = e_stack - 2;
  GetInt(descr);
  subvalue = descr->data.value;

  descr = e_stack - 3;
  GetInt(descr);
  value = descr->data.value;

  descr = e_stack - 4;
  GetInt(descr);
  field = descr->data.value;

  src_descr = e_stack - 5;
  k_get_string(src_descr);

  rdi(src_descr, field, value, subvalue, DYN_INSERT, new_descr, &result_descr, compatible);

  k_dismiss(); /* Insertion string */
  k_pop(3);    /* Subvalue, value, field and ADDR to source */
  k_dismiss(); /* Source string        0147 was k_pop() */

  *(e_stack++) = result_descr;
}

/* ======================================================================
   op_minimum()  -  MINIMUM() function                                    */

void op_minimum() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Dynamic array              | Result                      |
     |=============================|=============================|
 */

  k_recurse(pcode_minimum, 1); /* Execute recursive code */
}

/* ======================================================================
   op_rep()  -  Replace item in a string                                  */

void op_rep() {
  rep(FALSE);
}

void op_comprep() {
  rep(TRUE);
}

Private void rep(bool compatible) {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Replacement string         |                             |
     |-----------------------------|-----------------------------| 
     |  Subvalue number            |                             |
     |-----------------------------|-----------------------------| 
     |  Value number               |                             |
     |-----------------------------|-----------------------------| 
     |  Field number               |                             |
     |-----------------------------|-----------------------------| 
     |  ADDR to string to update   |                             |
     |=============================|=============================|
 */

  DESCRIPTOR *descr; /* Position descriptors */
  int32_t field;
  int32_t value;
  int32_t subvalue;

  DESCRIPTOR *str_descr; /* String to update */
  STRING_CHUNK *str_first_chunk;
  STRING_CHUNK *str_hdr;
  STRING_CHUNK *str_prev;
  int16_t chunk_size;

  STRING_CHUNK *new_str_hdr; /* For copying replaced chunk */

  DESCRIPTOR *rep_descr; /* Replacement string */
  STRING_CHUNK *rep_str_hdr;
  int32_t rep_len; /* Length of inserted data */
  int32_t s_len;   /* New length of result string */

  DESCRIPTOR result_descr; /* Result string */

  bool add_fm;

  rep_descr = e_stack - 1;
  k_get_string(rep_descr);
  rep_str_hdr = rep_descr->data.str.saddr;

  descr = e_stack - 2;
  GetInt(descr);
  subvalue = descr->data.value;

  descr = e_stack - 3;
  GetInt(descr);
  value = descr->data.value;

  descr = e_stack - 4;
  GetInt(descr);
  field = descr->data.value;

  str_descr = e_stack - 5;
  while (str_descr->type == ADDR)
    str_descr = str_descr->data.d_addr;
  k_get_string(str_descr);

  /* Optimised path for the S<-1, 0, 0> case with a reference count of one */

  if ((field < 0) && (value == 0) && (subvalue == 0)) {
    str_hdr = str_descr->data.str.saddr;

    if (str_hdr == NULL) {
      /* Appending a field to a null string.  Make the updated string a
        reference to the replacement string.                            */

      if (rep_str_hdr != NULL)
        rep_str_hdr->ref_ct++;
      str_descr->data.str.saddr = rep_str_hdr;
      goto exit_op_rep;
    }

    /* Examine the string we are to update. If this has a reference count of
      one, we can optimise the action by simply appending to the string
      without copying it.                                                  */

    if (str_hdr->ref_ct == 1) {
      /* Ensure we do not leave a remove pointer active */

      str_descr->flags &= ~DF_REMOVE;

      /* Find the final chunk of the string */

      str_first_chunk = str_hdr;
      str_prev = NULL;
      while (str_hdr->next != NULL) {
        str_prev = str_hdr;
        str_hdr = str_hdr->next;
      }

      /* We need to take into account the program's append mode. For
        compatible style, if the last character of the existing string
        is a field mark (i.e. the string ends with a null field), we
        do not add a further field mark.                                */
      /* 0407 Rearranged code so that mark length is only added if we
        will be adding the mark.                                        */

      add_fm = !compatible || (str_hdr->data[str_hdr->bytes - 1] != FIELD_MARK);
      if (rep_str_hdr == NULL)
        rep_len = add_fm;
      else
        rep_len = rep_str_hdr->string_len + add_fm;

      /* Update the total string length */

      s_len = str_first_chunk->string_len + rep_len;
      str_first_chunk->string_len = s_len;

      if (str_hdr->alloc_size >= s_len) {
        /* The entire result will fit in the allocated size of the target
          string.  Append the new data.                                  */

        if (add_fm)
          str_hdr->data[str_hdr->bytes++] = FIELD_MARK;

        while (rep_str_hdr != NULL) {
          memcpy(str_hdr->data + str_hdr->bytes, rep_str_hdr->data, rep_str_hdr->bytes);
          str_hdr->bytes += rep_str_hdr->bytes;
          rep_str_hdr = rep_str_hdr->next;
        }
        goto exit_op_rep;
      }

      /* If this chunk is less than maximum size, replace it by a new chunk. */

      if (str_hdr->bytes < MAX_STRING_CHUNK_SIZE) {
        /* Replace final chunk */

        rep_len += str_hdr->bytes;
        new_str_hdr = s_alloc(rep_len, &chunk_size);

        /* Copy old data and rechain chunks, freeing old one */

        memcpy(new_str_hdr->data, str_hdr->data, str_hdr->bytes);
        new_str_hdr->bytes = str_hdr->bytes;

        if (str_prev == NULL) /* First chunk */
        {
          new_str_hdr->string_len = str_hdr->string_len;
          new_str_hdr->ref_ct = 1;

          str_descr->data.str.saddr = new_str_hdr;
        } else {
          str_prev->next = new_str_hdr;
        }

        s_free(str_hdr);
        str_hdr = new_str_hdr;
      } else {
        chunk_size = (int16_t)min(rep_len, MAX_STRING_CHUNK_SIZE);
      }

      /* Append field mark and new data */

      if (add_fm) {
        str_hdr = copy_string(str_hdr, &(str_descr->data.str.saddr), FIELD_MARK_STRING, 1, &chunk_size);
      }

      while (rep_str_hdr != NULL) {
        str_hdr = copy_string(str_hdr, &(str_descr->data.str.saddr), rep_str_hdr->data, rep_str_hdr->bytes, &chunk_size);
        rep_str_hdr = rep_str_hdr->next;
      }

      goto exit_op_rep;
    }
  }

  rdi(str_descr, field, value, subvalue, DYN_REPLACE, rep_descr, &result_descr, compatible);
  k_release(str_descr);
  InitDescr(str_descr, STRING);
  str_descr->data.str.saddr = result_descr.data.str.saddr;

exit_op_rep:
  k_dismiss();
  k_pop(4); /* subvalue, value and field positions and ADDR to target */
}

/* ======================================================================
   op_repadd()  -  X<f,v,s> += n                                          */

void op_repadd() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Value to add               |                             |
     |-----------------------------|-----------------------------| 
     |  Subvalue number            |                             |
     |-----------------------------|-----------------------------| 
     |  Value number               |                             |
     |-----------------------------|-----------------------------| 
     |  Field number               |                             |
     |-----------------------------|-----------------------------| 
     |  ADDR to string to update   |                             |
     |=============================|=============================|
 */

  k_recurse(pcode_repadd, 5); /* Execute recursive code */
}
/* ======================================================================
   op_repcat()  -  X<f,v,s> := n                                          */

void op_repcat() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Value to append            |                             |
     |-----------------------------|-----------------------------| 
     |  Subvalue number            |                             |
     |-----------------------------|-----------------------------| 
     |  Value number               |                             |
     |-----------------------------|-----------------------------| 
     |  Field number               |                             |
     |-----------------------------|-----------------------------| 
     |  ADDR to string to update   |                             |
     |=============================|=============================|
 */

  k_recurse(pcode_repcat, 5); /* Execute recursive code */
}

/* ======================================================================
   op_repdiv()  -  X<f,v,s> /= n                                          */

void op_repdiv() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Divisor                    |                             |
     |-----------------------------|-----------------------------| 
     |  Subvalue number            |                             |
     |-----------------------------|-----------------------------| 
     |  Value number               |                             |
     |-----------------------------|-----------------------------| 
     |  Field number               |                             |
     |-----------------------------|-----------------------------| 
     |  ADDR to string to update   |                             |
     |=============================|=============================|
 */

  k_recurse(pcode_repdiv, 5); /* Execute recursive code */
}

/* ======================================================================
   op_replace()  -  Replace item in a string leaving result on e-stack    */

void op_replace() {
  replace(FALSE);
}

void op_compreplc() {
  replace(TRUE);
}

Private void replace(bool compatible) {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Replacement string         | Modified string             |
     |-----------------------------|-----------------------------| 
     |  Subvalue number            |                             |
     |-----------------------------|-----------------------------| 
     |  Value number               |                             |
     |-----------------------------|-----------------------------| 
     |  Field number               |                             |
     |-----------------------------|-----------------------------| 
     |  Source string              |                             |
     |=============================|=============================|
 */

  DESCRIPTOR *descr;       /* Position descriptors */
  DESCRIPTOR *src_descr;   /* Source string */
  DESCRIPTOR *new_descr;   /* Replacement string */
  DESCRIPTOR result_descr; /* Result string */
  int32_t field;
  int32_t value;
  int32_t subvalue;

  new_descr = e_stack - 1;
  k_get_string(new_descr);

  descr = e_stack - 2;
  GetInt(descr);
  subvalue = descr->data.value;

  descr = e_stack - 3;
  GetInt(descr);
  value = descr->data.value;

  descr = e_stack - 4;
  GetInt(descr);
  field = descr->data.value;

  src_descr = e_stack - 5;
  k_get_string(src_descr);

  rdi(src_descr, field, value, subvalue, DYN_REPLACE, new_descr, &result_descr, compatible);

  k_dismiss();
  k_pop(3); /* subvalue, value and field positions */
  k_dismiss();

  *(e_stack++) = result_descr;
}

/* ======================================================================
   op_repmul()  -  X<f,v,s> *= n                                          */

void op_repmul() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Value to multiply          |                             |
     |-----------------------------|-----------------------------| 
     |  Subvalue number            |                             |
     |-----------------------------|-----------------------------| 
     |  Value number               |                             |
     |-----------------------------|-----------------------------| 
     |  Field number               |                             |
     |-----------------------------|-----------------------------| 
     |  ADDR to string to update   |                             |
     |=============================|=============================|
 */

  k_recurse(pcode_repmul, 5); /* Execute recursive code */
}

/* ======================================================================
   op_repsub()  -  X<f,v,s> -= n                                          */

void op_repsub() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Value to subtract          |                             |
     |-----------------------------|-----------------------------| 
     |  Subvalue number            |                             |
     |-----------------------------|-----------------------------| 
     |  Value number               |                             |
     |-----------------------------|-----------------------------| 
     |  Field number               |                             |
     |-----------------------------|-----------------------------| 
     |  ADDR to string to update   |                             |
     |=============================|=============================|
 */

  k_recurse(pcode_repsub, 5); /* Execute recursive code */
}

/* ======================================================================
   op_repsubst()  -  X<f,v,s>[p,q] = x                                    */

void op_repsubst() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Replacement string         |                             |
     |-----------------------------|-----------------------------| 
     |  Substring length           |                             |
     |-----------------------------|-----------------------------| 
     |  Start character position   |                             |
     |-----------------------------|-----------------------------| 
     |  Subvalue number            |                             |
     |-----------------------------|-----------------------------| 
     |  Value number               |                             |
     |-----------------------------|-----------------------------| 
     |  Field number               |                             |
     |-----------------------------|-----------------------------| 
     |  ADDR to string to update   |                             |
     |=============================|=============================|
 */

  k_recurse(pcode_repsubst, 7); /* Execute recursive code */
}

/* ======================================================================
  copy_string()  -  Copy string to string chunk chain                     */

STRING_CHUNK *copy_string(STRING_CHUNK *tail,  /* Last chunk, may be null */
                          STRING_CHUNK **head, /* Ptr to head of string chunk chain */
                          char *src,           /* Ptr to data to append */
                          int16_t len,         /* Length of data to append */
                          int16_t *chunk_size) /* Desired (in), actual (out) chunk size */
{
  STRING_CHUNK *str_hdr; /* Ptr to current string chunk */
  int16_t space;         /* Bytes remaining in current chunk */
  int16_t bytes_to_move;

  if (tail == NULL)
    space = 0;
  else {
    str_hdr = tail;
    space = str_hdr->alloc_size - str_hdr->bytes;
  }

  while (len > 0) {
    /* Allocate new chunk if the current one is full */

    if (space == 0) {
      str_hdr = s_alloc((int32_t)*chunk_size, &space);
      *chunk_size = space * 2;

      if (tail == NULL) /* First chunk */
      {
        *head = str_hdr;
      } else /* Appending subsequent chunk */
      {
        tail->next = str_hdr;
      }
      tail = str_hdr;
    }

    /* Copy what will fit into current chunk */

    bytes_to_move = min(space, len);
    memcpy(str_hdr->data + str_hdr->bytes, src, bytes_to_move);
    src += bytes_to_move;
    len -= bytes_to_move;
    space -= bytes_to_move;
    str_hdr->bytes += bytes_to_move;
  }

  return tail;
}

/* ======================================================================
   find_item()  -  Find start of field, value or subvalue

   Returns TRUE if item found; chunk = ptr to chunk (may be NULL)
           FALSE if not found.                                              */

bool find_item(STRING_CHUNK *str, int32_t field, int32_t value, int32_t subvalue, STRING_CHUNK **chunk, int16_t *offset) {
  int32_t f, v, sv;
  STRING_CHUNK *first_chunk;
  int16_t bytes_remaining;
  char *p;
  char *q;
  register char c;
  int32_t hint_field;
  int32_t hint_offset;
  int32_t new_hint_offset = -1;
  int32_t skip;

  if ((field == 1) && (value == 1) && (subvalue == 1)) { /* <1,0,0> or <1,1,0> or <1,1,1> */
    *chunk = str;
    *offset = 0;

    if (str != NULL) {
      /* Set hint.  This is a bit unfortunate but it simplifies code elsewhere
        as all calls to find_item() that return true must set the hint to
        address the item that was found. Having a hint for the start of the
        string is somewhat unnecessary!                                      */

      first_chunk = str;
      first_chunk->field = 1;
      first_chunk->offset = 0;
    }
    return TRUE;
  }

  if (str == NULL)
    return FALSE;

  /* Walk the string to the desired item */

  first_chunk = str;

  hint_field = str->field;
  if ((hint_field)              /* Hint present... */
      && (hint_field <= field)) /* ...in suitable field */
  {
    /* We have a usable hint to aid location of the desired item */

    hint_offset = str->offset;
    skip = hint_offset;

    while (skip >= str->bytes) {
      skip -= str->bytes;
      if (str->next == NULL) /* Hint points to byte following end of string */
      {
        if ((hint_field == field) /* Right field */
            && (value == 1)       /* Right value */
            && (subvalue == 1))   /* Right subvalue */
        {
          *chunk = str;
          *offset = str->bytes;
          return TRUE;
        }

        return FALSE;
      }
      str = str->next;
    }

    hint_offset -= skip;
    p = str->data + skip;
    bytes_remaining = (int16_t)(str->bytes - skip);
    f = hint_field;
  } else /* No hint available */
  {
    hint_offset = 0;
    p = str->data;
    bytes_remaining = str->bytes;
    f = 1;
  }

  v = 1;
  sv = 1;

  if (f < field) {
    v = 1;
    do {
      while (bytes_remaining) {
        q = (char *)memchr(p, FIELD_MARK, bytes_remaining);
        if (q == NULL)
          break; /* No mark in this chunk */

        bytes_remaining -= (q - p) + 1;
        p = q + 1; /* Position after mark */
        if (++f == field)
          goto field_found;
      }

      if (str->next == NULL)
        return FALSE;

      hint_offset += str->bytes;
      str = str->next;
      p = str->data;
      bytes_remaining = str->bytes;
    } while (1);
  }

field_found:
  new_hint_offset = hint_offset + p - str->data;

  if ((value == v) && (subvalue == 1)) /* At start position */
  {
    goto exit_find_item;
  }

  /* Scan for value / subvalue */

  while (1) {
    while (bytes_remaining-- > 0) {
      c = *(p++);
      if (IsDelim(c)) {
        switch (c) {
          case FIELD_MARK:
            return FALSE; /* No such value or subvalue */

          case VALUE_MARK:
            if (++v > value)
              return FALSE; /* No such subvalue */
            sv = 1;
            break;

          case SUBVALUE_MARK:
            sv++;
            break;
        }

        if ((v == value) && (sv == subvalue)) /* At start position */
        {
          goto exit_find_item;
        }
      }
    }

    /* Advance to next chunk */

    if (str->next == NULL)
      break;

    str = str->next;
    p = str->data;
    bytes_remaining = str->bytes;
  }

  return FALSE;

exit_find_item:
  *chunk = str;
  *offset = p - str->data;

  if (new_hint_offset >= 0) {
    first_chunk->field = field;
    first_chunk->offset = new_hint_offset;
  }

  return TRUE;
}

/* ======================================================================
   rdi()  -  Common path for replace, delete and insert                   */

Private void rdi(DESCRIPTOR *src_descr, /* Source string */
                 int32_t field,
                 int32_t value,
                 int32_t subvalue,
                 int16_t mode,
                 DESCRIPTOR *new_descr,    /* Replacement / insertion string */
                 DESCRIPTOR *result_descr, /* Resultant string */
                 bool compatible)          /* Append style ($MODE COMPATIBLE.APPEND) */
{
  int32_t f, v, sv;
  STRING_CHUNK *str_hdr;
  int16_t bytes_remaining;
  STRING_CHUNK *new_str;
  int16_t len;
  char *p;
  bool done;
  char c;
  char mark;
  bool mark_skipped = FALSE;
  bool end_on_value_mark;
  bool end_on_subvalue_mark;
  bool item_found = FALSE; /* initialized to a default value to clear a valgrind warning - 06feb22 -gwb */
  char last_char = '\0';

  str_hdr = src_descr->data.str.saddr;

  /* Set up a string descriptor to receive the result */

  InitDescr(result_descr, STRING);
  result_descr->data.str.saddr = NULL;
  ts_init(&(result_descr->data.str.saddr), (str_hdr == NULL) ? 64 : str_hdr->string_len);

  if (field == 0)
    field = 1;

  if (value == 0) /* <n, 0, 0> */
  {
    end_on_value_mark = FALSE;
    end_on_subvalue_mark = FALSE;
    value = 1;
    subvalue = 1;
    mark = FIELD_MARK;
  } else if (subvalue == 0) /* <n, n, 0> */
  {
    end_on_value_mark = TRUE;
    end_on_subvalue_mark = FALSE;
    subvalue = 1;
    mark = VALUE_MARK;
  } else /* <n, n, n> */
  {
    end_on_value_mark = TRUE;
    end_on_subvalue_mark = TRUE;
    mark = SUBVALUE_MARK;
  }

  f = 1;
  v = 1;
  sv = 1;              /* Current position */
  bytes_remaining = 0; /* For null string path */

  /* Copy to start of selected item */

  if ((field == 1) && (value == 1) && (subvalue == 1)) { /* <1,0,0> or <1,1,0> or <1,1,1> */
    if (str_hdr != NULL) {
      p = str_hdr->data;
      bytes_remaining = str_hdr->bytes;
    }
    item_found = TRUE;
    goto found;
  }

  /* Walk the string to the desired item */

  if (str_hdr != NULL) {
    do {
      p = str_hdr->data;
      bytes_remaining = str_hdr->bytes;

      do {
        c = *p;
        if (IsDelim(c)) {
          switch (c) {
            case FIELD_MARK:
              if (f == field)
                goto not_found; /* No such value or subvalue */
              f++;
              v = 1;
              sv = 1;
              break;

            case VALUE_MARK:
              if ((f == field) && (v == value)) {
                goto not_found; /* No such subvalue */
              }
              v++;
              sv = 1;
              break;

            case SUBVALUE_MARK:
              sv++;
              break;
          }

          if ((f == field) && (v == value) && (sv == subvalue)) {
            p++;
            bytes_remaining--;
            item_found = TRUE;
            goto found; /* At start position */
          }
        }
        last_char = c;
        p++;
      } while (--bytes_remaining);

      /* Copy all of source chunk just searched */

      ts_copy(str_hdr->data, str_hdr->bytes);
    } while ((str_hdr = str_hdr->next) != NULL);
  }

  /* We have reached the end of the string without finding the field we
    were looking for.                                                  */

not_found:

found:

  if (str_hdr != NULL) /* Not run off end of string */
  {
    /* Copy up to current position in source chunk */

    len = p - str_hdr->data;

    if ((mode == DYN_DELETE) && (len > 0) && (c == mark)) /* Don't copy this mark */
    {
      mark_skipped = TRUE;
      len--;
    }

    if (len > 0)
      ts_copy(str_hdr->data, len);

    /* Skip old data if we are doing REPLACE or DELETE */

    if (item_found) {
      switch (mode) {
        case DYN_REPLACE:
        case DYN_DELETE:
          done = FALSE;

          /* Skip old item */

          do {
            /* Is there any data left in this chunk? */

            if (bytes_remaining > 0) {
              /* Look for delimiter in the current chunk */

              do {
                c = *p;
                if ((IsDelim(c)) && ((c == FIELD_MARK) || ((c == VALUE_MARK) && end_on_value_mark) || ((c == SUBVALUE_MARK) && end_on_subvalue_mark))) {
                  done = TRUE;
                  break;
                }
                p++;
              } while (--bytes_remaining);
            }

            if (!done) /* Find next chunk */
            {
              if (str_hdr->next == NULL)
                done = TRUE;
              else {
                str_hdr = str_hdr->next;
                p = str_hdr->data;
                bytes_remaining = str_hdr->bytes;
              }
            }
          } while (!done);

          if ((mode == DYN_DELETE) && !mark_skipped && (c == mark)) {
            /* Skip the mark too */
            p++;
            bytes_remaining--;
          }
          break;
      }
    }
  }

  /* Insert new data for REPLACE or INSERT */

  switch (mode) {
    case DYN_REPLACE:
    case DYN_INSERT:
      /* We may not be at the desired position. For example, if we try to
         insert <3,2,2> we might not have the necessary number of fields,
         values or subvalues to get to this position. We must insert marks
         to get us to the right position.                                  */

      if (result_descr->data.str.saddr != NULL) /* Not at start of string */
      {
        if (field < 0) {
          if (compatible && last_char == FIELD_MARK) {
            f = 1; /* Kill mark insertion */
            field = 1;
          } else {
            field = f + 1; /* Insert a single field mark */
          }
          v = 1;
          sv = 1;
        } else if (value < 0) {
          if ((field != f)                 /* No mark if adding new field... */
              || (last_char == FIELD_MARK) /* ... or at start of a field */
              || (compatible && last_char == VALUE_MARK)) {
            v = 1; /* 0222 Set both to one to kill mark insertion */
            value = 1;
          } else
            value = v + 1;
          sv = 1;
        } else if (subvalue < 0) {
          if ((field != f)                 /* No mark if adding new field... */
              || (value != v)              /* ...or adding new value... */
              || (last_char == FIELD_MARK) /* ...or at start of a field... */
              || (last_char == VALUE_MARK) /* ...or at start of a value */
              || (compatible && last_char == SUBVALUE_MARK)) {
            sv = 1; /* 0222 Set both to one to kill mark insertion */
            subvalue = 1;
          } else
            subvalue = sv + 1;
        }
      }

      while (f < field) {
        ts_copy_byte(FIELD_MARK);
        f++;
        v = 1;
        sv = 1;
      }

      while (v < value) {
        ts_copy_byte(VALUE_MARK);
        v++;
        sv = 1;
      }

      while (sv < subvalue) {
        ts_copy_byte(SUBVALUE_MARK);
        sv++;
      }

      /* Insert replacement item */

      for (new_str = new_descr->data.str.saddr; new_str != NULL; new_str = new_str->next) {
        ts_copy(new_str->data, new_str->bytes);
      }

      /* For insert mode, if we are not at the end of the source string and
         the next character of the source string is not a mark we must
         insert a mark to delimit the new data from the following item.     */

      if ((mode == DYN_INSERT) && (str_hdr != NULL)) {
        if (bytes_remaining == 0) /* Mark was at end of chunk */
        {
          if ((str_hdr = str_hdr->next) == NULL)
            goto no_mark_required;
          p = str_hdr->data;
          bytes_remaining = str_hdr->bytes;
        }

        if ((!IsDelim(*p)) || (*p <= mark))
          ts_copy_byte(mark);
      }
    no_mark_required:
      break;
  }

  /* Copy remainder of source string */

  if (str_hdr != NULL) {
    /* Is there any data left in this chunk? */

    if (bytes_remaining > 0)
      ts_copy(p, bytes_remaining);

    /* Copy any remaining chunks */

    while ((str_hdr = str_hdr->next) != NULL) {
      ts_copy(str_hdr->data, str_hdr->bytes);
    }
  }

  ts_terminate();
}

/* END-CODE */
