/* LINUXIO.C
 * Linux low level terminal driver functions
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
 * 03Sep25 gwb Cleaned up K&R-isms.
 * 30Nov23 mab Added "$y$" -  yescrypt to login_user()
 * 17Jan22 gwb Added a fix to login_user() to be able to grok SHA-512
 *             hashes ($6$) along with the exiting MD5 hashes ($1$).
 *             Thanks to Tom D. for the fix.
 * 
 * 05Mar20 gwb Added an include for crypt in order to eliminate a warning.
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 *
 * START-HISTORY (OpenQM):
 * 05 Nov 07  2.6-5 0566 Applied casts to handle keyin() correctly.
 * 13 Sep 07  2.6-3 0562 Need to inhibit input_handler() when doing SH command.
 * 03 Sep 07  2.6-3 Disable OPOST output mode so that LF is not mapped to CRLF.
 * 14 Aug 07  2.6-0 Enable asyncio for console mode too.
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 27 Jun 07  2.5-7 Added PTERM TELNET ON/OFF handling
 * 19 Jun 07  2.5-7 Start with null terminal type.
 * 18 Jun 07  2.5-7 Return immediately from do_input() at connection loss.
 * 06 Jun 07  2.5-7 Removed logging of connection loss as this was happening for
 *                  every VBSRVR termination. Also, because it is triggered off
 *                  SIGIO, there could be nested logmsg() calls.
 * 11 May 07  2.5-3 Log connection loss.
 * 05 Feb 07  2.4-20 Added transparent_newline argument to tio_display_string().
 * 04 Feb 07  2.4-20 tparm() now has length argument.
 * 25 Aug 06  2.4-12 Reverted to old handling of CR/LF pairs.
 * 02 Aug 06  2.4-10 Reworked do_input to make CR and LF accessible via keyin().
 * 25 May 06  2.4-5 Use qmpoll() as poll() is broken in Mac OS X 10.4.
 * 17 Mar 06  2.3-8 KEYIN() timeout now in milliseconds.
 * 21 Dec 05  2.3-3 Return status() from keyin() at exception conditions.
 * 11 Nov 05  2.2-16 Handle piped input as a special case, disabling the ring
 *                   buffer and checking for LF as line terminator.
 * 10 Nov 05  2.2-16 0429 Handle the situation where the signal handler might
 *                   take input between return from poll() and trying to read
 *                   the data.
 * 21 Sep 05  2.2-12 Added child pipe interface for use by op_sh.c to feed data
 *                   to child process.
 * 16 Aug 05  2.2-8 0393 Raise EVT_TERMINATE, not EVT_LOGOUT on connection loss
 *                  so that ON.EXIT runs.
 * 12 May 05  2.1-14 0353 Moved inewline to allow static initialisation.
 * 10 May 05  2.1-13 0351 Changes in 2.1-12 lost establishment of SIGINT handler
 *                   on console sessions.
 * 06 May 05  2.1-13 Added inewline translation.
 * 06 May 05  2.1-13 Removed raw mode argument from keyin().
 * 04 May 05  2.1-13 Don't abort if cannot get terminal settings - It might not
 *                   be a terminal device at all.
 * 21 Apr 05  2.1-12 Renamed trapping_breaks as trap_break_char.
 * 16 Apr 05  2.1-12 Use socket buffering to minimise packet delays.
 * 06 Apr 05  2.1-12 0338 Changes to avoid loop on network connection loss.
 * 06 Apr 05  2.1-12 Send DO SuppressLocalEcho on telnet connections.
 * 06 Mar 05  2.1-8 Honour telnet binary mode negotiation.
 * 10 Jan 05  2.1-0 Renamed tgetstr() as qmtgetstr().
 * 03 Jan 05  2.1-0 Added support for save/restore screen on non-QMTerm.
 * 21 Oct 04  2.0-7 write_console() now returns bool.
 * 12 Oct 04  2.0-5 Added support for client help.
 * 20 Sep 04  2.0-2 Use message handler.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 */

