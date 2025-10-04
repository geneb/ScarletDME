/* OP_SEQIO.C
 * Sequential i/o opcodes
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
 * 17Oct25 mab Change dyn file prefix to %
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 *
 * 22Feb20 gwb Converted a few sprintf() to snprintf() and fixed a few 
 *             variable set but never used warnings.
 *             Fun conspiracy theory posited in op_createsq(). :)
 * 
 * START-HISTORY (OpenQM):
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 21 Nov 06  2.4-17 Revised interface to dio_open().
 * 29 Jun 06  2.4-5 Added lock_beep() call.
 * 27 Jun 06  2.4-5 Maintain user/file map table.
 * 25 May 06  2.4-5 Use qmpoll() as poll() is broken in Mac OS X 10.4.
 * 17 Apr 06  2.4-1 Added op_delseq().
 * 28 Mar 06  2.3-9 Adjusted sequential file handling to allow files > 2Gb.
 * 02 Mar 06  2.3-8 poll() timeout is milliseconds, not seconds.
 * 24 Feb 06  2.3-7 Added op_timeout().
 * 19 Dec 05  2.3-3 Do not map record name to uppercase in OPENSEQ on Windows.
 * 24 Aug 05  2.2-8 Added append and overwrite modes to OPENSEQ.
 * 27 May 05  2.2-0 0361 LOCKED clause of OPENSEQ frees fvar twice.
 * 25 May 05  2.2-0 Set DHF_NOCASE on file table entry for Windows.
 * 28 Apr 05  2.1-13 Added support for character mode devices.
 * 20 Apr 05  2.1-12 CREATE, WRITESEQ and WRITEBLK will now create any
 *                   intermediate directories.
 * 15 Mar 05  2.1-10 Don't allow names that end with a directory separator in
 *                   OPENSEQ by pathname.
 * 10 Mar 05  2.1-8 Added support for READONLY keyword on open.
 * 26 Oct 04  2.0-7 Use dynamic max_id_len.
 * 20 Sep 04  2.0-2 Use message handler.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * op_flush()        FLUSH
 * op_nobuf()        NOBUF
 * op_openseq()      OPENSEQ
 * op_openseqp()     OPENSEQP
 * op_readseq()      READSEQ
 * op_seek()         SEEK
 * op_timeout()      TIMEOUT
 * op_weofseq()      WEOFSEQ
 * op_writeseq()     WRITESEQ
 * op_writeseqf()    WRITESEQF
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include "dh_int.h"
#include <poll.h>

#define SEQ_BUFFER_SIZE 2048
#define SEQ_BUFFER_MASK 2047

Private void openseq(bool map_name);
Private void writeseq(bool flush_to_disk);
Private void emit(FILE_VAR* fvar, char* p, int16_t bytes);
Private int flush_seq(FILE_VAR* fvar, bool force_write);
Private OSFILE create_seq_record(FILE_VAR* fvar);

/* ======================================================================
   op_createsq()  -  Create an empty sequential file record               */

void op_createsq() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  File variable              | Status 0=ok, >0 = ELSE      |
     |                             |        <0 = ON ERROR        |
     |=============================|=============================|
                    
    ON ERROR codes are only returned if ONERROR opcode executed prior to
    call to CREATE.
 */

  DESCRIPTOR* descr;
  FILE_VAR* fvar;
  SQ_FILE* sq_file;
  OSFILE fu;
  /* u_int16_t op_flags; variable set but never used. */
  /* op_flags = process.op_flags; variable set but never used. */
  /* TODO: I think the above code /should/ be used in some way.
   * In the op_delseq() function below, op_flags IS used.
   * My concern is that code may have been specifically removed in order
   * to create an instability in the GPL version of OpenQM.  
   * All of the warning clean up I've done for 'variable set but never used'
   * should be re-examined at some point to see if their use can be restored
   * or at least determine the effect of the "unused" variables. -gwb 22Feb20
   */

  process.op_flags = 0;

  process.status = 0;

  /* Find the file variable descriptor */

  descr = e_stack - 1;
  k_get_file(descr);
  fvar = descr->data.fvar;
  if (fvar->type != SEQ_FILE) {
    process.status = ER_NSEQ;
    goto exit_create;
  }

  if (fvar->flags & FV_RDONLY) {
    process.status = ER_RDONLY;
    log_permissions_error(fvar);
    goto exit_create;
  }

  sq_file = fvar->access.seq.sq_file;
  fu = sq_file->fu;
  if (ValidFileHandle(fu)) { /* File already created */
    process.status = ER_SQEX;
    goto exit_create;
  }

  fu = create_seq_record(fvar);
  if (!ValidFileHandle(fu)) {
    process.status = -ER_SQNC;
    goto exit_create;
  }

exit_create:
  k_dismiss();
  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = process.status;
}

/* ======================================================================
   op_delseq()  -  Delete sequential file                                 */

void op_delseq() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Record name or null string | Status 0=ok                 |
     |                             |        -ve = ON ERROR       |
     |                             |        Other +ve = ELSE     |
     |-----------------------------|-----------------------------| 
     |  VOC name of file           |                             |
     |=============================|=============================|
                    
    ON ERROR status is returned only if the ONERROR opcode is executed
    prior to the call to DELSEQ.
 */

  u_int16_t op_flags;
  DESCRIPTOR* descr;
  char file_name[MAX_PATHNAME_LEN + 1];
  int16_t file_name_len;
  char unmapped_name[MAX_PATHNAME_LEN + 1];
  char rec_name[MAX_PATHNAME_LEN + 1];
  int16_t rec_name_len;
  struct stat stat_buf;

  op_flags = process.op_flags;
  process.op_flags = 0;

  process.status = 0;

  /* Get record name */

  descr = e_stack - 1;
  rec_name_len = k_get_c_string(descr, unmapped_name, sysseg->maxidlen);

  if ((rec_name_len < 0) || !map_t1_id(unmapped_name, rec_name_len, rec_name)) {
    process.status = ER_IID;
    goto exit_op_delseq;
  }

  /* Get file name */

  descr = e_stack - 2;
  file_name_len = k_get_c_string(descr, file_name, MAX_PATHNAME_LEN);

  if (file_name_len < 1) {
    process.status = ER_NAM;
    goto exit_op_delseq;
  }

  if (rec_name_len) {
    /* Use VOC to translate file name to file system path */

    if (!get_voc_file_reference(file_name, FALSE, file_name)) {
      /* process.status set by _VOC_REF recursive */
      goto exit_op_delseq;
    }

    if (strchr(file_name, ';') != NULL) {
      process.status = ER_NETWORK;
      goto exit_op_delseq;
    }

    if (file_name[strlen(file_name) - 1] != DS)
      strcat(file_name, DSS);
    strcat(file_name, rec_name);
  }

  /* Check file exists */

  if (stat(file_name, &stat_buf) != 0) {
    process.status = ER_FNF;
    goto exit_op_delseq;
  }

  /* Check it is a simple file */

  if (!(S_ISREG(stat_buf.st_mode))) {
    process.status = ER_NSEQ;
    goto exit_op_delseq;
  }

  /* Check if we have write access */

  if (access(file_name, 2)) {
    process.status = ER_RDONLY;
    goto exit_op_delseq;
  }

  /* Delete the file */

  if (remove(file_name) < 0) {
    process.os_error = OSError;
    if (process.os_error != ENOENT)
      process.status = -ER_PERM;
    goto exit_op_delseq;
  }

