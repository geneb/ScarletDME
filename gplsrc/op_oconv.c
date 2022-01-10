/* OP_OCONV.C
 * OCONV opcode
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
 * 09Jan22 gwb Fixed some format specifier warnings.
 *
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 *
 * START-HISTORY:
 * 04 Oct 07  2.6-5 0564 Julian date conversions could have stray characters
 *                  at the end.
 * 04 Oct 07  2.6-5 0563 float_conversion() assumed double to be little endian.
 * 13 Sep 07  2.6-3 Added SPACE.MCT option.
 * 03 Aug 07  2.5-7 Added B64 conversion code.
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 13 Feb 07  2.4-20 Added ordinal date conversion.
 * 18 Jan 07  0536 Julian date conversion left justified value less than 10.
 * 20 Dec 06  2.4-18 0533 S conversion did not dismiss original data when
 *                   returning a field.
 * 27 Nov 06  2.4-17 Added R conversion.
 * 29 Aug 06  2.4-12 Added C conversion.
 * 05 Jul 06  2.4-8 Allow MD2$, as alternative to MD2,$
 * 28 Jun 06  2.4-5 0498 S conversion failed if value2 omitted.
 * 22 Mar 06  2.3-9 Added MCAN and MC/AN conversions.
 * 20 Mar 06  2.3-8 Added ISO week date conversion elements, WI and YI.
 * 27 Feb 06  2.3-8 Changed behaviour of MD2< to insert a space before positive
 *                  values as in UV. The previous behaviour was as for PI/open
 *                  which did not insert a space.
 * 22 Dec 05  2.3-3 Allow quoted delimiter in MT conversion so that MT'h' can be
 *                  distinguished from MTH.
 * 27 Oct 05  2.2-15 0428 date_conversion() should accept float source.
 * 20 Sep 05  2.2-11 0410 MRZ4# was returning null for zero, not four spaces.
 * 09 Sep 05  2.2-10 0407 E code in date conversion should toggle European
 *                   format, not set it.
 * 07 Sep 05  2.2-10 Reject stray characters at end of date conversion code.
 *                   Invalid date conversion code should return original
 *                   data, not null string and status 0.
 * 25 Aug 05  2.2-8 Added handling of format codes in MD/ML/MR.
 * 24 Aug 05  2.2-8 Changed default_date_conversion to hold entire code without
 *                  stripping leading D.
 * 09 Jun 05  2.2-1 Added truncate argument to ftoa() and T option to MD code.
 * 09 Jun 05  2.2-1 0367 MD02 was applying incorrect rounding.
 * 13 May 05  2.1-15 Added error arg to s_make_contiguous().
 * 11 May 05  2.1-13 0352 Use ftoa() to avoid unbiased rounding of printf().
 * 29 Dec 04  2.1-0 Added P (pattern matching) conversion.
 * 09 Dec 04  2.1-0 0291 Use GetUnsignedInt() in radix conversions.
 * 14 Nov 04  2.0-10 Added IFL and IFS conversions.
 * 03 Nov 04  2.0-9 Added support for MR%5 style field width and padding.
 * 20 Sep 04  2.0-2 Use message handler.
 * 20 Sep 04  2.0-2 Use dynamic pcode loader.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * OCONV():
 *
 *    B         Boolean (Y or N)
 *
 *    B64       Base64 encoding
 *
 *    C;1;2;3   Concatenation
 *
 *    D         Date conversion
 *              D {y}{c}{fmt[f1,f2,f3]}
 *
 *    G{skip}df Group conversion.
 *
 *    IFx       Convert 8 bytes to floating point value
 *    ILx       Convert 4 byte low byte first binary integer to number
 *    ISx       Convert 2 byte low byte first binary integer to number
 *              x = L, H or omitted to specify byte ordering.
 *
 *    L[n[,m]]  String length checks
 *
 *    MCD[X]    Convert decimal to hexadecimal
 *    MCL       Convert to lower case
 *    MCU       Convert to upper case
 *    MCX[D]    Convert hexadecimal to decimal
 *
 *    MD        Masked decimal conversion
 *    ML        Masked decimal conversion, left justified
 *    MR        Masked decimal conversion, right justified
 *              MD d [f] [x[c]]
 *              d = decimal places (0 - 9)
 *              f = scale factor (0 - 9), defaults to same as d
 *              x = field width
 *              c = padding char, defaults to space.
 *
 *    MO        Convert integer to octal string
 *
 *    MT        Time conversion
 *              MT [H] [S] [c]
 *              H = use 12 hour format with AM or PM suffix
 *              S = include seconds
 *              c = separator, defaults to colon
 *
 *    MX        Convert integer to hexadecimal string
 *
 *    P(xx)     Pattern matching
 *
 *    Rn,m{;n,m...}  Range test
 *
 *    S;v1;v2   Substitution
 *
 *    T{n,}m    Substring extraction
 *
 *    Tfile;cv;i;o   TRANS()
 *
 *    <f,v,s>   Extract field, value or subvalue
 *
 *  FMT():
 *    Format strings:
 *
 *       [length] [fill.char] L [n] [$] [,] [Z] [B] [mask]
 *                            R                  C
 *                            T                  D
 *                            C                  E
 *                                               M
 *                                               N
 *
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#define NO_INLINE /* insert_commas() fails if inline code used */
#include "qm.h"
#include "keys.h"
#include "config.h"
#include "options.h"
#include "syscom.h"

#include <math.h>
#include <time.h>

#define MAX_FMT_STRING_LEN 256   /* Max length of FMT() control string */
#define MAX_CONV_STRING_LEN 256  /* Max length of OCONV() control string */
#define MAX_DATA_STRING_LEN 4096 /* Max length of data in FMT() */

extern char* month_names[];
extern char* day_names[];

int32_t tens[] = {1,      10,      100,      1000,      10000,
                  100000, 1000000, 10000000, 100000000, 1000000000};

void op_extract(void);
void op_substr(void);
void op_substre(void);
void op_xtd(void);

void k_trace(void);
STRING_CHUNK* b64encode(STRING_CHUNK* str);

/* Public local functions */
int32_t concatenation_conversion(char* src_ptr);
int32_t conv_dtx(char* p);
int32_t conv_xtd(char* p);
int32_t group_conversion(char* p);
int32_t mc_conversion(char* p);
int32_t pattern_match_conversion(char* src_ptr);
int32_t range_conversion(char* src_ptr);
int32_t substitution_conversion(char* src_ptr);
int32_t substring_conversion(char* src_ptr);

/* Private local functions */

Private int32_t base64_conversion(void);
Private int32_t boolean_conversion(void);
Private int32_t date_conversion(char* src_ptr);
Private int32_t field_extraction(char* src_ptr);
Private int32_t float_conversion(char* p);
Private int32_t radix_conversion(char* src_ptr, int16_t radix);
Private int32_t integer_conversion(char* src_ptr, int16_t bytes);
Private int32_t masked_decimal_conversion(char* src_ptr);
Private int32_t time_conversion(char* src_ptr);
Private void insert_commas(char* s, char thousands, char decimal);
Private void insert_text_breaks(char* s,
                                int16_t* s_len,
                                int16_t width,
                                char pad_char,
                                bool word_align);
Private int apply_mask(char* src,
                       int src_len,
                       char* tgt,
                       char* mask,
                       bool right_justify,
                       char fill_char);

/* ======================================================================
   op_fmt()  -  Format string                                             */

void op_fmt() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Format description         |  Result string              |
     |-----------------------------|-----------------------------| 
     |  Source string              |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* src_descr; /* Source string */

  DESCRIPTOR* fmt_descr; /* Format string */
  char format_string[MAX_FMT_STRING_LEN + 1];
  bool ok;

  int32_t field_width = -1;
  char justification;
  char fill_char;
  int dp = -1;
  int16_t scale_factor = 0;
  char currency[8 + 1] = "";
  bool commas = FALSE;
  bool null_zero = FALSE; /* Zero -> null */
  char sign_handling = '\0';
  bool is_negative;
  bool is_integer;
  char* mask;
  double fvalue;
  int32_t int_value;

  char s[MAX_DATA_STRING_LEN + 1];
  int16_t s_len; /* Actual length of source */
  char z[MAX_DATA_STRING_LEN + 1];
  int16_t t_len; /* Target length remaining */

  int16_t pad;
  char c;
  char* p;
  char* q;
  int16_t n;

  process.status = 2;

  /* Get format string */

  fmt_descr = e_stack - 1;
  ok = k_get_c_string(fmt_descr, format_string, MAX_FMT_STRING_LEN) >= 0;
  k_dismiss();

  if (!ok)
    goto exit_op_fmt;

  /* Process the format string */

  p = format_string;

  if (*p == 'D') /* Not really a format at all - It's a date conversion */
  {
    k_get_value(e_stack - 1); /* 0140 */
    process.status = date_conversion(p + 1);
    goto exit_op_fmt;
  }

  /* Field width */

  if (IsDigit(*p)) {
    field_width = 0;
    do {
      field_width = (field_width * 10) + (*(p++) - '0');
    } while (IsDigit(*p));
  }

  /* Fill character */

  switch (UpperCase(*p)) {
    case '"': /* Quoted fill character */
    case '\'':
      c = *(p++);
      if (((fill_char = *(p++)) == '\0') || (*(p++) != c))
        goto exit_op_fmt;
      break;

    case 'C': /* Default fill character */
    case 'L':
    case 'R':
    case 'T':
    case 'U':
    case '\0':
      fill_char = ' ';
      break;

    default:
      fill_char = *(p++);
      break;
  }

  /* Justification */

  c = UpperCase(*p);
  switch (c) {
    case 'C':
    case 'L':
    case 'R':
    case 'T':
    case 'U':
      justification = c;
      p++;
      break;

    default:
      goto exit_op_fmt;
  }

  /* Decimal places */

  if (IsDigit(*p)) {
    dp = *(p++) - '0';

    /* 0088 Scale factor */

    if (IsDigit(*p))
      scale_factor = (*(p++) - '0') - process.program.precision;
    else
      scale_factor = 0;
  }

  /* Conversion elements */

  while (*p != '\0') {
    switch (UpperCase(*p)) {
      case '$': /* Currency symbol required. (0183 moved) */
        strcpy(currency, national.currency);
        p++;
        break;

      case ',': /* Thousands separator required */
        commas = TRUE;
        p++;
        break;

      case 'B': /* Special sign handling */
      case 'C':
      case 'D':
      case 'E':
      case 'M':
      case 'N':
        sign_handling = UpperCase(*p);
        p++;
        break;

      case 'Z': /* Null zero */
        null_zero = TRUE;
        p++;
        break;

      default:
        goto end_conversion;
    }
  }

