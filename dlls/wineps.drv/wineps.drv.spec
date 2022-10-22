@ cdecl wine_get_gdi_driver(long) PSDRV_get_gdi_driver

# Printer driver config exports
@ stdcall DrvDeviceCapabilities(ptr wstr long ptr ptr)
@ stdcall DrvDocumentPropertySheets(ptr long)

# Print processor exports
@ stdcall EnumPrintProcessorDatatypesW(ptr ptr long ptr long ptr ptr)
@ stdcall OpenPrintProcessor(ptr ptr)
@ stdcall PrintDocumentOnPrintProcessor(ptr ptr)
@ stdcall ControlPrintProcessor(ptr long)
@ stdcall ClosePrintProcessor(ptr)
