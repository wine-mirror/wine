100 stub @
@ stub ADVANCEDSETUPDIALOG
@ stub AbortPrinter
@ stdcall AddFormA(long long ptr) AddFormA
@ stdcall AddFormW(long long ptr) AddFormW
@ stdcall AddJobA(long long ptr long ptr) AddJobA
@ stdcall AddJobW(long long ptr long ptr) AddJobW
@ stdcall AddMonitorA(str long ptr) AddMonitorA
@ stub AddMonitorW
@ stub AddPortA
@ stub AddPortExA
@ stub AddPortExW
@ stub AddPortW
@ stub AddPrintProcessorA
@ stub AddPrintProcessorW
@ stub AddPrintProvidorA
@ stub AddPrintProvidorW
@ stdcall AddPrinterA(str long ptr) AddPrinterA
@ stub AddPrinterConnectionA
@ stub AddPrinterConnectionW
@ stdcall AddPrinterDriverA(str long ptr) AddPrinterDriverA
@ stdcall AddPrinterDriverW(wstr long ptr) AddPrinterDriverW
@ stdcall AddPrinterW(wstr long ptr) AddPrinterW
@ stub AdvancedDocumentPropertiesA
@ stub AdvancedDocumentPropertiesW
@ stub AdvancedSetupDialog
@ stdcall ClosePrinter(long) ClosePrinter
@ stub ConfigurePortA
@ stub ConfigurePortW
@ stub ConnectToPrinterDlg
@ stub CreatePrinterIC
@ stub DEVICECAPABILITIES
@ stub DEVICEMODE
@ stdcall DeleteFormA(long str) DeleteFormA
@ stdcall DeleteFormW(long wstr) DeleteFormW
@ stdcall DeleteMonitorA(str str str) DeleteMonitorA
@ stub DeleteMonitorW
@ stdcall DeletePortA(str long str) DeletePortA
@ stub DeletePortW
@ stub DeletePrintProcessorA
@ stub DeletePrintProcessorW
@ stub DeletePrintProvidorA
@ stub DeletePrintProvidorW
@ stdcall DeletePrinter(long) DeletePrinter
@ stub DeletePrinterConnectionA
@ stub DeletePrinterConnectionW
@ stdcall DeletePrinterDriverA(str str str) DeletePrinterDriverA
@ stub DeletePrinterDriverW
@ stub DeletePrinterIC
@ stub DevQueryPrint
@ stdcall DeviceCapabilities(str str long ptr ptr) DeviceCapabilitiesA
@ stdcall DeviceCapabilitiesA(str str long ptr ptr) DeviceCapabilitiesA
@ stdcall DeviceCapabilitiesW(wstr wstr long wstr ptr) DeviceCapabilitiesW
@ stub DeviceMode
@ stub DocumentEvent
@ stdcall DocumentPropertiesA(long long ptr ptr ptr long) DocumentPropertiesA
@ stdcall DocumentPropertiesW(long long ptr ptr ptr long) DocumentPropertiesW
@ stub EXTDEVICEMODE
@ stdcall EndDocPrinter(long) EndDocPrinter
@ stdcall EndPagePrinter(long) EndPagePrinter
@ stub EnumFormsA
@ stub EnumFormsW
@ stdcall EnumJobsA(long long long long ptr long ptr ptr) EnumJobsA
@ stdcall EnumJobsW(long long long long ptr long ptr ptr) EnumJobsW
@ stub EnumMonitorsA
@ stub EnumMonitorsW
@ stdcall EnumPortsA(ptr long ptr ptr ptr ptr) EnumPortsA
@ stub EnumPortsW
@ stub EnumPrintProcessorDatatypesA
@ stub EnumPrintProcessorDatatypesW
@ stub EnumPrintProcessorsA
@ stub EnumPrintProcessorsW
@ stub EnumPrinterDataA
@ stdcall EnumPrinterDataExA(long str ptr long ptr ptr) EnumPrinterDataExA
@ stdcall EnumPrinterDataExW(long wstr ptr long ptr ptr) EnumPrinterDataExW
@ stub EnumPrinterDataW
@ stdcall EnumPrinterDriversA(str str long ptr long ptr ptr) EnumPrinterDriversA
@ stdcall EnumPrinterDriversW(wstr wstr long ptr long ptr ptr) EnumPrinterDriversW
@ stdcall EnumPrintersA(long ptr long ptr long ptr ptr) EnumPrintersA
@ stdcall EnumPrintersW(long ptr long ptr long ptr ptr) EnumPrintersW
@ stub ExtDeviceMode
@ stub FindClosePrinterChangeNotification
@ stub FindFirstPrinterChangeNotification
@ stub FindNextPrinterChangeNotification
@ stub FreePrinterNotifyInfo
@ stdcall GetDefaultPrinterA(str ptr) GetDefaultPrinterA
@ stdcall GetDefaultPrinterW(wstr ptr) GetDefaultPrinterW
@ stdcall GetFormA(long str long ptr long ptr) GetFormA
@ stdcall GetFormW(long wstr long ptr long ptr) GetFormW
@ stub GetJobA
@ stub GetJobW
@ stub GetPrintProcessorDirectoryA
@ stub GetPrintProcessorDirectoryW
@ stdcall GetPrinterA(long long ptr long ptr) GetPrinterA
@ stdcall GetPrinterDataA(long str ptr ptr long ptr) GetPrinterDataA
@ stdcall GetPrinterDataExA(long str str ptr ptr long ptr) GetPrinterDataExA
@ stdcall GetPrinterDataExW(long wstr wstr ptr ptr long ptr) GetPrinterDataExW
@ stdcall GetPrinterDataW(long wstr ptr ptr long ptr) GetPrinterDataW
@ stdcall GetPrinterDriverA(long str long ptr long ptr) GetPrinterDriverA
@ stdcall GetPrinterDriverDirectoryA(str str long ptr long ptr) GetPrinterDriverDirectoryA
@ stdcall GetPrinterDriverDirectoryW(wstr wstr long ptr long ptr) GetPrinterDriverDirectoryW
@ stdcall GetPrinterDriverW(long str long ptr long ptr) GetPrinterDriverW
@ stdcall GetPrinterW(long long ptr long ptr) GetPrinterW
@ stub InitializeDll
@ stdcall OpenPrinterA(str ptr ptr) OpenPrinterA
@ stdcall OpenPrinterW(wstr ptr ptr) OpenPrinterW
@ stub PlayGdiScriptOnPrinterIC
@ stub PrinterMessageBoxA
@ stub PrinterMessageBoxW
@ stdcall PrinterProperties(long long) PrinterProperties
@ stdcall ReadPrinter(long ptr long ptr) ReadPrinter
@ stdcall ResetPrinterA(long ptr) ResetPrinterA
@ stdcall ResetPrinterW(long ptr) ResetPrinterW
@ stub ScheduleJob
@ stub SetAllocFailCount
@ stdcall SetFormA(long str long ptr) SetFormA
@ stdcall SetFormW(long wstr long ptr) SetFormW
@ stdcall SetJobA(long long long ptr long) SetJobA
@ stdcall SetJobW(long long long ptr long) SetJobW
@ stdcall SetPrinterA(long long ptr long) SetPrinterA
@ stdcall SetPrinterDataA(long str long ptr long) SetPrinterDataA
@ stdcall SetPrinterDataExA(long str str long ptr long) SetPrinterDataExA
@ stdcall SetPrinterDataExW(long wstr wstr long ptr long) SetPrinterDataExW
@ stdcall SetPrinterDataW(long wstr long ptr long) SetPrinterDataW
@ stdcall SetPrinterW(long long ptr long) SetPrinterW
@ stub SpoolerDevQueryPrintW
@ stub SpoolerInit
@ stub StartDocDlgA
@ stub StartDocDlgW
@ stdcall StartDocPrinterA(long long ptr) StartDocPrinterA
@ stdcall StartDocPrinterW(long long ptr) StartDocPrinterW
@ stdcall StartPagePrinter(long) StartPagePrinter
@ stub WaitForPrinterChange
@ stdcall WritePrinter(long ptr long ptr) WritePrinter
