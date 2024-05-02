TESTDLL   = rpcrt4.dll
IMPORTS   = oleaut32 ole32 rpcrt4 secur32 advapi32
EXTRADEFS = -DPROXY_DELEGATION

EXTRAIDLFLAGS = --prefix-server=s_
server_EXTRAIDLFLAGS = -Os --prefix-client=mixed_
server_interp_EXTRAIDLFLAGS = -Oicf --prefix-client=interp_

SOURCES = \
	cstub.c \
	cstub.idl \
	explicit_handle.idl \
	generated.c \
	ndr_marshall.c \
	rpc.c \
	rpc_async.c \
	server.c \
	server.idl \
	server_interp.idl
