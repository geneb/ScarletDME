/* OP_SKT.C
 * Socket interface.
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
 * 18Sep25 gwb Git Issue #89: Prevent SIGPIPE in op_writeskt() if the peer drops the connection.
 * 
 * START-HISTORY (OpenQM):
 * 10 Apr 09 gcb Added SO_REUSEADDR socket option to op_srvrskt() so that the
 *               network subsystem will allow rebinding during TIME_WAIT. 
 *
 * 10 Apr 09 gcb Added SKT$INFO.FAMILY for SOCKET.INFO so that you can determ
 *               ine which INET address family a socket was configured for. 
 *
 * 10 Apr 09 gwb Added code to validate passed in IP addresses.  Includes code
 *               to supress a DNS lookup by getaddrinfo() if the passed server
 *               address is a validly formed ip address.
 *               Fixed a bug that would cause invalid ip addresses to seg fault
 *               OpenQM due to freeing a null pointer.
 *
 * 09 Apr 09 gwb Re-wrote op_openskt().  Should now transparently handle IPv6.
 *               It should also be compatible with 2.8-10.
 *               Re-wrote op_acceptskt(), op_srvraddr().  Should now transparently handle IPv6.
 *
 * 03 Apr 09 gwb op_srvsckt() has pretty much been completely re-written.  It now
 *               supports creating IPv4 or IPv6 connections transparently.
 *
 * 30 Mar 09 gwb Added code to allow specification of transport & protocol type
 *               when creating new server sockets.  This was done in order to make the
 *               GPL version work like the commercial version.
 *               This applies to the op_srvsckt() routine for now.  op_openskt() is being
 *               examined.
 *
 * 13 Mar 09    eliminate some compiler warnings about pointer signedness
 *              due to the third argument of the accept function being
 *              an int instead of a socklen_t.
 * 18 Oct 07  2.6-5 Test in op_openskt() and op_srvrskt() for IP address was
 *                  inadequate.
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 02 Apr 07  2.5-1 0547 op_srvraddr should allow address as source.
 * 13 Nov 06  2.4-16 Added SKT$INFO.OPEN key to op_sktinfo().
 * 22 Dec 05  2.3-3 Added op_srvraddr().
 * 28 Jul 05  2.2-6 Added SET.SOCKET.MODE() and extended SOCKET.INFO().
 * 30 Jun 05  2.2-3 New module.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * This release does not support SSL. The context argument is always passed
 * in as integer zero.
 *
 * skt = ACCEPT.SOCKET.CONNECTION(srvr.skt, timeout)
 *   timeout = max wait time (mS), zero for infinite
 *
 *
 * CLOSE.SOCKET skt
 *
 *
 * srvr.skt = CREATE.SERVER.SOCKET(addr, port, protocol_options})
 *   addr = address to listen on. Leave blank for any local address.
 *   See function all for protocol options....
 *
 * skt = OPEN.SOCKET(addr, port, flags{, context})
 *   Flags:
 *      0x0001 = SKT$BLOCKING        Blocking
 *      0x0002 = SKT$NON.BLOCKING    Non-Blocking (default)
 *
 *
 * var = READ.SOCKET(skt, max.len, flags, timeout)
 *   Flags:
 *      0x0001 = SKT$BLOCKING        Blocking     } If neither, uses socket
 *      0x0002 = SKT$NON.BLOCKING    Non-Blocking } default from open.
 *   timeout = max wait time (mS), zero for infinite
 *     
 *
 * var = SET.SOCKET.MODE(skt, key, value)
 *   Keys:
 *      SKT$INFO.BLOCKING   Default blocking mode
 *      SKT$INFO.NO.DELAY   Nagle algorithm disabled?
 *
 * var = SOCKET.INFO(skt, key)
 *   Keys:
 *      SKT$INFO.OPEN       Is this a socket variable?
 *      SKT$INFO.TYPE       Socket type (Server, incoming, outgoing)
 *      SKT$INFO.PORT       Port number
 *      SKT$INFO.IP.ADDR    IP address
 *      SKT$INFO.BLOCKING   Default blocking mode
 *      SKT$INFO.NO.DELAY   Nagle algorithm disabled?
 *      SKT$INFO.FAMILY     Network family: INET, INET6, UDP
 *
 * var = SERVER.ADDR(name)
 *
 * bytes = WRITE.SOCKET(skt, data, flags, timeout)
 *  Flags:
 *      0x0001 = SKT$BLOCKING        Blocking     } If neither, uses socket
 *      0x0002 = SKT$NON.BLOCKING    Non-Blocking } default from open.
 *      0x4000 = SKT$MSG.NOSIGNAL    Don't throw SIGPIPE when peer disconnects.
 * 
 *   timeout = max wait time (mS), zero for infinite
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include "keys.h"
#include "qmnet.h"

#include <sys/time.h>
#include <sys/wait.h>
#include <signal.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#define SOCKET_DEBUG

Private char* skt_buff = NULL;
Private int skt_buff_size = 0;

bool socket_wait(SOCKET socket, bool read, int timeout);

int translate_sockerr(int errval) {
  /* simply translates Posix error constants to OpenQM error
     * constants.
     */

  int retval = 0;

  switch (errval) {
    case EAI_AGAIN:
      retval = ERAI_AGAIN;
      break;
    case EAI_BADFLAGS:
      retval = ERAI_BADFLAGS;
      break;
    case EAI_FAIL:
      retval = ERAI_FAIL;
      break;
    case EAI_FAMILY:
      retval = ERAI_FAMILY;
      break;
    case EAI_MEMORY:
      retval = ERAI_MEMORY;
      break;
    case EAI_NONAME:
      retval = ERAI_NONAME;
      break;
    case EAI_SERVICE:
      retval = ERAI_SERVICE;
      break;
    case EAI_SOCKTYPE:
      retval = ERAI_SOCKTYPE;
      break;
    case EAI_SYSTEM:
      retval = ERAI_SYSTEM;
      break;
    default:
      retval = ER_RESOLVE; /* for lack of anything better to use here... */
  }
  return retval;
}

