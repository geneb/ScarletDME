/* QMCLIENT.C
 * QM VB Client DLL
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
 * 19 Oct 07  2.6-5 Method used to recognise IP address in qmconnect() was
 *                  inadequate.
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 21 May 07  2.5-5 0553 QMReplace() handled negative value or subvalue position
 *                  wrongly.
 * 02 Nov 06  2.4-15 MATCHES template codes should be case insensitive.
 * 11 Aug 06  2.4-11 0513 Must cast BSTR to u_char to extract sort code.
 * 11 Aug 06  2.4-11 0512 Subvalue level QMLocate() started the scan at the
 *                   incorrect position.
 * 30 Jun 06  2.4-6 Revised subroutine name length limit in QMCall().
 * 04 May 06  2.4-4 Improved resilience to API calls before connection.
 *                  Use thread local storage.
 * 02 May 06  2.4-3 Use a separate buffer for each session to allow use in
 *                  multi-threaded applications.
 * 20 Mar 06  2.3-8 Added QMMarkMapping().
 * 05 Jan 06  2.3-3 Added QMSelectLeft(), QMSelectRight(), QMSetLeft() and
 *                  QMSetRight().
 * 08 Sep 05  2.2-10 Added QMLogto().
 * 22 Aug 05  2.2-8 Multiple session support.
 * 26 Jul 05  2.2-6 0380 QMIns() postmark logic was wrong.
 * 25 Jul 05  2.2-6 0379 QMLocate() was returning 2, not 1, for the insertion
 *                  position when inserting into a null item.
 * 18 Apr 05  2.1-12 Close process handle after process creation.
 * 01 Feb 05  2.1-5 Trap zero length host name as this crashes the socket
 *                  library.
 * 26 Oct 04  2.0-7 0272 Cannot use sizeof() for packet header transmission
 *                  size.
 * 26 Oct 04  2.0-7 Increased MAX_ID_LEN to absolute limit of current file
 *                  sysemt implementation.
 * 21 Oct 04  2.0-6 0267 QMSelectIndex() was sending the wrong length for the
 *                  indexed value string.
 * 30 Sep 04  2.0-3 Added QMRecordlock().
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 *
 *
 * Session Management
 * ==================
 * QMConnect()
 * QMConnected()
 * QMConnectLocal()
 * QMDisconnect()
 * QMDisconnectAll()
 * QMGetSession()
 * QMSetSession()
 * QMLogto()
 * 
 * 
 * File Handling
 * =============
 * QMOpen()
 * QMClose()
 * QMRead()
 * QMReadl()
 * QMReadu()
 * QMWrite()
 * QMWriteu()
 * QMDelete()
 * QMDeleteu()
 * QMRelease()
 * QMSelect()
 * QMSelectIndex()
 * QMSelectLeft()
 * QMSelectRight()
 * QMSetLeft()
 * QMSetRight()
 * QMReadNext()
 * QMClearSelect()
 * QMRecordlock()
 * QMMarkMapping()
 *
 *
 * Dynamic Array Handling
 * ======================
 * QMExtract()
 * QMReplace()
 * QMIns()
 * QMDel()
 * QMLocate()
 *
 *
 * String Handling
 * ===============
 * QMChange()
 * QMDcount()
 * QMField()
 * QMMatch()
 * QMMatchfield()
 *
 * Command Execution
 * =================
 * QMExecute()
 * QMRespond()
 * QMEndCommand()
 *
 * Subroutine Execution
 * ====================
 * QMCall()
 *
 *
 * Error Handling
 * ==============
 * QMError()
 * QMStatus()
 *
 *
 * Internal
 * ========
 * QMDebug()
 * QMEnterPackage()
 * QMExitPackage()
 *
 *
 * VB data type compatibility:
 *
 *  VB               C
 *  Bool ByVal       short int     (0 or -1)
 *  Bool ByRef       short int *   (0 or -1)
 *  Integer ByVal    short int
 *  Long ByVal       long int
 *  Integer          short int *
 *  Long             long int *
 *  String ByVal     BSTR
 *  String           BSTR *
 *  
 * END-DESCRIPTION
 *
 * START-CODE
 */

 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>
#include <winsock2.h>

#define Public
#include <qmdefs.h>
#include <revstamp.h>
#include <qmclient.h>
#include <err.h>
void set_default_character_maps(void);

#include <windows.h>
#include <stdarg.h>


/* Network data */
Private bool OpenSocket(struct SESSION * session, char * host, short int port);
Private bool CloseSocket(struct SESSION * session);
Private bool read_packet(struct SESSION * session);
Private bool write_packet(struct SESSION * session, int type, char * data, long int bytes);
Private void NetError(char * prefix);
Private void debug(unsigned char * p, int n);
Private struct SESSION * FindFreeSession(void);
Private void disconnect(struct SESSION * session);

typedef short int VBBool;
#define VBFalse 0
#define VBTrue  -1

typedef struct ARGDATA ARGDATA;
struct ARGDATA {
                short int argno;
                long int arglen;
                char text[1];
               };

/* Packet buffer */
#define BUFF_INCR      4096
typedef struct INBUFF INBUFF;
struct INBUFF {
               union {
                      struct {
                              char message[1];
                             } abort;
                      struct {                  /* QMCall returned arguments */
                              struct ARGDATA argdata;
                             } call;
                      struct {                  /* Error text retrieval */
                              char text[1];
                             } error;
                      struct {                  /* QMExecute */
                              char reply[1];
                             } execute;
                      struct {                  /* QMOpen */
                              short int fno;
                             } open;
                      struct {                  /* QMRead, QMReadl, QMReadu */
                              char rec[1];
                             } read;
                      struct {                  /* QMReadList */
                              char list[1];
                             } readlist;
                      struct {                  /* QMReadNext */
                              char id[1];
                             } readnext;
                      struct {                  /* QMSelectLeft, QMSelectRight */
                              char key[1];
                             } selectleft;
                     } data;
       };

FILE * srvr_debug = NULL;

#define MAX_SESSIONS 4
struct SESSION
{
 short int idx;                /* Session table index */
 bool is_local;
 short int context;
   #define CX_DISCONNECTED  0  /* No session active */
   #define CX_CONNECTED     1  /* Session active but not... */
   #define CX_EXECUTING     2  /* ...executing command (implies connected) */
   INBUFF * buff;
   long int buff_size;         /* Allocated size of buffer */
   long int buff_bytes;        /* Size of received packet */
 char qmerror[512+1];
 short int server_error;
 long int qm_status;
 SOCKET sock;
 HANDLE hPipe;
};

struct SESSION sessions[MAX_SESSIONS];

DWORD tls = -1;

#define CDebug(name) ClientDebug(name)


void _stdcall QMDisconnect(void);
void _stdcall _export QMEndCommand(void);

/* Internal functions */
BSTR SelectLeftRight(short int fno, BSTR index_name, short int listno, short int mode);
void SetLeftRight(short int fno, BSTR index_name, short int mode);
bool context_error(struct SESSION * session, short int expected);
void delete_record(short int mode, short int fno, BSTR id);
BSTR read_record(short int fno, BSTR id, short int * err, int mode);
void write_record(short int mode, short int fno, BSTR id, BSTR data);
bool GetResponse(struct SESSION * session);
void Abort(char * msg, bool use_response);
void catcall (struct SESSION * session, BSTR subrname, short int argc, ...);
char * memstr(char * str, char * substr, int str_len, int substr_len);
bool match_template(char * string, char * template, short int component,
                    short int return_component, char ** component_start,
                    char ** component_end);
bool message_pair(struct SESSION * session, int type, unsigned char * data, long int bytes);
char * sysdir(void);
void ClientDebug(char * name);

/* ======================================================================
   DllEntryPoint() -  Initialisation function                             */

BOOL WINAPI _export DllEntryPoint(instance, reason, reserved)
   HANDLE instance;
   DWORD reason;
   LPVOID reserved;
{
 short int i;

 switch(reason)
  {
   case DLL_PROCESS_ATTACH:
      if (tls == 0xFFFFFFFF)
       {
        tls = TlsAlloc();  /* Allocate thread storage for session index */

        for(i = 0; i < MAX_SESSIONS; i++)
         {
          sessions[i].idx = i;
          sessions[i].context = CX_DISCONNECTED;
          sessions[i].is_local = FALSE;
          sessions[i].buff = NULL;
          sessions[i].qmerror[0] = '\0';
          sessions[i].hPipe = INVALID_HANDLE_VALUE;
          sessions[i].sock = INVALID_SOCKET;
         }

        set_default_character_maps();
       }
      break;
  }

 return 1;
}

/* ======================================================================
   DllRegisterServer()                                                    */

int WINAPI _export DllRegisterServer()
{
 return S_OK;
}

/* ======================================================================
   QMCall()  - Call catalogued subroutine                                 */

void _stdcall _export QMCall(subrname, argc, a1, a2, a3, a4, a5, a6, a7,
                             a8, a9, a10, a11, a12, a13, a14, a15, a16,
                             a17, a18, a19, a20)
   BSTR subrname;
   short int argc;
   BSTR * a1;
   BSTR * a2;
   BSTR * a3;
   BSTR * a4;
   BSTR * a5;
   BSTR * a6;
   BSTR * a7;
   BSTR * a8;
   BSTR * a9;
   BSTR * a10;
   BSTR * a11;
   BSTR * a12;
   BSTR * a13;
   BSTR * a14;
   BSTR * a15;
   BSTR * a16;
   BSTR * a17;
   BSTR * a18;
   BSTR * a19;
   BSTR * a20;
{
 struct SESSION * session;

 CDebug("QMCall");
 session = TlsGetValue(tls);
 if (context_error(session, CX_CONNECTED)) return;

 catcall(session, subrname, argc, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11,
         a12, a13, a14, a15, a16, a17, a18, a19, a20);
}

/* ======================================================================
   QMChange()  -  Change substrings                                       */

BSTR _stdcall _export QMChange(src, old, new, occ, first)
   BSTR src;
   BSTR old;
   BSTR new;
   long int * occ;
   long int * first;
{
 int src_len;
 int old_len;
 int new_len;
 long int occurrences = -1;
 long int start = 1;
 long int bytes;         /* Remaining bytes counter */
 char * start_pos;
 BSTR new_str;
 long int changes;
 char * pos;
 char * p;
 char * q;
 long int n;


 src_len = SysStringByteLen(src);
 old_len = SysStringByteLen(old);
 new_len = SysStringByteLen(new);

 if (src_len == 0)
  {
   new_str = SysAllocStringByteLen(NULL, 0);
   return new_str;
  }

 if (old_len == 0) goto return_unchanged;

 if (occ != NULL)
  {
   occurrences = *occ;
   if (occurrences < 1) occurrences = -1;
  }

 if (first != NULL)
  {
   start = *first;
   if (start < 1) start = 1;
  }

 /* Count occurences of old string in source string, remembering start of
    region to change.                                                     */

 changes = 0;
 bytes = src_len;
 pos = (char *)src;
 while (bytes > 0)
  {
   p = memstr(pos, (char *)old, bytes, old_len);
   if (p == NULL) break;

   if (--start <= 0)
    {
     if (start == 0) start_pos = p;   /* This is the first one to replace */

     changes++;

     if (--occurrences == 0) break;  /* This was the last one to replace */
    }

   bytes -= (p - pos) + old_len;
   pos = p + old_len;
  }

 if (changes == 0) goto return_unchanged;

 /* Now make the changes */

 new_str = SysAllocStringByteLen(NULL, src_len + changes * (new_len - old_len));

 q = (char *)new_str;
 pos = (char *)src;
 bytes = src_len;
 p = start_pos;
 do {
     /* Copy intermediate text */

     n = p - pos;
     if (n)
      {
       memcpy(q, pos, n);
       q += n;
       pos += n;
       bytes -= n;
      }

     /* Insert replacement */

     if (new_len)
      {
       memcpy(q, new, new_len);
       q += new_len;
      }

     /* Skip old substring */

     pos += old_len;
     bytes -= old_len;

     /* Find next replacement */

     p = memstr(pos, (char *)old, bytes, old_len);

    } while(--changes);


 /* Copy trailing substring */

 if (bytes) memcpy(q, pos, bytes);
 return new_str;


return_unchanged:
 new_str = SysAllocStringByteLen((char *)src, src_len);
 return new_str;
}

