# Functions exported by the Win95 shell32.dll
# (these need to have these exact ordinals, for some
#  win95 and winNT dlls import shell32.dll by ordinal)
# This list was updated to dll version 4.72

   2 stdcall -ordinal SHChangeNotifyRegister(long long long long long ptr)
   3 stdcall SHDefExtractIconA(str long long ptr ptr long)
   4 stdcall -ordinal SHChangeNotifyDeregister(long)
   5 stdcall -noname SHChangeNotifyUpdateEntryList(long long long long)
   6 stdcall SHDefExtractIconW(wstr long long ptr ptr long)
   9 stub -ordinal PifMgr_OpenProperties
  10 stub -ordinal PifMgr_GetProperties
  11 stub -ordinal PifMgr_SetProperties
  13 stub -ordinal PifMgr_CloseProperties
  15 stdcall -noname ILGetDisplayName(ptr ptr)
  16 stdcall -ordinal ILFindLastID(ptr)
  17 stdcall -ordinal ILRemoveLastID(ptr)
  18 stdcall -ordinal ILClone(ptr)
  19 stdcall -ordinal ILCloneFirst(ptr)
  20 stdcall -noname ILGlobalClone(ptr)
  21 stdcall -ordinal ILIsEqual(ptr ptr)
  23 stdcall -ordinal ILIsParent(ptr ptr long)
  24 stdcall -ordinal ILFindChild(ptr ptr)
  25 stdcall -ordinal ILCombine(ptr ptr)
  26 stdcall -noname ILLoadFromStream(ptr ptr)
  27 stdcall -ordinal ILSaveToStream(ptr ptr)
  28 stdcall SHILCreateFromPath(ptr ptr ptr) SHILCreateFromPathAW
  29 stdcall -noname PathIsRoot(ptr) PathIsRootAW
  30 stdcall -noname PathBuildRoot(ptr long) PathBuildRootAW
  31 stdcall -noname PathFindExtension(ptr) PathFindExtensionAW
  32 stdcall -noname PathAddBackslash(ptr) PathAddBackslashAW
  33 stdcall -noname PathRemoveBlanks(ptr) PathRemoveBlanksAW
  34 stdcall -noname PathFindFileName(ptr) PathFindFileNameAW
  35 stdcall -noname PathRemoveFileSpec(ptr) PathRemoveFileSpecAW
  36 stdcall -noname PathAppend(ptr ptr) PathAppendAW
  37 stdcall -noname PathCombine(ptr ptr ptr) PathCombineAW
  38 stdcall -noname PathStripPath(ptr) PathStripPathAW
  39 stdcall -noname PathIsUNC(ptr) PathIsUNCAW
  40 stdcall -noname PathIsRelative(ptr) PathIsRelativeAW
  41 stdcall -ordinal IsLFNDriveA(str)
  42 stdcall -ordinal IsLFNDriveW(wstr)
  43 stdcall -ordinal PathIsExe(ptr) PathIsExeAW
  45 stdcall -noname PathFileExists(ptr) PathFileExistsAW
  46 stdcall -noname PathMatchSpec(ptr ptr) PathMatchSpecAW
  47 stdcall -ordinal PathMakeUniqueName(ptr long ptr ptr ptr) PathMakeUniqueNameAW
  48 stdcall -noname PathSetDlgItemPath(long long ptr) PathSetDlgItemPathAW
  49 stdcall -ordinal PathQualify(ptr) PathQualifyAW
  50 stdcall -noname PathStripToRoot(ptr) PathStripToRootAW
  51 stdcall PathResolve(str long long) PathResolveAW
  52 stdcall -noname PathGetArgs(str) PathGetArgsAW
  53 stdcall DoEnvironmentSubst(ptr long) DoEnvironmentSubstAW
  54 stub LogoffWindowsDialog
  55 stdcall -noname PathQuoteSpaces(ptr) PathQuoteSpacesAW
  56 stdcall -noname PathUnquoteSpaces(str) PathUnquoteSpacesAW
  57 stdcall -noname PathGetDriveNumber(str) PathGetDriveNumberAW
  58 stdcall -noname ParseField(str long ptr long) ParseFieldAW
  59 stdcall -ordinal RestartDialog(long wstr long)
  60 stdcall -noname ExitWindowsDialog(long)
  61 stdcall -noname RunFileDlg(long long str str str long) RunFileDlgAW
  62 stdcall -ordinal PickIconDlg(long wstr long ptr)
  63 stdcall -ordinal GetFileNameFromBrowse(long ptr long str str str str) GetFileNameFromBrowseAW
  64 stdcall -ordinal DriveType(long)
  65 stdcall -noname InvalidateDriveType(long)
  66 stdcall -ordinal IsNetDrive(long)
  67 stdcall -ordinal Shell_MergeMenus(long long long long long long)
  68 stdcall -ordinal SHGetSetSettings(ptr long long)
  69 stub -noname SHGetNetResource
  70 stdcall -noname SHCreateDefClassObject(ptr ptr long ptr ptr)
  71 stdcall -ordinal Shell_GetImageLists(ptr ptr)
  72 stdcall -ordinal Shell_GetCachedImageIndex(str long long) Shell_GetCachedImageIndexAW
  73 stdcall -ordinal SHShellFolderView_Message(long long long)
  74 stdcall -ordinal SHCreateStdEnumFmtEtc(long ptr ptr)
  75 stdcall -ordinal PathYetAnotherMakeUniqueName(ptr wstr wstr wstr)
  76 stub DragQueryInfo
  77 stdcall -ordinal SHMapPIDLToSystemImageListIndex(ptr ptr ptr)
  78 stdcall -noname OleStrToStrN(str long wstr long) OleStrToStrNAW
  79 stdcall -noname StrToOleStrN(wstr long str long) StrToOleStrNAW
  83 stdcall -ordinal CIDLData_CreateFromIDArray(ptr long ptr ptr)
  84 stub SHIsBadInterfacePtr
  85 stdcall -ordinal OpenRegStream(long str str long) shlwapi.SHOpenRegStreamA
  86 stdcall -noname SHRegisterDragDrop(long ptr)
  87 stdcall -noname SHRevokeDragDrop(long)
  88 stdcall -ordinal SHDoDragDrop(long ptr ptr long ptr)
  89 stdcall -ordinal SHCloneSpecialIDList(long long long)
  90 stdcall -ordinal SHFindFiles(ptr ptr)
  91 stub SHFindComputer
  92 stdcall -ordinal PathGetShortPath(ptr) PathGetShortPathAW
  93 stdcall -noname Win32CreateDirectory(wstr ptr) Win32CreateDirectoryAW
  94 stdcall -noname Win32RemoveDirectory(wstr) Win32RemoveDirectoryAW
  95 stdcall -noname SHLogILFromFSIL(ptr)
  96 stdcall -noname StrRetToStrN(ptr long ptr ptr) StrRetToStrNAW
  97 stdcall -noname SHWaitForFileToOpen (ptr long long)
  98 stdcall -ordinal SHGetRealIDL(ptr ptr ptr)
  99 stdcall -noname SetAppStartingCursor(long long)
 100 stdcall -ordinal SHRestricted(long)

 102 stdcall -ordinal SHCoCreateInstance(wstr ptr ptr ptr ptr)
 103 stdcall -ordinal SignalFileOpen(ptr)
 104 stdcall -noname FileMenu_DeleteAllItems(long)
 105 stdcall -noname FileMenu_DrawItem(long ptr)
 106 stdcall -noname FileMenu_FindSubMenuByPidl(long ptr)
 107 stdcall -noname FileMenu_GetLastSelectedItemPidls(long ptr ptr)
 108 stdcall -noname FileMenu_HandleMenuChar(long long)
 109 stdcall -noname FileMenu_InitMenuPopup(long)
 110 stdcall -noname FileMenu_InsertUsingPidl (long long ptr long long ptr)
 111 stdcall -noname FileMenu_Invalidate(long)
 112 stdcall -noname FileMenu_MeasureItem(long ptr)
 113 stdcall -noname FileMenu_ReplaceUsingPidl(long long ptr long ptr)
 114 stdcall -noname FileMenu_Create(long long long long long)
 115 stdcall -noname FileMenu_AppendItem(long ptr long long long long) FileMenu_AppendItemAW
 116 stdcall -noname FileMenu_TrackPopupMenuEx(long long long long long long)
 117 stdcall -noname FileMenu_DeleteItemByCmd(long long)
 118 stdcall -noname FileMenu_Destroy(long)
 119 stdcall -ordinal IsLFNDrive(ptr) IsLFNDriveAW
 120 stdcall -noname FileMenu_AbortInitMenu()
 121 stdcall -noname SHFlushClipboard()
 122 stdcall -noname RunDLL_CallEntry16(long long ptr str long)
 123 stdcall -noname SHFreeUnusedLibraries()
 124 stdcall -noname FileMenu_AppendFilesForPidl(long ptr long)
 125 stdcall -noname FileMenu_AddFilesForPidl(long long long ptr long long ptr)
 126 stdcall -noname SHOutOfMemoryMessageBox(long str long)
 127 stdcall -noname SHWinHelp(long long long long)
 128 stdcall -noname SHDllGetClassObject(ptr ptr ptr) DllGetClassObject
 129 stdcall -ordinal DAD_AutoScroll(long ptr ptr)
 130 stdcall -noname DAD_DragEnter(long)
 131 stdcall -ordinal DAD_DragEnterEx(long int64)
 132 stdcall -ordinal DAD_DragLeave()
 134 stdcall -ordinal DAD_DragMove(int64)
 136 stdcall -ordinal DAD_SetDragImage(long ptr)
 137 stdcall -ordinal DAD_ShowDragImage(long)
 139 stub Desktop_UpdateBriefcaseOnEvent
 140 stdcall -noname FileMenu_DeleteItemByIndex(long long)
 141 stdcall -noname FileMenu_DeleteItemByFirstID(long long)
 142 stdcall -noname FileMenu_DeleteSeparator(long)
 143 stdcall -noname FileMenu_EnableItemByCmd(long long long)
 144 stdcall -noname FileMenu_GetItemExtent(long long)
 145 stdcall -noname PathFindOnPath(ptr ptr) PathFindOnPathAW
 146 stdcall -noname RLBuildListOfPaths()
 147 stdcall -ordinal SHCLSIDFromString(ptr ptr) SHCLSIDFromStringAW
 148 stdcall -noname SHMapIDListToImageListIndexAsync(ptr ptr ptr long ptr ptr ptr ptr ptr)
 149 stdcall -ordinal SHFind_InitMenuPopup(long long long long)

 151 stdcall -noname SHLoadOLE(long)
 152 stdcall -ordinal ILGetSize(ptr)
 153 stdcall -ordinal ILGetNext(ptr)
 154 stdcall -ordinal ILAppendID(ptr ptr long)
 155 stdcall -ordinal ILFree(ptr)
 156 stdcall -noname ILGlobalFree(ptr)
 157 stdcall -ordinal ILCreateFromPath(ptr) ILCreateFromPathAW
 158 stdcall -noname PathGetExtension(str long long) PathGetExtensionAW
 159 stdcall -noname PathIsDirectory(ptr) PathIsDirectoryAW
 160 stub SHNetConnectionDialog
 161 stdcall -noname SHRunControlPanel(wstr long)
 162 stdcall -ordinal SHSimpleIDListFromPath(ptr) SHSimpleIDListFromPathAW
 163 stdcall -noname StrToOleStr(wstr str) StrToOleStrAW
 164 stdcall -ordinal Win32DeleteFile(str) Win32DeleteFileAW
 165 stdcall -ordinal SHCreateDirectory(long ptr)
 166 stdcall -noname CallCPLEntry16(long ptr long long long long)
 167 stdcall -ordinal SHAddFromPropSheetExtArray(long ptr long)
 168 stdcall -ordinal SHCreatePropSheetExtArray(long wstr long)
 169 stdcall -ordinal SHDestroyPropSheetExtArray(long)
 170 stdcall -ordinal SHReplaceFromPropSheetExtArray(long long ptr long)
 171 stdcall -ordinal PathCleanupSpec(ptr ptr)
 172 stdcall -noname SHCreateLinks(long str ptr long ptr)
 173 stdcall -ordinal SHValidateUNC(long wstr long)
 174 stdcall -ordinal SHCreateShellFolderViewEx(ptr ptr)
 175 stdcall -noname SHGetSpecialFolderPath(long long long long) SHGetSpecialFolderPathAW
 176 stdcall -ordinal SHSetInstanceExplorer(ptr) shcore.SetProcessReference
 177 stub DAD_SetDragImageFromListView
 178 stdcall -ordinal SHObjectProperties(long long wstr wstr)
 179 stdcall -ordinal SHGetNewLinkInfoA(str str ptr ptr long)
 180 stdcall -ordinal SHGetNewLinkInfoW(wstr wstr ptr ptr long)
 181 stdcall -noname RegisterShellHook(long long)
 182 varargs -ordinal ShellMessageBoxW(long long wstr wstr long)
 183 varargs -ordinal ShellMessageBoxA(long long str str long)
 184 stdcall -noname ArrangeWindows(long long ptr long ptr)
 185 stub SHHandleDiskFull
 186 stdcall -noname ILGetDisplayNameEx(ptr ptr ptr long)
 187 stub ILGetPseudoNameW
 188 stdcall -noname ShellDDEInit(long)
 189 stdcall -ordinal ILCreateFromPathA(str)
 190 stdcall -ordinal ILCreateFromPathW(wstr)
 191 stdcall -ordinal SHUpdateImageA(str long long long)
 192 stdcall -ordinal SHUpdateImageW(wstr long long long)
 193 stdcall -ordinal SHHandleUpdateImage(ptr)
 194 stdcall -noname SHCreatePropSheetExtArrayEx(long wstr long ptr)
 195 stdcall -ordinal SHFree(ptr)
 196 stdcall -ordinal SHAlloc(long)
 197 stub SHGlobalDefect
 198 stdcall -noname SHAbortInvokeCommand()
 199 stub SHGetFileIcon
 200 stub SHLocalAlloc
 201 stub SHLocalFree
 202 stub SHLocalReAlloc
 203 stub AddCommasW
 204 stub ShortSizeFormatW
 205 stdcall Printer_LoadIconsW(wstr ptr ptr)
 206 stub Link_AddExtraDataSection
 207 stub Link_ReadExtraDataSection
 208 stub Link_RemoveExtraDataSection
 209 stub Int64ToString
 210 stub LargeIntegerToString
 211 stub Printers_GetPidl
 212 stub Printers_AddPrinterPropPages
 213 stdcall Printers_RegisterWindowW(wstr long ptr ptr)
 214 stdcall Printers_UnregisterWindow(long long)
 215 stdcall -noname SHStartNetConnectionDialog(long str long)
 243 stdcall @(long long) shell32_243
 244 stdcall -noname SHInitRestricted(ptr ptr)
 249 stdcall -noname PathParseIconLocation(ptr) PathParseIconLocationAW
 250 stdcall -noname PathRemoveExtension(ptr) PathRemoveExtensionAW
 251 stdcall -noname PathRemoveArgs(ptr) PathRemoveArgsAW
 256 stdcall SHCreateShellFolderView(ptr ptr)
 258 stdcall -noname LinkWindow_RegisterClass()
 259 stdcall -noname LinkWindow_UnregisterClass()
