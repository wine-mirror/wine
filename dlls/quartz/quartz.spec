name	quartz
type	win32

#import ole2.dll
import ntdll.dll

debug_channels (quartz)

@ stub AMGetErrorTextA
@ stub AMGetErrorTextW
@ stub AmpFactorToDB
@ stub DBToAmpFactor
@ stdcall DllCanUnloadNow() QUARTZ_DllCanUnloadNow
@ stdcall DllGetClassObject(ptr ptr ptr) QUARTZ_DllGetClassObject
@ stdcall DllRegisterServer() QUARTZ_DllRegisterServer
@ stdcall DllUnregisterServer() QUARTZ_DllUnregisterServer