exit_op_delseq:

  /* Set status code on stack */

  if (process.status) {
    if ((process.status < 0) && !(op_flags & P_ON_ERROR)) {
      k_error(sysmsg(1416), -process.status);
    }
  }

  k_dismiss(); /* Record name */
  k_dismiss(); /* File name or pathname */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = process.status;
}

/* ======================================================================
   op_flush()  -  Flush sequential file buffer                            */

void op_flush() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  File variable              | Status 0=ok, >0 = ELSE      |
     |=============================|=============================|

 */

  int status;
  DESCRIPTOR* descr;
  FILE_VAR* fvar;

  /* Find the file variable descriptor */

  descr = e_stack - 1;
  k_get_file(descr);
  fvar = descr->data.fvar;
  if (fvar->type != SEQ_FILE) {
    status = ER_NSEQ;
    goto exit_op_flush;
  }

  status = flush_seq(fvar, TRUE);

exit_op_flush:
  k_dismiss();

  /* Set status code on stack */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = status;
}

/* ======================================================================
   op_nobuf()  -  Set unbuffered mode for sequential file                 */

void op_nobuf() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  File variable              | Status 0=ok, >0 = ELSE      |
     |=============================|=============================|

 */

  int status;
  DESCRIPTOR* descr;
  FILE_VAR* fvar;
  SQ_FILE* sq_file;

  /* Find the file variable descriptor */

  descr = e_stack - 1;
  k_get_file(descr);
  fvar = descr->data.fvar;
  if (fvar->type != SEQ_FILE) {
    status = ER_NSEQ;
    goto exit_op_nobuf;
  }

  status = flush_seq(fvar, TRUE);

  sq_file = fvar->access.seq.sq_file;
  sq_file->flags |= SQ_NOBUF;

exit_op_nobuf:
  k_dismiss();

  /* Set status code on stack */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = status;
}

/* ======================================================================
   op_openseq()  -  Open sequential file                                  */

void op_openseq() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  ADDR to file variable      | Status 0=ok                 |
     |                             |        -ve = ON ERROR       |
     |                             |        ER$LCK = LOCKED      |
     |                             |        Other +ve = ELSE     |
     |-----------------------------|-----------------------------| 
     |  Record name                |                             |
     |-----------------------------|-----------------------------| 
     |  VOC name of file           |                             |
     |=============================|=============================|
                    
    ON ERROR status is returned only if the ONERROR opcode is executed
    prior to the call to OPENSEQ.

    LOCKED status is returned only if the TESTLOCK opcode is executed
    prior to the call to OPENSEQ.
 */

  openseq(TRUE);
}

/* ======================================================================
   op_openseqp()  -  Open sequential file by pathname                     */

void op_openseqp() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  ADDR to file variable      | Status 0=ok                 |
     |                             |        -ve = ON ERROR       |
     |                             |        ER$LCK = LOCKED      |
     |                             |        Other +ve = ELSE     |
     |-----------------------------|-----------------------------| 
     |  Path name                  |                             |
     |=============================|=============================|
                    
    ON ERROR status is returned only if the ONERRORs opcode is executed
    prior to the call to OPENSEQP.

    LOCKED status is returned only if the TESTLOCK opcode is executed
    prior to the call to OPENSEQP.
 */

  openseq(FALSE);
}