/* ======================================================================
   QMClearSelect()  - Clear select list                                   */

void _stdcall _export QMClearSelect(listno)
   short int listno;
{
 struct {
         short int listno;
        } packet;
 struct SESSION * session;

 CDebug(("QMClearSelect"));
 session = TlsGetValue(tls);

 if (context_error(session, CX_CONNECTED)) goto exit_qmclearselect;

 packet.listno = listno;

 if (!message_pair(session, SrvrClearSelect, (char *)&packet, sizeof(packet)))
  {
   goto exit_qmclearselect;
  }

 switch(session->server_error)
  {
   case SV_ON_ERROR:
      Abort("CLEARSELECT generated an abort event", TRUE);
      break;
  }
 
exit_qmclearselect:
 return;
}

/* ======================================================================
   QMClose()  -  Close a file                                             */

void _stdcall _export QMClose(fno)
   short int fno;
{
 struct {
         short int fno;
        } packet;
 struct SESSION * session;

 CDebug("QMClose");
 session = TlsGetValue(tls);

 if (context_error(session, CX_CONNECTED)) goto exit_qmclose;

 packet.fno = fno;

 if (!message_pair(session, SrvrClose, (char *)&packet, sizeof(packet)))
  {
   goto exit_qmclose;
  }

 switch(session->server_error)
  {
   case SV_ON_ERROR:
      Abort("CLOSE generated an abort event", TRUE);
      break;
  }
 
exit_qmclose:
 return;
}

/* ======================================================================
   QMConnect()  -  Open connection to server.                             */

VBBool _stdcall _export QMConnect(host, port, username, password, account)
   BSTR host;
   short int port;
   BSTR username;
   BSTR password;
   BSTR account;
{
 VBBool status = VBFalse;
 char login_data[2 + MAX_USERNAME_LEN + 2 + MAX_USERNAME_LEN];
 int n;
 char * p;
 struct SESSION * session;


 if ((session = FindFreeSession()) == NULL) goto exit_qmconnect;
 session->qmerror[0] = '\0';
 session->is_local = FALSE;

 /* Set up login data */

 n = SysStringByteLen(host);
 if (n == 0)
  {
   strcpy(session->qmerror, "Invalid host name");
   goto exit_qmconnect;
  }

 n = SysStringByteLen(username);
 if (n > MAX_USERNAME_LEN)
  {
   strcpy(session->qmerror, "Invalid user name");
   goto exit_qmconnect;
  }

 p = login_data;
 *((short int *)p) = n;            /* User name len */
 p += 2;

 memcpy(p, (char *)username, n);   /* User name */
 p += n;
 if (n & 1) *(p++) = '\0';

 n = SysStringByteLen(password);
 if (n > MAX_USERNAME_LEN)
  {
   strcpy(session->qmerror, "Invalid password");
   goto exit_qmconnect;
  }

 *((short int *)p) = n;            /* Password len */
 p += 2;

 memcpy(p, (char *)password, n);   /* Password */
 p += n;
 if (n & 1) *(p++) = '\0';


 /* Open connection to server */

 if (!OpenSocket(session, (char *)host, port)) goto exit_qmconnect;

 /* Check username and password */

 n = p - login_data;
 if (!message_pair(session, SrvrLogin, login_data, n))
  {
   sprintf(session->qmerror, "Startup message returned an error");
   goto exit_qmconnect;
  }

 if (session->server_error != SV_OK)
  {
   if (session->server_error == SV_ON_ERROR)
    {
     n = session->buff_bytes - offsetof(INBUFF, data.abort.message);
     if (n > 0)
      {
       memcpy(session->qmerror, session->buff->data.abort.message, n);
       session->buff->data.abort.message[n] = '\0';
      }
    }
   goto exit_qmconnect;
  }

 /* Now attempt to attach to required account */

 if (!message_pair(session, SrvrAccount, (char *)account, SysStringByteLen(account)))
  {
   goto exit_qmconnect;
  }

 session->context = CX_CONNECTED;

 if (srvr_debug != NULL)
  {
   fprintf(srvr_debug, "Connected session %d\n", session->idx);
   fflush(srvr_debug);
  }

 status = VBTrue;

exit_qmconnect:
 if (!status) CloseSocket(session);

 return status;
}

/* ======================================================================
   QMConnected()  -  Are we connected?                                    */

VBBool _stdcall _export QMConnected()
{
 struct SESSION * session;

 session = TlsGetValue(tls);
 if (session == NULL) return VBFalse;

 session->qmerror[0] = '\0';

 return (session->context == CX_DISCONNECTED)?VBFalse:VBTrue;
}

/* ======================================================================
   QMConnectLocal()  -  Open connection to local system as current user   */

VBBool _stdcall _export QMConnectLocal(account)
   BSTR account;
{
 VBBool status = VBFalse;
 char command[MAX_PATHNAME_LEN + 50];
 char pipe_name[40];
 struct _PROCESS_INFORMATION process_information;
 struct _STARTUPINFOA startupinfo;
 struct SESSION * session;

 if ((session = FindFreeSession()) == NULL) goto exit_qmconnect_local;
 session->qmerror[0] = '\0';
 session->is_local = TRUE;

 /* Create pipe */

 sprintf(pipe_name, "\\\\.\\pipe\\~QMPipe%d-%d", GetCurrentProcessId(), session->idx);
 session->hPipe = CreateNamedPipe(pipe_name,
                         PIPE_ACCESS_DUPLEX,
                         PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                         1,                /* Max instances */
                         1024,             /* Output buffer size */
                         1024,             /* Input buffer size */
                         0,                /* Timeout */
                         NULL);            /* Security attributes */

 if (session->hPipe == INVALID_HANDLE_VALUE)
  {
   sprintf(session->qmerror, "Error %d creating pipe", GetLastError());
   goto exit_qmconnect_local;
  }

 /* Launch QM process */

 sprintf(command, "%s\\BIN\\QM.EXE -Q -C %s", sysdir(), pipe_name);

 GetStartupInfo(&startupinfo);

 if (CreateProcess(NULL, command, NULL, NULL, FALSE, DETACHED_PROCESS,
                   NULL, NULL, &startupinfo, &process_information))
  {
   CloseHandle(process_information.hThread);
  }
 else
  {
   sprintf(session->qmerror, "Failed to create child process. Error %d",
           (int)GetLastError());
   CloseHandle(session->hPipe);
   session->hPipe = INVALID_HANDLE_VALUE;
   goto exit_qmconnect_local;
  }

 /* Wait for child to connect to pipe */

 if (!ConnectNamedPipe(session->hPipe, NULL))
  {
   sprintf(session->qmerror, "Error %d connecting pipe", (int)GetLastError());
   CloseHandle(session->hPipe);
   session->hPipe = INVALID_HANDLE_VALUE;
   goto exit_qmconnect_local;
  }

 /* Send local login packet */

 if (!message_pair(session, SrvrLocalLogin, NULL, 0))
  {
   goto exit_qmconnect_local;
  }

 /* Now attempt to attach to required account */

 if (!message_pair(session, SrvrAccount, (char *)account, SysStringByteLen(account)))
  {
   goto exit_qmconnect_local;
  }

 session->context = CX_CONNECTED;

 if (srvr_debug != NULL)
  {
   fprintf(srvr_debug, "Connected session %d\n", session->idx);
   fflush(srvr_debug);
  }

 status = VBTrue;

exit_qmconnect_local:
 if (!status)
  {
   if (session->hPipe != INVALID_HANDLE_VALUE)
    {
     CloseHandle(session->hPipe);
     session->hPipe = INVALID_HANDLE_VALUE;
    }
  }

 return status;
}

/* ======================================================================
   QMDcount()  -  Count fields, values or subvalues                       */

long int _stdcall _export QMDcount(src, delim_str)
   BSTR src;
   BSTR delim_str;
{
 long int src_len;
 char * p;
 long int ct = 0;
 char delim;


 if (SysStringByteLen(delim_str) != 0)
  {
   delim = *delim_str;

   src_len = SysStringByteLen(src);
   if (src_len != 0)
    {
     ct = 1;
     while((p = memchr(src, delim, src_len)) != NULL)
      {
       src_len -= (1 + p - (char *)src);
       (char *)src = p + 1;
       ct++;
      }
    }
  }

 return ct;
}

/* ======================================================================
   QMDebug()  -  Turn on/off packet debugging                             */

void _stdcall _export QMDebug(mode)
   VBBool mode;
{
 if (mode && (srvr_debug == NULL))     /* Turn on */
  {
   srvr_debug = fopen("C:\\CLIDBG", "wt");
  }

 if (!mode && (srvr_debug != NULL))   /* Turn off */
  {
   fclose(srvr_debug);
   srvr_debug = NULL;
  }
}

/* ======================================================================
   QMDel()  -  Delete field, value or subvalue                            */

BSTR _stdcall _export QMDel(src, fno, vno, svno)
   BSTR src;
   short int fno;
   short int vno;
   short int svno;
{
 long int src_len;
 char * pos;             /* Rolling source pointer */
 long int bytes;         /* Remaining bytes counter */
 long int new_len;
 BSTR new_str;
 char * p;
 short int i;
 long int n;


 src_len = SysStringByteLen(src);
 if (src_len == 0) goto null_result;   /* Deleting from null string */


 /* Setp 1  -  Initialise varaibles */

 if (fno < 1) fno = 1;

 /* Step 2  -  Position to start of item */

 pos = (char *)src;
 bytes = src_len;

 /* Skip to start field */

 i = fno;
 while(--i)
  {
   p = memchr(pos, FIELD_MARK, bytes);
   if (p == NULL) goto unchanged_result;    /* No such field */
   bytes -= (1 + p - pos);
   pos = p + 1;
  }

 p = memchr(pos, FIELD_MARK, bytes);
 if (p != NULL) bytes = p - pos;    /* Length of field, including end mark */

 if (vno < 1)    /* Deleting whole field */
  {
   if (p == NULL)    /* Last field */
    {
     if (fno > 1) /* Not only field. Back up to include leading mark */
      {
       pos--;
       bytes++;
      }
    }
   else              /* Not last field */
    {
     bytes++;        /* Include trailing mark in deleted region */
    }
   goto done;
  }


 /* Skip to start value */

 i = vno;
 while(--i)
  {
   p = memchr(pos, VALUE_MARK, bytes);
   if (p == NULL) goto unchanged_result;    /* No such value */
   bytes -= (1 + p - pos);
   pos = p + 1;
  }

 p = memchr(pos, VALUE_MARK, bytes);
 if (p != NULL) bytes = p - pos;    /* Length of value, including end mark */

 if (svno < 1)     /* Deleting whole value */
  {
   if (p == NULL)    /* Last value */
    {
     if (vno > 1) /* Not only value. Back up to include leading mark */
      {
       pos--;
       bytes++;
      }
    }
   else              /* Not last value */
    {
     bytes++;        /* Include trailing mark in deleted region */
    }
   goto done;
  }

 /* Skip to start subvalue */

 i = svno;
 while(--i)
  {
   p = memchr(pos, SUBVALUE_MARK, bytes);
   if (p == NULL) goto unchanged_result;    /* No such subvalue */
   bytes -= (1 + p - pos);
   pos = p + 1;
  }
 p = memchr(pos, SUBVALUE_MARK, bytes);
 if (p != NULL) bytes = p - pos;    /* Length of subvalue, including end mark */

 if (p == NULL)    /* Last subvalue */
  {
   if (svno > 1) /* Not only subvalue. Back up to include leading mark */
    {
     pos--;
     bytes++;
    }
  }
 else              /* Not last subvalue */
  {
   bytes++;        /* Include trailing mark in deleted region */
  }

done:
 /* Now construct new string with 'bytes' bytes omitted starting at 'pos' */

 new_len = src_len - bytes;
 new_str = SysAllocStringByteLen(NULL, new_len);
 p = (char *)new_str;

 n = pos - (char *)src;    /* Length of leading substring */
 if (n)    
  {
   memcpy(p, src, n);
   p += n;
  }

 n = src_len - (bytes + n);    /* Length of trailing substring */
 if (n)
  {
   memcpy(p, pos + bytes, n);
  }

 return new_str;

null_result:
 return SysAllocStringByteLen(NULL, 0);

unchanged_result:
 return SysAllocStringByteLen((char *)src, src_len);
}