end_conversion:

  /* Mask */

  mask = p;

  /* Handle special case of zero field width */

  if (field_width == 0) {
    process.status = 0;
    k_dismiss(); /* Dismiss source string */
    InitDescr(e_stack, STRING);
    (e_stack++)->data.str.saddr = NULL;
    goto exit_op_fmt;
  }

  /* Get source data */

  process.status = 1;

  src_descr = e_stack - 1;
  k_get_value(src_descr);
  switch (src_descr->type) {
    case STRING:
      /* If we have specified any options that relate to numeric values
         only, try to convert the string to a number. Otherwise, process
         the data as a string.                                            */

      /* 0227 reworked */

      if ((dp >= 0) || null_zero || commas) /* Numeric type option present */
      {
        if ((src_descr->data.str.saddr == NULL) && Option(OptPickNull)) {
          /* Null string with PICK.NULL option set. Process as a string */
          dp = -1;
          null_zero = FALSE;
          commas = FALSE;
          if ((s_len = k_get_c_string(src_descr, s, MAX_DATA_STRING_LEN)) < 0) {
            goto exit_op_fmt;
          }
          break;
        }

        /* Try to convert the string to a number */

        if (!k_str_to_num(src_descr)) /* Failed - Process as a string */
        {
          if ((s_len = k_get_c_string(src_descr, s, MAX_DATA_STRING_LEN)) < 0) {
            goto exit_op_fmt;
          }
          break;
        }
      } else /* No numeric options present - Process as a string */
      {
        if ((s_len = k_get_c_string(src_descr, s, MAX_DATA_STRING_LEN)) < 0) {
          goto exit_op_fmt;
        }
        break;
      }
      /* **** FALL THROUGH TO TREAT AS A NUMBER **** */

    case INTEGER:
    case FLOATNUM:
      /* The source data is a number */

      p = s;

      is_integer = (src_descr->type == INTEGER);
      if (is_integer && (scale_factor == 0)) {
        int_value = src_descr->data.value;
        is_negative = int_value < 0;

        if ((sign_handling != '\0') && is_negative) {
          if (sign_handling == 'E')
            *(p++) = '<';
          int_value = labs(int_value);
        }

        strcpy(p, currency);
        p += strlen(currency); /* Position for number output */

        if (null_zero && (int_value == 0))
          *p = '\0';
        else if (dp >= 0)
          ftoa((double)int_value, dp, FALSE, p); /* 0352 */
        else
          sprintf(p, "%d", int_value);
      } else /* Either is float or is integer with scale factor */
      {
        if (is_integer)
          fvalue = (double)src_descr->data.value;
        else
          fvalue = src_descr->data.float_value;

        is_negative = fvalue < 0;

        if ((sign_handling != '\0') && is_negative) {
          if (sign_handling == 'E')
            *(p++) = '<';
          fvalue = fabs(fvalue);
        }

        strcpy(p, currency);
        p += strlen(currency); /* Position for number output */

        /* 0088 Apply scale factor */

        if (scale_factor > 0)
          fvalue /= tens[scale_factor];
        else if (scale_factor < 0)
          fvalue *= tens[-scale_factor];

        if (null_zero && (fabs(fvalue) <= pcfg.fltdiff)) {
          *p = '\0';
        } else if (dp >= 0) /* Convert with specific number of decimal places */
        {
          ftoa(fvalue, dp, FALSE, p); /* 0352 */
        } else                        /* Use default precision */
        {
          ftoa(fvalue, process.program.precision, FALSE, p); /* 0352 */

          /* Remove trailing zeros after the decimal point (0102, 0103) */

          if ((scale_factor == 0) && (strchr(p, '.') != NULL)) {
            for (q = p + strlen(p) - 1; q != p; q--) {
              if (*q == '.') {
                *q = '\0';
                break;
              }
              if (*q != '0')
                break;
              *q = '\0';
            }
          }
        }

        if (national.decimal != '.')
          strrep(p, '.', national.decimal);
      }

      if (commas)
        insert_commas(p, national.thousands, national.decimal);

      /* Special sign handling */

      switch (sign_handling) {
        case '\0':
          break;
        case 'B':
          if (Option(OptCRDBUpcase))
            strcat(s, (is_negative) ? "DB" : "  ");
          else
            strcat(s, (is_negative) ? "db" : "  ");
          break;
        case 'C':
          if (Option(OptCRDBUpcase))
            strcat(s, (is_negative) ? "CR" : "  ");
          else
            strcat(s, (is_negative) ? "cr" : "  ");
          break;
        case 'D':
          if (Option(OptCRDBUpcase))
            strcat(s, (is_negative) ? "  " : "DB");
          else
            strcat(s, (is_negative) ? "  " : "db");
          break;
        case 'E':
          strcat(s, (is_negative) ? ">" : " ");
          break;
        case 'M':
          strcat(s, (is_negative) ? "-" : " ");
          break;
      }

      s_len = strlen(s);
      break;

    default:
      k_error(sysmsg(1202));
  }

  p = s;

  /* The source data either was a string or has been converted to a string. In
    either case, the start of the string is addressed by p.  The length is
    given by s_len as the string could contain null characters.              */

  /* Apply masking */

  if (*mask != '\0') {
    t_len = apply_mask(p, s_len, s, mask, justification == 'R', fill_char);
    p = s;
    s_len = t_len;
  }

  /* Apply justification */

  if (field_width > 0) {
    pad = (int16_t)(field_width - s_len); /* Padding length */

    switch (justification) {
      case 'C':
        if (pad > 0) {
          n = pad / 2;
          if (n)
            memset(z, fill_char, n); /* Leading fill chars */
          memcpy(z + n, p, s_len);   /* Data */
          n = pad - n;
          if (n)
            memset(z + field_width - n, fill_char, n);
          s_len += pad;
          p = (char*)memcpy(s, z, s_len);
          *(p + s_len) = '\0';
        }

        if (s_len > field_width) {
          insert_text_breaks(p, &s_len, (int16_t)field_width, fill_char, FALSE);
        }
        break;

      case 'L':
      case 'U':
        if (pad > 0) {
          memset(p + s_len, fill_char, pad);
          s_len += pad;
          *(p + s_len) = '\0';
        }

        if (s_len > field_width) {
          insert_text_breaks(p, &s_len, (int16_t)field_width, fill_char, FALSE);
        }
        break;

      case 'R':
        if (pad > 0) {
          memset(z, fill_char, pad);
          memcpy(z + pad, p, s_len);
          s_len += pad;
          p = (char*)memcpy(s, z, s_len);
        }

        if (s_len > field_width) {
          insert_text_breaks(p, &s_len, (int16_t)field_width, fill_char, FALSE);
        }
        break;

      case 'T':
        insert_text_breaks(p, &s_len, (int16_t)field_width, fill_char, TRUE);
        break;
    }
  }

  k_dismiss(); /* Dismiss source string */
  k_put_string(p, s_len, e_stack++);

  process.status = 0;

exit_op_fmt:
  return;
}

/* ======================================================================
   op_oconv()  -  Output conversion                                       */

void op_oconv() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Conversion specification   |  Result string              |
     |-----------------------------|-----------------------------| 
     |  Source string              |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* conv_descr; /* Format string */
  DESCRIPTOR* src_descr;  /* Source string */
  char conv_string[MAX_CONV_STRING_LEN + 1];
  bool ok;

  char c;
  char* p;
  char* q;
  bool done = FALSE;

  /* Get conversion code */

  conv_descr = e_stack - 1;
  ok = k_get_c_string(conv_descr, conv_string, MAX_CONV_STRING_LEN) >= 0;
  k_dismiss();

  if (!ok)
    goto bad_conv_string;

  process.status = 0;

  /* Get the value of the item to be converted */

  src_descr = e_stack - 1;
  k_get_value(src_descr);

  //0227 if ((src_descr->type == STRING) && (src_descr->data.str.saddr == NULL))
  //0227  {
  //0227   if (UpperCase(conv_string[0]) != 'U') goto exit_op_oconv;
  //0227  }

  /* Process the conversion string */

  p = conv_string;

  do {
    q = strchr(p, VALUE_MARK);
    if (q == NULL)
      done = TRUE;
    else
      *q = '\0';

    while (*p == ' ')
      p++; /* Skip leading spaces */

    c = *(p++);
    switch (UpperCase(c)) {
      case '\0': /* Null conversion */
        break;

      case 'B': /* Boolean conversion */
        if (!strcmp(p, "64")) {
          process.status = base64_conversion();
        } else {
          process.status = boolean_conversion();
        }
        break;

      case 'C': /* Concatenation conversion */
        process.status = concatenation_conversion(p);
        break;

      case 'D': /* Date conversion */
        process.status = date_conversion(p);
        break;

      case 'G': /* Group conversion */
        process.status = group_conversion(p);
        break;

      case 'L': /* Length constraints */
        process.status = length_conversion(p);
        break;

      case 'M':
        switch (UpperCase(*p)) {
          case 'D': /* Masked decimal conversion */
          case 'L':
          case 'R':
            process.status = masked_decimal_conversion(p);
            break;

          case 'T': /* Time conversion */
            process.status = time_conversion(p);
            break;

          case 'B': /* Binary conversion */
            process.status = radix_conversion(++p, 2);
            break;

          case 'C': /* Case conversion (plus oddments) */
            c = *(++p);
            switch (UpperCase(c)) {
              case 'D': /* Decimal to hex */
                c = UpperCase(*(++p));
                if ((c != '\0') && (c != 'X'))
                  goto bad_conv_string;
                if (c == 'X')
                  p++;
                process.status = conv_dtx(p);
                break;
              case 'L':
                set_case(FALSE);
                break;
              case 'U':
                set_case(TRUE);
                break;
              case 'X': /* Hex to decimal */
                c = UpperCase(*(++p));
                if ((c != '\0') && (c != 'D'))
                  goto bad_conv_string;
                if (c == 'D')
                  p++;
                process.status = conv_xtd(p);
                break;
              default:
                mc_conversion(p);
                break;
            }
            break;

          case 'O': /* Octal conversion */
            process.status = radix_conversion(++p, 8);
            break;

          case 'X': /* Hexadecimal conversion */
            process.status = radix_conversion(++p, 16);
            break;

          default:
            goto bad_conv_string;
        }
        break;

      case 'I':
        switch (UpperCase(*(p++))) {
          case 'F': /* Float conversion */
            process.status = float_conversion(p);
            break;
          case 'L': /* Long integer conversion */
            process.status = integer_conversion(p, 4);
            break;
          case 'S': /* Short integer conversion */
            process.status = integer_conversion(p, 2);
            break;
          default:
            goto bad_conv_string;
        }
        break;

      case 'P':
        process.status = pattern_match_conversion(p);
        break;

      case 'R':
        process.status = range_conversion(p);
        break;

      case 'S':
        process.status = substitution_conversion(p);
        break;

      case 'T':
        if (strchr(p, ';') != NULL) /* Looks like a Tfile conversion */
        {
          /* Push conversion specification onto e-stack */
          k_put_c_string(p, e_stack++);

          /* Push oconv flag onto stack */
          InitDescr(e_stack, INTEGER);
          (e_stack++)->data.value = TRUE;

          k_recurse(pcode_tconv, 3); /* Execute recursive code */
        } else {
          process.status = substring_conversion(p);
        }
        break;

      case 'U': /* User defined */
        /* Push conversion specification onto e-stack */
        k_put_c_string(p, e_stack++);
        k_recurse(pcode_oconv, 2); /* Execute recursive code */
        break;

      case '<':
        process.status = field_extraction(p);
        break;

      default: /* Not recognised */
        goto bad_conv_string;
    }

    if (process.status != 0)
      goto exit_op_oconv;

    p = q + 1;
  } while (!done);