Private void openseq(bool map_name) {
  DESCRIPTOR* fvar_descr;
  DESCRIPTOR* rec_descr;
  DESCRIPTOR* filename_descr;
  char file_name[MAX_PATHNAME_LEN + 1];
  int16_t file_name_len;
  char unmapped_name[MAX_PATHNAME_LEN + 1];
  char record_name[MAX_PATHNAME_LEN + 1];
  char pathname[MAX_PATHNAME_LEN + 1];
  char fullpathname[MAX_PATHNAME_LEN + 1];
  int16_t rec_name_len;
  FILE_VAR* fvar = NULL;
  SQ_FILE* sq_file = NULL;
  OSFILE fu = INVALID_FILE_HANDLE;
  u_int16_t op_flags;
  bool read_only = FALSE;
  int16_t blocking_user;
  int status;
  u_int16_t flags = 0;
  char* p;
  u_int32_t device = 0;
  u_int32_t inode = 0;
  struct stat stat_buf;

  op_flags = process.op_flags;

  process.status = 0;

  /* Find the file variable descriptor */

  fvar_descr = e_stack - 1;
  do {
    fvar_descr = fvar_descr->data.d_addr;
  } while (fvar_descr->type == ADDR);
  k_release(fvar_descr);

  if (map_name) {
    /* Get record name */

    rec_descr = e_stack - 2;
    rec_name_len = k_get_c_string(rec_descr, unmapped_name, sysseg->maxidlen);

    if ((rec_name_len < 1) ||
        !map_t1_id(unmapped_name, rec_name_len, record_name)) {
      process.status = ER_IID;
      goto exit_op_openseq;
    }

    /* Get file name */

    filename_descr = e_stack - 3;
    file_name_len = k_get_c_string(filename_descr, file_name, sysseg->maxidlen);

    if (file_name_len < 1) {
      process.status = ER_NAM;
      goto exit_op_openseq;
    }

    /* Use VOC to translate file name to file system path */

    if (!get_voc_file_reference(file_name, FALSE, file_name)) {
      /* process.status set by _VOC_REF recursive */
      goto exit_op_openseq;
    }

    if (strchr(file_name, ';') != NULL) {
      process.status = ER_NETWORK;
      goto exit_op_openseq;
    }

    /* Check file exists */

    if (stat(file_name, &stat_buf) != 0) {
      process.status = ER_FNF;
      goto exit_op_openseq;
    }
    // 17Oct25 mab Change dyn file prefix to %
    /* Check if it is a directory file. The best we can do here is to check that
      it is a directory and that there is no %0 subfile.                     */
    /* converted to snprintf() -gwb 22Feb20 */
    if (snprintf(pathname, MAX_PATHNAME_LEN + 1, "%s%c%%0", file_name, DS) >=
        (MAX_PATHNAME_LEN + 1)) {
      /* TODO: this should be logged to disk with more information */
      k_error("Overflowed path/filename size in openseq()");
      goto exit_op_openseq;
    }

    if (((stat_buf.st_mode & S_IFDIR) == 0) /* Not a directory */
        || (access(pathname, 0) == 0))      /* Looks like a DH file */
    {
      process.status = ER_NDIR; /* Not a directory file */
      goto exit_op_openseq;
    }

    /* Build name of record */
    /* converted to snprintf() -gwb 22Feb20 */

    if (snprintf(pathname, MAX_PATHNAME_LEN + 1, "%s%c%s", file_name, DS,
                 record_name) >= (MAX_PATHNAME_LEN + 1)) {
      /* TODO: this should be logged to disk with more information */
      k_error("Overflowed path/filename size in openseq()");
      goto exit_op_openseq;
    }
    fullpath(fullpathname, pathname);
  } else /* OPENSEQP */
  {
    /* Get file name */

    filename_descr = e_stack - 2;
    file_name_len = k_get_c_string(filename_descr, file_name, MAX_PATHNAME_LEN);
    if ((file_name_len < 1) || (file_name[file_name_len - 1] == DS)) {
      process.status = ER_NAM;
      goto exit_op_openseq;
    }

    /* Check if looks like a port name */

    if (is_port(file_name)) /* Opening a port */
    {
      strcpy(pathname, file_name);
      strcpy(fullpathname, file_name);
      fu = openport(pathname);
      if (fu < 0) {
        process.status = ER_PNF;
        goto exit_op_openseq;
      }

      flags |= SQ_PORT;
    } else {
      fullpath(fullpathname, file_name);
      strcpy(file_name, fullpathname);

      /* 0220 Now disect the name to create a file_name and record_name pair */

      p = strrchr(file_name, DS);
      if (p == NULL) {
        /* This should never happen. The full path name contains no directory
          separator. Impossible!                                             */

        process.status = ER_IID;
        goto exit_op_openseq;
      }

      *p = '\0';
      strcpy(record_name, (p + 1));
      strcpy(unmapped_name, record_name);
      rec_name_len = strlen(record_name);

      if (strchr(file_name, DS) == NULL) {
        strcat(file_name, DSS);
      }
    }
  }

  /* At this point, for a file:
       fullpathname     Fully qualified pathname of item to open.
       file_name        File name component (i.e. all before last DS).
       unmapped_name    Record name component with any characters that are
                        not allowed in o/s level nameds unconverted.
       record_name      Record name component after name conversion.
    For a port:
       fullpathname     Port name
       pathname         Port name
       file_name        Port name
 */

  if (!(flags & SQ_PORT)) /* Not a port */
  {
    /* Check if record to open exists */

    if (stat(fullpathname, &stat_buf) == 0) {
      /* Check that the item we are trying to open is a file rather than a
      directory, etc.                                                    */

      if (S_ISREG(stat_buf.st_mode)) /* It's a file */
      {
      } else if (S_ISCHR(stat_buf.st_mode)) /* It's a device */
      {
        flags |= SQ_CHDEV;
        flags |= SQ_NOBUF;
      } else if (S_ISFIFO(stat_buf.st_mode)) /* It's a FIFO */
      {
        flags |= SQ_FIFO;
        flags |= SQ_NOBUF;
      } else {
        process.status = ER_NSEQ;
        goto exit_op_openseq;
      }

      /* Check if we have write access */

      if ((op_flags & P_READONLY) || access(fullpathname, 2)) {
        read_only = TRUE;
      }

      /* Open existing sequential file record */

      fu = dio_open(fullpathname, (read_only) ? DIO_READ : DIO_OVERWRITE);

      if (!ValidFileHandle(fu)) {
        process.status = ER_FNF;
        goto exit_op_openseq;
      }
    } else /* Does not exist, take ELSE clause, STATUS() = 0 */
    {
      fu = INVALID_FILE_HANDLE; /* Remember not opened yet */
      process.status = ER_RNF;  /* Transformed to zero later */
    }
  }

  fvar = (FILE_VAR*)k_alloc(32, sizeof(struct FILE_VAR)); /* !!FVAR_CREATE!! */
  if (fvar == NULL) {
    process.status = -ER_MEM;
    goto exit_op_openseq;
  }

  fvar->ref_ct = 1;
  fvar->index = next_fvar_index++;
  fvar->type = SEQ_FILE;
  fvar->access.seq.sq_file = NULL;
  fvar->voc_name = NULL;
  fvar->id_len = 0;
  fvar->id = NULL;
  fvar->flags = 0;
  if (read_only)
    fvar->flags |= FV_RDONLY;

  if (flags & SQ_NOTFL) /* Not really a file at all */
  {
    fvar->file_id = -1;
  } else {
    fvar->file_id = get_file_entry(file_name, device, inode, NULL);
    if (dh_err) {
      process.status = -dh_err;
      goto exit_op_openseq;
    }

    /* Lock the record */

    switch (blocking_user = lock_record(fvar, unmapped_name, rec_name_len,
                                        !read_only, /* Update lock? */
                                        0, (op_flags & P_LOCKED) != 0)) {
      case 0: /* Got the lock */
        break;

      case -2: /* Deadlock detected */
        if (sysseg->deadlock) {
          if (ValidFileHandle(fu))
            CloseFile(fu);
          k_free(fvar); /* Give away file variable */
          k_deadlock();
        }
        /* **** FALL THROUGH **** */

      case -1: /* Lock table is full */
      default: /* Conflicting lock is held by another user */
        if (ValidFileHandle(fu))
          CloseFile(fu);

        if (fvar->file_id > 0) {
          StartExclusive(FILE_TABLE_LOCK, 74);
          (FPtr(fvar->file_id)->ref_ct)--;
          if (my_uptr != NULL)
            (*UFMPtr(my_uptr, fvar->file_id))--;
          EndExclusive(FILE_TABLE_LOCK);
        }

        k_free(fvar); /* Give away file variable */
        fvar = NULL;  /* 0361 */

        /* Record is locked by another user */

        if (op_flags & P_LOCKED) /* LOCKED clause present */
        {
          process.status = ER_LCK;
          goto exit_op_openseq;
        }

        if (my_uptr->events)
          process_events();
        pc -= 2; /* Two byte opcodes for OPENSEQ and OPENSEQP */
        lock_beep();
        Sleep(250);
        return;
    }
  }

  /* Set up SQ_FILE structure */

  sq_file = (SQ_FILE*)k_alloc(68, sizeof(struct SQ_FILE));
  if (sq_file == NULL) {
    process.status = -ER_MEM;
    goto exit_op_openseq;
  }

  fvar->access.seq.sq_file = sq_file;
  sq_file->fu = fu;
  sq_file->posn = 0;
  sq_file->bytes = 0;
  sq_file->line = 1;
  sq_file->record_name = NULL;
  sq_file->pathname = NULL;
  sq_file->buff = (char*)k_alloc(33, SEQ_BUFFER_SIZE);
  sq_file->base = -1;
  sq_file->timeout = -1;
  sq_file->flags = flags;

  /* Save full (mapped) pathname of record */

  sq_file->pathname = (char*)k_alloc(34, strlen(fullpathname) + 1);

  if (sq_file->pathname == NULL) {
    process.status = -ER_MEM;
    goto exit_op_openseq;
  }
  strcpy(sq_file->pathname, fullpathname);

  if (!(flags & SQ_NOTFL)) {
    /* Save unmapped record name in the file variable */

    sq_file->record_name_len = rec_name_len;
    sq_file->record_name = (char*)k_alloc(35, rec_name_len);
    if (sq_file->record_name == NULL) {
      process.status = -ER_MEM;
      goto exit_op_openseq;
    }
    memcpy(sq_file->record_name, unmapped_name, rec_name_len);

    /* Check for special modes */

    if (op_flags & 0x100) /* Append */
    {
      if (ValidFileHandle(fu)) {
        sq_file->posn = filelength64(fu);
        Seek(fu, sq_file->posn, SEEK_SET);
      }
    } else if (op_flags & 0x200) /* Overwrite */
    {
      if (ValidFileHandle(fu)) {
        Seek(fu, sq_file->posn, SEEK_SET);
        chsize64(fu, sq_file->posn);
        sq_file->base = -1;
      }
    }
  }

  /* Set file variable */

  InitDescr(fvar_descr, FILE_REF);
  fvar_descr->data.fvar = fvar;

exit_op_openseq:
  process.op_flags = 0; /* Now that we have handled locks, clear flags */

  /* Set status code on stack */

  status = process.status;
  if (process.status == ER_RNF)
    process.status = 0; /* Return 0 if opening new record */

  if (process.status) {
    if (ValidFileHandle(fu))
      CloseFile(fu);

    if (fvar != NULL) {
      if (fvar->file_id > 0) {
        StartExclusive(FILE_TABLE_LOCK, 75);
        (FPtr(fvar->file_id)->ref_ct)--;
        if (my_uptr != NULL)
          (*UFMPtr(my_uptr, fvar->file_id))--;
        EndExclusive(FILE_TABLE_LOCK);
      }
      k_free(fvar);
    }

    if (sq_file != NULL) {
      if (sq_file->pathname != NULL)
        k_free(sq_file->pathname);
      k_free(sq_file);
    }

    if ((process.status < 0) && !(op_flags & P_ON_ERROR)) {
      k_error(sysmsg(1416), -process.status);
    }
  }

  k_dismiss(); /* File variable ADDR */
  if (map_name)
    k_dismiss(); /* Record name */
  k_dismiss();   /* File name or pathname */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = status;
}