#include "qm.h"
#include "config.h"
#include "err.h"
#include "header.h"
#include "qmnet.h"
#include "qmtermlb.h"
#include "telnet.h"
#include "tio.h"

#include <netdb.h>
#include <pwd.h>
#include <signal.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>

#include <sched.h>

#ifndef __APPLE__
#define _GNU_SOURCE /* eliminates the warning when we call crypt() below */
#include <crypt.h>
#endif

Public int ChildPipe;
Public bool in_sh; /* 0562 Doing SH command? */

#define RING_SIZE 1024
Private volatile char ring_buff[RING_SIZE];
Private volatile int16_t ring_in = 0;
Private volatile int16_t ring_out = 0;

Private void io_handler(int sig);
Private int stdin_modes;
Private void do_input(void);
Private bool input_handler_enabled = TRUE;
Private bool piped_input = FALSE;
Private bool connection_lost = FALSE;

/* Keyboard */

Private int ttyin = 0;
Private struct termios old_tty_settings;
Private struct termios new_tty_settings;
Private bool tty_modes_saved = FALSE;
Private int16_t type_ahead = -1;

Private void signal_handler(int signum);

void set_term(bool trap_break);
void set_old_tty_modes(void);
void set_new_tty_modes(void);
bool negotiate_telnet_parameter(void);

/* ======================================================================
   start_connection()  -  Start Linux socket / pipe based connection      */

bool start_connection(int unused) {
  socklen_t n;
  struct sockaddr_in sa;
  int flag;

  if (is_QMVbSrvr)
    strcpy(command_processor, "$VBSRVR");

  if (connection_type == CN_SOCKET) {
    if (is_QMVbSrvr) {
      flag = TRUE;
      setsockopt(0, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

      /* Fire an Ack character up the connection to start the conversation.
         This is necessary because Linux loses anything we send before the
         new process is up and running so QMClient isn't going to talk until
         we go first. The Ack comes from QMSvc on NT style systems.          */

      send(0, "\x06", 1, 0);
    } else {
      send(1, "\xff\xfd\x2d", 3, 0); /* DO SuppressLocalEcho */
      send(1, "\xff\xfb\x01", 3, 0); /* WILL echo */
      send(1, "\xff\xfd\x03", 3, 0); /* DO suppress go ahead */
      send(1, "\xff\xfb\x03", 3, 0); /* WILL suppress go ahead */
      send(1, "\xff\xfe\x22", 3, 0); /* DONT line mode */
      send(1, "\xff\xfd\x18", 3, 0); /* DO TERMTYPE */
    }

    n = sizeof(sa);
    getpeername(0, (struct sockaddr *)&sa, &n);
    strcpy(ip_addr, (char *)inet_ntoa(sa.sin_addr));

    n = sizeof(sa);
    getsockname(0, (struct sockaddr *)&sa, &n);
    port_no = ntohs(sa.sin_port);

    /* Create output buffer */

    outbuf = (char *)malloc(OUTBUF_SIZE);
    if (outbuf == NULL) {
      printf("Unable to allocate socket output buffer\n");
      return FALSE; /* Error */
    }
  }

  case_inversion = TRUE;
  set_term(TRUE);

  /* Set up signal handler */

  signal(SIGINT, signal_handler);
  signal(SIGHUP, signal_handler);
  signal(SIGTERM, signal_handler);

  /* Set up a signal handler to catch SIGIO generated by arrival of
     input data.                                                       */

  signal(SIGIO, io_handler);
  fcntl(0, F_SETOWN, (int)getpid());
  fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK | O_ASYNC);

  return TRUE;
}

/* ====================================================================== */

