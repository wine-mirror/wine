name	rpcrt4
type	win32
init	RPCRT4_LibMain

import	kernel32.dll
import	ntdll.dll

debug_channels (ole)

@ stdcall UuidCreate(ptr) UuidCreate
@ stdcall RpcStringFreeA(ptr) RpcStringFreeA
@ stdcall UuidToStringA(ptr ptr) UuidToStringA
