/* OP_ARITH.C
 * Arithmetic opcodes
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
 * 06Feb22 gwb Changed comparisions of LONG_MIN to INT32_MIN in op_dec().
 *             The comparision to LONG_MIN would always be true due to the
 *             fact that LONG_MIN is out of range for the int32_t data type.
 * 
 * 29Feb20 gwb Changed LONG_MAX to INT32_MAX.  When building for a 64 bit 
 *             platform, the LONG_MAX constant overflows the size of the
 *             int32_t variable type.  This change needed to be made across
 *             the entire code base.
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 *
 * START-HISTORY (OpenQM):
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 23 Jan 07  2.4-19 Reworked op_rnd() to improve results.
 * 04 Oct 06  2.4-14 0522 op_int() did not apply INTPREC when converting a
 *                   floating point value.
 * 31 Jul 06  2.4-10 0506 Use of GetNum() macro must explicitly unset the
 *                   numeric_array_allowed flag if it has been set as the macro
 *                   may not call k_get_num() where it is normally unset.
 * 14 Apr 06  2.4-1 INT() should return integerised float for overlarge values.
 * 30 Dec 04  2.1-0 Added op_iadd(), op_imul(), op_isub(0 and op_scale() for
 *                  correlatives. Reworked op_idiv() to allow large result.
 * 20 Sep 04  2.0-2 Use message handler.
 * 20 Sep 04  2.0-2 Use dynamic pcode loader.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * op_abs         ABS
 * op_acos        ACOS
 * op_add         ADD
 * op_asin        ASIN
 * op_atan        ATAN
 * op_cos         COS
 * op_dec         DEC
 * op_div         DIV
 * op_exp         EXP
 * op_iadd        IADD
 * op_idiv        IDIV
 * op_imul        IMUL
 * op_isub        ISUB
 * op_inc         INC
 * op_int         INT
 * op_ln          LN
 * op_mod         MOD
 * op_mul         MUL
 * op_neg         NEG
 * op_pwr         PWR
 * op_rem         REM
 * op_rnd         RND
 * op_sin         SIN
 * op_sqrt        SQRT
 * op_sub         SUB
 * op_sum         SUM
 * op_summation   SUMMATION
 * op_tan         TAN
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include "config.h"
#include "options.h"

#include <math.h>
#include <time.h>
#include <stdint.h>

extern int32_t tens[];   /* Defined in op_oconv.c */
extern double rounding[]; /* Defined in k_funcs.c */

#define Radians(d) (d / 57.29577951)
#define Degrees(d) (d * 57.29577951)

/* ======================================================================
   op_abs()  -  Get absolute value of a number                            */

void op_abs() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Number                     | Absolute form of number     |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;

  process.numeric_array_allowed = TRUE;
  descr = e_stack - 1;
  GetNum(descr);
  process.numeric_array_allowed = FALSE; /* 0506 */

  switch (descr->type) {
    case INTEGER:
      descr->data.value = labs(descr->data.value);
      break;

    case FLOATNUM:
      descr->data.float_value = fabs(descr->data.float_value);
      break;

    default:
      k_num_array1(op_abs);
      break;
  }
}

/* ======================================================================
   op_acos()  -  ACOS function                                            */

void op_acos() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Number                     | Float (angle in degrees)    |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;

  process.numeric_array_allowed = TRUE;
  descr = e_stack - 1;
  k_get_float(descr);

  if (descr->type == FLOATNUM) {
    descr->data.float_value = Degrees(acos(descr->data.float_value));
  } else {
    k_num_array1(op_acos);
  }
}

/* ======================================================================
   op_add()  -  Add                                                       */

void op_add() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Number B                   | Number A + B                |
     |-----------------------------|-----------------------------|
     |  Number A                   |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* arg1;
  DESCRIPTOR* arg2;
  int32_t new_int;

  process.numeric_array_allowed = TRUE;
  arg1 = e_stack - 1;
  GetNum(arg1);

  process.numeric_array_allowed = TRUE;
  arg2 = e_stack - 2;
  GetNum(arg2);
  process.numeric_array_allowed = FALSE; /* 0506 */

  /* Add values, placing result in arg2 */

  switch (arg1->type) {
    case INTEGER:
      switch (arg2->type) {
        case INTEGER: /* Both items are integers */
          new_int = arg1->data.value + arg2->data.value;
          if (((arg1->data.value ^ arg2->data.value) >= 0)
              /* Signs of original values are both the same. We must check
                 for overflow of the result.                               */
              && ((arg1->data.value ^ new_int) < 0)) /* Overflow */
          {
            /* Convert descriptor to a FLOATNUM */
            arg2->type = FLOATNUM;
            arg2->data.float_value =
                (double)(arg1->data.value) + arg2->data.value;
          } else /* no overflow */
          {
            arg2->data.value = new_int;
          }
          break;

        case FLOATNUM: /* arg1 is INTEGER, arg2 is FLOATNUM */
          arg2->data.float_value += arg1->data.value;
          break;

        case STRING: /* arg1 is INTEGER, arg2 is numeric array */
          k_num_array2(op_add, 0);
          break;
      }
      break;

    case FLOATNUM:
      switch (arg2->type) {
        case INTEGER: /* arg1 is FLOATNUM, arg2 is INTEGER */
          arg2->type = FLOATNUM;
          arg2->data.float_value =
              arg1->data.float_value + (double)(arg2->data.value);
          break;

        case FLOATNUM: /* Both args are FLOATNUM */
          arg2->data.float_value += arg1->data.float_value;
          break;

        case STRING: /* arg1 is FLOATNUM, arg2 is numeric array */
          k_num_array2(op_add, 0);
          break;
      }
      break;

    case STRING: /* Arg 1 is numeric array */
      k_num_array2(op_add, 0);
      break;
  }

  /* Dismiss arg1 descriptor */

  k_pop(1);
}

