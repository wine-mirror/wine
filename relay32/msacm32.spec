name msacm32
type win32
init MSACM32_LibMain

 1 stub XRegThunkEntry
 2 stdcall acmDriverAddA(ptr long long long long) acmDriverAdd32A
 3 stdcall acmDriverAddW(ptr long long long long) acmDriverAdd32W
 4 stdcall acmDriverClose(long long) acmDriverClose32
 5 stdcall acmDriverDetailsA(long ptr long) acmDriverDetails32A
 6 stdcall acmDriverDetailsW(long ptr long) acmDriverDetails32W
 7 stdcall acmDriverEnum(ptr long long) acmDriverEnum32
 8 stdcall acmDriverID(long ptr long) acmDriverID32
 9 stdcall acmDriverMessage(long long long long) acmDriverMessage32
10 stdcall acmDriverOpen(ptr long long) acmDriverOpen32
11 stdcall acmDriverPriority(long long long) acmDriverPriority32
12 stdcall acmDriverRemove(long long) acmDriverRemove32
13 stdcall acmFilterChooseA(ptr) acmFilterChoose32A
14 stdcall acmFilterChooseW(ptr) acmFilterChoose32W
15 stdcall acmFilterDetailsA(long ptr long) acmFilterDetails32A
16 stdcall acmFilterDetailsW(long ptr long) acmFilterDetails32W
17 stdcall acmFilterEnumA(long ptr ptr long long) acmFilterEnum32A
18 stdcall acmFilterEnumW(long ptr ptr long long) acmFilterEnum32W
19 stdcall acmFilterTagDetailsA(long ptr long) acmFilterTagDetails32A
20 stdcall acmFilterTagDetailsW(long ptr long) acmFilterTagDetails32W
21 stdcall acmFilterTagEnumA(long ptr ptr long long) acmFilterTagEnum32A
22 stdcall acmFilterTagEnumW(long ptr ptr long long) acmFilterTagEnum32W
23 stdcall acmFormatChooseA(ptr) acmFormatChoose32A
24 stdcall acmFormatChooseW(ptr) acmFormatChoose32W
25 stdcall acmFormatDetailsA(long ptr long) acmFormatDetails32A
26 stdcall acmFormatDetailsW(long ptr long) acmFormatDetails32W
27 stdcall acmFormatEnumA(long ptr ptr long long) acmFormatEnum32A
28 stdcall acmFormatEnumW(long ptr ptr long long) acmFormatEnum32W
29 stdcall acmFormatSuggest(long ptr ptr long long) acmFormatSuggest32
30 stdcall acmFormatTagDetailsA(long ptr long) acmFormatTagDetails32A
31 stdcall acmFormatTagDetailsW(long ptr long) acmFormatTagDetails32W
32 stdcall acmFormatTagEnumA(long ptr ptr long long) acmFormatTagEnum32A
33 stdcall acmFormatTagEnumW(long ptr ptr long long) acmFormatTagEnum32W
34 stdcall acmGetVersion() acmGetVersion32
35 stub acmMessage32
36 stdcall acmMetrics(long long ptr) acmMetrics32
37 stdcall acmStreamClose(long long) acmStreamClose32
38 stdcall acmStreamConvert(long ptr long) acmStreamConvert32
39 stdcall acmStreamMessage(long long long long) acmStreamMessage32
40 stdcall acmStreamOpen(ptr long ptr ptr ptr long long long) acmStreamOpen32
41 stdcall acmStreamPrepareHeader(long ptr long) acmStreamPrepareHeader32
42 stdcall acmStreamReset(long long) acmStreamReset32
43 stdcall acmStreamSize(long long ptr long) acmStreamSize32
44 stdcall acmStreamUnprepareHeader(long ptr long) acmStreamUnprepareHeader32
