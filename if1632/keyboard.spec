name	keyboard
type	win16

#1	pascal	Inquire
#2	pascal	Enable
#3	pascal	Disable
4   pascal16 ToAscii(word word ptr ptr word) ToAscii
5   pascal16 AnsiToOem(ptr ptr) AnsiToOem
6   pascal16 OemToAnsi(ptr ptr) OemToAnsi
7   return   SetSpeed 2 65535
#100	pascal	ScreenSwitchEnable
#126	pascal	GetTableSeg
#127	pascal	NewTable
128 pascal   OemKeyScan(word) OemKeyScan
129 pascal16 VkKeyScan(byte) VkKeyScan
130 pascal16 GetKeyboardType(byte) GetKeyboardType
131 pascal16 MapVirtualKey(word word) MapVirtualKey
132 pascal16 GetKbCodePage() GetKbCodePage
133 pascal16 GetKeyNameText(long ptr word) GetKeyNameText
134 pascal16 AnsiToOemBuff(ptr ptr word) AnsiToOemBuff
135 pascal16 OemToAnsiBuff(ptr ptr word) OemToAnsiBuff
#136	pascal	EnableKbSysReq
#137	pascal	GetBiosKeyProc

