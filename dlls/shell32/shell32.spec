name	shell32
type	win32
init	Shell32LibMain
rsrc	shres

# fixme: avoid this import
import ole32.dll

import shlwapi.dll
import comctl32.dll
import advapi32.dll
import user32.dll
import gdi32.dll
import kernel32.dll

# Functions exported by the Win95 shell32.dll 
# (these need to have these exact ordinals, for some 
#  win95 and winNT dlls import shell32.dll by ordinal)
# This list was updated to dll version 4.72

   2 stdcall SHChangeNotifyRegister(long long long long long long) SHChangeNotifyRegister
   3 stub CheckEscapesA@8
   4 stdcall SHChangeNotifyDeregister (long) SHChangeNotifyDeregister
   5 stub SHChangeNotifyUpdateEntryList@16
   6 stub CheckEscapesW@8
   7 stdcall CommandLineToArgvW(wstr ptr) CommandLineToArgvW
   9 stub PifMgr_OpenProperties@16
  10 stub PifMgr_GetProperties@20
  11 stub PifMgr_SetProperties@20
  13 stub PifMgr_CloseProperties@8
  15 stdcall ILGetDisplayName(ptr ptr) ILGetDisplayName
  16 stdcall ILFindLastID(ptr) ILFindLastID
  17 stdcall ILRemoveLastID(ptr) ILRemoveLastID
  18 stdcall ILClone(ptr) ILClone
  19 stdcall ILCloneFirst (ptr) ILCloneFirst
  20 stdcall ILGlobalClone (ptr) ILGlobalClone
  21 stdcall ILIsEqual (ptr ptr) ILIsEqual
  23 stdcall ILIsParent (long long long) ILIsParent
  24 stdcall ILFindChild (long long) ILFindChild
  25 stdcall ILCombine(ptr ptr) ILCombine
  26 stdcall ILLoadFromStream (long long) ILLoadFromStream
  27 stub ILSaveToStream@8
  28 stdcall SHILCreateFromPath (long long long) SHILCreateFromPathAW
  29 stdcall PathIsRoot(ptr) PathIsRootAW
  30 stdcall PathBuildRoot(ptr long) PathBuildRootAW
  31 stdcall PathFindExtension(ptr) PathFindExtensionAW
  32 stdcall PathAddBackslash(ptr) PathAddBackslashAW
  33 stdcall PathRemoveBlanks(ptr) PathRemoveBlanksAW
  34 stdcall PathFindFileName(ptr) PathFindFileNameAW
  35 stdcall PathRemoveFileSpec(ptr) PathRemoveFileSpecAW
  36 stdcall PathAppend(ptr ptr) PathAppendAW
  37 stdcall PathCombine(ptr ptr ptr) PathCombineAW
  38 stdcall PathStripPath(ptr)PathStripPathAW
  39 stdcall PathIsUNC (ptr) PathIsUNCAW
  40 stdcall PathIsRelative (ptr) PathIsRelativeAW
  43 stdcall PathIsExe (ptr) PathIsExeAW
  44 stub DoEnvironmentSubstA@8
  45 stdcall PathFileExists(ptr) PathFileExistsAW
  46 stdcall PathMatchSpec (ptr ptr) PathMatchSpecAW
  47 stdcall PathMakeUniqueName (ptr long ptr ptr ptr)PathMakeUniqueNameAW
  48 stdcall PathSetDlgItemPath (long long ptr) PathSetDlgItemPathAW
  49 stdcall PathQualify (ptr) PathQualifyAW
  50 stdcall PathStripToRoot (ptr) PathStripToRootAW
  51 stdcall PathResolve(str long long) PathResolveAW
  52 stdcall PathGetArgs(str) PathGetArgsAW
  53 stdcall DoEnvironmentSubst (long long) DoEnvironmentSubstAW
  54 stdcall DragAcceptFiles(long long) DragAcceptFiles
  55 stdcall PathQuoteSpaces (ptr) PathQuoteSpacesAW
  56 stdcall PathUnquoteSpaces(str) PathUnquoteSpacesAW
  57 stdcall PathGetDriveNumber (str) PathGetDriveNumberAW
  58 stdcall ParseField(str long ptr long) ParseFieldAW
  59 stub RestartDialog@12
  60 stdcall ExitWindowsDialog(long) ExitWindowsDialog
  61 stdcall RunFileDlg(long long long str str long) RunFileDlg
  62 stdcall PickIconDlg(long long long long) PickIconDlg
  63 stdcall GetFileNameFromBrowse(long long long long str str str) GetFileNameFromBrowse
  64 stdcall DriveType (long) DriveType
  65 stub InvalidateDriveType
  66 stub IsNetDrive
  67 stdcall Shell_MergeMenus (long long long long long long) Shell_MergeMenus
  68 stdcall SHGetSettings(ptr long long) SHGetSettings
  69 stub SHGetNetResource
  70 stdcall SHCreateDefClassObject(long long long long long)SHCreateDefClassObject
  71 stdcall Shell_GetImageList(ptr ptr) Shell_GetImageList
  72 stdcall Shell_GetCachedImageIndex(ptr ptr long) Shell_GetCachedImageIndexAW
  73 stdcall SHShellFolderView_Message(long long long) SHShellFolderView_Message 
  74 stub SHCreateStdEnumFmtEtc
  75 stdcall PathYetAnotherMakeUniqueName(ptr ptr ptr ptr) PathYetAnotherMakeUniqueNameA
  76 stub DragQueryInfo
  77 stdcall SHMapPIDLToSystemImageListIndex(long long long) SHMapPIDLToSystemImageListIndex
  78 stdcall OleStrToStrN(str long wstr long) OleStrToStrNAW
  79 stdcall StrToOleStrN(wstr long str long) StrToOleStrNAW
  80 stdcall DragFinish(long) DragFinish
  81 stdcall DragQueryFile(long long ptr long) DragQueryFileA
  82 stdcall DragQueryFileA(long long ptr long) DragQueryFileA
  83 stub CIDLData_CreateFromIDArray
  84 stub SHIsBadInterfacePtr
  85 forward OpenRegStream shlwapi.SHOpenRegStreamA
  86 stdcall SHRegisterDragDrop(long ptr) SHRegisterDragDrop
  87 stdcall SHRevokeDragDrop(long) SHRevokeDragDrop
  88 stdcall SHDoDragDrop(long long long long long) SHDoDragDrop
  89 stdcall SHCloneSpecialIDList(long long long) SHCloneSpecialIDList
  90 stub SHFindFiles
  91 stub SHFindComputer
  92 stdcall PathGetShortPath (ptr) PathGetShortPathAW
  93 stub Win32CreateDirectory
  94 stub Win32RemoveDirectory
  95 stdcall SHLogILFromFSIL (ptr) SHLogILFromFSIL
  96 stdcall StrRetToStrN (long long long long) StrRetToStrNAW
  97 stdcall SHWaitForFileToOpen (long long long) SHWaitForFileToOpen
  98 stdcall SHGetRealIDL (long long long) SHGetRealIDL
  99 stdcall SetAppStartingCursor (long long) SetAppStartingCursor
 100 stdcall SHRestricted(long) SHRestricted
 102 stdcall SHCoCreateInstance(ptr ptr long ptr ptr) SHCoCreateInstance
 103 stdcall SignalFileOpen(long) SignalFileOpen
 104 stdcall FileMenu_DeleteAllItems(long)FileMenu_DeleteAllItems
 105 stdcall FileMenu_DrawItem(long ptr)FileMenu_DrawItem
 106 stdcall FileMenu_FindSubMenuByPidl(long ptr)FileMenu_FindSubMenuByPidl
 107 stdcall FileMenu_GetLastSelectedItemPidls(long ptr ptr)FileMenu_GetLastSelectedItemPidls
 108 stdcall FileMenu_HandleMenuChar(long long)FileMenu_HandleMenuChar
 109 stdcall FileMenu_InitMenuPopup (long) FileMenu_InitMenuPopup
 110 stdcall FileMenu_InsertUsingPidl (long long ptr long long ptr) FileMenu_InsertUsingPidl
 111 stdcall FileMenu_Invalidate (long) FileMenu_Invalidate
 112 stdcall FileMenu_MeasureItem(long ptr)FileMenu_MeasureItem
 113 stdcall FileMenu_ReplaceUsingPidl (long long ptr long ptr) FileMenu_ReplaceUsingPidl
 114 stdcall FileMenu_Create (long long long long long) FileMenu_Create
 115 stdcall FileMenu_AppendItem (long ptr long long long long) FileMenu_AppendItemAW
 116 stdcall FileMenu_TrackPopupMenuEx (long long long long long long) FileMenu_TrackPopupMenuEx
 117 stdcall FileMenu_DeleteItemByCmd(long long)FileMenu_DeleteItemByCmd
 118 stdcall FileMenu_Destroy (long) FileMenu_Destroy
 119 stdcall IsLFNDrive(str) IsLFNDriveA
 120 stdcall FileMenu_AbortInitMenu () FileMenu_AbortInitMenu
 121 stdcall SHFlushClipboard () SHFlushClipboard
 122 stdcall RunDLL_CallEntry16 (long long long long long) RunDLL_CallEntry16 #name wrong?
 123 stdcall SHFreeUnusedLibraries () SHFreeUnusedLibraries
 124 stdcall FileMenu_AppendFilesForPidl(long ptr long)FileMenu_AppendFilesForPidl
 125 stdcall FileMenu_AddFilesForPidl(long long long ptr long long ptr)FileMenu_AddFilesForPidl
 126 stdcall SHOutOfMemoryMessageBox (long long long) SHOutOfMemoryMessageBox
 127 stdcall SHWinHelp (long long long long) SHWinHelp
 128 stdcall DllGetClassObject(long long ptr) SHELL32_DllGetClassObject
 129 stub DAD_AutoScroll
 130 stub DAD_DragEnter
 131 stub DAD_DragEnterEx
 132 stub DAD_DragLeave
 133 stdcall DragQueryFileW(long long ptr long) DragQueryFileW
 134 stub DAD_DragMove
 135 stdcall DragQueryPoint(long ptr) DragQueryPoint
 136 stdcall DAD_SetDragImage(long long) DAD_SetDragImage
 137 stdcall DAD_ShowDragImage (long) DAD_ShowDragImage
 139 stub Desktop_UpdateBriefcaseOnEvent
 140 stdcall FileMenu_DeleteItemByIndex(long long) FileMenu_DeleteItemByIndex
 141 stdcall FileMenu_DeleteItemByFirstID(long long)FileMenu_DeleteItemByFirstID
 142 stdcall FileMenu_DeleteSeparator(long)FileMenu_DeleteSeparator
 143 stdcall FileMenu_EnableItemByCmd(long long long)FileMenu_EnableItemByCmd
 144 stdcall FileMenu_GetItemExtent (long long) FileMenu_GetItemExtent
 145 stdcall PathFindOnPath (ptr ptr) PathFindOnPathAW
 146 stdcall RLBuildListOfPaths()RLBuildListOfPaths
 147 stdcall SHCLSIDFromString(long long) SHCLSIDFromStringAW
 149 stdcall SHFind_InitMenuPopup(long long long long) SHFind_InitMenuPopup
 151 stdcall SHLoadOLE (long) SHLoadOLE
 152 stdcall ILGetSize(ptr) ILGetSize
 153 stdcall ILGetNext(ptr) ILGetNext
 154 stdcall ILAppend (long long long) ILAppend
 155 stdcall ILFree (ptr) ILFree
 156 stdcall ILGlobalFree (ptr) ILGlobalFree
 157 stdcall ILCreateFromPath (ptr) ILCreateFromPathAW
 158 stdcall PathGetExtension(str) PathGetExtensionAW
 159 stdcall PathIsDirectory(ptr)PathIsDirectoryAW
 160 stub SHNetConnectionDialog
 161 stdcall SHRunControlPanel (long long) SHRunControlPanel
 162 stdcall SHSimpleIDListFromPath (ptr) SHSimpleIDListFromPathAW
 163 stdcall StrToOleStr (wstr str) StrToOleStrAW
 164 stdcall Win32DeleteFile(str) Win32DeleteFile
 165 stdcall SHCreateDirectory(long long) SHCreateDirectory
 166 stub CallCPLEntry16
 167 stub SHAddFromPropSheetExtArray
 168 stub SHCreatePropSheetExtArray
 169 stub SHDestroyPropSheetExtArray
 170 stub SHReplaceFromPropSheetExtArray
 171 stdcall PathCleanupSpec(ptr ptr) PathCleanupSpecAW
 172 stub SHCreateLinks
 173 stdcall SHValidateUNC(long long long)SHValidateUNC
 174 stdcall SHCreateShellFolderViewEx (ptr ptr) SHCreateShellFolderViewEx
 175 stdcall SHGetSpecialFolderPath(long long long long) SHGetSpecialFolderPathAW
 176 stdcall SHSetInstanceExplorer (long) SHSetInstanceExplorer
 177 stub DAD_SetDragImageFromListView
 178 stub SHObjectProperties
 179 stub SHGetNewLinkInfoA
 180 stub SHGetNewLinkInfoW
 181 stdcall RegisterShellHook(long long) RegisterShellHook
 182 varargs ShellMessageBoxW(long long long str long) ShellMessageBoxW
 183 varargs ShellMessageBoxA(long long long str long) ShellMessageBoxA
 184 stdcall ArrangeWindows(long long long long long) ArrangeWindows
 185 stub SHHandleDiskFull
 195 stdcall SHFree(ptr) SHFree
 196 stdcall SHAlloc(long) SHAlloc
 197 stub SHGlobalDefect
 198 stdcall SHAbortInvokeCommand () SHAbortInvokeCommand
 199 stub SHGetFileIcon
 200 stub SHLocalAlloc
 201 stub SHLocalFree
 202 stub SHLocalReAlloc
 203 stub AddCommasW
 204 stub ShortSizeFormatW
 205 stub Printer_LoadIconsW
 206 stub Link_AddExtraDataSection
 207 stub Link_ReadExtraDataSection
 208 stub Link_RemoveExtraDataSection
 209 stub Int64ToString
 210 stub LargeIntegerToString
 211 stub Printers_GetPidl
 212 stub Printer_AddPrinterPropPages
 213 stub Printers_RegisterWindowW
 214 stub Printers_UnregisterWindow
 215 stub SHStartNetConnectionDialog@12
 243 stdcall shell32_243(long long) shell32_243
 244 stdcall SHInitRestricted(ptr ptr) SHInitRestricted
 247 stdcall SHGetDataFromIDListA (ptr ptr long ptr long) SHGetDataFromIDListA
 248 stdcall SHGetDataFromIDListW (ptr ptr long ptr long) SHGetDataFromIDListW
 249 stdcall PathParseIconLocation (ptr) PathParseIconLocationAW
 250 stdcall PathRemoveExtension (ptr) PathRemoveExtensionAW
 251 stdcall PathRemoveArgs (ptr) PathRemoveArgsAW
 271 stub SheChangeDirA
 272 stub SheChangeDirExA
 273 stub SheChangeDirExW
 274 stdcall SheChangeDirW(wstr) SheChangeDirW
 275 stub SheConvertPathW
 276 stub SheFullPathA
 277 stub SheFullPathW
 278 stub SheGetCurDrive
 279 stub SheGetDirA@8
 280 stub SheGetDirExW@12
 281 stdcall SheGetDirW (long long) SheGetDirW
 282 stub SheGetPathOffsetW
 283 stub SheRemoveQuotesA
 284 stub SheRemoveQuotesW
 285 stub SheSetCurDrive
 286 stub SheShortenPathA
 287 stub SheShortenPathW
 288 stdcall ShellAboutA(long str str long) ShellAboutA
 289 stdcall ShellAboutW(long wstr wstr long) ShellAboutW
 290 stdcall ShellExecuteA(long str str str str long) ShellExecuteA
 291 stdcall ShellExecuteEx (long) ShellExecuteExAW
 292 stdcall ShellExecuteExA (long) ShellExecuteExA
 293 stdcall ShellExecuteExW (long) ShellExecuteExW
 294 stdcall ShellExecuteW (long wstr wstr wstr wstr long) ShellExecuteW
 296 stdcall Shell_NotifyIcon(long ptr) Shell_NotifyIcon
 297 stdcall Shell_NotifyIconA(long ptr) Shell_NotifyIconA
 298 stdcall Shell_NotifyIconW(long ptr) Shell_NotifyIconW
 299 stub Shl1632_ThunkData32
 300 stub Shl3216_ThunkData32
 301 stdcall StrChrA (str long) StrChrA   
 302 stub StrChrIA
 303 stub StrChrIW
 304 stdcall StrChrW (wstr long) StrChrW
 305 stdcall StrCmpNA(str str long) StrCmpNA 
 306 stdcall StrCmpNIA (str str long) StrCmpNIA
 307 stdcall StrCmpNIW (wstr wstr long) StrCmpNIW
 308 stdcall StrCmpNW (wstr wstr long) StrCmpNW
 309 stdcall StrCpyNA (ptr str long) lstrcpynA
 310 stdcall StrCpyNW (ptr wstr long)lstrcpynW
 311 stub StrNCmpA
 312 stub StrNCmpIA
 313 stub StrNCmpIW
 314 stub StrNCmpW
 315 stdcall StrNCpyA (ptr str long) lstrcpynA
 316 stdcall StrNCpyW (ptr wstr long)lstrcpynW
 317 stdcall StrRChrA (str str long) lstrrchr
 318 stub StrRChrIA
 319 stub StrRChrIW
 320 stdcall StrRChrW (wstr wstr long) lstrrchrw
 321 stub StrRStrA
 322 stub StrRStrIA
 323 stub StrRStrIW
 324 stub StrRStrW
 325 stdcall StrStrA(str str)StrStrA
 326 stdcall StrStrIA(str str)StrStrIA
 327 stdcall StrStrIW(wstr wstr)StrStrIW
 328 stdcall StrStrW(wstr wstr)StrStrW

 505 stdcall SHRegCloseKey (long) SHRegCloseKey
 506 stdcall SHRegOpenKeyA (long str long) SHRegOpenKeyA
 507 stdcall SHRegOpenKeyW (long wstr long) SHRegOpenKeyW
 508 stub SHRegQueryValueA@16
 509 stdcall SHRegQueryValueExA(long str ptr ptr ptr ptr) SHRegQueryValueExA
 510 stdcall SHRegQueryValueW (long long long long) SHRegQueryValueW
 511 stdcall SHRegQueryValueExW (long wstr ptr ptr ptr ptr) SHRegQueryValueExW
 512 stdcall SHRegDeleteKeyW (long wstr) SHRegDeleteKeyW

 520 stdcall SHAllocShared (long long long) SHAllocShared
 521 stdcall SHLockShared (long long) SHLockShared 
 522 stdcall SHUnlockShared (long) SHUnlockShared
 523 stdcall SHFreeShared (long long) SHFreeShared
 524 stub RealDriveType@8
 525 stub RealDriveTypeFlags@8

 640 stdcall NTSHChangeNotifyRegister (long long long long long long) NTSHChangeNotifyRegister
 641 stdcall NTSHChangeNotifyDeregister (long) NTSHChangeNotifyDeregister

 643 stub SHChangeNotifyReceive@16
 644 stub SHChangeNotification_Lock@16
 645 stub SHChangeNotification_Unlock@4
 646 stub SHChangeRegistrationReceive@8
 647 stub ReceiveAddToRecentDocs@8
 648 stub SHWaitOp_Operate@8

 650 stdcall PathIsSameRoot(ptr ptr)PathIsSameRootAW