/* ======================================================================
   op_asin()  -  ASIN function                                            */

void op_asin() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Number                     | Float (angle in degrees)    |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;

  process.numeric_array_allowed = TRUE;
  descr = e_stack - 1;
  k_get_float(descr);

  if (descr->type == FLOATNUM) {
    descr->data.float_value = Degrees(asin(descr->data.float_value));
  } else {
    k_num_array1(op_asin);
  }
}

/* ======================================================================
   op_atan()  -  ATAN function                                            */

void op_atan() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Number                     | Float (angle in degrees)    |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;

  process.numeric_array_allowed = TRUE;
  descr = e_stack - 1;
  k_get_float(descr);

  if (descr->type == FLOATNUM) {
    descr->data.float_value = Degrees(atan(descr->data.float_value));
  } else {
    k_num_array1(op_atan);
  }
}

/* ======================================================================
   op_cos()  -  COS function                                              */

void op_cos() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Number                     | Float                       |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;

  process.numeric_array_allowed = TRUE;
  descr = e_stack - 1;
  k_get_float(descr);

  if (descr->type == FLOATNUM) {
    descr->data.float_value = cos(Radians(descr->data.float_value));
  } else {
    k_num_array1(op_cos);
  }
}

/* ======================================================================
   op_dec()  -  Decrement value                                           */

void op_dec() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  ADDR to target variable    |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* arg;

  arg = e_stack - 1;
  while (arg->type == ADDR)
    arg = arg->data.d_addr;
  GetNum(arg);
  arg->flags &= ~DF_CHANGE;

  if (arg->type == INTEGER) {
    if (arg->data.value > INT32_MIN) {
      arg->data.value--;
    } else /* Must convert to float */
    {
      arg->type = FLOATNUM;
      arg->data.float_value = ((double)INT32_MIN) - 1;
    }
  } else {
    arg->data.float_value -= 1.0;
  }

  k_pop(1);
}

/* ======================================================================
   op_div()  -  Divide                                                    */

void op_div() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Number B                   | Number A / B                |
     |-----------------------------|-----------------------------|
     |  Number A                   |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* arg1;
  DESCRIPTOR* arg2;
  int32_t new_int;

  process.numeric_array_allowed = TRUE;
  arg1 = e_stack - 1;
  GetNum(arg1);

  process.numeric_array_allowed = TRUE;
  arg2 = e_stack - 2;
  GetNum(arg2);
  process.numeric_array_allowed = FALSE; /* 0506 */

  /* Divide arg2 by arg1, placing result in arg2 */

  switch (arg1->type) {
    case INTEGER:
      if (arg1->data.value == 0)
        goto div_zero;

      switch (arg2->type) {
        case INTEGER: /* Both items are integers */
          new_int = arg2->data.value / arg1->data.value;
          if ((new_int * arg1->data.value) !=
              arg2->data
                  .value) { /* Fractional result  -  convert descriptor to a FLOATNUM */
            arg2->type = FLOATNUM;
            arg2->data.float_value =
                (double)(arg2->data.value) / arg1->data.value;
          } else /* integer result */
          {
            arg2->data.value = new_int;
          }
          break;

        case FLOATNUM: /* Arg1 is integer, arg2 is float */
          arg2->data.float_value /= arg1->data.value;
          break;

        case STRING: /* Arg1 is integer, arg2 is numeric array */
          k_num_array2(op_div, 1);
          break;
      }
      break;

    case FLOATNUM:
      if (arg1->data.float_value == 0.0)
        goto div_zero;

      switch (arg2->type) {
        case INTEGER: /* Arg1 is float, arg2 is integer */
          arg2->type = FLOATNUM;
          arg2->data.float_value =
              (double)(arg2->data.value) / arg1->data.float_value;
          break;

        case FLOATNUM: /* Both args are float */
          arg2->data.float_value /= arg1->data.float_value;
          break;

        case STRING: /* Arg1 is float, arg2 is numeric array */
          k_num_array2(op_div, 1);
          break;
      }
      break;

    case STRING: /* Arg1 is numeric array */
      k_num_array2(op_div, 1);
      break;
  }

  /* Dismiss arg1 descriptor */

  k_pop(1);
  return;

