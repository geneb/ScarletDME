/* HEADER.H
 * Object code header
 * Copyright (c) 2004 Ladybridge Systems, All Rights Reserved
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
 * 08 Dec 04  2.1-0 Added HDR_CTYPE.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * *************************************************************************
 * ******                           WARNING                           ******
 * ******                                                             ******
 * ******   All changes to this file must be reflected by changes to  ******
 * ******   the HEADER.H BASIC include file.                          ******
 * ******                                                             ******
 * *************************************************************************
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */


typedef struct OBJECT_HEADER OBJECT_HEADER;

struct OBJECT_HEADER {
        u_char magic;
        u_char rev;
        int32_t id;               /* Object id of loaded object */
        int32_t start_offset;     /* Execution begins here */
        int16_t args;            /* No of arguments */
        int16_t no_vars;         /* No of variables */
        int16_t stack_depth;     /* Maximum stack requirements */
        int32_t sym_tab_offset;   /* Symbol table offset or zero */
        int32_t line_tab_offset;  /* Line table offset or zero */
        int32_t object_size;      /* Total bytes including symbol & x-ref */
        u_int16_t flags;  /* Flag bits */
#define HDR_IS_CPROC         0x0001  /* CPROC command processor */
#define HDR_INTERNAL         0x0002  /* Compiled in internal mode */
#define HDR_DEBUG            0x0004  /* Compiled in debug mode */
#define HDR_IS_DEBUGGER      0x0008  /* Debugger */
#define HDR_NOCASE           0x0010  /* Case insensitive string operations */
#define HDR_IS_FUNCTION      0x0020  /* Is a basic function */
#define HDR_VAR_ARGS         0x0040  /* Variable argument count (args = max) */
#define HDR_RECURSIVE        0x0080  /* Is a recursive program */
#define HDR_ITYPE            0x0100  /* Is an A/S/C/I-type */
#define HDR_ALLOW_BREAK      0x0200  /* Allow break key in recursive */
#define HDR_IS_TRUSTED       0x0400  /* Trusted program */
#define HDR_NETFILES         0x0800  /* Allow remote files by NFS */
#define HDR_CASE_SENSITIVE   0x1000  /* Program uses case sensitive names */
#define HDR_QMCALL_ALLOWED   0x2000  /* Can be called via QMCall() */
#define HDR_CTYPE            0x4000  /* Is a C-type */
#define HDR_IS_CLASS         0x8000  /* Is CLASS module */

        int32_t compile_time;     /* Date * 86400 + time (UNIX Epoch time) */
/* Extended header : Items differ depending on object type */
        union {
               struct {
                       int16_t refs;   /* Reference count of loaded object */
                       char program_name[MAX_PROGRAM_NAME_LEN+1];
                      } prog;
               struct {
                       u_char totals;    /* Number of TOTAL() functions */
                      } i_type;
              } ext_hdr;
} ALIGN2;

#ifdef BIG_ENDIAN_SYSTEM
   #define HDR_MAGIC 0x65
   #define HDR_MAGIC_INVERSE 0x64
#else
   #define HDR_MAGIC 0x64
   #define HDR_MAGIC_INVERSE 0x65
#endif


#define OBJECT_HEADER_SIZE 165    /* Debug header appears here if present */
#define ITYPE_HEADER_SIZE   36

#define ProgramName(obj) \
   ((((OBJECT_HEADER *)(obj))->flags & HDR_ITYPE)?"I-type":\
   ((OBJECT_HEADER *)(obj))->ext_hdr.prog.program_name)

void convert_object_header(OBJECT_HEADER * obj_header);




/* END-CODE */