# nt40/win98
 651 stdcall ReadCabinetState (long long) ReadCabinetState # OldReadCabinetState
 652 stdcall WriteCabinetState (long) WriteCabinetState
 653 stdcall PathProcessCommand (long long long long) PathProcessCommandAW

# win98
 654 stdcall shell32_654(long long)shell32_654 # ReadCabinetState@8
 660 stdcall FileIconInit(long)FileIconInit
 680 stdcall IsUserAdmin()IsUserAdmin

# >= NT5
 714 stdcall SHELL32_714(ptr)SHELL32_714 # PathIsTemporaryW

1217 stub FOOBAR1217   # no joke! This is the real name!!

#
# version 4.0 (win95)
# _WIN32_IE >= 0x0200
#
@ stdcall Control_FillCache_RunDLL(long long long long)Control_FillCache_RunDLL
@ stub Control_FillCache_RunDLLA@16
@ stub Control_FillCache_RunDLLW@16
@ stdcall Control_RunDLL(long long long long)Control_RunDLL
@ stub Control_RunDLLA@16
@ stub Control_RunDLLW@16
@ stdcall DllInstall(long wstr)SHELL32_DllInstall
@ stdcall DoEnvironmentSubstA(str str)DoEnvironmentSubstA
@ stdcall DoEnvironmentSubstW(wstr wstr)DoEnvironmentSubstW
@ stub DragQueryFileAorW
@ stub DuplicateIcon
@ stdcall ExtractAssociatedIconA(long ptr long)ExtractAssociatedIconA 
@ stub ExtractAssociatedIconExA 
@ stub ExtractAssociatedIconExW 
@ stub ExtractAssociatedIconW 
@ stdcall ExtractIconA(long str long)ExtractIconA 
@ stdcall ExtractIconEx(ptr long ptr ptr long)ExtractIconExAW
@ stdcall ExtractIconExA(str long ptr ptr long)ExtractIconExA
@ stdcall ExtractIconExW(wstr long ptr ptr long)ExtractIconExW
@ stdcall ExtractIconW(long wstr long)ExtractIconW 
@ stub ExtractIconResInfoA 
@ stub ExtractIconResInfoW 
@ stub ExtractVersionResource16W 
@ stub FindExeDlgProc 
@ stdcall FindExecutableA(ptr ptr ptr) FindExecutableA 
@ stdcall FindExecutableW(wstr wstr wstr) FindExecutableW 
@ stdcall FreeIconList(long) FreeIconList 
@ stub InternalExtractIconListA
@ stub InternalExtractIconListW
@ stub OpenAs_RunDLL
@ stub OpenAs_RunDLLA
@ stub OpenAs_RunDLLW
@ stub PrintersGetCommand_RunDLL
@ stub PrintersGetCommand_RunDLLA
@ stub PrintersGetCommand_RunDLLW
@ stub RealShellExecuteA 
@ stub RealShellExecuteExA 
@ stub RealShellExecuteExW 
@ stub RealShellExecuteW 
@ stub RegenerateUserEnvironment 
@ stdcall SHAddToRecentDocs (long ptr) SHAddToRecentDocs 
@ stdcall SHAppBarMessage(long ptr) SHAppBarMessage 
@ stdcall SHBrowseForFolder(ptr) SHBrowseForFolderA 
@ stdcall SHBrowseForFolderA(ptr) SHBrowseForFolderA 
@ stub SHBrowseForFolderW@4 
@ stdcall SHChangeNotify (long long ptr ptr) SHChangeNotifyAW 
@ stub ShellHookProc
@ stub SHEmptyRecycleBinA@12 
@ stub SHEmptyRecycleBinW@12 
@ stdcall SHFileOperation(ptr)SHFileOperationAW
@ stdcall SHFileOperationA(ptr)SHFileOperationA
@ stdcall SHFileOperationW(ptr)SHFileOperationW
@ stub SHFormatDrive@16 
@ stub SHFreeNameMappings@4 
@ stdcall SHGetDesktopFolder(ptr)SHGetDesktopFolder
@ stdcall SHGetFileInfo(ptr long ptr long long)SHGetFileInfoAW
@ stdcall SHGetFileInfoA(ptr long ptr long long)SHGetFileInfoA
@ stdcall SHGetFileInfoW(ptr long ptr long long)SHGetFileInfoW
@ stdcall SHGetInstanceExplorer(long)SHGetInstanceExplorer
@ stdcall SHGetMalloc(ptr)SHGetMalloc
@ stub SHGetNewLinkInfo@20
@ stdcall SHGetPathFromIDList(ptr ptr)SHGetPathFromIDListAW
@ stdcall SHGetPathFromIDListA(long long)SHGetPathFromIDListA
@ stdcall SHGetPathFromIDListW(long long)SHGetPathFromIDListW
@ stdcall SHGetSpecialFolderLocation(long long ptr)SHGetSpecialFolderLocation 
@ stdcall SHHelpShortcuts_RunDLL(long long long long) SHHelpShortcuts_RunDLL 
@ stub SHHelpShortcuts_RunDLLA@16 
@ stub SHHelpShortcuts_RunDLLW@16 
@ stdcall SHLoadInProc(long) SHLoadInProc 
@ stub SHQueryRecycleBinA@8 
@ stub SHQueryRecycleBinW@8 
@ stub SHUpdateRecycleBinIcon@0 
@ stub WOWShellExecute@28

#
# version 4.70 (IE3.0)
# _WIN32_IE >= 0x0300
#

#
# version 4.71 (IE4.0)
# _WIN32_IE >= 0x0400
#
@ stdcall DllCanUnloadNow() SHELL32_DllCanUnloadNow
@ stdcall DllGetVersion(ptr)SHELL32_DllGetVersion
@ stub SHGetFreeDiskSpace
@ stdcall SHGetSpecialFolderPathA(long ptr long long) SHGetSpecialFolderPathA

#
# version 4.72 (IE4.01)
# _WIN32_IE >= 0x0401
# no new exports
#

#
# version 5.00 (Win2K)
# _WIN32_IE >= 0x0500
#
@ stdcall SHGetFolderPathA(long long long long ptr)SHGetFolderPathA
@ stdcall SHGetFolderPathW(long long long long ptr)SHGetFolderPathW
@ stdcall SHGetFolderLocation(long long long long ptr)SHGetFolderLocation
