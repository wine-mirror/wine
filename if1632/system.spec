name	system
type	win16

1 pascal   InquireSystem(word word) InquireSystem
2 pascal16 CreateSystemTimer(word segptr) CreateSystemTimer
3 pascal16 KillSystemTimer(word) SYSTEM_KillSystemTimer
4 pascal16 EnableSystemTimers() EnableSystemTimers
5 pascal16 DisableSystemTimers() DisableSystemTimers
6 pascal   GetSystemMSecCount() GetTickCount
7 return Get80x87SaveSize 0 94
8 stub Save80x87State
9 stub Restore80x87State
#20 stub A20_Proc
