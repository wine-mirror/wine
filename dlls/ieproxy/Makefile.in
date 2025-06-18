MODULE = ieproxy.dll
IMPORTS = uuid ole32 rpcrt4

SOURCES = \
	ieproxy_ieautomation.idl \
	ieproxy_perhist.idl

dlldata_EXTRADEFS = -DWINE_REGISTER_DLL -DPROXY_DELEGATION -DPROXY_CLSID=CLSID_PSFactoryBuffer
