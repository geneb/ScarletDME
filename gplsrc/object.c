/* OBJECT.C
 * Object code management
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
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 *
 * 22Feb20 gwb Converted sprintf() to snprintf() in load_object();
 * 
 * START-HISTORY (OpenQM):
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 21 Nov 06  2.4-17 Revised interface to dio_open().
 * 09 Sep 05  2.2-10 Clear hsm_old_obj if discarding old program.
 * 26 Aug 05  2.2-8 0400 Added object invalidation.
 * 17 Nov 04  2.0-10 Added hot spot monitor.
 * 10 Nov 04  2.0-10 0284 An AK I-type that uses SUBR() fails because the
 *                   attempt to read a locally catalogued version of the
 *                   subroutine leaves dh_err set to DHE_RECORD_NOT_FOUND.
 * 20 Sep 04  2.0-2 Use message handler.
 * 20 Sep 04  2.0-2 Use dynamic pcode loader.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include "header.h"
#include "config.h"
#include <time.h>

Private int32_t object_total = 0;  /* Total bytes loaded */
Private int16_t object_items = 0; /* Number of objects loaded */

/* Object code is on an LRU chain with the most recently used item at the
   head.                                                                  */

typedef struct OBJECT OBJECT;
struct OBJECT {
  OBJECT* next; /* LRU chain */
  OBJECT* prev;
  u_int64 cp_time; /* Processor time */
  int32_t calls;
  u_int16_t flags;
#define OBJ_INVALID 0x0001 /* Invalidated object */
#define OBJ_GLOBAL 0x0002  /* Loaded from global catalogue */
  u_int16_t pad;
  struct OBJECT_HEADER code; /* Object code */
};
#define OBJHDRSIZE (offsetof(OBJECT, code))

Private OBJECT* object_head = NULL;
Private OBJECT* object_tail = NULL;
Private int32_t next_id = 1;

Private OBJECT* hsm_old_obj = NULL;

u_int16_t SwapShort(u_int16_t value) {
  int16_t newValue = 0;
  char* pnewValue = (char*)&newValue;
  char* poldValue = (char*)&value;

  pnewValue[0] = poldValue[1];
  pnewValue[1] = poldValue[0];

  return newValue;
}
#define Reverse2(a) a = SwapShort(a)

u_int32_t SwapLong(u_int32_t value) {
  u_int32_t newValue = 0;
  char* pnewValue = (char*)&newValue;
  char* poldValue = (char*)&value;

  pnewValue[0] = poldValue[3];
  pnewValue[1] = poldValue[2];
  pnewValue[2] = poldValue[1];
  pnewValue[3] = poldValue[0];

  return newValue;
}
#define Reverse4(a) a = SwapLong(a)

Private bool discard(void);
Private void hsm_log(OBJECT* obj);

/* ======================================================================
   load_object  -  Load an object item                                    */

