name	msisys
file	msisys.ocx
type	win32
init	MSISYS_DllMain

import ntdll.dll

debug_channels (msisys)

@ stdcall DllCanUnloadNow() MSISYS_DllCanUnloadNow
@ stdcall DllGetClassObject(ptr ptr ptr) MSISYS_DllGetClassObject
@ stdcall DllRegisterServer() MSISYS_DllRegisterServer
@ stdcall DllUnregisterServer() MSISYS_DllUnregisterServer