bool init_console() {
  struct stat statbuf;

  /* ----------------------- Display ------------------------ */

  tio.dsp.width = 80;
  tio.dsp.lines_per_page = 24;

  if (connection_type == CN_CONSOLE) {
    /* ----------------------- Keyboard ----------------------- */

    /* Fetch the current terminal settings and attempt to set new ones. */

    ttyin = 0;
    if (!tcgetattr(ttyin, &old_tty_settings)) {
      tty_modes_saved = TRUE;

      /* Construct desired settings */

      new_tty_settings = old_tty_settings;

      new_tty_settings.c_iflag &= ~ISTRIP; /* 8 bit input */
      new_tty_settings.c_iflag |= IGNPAR;  /* Disable parity */
      new_tty_settings.c_iflag &= ~ICRNL;  /* Do not map CR to NL */
      new_tty_settings.c_iflag &= ~IGNCR;  /* Do not discard CR */
      new_tty_settings.c_iflag &= ~INLCR;  /* Do not map NL to CR */
      new_tty_settings.c_iflag &= ~IXON;   /* Kill X-on/off for output... */
      new_tty_settings.c_iflag &= ~IXOFF;  /* ...and input */

      new_tty_settings.c_oflag &= ~OPOST; /* Do not convert LF to CRLF */

      new_tty_settings.c_cflag &= ~CSIZE; /* Enable... */
      new_tty_settings.c_cflag |= CS8;    /* ...8 bit operation */

      new_tty_settings.c_lflag &= ~ICANON; /* No erase/kill processing */
      new_tty_settings.c_lflag |= ISIG;    /* Enable signal processing */
      new_tty_settings.c_lflag &= ~ECHO;   /* Half duplex */
      new_tty_settings.c_lflag &= ~ECHONL; /* No echo of linefeed */

      new_tty_settings.c_cc[VMIN] = 1;     /* Single character input */
      new_tty_settings.c_cc[VQUIT] = '\0'; /* No quit character */
      new_tty_settings.c_cc[VSUSP] = '\0'; /* No suspend character */
      new_tty_settings.c_cc[VEOF] = '\0';  /* No eof character */

      /* Attempt to set device to this mode */

      tcsetattr(ttyin, TCSANOW, &new_tty_settings);
    }
  }

  case_inversion = TRUE;
  set_term(TRUE);

  fstat(0, &statbuf);
  if (S_ISFIFO(statbuf.st_mode))
    piped_input = TRUE;

  /* Set up signal handler  0351 */

  signal(SIGINT, signal_handler);
  signal(SIGHUP, signal_handler);
  signal(SIGTERM, signal_handler);

  /* Set up a signal handler to catch SIGIO generated by arrival of
     input data.                                                       */

  if (!piped_input) {
    signal(SIGIO, io_handler);
    fcntl(0, F_SETOWN, (int)getpid());
    stdin_modes = fcntl(0, F_GETFL);
    fcntl(0, F_SETFL, stdin_modes | O_NONBLOCK | O_ASYNC);
  }

  return TRUE;
}

/* ======================================================================
 *  set_term()  -  Set or reset terminal modes                            
 *  trap_break = true will treat break char as a break.
 */
void set_term(bool trap_break) {
  trap_break_char = trap_break;

  if (connection_type == CN_CONSOLE) {
    if (trap_break_char)
      new_tty_settings.c_lflag |= ISIG;
    else
      new_tty_settings.c_lflag &= ~ISIG;
    set_new_tty_modes();
  }
}

/* ======================================================================
   shut_console()  -  Shutdown console functions                          */

void shut_console() {
  if (connection_type == CN_CONSOLE) {
    set_old_tty_modes();

    /* Remove signal handler for SIGIO */

    if (!piped_input) {
      signal(SIGIO, SIG_DFL);
      fcntl(0, F_SETOWN, (int)getpid());
      fcntl(0, F_SETFL, stdin_modes);
    }
  }
}

/* ======================================================================
   set_old_tty_modes()  -  Reset tty to modes it had on entry             */

void set_old_tty_modes() {
  if (tty_modes_saved)
    tcsetattr(ttyin, TCSANOW, &old_tty_settings);
}

/* ======================================================================
   set_new_tty_modes()  -  Reset tty to modes required by QM              */

void set_new_tty_modes() {
  tcsetattr(ttyin, TCSANOW, &new_tty_settings);
}

/* ====================================================================== */

bool write_console(char *p, int bytes) {
  int n;

  while (bytes) {
    n = write(1, p, bytes);
    if (n < 0) /* An error occured */
    {
      if (errno != EAGAIN)
        return FALSE;
      sched_yield();
    } else {
      bytes -= n;
      p += n;
    }
  }

  return TRUE;
}

