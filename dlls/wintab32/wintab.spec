20 pascal16 WTInfo(word word ptr) WTInfo16
21 pascal16 WTOpen(word ptr word) WTOpen16
22 pascal16 WTClose(word) WTClose16
23 pascal16 WTPacketsGet(word s_word ptr) WTPacketsGet16
24 pascal16 WTPacket(word word ptr) WTPacket16
40 pascal16 WTEnable(word word) WTEnable16
41 pascal16 WTOverlap(word word) WTOverlap16
60 pascal16 WTConfig(word word) WTConfig16
61 pascal16 WTGet(word ptr) WTGet16
62 pascal16 WTSet(word ptr) WTSet16
63 pascal16 WTExtGet(word word ptr) WTExtGet16
64 pascal16 WTExtSet(word word ptr) WTExtSet16
65 pascal16 WTSave(word ptr) WTSave16
66 pascal16 WTRestore(word ptr word) WTRestore16
80 pascal16 WTPacketsPeek(word s_word ptr) WTPacketsPeek16
81 pascal16 WTDataGet(word word word s_word ptr ptr) WTDataGet16
82 pascal16 WTDataPeek(word word word s_word ptr ptr) WTDataPeek16
83 pascal   WTQueuePackets(word) WTQueuePackets16
84 pascal16 WTQueueSizeGet(word) WTQueueSizeGet16
85 pascal16 WTQueueSizeSet(word s_word) WTQueueSizeSet16
100 pascal16 WTMgrOpen(word word) WTMgrOpen16
101 pascal16 WTMgrClose(word) WTMgrClose16
120 pascal16 WTMgrContextEnum(word ptr long) WTMgrContextEnum16
121 pascal16 WTMgrContextOwner(word word) WTMgrContextOwner16
122 pascal16 WTMgrDefContext(word word) WTMgrDefContext16
140 pascal16 WTMgrDeviceConfig(word word word) WTMgrDeviceConfig16
141 pascal16 WTMgrConfigReplace(word word ptr) WTMgrConfigReplace16
160 pascal   WTMgrPacketHook(word word s_word ptr) WTMgrPacketHook16
161 pascal   WTMgrPacketHookDefProc(s_word word long ptr) WTMgrPacketHookDefProc16
180 pascal16 WTMgrExt(word word ptr) WTMgrExt16
181 pascal16 WTMgrCsrEnable(word word word) WTMgrCsrEnable16
182 pascal16 WTMgrCsrButtonMap(word word ptr ptr) WTMgrCsrButtonMap16
183 pascal16 WTMgrCsrPressureBtnMarks(word word long long) WTMgrCsrPressureBtnMarks16
184 pascal16 WTMgrCsrPressureResponse(word word ptr ptr) WTMgrCsrPressureResponse16
185 pascal16 WTMgrCsrExt(word word word ptr) WTMgrCsrExt16
200 pascal16 WTQueuePacketsEx(word ptr ptr) WTQueuePacketsEx16
201 pascal16 WTMgrCsrPressureBtnMarksEx(word word ptr ptr) WTMgrCsrPressureBtnMarksEx16
202 pascal16 WTMgrConfigReplaceEx(word word str str) WTMgrConfigReplaceEx16
203 pascal16 WTMgrPacketHookEx(word s_word str str) WTMgrPacketHookEx16
204 pascal16 WTMgrPacketUnhook(word) WTMgrPacketUnhook16
205 pascal16 WTMgrPacketHookNext(word s_word word long) WTMgrPacketHookNext16
206 pascal16 WTMgrDefContextEx(word word word) WTMgrDefContextEx16