void* load_object(char* name, bool abort_on_error) {
  char obj_path[MAX_PATHNAME_LEN + 1];
  OSFILE obj_fu;
  struct OBJECT_HEADER obj_header;
  int object_bytes;
  bool convert;
  OBJECT* obj;
  bool is_runfile = FALSE;
  char mapped_name[MAX_PATHNAME_LEN + 1];
  DESCRIPTOR pathname_descr;
  u_int16_t flags;

  /* Search object chain to see if already loaded */

  for (obj = object_head; obj != NULL; obj = obj->next) {
    if ((!(obj->flags & OBJ_INVALID)) &&
        (strcmp(name, obj->code.ext_hdr.prog.program_name) == 0)) {
      /* Move item to head of lru chain */

      if (obj != object_head) {
        if (obj->next != NULL)
          obj->next->prev = obj->prev;
        else
          object_tail = obj->prev;

        obj->prev->next = obj->next;
        obj->next = object_head;
        object_head->prev = obj;
        object_head = obj;
        obj->prev = NULL;
      }

      if (hsm)
        (obj->calls)++;

      return (void*)(&(obj->code));
    }
  }

  /* Not already loaded  -  must search for object.  Search sequence is:
     1.  Names containing a \ character are runfile pathnames
     2.  Try the local and private catalogues unless the name has a prefix
     3.  Try as a locally catalogued item
  */

  flags = 0;

  if (strchr(name, DS) != NULL) /* Runfile pathname */
  {
    obj_fu = dio_open(name, DIO_READ);
    if (!ValidFileHandle(obj_fu)) {
      if (abort_on_error)
        k_error(sysmsg(1123), name);
      else
        return NULL;
    }
    is_runfile = TRUE;
    goto found;
  }

  (void)map_t1_id(name, strlen(name), mapped_name);

  if (strchr("*$!_", name[0]) == NULL) {
    /* Try local catalogue */

    /* Push arguments onto e-stack */

    k_put_c_string(name, e_stack++);
    InitDescr(e_stack, ADDR); /* File path name (output) */
    (e_stack++)->data.d_addr = &pathname_descr;
    pathname_descr.type = UNASSIGNED;

    /* Execute recursive code */

    k_recurse(pcode_voc_cat, 2); 
    dh_err = 0; /* 0284 Ensure failure to find program doesn't give an error */

    /* Extract result string */
    k_get_c_string(&pathname_descr, obj_path, MAX_PATHNAME_LEN);
    k_release(&pathname_descr);
    if (obj_path[0] != '\0') {
      obj_fu = dio_open(obj_path, DIO_READ);
      if (ValidFileHandle(obj_fu))
        goto found;

      if (abort_on_error)
        k_error(sysmsg(1124), name);
      else
        return NULL;
    }

    /* Try private catalogue */
    /* converted to snprintf() - gwb 22Feb20 */
    if (snprintf(obj_path, MAX_PATHNAME_LEN + 1, "%s%c%s", private_catalogue, 
            DS, mapped_name) >= (MAX_PATHNAME_LEN + 1)) {
       /* TODO: this should be sent to the system log. */          
       k_error("Overflowed directory/pathname length in load_object()!");
       return NULL;
    }
    obj_fu = dio_open(obj_path, DIO_READ);
    if (ValidFileHandle(obj_fu))
      goto found;
  }
  /* converted to snprintf() -gwb 22Feb20 */
  if (snprintf(obj_path, MAX_PATHNAME_LEN + 1, "%s%cgcat%c%s", sysseg->sysdir, 
            DS, DS, mapped_name) >= (MAX_PATHNAME_LEN + 1)) {
     /* TODO: this should be sent to the system log. */          
     k_error("Overflowed directory/pathname length in load_object()!");
     return NULL;
   }
  obj_fu = dio_open(obj_path, DIO_READ);
  if (ValidFileHandle(obj_fu)) {
    flags |= OBJ_GLOBAL;
    goto found;
  }

  if (abort_on_error)
    k_error(sysmsg(1125), name); /* TODO: Magic numbers are bad, mmkay? */
  else
    return NULL;

found:
  /* Object found  - read header */

  if (Read(obj_fu, (char*)(&obj_header), OBJECT_HEADER_SIZE) < 0) {
    if (abort_on_error)
      k_error(sysmsg(1126), name); /* TODO: Magic numbers are bad, mmkay? */
    else
      return NULL;
  }

  switch (obj_header.magic) {
    case HDR_MAGIC:
      convert = FALSE;
      object_bytes = obj_header.object_size;
      break;
    case HDR_MAGIC_INVERSE:
      convert = TRUE;
      object_bytes = SwapLong(obj_header.object_size);
      break;
    default:
      if (abort_on_error)
        k_error(sysmsg(1127), name); /* TODO: Magic numbers are bad, mmkay? */
      else
        return NULL;
  }

  /* Consider discards if too much loaded.  May have items with 0 ref ct */

  while (((pcfg.objmem != 0) && (object_total > pcfg.objmem)) ||
         ((pcfg.objects != 0) && (object_items > pcfg.objects))) {
    if (!discard())
      break;
  }

  /* Load program into memory */

  object_items++;
  object_total += object_bytes;
  obj = (OBJECT*)k_alloc(22, OBJHDRSIZE + object_bytes);

  obj->cp_time = 0;
  obj->calls = (hsm) ? 1 : 0;
  obj->flags = flags;

  Seek(obj_fu, 0, SEEK_SET);
  if (Read(obj_fu, (char*)(&(obj->code)), object_bytes) < 0) {
    k_free(obj);
    k_error(sysmsg(1128), name); /* TODO: Magic numbers are bad, mmkay? */
  }

  if (convert) {
    convert_object_header(&(obj->code));
  }

  obj->code.ext_hdr.prog.refs = 0;

  /* 0123 Ensure name is correct in header. In theory, this should only be
     necessary for runfiles and locally catalogued items but we might as
     well do it for everything.                                            */

  strcpy(obj->code.ext_hdr.prog.program_name, name);

  if (is_runfile)
    obj->code.id = -(next_id++); /* Run files have negative ids */
  else
    obj->code.id = next_id++;

  /* Add to head of object chain */

  if (object_head != NULL)
    object_head->prev = obj;
  obj->next = object_head;
  obj->prev = NULL;
  object_head = obj;

  CloseFile(obj_fu);
  return (void*)(&(obj->code));
}

