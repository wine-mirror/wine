name	quartz
type	win32
init	QUARTZ_DllMain

import oleaut32.dll
import ole32.dll
import msvfw32.dll
import msacm32.dll
import winmm.dll
import user32.dll
import gdi32.dll
import advapi32.dll
import kernel32.dll
import ntdll.dll

debug_channels (quartz)

@ stdcall AMGetErrorTextA(long ptr long) AMGetErrorTextA
@ stdcall AMGetErrorTextW(long ptr long) AMGetErrorTextW
@ stdcall AmpFactorToDB(long) QUARTZ_AmpFactorToDB
@ stdcall DBToAmpFactor(long) QUARTZ_DBToAmpFactor
@ stdcall DllCanUnloadNow() QUARTZ_DllCanUnloadNow
@ stdcall DllGetClassObject(ptr ptr ptr) QUARTZ_DllGetClassObject
@ stdcall DllRegisterServer() QUARTZ_DllRegisterServer
@ stdcall DllUnregisterServer() QUARTZ_DllUnregisterServer