div_zero:
  if (!Option(OptDivZeroWarning))
    k_div_zero_error(arg1);
  k_div_zero_warning(arg1);
  k_dismiss();
  k_dismiss();
  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = 0;
}

/* ======================================================================
   op_exp()  -  EXP function                                              */

void op_exp() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Number                     | Float                       |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;

  process.numeric_array_allowed = TRUE;
  descr = e_stack - 1;
  k_get_float(descr);

  if (descr->type == FLOATNUM) {
    descr->data.float_value = exp(descr->data.float_value);
  } else {
    k_num_array1(op_exp);
  }
}

/* ======================================================================
   op_iadd()  -  Integer add                                              */

void op_iadd() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Number B                   | Number A + B                |
     |-----------------------------|-----------------------------|
     |  Number A                   |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* arg1;
  DESCRIPTOR* arg2;
  int32_t new_int;

  process.numeric_array_allowed = TRUE;
  arg1 = e_stack - 1;
  GetNum(arg1);

  process.numeric_array_allowed = TRUE;
  arg2 = e_stack - 2;
  GetNum(arg2);
  process.numeric_array_allowed = FALSE; /* 0506 */

  /* Add values, placing result in arg2 */

  switch (arg1->type) {
    case INTEGER:
      switch (arg2->type) {
        case INTEGER: /* Both items are integers */
          new_int = arg1->data.value + arg2->data.value;
          if (((arg1->data.value ^ arg2->data.value) >= 0)
              /* Signs of original values are both the same. We must check
                 for overflow of the result.                               */
              && ((arg1->data.value ^ new_int) < 0)) /* Overflow */
          {
            /* Convert descriptor to a FLOATNUM */
            arg2->type = FLOATNUM;
            arg2->data.float_value =
                floor((double)(arg1->data.value) + arg2->data.value);
          } else /* no overflow */
          {
            arg2->data.value = new_int;
          }
          break;

        case FLOATNUM: /* arg1 is INTEGER, arg2 is FLOATNUM */
          arg2->data.float_value =
              floor(arg2->data.float_value + arg1->data.value);
          break;

        case STRING: /* arg1 is INTEGER, arg2 is numeric array */
          k_num_array2(op_iadd, 0);
          break;
      }
      break;

    case FLOATNUM:
      switch (arg2->type) {
        case INTEGER: /* arg1 is FLOATNUM, arg2 is INTEGER */
          arg2->type = FLOATNUM;
          arg2->data.float_value =
              floor(arg1->data.float_value + (double)(arg2->data.value));
          break;

        case FLOATNUM: /* Both args are FLOATNUM */
          arg2->data.float_value =
              floor(arg2->data.float_value + arg1->data.float_value);
          break;

        case STRING: /* arg1 is FLOATNUM, arg2 is numeric array */
          k_num_array2(op_iadd, 0);
          break;
      }
      break;

    case STRING: /* Arg 1 is numeric array */
      k_num_array2(op_iadd, 0);
      break;
  }

  /* Dismiss arg1 descriptor */

  k_pop(1);
}

/* ======================================================================
   op_idiv()  -  Integer divide                                           */

void op_idiv() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Number B                   | Number A / B                |
     |-----------------------------|-----------------------------|
     |  Number A                   |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* arg1;
  DESCRIPTOR* arg2;

  process.numeric_array_allowed = TRUE;
  arg1 = e_stack - 1;
  GetNum(arg1);

  process.numeric_array_allowed = TRUE;
  arg2 = e_stack - 2;
  GetNum(arg2);
  process.numeric_array_allowed = FALSE; /* 0506 */

  /* Divide arg2 by arg1, placing result in arg2 */

  switch (arg1->type) {
    case INTEGER:
      if (arg1->data.value == 0)
        goto div_zero;

      switch (arg2->type) {
        case INTEGER: /* Both items are integers */
          arg2->data.value = arg2->data.value / arg1->data.value;
          break;

        case FLOATNUM: /* Arg1 is integer, arg2 is float */
          arg2->data.float_value =
              floor(arg2->data.float_value / arg1->data.value);
          break;

        case STRING: /* Arg1 is integer, arg2 is numeric array */
          k_num_array2(op_idiv, 1);
          break;
      }
      break;

    case FLOATNUM:
      if (arg1->data.float_value == 0.0)
        goto div_zero;

      switch (arg2->type) {
        case INTEGER: /* Arg1 is float, arg2 is integer */
          arg2->type = FLOATNUM;
          arg2->data.float_value =
              floor((double)(arg2->data.value) / arg1->data.float_value);
          break;

        case FLOATNUM: /* Both args are float */
          arg2->data.float_value =
              floor(arg2->data.float_value / arg1->data.float_value);
          break;

        case STRING: /* Arg1 is float, arg2 is numeric array */
          k_num_array2(op_idiv, 1);
          break;
      }
      break;

    case STRING: /* Arg1 is numeric array */
      k_num_array2(op_idiv, 1);
      break;
  }

  /* Dismiss arg1 descriptor */

  k_pop(1);
  return;

