name msacm32
type win32
init MSACM32_LibMain

import winmm.dll

 1 stub XRegThunkEntry
 2 stdcall acmDriverAddA(ptr long long long long) acmDriverAddA
 3 stdcall acmDriverAddW(ptr long long long long) acmDriverAddW
 4 stdcall acmDriverClose(long long) acmDriverClose
 5 stdcall acmDriverDetailsA(long ptr long) acmDriverDetailsA
 6 stdcall acmDriverDetailsW(long ptr long) acmDriverDetailsW
 7 stdcall acmDriverEnum(ptr long long) acmDriverEnum
 8 stdcall acmDriverID(long ptr long) acmDriverID
 9 stdcall acmDriverMessage(long long long long) acmDriverMessage
10 stdcall acmDriverOpen(ptr long long) acmDriverOpen
11 stdcall acmDriverPriority(long long long) acmDriverPriority
12 stdcall acmDriverRemove(long long) acmDriverRemove
13 stdcall acmFilterChooseA(ptr) acmFilterChooseA
14 stdcall acmFilterChooseW(ptr) acmFilterChooseW
15 stdcall acmFilterDetailsA(long ptr long) acmFilterDetailsA
16 stdcall acmFilterDetailsW(long ptr long) acmFilterDetailsW
17 stdcall acmFilterEnumA(long ptr ptr long long) acmFilterEnumA
18 stdcall acmFilterEnumW(long ptr ptr long long) acmFilterEnumW
19 stdcall acmFilterTagDetailsA(long ptr long) acmFilterTagDetailsA
20 stdcall acmFilterTagDetailsW(long ptr long) acmFilterTagDetailsW
21 stdcall acmFilterTagEnumA(long ptr ptr long long) acmFilterTagEnumA
22 stdcall acmFilterTagEnumW(long ptr ptr long long) acmFilterTagEnumW
23 stdcall acmFormatChooseA(ptr) acmFormatChooseA
24 stdcall acmFormatChooseW(ptr) acmFormatChooseW
25 stdcall acmFormatDetailsA(long ptr long) acmFormatDetailsA
26 stdcall acmFormatDetailsW(long ptr long) acmFormatDetailsW
27 stdcall acmFormatEnumA(long ptr ptr long long) acmFormatEnumA
28 stdcall acmFormatEnumW(long ptr ptr long long) acmFormatEnumW
29 stdcall acmFormatSuggest(long ptr ptr long long) acmFormatSuggest
30 stdcall acmFormatTagDetailsA(long ptr long) acmFormatTagDetailsA
31 stdcall acmFormatTagDetailsW(long ptr long) acmFormatTagDetailsW
32 stdcall acmFormatTagEnumA(long ptr ptr long long) acmFormatTagEnumA
33 stdcall acmFormatTagEnumW(long ptr ptr long long) acmFormatTagEnumW
34 stdcall acmGetVersion() acmGetVersion
35 stub acmMessage32
36 stdcall acmMetrics(long long ptr) acmMetrics
37 stdcall acmStreamClose(long long) acmStreamClose
38 stdcall acmStreamConvert(long ptr long) acmStreamConvert
39 stdcall acmStreamMessage(long long long long) acmStreamMessage
40 stdcall acmStreamOpen(ptr long ptr ptr ptr long long long) acmStreamOpen
41 stdcall acmStreamPrepareHeader(long ptr long) acmStreamPrepareHeader
42 stdcall acmStreamReset(long long) acmStreamReset
43 stdcall acmStreamSize(long long ptr long) acmStreamSize
44 stdcall acmStreamUnprepareHeader(long ptr long) acmStreamUnprepareHeader
