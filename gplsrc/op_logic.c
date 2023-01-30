/* OP_LOGIC.C
 * Logical opcodes
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
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 *
 * START-HISTORY (OpenQM):
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 10 Jan 07  2.4-19 Bitwise operations now use GetInt32 to ignore overflow.
 * 03 May 05  2.1-13 Added support for case insensitive strings.
 * 17 Sep 04  2.0-2 0252 MAX() and MIN() were failing with non-numeric data.
 * 16 Sep 04  2.0-1 Added MIN() and MAX().
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * op_and      AND
 * op_bitand   BITAND()
 * op_bitor    BITOR()
 * op_bitnot   BITNOT()
 * op_bitreset BITRESET()
 * op_bitset   BITSET()
 * op_bittest  BITTEST()
 * op_bitxor   BITXOR()
 * op_eq       EQ
 * op_ge       GE
 * op_gt       GT
 * op_le       LE
 * op_lt       LT
 * op_max      MAX
 * op_min      MIN
 * op_ne       NE
 * op_not      NOT()
 * op_or       OR
 * op_shift    SHIFT()
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include "config.h"
#include "header.h"

#include <math.h>

Private bool compare_values(int16_t mode, bool dismiss);
#define TEST_EQ 0
#define TEST_GT 1
#define TEST_GE 2

/* ======================================================================
   op_and()  -  Logical AND                                               */

void op_and() {
  DESCRIPTOR* arg1;
  DESCRIPTOR* arg2;

  arg1 = e_stack - 1;
  GetBool(arg1);

  arg2 = e_stack - 2;
  GetBool(arg2);

  arg2->data.value = arg1->data.value && arg2->data.value;

  k_pop(1);
}

/* ======================================================================
   op_bitand()  -  Bitwise AND                                            */

void op_bitand() {
  DESCRIPTOR* arg1;
  DESCRIPTOR* arg2;

  arg1 = e_stack - 1;
  GetInt32(arg1);

  arg2 = e_stack - 2;
  GetInt32(arg2);

  arg2->data.value &= arg1->data.value;

  k_pop(1);
}

/* ======================================================================
   op_bitnot()  -  Bitwise NOT                                            */

void op_bitnot() {
  DESCRIPTOR* arg1;

  arg1 = e_stack - 1;
  GetInt32(arg1);

  arg1->data.value = ~(arg1->data.value);
}

/* ======================================================================
   op_bitor()  -  Bitwise OR                                              */

void op_bitor() {
  DESCRIPTOR* arg1;
  DESCRIPTOR* arg2;

  arg1 = e_stack - 1;
  GetInt32(arg1);

  arg2 = e_stack - 2;
  GetInt32(arg2);

  arg2->data.value |= arg1->data.value;

  k_pop(1);
}

/* ======================================================================
   op_bitreset()  -  Reset bit                                            */

void op_bitreset() {
  DESCRIPTOR* descr;
  int bitno;

  descr = e_stack - 1;
  GetInt(descr);
  bitno = descr->data.value;

  descr = e_stack - 2;
  GetInt32(descr);
  descr->data.value &= ~(1 << bitno);

  k_pop(1);
}

/* ======================================================================
   op_bitset()  -  Set bit                                                */

void op_bitset() {
  DESCRIPTOR* descr;
  int bitno;

  descr = e_stack - 1;
  GetInt(descr);
  bitno = descr->data.value;

  descr = e_stack - 2;
  GetInt32(descr);
  descr->data.value |= 1 << bitno;

  k_pop(1);
}

/* ======================================================================
   op_bittest()  -  Bit test                                              */

void op_bittest() {
  DESCRIPTOR* descr;
  int bitno;

  descr = e_stack - 1;
  GetInt(descr);
  bitno = descr->data.value;

  descr = e_stack - 2;
  GetInt32(descr);
  descr->data.value = ((descr->data.value & (1 << bitno)) != 0);

  k_pop(1);
}

/* ======================================================================
   op_bitxor()  -  Bitwise XOR                                            */

void op_bitxor() {
  DESCRIPTOR* arg1;
  DESCRIPTOR* arg2;

  arg1 = e_stack - 1;
  GetInt32(arg1);

  arg2 = e_stack - 2;
  GetInt32(arg2);

  arg2->data.value ^= arg1->data.value;

  k_pop(1);
}

/* ======================================================================
   op_eq()  -  Test equals                                                */

void op_eq() {
  bool result;

  result = compare_values(TEST_EQ, TRUE);

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = (int32_t)result;
}

/* ======================================================================
   op_ge()  -  Test greater than or equal to                              */

void op_ge() {
  bool result;

  result = compare_values(TEST_GE, TRUE);

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = (int32_t)result;
}

/* ======================================================================
   op_gt()  -  Test greater then                                          */

void op_gt() {
  bool result;

  result = compare_values(TEST_GT, TRUE);

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = (int32_t)result;
}

/* ======================================================================
   op_le()  -  Test less than or equal to                                 */

void op_le() {
  bool result;

  result = !compare_values(TEST_GT, TRUE);

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = (int32_t)result;
}