div_zero:
  if (!Option(OptDivZeroWarning))
    k_div_zero_error(arg1);
  k_div_zero_warning(arg1);
  k_dismiss();
  k_dismiss();
  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = 0;
}

/* ======================================================================
   op_inc()  -  Increment value                                           */

void op_inc() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  ADDR to target variable    |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* arg;

  arg = e_stack - 1;
  while (arg->type == ADDR)
    arg = arg->data.d_addr;
  GetNum(arg);
  arg->flags &= ~DF_CHANGE;

  if (arg->type == INTEGER) {
    if (arg->data.value < INT32_MAX) {
      arg->data.value++;
    } else { /* Must convert to float */
      arg->type = FLOATNUM;
      arg->data.float_value = ((double)INT32_MAX) + 1;
      // this appears to force an overflow error via k_get_int().
    }
  } else {
    arg->data.float_value += 1.0;
  }

  k_pop(1);
}

/* ======================================================================
   op_int()  -  Convert number at top of stack to integer                 */

void op_int() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Number                     | Integer                     |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;
  double f;

  process.numeric_array_allowed = TRUE;
  descr = e_stack - 1;
  GetNum(descr);
  process.numeric_array_allowed = FALSE; /* 0506 */

  switch (descr->type) {
    case INTEGER:
      break;
    case FLOATNUM:
      if (pcfg.intprec) /* 0522 */
      {
        if (descr->data.float_value < 0) {
          descr->data.float_value -= rounding[pcfg.intprec - 1];
        } else {
          descr->data.float_value += rounding[pcfg.intprec - 1];
        }
      }
      f = floor(fabs(descr->data.float_value));
      descr->data.float_value = (descr->data.float_value < 0) ? -f : f;
      break;
    case STRING:
      k_num_array1(op_int);
      break;
  }
}

/* ======================================================================
   op_imul()  -  Integer multiply                                         */

void op_imul() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Number B                   | Number A * B                |
     |-----------------------------|-----------------------------|
     |  Number A                   |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* arg1;
  DESCRIPTOR* arg2;
  int32_t new_int;

  process.numeric_array_allowed = TRUE;
  arg1 = e_stack - 1;
  GetNum(arg1);

  process.numeric_array_allowed = TRUE;
  arg2 = e_stack - 2;
  GetNum(arg2);
  process.numeric_array_allowed = FALSE; /* 0506 */

  /* Multiply values, placing result in arg2 */

  switch (arg1->type) {
    case INTEGER:
      switch (arg2->type) {
        case INTEGER: /* Both items are integers */
          new_int = arg1->data.value * arg2->data.value;
          if ((arg2->data.value != 0) &&
              ((new_int / arg2->data.value) != arg1->data.value)) /* Overflow */
          {
            /* Convert descriptor to a FLOATNUM */
            arg2->type = FLOATNUM;
            arg2->data.float_value =
                floor((double)(arg1->data.value) * arg2->data.value);
          } else /* no overflow */
          {
            arg2->data.value = new_int;
          }
          break;

        case FLOATNUM: /* Arg1 is integer, arg2 is float */
          arg2->data.float_value =
              floor(arg2->data.float_value * arg1->data.value);
          break;

        case STRING: /* Arg1 is integer, arg2 is numeric array */
          k_num_array2(op_imul, 0);
          break;
      }
      break;

    case FLOATNUM:
      switch (arg2->type) {
        case INTEGER: /* Arg1 is float, arg2 is integer */
          arg2->type = FLOATNUM;
          arg2->data.float_value =
              floor(arg1->data.float_value * arg2->data.value);
          break;

        case FLOATNUM: /* Both args are float */
          arg2->data.float_value =
              floor(arg2->data.float_value * arg1->data.float_value);
          break;

        case STRING: /* Arg1 is float, arg2 is numeric array */
          k_num_array2(op_imul, 0);
          break;
      }
      break;

    case STRING: /* Arg1 is numeric array */
      k_num_array2(op_imul, 0);
      break;
  }

  /* Dismiss arg1 descriptor */

  k_pop(1);
}

