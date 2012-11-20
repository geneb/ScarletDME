/* NETFILES.C
 * Networked file access.
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
 * 04 Oct 07  2.6-5 net_open() was not using ShortInt() when extracting file
 *                  number form response packet.
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 21 May 06  2.4-4 Moved byte ordering functions into qmlib.c
 * 21 Apr 06  2.4-2 0480 Reworked to use SV_xxx status values.
 * 09 Mar 06  2.3-8 Added net_mark_mapping().
 * 05 Jan 06  2.3-3 Added index scanning functions.
 * 03 Nov 05  2.2-16 Added big endian support.
 * 04 Jan 05  2.1-0 0294 Various errors that cause QMNet connections to fail.
 * 26 Oct 04  2.0-7 0272 Cannot use sizeof() for packet header transmission
 *                  size.
 * 01 Oct 04  2.0-3 0256 Search of host table for open connection was using
 *                  wrong variable and hence never found a match.
 * 30 Sep 04  2.0-3 Added net_fileinfo().
 * 28 Sep 04  2.0-3 Replaced GetSysPaths() with config_path.
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
#include "qmclient.h"
#include "dh_fmt.h"
#include "qmnet.h"
#include "syscom.h"

   #include <sys/wait.h>

   #ifdef min
      #undef min
   #endif
   #define min(a,b) (((a) < (b))?(a):(b))

   #define DLLEntry


#define MAX_HOSTS            10     /* Simultaneous host connections */
#define MAX_SERVER_NAME_LEN  16     /* QM name of server */
#define MAX_HOST_NAME_LEN    32     /* IP address or name */

Private struct
{
 short int ref_ct;                         /* Zero on spare cell */
 char server_name[MAX_SERVER_NAME_LEN+1];  /* QM name for this server */
 SOCKET sock;
} host_table[MAX_HOSTS];
Private short int host_index;

/* Packet buffer */
#define BUFF_INCR      4096
typedef struct INBUFF INBUFF;
struct INBUFF
 {
  union {
         struct {
                 char message[1];
                } abort;
         struct {                  /* Error text retrieval */
                 char text[1];
                } error;
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
         struct {                  /* QMRecordlocked */
                 long int status;
                } recordlocked;
         struct {                  /* QMIndices */
                 char reply[1];
                } indices;
         struct {                  /* QMSelectList, QMSelectindex, QMSelectindexv */
                 char reply[1];
                } selectlist;
         struct {                  /* QMSelectLeft, QMSelectRight */
                 char key[1];
                } selectleft;
        } data;
 };

Private INBUFF * buff = NULL;
Private long int buff_size;         /* Allocated size of buffer */
Private long int buff_bytes;        /* Size of received packet */

short int server_error;
long int remote_status;


Private void close_connection(void);
Private bool message_pair(int type, char * data, long int bytes);
Private bool GetResponse(void);
Private bool read_packet(void);
Private bool write_packet(int type, char * data, long int bytes);


/* ======================================================================
   net_clearfile()                                                        */

int net_clearfile(FILE_VAR * fvar)
{
 struct {
         short int fno;
        } ALIGN2 packet;


 host_index = fvar->access.net.host_index;
 packet.fno = fvar->access.net.file_no;
 message_pair(SrvrClearfile, (char *)&packet, sizeof(packet));
 return remote_status;
}

/* ======================================================================
   net_close()                                                            */

void net_close(FILE_VAR * fvar)
{
 struct {
         short int fno;
        } ALIGN2 packet;


 host_index = fvar->access.net.host_index;

 packet.fno = ShortInt(fvar->access.net.file_no);

 (void)message_pair(SrvrClose, (char *)&packet, sizeof(packet));

 if (--host_table[host_index].ref_ct == 0) close_connection();
 if (host_table[host_index].ref_ct < 0) k_error("-ve in net_close");
}

/* ======================================================================
   net_delete()  -  Delete a record via QMNet                             */

int net_delete(
   FILE_VAR * fvar,
   char * id,
   short int id_len,
   bool keep_lock)
{
 struct PACKET {
                short int fno;
                char id[MAX_ID_LEN];
               } ALIGN2 packet;


 host_index = fvar->access.net.host_index;

 /* Set up outgoing packet */

 packet.fno = ShortInt(fvar->access.net.file_no);
 memcpy(packet.id, id, id_len);
 if (!message_pair((keep_lock)?SrvrDeleteu:SrvrDelete, (char *)&packet, id_len + 2))
  {
   server_error = SV_ON_ERROR;
  }

 process.status = remote_status;

 return server_error;
}

