/* OP_ICONV.C
 * ICONV opcode
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
 * START-HISTORY (OpenQM):
 * 04 Oct 07  2.6-5 0563 float_conversion() assumed double to be little endian.
 * 03 Aug 07  2.5-7 Added B64 conversion.
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 13 May 07  2.5-4 Use 1 Jan 0001 as internal day 0 during date conversion.
 * 01 May 07  2.5-3 0549 Reworked date conversion to handle dates prior to 1900
 *                  correctly.
 * 17 Jan 07  2.4-19 0535 Do not wrap minutes or seconds in MT conversion.
 * 17 Jan 07  2.4-19 Added NO.DATE.WRAPPING option.
 * 15 Jan 07  2.4-19 0534 Trap input date with zero day number.
 * 27 Nov 06  2.4-17 Added R conversion.
 * 29 Aug 06  2.4-12 Added C conversion.
 * 05 Jul 06  2.4-8 0501 Reworked masked_decimal_conversion() to handle NLS
 *                  correctly.
 * 09 Nov 05  2.2-16 Return status 1 from non-numeric data in MT conversion.
 * 09 Sep 05  2.2-10 0407 The E qualifier in a date conversion should toggle
 *                   European format, not set it.
 * 25 Jul 05  2.2-6 Allow for day numbers > end of month on input data
 *                  conversion, returning status code 3.
 * 11 Jan 05  2.1-0 0297 ILH conversion was extracting bytes in the wrong order.
 * 29 Dec 04  2.1-0 Added P (pattern matching) conversion.
 * 14 Nov 04  2.0-10 Added IFL and IFS conversions.
 * 03 Nov 04  2.0-9 Added support for MR%5 style field width and padding.
 * 20 Sep 04  2.0-2 Use dynamic pcode loader.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 *    B         Boolean (Y or N to 1 or 0)
 *
 *    B64       Base64 decoding
 *
 *    C;1;2;3   Concatenation
 *
 *    D         Date conversion
 *
 *    G{skip}df Group conversion.
 *
 *    IFx       Convert floating point value to 8 bytes
 *    ILx       Convert number to low byte first 4 byte binary integer
 *    ISx       Convert number to low byte first 2 byte binary integer
 *              x = L, H or omitted to specify byte ordering.
 *
 *    L[n[,m]]  String length checks
 *
 *    MCD[X]    Convert hexadecimal to decimal
 *    MCL       Convert to lower case
 *    MCU       Convert to upper case
 *    MCX[D]    Convert decimal to hexadecimal
 *
 *    MO        Convert octal string to integer
 *
 *    MT        Time conversion
 *              MT [H] [S] [c]
 *              H = use 12 hour format with AM or PM suffix
 *              S = include seconds
 *              c = separator, defaults to colon
 *
 *    MX        Convert hexadecimal string to integer
 *    MX0C      Convert hexadecimal string to characters
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
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include "config.h"
#include "options.h"

#include <time.h>

extern int32_t tens[]; /* Defined in op_oconv */
int16_t month_offsets[12] = {0,   31,  59,  90,  120, 151,
                               181, 212, 243, 273, 304, 334};
int16_t days_in_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
extern char* month_names[];

int32_t conv_dtx(char* p);
int32_t conv_xtd(char* p);
STRING_CHUNK* b64decode(STRING_CHUNK* str);

/* OP_OCONV.C */
int32_t concatenation_conversion(char* src_ptr);
int32_t group_conversion(char* p);
int32_t mc_conversion(char* src_ptr);
int32_t pattern_match_conversion(char* src_ptr);
int32_t range_conversion(char* src_ptr);
int32_t substitution_conversion(char* src_ptr);
int32_t substring_conversion(char* src_ptr);

/* Local functions */
Private int32_t base64_conversion(void);
Private int32_t boolean_conversion(void);
Private int32_t date_conversion(char* p);
Private int32_t float_conversion(char* p);
Private int32_t radix_conversion(char* p, int16_t radix);
Private int32_t integer_conversion(char* p, int16_t bytes);
Private int32_t masked_decimal_conversion(char* src_ptr);

/* ======================================================================
   op_iconv()  -  Input conversion                                        */