#299 stub Shl1632_ThunkData32
#300 stub Shl3216_ThunkData32

 505 stdcall SHRegCloseKey (long)
 506 stdcall SHRegOpenKeyA (long str long)
 507 stdcall SHRegOpenKeyW (long wstr long)
 508 stdcall SHRegQueryValueA(long str ptr ptr)
 509 stdcall SHRegQueryValueExA(long str ptr ptr ptr ptr)
 510 stdcall SHRegQueryValueW (long long long long)
 511 stdcall SHRegQueryValueExW (long wstr ptr ptr ptr ptr)
 512 stdcall SHRegDeleteKeyW (long wstr)

 520 stdcall -noname -import SHAllocShared(ptr long long)
 521 stdcall -noname -import SHLockShared(long long)
 522 stdcall -noname -import SHUnlockShared(ptr)
 523 stdcall -noname -import SHFreeShared(long long)
 524 stdcall -ordinal RealDriveType(long long)
 525 stub RealDriveTypeFlags
 526 stdcall SHFlushSFCache()

 640 stdcall -noname NTSHChangeNotifyRegister(long long long long long ptr)
 641 stdcall -noname NTSHChangeNotifyDeregister(long)

 643 stub SHChangeNotifyReceive
 644 stdcall -ordinal SHChangeNotification_Lock(long long ptr ptr)
 645 stdcall -ordinal SHChangeNotification_Unlock(long)
 646 stub SHChangeRegistrationReceive
 647 stub ReceiveAddToRecentDocs
 648 stub SHWaitOp_Operate

 650 stdcall -noname PathIsSameRoot(ptr ptr) PathIsSameRootAW

 651 stdcall -noname @(ptr long) ReadCabinetState # OldReadCabinetState
 652 stdcall -noname WriteCabinetState(ptr)
 653 stdcall -noname PathProcessCommand(long long long long) PathProcessCommandAW
 654 stdcall ReadCabinetState(ptr long)

 660 stdcall -noname FileIconInit(long)
 680 stdcall IsUserAnAdmin()

 685 stdcall SHPropStgCreate(ptr ptr ptr long long long ptr ptr)
 688 stdcall SHPropStgReadMultiple(ptr long long ptr ptr)
 689 stdcall SHPropStgWriteMultiple(ptr ptr long ptr ptr long)

 701 stdcall CDefFolderMenu_Create2(ptr ptr long ptr ptr ptr long ptr ptr)
 704 stdcall -noname GUIDFromStringW(wstr ptr)
 709 stdcall SHGetSetFolderCustomSettings(ptr wstr long)
 714 stdcall -noname PathIsTemporaryW(wstr)
 723 stdcall -noname SHCreateSessionKey(long ptr)
 727 stdcall SHGetImageList(long ptr ptr)
 730 stdcall -noname RestartDialogEx(long wstr long long)
 743 stdcall SHCreateFileExtractIconW(wstr long ptr ptr)
 747 stdcall SHLimitInputEdit(ptr ptr)