/* ======================================================================
   net_fileinfo()                                                        */

STRING_CHUNK * net_fileinfo(
   FILE_VAR * fvar,
   int key)
{
 STRING_CHUNK * str;
 int len;
 struct {
         short int fno;
         int key;
        } ALIGN2 packet;


 host_index = fvar->access.net.host_index;
 packet.fno = ShortInt(fvar->access.net.file_no);
 packet.key = key;
 message_pair(SrvrFileinfo, (char *)&packet, sizeof(packet));

 process.status = remote_status;
 if (server_error != SV_OK) return NULL;

 /* Convert received data to a chunked string */
 len = buff_bytes - offsetof(INBUFF, data.indices.reply);
 ts_init(&str, len);
 ts_copy(buff->data.indices.reply, len);
 ts_terminate();
 return str;
}

/* ======================================================================
   net_filelock()                                                        */

int net_filelock(
   FILE_VAR * fvar,
   bool wait)
{
 struct {
         short int fno;
         short int wait;
        } ALIGN2 packet;


 host_index = fvar->access.net.host_index;
 packet.fno = ShortInt(fvar->access.net.file_no);
 packet.wait = ShortInt(wait);
 message_pair(SrvrFilelock, (char *)&packet, sizeof(packet));
 return remote_status;
}

/* ======================================================================
   net_fileunlock()                                                        */

int net_fileunlock(FILE_VAR * fvar)
{
 struct {
         short int fno;
        } ALIGN2 packet;


 host_index = fvar->access.net.host_index;
 packet.fno = ShortInt(fvar->access.net.file_no);
 message_pair(SrvrFileunlock, (char *)&packet, sizeof(packet));
 return remote_status;
}

/* ======================================================================
   net_indices1()  -  Fetch information about indices                     */

STRING_CHUNK * net_indices1(FILE_VAR * fvar)
{
 struct {
         short int fno;
        } ALIGN2 packet;
 int len;
 STRING_CHUNK * str;

 host_index = fvar->access.net.host_index;

 packet.fno = ShortInt(fvar->access.net.file_no);

 if (!message_pair(SrvrIndices1, (char *)&packet, sizeof(packet))
     || (server_error == SV_ON_ERROR))
  {
   return NULL;
  }

 process.status = remote_status;
 if (server_error != SV_OK) return NULL;

 /* Convert received data to a chunked string */
 len = buff_bytes - offsetof(INBUFF, data.indices.reply);
 ts_init(&str, len);
 ts_copy(buff->data.indices.reply, len);
 ts_terminate();
 return str;
}

/* ======================================================================
   net_indices2()  -  Fetch information about specific index              */

STRING_CHUNK * net_indices2(
   FILE_VAR * fvar,
   char * index_name)
{
 struct {
         short int fno;
         char index_name[MAX_ID_LEN];
        } ALIGN2 packet;
 STRING_CHUNK * str;
 int len;

 host_index = fvar->access.net.host_index;

 packet.fno = ShortInt(fvar->access.net.file_no);
 len = strlen(index_name);
 memcpy(packet.index_name, index_name, len);

 if (!message_pair(SrvrIndices2, (char *)&packet, len + 2)
     || (server_error == SV_ON_ERROR))
  {
   return NULL;
  }

 process.status = remote_status;
 if (server_error != SV_OK) return NULL;

 /* Convert received data to a chunked string */
 len = buff_bytes - offsetof(INBUFF, data.indices.reply);
 ts_init(&str, len);
 ts_copy(buff->data.indices.reply, len);
 ts_terminate();
 return str;
}

/* ======================================================================
   net_lock()  -  Lock a record on remote system                          */

