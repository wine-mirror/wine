name	shell
type	win16

  1 pascal   RegOpenKey(long str ptr) RegOpenKey16
  2 pascal   RegCreateKey(long str ptr) RegCreateKey16
  3 pascal   RegCloseKey(long) RegCloseKey
  4 pascal   RegDeleteKey(long str) RegDeleteKey16
  5 pascal   RegSetValue(long str long str long) RegSetValue16
  6 pascal   RegQueryValue(long str ptr ptr) RegQueryValue16
  7 pascal   RegEnumKey(long long ptr long) RegEnumKey16
  9 pascal16 DragAcceptFiles(word word) DragAcceptFiles
 11 pascal16 DragQueryFile(word s_word ptr s_word) DragQueryFile
 12 pascal16 DragFinish(word) DragFinish
 13 pascal16 DragQueryPoint(word ptr) DragQueryPoint
 20 pascal16 ShellExecute(word str str str str s_word) ShellExecute16
 21 pascal16 FindExecutable(str str ptr) FindExecutable16
 22 pascal16 ShellAbout(word ptr ptr word) ShellAbout16
 33 pascal16 AboutDlgProc(word word word long) AboutDlgProc16
 34 pascal16 ExtractIcon(word str s_word) ExtractIcon16
 36 pascal16 ExtractAssociatedIcon(word ptr ptr) ExtractAssociatedIcon16
 37 pascal   DoEnvironmentSubst(ptr word) DoEnvironmentSubst
 38 pascal   FindEnvironmentString(ptr) FindEnvironmentString
 39 pascal16 InternalExtractIcon(word ptr s_word word) InternalExtractIcon
 40 stub ExtractIconEx
# 98 stub SHL3216_THUNKDATA16
# 99 stub SHL1632_THUNKDATA16

#100   4  0550  HERETHARBETYGARS exported, shared data
#101   8  010e  FINDEXEDLGPROC exported, shared data
#101 DLLENTRYPOINT #win95 SHELL.DLL

102 pascal16 RegisterShellHook(word word) RegisterShellHook
103 pascal   ShellHookProc(word word long) ShellHookProc

157 stub RESTARTDIALOG
#  166 PICKICONDLG

262 pascal16 DriveType(long) GetDriveType16

#  263 SH16TO32DRIVEIOCTL
#  264 SH16TO32INT2526
#  300 SHGETFILEINFO
#  400 SHFORMATDRIVE
401 stub SHCHECKDRIVE
#  402 _RUNDLLCHECKDRIVE

# 8 WEP 
#32 WCI
