@ stdcall PTQuerySchemaVersionSupport(wstr ptr)
@ stdcall PTOpenProvider(wstr long ptr)
@ stdcall PTOpenProviderEx(wstr long long ptr ptr)
@ stdcall PTCloseProvider(ptr)
@ stdcall BindPTProviderThunk(wstr long long ptr ptr) PTOpenProviderEx
@ stdcall PTGetPrintCapabilities(ptr ptr ptr ptr)
@ stdcall PTMergeAndValidatePrintTicket(ptr ptr ptr long ptr ptr)
@ stdcall PTConvertPrintTicketToDevMode(ptr ptr long long ptr ptr ptr)
@ stdcall PTConvertDevModeToPrintTicket(ptr long ptr long ptr)
@ stdcall PTReleaseMemory(ptr)
@ stdcall ConvertDevModeToPrintTicketThunk2(ptr ptr long long ptr ptr)
@ stub ConvertDevModeToPrintTicketThunk
@ stdcall ConvertPrintTicketToDevModeThunk2(ptr ptr long long long ptr ptr ptr)
@ stub ConvertPrintTicketToDevModeThunk
@ stdcall -private DllCanUnloadNow()
@ stub DllGetClassObject
@ stdcall -private DllMain(long long ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stub GetDeviceDefaultPrintTicketThunk
@ stub GetDeviceNamespacesThunk
@ stdcall GetPrintCapabilitiesThunk2(ptr ptr long ptr ptr ptr)
@ stub GetPrintCapabilitiesThunk
@ stub GetSchemaVersionThunk
@ stub MergeAndValidatePrintTicketThunk2
@ stub MergeAndValidatePrintTicketThunk
@ stdcall UnbindPTProviderThunk(ptr) PTCloseProvider