void op_iconv() {
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
#define MAX_CONV_STRING_LEN 256
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

  k_get_value(e_stack - 1);

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

    switch (UpperCase(*(p++))) {
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

      case 'L': /* Length constraints */
        process.status = length_conversion(p);
        break;

      case 'M':
        switch (UpperCase(*p)) {
          case 'B': /* Binary conversion */
            process.status = radix_conversion(p + 1, 2);
            break;

          case 'C': /* Case conversion (plus oddments) */
            switch (UpperCase(*(++p))) {
              case 'D': /* Hexadecimal to decimal */
                c = UpperCase(*(++p));
                if ((c != '\0') && (c != 'X'))
                  goto bad_conv_string;
                if (c == 'X')
                  p++;
                process.status = conv_xtd(p);
                break;
              case 'L':
                set_case(FALSE);
                break;
              case 'U':
                set_case(TRUE);
                break;
              case 'X': /* Decimal to hexadecimal */
                c = UpperCase(*(++p));
                if ((c != '\0') && (c != 'D'))
                  goto bad_conv_string;
                if (c == 'D')
                  p++;
                process.status = conv_dtx(p);
                break;
              default:
                mc_conversion(p);
                break;
            }
            break;

          case 'D': /* Masked decimal conversion */
          case 'L':
          case 'R':
            process.status = masked_decimal_conversion(p);
            break;

          case 'O': /* Octal conversion */
            process.status = radix_conversion(p + 1, 8);
            break;

          case 'T': /* Time conversion */
            process.status = iconv_time_conversion();
            break;

          case 'X': /* Hexadecimal conversion */
            process.status = radix_conversion(p + 1, 16);
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

      case 'S': /* Substitution conversion */
        process.status = substitution_conversion(p);
        break;

      case 'T':
        if (strchr(p, ';') != NULL) /* Looks like a Tfile conversion */
        {
          /* Push conversion specification onto e-stack */
          k_put_c_string(p, e_stack++);

          /* Push oconv flag onto stack */
          InitDescr(e_stack, INTEGER);
          (e_stack++)->data.value = FALSE;

          k_recurse(pcode_tconv, 3); /* Execute recursive code */
        } else {
          process.status = substring_conversion(p);
        }
        break;

      case 'U': /* User defined */
        /* Push conversion specification onto e-stack */
        k_put_c_string(p, e_stack++);

        k_recurse(pcode_iconv, 2); /* Execute recursive code */
        break;

      default: /* Not recognised - try user hook */
        goto bad_conv_string;
    }

    if (process.status != 0)
      goto exit_op_iconv;

    p = q + 1;
  } while (!done);

exit_op_iconv:
  if ((process.status == 1) || (process.status == 2)) {
    /* 0124 Return a null string for error codes 1 and 2 */
    k_dismiss();
    InitDescr(e_stack, STRING);
    (e_stack++)->data.str.saddr = NULL;
  }

  return;

bad_conv_string:
  process.status = 2;
  goto exit_op_iconv;
}

/* ======================================================================
   base64_conversion()  -  B64 conversion                                 */

Private int32_t base64_conversion() {
  DESCRIPTOR* src_descr;
  STRING_CHUNK* str;

  src_descr = e_stack - 1;
  k_get_string(src_descr);

  str = b64decode(src_descr->data.str.saddr);
  k_dismiss();
  InitDescr(e_stack, STRING);
  (e_stack++)->data.str.saddr = str;

  return 0;
}

/* ======================================================================
   boolean_conversion()  -  B conversion                                  */

Private int32_t boolean_conversion() {
  DESCRIPTOR* src_descr;
  STRING_CHUNK* str;
  bool result = FALSE;
  int32_t status = 1;

  src_descr = e_stack - 1;
  k_get_string(src_descr);
  str = src_descr->data.str.saddr;

  if (str == NULL)
    status = 0;
  else {
    if (str->string_len != 1)
      goto exit_boolean_conversion;

    switch (UpperCase(str->data[0])) {
      case 'Y':
        result = TRUE;
        status = 0;
        break;

      case 'N':
        result = FALSE;
        status = 0;
        break;

      default:
        goto exit_boolean_conversion;
    }
  }

  k_dismiss();
  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = result;

exit_boolean_conversion:
  return status;
}

/* ======================================================================
   date_conversion()  -  D conversion                                     */

