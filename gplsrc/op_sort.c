/* OP_SORT.C
 * Sort handling opcodes
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
 * 15Jan22 gwb Fixed an waring regarding "comparison of narrow type with wide type in 
 *             loop condition."
 * 
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 *
 * 22Feb20 gwb Converted a number of sprintf() to snprintf().
 *             If the snprintf() in op_sortclr() fails, a sane exit
 *             path needs to be created.
 * 
 * START-HISTORY (OpenQM):
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 21 Nov 06  2.4-17 Revised interface to dio_open().
 * 14 Sep 06  2.4-13 0520 A multi-key sort could ignore all items after a null
 *                   string because the new_value array element was not
 *                   initialised.
 * 21 Aug 06  2.4-11 Use 64 bit file opens via dio_open().
 * 21 Aug 06  2.4-11 Added os_error to errors flushing sort tree.
 * 18 Jul 06  2.4-10 Only save data if BT.DATA flag set in sort_flags[0].
 * 16 Jun 06  2.4-5 Added diagnostics to message 1480.
 * 22 Oct 04  2.0-7 0269 Need to initialise difference variable before
 *                  comparison loop to cope with comparing null with null.
 * 20 Sep 04  2.0-2 Use message handler.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * Opcodes:
 *    op_sortclr   SORTCLEAR   Clear sort data (tidy up after sort complete)
 *    op_sortdata  SORTDATA    Retrieve all sort data as dynamic array
 *    op_sortinit  SORTINIT    Initialise a sort
 *    op_sortnext  SORTNEXT    Retrieve next item from sort data
 *    op_sortadd   SORTADD     Add an entry to a sort tree
 *
 * END-DESCRIPTION
 *
 * Data starts out in a tree similar to the BTREE data item.  When the
 * total size reaches a limit set by the SORTMEM configuration parameter,
 * the data is written to disk into a temporary file as a series of
 * records.
 *
 * If the number of sort files reaches the SORTMRG configuration parameter
 * value, the files are merged to keep the number of files within this
 * range.
 *
 * When the end of the data to be inserted into the sort is reached, if any
 * files have been created, the final burst of data is flushed from memory
 * to disk.
 *
 * When data is retrieved from the sort, if it was all in memory we follow
 * the same procedures as for extracting data from a BTREE data item.  If
 * we have gone to disk, the subfiles are merged as items are read from the
 * list.
 *
 * Only one sort can be in progress for any process at a time.  The subfiles
 * are stored in the directory pointed to by the SORTWORK parameter. Sort
 * files are named ~QMSu.s where u is the process number and s is the subfile
 * number.
 *
 *
 * A disk based sort record is structured as:
 *
 *    int16_t bytes        : Total record length including this count
 *      int16_t bytes  }   : Count of data bytes followed by
 *      char[bytes]      }   : data itself.
 *
 * The data bytes and data area appears first for the record data and then
 * for each sort key in turn.  The length values are aligned on an even
 * byte boundary. The key strings are null terminated.
 *
 * START-CODE
 */

#include "qm.h"
#include "config.h"

#define DISK_BUFFER_SIZE 4096

Private BTREE_ELEMENT* sort_tree = NULL;  /* Head of sort tree */
Private int16_t sort_keys;              /* Number of keys... */
Private u_char sort_flags[MAX_SORT_KEYS]; /* ...and their details */
Private bool sort_has_data;               /* BT_DATA set in flags[0]? */
Private int32_t sort_size;               /* Memory data size (bytes) */
Private BTREE_ELEMENT* sort_ptr;          /* Pointer to next element */
Private bool sorting = FALSE;             /* True while collecting data */
Private int16_t sortwork = 0;           /* Sort work subfile counter */

/* Buffer and associated items for disk based SORTNEXT */
Private char* sort_buff = NULL;     /* Buffer */
Private int16_t sort_buff_bytes;  /* Bytes in buffer */
Private int16_t sort_buff_offset; /* Offset of next byte to extract */
Private OSFILE sort_fu = INVALID_FILE_HANDLE; /* File unit for sort file */

void op_sortclr(void);
Private bool sortmerge(void);
Private bool flush_sort_tree(void);
Private bool merge_sort_files(void);
Private char* read_merge_record(OSFILE fu,
                                char* buff,
                                int16_t* buff_bytes,
                                int16_t* buff_offset,
                                bool* temp);
Private bool write_merge_record(char* rec,
                                OSFILE fu,
                                char* buff,
                                int16_t* buff_bytes,
                                bool temp);

/* ======================================================================
   op_sortclr()  -  Tidy up after completion of a sort                    */