/* ======================================================================
   op_loaded  -  Check if object code is loaded                           */

void op_loaded() {
  /* Stack:

      |=============================|=============================|
      |            BEFORE           |           AFTER             |
      |=============================|=============================|
  top |  Id of object to find       | Loaded? True/false          |
      |=============================|=============================|
  */

  DESCRIPTOR* descr;
  OBJECT* obj;
  int32_t id;

  descr = e_stack - 1;
  GetInt(descr);
  id = descr->data.value;

  for (obj = object_head; obj != NULL; obj = obj->next) {
    if (obj->code.id == id) {
      descr->data.value = TRUE;
      goto exit_op_loaded;
    }
  }

  descr->data.value = FALSE;

exit_op_loaded:
  return;
}

/* ======================================================================
   op_unload  -  Unload all inactive object code                          */

void op_unload() {
  unload_all();
}

/* ======================================================================
   is_global() - Is object code from global catalogue?                    */

bool is_global(void* obj_hdr) {
  return (((OBJECT*)(((char*)obj_hdr) - OBJHDRSIZE))->flags & OBJ_GLOBAL) != 0;
}

/* ======================================================================
   unload_object  -  Unload object from cache                             */

void unload_object(void* obj_hdr) {
  OBJECT* obj;

  obj = (OBJECT*)(((char*)obj_hdr) - OBJHDRSIZE);

  if (hsm)
    hsm_log(obj);
  if (obj == hsm_old_obj)
    hsm_old_obj = NULL;

  /* Forward link */

  if (obj->next != NULL)
    obj->next->prev = obj->prev;
  if (obj->prev == NULL)
    object_head = obj->next;

  /* Backward link */

  if (obj->prev != NULL)
    obj->prev->next = obj->next;
  if (obj->next == NULL)
    object_tail = obj->prev;

  /* Give away memory */

  object_items--;
  object_total -= ((OBJECT_HEADER*)obj_hdr)->object_size;
  k_free(obj);
}

/* ======================================================================
   unload_all  -  Unload all cached objects not in use                    */

void unload_all() {
  OBJECT* obj;
  OBJECT* next_object;

  for (obj = object_head; obj != NULL; obj = next_object) {
    next_object = obj->next;

    if (obj->code.ext_hdr.prog.refs == 0) {
      if (hsm)
        hsm_log(obj);
      if (obj == hsm_old_obj)
        hsm_old_obj = NULL;

      /* Forward link */

      if (obj->next != NULL)
        obj->next->prev = obj->prev;
      if (obj->prev == NULL)
        object_head = obj->next;

      /* Backward link */

      if (obj->prev != NULL)
        obj->prev->next = obj->next;
      if (obj->next == NULL)
        object_tail = obj->prev;

      object_items--;
      object_total -= obj->code.object_size;
      k_free(obj);
    }
  }
}

/* ======================================================================
   invalidate_object()  -  Invalidate non-global prefixed object code     */

void invalidate_object() {
  /* This function is called when performing a LOGTO. It must invalidate
     all object code where the object name in the cache could clash across
     the old and new accounts. This is everything except globally
     catalogued items that start with a reserved prefix character.         */

  OBJECT* obj;
  OBJECT* next_object;

  for (obj = object_head; obj != NULL; obj = next_object) {
    next_object = obj->next;

    if (!(obj->flags & OBJ_GLOBAL) ||
        (strchr("*$!_", obj->code.ext_hdr.prog.program_name[0]) == NULL)) {
      obj->flags |= OBJ_INVALID;
    }
  }
}

/* ======================================================================
  find_object()  -  Find object by id number                              */

void* find_object(int32_t id) {
  OBJECT* obj;

  for (obj = object_head; obj != NULL; obj = obj->next) {
    if (obj->code.id == id)
      return (void*)(&(obj->code));
  }

  return NULL;
}

/* ======================================================================
   discard  -  Discard an unreferenced object                             */

Private bool discard() {
  OBJECT* obj;
  OBJECT* prev_object;

  for (obj = object_tail; obj != NULL; obj = prev_object) {
    prev_object = obj->prev;

    if (obj->code.ext_hdr.prog.refs == 0) {
      if (hsm)
        hsm_log(obj);
      if (obj == hsm_old_obj)
        hsm_old_obj = NULL;

      /* Forward link */

      if (obj->next != NULL)
        obj->next->prev = obj->prev;
      if (obj->prev == NULL)
        object_head = obj->next;

      /* Backward link */

      if (obj->prev != NULL)
        obj->prev->next = obj->next;
      if (obj->next == NULL)
        object_tail = obj->prev;

      object_items--;
      object_total -= obj->code.object_size;
      k_free(obj);

      return TRUE;
    }
  }

  return FALSE; /* Nothing to discard */
}

