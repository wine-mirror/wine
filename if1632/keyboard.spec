# $Id: keyboard.spec,v 1.1 1993/09/10 05:32:12 scott Exp $
#
name	keyboard
id	8
length	137

#1	pascal	Inquire
#2	pascal	Enable
#3	pascal	Disable
4	pascal	ToAscii(word word ptr ptr word) ToAscii(1 2 3 4 5)
5	pascal	AnsiToOem(ptr ptr) AnsiToOem(1 2)
6	pascal	OemToAnsi(ptr ptr) OemToAnsi(1 2)
#7	pascal	SetSpeed
#100	pascal	ScreenSwitchEnable
#126	pascal	GetTableSeg
#127	pascal	NewTable
128	pascal	OemKeyScan(word) OemKeyScan(1)
129	pascal	VkKeyScan(byte) VkKeyScan(1)
130	pascal	GetKeyboardType(byte) GetKeyboardType(1)
131	pascal	MapVirtualKey(word word) MapVirtualKey(1 2)
132	pascal	GetKbCodePage() GetKbCodePage()
133	pascal	GetKeyNameText(long ptr word) GetKeyNameText(1 2 3)
134	pascal	AnsiToOemBuff(ptr ptr word) AnsiToOemBuff(1 2 3)
135	pascal	OemToAnsiBuff(ptr ptr word) OemToAnsiBuff(1 2 3)
#136	pascal	EnableKbSysReq
#137	pascal	GetBiosKeyProc