/* ======================================================================
   QMDelete()  -  Delete record                                           */

void _stdcall _export QMDelete(fno, id)
   short int fno;
   BSTR id;
{
 CDebug(("QMDelete"));
 delete_record(SrvrDelete, fno, id);
}

/* ======================================================================
   QMDeleteu()  -  Delete record, retaining lock                          */

void _stdcall _export QMDeleteu(fno, id)
   short int fno;
   BSTR id;
{
 CDebug("QMDeleteu");
 delete_record(SrvrDeleteu, fno, id);
}

/* ======================================================================
   QMDisconnect()  -  Close connection to server.                         */

void _stdcall _export QMDisconnect()
{
 struct SESSION * session;

 CDebug(("QMDisconnect"));
 session = TlsGetValue(tls);
 if (session != NULL)
  {
   if (session->context != CX_DISCONNECTED)
    {
     disconnect(session);
    }
  }
}

/* ======================================================================
   QMDisconnectAll()  -  Close connection to all servers.                 */

void _stdcall _export QMDisconnectAll()
{
 short int i;

 CDebug("QMDisconnectAll");
 for (i = 0; i < MAX_SESSIONS; i++)
  {
   if (sessions[i].context != CX_DISCONNECTED)
    {
     disconnect(&(sessions[i]));
    }
  }
}

/* ======================================================================
   QMEndCommand()  -  End a command that is requesting input              */

void _stdcall _export QMEndCommand()
{
 struct SESSION * session;

 CDebug(("QMEndCommand"));
 session = TlsGetValue(tls);

 if (context_error(session, CX_EXECUTING)) goto exit_qmendcommand;

 if (!message_pair(session, SrvrEndCommand, NULL, 0))
  {
   goto exit_qmendcommand;
  }

 session->context = CX_CONNECTED;

exit_qmendcommand:
 return;
}

/* ======================================================================
   QMEnterPackage()  -  Enter a licensed package                          */

VBBool _stdcall _export QMEnterPackage(name)
   BSTR name;
{
 VBBool status = VBFalse;
 int name_len;
 struct SESSION * session;

 CDebug("QMEnterPackage");
 session = TlsGetValue(tls);

 if (context_error(session, CX_CONNECTED)) goto exit_EnterPackage;

 name_len = SysStringByteLen(name);
 if ((name_len < 1) || (name_len > MAX_PACKAGE_NAME_LEN))
  {
   session->server_error = SV_ELSE;
   session->qm_status = ER_BAD_NAME;
  }
 else
  {
   if (!message_pair(session, SrvrEnterPackage, (char *)name, name_len))
    {
     goto exit_EnterPackage;
    }
  }

 switch(session->server_error)
  {
   case SV_OK:
      status = VBTrue;
      break;

   case SV_ON_ERROR:
      Abort("EnterPackage generated an abort event", TRUE);
      break;

   case SV_LOCKED:
   case SV_ELSE:
   case SV_ERROR:
      break;
  }
 
exit_EnterPackage:
 return status;
}

/* ======================================================================
   QMExitPackage()  -  Exit from a licensed package                       */

VBBool _stdcall _export QMExitPackage(name)
   BSTR name;
{
 VBBool status = VBFalse;
 int name_len;
 struct SESSION * session;

 CDebug(("QMExitPackage"));
 session = TlsGetValue(tls);

 if (context_error(session, CX_CONNECTED)) goto exit_ExitPackage;

 name_len = SysStringByteLen(name);
 if ((name_len < 1) || (name_len > MAX_PACKAGE_NAME_LEN))
  {
   session->server_error = SV_ELSE;
   session->qm_status = ER_BAD_NAME;
  }
 else
  {
   if (!message_pair(session, SrvrExitPackage, (char *)name, name_len))
    {
     goto exit_ExitPackage;
    }
  }

 switch(session->server_error)
  {
   case SV_OK:
      status = VBTrue;
      break;

   case SV_ON_ERROR:
      Abort("ExitPackage generated an abort event", TRUE);
      break;

   case SV_LOCKED:
   case SV_ELSE:
   case SV_ERROR:
      break;
  }
 
exit_ExitPackage:
 return status;
}

/* ======================================================================
   QMError()  -  Return extended error string                             */

BSTR _stdcall _export QMError()
{
 BSTR s;
 struct SESSION * session;

 session = TlsGetValue(tls);

 s = SysAllocStringByteLen(session->qmerror, strlen(session->qmerror));

 return s;
}

/* ======================================================================
   QMExecute()  -  Execute a command                                      */

BSTR _stdcall _export QMExecute(cmnd, err)
   BSTR cmnd;
   short int * err;
{
 long int reply_len = 0;
 BSTR reply;
 struct SESSION * session;

 CDebug("QMExecute");
 session = TlsGetValue(tls);

 if (context_error(session, CX_CONNECTED)) goto exit_qmexecute;

 if (!message_pair(session, SrvrExecute, (char *)cmnd, SysStringByteLen(cmnd)))
  {
   goto exit_qmexecute;
  }

 switch(session->server_error)
  {
   case SV_PROMPT:
      session->context = CX_EXECUTING;
      /* **** FALL THROUGH **** */

   case SV_OK:
      reply_len = session->buff_bytes - offsetof(INBUFF, data.execute.reply);
      break;

   case SV_ON_ERROR:
      Abort("EXECUTE generated an abort event", TRUE);
      break;
  }

exit_qmexecute:
 reply = SysAllocStringByteLen((reply_len == 0)?NULL:(session->buff->data.execute.reply), reply_len);
 *err = session->server_error;
 return reply;
}

/* ======================================================================
   QMExtract()  -  Extract field, value or subvalue                       */

BSTR _stdcall _export QMExtract(src, fno, vno, svno)
   BSTR src;
   short int fno;
   short int vno;
   short int svno;
{
 long int src_len;
 char * p;


 src_len = SysStringByteLen(src);
 if (src_len == 0) goto null_result;   /* Extracting from null string */


 /* Setp 1  -  Initialise variables */

 if (fno < 1) fno = 1;


 /* Step 2  -  Position to start of item */

 /* Skip to start field */

 while(--fno)
  {
   p = memchr(src, FIELD_MARK, src_len);
   if (p == NULL) goto null_result;    /* No such field */
   src_len -= (1 + p - (char *)src);
   (char *)src = p + 1;
  }
 p = memchr(src, FIELD_MARK, src_len);
 if (p != NULL) src_len = p - (char *)src;    /* Adjust to ignore later fields */

 if (vno < 1) goto done;   /* Extracting whole field */
 

 /* Skip to start value */

 while(--vno)
  {
   p = memchr(src, VALUE_MARK, src_len);
   if (p == NULL) goto null_result;    /* No such value */
   src_len -= (1 + p - (char *)src);
   (char *)src = p + 1;
  }

 p = memchr(src, VALUE_MARK, src_len);
 if (p != NULL) src_len = p - (char *)src; /* Adjust to ignore later values */

 if (svno < 1) goto done;   /* Extracting whole value */


 /* Skip to start subvalue */

 while(--svno)
  {
   p = memchr(src, SUBVALUE_MARK, src_len);
   if (p == NULL) goto null_result;    /* No such subvalue */
   src_len -= (1 + p - (char *)src);
   (char *)src = p + 1;
  }
 p = memchr(src, SUBVALUE_MARK, src_len);
 if (p != NULL) src_len = p - (char *)src; /* Adjust to ignore later fields */

done:
 return SysAllocStringByteLen((char *)src, src_len);

null_result:
 return SysAllocStringByteLen(NULL, 0);
}

/* ======================================================================
   QMField()  -  Extract delimited substring                              */

BSTR _stdcall _export QMField(src, delim, first, occ)
   BSTR src;
   BSTR delim;
   long int first;
   long int * occ;
{
 int src_len;
 int delim_len;
 char delimiter;
 long int occurrences = -1;
 long int bytes;         /* Remaining bytes counter */
 char * pos;
 char * p;
 char * q;


 src_len = SysStringByteLen(src);

 delim_len = SysStringByteLen(delim);
 if ((delim_len == 0) || (src_len == 0)) goto return_null;

 if (first < 1) first = 1;

 if (occ != NULL)
  {
   occurrences = *occ;
   if (occurrences < 1) occurrences = 1;
  }

 delimiter = *delim;


 /* Find start of data to return */

 pos = (char *)src;
 bytes = src_len;
 while(--first)
  {
   p = memchr(pos, delimiter, bytes);
   if (p == NULL) goto return_null;
   bytes -= (p - pos + 1);
   pos = p + 1;
  }

 /* Find end of data to return */

 q = pos;
 do {
     p = memchr(q, delimiter, bytes);
     if (p == NULL)
      {
       p = q + bytes;
       break;
      }
     bytes -= (p - q + 1);
     q = p + 1;
    } while(--occurrences);

 return SysAllocStringByteLen(pos, p - pos);

return_null:
 return SysAllocStringByteLen(NULL, 0);
}

/* ======================================================================
   QMGetSession()  -  Return session index                                */

short int _stdcall _export QMGetSession()
{
 struct SESSION * session;

 CDebug(("QMGetSession"));
 session = TlsGetValue(tls);
 return session->idx;
}

/* ======================================================================
   QMIns()  -  Insert field, value or subvalue                            */

BSTR _stdcall _export QMIns(xsrc, fno, vno, svno, new)
   BSTR xsrc;
   short int fno;
   short int vno;
   short int svno;
   BSTR new;
{
 char * src;
 long int src_len;
 char * pos;             /* Rolling source pointer */
 long int bytes;         /* Remaining bytes counter */
 long int ins_len;       /* Length of inserted data */
 long int new_len;
 BSTR new_str;
 char * p;
 short int i;
 long int n;
 short int fm = 0;
 short int vm = 0;
 short int sm = 0;
 char postmark = '\0';


 src_len = SysStringByteLen(xsrc);
 src = (char *)xsrc;
 ins_len = SysStringByteLen(new);

 pos = src;
 bytes = src_len;

 if (fno < 1) fno = 1;
 if (vno < 0) vno = 0;
 if (svno < 0) svno = 0;

 if (src_len == 0)   /* Inserting in null string */
  {
   if (fno > 1) fm = fno - 1;
   if (vno > 1) vm = vno - 1;
   if (svno > 1) sm = svno - 1;
   goto done;
  }


 /* Skip to start field */

 for(i = 1; i < fno; i++)
  {
   p = memchr(pos, FIELD_MARK, bytes);
   if (p == NULL)    /* No such field */
    {
     fm = fno - i;
     if (vno > 1) vm = vno - 1;
     if (svno > 1) sm = svno - 1;
     pos = src + src_len;
     goto done;
    }
   bytes -= (1 + p - pos);
   pos = p + 1;
  }

 p = memchr(pos, FIELD_MARK, bytes);
 if (p != NULL) bytes = p - pos;    /* Length of field */

 if (vno == 0)   /* Inserting field */
  {
   postmark = FIELD_MARK;
   goto done;
  }


 /* Skip to start value */

 for(i = 1; i < vno; i++)
  {
   p = memchr(pos, VALUE_MARK, bytes);
   if (p == NULL)    /* No such value */
    {
     vm = vno - i;
     if (svno > 1) sm = svno - 1;
     pos += bytes;
     goto done;
    }
   bytes -= (1 + p - pos);
   pos = p + 1;
  }

 p = memchr(pos, VALUE_MARK, bytes);
 if (p != NULL) bytes = p - pos;    /* Length of value, including end mark */

 if (svno == 0)  /* Inserting value */
  {
   postmark = VALUE_MARK;
   goto done;
  }

 /* Skip to start subvalue */

 for(i = 1; i < svno; i++)
  {
   p = memchr(pos, SUBVALUE_MARK, bytes);
   if (p == NULL)    /* No such subvalue */
    {
     sm = svno - i;
     pos += bytes;
     goto done;
    }
   bytes -= (1 + p - pos);
   pos = p + 1;
  }

 postmark = SUBVALUE_MARK;


done:
 /* Now construct new string inserting fm, vm and sm marks and new data
    at 'pos'.                                                           */

 n = pos - src;    /* Length of leading substring */
 if ((n == src_len)
     || (IsDelim(src[n]) && src[n] > postmark))   /* 0380 */
  {
   postmark = '\0';
  }

 new_len = src_len + fm + vm + sm + ins_len + (postmark != '\0');
 new_str = SysAllocStringByteLen(NULL, new_len);
 p = (char *)new_str;

 if (n)    
  {
   memcpy(p, src, n);
   p += n;
  }

 while(fm--) *(p++) = FIELD_MARK;
 while(vm--) *(p++) = VALUE_MARK;
 while(sm--) *(p++) = SUBVALUE_MARK;

 if (ins_len)
  {
   memcpy(p, new, ins_len);
   p += ins_len;
  }

 if (postmark != '\0') *(p++) = postmark;

 n = src_len - n;    /* Length of trailing substring */
 if (n)
  {
   memcpy(p, pos, n);
  }

 return new_str;
}

