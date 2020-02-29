/* LINUXLB.H
 * Windows library substitutes for Linux.
 * Copyright (c) 2002 Ladybridge Systems, All Rights Reserved
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
 * 27Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 * 
 * START-HISTORY (OpenQM):
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#ifndef __LINUXLB
#define __LINUXLB

/* Inline macros */

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

/* Simple substitutes */

#define chsize(fd, bytes) ftruncate(fd, bytes)
#define GetCurrentProcessId() getpid()
#define stricmp(a, b) strcasecmp(a, b)

#define chsize64 chsize

/* Functions in linuxlb.c */

int64 filelength64(int fd);
#define filelength(f) (int)filelength64(f)
bool GetUserName(char* name, u_int32_t* bytes);
char* itoa(int value, char* string, int radix);
char* Ltoa(int32_t value, char* string, int radix);
char* qmrealpath(char* inpath, char* outpath);
void Sleep(int32_t n);
void strrep(char* s, char old, char new);

#endif

/* END-CODE */
