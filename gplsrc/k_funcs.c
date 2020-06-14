/* K_FUNCS.C
 * Kernel miscellaneous functions
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
 * 29Feb20 gwb Changed LONG_MAX to INT32_MAX.  When building for a 64 bit 
 *             platform, the LONG_MAX constant overflows the size of the
 *             int32_t variable type.  This change needed to be made across
 *             the entire code base.
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 *
 * START-HISTORY (OpenQM):
 * 15 Aug 07  2.6-0 Reworked remove pointers.
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 11 Jan 07  2.4-19 Added k_get_int32().
 * 21 Jul 06  2.4-10 Moved freeing of OBJDATA into objprog.c
 * 06 Apr 06  2.4-1 Added PERSISTENT descriptor type for CLASS module variables.
 * 04 Apr 06  2.4-1 0473 k_get_value() did not treat an OBJ as a value.
 * 02 Jan 06  2.3-3 Round float value in k_get_int() using pcfg.intprec.
 * 01 Dec 05  2.2-18 Added LOCALVARS.
 * 25 Oct 05  2.2-15 0426 k_str_to_num() should reject embedded spaces.
 * 01 Jul 05  2.2-3 Treat file and socket variables as true in get_bool().
 * 30 Jun 05  2.2-3 Added SOCK data type for sockets.
 * 09 Jun 05  2.2-1 Added truncate argument to ftoa().
 * 11 May 05  2.1-13 0352 Use ftoa() to avoid unbiased rounding of printf.
 * 28 Mar 05  2.1-11 0334 k_copy_string() failed if the source string was null.
 * 29 Oct 04  2.0-8 0279 [DBD] k_put_string() built target incorrectly if the
 *                  string required multiple chunks.
 * 21 Oct 04  2.0-7 Changed k_get_c_string() to allow long strings too.
 * 21 Oct 04  2.0-6 k_put_string() and k_put_c_string() can now handle strings
 *                  of over 32kb.
 * 01 Oct 04  2.0-3 0255 Yet another attempt at the overflow fix (0164, 0250).
 *                  There is no easy way to detect overflow after an
 *                  arithmetic operation. The old technique of reversing the
 *                  calculation and seeing if you got back what you started
 *                  with didn't work for the ten numbers starting at 2147483648 
 *                  but worked for everything else. The technique introduced
 *                  as 0250 didn't work for values with 2^32 set and 2^31
 *                  unset. This latest fix combined the two.
 * 29 Sep 04  2.0-3 Added support for PMATRIX.
 * 20 Sep 04  2.0-2 Use message handler.
 * 16 Sep 04  2.0-1 0250 Fix for 0164 was not adequate. Reworked overflow
 *                  detection.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * k_blank_string   Check if string is blank (null or all spaces)
 * k_copy_string    Make a copy of a string
 * k_dereference    Dereference a descriptor, allowing UNASSIGNED
 * k_deref_image    Decrement image reference count and release if zero
 * k_deref_string   Decrement string reference count and release if zero
 * k_get_bool       Convert top of e-stack to boolean
 * k_get_c_string   Extract string to C format string
 * k_get_file       Convert top of e-stack to file variable
 * k_get_float      Convert top of e-stack to float
 * k_get_int        Convert top of e-stack to integer
 * k_get_int32      Convert top of e-stack to integer, ignoring overflow
 * k_get_num        Convert top of e-stack to number
 * k_get_string     Convert top of e-stack to string
 * k_get_value      Convert top of e-stack to value descriptor
 * k_incr_refct     Increment ref count (where appropriate) on descr copy
 * k_is_num         Test if string can be converted to number
 * k_num_array1     Process single argument numeric array function
 * k_num_array2     Process two argument numeric array function
 * k_num_to_str     Convert number to string
 * k_put_c_string   Make string from C format string
 * k_put_string     Make string from contiguous string of given length
 * k_release        Release descriptor
 * k_str_to_num     Convert string to number
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include <math.h>
#include <stdint.h>

#include "qm.h"
#include "header.h"
#include "options.h"
#include "config.h"

double rounding[14] = {0.5,
                       0.05,
                       0.005,
                       0.0005,
                       0.00005,
                       0.000005,
                       0.0000005,
                       0.00000005,
                       0.000000005,
                       0.0000000005,
                       0.00000000005,
                       0.000000000005,
                       0.0000000000005,
                       0.00000000000005};

Private void k_non_numeric_zero(DESCRIPTOR* original, DESCRIPTOR* dereferenced);

/* ======================================================================
   k_blank_string()  -  Check for blank string                            */

bool k_blank_string(DESCRIPTOR* descr) /* Must be a STRING */
{
  STRING_CHUNK* str;
  int16_t i;
  char* p;

  for (str = descr->data.str.saddr; str != NULL; str = str->next) {
    for (p = str->data, i = str->bytes; i--; p++) {
      if (*p != ' ')
        return FALSE;
    }
  }

  return TRUE;
}

/* ======================================================================
   k_copy_string()  -  Make a copy of a string                            */