void op_sortclr() {
  char pathname[MAX_PATHNAME_LEN + 1];
  char prefix[12 + 1];
  int16_t prefix_len;
  DIR* dfu;
  struct dirent* dp;

  /* Cast off all sort tree elements */

  if (sort_tree != NULL) {
    free_btree_element(sort_tree, sort_keys);
    sort_tree = NULL;
  }

  /* Free the disk based sort read buffer */

  if (sort_buff != NULL) {
    k_free(sort_buff);
    sort_buff = NULL;
  }

  /* Close the file for a disk based sort */

  if (ValidFileHandle(sort_fu)) {
    CloseFile(sort_fu);
    sort_fu = INVALID_FILE_HANDLE;
  }

  /* Delete any sort work files (only if we have done a disk based sort) */

  if (sortwork) {
    if ((dfu = opendir(pcfg.sortworkdir)) == NULL) {
      k_error(sysmsg(1480), pcfg.sortworkdir, errno);
    }

    prefix_len = sprintf(prefix, "~QMS%d.", (int)process.user_no);

    while ((dp = readdir(dfu)) != NULL) {
      if (memcmp(dp->d_name, prefix, prefix_len) == 0) {
        /* convert to snprintf() -gwb 22Feb20 */
        if (snprintf(pathname, MAX_PATHNAME_LEN + 1, "%s%c%s", pcfg.sortworkdir,
                     DS, dp->d_name) >= (MAX_PATHNAME_LEN + 1)) {
          /* TODO: should write more detailed error to the log. */
          k_error("Overflowed path/filename max length in merge_sort_files()!");
          /* TODO: a sane exit path should be triggered here. */

        } else
          remove(pathname);
      }
    }

    closedir(dfu);

    sortwork = 0;
  }

  process.program.flags &= ~SORT_ACTIVE;
}

/* ======================================================================
   op_sortadd()  -  Add entry to the sort tree                            */

void op_sortadd() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Data (or Unassigned)       |                             |
     |-----------------------------|-----------------------------| 
     |  ADDR to keys array         |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;
  ARRAY_HEADER* a_hdr;
  BTREE_ELEMENT* bte;
  BTREE_ELEMENT* new_bte;
  int16_t index;
  int16_t d;
  char* key;
  char* key_str;
  char* bte_str;
  bool right_justified;
  bool descending;
  int16_t n;
  int bytes;
  bool numeric_compare[MAX_SORT_KEYS];
  double new_value[MAX_SORT_KEYS]; /* Value being inserted */
  double key_value;                /* Value being compared from tree */
  int32_t size = 4;               /* Equivalent disk record size */
  STRING_CHUNK* str;
  char* p;

  /* Find the keys array */

  descr = e_stack - 2;
  while (descr->type == ADDR)
    descr = descr->data.d_addr;
  if (descr->type != ARRAY)
    k_not_array_error(descr);

  a_hdr = descr->data.ahdr_addr;
  if ((a_hdr->rows != sort_keys) || (a_hdr->cols != 0)) {
    k_error(sysmsg(1481));
  }

  /* Make a new BTree element */

  bytes = sizeof(struct BTREE_ELEMENT) + ((sort_keys - 1) * sizeof(char*));
  new_bte = (BTREE_ELEMENT*)k_alloc(61, bytes);
  if (new_bte == NULL)
    k_error(sysmsg(1482));
  memset(new_bte, 0, bytes);

  for (index = 0; index < sort_keys; index++) {
    descr = Element(a_hdr, index + 1);
    GetString(descr);
    str = descr->data.str.saddr;
    if (str != NULL) {
      bytes = str->string_len + 1;
      p = (char*)k_alloc(115, bytes);
      k_get_c_string(descr, p, str->string_len);
      new_bte->key[index] = p;
      size += (bytes + 3) & ~1;
      if (sort_flags[index] & BT_RIGHT_ALIGNED) {
        numeric_compare[index] = strdbl(p, &new_value[index]);
      }
    } else /* Null key value */
    {
      numeric_compare[index] = TRUE;
      new_value[index] = 0.0; /* 0520 */
      size += 2;              /* Allow for length field in disk image */
    }
  }

  /* Find the data */

  if (sort_has_data) {
    descr = e_stack - 1;
    GetString(descr);
    str = descr->data.str.saddr;
    if (str != NULL) {
      bytes = str->string_len + 1;
      p = (char*)k_alloc(116, bytes);
      k_get_c_string(descr, p, str->string_len);
      new_bte->data = p;
      size += (bytes + 1) & ~1;
    }
  }

  /* Flush the tree if this would take us past the SORTMEM limit */

  sort_size += size;
  if (sort_size > pcfg.sortmem) {
    if (!flush_sort_tree())
      k_error(sysmsg(1485), process.os_error);
    sort_size = size;
  }

  /* Walk tree to insertion point */

  bte = sort_tree;
  if (bte == NULL) /* Inserting first element */
  {
    sort_tree = new_bte;
    new_bte->parent = NULL;
  } else {
    index = 0;

    do {
      key = new_bte->key[index];
      if (key == NULL)
        key = null_string;

      n = sort_flags[index];
      right_justified = n & BT_RIGHT_ALIGNED;
      descending = (n & BT_DESCENDING) != 0;

      /* Compare strings */

      key_str = key;
      bte_str = bte->key[index];
      if (bte_str == NULL)
        bte_str = null_string;

      d = 0; /* 0269 */

      if (right_justified) {
        if (numeric_compare[index]) {
          if (strdbl(bte_str, &key_value)) {
            if (new_value[index] > key_value)
              d = 1;
            else if (new_value[index] == key_value)
              d = 0;
            else
              d = -1;
            goto mismatch;
          }
        }

        n = strlen(key_str) - strlen(bte_str);
        if (n) {
          /* Compare leading characters of longer string against spaces */

          if (n > 0) /* New key longer than BTree element key */
          {
            while (n--) {
              if ((d = (((int16_t)*((u_char*)key_str++)) - ' ')) != 0)
                goto mismatch;
            }
          } else /* New key shorter than BTree element key */
          {
            while (n++) {
              if ((d = (' ' - ((int16_t)*((u_char*)bte_str++)))) != 0)
                goto mismatch;
            }
          }
        }
      }

      /* Compare to end of shorter string */

      while (*bte_str && *key_str) {
        if ((d = (((int16_t)(*((u_char*)key_str++))) -
                  *((u_char*)bte_str++))) != 0)
          goto mismatch;
      }

      /* If either string still has unprocessed characters, it is the greater
          string (d must be zero when we arrive here).                        */

      if (*key_str)
        d = 1;
      else if (*bte_str)
        d = -1;

    mismatch:
      if (descending)
        d = -d;

      if (d < 0) {
        if (bte->left == NULL) /* Add as new left entry */
        {
          bte->left = new_bte;
          new_bte->parent = bte;
          goto exit_op_sortadd;
        }
        bte = bte->left;
        index = 0;
      } else if (d > 0) {
        if (bte->right == NULL) /* Add as new right entry */
        {
          bte->right = new_bte;
          new_bte->parent = bte;
          goto exit_op_sortadd;
        }
        bte = bte->right;
        index = 0;
      } else /* Matching key value */
      {
        if (sort_flags[index] & BT_UNIQUE) {
          free_btree_element(new_bte, sort_keys);
          sort_size -= size;
          process.status = 1;
          goto exit_op_sortadd;
        }

        /* Move down to lower level index if there is one */

        if (++index == sort_keys) {
          /* Complete duplicate at all key levels.  Insert to left. */
          if (bte->left == NULL) /* Add as new left entry */
          {
            bte->left = new_bte;
            new_bte->parent = bte;
            goto exit_op_sortadd;
          }
          bte = bte->left;
          index = 0;
        }
      }
    } while (1);
  }

