@ stdcall acmDriverAddA(ptr long long long long) acmDriverAddA
@ stdcall acmDriverAddW(ptr long long long long) acmDriverAddW
@ stdcall acmDriverClose(long long) acmDriverClose
@ stdcall acmDriverDetailsA(long ptr long) acmDriverDetailsA
@ stdcall acmDriverDetailsW(long ptr long) acmDriverDetailsW
@ stdcall acmDriverEnum(ptr long long) acmDriverEnum
@ stdcall acmDriverID(long ptr long) acmDriverID
@ stdcall acmDriverMessage(long long long long) acmDriverMessage
@ stdcall acmDriverOpen(ptr long long) acmDriverOpen
@ stdcall acmDriverPriority(long long long) acmDriverPriority
@ stdcall acmDriverRemove(long long) acmDriverRemove
@ stdcall acmFilterChooseA(ptr) acmFilterChooseA
@ stdcall acmFilterChooseW(ptr) acmFilterChooseW
@ stdcall acmFilterDetailsA(long ptr long) acmFilterDetailsA
@ stdcall acmFilterDetailsW(long ptr long) acmFilterDetailsW
@ stdcall acmFilterEnumA(long ptr ptr long long) acmFilterEnumA
@ stdcall acmFilterEnumW(long ptr ptr long long) acmFilterEnumW
@ stdcall acmFilterTagDetailsA(long ptr long) acmFilterTagDetailsA
@ stdcall acmFilterTagDetailsW(long ptr long) acmFilterTagDetailsW
@ stdcall acmFilterTagEnumA(long ptr ptr long long) acmFilterTagEnumA
@ stdcall acmFilterTagEnumW(long ptr ptr long long) acmFilterTagEnumW
@ stdcall acmFormatChooseA(ptr) acmFormatChooseA
@ stdcall acmFormatChooseW(ptr) acmFormatChooseW
@ stdcall acmFormatDetailsA(long ptr long) acmFormatDetailsA
@ stdcall acmFormatDetailsW(long ptr long) acmFormatDetailsW
@ stdcall acmFormatEnumA(long ptr ptr long long) acmFormatEnumA
@ stdcall acmFormatEnumW(long ptr ptr long long) acmFormatEnumW
@ stdcall acmFormatSuggest(long ptr ptr long long) acmFormatSuggest
@ stdcall acmFormatTagDetailsA(long ptr long) acmFormatTagDetailsA
@ stdcall acmFormatTagDetailsW(long ptr long) acmFormatTagDetailsW
@ stdcall acmFormatTagEnumA(long ptr ptr long long) acmFormatTagEnumA
@ stdcall acmFormatTagEnumW(long ptr ptr long long) acmFormatTagEnumW
@ stdcall acmGetVersion() acmGetVersion
@ stub acmMessage32
@ stdcall acmMetrics(long long ptr) acmMetrics
@ stdcall acmStreamClose(long long) acmStreamClose
@ stdcall acmStreamConvert(long ptr long) acmStreamConvert
@ stdcall acmStreamMessage(long long long long) acmStreamMessage
@ stdcall acmStreamOpen(ptr long ptr ptr ptr long long long) acmStreamOpen
@ stdcall acmStreamPrepareHeader(long ptr long) acmStreamPrepareHeader
@ stdcall acmStreamReset(long long) acmStreamReset
@ stdcall acmStreamSize(long long ptr long) acmStreamSize
@ stdcall acmStreamUnprepareHeader(long ptr long) acmStreamUnprepareHeader

# this is wine only
@ stdcall DriverProc(long long long long long) PCM_DriverProc
