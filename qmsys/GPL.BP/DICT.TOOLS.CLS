      ********************************************************************************
      *
      * dictionary+data item access tools
      *
      * 23 Apr 09 gcb - Rewrote methods to cache dictionary data and item data
      * 16 Apr 09 gcb - Initial version
      *
      ********************************************************************************
      *
      * Instantiate a new dictionary tools object:
      *  myobj = object("dict.tools.cls")
      *
      * Set Filename:
      *  myobj->set.file.name("orders")
      *
      * Set Item ID:
      *  myobj->set.item.id("1000")
      *
      * Get a count of dictionary items:
      *  dictionary.count = myobj->dictionary.count
      *
      * Get @FM delimited list of dictionary items:
      *  dictionary.names = myobj->dictionary.list
      *
      * Get @FM delimited and sorted list of dictionary items:
      *  dictionary.names = myobj->sorted.dictionary.list
      *
      * Get a count of items in "filename":
      *  item.count = myobj->item.count
      *
      * Get @FM delimited list of item IDs:
      *  item.list = myojb->sort.items("by.dsnd order.number with customer.number = "123456")
      *     //The sort string can also be "" for default ascending item id sort//
      *
      * Get data for an item, based on a dictionary entry:
      *  mydata = myobj->customer.number
      *     //You must assign an item ID using the "set.item.id" set method before getting
      *       data for a specific field from "filename"//
      *
      * Get an error from the last object operation - returns null if no error:
      *  errormsg = myobj->get.last.error
      *
      ********************************************************************************
      *
      * To Do List:
      *
      *  1) Add write functionality, based on dictionary. I think this should be for
      *     D-type dictionaries only. Example:  myobj->write(mydata,"mydictionary")
      *
      *  2) Add itemid and filename validation before any read/write operations are
      *     about to be done. Return nulls with last.error message appropriately.
      *
      ********************************************************************************

      class dict.tools.cls
      $catalog global

      *
      * object memory-resident variables
      *
      *   item and file variables
      *
      private item.id, filename, item.list, item.list.count, field.data, item.data
      *
      *   dictionary variables
      *
      private dictionary.name, dictionary.list, dictionary.list.count, dictionaries(1), internal.dictionary.list
      *
      *   error and miscellaneous variables
      *
      private last.error

      *
      * object creation step
      * assign all of the memory-resident variables and null them
      *
      public subroutine create.object
      item.id = ''
      filename = ''
      item.list = ''
      item.list.count = ''
      field.data = ''
      item.data = ''
      dictionary.name = ''
      dictionary.list = ''
      dictionary.list.count = ''
      dim dictionaries(1)
      dictionaries(1) = ''
      internal.dictionary.list = ''
      last.error = ''
      return
      end

      *
      * set filename to get dictionary info
      *
      set set.file.name(filen)
      if filename # filen then
         filename = filen
         *
         * check that it's a valid file
         *
         open filename to file.f else
            dim dictionaries(1)
            dictionaries(1) = ''
            last.error = 'Unable to open ':filename
            filename = ''
            return
         end
         close file.f
         *
         * read the dictionary contents into a dimensioned array
         * fill a matching dynamic array of ids so we can locate
         * dict names easily
         *
         open 'DICT ':filename to dict.f else
            last.error = 'Unable to open DICT ':filename
            return
         end
         dict.ptr = 0
         eof = 0
         select dict.f
         loop until eof = 1 do
            readnext id else exit
            read dictrec from dict.f, id else
               field.data = ''
               last.error = 'Unable to read ':id:' from DICT ':filename
               return
            end
            dict.ptr = dict.ptr + 1
            dim dictionaries(dict.ptr)
            if inmat() then
               last.error = 'Unable to add dictionary to memory'
               dim dictionaries(1)
               dictionaries(1) = ''
               internal.dictionary.list = ''
               close dict.f
               return
            end
            dictionaries(dict.ptr) = dictrec
            internal.dictionary.list<dict.ptr> = id
         repeat
         dictionary.list.count = dict.ptr
         close dict.f
      end
      last.error = ''
      return
      end

      *
      * set itemname to get field data for
      *
      set set.item.id(itemvar)
      if itemvar # item.id then
         if filename = "" then
            last.error = "No filename assigned"
            item.id = ""
            return
         end
         item.id = itemvar
         open filename to file.f else
            item.data = ''
            dim dictionaries(1)
            dictionaries(1) = ''
            last.error = 'Unable to open ':filename
            filename = ''
            return
         end
         read item.data from file.f, item.id else
            item.data = ''
            last.error = 'Unable to read item ':item.id:' from ':filename
            return
         end
         close file.f
      end
      last.error = ''
      return
      end
      *
      * return unsorted dictionary list
      *
      get dictionary.list
      return internal.dictionary.list
      end
      *
      * get sorted/selected dictionary name list
      *
      public function sorted.dictionary.list
      last.error = ''
      output = ''
      execute 'sselect dict ':filename:' by f2' capturing output readlist dictionary.list else
         dictionary.list = ''
      end
      return dictionary.list
      end

      *
      * get dictionary count
      *
      get dictionary.count
      return dictionary.list.count
      end

      *
      * sort items into item list
      *
      public function sort.items(sort.criteria)
      last.error = ''
      if sort.criteria = '' then
         execute 'sselect ':filename capturing output readlist item.list else
            item.list = ''
         end
      end else
         execute 'sselect ':filename:' ':sort.criteria capturing output readlist item.list else
            item.list = ''
         end
      end
      return item.list
      end

      *
      * get item list count
      *
      public function item.count
      last.error = ''
      item.list.count = dcount(item.list,char(254))
      return item.list.count
      end

      *
      * get data for a field referenced by a dictionary name
      *
      public function undefined(dictionary.name)
      locate dictionary.name in internal.dictionary.list<0> setting dict.ptr then
         dictrec = dictionaries(dict.ptr)
         last.error = ''
         field.data = ''
         dict.type = upcase(dictrec<1>)
         * d-type
         if dict.type = "D" then
            attrib = dictrec<2>
            conversion = dictrec<3>
            if dictrec<4> # "" then
               realname = dictrec<4>
            end else
               realname = dictionary.name
            end
            format = dictrec<5>
            attrtype = dictrec<6>
            groupname = dictrec<7>
            field.data = item.data<attrib>
            if conversion # "" then
               cnvcnt = dcount(conversion,@VM)
               for cnvptr = 1 to cnvcnt
                  field.data = oconv(field.data,conversion<cnvptr>)
               next cnvptr
            end
            if format # "" then
               field.data = field.data:format
            end
         end
         * i-type
         if dict.type = "I" then
            *         expression = dictrec<2>
            *         conversion = dictrec<3>
            *         if dictrec<4> # "" then
            *            realname = dictrec<4>
            *         end else
            *            realname = dictionary.name
            *         end
            *         format = dictrec<5>
            *         attrtype = dictrec<6>
            *         groupname = dictrec<7>
            field.data = itype(dictrec)
         end
      end else
      end
      return field.data
      end

      *
      * Get last error message
      *
      public function get.last.error()
      return last.error
      end

      *
      * end of dictinfo.cls class
      *
      end
