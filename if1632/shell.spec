# $Id: shell.spec,v 1.3 1993/07/04 04:04:21 root Exp root $
#
name	shell
id	6
length	103

#
# WARNING ! These functions are not documented, so I didn't look for
# 			proper parameters. It's just to have stub for PROGMAN.EXE ...
#

  1 pascal RegOpenKey(word ptr ptr) RegOpenKey(1 2 3)
  2 pascal RegCreateKey(word ptr ptr) RegCreateKey(1 2 3)
  3 pascal RegCloseKey(word) RegCloseKey(1)
  4 pascal RegDeleteKey(word ptr) RegDeleteKey(1 2)
  5 pascal RegSetValue(word ptr long ptr long) RegSetValue(1 2 3 4 5)
  6 pascal RegQueryValue(word ptr ptr ptr) RegQueryValue(1 2 3 4)
  7 pascal RegEnumKey(word long ptr long) RegEnumKey(1 2 3 4)
  9 pascal DragAcceptFiles(word word) DragAcceptFiles(1 2)
 11 pascal DragQueryFile(word s_word ptr s_word) DragQueryFile(1 2 3 4)
 12 pascal DragFinish(word) DragFinish(1)
 13 pascal DragQueryPoint(word ptr) DragQueryPoint(1 2)
 20 pascal ShellExecute(word ptr ptr ptr ptr s_word) ShellExecute(1 2 3 4 5 6) 
 21 pascal FindExecutable(ptr ptr ptr) FindExecutable(1 2 3)
 22 pascal ShellAbout(word ptr ptr word) ShellAbout(1 2 3 4)
 33 pascal AboutDlgProc(word word word long) AboutDlgProc(1 2 3 4)
 34 pascal ExtractIcon(word ptr s_word) ExtractIcon(1 2 3)
102 pascal RegisterShellHook(ptr) RegisterShellHook(1)
103 pascal ShellHookProc() ShellHookProc()

#  8   7  0000  WEP exported, shared data
#100   4  0550  HERETHARBETYGARS exported, shared data
# 38   5  0000  FINDENVIRONMENTSTRING exported, shared data
# 37   5  00ae  DOENVIRONMENTSUBST exported, shared data
# 20   4  110a  SHELLEXECUTE exported, shared data
#101   8  010e  FINDEXEDLGPROC exported, shared data
# 39  10  026e  INTERNALEXTRACTICON exported, shared data
# 32   9  0829  WCI exported, shared data
# 36  10  08dc  EXTRACTASSOCIATEDICON exported, shared data


