/* OP_BTREE.C
 * Binary tree handling opcodes
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
 * 11Jan22 gwb Fix for Issue #17. 
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 *
 * START-HISTORY (OpenQM):
 * 05 Aug 07  2.5-7 0557 Reworked free_btree_element() not to use a recursive
 *                  method as sorts with a very large number of records with the
 *                  same sort key could overrun stack area.
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * Opcodes:
 *    op_btadd             BTADD opcode
 *    op_btadda            BTADDA opcode
 *    op_btfind            BTFIND opcode
 *    op_btinit            BTINIT opcode
 *    op_btmodify          BTMODIFY opcode
 *    op_btreset           BTRESET opcode
 *    op_btscan            BTSCAN opcode
 *    op_btscana           BTSCANA opcode
 *
 * Other externally callable functions:
 *    btree_to_string      Flatten a BTree to a delimited string
 *    free_btree_element   Free a BTree element
 *    bt_get_string        Allocate string in BTree element
 *
 * Private functions:
 *    bt_not_a_btree       Error handler
 *    bt_key_count         Error handler
 *    bt_no_mem            Error handler
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"

#define MAX_BTREE_KEYS 10

int bt_str_bytes; /* Bytes allocated by bt_get_string including \0 at end */

Private void bt_not_a_btree(void);
Private void bt_key_count(void);
Private void bt_no_mem(void);

/* ======================================================================
   op_btadd()  -  Add entry to BTREE variable                             */

void op_btadd() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  ADDR to BTREE              |                             |
     |-----------------------------|-----------------------------| 
     |  Data                       |                             |
     |-----------------------------|-----------------------------| 
     |  Item to add                |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;
  BTREE_HEADER* bth;
  BTREE_ELEMENT* bte;
  BTREE_ELEMENT* new_bte;
  int16_t d = 0;
  char* key;
  char* key_str;
  char* bte_str;
  bool right_justified;
  bool descending;
  int16_t n;
  bool numeric_compare = FALSE;
  double new_value; /* Value being inserted */
  double key_value; /* Value being compared from tree */

  /* Find the BTree */

  descr = e_stack - 1;
  do {
    descr = descr->data.d_addr;
  } while (descr->type == ADDR);
  if (descr->type != BTREE)
    bt_not_a_btree();

  bth = descr->data.btree;
  if (bth->keys != 1)
    bt_key_count();

  /* Make a new BTree element */

  new_bte = (BTREE_ELEMENT*)k_alloc(26, sizeof(struct BTREE_ELEMENT));
  if (new_bte == NULL)
    bt_no_mem();

  new_bte->left = NULL;
  new_bte->right = NULL;
  new_bte->data = NULL;
  new_bte->key[0] = NULL;

  /* Find the data */

  descr = e_stack - 2;
  if (!bt_get_string(descr, &(new_bte->data))) {
    free_btree_element(new_bte, 1); /* Free memory */
    bt_no_mem();
  }

  /* Find the key */

  descr = e_stack - 3;
  if (!bt_get_string(descr, &(new_bte->key[0]))) {
    free_btree_element(new_bte, 1); /* Free memory */
    bt_no_mem();
  }

  /* Walk tree to insertion point */

  process.status = 0;
  bte = bth->head;
  if (bte == NULL) /* Inserting first element */
  {
    bth->head = new_bte;
    new_bte->parent = NULL;
  } else {
    n = bth->flags[0];
    right_justified = n & BT_RIGHT_ALIGNED;
    descending = (n & BT_DESCENDING) != 0;
    key = new_bte->key[0];
    if (key == NULL)
      key = "";

    if (right_justified) {
      numeric_compare = strdbl(key, &new_value);
    }

    do {
      /* Compare strings */

      key_str = key;
      bte_str = bte->key[0];
      if (bte_str == NULL)
        bte_str = "";

      if (right_justified) {
        if (numeric_compare) {
          if (strdbl(bte_str, &key_value)) {
            if (new_value > key_value)
              d = 1;
            else if (new_value == key_value)
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
          goto exit_btadd;
        }
        bte = bte->left;
      } else if (d > 0) {
        if (bte->right == NULL) /* Add as new right entry */
        {
          bte->right = new_bte;
          new_bte->parent = bte;
          goto exit_btadd;
        }
        bte = bte->right;
      } else /* Matching key value */
      {
        if (bth->flags[0] & BT_UNIQUE) {
          free_btree_element(new_bte, 1);
          process.status = 1;
          goto exit_btadd;
        }

        /* Duplicate key.  Insert to left. */
        if (bte->left == NULL) /* Add as new left entry */
        {
          bte->left = new_bte;
          new_bte->parent = bte;
          goto exit_btadd;
        }
        bte = bte->left;
      }
    } while (1);
  }

exit_btadd:
  k_pop(1);    /* ADDR to BTree */
  k_dismiss(); /* Data */
  k_dismiss(); /* Key */
}