exit_op_sortadd:
  k_dismiss(); /* Data */
  k_pop(1);    /* ADDR to keys array */
}

/* ======================================================================
   op_sortinit()  -  Initialise a sort                                    */

void op_sortinit() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  ADDR to flags array        |                             |
     |-----------------------------|-----------------------------| 
     |  Number of keys             |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;
  int keys;
  ARRAY_HEADER* a_hdr;
  int i;  /* was int16_t, which makes it dumb to compare against an int. */

  op_sortclr(); /* Discard any existing sort tree and files */

  descr = e_stack - 2;
  GetInt(descr);
  keys = descr->data.value;
  if ((keys < 1) || (keys > MAX_SORT_KEYS))
    k_error(sysmsg(1486));

  descr = e_stack - 1;
  while (descr->type == ADDR)
    descr = descr->data.d_addr;
  if (descr->type != ARRAY)
    k_not_array_error(descr);
  a_hdr = descr->data.ahdr_addr;
  if ((keys > a_hdr->rows) || (a_hdr->cols != 0))
    k_index_error(descr);

  sort_keys = keys;
  for (i = 1; i <= keys; i++) {
    descr = Element(a_hdr, i);
    GetInt(descr); /* Side effect - may convert array element type */
    sort_flags[i - 1] = (u_char)(descr->data.value);
  }

  sort_has_data = (sort_flags[0] & BT_DATA) != 0;

  k_pop(2);

  sort_size = 0;
  sorting = TRUE;
  process.program.flags |= SORT_ACTIVE;
}

/* ======================================================================
   op_sortdata()  -  Retrieve all data from sort                          */