/* ======================================================================
   op_lt()  -  Test less than                                             */

void op_lt() {
  bool result;

  result = !compare_values(TEST_GE, TRUE);

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = (int32_t)result;
}

/* ======================================================================
   op_max()  -  MAX() function                                            */

void op_max() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  B                          |  Result                     |
     |-----------------------------|-----------------------------|
     |  A                          |                             |
     |=============================|=============================|

 */

  DESCRIPTOR* arg_a;
  DESCRIPTOR* arg_b;
  DESCRIPTOR temp;

  arg_b = e_stack - 1; /* 0252 Removed GetNum() calls */
  arg_a = e_stack - 2;

  if (!compare_values(TEST_GE, FALSE)) {
    temp = *arg_b;
    *arg_b = *arg_a;
    *arg_a = temp;
  }

  k_dismiss();
}

/* ======================================================================
   op_min()  -  MIN() function                                            */

void op_min() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  B                          |  Result                     |
     |-----------------------------|-----------------------------|
     |  A                          |                             |
     |=============================|=============================|

 */

  DESCRIPTOR* arg_a;
  DESCRIPTOR* arg_b;
  DESCRIPTOR temp;

  arg_b = e_stack - 1; /* 0252 Removed GetNum() calls */
  arg_a = e_stack - 2;

  if (compare_values(TEST_GE, FALSE)) {
    temp = *arg_b;
    *arg_b = *arg_a;
    *arg_a = temp;
  }

  k_dismiss();
}

/* ======================================================================
   op_ne()  -  Test not equals                                            */

void op_ne() {
  bool result;

  result = !compare_values(TEST_EQ, TRUE);

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = (int32_t)result;
}

/* ======================================================================
   op_not()  -  Logical NOT                                               */

void op_not() {
  DESCRIPTOR* descr;

  descr = e_stack - 1;
  GetBool(descr);
  descr->data.value = !descr->data.value;
}

/* ======================================================================
   op_or()  -  Logical OR                                                 */

void op_or() {
  DESCRIPTOR* arg1;
  DESCRIPTOR* arg2;

  arg1 = e_stack - 1;
  GetBool(arg1);

  arg2 = e_stack - 2;
  GetBool(arg2);

  arg2->data.value = arg1->data.value || arg2->data.value;

  k_pop(1);
}

/* ======================================================================
   op_shift()  -  Shift function                                          */

void op_shift() {
  /* Stack:

     |================================|=============================|
     |            BEFORE              |           AFTER             |
     |================================|=============================|
 top |  Shift distance. -ve = left    | Modified value              |
     |--------------------------------|-----------------------------| 
     |  Source value                  |                             |
     |================================|=============================|
 */

  DESCRIPTOR* descr;
  int32_t shift_count;
  u_int32_t value;

  descr = e_stack - 1;
  GetInt(descr);
  shift_count = descr->data.value;

  descr = e_stack - 2;
  GetInt(descr);
  value = (u_int32_t)descr->data.value;

  if (shift_count < 0)
    descr->data.value = value << -shift_count;
  else
    descr->data.value = value >> shift_count;

  k_pop(1);
}

/* ======================================================================
   compare_values()  -  Common comparison function for EQ, NE, LT and GT  */