int net_lock(         /* Returns 1000+blocking user if blocked */
   FILE_VAR * fvar,
   char * id,
   short int id_len,
   bool update,
   bool no_wait)
{
 short int flags;
 struct {
         short int fno;
         short int flags;        /* 0x0001 : Update lock */
                                 /* 0x0002 : No wait */
         char id[MAX_ID_LEN];
        } ALIGN2 packet;


 host_index = fvar->access.net.host_index;

 packet.fno = ShortInt(fvar->access.net.file_no);
 memcpy(packet.id, id, id_len);

 flags = (update)?1:0;
 if (no_wait) flags |= 2;
 packet.flags = ShortInt(flags);

 if (!message_pair(SrvrLockRecord, (char *)&packet, id_len + 4))
  {
   server_error = SV_ON_ERROR;
  }

 process.status = remote_status;
 if (server_error == SV_LOCKED) process.status += 1000;

 return server_error;
}

/* ======================================================================
   net_mark_mapping()  -  Enable/disable mark mapping                     */

void net_mark_mapping(
   FILE_VAR * fvar,
   bool state)
{
 struct PACKET {
                short int fno;
                short int state;
               } ALIGN2 packet;


 host_index = fvar->access.net.host_index;

 /* Set up outgoing packet */

 packet.fno = ShortInt(fvar->access.net.file_no);
 packet.state = ShortInt(state);
 message_pair(SrvrMarkMapping, (char *)&packet, sizeof(packet));
}

/* ======================================================================
   net_open()  -  Open a file on a remote QM server                       */

