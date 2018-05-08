@ stub ConvertAtJobsToTasks
@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stub GetNetScheduleAccountInformation
@ stdcall NetrJobAdd(wstr ptr ptr)
@ stdcall NetrJobDel(wstr long long)
@ stdcall NetrJobEnum(wstr ptr long ptr ptr)
@ stdcall NetrJobGetInfo(wstr long ptr)
@ stub SAGetAccountInformation
@ stub SAGetNSAccountInformation
@ stub SASetAccountInformation
@ stub SASetNSAccountInformation
@ stub SetNetScheduleAccountInformation
@ stub _ConvertAtJobsToTasks@0
@ stub _DllCanUnloadNow@0
@ stub _DllGetClassObject@12
@ stub _GetNetScheduleAccountInformation@12
@ stdcall _NetrJobAdd@12(wstr ptr ptr) NetrJobAdd
@ stdcall _NetrJobDel@12(wstr long long) NetrJobDel
@ stdcall _NetrJobEnum@20(wstr ptr long ptr ptr) NetrJobEnum
@ stdcall _NetrJobGetInfo@12(wstr long ptr) NetrJobGetInfo
@ stub _SAGetAccountInformation@16
@ stub _SAGetNSAccountInformation@12
@ stub _SASetAccountInformation@20
@ stub _SASetNSAccountInformation@12
@ stub _SetNetScheduleAccountInformation@12
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
