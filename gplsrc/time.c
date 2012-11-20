/* TIME.C
 * Time related functions
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
 * START-HISTORY:
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 23 Nov 05  2.2-17 Extracted from op_misc.c
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

   #include <sys/types.h>
#include "qm.h"
#include <time.h>

   #define _timezone timezone


time_t clock_time;

/* ====================================================================== */

long int local_time(void)
{
 static long int hour = -1;
 struct tm * ltm;
 static int dst;
 long int h;

 clock_time = time(NULL);

 /* Because daylight saving time does not take effect at midnight (though
    the Windows/Linux implementation might), we must reassess whether we
    are in a daylight saving time period if this call to local_time() is
    not in the same hour as the previous call.                            */

 h = clock_time / 3600;
 if (h != hour)   /* Must reassess whether we're in daylight saving time */
  {
   hour = h;
   ltm = localtime(&clock_time);
   dst = (ltm->tm_isdst)?3600:0;

  } 



 return clock_time - _timezone + dst;




}

/* ====================================================================== */
   
long int qmtime()
{
 return local_time() + (732 * 86400L);
}


/* END-CODE */