Private int32_t date_conversion(char* p) {
  int16_t status = 2;
  int16_t digits;
  int16_t year_digits;
  char sequence[3 + 1];

  DESCRIPTOR* src_descr;
#define MAX_DATE_STRING_LEN 50
  char s[MAX_DATE_STRING_LEN + 1];

  int16_t year = -1;
  int16_t month = 1;
  int16_t day = 1;
  char month_string[9 + 1];

  int16_t i;
  int16_t n;
  int16_t v;
  int32_t days;
  char* q;
  char* r;
  bool first;
  time_t timenow;

  /* Set default sequence */

  strcpy(sequence, (european_dates) ? "DMY" : "MDY");

  if (IsDigit(*p))
    p++;

  /* {c}  -  Separator character - ignore but force European format if
    no separator present                                              */

  if ((*p != '\0') && !IsAlpha(*p))
    p++;
  /*  1.1-9 Removed: else strcpy(sequence, "DMY"); */

  /* Format present? */

  switch (UpperCase(*p)) {
    case 'D':
    case 'M':
    case 'Y':
      sequence[0] = '\0';
      q = sequence;
      do {
        if ((strchr(sequence, UpperCase(*p)) != NULL) /* Repeated element */
            || (strlen(sequence) >= 3))               /* Too many elements */
        {
          goto exit_date_conversion;
        }

        *(q++) = UpperCase(*(p++));
        *q = '\0';
      } while ((*p != '\0') && (strchr("DMY", UpperCase(*p)) != NULL));
      break;

    case 'E':
      strcpy(sequence, (european_dates) ? "MDY" : "DMY"); /* 0407 */
      break;
  }

  /* Get source string */

  src_descr = e_stack - 1;
  if (k_get_c_string(src_descr, s, MAX_DATE_STRING_LEN) < 0)
    return 1;

  if (s[0] == '\0')
    return 0; /* Null string - converts ok to null string */

  /* Process elements */

  status = 1;

  for (p = sequence, q = s, first = TRUE; *p != '\0'; p++) {
    if (!first)
      while ((*q != '\0') && !IsAlnum(*q))
        q++;
    first = FALSE;

    if (IsAlpha(*q)) /* Alphabetic component */
    {
      if (*p == 'D') /* Expecting day of month */
      {
        /* We were expecting to see the day rather than the month.  If the
          month is still to come, simply interchange its position with the
          day.  If we have already processed the month, swap the value too. */

        r = strchr(p, 'M');
        if (r == NULL) /* Month is not yet to come */
        {
          r = strchr(sequence, 'M');
          if (r != NULL) /* Month has already been processed */
          {
            day = month;
          } else
            goto exit_date_conversion;
        }
        *r = 'D';
        *p = 'M';
      }

      for (i = 0; (i < 9) && IsAlpha(*q); i++)
        month_string[i] = *(q++);
      month_string[i] = '\0';

      if (i < 3)
        goto exit_date_conversion; /* At least 3 letters required */

      for (month = 0; month < 12; month++) {
        if (StringCompLenNoCase(month_string, month_names[month], i) == 0)
          break;
      }
      if (month == 12)
        goto exit_date_conversion;
      month++;
    } else if (IsDigit(*q)) /* Numeric component */
    {
      n = (*p == 'Y') ? 4 : 2;

      /* Process n digits or to non-digit */

      v = 0;
      digits = 0;
      while (n--) {
        digits++;
        v = (v * 10) + (*(q++) - '0');
        if (!IsDigit(*q))
          break;
      }

      switch (*p) {
        case 'D':
          day = v;
          break;
        case 'M':
          month = v;
          break;
        case 'Y':
          year = v;
          year_digits = digits;
          break;
      }
    } else if (*q == '\0')
      break;
    else
      goto exit_date_conversion;
  }

  /* Skip trailing spaces */

  while (*q == ' ')
    q++;
  if (*q != '\0')
    goto exit_date_conversion;

  /* Validate components */

  if (day == 0)
    goto exit_date_conversion; /* 0534 */

  if ((month < 1) || (month > 12))
    goto exit_date_conversion;

  if (year < 0) /* Default to current year */
  {
    timenow = time(NULL);
    year = gmtime(&timenow)->tm_year + 1900;
  } else if (year_digits == 2) {
    i = pcfg.yearbase % 100;
    if (year < i)
      year += 100;
    year += pcfg.yearbase - i;
  }

  days = -718430;

  if (((year % 4) == 0) && (((year % 100) != 0) || ((year % 400) == 0))) {
    days_in_month[1] = 29;
    if (month > 2)
      days++;
  } else {
    days_in_month[1] = 28;
  }

  year -= 1;

  /* Whole elapsed 400 year groups */
  n = year / 400;
  days += n * 146097;
  year -= n * 400;

  /* Whole elapsed 100 year groups */
  n = year / 100;
  days += n * 36524;
  year -= n * 100;

  /* Whole elapsed 4 year groups */
  n = year / 4;
  days += n * 1461;
  year -= n * 4;

  days += year * 365; /* Single years into current 4 year group */

  days += month_offsets[month - 1];

  if (day > days_in_month[month - 1]) {
    if (Option(OptNoDateWrapping))
      goto exit_date_conversion;
    status = 3;
  } else {
    status = 0;
  }

  days += day - 1;

  sprintf(s, "%d", days);
  k_dismiss();
  k_put_c_string(s, e_stack++);

exit_date_conversion:
  return status;
}

