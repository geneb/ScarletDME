/* OBJPROG.C
 * Object programming opcodes.
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
 * START-HISTORY:
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 19 Jun 07  2.5-7 k_call() now has additional argument for stack adjustment.
 * 28 Feb 07  2.5-0 Added read-only public variable handling.
 * 04 Aug 06  2.4-11 Change of definition. The undefined name handler should be
 *                   inherited according to the normal name search rules.
 * 03 Aug 06  2.4-10 Mark the destructor routine as having been run so that it
 *                   does not get run again recursively if it aborts.
 * 21 Jul 06  2.4-10 Added inheritance.
 * 21 Jun 06  2.4-5 Added op_objinfo().
 * 22 May 06  2.4-4 0489 Track whether an object function returns a value by
 *                  recalculating the stack depth as the stack may move.
 * 06 Apr 06  2.4-1 Added support for variable argument counts.
 * 22 Mar 06  2.3-9 New module.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * Opcodes:
 *   op_object     Instantiate object
 *   op_objinfo    Object information
 *   op_objmap     Create the name map
 *   op_objref     Resolve an object reference
 *   op_get        Retrieve a property value or execute a public function
 *   op_set        Set a property value or execute a public subroutine
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include "keys.h"
#include "header.h"
#include "stdarg.h"

Private bool find_name_map_entry(short int mode, OBJDATA * objdata, char * name, bool dismiss_obj);
Private bool find_undefined_name_handler(short int mode, OBJDATA * objdata, char * name, bool dismiss_obj);
Private DESCRIPTOR * resolve_index(DESCRIPTOR * array_descr,
                                   DESCRIPTOR * indx_descr, short int dims);

/* ======================================================================
   op_disinh()  -  DISINHERIT                                             */

void op_disinh()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Object                     |                             |
     |=============================|=============================|
 */

 DESCRIPTOR * descr;
 OBJDATA * inh_objdata;  /* Inherited OBJ structure */
 OBJDATA * obj;

 if (!(process.program.flags & HDR_IS_CLASS)) k_error("DISINHERIT not in class");

 descr = e_stack - 1;
 while(descr->type == ADDR) descr = descr->data.d_addr;
 if (descr->type != OBJ) k_error(sysmsg(3460)); /* Object variable required */

 inh_objdata = descr->data.objdata;

 /* Scan inheritance chain */

 if (process.program.objdata->inherits == NULL)
  {
   if (process.program.objdata->inherits == inh_objdata)
    {
     process.program.objdata->inherits = inh_objdata->next_inherited;
     free_object(inh_objdata);
    }
  }
 else
  {
   for(obj = process.program.objdata->inherits; obj->next_inherited != NULL;
       obj = obj->next_inherited)
    {
     if (obj->next_inherited == inh_objdata)
      {
       obj->next_inherited = inh_objdata->next_inherited;
       free_object(inh_objdata);
       break;
      }
    }
  }

 k_dismiss();
}

/* ======================================================================
   op_inherit()  -  Establish inherited object                            */

void op_inherit()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Object                     |                             |
     |=============================|=============================|
 */

 DESCRIPTOR * descr;
 OBJDATA * objdata;      /* Our own OBJ structure */
 OBJDATA * inh_objdata;  /* Inherited OBJ structure */
 OBJDATA * obj;

 if (!(process.program.flags & HDR_IS_CLASS)) k_error("INHERIT not in class");

 descr = e_stack - 1;
 while(descr->type == ADDR) descr = descr->data.d_addr;
 if (descr->type != OBJ) k_error(sysmsg(3460)); /* Object variable required */

 inh_objdata = descr->data.objdata;

 /* Add to inheritance chain */

 objdata = process.program.objdata;
 if (objdata->inherits == NULL)
  {
   objdata->inherits = inh_objdata;
  }
 else
  {
   for(obj = objdata->inherits; obj->next_inherited != NULL;
       obj = obj->next_inherited) {}
   obj->next_inherited = inh_objdata;
  }
 inh_objdata->ref_ct++;
 k_dismiss();
}