/* ======================================================================
   QMLocate()  -  Search dynamic array                                    */

VBBool _stdcall _export QMLocate(item, src, fno, vno, svno, pos, order)
   BSTR item;
   BSTR src;
   short int fno;
   short int vno;
   short int svno;
   short int * pos;
   BSTR order;
{
 int item_len;
 int src_len;
 char * p;
 char * q;
 bool ascending = TRUE;
 bool left = TRUE;
 bool sorted = FALSE;
 short int idx;
 int d;
 VBBool found = VBFalse;
 int i;
 int bytes;
 char mark;
 int n;
 int x;
 char * s1;
 char * s2;


 /* Establish sort mode */

 i = SysStringByteLen(order);
 if (i >= 1)
  {
   sorted = TRUE;
   ascending = ((order)[0] != 'D');  /* 0513 */
  }

 if (i >= 2) left = (order[1] != 'R');  /* 0513 */

 item_len = SysStringByteLen(item);    /* Length of item to find */

 src_len = SysStringByteLen(src);

 p = (char *)src;
 bytes = src_len;

 if (fno < 1) fno = 1;

 /* Scan to start field */

 mark = FIELD_MARK;
 idx = fno;
 for(i = 1; i < fno; i++)
  {
   if (bytes == 0) goto done;
   q = memchr(p, FIELD_MARK, bytes);
   if (q == NULL) goto done;    /* No such field */
   bytes -= (1 + q - p);
   p = q + 1;
  }

 if (vno > 0)  /* Searching for value or subvalue */
  {
   q = memchr(p, FIELD_MARK, bytes);
   if (q != NULL) bytes = q - p;    /* Limit view to length of field */

   mark = VALUE_MARK;
   idx = vno;
   for(i = 1; i < vno; i++)
    {
     if (bytes == 0) goto done;
     q = memchr(p, VALUE_MARK, bytes);
     if (q == NULL) goto done;    /* No such value */
     bytes -= (1 + q - p);
     p = q + 1;
    }

   if (svno > 0)    /* Searching for subvalue */
    {
     q = memchr(p, VALUE_MARK, bytes);
     if (q != NULL) bytes = q - p;    /* Limit view to length of value */

     mark = SUBVALUE_MARK;
     idx = svno;
     for(i = 1; i < svno; i++)    /* 0512 */
      {
       if (bytes == 0) goto done;
       q = memchr(p, SUBVALUE_MARK, bytes);
       if (q == NULL) goto done;    /* No such subvalue */
       bytes -= (1 + q - p);
       p = q + 1;
      }
    }
  }

 /* We are now at the start position for the search and 'mark' contains the
    delimiting mark character.  Because we have adjusted 'bytes' to limit
    our view to the end of the item, we do not need to worry about higher
    level marks.  Examine successive items from this point.                 */

 if (bytes == 0)
  {
   if (item_len == 0) found = VBTrue;
   goto done;
  }

 do {
     q = memchr(p, mark, bytes);
     n = (q == NULL)?bytes:(q - p);  /* Length of entry */
     if ((n == item_len) && (memcmp(p, item, n) == 0))
      {
       found = VBTrue;
       goto done;
      }

     if (sorted)   /* Check if we have gone past correct position */
      {
       if (left || (n == item_len))
        {
         d = memcmp(p, item, min(n, item_len));
         if (d == 0)
          {
           if ((n > item_len) == ascending) goto done;
          }
         else if ((d > 0) == ascending) goto done;
        }
       else   /* Right sorted and lengths differ */
        {
         x = n - item_len;
         s1 = p;
         s2 = (char *)item;
         if (x > 0)  /* Array entry longer than item to find */
          {
           do {d = *(s1++) - ' ';} while((d == 0) && --x);
          }
         else        /* Array entry shorter than item to find */
          {
           do {d = ' ' - *(s2++);} while((d == 0) && ++x);
          }
         if (d == 0) d = memcmp(s1, s2, min(n, item_len));
         if ((d > 0) == ascending) goto done;
        }
      }
     
     bytes -= (1 + q - p);
     p = q + 1;
     idx++;
    } while(q);


done:
 *pos = idx;
 return found;
}

/* ======================================================================
   QMLogto()  -  LOGTO                                                    */

VBBool _stdcall _export QMLogto(account_name)
   BSTR account_name;
{
 VBBool status = VBFalse;
 int name_len;
 struct SESSION * session;

 CDebug("QMLogto");
 session = TlsGetValue(tls);

 if (context_error(session, CX_CONNECTED)) goto exit_logto;

 name_len = SysStringByteLen(account_name);
 if ((name_len < 1) || (name_len > MAX_ACCOUNT_NAME_LEN))
  {
   session->server_error = SV_ELSE;
   session->qm_status = ER_BAD_NAME;
  }
 else
  {
   if (!message_pair(session, SrvrAccount, (char *)account_name, name_len))
    {
     goto exit_logto;
    }
  }

 switch(session->server_error)
  {
   case SV_OK:
      status = VBTrue;
      break;

   case SV_ON_ERROR:
      Abort("LOGTO generated an abort event", TRUE);
      break;

   case SV_LOCKED:
   case SV_ELSE:
   case SV_ERROR:
      break;
  }
 
exit_logto:
 return status;
}

/* ======================================================================
   QMMarkMapping()  -  Enable/disable mark mapping on a directory file    */

void _stdcall _export QMMarkMapping(fno, state)
   short int fno;
   short int state;
{
 struct {
         short int fno;
         short int state;
        } packet;
 struct SESSION * session;

 CDebug(("QMMarkMapping"));
 session = TlsGetValue(tls);

 if (!context_error(session, CX_CONNECTED))
  {
   packet.fno = fno;
   packet.state = state;

   message_pair(session, SrvrMarkMapping, (char *)&packet, sizeof(packet));
  }
}

/* ======================================================================
   QMMatch()  -  String matching                                          */

VBBool _stdcall _export QMMatch(str, pattern)
   BSTR str;
   BSTR pattern;
{
 char template[MAX_MATCH_TEMPLATE_LEN+1];
 char src_string[MAX_MATCHED_STRING_LEN+1];
 int n;
 char * p;
 char * q;
 char * component_start = NULL;
 char * component_end = NULL;


 /* Copy template and string to our own buffers so we can alter them */

 n = SysStringByteLen(pattern);
 if ((n == 0) || (n > MAX_MATCH_TEMPLATE_LEN)) goto no_match;
 memcpy(template, (char *)pattern, n);
 template[n] = '\0';

 n = SysStringByteLen(str);
 if (n > MAX_MATCHED_STRING_LEN) goto no_match;
 if (n) memcpy(src_string, (char *)str, n);
 src_string[n] = '\0';

 /* Try matching against each value mark delimited template */

 p = template;
 do {                             /* Outer loop - extract templates */
     q = strchr(p, (char)VALUE_MARK);
     if (q != NULL) *q = '\0';

     if (match_template(src_string, p, 0, 0, &component_start, &component_end))
      {
       return VBTrue;
      }

     p = q + 1;
    } while(q != NULL);

no_match:
 return VBFalse;
}

/* ======================================================================
   QMMatchfield()  -  String matching                                     */

BSTR _stdcall _export QMMatchfield(str, pattern, component)
   BSTR str;
   BSTR pattern;
   short int component;
{
 char template[MAX_MATCH_TEMPLATE_LEN+1];
 char src_string[MAX_MATCHED_STRING_LEN+1];
 int n;
 char * p;
 char * q;
 char * component_start = NULL;
 char * component_end = NULL;

 if (component < 1) component = 1;

 /* Copy template and string to our own buffers so we can alter them */

 n = SysStringByteLen(pattern);
 if ((n == 0) || (n > MAX_MATCH_TEMPLATE_LEN)) goto no_match;
 memcpy(template, (char *)pattern, n);
 template[n] = '\0';

 n = SysStringByteLen(str);
 if (n > MAX_MATCHED_STRING_LEN) goto no_match;
 if (n) memcpy(src_string, (char *)str, n);
 src_string[n] = '\0';

 /* Try matching against each value mark delimited template */

 p = template;
 do {                             /* Outer loop - extract templates */
     q = strchr(p, (char)VALUE_MARK);
     if (q != NULL) *q = '\0';

     if (match_template(src_string, p, 0, component, &component_start, &component_end))
      {
       if (component_end != NULL) *(component_end) = '\0';
       n = strlen(component_start);
       return SysAllocStringByteLen(component_start, n);
      }

     p = q + 1;
    } while(q != NULL);

no_match:
 return SysAllocStringByteLen(NULL, 0);
}

/* ======================================================================
   QMOpen()  -  Open file                                                 */

short int _stdcall _export QMOpen(filename)
   BSTR filename;
{
 short int fno = 0;
 struct SESSION * session;

 CDebug("QMOpen");
 session = TlsGetValue(tls);

 if (context_error(session, CX_CONNECTED)) goto exit_qmopen;

 if (!message_pair(session, SrvrOpen, (char *)filename, SysStringByteLen(filename)))
  {
   goto exit_qmopen;
  }

 switch(session->server_error)
  {
   case SV_OK:
      fno = session->buff->data.open.fno;
      break;

   case SV_ON_ERROR:
   case SV_ELSE:
   case SV_ERROR:
      break;
  }

exit_qmopen:
 return fno;
}

/* ======================================================================
   QMRead()  -  Read record                                               */

BSTR _stdcall _export QMRead(fno, id, err)
   short int fno;
   BSTR id;
   short int * err;
{
 CDebug(("QMRead"));
 return read_record(fno, id, err, SrvrRead);
}

/* ======================================================================
   QMReadl()  -  Read record with shared lock                             */

BSTR _stdcall _export QMReadl(fno, id, wait, err)
   short int fno;
   BSTR id;
   short int wait;
   short int * err;
{
 return read_record(fno, id, err, (wait)?SrvrReadlw:SrvrReadl);
}

/* ======================================================================
   QMReadList()  - Read select list                                       */

BSTR _stdcall _export QMReadList(listno, err)
   short int listno;
   short int * err;
{
 BSTR list;
 short int status = 0;
 long int data_len = 0;
 struct {
         short int listno;
        } packet;
 struct SESSION * session;

 CDebug("QMReadList");
 session = TlsGetValue(tls);

 if (context_error(session, CX_CONNECTED)) goto exit_qmreadlist;

 packet.listno = listno;
 
 if (!message_pair(session, SrvrReadList, (char *)&packet, sizeof(packet)))
  {
   goto exit_qmreadlist;
  }

 switch(session->server_error)
  {
   case SV_OK:
      data_len = session->buff_bytes - offsetof(INBUFF, data.readlist.list);
      break;

   case SV_ON_ERROR:
      Abort("READLIST generated an abort event", TRUE);
      break;

   case SV_LOCKED:
   case SV_ELSE:
   case SV_ERROR:
      break;
  }
 
 status = session->server_error;
 
exit_qmreadlist:

 list = SysAllocStringByteLen((data_len == 0)?NULL:(session->buff->data.readlist.list), data_len);
 *err = status;
 return list;
}

