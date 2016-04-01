1 stub @ # sfc_os.SfcInitProt
2 stub @ # sfc_os.SfcTerminateWatcherThread
3 stdcall @(long) sfc_os.SfcConnectToServer
4 stub @ # sfc_os.SfcClose
5 stub @ # sfc_os.SfcFileException
6 stub @ # sfc_os.SfcInitiateScan
7 stub @ # sfc_os.SfcInstallProtectedFiles
8 stub @ # sfc_os.SfpInstallCatalog
9 stub @ # SfpDeleteCatalog
@ stdcall SRSetRestorePoint(ptr ptr) sfc_os.SRSetRestorePointA
@ stdcall SRSetRestorePointA(ptr ptr) sfc_os.SRSetRestorePointA
@ stdcall SRSetRestorePointW(ptr ptr) sfc_os.SRSetRestorePointW
@ stdcall SfcGetNextProtectedFile(long ptr) sfc_os.SfcGetNextProtectedFile
@ stdcall SfcIsFileProtected(ptr wstr) sfc_os.SfcIsFileProtected
@ stdcall SfcIsKeyProtected(long wstr long) sfc_os.SfcIsKeyProtected
@ stub SfcWLEventLogoff
@ stub SfcWLEventLogon
@ stub SfpVerifyFile
