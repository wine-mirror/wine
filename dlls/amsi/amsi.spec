@ stdcall AmsiCloseSession(ptr ptr)
@ stdcall AmsiInitialize(wstr ptr)
@ stdcall AmsiOpenSession(ptr ptr)
@ stdcall AmsiScanBuffer(ptr ptr long wstr ptr ptr)
@ stdcall AmsiScanString(ptr wstr wstr ptr ptr)
@ stub AmsiUacInitialize
@ stub AmsiUacScan
@ stub AmsiUacUninitialize
@ stdcall AmsiUninitialize(ptr)
@ stdcall -private DllCanUnloadNow()
@ stub DllGetClassObject
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