/* ======================================================================
   op_btadda()  -  Add entry to BTREE variable using keys array           */

void op_btadda() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  ADDR to BTREE              |                             |
     |-----------------------------|-----------------------------| 
     |  Data                       |                             |
     |-----------------------------|-----------------------------| 
     |  ADDR to keys array         |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;
  int keys;
  BTREE_HEADER* bth;
  ARRAY_HEADER* a_hdr;
  BTREE_ELEMENT* bte;
  BTREE_ELEMENT* new_bte;
  int16_t index;
  int16_t d = 0;
  char* key;
  char* key_str;
  char* bte_str;
  bool right_justified;
  bool descending;
  int16_t n;
  bool numeric_compare[MAX_BTREE_KEYS];
  double new_value[MAX_BTREE_KEYS]; /* Value being inserted */
  double key_value;                 /* Value being compared from tree */

  /* Find the BTree */

  descr = e_stack - 1;
  do {
    descr = descr->data.d_addr;
  } while (descr->type == ADDR);
  if (descr->type != BTREE)
    bt_not_a_btree();
  bth = descr->data.btree;
  keys = bth->keys; /* Number of keys */

  /* Find the keys array */

  descr = e_stack - 3;
  while (descr->type == ADDR)
    descr = descr->data.d_addr;
  if (descr->type != ARRAY)
    k_not_array_error(descr);

  a_hdr = descr->data.ahdr_addr;
  if ((a_hdr->rows != keys) || (a_hdr->cols != 0))
    bt_key_count();

  /* Make a new BTree element */

  new_bte = (BTREE_ELEMENT*)k_alloc(
      27, sizeof(struct BTREE_ELEMENT) + ((keys - 1) * sizeof(char*)));
  if (new_bte == NULL)
    bt_no_mem();

  new_bte->left = NULL;
  new_bte->right = NULL;
  new_bte->data = NULL;
  for (index = 0; index < keys; index++)
    new_bte->key[index] = NULL;

  for (index = 0; index < keys; index++) {
    descr = (a_hdr->chunk[(index + 1) / MAX_ARRAY_CHUNK_SIZE])->descr +
            ((index + 1) % MAX_ARRAY_CHUNK_SIZE);
    if (!bt_get_string(descr, &(new_bte->key[index]))) {
      free_btree_element(new_bte, keys); /* Free memory */
      bt_no_mem();
    }
    key_str = new_bte->key[index];
    if (key_str == NULL)
      key_str = "";
    numeric_compare[index] = strdbl(key_str, &new_value[index]);
  }

  /* Find the data */

  descr = e_stack - 2;
  if (!bt_get_string(descr, &(new_bte->data))) {
    free_btree_element(new_bte, 1); /* Free memory */
    bt_no_mem();
  }

  /* Walk tree to insertion point */

  process.status = 0;
  bte = bth->head;
  if (bte == NULL) /* Inserting first element */
  {
    bth->head = new_bte;
    new_bte->parent = NULL;
  } else {
    index = 0;

    do {
      key = new_bte->key[index];
      if (key == NULL)
        key = "";

      n = bth->flags[index];
      right_justified = n & BT_RIGHT_ALIGNED;
      descending = (n & BT_DESCENDING) != 0;

      /* Compare strings */

      key_str = key;
      bte_str = bte->key[index];
      if (bte_str == NULL)
        bte_str = "";

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
          goto exit_btadda;
        }
        bte = bte->left;
        index = 0;
      } else if (d > 0) {
        if (bte->right == NULL) /* Add as new right entry */
        {
          bte->right = new_bte;
          new_bte->parent = bte;
          goto exit_btadda;
        }
        bte = bte->right;
        index = 0;
      } else /* Matching key value */
      {
        if (bth->flags[index] & BT_UNIQUE) {
          free_btree_element(new_bte, keys);
          process.status = 1;
          goto exit_btadda;
        }

        /* Move down to lower level index if there is one */

        if (++index == keys) {
          /* Complete duplicate at all key levels.  Insert to left. */
          if (bte->left == NULL) /* Add as new left entry */
          {
            bte->left = new_bte;
            new_bte->parent = bte;
            goto exit_btadda;
          }
          bte = bte->left;
          index = 0;
        }
      }
    } while (1);
  }

