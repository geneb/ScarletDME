/* OP_CCALL.C
 * Generic routine to call C library code from BASIC
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
 * 09Jan22 gwb Cleaned up outstanding cast warnings when building for 64 bit.
 *
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 *
 * START-HISTORY (OpenQM):
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 21 Mar 06  2.3-9 Applied patch from Doug Dumitru to remove lvalue casts.
 * 29 Oct 04  2.0-9 New module from Doug Dumitru of EasyCo.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * Copyright (c) 2004 EasyCo LLC, All Rights Reserved
 *
 * op_ccall          CCALL str1,str2
 *
 * Sub-pcode description
 *
 * 'str1' contains p-code that is interpreted here
 *
 *   Opcode - x01 - bin2 len - Library Name - Load librarybinary
 *            x02 - bin 4 lib handle - binary 2 len - Function Name - Get function address
 *            x03 - bin4 - push absolute value on stack
 *            x04 - bin4 str2 offset - push address relative in str2 on stack
 *            x10 - bin2 - sub stack
 *            x11 - bin 4 fn addr - call function
 *            x12 - bin4 str2 offset - pop 32-bit result to relative location in str2
 *            x13 - bin4 str2 offset - pop 64-bit result to relative location in str2
 *
 *            --- others as needed
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"

#include <stdarg.h>
#include <setjmp.h>
#include <time.h>

#ifdef __BORLANDC__
#pragma warn - pro
#endif

#include <dlfcn.h>

/* ======================================================================
   op_ccall(str1,str2)  -  ccall(...,...) function                                                */