1217 stub FOOBAR1217   # no joke! This is the real name!!

@ stdcall CheckEscapesA(str long)
@ stdcall CheckEscapesW(wstr long)
@ stdcall CommandLineToArgvW(wstr ptr) shcore.CommandLineToArgvW
@ stdcall Control_FillCache_RunDLL(long long long long) Control_FillCache_RunDLLA
@ stdcall Control_FillCache_RunDLLA(long long long long)
@ stdcall Control_FillCache_RunDLLW(long long long long)
@ stdcall Control_RunDLL(ptr ptr str long) Control_RunDLLA
@ stdcall Control_RunDLLA(ptr ptr str long)
@ stub Control_RunDLLAsUserW
@ stdcall Control_RunDLLW(ptr ptr wstr long)
@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllGetVersion(ptr)
@ stdcall -private DllInstall(long wstr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stdcall DoEnvironmentSubstA(str long)
@ stdcall DoEnvironmentSubstW(wstr long)
@ stdcall DragAcceptFiles(long long)
@ stdcall DragFinish(long)
@ stdcall DragQueryFile(long long ptr long) DragQueryFileA
@ stdcall DragQueryFileA(long long ptr long)
@ stub DragQueryFileAorW
@ stdcall DragQueryFileW(long long ptr long)
@ stdcall DragQueryPoint(long ptr)
@ stdcall DuplicateIcon(long long)
@ stdcall ExtractAssociatedIconA(long str ptr)
@ stdcall ExtractAssociatedIconExA(long str ptr ptr)
@ stdcall ExtractAssociatedIconExW(long wstr ptr ptr)
@ stdcall ExtractAssociatedIconW(long wstr ptr)
@ stdcall ExtractIconA(long str long)
@ stdcall ExtractIconEx(str long ptr ptr long) ExtractIconExA
@ stdcall ExtractIconExA(str long ptr ptr long)
@ stdcall ExtractIconExW(wstr long ptr ptr long)
@ stub ExtractIconResInfoA
@ stub ExtractIconResInfoW
@ stdcall ExtractIconW(long wstr long)
@ stdcall ExtractVersionResource16W(wstr long)
@ stub FindExeDlgProc
@ stdcall FindExecutableA(str str ptr)
@ stdcall FindExecutableW(wstr wstr ptr)
@ stub FixupOptionalComponents
@ stdcall FreeIconList(long)
@ stdcall GetCurrentProcessExplicitAppUserModelID(ptr) shcore.GetCurrentProcessExplicitAppUserModelID
@ stdcall InitNetworkAddressControl()
@ stub InternalExtractIconListA
@ stub InternalExtractIconListW
@ stub OCInstall
@ stdcall OpenAs_RunDLL(long long str long) OpenAs_RunDLLA
@ stdcall OpenAs_RunDLLA(long long str long)
@ stdcall OpenAs_RunDLLW(long long wstr long)
@ stub PrintersGetCommand_RunDLL
@ stub PrintersGetCommand_RunDLLA
@ stub PrintersGetCommand_RunDLLW
@ stub RealShellExecuteA
@ stub RealShellExecuteExA
@ stub RealShellExecuteExW
@ stub RealShellExecuteW
@ stdcall RegenerateUserEnvironment(ptr long)
@ stdcall SetCurrentProcessExplicitAppUserModelID(wstr) shcore.SetCurrentProcessExplicitAppUserModelID
@ stdcall SHAddToRecentDocs (long ptr)
@ stdcall SHAppBarMessage(long ptr)
@ stdcall SHAssocEnumHandlers(wstr long ptr)
@ stdcall SHBindToFolderIDListParent(ptr ptr ptr ptr ptr)
@ stdcall SHBindToObject(ptr ptr ptr ptr ptr)
@ stdcall SHBindToParent(ptr ptr ptr ptr)
@ stdcall SHBrowseForFolder(ptr) SHBrowseForFolderA
@ stdcall SHBrowseForFolderA(ptr)
@ stdcall SHBrowseForFolderW(ptr)
@ stdcall SHChangeNotify (long long ptr ptr)
@ stub SHChangeNotifySuspendResume
@ stdcall SHCreateAssociationRegistration(ptr ptr)
@ stdcall SHCreateDataObject(ptr long ptr ptr ptr ptr)
@ stdcall SHCreateDefaultContextMenu(ptr ptr ptr)
@ stdcall SHCreateDirectoryExA(long str ptr)
@ stdcall SHCreateDirectoryExW(long wstr ptr)
@ stdcall SHCreateItemFromIDList(ptr ptr ptr)
@ stdcall SHCreateItemFromParsingName(wstr ptr ptr ptr)
@ stdcall SHCreateItemInKnownFolder(ptr long wstr ptr ptr)
@ stdcall SHCreateItemFromRelativeName(ptr wstr ptr ptr ptr)
@ stdcall SHCreateItemWithParent(ptr ptr ptr ptr ptr)
@ stub SHCreateProcessAsUserW
@ stdcall SHCreateQueryCancelAutoPlayMoniker(ptr)
@ stdcall SHCreateShellItem(ptr ptr ptr ptr)
@ stdcall SHCreateShellItemArray(ptr ptr long ptr ptr)
@ stdcall SHCreateShellItemArrayFromDataObject(ptr ptr ptr)
@ stdcall SHCreateShellItemArrayFromShellItem(ptr ptr ptr)
@ stdcall SHCreateShellItemArrayFromIDLists(long ptr ptr)
@ stdcall SHEmptyRecycleBinA(long str long)
@ stdcall SHEmptyRecycleBinW(long wstr long)
@ stdcall SHEnumerateUnreadMailAccountsW(ptr long ptr long)
@ stdcall SHExtractIconsW(wstr long long long ptr ptr long long) user32.PrivateExtractIconsW
@ stdcall SHFileOperation(ptr) SHFileOperationA
@ stdcall SHFileOperationA(ptr)
@ stdcall SHFileOperationW(ptr)
@ stdcall SHFormatDrive(long long long long)
@ stdcall SHFreeNameMappings(ptr)
@ stdcall SHGetDataFromIDListA(ptr ptr long ptr long)
@ stdcall SHGetDataFromIDListW(ptr ptr long ptr long)
@ stdcall SHGetDesktopFolder(ptr)
@ stdcall SHGetDiskFreeSpaceA(str ptr ptr ptr) kernel32.GetDiskFreeSpaceExA
@ stdcall SHGetDiskFreeSpaceExA(str ptr ptr ptr) kernel32.GetDiskFreeSpaceExA
@ stdcall SHGetDiskFreeSpaceExW(wstr ptr ptr ptr) kernel32.GetDiskFreeSpaceExW
@ stdcall SHGetFileInfo(str long ptr long long) SHGetFileInfoA
@ stdcall SHGetFileInfoA(str long ptr long long)
@ stdcall SHGetFileInfoW(wstr long ptr long long)
@ stdcall SHGetFolderLocation(long long long long ptr)
@ stdcall SHGetFolderPathA(long long long long ptr)
@ stdcall SHGetFolderPathEx(ptr long ptr ptr long)
@ stdcall SHGetFolderPathAndSubDirA(long long long long str ptr)
@ stdcall SHGetFolderPathAndSubDirW(long long long long wstr ptr)
@ stdcall SHGetFolderPathW(long long long long ptr)
@ stub SHGetFreeDiskSpace
@ stdcall SHGetIconOverlayIndexA(str long)
@ stdcall SHGetIconOverlayIndexW(wstr long)
@ stdcall SHGetIDListFromObject(ptr ptr)
@ stdcall SHGetInstanceExplorer(ptr) shcore.GetProcessReference
@ stdcall SHGetItemFromDataObject(ptr long ptr ptr)
@ stdcall SHGetItemFromObject(ptr ptr ptr)
@ stdcall SHGetKnownFolderIDList(ptr long ptr ptr)
@ stdcall SHGetKnownFolderItem(ptr long long ptr ptr)
@ stdcall SHGetKnownFolderPath(ptr long ptr ptr)
@ stdcall SHGetLocalizedName(wstr ptr long ptr)
@ stdcall SHGetMalloc(ptr)
@ stdcall SHGetNameFromIDList(ptr long ptr)
@ stdcall SHGetNewLinkInfo(str str ptr ptr long) SHGetNewLinkInfoA
@ stdcall SHGetPathFromIDList(ptr ptr) SHGetPathFromIDListA
@ stdcall SHGetPathFromIDListA(ptr ptr)
@ stdcall SHGetPathFromIDListEx(ptr ptr long long)
@ stdcall SHGetPathFromIDListW(ptr ptr)
@ stdcall SHGetPropertyStoreForWindow(long ptr ptr)
@ stdcall SHGetPropertyStoreFromParsingName(wstr ptr long ptr ptr)
@ stdcall SHGetSettings(ptr long)
@ stdcall SHGetSpecialFolderLocation(long long ptr)
@ stdcall SHGetSpecialFolderPathA(long ptr long long)
@ stdcall SHGetSpecialFolderPathW(long ptr long long)
@ stdcall SHGetStockIconInfo(long long ptr)
@ stdcall SHHelpShortcuts_RunDLL(long long long long) SHHelpShortcuts_RunDLLA
@ stdcall SHHelpShortcuts_RunDLLA(long long long long)
@ stdcall SHHelpShortcuts_RunDLLW(long long long long)
@ stub SHInvokePrinterCommandA
@ stub SHInvokePrinterCommandW
@ stdcall SHIsFileAvailableOffline(wstr ptr)
@ stdcall SHLoadInProc(ptr)
@ stdcall SHLoadNonloadedIconOverlayIdentifiers()
@ stdcall SHMultiFileProperties(ptr long)
@ stdcall SHOpenFolderAndSelectItems(ptr long ptr long)
@ stdcall SHOpenWithDialog(long ptr)
@ stdcall SHParseDisplayName(wstr ptr ptr long ptr)
@ stdcall SHPathPrepareForWriteA(long ptr str long)
@ stdcall SHPathPrepareForWriteW(long ptr wstr long)
@ stdcall SHQueryRecycleBinA(str ptr)
@ stdcall SHQueryRecycleBinW(wstr ptr)
@ stdcall SHQueryUserNotificationState(ptr)
@ stdcall SHRemoveLocalizedName(wstr)
@ stdcall SHSetLocalizedName(wstr wstr long)
@ stdcall SHSetTemporaryPropertyForItem(ptr ptr ptr)
@ stdcall SHSetUnreadMailCountW(wstr long wstr)
@ stdcall SHUpdateRecycleBinIcon()
@ stdcall SheChangeDirA(str)
@ stub SheChangeDirExA
@ stub SheChangeDirExW
@ stdcall SheChangeDirW(wstr)
@ stub SheConvertPathW
@ stub SheFullPathA
@ stub SheFullPathW
@ stub SheGetCurDrive
@ stdcall SheGetDirA(long long)
@ stub SheGetDirExW
@ stdcall SheGetDirW (long long)
@ stub SheGetPathOffsetW
@ stub SheRemoveQuotesA
@ stub SheRemoveQuotesW
@ stub SheSetCurDrive
@ stub SheShortenPathA
@ stub SheShortenPathW
@ stdcall ShellAboutA(long str str long)
@ stdcall ShellAboutW(long wstr wstr long)
@ stdcall ShellExec_RunDLL(long long str long) ShellExec_RunDLLA
@ stdcall ShellExec_RunDLLA(long long str long)
@ stdcall ShellExec_RunDLLW(long long wstr long)
@ stdcall ShellExecuteA(long str str str str long)
@ stdcall ShellExecuteEx (long) ShellExecuteExA
@ stdcall ShellExecuteExA (long)
@ stdcall ShellExecuteExW (long)
@ stdcall ShellExecuteW (long wstr wstr wstr wstr long)
@ stdcall ShellHookProc(long long long)
@ stdcall Shell_GetCachedImageIndexA(str long long)
@ stdcall Shell_GetCachedImageIndexW(wstr long long)
@ stdcall Shell_NotifyIcon(long ptr) Shell_NotifyIconA
@ stdcall Shell_NotifyIconA(long ptr)
@ stdcall Shell_NotifyIconW(long ptr)
@ stdcall Shell_NotifyIconGetRect(ptr ptr)
@ stdcall -private StrChrA(str long) shlwapi.StrChrA
@ stdcall -private StrChrIA(str long) shlwapi.StrChrIA
@ stdcall -private StrChrIW(wstr long) shlwapi.StrChrIW
@ stdcall -private StrChrW(wstr long) shlwapi.StrChrW
@ stdcall -private StrCmpNA(str str long) shlwapi.StrCmpNA
@ stdcall -private StrCmpNIA(str str long) shlwapi.StrCmpNIA
@ stdcall -private StrCmpNIW(wstr wstr long) shlwapi.StrCmpNIW
@ stdcall -private StrCmpNW(wstr wstr long) shlwapi.StrCmpNW
@ stdcall -private StrCpyNA (ptr str long) kernel32.lstrcpynA
@ stdcall -private StrCpyNW(ptr wstr long) shlwapi.StrCpyNW
@ stdcall -private StrNCmpA(str str long) shlwapi.StrCmpNA
@ stdcall -private StrNCmpIA(str str long) shlwapi.StrCmpNIA
@ stdcall -private StrNCmpIW(wstr wstr long) shlwapi.StrCmpNIW
@ stdcall -private StrNCmpW(wstr wstr long) shlwapi.StrCmpNW
@ stdcall -private StrNCpyA (ptr str long) kernel32.lstrcpynA
@ stdcall -private StrNCpyW(ptr wstr long) shlwapi.StrCpyNW
@ stdcall -private StrRChrA(str str long) shlwapi.StrRChrA
@ stdcall -private StrRChrIA(str str long) shlwapi.StrRChrIA
@ stdcall -private StrRChrIW(wstr wstr long) shlwapi.StrRChrIW
@ stdcall -private StrRChrW(wstr wstr long) shlwapi.StrRChrW
@ stub -private StrRStrA
@ stdcall -private StrRStrIA(str str str) shlwapi.StrRStrIA
@ stdcall -private StrRStrIW(wstr wstr wstr) shlwapi.StrRStrIW
@ stub -private StrRStrW
@ stdcall -private StrStrA(str str) shlwapi.StrStrA
@ stdcall -private StrStrIA(str str) shlwapi.StrStrIA
@ stdcall -private StrStrIW(wstr wstr) shlwapi.StrStrIW
@ stdcall -private StrStrW(wstr wstr) shlwapi.StrStrW
@ stdcall WOWShellExecute(long str str str str long ptr)