exit_btadda:
  k_pop(1);    /* ADDR to BTree */
  k_dismiss(); /* Data */
  k_pop(1);    /* ADDR to keys array */
}

/* ======================================================================
   op_btfind()  -  Find entry in single key BTREE                         */

void op_btfind() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Key to find                | Data of item found          |
     |                             | Null string if not found    !
     |-----------------------------|-----------------------------|
     |  ADDR to BTREE              |                             |
     |=============================|=============================|

     STATUS() non-zero if item not found.
 */

  DESCRIPTOR* descr;
  BTREE_HEADER* bth;
  BTREE_ELEMENT* bte;
  int16_t d = 0;
  char key[256 + 1];
  char* key_str;
  char* bte_str;
  bool right_justified;
  bool descending;
  int16_t n;
  bool numeric_compare = FALSE;
  double find_value; /* Value being sought */
  double key_value;  /* Value being compared from tree */

  /* Find the BTree */

  descr = e_stack - 2;
  do {
    descr = descr->data.d_addr;
  } while (descr->type == ADDR);
  if (descr->type != BTREE)
    bt_not_a_btree();
  bth = descr->data.btree;
  if (bth->keys != 1)
    bt_key_count();

  bth->current = NULL;

  /* Find the key */

  descr = e_stack - 1;
  k_get_c_string(descr, key, 256);

  /* Walk tree */

  n = bth->flags[0];
  right_justified = n & BT_RIGHT_ALIGNED;
  descending = (n & BT_DESCENDING) != 0;

  if (right_justified) {
    numeric_compare = strdbl(key, &find_value);
  }

  bte = bth->head;
  while (bte != NULL) {
    /* Compare strings */

    key_str = key;
    bte_str = bte->key[0];
    if (bte_str == NULL)
      bte_str = "";

    if (right_justified) {
      if (numeric_compare) {
        if (strdbl(bte_str, &key_value)) {
          if (find_value > key_value)
            d = 1;
          else if (find_value == key_value)
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

    if (d < 0)
      bte = bte->left;
    else if (d > 0)
      bte = bte->right;
    else /* Found desired key value */
    {
      bth->current = bte;
      break;
    }
  }

  k_dismiss();
  k_pop(1);
  if (bth->current == NULL) {
    InitDescr(e_stack, STRING);
    (e_stack++)->data.str.saddr = NULL;
    process.status = 1;
  } else {
    k_put_c_string(bth->current->data, e_stack);
    e_stack++;
    process.status = 0;
  }
}

/* ======================================================================
   op_btinit()  -  Initialise BTREE variable                              */

void op_btinit() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  ADDR to flags array        | BTREE variable              |
     |-----------------------------|-----------------------------| 
     |  Number of keys             |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;
  int keys;
  BTREE_HEADER* bth;
  ARRAY_HEADER* a_hdr;
  int i; /* fix for issue #17 */

  descr = e_stack - 2;
  GetInt(descr);
  keys = descr->data.value;
  if ((keys < 1) || (keys > MAX_BTREE_KEYS)) {
    k_error("Invalid BTree key count (%d)", keys);
  }

  descr = e_stack - 1;
  while (descr->type == ADDR)
    descr = descr->data.d_addr;
  if (descr->type != ARRAY)
    k_not_array_error(descr);
  a_hdr = descr->data.ahdr_addr;
  if ((keys > a_hdr->rows) || (a_hdr->cols != 0))
    k_index_error(descr);

  bth = (BTREE_HEADER*)k_alloc(28, sizeof(struct BTREE_HEADER) + (keys - 1));
  if (bth == NULL)
    k_error("Insufficient memory for BTree header");

  bth->ref_ct = 1;
  bth->head = NULL;
  bth->current = NULL;
  bth->keys = keys;
  for (i = 1; i <= keys; i++) {
    descr = (a_hdr->chunk[i / MAX_ARRAY_CHUNK_SIZE])->descr +
            (i % MAX_ARRAY_CHUNK_SIZE);
    GetInt(descr); /* Side effect - may convert array element type */
    bth->flags[i - 1] = (u_char)(descr->data.value);
  }

  k_pop(2);

  InitDescr(e_stack, BTREE);
  (e_stack++)->data.btree = bth;
}

/* ======================================================================
   op_btmodify()  -  Modify current BTree element                         */

void op_btmodify() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Data to write              |                             |
     |-----------------------------|-----------------------------|
     |  ADDR to BTREE              |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;
  BTREE_HEADER* bth;
  BTREE_ELEMENT* bte;

  /* Find the BTree */

  descr = e_stack - 2;
  do {
    descr = descr->data.d_addr;
  } while (descr->type == ADDR);
  if (descr->type != BTREE)
    bt_not_a_btree();
  bth = descr->data.btree;
  bte = bth->current;
  if (bte == NULL)
    k_error("No current BTree element");

  if (bte->data != NULL) {
    k_free(bte->data);
    bte->data = NULL;
  }

  descr = e_stack - 1;
  if (!bt_get_string(descr, &(bte->data)))
    bt_no_mem();

  k_dismiss();
  k_pop(1);
}