/* ======================================================================
   Low level keyboard handling functions                                  */

bool keyready() {
  /* If there is type-ahead, we can return immediately */

  if (type_ahead >= 0)
    return TRUE;

  if (piped_input)
    return TRUE;

  input_handler_enabled = FALSE;

  /* Check if there is anything pending on stdin. We may have had
     the SIGIO handler disabled when this arrived.                       */

  if (qmpoll(0, 0) > 0) {
    /* There is something waiting.  Go get it as a type ahead character */
    type_ahead = (char)keyin(0);
  } else {
    /* Ok, so there's nothing out in the Linux world. Is there anything
       in our ring buffer?                                              */

    if (ring_in == ring_out) {
      input_handler_enabled = TRUE;
      return FALSE;
    }

    type_ahead = ring_buff[ring_out];
    ring_out = (ring_out + 1) % RING_SIZE;
  }

  input_handler_enabled = TRUE;
  return TRUE;
}

/* timeout in milliseconds */
int16_t keyin(int timeout) {
  char c;
  struct timeval tv;
  int64 t1;
  int64 t2;
  int td;
  int poll_ret;
  bool using_autologout = FALSE;

  /* Disable the signal handler. Once this is done, no input can arrive
     from anywhere else. We can then safely test the ring buffer. Because
     we use a signal handler rather than multi-threading, the two styles
     of input cannot be running simulataneously.                          */

  input_handler_enabled = FALSE;

  if (type_ahead >= 0) /* 0211 */
  {
    c = type_ahead;
    type_ahead = -1;
    goto exit_keyin;
  } else {
    if (piped_input) {
      if (read(0, &c, 1) <= 0) {
        process.status = ER_EOF;
        return -1;
      }

      if (c == 10)
        c = inewline;
      goto exit_keyin;
    }

    if (autologout)
      using_autologout = (autologout < timeout) || !timeout;
    if (using_autologout)
      timeout = autologout;

    if (timeout) {
      gettimeofday(&tv, NULL);
      t1 = (((int64)(tv.tv_sec)) * 1000) + (tv.tv_usec / 1000);
    }

    while (ring_in == ring_out) /* Nothing in ring buffer */
    {
      /* Do our own i/o wait so that we can handle events while we are
         waiting. The paths that return special values all re-enable the
         signal handler. If any input arrives between the call to poll()
         and re-enabling the handler, it will be picked up by the next
         call to keyin() or keyready() as it will still be in the queue. */

      do {
        poll_ret = qmpoll(0, (timeout && timeout < 1000) ? timeout : 1000);

        /* 0188 Added check for EINTR so that signals do not cause us
           to terminate the process.                                  */

        if ((poll_ret < 0) && (errno != EINTR)) {
          connection_lost = TRUE;
          k_exit_cause = K_TERMINATE;
        }

        if ((my_uptr != NULL) && (my_uptr->events))
          process_events();

        if (k_exit_cause & K_INTERRUPT) {
          /* Force our way out for logout, etc */
          input_handler_enabled = TRUE;
          return 0;
        }

        if (timeout) {
          gettimeofday(&tv, NULL);
          t2 = (((int64)(tv.tv_sec)) * 1000) + (tv.tv_usec / 1000);
          td = t2 - t1;
          timeout -= td;
          t1 = t2;
          if (timeout <= 0) {
            input_handler_enabled = TRUE;

            if (using_autologout) {
              log_printf("%s\n", sysmsg(2503)); /* Inactivity timer expired -
                                                   Process logged out */
              Sleep(3000);
              k_exit_cause = K_TERMINATE;
            }

            process.status = ER_TIMEOUT;
            return -1;
          }
        }
      } while (poll_ret <= 0);

      /* There should be something waiting for us.  Because we may choose
         to discard whatever is waiting (NUL, quit key, etc), we must go
         round the loop again to check if there is now anything in the
         ring buffer.                                                      */

      do_input();
    }

    /* Grab the character from the ring buffer */

    c = ring_buff[ring_out];
    ring_out = (ring_out + 1) % RING_SIZE;
  }

exit_keyin:

  /* Re-enable the handler. As above, any input arriving in the final
     moments before it was re-enabled will be picked up later.         */

  input_handler_enabled = TRUE;
  return (int16_t)((u_char)c);
}

