name	msisys
file	msisys.ocx
init	MSISYS_DllMain

@ stdcall DllCanUnloadNow() MSISYS_DllCanUnloadNow
@ stdcall DllGetClassObject(ptr ptr ptr) MSISYS_DllGetClassObject
@ stdcall DllRegisterServer() MSISYS_DllRegisterServer
@ stdcall DllUnregisterServer() MSISYS_DllUnregisterServer
