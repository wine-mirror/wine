name	rpcrt4
type	win32
init	RPCRT4_LibMain

import	ntdll.dll

@ stdcall UuidCreate(ptr) UuidCreate
@ stdcall RpcStringFreeA(ptr) RpcStringFreeA
@ stdcall UuidToStringA(ptr ptr) UuidToStringA
