name	toolhelp
type	win16
owner	kernel32

50 pascal16 GlobalHandleToSel(word) GlobalHandleToSel16
51 pascal16 GlobalFirst(ptr word) GlobalFirst16
52 pascal16 GlobalNext(ptr word) GlobalNext16
53 pascal16 GlobalInfo(ptr) GlobalInfo16
54 pascal16 GlobalEntryHandle(ptr word) GlobalEntryHandle16
55 pascal16 GlobalEntryModule(ptr word word) GlobalEntryModule16
56 pascal16 LocalInfo(ptr word) LocalInfo16
57 pascal16 LocalFirst(ptr word) LocalFirst16
58 pascal16 LocalNext(ptr) LocalNext16
59 pascal16 ModuleFirst(ptr) ModuleFirst16
60 pascal16 ModuleNext(ptr) ModuleNext16
61 pascal16 ModuleFindName(ptr ptr) ModuleFindName16
62 pascal16 ModuleFindHandle(ptr word) ModuleFindHandle16
63 pascal16 TaskFirst(ptr) TaskFirst16
64 pascal16 TaskNext(ptr) TaskNext16
65 pascal16 TaskFindHandle(ptr word) TaskFindHandle16
66 pascal16 StackTraceFirst(ptr word) StackTraceFirst16
67 pascal16 StackTraceCSIPFirst(ptr word word word word) StackTraceCSIPFirst16
68 pascal16 StackTraceNext(ptr) StackTraceNext16
#69 pascal16 ClassFirst(ptr) ClassFirst16
#70 pascal16 ClassNext(ptr) ClassNext16
#FIXME: window classes are USER objects
69 stub ClassFirst
70 stub ClassNext
71 pascal16 SystemHeapInfo(ptr) SystemHeapInfo16
72 pascal16 MemManInfo(ptr) MemManInfo16
73 pascal16 NotifyRegister(word segptr word) NotifyRegister16
74 pascal16 NotifyUnregister(word) NotifyUnregister16
75 pascal16 InterruptRegister(word segptr) InterruptRegister16
76 pascal16 InterruptUnRegister(word) InterruptUnRegister16
77 stub TERMINATEAPP
78 pascal   MemoryRead(word long ptr long) MemoryRead16
79 pascal   MemoryWrite(word long ptr long) MemoryWrite16
80 pascal16 TimerCount(ptr) TimerCount16
81 stub TASKSETCSIP
82 stub TASKGETCSIP
83 stub TASKSWITCH
84 pascal16 Local32Info(ptr word) Local32Info16
85 pascal16 Local32First(ptr word) Local32First16
86 pascal16 Local32Next(ptr) Local32Next16