/* ======================================================================
   op_object()  -  Create an object                                       */

void op_object()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Arguments                  |  Object reference           |
     |-----------------------------|-----------------------------|
     |  Class catalogue name       |                             |
     |=============================|=============================|
 */

 DESCRIPTOR * descr;
 short int num_args;
 char call_name[MAX_PROGRAM_NAME_LEN+1];
 OBJDATA * objdata;
 u_char * object_code;

 num_args = *(pc++);    /* User defined arguments - Currently always 0 */

 descr = e_stack - (num_args + 1);
 while(descr->type == ADDR) descr = descr->data.d_addr;

 if (k_get_c_string(descr, call_name, MAX_PROGRAM_NAME_LEN) <= 0)
  {
   k_illegal_call_name();
  }
 UpperCaseString(call_name);

 if (!valid_call_name(call_name)) k_illegal_call_name();

 object_code = (u_char *)load_object(call_name, FALSE);
 if (object_code == NULL) k_error(sysmsg(1134), call_name);

  if (!(((OBJECT_HEADER *)object_code)->flags & HDR_IS_CLASS))
   {
    k_error(sysmsg(3451), call_name); /* %s is not a CLASS module */
   }

 /* Create OBJDATA structure */

 objdata = create_objdata(object_code);
 ((OBJECT_HEADER *)object_code)->ext_hdr.prog.refs++;

 /* Replace the e-stack entry for the class catalogue call name with an
    OBJ descriptor that references this object.                         */

 descr = e_stack - (num_args + 1);  /* 0488 load_object() may move e-stack */
 k_release(descr);
 InitDescr(descr, OBJ);
 descr->data.objdata = objdata;

 /* Set "key" for instantiation */

 object_key = 0;
 
 /* Although we could run the instantiation as a normal subroutine by
    using k_call(), property routines are run as recursives and a single
    program module can only ever be entirely a recursive. Therefore, we
    must run the instantiation as a recursive too.                       */

 k_recurse_object(object_code, num_args, objdata);

 return;
}

/* ======================================================================
   op_objinfo()  -  Return information about object                       */

void op_objinfo()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Key                        |  Returned information       |
     |=============================|=============================|
     |  Addr to object variable    |                             |
     |=============================|=============================|
 */

 DESCRIPTOR * descr;
 int key;
 DESCRIPTOR result;
 OBJECT_HEADER * objprog;

 descr = e_stack - 1;
 GetInt(descr);
 key = descr->data.value;

 descr = e_stack - 2;
 while(descr->type == ADDR) descr = descr->data.d_addr;

 InitDescr(&result, INTEGER);
 if (key == OI_ISOBJ)
  {
   result.data.value = descr->type == OBJ;
  }
 else
  {
   if (descr->type != OBJ) k_error("Variable is not an object");

   switch(key)
    {
     case OI_CLASS:
        objprog = (OBJECT_HEADER *)(descr->data.objdata->objprog);
        k_put_c_string(ProgramName(objprog), &result);
        break;

     default:
        result.data.value = 0;
        break;
    }
  }

 k_pop(1);
 k_dismiss();
 *(e_stack++) = result;
}

/* ======================================================================
   op_objmap()  -  OBJMAP: Create object name map                         */

void op_objmap()
{
 OBJDATA * objdata;
 unsigned short int name_map_len;
 objdata = process.program.objdata;

 name_map_len = *pc | (((unsigned short int)*(pc + 1)) << 8);
 pc += 2;

 objdata->name_map = (name_map_len != 0)?((OBJECT_NAME_MAP *)pc):NULL;
 pc += name_map_len;
}

/* ======================================================================
   op_objref()  -  OBJREF object data reference                           */

