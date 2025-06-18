1 pascal InquireSystem(word word) InquireSystem16
2 pascal -ret16 CreateSystemTimer(word segptr) CreateSystemTimer16
3 pascal -ret16 KillSystemTimer(word) SYSTEM_KillSystemTimer
4 pascal -ret16 EnableSystemTimers() EnableSystemTimers16
5 pascal -ret16 DisableSystemTimers() DisableSystemTimers16
6 pascal GetSystemMSecCount() GetSystemMSecCount16
7 pascal -ret16 Get80x87SaveSize() Get80x87SaveSize16
8 pascal -ret16 Save80x87State(ptr) Save80x87State16
9 pascal -ret16 Restore80x87State(ptr) Restore80x87State16
13 stub INQUIRELONGINTS # W1.1, W2.0
#14 stub ordinal only W1.1
20 pascal -ret16 A20_Proc(word) A20_Proc16
