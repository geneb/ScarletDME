/* DH_STAT.H
 * FILESTATS structure definition.
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
 * 27Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 * 
 * START-HISTORY (OpenQM):
 * 18 Dec 06  2.4-18 Extracted from sysseg.h
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#ifndef _DH_STAT_
#define _DH_STAT_

/* File statistic counters. See also BP INT$KEYS.H
   The size of this area is fixed at 24 long ints as it is part of the file
   header. If we ever need to go over this limit, the structure will need to
   be split in the header. Except as indicated, entries are protected by the
   FILE_TABLE_LOCK semaphore.                                                */

struct FILESTATS {
   int32_t reset;       /*  1: qmtime() value when cleared */
   int32_t opens;       /*  2: Number of opens */
   int32_t reads;       /*  3: Number of reads */
   int32_t writes;      /*  4: Number of writes */
   int32_t deletes;     /*  5: Number of deletes */
   int32_t clears;      /*  6: Number of clearfiles */
   int32_t selects;     /*  7: Number of Basic selects */
   int32_t splits;      /*  8: Number of splits */
   int32_t merges;      /*  9: Number of merges */
   int32_t ak_reads;    /* 10: Number of AK reads */
   int32_t ak_writes;   /* 11: Number of AK writes */
   int32_t ak_deletes;  /* 12: Number of AK deletes */
   int32_t spare[12];
};
#define FILESTATS_COUNTERS 12    /* Used counters */

#endif

/* END-CODE */
