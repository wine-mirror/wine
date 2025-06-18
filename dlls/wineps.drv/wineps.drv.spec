@ cdecl wine_driver_open_dc(wstr ptr wstr) PSDRV_open_printer_dc
@ stdcall -private DllRegisterServer()

# Printer driver config exports
@ stdcall DrvDeviceCapabilities(ptr wstr long ptr ptr)
@ stdcall DrvDocumentPropertySheets(ptr long)

# Print processor exports
@ stdcall EnumPrintProcessorDatatypesW(ptr ptr long ptr long ptr ptr)
@ stdcall OpenPrintProcessor(ptr ptr)
@ stdcall PrintDocumentOnPrintProcessor(ptr ptr)
@ stdcall ControlPrintProcessor(ptr long)
@ stdcall ClosePrintProcessor(ptr)
