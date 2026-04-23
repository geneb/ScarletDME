/* DH_OPEN.C
 * Open file
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
 * 17Oct25 mab Change dyn file prefix to %
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 * 22Feb20 gwb Replaced a pair of sprintf() with snprintf() in dh_open().
 * 
 * START-HISTORY (OpenQM):
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 16 Mar 07  2.5-1 get_file_entry() should return 0 at error.
 * 21 Nov 06  2.4-17 Revised interface to dio_open() and removed dh_open_path().
 * 24 Jul 06  2.4-10 0505 Opening message file after failed login tried to
 *                   access user file map table.
 * 27 Jun 06  2.4-5 Maintain user/file map table.
 * 17 Mar 06  2.3-8 Added record count to file header.
 * 16 Dec 05  2.3-2 Log messages if file table overflows.
 * 21 Oct 05  2.2-15 Oversized files on the Personal Version are now read-only
 *                   rather than just inaccessable to comply with US law.
 * 10 Oct 05  2.2-14 Implemented akpath element of file header and DH_FILE.
 * 30 Aug 05  2.2-9 Track device and inode in FILE_ENTRY.
 * 17 Jun 05  2.2-1 Added journalling interface.
 * 25 May 05  2.2-0 Ensure file table flags initialised.
 * 13 May 05  2.1-15 Added error arg to s_make_contiguous().
 * 09 May 05  2.1-13 Initial changes for large file support.
 * 15 Dec 04  2.1-0 Limit file size on personal licences.
 * 26 Oct 04  2.0-7 Use dynamic max_id_len.
 * 11 Oct 04  2.0-5 0259 Memory for trigger name was incorrectly sized.
 * 11 Oct 04  2.0-5 Introduced trigger_modes.
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

#include "qm.h"
#include "dh_int.h"
#include "header.h"

/* ====================================================================== */