void op_sortdata() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |                             | Data from item              |
     |=============================|=============================|

    Returns STATUS() as the number of items found
 */

  BTREE_ELEMENT* bte;
  BTREE_ELEMENT* old_bte;
  STRING_CHUNK* head = NULL;
  int16_t skip_bytes;
  int16_t data_bytes;
  int16_t bytes_remaining;
  int16_t n;
  int16_t phase;

  process.status = 0;

  if (!sorting)
    k_error(sysmsg(1487));
  sorting = FALSE;

  ts_init(&head, 1024);

  if (sortwork) /* Disk based sort */
  {
    if (!sortmerge())
      goto exit_op_sortdata;

    phase = 1;
    do {
      while (sort_buff_offset >= sort_buff_bytes) /* Read a new buffer */
      {
        sort_buff_offset -= sort_buff_bytes;
        sort_buff_bytes = (int16_t)Read(sort_fu, sort_buff, DISK_BUFFER_SIZE);
        if (sort_buff_bytes <= 0)
          goto exit_op_sortdata; /* End of data */
      }

      switch (phase) {
        case 1: /* Extracting record length to form skip_bytes */
          skip_bytes = *((int16_t*)(sort_buff + sort_buff_offset)) - 2;
          sort_buff_offset += 2;
          if (head != NULL)
            ts_copy_byte(FIELD_MARK);
          if (!sort_has_data) {
            /* This tree has no data element so skip it. The code below
                 will then return the first key value as the "data".      */
            sort_buff_offset += 2;
            skip_bytes -= 2;
          }
          phase = 2;
          break;

        case 2: /* Extracting data length */
          data_bytes = *((int16_t*)(sort_buff + sort_buff_offset));
          /* If we are extracting the first key as the data, decrement
               the length to exclude the null terminator.                */
          if (!sort_has_data)
            data_bytes--;
          sort_buff_offset += 2;
          skip_bytes -= 2;
          phase = 3;
          break;

        case 3: /* Extracting data */
          bytes_remaining = sort_buff_bytes - sort_buff_offset;
          n = min(bytes_remaining, data_bytes);
          ts_copy(sort_buff + sort_buff_offset, n);
          data_bytes -= n;
          n = (n + 1) & ~1;
          sort_buff_offset += n;
          skip_bytes -= n;
          if (data_bytes == 0)
            phase = 4;
          break;

        case 4: /* Skipping keys */
          process.status++;
          sort_buff_offset += skip_bytes;
          phase = 1;
          break;
      }
    } while (1);
  } else /* Memory based sort */
  {
    bte = sort_tree;
    if (bte != NULL)
      while (bte->left != NULL)
        bte = bte->left;

    while (bte != NULL) {
      if (head != NULL)
        ts_copy_byte(FIELD_MARK);
      if (sort_has_data)
        ts_copy(bte->data, strlen(bte->data));
      else
        ts_copy(bte->key[0], strlen(bte->key[0]));
      process.status += 1;

      /* Locate next item */

      if (bte->right != NULL) /* Descend one step right then all the way left */
      {
        bte = bte->right;
        while (bte->left != NULL)
          bte = bte->left;
      } else /* Ascend to parent, repeating so long as we are */
      {      /* coming up a right limb */
        do {
          old_bte = bte;
          bte = bte->parent;
          if (bte == NULL)
            break;
        } while (old_bte == bte->right);
      }
    }
  }

exit_op_sortdata:
  ts_terminate();

  InitDescr(e_stack, STRING);
  (e_stack++)->data.str.saddr = head;
}

/* ======================================================================
   op_sortnext()  -  Retrieve next item from sort data                    */

void op_sortnext() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  ADDR to keys array         | Data from item              |
     |=============================|=============================|

    Keys array is set to all keys of item found.

    Returns STATUS() non-zero if no item found
 */

  DESCRIPTOR* data_descr;
  DESCRIPTOR* descr;
  ARRAY_HEADER* a_hdr;
  BTREE_ELEMENT* bte;
  BTREE_ELEMENT* old_bte;
  int16_t index;
  char* buff = NULL;
  int16_t bytes;
  int16_t n;
  char* q;

  process.status = 1;

  /* Find the keys array */

  descr = e_stack - 1;
  while (descr->type == ADDR)
    descr = descr->data.d_addr;
  if (descr->type != ARRAY)
    k_not_array_error(descr);

  a_hdr = descr->data.ahdr_addr;
  if ((a_hdr->rows != sort_keys) || (a_hdr->cols != 0)) {
    k_error(sysmsg(1488));
  }
  k_pop(1);

  /* Set up a null result for error paths (including normal exit when we
    reach the end ofthe sort data.                                       */

  data_descr = e_stack++;
  InitDescr(data_descr, STRING);
  data_descr->data.str.saddr = NULL;

  if (sorting) /* First call - Set pointers */
  {
    if (sortwork) /* Disk based sort */
    {
      if (!sortmerge())
        goto exit_op_sortnext;
    } else /* Memory based sort */
    {
      bte = sort_tree;
      if (bte != NULL)
        while (bte->left != NULL)
          bte = bte->left;
      sort_ptr = bte;
    }

    sorting = FALSE;
  }

  if (sortwork) /* Disk based sort */
  {
    if (sort_buff_offset >= sort_buff_bytes) /* Read a new buffer */
    {
      sort_buff_bytes = (int16_t)Read(sort_fu, sort_buff, DISK_BUFFER_SIZE);
      if (sort_buff_bytes <= 0)
        goto exit_op_sortnext; /* End of data */
      sort_buff_offset = 0;
    }

    /* Fetch the record length */

    bytes = *((int16_t*)(sort_buff + sort_buff_offset));

    /* If the entire record lies in the current buffer, work directly from
      the buffer.  If it extends beyond the current buffer, allocate a
      temporary buffer to hold the record.                                */

    if (sort_buff_offset + bytes <= sort_buff_bytes) /* It's all in memory */
    {
      q = sort_buff + sort_buff_offset + 2; /* Point to data byte count */
      sort_buff_offset += (bytes + 1) & ~1;
    } else /* Must use a temporary buffer */
    {
      buff = (char*)k_alloc(65, bytes);
      q = buff;

      while (bytes) {
        n = sort_buff_bytes - sort_buff_offset;
        if (n == 0) {
          sort_buff_bytes =
              (int16_t)Read(sort_fu, sort_buff, DISK_BUFFER_SIZE);
          if (sort_buff_bytes <= 0)
            goto exit_op_sortnext;
          sort_buff_offset = 0;
          n = sort_buff_bytes;
        }
        n = min(bytes, n);
        memcpy(q, sort_buff + sort_buff_offset, n);
        q += n;
        bytes -= n;
        sort_buff_offset += n;
      }

      q = buff + 2; /* Point to data byte count */
    }

    /* Extract the data */

    bytes = *((int16_t*)q);
    q += 2;
    if (bytes) {
      ts_init(&(data_descr->data.str.saddr), bytes);
      ts_copy(q, bytes);
      ts_terminate();
      q += (bytes + 1) & ~1;
    }

    /* Extract the keys */

    for (index = 1; index <= sort_keys; index++) {
      descr = Element(a_hdr, index);
      k_release(descr);
      bytes = *((int16_t*)q);
      q += 2;
      InitDescr(descr, STRING);
      descr->data.str.saddr = NULL;
      if (bytes) {
        /* The keys are stored in the sort files with a trailing \0 character.
          This should not be included in the copied key data.                */

        ts_init(&(descr->data.str.saddr), bytes - 1);
        ts_copy(q, bytes - 1);
        ts_terminate();
        q += (bytes + 1) & ~1;
      }
    }
  } else /* Memory based sort */
  {
    /* Return data from current item */

    if ((bte = sort_ptr) == NULL)
      goto exit_op_sortnext; /* No more data */

    k_put_c_string(bte->data, data_descr);

    /* Copy keys to array */

    for (index = 1; index <= sort_keys; index++) {
      descr = Element(a_hdr, index);
      k_release(descr);
      k_put_c_string(bte->key[index - 1], descr);
    }

    /* Locate next item */

    if (bte->right != NULL) /* Descend one step right then all the way left */
    {
      bte = bte->right;
      while (bte->left != NULL)
        bte = bte->left;
    } else /* Ascend to parent, repeating so long as we are */
    {      /* coming up a right limb */
      do {
        old_bte = bte;
        bte = bte->parent;
        if (bte == NULL)
          break;
      } while (old_bte == bte->right);
    }

    sort_ptr = bte;
  }

  process.status = 0;

