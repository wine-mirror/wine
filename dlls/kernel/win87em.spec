name	win87em
type	win16
owner	kernel32

1 register _fpMath() WIN87_fpmath
3 pascal16 __WinEm87Info(ptr word) WIN87_WinEm87Info
4 pascal16 __WinEm87Restore(ptr word) WIN87_WinEm87Restore
5 pascal16 __WinEm87Save(ptr word) WIN87_WinEm87Save