/* ======================================================================
   op_accptskt()  -  ACCEPT.SOCKET.CONNECTION                             */

void op_accptskt() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top | Timeout period              | Socket                      |
     |-----------------------------|-----------------------------|
     | Socket reference            |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;
  int timeout;
  SOCKET srvr_skt;
  SOCKET skt;
  SOCKVAR* sockvar;
  DESCRIPTOR result_descr;
  SOCKVAR* sock;
  socklen_t socklen;
  struct sockaddr_storage sinRemote;

  process.status = 0;

  InitDescr(&result_descr, INTEGER);
  result_descr.data.value = 0;

  /* Get timeout */

  descr = e_stack - 1;
  GetInt(descr);
  timeout = descr->data.value;
  if (timeout == 0)
    timeout = -1;

  /* Get socket reference */

  descr = e_stack - 2;
  while (descr->type == ADDR)
    descr = descr->data.d_addr;

  if (descr->type != SOCK)
    k_not_socket(descr);

  sockvar = descr->data.sock;
  srvr_skt = sockvar->socket_handle;

  /* Wait for connection */

  if (!socket_wait(srvr_skt, TRUE, timeout))
    goto exit_op_accptskt;

  socklen = sizeof sinRemote;

  skt = accept(srvr_skt, (struct sockaddr*)&sinRemote, &socklen);

  /* Create socket descriptor and SOCKVAR structure */

  sock = (SOCKVAR*)k_alloc(100, sizeof(SOCKVAR));
  sock->ref_ct = 1;
  sock->socket_handle = (int)skt;
  sock->flags = SKT_INCOMING;
  sock->family = sockvar->family;

  switch (sock->family) {
    case PF_INET:
      socklen = sizeof(sinRemote);
      getpeername(skt, (struct sockaddr*)&sinRemote, &socklen);
      struct sockaddr_in* s = (struct sockaddr_in*)&sinRemote;
      sock->port = ntohs(s->sin_port);
      if (inet_ntop(AF_INET, &s->sin_addr, sock->ip_addr, INET_ADDRSTRLEN) ==
          NULL) {
        process.status = ER_BADADDR;
      }
      break;
    case PF_INET6:
      socklen = sizeof(sinRemote);
      getpeername(skt, (struct sockaddr*)&sinRemote, &socklen);
      struct sockaddr_in6* s6 = (struct sockaddr_in6*)&sinRemote;
      sock->port = ntohs(s6->sin6_port);
      if (inet_ntop(AF_INET6, &s6->sin6_addr, sock->ip_addr,
                    INET6_ADDRSTRLEN) == NULL) {
        process.status = ER_BADADDR;
      }
      break;
  }
  if (process.status == 0) {
    InitDescr(&result_descr, SOCK);
    result_descr.data.sock = sock;
  }

exit_op_accptskt:
  k_pop(1);
  k_dismiss();

  *(e_stack++) = result_descr;
}