exit_op_sortnext:
  if (buff != NULL)
    k_free(buff);

  return;
}

/* ======================================================================
   sortmerge()  -  Perform merge phase of disk sort                       */

Private bool sortmerge() {
  bool status = FALSE;
  char pathname[MAX_PATHNAME_LEN + 1];

  /* Flush final part to disk */

  if (!flush_sort_tree())
    k_error(sysmsg(1489), process.os_error);

  /* Initialise scan pointer to start of sort data */

  if (!merge_sort_files())
    k_error(sysmsg(1490)); /* 1.5-9 */
  /* convert to snprintf() -gwb 22Feb20 */
  if (snprintf(pathname, MAX_PATHNAME_LEN + 1, "%s%c~QMS%d.%d",
               pcfg.sortworkdir, DS, (int)process.user_no,
               (int)sortwork) >= (MAX_PATHNAME_LEN + 1)) {
    /* TODO: should write more detailed error to the log. */
    k_error("Overflowed path/filename max length in sortmerge()!");
    goto exit_sortmerge;
  }
  sort_fu = dio_open(pathname, DIO_READ);
  if (!ValidFileHandle(sort_fu))
    goto exit_sortmerge;

  sort_buff = (char*)k_alloc(64, DISK_BUFFER_SIZE);
  sort_buff_bytes = 0;
  sort_buff_offset = 0;

  status = TRUE;

exit_sortmerge:
  return status;
}

/* ======================================================================
   flush_sort_tree()  - Write sort tree to disk                           */

/* process.os_error will be set for error returns */