Private void ccall_c(unsigned char* s1, void* s2) {
  char* p;
  unsigned char ch;
  #ifndef __LP64__ /* 09Jan22 gwb - 64 bit fixup */
  u_int32_t Val;
  #else
  u_int64 Val;
  #endif
  u_int32_t Stk[50];
  int StkCnt;
  u_int64 res64;
  void* v;

  u_int64 (*Fn0)();

  StkCnt = 0;

  while (1) {
    ch = *s1;
    s1++;
    switch (ch) {
      case 1: /* dlload / LoadLibrary */
        p = (char*)s1;
        s1 += strlen(p) + 1;
        /* 09Jan22 gwb - fix for 64 bit */
#ifndef __LP64__        
        res64 = (u_int32_t)dlopen(p, RTLD_NOW | RTLD_GLOBAL);
#else
        res64 = (u_int64)dlopen(p, RTLD_NOW | RTLD_GLOBAL);
#endif
        break;

      case 2: /* dlsym / GetProcAddr */
        p = (char*)s1;
        s1 += strlen(p) + 1;
        v = *(void**)s1;
        s1 += sizeof(void*);
        /* 09Jan22 gwb - fix for 64 bit */
#ifndef __LP64__        
        res64 = (u_int32_t)dlsym(v, p);
#else
        res64 = (u_int64)dlsym(v, p);
#endif
        break;

      case 3: /* Push value */
        Stk[StkCnt++] = *(u_int32_t*)s1;
        s1 += sizeof(u_int32_t*);
        break;

      case 4: /* Push mem offset */
#ifndef __LP64__ /* 09Jan22 gwb - 64 bit fixup */      
        Stk[StkCnt++] = *(u_int32_t*)s1 + (u_int32_t)s2;
        s1 += sizeof(u_int32_t*);
#else
        Stk[StkCnt++] = *(u_int64_t*)s1 + (u_int64_t)s2;
        s1 += sizeof(u_int64_t*);
#endif
        break;

      case 5: /* Inc stack */
        break;

      case 6: /* call function */
        Fn0 = *(void**)s1;
        s1 += sizeof(void*);
        switch (StkCnt) {
          case 0:
            res64 = (*Fn0)();
            break;
          case 1:
            res64 = (*Fn0)(Stk[0]);
            break;
          case 2:
            res64 = (*Fn0)(Stk[0], Stk[1]);
            break;
          case 3:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2]);
            break;
          case 4:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3]);
            break;
          case 5:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4]);
            break;
          case 6:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5]);
            break;
          case 7:
            res64 =
                (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5], Stk[6]);
            break;
          case 8:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5],
                           Stk[6], Stk[7]);
            break;
          case 9:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5],
                           Stk[6], Stk[7], Stk[8]);
            break;
          case 10:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5],
                           Stk[6], Stk[7], Stk[8], Stk[9]);
            break;
          case 11:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5],
                           Stk[6], Stk[7], Stk[8], Stk[9], Stk[10]);
            break;
          case 12:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5],
                           Stk[6], Stk[7], Stk[8], Stk[9], Stk[10], Stk[11]);
            break;
          case 13:
            res64 =
                (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5], Stk[6],
                       Stk[7], Stk[8], Stk[9], Stk[10], Stk[11], Stk[12]);
            break;
          case 14:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5],
                           Stk[6], Stk[7], Stk[8], Stk[9], Stk[10], Stk[11],
                           Stk[12], Stk[13]);
            break;
          case 15:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5],
                           Stk[6], Stk[7], Stk[8], Stk[9], Stk[10], Stk[11],
                           Stk[12], Stk[13], Stk[14]);
            break;
          case 16:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5],
                           Stk[6], Stk[7], Stk[8], Stk[9], Stk[10], Stk[11],
                           Stk[12], Stk[13], Stk[14], Stk[15]);
            break;
          case 17:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5],
                           Stk[6], Stk[7], Stk[8], Stk[9], Stk[10], Stk[11],
                           Stk[12], Stk[13], Stk[14], Stk[15], Stk[16]);
            break;
          case 18:
            res64 =
                (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5], Stk[6],
                       Stk[7], Stk[8], Stk[9], Stk[10], Stk[11], Stk[12],
                       Stk[13], Stk[14], Stk[15], Stk[16], Stk[17]);
            break;
          case 19:
            res64 =
                (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5], Stk[6],
                       Stk[7], Stk[8], Stk[9], Stk[10], Stk[11], Stk[12],
                       Stk[13], Stk[14], Stk[15], Stk[16], Stk[17], Stk[18]);
            break;
          case 20:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5],
                           Stk[6], Stk[7], Stk[8], Stk[9], Stk[10], Stk[11],
                           Stk[12], Stk[13], Stk[14], Stk[15], Stk[16], Stk[17],
                           Stk[18], Stk[19]);
            break;
          case 21:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5],
                           Stk[6], Stk[7], Stk[8], Stk[9], Stk[10], Stk[11],
                           Stk[12], Stk[13], Stk[14], Stk[15], Stk[16], Stk[17],
                           Stk[18], Stk[19], Stk[20]);
            break;
          case 22:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5],
                           Stk[6], Stk[7], Stk[8], Stk[9], Stk[10], Stk[11],
                           Stk[12], Stk[13], Stk[14], Stk[15], Stk[16], Stk[17],
                           Stk[18], Stk[19], Stk[20], Stk[21]);
            break;
          case 23:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5],
                           Stk[6], Stk[7], Stk[8], Stk[9], Stk[10], Stk[11],
                           Stk[12], Stk[13], Stk[14], Stk[15], Stk[16], Stk[17],
                           Stk[18], Stk[19], Stk[20], Stk[21], Stk[22]);
            break;
          case 24:
            res64 =
                (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5], Stk[6],
                       Stk[7], Stk[8], Stk[9], Stk[10], Stk[11], Stk[12],
                       Stk[13], Stk[14], Stk[15], Stk[16], Stk[17], Stk[18],
                       Stk[19], Stk[20], Stk[21], Stk[22], Stk[23]);
            break;
          case 25:
            res64 =
                (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5], Stk[6],
                       Stk[7], Stk[8], Stk[9], Stk[10], Stk[11], Stk[12],
                       Stk[13], Stk[14], Stk[15], Stk[16], Stk[17], Stk[18],
                       Stk[19], Stk[20], Stk[21], Stk[22], Stk[23], Stk[24]);
            break;
          case 26:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5],
                           Stk[6], Stk[7], Stk[8], Stk[9], Stk[10], Stk[11],
                           Stk[12], Stk[13], Stk[14], Stk[15], Stk[16], Stk[17],
                           Stk[18], Stk[19], Stk[20], Stk[21], Stk[22], Stk[23],
                           Stk[24], Stk[25]);
            break;
          case 27:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5],
                           Stk[6], Stk[7], Stk[8], Stk[9], Stk[10], Stk[11],
                           Stk[12], Stk[13], Stk[14], Stk[15], Stk[16], Stk[17],
                           Stk[18], Stk[19], Stk[20], Stk[21], Stk[22], Stk[23],
                           Stk[24], Stk[25], Stk[26]);
            break;
          case 28:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5],
                           Stk[6], Stk[7], Stk[8], Stk[9], Stk[10], Stk[11],
                           Stk[12], Stk[13], Stk[14], Stk[15], Stk[16], Stk[17],
                           Stk[18], Stk[19], Stk[20], Stk[21], Stk[22], Stk[23],
                           Stk[24], Stk[25], Stk[26], Stk[27]);
            break;
          case 29:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5],
                           Stk[6], Stk[7], Stk[8], Stk[9], Stk[10], Stk[11],
                           Stk[12], Stk[13], Stk[14], Stk[15], Stk[16], Stk[17],
                           Stk[18], Stk[19], Stk[20], Stk[21], Stk[22], Stk[23],
                           Stk[24], Stk[25], Stk[26], Stk[27], Stk[28]);
            break;
          case 30:
            res64 =
                (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5], Stk[6],
                       Stk[7], Stk[8], Stk[9], Stk[10], Stk[11], Stk[12],
                       Stk[13], Stk[14], Stk[15], Stk[16], Stk[17], Stk[18],
                       Stk[19], Stk[20], Stk[21], Stk[22], Stk[23], Stk[24],
                       Stk[25], Stk[26], Stk[27], Stk[28], Stk[29]);
            break;
          case 31:
            res64 =
                (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5], Stk[6],
                       Stk[7], Stk[8], Stk[9], Stk[10], Stk[11], Stk[12],
                       Stk[13], Stk[14], Stk[15], Stk[16], Stk[17], Stk[18],
                       Stk[19], Stk[20], Stk[21], Stk[22], Stk[23], Stk[24],
                       Stk[25], Stk[26], Stk[27], Stk[28], Stk[29], Stk[30]);
            break;
          case 32:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5],
                           Stk[6], Stk[7], Stk[8], Stk[9], Stk[10], Stk[11],
                           Stk[12], Stk[13], Stk[14], Stk[15], Stk[16], Stk[17],
                           Stk[18], Stk[19], Stk[20], Stk[21], Stk[22], Stk[23],
                           Stk[24], Stk[25], Stk[26], Stk[27], Stk[28], Stk[29],
                           Stk[30], Stk[31]);
            break;
          case 33:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5],
                           Stk[6], Stk[7], Stk[8], Stk[9], Stk[10], Stk[11],
                           Stk[12], Stk[13], Stk[14], Stk[15], Stk[16], Stk[17],
                           Stk[18], Stk[19], Stk[20], Stk[21], Stk[22], Stk[23],
                           Stk[24], Stk[25], Stk[26], Stk[27], Stk[28], Stk[29],
                           Stk[30], Stk[31], Stk[32]);
            break;
          case 34:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5],
                           Stk[6], Stk[7], Stk[8], Stk[9], Stk[10], Stk[11],
                           Stk[12], Stk[13], Stk[14], Stk[15], Stk[16], Stk[17],
                           Stk[18], Stk[19], Stk[20], Stk[21], Stk[22], Stk[23],
                           Stk[24], Stk[25], Stk[26], Stk[27], Stk[28], Stk[29],
                           Stk[30], Stk[31], Stk[32], Stk[33]);
            break;
          case 35:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5],
                           Stk[6], Stk[7], Stk[8], Stk[9], Stk[10], Stk[11],
                           Stk[12], Stk[13], Stk[14], Stk[15], Stk[16], Stk[17],
                           Stk[18], Stk[19], Stk[20], Stk[21], Stk[22], Stk[23],
                           Stk[24], Stk[25], Stk[26], Stk[27], Stk[28], Stk[29],
                           Stk[30], Stk[31], Stk[32], Stk[33], Stk[34]);
            break;
          case 36:
            res64 = (*Fn0)(
                Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5], Stk[6], Stk[7],
                Stk[8], Stk[9], Stk[10], Stk[11], Stk[12], Stk[13], Stk[14],
                Stk[15], Stk[16], Stk[17], Stk[18], Stk[19], Stk[20], Stk[21],
                Stk[22], Stk[23], Stk[24], Stk[25], Stk[26], Stk[27], Stk[28],
                Stk[29], Stk[30], Stk[31], Stk[32], Stk[33], Stk[34], Stk[35]);
            break;
          case 37:
            res64 =
                (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5], Stk[6],
                       Stk[7], Stk[8], Stk[9], Stk[10], Stk[11], Stk[12],
                       Stk[13], Stk[14], Stk[15], Stk[16], Stk[17], Stk[18],
                       Stk[19], Stk[20], Stk[21], Stk[22], Stk[23], Stk[24],
                       Stk[25], Stk[26], Stk[27], Stk[28], Stk[29], Stk[30],
                       Stk[31], Stk[32], Stk[33], Stk[34], Stk[35], Stk[36]);
            break;
          case 38:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5],
                           Stk[6], Stk[7], Stk[8], Stk[9], Stk[10], Stk[11],
                           Stk[12], Stk[13], Stk[14], Stk[15], Stk[16], Stk[17],
                           Stk[18], Stk[19], Stk[20], Stk[21], Stk[22], Stk[23],
                           Stk[24], Stk[25], Stk[26], Stk[27], Stk[28], Stk[29],
                           Stk[30], Stk[31], Stk[32], Stk[33], Stk[34], Stk[35],
                           Stk[36], Stk[37]);
            break;
          case 39:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5],
                           Stk[6], Stk[7], Stk[8], Stk[9], Stk[10], Stk[11],
                           Stk[12], Stk[13], Stk[14], Stk[15], Stk[16], Stk[17],
                           Stk[18], Stk[19], Stk[20], Stk[21], Stk[22], Stk[23],
                           Stk[24], Stk[25], Stk[26], Stk[27], Stk[28], Stk[29],
                           Stk[30], Stk[31], Stk[32], Stk[33], Stk[34], Stk[35],
                           Stk[36], Stk[37], Stk[38]);
            break;
          case 40:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5],
                           Stk[6], Stk[7], Stk[8], Stk[9], Stk[10], Stk[11],
                           Stk[12], Stk[13], Stk[14], Stk[15], Stk[16], Stk[17],
                           Stk[18], Stk[19], Stk[20], Stk[21], Stk[22], Stk[23],
                           Stk[24], Stk[25], Stk[26], Stk[27], Stk[28], Stk[29],
                           Stk[30], Stk[31], Stk[32], Stk[33], Stk[34], Stk[35],
                           Stk[36], Stk[37], Stk[38], Stk[39]);
            break;
          case 41:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5],
                           Stk[6], Stk[7], Stk[8], Stk[9], Stk[10], Stk[11],
                           Stk[12], Stk[13], Stk[14], Stk[15], Stk[16], Stk[17],
                           Stk[18], Stk[19], Stk[20], Stk[21], Stk[22], Stk[23],
                           Stk[24], Stk[25], Stk[26], Stk[27], Stk[28], Stk[29],
                           Stk[30], Stk[31], Stk[32], Stk[33], Stk[34], Stk[35],
                           Stk[36], Stk[37], Stk[38], Stk[39], Stk[40]);
            break;
          case 42:
            res64 = (*Fn0)(
                Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5], Stk[6], Stk[7],
                Stk[8], Stk[9], Stk[10], Stk[11], Stk[12], Stk[13], Stk[14],
                Stk[15], Stk[16], Stk[17], Stk[18], Stk[19], Stk[20], Stk[21],
                Stk[22], Stk[23], Stk[24], Stk[25], Stk[26], Stk[27], Stk[28],
                Stk[29], Stk[30], Stk[31], Stk[32], Stk[33], Stk[34], Stk[35],
                Stk[36], Stk[37], Stk[38], Stk[39], Stk[40], Stk[41]);
            break;
          case 43:
            res64 = (*Fn0)(
                Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5], Stk[6], Stk[7],
                Stk[8], Stk[9], Stk[10], Stk[11], Stk[12], Stk[13], Stk[14],
                Stk[15], Stk[16], Stk[17], Stk[18], Stk[19], Stk[20], Stk[21],
                Stk[22], Stk[23], Stk[24], Stk[25], Stk[26], Stk[27], Stk[28],
                Stk[29], Stk[30], Stk[31], Stk[32], Stk[33], Stk[34], Stk[35],
                Stk[36], Stk[37], Stk[38], Stk[39], Stk[40], Stk[41], Stk[42]);
            break;
          case 44:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5],
                           Stk[6], Stk[7], Stk[8], Stk[9], Stk[10], Stk[11],
                           Stk[12], Stk[13], Stk[14], Stk[15], Stk[16], Stk[17],
                           Stk[18], Stk[19], Stk[20], Stk[21], Stk[22], Stk[23],
                           Stk[24], Stk[25], Stk[26], Stk[27], Stk[28], Stk[29],
                           Stk[30], Stk[31], Stk[32], Stk[33], Stk[34], Stk[35],
                           Stk[36], Stk[37], Stk[38], Stk[39], Stk[40], Stk[41],
                           Stk[42], Stk[43]);
            break;
          case 45:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5],
                           Stk[6], Stk[7], Stk[8], Stk[9], Stk[10], Stk[11],
                           Stk[12], Stk[13], Stk[14], Stk[15], Stk[16], Stk[17],
                           Stk[18], Stk[19], Stk[20], Stk[21], Stk[22], Stk[23],
                           Stk[24], Stk[25], Stk[26], Stk[27], Stk[28], Stk[29],
                           Stk[30], Stk[31], Stk[32], Stk[33], Stk[34], Stk[35],
                           Stk[36], Stk[37], Stk[38], Stk[39], Stk[40], Stk[41],
                           Stk[42], Stk[43], Stk[44]);
            break;
          case 46:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5],
                           Stk[6], Stk[7], Stk[8], Stk[9], Stk[10], Stk[11],
                           Stk[12], Stk[13], Stk[14], Stk[15], Stk[16], Stk[17],
                           Stk[18], Stk[19], Stk[20], Stk[21], Stk[22], Stk[23],
                           Stk[24], Stk[25], Stk[26], Stk[27], Stk[28], Stk[29],
                           Stk[30], Stk[31], Stk[32], Stk[33], Stk[34], Stk[35],
                           Stk[36], Stk[37], Stk[38], Stk[39], Stk[40], Stk[41],
                           Stk[42], Stk[43], Stk[44], Stk[45]);
            break;
          case 47:
            res64 = (*Fn0)(Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5],
                           Stk[6], Stk[7], Stk[8], Stk[9], Stk[10], Stk[11],
                           Stk[12], Stk[13], Stk[14], Stk[15], Stk[16], Stk[17],
                           Stk[18], Stk[19], Stk[20], Stk[21], Stk[22], Stk[23],
                           Stk[24], Stk[25], Stk[26], Stk[27], Stk[28], Stk[29],
                           Stk[30], Stk[31], Stk[32], Stk[33], Stk[34], Stk[35],
                           Stk[36], Stk[37], Stk[38], Stk[39], Stk[40], Stk[41],
                           Stk[42], Stk[43], Stk[44], Stk[45], Stk[46]);
            break;
          case 48:
            res64 = (*Fn0)(
                Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5], Stk[6], Stk[7],
                Stk[8], Stk[9], Stk[10], Stk[11], Stk[12], Stk[13], Stk[14],
                Stk[15], Stk[16], Stk[17], Stk[18], Stk[19], Stk[20], Stk[21],
                Stk[22], Stk[23], Stk[24], Stk[25], Stk[26], Stk[27], Stk[28],
                Stk[29], Stk[30], Stk[31], Stk[32], Stk[33], Stk[34], Stk[35],
                Stk[36], Stk[37], Stk[38], Stk[39], Stk[40], Stk[41], Stk[42],
                Stk[43], Stk[44], Stk[45], Stk[46], Stk[47]);
            break;
          case 49:
            res64 = (*Fn0)(
                Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5], Stk[6], Stk[7],
                Stk[8], Stk[9], Stk[10], Stk[11], Stk[12], Stk[13], Stk[14],
                Stk[15], Stk[16], Stk[17], Stk[18], Stk[19], Stk[20], Stk[21],
                Stk[22], Stk[23], Stk[24], Stk[25], Stk[26], Stk[27], Stk[28],
                Stk[29], Stk[30], Stk[31], Stk[32], Stk[33], Stk[34], Stk[35],
                Stk[36], Stk[37], Stk[38], Stk[39], Stk[40], Stk[41], Stk[42],
                Stk[43], Stk[44], Stk[45], Stk[46], Stk[47], Stk[48]);
            break;
          case 50:
            res64 = (*Fn0)(
                Stk[0], Stk[1], Stk[2], Stk[3], Stk[4], Stk[5], Stk[6], Stk[7],
                Stk[8], Stk[9], Stk[10], Stk[11], Stk[12], Stk[13], Stk[14],
                Stk[15], Stk[16], Stk[17], Stk[18], Stk[19], Stk[20], Stk[21],
                Stk[22], Stk[23], Stk[24], Stk[25], Stk[26], Stk[27], Stk[28],
                Stk[29], Stk[30], Stk[31], Stk[32], Stk[33], Stk[34], Stk[35],
                Stk[36], Stk[37], Stk[38], Stk[39], Stk[40], Stk[41], Stk[42],
                Stk[43], Stk[44], Stk[45], Stk[46], Stk[47], Stk[48], Stk[49]);
            break;
        }
        break;

      case 7: /* save 32-bit result */
