20 stdcall WTInfoA(long long ptr) WTInfoA
21 stdcall WTOpenA(long ptr long) WTOpenA
22 stdcall WTClose(long) WTClose
23 stdcall WTPacketsGet(long long ptr) WTPacketsGet
24 stdcall WTPacket(long long ptr) WTPacket
40 stdcall WTEnable(long long) WTEnable
41 stdcall WTOverlap(long long) WTOverlap
60 stdcall WTConfig(long long) WTConfig
61 stdcall WTGetA(long ptr) WTGetA
62 stdcall WTSetA(long ptr) WTSetA
63 stdcall WTExtGet(long long ptr) WTExtGet
64 stdcall WTExtSet(long long ptr) WTExtSet
65 stdcall WTSave(long ptr) WTSave
66 stdcall WTRestore(long ptr long) WTRestore
80 stdcall WTPacketsPeek(long long ptr) WTPacketsPeek
81 stdcall WTDataGet(long long long long ptr ptr) WTDataGet
82 stdcall WTDataPeek(long long long long ptr ptr) WTDataPeek
84 stdcall WTQueueSizeGet(long) WTQueueSizeGet
85 stdcall WTQueueSizeSet(long long) WTQueueSizeSet
100 stdcall WTMgrOpen(long long) WTMgrOpen
101 stdcall WTMgrClose(long) WTMgrClose
120 stdcall WTMgrContextEnum(long ptr long) WTMgrContextEnum
121 stdcall WTMgrContextOwner(long long) WTMgrContextOwner
122 stdcall WTMgrDefContext(long long) WTMgrDefContext
140 stdcall WTMgrDeviceConfig(long long long) WTMgrDeviceConfig
180 stdcall WTMgrExt(long long ptr) WTMgrExt
181 stdcall WTMgrCsrEnable(long long long) WTMgrCsrEnable
182 stdcall WTMgrCsrButtonMap(long long ptr ptr) WTMgrCsrButtonMap
183 stdcall WTMgrCsrPressureBtnMarks(long long long long) WTMgrCsrPressureBtnMarks
184 stdcall WTMgrCsrPressureResponse(long long ptr ptr) WTMgrCsrPressureResponse
185 stdcall WTMgrCsrExt(long long long ptr) WTMgrCsrExt
200 stdcall WTQueuePacketsEx(long ptr ptr) WTQueuePacketsEx
201 stdcall WTMgrCsrPressureBtnMarksEx(long long ptr ptr) WTMgrCsrPressureBtnMarksEx
202 stdcall WTMgrConfigReplaceExA(long long str str) WTMgrConfigReplaceExA
203 stdcall WTMgrPacketHookExA(long long str str) WTMgrPacketHookExA
204 stdcall WTMgrPacketUnhook(long) WTMgrPacketUnhook
205 stdcall WTMgrPacketHookNext(long long long long) WTMgrPacketHookNext
206 stdcall WTMgrDefContextEx(long long long) WTMgrDefContextEx
1020 stdcall WTInfoW(long long ptr) WTInfoW
1021 stdcall WTOpenW(long ptr long) WTOpenW
1061 stdcall WTGetW(long ptr) WTGetW
1062 stdcall WTSetW(long ptr) WTSetW
1202 stdcall WTMgrConfigReplaceExW(long long wstr str) WTMgrConfigReplaceExW
1203 stdcall WTMgrPacketHookExW(long long wstr str) WTMgrPacketHookExW
