/* QMLIB.C
 * Library routines.
 * Copyright (c) 2005 Ladybridge Systems, All Rights Reserved
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
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 08 Sep 05  2.2-10 Added strdcount().
 * 09 Jun 05  2.2-1 Added truncate argument to ftoa().
 * 12 May 05  2.1-14 0354 fmt(-0.1, 'R0') was returning "-0".
 * 12 May 05  2.1-14 Adapted to work on FreeBSD.
 * 11 May 05  2.1-13 Added ftoa() because printf() uses unbiased rounding where
 *                   6.5 rounds to 6, not 7 as an integer.
 * 27 Sep 04  2.0-3 Added memucpy().
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


/* ======================================================================
   ftoa()  -  Convert double to string

   This whole routine is horrible but necessary because printf uses
   unbiased rounding where 6.5 rounds to 6, not 7.                       */

int ftoa(double f, short int dp, bool truncate, char * result)
{
 register char c;
 char s[500+1];      /* IEEE can be enormous! */
 int neg;
 char * p;
 char * q;
 int n;
 bool carry = FALSE;
#define MAX_DP 15

 if ((neg = (f < 0)) != 0) f = -f;
 sprintf(s, "%.*lf", MAX_DP, f);

 /* Now perform rounding */

 p = s;
 n = strlen(p) - (MAX_DP - dp);
 c = p[n];
 p[n] = '\0';

 if (!truncate && c >= '5') /* Round up */
  {
   q = p + n - 1;
   while(n--)
    {
     c = *q;
     if (c == '.')
      {
       q--;
      }
     else if (c < '9')
      {
       (*q)++;
       goto rounding_done;
      }
     else *(q--) = '0';
    }
   carry = TRUE;
  }

rounding_done:

 n = strlen(s)-1;
 if (s[n] == '.') s[n] = '\0';

 q = result;

 if (neg)
  {
   if (carry) *(q++) = '-';
   else
    {
     for(p = s; *p; p++)   /* 0354 */
      {
       if ((*p != '0') && (*p != '.'))
        {
         *(q++) = '-';
         break;
        }
      }
    }
  }
 if (carry) *(q++) = '1';
 strcpy(q, s);

 return strlen(result);
}

/* ======================================================================
   strdcount()  -  C string equivalent of DCOUNT()                        */

int strdcount(char * s, char d)
{
 int n = 1;

 if (*s == '\0') return 0;

 while(*s != '\0') if (*(s++) == d) n++;
 return n;
}

/* ======================================================================
   strrep()  -  Replace one character by another in an entire string      */

void strrep(char * s, char old, char newchar)
{
 register u_char c;

 while((c = *s) != '\0')
  {
   if (c == old) *s = newchar;
   s++;
  }
}

#ifdef BIG_ENDIAN_SYSTEM
/* ======================================================================
   swap2()                                                                */

short int swap2(short int data)
{
 union {
        short int val;
        unsigned char chr[2];
       } in, out;

 in.val = data;
 out.chr[0] = in.chr[1];
 out.chr[1] = in.chr[0];
 return out.val;
}

/* ======================================================================
   swap4()                                                                */

long int swap4(long int data)
{
 union {
        long int val;
        unsigned char chr[4];
       } in, out;

 in.val = data;
 out.chr[0] = in.chr[3];
 out.chr[1] = in.chr[2];
 out.chr[2] = in.chr[1];
 out.chr[3] = in.chr[0];
 return out.val;
}

#endif


/* END-CODE */