#ifndef __LP64__  /* 09Jan22 gwb - 64 bit fixup */      
        Val = *(u_int32_t*)s1 + (u_int32_t)s2;
        s1 += sizeof(u_int32_t*);
        *(u_int32_t*)Val = res64 & 0xFFFFFFFF;
#else
        Val = *(u_int64*)s1 + (u_int64)s2;
        s1 += sizeof(u_int64*);
        *(u_int64*)Val = res64 & 0xFFFFFFFFFFFFFFFF;
#endif
        break;

      case 8: /* save 64-bit result */
      /* 09Jan22 gwb - 64 bit fix */
#ifndef __LP64__      
        Val = *(u_int32_t*)s1 + (u_int32_t)s2;
        s1 += sizeof(u_int32_t*);
        *(int64*)Val = res64;
#else
        Val = *(u_int64*)s1 + (u_int64)s2;
        s1 += sizeof(u_int64*);
        *(u_int64*)Val = res64;
#endif
        break;

      case 9: /* copy from addr in eax to offset */
        break;

      case 10: /* save errno - call GetLastError() */
      /* 09Jan22 gwb - 64 bit fix */
#ifndef __LP64__      
        Val = *(u_int32_t*)s1 + (u_int32_t)s2;
        s1 += sizeof(u_int32_t*);