exit_op_oconv:
  return;

bad_conv_string:
  process.status = 2;
  goto exit_op_oconv;
}

/* ======================================================================
   base64_conversion()  -  B64 conversion                                 */

Private int32_t base64_conversion() {
  DESCRIPTOR* src_descr;
  STRING_CHUNK* str;

  src_descr = e_stack - 1;
  k_get_string(src_descr);

  str = b64encode(src_descr->data.str.saddr);
  k_dismiss();
  InitDescr(e_stack, STRING);
  (e_stack++)->data.str.saddr = str;

  return 0;
}

/* ======================================================================
   boolean_conversion()  -  True/False -> Y/N                             */

Private int32_t boolean_conversion() {
  DESCRIPTOR* src_descr;
  STRING_CHUNK* str;
  int16_t actual_size;

  /* Get source data */

  src_descr = e_stack - 1;
  GetBool(src_descr);

  str = s_alloc(1, &actual_size);
  str->data[0] = (src_descr->data.value != 0) ? 'Y' : 'N';
  str->string_len = 1; /* 0246 */
  str->bytes = 1;
  str->ref_ct = 1;

  k_pop(1);

  InitDescr(e_stack, STRING);
  (e_stack++)->data.str.saddr = str;

  return 0;
}

/* ======================================================================
   concatenation_conversion()  -  C conversion                            */

int32_t concatenation_conversion(char* p) {
  k_put_c_string(p, e_stack++);
  k_recurse(pcode_cconv, 2);
  return process.status;
}

/* ======================================================================
   date_conversion()  -  D conversion                                     */

Private int32_t date_conversion(char* p) {
  int32_t status = 2;
  DESCRIPTOR* src_descr;
  char s[100 + 1];

  s[0] = '\0';

  /* Convert the date to day, month, year components */

  src_descr = e_stack - 1;
  if ((src_descr->type == STRING) && (k_blank_string(src_descr))) {
    goto exit_date_conversion_ok;
  }

  if (src_descr->type == STRING)
    (void)k_str_to_num(src_descr);
  if (src_descr->type != INTEGER) {
    if (src_descr->type != FLOATNUM) {
      status = 1;
      goto exit_date_conversion;
    }
    GetInt(src_descr); /* 0428 */
  }

  if ((status = oconv_d(src_descr->data.value, p, s)) != 0)
    goto exit_date_conversion;

exit_date_conversion_ok:
  k_dismiss();
  k_put_c_string(s, e_stack);
  e_stack++;
  status = 0;

exit_date_conversion:
  return status;
}

/* ======================================================================
   group_conversion()  -  G conversion                                    */

int32_t group_conversion(char* p) {
  int32_t status = 2;
  DESCRIPTOR* src_descr;
  int16_t skip = 0;
  char delim;
  int16_t fields = 0;
  STRING_CHUNK* str;
  DESCRIPTOR result_descr;
  int16_t bytes_remaining;
  char* src;
  char* q;
  int16_t len;

  /* Extract optional skip count */

  while (IsDigit(*p)) {
    skip = skip * 10 + (*(p++) - '0');
  }

  /* Extract delimiter */

  if ((delim = *(p++)) == '\0')
    goto exit_group_conversion;
  if (IsMark(delim))
    goto exit_group_conversion;

  /* Extract field count */

  while (IsDigit(*p)) {
    fields = fields * 10 + (*(p++) - '0');
  }
  if (fields == 0)
    goto exit_group_conversion;

  /* Get source data */

  src_descr = e_stack - 1;
  k_get_string(src_descr);
  str = src_descr->data.str.saddr;

  status = 0; /* No errors from here onwards */

  /* Do the extraction */

  if (str == NULL)
    goto exit_group_conversion; /* Return source null string */

  /* Set up a string descriptor to receive the result */

  InitDescr(&result_descr, STRING);
  result_descr.data.str.saddr = NULL;
  ts_init(&(result_descr.data.str.saddr), 32);

  /* Walk the string looking for the start of this item */

  src = str->data;
  bytes_remaining = str->bytes;

  if (skip == 0)
    goto found_start;

  do {
    while (bytes_remaining) {
      q = (char*)memchr(src, delim, bytes_remaining);
      if (q == NULL)
        break; /* No delimiter in this chunk */

      bytes_remaining -= (q - src) + 1;
      src = q + 1;
      if (--skip == 0)
        goto found_start;
    }

    if ((str = str->next) == NULL) {
      goto found_end; /* We did not find the desired item */
    }

    src = str->data;
    bytes_remaining = str->bytes;
  } while (TRUE);

found_start:
  /* We have found the start of the item. Now copy the specified number
     of components to the result string.                                 */

  do {
    /* Look for delimiter in the current chunk */

    while (bytes_remaining) {
      q = (char*)memchr(src, delim, bytes_remaining);
      if (q != NULL) /* Found delimiter */
      {
        len = q - src;
        if (len)
          ts_copy(src, len); /* Copy up to delimiter */

        bytes_remaining -= len + 1;
        src = q + 1;
        if (--fields == 0)
          goto found_end;

        ts_copy_byte(delim); /* Copy delimiter */
      } else                 /* No delimiter - copy all to target */
      {
        ts_copy(src, bytes_remaining);
        break;
      }
    }

    if ((str = str->next) == NULL)
      break;

    src = str->data;
    bytes_remaining = str->bytes;
  } while (1);

found_end:
  ts_terminate();

  k_dismiss();
  *(e_stack++) = result_descr;

exit_group_conversion:
  return status;
}

/* ======================================================================
   length_conversion()  -  L conversion (input and output modes)          */

int32_t length_conversion(char* p) {
  int32_t status = 2;
  DESCRIPTOR* src_descr;
  int32_t p1 = 0;
  int32_t p2 = 0;
  STRING_CHUNK* str;
  int32_t len;

  /* Get source data */

  src_descr = e_stack - 1;
  k_get_string(src_descr);
  str = src_descr->data.str.saddr;
  len = (str == NULL) ? 0 : (str->string_len);

  if (IsDigit(*p)) /* p1 present */
  {
    do {
      p1 = (p1 * 10) + (*(p++) - '0');
    } while (IsDigit(*p));

    if (*p == ',') {
      p++;
      if (IsDigit(*p)) /* p2 present */
      {
        do {
          p2 = (p2 * 10) + (*(p++) - '0');
        } while (IsDigit(*p));
      }
    }
  }

  if (*p != '\0')
    goto exit_length_conversion; /* Format error */

  if (p1 == 0) /* No parameters or p1 = 0 - return string length */
  {
    k_dismiss();
    InitDescr(e_stack, INTEGER);
    (e_stack++)->data.value = len;
  } else if (((p2 == 0) && (len > p1)) ||
             ((p2 != 0) && ((len < p1) || (len > p2)))) {
    k_dismiss();
    InitDescr(e_stack, STRING);
    (e_stack++)->data.str.saddr = NULL;
  }

  status = 0;

exit_length_conversion:

  return status;
}

/* ======================================================================
   field_extraction()  -  <f,v,s> conversion                              */