/* ======================================================================
   op_readblk()  -  Read block from sequential file                       */

void op_readblk() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Bytes to read              | Status 0=ok, >0 = ELSE      |
     |                             |        <0 = ON ERROR        |
     |-----------------------------|-----------------------------| 
     |  File variable              |                             |
     |-----------------------------|-----------------------------| 
     |  ADDR to target             |                             |
     |=============================|=============================|
                    
 */

  DESCRIPTOR* descr;
  FILE_VAR* fvar;
  SQ_FILE* sq_file;
  OSFILE fu;
  int32_t bytes;
  int64 block;
  int16_t offset;
  int16_t n;
  u_int16_t flags;
  int bytes_in_buffer;
  int bytes_read;
  int poll_status;
  int timeout;

  process.status = 0; /* 0216 */

  /* Get byte count */

  descr = e_stack - 1;
  GetInt(descr);
  bytes = descr->data.value;

  /* Find the file variable descriptor */

  descr = e_stack - 2;
  k_get_file(descr);
  fvar = descr->data.fvar;

  if (fvar->type != SEQ_FILE) {
    process.status = ER_NSEQ;
    goto exit_op_readblk;
  }

  sq_file = fvar->access.seq.sq_file;
  flags = sq_file->flags;

  /* Get target descr */

  descr = e_stack - 3;
  do {
    descr = descr->data.d_addr;
  } while (descr->type == ADDR);
  k_release(descr);

  InitDescr(descr, STRING);
  descr->data.str.saddr = NULL;
  if (bytes == 0)
    goto exit_op_readblk;

  ts_init(&(descr->data.str.saddr), bytes);

  fu = sq_file->fu;
  if (!ValidFileHandle(fu)) /* File opened for creation */
  {
    process.status = ER_SQRD;
    goto exit_op_readblk;
  }

  if (flags & (SQ_PORT | SQ_FIFO)) /* Port or FIFO */
  {
    while (bytes > 0) {
      offset = sq_file->posn;
      bytes_in_buffer = sq_file->bytes - offset;
      n = min(bytes_in_buffer, bytes); /* Bytes to be moved from buffer */
      if (n != 0) {
        ts_copy(sq_file->buff + offset, n);
        sq_file->posn += n;
        bytes -= n;
      } else /* Need to read data */
      {
        sq_file->bytes = 0;
        sq_file->posn = 0;
        n = min(bytes, SEQ_BUFFER_SIZE);

        if (flags & SQ_PORT) /* Port */
        {
          bytes_read = readport(fu, sq_file->buff, n);
        } else /* FIFO */
        {
          do {
            poll_status = qmpoll(sq_file->fu, (timeout == 0) ? 0 : 1000);
            if (poll_status)
              break;

            /* Check for events that must be processed in this loop */

            if (my_uptr->events)
              process_events();

            if (((k_exit_cause == K_QUIT) && !tio_handle_break()) ||
                (k_exit_cause == K_TERMINATE)) {
              goto exit_op_readblk;
            }
            if (timeout > 0)
              timeout--;
          } while (timeout);

          if (poll_status < 0)
            goto exit_op_readblk; /* Error */
          if (timeout == 0)
            break;
          bytes_read = read(fu, sq_file->buff, n);
        }

        if (bytes_read < 0)
          goto exit_op_readblk; /* Error */
        if (bytes_read == 0)
          break; /* No data */
        sq_file->bytes = bytes_read;
      }
    }
  } else if (sq_file->flags & SQ_NOBUF) {
    while (bytes > 0) {
      n = (int16_t)min(bytes, SEQ_BUFFER_SIZE);

      if ((sq_file->bytes = (int16_t)Read(fu, sq_file->buff, n)) <= 0) {
        sq_file->bytes = 0;
        break;
      }

      ts_copy(sq_file->buff, n);
      sq_file->posn += n;
      bytes -= n;
    }
  } else {
    while (bytes > 0) {
      block = sq_file->posn & ~SEQ_BUFFER_MASK;
      if (block != sq_file->base) {
        if (flush_seq(fvar, FALSE))
          goto exit_op_readblk;

        Seek(fu, block, SEEK_SET);
        if ((sq_file->bytes =
                 (int16_t)Read(fu, sq_file->buff, SEQ_BUFFER_SIZE)) <= 0) {
          sq_file->bytes = 0;
          break;
        }
        sq_file->base = block;
      }

      offset = (int16_t)(sq_file->posn & SEQ_BUFFER_MASK);
      n = sq_file->bytes - offset;
      if (n == 0)
        break; /* End of file */
      if (bytes < n)
        n = (int16_t)bytes;
      ts_copy(sq_file->buff + offset, n);
      sq_file->posn += n;
      bytes -= n;
    }
  }

  if (ts_terminate() == 0) {
    if (flags & (SQ_PORT | SQ_FIFO))
      process.status = ER_TIMEOUT;
    else
      process.status = ER_EOF; /* 0224 */
  }