bool net_open(
   char * server,       /* Server name */
   char * remote_file,  /* account;file of target file */
   FILE_VAR * fvar)     /* File variable to populate */
{
 int server_len;
 FILE * ini_file;
 char section[32+1];
 char ini_rec[200+1];
 bool found;
 SOCKET sock;
 unsigned long nInterfaceAddr;
 struct sockaddr_in sock_addr;
 int nPort;
 struct hostent * hostdata;
 char login_data[2 + MAX_USERNAME_LEN + 2 + MAX_USERNAME_LEN];
 char ack_buff;
 int roll;
 char * host;
 char * username;
 char * password;
 int port = 4243;
 bool connected = FALSE;
 char mapped_chars[] = "PWbfTYR.BZKwm6qX4tH-avjUd0GI18Lx37ehiFSJEn52lMocy9OQDNszAVprkuCg";
 short int map_len;
 char c;
 short int i;
 int j;
 int k;
 int m;
 int n;
 char * p;
 char * q;
 char * r;



 if (buff == NULL)
  {
   buff_size = 32768;
   buff = (INBUFF *)malloc(buff_size);

   for(i = 0; i < MAX_HOSTS; i++)
    {
     host_table[i].ref_ct = 0;
    }
  }

 if ((server_len = strlen(server)) > MAX_SERVER_NAME_LEN)
  {
   process.status = ER_HOSTNAME;
   goto exit_open_networked_file;
  }

 /* Have we already got this host open? */

 host_index = -1;
 for(i = 0; i < MAX_HOSTS; i++)
  {
   if (host_table[i].ref_ct != 0)
    {
     if (!stricmp(host_table[i].server_name, server))     /* 0256, 0294 */
      {
       host_index = i;
       host_table[host_index].ref_ct++;   /* 0294 */
       goto host_open;
      }
    }
   else
    {
     if (host_index < 0) host_index = i;  /* Remember free cell */
    }
  }

 if (host_index < 0)     /* 0294 */
  {
   process.status = ER_HOST_TABLE;
   goto exit_open_networked_file;
  }

 /* Use configuration file to transform the server name to its
    corresponding host name and to retrieve the user name, password
    and port number.                                                */

 if ((ini_file = fopen(config_path, FOPEN_READ_MODE)) == NULL)
  {
   process.status = ER_NO_CONFIG;
   goto exit_open_networked_file;
  }

 found = FALSE;
 section[0] = '\0';
 while(fgets(ini_rec, sizeof(ini_rec), ini_file) != NULL)
  {
   if ((p = strchr(ini_rec, '\n')) != NULL) *p = '\0';

   if ((ini_rec[0] == '#') || (ini_rec[0] == '\0')) continue;

   if (ini_rec[0] == '[')
    {
     if ((p = strchr(ini_rec, ']')) != NULL) *p = '\0';
     strcpy(section, ini_rec+1);
     UpperCaseString(section);
     continue;
    }

   if (strcmp(section, "QMNET") == 0)        /* [qmnet] items */
    {
     if (!StringCompLenNoCase((char *)ini_rec, (char *)server, server_len)
         && (ini_rec[server_len] == '='))
      {
       /* Found this server */
       found = TRUE;
       break;
      }
    }
  }

 fclose(ini_file);

 if (!found)
  {
   process.status = ER_SERVER;
   goto exit_open_networked_file;
  }

 p = ini_rec + server_len + 1;
 host = strtok(p, ",");
 username = strtok(NULL, ",");
 password = strtok(NULL, ",");
 if ((p = strchr(host, ':')) != NULL)
  {
   *p = '\0';
   port = atoi(p+1);
  }

 if (strchr(host, '.'))
  {
   nInterfaceAddr = inet_addr(host);
  }
 else
  {
   hostdata = gethostbyname(host);
   if (hostdata == NULL)
    {
     process.status = ER_RESOLVE;
     process.os_error = NetError;
     goto exit_open_networked_file;
    }

   nInterfaceAddr = *((long int *)(hostdata->h_addr));
  }


 nPort= htons(port);

 sock = socket(AF_INET, SOCK_STREAM, 0);
 if (sock == INVALID_SOCKET)
  {
   process.status = ER_NOSOCKET;
   process.os_error = NetError;
   goto exit_open_networked_file;
  }

 sock_addr.sin_family = AF_INET;
 sock_addr.sin_addr.s_addr = nInterfaceAddr;
 sock_addr.sin_port = nPort;

 if (connect(sock, (struct sockaddr *)&sock_addr, sizeof(sock_addr)))
  {
   process.status = ER_CONNECT;
   process.os_error = NetError;
   goto exit_open_networked_file;
  }

 /* Connection established. Make a host table entry */

 connected = TRUE;
 strcpy(host_table[host_index].server_name, server);
 UpperCaseString(host_table[host_index].server_name);   /* 0294 */
 host_table[host_index].sock = sock;
 host_table[host_index].ref_ct++;              /* 0294 Moved */

 n = TRUE;
 setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&n, sizeof(int));

 /* Wait for an Ack character to arrive before we assume the connection
    to be open and working. This is necessary because Linux loses anything
    we send before the QM process is up and running.                       */

 do {
     if (recv(sock, &ack_buff, 1, 0) < 1)
      {
       process.status = ER_RECV_ERR;
       process.os_error = NetError;
       goto exit_open_networked_file;
      }
    } while(ack_buff != '\x06');


 /* Complete login process */

 /* Set up login data */

 p = login_data;

 n = strlen(username);
 *((short int *)p) = ShortInt(n);  /* User name len */
 p += 2;

 memcpy(p, (char *)username, n);   /* User name */
 p += n;
 if (n & 1) *(p++) = '\0';

 q = (char *)password;
   n = strlen(password);
   *((short int *)p) = ShortInt(n);  /* Password len */
   p += 2;
 
 /* Copy password, decrypting on the way. This is a very simple encryption. */

   map_len = strlen(mapped_chars);
   roll = 10;
   for(k = 0; k < n; k++)
    {
     c = *(q++);
     if ((r = strchr(mapped_chars, c)) != NULL)
      {
       m = r - mapped_chars - roll;
       while(m < 0) m += map_len;
       j = m % map_len;
       c = mapped_chars[j];
       roll = c;
      }
     *(p++) = c;
    }
 if (n & 1) *(p++) = '\0';

 n = p - login_data;
 if ((!message_pair(SrvrLogin, (char *)login_data, n))
      || (server_error != SV_OK))
  {
   process.status = ER_LOGIN;
   goto exit_open_networked_file;
  }

 /* Now attempt to attach to QMSYS account */

 if (!message_pair(SrvrAccount, "QMSYS", 5))
  {
   process.status = ER_ACCOUNT;
   goto exit_open_networked_file;
  }

host_open:

 /* Now open the file */

 if (!message_pair(SrvrOpenQMNet, (char *)remote_file, strlen(remote_file)))
  {
   process.status = remote_status;
   goto exit_open_networked_file;
  }

 if (server_error)
  {
   process.status = remote_status;
   goto exit_open_networked_file;
  }
  
 /* Set up file variable */

 fvar->type = NET_FILE;
 fvar->access.net.host_index = host_index;
 fvar->access.net.file_no = ShortInt(buff->data.open.fno);

 process.status = 0;