Private int32_t field_extraction(char* p) {
  int32_t status = 2;
  int32_t f;
  int32_t v;
  int32_t sv;
  bool end_on_value_mark;
  bool end_on_subvalue_mark;
  DESCRIPTOR* src_descr; /* Source string */
  STRING_CHUNK* str_hdr;
  DESCRIPTOR result_descr; /* Result string */
  int16_t offset;
  int16_t bytes_remaining;
  int16_t len;
  char* q;
  register char c;

  /* Identify the field, value or subvalue to be extracted */

  f = 0;
  v = 0;
  sv = 0;

  if (!IsDigit(*p))
    goto exit_field_extraction;
  while (IsDigit(*p))
    f = (f * 10) + (*(p++) - '0');

  if (*p == ',') {
    p++;
    if (!IsDigit(*p))
      goto exit_field_extraction;
    while (IsDigit(*p))
      v = (v * 10) + (*(p++) - '0');

    if (*p == ',') {
      p++;
      if (!IsDigit(*p))
        goto exit_field_extraction;
      while (IsDigit(*p))
        sv = (sv * 10) + (*(p++) - '0');
    }
  }

  if (*(p++) != '>')
    goto exit_field_extraction;
  if (*p != '\0')
    goto exit_field_extraction;

  /* Do the extraction */

  if (v == 0) /* Extracting field */
  {
    end_on_value_mark = FALSE;
    end_on_subvalue_mark = FALSE;
    v = 1;
    sv = 1;
  } else if (sv == 0) /* Extracting value */
  {
    end_on_value_mark = TRUE;
    end_on_subvalue_mark = FALSE;
    sv = 1;
  } else /* Extracting subvalue */
  {
    end_on_value_mark = TRUE;
    end_on_subvalue_mark = TRUE;
  }

  src_descr = e_stack - 1;
  k_get_string(src_descr);
  str_hdr = src_descr->data.str.saddr;

  /* Set up a string descriptor on our C stack to receive the result */

  InitDescr(&result_descr, STRING);
  result_descr.data.str.saddr = NULL;
  ts_init(&result_descr.data.str.saddr, 32);

  if (find_item(str_hdr, f, v, sv, &str_hdr, &offset)) {
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
          if ((IsDelim(c)) &&
              ((c == FIELD_MARK) || ((c == VALUE_MARK) && end_on_value_mark) ||
               ((c == SUBVALUE_MARK) && end_on_subvalue_mark))) {
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

  k_release(src_descr); /* Release source string */
  InitDescr(src_descr, STRING);
  *src_descr = result_descr; /* Replace by result string */
  status = 0;

exit_field_extraction:

  return status;
}

/* ======================================================================
   float_conversion()  -  IF conversion                                   */

Private int32_t float_conversion(char* p) {
  DESCRIPTOR* descr;
  double flt;
  bool hi_byte_first;
  int16_t n;
  char* q;
  char* r;

  switch (*p) {
    case '\0':
#ifdef BIG_ENDIAN_SYSTEM
      hi_byte_first = TRUE;
#else
      hi_byte_first = FALSE;
#endif
      break;
    case 'L':
      hi_byte_first = FALSE;
      break;
    case 'H':
      hi_byte_first = TRUE;
      break;
    default:
      return 2;
  }

  descr = e_stack - 1;
  k_get_string(descr);
  if (descr->data.str.saddr == NULL)
    return 1;

  if (descr->data.str.saddr->ref_ct > 1) {
    descr->data.str.saddr = s_make_contiguous(descr->data.str.saddr, NULL);
  }

#ifdef BIG_ENDIAN_SYSTEM
  if (!hi_byte_first) /* 0563 */
#else
  if (hi_byte_first)
#endif
  {
    for (n = sizeof(double), q = (char*)&flt,
        r = descr->data.str.saddr->data + n;
         n--;) {
      *(q++) = *(--r);
    }
  } else {
    memcpy((char*)&flt, descr->data.str.saddr->data, sizeof(double));
  }

  k_release(descr);
  InitDescr(descr, FLOATNUM);
  descr->data.float_value = flt;

  return 0;
}

/* ======================================================================
   radix_conversion()  -  MB, MO and MX conversions                       */

Private int32_t radix_conversion(char* src, int16_t radix) {
  DESCRIPTOR* src_descr;
  u_int32_t value;
  int16_t shift;
  int16_t step;
  int32_t mask;
  int16_t n;
  int16_t k;
  int16_t digits; /* Digits per character in Mx0C */
  bool emit;
  char s[32 + 1];
  char* p;
  STRING_CHUNK* str_hdr;
  DESCRIPTOR tgt_descr;
  int bytes;

#define HexChar(d) ((char)((d) + (((d) > 9) ? 'A' - 10 : '0')))

  if (*src == '\0') /* MB, MO, MX */
  {
    src_descr = e_stack - 1;

    /* 0227 Return null string for null data */

    if ((src_descr->type == STRING) && (src_descr->data.str.saddr == NULL)) {
      return 0;
    }

    value = GetUnsignedInt(src_descr); /* 0291 */

    switch (radix) {
      case 2:
        shift = 31;
        step = 1;
        mask = 0x01;
        break;
      case 8:
        shift = 30;
        step = 3;
        mask = 0x07;
        break;
      case 16:
        shift = 28;
        step = 4;
        mask = 0x0F;
        break;
    }

    p = s;

    if (value == 0) {
      *(p++) = '0';
      *(p++) = '0';
    } else {
      emit = FALSE;
      do {
        n = (int16_t)((value >> shift) & mask);
        emit |= (n != 0);
        if (emit)
          *(p++) = HexChar(n);
        shift -= step;
      } while (shift >= 0);
    }
    *p = '\0';

    k_pop(1);
    k_put_c_string(s, e_stack);
    e_stack++;
  } else if ((memcmp(src, "0C", 2) == 0) ||
             (memcmp(src, "0c", 2) == 0)) { /* MB0C, MO0C, MX0C */
    src_descr = e_stack - 1;
    k_get_string(src_descr);
    str_hdr = src_descr->data.str.saddr;

    if (str_hdr != NULL) {
      InitDescr(&tgt_descr, STRING);
      tgt_descr.data.str.saddr = NULL;

      switch (radix) {
        case 2:
          shift = 7;
          step = 1;
          mask = 0x01;
          digits = 8;
          break;
        case 8:
          shift = 6;
          step = 3;
          mask = 0x07;
          digits = 3;
          break;
        case 16:
          shift = 4;
          step = 4;
          mask = 0x0F;
          digits = 2;
          break;
      }

      ts_init(&(tgt_descr.data.str.saddr), str_hdr->string_len * digits);
      do {
        p = str_hdr->data;
        bytes = str_hdr->bytes;
        while (bytes--) {
          n = *(p++);
          for (k = shift; k >= 0; k -= step) {
            ts_copy_byte(HexChar((n >> k) & mask));
          }
        }
      } while ((str_hdr = str_hdr->next) != NULL);
      (void)ts_terminate();

      k_dismiss();
      *(e_stack++) = tgt_descr;
    }
  } else {
    return 2;
  }

  return 0;
}

/* ======================================================================
   integer_conversion()  -  IL and IS conversions                         */

Private int32_t integer_conversion(char* src, int16_t bytes) {
  DESCRIPTOR* src_descr;
  u_int32_t value;
  u_char s[4 + 1];
  int16_t i;
  bool hi_byte_first;

  switch (*src) {
    case '\0':
#ifdef BIG_ENDIAN_SYSTEM
      hi_byte_first = TRUE;
#else
      hi_byte_first = FALSE;
#endif
      break;
    case 'L':
      hi_byte_first = FALSE;
      break;
    case 'H':
      hi_byte_first = TRUE;
      break;
    default:
      return 2;
  }

  src_descr = e_stack - 1;

  /* 0227 Return null string for null data */

  if ((src_descr->type == STRING) && (src_descr->data.str.saddr == NULL)) {
    return 0;
  }

  if (k_get_c_string(src_descr, (char*)s, 4) != bytes)
    return 1; /* Wrong length */

  value = 0;
  if (hi_byte_first) {
    for (i = 0; i < bytes; i++) {
      value = (value << 8) | s[i];
    }
  } else {
    for (i = 0; i < bytes; i++) {
      value |= ((u_int32_t)s[i]) << (i * 8);
    }
  }

  if ((bytes == 2) && (value & 0x8000L)) /* Extend sign */
  {
    value |= 0xFFFF0000L;
  }

  k_dismiss();
  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = (int32_t)value;

  return 0;
}

/* ======================================================================
   masked_decimal_conversion()  -  MD, ML and MR conversions              */

Private int32_t masked_decimal_conversion(char* p) {
  int32_t status = 2;
  DESCRIPTOR* src_descr;
  double value;
  int32_t int_value;
  char mode;            /* Conversion mode (D, L or R) */
  int16_t dp;           /* Decimal places */
  int16_t scale_factor; /* Scale factor */
  bool commas = FALSE;  /* Insert comma thousands separator? */
  char prefix[32 + 1] = "";
  char thousands;
  char decimal;
  char suffix[32] = "";
  int16_t neg; /* Action for negative values */
#define LEADING_MINUS 0
#define TRAILING_SIGN 1
#define TRAILING_MINUS 2
#define ANGLE_BRACKETS 3
#define ROUND_BRACKETS 4
#define TRAILING_CR 5
#define TRAILING_DB 6
  bool suppress_decimal_scale;
  bool null_zero;
  bool dp_present = FALSE;
  bool truncate = FALSE;
  int16_t width = 0;   /* Field width (0 implies not set) */
  int16_t padding = 0; /* Width of padding... */
  char pad_char = ' '; /* ...and character to use */
  char s[99 + 1];
  char z[99 + 1];
  bool is_negative;
  bool is_integer;
  int16_t i;
  int16_t n;
  char* q;
  char* r;
  char delim;
  char* mask = NULL;

  thousands = national.thousands;
  decimal = national.decimal;

  mode = *(p++); /* Fetch mode character (D, L, R) */
  mode = UpperCase(mode);

  src_descr = e_stack - 1;

  /* 0227 Return null string for null data */

  if ((src_descr->type == STRING) && (src_descr->data.str.saddr == NULL)) {
    if ((mode == 'D') || Option(OptPickNull))
      return 0;
  }

rescan:
  switch (src_descr->type) {
    case INTEGER:
      int_value = src_descr->data.value;
      is_integer = TRUE;
      break;

    case FLOATNUM:
      value = src_descr->data.float_value;
      is_integer = FALSE;
      dp_present = fabs(value - (int32_t)value) > pcfg.fltdiff;
      break;

    case STRING:
      //0227      if (src_descr->data.str.saddr == NULL) goto exit_masked_decimal_ok;

      if (!k_str_to_num(src_descr)) /* Not a number - return unconverted */
      {
        status = 1;
        goto exit_masked_decimal;
      }
      dp_present = src_descr->type == FLOATNUM;
      goto rescan;

    default:
      k_error(sysmsg(1202));
  }

  /* Get decimal places */

  dp = (IsDigit(*p)) ? (*(p++) - '0') : 0;

  /* Scale factor */

  if (IsDigit(*p))
    scale_factor = *(p++) - '0'; /* Scale factor present */
  else
    scale_factor = dp;

  while (1) {
    if (*p == ',') /* Comma separator? */
    {
      commas = TRUE;
      p++;
    } else if (*p == '$') /* Currency symbol */
    {
      strcpy(prefix, national.currency);
      p++;
    } else
      break;
  }

  if (*p == '[') {
    p++;
    while (*p == ' ')
      p++;                           /* Skip spaces */
    if ((*p == '"') || (*p == '\'')) /* Prefix */
    {
      for (delim = *(p++), q = prefix, n = 32; /* Collect prefix string */
           (*p != delim) && (*p != '\0') && n--; *(q++) = *(p++))
        ;
      *q = '\0';
      while ((*p != delim) && (*p != '\0'))
        p++; /* Ensure at delimiter */
      if (*p == delim)
        p++; /* Skip delimiter */
      while (*p == ' ')
        p++; /* Skip spaces */
    } else {
      for (q = prefix, n = 32; /* Collect prefix string */
           (*p != ']') && (*p != ',') && (*p != '\0') && n--; *(q++) = *(p++))
        ;
      *q = '\0';
      while ((*p != ']') && (*p != ',') && (*p != '\0'))
        p++; /* Ensure at delimiter */
    }

    if (*p == ',') {
      p++; /* Skip comma */
      while (*p == ' ')
        p++;                           /* Skip spaces */
      if ((*p == '"') || (*p == '\'')) /* Thousands delimiter */
      {
        delim = *(p++);
        if ((*p) != delim) {
          thousands = *(p++);
          commas = TRUE;
        }
        while ((*p != delim) && (*p != '\0'))
          p++; /* Ensure at delimiter */
        if (*p == delim)
          p++; /* Skip delimiter */
        while (*p == ' ')
          p++; /* Skip spaces */
      } else if ((*p != ']') && (*p != ',') && (*p != '\0')) {
        thousands = *(p++);
        commas = TRUE;
        while (*p == ' ')
          p++; /* Skip spaces */
      }
    }

    if (*p == ',') {
      p++; /* Skip comma */
      while (*p == ' ')
        p++;                           /* Skip spaces */
      if ((*p == '"') || (*p == '\'')) /* Decimal point symbol */
      {
        delim = *(p++);
        if ((*p) != delim)
          decimal = *(p++);
        while ((*p != delim) && (*p != '\0'))
          p++; /* Ensure at delimiter */
        if (*p == delim)
          p++; /* Skip delimiter */
        while (*p == ' ')
          p++; /* Skip spaces */
      } else if ((*p != ']') && (*p != ',') && (*p != '\0')) {
        decimal = *(p++);
        while (*p == ' ')
          p++; /* Skip spaces */
      }
    }

    if (*p == ',') {
      p++; /* Skip comma */
      while (*p == ' ')
        p++;                           /* Skip spaces */
      if ((*p == '"') || (*p == '\'')) /* Suffix */
      {
        for (delim = *(p++), q = suffix, n = 32; /* Collect suffix string */
             (*p != delim) && (*p != '\0') && n--; *(q++) = *(p++))
          ;
        *q = '\0';
        while ((*p != delim) && (*p != '\0'))
          p++; /* Ensure at delimiter */
        if (*p == delim)
          p++; /* Skip delimiter */
        while (*p == ' ')
          p++; /* Skip spaces */
      } else {
        for (q = suffix, n = 32; /* Collect suffix string */
             (*p != ']') && (*p != ',') && (*p != '\0') && n--; *(q++) = *(p++))
          ;
        *q = '\0';
        while ((*p != ']') && (*p != ',') && (*p != '\0'))
          p++; /* Ensure at delimiter */
      }
    }

    if (*p != ']')
      goto exit_masked_decimal;
    p++;
  }

  /* Negative value action */

  switch (UpperCase(*p)) {
    case '+':
      neg = TRAILING_SIGN;
      p++;
      break;
    case '-':
      neg = TRAILING_MINUS;
      p++;
      break;
    case '<':
      neg = ANGLE_BRACKETS;
      p++;
      break;
    case '(':
      neg = ROUND_BRACKETS;
      p++;
      break;
    case 'C':
      neg = TRAILING_CR;
      p++;
      break;
    case 'D':
      neg = TRAILING_DB;
      p++;
      break;
    default:
      neg = LEADING_MINUS;
      break;
  }

  /* Suppress scaling if decimal point present? */

  if ((suppress_decimal_scale = (UpperCase(*p) == 'P')) != 0)
    p++;

  /* Represent zero value by null string? */

  if ((null_zero = (UpperCase(*p) == 'Z')) != 0)
    p++;

  /* Truncate rather than round? */

  if ((truncate = (UpperCase(*p) == 'T')) != 0)
    p++;

  /* Field width */

  if (IsDigit(*p)) /* Field width present (max two digits) */
  {
    width = *(p++) - '0';
    if (IsDigit(*p))
      width = (width * 10) + (*(p++) - '0');
    if (*p != '\0')
      pad_char = *(p++);
  } else if ((*p != '\0') && (strchr("#*%", *p) !=
                              NULL)) { /* Format code style fill specfication */
    mask = p;
  }

  /* Do the conversion */

  if (is_integer && (dp == scale_factor)) {
    if ((is_negative = (int_value < 0)) != 0)
      int_value = -int_value;

    if (null_zero && (int_value == 0)) {
      z[0] = '\0'; /* 0410 */
      goto apply_formatting;
    }

    if (dp) {
      /* Convert as integer then insert decimal point at correct position */

      Ltoa(int_value, z, 10);
      i = strlen(z) - dp; /* Digits to left of decimal point */
      q = z;
      r = s;
      if (i <= 0) /* Must insert leading zeros */
      {
        *(r++) = '0';
        *(r++) = decimal;
        while (i++ < 0)
          *(r++) = '0';
        strcpy(r, q);
      } else {
        memcpy(r, q, i);
        r += i;
        q += i;
        *(r++) = decimal;
        strcpy(r, q);
      }
    } else /* No decimal point required */
    {
      Ltoa(int_value, s, 10);
    }
  } else {
    if (is_integer)
      value = (double)int_value;

    if (scale_factor && !(suppress_decimal_scale && dp_present)) {
      value /= tens[scale_factor];
    }

    if (null_zero && (fabs(value) < pcfg.fltdiff)) {
      z[0] = '\0'; /* 0410 */
      goto apply_formatting;
    }

    if ((is_negative = (value < 0.0)) != 0)
      value = -value;

    /* Convert value to string */

    ftoa(value, dp, truncate, s); /* 0352, 0367 */

    if (decimal != '.') {
      q = strchr(s, '.');
      if (q != NULL)
        *q = decimal;
    }
  }

  /* Insert thousands delimiters if required */

  if (commas)
    insert_commas(s, thousands, decimal);

  /* Insert prefix and handle negative value indications */

  q = z;
  switch (neg) {
    case LEADING_MINUS:
      if (is_negative)
        *(q++) = '-';
      if (prefix[0] != '\0') {
        strcpy(q, prefix);
        q += strlen(prefix);
      }
      strcpy(q, s);
      break;

    case TRAILING_SIGN:
      if (prefix[0] != '\0') {
        strcpy(q, prefix);
        q += strlen(prefix);
      }
      strcpy(q, s);
      strcat(q, (is_negative) ? "-" : "+");
      break;

    case TRAILING_MINUS:
      if (prefix[0] != '\0') {
        strcpy(q, prefix);
        q += strlen(prefix);
      }
      strcpy(q, s);
      strcat(q, (is_negative) ? "-" : " ");
      break;

    case ANGLE_BRACKETS:
      *(q++) = (is_negative) ? '<' : ' ';
      if (prefix[0] != '\0') {
        strcpy(q, prefix);
        q += strlen(prefix);
      }
      strcpy(q, s);
      strcat(q, (is_negative) ? ">" : " ");
      break;

    case ROUND_BRACKETS:
      *(q++) = (is_negative) ? '(' : ' ';
      if (prefix[0] != '\0') {
        strcpy(q, prefix);
        q += strlen(prefix);
      }
      strcpy(q, s);
      strcat(q, (is_negative) ? ")" : " ");
      break;

    case TRAILING_CR:
      if (prefix[0] != '\0') {
        strcpy(q, prefix);
        q += strlen(prefix);
      }
      strcpy(q, s);
      if (Option(OptCRDBUpcase))
        strcat(q, (is_negative) ? "CR" : "  ");
      else
        strcat(q, (is_negative) ? "cr" : "  ");
      break;

    case TRAILING_DB:
      if (prefix[0] != '\0') {
        strcpy(q, prefix);
        q += strlen(prefix);
      }
      strcpy(q, s);
      if (Option(OptCRDBUpcase))
        strcat(q, (is_negative) ? "DB" : "  ");
      else
        strcat(q, (is_negative) ? "db" : "  ");
      break;
  }

  /* Add suffix */

  if (suffix[0] != '\0')
    strcat(q, suffix);

apply_formatting:

  /* Pad or truncate to field width */

  n = strlen(z);
  if (width) {
    if (n > width) /* Truncate if over-long */
    {
      z[width] = '\0';
      n = width;
    }
    padding = width - n;
  } else {
    width = n;
    padding = 0;
    if (mask != NULL) {
      n = apply_mask(z, n, z, mask, mode == 'R', ' ');
    }
  }

  /* Form result string */

  q = s;
  if (mode == 'R')
    while (padding-- > 0)
      *(q++) = pad_char;

  memcpy(q, z, n);
  q += n;

  if (mode == 'L')
    while (padding-- > 0)
      *(q++) = pad_char;

  *q = '\0';

  k_pop(1);
  k_put_c_string(s, e_stack);
  e_stack++;

  status = 0;

exit_masked_decimal:

  return status;
}

/* ======================================================================
   mc_conversion()  -  MCA, MCN, MC/A, MC/N, MCT, MCP conversions         */

int32_t mc_conversion(char* p) {
  int16_t mode;
  int32_t status = 2;
  DESCRIPTOR* src_descr; /* Source string */
  STRING_CHUNK* str_hdr;
  DESCRIPTOR result_descr; /* Result string */
  int16_t bytes;
  register char c;
  bool upcase_next;

  /* Get conversion mode */

  UpperCaseString(p);
  if (strcmp(p, "A") == 0)
    mode = 1;
  else if (strcmp(p, "N") == 0)
    mode = 2;
  else if (strcmp(p, "/A") == 0)
    mode = 3;
  else if (strcmp(p, "/N") == 0)
    mode = 4;
  else if (strcmp(p, "T") == 0) {
    mode = 5;
    upcase_next = TRUE;
  } else if (strcmp(p, "P") == 0)
    mode = 6;
  else if (strcmp(p, "AN") == 0)
    mode = 7;
  else if (strcmp(p, "/AN") == 0)
    mode = 8;
  else
    goto exit_mc_conversion;

  src_descr = e_stack - 1;
  k_get_string(src_descr);
  str_hdr = src_descr->data.str.saddr;

  /* Set up a string descriptor on our C stack to receive the result */

  InitDescr(&result_descr, STRING);
  result_descr.data.str.saddr = NULL;
  ts_init(&result_descr.data.str.saddr, 32);

  while (str_hdr != NULL) {
    bytes = str_hdr->bytes;
    p = str_hdr->data;

    switch (mode) {
      case 1: /* Delete all non-alpha */
        while (bytes--) {
          c = *(p++);
          if (IsAlpha(c))
            ts_copy_byte(c);
        }
        break;

      case 2: /* Delete all non-numeric */
        while (bytes--) {
          c = *(p++);
          if (IsDigit(c))
            ts_copy_byte(c);
        }
        break;

      case 3: /* Delete all alpha */
        while (bytes--) {
          c = *(p++);
          if (!IsAlpha(c))
            ts_copy_byte(c);
        }
        break;

      case 4: /* Delete all numeric */
        while (bytes--) {
          c = *(p++);
          if (!IsDigit(c))
            ts_copy_byte(c);
        }
        break;

      case 5: /* Capital intial */
        if (Option(OptSpaceMCT)) {
          while (bytes--) {
            c = *(p++);
            if (upcase_next)
              c = UpperCase(c);
            else
              c = LowerCase(c);
            upcase_next = (c == ' ');
            ts_copy_byte(c);
          }
        } else {
          while (bytes--) {
            c = *(p++);
            if (IsAlpha(c)) {
              if (upcase_next)
                c = UpperCase(c);
              else
                c = LowerCase(c);
              upcase_next = FALSE;
            } else {
              upcase_next = TRUE;
            }
            ts_copy_byte(c);
          }
        }
        break;

      case 6: /* Replace non-printing characters by . */
        while (bytes--) {
          c = *(p++);
          if ((c == ' ') || IsGraph(c))
            ts_copy_byte(c);
          else
            ts_copy_byte('.');
        }
        break;

      case 7: /* Delete all non-alphanumeric */
        while (bytes--) {
          c = *(p++);
          if (IsAlnum(c))
            ts_copy_byte(c);
        }
        break;

      case 8: /* Delete all alphanumeric */
        while (bytes--) {
          c = *(p++);
          if (!IsAlnum(c))
            ts_copy_byte(c);
        }
        break;
    }
    str_hdr = str_hdr->next;
  }

  ts_terminate();

  k_release(src_descr);      /* Release source string */
  *src_descr = result_descr; /* Replace by result string */
  status = 0;

exit_mc_conversion:
  return status;
}

/* ======================================================================
   pattern_match_conversion()  -  P conversion                            */

int32_t pattern_match_conversion(char* p) {
  DESCRIPTOR* descr;
  char* q;
  char src_string[MAX_MATCHED_STRING_LEN + 1];
  bool match;
  char c;

  descr = e_stack - 1;
  if (k_get_c_string(descr, src_string, MAX_MATCHED_STRING_LEN) < 0) {
    k_error(sysmsg(1604));
  }

  /* Try matching against each value mark delimited template */

  do { /* Outer loop - extract templates */
    if (*(p++) != '(')
      return 2;
    q = strchr(p, ')');
    if (q == NULL)
      return 2;
    *q = '\0';

    match = match_template(src_string, p, 0, 1);
    if (match)
      break; /* Match found */

    p = q + 1;
    c = *(p++);
  } while ((c == ';') || (c == '/'));

  if (!match) {
    k_dismiss();
    InitDescr(e_stack, STRING);
    (e_stack++)->data.str.saddr = NULL;
  }

  return 0;
}

/* ======================================================================
   range_conversion()  -  R conversion                                    */

int32_t range_conversion(char* p) {
  int32_t status = 2;
  DESCRIPTOR* descr;
  char s[256 + 1];
  int16_t n;
  bool neg;
  int lo;
  int hi;
  int32_t value;

  descr = e_stack - 1;
  if ((n = k_get_c_string(descr, s, 256)) < 0)
    return 1;
  if (!strnint(s, n, &value))
    return 1;

  if (*p != '\0') {
    do {
      neg = (*p == '-');
      if (neg)
        p++;
      if (!IsDigit(*p))
        goto exit_range_conversion;
      lo = 0;
      while (IsDigit(*p))
        lo = (lo * 10) + (*(p++) - '0');
      if (neg)
        lo = -lo;

      if (*(p++) != ',')
        goto exit_range_conversion;

      neg = (*p == '-');
      if (neg)
        p++;
      if (!IsDigit(*p))
        goto exit_range_conversion;
      hi = 0;
      while (IsDigit(*p))
        hi = (hi * 10) + (*(p++) - '0');
      if (neg)
        hi = -hi;

      if ((value >= lo) && (value <= hi))
        return 0;

      if (*p == '\0')
        break;
      if ((*p != ';') && (*p != '/'))
        goto exit_range_conversion;
      p++;
    } while (1);

    k_dismiss();
    InitDescr(e_stack, STRING);
    (e_stack++)->data.str.saddr = NULL;
  }

exit_range_conversion:
  return status;
}

/* ======================================================================
   substitution_conversion()  -  S conversion                             */

int32_t substitution_conversion(char* p) {
  int32_t status = 2;
  DESCRIPTOR* src_descr;
  STRING_CHUNK* str;
  bool seen_sign = FALSE;
  bool seen_digit = FALSE;
  bool seen_dp = FALSE;
  int fno;
  bool use_value_1;
  int16_t i;
  char* q;
  char c;

  /* Position to value 1 in conversion string */

  if ((p = strchr(p, ';')) == NULL)
    goto exit_substitution_conversion;
  p++;

  /* Get the value */

  src_descr = e_stack - 1;
  switch (src_descr->type) {
    case STRING:
      /* This one is a bit awkward as we cannot simply use k_str_to_num()
         because we may need the data in its original form.                */

      if ((str = src_descr->data.str.saddr) != NULL) {
        use_value_1 = TRUE;
        do {
          for (i = str->bytes, q = str->data; i > 0; i--) {
            c = *(q++);
            if (IsDigit(c)) {
              if (c != '0')
                goto use_first_value;
              seen_digit = TRUE;
            } else {
              switch (c) {
                case ' ': /* Note: embedded spaces are ignored */
                  break;

                case '+':
                case '-':
                  if (seen_digit || seen_sign)
                    goto use_first_value;
                  seen_sign = TRUE;
                  break;

                case '.':
                  if (seen_dp)
                    goto use_first_value;
                  seen_dp = TRUE;
                  break;

                default:
                  goto use_first_value;
              }
            }
          }

          str = str->next;
        } while (str != NULL);
      }
      use_value_1 = FALSE;

    use_first_value:
      break;

    case INTEGER:
      use_value_1 = (src_descr->data.value != 0);
      break;

    case FLOATNUM:
      use_value_1 = (src_descr->data.float_value != 0.0);
      break;

    default:
      status = 1;
      goto exit_substitution_conversion;
  }

  /* Select the item to be used */

  q = strchr(p, ';');
  if (q != NULL) {
    if (use_value_1)
      *q = '\0';
    else
      p = q + 1;
  } else {
    if (!use_value_1)
      p = null_string;
  }

  c = *p;
  if (IsDigit(c)) /* Substitute field */
  {
    fno = 0;
    do {
      fno = (fno * 10) + (*(p++) - '0');
    } while (IsDigit(*p));
    if (*p != '\0')
      goto exit_substitution_conversion;

    if (fno == 0) /* Record id */
    {
      k_release(src_descr);
      InitDescr(src_descr, ADDR);
      src_descr->data.d_addr = Element(process.syscom, SYSCOM_QPROC_ID);
      k_get_value(src_descr);
    } else /* Data field */
    {
      k_dismiss(); /* 0533 */
      InitDescr(e_stack, ADDR);
      (e_stack++)->data.d_addr = Element(process.syscom, SYSCOM_QPROC_RECORD);
      InitDescr(e_stack, INTEGER);
      (e_stack++)->data.value = fno;
      InitDescr(e_stack, INTEGER);
      (e_stack++)->data.value = 0;
      InitDescr(e_stack, INTEGER);
      (e_stack++)->data.value = 0;
      op_extract();
    }
  } else if (c == '\0') /* Absent component */
  {
    k_release(src_descr);
    InitDescr(src_descr, STRING);
    src_descr->data.str.saddr = NULL;
  } else if (strchr("'\"\\", c) != NULL) /* Literal string substitution */
  {
    if ((q = strchr(++p, c)) == NULL) {
      /* No closing quote */
      goto exit_substitution_conversion;
    }
    *q = '\0';
    k_release(src_descr);
    k_put_c_string(p, src_descr);

  } else if (!strcmp(p, "*")) /* Retain original data */
  {
    status = 0;
    goto exit_substitution_conversion;
  } else /* Unrecognised code */
  {
    goto exit_substitution_conversion;
  }

  status = 0;

exit_substitution_conversion:

  return status;
}

/* ======================================================================
   substring_conversion()  -  Tm,n conversion                             */

int32_t substring_conversion(char* p) {
  int32_t status = 2;
  int m;
  int n = -1;

  /* Get position data from conversion code */

  if (!IsDigit(*p))
    goto exit_substring_conversion;

  m = 0;
  do {
    m = (m * 10) + (*(p++) - '0');
  } while (IsDigit(*p));

  if (*p == ',') {
    if (!IsDigit(*(++p)))
      goto exit_substring_conversion;

    n = 0;
    do {
      n = (n * 10) + (*(p++) - '0');
    } while (IsDigit(*p));
  }

  if (*p != '\0')
    goto exit_substring_conversion;

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = m;

  if (n < 0) /* Rightmost substring extraction */
  {
    op_substre();
  } else /* Standard substring extraction */
  {
    InitDescr(e_stack, INTEGER);
    (e_stack++)->data.value = n;
    op_substr();
  }

  status = 0;

exit_substring_conversion:

  return status;
}

/* ======================================================================
   time_conversion()  -  MT conversion                                    */

Private int32_t time_conversion(char* p) {
  int32_t status = 2;
  DESCRIPTOR* src_descr;
  char separator;
  bool am_pm = FALSE;
  bool include_seconds = FALSE;
  bool am;
  int16_t hours;
  int16_t mins;
  int32_t secs;
  char s[11 + 1];

  s[0] = '\0';

  /* Get the time value */

  src_descr = e_stack - 1;
  if ((src_descr->type == STRING) && (k_blank_string(src_descr))) {
    goto exit_time_conversion_ok;
  }

  GetInt(src_descr);
  secs = src_descr->data.value;

  p++; /* Skip T */

  if (UpperCase(*p) == 'H') /* 12 hour format */
  {
    p++;
    am_pm = TRUE;
    secs = secs % 86400L; /* 0152 */
  }

  if (UpperCase(*p) == 'S') /* Include seconds */
  {
    p++;
    include_seconds = TRUE;
  }

  if (*p != '\0') /* Separator specified */
  {
    if (((*p == '"') || (*p == '\'')) && (*(p + 1) != '\0') &&
        (*(p + 2) == *p)) {
      p++;
      separator = *(p++); /* Quoted delimiter */
      p++;
    } else {
      separator = *(p++);
    }
  } else
    separator = ':';

  hours = (int16_t)(secs / 3600);
  if (am_pm) {
    am = hours < 12;
    hours = hours % 12;
    if (hours == 0)
      hours = 12; /* 0152 */
  }

  secs = secs % 3600;
  mins = (int16_t)(secs / 60);

  secs = secs % 60;

  sprintf(s, (include_seconds) ? "%02d%c%02d%c%02d" : "%02d%c%02d", (int)hours,
          separator, (int)mins, separator, (int)secs);
  if (am_pm) {
    if (Option(OptAMPMUpcase))
      strcat(s, (am) ? "AM" : "PM");
    else
      strcat(s, (am) ? "am" : "pm");
  }

exit_time_conversion_ok:
  k_pop(1);
  k_put_c_string(s, e_stack);
  e_stack++;
  status = 0;

  return status;
}

/* ======================================================================
   insert_commas()  -  Add thousands delimiters to string                 */

Private void insert_commas(char* s,
                           char thousands, /* Delimiter to insert */
                           char decimal)   /* Decimal separator */
{
  char z[MAX_DATA_STRING_LEN + 1];
  char* p;
  char* q;
  int16_t i;
  int16_t n;

  if (thousands != '\0') {
    q = strchr(s, decimal);
    n = (q != NULL) ? (q - s) : strlen(s); /* Chars before decimal */
    if (n > 3) {
      i = n % 3;
      if (i == 0)
        i = 3;
      p = z;
      q = s;
      while (n > 3) {
        memcpy(p, q, i);
        p += i;
        if (IsDigit(*(q + i - 1)))
          *(p++) = thousands; /* 0153 */
        q += i;
        i = 3;
        n -= 3;
      }
      strcpy(p, q);
      strcpy(s, z);
    }
  }
}

/* ======================================================================
   insert_text_breaks()  -  Insert text marks in overlong string          */

Private void insert_text_breaks(char* s,
                                int16_t* s_len,
                                int16_t width,
                                char pad_char,
                                bool word_align) {
  char z[MAX_DATA_STRING_LEN + 1];
  char* p;
  char* q;
  char* r;
  int16_t i;
  int16_t j;
  int16_t remaining_bytes;
  int16_t skip;
  int16_t len;
  int16_t x;

  p = s;
  q = z;
  remaining_bytes = len = *s_len;

  do {
    i = min(width, remaining_bytes);
    skip = 0;

    if (word_align && (remaining_bytes > width)) {
      for (x = i, r = p + i; x; x--, r--) {
        if (*r == ' ') /* Found a space at which to break */
        {
          i = x;
          skip = 1;
          break;
        }
      }
    }

    /* Copy data for this fragment */

    memcpy(q, p, i);
    q += i;

    /* 0088 Insert spaces to pad fragment to specified width */

    if (i < width) {
      j = width - i;
      memset(q, pad_char, j);
      q += j;
      len += j;
    }

    i += skip;
    p += i;
    remaining_bytes -= i;
    if (!remaining_bytes)
      break;
    *(q++) = TEXT_MARK;
    if (!skip)
      len++; /* Allow for inserted text mark */
  } while (1);

  memcpy(s, z, len);
  *s_len = len;
}

/* ======================================================================
   op_getnls()  -  Get NLS data                                           */

void op_getnls() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Key                        | Value                       |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;
  int16_t key;

  descr = e_stack - 1;
  GetInt(descr);
  key = (int16_t)(descr->data.value);

  InitDescr(descr, STRING);
  descr->data.str.saddr = NULL;
  ts_init(&(descr->data.str.saddr), 10);

  switch (key) {
    case NLS_CURRENCY:
      ts_copy_c_string(national.currency);
      break;

    case NLS_THOUSANDS:
      ts_copy_byte(national.thousands);
      break;

    case NLS_DECIMAL:
      ts_copy_byte(national.decimal);
      break;
  }

  (void)ts_terminate();
}

/* ======================================================================
   op_setnls()  -  Set NLS data                                           */

void op_setnls() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Value                      |                             |
     |-----------------------------|-----------------------------| 
     |  Key                        |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;
  int16_t key;
  STRING_CHUNK* str;

  descr = e_stack - 2;
  GetInt(descr);
  key = (int16_t)(descr->data.value);

  descr = e_stack - 1;
  k_get_string(descr);
  str = descr->data.str.saddr;

  switch (key) {
    case NLS_CURRENCY:
      (void)k_get_c_string(descr, national.currency, MAX_NLS_CURRENCY);
      break;

    case NLS_THOUSANDS:
      if (str != NULL)
        national.thousands = str->data[0];
      break;

    case NLS_DECIMAL:
      national.decimal = (str != NULL) ? (str->data[0]) : '\0';
      break;
  }

  k_dismiss();
  k_pop(1);
}