STRING_CHUNK* k_copy_string(STRING_CHUNK* src_str) {
  int16_t src_bytes_remaining;
  char* src;
  int32_t total_src_bytes_remaining;

  STRING_CHUNK* tgt_head = NULL;
  STRING_CHUNK* tgt_str;
  STRING_CHUNK* next_tgt;
  char* tgt;
  int16_t tgt_bytes_remaining = 0;

  int16_t n;

  if (src_str != NULL) /* 0334 */
  {
    total_src_bytes_remaining = src_str->string_len;

    do {
      src_bytes_remaining = src_str->bytes;
      src = src_str->data;

      /* Copy this chunk to the target string */

      while (src_bytes_remaining > 0) {
        if (tgt_bytes_remaining == 0) {
          next_tgt = s_alloc(total_src_bytes_remaining, &tgt_bytes_remaining);
          if (tgt_head == NULL)
            tgt_head = next_tgt;
          else
            tgt_str->next = next_tgt;
          tgt_str = next_tgt;
          tgt_str->string_len =
              total_src_bytes_remaining; /* Only right in... */
          tgt_str->ref_ct = 1;           /* ...first chunk */
          tgt = tgt_str->data;
        }

        n = min(tgt_bytes_remaining, src_bytes_remaining);

        memcpy(tgt, src, n);
        src += n;
        src_bytes_remaining -= n;
        total_src_bytes_remaining -= n;

        tgt += n;
        tgt_bytes_remaining -= n;
        tgt_str->bytes += n;
      }

      src_str = src_str->next;
    } while (src_str != NULL);
  }

  return tgt_head;
}

/* ======================================================================
   k_dereference()  -  Replace ADDR by value, allowing UNASSIGNED
   Returns a pointer to the addressed descriptor and replaces the
   argument descriptor by the dereferenced item.                          */

DESCRIPTOR* k_dereference(DESCRIPTOR* p) /* Descriptor to be dereferenced */
{
  register STRING_CHUNK* str;
  register u_char flags;
  DESCRIPTOR* q;

  if (p->type != ADDR)
    return p;

  flags = p->flags & DF_REUSE;
  q = p->data.d_addr;
  while (q->type == ADDR)
    q = q->data.d_addr;
  *p = *q;
  p->flags |= flags;

  switch (p->type) /* ++ALLTYPES++ */
  {
    case STRING:
    case SELLIST:
      p->flags &= ~DF_REMOVE; /* No table entry for e-stack descriptors */
      if ((str = p->data.str.saddr) != NULL)
        str->ref_ct++;
      break;

    case FILE_REF:
      p->data.fvar->ref_ct++;
      break;

    case SUBR:
      (((OBJECT_HEADER*)(p->data.subr.object))->ext_hdr.prog.refs)++;
      /* 0095 Above line was decrementing count */
      (p->data.subr.saddr->ref_ct)++; /* 0163 */
      break;

    case IMAGE:
      p->data.i_addr->ref_ct++;
      break;

    case BTREE:
      p->data.btree->ref_ct++;
      break;

    case SOCK:
      p->data.sock->ref_ct++;
      break;

    case OBJ:
      p->data.objdata->ref_ct++;
      break;
  }

  return q;
}

/* ======================================================================
   k_deref_image()  -  Decrement reference count, free if now zero        */

void k_deref_image(SCREEN_IMAGE* image) {
  if (image != NULL) {
    if (--(image->ref_ct) == 0)
      freescrn(image);
  }
}

/* ======================================================================
   k_deref_string()  -  Decrement reference count, free if now zero       */

void k_deref_string(STRING_CHUNK* str) {
  if ((str != NULL) && (--(str->ref_ct) == 0))
    s_free(str);
}

/* ======================================================================
   k_get_bool()  -  Get item as a boolean value                           */

void k_get_bool(DESCRIPTOR* p) {
  DESCRIPTOR* descr;
  bool result;
  STRING_CHUNK* str;
  int16_t bytes;
  char* s;
  char c;
  bool dp_seen;

  descr = k_dereference(p);

  switch (p->type) /* ++ALLTYPES++ */
  {
    case INTEGER:
      result = (p->data.value != 0);
      break;

    case FLOATNUM:
      result = (p->data.float_value != 0.0);
      break;

    case STRING:
    case SELLIST:
      if ((str = p->data.str.saddr) == NULL)
        result = FALSE;
      else {
        /* Result is false if string is number 0, true otherwise */

        result = FALSE;
        dp_seen = FALSE;

        bytes = str->bytes;
        s = str->data;
        c = *s;
        if ((c == '+') || (c == '-')) {
          bytes--;
          s++;
        }

        do {
          while (bytes) {
            c = *(s++);
            bytes--;
            switch (c) {
              case '0':
                break;

              case '.':
                if (dp_seen) {
                  result = TRUE;
                  goto set_result;
                }
                dp_seen = TRUE;
                break;

              default:
                result = TRUE;
                goto set_result;
            }
          }

          str = str->next;
          if (str == NULL)
            break;
          bytes = str->bytes;
          s = str->data;
        } while (1);
      }
      break;

    case FILE_REF:
    case SOCK:
      result = TRUE;
      break;

    case UNASSIGNED:
      if (!Option(OptUnassignedWarning))
        k_unassigned(descr);
      k_unass_zero(descr, p);
      result = (p->data.value != 0);
      break;

    default:
      k_non_numeric_zero(descr, p);
      result = (p->data.value != 0);
      break;
  }

set_result:
  Release(p);
  InitDescr(p, INTEGER);
  p->data.value = result;

  return;
}

/* ======================================================================
   k_get_c_string()  -  Extract string to C string
   If the string is longer than the given maximum, it is truncated
   and the function returns -1.                                           */

