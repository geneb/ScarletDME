* SYSCOM.H
* Per-process common
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
* 20 Aug 07  2.6-0 Command stack size now configurable.
* 24 Jan 07  2.4-20 0537 @USER3 was incorrectly pointing to @USER2.
* 07 Dec 06  2.4-17 Added TRANS file cache.
* 17 Aug 06  2.4-11 Added COMMAND.PROMPT entry.
* 10 Feb 06  2.3-6 Autologout now handled in kernel.
* 06 Oct 05  2.2-14 Added support for multiple breakpoint values.
* 12 Jan 05  2.1-0 Renamed command and sentence as at.command and at.sentence.
* 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
* END-HISTORY
*
* START-DESCRIPTION:
*
* After changing this module, it is important to recompile things in the
* correct order or the build will fail.
*
* 1. Change this module.
* 2. Compile (in order) in a single run of the compiler:
*        CPROC, LOGIN, BASIC, BCOMP, PTERM, CATALOG
* 3. Exit QM and re-enter
* 4. Compile all of BP
*
* END-DESCRIPTION
*
* START-CODE

* ==========================================================================
* This common is limited to 255 variables unless an extended form
* of LDSYS and LDSYSV is implemented.
*
* SEE ALSO, SYSCOM.H (C definition)
* ==========================================================================


$define HIGH.SELECT            12  ;* Highest system select list
$define HIGH.USER.SELECT       10  ;* Highest user select list
$define TRANS.FILE.CACHE.SIZE  10

common /$syscom/ itype.mode,       ;*  1  I-type mode
       at.command,                 ;*  2  @COMMAND : Last terminal or EXECUTEd command
       at.sentence,                ;*  3  @SENTENCE : Active sentence
       parasentence,               ;*  4  Sentence which invoked paragraph
       voc,                        ;*  5  VOC file
       xeq.command,                ;*  6  EXECUTE command
       parser.sentence,            ;*  7  Used by !PARSER function
       system.return.code,         ;*  8  System return code from verbs
       data.queue,                 ;*  9  DATA statement queue
       command.stack,              ;* 10  Stored command stack
       report.src,                 ;* 11  Report @SYSTEM.RETURN.CODE
       select.list(HIGH.SELECT),   ;* 12  SELECT list
       select.count(HIGH.SELECT),  ;* 13  Count of items. Zero if not active
       logname,                    ;* 14  @LOGNAME
       echo.input,                 ;* 15  ECHO statement state
       dispatch.info,              ;* 16  Supplementary dispatch info (@option)
       cproc.date,                 ;* 17  Date at start of CPROC command
       cproc.time,                 ;* 18  Time at start of CPROC command
       qproc.file.name,            ;* 19  File name in QPROC command
       qproc.id,                   ;* 20  @ID
       qproc.record,               ;* 21  @RECORD
       qproc.ni,                   ;* 22  @NI Number of items in QPROC
       qproc.break.level,          ;* 23  Current break level in QPROC
       sys.bell,                   ;* 24  @SYS.BELL
       user.return.code,           ;* 25  User return code
       account.path,               ;* 26  @PATH - Pathname of account directory
       who,                        ;* 27  @WHO
       trap.aborts,                ;* 28  Special actions on abort
       ipc.f,                      ;* 29  IPC file
       qproc.breakpoint.value(9),  ;* 30  Values for Bn heading option
       last.command,               ;* 31  Last command for PSTAT
       abort.code,                 ;* 32  @ABORT.CODE
       initial.account.path,       ;* 33  Account path on entry to QM
       inline.prompts,             ;* 34  Prompt texts...
       inline.responses,           ;* 35  ...and responses
       como.file,                  ;* 36  COMO file variable
       como.record,                ;* 37  Current COMO file record name
       parser,                     ;* 38  Subroutine reference to !PARSER
       selected,                   ;* 39  @SELECTED
       at.conv,                    ;* 40  @CONV
       tty,                        ;* 41  @TTY
       cursor.at.end,              ;* 42  Command editor mode
       command.overlay,            ;* 43  Command editor mode
       show.stack,                 ;* 44  Command editor mode
       clear.on.data,              ;* 45  Command editor mode
       qproc.nd,                   ;* 46  @ND - Detail lines since breakpoint
       user.var.names,             ;* 47  Variables set using SET...
       user.var.values,            ;* 48  ... and their values
       user0,                      ;* 49  @USER0
       user1,                      ;* 50  @USER1
       user2,                      ;* 51  @USER2
       user3,                      ;* 52  @USER3
       user4,                      ;* 53  @USER4
       qproc.totals(1),            ;* 54  TOTAL() array
       trigger.return.code,        ;* 55  @TRIGGER.RETURN.CODE
       abort.message,              ;* 56  @ABORT.MESSAGE
       dir.separator,              ;* 57  @DS directory separator
       initial.account,            ;* 58  Account name on entry to QM
       command.prompt,             ;* 59  Command prompt (both variants)
       private.catalogue,          ;* 60  Private catalogue location
       qproc.nv,                   ;* 61  @NV
       qproc.ns,                   ;* 62  @NS
       qproc.lpv,                  ;* 63  @LPV
       term.reset.string,          ;* 64  Terminal reset string
       aliased.commands,           ;* 65  These commands get parsed as...
       alias.targets,              ;* 66  ...these commands.
       proc.ibuf(1),               ;* 67  Primary/secondary input buffers
       proc.obuf(1),               ;* 68  Primary/secondary output buffers
       proc.frec(9),               ;* 69  File buffers and...
       proc.fvar(9),               ;* 70  ...file variables
       proc.iptr,                  ;* 71  Input buffer pointer field...
       proc.iptr.cpos,             ;* 72  ...and character
       proc.acti,                  ;* 73  Active input buffer index
       proc.acto,                  ;* 74  Active output buffer index
       at.ans,                     ;* 75  @ANS
       at.sys0,                    ;* 76  @SYS0  System equivalent of @USER0
       default.style.rec,          ;* 77  Default STYLE for query processor
       trans.files,                ;* 78  Files open to TRANS()...
       trans.fvars(TRANS.FILE.CACHE.SIZE), ;* 79  ...and their file variables
       command.stack.depth         ;* 80

