@ stub ConvertAtJobsToTasks
@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stub GetNetScheduleAccountInformation
@ stdcall NetrJobAdd(wstr ptr ptr) NetrJobAdd_wrapper
@ stdcall NetrJobDel(wstr long long) NetrJobDel_wrapper
@ stdcall NetrJobEnum(wstr ptr long ptr ptr) NetrJobEnum_wrapper
@ stdcall NetrJobGetInfo(wstr long ptr) NetrJobGetInfo_wrapper
@ stub SAGetAccountInformation
@ stub SAGetNSAccountInformation
@ stub SASetAccountInformation
@ stub SASetNSAccountInformation
@ stub SetNetScheduleAccountInformation
@ stub _ConvertAtJobsToTasks@0
@ stdcall -private _DllCanUnloadNow@0() DllCanUnloadNow
@ stdcall -private _DllGetClassObject@12(ptr ptr ptr) DllGetClassObject
@ stub _GetNetScheduleAccountInformation@12
@ stdcall _NetrJobAdd@12(wstr ptr ptr) NetrJobAdd_wrapper
@ stdcall _NetrJobDel@12(wstr long long) NetrJobDel_wrapper
@ stdcall _NetrJobEnum@20(wstr ptr long ptr ptr) NetrJobEnum_wrapper
@ stdcall _NetrJobGetInfo@12(wstr long ptr) NetrJobGetInfo_wrapper
@ stub _SAGetAccountInformation@16
@ stub _SAGetNSAccountInformation@12
@ stub _SASetAccountInformation@20
@ stub _SASetNSAccountInformation@12
@ stub _SetNetScheduleAccountInformation@12
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