void op_objref()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Property name              |  Reference                  |
     |-----------------------------|-----------------------------|
     |  Object variable            |                             |
     |=============================|=============================|
 */

 u_char mode;  /* 0 = SET, 1 = GET */
 DESCRIPTOR * descr;
 char name[64+1];
 OBJDATA * objdata;

 mode = *(pc++);

 /* Get name */

 descr = e_stack - 1;
 k_get_c_string(descr, name, 64);
 UpperCaseString(name);
 k_dismiss();


 /* Get object */

 descr = e_stack - 1;
 while(descr->type == ADDR) descr = descr->data.d_addr;
 if (descr->type != OBJ) k_error("Invalid object reference");
 objdata = descr->data.objdata;

 /* Scan name table */

 if (!find_name_map_entry(mode, objdata, name, TRUE))
  {
   /* If there is an UNDEFINED handler, set up an OBJCDX descriptor to
      reference this.                                                  */

   if (!find_undefined_name_handler(mode, objdata, name, TRUE))
    {
     k_error("Unrecognised property/method name (%s)", name);
    }
  }

 return;
}

/* ======================================================================
   create_objdata()                                                       */

OBJDATA * create_objdata(u_char * obj)
{
 OBJDATA * objdata;

 objdata = (OBJDATA *)k_alloc(112, sizeof(OBJDATA));
 objdata->ref_ct = 1;
 objdata->objprog = obj;
 objdata->name_map = NULL;
 objdata->obj_vars = NULL;
 objdata->inherits = NULL;
 objdata->next_inherited = NULL;
 objdata->destructor_run = FALSE;
 return objdata;
}

/* ======================================================================
   find_name_map_entry()                                                  */

Private bool find_name_map_entry(short int mode, OBJDATA * objdata, char * name, bool dismiss_obj)
{
 OBJDATA * scanobj;
 OBJDATA * nextobj;
 OBJECT_NAME_MAP * p;
 short int var;
 short int key;
 u_char arg_ct;
 DESCRIPTOR * var_descr;

 scanobj = objdata;
 nextobj = objdata->inherits;

 do {
     for (p = scanobj->name_map; p != NULL; p = NextNameMapEntry(p))
      {
       if (!strcmp(p->name, name))  /* Found it */
        {
         var = PublicVar(p);

         if (mode == 0)    /* PUT / Method subroutine */
          {
           key = SetKey(p);
           arg_ct = p->set_arg_ct;
          }
         else              /* GET / Method function */
          {
           key = GetKey(p);
           arg_ct = p->get_arg_ct;
          }

         if (key == 0)  /* Bind to public variable */
          {
           if (var == 0) k_error("Illegal reference to property (%s)", name);

           if (var < 0)
            {
             if (mode == 0) return FALSE;  /* Hide read-only if PUT */
             var = -var;
            }

           var_descr = Element(scanobj->obj_vars, var);
           if (dismiss_obj) k_dismiss();
           InitDescr(e_stack, ADDR);
           (e_stack++)->data.d_addr = var_descr;
          }
         else            /* Create OBJCODE reference */
          {
           if (dismiss_obj) k_dismiss();
           InitDescr(e_stack, OBJCD);
           e_stack->data.objcode.objdata = scanobj;
           e_stack->data.objcode.key = key;
           e_stack->data.objcode.arg_ct = arg_ct;
           e_stack++;
          }

         return TRUE;
        }
      }

     /* If this object has an inheritance chain, scan that before going
        on down our own inheritance chain.                              */

     if (scanobj->inherits != NULL)
      {
       if (find_name_map_entry(mode, scanobj->inherits, name, dismiss_obj))
        {
         return TRUE;
        }
      }

     if (nextobj == NULL) return FALSE;
     scanobj = nextobj;
     nextobj = scanobj->next_inherited;
    } while(1);
}

/* ======================================================================
   find_undefined_name_handler()                                          */