$define syscom.itype.mode              1
$define syscom.command                 2
$define syscom.sentence                3
$define syscom.parasentence            4
$define syscom.voc                     5
$define syscom.xeq.command             6
$define syscom.system.return.code      8
$define syscom.data.queue              9
$define syscom.command.stack          10
$define syscom.select.list            12
$define syscom.select.count           13
$define syscom.logname                14
$define syscom.echo.input             15
$define syscom.option                 16
$define syscom.cproc.date             17
$define syscom.cproc.time             18
$define syscom.qproc.file.name        19
$define syscom.qproc.id               20
$define syscom.qproc.record           21
$define syscom.qproc.ni               22
$define syscom.qproc.break.level      23
$define syscom.bell                   24
$define syscom.user.return.code       25
$define syscom.account.path           26
$define syscom.who                    27
$define syscom.ipc                    29
$define syscom.break.value            30
$define syscom.last.command           31
$define syscom.abort.code             32
$define syscom.selected               39
$define syscom.conv                   40
$define syscom.tty                    41
$define syscom.qproc.nd               46
$define syscom.user0                  49
$define syscom.user1                  50
$define syscom.user2                  51
$define syscom.user3                  52
$define syscom.user4                  53
$define syscom.qproc.totals           54
$define syscom.trigger.return.code    55
$define syscom.abort.message          56
$define syscom.ds                     57
$define syscom.qproc.nv               61
$define syscom.qproc.ns               62
$define syscom.qproc.lpv              63
$define syscom.proc.ibuf              67
$define syscom.proc.obuf              68
$define syscom.proc.acti              73
$define syscom.proc.acto              74
$define syscom.at.ans                 75
$define syscom.sys0                   76

* Values of trap.aborts (Used to decide what to do about an abort)
$define ABORT.NORMAL                   0  ;* Nothing special happening
$define ABORT.ON.ABORT                 1  ;* Running ON.ABORT
$define ABORT.ON.EXIT                  2  ;* Running ON.EXIT
$define ABORT.TERMINATE                3  ;* From single shot mode (QM xxx)

* END-CODE
