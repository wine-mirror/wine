name	shell
id	5

#
# WARNING ! These functions are not documented, so I didn't look for
# 			proper parameters. It's just to have stub for PROGMAN.EXE ...
#

  1 pascal   RegOpenKey(long ptr ptr) RegOpenKey
  2 pascal   RegCreateKey(long ptr ptr) RegCreateKey
  3 pascal   RegCloseKey(long) RegCloseKey
  4 pascal   RegDeleteKey(long ptr) RegDeleteKey
  5 pascal   RegSetValue(long ptr long ptr long) RegSetValue
  6 pascal   RegQueryValue(long ptr ptr ptr) RegQueryValue
  7 pascal   RegEnumKey(long long ptr long) RegEnumKey
  9 pascal16 DragAcceptFiles(word word) DragAcceptFiles
 11 pascal16 DragQueryFile(word s_word ptr s_word) DragQueryFile
 12 pascal16 DragFinish(word) DragFinish
 13 pascal16 DragQueryPoint(word ptr) DragQueryPoint
 20 pascal16 ShellExecute(word ptr ptr ptr ptr s_word) ShellExecute 
 21 pascal16 FindExecutable(ptr ptr ptr) FindExecutable
 22 pascal16 ShellAbout(word ptr ptr word) ShellAbout
 33 pascal16 AboutDlgProc(word word word long) AboutDlgProc
 34 pascal16 ExtractIcon(word ptr s_word) ExtractIcon
 36 pascal16 ExtractAssociatedIcon(word ptr ptr) ExtractAssociatedIcon
 37 pascal   DoEnvironmentSubst(ptr word) DoEnvironmentSubst
 38 stub FindEnvironmentString
 39 stub InternalExtractIcon
102 pascal16 RegisterShellHook(ptr) RegisterShellHook
103 pascal16 ShellHookProc() ShellHookProc

#  8   7  0000  WEP exported, shared data
#100   4  0550  HERETHARBETYGARS exported, shared data
# 38   5  0000  FINDENVIRONMENTSTRING exported, shared data
# 37   5  00ae  DOENVIRONMENTSUBST exported, shared data
# 20   4  110a  SHELLEXECUTE exported, shared data
#101   8  010e  FINDEXEDLGPROC exported, shared data
# 39  10  026e  INTERNALEXTRACTICON exported, shared data
# 32   9  0829  WCI exported, shared data
# 36  10  08dc  EXTRACTASSOCIATEDICON exported, shared data