/* ======================================================================
   op_closeskt()  -  CLOSE.SOCKET                                         */

void op_closeskt() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top | Socket reference            |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;

  descr = e_stack - 1;
  while (descr->type == ADDR)
    descr = descr->data.d_addr;

  if (descr->type != SOCK)
    k_not_socket(descr);

  k_release(descr); /* This will close the socket */
  k_pop(1);
}

/* ======================================================================
   op_openskt()  -  OPEN.SOCKET                                           */

void op_openskt() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top | Security context or zero    | Socket                      |
     |-----------------------------|-----------------------------|
     | Flags                       |                             |
     |-----------------------------|-----------------------------|
     | Port                        |                             |
     |-----------------------------|-----------------------------|
     | Server address              |                             |
     |=============================|=============================|

 Returns zero if fails.
 Caller can test with socket variable or check STATUS() value
 */

  DESCRIPTOR* descr;
  int flags;
  int port;
  char server[80 + 1];
  char server_addr[80];

  SOCKET skt;
  DESCRIPTOR result_descr;
  SOCKVAR* sock;

  struct addrinfo hint, *res;
  char port_name[30];
  int err_info = 0;

  process.status = 0;
  InitDescr(&result_descr, INTEGER);
  result_descr.data.value = 0;

  /* Get flags */

  descr = e_stack - 2;
  GetInt(descr);
  flags = descr->data.value;

  /* Get port */

  descr = e_stack - 3;
  GetInt(descr);
  port = descr->data.value;

  /* Get server address */

  descr = e_stack - 4;
  if (k_get_c_string(descr, server, 80) <= 0) {
    process.status = ER_BAD_NAME;
  }

  if (process.status == 0) {
    hint.ai_flags = 0; /* we're using connect(), not bind() */

    memset(&hint, 0, sizeof(hint));
    memset(&port_name, 0, sizeof(port_name));
    memset(&server_addr, 0, sizeof(server_addr));
    char tempserver[80] = "";
    if (inet_pton(AF_INET, server, &tempserver) == 1) {
      hint.ai_family = AF_INET;
      hint.ai_flags |= AI_NUMERICHOST;
    } else {
      if (inet_pton(AF_INET6, server, &tempserver) == 1) {
        hint.ai_family = AF_INET6;
        hint.ai_flags |= AI_NUMERICHOST;
      } else {
        /* might be a hostname */
        hint.ai_family = AF_UNSPEC;
      }
    }

    snprintf(port_name, sizeof(port_name), "%u", port);

    /* if anyone knows of a better way to handle this, I'm game... */

    if (flags == 0) { /* SKT$STREAM + SKT$TCP */
      hint.ai_socktype = SOCK_STREAM;
      hint.ai_protocol = IPPROTO_TCP;
    }

    if (flags & SKT_UDP) {
      hint.ai_socktype = SOCK_DGRAM;
      hint.ai_protocol = IPPROTO_UDP;
    }
    if (flags & SKT_RAW) {
      hint.ai_socktype = SOCK_RAW;
    }
    if (flags & SKT_ICMP) {
      hint.ai_socktype = SOCK_RAW;
      hint.ai_protocol = 0;
    }
    if ((flags & SKT_STREAM) && (flags & SKT_UDP)) {
      process.status = ER_INCOMP_PROTO;
    }
    if ((flags & SKT_DGRM) && (flags & SKT_TCP)) {
      process.status = ER_INCOMP_PROTO;
    }
  }
  if (process.status == 0) {
    err_info = getaddrinfo(server, port_name, &hint, &res);
    if (err_info) {
      process.status = translate_sockerr(err_info);
      process.os_error = NetError;
    }
  }
  if (process.status == 0) {
    skt = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (skt < 0) {
      process.status = ER_NOSOCKET;
      process.os_error = NetError;
    } else {
      if (flags & SKT_NON_BLOCKING) {
        if (fcntl(skt, F_SETFL, O_NONBLOCK) == -1) {
          process.status = ER_NONBLOCK_FAIL;
          process.os_error = NetError;
        }
      }
    }
  }
  if (process.status == 0) {
    if (connect(skt, res->ai_addr, res->ai_addrlen) == -1) {
      if (errno == EINPROGRESS) {
        struct timeval tv;
        fd_set writefds;
        tv.tv_sec = DEFAULT_CONNECT_TIMEOUT;
        tv.tv_usec = 0;
        FD_ZERO(&writefds);
        FD_SET(skt, &writefds);
        if (select(skt + 1, NULL, &writefds, NULL, &tv) != -1) {
          if (connect(skt, res->ai_addr, res->ai_addrlen) == -1) {
            process.status = ER_CONNECT;
#ifdef SOCKET_DEBUG
            k_error("!Failed to connect: %s\n", strerror(NetError));
#endif
            process.os_error = NetError;
          }
        } else {
          process.status = ER_CONNECT;
#ifdef SOCKET_DEBUG
          k_error("!Failed to connect after select() delay: %s\n",
                  strerror(NetError));
#endif
          process.os_error = NetError;
        }
      } else {
        process.status = ER_CONNECT;
#ifdef SOCKET_DEBUG
        k_error("!Failed to connect: %d: %s\n", NetError, strerror(NetError));
#endif
        process.os_error = NetError;
      }
    }
  }

  /* Create socket descriptor and SOCKVAR structure */

  if (process.status == 0) {
    sock = (SOCKVAR*)k_alloc(97, sizeof(SOCKVAR));
    sock->ref_ct = 1;
    sock->socket_handle = (unsigned int)skt;
    sock->family = res->ai_family;
    sock->flags = flags & SKT_USER_MASK;
    struct sockaddr_in const* sin;
    struct sockaddr_in6 const* sin6;
    switch (res->ai_family) {
      case PF_INET:
        sin = (struct sockaddr_in*)res->ai_addr;
        if (inet_ntop(AF_INET, &sin->sin_addr, server_addr, INET_ADDRSTRLEN) ==
            NULL) {
          process.status = ER_BADADDR;
        }
        break;

      case PF_INET6:
        sin6 = (struct sockaddr_in6*)res->ai_addr;
        if (inet_ntop(AF_INET6, &sin6->sin6_addr, server_addr,
                      INET6_ADDRSTRLEN) == NULL) {
          process.status = ER_BADADDR;
        }
        break;

      default:
        strcpy(server_addr, "Unknown Address Family!");
    }

    if (process.status == 0) {
      strcpy(sock->ip_addr, server_addr);
      sock->port = port;
      InitDescr(&result_descr, SOCK);
      result_descr.data.sock = sock;
    }
  }

  if (process.status == 0)
    freeaddrinfo(res);

  // exit_open_socket:
  k_dismiss();
  k_pop(2);
  k_dismiss();

  *(e_stack++) = result_descr;
}

