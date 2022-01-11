/* QMDEFS.H
 * QM definitions common to all components.
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
 * 11Jan22 gwb Created a couple of new defines to eliminate some magic number use
 *             in the k_error() function.
 * 
 * 27Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 * 
 * START-HISTORY (OpenQM):
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 30 May 06  2.4-5 0493 UpperCase() macro should cast its argument to u_char,
 *                  not unsigned int. Otherwise it fails with signed char source
 *                  data in C2/G2.
 * 25 May 06  2.4-5 poll() is broken in Mac OS X 10.4 so set DO_NOT_USE_POLL.
 * 30 Apr 06  2.4-2 Changed NIX default permissions to 666.
 * 23 Nov 05  2.2-17 Extracted from qm.h
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#ifndef __QMDEFS
#define __QMDEFS

/* Platforms and derivations */

#define PRODUCT_KEY 2
#define PLATFORM_NAME "Linux"
#define NIX
#define BSD
#define FALLBACK
#define HOT_SPOT_MONITOR

#define _XOPEN_SOURCE
#define _XOPEN_CRYPT
#include <unistd.h>
#ifdef __APPLE__
#include <machine/endian.h>
#else
#include <endian.h>
#endif
#if __BYTE_ORDER__ == __BIG_ENDIAN__
#define BIG_ENDIAN_SYSTEM
#endif
#ifdef __APPLE__
extern char **environ;
#else
#define environ __environ
#endif

#define Seek(fu, offset, whence) lseek(fu, offset, whence)

/* Derived items */

#define DS '/'
#define DSS "/"
#define Newline "\n"
#define NewlineBytes 1

#define default_access 0666
#define O_BINARY 0
#define O_TEXT 0
#define FOPEN_READ_MODE "r"
#define FOPEN_WRITE_MODE "w"
#define NULL_DEVICE "/dev/null"

#define ALIGN2 __attribute__((aligned(2), packed))

#define MakeDirectory(path) mkdir(path, 0777)

#define QM_SHM_KEY 0x716d0101
#define QM_SEM_KEY 0x716d0102
/* To allow the GPL version and the chargeable version of QM to
 * coexist, GPL developers should use keys with the third byte
 * non-zero for any future values.                               
 */

#define RelinquishTimeslice sched_yield()

/* WINFS denotes use of the Windows file system interface (PDA only so far) */

#define OSError errno
#define OSFILE int
#define INVALID_FILE_HANDLE -1
#define ValidFileHandle(fu) (fu >= 0)
#define CloseFile(fu) close(fu)
#define Read(fu, buff, bytes) (read(fu, buff, bytes))
#define Write(fu, buff, bytes) (write(fu, buff, bytes))

#ifndef DS
#error No environment set
#endif

#define MAX_PATHNAME_LEN 255    /* Changes affect file headers */
#define MAX_ID_LEN 255          /* Increasing requires major file changes */
#define MAX_CALL_NAME_LEN 63    /* Cannot exceed MAX_ID_LEN */
#define MAX_TRIGGER_NAME_LEN 32 /* Increasing would alter file header */
#define MAX_PROGRAM_NAME_LEN 128
#define MAX_USERNAME_LEN 32
#define MAX_MATCH_TEMPLATE_LEN 256
#define MAX_MATCHED_STRING_LEN 8192
#define MAX_PACKAGES 32
#define MAX_PACKAGE_NAME_LEN 15
#define MAX_ACCOUNT_NAME_LEN 32
#define MAX_SORTMRG 10
#define MAX_SORT_KEYS 32
#define MAX_SORT_KEY_LEN 1024

#define MAX_ERROR_LINES 3        /* because I HATE magic numbers! */
#define MAX_EMSG_LEN 80          /* These are used in k_error() in k_error.c */

#define Private static

#ifndef Public
#define Public extern
#endif

#ifndef init
#define init(a)
#endif

/* ======================================================================
   Type definitions                                                       */

typedef int16_t bool;
#define FALSE 0
#define TRUE 1

typedef int64_t int64;
typedef u_int64_t u_int64;

/* ======================================================================
   Byte ordering macros                                                   */

#ifdef BIG_ENDIAN_SYSTEM
short int swap2(int16_t n);
long int swap4(int32_t n);
#define ShortInt(n) swap2(n)
#define LongInt(n) swap4(n)
#else
#define ShortInt(n) (n)
#define LongInt(n) (n)
#endif

/* ======================================================================
   Case conversion macros and data                                        */

Public char uc_chars[256];
Public char lc_chars[256];
#define UpperCase(c) (uc_chars[((u_char)(c))])
#define LowerCase(c) (lc_chars[((u_char)(c))])

Public u_char char_types[256];
#define CT_ALPHA 0x01
#define CT_DIGIT 0x02
#define CT_GRAPH 0x04
#define CT_MARK 0x08
#define CT_DELIM 0x10

#define IsAlnum(c) (char_types[((u_char)(c))] & (CT_ALPHA | CT_DIGIT))
#define IsAlpha(c) (char_types[((u_char)(c))] & CT_ALPHA)
#define IsDigit(c) (char_types[((u_char)(c))] & CT_DIGIT)
#define IsGraph(c) (char_types[((u_char)(c))] & CT_GRAPH)
#define IsDelim(c) (char_types[((u_char)(c))] & CT_DELIM)
#define IsMark(c) (char_types[((u_char)(c))] & CT_MARK)

/* Collation map */

Public char* collation_map_name init(NULL);
Public char* collation init(NULL);

#define SortCompare(s1, s2, n, nocase) \
  ((nocase) ? MemCompareNoCase(s1, s2, n) : memcmp(s1, s2, n))

#define TEXT_MARK ((char)-5)
#define SUBVALUE_MARK ((char)-4)
#define VALUE_MARK ((char)-3)
#define FIELD_MARK ((char)-2)
#define ITEM_MARK ((char)-1)

#define U_TEXT_MARK ((u_char)'\xFB')
#define U_SUBVALUE_MARK ((u_char)'\xFC')
#define U_VALUE_MARK ((u_char)'\xFD')
#define U_FIELD_MARK ((u_char)'\xFE')
#define U_ITEM_MARK ((u_char)'\xFF')

#define TEXT_MARK_STRING "\xFB"
#define SUBVALUE_MARK_STRING "\xFC"
#define VALUE_MARK_STRING "\xFD"
#define FIELD_MARK_STRING "\xFE"
#define ITEM_MARK_STRING "\xFF"

#endif

/* END-CODE */
