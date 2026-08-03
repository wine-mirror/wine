@ stub DllInitialize
@ stub DllUnload
@ stdcall HidP_FreeCollectionDescription(ptr)
@ stdcall HidP_GetButtonCaps(long ptr ptr ptr)
@ stdcall HidP_GetCaps(ptr ptr)
@ stdcall HidP_GetCollectionDescription(ptr long long ptr)
@ stdcall HidP_GetData(long ptr ptr ptr ptr long)
@ stub HidP_GetExtendedAttributes
@ stdcall HidP_GetLinkCollectionNodes(ptr ptr ptr)
@ stdcall HidP_GetScaledUsageValue(long long long long ptr ptr ptr long)
@ stdcall HidP_GetSpecificButtonCaps(long long long long ptr ptr ptr)
@ stdcall HidP_GetSpecificValueCaps(long long long long ptr ptr ptr)
@ stdcall HidP_GetUsages(long long long ptr ptr ptr ptr long)
@ stdcall HidP_GetUsagesEx(long long ptr ptr ptr ptr long)
@ stdcall HidP_GetUsageValue(long long long long ptr ptr ptr long)
@ stdcall HidP_GetUsageValueArray(long long long long ptr long ptr ptr long)
@ stdcall HidP_GetValueCaps(long ptr ptr ptr)
@ stdcall HidP_InitializeReportForID(long long ptr ptr long)
@ stdcall HidP_MaxDataListLength(long ptr)
@ stdcall HidP_MaxUsageListLength(long long ptr)
@ stub HidP_SetData
@ stub HidP_SetScaledUsageValue
@ stdcall HidP_SetUsages(long long long ptr ptr ptr ptr long)
@ stdcall HidP_SetUsageValue(long long long long long ptr ptr long)
@ stdcall HidP_SetUsageValueArray(long long long long ptr long ptr ptr long)
@ stub HidP_SysPowerCaps
@ stub HidP_SysPowerEvent
@ stub HidP_TranslateUsageAndPagesToI8042ScanCodes
@ stdcall HidP_TranslateUsagesToI8042ScanCodes(ptr long long ptr ptr ptr)
@ stub HidP_UnsetUsages
@ stub HidP_UsageAndPageListDifference
@ stub HidP_UsageListDifference
