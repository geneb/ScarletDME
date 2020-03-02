/* QM.H
 * Common include file for all modules
 * Copyright (c) 2005 Ladybridge Systems, All Rights Reserved
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
 * 27Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 * 
 * START-HISTORY (OpenQM):
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 19 Jun 07  2.5-7 k_call() now has additional argument for stack adjustment.
 * 05 Feb 07  2.4-20 Added transparent_newline argument to tio_display_string().
 * 31 Aug 05  2.2-9 Reverted MAX_PATHNAME_LEN back to 256 on all platforms as
 *                  using PATH_MAX on Linux made file table enormous.
 * 09 Jun 05  2.2-1 Added truncate argument to ftoa().
 * 16 May 05  2.1-15 Use Seek() macro instead of lseek/lseek64 for portability.
 * 13 May 05  2.1-15 Added error arg to s_make_contiguous().
 * 21 Oct 04  2.0-7 write_console() now returns bool.
 * 28 Sep 04  2.0-3 Replaced GetSysPaths() with GetConfigPath().
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

#ifndef __QM
#define __QM

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <limits.h>
   #include <fcntl.h>
   #include <dirent.h>
   #include <errno.h>
   #include <sys/types.h>
   #include <sys/stat.h>

#include "qmdefs.h"


/* Select list limits */

#define HIGH_SELECT          12
#define HIGH_USER_SELECT     10
#define InvalidSelectListNo(list_no) \
 ((list_no < 0) \
  || (list_no > HIGH_SELECT) \
  || ((list_no > HIGH_USER_SELECT) && !(process.program.flags & HDR_INTERNAL)))
#define SelectList(n) \
 Element(Element(process.syscom, SYSCOM_SELECT_LIST)->data.ahdr_addr, (n))
#define SelectCount(n) \
 Element(Element(process.syscom, SYSCOM_SELECT_COUNT)->data.ahdr_addr, (n))



/* Memory management */

   #define k_alloc(tag, n) malloc(n)
   #define k_free(p) free(p)

#define k_free_ptr(p) if (p != NULL) k_free(p), p = NULL

/* Typedefs */

typedef struct DESCRIPTOR DESCRIPTOR;
typedef struct STRING_CHUNK STRING_CHUNK;
typedef struct FILE_VAR FILE_VAR;
typedef struct ARRAY_CHUNK ARRAY_CHUNK;
typedef struct ARRAY_HEADER ARRAY_HEADER;
typedef struct SCREEN_IMAGE SCREEN_IMAGE;
typedef struct BTREE_HEADER BTREE_HEADER;
typedef struct PMATRIX_HEADER PMATRIX_HEADER;
typedef struct SOCKVAR SOCKVAR;
typedef struct OBJDATA OBJDATA;

/* ======================================================================
   Standard includes needed everywhere                                    */

#include "dh.h"
#include "sysseg.h"
#include "err.h"
#include "descr.h"
#include "kernel.h"
#include "qmsem.h"
#include "linuxlb.h"

/* ======================================================================
   Command line items                                                     */

Public int16_t command_options init(0);
#define CMD_APPLY_LICENCE    0x0001      /* -l option */
#define CMD_QUERY_ACCOUNT    0x0002      /* -a option */
#define CMD_INSTALL          0x0004      /* -i option */
#define CMD_QUIET            0x0008      /* -quiet option */
#define CMD_PERSONAL         0x0010      /* -ip option */
#define CMD_STDOUT           0x0020      /* -stdout option (Windows) */
#define CMD_FLASH            0x0040      /* -f option */

Public bool trace_option init(FALSE);    /* -t option */

/* ======================================================================
   Environment                                                            */

Public bool internal_mode
#ifdef INTERNAL
            init(TRUE);
#else
            init(FALSE);
#endif

/* Mapping for directory file record names */

Public char * df_restricted_chars init("*,=><%/+:;?\\\"");
Public char * df_substitute_chars init("ACEGLPSVXYZBQ");
/* Also, leading . and ~ become %d and %t */

/* Date formats */

Public bool european_dates init(FALSE);
Public char default_date_conversion[32+1] init("D");

#define MAX_NLS_CURRENCY 8
Public struct
{
 char currency[MAX_NLS_CURRENCY+1];
 char thousands;
 char decimal;
} national;

Public char * null_string init("");
Public char * CRLF init("\r\n");

/* ======================================================================
   Internal function definitions                                          */

/* QM.C */
void fatal(void);
void dump(u_char * addr, int32_t bytes);
void set_console_title(void);

/* ANALYSE.C */
int64 dir_filesize(FILE_VAR * fvar);

/* CONFIG.C */
   struct CONFIG * read_config(char * errmsg);

