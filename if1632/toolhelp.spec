name	toolhelp
id	12
length	83

50 pascal16 GlobalHandleToSel(word) GlobalHandleToSel
51 pascal16 GlobalFirst(ptr word) GlobalFirst
52 pascal16 GlobalNext(ptr word) GlobalNext
53 pascal16 GlobalInfo(ptr) GlobalInfo
54 pascal16 GlobalEntryHandle(ptr word) GlobalEntryHandle
55 pascal16 GlobalEntryModule(ptr word word) GlobalEntryModule
56 pascal16 LocalInfo(ptr word) LocalInfo
57 pascal16 LocalFirst(ptr word) LocalFirst
58 pascal16 LocalNext(ptr) LocalNext
59 pascal16 ModuleFirst(ptr) ModuleFirst
60 pascal16 ModuleNext(ptr) ModuleNext
61 pascal16 ModuleFindName(ptr ptr) ModuleFindName
62 pascal16 ModuleFindHandle(ptr word) ModuleFindHandle
63 pascal16 TaskFirst(ptr) TaskFirst
64 pascal16 TaskNext(ptr) TaskNext
65 pascal16 TaskFindHandle(ptr word) TaskFindHandle
# 66   1  0f1c  STACKTRACEFIRST exported, shared data
# 67   1  0f67  STACKTRACECSIPFIRST exported, shared data
# 68   1  0fca  STACKTRACENEXT exported, shared data
69 pascal16 ClassFirst(ptr) ClassFirst
70 pascal16 ClassNext(ptr) ClassNext
71 pascal16 SystemHeapInfo(ptr) SystemHeapInfo
72 pascal16 MemManInfo(ptr) MemManInfo
# 73   1  1b72  NOTIFYREGISTER exported, shared data
# 74   1  1c29  NOTIFYUNREGISTER exported, shared data
# 75   1  2060  INTERRUPTREGISTER exported, shared data
# 76   1  2111  INTERRUPTUNREGISTER exported, shared data
# 77   1  26ea  TERMINATEAPP exported, shared data
78 pascal MemoryRead(word long ptr long) MemoryRead
79 pascal MemoryWrite(word long ptr long) MemoryWrite
# 80   1  2dae  TIMERCOUNT exported, shared data
# 81   1  0d68  TASKSETCSIP exported, shared data
# 82   1  0d97  TASKGETCSIP exported, shared data
# 83   1  0dc0  TASKSWITCH exported, shared data