int k_get_c_string(DESCRIPTOR* descr,
                   char* s,
                   int max_bytes) /* Excludes terminator */
{
  STRING_CHUNK* str;
  int bytes = 0;
  int n;

  if (descr->type != STRING)
    k_get_string(descr);

  if ((str = descr->data.str.saddr) != NULL) {
    if ((bytes = str->string_len) > max_bytes)
      bytes = -1;

    do {
      n = min(str->bytes, max_bytes);
      memcpy(s, str->data, n);
      s += n;
      max_bytes -= n;
      if ((str = str->next) == NULL)
        break;
    } while (max_bytes);
  }

  *s = '\0';

  return bytes;
}

/* ======================================================================
   k_get_file()  -  Get item as a file variable                           */

void k_get_file(DESCRIPTOR* p) {
  if (p->type == ADDR)
    (void)k_dereference(p);
  if (p->type != FILE_REF)
    k_error(sysmsg(1200));
}

/* ======================================================================
   k_get_float()  -  Get item as a float                                  */

void k_get_float(DESCRIPTOR* p) {
  DESCRIPTOR* descr;

  descr = k_dereference(p);

  switch (p->type) /* ++ALLTYPES++ */
  {
    case INTEGER:
      InitDescr(p, FLOATNUM);
      p->data.float_value = (double)(p->data.value);
      break;

    case FLOATNUM:
      break;

    case STRING:
    case SELLIST:
      if (!k_str_to_num(p))
        k_non_numeric_zero(descr, p);
      if (p->type == INTEGER) {
        InitDescr(p, FLOATNUM);
        p->data.float_value = (double)(p->data.value);
      }
      break;

    case UNASSIGNED:
      if (!Option(OptUnassignedWarning))
        k_unassigned(descr);
      k_unass_zero(descr, p);
      InitDescr(p, FLOATNUM);
      p->data.float_value = 0;
      break;

    default:
      k_non_numeric_zero(descr, p);
      InitDescr(p, FLOATNUM);
      p->data.float_value = (double)(p->data.value);
      break;
  }

  process.numeric_array_allowed = FALSE;
  return;
}

/* ======================================================================
   k_get_int()  -  Get item as an integer                                 */

void k_get_int(DESCRIPTOR* p) {
  DESCRIPTOR* descr;

  descr = k_dereference(p);

  switch (p->type) { /* ++ALLTYPES++ */
    case INTEGER:
      break;

    case FLOATNUM:
      if (fabs(p->data.float_value) > INT32_MAX) {
        k_error(sysmsg(1220));
      }

      if (pcfg.intprec) {
        if (p->data.float_value < 0) {
          p->data.float_value -= rounding[pcfg.intprec - 1];
        } else {
          p->data.float_value += rounding[pcfg.intprec - 1];
        }
      }

      InitDescr(p, INTEGER);
      p->data.value = (int32_t)(p->data.float_value);
      break;

    case STRING:
    case SELLIST:
      if (!k_str_to_num(p))
        k_non_numeric_zero(descr, p);
      if (p->type == FLOATNUM) {
        if (fabs(p->data.float_value) > INT32_MAX) {
          k_error(sysmsg(1220));
        } 
        InitDescr(p, INTEGER);
        p->data.value = (int32_t)p->data.float_value;
      }
      break;

    case UNASSIGNED:
      if (!Option(OptUnassignedWarning))
        k_unassigned(descr);
      k_unass_zero(descr, p);
      break;

    default:
      k_non_numeric_zero(descr, p);
  }

  process.numeric_array_allowed = FALSE;
  return;
}

/* ======================================================================
   k_get_int32()  -  Get item as an integer, ignoring overflow            */

void k_get_int32(DESCRIPTOR* p) {
  DESCRIPTOR* descr;

  descr = k_dereference(p);

  switch (p->type) /* ++ALLTYPES++ */
  {
    case INTEGER:
      break;

    case FLOATNUM:
      if (pcfg.intprec) {
        if (p->data.float_value < 0) {
          p->data.float_value -= rounding[pcfg.intprec - 1];
        } else {
          p->data.float_value += rounding[pcfg.intprec - 1];
        }
      }

      InitDescr(p, INTEGER);
      p->data.value = (int32_t)(fmod(p->data.float_value, 0xFFFFFFFF));
      break;

    case STRING:
    case SELLIST:
      if (!k_str_to_num(p))
        k_non_numeric_zero(descr, p);
      if (p->type == FLOATNUM) {
        InitDescr(p, INTEGER);
        p->data.value = (int32_t)p->data.float_value;
      }
      break;

    case UNASSIGNED:
      if (!Option(OptUnassignedWarning))
        k_unassigned(descr);
      k_unass_zero(descr, p);
      break;

    default:
      k_non_numeric_zero(descr, p);
  }

  process.numeric_array_allowed = FALSE;
  return;
}

/* ======================================================================
   k_get_num()  -  Get item as a numeric                                  */

void k_get_num(DESCRIPTOR* p) {
  DESCRIPTOR* descr;

  descr = k_dereference(p);

  switch (p->type) /* ++ALLTYPES++ */
  {
    case INTEGER: /* Nothing to do for these types */
    case FLOATNUM:
      break;

    case STRING:
      if (!k_str_to_num(p))
        k_non_numeric_zero(descr, p);
      break;

    case UNASSIGNED:
      if (!Option(OptUnassignedWarning))
        k_unassigned(descr);
      k_unass_zero(descr, p);
      break;

    default:
      k_non_numeric_zero(descr, p);
  }

  process.numeric_array_allowed = FALSE;
  return;
}

