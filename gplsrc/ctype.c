/* CTYPE.C
 * Character type handling and associated functions.
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
 * START-HISTORY:
 * 01 Jul 07  2.5-7 Extensive changes for PDA merge.
 * 15 Jan 06  2.3-4 0449 All uses of character maps need to be via unsigned
 *                  indices.
 * 28 Dec 05  2.3-3 New module.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * This module replaces all the standard casing and character type functions
 * to enable support of user defined collation sequences and upper/lower case
 * pairing rules. By encapsulating all these functions here, future changes
 * should be relatively easy to implement.
 *
 * Although the C library provides locale support functions, these are not
 * immediately applicable here because QM requires binary transparency and
 * the ability to sort right justified strings (amongst other problems).
 *
 * It is likely that QM will be adapted to support Unicode and all the
 * associated locale related operations in the long term future.
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"

/* ======================================================================
   Set default_character maps                                             */

void set_default_character_maps()
{
 int i;
 int j;
 for(i = 0; i < 256; i++)
  {
   uc_chars[i] = (char)i;
   lc_chars[i] = (char)i;
   char_types[i] = 0;
  }

 for(i = 'a', j = 'A'; i <= 'z'; i++, j++)
  {
   uc_chars[i] = j;
   lc_chars[j] = i;
   char_types[i] |= CT_ALPHA;
   char_types[j] |= CT_ALPHA;
  }

 for(i = '0'; i <= '9'; i++)
  {
   char_types[i] |= CT_DIGIT;
  }

 for(i = 33; i <= 126; i++)
  {
   char_types[i] |= CT_GRAPH;
  }

 char_types[U_TEXT_MARK] |= CT_MARK;
 char_types[U_SUBVALUE_MARK] |= CT_MARK | CT_DELIM;
 char_types[U_VALUE_MARK] |= CT_MARK | CT_DELIM;
 char_types[U_FIELD_MARK] |= CT_MARK | CT_DELIM;
 char_types[U_ITEM_MARK] |= CT_MARK;
}

/* ======================================================================
   LowerCaseString()  -  Convert string to lower case                     */

char * LowerCaseString(char * s)
{
 char * p;

 p = s;
 while((*(p++) = LowerCase(*p)) != '\0') {}
 return s;
}

/* ======================================================================
   MemCompareNoCase()  -  Case insensitive variant of memcmp              */

int MemCompareNoCase(char * p, char * q, short int len)
{
 signed char c;

 while(len--)
  {
   if ((c = UpperCase(*p) - UpperCase(*q)) != 0) return c;
   p++;
   q++;
  }

 return 0;
}

/* ======================================================================
   memichr()  -  Case insensitive variant of memchr()                     */

char * memichr(char * s, char c, int n)
{
 c = UpperCase(c);

 while(n--)
  {
   if (UpperCase(*s) == c) return s;
   s++;
  }

 return NULL;
}

/* ======================================================================
   memucpy()  -  Copy a specified number of bytes, converting to uppercase */

void memucpy(char * tgt, char * src, short int len)
{
 while(len--) *(tgt++) = UpperCase(*(src++));
}


/* ======================================================================
   StringCompLenNoCase()  -  Case insensitive variant of strncmp          */

int StringCompLenNoCase(char * p, char * q, short int len)
{
 register char c;

 while(len--)
  {
   if (((c = UpperCase(*p) - UpperCase(*q)) != 0)
     || (*p == '\0') || (*q == '\0')) return c;
   p++;
   q++;
  }

 return 0;
}

/* ======================================================================
   UpperCaseMem()  -  Uppercase specified number of bytes                 */

void UpperCaseMem(char * str, short int len)
{
 register char c;

 while(len--)
  {
   c = UpperCase(*str);
   *(str++) = c;
  }
}

/* ======================================================================
   UpperCaseString()  -  Convert string to upper case                     */

char * UpperCaseString(char * s)
{
 char * p;

 p = s;
 while((*p = UpperCase(*p)) != '\0') {p++;}

 return s;
}

/* END-CODE */