/* ======================================================================
   op_isub()  -  Integer subtract                                         */

void op_isub() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Number B                   | Number A - B                |
     |-----------------------------|-----------------------------|
     |  Number A                   |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* arg1;
  DESCRIPTOR* arg2;
  int32_t new_int;

  process.numeric_array_allowed = TRUE;
  arg1 = e_stack - 1;
  GetNum(arg1);

  process.numeric_array_allowed = TRUE;
  arg2 = e_stack - 2;
  GetNum(arg2);
  process.numeric_array_allowed = FALSE; /* 0506 */

  /* Subtract arg1 from arg2, placing result in arg2 */

  switch (arg1->type) {
    case INTEGER:
      switch (arg2->type) {
        case INTEGER: /* Both items are integers */
          new_int = arg2->data.value - arg1->data.value;
          if (((arg1->data.value ^ arg2->data.value) < 0)
              /* Signs of original values are different. We must check for
                overflow of the result.                                       */
              && ((arg2->data.value ^ new_int) < 0)) /* Overflow */
          {
            /* Convert descriptor to a FLOATNUM */
            arg2->type = FLOATNUM;
            arg2->data.float_value =
                floor((double)(arg2->data.value) - arg1->data.value);
          } else /* no overflow */
          {
            arg2->data.value = new_int;
          }
          break;

        case FLOATNUM: /* Arg1 is integer, arg2 is float */
          arg2->data.float_value =
              floor(arg2->data.float_value - arg1->data.value);
          break;

        case STRING: /* Arg1 is integer, arg2 is numeric array */
          k_num_array2(op_isub, 0);
          break;
      }
      break;

    case FLOATNUM:
      switch (arg2->type) {
        case INTEGER: /* Arg1 is float, arg2 is integer */
          arg2->type = FLOATNUM;
          arg2->data.float_value =
              floor((double)(arg2->data.value) - arg1->data.float_value);
          break;

        case FLOATNUM: /* Both args are float */
          arg2->data.float_value =
              floor(arg2->data.float_value - arg1->data.float_value);
          break;

        case STRING: /* Arg1 is float, arg2 is numeric array */
          k_num_array2(op_isub, 0);
          break;
      }
      break;

    case STRING: /* Arg1 is numeric array */
      k_num_array2(op_isub, 0);
      break;
  }

  /* Dismiss arg1 descriptor */

  k_pop(1);
}

/* ======================================================================
   op_ln()  -  LN function                                                */

void op_ln() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Number                     | Float                       |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;

  process.numeric_array_allowed = TRUE;
  descr = e_stack - 1;
  k_get_float(descr);

  if (descr->type == FLOATNUM) {
    if (descr->data.float_value < 0.0)
      k_error(sysmsg(1222));

    descr->data.float_value = log(descr->data.float_value);
  } else {
    k_num_array1(op_ln);
  }
}

/* ======================================================================
   op_mod()  -  Form modulus                                              */

void op_mod() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Number Y                   |  Result                     |
     |-----------------------------|-----------------------------|
     |  Number X                   |                             |
     |=============================|=============================|

     mod(x,y) = if y = 0 then x else x - (y * floor(x/y))
 */

  DESCRIPTOR* arg_x;
  DESCRIPTOR* arg_y;
  double x;
  double y;
  double z;
  int32_t xx;
  int32_t yy;
  int32_t zz;

  arg_y = e_stack - 1;
  GetNum(arg_y);

  arg_x = e_stack - 2;
  GetNum(arg_x);

  if ((arg_x->type == INTEGER) && (arg_y->type == INTEGER)) {
    xx = arg_x->data.value;
    yy = arg_y->data.value;

    if (yy == 0)
      zz = xx;
    else if ((xx ^ yy) & 0x80000000L) /* Signs differ */
    {
      zz = xx - (yy * ((xx - (yy - 1)) / yy));
    } else {
      zz = xx - (yy * (xx / yy));
    }

    k_pop(1);
    arg_x->data.value = zz;
  } else /* At least one is a FLOATNUM */
  {
    if (arg_x->type == INTEGER)
      k_get_float(arg_x);
    else if (arg_y->type == INTEGER)
      k_get_float(arg_y);

    x = arg_x->data.float_value;
    y = arg_y->data.float_value;

    if (y == 0.0)
      z = x;
    else
      z = x - (y * floor(x / y));

    if (z == (int32_t)z) {
      k_pop(2);
      InitDescr(e_stack, INTEGER);
      (e_stack++)->data.value = (int32_t)z;
    } else {
      k_pop(1);
      arg_x->data.float_value = z;
    }
  }
}

/* ======================================================================
   op_mul()  -  Multiply                                                  */