exit_op_readblk:
  k_pop(1);    /* Byte count */
  k_dismiss(); /* File variable */
  k_dismiss(); /* Target */

  /* Set status code on stack */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = process.status;
}

/* ======================================================================
   op_readseq()  -  Read record from sequential file                      */

void op_readseq() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  File variable              | Status 0=ok, >0 = ELSE      |
     |                             |        <0 = ON ERROR        |
     |-----------------------------|-----------------------------| 
     |  ADDR to target             |                             |
     |=============================|=============================|
                    
    ON ERROR codes are only returned if ONERROR opcode executed prior to
    call to READSEQ.
 */

  DESCRIPTOR* fvar_descr;
  DESCRIPTOR* tgt_descr;
  FILE_VAR* fvar;
  SQ_FILE* sq_file;
  OSFILE fu;
  int64 block;
  int16_t offset;
  int16_t bytes;
  /* bool check_lf; variable set but never used */
  char* p;
  u_int16_t op_flags;
  int16_t n;
  char* q;
  int bytes_read;
  bool lf_found;
  bool done = FALSE;
  u_int16_t flags;
  int poll_status;
  int timeout;

  op_flags = process.op_flags;
  process.op_flags = 0;

  process.status = 0;

  /* Find the file variable descriptor */

  fvar_descr = e_stack - 1;
  k_get_file(fvar_descr);
  fvar = fvar_descr->data.fvar;
  if (fvar->type != SEQ_FILE) {
    process.status = ER_NSEQ;
    goto exit_op_readseq;
  }

  sq_file = fvar->access.seq.sq_file;
  flags = sq_file->flags;

  /* Get target descr */

  tgt_descr = e_stack - 2;
  do {
    tgt_descr = tgt_descr->data.d_addr;
  } while (tgt_descr->type == ADDR);

  k_dismiss();
  k_dismiss(); /* Dismiss ADDR */
  k_release(tgt_descr);

  InitDescr(tgt_descr, STRING);
  tgt_descr->data.str.saddr = NULL;
  ts_init(&(tgt_descr->data.str.saddr), 128);

  fu = sq_file->fu;
  if (!ValidFileHandle(fu)) /* File opened for creation */
  {
    process.status = ER_SQRD;
    goto exit_op_readseq;
  }

  if (flags & (SQ_PORT | SQ_FIFO)) /* Port or FIFO */
  {
    lf_found = FALSE;
    do {
      offset = sq_file->posn;
      n = sq_file->bytes - offset; /* Bytes in buffer */
      if (n != 0) {
        p = sq_file->buff + offset;
        if ((q = memchr(p, '\n', n)) != NULL) {
          sq_file->posn += 1; /* Skip line feed */
          n = q - p;
          lf_found = TRUE;
        }
        ts_copy(p, n);
        sq_file->posn += n;
      } else /* Need to read data */
      {
        sq_file->bytes = 0;
        sq_file->posn = 0;

        if (flags & SQ_PORT) /* Port */
        {
          bytes_read = readport(fu, sq_file->buff, SEQ_BUFFER_SIZE);
        } else /* FIFO */
        {
          do {
            poll_status = qmpoll(sq_file->fu, (timeout == 0) ? 0 : 1000);
            if (poll_status)
              break;

            /* Check for events that must be processed in this loop */

            if (my_uptr->events)
              process_events();

            if (((k_exit_cause == K_QUIT) && !tio_handle_break()) ||
                (k_exit_cause == K_TERMINATE)) {
              goto exit_op_readseq;
            }
            if (timeout > 0)
              timeout--;
          } while (timeout);

          if (poll_status < 0)
            goto exit_op_readseq; /* Error */
          if (timeout == 0)
            break;
          bytes_read = read(fu, sq_file->buff, SEQ_BUFFER_SIZE);
        }

        if (bytes_read < 0)
          goto exit_op_readseq; /* Error */
        if (bytes_read == 0)
          break; /* No data */
        sq_file->bytes = bytes_read;
      }
    } while (!lf_found);

    (void)ts_terminate();

    if (!lf_found) {
      process.status = ER_TIMEOUT;

      /* Discard any received data */

      s_free(tgt_descr->data.str.saddr);
      tgt_descr->data.str.saddr = NULL;
    }
  } else /* Normal file or device */
  {
    /* Copy data until we find EOF or a newline */

    /* check_lf = FALSE; variable set but never used */
    do {
      block = sq_file->posn & ~SEQ_BUFFER_MASK;
      if (block != sq_file->base) {
        if (flush_seq(fvar, FALSE))
          goto exit_op_readseq;

        Seek(fu, block, SEEK_SET);
        if ((sq_file->bytes =
                 (int16_t)Read(fu, sq_file->buff, SEQ_BUFFER_SIZE)) < 0) {
          sq_file->bytes = 0;
        }
        sq_file->base = block;
      }

      offset = (int16_t)(sq_file->posn & SEQ_BUFFER_MASK);
      bytes = sq_file->bytes - offset;
      if (bytes == 0) /* End of file */
      {
        if (tgt_descr->data.str.saddr != NULL)
          break; /* 0915 Found some data */

        process.status = ER_RNF;
        goto exit_op_readseq;
      }

      p = sq_file->buff + offset;

      q = memchr(p, '\x0A', bytes);
      if (q != NULL) /* Found a LF.  Copy to but not including LF */
      {
        n = q - p;
        ts_copy(p, n);
        sq_file->posn += n + 1;
        sq_file->line++;
        break;
      } else /* No LF.  Copy all data */
      {
        ts_copy(p, bytes);
        sq_file->posn += bytes;
      }
    } while (!done);
    (void)ts_terminate();
  }

