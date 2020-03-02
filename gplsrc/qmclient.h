/* QMCLIENT.H
 * QMClient Server Interface.
 * Copyright (c) 2004 Ladybridge Systems, All Rights Reserved
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
 * 
 * START-HISTORY (OpenQM):
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

/* Server commands */

#define SrvrQuit          1    /* Disconnect */
#define SrvrGetError      2    /* Get extended error text */
#define SrvrAccount       3    /* Set account */
#define SrvrOpen          4    /* Open file */
#define SrvrClose         5    /* Close file */
#define SrvrRead          6    /* Read record */
#define SrvrReadl         7    /* Read record with shared lock */
#define SrvrReadlw        8    /* Read record with shared lock, waiting */
#define SrvrReadu         9    /* Read record with exclusive lock */
#define SrvrReaduw       10    /* Read record with exclusive lock, waiting */
#define SrvrSelect       11    /* Select file */
#define SrvrReadNext     12    /* Read next id from select list */
#define SrvrClearSelect  13    /* Clear select list */
#define SrvrReadList     14    /* Read a select list */
#define SrvrRelease      15    /* Release lock */
#define SrvrWrite        16    /* Write record */
#define SrvrWriteu       17    /* Write record, retaining lock */
#define SrvrDelete       18    /* Delete record */
#define SrvrDeleteu      19    /* Delete record, retaining lock */
#define SrvrCall         20    /* Call catalogued subroutine */
#define SrvrExecute      21    /* Execute command */
#define SrvrRespond      22    /* Respond to request for input */
#define SrvrEndCommand   23    /* Abort command */
#define SrvrLogin        24    /* Network login */
#define SrvrLocalLogin   25    /* QMLocal login */
#define SrvrSelectIndex  26    /* Select index */
#define SrvrEnterPackage 27    /* Enter a licensed package */
#define SrvrExitPackage  28    /* Exit from a licensed package */
#define SrvrOpenQMNet    29    /* Open QMNet file */
#define SrvrLockRecord   30    /* Lock a record */
#define SrvrClearfile    31    /* Clear file */
#define SrvrFilelock     32    /* Get file lock */
#define SrvrFileunlock   33    /* Release file lock */
#define SrvrRecordlocked 34    /* Test lock */
#define SrvrIndices1     35    /* Fetch information about indices */
#define SrvrIndices2     36    /* Fetch information about specific index */
#define SrvrSelectList   37    /* Select file and return list */
#define SrvrSelectIndexv 38    /* Select index, returning indexed values */
#define SrvrSelectIndexk 39    /* Select index, returning keys for indexed value */
#define SrvrFileinfo     40    /* FILEINFO() */
#define SrvrReadv        41    /* READV and variants */
#define SrvrSetLeft      42    /* Align index position to left */
#define SrvrSetRight     43    /* Align index position to right */
#define SrvrSelectLeft   44    /* Move index position to left */
#define SrvrSelectRight  45    /* Move index position to right */
#define SrvrMarkMapping  46    /* Enable/disable mark mapping */

/* Server error status values */
#define SV_OK             0    /* Action successful                       */
#define SV_ON_ERROR       1    /* Action took ON ERROR clause             */
#define SV_ELSE           2    /* Action took ELSE clause                 */
#define SV_ERROR          3    /* Action failed. Error text available     */
#define SV_LOCKED         4    /* Action took LOCKED clause               */
#define SV_PROMPT         5    /* Server requesting input                 */

/* END-CODE */