DH_FILE* dh_open(char path[]) {
  DH_FILE* dh_file = NULL;
  FILE_ENTRY* fptr;
  char filename[MAX_PATHNAME_LEN + 1];
  char pathname[MAX_PATHNAME_LEN + 1];
  int16_t i;
  char* p;
  struct DH_HEADER header;
  struct DH_AK_HEADER ak_header;
  char* ibuff = NULL;
  int16_t file_id = -1;
  OSFILE fu = INVALID_FILE_HANDLE;  /* Primary subfile */
  OSFILE ofu = INVALID_FILE_HANDLE; /* Overflow subfile */
  int bytes;
  u_int32_t ak_map;
  int16_t no_of_subfiles;
  int16_t no_of_aks;
  bool read_only = FALSE;
  DESCRIPTOR* descr;
  int n;
  int subfile;
  int32_t ak_node_num;
  OBJECT_HEADER* obj;
  struct stat statbuf;
  u_int32_t device = 0;
  u_int32_t inode = 0;

  dh_err = 0;
  process.os_error = 0;

  /* Validate file name */

  p = strrchr(path, DS); /* Find final directory separator */
  if (p == NULL)
    p = path;
  if ((*p == '\0')                  /* Null name in last component */
      || !fullpath(filename, path)) { /* Unable to map to absolute pathname */
    dh_err = DHE_FILE_NOT_FOUND;
    goto exit_dh_open;
  }

  /* Check if this file is already open to this process */

  if (stat(filename, &statbuf) != 0) {
    dh_err = DHE_FILE_NOT_FOUND;
    goto exit_dh_open;
  }
  device = statbuf.st_dev;
  inode = statbuf.st_ino;

  for (dh_file = dh_file_head; dh_file != NULL; dh_file = dh_file->next_file) {
    fptr = FPtr(dh_file->file_id);
    if ((device == 0) && (inode == 0)) /* Compare by name */
    {
      if (strcmp((char*)(fptr->pathname), filename) != 0)
        continue;
    } else {
      if ((fptr->device != device) || (fptr->inode != inode))
        continue;
    }

    /* Found a match */

    if (fptr->ref_ct < 0) /* Already open for exclusive access */
    {
      dh_err = DHE_EXCLUSIVE;
      goto exit_dh_open;
    }

    dh_file->open_count++;
    goto exit_dh_open;
  }

  /* Open primary subfile */
  /* replaced sprintf() -gwb 22Feb20 */
// 17Oct25 mab Change dyn file prefix to %
  if (snprintf(pathname, MAX_PATHNAME_LEN + 1, "%s%c%%0", filename, DS) >= (MAX_PATHNAME_LEN + 1)) {
    /* TODO: this should be added to the system log file. */
     k_error("Overflowed directory/filename path length in dh_open()!");
     goto exit_dh_open;
  }
  if (access(pathname, 2))
    read_only = TRUE;

  fu = dio_open(pathname, (read_only) ? DIO_READ : DIO_UPDATE);
  if (!ValidFileHandle(fu)) {
    dh_err = DHE_FILE_NOT_FOUND;
    goto exit_dh_open;
  }

  /* Validate primary subfile header */

  if (Read(fu, (char*)(&header), DH_HEADER_SIZE) < 0) {
    dh_err = DHE_READ_ERROR;
    process.os_error = OSError;
    goto exit_dh_open;
  }

  switch (header.magic) {
    case DH_PRIMARY:
      if (header.hash) {
        dh_err = DHE_HASH_TYPE;
        goto exit_dh_open;
      }
      break;
    default:
      dh_err = DHE_PSFH_FAULT;
      goto exit_dh_open;
  }

  if ((header.flags & DHF_TRUSTED) /* Access requires trusted status */
      && (!(process.program.flags & HDR_IS_TRUSTED))) {
    dh_err = DHE_TRUSTED;
    goto exit_dh_open;
  }

  /* !!FILE_VERSION!! */
  if (header.file_version > DH_VERSION) {
    dh_err = DHE_VERSION;
    goto exit_dh_open;
  }

  /* Check if this file may contain a record id longer than the maximum
     we can support.  The longest_id element of the DH_PARAMS structure
     is set to the length of the longest id written to the file.  This
     record may not be there anymore.  QMFix will sort this out.         */

  if ((header.file_version > 0) &&
      (header.params.longest_id > sysseg->maxidlen)) {
    dh_err = DHE_ID_LEN;
    goto exit_dh_open;
  }

  /* Open overflow subfile */
  /* converted to snprintf() -gwb 22Feb20 */
  // 17Oct25 mab Change dyn file prefix to %
  if (snprintf(pathname, MAX_PATHNAME_LEN + 1, "%s%c%%1", filename, DS) >= (MAX_PATHNAME_LEN + 1)) {
    /* TODO: this should be added to the system log file. */
     k_error("Overflowed directory/filename path length in dh_open()!");
     goto exit_dh_open;
  }
  if (access(pathname, 2))
    read_only = TRUE;

  ofu = dio_open(pathname, (read_only) ? DIO_READ : DIO_UPDATE);
  if (!ValidFileHandle(ofu)) {
    dh_err = DHE_FILE_NOT_FOUND;
    goto exit_dh_open;
  }

  /* Allocate new DH_FILE structure
     AKs use subfile numbers from AK_BASE_SUBFILE upwards, based on position
     in the AK map. Because AKs can be deleted, there may be gaps in the
     numbering. At this stage, simply find the highest used AK number so that
     we know how big the data structures need to be. We will go through the
     map again once we have created the AK data array. */

  no_of_aks = 0;
  for (i = 0, ak_map = header.ak_map; i<MAX_INDICES; i++, ak_map = ak_map>> 1) {
    if (ak_map & 1)
      no_of_aks = i + 1; /* Not really  -  There may be gaps */
  }
  no_of_subfiles = AK_BASE_SUBFILE + no_of_aks;

  bytes = sizeof(struct DH_FILE) +
          (sizeof(struct SUBFILE_INFO) * (no_of_subfiles - 1));
  dh_file = (DH_FILE*)k_alloc(9, bytes);
  if (dh_file == NULL) {
    dh_err = DHE_OPEN_NO_MEMORY;
    goto exit_dh_open;
  }

  for (i = 0; i < no_of_subfiles; i++)
    dh_file->sf[i].fu = INVALID_FILE_HANDLE;

  dh_file->file_version = header.file_version;
  dh_file->header_bytes = DHHeaderSize(header.file_version, header.group_size);
  dh_file->ak_header_bytes = AKHeaderSize(header.file_version);
  dh_file->group_size = header.group_size;

  dh_file->no_of_subfiles = no_of_subfiles;
  dh_file->open_count = 1;
  dh_file->flags = 0;
  dh_file->ak_map = header.ak_map;
  dh_file->trigger_name = NULL;
  dh_file->trigger = NULL;
  dh_file->trigger_modes = 0;

  if (read_only)
    dh_file->flags |= DHF_RDONLY; /* Read only */

  if (header.akpath[0] == '\0')
    dh_file->akpath = NULL;
  else {
    dh_file->akpath = (char*)k_alloc(106, strlen(header.akpath) + 1);
    strcpy(dh_file->akpath, header.akpath);
  }

  /* Transfer file units into DH_FILE structure so that we do not have
     to open them again on the first access.                           */

  dh_set_subfile(dh_file, PRIMARY_SUBFILE, fu);
  fu = INVALID_FILE_HANDLE;

  dh_set_subfile(dh_file, OVERFLOW_SUBFILE, ofu);
  ofu = INVALID_FILE_HANDLE;

  /* Load trigger function, if any */

  if (header.trigger_name[0] != '\0') {
    dh_file->trigger_name =
        (char*)k_alloc(69, strlen(header.trigger_name) + 1); /* 0259 */
    strcpy(dh_file->trigger_name, header.trigger_name);

    /* Attempt to snap link to trigger function */

    obj = (OBJECT_HEADER*)load_object(header.trigger_name, FALSE);
    if (obj != NULL)
      obj->ext_hdr.prog.refs += 1;
    dh_file->trigger = (u_char*)obj;
    dh_file->flags |= DHF_TRIGGER;

    /* Copy the trigger mode flags. If this file pre-dates the introduction
       of these flags, the byte will be zero. Set the defaults to be as they
       were for old versions of the system.                                 */

    if (header.trigger_modes)
      dh_file->trigger_modes = header.trigger_modes;
    else
      dh_file->trigger_modes = TRG_PRE_WRITE | TRG_PRE_DELETE;
  }

  /* Now process any AK indices */

  if (no_of_aks) /* Has AK indices */
  {
    dh_file->flags |= DHF_AK;

    /* Set up AK data matrix */

    dh_file->ak_data = a_alloc(no_of_aks, AKD_COLS, TRUE);

    for (i = 0, ak_map = header.ak_map; i<no_of_aks; i++, ak_map = ak_map>> 1) {
      /* AK field name - Set as null string for now, filled in for used
         entries after we have validated the subfile.                    */

      descr = Element(dh_file->ak_data, (i * AKD_COLS) + AKD_NAME);
      InitDescr(descr, STRING);
      descr->data.str.saddr = NULL;

      if (ak_map & 1) {
        /* Open AK subfile and validate header */

        subfile = i + AK_BASE_SUBFILE;
        if (!dh_open_subfile(dh_file, filename, subfile, FALSE)) {
          dh_err = DHE_AK_NOT_FOUND;
          goto exit_dh_open;
        }

        if (!dh_read_group(dh_file, subfile, 0, (char*)(&ak_header),
                           DH_AK_HEADER_SIZE)) {
          dh_err = DHE_AK_HDR_READ_ERROR;
          process.os_error = OSError;
          goto exit_dh_open;
        }

        if (ak_header.magic != DH_INDEX) {
          dh_err = DHE_AK_HDR_FAULT;
          goto exit_dh_open;
        }

        /* Cross-check the index subfile with the primary data subfile */

        if (header.creation_timestamp != ak_header.data_creation_timestamp) {
          dh_err = DHE_AK_CROSS_CHECK;
          goto exit_dh_open;
        }

        /* Fill in AK name */

        ts_init(&(descr->data.str.saddr), strlen(ak_header.ak_name));
        ts_copy_c_string(ak_header.ak_name);
        (void)ts_terminate();

        /* Field number */

        descr = Element(dh_file->ak_data, (i * AKD_COLS) + AKD_FNO);
        InitDescr(descr, INTEGER);
        descr->data.value = ak_header.fno;

        /* Flags */

        descr = Element(dh_file->ak_data, (i * AKD_COLS) + AKD_FLGS);
        InitDescr(descr, INTEGER);
        descr->data.value = ak_header.flags;

        /* I-type code */

        descr = Element(dh_file->ak_data, (i * AKD_COLS) + AKD_OBJ);
        InitDescr(descr, STRING);
        descr->data.str.saddr = NULL;
        ts_init(&(descr->data.str.saddr), ak_header.itype_len);

        ak_node_num = GetAKFwdLink(dh_file, ak_header.itype_ptr);
        if (ak_node_num != 0) /* Long expression */
        {
          /* Fetch I-type from separate node */

          ibuff = (char*)k_alloc(53, DH_AK_NODE_SIZE);
          do {
            if (!dh_read_group(dh_file, subfile, ak_node_num, ibuff,
                               DH_AK_NODE_SIZE)) {
              goto exit_dh_open;
            }

            n = ((DH_ITYPE_NODE*)ibuff)->used_bytes -
                offsetof(DH_ITYPE_NODE, data);
            ts_copy((char*)(((DH_ITYPE_NODE*)ibuff)->data), n);
            ak_node_num = GetAKFwdLink(dh_file, ((DH_ITYPE_NODE*)ibuff)->next);
          } while (ak_node_num);
        } else {
          ts_copy((char*)ak_header.itype, ak_header.itype_len);
        }
        (void)ts_terminate();

        descr->data.str.saddr = s_make_contiguous(descr->data.str.saddr, NULL);

        /* Collation map name */

        descr = Element(dh_file->ak_data, (i * AKD_COLS) + AKD_MAPNAME);
        InitDescr(descr, STRING);
        k_put_c_string(ak_header.collation_map_name, descr);

        /* Collation map (contiguous string) */

        descr = Element(dh_file->ak_data, (i * AKD_COLS) + AKD_MAP);
        if (ak_header.collation_map_name[0] == '\0') {
          InitDescr(descr, STRING);
          descr->data.str.saddr = NULL;
        } else {
          k_put_string(ak_header.collation_map, 256, descr);
        }
      }
    }
  } else {
    dh_file->ak_data = NULL;
  }

  file_id = get_file_entry(filename, device, inode, &header);
  if (dh_err)
    goto exit_dh_open;

  dh_file->file_id = file_id;

  dh_file->flags |= (u_int32_t)header.flags;

  /* Add DH_FILE structure to chain of open files */

  dh_file->next_file = dh_file_head;
  dh_file_head = dh_file;

exit_dh_open:
  if (ibuff != NULL)
    k_free(ibuff);

  if (dh_err) {
    if (file_id > 0) /* Must decrement reference count in file table */
    {
      StartExclusive(FILE_TABLE_LOCK, 73);
      fptr = FPtr(file_id);
      (fptr->ref_ct)--;
      if (my_uptr != NULL)
        (*UFMPtr(my_uptr, file_id))--;
      EndExclusive(FILE_TABLE_LOCK);
    }

    if (ValidFileHandle(fu))
      dh_close_file(fu);
    if (ValidFileHandle(ofu))
      dh_close_file(ofu);

    if (dh_file != NULL) {
      deallocate_dh_file(dh_file);
      dh_file = NULL;
    }
  }

  return dh_file;
}