exit_open_networked_file:

 if (process.status != 0)
  {
   if (connected)
    {
     if (--host_table[host_index].ref_ct == 0) close_connection();
    }
  }
 return (process.status == 0);
}

/* ======================================================================
   net_read()  -  Read a record via QMNet                                 */

int net_read(
   FILE_VAR * fvar,
   char * id,
   short int id_len,
   unsigned short int op_flags,
   STRING_CHUNK ** str)
{
 short int mode;
 long int rec_len = 0;
 struct {
         short int fno;
         char id[MAX_ID_LEN];
        } ALIGN2 packet;


 if (op_flags & P_ULOCK)    /* Update lock */
  {
   mode = (op_flags & P_LOCKED)?SrvrReadu:SrvrReaduw;
  }
 else if (op_flags & P_LLOCK)    /* Shared lock */
  {
   mode = (op_flags & P_LOCKED)?SrvrReadl:SrvrReadlw;
  }
 else                       /* No lock */
  {
   mode = SrvrRead;
  }

 host_index = fvar->access.net.host_index;

 packet.fno = ShortInt(fvar->access.net.file_no);
 memcpy(packet.id, id, id_len);
 
 if (!message_pair(mode, (char *)&packet, id_len + 2))
  {
   server_error = SV_ON_ERROR;
  }

 process.status = remote_status;

 if (server_error == SV_OK)
  {
   /* Convert received data to a chunked string */
   rec_len = buff_bytes - offsetof(INBUFF, data.read.rec);
   ts_init(str, rec_len);
   ts_copy(buff->data.read.rec, rec_len);
   ts_terminate();
  }

 return server_error;
}

/* ======================================================================
   net_readv()  -  Perform a READV via QMNet                              */

int net_readv(
   FILE_VAR * fvar,
   char * id,
   short int id_len,
   int field_no,
   unsigned short int op_flags,
   STRING_CHUNK ** str)
{
 long int rec_len = 0;
 short int flags;
 struct {
         short int fno;
         short int flags;
         int field_no;
         char id[MAX_ID_LEN];
        } ALIGN2 packet;


 host_index = fvar->access.net.host_index;

 packet.fno = ShortInt(fvar->access.net.file_no);

 flags = 0;
 if (op_flags & P_ULOCK) flags |= 0x0001;
 if (op_flags & P_LLOCK) flags |= 0x0002;
 if (op_flags & P_LOCKED) flags |= 0x0004;
 packet.flags = ShortInt(flags);

 packet.field_no = LongInt(field_no);
 memcpy(packet.id, id, id_len);
 
 if (!message_pair(SrvrReadv, (char *)&packet, id_len + 8))
  {
   server_error = SV_ON_ERROR;
  }

 process.status = remote_status;

 if (server_error == SV_OK)
  {
   /* Convert received data to a chunked string */
   rec_len = buff_bytes - offsetof(INBUFF, data.read.rec);
   ts_init(str, rec_len);
   ts_copy(buff->data.read.rec, rec_len);
   ts_terminate();
  }

 return server_error;
}

/* ======================================================================
   net_recordlocked()  -  Test record lock on remote system               */

int net_recordlocked(
   FILE_VAR * fvar,
   char * id,
   short int id_len)
{
 struct {
         short int fno;
         char id[MAX_ID_LEN];
        } ALIGN2 packet;


 host_index = fvar->access.net.host_index;

 packet.fno = ShortInt(fvar->access.net.file_no);
 memcpy(packet.id, id, id_len);

 message_pair(SrvrRecordlocked, (char *)&packet, id_len + 2);
 process.status = remote_status;
 return buff->data.recordlocked.status;
}

/* ======================================================================
   net_scanindex()  -  SELECTLEFT/SELECTRIGHT                             */