/* ======================================================================
   conv_dtx()  -  Convert decimal to hexadecimal                          */

int32_t conv_dtx(char* p) {
  DESCRIPTOR* descr;
  char s[32 + 1];

  if (*p != '\0')
    return 2;

  descr = e_stack - 1;

  /* 0227  Return null for null input string */

  if ((descr->type == STRING) && (descr->data.str.saddr == NULL)) {
    return 0;
  }

  sprintf(s, "%u", GetUnsignedInt(descr)); /* 0291 */
  k_put_c_string(s, descr);

  return 0;
}

/* ======================================================================
   conv_xtd()  -  Convert hexadecimal to decimal                          */

int32_t conv_xtd(char* p) {
  if (*p != '\0')
    return 2;

  op_xtd();
  return 0;
}

/* ======================================================================
   apply_mask()  -  Apply format mask                                     */

Private int apply_mask(char* src,
                       int src_len,
                       char* tgt,
                       char* mask,
                       bool right_justify,
                       char fill_char) {
  char buff[MAX_DATA_STRING_LEN + 1];
  int pad;   /* Count of padding bytes to be added */
  char mpad; /* Actual padding character for this element */
  int tgt_len;
  int bytes_remaining;
  char c;
  int i;
  int n;
  char* q;

  pad = 0;

  if (right_justify) {
    /* For a right justified mask we must adjust src to point to a position in
      the source string that will result in the number of characters
      specified by the mask, counting from the right hand end of the source */

    q = mask;
    i = 0; /* Accumulated length of data copied by mask */
    do {
      c = *(q++);
      if (c == '\\') {
        if (*(q++) == '\0')
          break; /* Illegal - Mask ends with \ */
      } else if ((c == '#') || (c == '%') || (c == '*')) {
        if (IsDigit(*q)) /* Repeat count present */
        {
          n = 0;
          do {
            n = (n * 10) + (*(q++) - '0');
          } while (IsDigit(*q));
          i += n;
        } else
          i++;
      }
    } while (*q != '\0');

    n = src_len;

    if (i < n) {
      src += n - i; /* Adjust to lose front of source string */
      src_len = i;
    } else {
      pad = i - n; /* Leading padding length */
    }
  }

  /* Now copy data according to the mask */

  tgt_len = 0; /* Length of target string */
  q = buff;

  bytes_remaining = src_len;
  c = *(mask++);
  do {
    if ((c == '#') || (c == '%') || (c == '*')) {
      /* Select fill character */

      if (c == '#')
        mpad = fill_char;
      else if (c == '%')
        mpad = '0';
      else
        mpad = '*';

      if (IsDigit(*mask)) /* Repeat count present */
      {
        n = 0;
        do {
          n = (n * 10) + (*(mask++) - '0');
        } while (IsDigit(*mask));

        while ((tgt_len < MAX_DATA_STRING_LEN) && n--) {
          if (pad) {
            *(q++) = mpad;
            pad--;
          } else {
            if (bytes_remaining) {
              bytes_remaining--;
              *(q++) = *(src++);
            } else
              *(q++) = mpad;
          }
          tgt_len++;
        }
      } else /* No repeat count present */
      {
        if (pad) {
          *(q++) = mpad;
          pad--;
        } else {
          if (bytes_remaining) {
            bytes_remaining--;
            *(q++) = *(src++);
          } else
            *(q++) = mpad;
        }
        tgt_len++;
      }
    } else /* Copy character */
    {
      if (c == '\\')
        c = *(mask++);
      *(q++) = c;
      tgt_len++;
    }
  } while ((tgt_len < MAX_DATA_STRING_LEN) && ((c = *(mask++)) != '\0'));
  *q = '\0';

  /* Copy masked string into tgt */

  memcpy(tgt, buff, tgt_len);

  return tgt_len;
}