void op_mul() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Number B                   | Number A * B                |
     |-----------------------------|-----------------------------|
     |  Number A                   |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* arg1;
  DESCRIPTOR* arg2;
  int32_t new_int;

  process.numeric_array_allowed = TRUE;
  arg1 = e_stack - 1;
  GetNum(arg1);

  process.numeric_array_allowed = TRUE;
  arg2 = e_stack - 2;
  GetNum(arg2);
  process.numeric_array_allowed = FALSE; /* 0506 */

  /* Multiply values, placing result in arg2 */

  switch (arg1->type) {
    case INTEGER:
      switch (arg2->type) {
        case INTEGER: /* Both items are integers */
          new_int = arg1->data.value * arg2->data.value;
          if ((arg2->data.value != 0) &&
              ((new_int / arg2->data.value) != arg1->data.value)) /* Overflow */
          {
            /* Convert descriptor to a FLOATNUM */
            arg2->type = FLOATNUM;
            arg2->data.float_value =
                (double)(arg1->data.value) * arg2->data.value;
          } else /* no overflow */
          {
            arg2->data.value = new_int;
          }
          break;

        case FLOATNUM: /* Arg1 is integer, arg2 is float */
          arg2->data.float_value *= arg1->data.value;
          break;

        case STRING: /* Arg1 is integer, arg2 is numeric array */
          k_num_array2(op_mul, 0);
          break;
      }
      break;

    case FLOATNUM:
      switch (arg2->type) {
        case INTEGER: /* Arg1 is float, arg2 is integer */
          arg2->type = FLOATNUM;
          arg2->data.float_value = arg1->data.float_value * arg2->data.value;
          break;

        case FLOATNUM: /* Both args are float */
          arg2->data.float_value *= arg1->data.float_value;
          break;

        case STRING: /* Arg1 is float, arg2 is numeric array */
          k_num_array2(op_mul, 0);
          break;
      }
      break;

    case STRING: /* Arg1 is numeric array */
      k_num_array2(op_mul, 0);
      break;
  }

  /* Dismiss arg1 descriptor */

  k_pop(1);
}

/* ======================================================================
   op_neg()  -  Negate number at top of stack                             */

void op_neg() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Number                     |  Number                     |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;

  process.numeric_array_allowed = TRUE;
  descr = e_stack - 1;
  GetNum(descr);
  process.numeric_array_allowed = FALSE; /* 0506 */

  switch (descr->type) {
    case INTEGER:
      descr->data.value = -(descr->data.value);
      break;

    case FLOATNUM:
      descr->data.float_value = -(descr->data.float_value);
      break;

    case STRING:
      k_num_array1(op_neg);
      break;
  }
}

/* ======================================================================
   op_pwr()  -  PWR opcode                                                */

void op_pwr() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Number B                   |  Float A ^ B                |
     |-----------------------------|-----------------------------|
     |  Number A                   |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* arg1;
  DESCRIPTOR* arg2;

  process.numeric_array_allowed = TRUE;
  arg2 = e_stack - 1;
  k_get_float(arg2);

  process.numeric_array_allowed = TRUE;
  arg1 = e_stack - 2;
  k_get_float(arg1);

  if ((arg1->type == FLOATNUM) && (arg2->type == FLOATNUM)) {
    arg1->data.float_value =
        pow(arg1->data.float_value, arg2->data.float_value);
  } else /* Numeric array */
  {
    k_num_array2(op_pwr, 1);
  }

  /* Dismiss arg2 descriptor */

  k_pop(1);
}

/* ======================================================================
   op_quotient()  -  Quotient                                             */

void op_quotient() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Number B                   | Int(Number A / B)           |
     |-----------------------------|-----------------------------|
     |  Number A                   |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* arg1;
  DESCRIPTOR* arg2;
  int32_t result;

  arg1 = e_stack - 1;
  GetNum(arg1);

  arg2 = e_stack - 2;
  GetNum(arg2);

  /* Divide arg2 by arg1, placing result in arg2 */

  switch (arg1->type) {
    case INTEGER:
      if (arg1->data.value == 0)
        goto div_zero;

      switch (arg2->type) {
        case INTEGER: /* Both items are integers */
          result = arg2->data.value / arg1->data.value;
          break;

        case FLOATNUM: /* Arg1 is integer, arg2 is float */
          result = (int32_t)(arg2->data.float_value / arg1->data.value);
          break;
      }
      break;

    case FLOATNUM:
      if (arg1->data.float_value == 0.0)
        goto div_zero;

      switch (arg2->type) {
        case INTEGER: /* Arg1 is float, arg2 is integer */
          result = (int32_t)((double)(arg2->data.value) / arg1->data.float_value);
          break;

        case FLOATNUM: /* Both args are float */
          result = (int32_t)(arg2->data.float_value / arg1->data.float_value);
          break;
      }
      break;
  }

  /* Dismiss arg1 descriptor */

  k_pop(1);
  InitDescr(arg2, INTEGER);
  arg2->data.value = result;
  return;

