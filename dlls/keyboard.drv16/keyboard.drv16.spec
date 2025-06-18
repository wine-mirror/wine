1   pascal -ret16 Inquire(ptr) Inquire16
2   pascal -ret16 Enable(segptr ptr) Enable16
3   pascal -ret16 Disable() Disable16
4   pascal -ret16 ToAscii(word word ptr ptr word) ToAscii16
5   pascal -ret16 AnsiToOem(str ptr) AnsiToOem16
6   pascal -ret16 OemToAnsi(str ptr) OemToAnsi16
7   pascal -ret16 SetSpeed(word) SetSpeed16
#8   stub WEP
9   stub INQUIREEX
10  stub TOASCIIEX
11  stub VKKEYSCANEX
12  stub MAPVIRTUALKEYEX
13  stub NEWTABLEEX
14  stub __EXPORTEDSTUB

100	pascal	ScreenSwitchEnable(word) ScreenSwitchEnable16
#126	pascal	GetTableSeg
#127	pascal	NewTable
128 pascal OemKeyScan(word) OemKeyScan16
129 pascal -ret16 VkKeyScan(word) VkKeyScan16
130 pascal -ret16 GetKeyboardType(word) GetKeyboardType16
131 pascal -ret16 MapVirtualKey(word word) MapVirtualKey16
132 pascal -ret16 GetKBCodePage() GetKBCodePage16
133 pascal -ret16 GetKeyNameText(long ptr word) GetKeyNameText16
134 pascal -ret16 AnsiToOemBuff(ptr ptr word) AnsiToOemBuff16
135 pascal -ret16 OemToAnsiBuff(ptr ptr word) OemToAnsiBuff16
#136	pascal	EnableKbSysReq
#137	pascal	GetBiosKeyProc
