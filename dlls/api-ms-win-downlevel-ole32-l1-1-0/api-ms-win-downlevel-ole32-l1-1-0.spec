@ stdcall CLSIDFromProgID(wstr ptr) ole32.CLSIDFromProgID
@ stdcall CLSIDFromString(wstr ptr) ole32.CLSIDFromString
@ stdcall CoCopyProxy(ptr ptr) ole32.CoCopyProxy
@ stdcall CoCreateFreeThreadedMarshaler(ptr ptr) ole32.CoCreateFreeThreadedMarshaler
@ stdcall CoCreateGuid(ptr) ole32.CoCreateGuid
@ stdcall CoCreateInstance(ptr ptr long ptr ptr) ole32.CoCreateInstance
@ stdcall CoCreateInstanceEx(ptr ptr long ptr long ptr) ole32.CoCreateInstanceEx
@ stdcall CoDisconnectObject(ptr long) ole32.CoDisconnectObject
@ stdcall CoFreeUnusedLibraries() ole32.CoFreeUnusedLibraries
@ stdcall CoFreeUnusedLibrariesEx(long long) ole32.CoFreeUnusedLibrariesEx
@ stdcall CoGetApartmentType(ptr ptr) ole32.CoGetApartmentType
@ stdcall CoGetClassObject(ptr long ptr ptr ptr) ole32.CoGetClassObject
@ stdcall CoGetCurrentLogicalThreadId(ptr) ole32.CoGetCurrentLogicalThreadId
@ stdcall CoGetInterfaceAndReleaseStream(ptr ptr ptr) ole32.CoGetInterfaceAndReleaseStream
@ stdcall CoGetMalloc(long ptr) ole32.CoGetMalloc
@ stdcall CoGetMarshalSizeMax(ptr ptr ptr long ptr long) ole32.CoGetMarshalSizeMax
@ stdcall CoGetObjectContext(ptr ptr) ole32.CoGetObjectContext
@ stub CoGetStdMarshalEx
@ stdcall CoGetTreatAsClass(ptr ptr) ole32.CoGetTreatAsClass
@ stdcall CoImpersonateClient() ole32.CoImpersonateClient
@ stdcall CoInitializeEx(ptr long) ole32.CoInitializeEx
@ stdcall CoInitializeSecurity(ptr long ptr ptr long long ptr long ptr) ole32.CoInitializeSecurity
@ stdcall CoMarshalInterThreadInterfaceInStream(ptr ptr ptr) ole32.CoMarshalInterThreadInterfaceInStream
@ stdcall CoMarshalInterface(ptr ptr ptr long ptr long) ole32.CoMarshalInterface
@ stdcall CoRegisterClassObject(ptr ptr long long ptr) ole32.CoRegisterClassObject
@ stdcall CoRegisterInitializeSpy(ptr ptr) ole32.CoRegisterInitializeSpy
@ stdcall CoRegisterMessageFilter(ptr ptr) ole32.CoRegisterMessageFilter
@ stdcall CoReleaseMarshalData(ptr) ole32.CoReleaseMarshalData
@ stdcall CoRevertToSelf() ole32.CoRevertToSelf
@ stdcall CoRevokeClassObject(long) ole32.CoRevokeClassObject
@ stdcall CoRevokeInitializeSpy(int64) ole32.CoRevokeInitializeSpy
@ stdcall CoSetProxyBlanket(ptr long long ptr long long ptr long) ole32.CoSetProxyBlanket
@ stdcall CoTaskMemAlloc(long) ole32.CoTaskMemAlloc
@ stdcall CoTaskMemFree(ptr) ole32.CoTaskMemFree
@ stdcall CoTaskMemRealloc(ptr long) ole32.CoTaskMemRealloc
@ stdcall CoUninitialize() ole32.CoUninitialize
@ stdcall CoUnmarshalInterface(ptr ptr ptr) ole32.CoUnmarshalInterface
@ stdcall CoWaitForMultipleHandles(long long long ptr ptr) ole32.CoWaitForMultipleHandles
@ stdcall CreateStreamOnHGlobal(ptr long ptr) ole32.CreateStreamOnHGlobal
@ stdcall FreePropVariantArray(long ptr) ole32.FreePropVariantArray
@ stdcall GetHGlobalFromStream(ptr ptr) ole32.GetHGlobalFromStream
@ stdcall IIDFromString(wstr ptr) ole32.IIDFromString
@ stdcall ProgIDFromCLSID(ptr ptr) ole32.ProgIDFromCLSID
@ stdcall PropVariantClear(ptr) ole32.PropVariantClear
@ stdcall PropVariantCopy(ptr ptr) ole32.PropVariantCopy
@ stdcall StringFromCLSID(ptr ptr) ole32.StringFromCLSID
@ stdcall StringFromGUID2(ptr ptr long) ole32.StringFromGUID2
@ stdcall StringFromIID(ptr ptr) ole32.StringFromIID
