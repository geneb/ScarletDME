/* KERNEL.H
 * Run Machine Kernel include file
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
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 29 Nov 06  2.4-17 0528 FLAG_COPY_MASK should not include IS_CLEXEC otherwise
 *                   STOP in subroutine called from program run by Proc does not
 *                   clear down call stack.
 * 31 May 06  2.4-5 Moved printer on flag into PROGRAM structure.
 * 23 Mar 06  2.3-9 Added arg_ct to PROGRAM structure.
 * 22 Mar 06  2.3-9 Save OBJDATA pointer for active object program.
 * 20 Oct 05  2.2-15 Pcode now in shared memory.
 * 20 Sep 05  2.2-11 Added IS_CLEXEC for EXECUTE CURRENT.LEVEL pseudo command
 *                   processor.
 * 20 Sep 05  2.2-11 User map now dynamic.
 * 01 Sep 05  2.2-9 Added EVT_MESSAGE.
 * 04 Aug 05  2.2-7 Added EVT_TERMINATE event.
 * 21 Jun 05  2.2-1 Added journalling data.
 * 20 Jun 05  2.2-1 Added EVT_FLUSH_CACHE.
 * 31 Mar 05  2.1-11 Added PORTMAP variables.
 * 22 Dec 04  2.1-0 Added saved_capture_data to PROGRAM structure.
 * 15 Oct 04  2.0-5 Added ERRLOG_LOCK semaphore.
 * 28 Sep 04  2.0-3 Added config_path.
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

Public short int recursion_depth;     /* Recursion depth */
Public bool break_pending;            /* Break seen with breaks inhibited */

Public unsigned short int object_key;

Public int exit_status init(0);       /* Final exit status */
Public char * entry_dir init(NULL);   /* Current directory on entry */

Public bool is_phantom init(FALSE);   /* Process is a phantom */
Public bool is_QMVbSrvr init(FALSE);  /* Process is a QMVbSrvr */
Public bool is_nt init(FALSE);        /* Windows NT/2000/XP? */

Public int phantom_user_index init(0);/* User table index for phantom */

/* Connection data */
#define CN_NONE        0x00
#define CN_CONSOLE     0x01
#define CN_SOCKET      0x02
#define CN_PIPE        0x03
#define CN_PORT        0x04
#define CN_WINSTDOUT   0x05
Public short int connection_type init(CN_CONSOLE);

Public char ip_addr[15+1] init("");    /* IP address and... */
Public int port_no init(0);            /* ...port number for telnet */
Public char port_name[20+1] init("");  /* Port name for serial connection */
Public int forced_user_no init(0);     /* Login as specific user */

Public bool hsm init(FALSE);           /* Hot spot monitor enabled? */


Public char config_path[MAX_PATHNAME_LEN+1] init("");
Public char * single_command init(NULL);   /* User typed "QM xxx" */
Public char * forced_account init(NULL);   /* User typed "QM -Axxx" */

Public char command_processor[MAX_PROGRAM_NAME_LEN+1] init("$CPROC");

Public char private_catalogue[MAX_PATHNAME_LEN+1] init("cat");

Public char primary_collation_map[256];   /* For everything except AKs */
Public char * primary_collation init(NULL); /* Points to above map if active,
                                               else NULL.                   */

Public DESCRIPTOR * watch_descr init(NULL);

Public short int cproc_level init(0);
Public bool rtnlist init(FALSE);

Public bool map_dir_ids init(TRUE);   /* Map illegal characters in names */

/* Define dynamically loaded pcode items */

#define Pcode(a) Public u_char * pcode_##a;
#include <pcode.h>

/* ======================================================================
   Program control data                                                   */

