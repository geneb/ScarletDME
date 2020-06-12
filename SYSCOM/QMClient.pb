; QMClient.pb
; QMClient PureBasic interface
; Copyright (c) 2005 Ladybridge Systems, All Rights Reserved

; Change History:
; 22 Mar 07  2.5-1 Changed UseFile() to FileID() to correspond to change in
;                  PureBasic version 4.
;                  Changed interface to ReadString(). Corrected definitions
;                  for QMError() and QMExecute().
; Usage notes:
;
; All arguments to QMCall() must be strings. Where a called subroutine returns
; a modified argument, the input string must be large enough to receive the
; result.
;
; Error code arguments, declared as *Err.l must be passed as pointers, for
; example:
;    s = QMRead(fno, id, @errno)


; Procedure declarations for use where needed

Declare QMCall0(Subrname.s)
Declare QMCall1(Subrname.s, *Arg1)
Declare QMCall2(Subrname.s, *Arg1, *Arg2)
Declare QMCall3(Subrname.s, *Arg1, *Arg2, *Arg3)
Declare QMCall4(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4)
Declare QMCall5(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5)
Declare QMCall6(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6)
Declare QMCall7(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7)
Declare QMCall8(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8)
Declare QMCall9(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9)
Declare QMCall10(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10)
Declare QMCall11(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10, *Arg11)
Declare QMCall12(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10, *Arg11, *Arg12)
Declare QMCall13(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10, *Arg11, *Arg12, *Arg13)
Declare QMCall14(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10, *Arg11, *Arg12, *Arg13, *Arg14)
Declare QMCall15(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10, *Arg11, *Arg12, *Arg13, *Arg14, *Arg15)
Declare QMCall16(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10, *Arg11, *Arg12, *Arg13, *Arg14, *Arg15, *Arg16)
Declare QMCall17(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10, *Arg11, *Arg12, *Arg13, *Arg14, *Arg15, *Arg16, *Arg17)
Declare QMCall18(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10, *Arg11, *Arg12, *Arg13, *Arg14, *Arg15, *Arg16, *Arg17, *Arg18)
Declare QMCall19(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10, *Arg11, *Arg12, *Arg13, *Arg14, *Arg15, *Arg16, *Arg17, *Arg18, *Arg19)
Declare QMCall20(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10, *Arg11, *Arg12, *Arg13, *Arg14, *Arg15, *Arg16, *Arg17, *Arg18, *Arg19, *Arg20)
Declare.s QMChange(Src.s, Old.s, New.s, Occ.l, Start.l)
Declare QMClearSelect(ListNo.l)
Declare QMClose(Fileno.l)
Declare QMClose(Fileno.l)
Declare.l QMConnect(Host.s, Port.l, User.s, Password.s, Account.s)
Declare.l QMConnectLocal(Account.s)
Declare.l QMConnected()
Declare.l QMDcount(String.s, Delim.s)
Declare.s QMDel(Src.s, Fno.l, Vno.l, SvNo.l)
Declare QMDelete(Fileno.l, Id.s)
Declare QMDeleteu(Fileno.l, Id.s)
Declare QMDisconnect()
Declare QMDisconnectAll()
Declare QMEndCommand()
Declare.s QMError()
Declare.s QMExecute(Cmd.s, *Err.l)
Declare.s QMExtract(Src.s, Fno.l, Vno.l, SvNo.l)
Declare.s QMField(Src.s, Delim.s, Start.l, Occ.l)
Declare.l QMGetSession()
Declare.s QMIns(Src.s, Fno.l, Vno.l, SvNo.l, New.s)
Declare.s QMLocate(Item.s, Dyn.s, Fno.l, Vno.l, SvNo.l, *Pos.l, Order.s)
Declare QMMarkMapping(Fileno.l, State.l)
Declare.l QMMatch(String.s, Template.s)
Declare.s QMMatchfield(String.s, Template.s, Component.l)
Declare.l QMOpen(Filename.s)
Declare.s QMRead(Fileno.l, Id.s, *Err.l)
Declare.s QMReadl(Fileno.l, Id.s, Wait.l, *Err.l)
Declare.s QMReadList(Listno.l)
Declare.s QMReadNext(Listno.l)
Declare.s QMReadu(Fileno.l, Id.s, Wait.l, *Err.l)
Declare QMRecordlock(Fileno.l, Id.s, Update.l, Wait.l)
Declare QMRelease(Fileno.l, Id.s)
Declare.s QMReplace(Src.s, Fno.l, Vno.l, SvNo.l, New.s)
Declare.s QMRespond(Response.s, *Err.l)
Declare QMSelect(Fileno.l, Listno.l)
Declare QMSelectIndex(Fileno.l, IndexName.s, IndexValue.s, Listno.l)
Declare.s QMSelectLeft(Fileno.l, IndexName.s, Listno.l)
Declare.s QMSelectRight(Fileno.l, IndexName.s, Listno.l)
Declare QMSetLeft(Fileno.l, IndexName.s)
Declare QMSetRight(Fileno.l, IndexName.s)
Declare.l QMSetSession(Idx.l)
Declare.l QMStatus()
Declare QMWrite(Fileno.l, Id.s, Rec.s)
Declare QMWriteu(Fileno.l, Id.s, Rec.s)