void io_handler(int sig) {
  /* Collect the input and re-enable the signal */

  if (input_handler_enabled && !in_sh)
    do_input(); /* 0562 */
  signal(SIGIO, io_handler);
}

Private void do_input() {
  char c;
  int16_t n;
  static bool last_was_cr = TRUE; /* May need to skip leading NUL/LF */

again:
  while (!connection_lost) {
    if (qmpoll(0, 0) <= 0)
      break;

    n = (ring_in + 1) % RING_SIZE;
    if (n == ring_out)
      return; /* Ring buffer is full */

    if (read(0, &c, 1) <= 0) {
      if (errno == EAGAIN)
        goto again; /* 0429 io_handler() stole our data */

      connection_lost = TRUE;     /* Lost connection */
      k_exit_cause = K_TERMINATE; /* 0393 */
      c = 0;                      /* 0338 */
                                  // 0338     return;
    }

    if (!is_QMVbSrvr) {
      if (c == tio.break_char) /* The break key */
      {
        if (trap_break_char) {
          break_key();
          continue;
        }
      } else {
        if (((u_char)c == TN_IAC) && telnet_negotiation) {
          (void)negotiate_telnet_parameter();
          continue;
        }
      }
    }

    if (!telnet_binary_mode_in) {
      if (last_was_cr) {
        last_was_cr = FALSE;
        if (c == 0)
          continue; /* Ignore NUL after CR */
        if (c == 10)
          continue; /* Ignore LF after CR */
      }
      last_was_cr = (c == 13);

      if (c == 13)
        c = inewline;
    } else
      last_was_cr = FALSE; /* Ready for exit from binary mode */

    if (ChildPipe >= 0) {
      if (c == inewline)
        c = 10;
      tio_display_string(&c, 1, TRUE, FALSE);
      write(ChildPipe, &c, 1);
    } else {
      /* Pop this character into the ring buffer */

      ring_buff[ring_in] = c;
      ring_in = (ring_in + 1) % RING_SIZE;
    }
  }
}

/* ======================================================================
   inblk()  -  Input a block from the ring buffer                         */

STRING_CHUNK *inblk(int max_bytes) {
  int n;
  int16_t actual_size;
  STRING_CHUNK *str = NULL;
  char *p;
  int bytes;

  n = (ring_in + RING_SIZE - ring_out) % RING_SIZE; /* Bytes in buffer */
  if (n > max_bytes)
    n = max_bytes;

  if (n != 0) {
    str = s_alloc(n, &actual_size); /* Will never be smaller than n */
    str->ref_ct = 1;
    str->string_len = n;
    str->bytes = n;

    bytes = min(RING_SIZE - ring_out, n); /* Portion up to end of ring buffer */
    p = str->data;
    memcpy(p, ((char *)ring_buff) + ring_out, bytes);
    ring_out = (ring_out + bytes) % RING_SIZE;
    n -= bytes;

    if (n) /* More at start of buffer */
    {
      p += bytes;
      memcpy(p, (char *)ring_buff, n);
      ring_out = n;
    }
  }

  return str;
}

/* ======================================================================
   save_screen()  -  Save screen image                                    */

bool save_screen(SCREEN_IMAGE *scrn, int16_t x, int16_t y, int16_t w, int16_t h) {
  char *p;
  char *q;
  static int32_t image_id = 0;
  int n;

  if (connection_type == CN_SOCKET) {
    scrn->id = image_id++;
    p = qmtgetstr("sreg");
    if (p != NULL) {
      q = tparm(&n, p, (int)(scrn->id), (int)x, (int)y, (int)w, (int)h);
      write_socket(q, n, TRUE);
    }
  }

  return TRUE;
}

/* ====================================================================== */