/* ======================================================================
   op_readskt()  -  READ.SOCKET                                           */

void op_readskt() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top | Timeout period              | Data                        |
     |-----------------------------|-----------------------------|
     | Flags                       |                             |
     |-----------------------------|-----------------------------|
     | Max len                     |                             |
     |-----------------------------|-----------------------------|
     | Socket                      |                             |
     |=============================|=============================|

 */

  DESCRIPTOR* descr;
  int timeout;
  int max_len;
  int flags;
  SOCKVAR* sock;
  bool blocking;
  int bytes;
  int rcvd_bytes;
  STRING_CHUNK* head = NULL;
  SOCKET skt;

  process.status = 0;

  /* Get timeout period */

  descr = e_stack - 1;
  GetInt(descr);
  timeout = descr->data.value;
  if (timeout == 0)
    timeout = -1;

  /* Get flags */

  descr = e_stack - 2;
  GetInt(descr);
  flags = descr->data.value;

  /* Get max len */

  descr = e_stack - 3;
  GetInt(descr);
  max_len = descr->data.value;

  if (skt_buff_size < max_len) {
    bytes = (max_len + 1023) & ~1023; /* Round up to 1k multiple */
    if (skt_buff != NULL) {
      k_free(skt_buff);
      skt_buff_size = 0;
    }

    skt_buff = (char*)k_alloc(98, bytes);
    if (skt_buff == NULL) {
      process.status = ER_MEM;
      goto exit_op_readskt;
    }

    skt_buff_size = bytes;
  }

  /* Get socket */

  descr = e_stack - 4;
  k_get_value(descr);
  if (descr->type != SOCK)
    k_not_socket(descr);
  sock = descr->data.sock;
  skt = sock->socket_handle;

  /* Determine blocking mode for this read */

  if (flags & SKT_BLOCKING)
    blocking = TRUE;
  else if (flags & SKT_NON_BLOCKING)
    blocking = FALSE;
  else
    blocking = ((sock->flags & SKT_BLOCKING) != 0);

  /* Wait for data to arrive */

  if (!socket_wait(skt, TRUE, (blocking) ? timeout : 0))
    goto exit_op_readskt;

  /* Read the data */

  ts_init(&head, max_len);

  rcvd_bytes = recv(skt, skt_buff, max_len, 0);
  if (rcvd_bytes <= 0) /* Lost connection */
  {
    process.status = (rcvd_bytes == 0) ? ER_SKT_CLOSED : ER_FAILED;
    process.os_error = NetError;
    goto exit_op_readskt;
  }

  ts_copy(skt_buff, rcvd_bytes);
  ts_terminate();

