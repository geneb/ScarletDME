Attribute VB_Name = "QMClient"
' QMClient.bas
' QMClient API function and constant definitions.
' Copyright (c) 2002, Ladybridge Systems, All Rights Reserved


Declare Sub QMCall Lib "QMClient.dll" (ByVal name As String, ByVal ArgCount As Integer, Optional ByRef a1 As String, Optional ByRef a2 As String, Optional ByRef a3 As String, Optional ByRef a4 As String, Optional ByRef a5 As String, Optional ByRef a6 As String, Optional ByRef a7 As String, Optional ByRef a8 As String, Optional ByRef a9 As String, Optional ByRef a10 As String, Optional ByRef a11 As String, Optional ByRef a12 As String, Optional ByRef a13 As String, Optional ByRef a14 As String, Optional ByRef a15 As String, Optional ByRef a16 As String, Optional ByRef a17 As String, Optional ByRef a18 As String, Optional ByRef a19 As String, Optional ByRef a20 As String)
Declare Function QMChange Lib "QMClient.dll" (ByVal Str As String, ByVal OldStr As String, ByVal NewStr As String, Optional ByRef Occurrences As Long, Optional ByRef Start As Long) As String
Declare Function QMConnect Lib "QMClient.dll" (ByVal Host As String, ByVal Port As Integer, ByVal UserName As String, ByVal Password As String, ByVal Account As String) As Boolean
Declare Function QMConnectLocal Lib "QMClient.dll" (ByVal Account As String) As Boolean
Declare Function QMConnected Lib "QMClient.dll" () As Boolean
Declare Sub QMClearSelect Lib "QMClient.dll" (ByVal ListNo As Integer)
Declare Sub QMClose Lib "QMClient.dll" (ByVal FileNo As Integer)
Declare Function QMDcount Lib "QMClient.dll" (ByVal Src As String, ByVal Delim As String) As Long
Declare Function QMDel Lib "QMClient.dll" (ByVal Src As String, ByVal Fno As Integer, ByVal Vno As Integer, ByVal Svno As Integer) As String
Declare Sub QMDelete Lib "QMClient.dll" (ByVal FileNo As Integer, ByVal Id As String)
Declare Sub QMDeleteu Lib "QMClient.dll" (ByVal FileNo As Integer, ByVal Id As String)
Declare Sub QMDisconnect Lib "QMClient.dll" ()
Declare Sub QMDisconnectAll Lib "QMClient.dll" ()
Declare Sub QMEndCommand Lib "QMClient.dll" ()
Declare Function QMError Lib "QMClient.dll" () As String
Declare Function QMExecute Lib "QMClient.dll" (ByVal Cmnd As String, ByRef ErrNo As Integer) As String
Declare Function QMExtract Lib "QMClient.dll" (ByVal Src As String, ByVal Fno As Integer, ByVal Vno As Integer, ByVal Svno As Integer) As String
Declare Function QMField Lib "QMClient.dll" (ByVal Str As String, ByVal Delimited As String, ByVal Start As Long, Optional ByRef Occurrences As Long) As String
Declare Function QMGetSession Lib "QMClient.dll" () As Integer
Declare Function QMIns Lib "QMClient.dll" (ByVal Src As String, ByVal Fno As Integer, ByVal Vno As Integer, ByVal Svno As Integer, ByVal NewData As String) As String
Declare Function QMLocate Lib "QMClient.dll" (ByVal Item As String, ByVal Src As String, ByVal Fno As Integer, ByVal Vno As Integer, ByVal Svno As Integer, ByRef Pos As Integer, ByVal Order As String) As Boolean
Declare Function QMLogto Lib "QMClient.dll" (ByVal Cmnd As String) As Boolean
Declare Function QMMarkMapping Lib "QMClient.dll" (ByVal FileNo As Integer, ByVal State As Integer)
Declare Function QMMatch Lib "QMClient.dll" (ByVal Src As String, ByVal Template As String) As Boolean
Declare Function QMMatchfield Lib "QMClient.dll" (ByVal Src As String, ByVal Template As String, ByVal Component As Integer) As String
Declare Function QMOpen Lib "QMClient.dll" (ByVal Filename As String) As Integer
Declare Function QMRead Lib "QMClient.dll" (ByVal FileNo As Integer, ByVal Id As String, ByRef ErrNo As Integer) As String
Declare Function QMReadl Lib "QMClient.dll" (ByVal FileNo As Integer, ByVal Id As String, ByVal Wait As Boolean, ByRef ErrNo As Integer) As String
Declare Function QMReadList Lib "QMClient.dll" (ByVal ListNo As Integer, ByRef ErrNo As Integer) As String
Declare Function QMReadNext Lib "QMClient.dll" (ByVal ListNo As Integer, ByRef ErrNo As Integer) As String
Declare Function QMReadu Lib "QMClient.dll" (ByVal FileNo As Integer, ByVal Id As String, ByVal Wait As Boolean, ByRef ErrNo As Integer) As String
Declare Sub QMRecordlock Lib "QMClient.dll" (ByVal FileNo As Integer, ByVal Id As String, ByVal UpdateLock as Integer, ByVal Wait as Integer)
Declare Sub QMRelease Lib "QMClient.dll" (ByVal FileNo As Integer, ByVal Id As String)
Declare Function QMReplace Lib "QMClient.dll" (ByVal Src As String, ByVal Fno As Integer, ByVal Vno As Integer, ByVal Svno As Integer, ByVal NewData As String) As String
Declare Function QMRespond Lib "QMClient.dll" (ByVal Response As String, ByRef ErrNo As Integer) As String
Declare Sub QMSelect Lib "QMClient.dll" (ByVal FileNo As Integer, ByVal ListNo As Integer)
Declare Sub QMSelectIndex Lib "QMClient.dll" (ByVal FileNo As Integer, ByVal IndexName as String, ByVal IndexValue as String, ByVal ListNo As Integer)
Declare Function QMSelectLeft Lib "QMClient.dll" (ByVal FileNo As Integer, ByVal IndexName as String, ByVal ListNo As Integer) as String
Declare Function QMSelectRight Lib "QMClient.dll" (ByVal FileNo As Integer, ByVal IndexName as String, ByVal ListNo As Integer) as String
Declare Sub QMSetLeft Lib "QMClient.dll" (ByVal FileNo As Integer, ByVal IndexName as String)
Declare Sub QMSetRight Lib "QMClient.dll" (ByVal FileNo As Integer, ByVal IndexName as String)
Declare Function QMSetSession Lib "QMClient.dll" (ByVal Session as Integer) As Boolean
Declare Function QMStatus Lib "QMClient.dll" () As Long
Declare Sub QMWrite Lib "QMClient.dll" (ByVal FileNo As Integer, ByVal Id As String, ByVal Data As String)
Declare Sub QMWriteu Lib "QMClient.dll" (ByVal FileNo As Integer, ByVal Id As String, ByVal Data As String)


Global Const SV_OK = 0        ' Action successul
Global Const SV_ON_ERROR = 1  ' Trapped abort
Global Const SV_ELSE = 2      ' Action took the ELSE clause
Global Const SV_ERROR = 3     ' Error for which QMError() can be used to retrieve error text
Global Const SV_LOCKED = 4    ' Action blocked by a lock held by another user
Global Const SV_PROMPT = 5    ' Server requesting input

Global Const QMDateOffset = 24837
