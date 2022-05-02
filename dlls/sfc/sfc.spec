1 stdcall @() sfc_os.SfcInitProt
2 stdcall @() sfc_os.SfcTerminateWatcherThread
3 stdcall @(long) sfc_os.SfcConnectToServer
4 stdcall @() sfc_os.SfcClose
5 stdcall @() sfc_os.SfcFileException
6 stdcall @() sfc_os.SfcInitiateScan
7 stdcall @() sfc_os.SfcInstallProtectedFiles
8 stdcall @() sfc_os.SfpInstallCatalog
9 stdcall @() sfc_os.SfpDeleteCatalog
@ stdcall SRSetRestorePoint(ptr ptr) sfc_os.SRSetRestorePointA
@ stdcall SRSetRestorePointA(ptr ptr) sfc_os.SRSetRestorePointA
@ stdcall SRSetRestorePointW(ptr ptr) sfc_os.SRSetRestorePointW
@ stdcall SfcGetNextProtectedFile(long ptr) sfc_os.SfcGetNextProtectedFile
@ stdcall SfcIsFileProtected(ptr wstr) sfc_os.SfcIsFileProtected
@ stdcall SfcIsKeyProtected(long wstr long) sfc_os.SfcIsKeyProtected
@ stdcall SfpVerifyFile(str ptr long) sfc_os.SfpVerifyFile
