name	system
type	win16

1 pascal   InquireSystem(word word) InquireSystem16
2 pascal16 CreateSystemTimer(word segptr) WIN16_CreateSystemTimer
3 pascal16 KillSystemTimer(word) SYSTEM_KillSystemTimer
4 pascal16 EnableSystemTimers() EnableSystemTimers16
5 pascal16 DisableSystemTimers() DisableSystemTimers16
6 pascal   GetSystemMSecCount() GetTickCount
7 pascal16 Get80x87SaveSize() Get80x87SaveSize16
8 pascal16 Save80x87State(ptr) Save80x87State16
9 pascal16 Restore80x87State(ptr) Restore80x87State16
20 pascal16 A20_Proc(word) A20Proc16