/* ======================================================================
   op_btreset()  -  Reset scan posiiton in BTree                          */

void op_btreset() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  ADDR to BTREE              |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;
  BTREE_HEADER* bth;
  BTREE_ELEMENT* bte;

  /* Find the BTree */

  descr = e_stack - 1;
  do {
    descr = descr->data.d_addr;
  } while (descr->type == ADDR);
  if (descr->type != BTREE)
    bt_not_a_btree();
  bth = descr->data.btree;
  bte = bth->head;
  if (bte != NULL)
    while (bte->left != NULL)
      bte = bte->left;
  bth->current = bte;
  k_pop(1);
}

/* ======================================================================
   op_btscan()  -  Scan BTree                                             */

void op_btscan() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  ADDR to BTREE              | Data from item              |
     |=============================|=============================|

    Returns key value of data or final key.

    Returns STATUS() non-zero if no item found
 */

  DESCRIPTOR* descr;
  BTREE_HEADER* bth;
  BTREE_ELEMENT* bte;
  BTREE_ELEMENT* old_bte;
  int16_t keys;

  /* Find the BTree */

  descr = e_stack - 1;
  do {
    descr = descr->data.d_addr;
  } while (descr->type == ADDR);
  if (descr->type != BTREE)
    bt_not_a_btree();
  bth = descr->data.btree;
  bte = bth->current;
  keys = bth->keys;
  k_pop(1);

  /* Return data from current item */

  if (bth->current == NULL) {
    process.status = 1;
    InitDescr(e_stack, STRING);
    (e_stack++)->data.str.saddr = NULL;
    goto exit_btscan;
  }

  process.status = 0;
  if (bth->flags[keys - 1] & BT_DATA) {
    k_put_c_string(bte->data, e_stack);
  } else {
    k_put_c_string(bte->key[bth->keys - 1], e_stack);
  }
  e_stack++;

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

  bth->current = bte;
exit_btscan:
  return;
}

/* ======================================================================
   op_btscana()  -  Scan BTree returning data and keys                    */

void op_btscana() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  ADDR to keys array         | Data from item              |
     |-----------------------------|-----------------------------|
     |  ADDR to BTREE              |                             |
     |=============================|=============================|

    Returns key value of data or final key.
    Keys array is set to all keys of item found.

    Returns STATUS() non-zero if no item found
 */

  DESCRIPTOR* descr;
  ARRAY_HEADER* a_hdr;
  BTREE_HEADER* bth;
  BTREE_ELEMENT* bte;
  BTREE_ELEMENT* old_bte;
  int16_t index;
  int16_t keys;

  /* Find the BTree */

  descr = e_stack - 2;
  do {
    descr = descr->data.d_addr;
  } while (descr->type == ADDR);
  if (descr->type != BTREE)
    bt_not_a_btree();
  bth = descr->data.btree;
  bte = bth->current;
  keys = bth->keys;

  /* Find the keys array */

  descr = e_stack - 1;
  while (descr->type == ADDR)
    descr = descr->data.d_addr;
  if (descr->type != ARRAY)
    k_not_array_error(descr);

  a_hdr = descr->data.ahdr_addr;
  if ((a_hdr->rows != keys) || (a_hdr->cols != 0))
    bt_key_count();

  k_pop(2);

  /* Return data from current item */

  if (bth->current == NULL) {
    process.status = 1;
    InitDescr(e_stack, STRING);
    (e_stack++)->data.str.saddr = NULL;
    goto exit_btscan;
  }

  process.status = 0;
  if (bth->flags[keys - 1] & BT_DATA) {
    k_put_c_string(bte->data, e_stack);
  } else {
    k_put_c_string(bte->key[bth->keys - 1], e_stack);
  }
  e_stack++;

  /* Copy keys to array */

  for (index = 1; index <= keys; index++) {
    descr = (a_hdr->chunk[index / MAX_ARRAY_CHUNK_SIZE])->descr +
            (index % MAX_ARRAY_CHUNK_SIZE);
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
  {      /*    coming up a right limb */
    do {
      old_bte = bte;
      bte = bte->parent;
      if (bte == NULL)
        break;
    } while (old_bte == bte->right);
  }

  bth->current = bte;
exit_btscan:
  return;
}

