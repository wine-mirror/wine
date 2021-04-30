3 stdcall ADsGetObject(wstr ptr ptr)
4 stdcall ADsBuildEnumerator(ptr ptr)
5 stdcall ADsFreeEnumerator(ptr)
6 stdcall ADsEnumerateNext(ptr long ptr ptr)
7 stdcall ADsBuildVarArrayStr(ptr long ptr)
8 stdcall ADsBuildVarArrayInt(ptr long ptr)
9 stdcall ADsOpenObject(wstr wstr wstr long ptr ptr)
12 stdcall ADsSetLastError(long ptr ptr)
13 stdcall ADsGetLastError(ptr ptr long ptr long)
14 stdcall AllocADsMem(long)
15 stdcall FreeADsMem(ptr)
16 stdcall ReallocADsMem(ptr long long)
17 stdcall AllocADsStr(ptr)
18 stdcall FreeADsStr(ptr)
19 stdcall ReallocADsStr(ptr ptr)
20 stdcall ADsEncodeBinaryData(ptr long ptr)
21 stub PropVariantToAdsType
22 stub AdsTypeToPropVariant
23 stub AdsFreeAdsValues
24 stub ADsDecodeBinaryData
25 stub AdsTypeToPropVariant2
26 stub PropVariantToAdsType2
27 stub ConvertSecDescriptorToVariant
28 stub ConvertSecurityDescriptorToSecDes
@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