Private bool find_undefined_name_handler(short int mode, OBJDATA * objdata, char * name, bool dismiss_obj)
{
 OBJDATA * scanobj;
 OBJDATA * nextobj;
 OBJECT_NAME_MAP * p;
 short int key;
 u_char arg_ct;

 scanobj = objdata;
 nextobj = objdata->inherits;

 do {
     for (p = scanobj->name_map; p != NULL; p = NextNameMapEntry(p))
      {
       if (!strcmp(p->name, "UNDEFINED"))  /* Found it */
        {
         if (mode == 0)    /* PUT / Method subroutine */
          {
           key = SetKey(p);
           arg_ct = p->set_arg_ct;
          }
         else              /* GET / Method function */
          {
           key = GetKey(p);
           arg_ct = p->get_arg_ct;
          }

         if (key != 0)   /* Create OBJCODE reference */
          {
           if (dismiss_obj) k_dismiss();
           InitDescr(e_stack, OBJCDX);
           e_stack->n1 = key;
           e_stack->data.objundef.objdata = scanobj;
           e_stack->data.objundef.undefined_name = (char *)k_alloc(113, strlen(name)+1);
           strcpy(e_stack->data.objundef.undefined_name, name);
           e_stack++;
           return TRUE;
          }
        }
      }

     /* If this object has an inheritance chain, scan that before going
        on down our own inheritance chain.                              */

     if (scanobj->inherits != NULL)
      {
       if (find_undefined_name_handler(mode, scanobj->inherits, name, dismiss_obj))
        {
         return TRUE;
        }
      }

     if (nextobj == NULL) return FALSE;
     scanobj = nextobj;
     nextobj = scanobj->next_inherited;
    } while(1);
}

/* ======================================================================
   op_get()  -  Resolve a property retrieval reference                    */

void op_get()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Args (variable number)     | Resolved value              |
     |-----------------------------|-----------------------------|
     |  OBJCD                      |                             |
     |=============================|=============================|

 The opcode is followed by a single byte containing the argument count.

 If the caller has referenced a public property with no Get routine, the
 OBJCD descriptor will be an ADDR to that property variable.             */

 DESCRIPTOR * descr;
 short int arg_ct;
 short int obj_arg_ct;
 int stack_depth;
 short int i;
 DESCRIPTOR * p;

 arg_ct = *(pc++);

 descr = e_stack - (arg_ct + 1);
 switch(descr->type)
  {
   case OBJCD:      /* Property has a Get routine */
      obj_arg_ct = descr->data.objcode.arg_ct;
      if ((obj_arg_ct & 0x80)?(arg_ct > (obj_arg_ct & 0x7F)):(arg_ct != obj_arg_ct))
       {
        k_error(sysmsg(1026)); /* Argument count mismatch in object reference */
       }

      object_key = descr->data.objcode.key;

      k_call("", arg_ct, (u_char *)(descr->data.objcode.objdata->objprog), 1);
      process.program.objdata = descr->data.objcode.objdata;
      k_dismiss();
      stack_depth = e_stack - e_stack_base;   /* 0489 */
      k_run_program(); /* Execute program */

      /* Ensure that we reassess trace mode in the lower layer */
      if (k_exit_cause == K_EXIT_RECURSIVE) k_exit_cause = K_TOGGLE_TRACER;

     if (e_stack - e_stack_base != stack_depth + 1)   /* 0489 */
       {
        k_error("Object did not return a property value");
       }
      break;

   case OBJCDX:     /* Calling undefined name handler */
      if (arg_ct >= ((OBJECT_HEADER *)(descr->data.objundef.objdata->objprog))->args)
       {
        k_error(sysmsg(1026)); /* Argument count mismatch in object reference */
       }

      object_key = descr->n1;

      /* We need to insert a new first argument */
      for (i = 0, p = e_stack; i < arg_ct; i++, p--) *p = *(p-1);
      e_stack++;
      k_put_c_string(descr->data.objundef.undefined_name, p);

      k_call("", arg_ct + 1, (u_char *)(descr->data.objcode.objdata->objprog), 1);
      process.program.objdata = descr->data.objcode.objdata;
      k_dismiss();
      stack_depth = e_stack - e_stack_base;   /* 0489 */
      k_run_program(); /* Execute program */

      /* Ensure that we reassess trace mode in the lower layer */
      if (k_exit_cause == K_EXIT_RECURSIVE) k_exit_cause = K_TOGGLE_TRACER;

      if (e_stack - e_stack_base != stack_depth + 1)   /* 0489 */
       {
        k_error("Object did not return a property value");
       }
      break;

   default:   
      p = descr;
      while(p->type == ADDR) p = p->data.d_addr;

      switch(p->type)
       {
        case ARRAY:
           i = (p->data.ahdr_addr->cols != 0) + 1;
           if (arg_ct != i) k_error(sysmsg(1026)); /* Argument count mismatch in object reference */

           /* Revise ADDR to point to array element */
           descr->data.d_addr = resolve_index(p, e_stack - 1, arg_ct);
           for (i = 0; i < arg_ct; i++) k_dismiss();
           break;

        default:
           if (arg_ct != 0) k_error(sysmsg(1026)); /* Argument count mismatch in object reference */
       }

      k_get_value(descr);
      break;
  }
}