int net_scanindex(
   FILE_VAR * fvar,
   char * index_name,
   short int list_no,
   DESCRIPTOR * key_descr,  /* Null if not returning key */
   bool right)
{
 struct {
         short int fno;
         short int list_no;
         char index_name[MAX_AK_NAME_LEN];
        } ALIGN2 packet;
 int len;
 STRING_CHUNK * str = NULL;
 long int count = 0;
 char * p;

 host_index = fvar->access.net.host_index;

 packet.fno = ShortInt(fvar->access.net.file_no);
 packet.list_no = ShortInt(list_no);
 len = strlen(index_name);
 memcpy(packet.index_name, index_name, len);

 if (message_pair((right)?SrvrSelectRight:SrvrSelectLeft, (char *)&packet, len + 4))
  {
   if (key_descr != NULL)
    {
     len = buff_bytes - offsetof(INBUFF, data.selectleft.key);
     k_put_string(buff->data.selectleft.key, len, key_descr);
    }

   if (remote_status == 0)
    {
     /* Now retrieve the actual select list */

     if (message_pair(SrvrReadList, (char *)&packet.list_no, 2))
      {
       /* Convert received data to a chunked string */
       len = buff_bytes - offsetof(INBUFF, data.readlist.list);
       ts_init(&str, len);
       ts_copy(buff->data.readlist.list, len);
       ts_terminate();

       if (len)
        {
         count = 1;
         for(p = buff->data.readlist.list; len--; p++)
          {
           if (*p == FIELD_MARK) count++;
          }
        }
      }
    }
  }

 /* Save the list. The caller has already set the data types */

 SelectList(list_no)->data.str.saddr = str;
 SelectCount(list_no)->data.value = count;

 return remote_status;
}

/* ======================================================================
   net_select()  -  Select file, returning id list                        */

int net_select(
   FILE_VAR * fvar,
   STRING_CHUNK ** str,
   long int * count)
{
 struct {
         short int fno;
        } ALIGN2 packet;
 int len;
 char * p;

 host_index = fvar->access.net.host_index;

 packet.fno = ShortInt(fvar->access.net.file_no);

 if (!message_pair(SrvrSelectList, (char *)&packet, sizeof(packet))
     || (server_error == SV_ON_ERROR))
  {
   return SV_ON_ERROR;
  }

 /* Convert received data to a chunked string */
 len = buff_bytes - offsetof(INBUFF, data.selectlist.reply);
 ts_init(str, len);
 ts_copy(buff->data.selectlist.reply, len);
 ts_terminate();

 if (len)
  {
   *count = 1;
   for(p = buff->data.selectlist.reply; len--; p++)
    {
     if (*p == FIELD_MARK) (*count)++;
    }
  }
 else *count = 0;

 return SV_OK;
}

/* ======================================================================
   net_selectindex()  -  SELECTINDEX, no value                            */

int net_selectindex(
   FILE_VAR * fvar,
   char * index_name,
   STRING_CHUNK ** str)
{
 struct {
         short int fno;
         char index_name[MAX_AK_NAME_LEN];
        } ALIGN2 packet;
 int len;
 long int count;
 char * p;

 host_index = fvar->access.net.host_index;

 packet.fno = ShortInt(fvar->access.net.file_no);
 len = strlen(index_name);
 memcpy(packet.index_name, index_name, len);

 if (!message_pair(SrvrSelectIndexv, (char *)&packet, len + 2)
     || (server_error == SV_ON_ERROR))
  {
   return 0;
  }

 process.status = remote_status;
 if (server_error != SV_OK) return 0;

 /* Convert received data to a chunked string */
 len = buff_bytes - offsetof(INBUFF, data.selectlist.reply);
 ts_init(str, len);
 ts_copy(buff->data.selectlist.reply, len);
 ts_terminate();

 if (len)
  {
   count = 1;
   for(p = buff->data.selectlist.reply; len--; p++)
    {
     if (*p == FIELD_MARK) count++;
    }
  }
 else count = 0;

 return count;
}

/* ======================================================================
   net_selectindexv()  -  SELECTINDEX for specified value                 */

int net_selectindexv(
   FILE_VAR * fvar,
   char * index_name,
   char * value,
   STRING_CHUNK ** str)
{
 int len;
 long int count;
 char * p;
 struct PACKET {
                short int fno;
                short int index_name_len;
                char index_name[1];
               } ALIGN2;

 host_index = fvar->access.net.host_index;

 ((struct PACKET *)buff)->fno = ShortInt(fvar->access.net.file_no);

 len = strlen(index_name);
 ((struct PACKET *)buff)->index_name_len = ShortInt(len);
 memcpy(((struct PACKET *)buff)->index_name, index_name, len);
 p = (char *)(((struct PACKET *)buff)->index_name + len);
 len = strlen(value);
 memcpy(p, value, len);
 p += len;

 if (!message_pair(SrvrSelectIndexk, (char *)buff, (char *)p - (char *)buff)
     || (server_error == SV_ON_ERROR))
  {
   return 0;
  }

 process.status = remote_status;
 if (server_error != SV_OK) return 0;

 /* Convert received data to a chunked string */
 len = buff_bytes - offsetof(INBUFF, data.selectlist.reply);
 ts_init(str, len);
 ts_copy(buff->data.selectlist.reply, len);
 ts_terminate();

 if (len)
  {
   count = 1;
   for(p = buff->data.selectlist.reply; len--; p++)
    {
     if (*p == FIELD_MARK) count++;
    }
  }
 else count = 0;

 return count;
}

