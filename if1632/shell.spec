# $Id: shell.spec,v 1.3 1993/07/04 04:04:21 root Exp root $
#
name	shell
id	6
length	256

#
# WARNING ! These functions are not documented, so I didn't look for
# 			proper parameters. It's just to have stub for PROGMAN.EXE ...
#

  1 pascal RegOpenKey() RegOpenKey()
  2 pascal RegCreateKey() RegCreateKey()
  3 pascal RegCloseKey() RegCloseKey()
  4 pascal RegDeleteKey() RegDeleteKey()
 20 pascal ShellExecute(ptr) ShellExecute(1) 
102 pascal RegisterShellHook(ptr) RegisterShellHook(1)
103 pascal ShellHookProc() ShellHookProc()

#  8   7  0000  WEP exported, shared data
# 33   9  0136  ABOUTDLGPROC exported, shared data
# 34  10  021a  EXTRACTICON exported, shared data
# 21   4  1154  FINDEXECUTABLE exported, shared data
#  9   6  0052  DRAGACCEPTFILES exported, shared data
#100   4  0550  HERETHARBETYGARS exported, shared data
# 38   5  0000  FINDENVIRONMENTSTRING exported, shared data
#  7   2  14dc  REGENUMKEY exported, shared data
# 37   5  00ae  DOENVIRONMENTSUBST exported, shared data
# 20   4  110a  SHELLEXECUTE exported, shared data
#101   8  010e  FINDEXEDLGPROC exported, shared data
# 11   6  0094  DRAGQUERYFILE exported, shared data
# 13   6  0000  DRAGQUERYPOINT exported, shared data
#  5   2  16f4  REGSETVALUE exported, shared data
# 39  10  026e  INTERNALEXTRACTICON exported, shared data
# 22   9  0000  SHELLABOUT exported, shared data
#  6   2  168e  REGQUERYVALUE exported, shared data
# 32   9  0829  WCI exported, shared data
# 36  10  08dc  EXTRACTASSOCIATEDICON exported, shared data
# 12   6  0142  DRAGFINISH exported, shared data