/* ======================================================================
   op_set()  -  Save a property value                                     */

void op_set()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  Args (variable number)     | Resolved value              |
     |-----------------------------|-----------------------------|
     |  OBJCD                      |                             |
     |=============================|=============================|

 The opcode is followed by a single byte containing the argument count.
 There is always at least one argument (the value to save).

 If the caller has referenced a public property with no SET routine, the
 OBJCD descriptor will be an ADDR to that property variable.             */

 DESCRIPTOR * descr;
 short int arg_ct;
 short int obj_arg_ct;
 short int i;
 DESCRIPTOR * p;


 arg_ct = *(pc++);

 descr = e_stack - (arg_ct + 1);
 switch(descr->type)
  {
   case OBJCD:      /* Property has a SET routine */
      obj_arg_ct = descr->data.objcode.arg_ct;
      if ((obj_arg_ct & 0x80)?(arg_ct > (obj_arg_ct & 0x7F)):(arg_ct != obj_arg_ct))
       {
        k_error(sysmsg(1026)); /* Argument count mismatch in object reference */
       }

      object_key = descr->data.objcode.key;

      k_call("", arg_ct, (u_char *)(descr->data.objcode.objdata->objprog), 1);
      process.program.objdata = descr->data.objcode.objdata;
      k_dismiss();
      k_run_program(); /* Execute program */

      /* Ensure that we reassess trace mode in the lower layer */
      if (k_exit_cause == K_EXIT_RECURSIVE) k_exit_cause = K_TOGGLE_TRACER;
      break;

   case OBJCDX:     /* Calling undefined name handler */
      if (arg_ct >= ((OBJECT_HEADER *)(descr->data.objundef.objdata->objprog))->args)
       {
        k_error(sysmsg(1026)); /* Argument count mismatch in object reference */
       }

      object_key = descr->n1;

      /* We need to insert a new first argument */
      for (i = 0, p = e_stack; i < arg_ct; i++, p--) *p = *(p-1);
      e_stack++;
      k_put_c_string(descr->data.objundef.undefined_name, p);

      k_call("", arg_ct + 1, (u_char *)(descr->data.objcode.objdata->objprog), 1);
      process.program.objdata = descr->data.objcode.objdata;
      k_dismiss();
      k_run_program(); /* Execute program */

      /* Ensure that we reassess trace mode in the lower layer */
      if (k_exit_cause == K_EXIT_RECURSIVE) k_exit_cause = K_TOGGLE_TRACER;
      break;

   default:          /* Property is a public variable */
      while(descr->type == ADDR) descr = descr->data.d_addr;

      switch(descr->type)
       {
        case ARRAY:
           i = (descr->data.ahdr_addr->cols != 0) + 2;
           if (arg_ct != i) k_error(sysmsg(1026)); /* Argument count mismatch in object reference */

           descr = resolve_index(descr, e_stack - 2, arg_ct - 1);
           break;

        default:
           if (arg_ct != 1) k_error(sysmsg(1026)); /* Argument count mismatch in object reference */
       }

      k_release(descr);                            /* Release old value and... */
      *descr = *(--e_stack);                       /* ...Move new value */
      for (i = 0; i < arg_ct; i++) k_dismiss();
      break;
  }
}