Private bool compare_values(int16_t mode, bool dismiss) {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Arg 2                      | Result: 1 = TRUE, 0 = FALSE |
     |-----------------------------|-----------------------------| 
     |  Arg 1                      |                             |
     |=============================|=============================|
*/

  DESCRIPTOR* arg1;
  DESCRIPTOR* arg2;
  STRING_CHUNK* str1;
  int32_t len1;
  int16_t bytes1;
  char* p1;
  STRING_CHUNK* str2;
  int32_t len2;
  int16_t bytes2;
  char* p2;
  int diff;   /* int16_t diff will cause truncate issue with SortCompare (MemCompareNoCase / memcmp) */
  bool eq = FALSE; /* TRUE if arg1 = arg2 */
  bool gt = FALSE; /* TRUE if arg1 > arg2 */
  int16_t n;
  register int32_t v1;
  register int32_t v2;
  bool nocase;

  arg2 = e_stack - 1;
  k_get_value(arg2);

  arg1 = e_stack - 2;
  k_get_value(arg1);

recompare:

  switch (arg1->type) {
    case INTEGER:
      switch (arg2->type) {
        case INTEGER:
          v1 = arg1->data.value;
          v2 = arg2->data.value;
          if (!(eq = (v1 == v2))) {
            gt = (v1 > v2);
          }
          break;

        case FLOATNUM:
          if (!(eq = (fabs(((double)arg1->data.value) -
                           arg2->data.float_value) <= pcfg.fltdiff))) {
            gt = (((double)arg1->data.value) > arg2->data.float_value);
          }
          break;

        case SUBR:
          k_get_string(arg2);
          /* **** FALL THROUGH **** */

        case STRING:
          if (arg2->data.str.saddr == NULL) /* Special case, int vs null */
          {
            gt = TRUE;
            goto exit_compare_values;
          }

          if (k_str_to_num(arg2))
            goto recompare;

          /* Convert arg1 e-stack item to a string and compare as strings */

          k_num_to_str(arg1);
          goto string_comparison;

        default:
          k_value_error(arg2);
      }
      break;

    case FLOATNUM:
      switch (arg2->type) {
        case INTEGER:
          if (!(eq = (fabs(arg1->data.float_value -
                           ((double)arg2->data.value)) <= pcfg.fltdiff))) {
            gt = (arg1->data.float_value > ((double)arg2->data.value));
          }
          break;

        case FLOATNUM:
          if (!(eq = (fabs(arg1->data.float_value - arg2->data.float_value) <=
                      pcfg.fltdiff))) {
            gt = (arg1->data.float_value > arg2->data.float_value);
          }
          break;

        case SUBR:
          k_get_string(arg2);
          /* **** FALL THROUGH **** */

        case STRING:
          if (arg2->data.str.saddr == NULL) /* Special case, float vs null */
          {
            gt = TRUE;
            goto exit_compare_values;
          }

          if (k_str_to_num(arg2))
            goto recompare;

          /* Convert arg1 to a string and compare as strings */

          k_num_to_str(arg1);
          goto string_comparison;

        default:
          k_value_error(arg2);
      }
      break;

    case SUBR:
      k_get_string(arg1);
      /* **** FALL THROUGH **** */

    case STRING:
      switch (arg2->type) {
        case INTEGER:
        case FLOATNUM:
          if (arg1->data.str.saddr == NULL) /* Special case, null vs num */
          {
            goto exit_compare_values;
          }

          if (k_str_to_num(arg1))
            goto recompare;
          /* Convert arg2 to a string and compare as strings */

          k_num_to_str(arg2);
          goto string_comparison;

        case SUBR:
          k_get_string(arg2);
          /* **** FALL THROUGH **** */

        case STRING:
          /* If one but not both strings is null, they do not match.
              If both are null, they match without any further work.   */

          if (arg1->data.str.saddr == NULL) {
            if (arg2->data.str.saddr == NULL) {
              eq = TRUE;
            }
            goto exit_compare_values;
          } else if (arg2->data.str.saddr == NULL) {
            gt = TRUE;
            goto exit_compare_values;
          }

          /* Both items are strings. See if they can BOTH be converted to
              numeric before we do anything else                          */

          if (k_is_num(arg1) && k_is_num(arg2)) {
            k_str_to_num(arg1);
            k_str_to_num(arg2);
            goto recompare;
          }

        string_comparison: /* **** YUCK!  Jumping into a switch statement */

          str1 = arg1->data.str.saddr;
          str2 = arg2->data.str.saddr;

          if (str1 == str2) { /* Both point to same string */
            eq = TRUE;
          } else if (str1 == NULL) {
            /* arg1 is a null string. arg2 is not */
            eq = FALSE;
            /* gt = FALSE;   already done */
          } else if (str2 == NULL) {
            /* arg2 is a null string. arg1 is not */
            eq = FALSE;
            gt = TRUE;
          } else /* Neither string is null */
          {
            len1 = str1->string_len;
            bytes1 = str1->bytes;
            p1 = str1->data;

            len2 = str2->string_len;
            bytes2 = str2->bytes;
            p2 = str2->data;

            nocase = (process.program.flags & HDR_NOCASE) != 0;
            do {
              n = min(bytes1, bytes2);
              diff = SortCompare(p1, p2, n, nocase);

              if (diff != 0) /* Found a difference */
              {
                eq = FALSE;
                gt = (diff > 0);
                goto exit_compare_values;
              }

              /* The strings match to the end of the shorter */

              p1 += n;
              bytes1 -= n;
              len1 -= n;

              p2 += n;
              bytes2 -= n;
              len2 -= n;

              if (bytes2 == 0) {
                if (len2 == 0) /* We have hit the end of string 2 */
                {
                  if (len1 == 0) /* Perfect match */
                  {
                    eq = TRUE;
                  } else /* End of string 2 before end of string 1 */
                  {
                    eq = FALSE;
                    gt = TRUE;
                  }
                  goto exit_compare_values;
                }

                /* Find next chunk of string 2 */

                str2 = str2->next;
                p2 = str2->data;
                bytes2 = str2->bytes;
              }

              if (bytes1 == 0) {
                if (len1 == 0) /* We have hit the end of string 1 */
                {
                  /* End of string 1 before end of string 2 */
                  eq = FALSE;
                  goto exit_compare_values;
                }

                str1 = str1->next;
                p1 = str1->data;
                bytes1 = str1->bytes;
              }
            } while (TRUE);
          }
          break;
        default:
          k_value_error(arg2);
      }
      break;

    default:
      k_value_error(arg2);
  }

exit_compare_values:
  if (dismiss) {
    k_dismiss();
    k_dismiss();
  }
  switch (mode) {
    case TEST_EQ:
      return eq;
    case TEST_GT:
      return gt;
  }

  return gt || eq; /* TEST_GE */
}

/* END-CODE */