div_zero:
  if (!Option(OptDivZeroWarning))
    k_div_zero_error(arg1);
  k_div_zero_warning(arg1);
  k_dismiss();
  k_dismiss();
  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = 0;
}

/* ======================================================================
   op_rdiv()  -  Integer divide with rounding                             */

void op_rdiv() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Number B                   | Integer                     |
     |-----------------------------|-----------------------------|
     |  Number A                   |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* arg1;
  DESCRIPTOR* arg2;
  double v1;
  double v2;

  arg1 = e_stack - 1;
  k_get_float(arg1);
  v1 = arg1->data.float_value;

  arg2 = e_stack - 2;
  k_get_float(arg2);
  v2 = arg2->data.float_value;

  /* Divide arg2 by arg1, placing result in arg2 */

  if (v1 == 0.0)
    k_div_zero_error(arg1);

  if ((v1 >= 0) == (v2 >= 0))
    arg2->data.value = (int32_t)((v2 / v1) + 0.5);
  else
    arg2->data.value = (int32_t)((v2 / v1) - 0.5);
  arg2->type = INTEGER;

  /* Dismiss arg1 descriptor */

  k_pop(1);
}

/* ======================================================================
   op_rem()  -  Form remainder                                            */

void op_rem() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Number Y                   |  Result                     |
     |-----------------------------|-----------------------------|
     |  Number X                   |                             |
     |=============================|=============================|

     rem(x,y) = sign(x) * mod(abs(x), abs(y))
 */

  DESCRIPTOR* arg_x;
  DESCRIPTOR* arg_y;
  double x;
  double y;
  double z;
  int32_t xx;
  int32_t yy;
  int32_t zz;

  process.numeric_array_allowed = TRUE;
  arg_y = e_stack - 1;
  GetNum(arg_y);

  process.numeric_array_allowed = TRUE;
  arg_x = e_stack - 2;
  GetNum(arg_x);
  process.numeric_array_allowed = FALSE; /* 0506 */

  if ((arg_x->type == INTEGER) && (arg_y->type == INTEGER)) {
    xx = arg_x->data.value;
    yy = arg_y->data.value;

    if (yy == 0)
      zz = xx;
    else
      zz = xx % yy;

    k_pop(1);
    arg_x->data.value = zz;
  } else if ((arg_x->type == STRING) || (arg_y->type == STRING)) {
    k_num_array2(op_rem, 0);
    k_pop(1);
  } else /* At least one is a FLOATNUM */
  {
    if (arg_x->type == INTEGER)
      k_get_float(arg_x);
    else if (arg_y->type == INTEGER)
      k_get_float(arg_y);

    x = fabs(arg_x->data.float_value);
    y = fabs(arg_y->data.float_value);

    if (y == 0.0)
      z = x;
    else
      z = x - (y * floor(x / y));

    if (arg_x->data.float_value < 0.0)
      z = -z;

    if (z == (int32_t)z) {
      k_pop(2);
      InitDescr(e_stack, INTEGER);
      (e_stack++)->data.value = (int32_t)z;
    } else {
      k_pop(1);
      arg_x->data.float_value = z;
    }
  }
}

/* ======================================================================
   op_rnd()  -  RND function                                              */

void op_rnd() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Number                     | Integer                     |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;
  int32_t n;
  int32_t r;

  process.numeric_array_allowed = TRUE;
  descr = e_stack - 1;
  GetInt(descr);
  process.numeric_array_allowed = FALSE; /* 0506 */

  if (descr->type == INTEGER) {
    n = descr->data.value;
#if RAND_MAX == 0x7FFFFFFF
    r = rand();
#else
    r = ((((int32_t)rand()) << 15) + rand()) & 0x7FFFFFFF;
#endif
    descr->data.value = (n < 0) ? -(r % -n) : (r % n);
  } else /* Numeric array */
  {
    k_num_array1(op_rnd);
  }
}

/* ======================================================================
   op_scale()  -  Scale number                                            */

void op_scale() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Scale factor               | Result (integer)            |
     |-----------------------------|-----------------------------|
     |  Number                     |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;
  int factor;

  /* Get scaling factor */

  descr = e_stack - 1;
  GetNum(descr);
  factor = descr->data.value;

  if (factor > 0) {
    /* Get source data */

    process.numeric_array_allowed = TRUE;
    descr = e_stack - 2;
    GetNum(descr);
    process.numeric_array_allowed = FALSE; /* 0506 */

    switch (descr->type) {
      case INTEGER:
        descr->data.value /= tens[factor];
        break;

      case FLOATNUM:
        descr->data.float_value = floor(descr->data.float_value / tens[factor]);
        break;

      case STRING: /* Arg1 is numeric array */
        k_num_array2(op_scale, 0);
        break;
    }
  }

  k_pop(1);
}

