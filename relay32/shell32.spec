name	shell32
type	win32

# Functions exported by the Win95 shell32.dll 
# (these need to have these exact ordinals, for some 
#  win95 and winNT dlls import shell32.dll by ordinal)
# This list was updated to dll version 4.72

   2 stdcall SHChangeNotifyRegister(long long long long long long) SHChangeNotifyRegister
   3 stub CheckEscapesA@8   # exported by name
   4 stdcall SHChangeNotifyDeregister (long long) SHChangeNotifyDeregister
   5 stub SHChangeNotifyUpdateEntryList@16
   6 stub CheckEscapesW@8   # exported by name
   7 stdcall CommandLineToArgvW(wstr ptr) CommandLineToArgvW   # exported by name
   8 stub Control_FillCache_RunDLL@16   # exported by name
   9 stub PifMgr_OpenProperties@16
  10 stub PifMgr_GetProperties@20
  11 stub PifMgr_SetProperties@20
  12 stub Control_FillCache_RunDLLA   # exported by name
  13 stub PifMgr_CloseProperties@8
  14 stub Control_FillCache_RunDLLW@16   # exported by name
  15 stdcall ILGetDisplayName(ptr ptr) ILGetDisplayName
  16 stdcall ILFindLastID(ptr) ILFindLastID
  17 stub ILRemoveLastID@4
  18 stdcall ILClone(ptr) ILClone
  19 stub ILCloneFirst@4
  20 stub ILGlobalClone@4
  21 stub ILIsEqual@8
  22 stdcall Control_RunDLL(long long long long) Control_RunDLL # exported by name
  23 stub ILIsParent@12
  24 stub ILFindChild@8
  25 stdcall ILCombine(ptr ptr) ILCombine
  26 stub ILLoadFromStream@8
  27 stub ILSaveToStream@8
  28 stub SHILCreateFromPath@12
  29 stdcall PathIsRoot(str) PathIsRoot
  30 stdcall PathBuildRoot(ptr long) PathBuildRoot
  31 stdcall PathFindExtension(str) PathFindExtension
  32 stdcall PathAddBackslash(str) PathAddBackslash
  33 stdcall PathRemoveBlanks(str) PathRemoveBlanks
  34 stdcall PathFindFilename(str) PathFindFilename
  35 stdcall PathRemoveFileSpec(str) PathRemoveFileSpec
  36 stdcall PathAppend(str str) PathAppend
  37 stdcall PathCombine(ptr str str) PathCombine
  38 stub PathStripPath
  39 stdcall PathIsUNC(str) PathIsUNC
  40 stub PathIsRelative
  41 stub Control_RunDLLA   # exported by name
  42 stub Control_RunDLLW   # exported by name
  43 stub PathIsExe
  44 stub DoEnvironmentSubstA   # exported by name
  45 stdcall PathFileExists(str) PathFileExists
  46 stub PathMatchSpec
  47 stub PathMakeUniqueName
  48 stub PathSetDlgItemPath
  49 stub PathQualify
  50 stub PathStripToRoot
  51 stdcall PathResolve(str long long) PathResolve
  52 stdcall PathGetArgs(str) PathGetArgs
  53 stub DoEnvironmentSubstW   # exported by name
  54 stdcall DragAcceptFiles(long long) DragAcceptFiles   # exported by name
  55 stub PathQuoteSpaces
  56 stdcall PathUnquoteSpaces(str) PathUnquoteSpaces
  57 stub PathGetDriveNumber
  58 stdcall ParseField(str long str long) ParseField
  59 stub RestartDialog
  60 stdcall ExitWindowsDialog(long) ExitWindowsDialog
  61 stdcall RunFileDlg(long long long str str long) RunFileDlg
  62 stdcall PickIconDlg(long long long long) PickIconDlg
  63 stdcall GetFileNameFromBrowse(long long long long str str str) GetFileNameFromBrowse
  64 stub DriveType
  65 stub InvalidateDriveType
  66 stub IsNetDrive
  67 stub Shell_MergeMenus
  68 stdcall SHGetSettings(long long long) SHGetSettings
  69 stub SHGetNetResource
  70 stub SHCreateDefClassObject
  71 stdcall Shell_GetImageList(ptr ptr) Shell_GetImageList
  72 stdcall Shell_GetCachedImageIndex(ptr ptr long) Shell_GetCachedImageIndex
  73 stub SHShellFolderView_Message
  74 stub SHCreateStdEnumFmtEtc
  75 stdcall PathYetAnotherMakeUniqueName(ptr ptr) PathYetAnotherMakeUniqueName
  76 stub DragQueryInfo
  77 stdcall SHMapPIDLToSystemImageListIndex(long long long) SHMapPIDLToSystemImageListIndex
  78 stdcall OleStrToStrN(str long wstr long) OleStrToStrN
  79 stdcall StrToOleStrN(wstr long str long) StrToOleStrN
  80 stub DragFinish   # exported by name
  81 stub DragQueryFile   # exported by name
  82 stub DragQueryFileA   # exported by name
  83 stub CIDLData_CreateFromIDArray
  84 stub SHIsBadInterfacePtr
  85 stdcall OpenRegStream(long long long long) OpenRegStream
  86 stdcall SHRegisterDragDrop(long ptr) SHRegisterDragDrop
  87 stdcall SHRevokeDragDrop(long) SHRevokeDragDrop
  88 stub SHDoDragDrop
  89 stdcall SHCloneSpecialIDList(long long long) SHCloneSpecialIDList
  90 stub SHFindFiles
  91 stub SHFindComputer
  92 stub PathGetShortPath
  93 stub Win32CreateDirectory
  94 stub Win32RemoveDirectory
  95 stub SHLogILFromFSIL
  96 stub StrRetToStrN
  97 stub SHWaitForFileToOpen
  98 stub SHGetRealIDL
  99 stub SetAppStartingCursor
 100 stdcall SHRestricted(long) SHRestricted
 101 stub DragQueryFileAorW   # exported by name
 102 stdcall SHCoCreateInstance(ptr ptr long ptr ptr) SHCoCreateInstance
 103 stdcall SignalFileOpen(long) SignalFileOpen
 104 stub FileMenu_DeleteAllItems
 105 stub FileMenu_DrawItem
 106 stub FileMenu_FindSubMenuByPidl
 107 stub FileMenu_GetLastSelectedItemPidls
 108 stub FileMenu_HandleMenuChar
 109 stub FileMenu_InitMenuPopup
 110 stub FileMenu_InsertUsingPidl
 111 stub FileMenu_Invalidate
 112 stub FileMenu_MeasureItem
 113 stub FileMenu_ReplaceUsingPidl
 114 stub FileMenu_Create
 115 stub FileMenu_AppendItem
 116 stub FileMenu_TrackPopupMenuEx
 117 stub FileMenu_DeleteItemByCmd
 118 stub FileMenu_Destroy
 119 stdcall IsLFNDrive(ptr) IsLFNDrive
 120 stub FileMenu_AbortInitMenu
 121 stub SHFlushClipboard
 122 stub RunDLL_CallEntry16
 123 stub SHFreeUnusedLibraries
 124 stub FileMenu_AppendFilesForPidl
 125 stub FileMenu_AddFilesForPidl
 126 stub SHOutOfMemoryMessageBox
 127 stub SHWinHelp
 128 stdcall DllGetClassObject(long long ptr) SHELL32_DllGetClassObject
 129 stub DAD_AutoScroll
 130 stub DAD_DragEnter
 131 stub DAD_DragEnterEx
 132 stub DAD_DragLeave
 133 stub DragQueryFileW   # exported by name
 134 stub DAD_DragMove
 135 stub DragQueryPoint   # exported by name
 136 stub DAD_SetDragImage
 137 stub DAD_ShowDragImage
 138 stub DuplicateIcon   # exported by name
 139 stub Desktop_UpdateBriefcaseOnEvent
 140 stub FileMenu_DeleteItemByIndex
 141 stub FileMenu_DeleteItemByFirstID
 142 stub FileMenu_DeleteSeparator
 143 stub FileMenu_EnableItemByCmd
 144 stub FileMenu_GetItemExtent
 145 stub PathFindOnPath
 146 stub RLBuildListOfPaths
 147 stdcall SHCLSIDFromString(long long) SHCLSIDFromString
 148 stdcall ExtractAssociatedIconA(long ptr long) ExtractAssociatedIcon32A   # exported by name
 149 stub SHFind_InitMenuPopup
 150 stub ExtractAssociatedIconExA   # exported by name
 151 stub SHLoadOLE
 152 stdcall ILGetSize(ptr) ILGetSize
 153 stub ILGetNext
 154 stub ILAppend
 155 stdcall ILFree(ptr) ILFree
 156 stub ILGlobalFree
 157 stub ILCreateFromPath
 158 stdcall PathGetExtension(str long long) PathGetExtension
 159 stub PathIsDirectory
 160 stub SHNetConnectionDialog
 161 stub SHRunConrolPanel
 162 stub SHSimpleIDListFromPath
 163 stub StrToOleStr
 164 stub Win32DeleteFile
 165 stdcall SHCreateDirectory(long long) SHCreateDirectory
 166 stub CallCPLEntry16
 167 stub SHAddFromPropSheetExtArray
 168 stub SHCreatePropSheetExtArray
 169 stub SHDestroyPropSheetExtArray
 170 stub SHReplaceFromPropSheetExtArray
 171 stub PathCleanupSpec
 172 stub SHCreateLinks
 173 stub SHValidateUNC
 174 stdcall SHCreateShellFolderViewEx (ptr ptr) SHCreateShellFolderViewEx32
 175 stdcall SHGetSpecialFolderPath(long long long long) SHGetSpecialFolderPath
 176 stub SHSetInstanceExplorer
 177 stub DAD_SetDragImageFromListView
 178 stub SHObjectProperties
 179 stub SHGetNewLinkInfoA
 180 stub SHGetNewLinkInfoW
 181 stdcall RegisterShellHook(long long) RegisterShellHook32
 182 stub ShellMessageBoxW
 183 cdecl ShellMessageBoxA(long long long long long long) ShellMessageBoxA
 184 stdcall ArrangeWindows(long long long long long) ArrangeWindows
 185 stub SHHandleDiskFull
 186 stub ExtractAssociatedIconExW   # exported by name
 187 stub ExtractAssociatedIconW   # exported by name
 188 stdcall ExtractIconA(long str long) ExtractIcon32A   # exported by name
 189 stub ExtractIconEx   # exported by name
 190 stub ExtractIconExA   # exported by name
 191 stub ExtractIconExW   # exported by name
 192 stub ExtractIconResInfoA   # exported by name
 193 stub ExtractIconResInfoW   # exported by name
 194 stdcall ExtractIconW(long wstr long) ExtractIcon32W   # exported by name
 195 stdcall SHFree(ptr) SHFree
 196 stdcall SHAlloc(long) SHAlloc
 197 stub SHGlobalDefect
 198 stub SHAbortInvokeCommand
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
 216 stub ExtractVersionResource16W   # exported by name
 217 stub FindExeDlgProc   # exported by name
 218 stdcall FindExecutableA(ptr ptr ptr) FindExecutable32A   # exported by name
 219 stub FindExecutableW   # exported by name
 220 stdcall FreeIconList(long) FreeIconList   # exported by name
 221 stub InternalExtractIconListA   # exported by name
 222 stub InternalExtractIconListW   # exported by name
 223 stub OpenAs_RunDLL   # exported by name
 224 stub OpenAs_RunDLLA   # exported by name
 225 stub OpenAs_RunDLLW   # exported by name
 226 stub PrintersGetCommand_RunDLL   # exported by name
 227 stub PrintersGetCommand_RunDLLA   # exported by name
 228 stub PrintersGetCommand_RunDLLW   # exported by name
 229 stub RealShellExecuteA   # exported by name
 230 stub RealShellExecuteExA   # exported by name
 231 stub RealShellExecuteExW   # exported by name
 232 stub RealShellExecuteW   # exported by name
 233 stub RegenerateUserEnvironment@8   # exported by name
 234 stdcall SHAddToRecentDocs (long ptr) SHAddToRecentDocs32  # exported by name
 235 stdcall SHAppBarMessage(long ptr) SHAppBarMessage32   # exported by name
 236 stdcall SHBrowseForFolder(ptr) SHBrowseForFolder32A   # exported by name
 237 stdcall SHBrowseForFolderA(ptr) SHBrowseForFolder32A   # exported by name
 238 stub SHBrowseForFolderW@4   # exported by name
 239 stdcall SHChangeNotify (long long ptr ptr) SHChangeNotify32  # exported by name
 240 stub SHEmptyRecycleBinA@12   # exported by name
 241 stub SHEmptyRecycleBinW@12   # exported by name
 242 stdcall SHFileOperation(ptr) SHFileOperation32   # exported by name
 243 stub SHFileOperationA@4   # exported by name
 244 stub SHFileOperationW@4   # exported by name
 245 stub SHFormatDrive@16   # exported by name
 246 stub SHFreeNameMappings@4   # exported by name
 247 stub SHGetDataFromIDListA@20   # exported by name
 248 stub SHGetDataFromIDListW@20   # exported by name
 249 stub PathParseIconLocation@4
 250 stub PathRemoveExtension@4
 251 stub PathRemoveArgs@4
 252 stdcall SHGetDesktopFolder(ptr) SHGetDesktopFolder   # exported by name
 253 stdcall SHGetFileInfo(ptr long ptr long long) SHGetFileInfo32A   # exported by name
 254 stdcall SHGetFileInfoA(ptr long ptr long long) SHGetFileInfo32A   # exported by name
 255 stub SHGetFileInfoW@20 # exported by name
 256 stub SHGetInstanceExplorer   # exported by name
 257 stdcall SHGetMalloc(ptr) SHGetMalloc   # exported by name
 258 stub SHGetNewLinkInfo@20   # exported by name
 259 stdcall SHGetPathFromIDList(ptr ptr) SHGetPathFromIDList   # exported by name
 260 stub SHGetPathFromIDList@8 # exported by name
 261 stdcall SHGetPathFromIDListA (long long) SHGetPathFromIDList32A # exported by name
 262 stdcall SHGetPathFromIDListW (long long) SHGetPathFromIDList32W # exported by name
 263 stdcall SHGetSpecialFolderLocation(long long ptr) SHGetSpecialFolderLocation   # exported by name
 264 stdcall SHHelpShortcuts_RunDLL(long long long long) SHHelpShortcuts_RunDLL   # exported by name
 265 stub SHHelpShortcuts_RunDLLA@16   # exported by name
 266 stub SHHelpShortcuts_RunDLLW@16   # exported by name
 267 stdcall SHLoadInProc(long) SHLoadInProc   # exported by name
 268 stub SHQueryRecycleBinA@8   # exported by name
 269 stub SHQueryRecycleBinW@8   # exported by name
 270 stub SHUpdateRecycleBinIcon@0   # exported by name

 271 stub SheChangeDirA
 272 stub SheChangeDirExA
 273 stub SheChangeDirExW
 274 stub SheChangeDirW
 275 stub SheConvertPathW
 276 stub SheFullPathA
 277 stub SheFullPathW
 278 stub SheGetCurDrive
 279 stub SheGetDirA
 280 stub SheGetDirExW
 281 stub SheGetDirW
 282 stub SheGetPathOffsetW
 283 stub SheRemoveQuotesA
 284 stub SheRemoveQuotesW
 285 stub SheSetCurDrive
 286 stub SheShortenPathA
 287 stub SheShortenPathW
 288 stdcall ShellAboutA(long str str long) ShellAbout32A
 289 stdcall ShellAboutW(long wstr wstr long) ShellAbout32W
 290 stdcall ShellExecuteA(long str str str str long) ShellExecute32A
 291 stub ShellExecuteEx
 292 stub ShellExecuteExA
 293 stub ShellExecuteExW
 294 stub ShellExecuteW
 295 stub ShellHookProc   # exported by name
 296 stdcall Shell_NotifyIcon(long ptr) Shell_NotifyIcon
 297 stdcall Shell_NotifyIconA(long ptr) Shell_NotifyIconA
 298 stub Shell_NotifyIconW   # exported by name
 299 stub Shl1632_ThunkData32
 300 stub Shl3216_ThunkData32
 301 stub StrChrA # proper ordinal unknown
 302 stub StrChrIA # proper ordinal unknown
 303 stub StrChrIW # proper ordinal unknown
 304 stub StrChrW # proper ordinal unknown
 305 stub StrCmpNA # proper ordinal unknown
 306 stub StrCmpNIA # proper ordinal unknown
 307 stub StrCmpNIW # proper ordinal unknown
 308 stub StrCmpNW # proper ordinal unknown
 309 stub StrCpyNA # proper ordinal unknown
 310 stub StrCpyNW # proper ordinal unknown
 311 stub StrNCmpA # proper ordinal unknown
 312 stub StrNCmpIA # proper ordinal unknown
 313 stub StrNCmpIW # proper ordinal unknown
 314 stub StrNCmpW # proper ordinal unknown
 315 stub StrNCpyA # proper ordinal unknown
 316 stub StrNCpyW # proper ordinal unknown
 317 stub StrRChrA # proper ordinal unknown
 318 stub StrRChrIA # proper ordinal unknown
 319 stub StrRChrIW # proper ordinal unknown
 320 stub StrRChrW # proper ordinal unknown
 321 stub StrRStrA # proper ordinal unknown
 322 stub StrRStrIA # proper ordinal unknown
 323 stub StrRStrIW # proper ordinal unknown
 324 stub StrRStrW # proper ordinal unknown
 325 stub StrStrA # proper ordinal unknown
 326 stub StrStrIA # proper ordinal unknown
 327 stub StrStrIW # proper ordinal unknown
 328 stub StrStrW # proper ordinal unknown 
 329 stub WOWShellExecute # proper ordinal unknown

 505 stub SHRegCloseKey@4
 506 stub SHRegOpenKeyA@12
 507 stub SHRegOpenKeyW@12
 508 stub SHRegQueryValueA@16
 509 stub SHRegQueryValueExA@24
 510 stub SHRegQueryValueW@16
 511 stub SHRegQueryValueExW@24
 512 stub SHRegDeleteKeyW@8

 520 stub SHAllocShared@12
 521 stub SHLockShared@8
 522 stub SHUnlockShared@4
 523 stub SHFreeShared@8
 524 stub RealDriveType@8
 525 stub RealDriveTypeFlags@8

 640 stub NTSHChangeNotifyRegister@24
 641 stub NTSHChangeNotifyDeregister@4

 643 stub SHChangeNotifyReceive@16
 644 stub SHChangeNotification_Lock@16
 645 stub SHChangeNotification_Unlock@4
 646 stub SHChangeRegistrationReceive@8
 647 stub ReceiveAddToRecentDocs
 648 stub SHWaitOp_Operate@8

 650 stub PathIsSameRoot
 651 stub ReadCabinetState
 652 stub WriteCabinetState@4
 653 stub PathProcessCommand

 660 stub FileIconInit

 680 stub IsUserAdmin

1217 stub FOOBAR1217   # no joke! This is the real name!!