Private bool flush_sort_tree() {
  bool status = FALSE;
  char pathname[MAX_PATHNAME_LEN + 1];
  OSFILE fu = INVALID_FILE_HANDLE;
  BTREE_ELEMENT* bte;
  BTREE_ELEMENT* old_bte;
  int bytes;
  int assembly_buffer_size = 0;
  char* assembly_buffer = NULL;
  char* disk_buffer = NULL; /* Disk output buffer and... */
  int16_t used_bytes;     /* ...used byte count */
  int16_t space;
  int16_t i;
  int16_t n;
  char* p;
  char* q;

  /* Flush the memory tree to disk */

  process.os_error = 0;
  /* convert to snprintf() -gwb 22Feb20 */
  if (snprintf(pathname, MAX_PATHNAME_LEN + 1, "%s%c~QMS%d.%d",
               pcfg.sortworkdir, DS, (int)process.user_no,
               (int)sortwork) >= (MAX_PATHNAME_LEN + 1)) {
    /* TODO: should write more detailed error to the log. */
    k_error("Overflowed path/filename max length in flush_sort_tree()!");
    goto exit_flush_sort_tree;
  }
  fu = dio_open(pathname, DIO_REPLACE);
  if (!ValidFileHandle(fu))
    k_error(sysmsg(1491), pathname, OSError);

  disk_buffer = (char*)k_alloc(63, DISK_BUFFER_SIZE);
  used_bytes = 0;

  bte = sort_tree;
  if (bte != NULL)
    while (bte->left != NULL)
      bte = bte->left;
  while (bte != NULL) {
    /* Count bytes required for this entry */

    bytes = 4; /* Record length + data length */
    if (bte->data != NULL)
      bytes += (strlen(bte->data) + 1) & ~1;

    for (i = 0; i < sort_keys; i++) /* Keys - Written with trailing \0 */
    {
      bytes += 2; /* Key length */
      if (bte->key[i] != NULL)
        bytes += (strlen(bte->key[i]) + 2) & ~1;
    }

    if (assembly_buffer_size <
        bytes) { /* Need to increase assembly buffer size */
      if (assembly_buffer != NULL)
        k_free(assembly_buffer);
      assembly_buffer_size = (bytes + 1023) & ~1023;
      assembly_buffer = (char*)k_alloc(62, assembly_buffer_size);
    }

    /* Assemble record */

    q = assembly_buffer;
    *((int16_t*)q) = bytes; /* Record header: total byte count */
    q += 2;

    if (bte->data != NULL) /* Data item with preceding byte count */
    {
      n = strlen(bte->data);
      *((int16_t*)q) = n;
      q += 2;
      memcpy(q, bte->data, n);
      q += n;
      if (n & 1)
        *(q++) = '\0';
    } else {
      *((int16_t*)q) = 0;
      q += 2;
    }

    for (i = 0; i < sort_keys; i++) /* Each key item */
    {
      if (bte->key[i] != NULL) {
        n = strlen(bte->key[i]) + 1;
        *((int16_t*)q) = n;
        q += 2;
        memcpy(q, bte->key[i], n);
        q += n;
        if (n & 1)
          *(q++) = '\0';
      } else {
        *((int16_t*)q) = 0;
        q += 2;
      }
    }

    /* Copy to disk buffer */

    p = assembly_buffer;
    do {
      space = DISK_BUFFER_SIZE - used_bytes;
      if (space == 0) {
        if (Write(fu, disk_buffer, used_bytes) != used_bytes) {
          process.os_error = OSError;
          goto exit_flush_sort_tree;
        }

        used_bytes = 0;
        space = DISK_BUFFER_SIZE;
      }

      n = min(bytes, space);
      memcpy(disk_buffer + used_bytes, p, n);
      p += n;
      used_bytes += n;
      bytes -= n;
    } while (bytes);

    /* Locate next item */

    if (bte->right != NULL) /* Descend one step right then all the way left */
    {
      bte = bte->right;
      while (bte->left != NULL)
        bte = bte->left;
    } else /* Ascend to parent, repeating so long as we are
                              coming up a right limb */
    {
      do {
        old_bte = bte;
        bte = bte->parent;
        if (bte == NULL)
          break;
      } while (old_bte == bte->right);
    }
  }

  /* Flush final buffer */

  if (used_bytes != 0) {
    if (Write(fu, disk_buffer, used_bytes) != used_bytes) {
      process.os_error = OSError;
      goto exit_flush_sort_tree;
    }
  }

  sortwork++;

  /* Cast off all tree elements */

  if (sort_tree != NULL)
    free_btree_element(sort_tree, sort_keys);
  sort_tree = NULL;
  sort_size = 0;

  status = TRUE;

exit_flush_sort_tree:
  if (ValidFileHandle(fu))
    CloseFile(fu);

  if (disk_buffer != NULL)
    k_free(disk_buffer);
  if (assembly_buffer != NULL)
    k_free(assembly_buffer);

  return status;
}

/* ======================================================================
   merge_sort_files()  -  Merge individual disk based sort files          */