/* ======================================================================
   radix_conversion()  -  MB, MO and MX conversion                        */

Private int32_t radix_conversion(char* src, int16_t radix) {
  DESCRIPTOR* src_descr;
  DESCRIPTOR result;
  u_int32_t value;
  STRING_CHUNK* str_hdr;
#define MAX_RSTRING_LEN 32
  char s[MAX_RSTRING_LEN + 1];
  int16_t bytes;
  int16_t s_len;
  char* p;
  char* q;
  char c;
  int16_t k;
  int16_t n;
  int16_t digits;
  int16_t shift;
  static char HexChars[] = "0123456789ABCDEF";

  src_descr = e_stack - 1;
  k_get_string(src_descr);
  str_hdr = src_descr->data.str.saddr;
  if (str_hdr == NULL)
    return 1;

  switch (radix) {
    case 2:
      shift = 1;
      digits = 8;
      break;
    case 8:
      shift = 3;
      digits = 3;
      break;
    case 16:
      shift = 4;
      digits = 2;
      break;
  }

  if (*src == '\0') /* MB, MO, MX */
  {
    s_len = k_get_c_string(src_descr, s, MAX_RSTRING_LEN);
    if (s_len < 0)
      return 1;
    UpperCaseString(s);

    value = 0;
    p = s;
    while (((q = strchr(HexChars, *p)) != NULL) &&
           ((n = q - HexChars) < radix)) {
      value = (value << shift) | n;
      p++;
    }

    if (*p != '\0')
      return 1;

    k_dismiss();
    InitDescr(e_stack, INTEGER);
    (e_stack++)->data.value = value;
  } else if ((memcmp(src, "0C", 2) == 0) ||
             (memcmp(src, "0c", 2) == 0)) { /* MB0C, MO0C, MX0C */
    /* Process the string as digits of the selected radix, inserting implied
      leading zeros if requirde.  Return the character equivalent of this
      string.  Invalid characters cause the conversion to return a null
      string and status 1.                                                  */

    InitDescr(&result, STRING);
    result.data.str.saddr = NULL;
    ts_init(&(result.data.str.saddr), str_hdr->string_len / digits + 1);

    k = (int16_t)((digits - (str_hdr->string_len % digits)) % digits);
    do {
      p = str_hdr->data;
      bytes = str_hdr->bytes;
      value = 0;
      while (bytes--) {
        c = UpperCase(*(p++));
        q = strchr(HexChars, c);
        if (q == NULL)
          goto return_null;
        if ((n = q - HexChars) >= radix)
          goto return_null;
        value = (value << shift) | n;

        if (++k == digits) {
          if (value > 255)
            goto return_null; /* e.g. octal 419 */
          ts_copy_byte((char)value);
          k = 0;
          value = 0;
        }
      }
    } while ((str_hdr = str_hdr->next) != NULL);

    (void)ts_terminate();

    k_dismiss();
    *(e_stack++) = result;
  } else {
    return 2;
  }

  return 0;

return_null:
  (void)ts_terminate(); /* Kill partially built string */
  k_dismiss();
  InitDescr(e_stack, STRING);
  (e_stack++)->data.str.saddr = NULL;
  return 1;
}

/* ======================================================================
   integer_conversion()  -  IL and IS conversions                         */

Private int32_t integer_conversion(char* src, int16_t bytes) {
  DESCRIPTOR* descr;
  int32_t value;
  u_char s[4 + 1];
  int16_t i;
  int16_t j;
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

  descr = e_stack - 1;
  GetInt(descr);
  value = descr->data.value;

  if (hi_byte_first) {
    for (i = 0, j = (bytes - 1) * 8; i < bytes; i++, j -= 8) /* 0297 */
    {
      s[i] = (u_char)((value >> j) & 0xFF);
    }
  } else {
    for (i = 0; i < bytes; i++) {
      s[i] = (u_char)(value & 0xFF);
      value >>= 8;
    }
  }

  k_put_string((char*)s, bytes, descr);

  return 0;
}