/* ======================================================================
   QMReadNext()  - Read select list entry                                 */

BSTR _stdcall _export QMReadNext(listno, err)
   short int listno;
   short int * err;
{
 BSTR id;
 short int status = 0;
 long int id_len = 0;
 struct {
         short int listno;
        } packet;
 struct SESSION * session;

 CDebug(("QMReadNext"));
 session = TlsGetValue(tls);

 if (context_error(session, CX_CONNECTED)) goto exit_qmreadnext;

 packet.listno = listno;
 
 if (!message_pair(session, SrvrReadNext, (char *)&packet, sizeof(packet)))
  {
   goto exit_qmreadnext;
  }

 switch(session->server_error)
  {
   case SV_OK:
      id_len = session->buff_bytes - offsetof(INBUFF, data.readnext.id);
      break;

   case SV_ON_ERROR:
      Abort("READNEXT generated an abort event", TRUE);
      break;

   case SV_LOCKED:
   case SV_ELSE:
   case SV_ERROR:
      break;
  }
 
 status = session->server_error;

exit_qmreadnext:
 id = SysAllocStringByteLen((id_len == 0)?NULL:(session->buff->data.readnext.id), id_len);
 *err = status;
 return id;
}

/* ======================================================================
   QMReadu()  -  Read record with exclusive lock                          */

BSTR _stdcall _export QMReadu(fno, id, wait, err)
   short int fno;
   BSTR id;
   short int wait;
   short int * err;
{
 CDebug("QMReadu");
 return read_record(fno, id, err, (wait)?SrvrReaduw:SrvrReadu);
}

/* ======================================================================
   QMRecordlock()  -  Get lock on a record                                */

void _stdcall _export QMRecordlock(fno, id, update_lock, wait)
   short int fno;
   BSTR id;
   short int update_lock;
   short int wait;
{
 int id_len;
 struct {
         short int fno;
         short int flags;        /* 0x0001 : Update lock */
                                 /* 0x0002 : No wait */
         char id[MAX_ID_LEN];
        } packet;
 struct SESSION * session;

 CDebug(("QMRecordlock"));
 session = TlsGetValue(tls);

 if (!context_error(session, CX_CONNECTED))
  {
   packet.fno = fno;

   id_len = SysStringByteLen(id);
   if (id_len > MAX_ID_LEN)
    {
     session->server_error = SV_ON_ERROR;
     session->qm_status = ER_IID;
    }
   else
    {
     memcpy(packet.id, id, id_len);
     packet.flags = (update_lock)?1:0;
     if (!wait) packet.flags |= 2;

     if (!message_pair(session, SrvrLockRecord, (char *)&packet, id_len + 4))
      {
       session->server_error = SV_ON_ERROR;
      }
    }

   switch(session->server_error)
    {
     case SV_OK:
        break;

     case SV_ON_ERROR:
        Abort("RECORDLOCK generated an abort event", TRUE);
        break;

     case SV_LOCKED:
     case SV_ELSE:
     case SV_ERROR:
        break;
    }
  }
}

/* ======================================================================
   QMRelease()  -  Release lock                                           */

void _stdcall _export QMRelease(fno, id)
   short int fno;
   BSTR id;
{
 int id_len;
 struct {
         short int fno;
         char id[MAX_ID_LEN];
        } packet;
 struct SESSION * session;

 CDebug("QMRelease");
 session = TlsGetValue(tls);

 if (context_error(session, CX_CONNECTED)) goto exit_release;

 packet.fno = fno;

 if (fno == 0)    /* Release all locks */
  {
   id_len = 0;
  }
 else
  {
   id_len = SysStringByteLen(id);
   if (id_len > MAX_ID_LEN)
    {
     session->server_error = SV_ON_ERROR;
     session->qm_status = ER_IID;
     goto release_error;
    }
   memcpy(packet.id, id, id_len);
  }

 if (!message_pair(session, SrvrRelease, (char *)&packet, id_len + 2))
  {
   goto exit_release;
  }


release_error:
 switch(session->server_error)
  {
   case SV_OK:
      break;

   case SV_ON_ERROR:
      Abort("RELEASE generated an abort event", TRUE);
      break;

   case SV_LOCKED:
   case SV_ELSE:
   case SV_ERROR:
      break;
  }
 
exit_release:
 return;
}

/* ======================================================================
   QMReplace()  -  Replace field, value or subvalue                       */

BSTR _stdcall _export QMReplace(src, fno, vno, svno, new)
   BSTR src;
   short int fno;
   short int vno;
   short int svno;
   BSTR new;
{
 long int src_len;
 char * pos;             /* Rolling source pointer */
 long int bytes;         /* Remaining bytes counter */
 long int ins_len;       /* Length of inserted data */
 long int new_len;
 BSTR new_str;
 char * p;
 short int i;
 long int n;
 short int fm = 0;
 short int vm = 0;
 short int sm = 0;


 src_len = SysStringByteLen(src);
 ins_len = SysStringByteLen(new);

 pos = (char *)src;
 bytes = src_len;

 if (src_len == 0)   /* Replacing in null string */
  {
   if (fno > 1) fm = fno - 1;
   if (vno > 1) vm = vno - 1;
   if (svno > 1) sm = svno - 1;
   bytes = 0;
   goto done;
  }


 if (fno < 1)        /* Appending a new field */
  {
   pos = (char *)src + src_len;
   fm = 1;
   if (vno > 1) vm = vno - 1;
   if (svno > 1) sm = svno - 1;
   bytes = 0;
   goto done;
  }


 /* Skip to start field */

 for(i = 1; i < fno; i++)
  {
   p = memchr(pos, FIELD_MARK, bytes);
   if (p == NULL)    /* No such field */
    {
     fm = fno - i;
     if (vno > 1) vm = vno - 1;
     if (svno > 1) sm = svno - 1;
     pos = (char *)src + src_len;
     bytes = 0;
     goto done;
    }
   bytes -= (1 + p - pos);
   pos = p + 1;
  }

 p = memchr(pos, FIELD_MARK, bytes);
 if (p != NULL) bytes = p - pos;    /* Length of field */

 if (vno == 0) goto done;   /* Replacing whole field */

 if (vno < 0)    /* Appending new value */
  {
   if (p != NULL) pos = p;   /* 0553 */
   else pos += bytes;        /* 0553 */
   if (bytes) vm = 1;        /* 0553 */

   if (svno > 1) sm = svno - 1;
   bytes = 0;
   goto done;
  }


 /* Skip to start value */

 for(i = 1; i < vno; i++)
  {
   p = memchr(pos, VALUE_MARK, bytes);
   if (p == NULL)    /* No such value */
    {
     vm = vno - i;
     if (svno > 1) sm = svno - 1;
     pos += bytes;
     bytes = 0;
     goto done;
    }
   bytes -= (1 + p - pos);
   pos = p + 1;
  }

 p = memchr(pos, VALUE_MARK, bytes);
 if (p != NULL) bytes = p - pos;    /* Length of value, including end mark */

 if (svno == 0) goto done;   /* Replacing whole value */

 if (svno < 1)     /* Appending new subvalue */
  {
   if (p != NULL) pos = p;  /* 0553 */
   else pos += bytes;       /* 0553 */
   if (bytes) sm = 1;       /* 0553 */
   bytes = 0;
   goto done;
  }

 /* Skip to start subvalue */

 for(i = 1; i < svno; i++)
  {
   p = memchr(pos, SUBVALUE_MARK, bytes);
   if (p == NULL)    /* No such subvalue */
    {
     sm = svno - i;
     pos += bytes;
     bytes = 0;
     goto done;
    }
   bytes -= (1 + p - pos);
   pos = p + 1;
  }

 p = memchr(pos, SUBVALUE_MARK, bytes);
 if (p != NULL) bytes = p - pos;    /* Length of subvalue, including end mark */

done:
 /* Now construct new string with 'bytes' bytes omitted starting at 'pos',
    inserting fm, vm and sm marks and new data                             */

 new_len = src_len - bytes + fm + vm + sm + ins_len;
 new_str = SysAllocStringByteLen(NULL, new_len);
 p = (char *)new_str;

 n = pos - (char *)src;    /* Length of leading substring */
 if (n)    
  {
   memcpy(p, src, n);
   p += n;
  }

 while(fm--) *(p++) = FIELD_MARK;
 while(vm--) *(p++) = VALUE_MARK;
 while(sm--) *(p++) = SUBVALUE_MARK;

 if (ins_len)
  {
   memcpy(p, new, ins_len);
   p += ins_len;
  }

 n = src_len - (bytes + n);    /* Length of trailing substring */
 if (n)
  {
   memcpy(p, pos + bytes, n);
  }

 return new_str;
}

/* ======================================================================
   QMRespond()  -  Respond to request for input                           */

BSTR _stdcall _export QMRespond(response, err)
   BSTR response;
   short int * err;
{
 long int reply_len = 0;
 BSTR reply;
 struct SESSION * session;

 CDebug(("QMRespond"));
 session = TlsGetValue(tls);

 if (context_error(session, CX_EXECUTING)) goto exit_qmrespond;

 if (!message_pair(session, SrvrRespond, (char *)response, SysStringByteLen(response)))
  {
   goto exit_qmrespond;
  }

 switch(session->server_error)
  {
   case SV_OK:
      session->context = CX_CONNECTED;
      /* **** FALL THROUGH **** */

   case SV_PROMPT:
      reply_len = session->buff_bytes - offsetof(INBUFF, data.execute.reply);
      break;

   case SV_ERROR:      /* Probably QMRespond() used when not expected */
      break;

   case SV_ON_ERROR:
      session->context = CX_CONNECTED;
      Abort("EXECUTE generated an abort event", TRUE);
      break;
  }

exit_qmrespond:
 reply = SysAllocStringByteLen((reply_len == 0)?NULL:(session->buff->data.execute.reply), reply_len);
 *err = session->server_error;
 return reply;
}

/* ======================================================================
   QMSelect()  - Generate select list                                     */

void _stdcall _export QMSelect(fno, listno)
   short int fno;
   short int listno;
{
 struct {
         short int fno;
         short int listno;
        } packet;
 struct SESSION * session;

 CDebug("QMSelect");
 session = TlsGetValue(tls);

 if (context_error(session, CX_CONNECTED)) goto exit_qmselect;

 packet.fno = fno;
 packet.listno = listno;

 if (!message_pair(session, SrvrSelect, (char *)&packet, sizeof(packet)))
  {
   goto exit_qmselect;
  }

 switch(session->server_error)
  {
   case SV_OK:
      break;

   case SV_ON_ERROR:
      Abort("Select generated an abort event", TRUE);
      break;
  }
 
exit_qmselect:
 return;
}

/* ======================================================================
   QMSelectIndex()  - Generate select list from index entry               */

void _stdcall _export QMSelectIndex(fno, index_name, index_value, listno)
   short int fno;
   BSTR index_name;
   BSTR index_value;
   short int listno;
{
 struct {
         short int fno;
         short int listno;
         short int name_len;
         char index_name[255+1];
         short int data_len_place_holder;      /* Index name is actually... */
         char index_data_place_holder[255+1];  /* ...var length */
        } packet;
 short int n;
 char * p;
 struct SESSION * session;

 CDebug(("QMSelectIndex"));
 session = TlsGetValue(tls);

 if (context_error(session, CX_CONNECTED)) goto exit_qmselectindex;

 packet.fno = fno;
 packet.listno = listno;

 /* Insert index name */

 n = SysStringByteLen(index_name);
 packet.name_len = n;
 p = packet.index_name;
 memcpy(p, index_name, n);
 p += n;
 if (n & 1) *(p++) = '\0';

 /* Insert index value */

 n = SysStringByteLen(index_value);
 *((short int *)p) = n;
 p += sizeof(short int);
 memcpy(p, index_value, n);
 p += n;
 if (n & 1) *(p++) = '\0';
 

 if (!message_pair(session, SrvrSelectIndex, (char *)&packet, p - (char *)&packet))
  {
   goto exit_qmselectindex;
  }

 switch(session->server_error)
  {
   case SV_OK:
      break;

   case SV_ON_ERROR:
      Abort("SelectIndex generated an abort event", TRUE);
      break;
  }
 
exit_qmselectindex:
 return;
}

