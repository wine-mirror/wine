name	rpcrt4
type	win32
init	RPCRT4_LibMain

import	kernel32.dll
import	ntdll.dll

debug_channels (ole)

@ stdcall UuidCreate(ptr) UuidCreate
@ stdcall RpcStringFreeA(ptr) RpcStringFreeA
@ stdcall UuidToStringA(ptr ptr) UuidToStringA
@ stub NdrCStdStubBuffer_Release
@ stub NdrDllGetClassObject
@ stub RpcRaiseException
@ stub NdrClientCall
@ stub NdrOleAllocate
@ stub NdrOleFree
@ stub IUnknown_QueryInterface_Proxy
@ stub IUnknown_AddRef_Proxy
@ stub IUnknown_Release_Proxy
@ stub CStdStubBuffer_QueryInterface
@ stub CStdStubBuffer_AddRef
@ stub CStdStubBuffer_Connect
@ stub CStdStubBuffer_Disconnect
@ stub CStdStubBuffer_Invoke
@ stub CStdStubBuffer_IsIIDSupported
@ stub CStdStubBuffer_CountRefs
@ stub CStdStubBuffer_DebugServerQueryInterface
@ stub CStdStubBuffer_DebugServerRelease
@ stdcall NdrDllRegisterProxy(long ptr ptr) NdrDllRegisterProxy