/* ======================================================================
   float_conversion()  -  IF conversion                                   */

Private int32_t float_conversion(char* p) {
  DESCRIPTOR* descr;
  char s[sizeof(double)];
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
  k_get_float(descr);

#ifdef BIG_ENDIAN_SYSTEM
  if (!hi_byte_first) /* 0563 */
#else
  if (hi_byte_first)
#endif
  {
    for (n = sizeof(double), q = s, r = ((char*)&(descr->data.float_value)) + n;
         n--;) {
      *(q++) = *(--r);
    }
  } else {
    memcpy(s, &(descr->data.float_value), sizeof(double));
  }

  k_put_string(s, sizeof(double), descr);

  return 0;
}

/* ======================================================================
   masked_decimal_conversion()  -  MD, ML and MR conversions              */

Private int32_t masked_decimal_conversion(char* p) {
  int32_t status = 2;
  DESCRIPTOR* src_descr;
  char prefix[32 + 1];
  char thousands;
  char decimal;
  char suffix[32] = "";
  int16_t dp;           /* Decimal places */
  int16_t scale_factor; /* Scale factor */
  int16_t width;        /* Field width */
  char pad_char = ' ';    /* ...and character to use */
  bool neg = FALSE;
  char s[160 + 1];
  char z[160 + 1];
  char delim;
  char* q;
  char* r;
  int16_t n;
  int64 value;

  strcpy(prefix, national.currency);
  thousands = national.thousands;
  decimal = national.decimal;

  /* Get the value to be converted */

  src_descr = e_stack - 1;
  k_get_string(src_descr);
  if (src_descr->data.str.saddr == NULL) {
    q = "";
    goto exit_masked_decimal_ok;
  }
  k_get_c_string(src_descr, s, 160);

  /* Process the conversion code */

  p++; /* The mode character D, L or R is insignificant to input conversion */

  /* Get decimal places */

  dp = (IsDigit(*p)) ? (*(p++) - '0') : 0;

  if (IsDigit(*p))
    scale_factor = *(p++) - '0'; /* Scale factor present */
  else
    scale_factor = dp;

  while (1) {
    if (*p == ',')
      p++; /* Comma separator? Not significant */
    else if (*p == '$')
      p++; /* Prefix? Not significant */
    else
      break;
  }

  /* International options? */

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
        }
        while ((*p != delim) && (*p != '\0'))
          p++; /* Ensure at delimiter */
        if (*p == delim)
          p++; /* Skip delimiter */
        while (*p == ' ')
          p++; /* Skip spaces */
      } else if ((*p != ']') && (*p != ',') && (*p != '\0')) {
        thousands = *(p++);
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

  /* Negative value action present? */

  switch (UpperCase(*p)) {
    case '+':
    case '-':
    case '<':
    case '(':
    case 'C':
    case 'D':
      p++;
      break;
  }

  /* Suppress scaling if decimal point present? */

  if (UpperCase(*p) == 'P')
    p++;

  /* Represent zero value by null string? */

  if (UpperCase(*p) == 'Z')
    p++;

  if (IsDigit(*p)) /* Field width present (max two digits) */
  {
    width = *(p++) - '0';
    if (IsDigit(*p))
      width = (width * 10) + (*(p++) - '0');
    if (*p != '\0')
      pad_char = *(p++);
  } else if ((*p != '\0') && (strchr("#*%", *p) !=
                              NULL)) { /* Format code style fill specfication */
    width = 0;
    if (*p == '%')
      pad_char = '0';
    else if (*p == '*')
      pad_char = '*';
    p++;
    while (IsDigit(*p)) {
      width = (width * 10) + (*(p++) - '0');
    }
  }

  /* Do the conversion */

  status = 1;

  /* Strip prefix string */

  if ((prefix[0] != '\0') && ((r = strstr(s, prefix)) != NULL)) {
    strcpy(r, r + strlen(prefix));
  }

  /* Copy string to z, removing any further padding characters, spaces and
    thousands delimtiers without regard to their correct positioning.                   */

  for (r = s, q = z; *r != '\0'; r++) {
    if (*r == pad_char)
      continue;
    if (*r == ' ')
      continue;
    if (*r == thousands)
      continue;
    *(q++) = *r;
  }
  *q = '\0';

  /* String to process is now in z. Check for leading sign character */

  r = z;
  switch (*r) {
    case '+':
      r++;
      break;
    case '-':
      neg = TRUE;
      r++;
      break;
    case '(':
      n = strlen(z) - 1;
      if ((n < 1) || (z[n] != ')'))
        goto exit_masked_decimal;
      z[n] = '\0';
      neg = TRUE;
      r++;
      break;
    case '<':
      n = strlen(z) - 1;
      if ((n < 1) || (z[n] != '>'))
        goto exit_masked_decimal;
      z[n] = '\0';
      neg = TRUE;
      r++;
      break;
  }

  if (!neg) /* Check for trailing sign */
  {
    n = strlen(z) - 1;
    if (n >= 0) {
      switch (z[n]) {
        case '+':
          z[n] = '\0';
          break;
        case '-':
          neg = TRUE;
          z[n] = '\0';
          break;
        default:
          if (n > 0) {
            n -= 1;
            if ((stricmp(z + n, "CR") == 0) || (stricmp(z + n, "DB") == 0)) {
              neg = TRUE;
              z[n] = '\0';
            }
          }
          break;
      }
    }
  }

  /* Process the value string
    r points to start of value which may not be at start of z string buffer. */

  if (*r == '\0') {
    q = "";
    goto exit_masked_decimal_ok; /* What value string? */
  }

  if (!IsDigit(*r) && (*r != decimal))
    goto exit_masked_decimal;

  value = 0;
  while (IsDigit(*r)) {
    value = (value * 10) + (*(r++) - '0');
  }

  /* We have built the integer part of the value. We now need to process any
    decimal places up to the scale factor limit.                            */

  if (*r == decimal) {
    r++;
    while (scale_factor && IsDigit(*r)) {
      value = (value * 10) + (*(r++) - '0');
      scale_factor--;
    }
  }

  if (scale_factor) {
    /* We ran out of digits before we had scaled completely.  Apply the
      remaining scaling now.                                           */
    value = value * tens[scale_factor];
  } else if (IsDigit(*r)) {
    /* Do we need to round up the value? */
    if (*r >= '5')
      value++;
  }

  q = s;
  if (neg)
    *(q++) = '-';
#ifndef __LP64__
  sprintf(q, "%lld", value);
#else
  sprintf(q, "%ld", value);
#endif
  /* Strip leading zeros */

  q = s;
  while (*q == '0')
    q++;
  if (*q == '\0')
    q--;

exit_masked_decimal_ok:
  k_dismiss();
  k_put_c_string(q, e_stack++);

  status = 0;

exit_masked_decimal:

  return status;
}