/* ======================================================================
   convert_object_header()  -  Byte swap object header                    */

void convert_object_header(OBJECT_HEADER* obj) {
  Reverse4(obj->id);
  Reverse4(obj->start_offset);
  Reverse2(obj->args);
  Reverse2(obj->no_vars);
  Reverse2(obj->stack_depth);
  Reverse4(obj->sym_tab_offset);
  Reverse4(obj->line_tab_offset);
  Reverse4(obj->object_size);
  Reverse2(obj->flags);
  Reverse4(obj->compile_time);
  obj->magic = HDR_MAGIC;

  /* If it ever becomes necessary to convert the actual object stream, the
     code to do it is in qmconv.  Currently, only float constants cause a
     problem and we convert these where necessary as part of LDFLOAT.     */
}

/* ======================================================================
   ==========              Hot Spot Monitor (HSM)              ==========
   ====================================================================== */

typedef struct HSM HSM;
struct HSM {
  HSM* next;
  u_int64 cp_time;
  int32_t calls;
  char name[1];
};

Private HSM* hsm_head = NULL;
Private u_int64 last_cp = 0;

/* ======================================================================
   hsm_log()  -  Update hot spot monitor data from OBJECT structure       */

Private void hsm_log(OBJECT* obj) {
  HSM* p;
  int bytes;

  hsm_enter(); /* Force update of OBJECT cp_time */

  for (p = hsm_head; p != NULL; p = p->next) {
    if (!strcmp(obj->code.ext_hdr.prog.program_name, (char*)(p->name))) {
      /* Found it */

      p->cp_time += obj->cp_time;
      p->calls += obj->calls;
      goto exit_hsm_log;
    }
  }

  /* Not found - Make a new entry */

  bytes = sizeof(HSM) + strlen(obj->code.ext_hdr.prog.program_name);
  p = (HSM*)k_alloc(91, bytes);
  p->next = hsm_head;
  hsm_head = p;

  strcpy((char*)(p->name), obj->code.ext_hdr.prog.program_name);
  p->cp_time = obj->cp_time;
  p->calls = obj->calls;

exit_hsm_log:
  obj->cp_time = 0;
  obj->calls = 0;
}

/* ======================================================================
   hsm_on()  -  Start monitoring, clearing cached data                    */

void hsm_on() {
  HSM* p;
  HSM* q;
  OBJECT* obj;

  for (obj = object_head; obj != NULL; obj = obj->next) {
    obj->calls = 0;
    obj->cp_time = 0;
  }

  for (p = hsm_head; p != NULL; p = q) {
    q = p->next;
    k_free(p);
  }
  hsm_head = NULL;

  last_cp = (((u_int64)clock()) * 1000000L) / CLOCKS_PER_SEC;

  hsm = TRUE;
}

/* ======================================================================
   hsm_enter() - Enter a new program (or just force an update)            */

void hsm_enter() {
  u_int64 cp;
  OBJECT* new_obj;

  if (c_base != NULL) {
    new_obj = (OBJECT*)(c_base - OBJHDRSIZE);

    cp = (((u_int64)clock()) * 1000000L) / CLOCKS_PER_SEC;

    if (hsm_old_obj != NULL)
      hsm_old_obj->cp_time += cp - last_cp;

    if (new_obj->code.id != 0)
      hsm_old_obj = new_obj; /* It's a real program */

    last_cp = cp;
  }
}

/* ======================================================================
   hsm_dump()  -  Return HSM data                                         */

STRING_CHUNK* hsm_dump() {
  HSM* p;
  OBJECT* obj;
  STRING_CHUNK* str;

  /* Update cache for all active programs */

  if (hsm) {
    for (obj = object_head; obj != NULL; obj = obj->next) {
      hsm_log(obj);
    }
  }

  str = NULL;
  ts_init(&str, 256);

  for (p = hsm_head; p != NULL; p = p->next) {
    if (str != NULL)
      ts_copy_byte(FIELD_MARK);
    ts_printf("%s\xfd%d\xfd%lld", p->name, p->calls, p->cp_time / 1000);
  }

  ts_terminate();

  return str;
}

/* END-CODE */