/* ======================================================================
   btree_to_string()  -  Flatten a BTree into a delimited string          */

void btree_to_string(DESCRIPTOR* descr) {
  BTREE_HEADER* bth;
  BTREE_ELEMENT* bte;
  BTREE_ELEMENT* old_bte;
  STRING_CHUNK* str_hdr;
  char* p;
  int16_t n;

  /* Find the BTree */

  bth = descr->data.btree;
  n = bth->keys - 1;

  ts_init(&str_hdr, 128);

  bte = bth->head;
  if (bte != NULL)
    while (bte->left != NULL)
      bte = bte->left;
  while (bte != NULL) {
    if (bth->flags[n] & BT_DATA)
      p = bte->data;
    else
      p = bte->key[n];
    ts_copy_c_string(p);

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

    if (bte != NULL)
      ts_copy_byte(FIELD_MARK);
  }

  ts_terminate();

  k_release(descr);
  InitDescr(descr, STRING);
  descr->data.str.saddr = str_hdr;
}

/* ======================================================================
   free_btree_element()  -  Free a BTree element                          */

void free_btree_element(BTREE_ELEMENT* element, int16_t keys) {
  int16_t i;
  BTREE_ELEMENT* bte;
  BTREE_ELEMENT* parent;

  if (element != NULL) {
    bte = element;
    do {
      /* Descend to bottom of tree, following left branch where possible */

      do {
        while (bte->left != NULL)
          bte = bte->left;
        if (bte->right == NULL)
          break;
        bte = bte->right;
      } while (1);

      /* We are now at a node with no children. Release the data, keys and
          the node itself.                                                  */

      if (bte->data != NULL)
        k_free(bte->data);
      for (i = 0; i < keys; i++) {
        if (bte->key[i] != NULL)
          k_free(bte->key[i]);
      }

      parent = bte->parent;

      k_free(bte);
      if (bte == element)
        break; /* Released the top item of interest */

      /* Free the pointer that took us down to the node we just released */

      if (parent->left == bte)
        parent->left = NULL;
      else if (parent->right == bte)
        parent->right = NULL;

      bte = parent;

    } while (1);
  }
}

/* ======================================================================
   bt_get_string()  -  Allocate string in BTree element                   */

bool bt_get_string(
    DESCRIPTOR* descr, /* Item to get (may not be a string, will not convert) */
    char** s)          /* Ptr to target string ptr */
{
  DESCRIPTOR str_descr;
  STRING_CHUNK* str;
  char* p;
  bool converted = FALSE;
  bool status = TRUE;

  if (descr->type != STRING) {
    InitDescr(&str_descr, ADDR);
    str_descr.data.d_addr = descr;
    k_get_string(&str_descr);
    descr = &str_descr;
    converted = TRUE;
  }

  str = descr->data.str.saddr;
  if (str == NULL)
    *s = NULL;
  else {
    bt_str_bytes = str->string_len + 1;
    p = (char*)k_alloc(29, bt_str_bytes);
    if (p == NULL) {
      status = FALSE;
      goto exit_bt_get_string;
    }
    k_get_c_string(descr, p, str->string_len);
    *s = p;
  }

exit_bt_get_string:
  if (converted)
    k_release(&str_descr);
  return status;
}

/* ======================================================================
   Common error functions                                                 */

Private void bt_not_a_btree() {
  k_error("Not a BTree");
}

Private void bt_key_count() {
  k_error("Incorrect number of BTree keys");
}

Private void bt_no_mem() {
  k_error("Insufficient memory for BTree element or data");
}

/* END-CODE */
