@ stdcall PTQuerySchemaVersionSupport(wstr ptr)
@ stdcall PTOpenProvider(wstr long ptr)
@ stdcall PTOpenProviderEx(wstr long long ptr ptr)
@ stdcall PTCloseProvider(ptr)
@ stub BindPTProviderThunk
@ stub PTGetPrintCapabilities
@ stub PTMergeAndValidatePrintTicket
@ stdcall PTConvertPrintTicketToDevMode(ptr ptr long long ptr ptr ptr)
@ stub PTConvertDevModeToPrintTicket
@ stdcall PTReleaseMemory(ptr)
@ stub ConvertDevModeToPrintTicketThunk2
@ stub ConvertDevModeToPrintTicketThunk
@ stub ConvertPrintTicketToDevModeThunk2
@ stub ConvertPrintTicketToDevModeThunk
@ stub DllCanUnloadNow
@ stub DllGetClassObject
@ stdcall -private DllMain(long long ptr)
@ stub DllRegisterServer
@ stub DllUnregisterServer
@ stub GetDeviceDefaultPrintTicketThunk
@ stub GetDeviceNamespacesThunk
@ stub GetPrintCapabilitiesThunk2
@ stub GetPrintCapabilitiesThunk
@ stub GetSchemaVersionThunk
@ stub MergeAndValidatePrintTicketThunk2
@ stub MergeAndValidatePrintTicketThunk
@ stub UnbindPTProviderThunk