Private bool merge_sort_files() {
  bool status = FALSE;
  int16_t num_files; /* Number of files to process */
  int16_t lo_file;   /* Lowest existing file index */
  int16_t nstream;

  OSFILE infu[MAX_SORTMRG];           /* File unit for first file... */
  char* buff[MAX_SORTMRG];            /* ...disk buffer... */
  int16_t buff_bytes[MAX_SORTMRG];  /* ...byte count... */
  int16_t buff_offset[MAX_SORTMRG]; /* ...and current offset */
  char* rec[MAX_SORTMRG];             /* Record pointer and... */
  bool temp[MAX_SORTMRG];             /* Temporary buffer flag */
  char* p1;                           /* Rolling pointer to extracted keys */
  int16_t n1;                       /* Bytes in element to be extracted */
  char* k1;                           /* Key data pointer */
  double v1;                          /* Extracted numeric data */
  char* p2;                           /* Rolling pointer to extracted keys */
  int16_t n2;                       /* Bytes in element to be extracted */
  char* k2;                           /* Key data pointer */
  double v2;                          /* Extracted numeric data */
  int16_t best;
  int16_t ndata; /* Number of streams not exhausted */

  OSFILE outfu = INVALID_FILE_HANDLE; /* File unit for target file... */
  char* outbuff = NULL;               /* ...disk buffer... */
  int16_t outbuff_bytes;            /* ...byte count */

  char pathname[MAX_PATHNAME_LEN + 1];
  int16_t index;
  int16_t i;
  int16_t n;
  int16_t d;

  /* At this time, sortwork contains the number of sort work files
    (i.e. the highest sort work file index plus 1 because they start
    at zero.
    We merge sets of files repeatedly until we have just one file
    left.  The sortwork variable is left addressing this subfile.     */

  num_files = sortwork--;
  lo_file = 0;

  for (i = 0; i < pcfg.sortmrg; i++) {
    infu[i] = INVALID_FILE_HANDLE;
    buff[i] = (char*)k_alloc(66, DISK_BUFFER_SIZE);
  }

  while (num_files > 1) {
    nstream = min(pcfg.sortmrg, num_files);
    /* Open files */

    for (i = 0; i < nstream; i++) {
      /* convert to snprintf() -gwb 22Feb20 */
      if (snprintf(pathname, MAX_PATHNAME_LEN + 1, "%s%c~QMS%d.%d",
                   pcfg.sortworkdir, DS, (int)process.user_no,
                   (int)lo_file + i) >= (MAX_PATHNAME_LEN + 1)) {
        /* TODO: should write more detailed error to the log. */
        k_error("Overflowed path/filename max length in merge_sort_files()!");
        goto exit_merge_sort_files;
      }
      infu[i] = dio_open(pathname, DIO_READ);
      if (!ValidFileHandle(infu[i]))
        goto exit_merge_sort_files;
      buff_bytes[i] = 0;
      buff_offset[i] = 0;
      rec[i] = read_merge_record(infu[i], buff[i], &(buff_bytes[i]),
                                 &(buff_offset[i]), &(temp[i]));
    }

    sortwork++;
    /* convert to snprintf() -gwb 22Feb20 */
    if (snprintf(pathname, MAX_PATHNAME_LEN + 1, "%s%c~QMS%d.%d",
                 pcfg.sortworkdir, DS, (int)process.user_no,
                 (int)sortwork) >= (MAX_PATHNAME_LEN + 1)) {
      /* TODO: should write more detailed error to the log. */
      k_error("Overflowed path/filename max length in merge_sort_files()!");
      goto exit_merge_sort_files;
    }
    outfu = dio_open(pathname, DIO_REPLACE);
    if (!ValidFileHandle(outfu))
      goto exit_merge_sort_files;

    outbuff = (char*)k_alloc(66, DISK_BUFFER_SIZE);
    outbuff_bytes = 0;

    ndata = nstream;
    do {
      /* Compare records. We compare each of the parallel streams in turn,
          keeping track of the "best" one to use.                            */

      for (best = 0; rec[best] == NULL; best++) {
      } /* Find first stream */

      for (i = best + 1; i < nstream; i++) {
        if (rec[i] == NULL)
          continue; /* This stream is exhausted */

        p1 = rec[best] + 2;                 /* Skip overall byte count */
        p1 += (*((int16_t*)p1) + 3) & ~1; /* Skip data segment */

        p2 = rec[i] + 2;                    /* Skip overall byte count */
        p2 += (*((int16_t*)p2) + 3) & ~1; /* Skip data segment */

        for (index = 0; index < sort_keys; index++) {
          n1 = *((int16_t*)p1);
          k1 = (n1 == 0) ? "" : (p1 + 2);
          p1 += (n1 + 3) & ~1;

          n2 = *((int16_t*)p2);
          k2 = (n2 == 0) ? "" : (p2 + 2);
          p2 += (n2 + 3) & ~1;

          d = 0; /* 0269 */

          if (sort_flags[index] & BT_RIGHT_ALIGNED) {
            if (strdbl(k1, &v1) && strdbl(k2, &v2)) /* Both are numbers */
            {
              if (v1 > v2)
                d = 1;
              else if (v1 == v2)
                d = 0;
              else
                d = -1;
              goto mismatch;
            }

            n = n1 - n2;
            if (n) {
              /* Compare leading characters of longer string against spaces */

              if (n > 0) /* Key 1 longer than key 2 */
              {
                while (n--) {
                  if ((d = (((int16_t)*((u_char*)k1++)) - ' ')) != 0)
                    goto mismatch;
                }
              } else /* Key 1 shorter than key 2 */
              {
                while (n++) {
                  if ((d = (' ' - ((int16_t)*((u_char*)k2++)))) != 0)
                    goto mismatch;
                }
              }
            }
          }

          /* Compare to end of shorter string */

          while (*k1 && *k2) {
            if ((d = (((int16_t)(*((u_char*)k1++))) - *((u_char*)k2++))) != 0)
              goto mismatch;
          }

          /* If either string still has unprocessed characters, it is the greater
              string (d must be zero when we arrive here).                        */

          if (*k1)
            d = 1;
          else if (*k2)
            d = -1;

        mismatch:
          if (sort_flags[index] & BT_DESCENDING)
            d = -d;

          if (d)
            break;
        }

        /* We have either found a difference or all the keys are equal */

        if (d >= 0)
          best = i;
      }

      /* The best item has now been identified. Copy it to the output stream */
      if (!write_merge_record(rec[best], outfu, outbuff, &outbuff_bytes,
                              temp[best])) {
        goto exit_merge_sort_files;
      }
      /* Fetch the next record from this stream */

      rec[best] = read_merge_record(infu[best], buff[best], &(buff_bytes[best]),
                                    &(buff_offset[best]), &(temp[best]));
      if (rec[best] == NULL)
        ndata--;
    } while (ndata > 1);

    /* There is only one file left active. Copy the data from it to the
      output file.                                                     */

    for (i = 0; i < nstream; i++) {
      if (rec[i] != NULL) {
        do {
          if (!write_merge_record(rec[i], outfu, outbuff, &outbuff_bytes,
                                  temp[i])) {
            goto exit_merge_sort_files;
          }
          rec[i] = read_merge_record(infu[i], buff[i], &(buff_bytes[i]),
                                     &(buff_offset[i]), &(temp[i]));
        } while (rec[i] != NULL);
        break;
      }
    }

    /* Flush final buffer */

    if (outbuff_bytes) {
      if (Write(outfu, outbuff, outbuff_bytes) != outbuff_bytes) /* 0241 */
      {
        goto exit_merge_sort_files; /* 0239 */
      }
    }

    CloseFile(outfu);
    outfu = INVALID_FILE_HANDLE;

    /* Close and delete input files */

    for (i = 0; i < nstream; i++) {
      CloseFile(infu[i]);
      infu[i] = INVALID_FILE_HANDLE;
      /* convert snprintf() -gwb 22Feb20 */
      if (snprintf(pathname, MAX_PATHNAME_LEN + 1, "%s%c~QMS%d.%d",
                   pcfg.sortworkdir, DS, (int)process.user_no,
                   (int)lo_file + i) >= (MAX_PATHNAME_LEN + 1)) {
        /* TODO: should write more detailed error to the log. */
        k_error("Overflowed path/filename max length in merge_sort_files()!");
        goto exit_merge_sort_files;
      }
      remove(pathname);
    }

    lo_file += nstream;
    num_files -= nstream - 1;
  }

  status = TRUE;

exit_merge_sort_files:
  for (i = 0; i < pcfg.sortmrg; i++) {
    if (ValidFileHandle(infu[i]))
      CloseFile(infu[i]);
    if (buff[i] != NULL)
      k_free(buff[i]);
  }

  return status;
}