/* ======================================================================
   k_get_string()  -  Get item as a string                                */

void k_get_string(DESCRIPTOR* p) {
  DESCRIPTOR* descr;

  descr = k_dereference(p);

  switch (p->type) /* ++ALLTYPES++ */
  {
    case INTEGER:
    case FLOATNUM:
      k_num_to_str(p);
      break;

    case STRING: /* Nothing to do */
      break;

    case SUBR:
      InitDescr(p, STRING);
      --(((OBJECT_HEADER*)(p->data.subr.object))->ext_hdr.prog.refs);
      p->data.str.saddr = p->data.subr.saddr;
      /* 0163, 0099     p->data.str.saddr->ref_ct++;  */
      break;

    case BTREE:
      btree_to_string(p);
      break;

    case UNASSIGNED:
      if (!Option(OptUnassignedWarning))
        k_unassigned(descr);
      k_unass_null(descr, p);
      break;

    default:
      k_error(sysmsg(1221));
  }
}

/* ======================================================================
   k_get_value()  -  Replace e_stack ADDR by value                      */

void k_get_value(DESCRIPTOR* p) {
  DESCRIPTOR* descr;

  descr = k_dereference(p);

  switch (p->type) /* ++ALLTYPES++ */
  {
    case INTEGER:
    case FLOATNUM:
    case SUBR:
    case STRING:
    case FILE_REF:
    case IMAGE:
    case BTREE:
    case SELLIST:
    case SOCK:
    case OBJ: /* 0473 */
      break;

    case UNASSIGNED:
      if (!Option(OptUnassignedWarning))
        k_unassigned(descr);
      k_unass_zero(descr, p);
      break;

    default:
      k_error(sysmsg(1201));
      break;
  }
}

/* ======================================================================
   k_incr_refct() - Increment reference count on descriptor copy          */

void k_incr_refct(DESCRIPTOR* p) {
  STRING_CHUNK* str;

  switch (p->type) /* ++ALLTYPES++ */
  {
    case UNASSIGNED:
    case ADDR:
    case INTEGER:
    case FLOATNUM:
    case PMATRIX:
    case OBJCD:
    case OBJCDX:
      break;

    case SUBR:
      (((OBJECT_HEADER*)(p->data.subr.object))->ext_hdr.prog.refs)++;
      str = p->data.subr.saddr;
      if (str != NULL)
        str->ref_ct++;
      break;

    case STRING:
    case SELLIST:
      str = p->data.str.saddr;
      if (str != NULL)
        (str->ref_ct)++;
      p->flags &= ~DF_REMOVE;
      break;

    case FILE_REF:
      p->data.fvar->ref_ct++;
      break;

    case ARRAY:
    case COMMON:
    case LOCALVARS:
    case PERSISTENT:
      p->data.ahdr_addr->ref_ct++;
      break;

    case IMAGE:
      p->data.i_addr->ref_ct++;
      break;

    case BTREE:
      p->data.btree->ref_ct++;
      break;

    case SOCK:
      p->data.sock->ref_ct++;
      break;

    case OBJ:
      p->data.objdata->ref_ct++;
      break;
  }
}

/* ======================================================================
   k_is_num()  -  Test if could convert string to number without actually
                  converting the descriptor type.                         */

bool k_is_num(DESCRIPTOR* p) {
  bool status = FALSE;
  STRING_CHUNK* str_hdr;
  register char c;
  int16_t i;
  bool sign = FALSE;    /* Sign seen? */
  int16_t digits = 0; /* No of digits seen before decimal point */
  bool dp = FALSE;      /* Decimal point seen */
  char* q;
  bool digit_seen = FALSE;

  str_hdr = p->data.str.saddr;
  if (str_hdr == NULL)
    goto is_numeric;

  do {
    for (i = str_hdr->bytes, q = str_hdr->data; i > 0; i--) {
      c = *(q++);

      if ((c >= '0') && (c <= '9')) {
        digit_seen = TRUE;
        if (!dp) {
          if (++digits > 15)
            goto non_numeric; /* Too big */
        }
      } else {
        switch (c) {
          case '+':
          case '-':
            if (digits || sign)
              goto non_numeric;
            sign = TRUE;
            break;

          case '.':
            if (dp)
              goto non_numeric; /* Already seen decimal point */
            dp = TRUE;
            digits++; /* Assume a zero before the decimal point */
            break;

          default:
            goto non_numeric;
        }
      }
    }
  } while ((str_hdr = str_hdr->next) != NULL);

  if (!digit_seen)
    goto non_numeric;

is_numeric:
  status = TRUE;

non_numeric:
  return status;
}

/* ======================================================================
   k_num_array1()  -  Process single argument numeric array function      */