/* CONSOLE.C */
bool init_console(void);
void shut_console(void);
bool write_console(char * p, int bytes);
bool save_screen(SCREEN_IMAGE * scrn, int16_t x, int16_t y,
                 int16_t w, int16_t h);
void restore_screen(SCREEN_IMAGE * scrn, bool restore_cursor);
void set_codepage(void);

/* CTYPE.C */
void set_default_character_maps(void);
char * LowerCaseString(char * s);
int MemCompareNoCase(char * p, char * q, int16_t len);
char * memichr(char * s, char c, int n);
void memucpy(char * tgt, char * src, int16_t len);
int sort_compare(char * s1, char * s2, int16_t bytes, bool nocase);
int StringCompLenNoCase(char * p, char * q, int16_t len);
void UpperCaseMem(char * str, int16_t len);
char * UpperCaseString(char * s);

/* DH_FILE.C */
OSFILE dio_open(char * fn, int mode);
   #define DIO_NEW       1 /* Create new file, fail if exists */
   #define DIO_REPLACE   2 /* Create new file, overwriting if exists */
   #define DIO_READ      3 /* Open existing file, read only */
   #define DIO_UPDATE    4 /* Open existing file, read/write */
   #define DIO_OVERWRITE 5 /* Open file, creating if doesn't exist */

/* INIPATH.C */
bool GetConfigPath(char * inipath);

/* KERNEL.C */
int16_t assign_user_no(int16_t user_table_index);
bool init_kernel(void);
int16_t in_group(char * name);
char * account(void);
void kernel(void);
void k_recurse(u_char * code_ptr, int num_args);
void k_recurse_object(u_char * code_ptr, int num_args, OBJDATA * objdata);
void k_call(char * name, int num_args, u_char * code_ptr, int16_t stack_adj);
void k_return(void);
void k_run_program(void);
bool raise_event(int16_t event, int16_t user);
void process_events(void);
void show_stack(void);
void sigchld_handler(int signum);
void suspend_updates(void);
void ReleaseLicence(USER_ENTRY * uptr);

/* K_ERROR.C */
void k_data_type(DESCRIPTOR * descr);
void k_deadlock(void);
void k_dh_error(DESCRIPTOR * descr);
void k_div_zero_error(DESCRIPTOR * descr);
void k_div_zero_warning(DESCRIPTOR * descr);
void k_illegal_call_name(void);
void k_index_error(DESCRIPTOR * descr);
void k_not_array_error(DESCRIPTOR * descr);
void k_value_error(DESCRIPTOR * descr);
void k_non_numeric_error(DESCRIPTOR * descr);
void k_non_numeric_warning(DESCRIPTOR * descr);
void k_unassigned(DESCRIPTOR * descr);
void k_strnum_len(DESCRIPTOR * descr);
void k_not_socket(DESCRIPTOR * descr);
void k_null_id(DESCRIPTOR * descr);
void k_illegal_id(DESCRIPTOR * descr);
void k_inva_task_lock_error(void);
void k_nary_length_error(void);
void k_select_range_error(void);
void k_txn_error(void);
void k_unassigned_zero(DESCRIPTOR * descr);
void k_unassigned_null(DESCRIPTOR * descr);
void k_error(char msg[], ...);
void k_quit(void);
void k_err_pu(void);
int k_line_no(int32_t offset, u_char * xcbase);
char * k_var_name(DESCRIPTOR * descr);
void log_message(char * msg);
int log_printf(char * str, ...);
void log_permissions_error(FILE_VAR * fvar);

/* K_FUNCS.C */
bool k_blank_string(DESCRIPTOR * descr);
DESCRIPTOR * k_dereference(DESCRIPTOR * p);
void k_get_num(DESCRIPTOR * p);
void k_get_bool(DESCRIPTOR * p);
void k_get_int(DESCRIPTOR * p);
void k_get_int32(DESCRIPTOR * p);
void k_get_float(DESCRIPTOR * p);
void k_get_string(DESCRIPTOR * p);
void k_get_file(DESCRIPTOR * p);
void k_get_value(DESCRIPTOR * p);
void k_num_array1(void(op(void)));
void k_num_array2(void(op(void)), int16_t default_value);
void k_release(DESCRIPTOR * p);
void k_deref_image(SCREEN_IMAGE * image);
void k_deref_string(STRING_CHUNK * str);
int k_get_c_string(DESCRIPTOR * descr, char * s, int max_bytes);
void k_incr_refct(DESCRIPTOR * p);
void k_put_c_string(char * s, DESCRIPTOR * descr);
void k_put_string(char * s, int len, DESCRIPTOR * descr);
void k_num_to_str(DESCRIPTOR * p);
bool k_str_to_num(DESCRIPTOR * p);
STRING_CHUNK * k_copy_string(STRING_CHUNK * src);
bool k_is_num(DESCRIPTOR * p);
bool strdbl(char * p, double * value);
bool strnint(char * p, int16_t len, int32_t * value);
void k_unass_zero(DESCRIPTOR * original, DESCRIPTOR * dereferenced);
void k_unass_null(DESCRIPTOR * original, DESCRIPTOR * dereferenced);
u_int32_t GetUnsignedInt(DESCRIPTOR * descr);

