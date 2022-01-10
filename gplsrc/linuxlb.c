/* LINUXLB.C
 * Windows library substitutes for Linux
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
 * 10Jan22 gwb Fixed a format specifier warning.
 *
 * 28Feb20 gwb Changed integer declarations to be portable across address
 *             space sizes (32 vs 64 bit)
 *
 * START-HISTORY (OpenQM):
 * 09 Oct 07  2.6-5 Added qmrealpath() as emulation of realpath() but allowing
 *                  for pathnames that do not exist, appending all trailing
 *                  elements when non-existant item is found.
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 27 Apr 05  2.1-13 0348 Use cuserid() rather than getlogin() as the latter
 *                   does not work for QMClient processes.
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

#include "qm.h"

#include <pwd.h>
#include <time.h>

#ifndef __APPLE__
#include <crypt.h>
#endif

/* ======================================================================
   filelength64()  -  Return file size in bytes                           */

int64 filelength64(fd) int fd;
{
  struct stat statbuf;

  fstat(fd, &statbuf);
  return statbuf.st_size;
}

/* ======================================================================
   IsAdmin()  -  Is this user an administrator at the o/s level?          */

bool IsAdmin(void) {
  return (getuid() == 0);
}

/* ======================================================================
   itoa()  -  Convert integer to string                                   */

char* itoa(value, string, radix) int value;
char* string;
int radix; /* Ignored */
{
  sprintf(string, "%d", value);
  return string;
}

/* ======================================================================
   Ltoa()  -  Convert long integer to string                              */

char* Ltoa(value, string, radix) int32_t value;
char* string;
int radix; /* Ignored */
{
  sprintf(string, "%d", value);
  return string;
}

/* ======================================================================
   GetUserName()  -  Return user name for logged in user.                 */

bool GetUserName(name, bytes) char* name;
u_int32_t* bytes; /* Buffer size - updated to actual size on exit */
{
  char* p;
  int n = 0;
  struct passwd* pw;

  pw = getpwuid(getuid());
  p = (pw == NULL) ? NULL : (pw->pw_name);

  if (p != NULL) {
    n = strlen(p);
    if (*bytes >= n)
      n = *bytes - 1;
    memcpy(name, p, n);
  }
  *(name + n) = '\0';
  *bytes = n;

  return TRUE;
}

/* ======================================================================
   qmrealpath()  -  Emulation of realpath() with extension to handle
                    pathnames that do not exist.                          */

char* qmrealpath(char* inpath,  /* Supplied path */
                 char* outpath) /* Full path */
{
  char* tgt;
  char* p;
  char* q;
  struct stat st;
  int n;
  int link_depth = 0;
  char link_buf[PATH_MAX + 1];

  switch (inpath[0]) {
    case '/': /* Absolute pathname */
      outpath[0] = '/';
      tgt = outpath + 1;
      break;

    case '\0': /* Null pathname - error */
      return NULL;

    default: /* Relative pathname - get current directory */
      getcwd(outpath, PATH_MAX);
      tgt = strchr(outpath, '\0');
      break;
  }

  p = inpath; /* Source pointer */
  while (*p != '\0') {
    /* Skip over multiple delimiters */
    while (*p == '/')
      p++;

    /* Find next delimiter or end of inpath */
    q = p;
    while (*q != '\0' && *q != '/')
      q++;
    n = q - p;

    if ((*p == '.') && (n == 1)) /* . reference */
    {
      /* Nothing to do */
    } else if ((*p == '.') && (*(p + 1) == '.') && (n == 2)) /* .. reference */
    {
      /* Revert one level unless already at root */
      if (tgt > outpath + 1) {
        while (*((--tgt) - 1) != '/') {
        }
      }
    } else /* Name reference */
    {
      if (*(tgt - 1) != '/')
        *(tgt++) = '/';

      /* Append this name unless it would overrun the buffer */

      if (tgt + n - outpath >= PATH_MAX)
        return NULL;

      memcpy(tgt, p, n);
      p = q + 1;
      tgt += n;
      *tgt = '\0';

      /* Check the path exists and whether it is a symlink */

      if (lstat(outpath, &st) < 0) {
        if (errno != ENOENT)
          return NULL;

        /* Simply glue unrecognised component(s) on the end so that we
          return a fully resolved path of what we might be trying to
          create.                                                      */

        if ((p - inpath) <= strlen(inpath)) {
          if (tgt + strlen(p) + 1 >= outpath + PATH_MAX)
            return NULL; /* Too long */

          *(tgt++) = '/';
          strcpy(tgt, p);
        }
        return outpath;
      }

      if (S_ISLNK(st.st_mode)) {
        if (++link_depth > 20)
          return NULL; /* Symlinks too deep */

        n = readlink(outpath, link_buf, PATH_MAX);
        if (n < 0)
          return NULL;

        link_buf[n] = '\0';

        if (link_buf[0] == '/') /* It's an absolute symlink */
        {
          strcpy(outpath, link_buf);
          tgt = outpath + n;
        } else {
          /* Back up one level unless already at root directory */
          if (tgt > outpath + 1)
            while (*((--tgt) - 1) != '/') {
            }

          if (tgt + n - outpath >= PATH_MAX)
            return NULL;

          strcpy(tgt, link_buf);
          tgt += n;
        }
      }
    }

    p = q;
  }

  /* Remove trailing / if present unless root directory reference */

  if (tgt > outpath + 1 && *(tgt - 1) == '/')
    tgt--;
  *tgt = '\0';

  return outpath;
}

/* ======================================================================
   Sleep()  -  Sleep for period in milliseconds                           */

void Sleep(n) int32_t n;
{
  struct timespec period;
  struct timespec remaining;

  period.tv_sec = n / 1000;
  period.tv_nsec = (n % 1000) * 1000000;
  nanosleep(&period, &remaining);
}

/* END-CODE */