void k_num_array1(void op_func(void)) /* Opcode function */
{
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  Source string                 | Result _ always a STRING    |
     |================================|=============================|

 */

#define MAX_NA_ELEMENT_LEN 64

  DESCRIPTOR* src_descr;   /* Source descriptor */
  int16_t delimiter = 0; /* Last mark character */
  STRING_CHUNK* src_str;   /* Current chunk */
  int16_t src_offset = 0;
  char* src;
  char s[MAX_NA_ELEMENT_LEN + 1]; /* Substring */

  DESCRIPTOR* result_descr;
  STRING_CHUNK* result_str;
  bool first = TRUE;

  register char c;
  int16_t n;

  void op_cat(void);

  src_descr = e_stack - 1;
  src_str = src_descr->data.str.saddr;

  result_descr = e_stack++;
  InitDescr(result_descr, STRING);
  result_descr->data.str.saddr = NULL;

  while (src_str != NULL) {
    /* Insert delimiter */

    if (!first) {
      InitDescr(e_stack, STRING);
      result_str = e_stack->data.str.saddr = s_alloc(1, &n);
      result_str->ref_ct = 1;
      result_str->string_len = 1;
      result_str->bytes = 1;
      result_str->data[0] = (char)delimiter;
      e_stack++;

      op_cat();
    }

    /* Fetch an element of the source array */

    n = 0;
    do {
      src = src_str->data + src_offset;
      while (src_offset++ < src_str->bytes) {
        c = *(src++);
        if (IsMark(c)) {
          delimiter = c;
          goto end_item;
        }
        if (n == MAX_NA_ELEMENT_LEN)
          k_nary_length_error();
        s[n++] = c;
      }
      src_str = src_str->next;
      src_offset = 0;
    } while (src_str != NULL);

  end_item:
    s[n] = '\0';

    /* Perform operation */

    k_put_c_string(s, e_stack);
    e_stack++;
    op_func();

    /* Save result */

    op_cat();

    first = FALSE;
  }

  /* Release the source descriptor and replace it by the result */

  k_release(src_descr);
  *src_descr = *result_descr;
  e_stack--;
}

/* ======================================================================
   k_num_array2()  -  Process two argument numeric array function         */

void k_num_array2(
    void op_func(void),      /* Opcode function */
    int16_t default_value) /* Value to use for arg2 in absence of REUSE.
                                 (Always uses zero for arg1) */
{
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  arg2                          | Result _ always a STRING    |
     |--------------------------------|-----------------------------| 
     |  arg1                          |                             |
     |================================|=============================|

    Example:
       A = B + C
    arg1 = B
    arg2 = C
 */

#define MAX_NA_ELEMENT_LEN 64

  DESCRIPTOR* arg1; /* arg1 source descriptor */
  bool reuse1;
  int16_t delimiter1 = 0; /* Last mark character */
  STRING_CHUNK* src1_str;   /* Current chunk */
  int16_t src1_offset = 0;
  char* src1;
  char s1[MAX_NA_ELEMENT_LEN + 1]; /* Substring */
  bool need1;

  DESCRIPTOR* arg2; /* arg2 source descriptor */
  bool reuse2;
  int16_t delimiter2 = 0; /* Last mark character */
  STRING_CHUNK* src2_str;   /* Current chunk */
  int16_t src2_offset = 0;
  char* src2;
  char s2[MAX_NA_ELEMENT_LEN + 1]; /* Substring */
  bool need2;

  DESCRIPTOR op_arg1;
  DESCRIPTOR op_arg2;

  DESCRIPTOR* result_descr;
  STRING_CHUNK* result_str;
  char result_delimiter;
  bool first = TRUE;

  register char c;
  int16_t n;

  void op_cat(void);

  /* Ensure that both arguments are strings */

  arg2 = e_stack - 1;
  reuse2 = arg2->flags & DF_REUSE;
  k_get_string(arg2);
  src2_str = arg2->data.str.saddr;

  arg1 = e_stack - 2;
  reuse1 = arg1->flags & DF_REUSE;
  k_get_string(arg1);
  src1_str = arg1->data.str.saddr;

  InitDescr(&op_arg1, UNASSIGNED);
  InitDescr(&op_arg2, UNASSIGNED);

  result_descr = e_stack++;
  InitDescr(result_descr, STRING);
  result_descr->data.str.saddr = NULL;

  while ((src1_str != NULL) || (src2_str != NULL)) {
    /* Determine which items we need to fetch from the source strings */

    need1 = (delimiter1 <= delimiter2);
    need2 = (delimiter1 >= delimiter2);

    if (need1) {
      /* Fetch an element of arg1 */

      delimiter1 = 256; /* For end of string paths */
      n = 0;
      while (src1_str != NULL) {
        src1 = src1_str->data + src1_offset;
        while (src1_offset++ < src1_str->bytes) {
          c = *(src1++);
          if (IsMark(c)) {
            delimiter1 = c;
            goto end_item1;
          }
          if (n == MAX_NA_ELEMENT_LEN)
            k_nary_length_error();
          s1[n++] = c;
        }
        src1_str = src1_str->next;
        src1_offset = 0;
      }
    end_item1:
      s1[n] = '\0';

      k_release(&op_arg1);
      k_put_c_string(s1, &op_arg1);
      k_get_num(&op_arg1);
    } else /* Reuse or insert default value for item 1 */
    {
      if (!reuse1) {
        k_release(&op_arg1);
        InitDescr(&op_arg1, INTEGER);
        op_arg1.data.value = 0;
      }
    }

    if (need2) {
      /* Fetch an element of arg2 */

      delimiter2 = 256; /* For end of string paths */
      n = 0;
      while (src2_str != NULL) {
        src2 = src2_str->data + src2_offset;
        while (src2_offset++ < src2_str->bytes) {
          c = *(src2++);
          if (IsMark(c)) {
            delimiter2 = c;
            goto end_item2;
          }
          if (n == MAX_NA_ELEMENT_LEN)
            k_nary_length_error();
          s2[n++] = c;
        }
        src2_str = src2_str->next;
        src2_offset = 0;
      }
    end_item2:
      s2[n] = '\0';

      k_release(&op_arg2);
      k_put_c_string(s2, &op_arg2);
      k_get_num(&op_arg2);
    } else /* Reuse or insert default value for item 2 */
    {
      if (!reuse2) {
        k_release(&op_arg2);
        InitDescr(&op_arg2, INTEGER);
        op_arg2.data.value = default_value;
      }
    }

    /* Insert delimiter */

    if (!first) {
      InitDescr(e_stack, STRING);
      result_str = e_stack->data.str.saddr = s_alloc(1, &n);
      result_str->ref_ct = 1;
      result_str->string_len = 1;
      result_str->bytes = 1;
      result_str->data[0] = result_delimiter;
      e_stack++;

      op_cat();
    }

    result_delimiter = (char)min(delimiter1, delimiter2);

    /* Perform operation */

    InitDescr(e_stack, ADDR);
    (e_stack++)->data.d_addr = &op_arg1;
    InitDescr(e_stack, ADDR);
    (e_stack++)->data.d_addr = &op_arg2;

    op_func();

    /* Save result */

    op_cat();

    first = FALSE;
  }

  k_release(&op_arg1);
  k_release(&op_arg2);

  /* Release the source descriptors as the calling code will probably only
    pop these as it usually expects numeric items.
    Copy the result value into the arg1 descriptor.
    Pop the result descriptor - Do not release as we copied it.           */

  k_release(arg1);
  k_release(arg2);
  *arg1 = *result_descr;
  e_stack--;
}