exit_op_readskt:
  k_pop(3);
  k_dismiss();

  InitDescr(e_stack, STRING);
  (e_stack++)->data.str.saddr = head;
}

/* ======================================================================
   op_setskt()  -  SET.SOCKET.MODE()                                      */

void op_setskt() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top | Qualifier                   | 1 = success, 0 = failure    |
     |-----------------------------|-----------------------------|
     | Action key                  |                             |
     |-----------------------------|-----------------------------|
     | Socket reference            |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;
  SOCKVAR* sockvar;
  int key;
  int n;

  process.status = 0;

  /* Get action key */

  descr = e_stack - 2;
  GetInt(descr);
  key = descr->data.value;

  /* Get socket reference */

  descr = e_stack - 3;
  while (descr->type == ADDR)
    descr = descr->data.d_addr;
  if (descr->type != SOCK)
    k_not_socket(descr);
  sockvar = descr->data.sock;

  descr = e_stack - 1; /* Qualifier */

  switch (key) {
    case SKT_INFO_BLOCKING:
      GetInt(descr);
      if (descr->data.value)
        sockvar->flags |= SKT_BLOCKING;
      else
        sockvar->flags &= ~SKT_BLOCKING;
      break;

    case SKT_INFO_NO_DELAY:
      GetInt(descr);
      n = (descr->data.value != 0);
      setsockopt(sockvar->socket_handle, IPPROTO_TCP, TCP_NODELAY, (char*)&n,
                 sizeof(int));
      break;

    case SKT_INFO_KEEP_ALIVE:
      GetInt(descr);
      n = (descr->data.value != 0);
      n = TRUE;
      setsockopt(sockvar->socket_handle, SOL_SOCKET, SO_KEEPALIVE, (char*)&n,
                 sizeof(int));
      break;

    default:
      process.status = ER_BAD_KEY;
      break;
  }

  k_dismiss();
  k_dismiss();
  k_dismiss();

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = (process.status == 0);
}

/* ======================================================================
   op_sktinfo()  -  SOCKET.INFO()                                         */

void op_sktinfo() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top | Key                         | Returned information        |
     |-----------------------------|-----------------------------|
     | Socket reference            |                             |
     |=============================|=============================|
 */

  DESCRIPTOR* descr;
  SOCKVAR* sockvar;
  DESCRIPTOR result_descr;
  int key;
  int n;
  socklen_t socklen;

  InitDescr(&result_descr, INTEGER);
  result_descr.data.value = 0;

  /* Get action key */

  descr = e_stack - 1;
  GetInt(descr);
  key = descr->data.value;

  /* Get socket reference */

  descr = e_stack - 2;
  while (descr->type == ADDR)
    descr = descr->data.d_addr;
  if ((descr->type != SOCK) && (key != SKT_INFO_OPEN))
    k_not_socket(descr);
  sockvar = descr->data.sock;

  switch (key) {
    case SKT_INFO_OPEN:
      result_descr.data.value = (descr->type == SOCK);
      break;

    case SKT_INFO_TYPE:
      if (sockvar->flags & SKT_SERVER)
        result_descr.data.value = SKT_INFO_TYPE_SERVER;
      else if (sockvar->flags & SKT_INCOMING)
        result_descr.data.value = SKT_INFO_TYPE_INCOMING;
      else
        result_descr.data.value = SKT_INFO_TYPE_OUTGOING;
      break;

    case SKT_INFO_PORT:
      result_descr.data.value = sockvar->port;
      break;

    case SKT_INFO_IP_ADDR:
      k_put_c_string(sockvar->ip_addr, &result_descr);
      break;

    case SKT_INFO_BLOCKING:
      result_descr.data.value = ((sockvar->flags & SKT_BLOCKING) != 0);
      break;

    case SKT_INFO_NO_DELAY:
      n = 0;
      socklen = sizeof(int);
      getsockopt(sockvar->socket_handle, IPPROTO_TCP, TCP_NODELAY, (char*)&n,
                 &socklen);
      result_descr.data.value = (n != 0);
      break;

    case SKT_INFO_KEEP_ALIVE:
      n = 0;
      socklen = sizeof(int);
      getsockopt(sockvar->socket_handle, SOL_SOCKET, SO_KEEPALIVE, (char*)&n,
                 &socklen);
      result_descr.data.value = (n != 0);
      break;

    case SKT_INFO_FAMILY:
      switch (sockvar->family) {
        case PF_INET:
          result_descr.data.value = SKT_INFO_FAMILY_IPV4;
          break;
        case PF_INET6:
          result_descr.data.value = SKT_INFO_FAMILY_IPV6;
          break;
      }
  }

  k_pop(1);
  k_dismiss();

  *(e_stack++) = result_descr;
}

