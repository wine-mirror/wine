name	rpcrt4
type	win32
init	RPCRT4_LibMain

import	kernel32.dll
import	ntdll.dll

debug_channels (ole)

@ stdcall NdrDllRegisterProxy(long ptr ptr) NdrDllRegisterProxy
@ stdcall RpcBindingFromStringBindingA(str  ptr) RpcBindingFromStringBindingA
@ stdcall RpcBindingFromStringBindingW(wstr ptr) RpcBindingFromStringBindingW
@ stdcall RpcServerListen(long long long) RpcServerListen
@ stdcall RpcServerUseProtseqEpA(str  long str  ptr) RpcServerUseProtseqEpA
@ stdcall RpcServerUseProtseqEpW(wstr long wstr ptr) RpcServerUseProtseqEpW
@ stdcall RpcServerUseProtseqEpExA(str  long str  ptr ptr) RpcServerUseProtseqEpExA
@ stdcall RpcServerUseProtseqEpExW(wstr long wstr ptr ptr) RpcServerUseProtseqEpExW
@ stdcall RpcServerRegisterIf(ptr ptr ptr)                     RpcServerRegisterIf
@ stdcall RpcServerRegisterIfEx(ptr ptr ptr long long ptr)     RpcServerRegisterIfEx
@ stdcall RpcServerRegisterIf2(ptr ptr ptr long long long ptr) RpcServerRegisterIf2
@ stdcall RpcServerRegisterAuthInfoA(str  long ptr ptr) RpcServerRegisterAuthInfoA
@ stdcall RpcServerRegisterAuthInfoW(wstr long ptr ptr) RpcServerRegisterAuthInfoW
@ stdcall RpcStringBindingComposeA(str  str  str  str  str  ptr) RpcStringBindingComposeA
@ stdcall RpcStringBindingComposeW(wstr wstr wstr wstr wstr ptr) RpcStringBindingComposeW
@ stdcall RpcStringFreeA(ptr) RpcStringFreeA
@ stdcall UuidCreate(ptr) UuidCreate
@ stdcall UuidToStringA(ptr ptr) UuidToStringA

@ stub CStdStubBuffer_QueryInterface
@ stub CStdStubBuffer_AddRef
@ stub CStdStubBuffer_Connect
@ stub CStdStubBuffer_Disconnect
@ stub CStdStubBuffer_Invoke
@ stub CStdStubBuffer_IsIIDSupported
@ stub CStdStubBuffer_CountRefs
@ stub CStdStubBuffer_DebugServerQueryInterface
@ stub CStdStubBuffer_DebugServerRelease

@ stub IUnknown_QueryInterface_Proxy
@ stub IUnknown_AddRef_Proxy
@ stub IUnknown_Release_Proxy

@ stub I_RpcGetBuffer

@ stub NdrClientCall
@ stub NdrClientCall2
@ stub NdrConformantVaryingArrayBufferSize
@ stub NdrConformantVaryingArrayMarshall
@ stub NdrConformantVaryingArrayUnmarshall
@ stub NdrConformantStringUnmarshall
@ stub NDRSContextUnmarshall
@ stub NdrConvert
@ stub NdrCStdStubBuffer_Release
@ stub NdrDllGetClassObject
@ stub NdrOleAllocate
@ stub NdrOleFree
@ stub NdrServerContextUnmarshall
@ stub NdrServerInitializeNew
@ stub NdrServerContextMarshall

@ stub RpcBindingFree
@ stub RpcImpersonateClient
@ stub RpcMgmtSetCancelTimeout
@ stub RpcMgmtStopServerListening
@ stub RpcRaiseException
@ stub RpcRevertToSelf
@ stub RpcServerUnregisterIf
@ stub RpcStringFreeW


@ stub UuidIsNil