/* ======================================================================
   k_num_to_str()  -  Convert descriptor from numeric to string           */

void k_num_to_str(DESCRIPTOR* p) {
  STRING_CHUNK* str;
  char s[32 + 1];
  int16_t s_len;
  int16_t actual_size;
  char* q;

  switch (p->type) /* ++ALLTYPES++ */
  {
    case INTEGER:
      Ltoa(p->data.value, s, 10);
      break;

    case FLOATNUM:
      s_len = ftoa(p->data.float_value, process.program.precision, FALSE,
                   s); /* 0352 */
      if (strchr(s, '.') != NULL) {
        /* Truncate trailing zeros */
        for (q = s + s_len - 1; *q == '0'; q--) {
        }
        if (*q == '.')
          *q = '\0';
        else
          *(q + 1) = '\0';
      }
      break;
  }

  s_len = strlen(s);
  str = s_alloc((int32_t)s_len, &actual_size);
  str->bytes = s_len;
  str->string_len = s_len;
  str->ref_ct = 1;
  memcpy(str->data, s, s_len);

  /* Do not bother to release descriptor as it must be integer or float and
    we will overlay it with our result string.                             */

  InitDescr(p, STRING);
  p->data.str.saddr = str;
}

/* ======================================================================
   Create string from C string                                            */

void k_put_c_string(char* s, DESCRIPTOR* descr) {
  k_put_string(s, (s) ? strlen(s) : 0, descr);
}

/* ======================================================================
   k_put_string() - Create string from contiguous string of given length  */

void k_put_string(char* s, /* Data to copy. NULL returns blank string */
                  int len, /* Byte count.   Zero returns blank string */
                  DESCRIPTOR* descr) /* Target descriptor */
{
  char* src;
  STRING_CHUNK* tgt_str;
  STRING_CHUNK* next_tgt;
  int16_t n;
  int16_t actual_size;

  InitDescr(descr, STRING);
  descr->data.str.saddr = NULL;

  if (s != NULL) {
    src = s;

    while (len) {
      next_tgt = s_alloc(len, &actual_size); /* 0279 */
      n = min(actual_size, len);

      if (descr->data.str.saddr == NULL)
        descr->data.str.saddr = next_tgt;
      else
        tgt_str->next = next_tgt;
      tgt_str = next_tgt;

      memcpy(tgt_str->data, src, n);
      tgt_str->bytes = n;

      /* Set total string length and reference count for first chunk. This is
        also set (incorrectly) in subsequent chunks.                         */

      tgt_str->string_len = len;
      tgt_str->ref_ct = 1;

      src += n;
      len -= n;
    }
  }
}

/* ======================================================================
   k_release()  -  Release descriptor, converting to UNASSIGNED         */

