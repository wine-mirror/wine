name	w32sys
type	win16

#1 WEP
2 pascal16 IsPeFormat(str word) IsPeFormat
3 stub EXECPE
4 stub GETPEEXEINFO
5 return GETW32SYSVERSION 0 0x100
6 stub LOADPERESOURCE
7 pascal16 GetPEResourceTable(word) GetPEResourceTable
8 stub EXECPEEX
9 stub ITSME
10 stub W32SERROR
11 stub EXP1
12 pascal16 GetWin32sInfo(ptr) GetWin32sInfo