exit_op_readseq:

  /* Set status code on stack */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = process.status;

  if ((process.status < 0) && !(op_flags & P_ON_ERROR)) {
    k_error(sysmsg(1417), -process.status);
  }
}

/* ======================================================================
   op_seek()  -  Position in sequential file                              */

void op_seek() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Relto                      | Status 0=ok, >0 = ELSE      |
     |-----------------------------|-----------------------------| 
     |  Position                   |                             |
     |-----------------------------|-----------------------------| 
     |  File variable              |                             |
     |=============================|=============================|

     Relto = 0 (beginning), 1 (current), 2 (end)
 */

  DESCRIPTOR* descr;
  int32_t relto;
  int64 offset;
  FILE_VAR* fvar;
  SQ_FILE* sq_file;
  OSFILE fu;
  int64 fsize;
  int64 posn;

  /* Get relto flag */

  descr = e_stack - 1;
  GetInt(descr);
  relto = descr->data.value;

  /* Get offset */

  descr = e_stack - 2;
  GetNum(descr);
  if (descr->type == INTEGER)
    offset = descr->data.value;
  else
    offset = (int64)(descr->data.float_value);

  /* Find the file variable descriptor */

  descr = e_stack - 3;
  k_get_file(descr);
  fvar = descr->data.fvar;
  if (fvar->type != SEQ_FILE) {
    process.status = ER_NSEQ;
    goto exit_op_seek;
  }

  sq_file = fvar->access.seq.sq_file;

  fu = sq_file->fu;
  if (!ValidFileHandle(fu)) /* File not created */
  {
    process.status = ER_SQRD;
    goto exit_op_seek;
  }

  if (sq_file->flags & SQ_PORT) {
    process.status = ER_PORT;
    goto exit_op_seek;
  }

  flush_seq(fvar, FALSE);

  fsize = filelength64(fu);
  switch (relto) {
    case 0:
      posn = offset;
      break;

    case 1:
      posn = sq_file->posn + offset;
      break;

    case 2:
      posn = fsize - offset;
      break;

    default:
      process.status = ER_SQREL;
      goto exit_op_seek;
  }

  if ((posn < 0) || (posn > fsize)) {
    process.status = ER_SQSEEK;
    goto exit_op_seek;
  }

  sq_file->posn = posn;

  if (sq_file->flags & SQ_NOBUF) /* Do the seek now */
  {
    Seek(fu, sq_file->posn, SEEK_SET);
  }

  process.status = 0;

exit_op_seek:
  k_pop(2);
  k_dismiss();

  /* Set status code on stack */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = process.status;
}

/* ======================================================================
   op_timeout()  -  Set timeout for READSEQ/READBLK                       */

void op_timeout() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Timeout period, -ve = none |                             |
     |-----------------------------|-----------------------------| 
     |  File variable              |                             |
     |=============================|=============================|

 */

  DESCRIPTOR* descr;
  FILE_VAR* fvar;
  SQ_FILE* sq_file;
  int32_t timeout;

  /* Get timeout period */

  descr = e_stack - 1;
  GetInt(descr);
  timeout = descr->data.value;

  /* Find the file variable descriptor */

  descr = e_stack - 2;
  k_get_file(descr);
  fvar = descr->data.fvar;
  if (fvar->type != SEQ_FILE) {
    process.status = ER_NSEQ;
    goto exit_op_timeout;
  }

  sq_file = fvar->access.seq.sq_file;
  sq_file->timeout = timeout;

  process.status = 0;

