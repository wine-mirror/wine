@ stdcall CLSIDFromProgID(wstr ptr) ole32.CLSIDFromProgID
@ stdcall CLSIDFromString(wstr ptr) ole32.CLSIDFromString
@ stdcall CoAddRefServerProcess() ole32.CoAddRefServerProcess
@ stub CoAllowUnmarshalerCLSID
@ stub CoCancelCall
@ stdcall CoCopyProxy(ptr ptr) ole32.CoCopyProxy
@ stdcall CoCreateFreeThreadedMarshaler(ptr ptr) ole32.CoCreateFreeThreadedMarshaler
@ stdcall CoCreateGuid(ptr) ole32.CoCreateGuid
@ stdcall CoCreateInstance(ptr ptr long ptr ptr) ole32.CoCreateInstance
@ stdcall CoCreateInstanceEx(ptr ptr long ptr long ptr) ole32.CoCreateInstanceEx
@ stdcall CoCreateInstanceFromApp(ptr ptr long ptr long ptr) ole32.CoCreateInstanceFromApp
@ stub CoDecodeProxy
@ stdcall CoDecrementMTAUsage(ptr) ole32.CoDecrementMTAUsage
@ stdcall CoDisableCallCancellation(ptr) ole32.CoDisableCallCancellation
@ stub CoDisconnectContext
@ stdcall CoDisconnectObject(ptr long) ole32.CoDisconnectObject
@ stdcall CoEnableCallCancellation(ptr) ole32.CoEnableCallCancellation
@ stdcall CoFreeUnusedLibraries() ole32.CoFreeUnusedLibraries
@ stdcall CoFreeUnusedLibrariesEx(long long) ole32.CoFreeUnusedLibrariesEx
@ stdcall CoGetApartmentType(ptr ptr) ole32.CoGetApartmentType
@ stdcall CoGetCallContext(ptr ptr) ole32.CoGetCallContext
@ stdcall CoGetCallerTID(ptr) ole32.CoGetCallerTID
@ stub CoGetCancelObject
@ stdcall CoGetClassObject(ptr long ptr ptr ptr) ole32.CoGetClassObject
@ stdcall CoGetContextToken(ptr) ole32.CoGetContextToken
@ stdcall CoGetCurrentLogicalThreadId(ptr) ole32.CoGetCurrentLogicalThreadId
@ stdcall CoGetCurrentProcess() ole32.CoGetCurrentProcess
@ stdcall CoGetDefaultContext(long ptr ptr) ole32.CoGetDefaultContext
@ stdcall CoGetInterfaceAndReleaseStream(ptr ptr ptr) ole32.CoGetInterfaceAndReleaseStream
@ stdcall CoGetMalloc(long ptr) ole32.CoGetMalloc
@ stdcall CoGetMarshalSizeMax(ptr ptr ptr long ptr long) ole32.CoGetMarshalSizeMax
@ stdcall CoGetObjectContext(ptr ptr) ole32.CoGetObjectContext
@ stdcall CoGetPSClsid(ptr ptr) ole32.CoGetPSClsid
@ stdcall CoGetStandardMarshal(ptr ptr long ptr long ptr) ole32.CoGetStandardMarshal
@ stub CoGetStdMarshalEx
@ stdcall CoGetTreatAsClass(ptr ptr) ole32.CoGetTreatAsClass
@ stdcall CoImpersonateClient() ole32.CoImpersonateClient
@ stdcall CoIncrementMTAUsage(ptr) ole32.CoIncrementMTAUsage
@ stdcall CoInitializeEx(ptr long) ole32.CoInitializeEx
@ stdcall CoInitializeSecurity(ptr long ptr ptr long long ptr long ptr) ole32.CoInitializeSecurity
@ stub CoInvalidateRemoteMachineBindings
@ stdcall CoIsHandlerConnected(ptr) ole32.CoIsHandlerConnected
@ stdcall CoLockObjectExternal(ptr long long) ole32.CoLockObjectExternal
@ stdcall CoMarshalHresult(ptr long) ole32.CoMarshalHresult
@ stdcall CoMarshalInterThreadInterfaceInStream(ptr ptr ptr) ole32.CoMarshalInterThreadInterfaceInStream
@ stdcall CoMarshalInterface(ptr ptr ptr long ptr long) ole32.CoMarshalInterface
@ stub CoQueryAuthenticationServices
@ stdcall CoQueryClientBlanket(ptr ptr ptr ptr ptr ptr ptr) ole32.CoQueryClientBlanket
@ stdcall CoQueryProxyBlanket(ptr ptr ptr ptr ptr ptr ptr ptr) ole32.CoQueryProxyBlanket
@ stdcall CoRegisterClassObject(ptr ptr long long ptr) ole32.CoRegisterClassObject
@ stdcall CoRegisterPSClsid(ptr ptr) ole32.CoRegisterPSClsid
@ stdcall CoRegisterSurrogate(ptr) ole32.CoRegisterSurrogate
@ stdcall CoReleaseMarshalData(ptr) ole32.CoReleaseMarshalData
@ stdcall CoReleaseServerProcess() ole32.CoReleaseServerProcess
@ stdcall CoResumeClassObjects() ole32.CoResumeClassObjects
@ stdcall CoRevertToSelf() ole32.CoRevertToSelf
@ stdcall CoRevokeClassObject(long) ole32.CoRevokeClassObject
@ stub CoSetCancelObject
@ stdcall CoSetProxyBlanket(ptr long long ptr long long ptr long) ole32.CoSetProxyBlanket
@ stdcall CoSuspendClassObjects() ole32.CoSuspendClassObjects
@ stdcall CoSwitchCallContext(ptr ptr) ole32.CoSwitchCallContext
@ stdcall CoTaskMemAlloc(long) ole32.CoTaskMemAlloc
@ stdcall CoTaskMemFree(ptr) ole32.CoTaskMemFree
@ stdcall CoTaskMemRealloc(ptr long) ole32.CoTaskMemRealloc
@ stub CoTestCancel
@ stdcall CoUninitialize() ole32.CoUninitialize
@ stdcall CoUnmarshalHresult(ptr ptr) ole32.CoUnmarshalHresult
@ stdcall CoUnmarshalInterface(ptr ptr ptr) ole32.CoUnmarshalInterface
@ stdcall CoWaitForMultipleHandles(long long long ptr ptr) ole32.CoWaitForMultipleHandles
@ stub CoWaitForMultipleObjects
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