/* ======================================================================
   op_srvraddr()  -  SERVER.ADDR()                                        */

void op_srvraddr() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top | Server name                 | Sever address or null       |
     |=============================|=============================|

 */

  DESCRIPTOR* descr;
  char hostname[64 + 1];
  char ip_addr[40] = "";

  struct addrinfo hint, *res;

  process.status = 0;

  descr = e_stack - 1;
  if (k_get_c_string(descr, hostname, 64) <= 0) {
    process.status = ER_BAD_NAME;
  }

  if (process.status == 0) {
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_UNSPEC;

    if (getaddrinfo(hostname, "", &hint, &res)) {
      process.status = ER_SERVER;
    } else {
      struct sockaddr_in* sin;
      struct sockaddr_in6* sin6;
      switch (res->ai_family) {
        case PF_INET:
          sin = (struct sockaddr_in*)res->ai_addr;
          if (inet_ntop(AF_INET, &sin->sin_addr, ip_addr, INET_ADDRSTRLEN) ==
              NULL) {
            process.status = ER_BADADDR;
          }
          break;
        case PF_INET6:
          sin6 = (struct sockaddr_in6*)res->ai_addr;
          if (inet_ntop(AF_INET6, &sin6->sin6_addr, ip_addr,
                        INET6_ADDRSTRLEN) == NULL) {
            process.status = ER_BADADDR;
          }
          break;
        default:
          strcpy(ip_addr, "Unknown Address Family!");
      }
    }

    freeaddrinfo(res);
  }

  k_dismiss();
  k_put_c_string(ip_addr, e_stack++);
}

/* ======================================================================
   op_srvrskt()  -  CREATE.SERVER.SOCKET                                  */

void op_srvrskt() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top | Security context or zero    | Socket                      |
     |-----------------------------|-----------------------------|
     | Flags                       |                             |
     |-----------------------------|-----------------------------|
     | Port                        |                             |
     |-----------------------------|-----------------------------|
     | Server Address              |                             |
     |=============================|=============================|

 Returns zero if fails.
 Caller can test socket variable or check STATUS() value