/* ======================================================================
   free_object()  -  Decrement reference count and possibly free OBJDATA  */

void free_object(OBJDATA * objdata)
{
 OBJECT_NAME_MAP * p;
 short int key;
 OBJDATA * inh;
 ARRAY_HEADER * ahdr;

 if (objdata->ref_ct == 1)
  {
   /* Free inherited objects
      Remove objects one by one from the head of the inheritance chain,
      updating the chain to point to the next inherited object so that
      we can survive errors during the disinheritance process. The worst
      case should be that an abort (which always represents an
      application error) might leave an object stranded in memory with
      no references to it.                                               */

   while((inh = objdata->inherits) != NULL)
    {
     objdata->inherits = inh->next_inherited;
     free_object(inh);
    }

   /* Scan the name map looking for a DESTROY.OBJECT item */

   if (!(objdata->destructor_run))
    {
     for (p = objdata->name_map; p != NULL; p = NextNameMapEntry(p))
      {
       if (!strcmp(p->name, "DESTROY.OBJECT"))  /* Found it */
        {
         /* Mark the object destructor routine as having been run so that
            an abort in the destructor does not cause us to run it again. */

         objdata->destructor_run = TRUE;

         key = SetKey(p);
         if ((key != 0) && (PublicVar(p) == 0) && (p->set_arg_ct == 0))
          {
           object_key = key;
           k_recurse_object((u_char *)(objdata->objprog), 0, objdata);
          }
         break;
        }
      }
    }

   --(((OBJECT_HEADER *)(objdata->objprog))->ext_hdr.prog.refs);

   ahdr = objdata->obj_vars;
   if (--(ahdr->ref_ct) == 0) free_array(ahdr);

   k_free(objdata);
  }
 else (objdata->ref_ct)--;
}

/* ====================================================================== */

Private DESCRIPTOR * resolve_index(
   DESCRIPTOR * array_descr,   /* The matrix being accessed */
   DESCRIPTOR * index_descr,   /* The top index on the e-stack */
   short int dims)             /* Expected dimensionality */
{
 ARRAY_HEADER * ahdr;
 long int rows;
 long int cols;
 long int row;
 long int col;
 long int indx;


 ahdr = array_descr->data.ahdr_addr;
 rows = ahdr->rows;
 cols = ahdr->cols;

 if (cols == 0)   /* One dimensional matrix */
  {
   GetInt(index_descr);

   indx = index_descr->data.value;
   if ((indx < 0) || (indx > ahdr->rows)
       || (indx == 0 && (ahdr->flags & AH_PICK_STYLE)))
    {
     k_index_error(array_descr);
    }
  }
 else             /* Two dimensional matrix */
  {
   GetInt(index_descr);
   col = index_descr->data.value;

   index_descr--;
   GetInt(index_descr);
   row = index_descr->data.value;

   if ((row == 0) || (col == 0))
    {
     if (ahdr->flags & AH_PICK_STYLE) k_index_error(array_descr);
     indx = 0;
    }
   else
    {
     if ((row <= 0) || (row > rows) || (col <= 0) || (col > cols))
      {
       k_index_error(array_descr);
      }
     indx = ((row - 1) * cols) + col;
    }
  }

 return Element(ahdr, indx);
}


/* END-CODE */
