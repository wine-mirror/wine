name	toolhelp
id	12

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
66 stub STACKTRACEFIRST
67 stub STACKTRACECSIPFIRST
68 stub STACKTRACENEXT
69 pascal16 ClassFirst(ptr) ClassFirst
70 pascal16 ClassNext(ptr) ClassNext
71 pascal16 SystemHeapInfo(ptr) SystemHeapInfo
72 pascal16 MemManInfo(ptr) MemManInfo
73 stub NOTIFYREGISTER
74 stub NOTIFYUNREGISTER
75 stub INTERRUPTREGISTER
76 stub INTERRUPTUNREGISTER
77 stub TERMINATEAPP
78 pascal   MemoryRead(word long ptr long) MemoryRead
79 pascal   MemoryWrite(word long ptr long) MemoryWrite
80 stub TIMERCOUNT
81 stub TASKSETCSIP
82 stub TASKGETCSIP
83 stub TASKSWITCH