---------------------------------------------------------------
     This is going to be the poster child function for how BASIC functions are 
     called in OpenQM.
     First of all, the parmeters are processed in the reverse order that 
     they're specified in the calling function.  In this case, the 
     caller looks like this:

     CREATE.SERVER.SOCKET(addr, port, flags)

     The top of the stack is where the return value lives.  It can
     be directly referenced as "e_stack - 1".

     The Flags paremter is at e_stack - 2.
     The Port parameter is at e_stack - 3.
     The Server Address parameter is at e_stack - 4.

     To see how the BASIC compiler works with this, open GPL.BP BCOMP
     and search for the label "in.create.socket.server".

     Through the use of k_dismiss() and k_pop(n), the result is returned
     in e_stack -1.  I'm not quite sure what the rules are for the correct
     use of k_dismiss() and k_pop(n) are yet.... 
    
 */

  DESCRIPTOR* descr;
  SOCKVAR* sock;
  SOCKET skt;
  int port;
  int flags;
  int optval;

  char server[80 + 1];
  char server_addr[80];

  DESCRIPTOR result_descr;

  /* for getaddrinfo() call... */
  struct addrinfo hint, *res;
  char port_name[30];
  int err_info = 0;

  process.status = 0;

  InitDescr(&result_descr, INTEGER);
  result_descr.data.value = 0;

  /* Get flags */

  descr = e_stack - 2;
  GetInt(descr);
  flags = descr->data.value;

  /* Get port */

  descr = e_stack - 3;
  GetInt(descr);
  port = descr->data.value;

  /* get server address */
  descr = e_stack - 4;
  if (k_get_c_string(descr, server, 80) < 0) {
    process.status = ER_BAD_NAME;
    goto exit_srvrskt;
  }

  memset(&hint, 0, sizeof(hint));
  memset(&port_name, 0, sizeof(port_name));
  memset(&server_addr, 0, sizeof(server_addr));

  hint.ai_flags = AI_PASSIVE; /* we're going to bind a socket... */

  char tempserver[80] = "";
  if (inet_pton(AF_INET, server, &tempserver) == 1) {
    hint.ai_family = AF_INET;
    hint.ai_flags |= AI_NUMERICHOST;
  } else {
    if (inet_pton(AF_INET6, server, &tempserver) == 1) {
      hint.ai_family = AF_INET6;
      hint.ai_flags |= AI_NUMERICHOST;
    } else {
      /* might be a hostname */
      hint.ai_family = AF_UNSPEC;
    }
  }

  snprintf(port_name, sizeof(port_name), "%u", port);
  if (flags == 0) { /* SKT$TCP */
    hint.ai_socktype = SOCK_STREAM;
  }

  if (flags & SKT_UDP) { /* SKT$UDP */
    hint.ai_socktype =
        SOCK_DGRAM; /* for the moment we're going to force datagram for UDP sockets... */
  }

  if (flags & SKT_ICMP) {
    hint.ai_socktype =
        SOCK_RAW; /* for the moment we're going to force raw sockets for ICMP sockets... */
  }

  /* the getaddrinfo() call sets the address family member (res->ai_family) 
     * so we don't need to tell the system beforehand that we're using IPv4 or
     * IPv6.  It'll figure it out based on the ip address or name resolution that
     * it does.
     */

  err_info = getaddrinfo(server, port_name, &hint, &res);

  if (err_info != 0) {
    /* we failed.. */
    process.status = translate_sockerr(err_info);
    process.os_error = NetError;
  }

  if (process.status == 0) {
    skt = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    /* Enable address reuse to avoid re-binding issues - gcb Mar 10 2009 */
    optval = 1;
    setsockopt(skt, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
    //

    if (skt < 0) {
      process.status = ER_NOSOCKET;
      process.os_error = NetError;
    } else {
      if (flags & SKT_NON_BLOCKING) {
        if (fcntl(skt, F_SETFL, O_NONBLOCK) == -1) {
          process.status = ER_NONBLOCK_FAIL;
          process.os_error = NetError;
        }
      }
    }
  }
  if (process.status == 0) {
    int bind_result = 0;
    bind_result = bind(skt, res->ai_addr, res->ai_addrlen);
    if (bind_result < 0) {
      process.status = ER_BIND;
      process.os_error = NetError;
    } else {
      listen(skt, SOMAXCONN);
      /* Create socket descriptor and SOCKVAR structure */

      sock = (SOCKVAR*)k_alloc(99, sizeof(SOCKVAR));
      sock->ref_ct = 1;
      sock->socket_handle = (int)skt;
      sock->family = res->ai_family;
      sock->flags = SKT_SERVER;

      struct sockaddr_in const* sin;
      struct sockaddr_in6 const* sin6;

      switch (res->ai_family) {
        case PF_INET:
          sin = (struct sockaddr_in*)res->ai_addr;
          if (inet_ntop(AF_INET, &sin->sin_addr, server_addr,
                        INET_ADDRSTRLEN) == NULL) {
            process.status = ER_BADADDR;
          }
          break;
        case PF_INET6:
          sin6 = (struct sockaddr_in6*)res->ai_addr;
          if (inet_ntop(AF_INET6, &sin6->sin6_addr, server_addr,
                        INET6_ADDRSTRLEN) == NULL) {
            process.status = ER_BADADDR;
          }
          break;
        default:
          strcpy(server_addr, "Unknown Address Family!");
      }
      if (process.status == 0) {
        strcpy(sock->ip_addr, server_addr);
        sock->port = port;
        InitDescr(&result_descr, SOCK);
        result_descr.data.sock = sock;
      }
    }
  }

  if (process.status == 0)
    freeaddrinfo(res);

exit_srvrskt:

  k_dismiss();
  k_pop(
      2); /* this was set to 1 for the old routine.  I'm still not quite sure how these work,
                  but I'm getting there....*/
  k_dismiss();

  *(e_stack++) = result_descr;
}

/* ======================================================================
   op_writeskt()  -  WRITE.SOCKET                                         */

