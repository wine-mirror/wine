100 stub @
@ stub ADVANCEDSETUPDIALOG
@ stub AbortPrinter
@ stdcall AddFormA(long long ptr)
@ stdcall AddFormW(long long ptr)
@ stdcall AddJobA(long long ptr long ptr)
@ stdcall AddJobW(long long ptr long ptr)
@ stdcall AddMonitorA(str long ptr)
@ stub AddMonitorW
@ stdcall AddPortA(str ptr str)
@ stub AddPortExA
@ stub AddPortExW
@ stub AddPortW
@ stub AddPrintProcessorA
@ stub AddPrintProcessorW
@ stub AddPrintProvidorA
@ stub AddPrintProvidorW
@ stdcall AddPrinterA(str long ptr)
@ stub AddPrinterConnectionA
@ stub AddPrinterConnectionW
@ stdcall AddPrinterDriverA(str long ptr)
@ stdcall AddPrinterDriverW(wstr long ptr)
@ stdcall AddPrinterDriverExA(str long ptr long)
@ stdcall AddPrinterDriverExW(wstr long ptr long)
@ stdcall AddPrinterW(wstr long ptr)
@ stub AdvancedDocumentPropertiesA
@ stub AdvancedDocumentPropertiesW
@ stub AdvancedSetupDialog
@ stdcall ClosePrinter(long)
@ stub ConfigurePortA
@ stub ConfigurePortW
@ stub ConnectToPrinterDlg
@ stub CreatePrinterIC
@ stub DEVICECAPABILITIES
@ stub DEVICEMODE
@ stdcall DeleteFormA(long str)
@ stdcall DeleteFormW(long wstr)
@ stdcall DeleteMonitorA(str str str)
@ stub DeleteMonitorW
@ stdcall DeletePortA(str long str)
@ stub DeletePortW
@ stub DeletePrintProcessorA
@ stub DeletePrintProcessorW
@ stub DeletePrintProvidorA
@ stub DeletePrintProvidorW
@ stdcall DeletePrinter(long)
@ stub DeletePrinterConnectionA
@ stub DeletePrinterConnectionW
@ stdcall DeletePrinterDataExA(long str str)
@ stdcall DeletePrinterDataExW(long wstr wstr)
@ stdcall DeletePrinterDriverA(str str str)
@ stub DeletePrinterDriverW
@ stdcall DeletePrinterDriverExA(str str str long long)
@ stdcall DeletePrinterDriverExW(wstr wstr wstr long long)
@ stub DeletePrinterIC
@ stub DevQueryPrint
@ stdcall DeviceCapabilities(str str long ptr ptr) DeviceCapabilitiesA
@ stdcall DeviceCapabilitiesA(str str long ptr ptr)
@ stdcall DeviceCapabilitiesW(wstr wstr long wstr ptr)
@ stub DeviceMode
@ stub DocumentEvent
@ stdcall DocumentPropertiesA(long long ptr ptr ptr long)
@ stdcall DocumentPropertiesW(long long ptr ptr ptr long)
@ stub EXTDEVICEMODE
@ stdcall EndDocPrinter(long)
@ stdcall EndPagePrinter(long)
@ stub EnumFormsA
@ stub EnumFormsW
@ stdcall EnumJobsA(long long long long ptr long ptr ptr)
@ stdcall EnumJobsW(long long long long ptr long ptr ptr)
@ stdcall EnumMonitorsA(str long ptr long long long)
@ stdcall EnumMonitorsW(wstr long ptr long long long)
@ stdcall EnumPortsA(ptr long ptr ptr ptr ptr)
@ stub EnumPortsW
@ stub EnumPrintProcessorDatatypesA
@ stub EnumPrintProcessorDatatypesW
@ stub EnumPrintProcessorsA
@ stub EnumPrintProcessorsW
@ stub EnumPrinterDataA
@ stdcall EnumPrinterDataExA(long str ptr long ptr ptr)
@ stdcall EnumPrinterDataExW(long wstr ptr long ptr ptr)
@ stub EnumPrinterDataW
@ stdcall EnumPrinterDriversA(str str long ptr long ptr ptr)
@ stdcall EnumPrinterDriversW(wstr wstr long ptr long ptr ptr)
@ stdcall EnumPrintersA(long ptr long ptr long ptr ptr)
@ stdcall EnumPrintersW(long ptr long ptr long ptr ptr)
@ stub ExtDeviceMode
@ stub FindClosePrinterChangeNotification
@ stub FindFirstPrinterChangeNotification
@ stub FindNextPrinterChangeNotification
@ stub FreePrinterNotifyInfo
@ stdcall GetDefaultPrinterA(ptr ptr)
@ stdcall GetDefaultPrinterW(ptr ptr)
@ stdcall GetFormA(long str long ptr long ptr)
@ stdcall GetFormW(long wstr long ptr long ptr)
@ stub GetJobA
@ stub GetJobW
@ stdcall GetPrintProcessorDirectoryA(str str long ptr long ptr)
@ stub GetPrintProcessorDirectoryW
@ stdcall GetPrinterA(long long ptr long ptr)
@ stdcall GetPrinterDataA(long str ptr ptr long ptr)
@ stdcall GetPrinterDataExA(long str str ptr ptr long ptr)
@ stdcall GetPrinterDataExW(long wstr wstr ptr ptr long ptr)
@ stdcall GetPrinterDataW(long wstr ptr ptr long ptr)
@ stdcall GetPrinterDriverA(long str long ptr long ptr)
@ stdcall GetPrinterDriverDirectoryA(str str long ptr long ptr)
@ stdcall GetPrinterDriverDirectoryW(wstr wstr long ptr long ptr)
@ stdcall GetPrinterDriverW(long str long ptr long ptr)
@ stdcall GetPrinterW(long long ptr long ptr)
@ stub InitializeDll
@ stdcall OpenPrinterA(str ptr ptr)
@ stdcall OpenPrinterW(wstr ptr ptr)
@ stub PlayGdiScriptOnPrinterIC
@ stub PrinterMessageBoxA
@ stub PrinterMessageBoxW
@ stdcall PrinterProperties(long long)
@ stdcall ReadPrinter(long ptr long ptr)
@ stdcall ResetPrinterA(long ptr)
@ stdcall ResetPrinterW(long ptr)
@ stub ScheduleJob
@ stub SetAllocFailCount
@ stdcall SetFormA(long str long ptr)
@ stdcall SetFormW(long wstr long ptr)
@ stdcall SetJobA(long long long ptr long)
@ stdcall SetJobW(long long long ptr long)
@ stdcall SetPrinterA(long long ptr long)
@ stdcall SetPrinterDataA(long str long ptr long)
@ stdcall SetPrinterDataExA(long str str long ptr long)
@ stdcall SetPrinterDataExW(long wstr wstr long ptr long)
@ stdcall SetPrinterDataW(long wstr long ptr long)
@ stdcall SetPrinterW(long long ptr long)
@ stub SpoolerDevQueryPrintW
@ stub SpoolerInit
@ stub StartDocDlgA
@ stub StartDocDlgW
@ stdcall StartDocPrinterA(long long ptr)
@ stdcall StartDocPrinterW(long long ptr)
@ stdcall StartPagePrinter(long)
@ stub WaitForPrinterChange
@ stdcall WritePrinter(long ptr long ptr)
@ stdcall XcvDataW(long wstr ptr long ptr long ptr ptr)