#define k_pop(n) e_stack -= n
#define k_dismiss() k_release(--e_stack)


/* MESSAGES.C */
bool load_language(char * language_prefix);
char * sysmsg(int msg_no);

/* NETFILES.C */
int net_clearfile(FILE_VAR * fvar);
void net_close(FILE_VAR * fvar);
int net_delete(FILE_VAR * fvar, char * id, int16_t id_len, bool keep_lock);
STRING_CHUNK * net_fileinfo(FILE_VAR * fvar, int key);
int net_filelock(FILE_VAR * fvar, bool wait);
int net_fileunlock(FILE_VAR * fvar);
STRING_CHUNK * net_indices1(FILE_VAR * fvar);
STRING_CHUNK * net_indices2(FILE_VAR * fvar, char * index_name);
int net_lock(FILE_VAR * fvar, char * id, int16_t id_len, bool update, bool no_wait);
void net_mark_mapping(FILE_VAR * fvar, bool state);
bool net_open(char * server, char * remote_file, FILE_VAR * fvar);
int net_read(FILE_VAR * fvar, char * id, int16_t id_len, u_int16_t op_flags, STRING_CHUNK ** str);
int net_readv(FILE_VAR * fvar, char * id, int16_t id_len, int field_no, u_int16_t op_flags, STRING_CHUNK ** str);
int net_recordlocked(FILE_VAR * fvar, char * id, int16_t id_len);
int net_scanindex(FILE_VAR * fvar, char * index_name, int16_t list_no,
                  DESCRIPTOR * key_descr, bool right);
int net_select(FILE_VAR * fvar, STRING_CHUNK ** list, int32_t * count);
int net_selectindex(FILE_VAR * fvar, char * index_name, STRING_CHUNK ** str);
int net_selectindexv(FILE_VAR * fvar, char * index_name, char * value, STRING_CHUNK ** str);
int net_setindex(FILE_VAR * fvar, char * index_name, bool right);
int net_unlock(FILE_VAR * fvar, char * id, int16_t id_len);
int net_unlock_all(void);
int net_write(FILE_VAR * fvar, char * id, int16_t id_len, STRING_CHUNK * str, bool keep_lock);
STRING_CHUNK * get_qmnet_connections(void);

/* OBJECT.C */
void * load_object(char * name, bool abort_on_error);
void unload_object(void * obj_hdr);
void unload_all(void);
bool is_global(void * obj_hdr);
void invalidate_object(void);
void * find_object(int32_t id);
STRING_CHUNK * hsm_dump(void);
void hsm_enter(void);
void hsm_on(void);

/* OBJPROG.C */
OBJDATA * create_objdata(u_char * obj);
void free_object(OBJDATA * objdata);

/* OP_ARRAY.C */
ARRAY_HEADER * a_alloc(int32_t rows, int32_t cols, bool init_zero);
void free_common(ARRAY_HEADER * addr);
void free_array(ARRAY_HEADER * hdr_addr);
void delete_common(ARRAY_HEADER * ahdr);

/* OP_BTREE.C */
bool bt_get_string(DESCRIPTOR * descr, char ** s);
void btree_to_string(DESCRIPTOR * descr);
void free_btree_element(BTREE_ELEMENT * element, int16_t keys);

/* OP_DIO1.C */
void dio_close(FILE_VAR * fvar);
bool get_voc_file_reference(char * voc_name, bool get_dict_name,
                            char * path);
void flush_dh_cache(void);

/* OP_DIO2.C */
bool delete_path(char * path);
bool fullpath(char * path, char * name);
bool make_path(char * tgt);

/* OP_DIO3.C */
bool dir_write(FILE_VAR * fvar, char * mapped_id, STRING_CHUNK * str);
bool map_t1_id(char * id, int16_t id_len, char * mapped_id);
bool call_trigger(DESCRIPTOR * fvar_descr, int16_t mode, DESCRIPTOR * id_descr,
                  DESCRIPTOR * data_descr, bool on_error, bool updatable);

/* OP_DIO4.C */
bool dio_init(void);
void clear_select(int16_t list_no);
void complete_select(int16_t list_no);
void end_select(int16_t list_no);