void op_writeskt() {
  /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top | Timeout period              | Bytes written               |
     |-----------------------------|-----------------------------|
     | Flags                       |                             |
     |-----------------------------|-----------------------------|
     | Data                        |                             |
     |-----------------------------|-----------------------------|
     | Socket                      |                             |
     |=============================|=============================|

 */

  DESCRIPTOR* descr;
  int timeout;
  int flags;
  SOCKVAR* sock;
  bool blocking;
  int bytes;
  int bytes_sent;
  int total_bytes = 0;
  STRING_CHUNK* str;
  SOCKET skt;
  char* p;

  process.status = 0;

  /* Get timeout period */

  descr = e_stack - 1;
  GetInt(descr);
  timeout = descr->data.value;
  if (timeout == 0)
    timeout = -1;

  /* Get flags */

  descr = e_stack - 2;
  GetInt(descr);
  flags = descr->data.value;

  /* Get data to write */

  descr = e_stack - 3;
  GetString(descr);
  str = descr->data.str.saddr;

  /* Get socket */

  descr = e_stack - 4;
  k_get_value(descr);
  if (descr->type != SOCK)
    k_not_socket(descr);
  sock = descr->data.sock;
  skt = sock->socket_handle;

  if (str == NULL)
    goto exit_op_writeskt;

  /* Determine blocking mode for this write */

  if (flags & SKT_BLOCKING)
    blocking = TRUE;
  else if (flags & SKT_NON_BLOCKING)
    blocking = FALSE;
  else
    blocking = ((sock->flags & SKT_BLOCKING) != 0);

  /* Write the data */

  while (str != NULL) {
    p = str->data;
    bytes = str->bytes;
    do {

      if (!socket_wait(skt, FALSE, (blocking) ? timeout : 0))
        goto exit_op_writeskt;

      bytes_sent = send(skt, p, bytes, MSG_NOSIGNAL); /* git issue #89 */
      if (bytes_sent < 0) {  /* Lost connection */
        process.status = ER_FAILED;
        process.os_error = NetError;
        goto exit_op_writeskt;
      }

      bytes -= bytes_sent;
      total_bytes += bytes_sent;
      p += bytes_sent;

    } while (bytes);

    str = str->next;
  }

exit_op_writeskt:
  k_pop(2);
  k_dismiss();
  k_dismiss();

  InitDescr(e_stack, INTEGER);
  (e_stack++)->data.value = total_bytes;
}

/* ====================================================================== */

void close_skt(SOCKVAR* sock) {
  closesocket((SOCKET)(sock->socket_handle));
}

/* ======================================================================
   socket_wait()                                                          */

bool socket_wait(SOCKET skt,
                 bool read, /* Read mode? */
                 int timeout) {
  fd_set socket_set;
  fd_set wait_set;
  struct timeval tm;
  sigset_t sigset;

  /* The select() call on Linux systems hangs if the SIGINT signal is
    received while in the function. Set up a mask to allow blocking
    of this signal during the select().                              */

  sigemptyset(&sigset);
  sigaddset(&sigset, SIGINT);

  /* If timeout < 0 (infinite wait), we actually wait in one second
    steps, looking for events each time we wake up.                   */

  if (timeout >= 0) {
    tm.tv_usec = (timeout % 1000) * 1000; /* Fractional seconds and... */
    timeout /= 1000;                      /* ...whole seconds */
    tm.tv_sec = (timeout > 0);
  } else {
    timeout = 2147482647; /* A long time! */
    tm.tv_sec = 1;
    tm.tv_usec = 0;
  }

  FD_ZERO(&socket_set);
  FD_SET(skt, &socket_set);

  while (1) {
    wait_set = socket_set;

    sigprocmask(SIG_BLOCK, &sigset, NULL);

    if (select(FD_SETSIZE, (read) ? (&wait_set) : NULL,
               (read) ? NULL : (&wait_set), NULL, &tm) != 0)
      break;

    sigprocmask(SIG_UNBLOCK, &sigset, NULL);

    if (--timeout <= 0) {
      process.status = ER_TIMEOUT;
      return FALSE;
    }

    /* Check for events that must be processed in this loop */

    if (my_uptr->events)
      process_events();

    if (((k_exit_cause == K_QUIT) && !tio_handle_break()) ||
        (k_exit_cause == K_TERMINATE)) {
      return FALSE;
    }

    tm.tv_sec = 1;
    tm.tv_usec = 0;
  }

  return TRUE;
}

/* END-CODE */