/* ======================================================================
   iconv_time_conversion()  -  MT conversion
   Also used by SLEEP opcode                                              */

int32_t iconv_time_conversion() {
  int32_t status = 1;
  DESCRIPTOR* src_descr;
  char* q;
  int32_t time_value = 0;
  int16_t hours = 0;
  int16_t mins = 0;
  int16_t secs = 0;
  char s[11 + 1];
  char c;

  /* Get the time string */

  src_descr = e_stack - 1;
  if (k_get_c_string(src_descr, s, 11) > 0) {
    q = s;
    while (*q == ' ')
      q++; /* Skip leading spaces */

    if (!IsDigit(*q))
      goto exit_time_conversion;

    while (IsDigit(*q))
      hours = (hours * 10) + (*(q++) - '0');

    if (*q != '\0') {
      c = UpperCase(*q);
      if ((c != 'A') && (c != 'P')) {
        q++;
        while (IsDigit(*q))
          mins = (mins * 10) + (*(q++) - '0');

        if (*q != '\0') {
          c = UpperCase(*q);
          if ((c != 'A') && (c != 'P')) {
            q++;
            while (IsDigit(*q))
              secs = (secs * 10) + (*(q++) - '0');
            q++;
          }
        }
      }
    }

    if (*q != '\0') {
      if ((stricmp(q, "A") == 0) || (stricmp(q, "AM") == 0)) {
      } else if ((stricmp(q, "P") == 0) || (stricmp(q, "PM") == 0)) {
        if (hours < 12)
          hours += 12;
      }
    }

    if ((mins > 59) || (secs > 59))
      goto exit_time_conversion; /* 0535 */
    time_value = (((hours * 60L) + mins) * 60) + secs;

    status = 0;
  }

  sprintf(s, "%d", time_value);
  k_dismiss();
  k_put_c_string(s, e_stack++);

exit_time_conversion:
  return status;
}

/* END-CODE */
