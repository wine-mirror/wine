@ stdcall CLSIDFromProgID(wstr ptr) combase.CLSIDFromProgID
@ stdcall CLSIDFromString(wstr ptr) combase.CLSIDFromString
@ stdcall CoCopyProxy(ptr ptr) combase.CoCopyProxy
@ stdcall CoCreateFreeThreadedMarshaler(ptr ptr) combase.CoCreateFreeThreadedMarshaler
@ stdcall CoCreateGuid(ptr) combase.CoCreateGuid
@ stdcall CoCreateInstance(ptr ptr long ptr ptr) combase.CoCreateInstance
@ stdcall CoCreateInstanceEx(ptr ptr long ptr long ptr) combase.CoCreateInstanceEx
@ stdcall CoDisconnectObject(ptr long) combase.CoDisconnectObject
@ stdcall CoFreeUnusedLibraries() combase.CoFreeUnusedLibraries
@ stdcall CoFreeUnusedLibrariesEx(long long) combase.CoFreeUnusedLibrariesEx
@ stdcall CoGetApartmentType(ptr ptr) combase.CoGetApartmentType
@ stdcall CoGetClassObject(ptr long ptr ptr ptr) combase.CoGetClassObject
@ stdcall CoGetCurrentLogicalThreadId(ptr) combase.CoGetCurrentLogicalThreadId
@ stdcall CoGetInterfaceAndReleaseStream(ptr ptr ptr) combase.CoGetInterfaceAndReleaseStream
@ stdcall CoGetMalloc(long ptr) combase.CoGetMalloc
@ stdcall CoGetMarshalSizeMax(ptr ptr ptr long ptr long) combase.CoGetMarshalSizeMax
@ stdcall CoGetObjectContext(ptr ptr) combase.CoGetObjectContext
@ stub CoGetStdMarshalEx
@ stdcall CoGetTreatAsClass(ptr ptr) combase.CoGetTreatAsClass
@ stdcall CoImpersonateClient() combase.CoImpersonateClient
@ stdcall CoInitializeEx(ptr long) combase.CoInitializeEx
@ stdcall CoInitializeSecurity(ptr long ptr ptr long long ptr long ptr) combase.CoInitializeSecurity
@ stdcall CoMarshalInterThreadInterfaceInStream(ptr ptr ptr) combase.CoMarshalInterThreadInterfaceInStream
@ stdcall CoMarshalInterface(ptr ptr ptr long ptr long) combase.CoMarshalInterface
@ stdcall CoRegisterClassObject(ptr ptr long long ptr) combase.CoRegisterClassObject
@ stdcall CoRegisterInitializeSpy(ptr ptr) combase.CoRegisterInitializeSpy
@ stdcall CoRegisterMessageFilter(ptr ptr) combase.CoRegisterMessageFilter
@ stdcall CoReleaseMarshalData(ptr) combase.CoReleaseMarshalData
@ stdcall CoRevertToSelf() combase.CoRevertToSelf
@ stdcall CoRevokeClassObject(long) combase.CoRevokeClassObject
@ stdcall CoRevokeInitializeSpy(int64) combase.CoRevokeInitializeSpy
@ stdcall CoSetProxyBlanket(ptr long long ptr long long ptr long) combase.CoSetProxyBlanket
@ stdcall CoTaskMemAlloc(long) combase.CoTaskMemAlloc
@ stdcall CoTaskMemFree(ptr) combase.CoTaskMemFree
@ stdcall CoTaskMemRealloc(ptr long) combase.CoTaskMemRealloc
@ stdcall CoUninitialize() combase.CoUninitialize
@ stdcall CoUnmarshalInterface(ptr ptr ptr) combase.CoUnmarshalInterface
@ stdcall CoWaitForMultipleHandles(long long long ptr ptr) combase.CoWaitForMultipleHandles
@ stdcall CreateStreamOnHGlobal(ptr long ptr) combase.CreateStreamOnHGlobal
@ stdcall FreePropVariantArray(long ptr) combase.FreePropVariantArray
@ stdcall GetHGlobalFromStream(ptr ptr) combase.GetHGlobalFromStream
@ stdcall IIDFromString(wstr ptr) combase.IIDFromString
@ stdcall ProgIDFromCLSID(ptr ptr) combase.ProgIDFromCLSID
@ stdcall PropVariantClear(ptr) combase.PropVariantClear
@ stdcall PropVariantCopy(ptr ptr) combase.PropVariantCopy
@ stdcall StringFromCLSID(ptr ptr) combase.StringFromCLSID
@ stdcall StringFromGUID2(ptr ptr long) combase.StringFromGUID2
@ stdcall StringFromIID(ptr ptr) combase.StringFromIID