/* ======================================================================
   read_merge_record()  -  Read a record for a sort file merge            */

Private char* read_merge_record(
    OSFILE fu,              /* File unit */
    char* buff,             /* Buffer base address */
    int16_t* buff_bytes,  /* Bytes in buffer (may be updated) */
    int16_t* buff_offset, /* Offset of next byte to extract (updated) */
    bool* temp)             /* Temporary buffer? (set on exit) */
{
  char* rec = NULL;
  int16_t buff_len;
  int16_t offset;
  int16_t bytes;
  char* q;
  int16_t n;

  *temp = FALSE;
  buff_len = *buff_bytes;
  offset = *buff_offset;

  if (offset >= buff_len) /* Read a new buffer */
  {
    buff_len = (int16_t)Read(fu, buff, DISK_BUFFER_SIZE);
    if (buff_len <= 0)
      goto exit_read_merge_record; /* End of data */
    offset = 0;
  }

  /* Fetch the record length */

  bytes = *((int16_t*)(buff + offset));

  /* If the entire record lies in the current buffer, work directly from
    the buffer.  If it extends beyond the current buffer, allocate a
    temporary buffer to hold the record.                                */

  if (offset + bytes <= buff_len) /* It's all in the buffer */
  {
    rec = buff + offset;
    offset += bytes;
  } else /* Must use a temporary buffer */
  {
    rec = (char*)k_alloc(67, bytes);
    *temp = TRUE;

    q = rec;

    while (bytes) {
      n = buff_len - offset;
      if (n == 0) {
        buff_len = (int16_t)Read(fu, buff, DISK_BUFFER_SIZE);
        if (buff_len <= 0)
          goto exit_read_merge_record;
        offset = 0;
        n = buff_len;
      }
      n = min(bytes, n);
      memcpy(q, buff + offset, n);
      q += n;
      bytes -= n;
      offset += n;
    }
  }

exit_read_merge_record:
  *buff_bytes = buff_len;
  *buff_offset = offset;

  return rec;
}

/* ======================================================================
   write_merge_record()  -  Write to merge target file buffer             */

Private bool write_merge_record(
    char* rec,             /* Record to write */
    OSFILE fu,             /* File unit of target file */
    char* buff,            /* Target buffer */
    int16_t* buff_bytes, /* Used space in buffer (updated) */
    bool temp)             /* Delete temporary record buffer? */
{
  bool status = FALSE;
  int16_t bytes;
  int16_t space;
  int16_t used_bytes;
  int16_t n;
  char* q;

  used_bytes = *buff_bytes;

  bytes = *((int16_t*)rec); /* Record length */

  q = rec;
  do {
    space = DISK_BUFFER_SIZE - used_bytes;
    if (space == 0) {
      if (Write(fu, buff, used_bytes) != used_bytes) {
        goto exit_write_merge_record;
      }

      used_bytes = 0;
      space = DISK_BUFFER_SIZE;
    }

    n = min(bytes, space);
    memcpy(buff + used_bytes, q, n);
    q += n;
    used_bytes += n;
    bytes -= n;
  } while (bytes);

  status = TRUE;

exit_write_merge_record:

  if (temp)
    k_free(rec);

  *buff_bytes = used_bytes;

  return status;
}

/* END-CODE */