/* ======================================================================
   oconv_d()  -  Output date conversion                                   */

int oconv_d(int dt,     /* Date value to convert */
            char* code, /* Conversion code with leading D removed */
            char* tgt)  /* Buffer to receive result */
{
  int32_t status = 2;
  bool european_format;
  int16_t day;
  int16_t mon;
  int16_t year;
  int16_t julian;
  int16_t dow;
  int16_t year_digits;
  char separator[2] = "\0"; /* Double null to allow simple overwrite */
  char space[2] = " ";
  char* sep;
  struct FORMAT {
    char code; /* D, M, Y, J, A (alpha month), W (Alpha day)
                                 N (Day of week as number), Q, O (ordinal day),
                                 I (ISO week number), y (ISO year number) */
    int16_t width;
    bool zero_suppress;
    char text[20 + 1];
  } format[5];
  int16_t i;
  int16_t j;
  int16_t n;
  bool done;
  char c;
  char* p;
  char* q;
  char z[12];
  bool leading_upper = FALSE;
  bool iso = FALSE;
  int iso_wk;
  int iso_year;
  int w;
  int y;
  int jan1_dow;
  int leap_year;
  int prev_leap_year;
  static char* ordinals[31] = {NULL};

  tgt[0] = '\0';
  memset(format, 0, sizeof(format));

  /* Convert the date to day, month, year components */

  day_to_dmy(dt, &day, &mon, &year, &julian);
  dow = ((dt - 1) % 7) + 1;
  if (dow <= 0)
    dow += 7; /* 1 = Monday */

  /* Use system default if converion code is just D */

  if (*code == '\0') {
    code = default_date_conversion;
    if (*code != '\0')
      code++;
  }

  /* {y}  -  Year digits */

  if (IsDigit(*code)) {
    year_digits = *(code++) - '0';
    if (year_digits > 4)
      goto exit_oconv_d;
  } else
    year_digits = 4;

  /* Set up default format, taking E mode into account */

  if ((*code == '\0') || (UpperCase(*code) == 'L')) {
    format[0].code = 'D'; /* Default to dd mmm yyyy */
    format[0].width = 2;
    format[0].zero_suppress = FALSE;
    format[1].code = 'A';
    format[1].width = 3;
  } else {
    /* {c}  -  Separator character */

    if ((*code != '\0') && !IsAlpha(*code)) /* Separator is specified */
    {
      separator[0] = *(code++);
    }

    european_format = european_dates ^ (UpperCase(*code) == 'E'); /* 0407 */
    if (UpperCase(*code) == 'E')
      code++;

    if (european_format) {
      format[0].code = 'D'; /* Default to dd mm yyyy */
      format[0].width = 2;
      format[0].zero_suppress = FALSE;
      if (european_format && (separator[0] == '\0')) {
        format[1].code = 'A';
        format[1].width = 3;
      } else {
        format[1].code = 'M';
        format[1].width = 2;
      }
      format[1].zero_suppress = FALSE;
    } else {
      format[0].code = 'M'; /* Default to mm dd yyyy */
      format[0].width = 2;
      format[0].zero_suppress = FALSE;
      format[1].code = 'D';
      format[1].width = 2;
      format[1].zero_suppress = FALSE;
    }
  }
  format[2].code = 'Y';
  format[2].width = year_digits;
  format[2].zero_suppress = FALSE;
  format[3].code = '\0';
  format[4].code = '\0';

  /* Look for remaining components */

  for (i = 0, done = FALSE; i < 5; i++) {
  reparse:
    c = UpperCase(*code);
    switch (c) {
      case 'D':
        if (*(code + 1) == 'O') {
          format[i].code = 'O';
          code += 2;
          break;
        }
        /* **** Fall through **** */
      case 'J':
        format[i].code = c;
        format[i].width = 2;
        code++;
        break;

      case 'Y':
        code++;
        if (UpperCase(*code) == 'I') {
          format[i].code = 'y';
          iso = TRUE;
          code++;
        } else {
          format[i].code = 'Y';
        }
        format[i].width = (c == 'Y') ? year_digits : 2;
        break;

      case 'I':
        format[i].code = c;
        format[i].width = 2;
        code++;
        break;

      case 'Q':
        format[i].code = c;
        format[i].width = 1;
        code++;
        break;

      case 'L':
        leading_upper = TRUE;
        code++;
        goto reparse; /* Yuck!  Process this component level again */

      case 'M':
        code++;
        if (UpperCase(*code) == 'A') {
          format[i].code = 'A';
          format[i].width = 0; /* Width as necessary */
          code++;
        } else {
          format[i].code = 'M';
          format[i].width = 2;
        }
        break;

      case 'W':
        code++;
        if (UpperCase(*code) == 'A') {
          format[i].code = 'W';
          format[i].width = 0; /* Width as necessary */
          code++;
        } else if (UpperCase(*code) == 'I') {
          format[i].code = 'I';
          format[i].width = 0; /* Width as necessary */
          code++;
        } else {
          format[i].code = 'N';
          format[i].width = 1;
        }
        break;

      default:
        done = TRUE;
        break;
    }

    format[i].zero_suppress = FALSE;

    if (done)
      break;
  }

  if (i && (i < 5))
    format[i].code = '\0'; /* Kill off further defaults */

  /* Check for L (day and month names have only leading capital) */

  if (UpperCase(*code) == 'L') {
    leading_upper = TRUE;
    code++;
  }

  /* Look for format modifiers */

  if (*code == '[') {
    code++;
    i = 0;
    while (*code != ']') {
      if (format[i].code == '\0')
        goto exit_oconv_d;
      /* Modifier but no code */

      c = UpperCase(*code);
      if (c == 'A') /* Alphabetic modifier - Valid for month and day name */
      {
        if (format[i].code == 'M') {
          format[i].code = 'A';
          format[i].width = 0; /* Width to fit */
          code++;
        } else if (strchr("NW", format[i].code) != NULL) {
          format[i].code = 'W';
          format[i].width = 0; /* Width to fit */
          code++;
        } else {
          goto exit_oconv_d;
        }
      } else if (c ==
                 'Z') /* Zero suppression - Valid for D, I, M and Y codes */
      {
        if (strchr("DIMY", format[i].code) != NULL) {
          format[i].zero_suppress = TRUE;
          format[i].width = 1;
          code++;
        } else if (strchr("J", format[i].code) != NULL) {
          format[i].zero_suppress = TRUE;
          format[i].width = 2;
          code++;
        } else {
          goto exit_oconv_d;
        }
      } else if (IsDigit(c)) /* Width modifier */
      {
        n = *(code++) - '0';
        if (IsDigit(*code)) {
          n = (n * 10) + (*(code++) - '0');
        }
        if ((n < 1) || (n > 32))
          goto exit_oconv_d;
        format[i].width = n;
      } else if ((c == '"') || (c == '\'') || (c == '\\')) /* Text modifier */
      {
        code++;
        p = strchr(code, c);
        if (p == NULL)
          goto exit_oconv_d; /* No closing delimiter */
        n = p - code;
        if (n > 20)
          goto exit_oconv_d; /* Too long */
        memcpy(format[i].text, code, n);
        format[i].text[n] = '\0';
        code = p + 1;
      } else if (c == ',') {
        i++;
        code++;
        if (i == 5)
          break;
      } else {
        goto exit_oconv_d;
      }
    }
    code++; /* Skip ] */
  }

  if (*code != '\0')
    goto exit_oconv_d;

  /* Do we need an ISO week conversion? */

  if (iso) {
    iso_year = year;

    /* Is this a leap year? */

    leap_year =
        ((((year % 4) == 0) && ((year % 100) != 0)) || ((year % 400) == 0));

    /* Is the previous year a leap year? */

    y = year - 1;
    prev_leap_year = ((((y % 4) == 0) && ((y % 100) != 0)) || ((y % 400) == 0));

    /* Calculate day for 1 January (Monday = 1, Sunday = 7) */

    y = (year - 1) % 100;
    jan1_dow = (((((((year - 1) - y) / 100) % 4) * 5) + (y + y / 4)) % 7) + 1;

    /* Find if date falls in previous year, week 52 or 53 */

    if ((julian <= (8 - jan1_dow)) && (jan1_dow > 4)) {
      iso_year = year - 1;
      if ((jan1_dow == 5) || ((jan1_dow == 6) && prev_leap_year)) {
        iso_wk = 53;
      } else {
        iso_wk = 52;
      }
    }

    /* Find if date falls in next year, week 1 */

    if (iso_year == year) {
      if ((((leap_year) ? 366 : 365) - julian) < (4 - dow)) {
        iso_year = year + 1;
        iso_wk = 1;
      }
    }

    /* Find if date falls in week 1 to 53 of expected year */

    if (iso_year == year) {
      iso_wk = ((julian + (7 - dow) + (jan1_dow - 1))) / 7;
      if (jan1_dow > 4)
        iso_wk--;
    }
  }

  /* Do the conversion */

  if (separator[0] == '\0')
    separator[0] = ' ';
  sep = NULL; /* No separator before first item */

  p = tgt;

  for (i = 0; (i < 5) && (format[i].code != '\0'); i++) {
    /* Insert separator */

    if (sep != NULL) {
      strcpy(p, sep);
      p += strlen(sep);
    }

    n = format[i].width;

    switch (format[i].code) {
      case 'A': /* Alphabetic month */
        strcpy(z, month_names[mon - 1]);
        if (!leading_upper)
          UpperCaseString(z);
        if (n)
          p += sprintf(p, "%-*.*s", (int)n, (int)n, z);
        else
          p += sprintf(p, "%s", z);
        sep = separator;
        break;

      case 'D': /* Numeric day of month */
        p += sprintf(p, "%-*.*d", (int)n, (format[i].zero_suppress) ? 1 : 2,
                     (int)day);
        sep = separator;
        break;

      case 'I': /* ISO week number */
        p += sprintf(p, "%-*.*d", (int)n, (format[i].zero_suppress) ? 1 : 2,
                     (int)iso_wk);
        sep = separator;
        break;

      case 'J':
        w = sprintf(z, "%d", (int)julian);
        y = n - w;
        while (y-- > 0) {
          *(p++) = (format[i].zero_suppress) ? ' ' : '0';
        }
        memcpy(p, z, w);
        p += w;
        *p = '\0'; /* 0564 */
        sep = separator;
        break;

      case 'M': /* Numeric month */
        p += sprintf(p, "%-*.*d", (int)n, (format[i].zero_suppress) ? 1 : 2,
                     (int)mon);
        sep = separator;
        break;

      case 'N': /* Numeric day of week */
        p += sprintf(p, "%-*d", (int)n, (int)dow);
        sep = space;
        break;

      case 'O': /* Ordinal day of month */
        if (ordinals[0] == NULL) {
          q = sysmsg(1502);
          if (strdcount(q, ',') != 31)
            continue; /* Omit element */

          ordinals[0] = (char*)k_alloc(89, strlen(q) + 1);
          strcpy(ordinals[0], q);
          (void)strtok(ordinals[0], ",");
          for (j = 1; j < 31; j++)
            ordinals[j] = strtok(NULL, ",");
        }
        p += sprintf(p, "%s", ordinals[day - 1]);
        sep = separator;
        break;

      case 'Q': /* Quarter */
        p += sprintf(p, "%-*d", (int)n, (int)((mon + 2) / 3));
        sep = space;
        break;

      case 'W': /* Alphabetic day of week */
        strcpy(z, day_names[dow - 1]);
        if (!leading_upper)
          UpperCaseString(z);
        if (n)
          p += sprintf(p, "%-*.*s", (int)n, (int)n, z);
        else
          p += sprintf(p, "%s", z);
        sep = space;
        break;

      case 'Y':
        if (format[i].width > 0) {
          if (format[i].zero_suppress) {
            p += sprintf(p, "%d", (int)(year % (int)tens[n]));
          } else {
            p += sprintf(p, "%.*d", (int)format[i].width,
                         (int)(year % (int)tens[n]));
          }

          sep = separator;
        }
        break;

      case 'y':
        if (format[i].width > 0) {
          if (format[i].zero_suppress) {
            p += sprintf(p, "%d", (int)(iso_year % (int)tens[n]));
          } else {
            p += sprintf(p, "%.*d", (int)format[i].width,
                         (int)(iso_year % (int)tens[n]));
          }

          sep = separator;
        }
        break;
    }

    if (format[i].text[0] != '\0')
      sep = format[i].text;
  }

  status = 0;

exit_oconv_d:
  return status;
}

/* END-CODE */