/* ======================================================================
   op_seed()  -  RANDOMIZE                                                */

void op_seed() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Number or null string      |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;

  descr = e_stack - 1;
  k_get_value(descr);

  if ((descr->type == STRING) && (descr->data.str.saddr == NULL)) {
    srand(time(NULL));
  } else {
    GetInt(descr);
    srand(descr->data.value);
  }
  k_pop(1);
}

/* ======================================================================
   op_sin()  -  SIN function                                              */

void op_sin() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Number                     | Float                       |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;

  process.numeric_array_allowed = TRUE;
  descr = e_stack - 1;
  k_get_float(descr);

  if (descr->type == FLOATNUM) {
    descr->data.float_value = sin(Radians(descr->data.float_value));
  } else {
    k_num_array1(op_sin);
  }
}

/* ======================================================================
   op_sqrt()  -  SQRT function                                            */

void op_sqrt() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Number                     | Float                       |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;

  process.numeric_array_allowed = TRUE;
  descr = e_stack - 1;
  k_get_float(descr);

  if (descr->type == FLOATNUM) {
    if (descr->data.float_value < 0.0)
      k_error(sysmsg(1223));

    descr->data.float_value = sqrt(descr->data.float_value);
  } else {
    k_num_array1(op_sqrt);
  }
}

/* ======================================================================
   op_sub()  -  Subtract                                                  */

void op_sub() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Number B                   | Number A - B                |
     |-----------------------------|-----------------------------|
     |  Number A                   |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* arg1;
  DESCRIPTOR* arg2;
  int32_t new_int;

  process.numeric_array_allowed = TRUE;
  arg1 = e_stack - 1;
  GetNum(arg1);

  process.numeric_array_allowed = TRUE;
  arg2 = e_stack - 2;
  GetNum(arg2);
  process.numeric_array_allowed = FALSE; /* 0506 */

  /* Subtract arg1 from arg2, placing result in arg2 */

  switch (arg1->type) {
    case INTEGER:
      switch (arg2->type) {
        case INTEGER: /* Both items are integers */
          new_int = arg2->data.value - arg1->data.value;
          if (((arg1->data.value ^ arg2->data.value) < 0)
              /* Signs of original values are different. We must check for
                overflow of the result.                                       */
              && ((arg2->data.value ^ new_int) < 0)) /* Overflow */
          {
            /* Convert descriptor to a FLOATNUM */
            arg2->type = FLOATNUM;
            arg2->data.float_value =
                (double)(arg2->data.value) - arg1->data.value;
          } else /* no overflow */
          {
            arg2->data.value = new_int;
          }
          break;

        case FLOATNUM: /* Arg1 is integer, arg2 is float */
          arg2->data.float_value -= arg1->data.value;
          break;

        case STRING: /* Arg1 is integer, arg2 is numeric array */
          k_num_array2(op_sub, 0);
          break;
      }
      break;

    case FLOATNUM:
      switch (arg2->type) {
        case INTEGER: /* Arg1 is float, arg2 is integer */
          arg2->type = FLOATNUM;
          arg2->data.float_value =
              (double)(arg2->data.value) - arg1->data.float_value;
          break;

        case FLOATNUM: /* Both args are float */
          arg2->data.float_value -= arg1->data.float_value;
          break;

        case STRING: /* Arg1 is float, arg2 is numeric array */
          k_num_array2(op_sub, 0);
          break;
      }
      break;

    case STRING: /* Arg1 is numeric array */
      k_num_array2(op_sub, 0);
      break;
  }

  /* Dismiss arg1 descriptor */

  k_pop(1);
}

/* ======================================================================
   op_sum()  -  Sum multi-valued dynamic array, raising one level         */

void op_sum() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  String to sum              |  Result                     |
     |=============================|=============================|
 */

  k_recurse(pcode_sum, 1); /* Execute recursive code */
}

/* ======================================================================
   op_summation()  -  Sum all elements of a numeric array                 */

void op_summation() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  String to sum              |  Result                     |
     |=============================|=============================|
 */

  k_recurse(pcode_sumall, 1); /* Execute recursive code */
}

/* ======================================================================
   op_tan()  -  TAN function                                              */

void op_tan() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Number                     | Float                       |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;

  process.numeric_array_allowed = TRUE;
  descr = e_stack - 1;
  k_get_float(descr);

  if (descr->type == FLOATNUM) {
    descr->data.float_value = tan(Radians(descr->data.float_value));
  } else {
    k_num_array1(op_tan);
  }
}

/* END-CODE */