/* ======================================================================
   QMSelectLeft()  - Scan index position to left
   QMSelectRight()  - Scan index position to right                        */

BSTR _stdcall _export QMSelectLeft(fno, index_name, listno)
   short int fno;
   BSTR index_name;
   short int listno;
{
 CDebug("QMSelectLeft");
 return SelectLeftRight(fno, index_name, listno, SrvrSelectLeft);
}

BSTR _stdcall _export QMSelectRight(fno, index_name, listno)
   short int fno;
   BSTR index_name;
   short int listno;
{
 CDebug(("QMSelectRight"));
 return SelectLeftRight(fno, index_name, listno, SrvrSelectRight);
}

BSTR SelectLeftRight(fno, index_name, listno, mode)
   short int fno;
   BSTR index_name;
   short int listno;
   short int mode;
{
 long int key_len = 0;
 BSTR key;
 struct {
         short int fno;
         short int listno;
         char index_name[255+1];
        } packet;
 short int n;
 char * p;
 struct SESSION * session;

 session = TlsGetValue(tls);

 if (!context_error(session, CX_CONNECTED))
  {
   packet.fno = fno;
   packet.listno = listno;

   /* Insert index name */

   n = SysStringByteLen(index_name);
   p = packet.index_name;
   memcpy(p, index_name, n);
   p += n;

   if (message_pair(session, mode, (char *)&packet, p - (char *)&packet))
    {
     switch(session->server_error)
      {
       case SV_OK:
          key_len = session->buff_bytes - offsetof(INBUFF, data.selectleft.key);
          break;

       case SV_ON_ERROR:
          Abort("SelectLeft / SelectRight generated an abort event", TRUE);
          break;
      }
    } 
  }

 key = SysAllocStringByteLen((key_len == 0)?NULL:(session->buff->data.selectleft.key), key_len);
 return key;
}

/* ======================================================================
   QMSetLeft()  - Set index position at extreme left
   QMSetRight()  - Set index position at extreme right                    */

void _stdcall _export QMSetLeft(fno, index_name)
   short int fno;
   BSTR index_name;
{
 CDebug("QMSetLeft");
 SetLeftRight(fno, index_name, SrvrSetLeft);
}

void _stdcall _export QMSetRight(fno, index_name)
   short int fno;
   BSTR index_name;
{
 CDebug(("QMSetRight"));
 SetLeftRight(fno, index_name, SrvrSetRight);
}

void SetLeftRight(fno, index_name, mode)
   short int fno;
   BSTR index_name;
   short int mode;
{
 struct {
         short int fno;
         char index_name[255+1];
        } packet;
 short int n;
 char * p;
 struct SESSION * session;

 session = TlsGetValue(tls);

 if (!context_error(session, CX_CONNECTED))
  {
   packet.fno = fno;

   /* Insert index name */

   n = SysStringByteLen(index_name);
   p = packet.index_name;
   memcpy(p, index_name, n);
   p += n;

   if (message_pair(session, mode, (char *)&packet, p - (char *)&packet))
    {
     switch(session->server_error)
      {
       case SV_OK:
          break;

       case SV_ON_ERROR:
          Abort("SetLeft / SetRight generated an abort event", TRUE);
          break;
      }
    }
  }
}

/* ======================================================================
   QMSetSession()  -  Set session index                                   */

VBBool _stdcall _export QMSetSession(idx)
   short int idx;
{
 if ((idx < 0) || (idx >= MAX_SESSIONS)
    || (sessions[idx].context == CX_DISCONNECTED))
  {
   return VBFalse;
  }

 CDebug("QMSetSession");
 TlsSetValue(tls, &(sessions[idx]));

 return VBTrue;
}

/* ======================================================================
   QMStatus()  -  Return STATUS() value                                   */

long int _stdcall _export QMStatus()
{
 struct SESSION * session;

 session = TlsGetValue(tls);

 return session->qm_status;
}

/* ======================================================================
   QMWrite()  -  Write record                                             */

void _stdcall _export QMWrite(fno, id, data)
   short int fno;
   BSTR id;
   BSTR data;
{
 CDebug(("QMWrite"));
 write_record(SrvrWrite, fno, id, data);
}

/* ======================================================================
   QMWriteu()  -  Write record, retaining lock                            */

void _stdcall _export QMWriteu(fno, id, data)
   short int fno;
   BSTR id;
   BSTR data;
{
 CDebug("QMWriteu");
 write_record(SrvrWriteu, fno, id, data);
}

/* ======================================================================
   context_error()  - Check for appropriate context                       */

bool context_error(session, expected)
   struct SESSION * session;
   short int expected;
{
 char * p;

 if (session->context != expected)
  {
   switch(session->context)
    {
     case CX_DISCONNECTED:
        p = "A server function has been attempted when no connection has been established";
        break;

     case CX_CONNECTED:
        switch(expected)
         {
          case CX_DISCONNECTED:
             p = "A function has been attempted which is not allowed when a connection has been established";
             break;

          case CX_EXECUTING:
             p = "Cannot send a response or end a command when not executing a server command";
             break;
         }
        break;

     case CX_EXECUTING:
        p = "A new server function has been attempted while executing a server command";
        break;

     default:
        p = "A function has been attempted in the wrong context";
        break;
    }

   MessageBox(NULL, p, "QMClient: Context error", MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);
   return TRUE;
  }

 return FALSE;
}

/* ====================================================================== */

void delete_record(mode, fno, id)
   short int mode;
   short int fno;
   BSTR id;
{
 int id_len;
 struct {
         short int fno;
         char id[MAX_ID_LEN];
        } packet;
 struct SESSION * session;

 session = TlsGetValue(tls);

 if (context_error(session, CX_CONNECTED)) goto exit_delete;

 packet.fno = fno;

 id_len = SysStringByteLen(id);
 if ((id_len < 1) || (id_len > MAX_ID_LEN))
  {
   session->server_error = SV_ON_ERROR;
   session->qm_status = ER_IID;
  }
 else
  {
   memcpy(packet.id, id, id_len);

   if (!message_pair(session, mode, (char *)&packet, id_len + 2))
    {
     goto exit_delete;
    }
  }

 switch(session->server_error)
  {
   case SV_OK:
      break;

   case SV_ON_ERROR:
      Abort("DELETE generated an abort event", TRUE);
      break;

   case SV_LOCKED:
   case SV_ELSE:
   case SV_ERROR:
      break;
  }
 
exit_delete:
 return;
}

/* ======================================================================
   read_record()  -  Common path for READ, READL and READU                */

BSTR read_record(fno, id, err, mode)
   short int fno;
   BSTR id;
   short int * err;
   int mode;
{
 long int status;
 long int rec_len = 0;
 int id_len;
 BSTR rec;
 struct {
         short int fno;
         char id[MAX_ID_LEN];
        } packet;
 struct SESSION * session;

 session = TlsGetValue(tls);

 if (context_error(session, CX_CONNECTED)) goto exit_read;

 id_len = SysStringByteLen(id);
 if ((id_len < 1) || (id_len > MAX_ID_LEN))
  {
   session->qm_status = ER_IID;
   status = SV_ON_ERROR;
   goto exit_read;
  }

 packet.fno = fno;
 memcpy(packet.id, id, id_len);
 
 if (!message_pair(session, mode, (char *)&packet, id_len + 2))
  {
   goto exit_read;
  }

 switch(session->server_error)
  {
   case SV_OK:
      rec_len = session->buff_bytes - offsetof(INBUFF, data.read.rec);
      break;

   case SV_ON_ERROR:
      Abort("Read generated an abort event", TRUE);
      break;

   case SV_LOCKED:
   case SV_ELSE:
   case SV_ERROR:
      break;
  }
 
 status = session->server_error;
 
exit_read:
 rec = SysAllocStringByteLen((rec_len == 0)?NULL:(session->buff->data.read.rec), rec_len);
 *err = status;
 return rec;
}

/* ====================================================================== */

void write_record(mode, fno, id, data)
   short int mode;
   short int fno;
   BSTR id;
   BSTR data;
{
 int id_len;
 long int data_len;
 int bytes;
 INBUFF * q;
 struct PACKET {
                short int fno;
                short int id_len;
                char id[1];
               };
 struct SESSION * session;

 session = TlsGetValue(tls);

 if (context_error(session, CX_CONNECTED)) goto exit_write;

 id_len = SysStringByteLen(id);
 if ((id_len < 1) || (id_len > MAX_ID_LEN))
  {
   Abort("Illegal record id", FALSE);
   session->qm_status = ER_IID;
   goto exit_write;
  }

 data_len = SysStringByteLen(data);

 /* Ensure buffer is big enough for this record */

 bytes = sizeof(struct PACKET) + id_len + data_len;
 if (bytes >= session->buff_size)     /* Must reallocate larger buffer */
  {
   bytes = (bytes + BUFF_INCR - 1) & ~BUFF_INCR;
   q = (INBUFF *)malloc(bytes);
   if (q == NULL)
    {
     Abort("Insufficient memory", FALSE);
     session->qm_status = ER_SRVRMEM;
     goto exit_write;
    }
   free(session->buff);
   session->buff = q;
   session->buff_size = bytes;
  }

 /* Set up outgoing packet */

 ((struct PACKET *)(session->buff))->fno = fno;
 ((struct PACKET *)(session->buff))->id_len = id_len;
 memcpy(((struct PACKET *)(session->buff))->id, id, id_len);
 memcpy(((struct PACKET *)(session->buff))->id + id_len, data, data_len);

 if (!message_pair(session, mode, (char *)(session->buff), offsetof(struct PACKET, id) + id_len + data_len))
  {
   goto exit_write;
  }

 switch(session->server_error)
  {
   case SV_OK:
      break;

   case SV_ON_ERROR:
      Abort("Write generated an abort event", TRUE);
      break;

   case SV_LOCKED:
   case SV_ELSE:
   case SV_ERROR:
      break;
  }

exit_write:
 return;
}

/* ====================================================================== */

bool GetResponse(struct SESSION * session)
{
 if (!read_packet(session)) return FALSE;

 if (session->server_error == SV_ERROR)
  {
   strcpy(session->qmerror,"Unable to retrieve error text");
   write_packet(session, SrvrGetError, NULL, 0);
   if (read_packet(session)) strcpy(session->qmerror,session->buff->data.error.text);
   return FALSE;
  }

 return TRUE;
}

/* ====================================================================== */

void Abort(msg, use_response)
   char * msg;
   bool use_response;
{
 char abort_msg[1024+1];
 int n;
 char * p;
 struct SESSION * session;

 strcpy(abort_msg, msg);

 if (use_response)
  {
   session = TlsGetValue(tls);
   n = session->buff_bytes - offsetof(INBUFF, data.abort.message);
   if (n > 0)
    {
     p = abort_msg + strlen(msg);
     *(p++) = '\r';
     memcpy(p, session->buff->data.abort.message, n);
     *(p + n) = '\0';
    }
  }

 MessageBox(NULL, abort_msg, NULL, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);
}

/* ======================================================================
   Call a catalogued subroutine                                           */

