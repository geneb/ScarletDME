* VFS.CLS
* Template VFS class module.
* Copyright (c) 2006 Ladybridge Systems, All Rights Reserved
*
* This is a template class module for the creation of a Virtual File System
* interface for QM. Users may freely copy and modify this module as
* required.
*
* Except where explicitly stated, all public functions return a status value
* that determines which clause of the corresponding QMBasic statement will be
* executed on return:
*
*    A value of zero executes the THEN clause.
*
*    A value greater than zero executes the ELSE clause, setting STATUS() to
*    this value. There is one exception to this. A value of ER$LCK for a
*    statement with a LOCKED clause will execute that clause.
*
*    A negative value executes the ON ERROR clause, setting STATUS() to the
*    absolute (positive) value of the returned status. If there is no ON ERROR
*    clause, this will cause the program to abort.


class myvfs
* $catalogue xxx global

* ======================================================================
* V$CLEARFILE
* Called on use of the QMBasic CLEARFILE statement.

   public function v$clearfile()
      return 0
   end

* ======================================================================
* V$CLOSE
* Called on closing the file, either explicitly using the QMBasic CLOSE
* statement or implicitly, for example, on discard of a local file variable.

   public subroutine v$close()
   end

* ======================================================================
* V$COMPLETE.SELECT
* Called by various QMBasic elements that require any partial select list
* to be completed.
* This function returns the remainder of the list as a field mark
* delimited dynamic array

   public function v$complete.select(list.no)
      return ''
   end

* ======================================================================
* V$CONTINUE.SELECT
* Called by the QMBasic READNEXT operation when attempting to read an
* entry from an exhausted select list. This function allows a VFS handler
* to build a select list in small pieces in much the same way as the DH
* file handling within the QM file system.
* This function returns the next part of the list as a field mark
* delimited dynamic array

   public function v$continue.select(list.no)
      return ''
   end

* ======================================================================
* V$DELETE
* Called on use of the QMBasic DELETE or DELETEU statements,
* The KEEP.LOCK argument will be zero for DELETE and non-zero for DELETEU.

   public function v$delete(id, keep.lock)
      return 0
   end

* ======================================================================
* V$END.SELECT
* Called by various QMBasic elements that terminate generation of a
* partial select list.

   public subroutine v$end.select()
   end

* ======================================================================
* V$FILEINFO
* Called on use of the QMBasic FILEINFO() function for all keys except
* FL$OPEN and FL$TYPE.LOCK statement. The KEY argument contains the action
* key used in the FILEINFO() call.
* This function returns the value to be returned by the FILEINFO() call.

   public function v$fileinfo(key)
      return 0
   end

* ======================================================================
* V$FILELOCK
* Called on use of the QMBasic FILELOCK statement. The WAIT argument will
* be non-zero if there is no LOCKED clause.

   public function v$filelock(wait)
      return 0
   end

* ======================================================================
* V$FILEUNLOCK
* Called on use of the QMBasic FILEUNLOCK statement or any other situation
* that implicitly releases a file lock.

   public subroutine v$fileunlock()
   end

* ======================================================================
* V$INDICES
* Called on use of the INDICES() function.
* This function has two variants, one with no arguments and one that takes
* the name of an index as its argument. The public subroutine below can
* distinguish the two cases by checking whether the AK.NAME argument is
* assigned.
* The function should return the string to be returned to the calling
* program.

   public function v$indices(ak.name)
      return ''
   end

* ======================================================================
* V$LOCK
* Called on use of the QMBasic RECORDLOCKL and RECORDLOCKU statements.
* UPDATE.LOCK is true for RECORDLOCKU, false for RECORDLOCKL.
* WAIT is true if there is no LOCKED clause.

   public function v$lock(id, update.lock, wait)
      return 0
   end

* ======================================================================
* V$OPEN
* Called on opening the file. The NAME argument contains the qualifying
* data in the VOC entry after the handler name (if any) and is typically
* used to identify the "file" to be opened by the VFS handler.

   public function v$open(name)
      return 0
   end

* ======================================================================
* V$READ
* Called by QMBasic operations that read a record.
* The record id is passed in via the ID argument.
* The FLAGS argument is bit significant and is formed from the following
* additive values:
*   2 The LOCKED clause is present.
*   4 Obtain a shared (READL) lock.    } At most one of these flags will be
*   8 Obtain an update (READU) lock.   } set. A simple READ sets neither.
* Other bits may be set and should be ignored.
* The record data should be returned via the STR argument.

   public function v$read(id, flags, str)
      str = '...your data...'
      return 0
   end

* ======================================================================
* V$READV
* Called by the QMBsaic READV, READVL and READVU statements.
* These operations are treated as a special case as it may be possible for
* the handler to optimise the processing to access only the requested data.
* The record id is passed in via the ID argument.
* The field number of the field to be read is passed via the FNO argument
* which may be zero to check simply whether the record exists.
* The FLAGS argument is bit significant and is formed from the following
* additive values:
*   2 The LOCKED clause is present.
*   4 Obtain a shared (READVL) lock.    } At most one of these flags will be
*   8 Obtain an update (READVU) lock.   } set. A simple READV sets neither.
* Other bits may be set and should be ignored.
* The record data should be returned via the STR argument.

   public function v$readv(id, fno, flags, str)
      str = '...your data...'
      return 0
   end

* ======================================================================
* V$SELECT
* Called by the QMBasic SELECT operation to build a select list.
* This function returns the list as a field mark delimited dynamic array

   public function v$select(list.no)
      return ''
   end

* ======================================================================
* V$UNLOCK
* Called by actions that unlock a record. The ID arguemnt identifies the
* record to be unlocked.

   public subroutine v$unlock(id)
   end

* ======================================================================
* V$UNLOCK.ALL
* Called by actions that unlock all records in this file.

   public subroutine v$unlock.all()
   end

* ======================================================================
* V$WRITE
* Called by QMBasic statements that write a record, including WRITEV
* which will pass in the updated record.
* The ID argument contains the record id.
* The KEEP.LOCK argument will be non-zero for WRITEU, WRITEVU and MATWRITEU.
* The data to be written is in the STR argument.

   public function v$write(id, keep.lock, str)
      return 0
   end

* ======================================================================
* CREATE.OBJECT
* Probably not of much use in a VFS handler, this will be called when the
* object is instantiated, followed immeidately by a call to the V$OPEN
* function. The CREATE.OBJECT subroutine may be removed if not required.

   public subroutine create.object
      display 'MYVFS2 instantiated'
   end

* ======================================================================
* DESTROY.OBJECT
* Also probably not of much use as this will be called immediately after
* the V$CLOSE subroutine. The DESTROY.OBJECT subroutine may be removed if
* not required.

   public subroutine destroy.object
      display 'Destroyed'
   end

* ======================================================================
* Undefined name handlers
* More for diagnostics than anything else.

   public subroutine undefined(name)
      display 'VFS: Undefined subroutine "' : name : '"'
   end

   public function undefined(name)
      display 'VFS: Undefined function "' : name : '"'
      return 0
   end
end

* END-CODE
