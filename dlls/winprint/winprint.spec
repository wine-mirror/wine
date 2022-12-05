@ stdcall -private DllRegisterServer()
@ stdcall ClosePrintProcessor(ptr)
@ stdcall ControlPrintProcessor(ptr long)
@ stdcall EnumPrintProcessorDatatypesW(ptr ptr long ptr long ptr ptr)
@ stub GetPrintProcessorCapabilities
@ stdcall OpenPrintProcessor(ptr ptr)
@ stdcall PrintDocumentOnPrintProcessor(ptr ptr)