/* ======================================================================
  dh_modulus()  -  Return current modulus of DH file                      */

int32_t dh_modulus(DH_FILE* dh_file) {
  FILE_ENTRY* fptr;

  fptr = FPtr(dh_file->file_id);
  return fptr->params.modulus;
}

/* ======================================================================
   Find and reserve a new file table entry                                */

int16_t get_file_entry(char* filename,
                         u_int32_t device,
                         u_int32_t inode,
                         struct DH_HEADER* header) {
  int16_t free_table_entry;
  bool found;
  FILE_ENTRY* fptr;
  int16_t file_id;

  dh_err = 0;

  free_table_entry = 0;
  found = FALSE;
  StartExclusive(FILE_TABLE_LOCK, 10);

  for (file_id = 1; file_id <= sysseg->used_files; file_id++) {
    fptr = FPtr(file_id);
    if (fptr->ref_ct == 0) {
      if (free_table_entry == 0)
        free_table_entry = file_id;
    } else {
      if ((device == 0) && (inode == 0)) /* Compare by name */
      {
        if (strcmp((char*)(fptr->pathname), filename) != 0)
          continue;
      } else {
        if ((fptr->inode != inode) || (fptr->device != device))
          continue;
      }

      /* Found a match */

      if (fptr->ref_ct < 0) /* File already open for exclusive access */
      {
        dh_err = DHE_EXCLUSIVE;
        goto exit_get_file_entry;
      }
      found = TRUE;
      break;
    }
  }

  if (found) {
    fptr->ref_ct++;
    if (my_uptr != NULL)
      (*UFMPtr(my_uptr, file_id))++; /* 0505 */
  } else {
    if (free_table_entry == 0) /* No spare cells */
    {
      if (sysseg->used_files == sysseg->numfiles) {
        log_message("Cannot open file - file table full");
        dh_err = DHE_TOO_MANY_FILES;
        goto exit_get_file_entry;
      }
      free_table_entry = ++(sysseg->used_files);
    }

    file_id = free_table_entry;
    fptr = FPtr(free_table_entry);
    memset((char*)fptr, 0, sizeof(FILE_ENTRY));

    fptr->ref_ct = 1;
    if (my_uptr != NULL)
      (*UFMPtr(my_uptr, file_id))++; /* 0505 */
    fptr->upd_ct = 1;
    fptr->ak_upd = 1;
    fptr->device = device;
    fptr->inode = inode;
    strcpy((char*)(fptr->pathname), filename);

    if (header == NULL) /* For directory files */
    {
//     fptr->params.modulus = 0;
//     fptr->flags = 0;
#ifdef CASE_INSENSITIVE_FILE_SYSTEM
      fptr->flags |= DHF_NOCASE;
#endif
    } else {
      fptr->params.modulus = header->params.modulus;
      fptr->params.min_modulus = header->params.min_modulus;
      fptr->params.big_rec_size = header->params.big_rec_size;
      fptr->params.split_load = header->params.split_load;
      fptr->params.merge_load = header->params.merge_load;
      fptr->params.load_bytes = HeaderLoadBytes(header);
      fptr->params.mod_value = header->params.mod_value;
      fptr->params.longest_id = header->params.longest_id;
      fptr->record_count = header->record_count - 1; /* See dh_fmt.h */
      if (header->file_version < 2) {
        fptr->params.free_chain =
            header->params.free_chain / header->group_size;
      } else {
        fptr->params.free_chain = header->params.free_chain;
      }

      fptr->flags = header->flags;
      fptr->stats = header->stats;
      fptr->stats.opens++;
      sysseg->global_stats.opens++;
    }
  }

exit_get_file_entry:
  EndExclusive(FILE_TABLE_LOCK);

  if (dh_err)
    file_id = 0;

  return file_id;
}

/* END-CODE */
