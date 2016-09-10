1 stdcall -private DllMain(long long ptr)
2 stdcall XInputGetState(long ptr) xinput1_3.XInputGetState
3 stdcall XInputSetState(long ptr) xinput1_3.XInputSetState
4 stdcall XInputGetCapabilities(long long ptr) xinput1_3.XInputGetCapabilities
5 stdcall XInputEnable(long) xinput1_3.XInputEnable
7 stdcall XInputGetBatteryInformation(long long ptr) xinput1_3.XInputGetBatteryInformation
8 stdcall XInputGetKeystroke(long long ptr) xinput1_3.XInputGetKeystroke
10 stub XInputGetAudioDeviceIds(long ptr ptr ptr ptr)
100 stdcall XInputGetStateEx(long ptr) xinput1_3.XInputGetStateEx
