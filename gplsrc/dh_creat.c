/* DH_CREAT.C
 * Create file
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
 * START-HISTORY (OpenQM):
 * 01 Jul 07  2.5-7 Extensive changes for PDA merge.
 * 21 Nov 06  2.4-17 Revised interface to dio_open().
 * 19 Sep 05  2.2-11 Use dh_buffer to reduce stack space.
 * 09 May 05  2.1-13 Initial changes for large file support.
 * 21 Apr 05  2.1-13 Create intermediate directories in path.
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

/* ====================================================================== */

bool dh_create_file(char path[],
                    int16_t group_size,  /* -ve sets default */
                    int32_t min_modulus,  /* -ve sets default */
                    int32_t big_rec_size, /* -ve sets default */
                    int16_t merge_load,  /* -ve sets default */
                    int16_t split_load,  /* -ve sets default */
                    u_int32_t flags,
                    int16_t version) /* -ve sets default */
{
  bool status = FALSE;
  char primary_subfile[MAX_PATHNAME_LEN + 1];
  char overflow_subfile[MAX_PATHNAME_LEN + 1];
  bool primary_subfile_created = FALSE;
  bool overflow_subfile_created = FALSE;
  OSFILE fu = INVALID_FILE_HANDLE;
  struct DH_HEADER header;
  int header_bytes;
  int group_size_bytes;
  int32_t group;
  double file_size;

  dh_err = 0;
  process.os_error = 0;

  /* Validate arguments */

  if (group_size < 0) {
    group_size = DEFAULT_GROUP_SIZE;
  } else if ((group_size < 1) || (group_size > MAX_GROUP_SIZE)) {
    dh_err = DHE_ILLEGAL_GROUP_SIZE;
    goto exit_dh_create_file;
  }
  group_size_bytes = group_size * DH_GROUP_MULTIPLIER;

  if (min_modulus < 1) {
    min_modulus = DEFAULT_MIN_MODULUS;
  } else if (min_modulus < 1) {
    dh_err = DHE_ILLEGAL_MIN_MODULUS;
    goto exit_dh_create_file;
  }

  if (big_rec_size < 0) {
    big_rec_size = (int32_t)((group_size_bytes)*0.8);
  } else if (big_rec_size > group_size_bytes - BLOCK_HEADER_SIZE) {
    big_rec_size = group_size_bytes - BLOCK_HEADER_SIZE;
  }

  if (merge_load < 0) {
    merge_load = DEFAULT_MERGE_LOAD;
  } else if (merge_load > 99) {
    dh_err = DHE_ILLEGAL_MERGE_LOAD;
    goto exit_dh_create_file;
  }

  if (split_load < 0) {
    split_load = DEFAULT_SPLIT_LOAD;
  } else if (split_load <= merge_load) {
    dh_err = DHE_ILLEGAL_SPLIT_LOAD;
    goto exit_dh_create_file;
  }

  /* !!FILE_VERSION!! */
  if (version < 0) {
    version = DH_VERSION;
  } else if (version > DH_VERSION) {
    dh_err = DHE_VERSION;
    goto exit_dh_create_file;
  }

  header_bytes = DHHeaderSize(version, group_size_bytes);

  if (version < 2) {
    /* Check if file exceeds 2Gb */

    file_size = (((double)group_size_bytes) * min_modulus) + header_bytes;
    if (file_size > (u_int32_t)0x80000000L) {
      dh_err = DHE_SIZE;
      goto exit_dh_create_file;
    }
  }

  /* Check if directory to hold subfiles exists */

  if (access(path, 0) == 0) {
    dh_err = DHE_FILE_EXISTS;
    goto exit_dh_create_file;
  }

  /* Create directory */

  if (!make_path(path)) {
    dh_err = DHE_CREATE_DIR_ERR;
    process.os_error = OSError;
    goto exit_dh_create_file;
  }

  /* Create primary subfile */
  
// 17Oct25 mab Change dyn file prefix to %
  sprintf(primary_subfile, "%s%c%%0", path, DS);
  fu = dio_open(primary_subfile, DIO_NEW);

  if (!ValidFileHandle(fu)) {
    dh_err = DHE_CREATE0_ERR;
    process.os_error = OSError;
    goto exit_dh_create_file;
  }

  primary_subfile_created = TRUE;

  /* Write primary subfile header */

  memset((char*)&header, 0, sizeof(header));
  header.magic = DH_PRIMARY;
  header.file_version = (u_char)version;
  header.group_size = group_size_bytes;
  header.flags = (u_int16_t)(flags & DHF_CREATE);
  header.params.modulus = min_modulus;
  header.params.min_modulus = min_modulus;
  header.params.big_rec_size = big_rec_size;
  header.params.split_load = split_load;
  header.params.merge_load = merge_load;
  header.params.load_bytes = 0;
  header.params.extended_load_bytes = 0;
  header.params.free_chain = 0;
  header.creation_timestamp = qmtime();
  header.record_count = 1; /* Actually zero, see dh_fmt.h */

  /* Calculate mod_value as next power of two >= min_modulus */

  for (header.params.mod_value = 1; header.params.mod_value < min_modulus;
       header.params.mod_value <<= 1) {
  }

  memset(dh_buffer, 0, header_bytes);
  memcpy(dh_buffer, (char*)(&header), sizeof(header));
  if (Write(fu, dh_buffer, header_bytes) < 0) {
    dh_err = DHE_PSFH_WRITE_ERROR;
    process.os_error = OSError;
    goto exit_dh_create_file;
  }

  /* Initialise data groups */

  memset(dh_buffer, 0, group_size_bytes);
  ((DH_BLOCK*)(dh_buffer))->used_bytes = BLOCK_HEADER_SIZE;
  ((DH_BLOCK*)(dh_buffer))->block_type = DHT_DATA;

  for (group = 1; group <= min_modulus; group++) {
    if (Write(fu, dh_buffer, group_size_bytes) < 0) {
      dh_err = DHE_INIT_DATA_ERROR;
      process.os_error = OSError;
      goto exit_dh_create_file;
    }
  }

  CloseFile(fu);
  fu = INVALID_FILE_HANDLE;

  /* Create overflow subfile */

// 17Oct25 mab Change dyn file prefix to %
  sprintf(overflow_subfile, "%s%c%%1", path, DS);
  fu = dio_open(overflow_subfile, DIO_NEW);

  if (!ValidFileHandle(fu)) {
    dh_err = DHE_CREATE1_ERR;
    process.os_error = OSError;
    goto exit_dh_create_file;
  }

  /* Write overflow subfile header */

  memset((char*)(&header), 0, sizeof(header));
  header.magic = DH_OVERFLOW;
  header.group_size = group_size_bytes;

  memset(dh_buffer, 0, header_bytes);
  memcpy(dh_buffer, (char*)(&header), sizeof(header));

  if (Write(fu, dh_buffer, header_bytes) < 0) {
    dh_err = DHE_OSFH_WRITE_ERROR;
    process.os_error = OSError;
    goto exit_dh_create_file;
  }

  CloseFile(fu);
  fu = INVALID_FILE_HANDLE;

  status = TRUE;

exit_dh_create_file:
  if (status == FALSE) {
    if (ValidFileHandle(fu))
      CloseFile(fu);

    if (primary_subfile_created)
      remove(primary_subfile);

    if (overflow_subfile_created)
      remove(overflow_subfile);
  }

  return status;
}

/* END-CODE */