void restore_screen(SCREEN_IMAGE *scrn, bool restore_cursor) {
  char *p;
  char *q;
  int n;

  if (connection_type == CN_SOCKET) {
    p = qmtgetstr("rreg");
    if (p != NULL) {
      q = tparm(&n, p, (int)(scrn->id), (int)(scrn->x), (int)(scrn->y), (int)restore_cursor);
      write_socket(q, n, TRUE);
    }
  }
}

/* Interludes to map onto Windows style interfaces */

bool read_socket(char *str, int bytes) {
  while (bytes--)
    *(str++) = (char)keyin(0);
  return 1;
}

char socket_byte() {
  char c;

  read(0, &c, 1);

  return c;
}

/* ======================================================================
   login_user()  -  Perform checks and login as specified user            */

//
// Message that details the change is here: https://groups.google.com/g/scarletdme/c/Xza0TPEVqb8
//
// Summarized:
//  change this line: if (memcmp(p, "$1$", 3) == 0) /* MD5 algorithm */
//  to: if ((memcmp(p, "$1$", 3) == 0) || /* MD5 algorithm */
//         (memcmp(p, "$6$", 3) == 0)
// 17Jan22 gwb Added above change
// 30Nov23 mab Added (memcmp(p,"$y$", 3) == 0)) {  /* yescrypt */
bool login_user(char *username, char *password) {
  FILE *fu;
  struct passwd *pwd;
  char pw_rec[200 + 1];
  int16_t len;
  char *p = NULL;
  char *q;

  if ((fu = fopen(PASSWD_FILE_NAME, "r")) == NULL) {
    tio_printf("%s\n", sysmsg(1007));
    return FALSE;
  }

  len = strlen(username);

  while (fgets(pw_rec, sizeof(pw_rec), fu) > 0) {
    if ((pw_rec[len] == ':') && (memcmp(pw_rec, username, len) == 0)) {
      p = pw_rec + len + 1;
      break;
    }
  }
  fclose(fu);

  if (p != NULL) {
    if ((memcmp(p, "$1$", 3) == 0) || /* MD5 algorithm */
        (memcmp(p, "$6$", 3) == 0) || /* SHA512 */
        (memcmp(p,"$y$", 3) == 0)) {  /* yescrypt */
      if ((q = strchr(p, ':')) != NULL)
        *q = '\0';
      if (strcmp((char *)crypt(password, p), p) == 0) {
        if (((pwd = getpwnam(username)) != NULL) && (setgid(pwd->pw_gid) == 0) && (setuid(pwd->pw_uid) == 0)) {
          //         set_groups();
          return TRUE;
        }
      }
    }
  }

  return FALSE;
}

/* ======================================================================
   Signal handler                                                         */

void signal_handler(int signum) {
  switch (signum) {
    case SIGINT:
      break_key();
      break;

    case SIGHUP:
    case SIGTERM:
      signal(SIGHUP, SIG_IGN);
      signal(SIGTERM, SIG_IGN);
      log_printf("Received termination signal %d\n", signum);
      if (my_uptr != NULL)
        my_uptr->events |= EVT_TERMINATE; /* 0393 */
      break;
  }
}

/* ======================================================================
   flush_outbuf()  -  Flush socket output buffer                          */

bool flush_outbuf() {
  if (outbuf_bytes) {
    if (!write_console(outbuf, outbuf_bytes))
      return FALSE;
    outbuf_bytes = 0;
  }

  return TRUE;
}

/* ====================================================================== */

int qmpoll(int fd, int timeout) {
#ifdef DO_NOT_USE_POLL
  fd_set fds;
  struct timeval tv;

  FD_ZERO(&fds);
  FD_SET(fd, &fds);
  tv.tv_sec = timeout / 1000;
  tv.tv_usec = (timeout % 1000) * 1000;
  return select(1, &fds, NULL, NULL, &tv);
#else
  struct pollfd fds[1];

  fds[0].fd = fd;
  fds[0].events = POLLIN;

  return poll(fds, 1, timeout);
#endif
}

/* END-CODE */