exit_op_timeout:
  k_pop(1);
  k_dismiss();
}

/* ======================================================================
   op_weofseq()  -  Write end of file marker to sequential file           */

void op_weofseq() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  File variable              | Status 0=ok                 |
     |                             |        <0 = ON ERROR        |
     |=============================|=============================|
                    
    ON ERROR codes are only returned if ONERROR opcode executed prior to
    call to WEOFSEQ.
 */

  DESCRIPTOR* fvar_descr;
  FILE_VAR* fvar;
  SQ_FILE* sq_file;
  OSFILE fu;
  u_int16_t op_flags;

  op_flags = process.op_flags;
  process.op_flags = 0;

  process.status = 0;

  /* Find the file variable descriptor */

  fvar_descr = e_stack - 1;
  k_get_file(fvar_descr);
  fvar = fvar_descr->data.fvar;
  if (fvar->type != SEQ_FILE)
    k_error(sysmsg(1418));
  if (fvar->flags & FV_RDONLY)
    k_error(sysmsg(1419));
  sq_file = fvar->access.seq.sq_file;

  k_dismiss();
  fu = sq_file->fu;
  if (!ValidFileHandle(fu)) /* File opened for creation */
  {
    if ((fu = create_seq_record(fvar)) < 0) {
      process.status = -ER_SQNC;
      goto exit_op_weofseq;
    }
  }

  if (sq_file->flags & SQ_PORT) {
    process.status = ER_PORT;
    goto exit_op_weofseq;
  }

  flush_seq(fvar, FALSE);

  Seek(fu, sq_file->posn, SEEK_SET);
  chsize64(fu, sq_file->posn);
  sq_file->base = -1;

exit_op_weofseq:
  /* Set status code on stack */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = process.status;

  if ((process.status < 0) && !(op_flags & P_ON_ERROR)) {
    k_error(sysmsg(1420), -process.status);
  }
}

/* ======================================================================
   op_writeblk()  -  Write block to sequential file                       */

void op_writeblk() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  File variable              | Status 0=ok, >0 = ELSE      |
     |-----------------------------|-----------------------------| 
     |  String to write            |                             |
     |=============================|=============================|

 */

  DESCRIPTOR* fvar_descr;
  DESCRIPTOR* src_descr;
  FILE_VAR* fvar;
  SQ_FILE* sq_file;
  OSFILE fu;
  STRING_CHUNK* src_str;

  /* Find the file variable descriptor */

  fvar_descr = e_stack - 1;
  k_get_file(fvar_descr);
  fvar = fvar_descr->data.fvar;

  if (fvar->type != SEQ_FILE) {
    process.status = ER_NSEQ;
    goto exit_op_writeblk;
  }

  if (fvar->flags & FV_RDONLY) {
    process.status = ER_RDONLY;
    log_permissions_error(fvar);
    goto exit_op_writeblk;
  }

  sq_file = fvar->access.seq.sq_file;

  fu = sq_file->fu;
  if (!ValidFileHandle(fu)) /* File opened for creation */
  {
    if ((fu = create_seq_record(fvar)) < 0) {
      process.status = -ER_SQNC;
      goto exit_op_writeblk;
    }
  }

  /* Get source string descr */

  src_descr = e_stack - 2;
  k_get_string(src_descr);
  src_str = src_descr->data.str.saddr;

  if (sq_file->flags & SQ_PORT) {
    /* Write each string chunk to the port */

    while (src_str != NULL) {
      if (!writeport(fu, src_str->data, src_str->bytes))
        goto exit_op_writeblk;
      src_str = src_str->next;
    }
  } else {
    flush_seq(fvar, FALSE);

    /* Position ready for the write */

    Seek(fu, sq_file->posn, SEEK_SET);

    /* Write each string chunk to the file */

    while (src_str != NULL) {
      if (Write(fu, src_str->data, src_str->bytes) < 0) {
        process.status = ER_WRITE_ERROR;
        process.os_error = OSError;
        goto exit_op_writeblk;
      }
      sq_file->posn += src_str->bytes;
      src_str = src_str->next;
    }
  }

  process.status = 0;

exit_op_writeblk:
  sq_file->base = -1; /* Ensure read buffer is dead */

  k_dismiss(); /* File variable */
  k_dismiss(); /* Source string */

  /* Set status code on stack */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = process.status;
}

/* ======================================================================
   op_writeseq()  -  Write record to sequential file
   op_writeseqf() -  Write record to sequential file, flushing to disk    */

void op_writeseq() {
  writeseq(FALSE);
}

void op_writeseqf() {
  writeseq(TRUE);
}