#else
        Val = *(u_int64*)s1 + (u_int64)s2;
        s1 += sizeof(u_int64);
#endif
        /* 09Jan22 gwb - 64 bit fix */      
#ifndef __LP64__        
        *(u_int32_t*)Val = errno;
#else
        *(u_int64*)Val = errno;
#endif        
        break;

      default:
        return;
    }
  }
}

/* ====================================================================== */

void op_ccall() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  str2                       | Result                      |
     |-----------------------------|-----------------------------|
     |  str1 - Pcode to execute    |                             |
     |=============================|=============================|
 */

#define MAX_LOCAL_STRING_LEN 1000

  /* String 1 - Pcode */
  DESCRIPTOR* arg1;
  unsigned char* s1;
  int len1 = 0;
  unsigned char ls1[MAX_LOCAL_STRING_LEN + 1];

  /* String 2 */
  DESCRIPTOR* arg2;
  unsigned char* s2;
  int len2 = 0;
  unsigned char ls2[MAX_LOCAL_STRING_LEN + 1];

  arg1 = e_stack - 2;
  k_get_string(arg1);
  if (arg1->data.str.saddr == NULL)
    k_error("CCALL string 1 null");
  len1 = arg1->data.str.saddr->string_len;
  if (len1 > MAX_LOCAL_STRING_LEN) {
    s1 = k_alloc(86, len1 + 1);
    if (s1 == NULL)
      k_error("CCALL string 1 memory allocation error");
  } else
    s1 = ls1;
  k_get_c_string(arg1, (char*)s1, len1);

  arg2 = e_stack - 1;
  k_get_string(arg2);
  if (arg2->data.str.saddr == NULL)
    k_error("CCALL string 2 null");
  len2 = arg2->data.str.saddr->string_len;
  if (len2 > MAX_LOCAL_STRING_LEN) {
    s2 = k_alloc(87, len1 + 1);
    if (s2 == NULL) {
      if (len1 > MAX_LOCAL_STRING_LEN)
        k_free(s1);
      k_error("CCALL string 2 memory allocation error");
    }
  } else
    s2 = ls2;
  k_get_c_string(arg2, (char*)s2, len2);

  k_dismiss();
  k_dismiss();

  ccall_c(s2, s1);

  k_put_string((char*)s1, len1, e_stack++);

  if (len1 > MAX_LOCAL_STRING_LEN)
    k_free(s1);
  if (len2 > MAX_LOCAL_STRING_LEN)
    k_free(s2);
}

/* END-CODE */