void catcall (struct SESSION * session, BSTR subrname, short int argc, ...)
{
 va_list ap;
 short int i;
 BSTR arg;
 int subrname_len;
 long int arg_len;
 long int bytes;     /* Total length of outgoing packet */
 long int n;
 char * p;
 INBUFF * q;
 struct ARGDATA * argptr;
 BSTR * argx;
 int offset;

#define MAX_ARGS 20

 subrname_len = SysStringByteLen(subrname);
 if ((subrname_len < 1) || (subrname_len > MAX_CALL_NAME_LEN))
  {
   Abort("Illegal subroutine name in call", FALSE);
  }

 if ((argc < 0) || (argc > MAX_ARGS))
  {
   Abort("Illegal argument count in call", FALSE);
  }

 /* Accumulate outgoing packet size */

 bytes = 2;                           /* Subrname length */
 bytes += (subrname_len + 1) & ~1;    /* Subrname string (padded) */
 bytes += 2;                          /* Arg count */

 va_start(ap, argc);
 for (i = 0; i < argc; i++)
  {
   arg = *(va_arg(ap, BSTR *));
   arg_len = (arg == NULL)?0:SysStringByteLen(arg);
   bytes += 4 + ((arg_len + 1) & ~1); /* Four byte length + arg text (padded) */
  }
 va_end(ap);

 /* Ensure buffer is big enough */

 if (bytes >= session->buff_size)     /* Must reallocate larger buffer */
  {
   n = (bytes + BUFF_INCR - 1) & ~BUFF_INCR;
   q = (INBUFF *)malloc(n);
   if (q == NULL)
    {
     Abort("Unable to allocate network buffer", FALSE);
    }
   free(session->buff);
   session->buff = q;
   session->buff_size = bytes;
  }

 /* Set up outgoing packet */

 p = (char *)(session->buff);

 *((short int *)p) = subrname_len;    /* Subrname length */
 p += 2;

 memcpy(p, subrname, subrname_len);   /* Subrname */
 p += (subrname_len + 1) & ~1;
 
 *((short int *)p) = argc;            /* Arg count */
 p += 2;

 va_start(ap, argc);
 for (i = 1; i <= argc; i++)
  {
   arg = *(va_arg(ap, BSTR *));
   arg_len = (arg == NULL)?0:SysStringByteLen(arg);
   *((long int *)p) = arg_len;        /* Arg length */
   p += 4;
   if (arg_len) memcpy(p, arg, arg_len);           /* Arg text */
   p += (arg_len + 1) & ~1;
  }
 va_end(ap);

 if (!message_pair(session, SrvrCall, (char *)(session->buff), bytes))
  {
   goto err;
  }

 /* Now update any returned arguments */

 offset = offsetof(INBUFF, data.call.argdata);
 if (offset < session->buff_bytes)
  {
   va_start(ap, argc);
   for (i = 1; i <= argc; i++)
    {
     argptr = (ARGDATA *)(((char *)(session->buff)) + offset);

     argx = va_arg(ap, BSTR *);
     if (i == argptr->argno)
      {
       if (*argx != NULL) SysFreeString(*argx);

       *argx = SysAllocStringByteLen(argptr->text, argptr->arglen);

       offset += offsetof(ARGDATA, text) + ((argptr->arglen + 1) & ~1);
       if (offset >= session->buff_bytes) break;
      }
    }
   va_end(ap);
  }

err:
 switch(session->server_error)
  {
   case SV_OK:
      break;

   case SV_ON_ERROR:
      Abort("CALL generated an abort event", TRUE);
      break;

   case SV_LOCKED:
   case SV_ELSE:
   case SV_ERROR:
      break;
  }
}

/* ====================================================================== */

char * memstr(str, substr, str_len, substr_len)
   char * str;
   char * substr;
   int str_len;
   int substr_len;
{
 char * p;

 while(str_len != 0)
  {
   p = memchr(str, *substr, str_len);
   if (p == NULL) break;

   str_len -= p - str;
   if (str_len < substr_len) break;

   if (memcmp(p, substr, substr_len) == 0) return p;

   str = p + 1;
   str_len--;
  }

 return NULL;
}

/* ======================================================================
   match_template()  -  Match string against template                     */

bool match_template(char * string,
                    char * template,
                    short int component,        /* Current component number - 1 (incremented) */
                    short int return_component, /* Required component number */
                    char ** component_start,
                    char ** component_end)
{
 bool not;
 short int n;
 short int m;
 short int z;
 char * p;
 char delimiter;
 char * start;

 while(*template != '\0')
  {
   component++;
   if (component == return_component) *component_start = string;
   else if (component == return_component + 1) *component_end = string;

   start = template;

   if (*template == '~')
    {
     not = TRUE;
     template++;
    }
   else not = FALSE;

   if (isdigit(*template))
    {
     n = 0;
     do {
         n = (n * 10) + (*(template++) - '0');
        } while(isdigit(*template));

     switch(UpperCase(*(template++)))
      {
       case '\0':              /* String longer than template */
          /* 0115 rewritten */
          n = --template - start;
          if (n == 0) return FALSE;
          if (!memcmp(start, string, n) == not) return FALSE;
          string += n;
          break;
 
       case 'A':               /* nA  Alphabetic match */
          if (n)
           {
            while(n--)
             {
              if (*string == '\0') return FALSE;
              if ((isalpha(*string) != 0) == not) return FALSE;
              string++;
             }
           }
          else                 /* 0A */
           {
            if (*template != '\0') /* Further template items exist */
             {
              /* Match as many as possible further chaarcters */

              for (z = 0, p = string; ; z++, p++)
               {
                if ((*p == '\0') || ((isalpha(*p) != 0) == not)) break;
               }

              /* z now holds number of contiguous alphabetic characters ahead
                 of current position. Starting one byte after the final
                 alphabetic character, backtrack until the remainder of the
                 string matches the remainder of the template.               */

              for (p = string + z; z-- >= 0; p--)
               {
                if (match_template(p, template, component, return_component, component_start, component_end))
                 {
                  goto match_found;
                 }
               }
              return FALSE;
             }
            else
             {
              while((*string != '\0') && ((isalpha(*string) != 0) != not))
               {
                string++;
               }
             }
           }
          break;

       case 'N':               /* nN  Numeric match */
          if (n)
           {
            while(n--)
             {
              if (*string == '\0') return FALSE;
              if ((isdigit(*string) != 0) == not) return FALSE;
              string++;
             }
           }
          else                 /* 0N */
           {
            if (*template != '\0') /* Further template items exist */
             {
              /* Match as many as possible further chaarcters */
  
              for (z = 0, p = string; ; z++, p++)
               {
                if ((*p == '\0') || ((isdigit(*p) != 0) == not)) break;
               }

              /* z now holds number of contiguous numeric characters ahead
                 of current position. Starting one byte after the final
                 numeric character, backtrack until the remainder of the
                 string matches the remainder of the template.               */

              for (p = string + z; z-- >= 0; p--)
               {
                if (match_template(p, template, component, return_component, component_start, component_end))
                 {
                  goto match_found;
                 }
               }
              return FALSE;
             }
            else
             {
              while((*string != '\0') && ((isdigit(*string) != 0) != not))
               {
                string++;
               }
             }
           }
          break;

       case 'X':               /* nX  Unrestricted match */
          if (n)
           {
            while(n--)
             {
              if (*(string++) == '\0') return FALSE;
             }
           }
          else                 /* 0X */
           {
            if (*template != '\0')    /* Further template items exist */
             {
              /* Match as few as possible further characters */
  
              do {
                  if (match_template(string, template, component, return_component, component_start, component_end))
                   {
                    goto match_found;
                   }
                 } while(*(string++) != '\0');
              return FALSE;
             }
            goto match_found;
           }
          break;

       case '-':               /* Count range */
          if (!isdigit(*template)) return FALSE;
          m = 0;
          do {
              m = (m * 10) + (*(template++) - '0');
             } while(isdigit(*template));
          m -= n;
          if (m < 0) return FALSE;

          switch(UpperCase(*(template++)))
           {
            case '\0':              /* String longer than template */
               n = --template - start;
               if (n)          /* We have found a trailing unquoted literal */
                {
                 if ((memcmp(start, string, n) == 0) != not) return TRUE;
                }
               return FALSE;

            case 'A':               /* n-mA  Alphabetic match */
               /* Match n alphabetic items */
 
              while(n--)
                {
                 if (*string == '\0') return FALSE;
                 if ((isalpha(*string) != 0) == not) return FALSE;
                 string++;
                }

               /* We may match up to m further alphabetic characters but must
                  also match as many as possible.  Check how many alphabetic
                  characters there are (up to m) and then backtrack trying
                  matches against the remaining template (if any).           */

               for(z = 0, p = string; z < m; z++, p++)
                {
                 if ((*p == '\0') || ((isalpha(*p) != 0) == not)) break;
                }

               /* z now holds max number of matchable characters.
                  Try match at each of these positions and also at the next
                  position (Even if it is the end of the string)            */

               if (*template != '\0')    /* Further template items exist */
                {
                 for (p = string + z; z-- >= 0; p--)
                  {
                   if (match_template(p, template, component, return_component, component_start, component_end))
                    {
                     goto match_found;
                    }
                  }
                 return FALSE;
                }
               else string += z;
               break;

            case 'N':               /* n-mN  Numeric match */
               /* Match n numeric items */
 
              while(n--)
                {
                 if (*string == '\0') return FALSE;
                 if ((isdigit(*string) != 0) == not) return FALSE;
                 string++;
                }

               /* We may match up to m further numeric characters but must
                  also match as many as possible.  Check how many numeric
                  characters there are (up to m) and then backtrack trying
                  matches against the remaining template (if any).           */

               for(z = 0, p = string; z < m; z++, p++)
                {
                 if ((*p == '\0') || ((isdigit(*p) != 0) == not)) break;
                }
             
               /* z now holds max number of matchable characters.
                  Try match at each of these positions and also at the next
                  position (Even if it is the end of the string)            */

               if (*template != '\0')    /* Further template items exist */
                {
                 for (p = string + z; z-- >= 0; p--)
                  {
                   if (match_template(p, template, component, return_component, component_start, component_end))
                    {
                     goto match_found;
                    }
                  }
                 return FALSE;
                }
               else string += z;
               break;

            case 'X':               /* n-mX  Unrestricted match */
               /* Match n items of any type */

               while(n--)
                {
                 if (*(string++) == '\0') return FALSE;
                }

               /* Match as few as possible up to m further characters */

               if (*template != '\0')
                {
                 while(m--)
                  {
                   if (match_template(string, template, component, return_component, component_start, component_end))
                    {
                     goto match_found;
                    }
                   string++;
                  }
                 return FALSE;
                }
               else
                {
                 if ((signed int)strlen(string) > m) return FALSE;
                 goto match_found;
                }

            default:
               /* We have found an unquoted literal */
               n = --template - start;
               if ((memcmp(start, string, n) == 0) == not) return FALSE;
               string += n;
               break;
           }
          break;

       default:
          /* We have found an unquoted literal */
          n = --template - start;
          if ((memcmp(start, string, n) == 0) == not) return FALSE;
          string += n;
          break;
      }
    }
   else if (memcmp(template, "...", 3) == 0)   /* ... same as 0X */
    {
     template += 3;
     if (not) return FALSE;
     if (*template != '\0')    /* Further template items exist */
      {
       /* Match as few as possible further characters */

       while(*string != '\0')
        {
         if (match_template(string, template, component, return_component, component_start, component_end))
          {
           goto match_found;
          }
         string++;
        }
       return FALSE;
      }
     goto match_found;
    }
   else /* Must be literal text item */
    {
     delimiter = *template;
     if ((delimiter == '\'') || (delimiter == '"')) /* Quoted literal */
      {
       template++;
       p = strchr(template, (char)delimiter);
       if (p == NULL) return FALSE;
       n = p - template;
       if (n)
        {
         if ((memcmp(template, string, n) == 0) == not) return FALSE;
         string += n;
        }
       template += n + 1;
      }
     else                /* Unquoted literal. Treat as single character */
      {
       if ((*(template++) == *(string++)) == not) return FALSE;
      }
    }
  }

 if (*string != '\0') return FALSE;  /* String longer than template */
 
match_found:
 return TRUE;
}

/* ======================================================================
   message_pair()  -  Send message and receive response                   */

bool message_pair(session, type, data, bytes)
   struct SESSION * session;
   int type;
   unsigned char * data;
   long int bytes;
{
 if (write_packet(session, type, data, bytes))
  {
   return GetResponse(session);
  }

 return FALSE;
}