/* ======================================================================
   net_setindex()  -  SETLEFT/SETRIGHT                                    */

int net_setindex(
   FILE_VAR * fvar,
   char * index_name,
   bool right)
{
 struct {
         short int fno;
         char index_name[MAX_AK_NAME_LEN];
        } ALIGN2 packet;
 int len;

 host_index = fvar->access.net.host_index;

 packet.fno = ShortInt(fvar->access.net.file_no);
 len = strlen(index_name);
 memcpy(packet.index_name, index_name, len);

 message_pair((right)?SrvrSetRight:SrvrSetLeft, (char *)&packet, len + 2);

 return remote_status;
}

/* ======================================================================
   net_unlock()  -  Unlock a record on remote system                      */

int net_unlock(
   FILE_VAR * fvar,
   char * id,
   short int id_len)
{
 int status = SV_OK;
 struct {
         short int fno;
         char id[MAX_ID_LEN];
        } ALIGN2 packet;


 host_index = fvar->access.net.host_index;

 packet.fno = ShortInt(fvar->access.net.file_no);
 memcpy(packet.id, id, id_len);

 if (!message_pair(SrvrRelease, (char *)&packet, id_len + 2)
     || (server_error == SV_ON_ERROR))
  {
   status = SV_ON_ERROR;
  }

 process.status = remote_status;

 return status;
}

/* ======================================================================
   net_unlock_all()  -  Unlock all records on all remote systems          */

int net_unlock_all()
{
 struct {
         short int fno;
        } ALIGN2 packet;

 packet.fno = ShortInt(0);

 for(host_index = 0; host_index < MAX_HOSTS; host_index++)
  {
   if (host_table[host_index].ref_ct)
    {
     message_pair(SrvrRelease, (char *)&packet, 2);
    }
  }

 return SV_OK;
}

/* ======================================================================
   net_write()  -  Write a record via QMNet                               */

int net_write(
   FILE_VAR * fvar,
   char * id,
   short int id_len,
   STRING_CHUNK * str,
   bool keep_lock)
{
 short int mode;
 long int data_len;
 int bytes;
 char * p;
 INBUFF * q;
 struct PACKET {
                short int fno;
                short int id_len;
                char id[1];
               } ALIGN2;

 host_index = fvar->access.net.host_index;

 data_len = (str != NULL)?(str->string_len):0;

 /* Ensure buffer is big enough for this record */

 bytes = sizeof(struct PACKET) + id_len + data_len;
 if (bytes >= buff_size)     /* Must reallocate larger buffer */
  {
   bytes = (bytes + BUFF_INCR - 1) & ~BUFF_INCR;
   q = (INBUFF *)malloc(bytes);
   if (q == NULL)
    {
     process.status = -ER_MEM;
     return SV_ON_ERROR;
    }
   free(buff);
   buff = q;
  }

 /* Set up outgoing packet */

 ((struct PACKET *)buff)->fno = ShortInt(fvar->access.net.file_no);
 ((struct PACKET *)buff)->id_len = ShortInt(id_len);
 memcpy(((struct PACKET *)buff)->id, id, id_len);
 p = ((struct PACKET *)buff)->id + id_len;
 while(str != NULL)
  {
   memcpy(p, str->data, str->bytes);
   p += str->bytes;
   str = str->next;
  }

 mode = (keep_lock)?SrvrWriteu:SrvrWrite;
 if ((!message_pair(mode, (char *)buff, offsetof(struct PACKET, id) + id_len + data_len))
    || (server_error == SV_ON_ERROR))
  {
   return SV_ON_ERROR;
  }

 return SV_OK;
}

