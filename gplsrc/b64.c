/* B64.C
 * Base64 encoding/decoding.
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
 * START-HISTORY:
 * 03 Aug 07  2.5-7 New program.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * This module is adapted from source code published by Bob Trower.
 * Copyright (c) Trantor Standard Systems Inc., 2001
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"

/* Translation Table as described in RFC1113 */
static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* Translation Table to decode (created by author) */
static const char cd64[]="|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";


/* ======================================================================
 * encode()  -  Base64 encode adding padding and line breaks              */

STRING_CHUNK * b64encode(STRING_CHUNK * str)
{
 unsigned char in[3];
 unsigned char out[4];
 int i;
 int len;
 STRING_CHUNK * tgt;
 char * p;
 int bytes;

 if (str == NULL) return NULL;

 ts_init(&tgt, str->string_len);

 p = str->data;
 bytes = str->bytes;

 while(str != NULL)
  {
   len = 0;
   for(i = 0; i < 3; i++)
    {
     if (str != NULL)
      {
       in[i] = (unsigned char)(*(p++));
       len++;
       if (--bytes == 0)
        {
         str = str->next;
         if (str != NULL)
          {
           p = str->data;
           bytes = str->bytes;
          }
        }
      }
     else in[i] = 0;
    }

   if (len)
    {
     out[0] = cb64[ in[0] >> 2 ];
     out[1] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
     out[2] = (unsigned char)((len > 1)?cb64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');
     out[3] = (unsigned char)((len > 2)?cb64[ in[2] & 0x3f ] : '=');

     ts_copy((char *)out, 4);
    }
  }

 ts_terminate();

 return tgt;
}

/* ======================================================================
   decode()  -  Decode a base64 encoded stream discarding padding, line
                breaks and noise                                          */

STRING_CHUNK * b64decode(STRING_CHUNK * str)
{
 unsigned char in[4];
 unsigned char out[3];
 unsigned char v;
 int i;
 int len;
 STRING_CHUNK * tgt;
 char * p;
 int bytes;


 if (str == NULL) return NULL;

 ts_init(&tgt, str->string_len);

 p = str->data;
 bytes = str->bytes;

 while(str != NULL)
  {
   for(len = 0, i = 0; i < 4 && str != NULL; i++)
    {
     v = 0;
     while(str != NULL && v == 0)
      {
       if (bytes == 0)
        {
         str = str->next;
         if (str != NULL)
          {
           p = str->data;
           bytes = str->bytes;
          }
        }
       v = (unsigned char)*(p++);
       bytes--;

       v = (unsigned char)((v < 43 || v > 122) ? 0 : cd64[v-43]);
       if (v)
        {
         v = (unsigned char)((v == '$')?0:(v-61));
        }
      }

     if (str != NULL)
      {
       len++;
       if (v)
        {
         in[i] = (unsigned char)(v - 1);
        }
      }
     else
      {
       in[i] = 0;
      }
    }

   if (len)
    {
     out[0] = (unsigned char)(in[0] << 2 | in[1] >> 4);
     out[1] = (unsigned char)(in[1] << 4 | in[2] >> 2);
     out[2] = (unsigned char)(((in[2] << 6) & 0xc0) | in[3]);

     ts_copy((char *)out, len - 1);
    }
  }

 ts_terminate();

 return tgt;
}

/* END-CODE */