void k_release(DESCRIPTOR* p) {
  FILE_VAR* fvar;
  SCREEN_IMAGE* scrn;
  STRING_CHUNK* str;
  BTREE_HEADER* btree;
  ARRAY_HEADER* ahdr;
  ARRAY_HEADER* next_ahdr;
  SOCKVAR* sock;
  OBJDATA* objdata;

  switch (p->type) /* ++ALLTYPES++ */
  {
    case UNASSIGNED:
    case ADDR:
    case INTEGER:
    case FLOATNUM:
    case OBJCD:
      break;

    case SUBR:
      /* Release object */

      if (--(((OBJECT_HEADER*)(p->data.subr.object))->ext_hdr.prog.refs) == 0) {
      }

      /* Release name string */

      str = p->data.subr.saddr;
      if ((str != NULL) && (--(str->ref_ct) == 0))
        s_free(str);
      break;

    case STRING:
    case SELLIST:
      str = p->data.str.saddr;
      if ((str != NULL) && (--(str->ref_ct) == 0))
        s_free(str);
      break;

    case FILE_REF:
      fvar = p->data.fvar;
      if (--(fvar->ref_ct) == 0)
        dio_close(fvar);
      break;

    case ARRAY:
      ahdr = p->data.ahdr_addr;
      if (--(ahdr->ref_ct) == 0)
        free_array(ahdr);
      break;

    case COMMON:
      ahdr = p->data.ahdr_addr;
      if (--(ahdr->ref_ct) == 0) {
        if (ahdr->flags & AH_AUTO_DELETE)
          delete_common(ahdr);
      }
      break;

    case IMAGE:
      scrn = p->data.i_addr;
      if (--(scrn->ref_ct) == 0)
        freescrn(scrn);
      break;

    case BTREE:
      btree = p->data.btree;
      if (--(btree->ref_ct) == 0) {
        if (btree->head != NULL)
          free_btree_element(btree->head, btree->keys);
        k_free(btree);
      }
      break;

    case PMATRIX:
      k_free(p->data.pmatrix);
      break;

    case SOCK:
      sock = p->data.sock;
      if (--(sock->ref_ct) == 0) {
        close_skt(sock);
        k_free(sock);
      }
      break;

    case LOCALVARS:
      for (ahdr = p->data.ahdr_addr; ahdr != NULL; ahdr = next_ahdr) {
        next_ahdr = ahdr->next_common;
        free_array(ahdr);
      }
      break;

    case OBJ:
      objdata = p->data.objdata;
      free_object(objdata);
      break;

    case OBJCDX:
      if (p->data.objundef.undefined_name != NULL) {
        k_free(p->data.objundef.undefined_name);
      }
      break;

    case PERSISTENT:
      ahdr = p->data.ahdr_addr;
      --(ahdr->ref_ct);
      break;
  }

  InitDescr(p, UNASSIGNED);
}

/* ======================================================================
   k_str_to_num()  -  Convert descriptor from string to numeric           */

bool k_str_to_num(DESCRIPTOR* p) /* Returns FALSE if cannot convert */
{
  bool status = FALSE;
  STRING_CHUNK* str_chnk;
  STRING_CHUNK* first_chunk;
  register char c;
  int16_t i;
  bool negative;    /* Negative value? */
  int16_t digits; /* Digits before decimal point */
  int16_t dp;     /* Decimal places */
  bool floating;
  int32_t value;
  double float_value;
  double dplace;
  char* q;
  u_char reuse_flag;
  bool num_array;
  register int32_t temp;
  bool trailing_space;

  num_array = FALSE;
  dp = 0;
  value = 0;
  negative = FALSE;
  floating = FALSE;
  trailing_space = FALSE;

  reuse_flag = p->flags & DF_REUSE; /* Preserve reuse flag */

  first_chunk = p->data.str.saddr; /* Remember start of chain for later */
  if (first_chunk != NULL) {
    digits = 0;
    str_chnk = first_chunk;
    do {
      for (i = str_chnk->bytes, q = str_chnk->data; i > 0; i--) {
        c = *(q++);

        if ((c >= '0') && (c <= '9')) {
          if (trailing_space)
            goto non_numeric;

          if (floating) {
            if (dp >= 0) /* We have seen decimal point */
            {
              if (dplace != 0.0) {
                float_value += (c - '0') * dplace;
                dp++;
                dplace /= 10;
              }
            } else {
              if (++digits > 15)
                goto non_numeric; /* Too big */
              float_value = (float_value * 10) + (c - '0');
            }
          } else {
            c -= '0';
            temp = value * 10 + c;
            if ((temp < value) || (((temp - c) / 10) != value)) {
              /* Must convert to float */
              floating = TRUE;
              float_value = ((double)value * 10) + c;
              dp = -1; /* -ve implies not seen decimal point */
            } else {
              value = temp;
            }
            digits++;
          }
        } else if (IsMark(c) && process.numeric_array_allowed) {
          /* Ok, perhaps it is going to turn out to be a numeric
              array.  Start processing again for next element     */

          num_array = TRUE;
          dp = 0;
          digits = 0;
          value = 0;
          negative = FALSE;
          floating = FALSE;
          trailing_space = FALSE;
        } else if (c == ' ') {
          /* Leading and trailing spaces are ignored but embedded
              spaces should cause an error. If we see a space when we
              have already collected some data, set the trailing_space
              flag. Any further data characters will give an error.     */

          trailing_space = (digits || negative);
        } else {
          if (trailing_space)
            goto non_numeric;

          switch (c) {
            case '+':
              if (digits || negative)
                goto non_numeric;
              break;

            case '-':
              if (digits || negative)
                goto non_numeric;
              negative = TRUE;
              break;

            case '.':
              digits++; /* Assume at least one */
              if (floating) {
                if (dp >= 0)
                  goto non_numeric; /* Already seen decimal point */
              } else {
                floating = TRUE;
                float_value = (double)value;
              }

              dp = 0;
              dplace = 0.1;
              break;

            default:
              goto non_numeric;
          }
        }
      }

      str_chnk = str_chnk->next;
    } while (str_chnk != NULL);

    if (num_array) /* We have found a numeric array */
    {
      p->flags |= reuse_flag;
      return TRUE;
    }

    if ((digits | dp) == 0)
      goto non_numeric;

    /* Release string chunks */

    k_deref_string(first_chunk); /* 0160  Moved to after non-numeric test */
  }

  if (floating) {
    InitDescr(p, FLOATNUM);
    p->data.float_value = (negative) ? -float_value : float_value;
  } else {
    InitDescr(p, INTEGER);
    p->data.value = (negative) ? -value : value;
  }

  p->flags |= reuse_flag;
  status = TRUE;

non_numeric:
  return status;
}

