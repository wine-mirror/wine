name dinput
type win32

import user32.dll
import kernel32.dll

@ stdcall DirectInputCreateA(long long ptr ptr) DirectInputCreateA
@ stub DirectInputCreateW
@ stdcall DllCanUnloadNow() DINPUT_DllCanUnloadNow
@ stdcall DllGetClassObject(ptr ptr ptr) DINPUT_DllGetClassObject
@ stdcall DllRegisterServer() DINPUT_DllRegisterServer
@ stdcall DllUnregisterServer() DINPUT_DllUnregisterServer