struct PROGRAM {
                   struct PROGRAM * prev;
                   short int no_vars;
                   DESCRIPTOR * vars;
                   unsigned long int flags; 
/* MS 16 bits, kernel specific; LS 16 bits from object header */
#define IS_EXECUTE     0x00010000L  /* Started via EXECUTE */
#define IS_CLEXEC      0x00020000L  /* Is pseudo CPROC from EXECUTE CURRENT.LEVEL */
#define IGNORE_ABORTS  0x00040000L  /* Ignore aborts from EXECUTEd sentence */
#define PF_IS_TRIGGER  0x00080000L  /* Is trigger program */
#define SORT_ACTIVE    0x00100000L  /* Program has sort in progress */
#define PF_IS_VFS      0x00200000L  /* Is VFS handler */
#define PF_CAPTURING   0x00400000L  /* Capture data stacked for this CPROC */
#define PF_IN_TRIGGER  0x00800000L  /* This or lower program is a trigger */
#define PF_PRINTER_ON  0x01000000L  /* PRINTER ON? */
#define FLAG_COPY_MASK 0x01800000L  /* Copy these flags to called program */
                   long int col1;
                   long int col2;
                   short int precision;
                   u_char * saved_c_base;
                   long int saved_pc_offset;
                   char saved_prompt_char;
                   STRING_CHUNK * saved_capture_head;
                   STRING_CHUNK * saved_capture_tail;
                   char * break_handler;   /* Break handler name */
                   OBJDATA * objdata;
#define MAX_GOSUB_DEPTH 256
                   long int gosub_stack[MAX_GOSUB_DEPTH];
                   short int gosub_depth;
                   u_char arg_ct;      /* Number of arguments passed */
                   short int e_stack_depth; /* Depth on entry to program */
                  };

/* ======================================================================
   Process control data                                                   */

struct PROCESS {
                /* Dispatch loop control */
                short int k_abort_code; /* @ABORT.CODE value */

                short int user_no;      /* -1 for cleanup, -2 for qmlnxd */
                char username[MAX_USERNAME_LEN+1];

                /* Program control */
                struct PROGRAM program; /* Current program state */
                int call_depth;

                bool debugging;              /* Debugger active? */

                /* Common areas */
                ARRAY_HEADER * named_common; /* Head of named common header chain */
                ARRAY_HEADER * syscom;       /* $SYSCOM (also in named_common) */

                /* Opcode actions */
                bool for_init;          /* Used by FORINIT and FORTEST */
                short int break_inhibits;  /* Count of BREAK OFF calls */
                unsigned short int op_flags;     /* Opcode prefix flags */
/* Exact meaning of these flags is opcode dependent, especially the top byte */
#define P_ON_ERROR  0x0001  /* ONERROR opcode executed */
#define P_LOCKED    0x0002  /* NOWAIT opcode executed */
#define P_LLOCK     0x0004  /* LLOCK opcode executed       ** See int$keys.h */
#define P_ULOCK     0x0008  /* ULOCK opcode executed       ** See int$keys.h */
#define P_READONLY  0x0010  /* READONLY opcode executed */
#define P_PICKREAD  0x0020  /* PICKREAD opcode executed */
#define P_REC_LOCKS  (P_LLOCK | P_ULOCK)  /* Either LLOCK or ULOCK */
                bool numeric_array_allowed; /* Opcode allows numeric arrays? */
                long int status;        /* Value from STATUS() function */
                long int inmat;         /* Value of INMAT() function */
                long int os_error;      /* Operating system error number */

                unsigned long txn_id;   /* Transaction id. 0 if none */
               };
Public struct PROCESS process;

/* Evaluation stack control */
Public DESCRIPTOR * e_stack_base; /* Base of e-stack */
Public DESCRIPTOR * e_stack;      /* Ptr to next e-stack descriptor */
Public short int e_stack_depth init(0);   /* Depth of e-stack */


Public short int k_exit_cause; /* Kernel dispatch loop exit cause */
   /* Normal exit causes (some also in BP int$keys.h) */
   #define K_RETURN          0x0001    /* Return to caller */
   #define K_STOP            0x0002    /* STOP or Q from break actions */
   #define K_ABORT           0x0003    /* ABORT or A from break actions */
   #define K_CHAIN           0x0004    /* Chain a new CPROC command, not a PROC */
   #define K_EXIT_RECURSIVE  0x0005    /* RETURN from recursive code */
   #define K_LOGOUT          0x0006    /* Logout process on final return */
   #define K_TOGGLE_TRACER   0x0007    /* Enter or leave trace mode */
   #define K_CHAIN_PROC      0x0010    /* Chain to a PROC */
   /* Special causes */
   #define K_QUIT            0x0020    /* Ctrl-C */
   #define K_TERMINATE       0x0040    /* Forced logout of process */
   /* Any bits in K_INTERRUPT cause waits for keyboard input to be aborted
      and may also terminate other lengthy operations.                     */
   #define K_INTERRUPT       0x0060    /* Bit values for interrupt causes */

Public void * object;           /* Pointer to current OBJECT */
Public u_char * c_base;         /* Base address of current object code */
Public u_char * pc;             /* Next opcode byte */
Public u_char * op_pc;          /* Address of current opcode */


/* FILE_VAR allocation */
Public long int next_fvar_index init(0);


/* END-CODE */