Private void writeseq(bool flush_to_disk) {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  File variable              | Status 0=ok, >0 = ELSE      |
     |                             |        <0 = ON ERROR        |
     |-----------------------------|-----------------------------| 
     |  String to write            |                             |
     |=============================|=============================|
                    
    ON ERROR codes are only returned if ONERROR opcode executed prior to
    call to WRITESEQ.
 */

  DESCRIPTOR* fvar_descr;
  DESCRIPTOR* src_descr;
  FILE_VAR* fvar;
  SQ_FILE* sq_file;
  OSFILE fu;
  STRING_CHUNK* src_str;
  u_int16_t op_flags;

  op_flags = process.op_flags;
  process.op_flags = 0;

  process.status = 0;

  /* Find the file variable descriptor */

  fvar_descr = e_stack - 1;
  k_get_file(fvar_descr);
  fvar = fvar_descr->data.fvar;
  if (fvar->type != SEQ_FILE) {
    process.status = ER_NSEQ;
    goto exit_op_writeseq;
  }

  if (fvar->flags & FV_RDONLY) {
    process.status = ER_RDONLY;
    log_permissions_error(fvar);
    goto exit_op_writeseq;
  }

  k_dismiss();
  sq_file = fvar->access.seq.sq_file;

  fu = sq_file->fu;
  if (!ValidFileHandle(fu)) /* File opened for creation */
  {
    if ((fu = create_seq_record(fvar)) < 0) {
      process.status = -ER_SQNC;
      goto exit_op_writeseq;
    }
  }

  /* Get source string descr */

  src_descr = e_stack - 1;
  k_get_string(src_descr);
  src_str = src_descr->data.str.saddr;

  /* Write each string chunk to the file */

  if (sq_file->flags & SQ_PORT) {
    /* Write each string chunk to the port */

    while (src_str != NULL) {
      if (!writeport(fu, src_str->data, src_str->bytes))
        goto exit_op_writeseq;
      src_str = src_str->next;
    }
    if (!writeport(fu, "\r\n", 1))
      goto exit_op_writeseq;
  } else if (sq_file->flags & SQ_NOBUF) {
    while (src_str != NULL) {
      if (Write(fu, src_str->data, src_str->bytes) < 0) {
        process.status = ER_WRITE_ERROR;
        process.os_error = OSError;
        goto exit_op_writeseq;
      }
      sq_file->posn += src_str->bytes;
      src_str = src_str->next;
    }

    /* Write newline sequence */

    if (Write(fu, Newline, NewlineBytes) < 0) {
      process.status = ER_WRITE_ERROR;
      process.os_error = OSError;
      goto exit_op_writeseq;
    }
    sq_file->posn += NewlineBytes;
  } else {
    while (src_str != NULL) {
      emit(fvar, src_str->data, src_str->bytes);
      src_str = src_str->next;
    }

    /* Write newline sequence */

    emit(fvar, Newline, NewlineBytes);

    if (flush_to_disk)
      flush_seq(fvar, TRUE);
  }

  sq_file->line++;

exit_op_writeseq:
  k_dismiss(); /* Dismiss source string */

  /* Set status code on stack */

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = process.status;

  if ((process.status < 0) && !(op_flags & P_ON_ERROR)) {
    k_error(sysmsg(1421), -process.status);
  }
}

/* ======================================================================
   emit()  -  Write data to sequential file                               */

Private void emit(FILE_VAR* fvar, char* p, int16_t bytes) {
  int16_t n;
  int64 block;
  int16_t offset;
  SQ_FILE* sq_file;

  sq_file = fvar->access.seq.sq_file;

  while (bytes) {
    block = sq_file->posn & ~SEQ_BUFFER_MASK;
    if (block != sq_file->base) {
      flush_seq(fvar, FALSE);

      Seek(sq_file->fu, block, SEEK_SET);
      if ((sq_file->bytes = (int16_t)Read(sq_file->fu, sq_file->buff,
                                            SEQ_BUFFER_SIZE)) < 0) {
        sq_file->bytes = 0;
      }
      sq_file->base = block;
    }

    offset = (int16_t)(sq_file->posn & SEQ_BUFFER_MASK);

    n = min(bytes, SEQ_BUFFER_SIZE - offset);
    memcpy(sq_file->buff + offset, p, n);
    p += n;
    bytes -= n;
    sq_file->posn += n;
    offset += n;
    if (offset > sq_file->bytes)
      sq_file->bytes = offset;
    sq_file->flags |= SQ_DIRTY;
  }
}

/* ======================================================================
   flush_seq()  -  Flush buffer to disk                                   */

Private int flush_seq(FILE_VAR* fvar, bool force_write) {
  int status = 0;
  SQ_FILE* sq_file;

  sq_file = fvar->access.seq.sq_file;

  if (!(sq_file->flags & SQ_PORT)) {
    if (sq_file->flags & SQ_DIRTY) {
      if (Seek(sq_file->fu, sq_file->base, SEEK_SET) < 0) {
        status = -ER_SEEK_ERROR;
        goto exit_flush_seq;
      }

      if (Write(sq_file->fu, sq_file->buff, sq_file->bytes) != sq_file->bytes) {
        status = -ER_WRITE_ERROR;
        process.os_error = OSError;
        goto exit_flush_seq;
      }

      sq_file->flags &= ~SQ_DIRTY;
    }

    if (force_write)
      fsync(sq_file->fu);
  }

exit_flush_seq:
  process.status = status;
  return status;
}

/* ======================================================================
   Create a sequential record on first write                              */

Private OSFILE create_seq_record(FILE_VAR* fvar) {
  OSFILE fu;
  SQ_FILE* sq_file;
  char dir_path[MAX_PATHNAME_LEN + 1];
  char* p;
  int n;
  struct stat stat_buf;

  sq_file = fvar->access.seq.sq_file;

  p = strrchr(sq_file->pathname, DS);
  if (p != NULL) {
    n = p - sq_file->pathname;
    memcpy(dir_path, sq_file->pathname, n);
    dir_path[n] = '\0';

    if (strchr(dir_path, DS) != NULL) /* Not in root directory */
    {
      if (stat(dir_path, &stat_buf) != 0) /* Doesn't exist */
      {
        if (!make_path(dir_path))
          return INVALID_FILE_HANDLE;
      }
    }
  }

  fu = dio_open(sq_file->pathname, DIO_OVERWRITE);
  if (ValidFileHandle(fu))
    sq_file->fu = fu;

  return fu;
}

/* ======================================================================
   close_seq()  -  Close sequential file                                  */

void close_seq(FILE_VAR* fvar) {
  FILE_ENTRY* fptr;
  SQ_FILE* sq_file;

  sq_file = fvar->access.seq.sq_file;
  if (sq_file != NULL) {
    flush_seq(fvar, TRUE);

    if (sq_file->buff != NULL)
      k_free(sq_file->buff);

    if (sq_file->record_name != NULL) {
      (void)unlock_record(fvar, sq_file->record_name, sq_file->record_name_len);
      k_free(sq_file->record_name);
    }

    if (sq_file->pathname != NULL)
      k_free(sq_file->pathname);

    if (fvar->file_id >= 0) {
      StartExclusive(FILE_TABLE_LOCK, 72);
      fptr = FPtr(fvar->file_id);
      (fptr->ref_ct)--;
      (*UFMPtr(my_uptr, fvar->file_id))--;
      EndExclusive(FILE_TABLE_LOCK);
    }

    if (sq_file->flags & SQ_PORT) {
      closeport(sq_file->fu);
    } else {
      CloseFile(sq_file->fu);
    }

    k_free(sq_file);
    fvar->access.seq.sq_file = NULL;
  }
}

/* END-CODE */