; ======================================================================
; The actual interface routines

Global QMLib
Global Connected


;===== OpenQMClientLibrary  (Called internally - Do not use in user programs)
Procedure OpenQMClientLibrary()
   If QMLib = 0
      ConfigPath$ = "/etc/qmconfig"
      fno = ReadFile(#PB_Any, ConfigPath$)
      If fno = 0
         printn("Cannot open " + ConfigPath$)
         End
      EndIf

; PureBasic version 4 replacement code
      FileID(fno)
      Repeat
         String$ = ReadString(fno, #PB_Ascii)
      Until Eof(fno) Or Left(String$,6) = "QMSYS="

;     FileID(fno)
;     Repeat
;        String$ = ReadString()
;     Until Eof(fno) Or Left(String$,6) = "QMSYS="
; End of replacement code

      If Eof(fno)
         Printn("Cannot find QMSYS pointer in configuration file")
         CloseFile(fno)
         End
      EndIf

      CloseFile(fno)
      LibPath$ = Mid(String$, 7, 999) + "/bin/qmclilib.so"
      QMLib = OpenLibrary(#PB_Any, LibPath$)
      If QMLib = 0
         Printn("Cannot open " + LibPath$)
         End
      EndIf
   EndIf
EndProcedure


;===== CheckConnected  (Called internally - Do not use in user programs)
Procedure CheckConnected()
   If Connected = 0
      printn("QMClient server function attempted when not connected")
      End
   EndIf
EndProcedure


;===== QMCall
Procedure QMCall0(Subrname.s)
   CheckConnected()
   CallCFunction(QMLib, "QMCall", Subrname, 0)
EndProcedure

Procedure QMCall1(Subrname.s, *Arg1)
   CheckConnected()
   CallCFunction(QMLib, "QMCall", Subrname, 1, *Arg1)
EndProcedure

Procedure QMCall2(Subrname.s, *Arg1, *Arg2)
   CheckConnected()
   CallCFunction(QMLib, "QMCall", Subrname, 2, *Arg1, *Arg2)
EndProcedure

Procedure QMCall3(Subrname.s, *Arg1, *Arg2, *Arg3)
   CheckConnected()
   CallCFunction(QMLib, "QMCall", Subrname, 3, *Arg1, *Arg2, *Arg3)
EndProcedure

Procedure QMCall4(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4)
   CheckConnected()
   CallCFunction(QMLib, "QMCall", Subrname, 4, *Arg1, *Arg2, *Arg3, *Arg4)
EndProcedure

Procedure QMCall5(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5)
   CheckConnected()
   CallCFunction(QMLib, "QMCall", Subrname, 5, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5)
EndProcedure

Procedure QMCall6(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6)
   CheckConnected()
   CallCFunction(QMLib, "QMCall", Subrname, 6, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6)
EndProcedure

Procedure QMCall7(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7)
   CheckConnected()
   CallCFunction(QMLib, "QMCall", Subrname, 7, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7)
EndProcedure

Procedure QMCall8(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8)
   CheckConnected()
   CallCFunction(QMLib, "QMCall", Subrname, 8, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8)
EndProcedure

Procedure QMCall9(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9)
   CheckConnected()
   CallCFunction(QMLib, "QMCall", Subrname, 9, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9)
EndProcedure

Procedure QMCall10(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10)
   CheckConnected()
   CallCFunction(QMLib, "QMCall", Subrname, 10, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10)
EndProcedure

Procedure QMCall11(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10, *Arg11)
   CheckConnected()
   CallCFunction(QMLib, "QMCall", Subrname, 11, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10, *Arg11)
EndProcedure

Procedure QMCall12(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10, *Arg11, *Arg12)
   CheckConnected()
   CallCFunction(QMLib, "QMCall", Subrname, 12, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10, *Arg11, *Arg12)
EndProcedure

Procedure QMCall13(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10, *Arg11, *Arg12, *Arg13)
   CheckConnected()
   CallCFunction(QMLib, "QMCall", Subrname, 13, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10, *Arg11, *Arg12, *Arg13)
EndProcedure

Procedure QMCall14(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10, *Arg11, *Arg12, *Arg13, *Arg14)
   CheckConnected()
   CallCFunction(QMLib, "QMCall", Subrname, 14, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10, *Arg11, *Arg12, *Arg13, *Arg14)
EndProcedure

Procedure QMCall15(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10, *Arg11, *Arg12, *Arg13, *Arg14, *Arg15)
   CheckConnected()
   CallCFunction(QMLib, "QMCall", Subrname, 15, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10, *Arg11, *Arg12, *Arg13, *Arg14, *Arg15)
EndProcedure

Procedure QMCall16(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10, *Arg11, *Arg12, *Arg13, *Arg14, *Arg15, *Arg16)
   CheckConnected()
   CallCFunction(QMLib, "QMCall", Subrname, 16, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10, *Arg11, *Arg12, *Arg13, *Arg14, *Arg15, *Arg16)
EndProcedure

Procedure QMCall17(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10, *Arg11, *Arg12, *Arg13, *Arg14, *Arg15, *Arg16, *Arg17)
   CheckConnected()
   CallCFunction(QMLib, "QMCall", Subrname, 17, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10, *Arg11, *Arg12, *Arg13, *Arg14, *Arg15, *Arg16, *Arg17)
EndProcedure

Procedure QMCall18(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10, *Arg11, *Arg12, *Arg13, *Arg14, *Arg15, *Arg16, *Arg17, *Arg18)
   CheckConnected()
   CallCFunction(QMLib, "QMCall", Subrname, 18, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10, *Arg11, *Arg12, *Arg13, *Arg14, *Arg15, *Arg16, *Arg17, *Arg18)
EndProcedure

Procedure QMCall19(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10, *Arg11, *Arg12, *Arg13, *Arg14, *Arg15, *Arg16, *Arg17, *Arg18, *Arg19)
   CheckConnected()
   CallCFunction(QMLib, "QMCall", Subrname, 19, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10, *Arg11, *Arg12, *Arg13, *Arg14, *Arg15, *Arg16, *Arg17, *Arg18, *Arg19)
EndProcedure

Procedure QMCall20(Subrname.s, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10, *Arg11, *Arg12, *Arg13, *Arg14, *Arg15, *Arg16, *Arg17, *Arg18, *Arg19, *Arg20)
   CheckConnected()
   CallCFunction(QMLib, "QMCall", Subrname, 20, *Arg1, *Arg2, *Arg3, *Arg4, *Arg5, *Arg6, *Arg7, *Arg8, *Arg9, *Arg10, *Arg11, *Arg12, *Arg13, *Arg14, *Arg15, *Arg16, *Arg17, *Arg18, *Arg19, *Arg20)
EndProcedure


;===== QMChange
Procedure.s QMChange(Src.s, Old.s, New.s, Occ.l, Start.l)
   OpenQMClientLibrary()
   *String = CallCFunction(QMLib, "QMChange", Src, Old, New, Occ, Start)
   If *string
      String$ = PeekS(*String)
      CallCFunction(QMLib, "QMFree", *String)
   EndIf
   ProcedureReturn String$
EndProcedure


;===== QMClearSelect
Procedure QMClearSelect(ListNo.l)
   CheckConnected()
   CallCFunction(QMLib, "QMClearSelect", ListNo)
EndProcedure


;===== QMClose
Procedure QMClose(Fileno.l)
   CheckConnected()
   CallCFunction(QMLib, "QMClose", Fileno)
EndProcedure


;===== QMConnect
Procedure.l QMConnect(Host.s, Port.l, User.s, Password.s, Account.s)
   OpenQMClientLibrary()
   If Connected
      QMDisconnect()
   EndIf
   Connected = CallCFunction(QMLib, "QMConnect", Host, Port, User, Password, Account)
   ProcedureReturn Connected
EndProcedure


;===== QMConnectLocal
Procedure.l QMConnectLocal(Account.s)
   OpenQMClientLibrary()
   If Connected
      QMDisconnect()
   EndIf
   Connected = CallCFunction(QMLib, "QMConnectLocal", Account)
   ProcedureReturn Connected
EndProcedure


;===== QMConnected
Procedure.l QMConnected()
   OpenQMClientLibrary()
   ProcedureReturn CallCFunction(QMLib, "QMConnected")
EndProcedure


;===== QMDcount
Procedure.l QMDcount(String.s, Delim.s)
   OpenQMClientLibrary()
   ProcedureReturn CallCFunction(QMLib, "QMDcount", String, Delim)
EndProcedure


;===== QMDel
Procedure.s QMDel(Src.s, Fno.l, Vno.l, SvNo.l)
   OpenQMClientLibrary()
   *String = CallCFunction(QMLib, "QMDel", Src, OFno, Vno, SvNo)
   If *string
      String$ = PeekS(*String)
      CallCFunction(QMLib, "QMFree", *String)
   EndIf
   ProcedureReturn String$
EndProcedure


;===== QMDelete
Procedure QMDelete(Fileno.l, Id.s)
   CheckConnected()
   CallCFunction(QMLib, "QMDelete", Fileno, Id)
EndProcedure


;===== QMDeleteu
Procedure QMDeleteu(Fileno.l, Id.s)
   CheckConnected()
   CallCFunction(QMLib, "QMDeleteu", Fileno, Id)
EndProcedure


;===== QMDisconnect
Procedure QMDisconnect()
   if Connected
      CallCFunction(QMLib, "QMDisconnect")
      Connected = 0
   EndIf
EndProcedure


;===== QMDisconnectAll
Procedure QMDisconnectAll()
   if Connected
      CallCFunction(QMLib, "QMDisconnectAll")
      Connected = 0
   EndIf
EndProcedure


;===== QMEndCommand
Procedure QMEndCommand()
   CheckConnected()
   CallCFunction(QMLib, "QMEndCommand")
EndProcedure


;===== QMError
Procedure.s QMError()
   OpenQMClientLibrary()
   *String = CallCFunction(QMLib, "QMError")
   If *string
      String$ = PeekS(*String)
      CallCFunction(QMLib, "QMFree", *String)
   EndIf
   ProcedureReturn String$
EndProcedure


;===== QMExecute
Procedure.s QMExecute(Cmd.s, *Err.l)
   CheckConnected()
   *String = CallCFunction(QMLib, "QMExecute", Cmd, *Err)
   If *string
      String$ = PeekS(*String)
      CallCFunction(QMLib, "QMFree", *String)
   EndIf
   ProcedureReturn String$
EndProcedure


;===== QMExtract
Procedure.s QMExtract(Src.s, Fno.l, Vno.l, SvNo.l)
   OpenQMClientLibrary()
   *String = CallCFunction(QMLib, "QMExtract", Src, OFno, Vno, SvNo)
   If *string
      String$ = PeekS(*String)
      CallCFunction(QMLib, "QMFree", *String)
   EndIf
   ProcedureReturn String$
EndProcedure


;===== QMField
Procedure.s QMField(Src.s, Delim.s, Start.l, Occ.l)
   OpenQMClientLibrary()
   *String = CallCFunction(QMLib, "QMField", Src, Delim, Start, Occ)
   If *string
      String$ = PeekS(*String)
      CallCFunction(QMLib, "QMFree", *String)
   EndIf
   ProcedureReturn String$
EndProcedure


;===== QMGetSession
Procedure.l QMGetSession()
   OpenQMClientLibrary()
   ProcedureReturn CallCFunction(QMLib, "QMGetSession")
EndProcedure


;===== QMIns
Procedure.s QMIns(Src.s, Fno.l, Vno.l, SvNo.l, New.s)
   OpenQMClientLibrary()
   *String = CallCFunction(QMLib, "QMIns", Src, OFno, Vno, SvNo, New)
   If *string
      String$ = PeekS(*String)
      CallCFunction(QMLib, "QMFree", *String)
   EndIf
   ProcedureReturn String$
EndProcedure


;===== QMLocate
Procedure.s QMLocate(Item.s, Dyn.s, Fno.l, Vno.l, SvNo.l, *Pos.l, Order.s)
   OpenQMClientLibrary()
   *String = CallCFunction(QMLib, "QMLocate", Item, Dyn, Fno, Vno, SvNo, *Pos, Order)
   If *string
      String$ = PeekS(*String)
      CallCFunction(QMLib, "QMFree", *String)
   EndIf
   ProcedureReturn String$
EndProcedure


;===== QMMatch
Procedure.l QMMatch(String.s, Template.s)
   OpenQMClientLibrary()
   ProcedureReturn CallCFunction(QMLib, "QMMatch", String, Template)
EndProcedure


;===== QMMatchfield
Procedure.s QMMatchfield(String.s, Template.s, Component.l)
   OpenQMClientLibrary()
   *String = CallCFunction(QMLib, "QMMatchfield", String, Template, Component)
   If *string
      String$ = PeekS(*String)
      CallCFunction(QMLib, "QMFree", *String)
   EndIf
   ProcedureReturn String$
EndProcedure


;===== QMOpen
Procedure.l QMOpen(Filename.s)
   CheckConnected()
   ProcedureReturn CallCFunction(QMLib, "QMOpen", Filename)
EndProcedure


;===== QMRead
Procedure.s QMRead(Fileno.l, Id.s, *Err.l)
   CheckConnected()
   *String = CallCFunction(QMLib, "QMRead", Fileno, Id, *Err)
   If *string
      String$ = PeekS(*String)
      CallCFunction(QMLib, "QMFree", *String)
   EndIf
   ProcedureReturn String$
EndProcedure


;===== QMReadl
Procedure.s QMReadl(Fileno.l, Id.s, Wait.l, *Err.l)
   CheckConnected()
   *String = CallCFunction(QMLib, "QMReadl", Fileno, Id, Wait, *Err)
   If *string
      String$ = PeekS(*String)
      CallCFunction(QMLib, "QMFree", *String)
   EndIf
   ProcedureReturn String$
EndProcedure


;===== QMReadList
Procedure.s QMReadList(Listno.l)
   CheckConnected()
   *String = CallCFunction(QMLib, "QMReadList", Listno)
   If *string
      String$ = PeekS(*String)
      CallCFunction(QMLib, "QMFree", *String)
   EndIf
   ProcedureReturn String$
EndProcedure


;===== QMReadNext
Procedure.s QMReadNext(Listno.l)
   CheckConnected()
   *String = CallCFunction(QMLib, "QMReadNext", Listno)
   If *string
      String$ = PeekS(*String)
      CallCFunction(QMLib, "QMFree", *String)
   EndIf
   ProcedureReturn String$
EndProcedure


;===== QMReadu
Procedure.s QMReadu(Fileno.l, Id.s, Wait.l, *Err.l)
   CheckConnected()
   *String = CallCFunction(QMLib, "QMReadu", Fileno, Id, Wait, *Err)
   If *string
      String$ = PeekS(*String)
      CallCFunction(QMLib, "QMFree", *String)
   EndIf
   ProcedureReturn String$
EndProcedure


;===== QMRecordlock
Procedure QMRecordlock(Fileno.l, Id.s, Update.l, Wait.l)
   CheckConnected()
   CallCFunction(QMLib, "QMRecordlock", Fileno, Id, Update, Wait)
EndProcedure


;===== QMRelease
Procedure QMRelease(Fileno.l, Id.s)
   CheckConnected()
   CallCFunction(QMLib, "QMRelease", Fileno, Id)
EndProcedure


;===== QMReplace
Procedure.s QMReplace(Src.s, Fno.l, Vno.l, SvNo.l, New.s)
   OpenQMClientLibrary()
   *String = CallCFunction(QMLib, "QMReplace", Src, OFno, Vno, SvNo, New)
   If *string
      String$ = PeekS(*String)
      CallCFunction(QMLib, "QMFree", *String)
   EndIf
   ProcedureReturn String$
EndProcedure


;===== QMRespond
Procedure.s QMRespond(Response.s, *Err.l)
   CheckConnected()
   *String = CallCFunction(QMLib, "QMRespond", Response, *Err)
   If *string
      String$ = PeekS(*String)
      CallCFunction(QMLib, "QMFree", *String)
   EndIf
   ProcedureReturn String$
EndProcedure


;===== QMSelect
Procedure QMSelect(Fileno.l, Listno.l)
   CheckConnected()
   CallCFunction(QMLib, "QMSelect", Fileno, Listno)
EndProcedure


;===== QMSelectIndex
Procedure QMSelectIndex(Fileno.l, IndexName.s, IndexValue.s, Listno.l)
   CheckConnected()
   CallCFunction(QMLib, "QMSelectIndex", Fileno, IndexName, IndexValue, Listno)
EndProcedure


;===== QMSelectLeft
Procedure.s QMSelectLeft(Fileno.l, IndexName.s, Listno.l)
   CheckConnected()
   *String = CallCFunction(QMLib, "QMSelectLeft", Fileno, IndexName, Listno)
   If *string
      String$ = PeekS(*String)
      CallCFunction(QMLib, "QMFree", *String)
   EndIf
   ProcedureReturn String$
EndProcedure


;===== QMSelectRight
Procedure.s QMSelectRight(Fileno.l, IndexName.s, Listno.l)
   CheckConnected()
   *String = CallCFunction(QMLib, "QMSelectRight", Fileno, IndexName, Listno)
   If *string
      String$ = PeekS(*String)
      CallCFunction(QMLib, "QMFree", *String)
   EndIf
   ProcedureReturn String$
EndProcedure


;===== QMSetLeft
Procedure QMSetLeft(Fileno.l, IndexName.s)
   CheckConnected()
   CallCFunction(QMLib, "QMSetLeft", Fileno, IndexName)
EndProcedure


;===== QMSetRight
Procedure QMSetRight(Fileno.l, IndexName.s)
   CheckConnected()
   CallCFunction(QMLib, "QMSetRight", Fileno, IndexName)
EndProcedure


;===== QMSetSession
Procedure.l QMSetSession(Idx.l)
   OpenQMClientLibrary()
   ProcedureReturn CallCFunction(QMLib, "QMSetSession", Idx)
EndProcedure


;===== QMStatus
Procedure.l QMStatus()
   OpenQMClientLibrary()
   ProcedureReturn CallCFunction(QMLib, "QMStatus")
EndProcedure


;===== QMWrite
Procedure QMWrite(Fileno.l, Id.s, Rec.s)
   CheckConnected()
   CallCFunction(QMLib, "QMWrite", Fileno, Id, Rec)
EndProcedure


;===== QMWriteu
Procedure QMWriteu(Fileno.l, Id.s, Rec.s)
   CheckConnected()
   CallCFunction(QMLib, "QMWriteu", Fileno, Id, Rec)
EndProcedure