/* ======================================================================
   OpenSocket()  -  Open connection to server                            */

Private bool OpenSocket(struct SESSION * session, char * host, short int port)
{
 bool status = FALSE;
 WSADATA wsadata;
 unsigned long nInterfaceAddr;
 struct sockaddr_in sock_addr;
 int nPort;
 struct hostent * hostdata;
 char ack_buff;
 int n;
 unsigned int n1, n2, n3, n4;


 if (port < 0) port = 4243;

 /* Start Winsock up */

 if (WSAStartup(MAKEWORD(1, 1), &wsadata) != 0)
  {
   sprintf(session->qmerror, "WSAStartup error");
   goto exit_opensocket;
  }

 if ((sscanf(host, "%u.%u.%u.%u", &n1, &n2, &n3, &n4) == 4)
     && (n1 <= 255) && (n2 <= 255) && (n3 <= 255) && (n4 <= 255))
  {
   /* Looks like an IP address */
   nInterfaceAddr = inet_addr(host);
  }
 else
  {
   hostdata = gethostbyname(host);
   if (hostdata == NULL)
    {
     NetError("gethostbyname()");
     goto exit_opensocket;
    }

   nInterfaceAddr = *((long int *)(hostdata->h_addr));
  }

 nPort= htons(port);

 session->sock = socket(AF_INET, SOCK_STREAM, 0);
 if (session->sock == INVALID_SOCKET)
  {
   NetError("socket()");
   goto exit_opensocket;
  }

 sock_addr.sin_family = AF_INET;
 sock_addr.sin_addr.s_addr = nInterfaceAddr;
 sock_addr.sin_port = nPort;

 if (connect(session->sock, (struct sockaddr *)&sock_addr, sizeof(sock_addr)))
  {
   NetError("connect()");
   goto exit_opensocket;
  }

 n = TRUE;
 setsockopt(session->sock, IPPROTO_TCP, TCP_NODELAY, (char *)&n, sizeof(int));

 /* Wait for an Ack character to arrive before we assume the connection
    to be open and working. This is necessary because Linux loses anything
    we send before the QM process is up and running.                       */

 do {
     if (recv(session->sock, &ack_buff, 1, 0) < 1) goto exit_opensocket;
    } while(ack_buff != '\x06');

 status = 1;

exit_opensocket:
 return status;
}

/* ======================================================================
   CloseSocket()  -  Close connection to server                          */

Private bool CloseSocket(struct SESSION * session)
{
 bool status = FALSE;

 if (session->sock == INVALID_SOCKET) goto exit_closesocket;

 closesocket(session->sock);
 session->sock = INVALID_SOCKET;

 status = TRUE;

exit_closesocket:
 return status;
}

/* ======================================================================
   NetError                                                               */

static void NetError(char * prefix)
{
 char msg[80];

 sprintf(msg, "Error %d from %s", WSAGetLastError(), prefix);
 MessageBox(NULL, msg, "QMClient: Network error", MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);
}

/* ======================================================================
   read_packet()  -  Read a QM data packet                                */

Private bool read_packet(struct SESSION * session)
{
 int rcvd_bytes;         /* Length of received packet fragment */
 long int packet_bytes;  /* Total length of incoming packet */
 int rcv_len;
 long int n;
 unsigned char * p;

 /* 0272 restructured */
 struct {
         long int packet_length;
         short int server_error;
         long int status;
        } in_packet_header;
 #define IN_PKT_HDR_BYTES 10

 if ((!session->is_local) && (session->sock == INVALID_SOCKET)) return FALSE;

 /* Read packet header */ 

 p = (char *)&in_packet_header;
 session->buff_bytes = 0;
 do {
     rcv_len = IN_PKT_HDR_BYTES - session->buff_bytes;
     if (session->is_local)
      {
       if (!ReadFile(session->hPipe, p, rcv_len, (DWORD *)&rcvd_bytes, NULL)) return FALSE;
      }
     else
      {
       if ((rcvd_bytes = recv(session->sock, p, rcv_len, 0)) <= 0) return FALSE;
      }

     session->buff_bytes += rcvd_bytes;
     p += rcvd_bytes;
    } while(session->buff_bytes < IN_PKT_HDR_BYTES);

 if (srvr_debug != NULL) debug((char *)&in_packet_header, IN_PKT_HDR_BYTES);

 /* Calculate remaining bytes to read */

 packet_bytes = in_packet_header.packet_length - IN_PKT_HDR_BYTES;

 if (srvr_debug != NULL)
  {
   fprintf(srvr_debug, "IN (%ld bytes)\n", packet_bytes);
   fflush(srvr_debug);
  }

 if (packet_bytes >= session->buff_size)      /* Must reallocate larger buffer */
  {
   free(session->buff);
   n = (packet_bytes + BUFF_INCR) & ~(BUFF_INCR - 1);
   session->buff = (INBUFF *)malloc(n);
   if (session->buff == NULL) return FALSE;
   session->buff_size = n;

   if (srvr_debug != NULL)
    {
     fprintf(srvr_debug, "Resized buffer to %ld bytes\n", session->buff_size);
     fflush(srvr_debug);
    }
  }

 /* Read data part of packet */

 p = (char *)(session->buff);
 session->buff_bytes = 0;
 while(session->buff_bytes < packet_bytes)
  {
   rcv_len = min(session->buff_size - session->buff_bytes, 16384);
   if (session->is_local)
    {
     if (!ReadFile(session->hPipe, p, rcv_len, (DWORD *)&rcvd_bytes, NULL)) return FALSE;
    }
   else
    {
     if ((rcvd_bytes = recv(session->sock, p, rcv_len, 0)) <= 0) return FALSE;
    }

   session->buff_bytes += rcvd_bytes;
   p += rcvd_bytes;
  }

 ((char *)(session->buff))[session->buff_bytes] = '\0';

 if (srvr_debug != NULL) debug((char *)(session->buff), session->buff_bytes);

 session->server_error = in_packet_header.server_error;
 session->qm_status = in_packet_header.status;

 return TRUE;
}

/* ======================================================================
   write_packet()  -  Send QM data packet                                 */

Private bool write_packet(struct SESSION * session, int type, char * data, long int bytes)
{
 DWORD n;

 struct {
         long int length;
         short int type;
        } packet_header;
#define PKT_HDR_BYTES 6

 if ((!session->is_local) && (session->sock == INVALID_SOCKET)) return FALSE;

 packet_header.length = bytes + PKT_HDR_BYTES;  /* 0272 */
 packet_header.type = type;
 if (session->is_local)
  {
   if (!WriteFile(session->hPipe, (char *)&packet_header, PKT_HDR_BYTES, &n, NULL))
    {
     return FALSE;
    }
  }
 else
  {
   if (send(session->sock, (unsigned char *)&packet_header, PKT_HDR_BYTES, 0) != PKT_HDR_BYTES) return FALSE;
  }

 if (srvr_debug != NULL)
  {
   fprintf(srvr_debug, "OUT (%ld bytes). Type %d\n",
           packet_header.length, (int)packet_header.type);
   fflush(srvr_debug);
  }

 if ((data != NULL) && (bytes > 0))
  {
   if (session->is_local)
    {
     if (!WriteFile(session->hPipe, data, (DWORD)bytes, &n, NULL))
      {
       return FALSE;
      }
    }
   else
    {
     if (send(session->sock, data, bytes, 0) != bytes) return FALSE;
    }

   if (srvr_debug != NULL) debug(data, bytes);
  }

 return TRUE;
}

/* ======================================================================
   debug()  -  Debug function                                             */

Private void debug(unsigned char * p, int n)
{
 short int i;
 short int j;
 unsigned char c;
 char s[72+1];
 static char hex[] = "0123456789ABCDEF";

 for(i = 0; i < n; i += 16)
  {
   memset(s, ' ', 72);
   s[72] = '\0';

   s[0] = hex[i >> 12];
   s[1] = hex[(i >> 8) & 0x0F];
   s[2] = hex[(i >> 4) & 0x0F];
   s[3] = hex[i & 0x0F];
   s[4] = ':';
   s[54] = '|';

   for(j = 0; (j < 16) && ((j + i) < n); j++)
    {
     c = *(p++);
     s[6 + 3 * j] = hex[c >> 4];
     s[7 + 3 * j] = hex[c & 0x0F];
     s[56 + j] = ((c >= 32) && (c < 128))?c:'.';
    }

   fprintf(srvr_debug, "%s\n", s);
  }
 fprintf(srvr_debug, "\n");
 fflush(srvr_debug);
}

/* ======================================================================
   sysdir()  -  Return static QMSYS directory pointer                     */

char * sysdir()
{
 static char sysdirpath[MAX_PATHNAME_LEN + 1];
 FILE * fu;
 char * p;
 char path[MAX_PATHNAME_LEN + 1];
 char section[50];
 char rec[200+1];
 struct SESSION * session;

 session = TlsGetValue(tls);

 GetWindowsDirectory(path, MAX_PATHNAME_LEN);
 strcat(path, "\\qm.ini");

 fu = fopen(path, "rt");
 if (fu == NULL)
  {
   sprintf(session->qmerror, "%s not found", path);
   return NULL;
  }

 section[0] = '\0';
 while(fgets(rec, 200, fu) != NULL)
  {
   if ((p = strchr(rec, '\n')) != NULL) *p = '\0';

   if ((rec[0] == '#') || (rec[0] == '\0')) continue;

   if (rec[0] == '[')
    {
     if ((p = strchr(rec, ']')) != NULL) *p = '\0';
     strcpy(section, rec+1);
     strupr(section);
     continue;
    }

   if (strcmp(section, "QM") == 0)        /* [qm] items */
    {
     if (strncmp(rec, "QMSYS=", 6) == 0)
      {
       strcpy(sysdirpath, rec+6);
       break;
      }
    }
  }

 fclose(fu);

 if (sysdirpath[0] == '\0')
  {
   sprintf(session->qmerror, "No QMSYS parameter in configuration file");
   return NULL;
  }

 return sysdirpath;
}

/* ====================================================================== */

Private struct SESSION * FindFreeSession()
{
 short int i;
 struct SESSION * session;

 /* Find a free session table entry */

 for(i = 0; i < MAX_SESSIONS; i++)
  {
   if (sessions[i].context == CX_DISCONNECTED) break;
  }

 if (i == MAX_SESSIONS)
  {
   /* Must return error via a currently connected session */
   session = TlsGetValue(tls);
   strcpy(session->qmerror, "Too many sessions");
   return NULL;
  }

 session = &(sessions[i]);
 TlsSetValue(tls, session);
 if (srvr_debug != NULL)
  {
   fprintf(srvr_debug, "Set session %d as %08lX, tls %08lX\n",
           session->idx, (long)session, (long)tls);
   fflush(srvr_debug);
  }

 if (session->buff == NULL)
  {
   session->buff_size = 2048;
   session->buff = (INBUFF *)malloc(session->buff_size);
  }

 return session;
}

/* ====================================================================== */

Private void disconnect(struct SESSION * session)
{
 if (srvr_debug != NULL)
  {
   fprintf(srvr_debug, "Disconnect session %d (%08lX)\n", session->idx, (long)session);
   fflush(srvr_debug);
  }

 (void)write_packet(session, SrvrQuit, NULL, 0);
 if (session->is_local)
  {
   if (session->hPipe != INVALID_HANDLE_VALUE)
    {
     CloseHandle(session->hPipe);
     session->hPipe = INVALID_HANDLE_VALUE;
    }
  }
 else (void)CloseSocket(session);
 session->context = CX_DISCONNECTED;

 if (session->buff != NULL)
  {
   free(session->buff);
   session->buff = NULL;
  }
}

/* ====================================================================== */

void ClientDebug(char * name)
{
 struct SESSION * session;

 if (srvr_debug != NULL)
  {
   session = TlsGetValue(tls);
   fprintf(srvr_debug, "%d (%08lX): %s\n",
           session->idx, GetCurrentThreadId(), name);
   fflush(srvr_debug);
  }
}

/* END-CODE */
