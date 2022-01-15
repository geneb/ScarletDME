/* CONFIG.H
 * Configuration data
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
 * 13Jan22 gwb Minor reformatting.
 * 
 * 27Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 * 
 * START-HISTORY (OpenQM):
 * 05 Oct 07  2.6-5 Added PDUMP parameter.
 * 20 Aug 07  2.6-0 Added CMDSTACK parameter.
 * 01 Jul 07  2.5-7 Extensive changes for PDA merge.
 * 28 Aug 06  2.4-12 Added FIXUSERS parameter.
 * 18 Aug 06  2.4-11 Added STARTUP parameter.
 * 22 May 06  2.4-4 Added CODEPAGE parameter.
 * 09 Mar 06  2.3-8 Added MAXCALL parameter.
 * 19 Jan 06  2.3-5 Added TEMPDIR parameter.
 * 02 Jan 06  2.3-3 Added INTPREC parameter.
 * 21 Sep 05  2.2-12 Added SH and SH1 parameters.
 * 09 Sep 05  2.2-10 Removed WIN95FIX.
 * 24 Aug 05  2.2-8 Added SPOOLER parameter.
 * 01 Aug 05  2.2-7 Added EXCLREM.
 * 21 Jun 05  2.2-1 Added journalling items.
 * 26 May 05  2.2-0 Added device licensing.
 * 08 Apr 05  2.1-12 Added DUMPDIR parameter.
 * 31 Mar 05  2.1-11 Added PORTMAP variables.
 * 15 Oct 04  2.0-5 Added ERRLOG parameter.
 * 03 Oct 04  2.0-4 Added FILERULE parameter.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

/* !!CONFIG!! All places requiring changes for config parameters are marked */

struct CONFIG {
 
  int16_t max_users;               /* User limit */
  char sysdir[MAX_PATHNAME_LEN+1];
  int16_t cmdstack;                /* CMDSTACK: Command stack depth */
  bool deadlock;                   /* DEADLOCK: Trap deadlocks */
  u_int16_t debug;                 /* DEBUG:    Controls debug features */
  int errlog;                      /* ERRLOG:   Max errlog size in bytes, 0 if disabled */
  int16_t fds_limit;               /* FDS */
  int16_t fixusers_base;           /* FIXUSERS: First user number and... */
  int16_t fixusers_range;          /*          ...Number of users */
  int16_t jnlmode;                 /* JNLMODE:  Journalling mode */
  char jnldir[MAX_PATHNAME_LEN+1]; /* JNLDIR:   Journal file directory */
  int16_t maxidlen;                /* MAXIDLEN: Maximum record id length */
  int16_t netfiles;                /* NETFILES:
                                      0x0001    Allow outgoing NFS
                                      0x0002    Allow incoming QMNet   */
  int16_t numfiles;                /* NUMFILES: Maximum number of files open */
  int16_t numlocks;                /* NUMLOCKS: Maximum number of record locks */
  int16_t pdump;                   /* PDUMP:    PDUMP mode flags */
  int16_t portmap_base_port;       /* PORTMAP: First port number ... */
  int16_t portmap_base_user;       /*          ...First user number... */
  int16_t portmap_range;           /*          ...Number of ports/users */
  char startup[80+1];              /* STARTUP: Startup command */
 };


/* Config parameters loaded per process to allow local changes */

#define MAX_SH_CMD_LEN 80
struct PCFG  {

  unsigned int codepage;                /* CODEPAGE: Set console codepage */
  char dumpdir[MAX_PATHNAME_LEN+1];     /* DUMPDIR:  Directory for process dump files */
  bool exclrem;                         /* EXCLREM:  Exclude remote files from ACCOUNT.SAVE? */
  int16_t filerule;                     /* FILERULE: Rules for special filename formats */
  double fltdiff;                       /* FLTDIFF : Wide zero for float comparisons */
  int16_t fsync;                        /* FSYNC:    Controls when to do fsync() */
  bool gdi;                             /* GDI:      Default to GDI print API? */
  int16_t grpsize;                      /* GRPSIZE:  Default group size (1-8) */
  int16_t intprec;                      /* INTPREC:  Precision for INT() etc */
  int16_t lptrhigh;                     /* LPTRHIGH: Default printer lines */
  int16_t lptrwide;                     /* LPTRWIDE: Default printer width */
  int maxcall;                          /* MAXCALL:  Maximum call depth */
  bool must_lock;                       /* MUSTLOCK: Enforce locking rules */
  int16_t objects;                      /* OBJECTS:  Max loaded objects */
  int32_t objmem;                       /* OBJMEM:   Object size limit, zero = none */
  int16_t qmclient_mode;                /* QMCLIENT: 0 = any, 1 = no open/exec, 2 = restricted call */
  int16_t reccache;                     /* RECCACHE: Record cache size */
  bool ringwait;                        /* RINGWAIT: Wait if ring buffer full */
  bool safedir;                         /* SAFEDIR:  Use careful update on dir file write */
  char sh[MAX_SH_CMD_LEN+1];            /* SH:       Command to run interactive shell */
  char sh1[MAX_SH_CMD_LEN+1];           /* SH1:      Command to run single shell */
  int32_t sortmem;                      /* SORTMEM: Limit on in-memory sort size */
  int16_t sortmrg;                      /* SORTMRG: Number of files in merge */
  char sortworkdir[MAX_PATHNAME_LEN+1]; /* SORTWORK */
  char spooler[MAX_PATHNAME_LEN+1];     /* SPOOLER: Non-default spooler */
  char tempdir[MAX_PATHNAME_LEN+1];     /* TEMPDIR */
  char terminfodir[MAX_PATHNAME_LEN+1]; /* TERMINFO */
  bool txchar;                          /* TXCHAR */
  int16_t yearbase;                     /* YEARBASE: Two digit year start */
 };

Public struct PCFG pcfg;

/* END-CODE */
