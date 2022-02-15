@ stdcall CLSIDFromProgID(wstr ptr) combase.CLSIDFromProgID
@ stdcall CLSIDFromString(wstr ptr) combase.CLSIDFromString
@ stdcall CoAddRefServerProcess() combase.CoAddRefServerProcess
@ stub CoAllowUnmarshalerCLSID
@ stub CoCancelCall
@ stdcall CoCopyProxy(ptr ptr) combase.CoCopyProxy
@ stdcall CoCreateFreeThreadedMarshaler(ptr ptr) combase.CoCreateFreeThreadedMarshaler
@ stdcall CoCreateGuid(ptr) combase.CoCreateGuid
@ stdcall CoCreateInstance(ptr ptr long ptr ptr) combase.CoCreateInstance
@ stdcall CoCreateInstanceEx(ptr ptr long ptr long ptr) combase.CoCreateInstanceEx
@ stdcall CoCreateInstanceFromApp(ptr ptr long ptr long ptr) combase.CoCreateInstanceFromApp
@ stdcall CoDecodeProxy(long int64 ptr) combase.CoDecodeProxy
@ stdcall CoDecrementMTAUsage(ptr) combase.CoDecrementMTAUsage
@ stdcall CoDisableCallCancellation(ptr) combase.CoDisableCallCancellation
@ stub CoDisconnectContext
@ stdcall CoDisconnectObject(ptr long) combase.CoDisconnectObject
@ stdcall CoEnableCallCancellation(ptr) combase.CoEnableCallCancellation
@ stdcall CoFreeUnusedLibraries() combase.CoFreeUnusedLibraries
@ stdcall CoFreeUnusedLibrariesEx(long long) combase.CoFreeUnusedLibrariesEx
@ stdcall CoGetApartmentType(ptr ptr) combase.CoGetApartmentType
@ stdcall CoGetCallContext(ptr ptr) combase.CoGetCallContext
@ stdcall CoGetCallerTID(ptr) combase.CoGetCallerTID
@ stub CoGetCancelObject
@ stdcall CoGetClassObject(ptr long ptr ptr ptr) combase.CoGetClassObject
@ stdcall CoGetContextToken(ptr) combase.CoGetContextToken
@ stdcall CoGetCurrentLogicalThreadId(ptr) combase.CoGetCurrentLogicalThreadId
@ stdcall CoGetCurrentProcess() combase.CoGetCurrentProcess
@ stdcall CoGetDefaultContext(long ptr ptr) combase.CoGetDefaultContext
@ stdcall CoGetInterfaceAndReleaseStream(ptr ptr ptr) combase.CoGetInterfaceAndReleaseStream
@ stdcall CoGetMalloc(long ptr) combase.CoGetMalloc
@ stdcall CoGetMarshalSizeMax(ptr ptr ptr long ptr long) combase.CoGetMarshalSizeMax
@ stdcall CoGetObjectContext(ptr ptr) combase.CoGetObjectContext
@ stdcall CoGetPSClsid(ptr ptr) combase.CoGetPSClsid
@ stdcall CoGetStandardMarshal(ptr ptr long ptr long ptr) combase.CoGetStandardMarshal
@ stub CoGetStdMarshalEx
@ stdcall CoGetTreatAsClass(ptr ptr) combase.CoGetTreatAsClass
@ stdcall CoImpersonateClient() combase.CoImpersonateClient
@ stdcall CoIncrementMTAUsage(ptr) combase.CoIncrementMTAUsage
@ stdcall CoInitializeEx(ptr long) combase.CoInitializeEx
@ stdcall CoInitializeSecurity(ptr long ptr ptr long long ptr long ptr) combase.CoInitializeSecurity
@ stub CoInvalidateRemoteMachineBindings
@ stdcall CoIsHandlerConnected(ptr) combase.CoIsHandlerConnected
@ stdcall CoLockObjectExternal(ptr long long) combase.CoLockObjectExternal
@ stdcall CoMarshalHresult(ptr long) combase.CoMarshalHresult
@ stdcall CoMarshalInterThreadInterfaceInStream(ptr ptr ptr) combase.CoMarshalInterThreadInterfaceInStream
@ stdcall CoMarshalInterface(ptr ptr ptr long ptr long) combase.CoMarshalInterface
@ stub CoQueryAuthenticationServices
@ stdcall CoQueryClientBlanket(ptr ptr ptr ptr ptr ptr ptr) combase.CoQueryClientBlanket
@ stdcall CoQueryProxyBlanket(ptr ptr ptr ptr ptr ptr ptr ptr) combase.CoQueryProxyBlanket
@ stdcall CoRegisterClassObject(ptr ptr long long ptr) combase.CoRegisterClassObject
@ stdcall CoRegisterPSClsid(ptr ptr) combase.CoRegisterPSClsid
@ stdcall CoRegisterSurrogate(ptr) combase.CoRegisterSurrogate
@ stdcall CoReleaseMarshalData(ptr) combase.CoReleaseMarshalData
@ stdcall CoReleaseServerProcess() combase.CoReleaseServerProcess
@ stdcall CoResumeClassObjects() combase.CoResumeClassObjects
@ stdcall CoRevertToSelf() combase.CoRevertToSelf
@ stdcall CoRevokeClassObject(long) combase.CoRevokeClassObject
@ stub CoSetCancelObject
@ stdcall CoSetProxyBlanket(ptr long long ptr long long ptr long) combase.CoSetProxyBlanket
@ stdcall CoSuspendClassObjects() combase.CoSuspendClassObjects
@ stdcall CoSwitchCallContext(ptr ptr) combase.CoSwitchCallContext
@ stdcall CoTaskMemAlloc(long) combase.CoTaskMemAlloc
@ stdcall CoTaskMemFree(ptr) combase.CoTaskMemFree
@ stdcall CoTaskMemRealloc(ptr long) combase.CoTaskMemRealloc
@ stub CoTestCancel
@ stdcall CoUninitialize() combase.CoUninitialize
@ stdcall CoUnmarshalHresult(ptr ptr) combase.CoUnmarshalHresult
@ stdcall CoUnmarshalInterface(ptr ptr ptr) combase.CoUnmarshalInterface
@ stdcall CoWaitForMultipleHandles(long long long ptr ptr) combase.CoWaitForMultipleHandles
@ stub CoWaitForMultipleObjects
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