/* OP_ICONV.C */
int32_t iconv_time_conversion(void);

/* OP_JUMPS.C */
bool valid_call_name(char * call_name);

/* OP_LOCK.C */
bool check_lock(FILE_VAR * fvar, char * id, int16_t id_len);
int16_t lock_record(FILE_VAR *, char * id, int16_t id_len, bool update,
                      u_int32_t txn_id, bool no_wait);
bool unlock_record(FILE_VAR *, char * id, int16_t id_len);
void unlock_txn(u_int32_t txn);
void clear_lock_wait(void);
void rebuild_llt(void);

/* OP_MISC.C */
void day_to_dmy(int32_t day_no, int16_t * day, int16_t * mon,
                int16_t * year, int16_t * julian);
char * day_to_ddmmmyyyy(int32_t day_no);

/* OP_OCONV.C */
int32_t length_conversion(char * p);
int oconv_d(int dt, char * code, char * tgt);

/* OP_SEQIO.C */
void close_seq(FILE_VAR * fvar);

/* OP_SKT.C */
void close_skt(SOCKVAR * skt);

/* OP_STR1.C */
void set_case(bool upcase);

/* OP_STR2.C */
bool find_item(STRING_CHUNK * str, int32_t field, int32_t value,
               int32_t subvalue, STRING_CHUNK ** chunk, int16_t * offset);
STRING_CHUNK * copy_string(STRING_CHUNK * tail, STRING_CHUNK ** head,
                      char * src, int16_t len, int16_t * chunk_size);

/* OP_STR4.C */
bool match_template(char * string, char * tmpl,
                    int16_t component, int16_t return_component);

/* TIME.C */
int32_t local_time(void);
int32_t qmtime(void);

/* OP_TIO.C */
void como_close(void);
void free_print_units(void);
bool settermtype(char * name);
void squeek(void);
bool tio_display_string(char * q, int16_t bytes, bool flush, bool transparent_newline);
bool tio_handle_break(void);
bool tio_init(void);
int tio_printf(char * tmpl, ...);
void tio_shut(void);
void tio_write(char * s);
void freescrn(SCREEN_IMAGE * image);
void break_key(void);
int16_t keyin(int timeout);
bool keyready(void);
bool write_socket(char * str, int bytes, bool flush);
bool to_outbuf(char * str, int bytes);
void lock_beep(void);

/* PDUMP.C */
void pdump(void);

/* QMLIB.C */
int ftoa(double f, int16_t dp, bool truncate, char * result);
int strdcount(char * s, char d);
void strrep(char * s, char oldstr, char newstr);

/* RECCACHE.C */
void dump_rec_cache(void);
void init_record_cache(void);
void cache_record(int16_t fno, int16_t id_len, char * id, STRING_CHUNK * head);
bool scan_record_cache(int16_t fno, int16_t id_len, char * id, 
                       STRING_CHUNK ** data);

/* SOCKIO.C */
bool start_connection(int sa);
void shut_connection(void);
bool read_socket(char * str, int bytes);
bool flush_outbuf(void);

/* STRINGS.C */
char * alloc_c_string(DESCRIPTOR * descr);
char * dupstring(char * str);
STRING_CHUNK * s_alloc(int32_t size, int16_t * actual_size);
STRING_CHUNK * s_make_contiguous(STRING_CHUNK * str_addr, int16_t * errnum);
void s_free(STRING_CHUNK * p);
void s_free_all(void);
void setqmstring(char ** strptr, DESCRIPTOR * descr);
void setstring(char ** strptr, char * string);
void ts_fill(char c, int32_t len);
void ts_init(STRING_CHUNK ** head, int32_t base_size);
void ts_new_chunk(void);
void ts_copy_byte(char c);
void ts_copy(char * src, int len);
void ts_copy_c_string(char * str);
int ts_printf(char * tmpl, ...);
int32_t ts_terminate(void);
void ts_stack(void);
void ts_unstack(void);

/* SYSSEG.C */
bool attach_shared_memory(void);
void unbind_sysseg(void);
bool start_qm(void);
bool stop_qm(void);


/* WIN.C / LINUXIO.C */
bool login_user(char * username, char * password);
int64 lseek64(OSFILE handle, int64 offset, int fromwhere);
int64 filelength64(OSFILE handle);
int chsize64(OSFILE handle, int64 posn);
int qmpoll(int fd, int timeout);

/* WINPORT.C / LNXPORT.C */

bool is_port(char * name);
int openport(char * name);
void closeport(int hPort);
int readport(int hPort, char * str, int16_t bytes);
bool writeport(int hPort, char * str, int16_t bytes);
#endif

/* END-CODE */