/* ======================================================================
   strdbl()  -  Attempt to convert string to number as double             */

bool strdbl(               /* Returns FALSE if cannot convert */
            char* p,       /* String to convert */
            double* value) /* Unchanged if conversion fails */
{
  bool status = FALSE;
  bool negative = FALSE; /* Negative value? */
  int16_t digits = 0;  /* Digits before decimal point */
  double float_value = 0;
  double dplace;

  /* Skip leading spaces */

  while (*p == ' ')
    p++;

  if (*p != '\0') {
    /* Check for a minus sign */

    if (*p == '-') {
      negative = TRUE;
      p++;
    }

    /* Collect digits before decimal point */

    while (IsDigit(*p)) {
      if (++digits > 15)
        goto non_numeric; /* Too big */
      float_value = (float_value * 10) + (*(p++) - '0');
    }

    if (*p == '.') /* Decimal point found */
    {
      p++;
      dplace = 0.1;
      while (IsDigit(*p)) {
        float_value += (*(p++) - '0') * dplace;
        dplace /= 10;
      }
    }

    if (*p != '\0')
      goto non_numeric;
  }

  status = TRUE;
  *value = (negative) ? (-float_value) : float_value;

non_numeric:
  return status;
}

/* ======================================================================
   strnint()  -  Attempt to convert string to number as int32_t          */

bool strnint(                 /* Returns FALSE if cannot convert */
             char* p,         /* String to convert */
             int16_t len,   /* Bytes in string */
             int32_t* value) /* Unchanged if conversion fails */
{
  bool status = FALSE;
  bool negative = FALSE; /* Negative value? */
  int32_t ivalue = 0;
  register int32_t temp;
  int16_t c;

  if (len && (*p != '\0')) {
    /* Check for a sign */

    if (*p == '-') {
      negative = TRUE;
      p++;
      len--;
    } else if (*p == '+') {
      p++;
      len--;
    }

    /* Collect digits */

    while (len && IsDigit(*p)) {
      c = (*(p++) - '0');
      temp = (ivalue * 10) + c;
      if ((temp < ivalue) || (((temp - c) / 10) != ivalue))
        goto non_numeric;
      ivalue = temp;
      len--;
    }

    if (len && (*p != '\0'))
      goto non_numeric;
  }

  status = TRUE;
  *value = (negative) ? (-ivalue) : ivalue;

non_numeric:
  return status;
}

/* ======================================================================
   GetUnsignedInt() - Return value as unsigned int, ignoring overflow     */

u_int32_t GetUnsignedInt(DESCRIPTOR* descr) {
  DESCRIPTOR* q;
  u_int32_t ivalue;
  STRING_CHUNK* str;
  char* p;
  int16_t bytes;
  bool negative = FALSE;

  q = descr;
  while (q->type == ADDR)
    q = descr->data.d_addr;

  switch (q->type) /* ++ALLTYPES++ */
  {
    case INTEGER:
      return *((u_int32_t*)&(q->data.value));

    case FLOATNUM:
      return (u_int32_t)(q->data.float_value);

    case STRING:
      ivalue = 0;
      str = q->data.str.saddr;
      if (str != NULL) {
        bytes = str->bytes;
        p = str->data;

        /* Check for a sign */

        if (*p == '-') {
          negative = TRUE;
          p++;
          bytes--;
        } else if (*p == '+') {
          p++;
          bytes--;
        }

        while (1) {
          while (bytes--) {
            if (!IsDigit(*p))
              goto non_numeric;
            ivalue = (ivalue * 10) + (*(p++) - '0');
          }

          str = str->next;
          if (str == NULL)
            break;

          bytes = str->bytes;
          p = str->data;
        }
      }

      if (negative) {
        ivalue = (ivalue ^ 0xFFFFFFFFL) + 1;
      }

      return ivalue;
  }

non_numeric:
  k_non_numeric_zero(descr, q);
  return 0;
}

/* ======================================================================
   k_non_numeric_zero()  -  Substitue zero for non-numeric variable       */

Private void k_non_numeric_zero(DESCRIPTOR* original,
                                DESCRIPTOR* dereferenced) {
  if (!Option(OptNonNumericWarning))
    k_non_numeric_error(original);
  k_non_numeric_warning(original);
  k_release(dereferenced);
  InitDescr(dereferenced, INTEGER);
  dereferenced->data.value = 0;
}

/* ======================================================================
   k_unass_zero()  -  Substitue zero for unassigned variable              */

void k_unass_zero(DESCRIPTOR* original, DESCRIPTOR* dereferenced) {
  InitDescr(dereferenced, INTEGER);
  dereferenced->data.value = 0;
  k_unassigned_zero(original);
}

/* ======================================================================
   k_unass_null()  -  Substitute null for unassigned variable             */

void k_unass_null(DESCRIPTOR* original, DESCRIPTOR* dereferenced) {
  InitDescr(dereferenced, STRING);
  dereferenced->data.str.saddr = NULL;
  k_unassigned_null(original);
}

/* END-CODE */
