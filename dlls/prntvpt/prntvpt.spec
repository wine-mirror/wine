@ stdcall PTQuerySchemaVersionSupport(wstr ptr)
@ stdcall PTOpenProvider(wstr long ptr)
@ stdcall PTOpenProviderEx(wstr long long ptr ptr)
@ stdcall PTCloseProvider(ptr)
@ stub BindPTProviderThunk
@ stdcall PTGetPrintCapabilities(ptr ptr ptr ptr)
@ stdcall PTMergeAndValidatePrintTicket(ptr ptr ptr long ptr ptr)
@ stdcall PTConvertPrintTicketToDevMode(ptr ptr long long ptr ptr ptr)
@ stdcall PTConvertDevModeToPrintTicket(ptr long ptr long ptr)
@ stdcall PTReleaseMemory(ptr)
@ stub ConvertDevModeToPrintTicketThunk2
@ stub ConvertDevModeToPrintTicketThunk
@ stub ConvertPrintTicketToDevModeThunk2
@ stub ConvertPrintTicketToDevModeThunk
@ stdcall -private DllCanUnloadNow()
@ stub DllGetClassObject
@ stdcall -private DllMain(long long ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stub GetDeviceDefaultPrintTicketThunk
@ stub GetDeviceNamespacesThunk
@ stub GetPrintCapabilitiesThunk2
@ stub GetPrintCapabilitiesThunk
@ stub GetSchemaVersionThunk
@ stub MergeAndValidatePrintTicketThunk2
@ stub MergeAndValidatePrintTicketThunk
@ stub UnbindPTProviderThunk
