50 pascal -ret16 GlobalHandleToSel(word) GlobalHandleToSel16
51 pascal -ret16 GlobalFirst(ptr word) GlobalFirst16
52 pascal -ret16 GlobalNext(ptr word) GlobalNext16
53 pascal -ret16 GlobalInfo(ptr) GlobalInfo16
54 pascal -ret16 GlobalEntryHandle(ptr word) GlobalEntryHandle16
55 pascal -ret16 GlobalEntryModule(ptr word word) GlobalEntryModule16
56 pascal -ret16 LocalInfo(ptr word) LocalInfo16
57 pascal -ret16 LocalFirst(ptr word) LocalFirst16
58 pascal -ret16 LocalNext(ptr) LocalNext16
59 pascal -ret16 ModuleFirst(ptr) ModuleFirst16
60 pascal -ret16 ModuleNext(ptr) ModuleNext16
61 pascal -ret16 ModuleFindName(ptr ptr) ModuleFindName16
62 pascal -ret16 ModuleFindHandle(ptr word) ModuleFindHandle16
63 pascal -ret16 TaskFirst(ptr) TaskFirst16
64 pascal -ret16 TaskNext(ptr) TaskNext16
65 pascal -ret16 TaskFindHandle(ptr word) TaskFindHandle16
66 pascal -ret16 StackTraceFirst(ptr word) StackTraceFirst16
67 pascal -ret16 StackTraceCSIPFirst(ptr word word word word) StackTraceCSIPFirst16
68 pascal -ret16 StackTraceNext(ptr) StackTraceNext16
#69 pascal -ret16 ClassFirst(ptr) ClassFirst16
#70 pascal -ret16 ClassNext(ptr) ClassNext16
#FIXME: window classes are USER objects
69 stub ClassFirst
70 stub ClassNext
71 pascal -ret16 SystemHeapInfo(ptr) SystemHeapInfo16
72 pascal -ret16 MemManInfo(ptr) MemManInfo16
73 pascal -ret16 NotifyRegister(word segptr word) NotifyRegister16
74 pascal -ret16 NotifyUnregister(word) NotifyUnregister16
75 pascal -ret16 InterruptRegister(word segptr) InterruptRegister16
76 pascal -ret16 InterruptUnRegister(word) InterruptUnRegister16
77 pascal -ret16 TerminateApp(word word) TerminateApp16
78 pascal   MemoryRead(word long ptr long) MemoryRead16
79 pascal   MemoryWrite(word long ptr long) MemoryWrite16
80 pascal -ret16 TimerCount(ptr) TimerCount16
81 stub TASKSETCSIP
82 stub TASKGETCSIP
83 stub TASKSWITCH
84 pascal -ret16 Local32Info(ptr word) Local32Info16
85 pascal -ret16 Local32First(ptr word) Local32First16
86 pascal -ret16 Local32Next(ptr) Local32Next16