/* ======================================================================
   close_connection()                                                     */

Private void close_connection()
{
 (void)write_packet(SrvrQuit, NULL, 0);
 closesocket(host_table[host_index].sock);
 host_table[host_index].ref_ct = 0;
}

/* ======================================================================
   message_pair()  -  Send message and receive response                   */

Private bool message_pair(
   int type,
   char * data,
   long int bytes)
{
 if (write_packet(type, data, bytes))
  {
   return GetResponse();
  }

 return FALSE;
}

/* ====================================================================== */

Private bool GetResponse()
{
 if (!read_packet()) return FALSE;

 return (server_error != SV_ERROR);
}

/* ======================================================================
   read_packet()  -  Read a QM data packet                                */

Private bool read_packet()
{
 int rcvd_bytes;         /* Length of received packet fragment */
 long int packet_bytes;  /* Total length of incoming packet */
 int rcv_len;
 long int n;
 char * p;

 struct {
         long int packet_length;
         short int server_error ALIGN2;
         long int status ALIGN2;
        } in_packet_header;
 #define IN_PKT_HDR_BYTES 10

 /* Read packet header */ 

 p = (char *)&in_packet_header;
 buff_bytes = 0;
 do {
     rcv_len = IN_PKT_HDR_BYTES - buff_bytes;
     if ((rcvd_bytes = recv(host_table[host_index].sock, (char *)p, rcv_len, 0)) <= 0)
      {
       return FALSE;
      }
     buff_bytes += rcvd_bytes;
     p += rcvd_bytes;
    } while(buff_bytes < IN_PKT_HDR_BYTES);

 /* Calculate remaining bytes to read */

 packet_bytes = LongInt(in_packet_header.packet_length) - IN_PKT_HDR_BYTES;

 if (packet_bytes >= buff_size)      /* Must reallocate larger buffer */
  {
   free(buff);
   n = (packet_bytes + BUFF_INCR) & ~(BUFF_INCR - 1);
   buff = (INBUFF *)malloc(n);
   if (buff == NULL) return FALSE;
   buff_size = n;
  }

 /* Read data part of packet */

 p = (char *)buff;
 buff_bytes = 0;
 while(buff_bytes < packet_bytes)
  {
   rcv_len = min(buff_size - buff_bytes, 16384);
   if ((rcvd_bytes = recv(host_table[host_index].sock, (char *)p, rcv_len, 0)) <= 0)
    {
     return FALSE;
    }

   buff_bytes += rcvd_bytes;
   p += rcvd_bytes;
  }

 ((char *)buff)[buff_bytes] = '\0';

 server_error = ShortInt(in_packet_header.server_error);
 remote_status = LongInt(in_packet_header.status);

 return TRUE;
}

/* ======================================================================
   write_packet()  -  Send QM data packet                                 */

Private bool write_packet(int type, char * data, long int bytes)
{
 struct {
         long int length;
         short int type;
        } packet_header;
#define PKT_HDR_BYTES 6
 int bytes_sent;

 packet_header.length = LongInt(bytes + PKT_HDR_BYTES);   /* 0272 */
 packet_header.type = ShortInt(type);

 if (send(host_table[host_index].sock, (char *)&packet_header, PKT_HDR_BYTES, 0) != PKT_HDR_BYTES)
  {
   return FALSE;
  }

 if (data != NULL)
  {
   while(bytes > 0)
    {
     if ((bytes_sent = send(host_table[host_index].sock, data, bytes, 0)) < 0)
      {
       return FALSE;
      }

     data += bytes_sent;
     bytes -= bytes_sent;
    }
  }

 return TRUE;
}

/* ======================================================================
   get_qmnet_connections() - Return list of open connections              */

STRING_CHUNK * get_qmnet_connections()
{
 STRING_CHUNK * str;
 int i;

 if (buff == NULL) return NULL;  /* Nothing opened yet */

 ts_init(&str, 128);
 str = NULL;

 for(i = 0; i < MAX_HOSTS; i++)
  {
   if (host_table[i].ref_ct)
    {
     if (str != NULL) ts_copy_byte(FIELD_MARK);
     ts_printf("%s\xFD%d", host_table[i].server_name, host_table[i].ref_ct);
    }
  }

 ts_terminate();
 return str;
}

/* END-CODE */
