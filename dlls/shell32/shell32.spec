name	shell32
type	win32
init	Shell32LibMain

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
   8 stdcall Control_FillCache_RunDLL (long long long long) Control_FillCache_RunDLL
   9 stub PifMgr_OpenProperties@16
  10 stub PifMgr_GetProperties@20
  11 stub PifMgr_SetProperties@20
  12 stub Control_FillCache_RunDLLA@16
  13 stub PifMgr_CloseProperties@8
  14 stub Control_FillCache_RunDLLW@16
  15 stdcall ILGetDisplayName(ptr ptr) ILGetDisplayName
  16 stdcall ILFindLastID(ptr) ILFindLastID
  17 stdcall ILRemoveLastID(ptr) ILRemoveLastID
  18 stdcall ILClone(ptr) ILClone
  19 stdcall ILCloneFirst (ptr) ILCloneFirst
  20 stdcall ILGlobalClone (ptr) ILGlobalClone
  21 stdcall ILIsEqual (ptr ptr) ILIsEqual
  22 stdcall Control_RunDLL(long long long long) Control_RunDLL # exported by name
  23 stdcall ILIsParent (long long long) ILIsParent
  24 stdcall ILFindChild (long long) ILFindChild
  25 stdcall ILCombine(ptr ptr) ILCombine
  26 stdcall ILLoadFromStream (long long) ILLoadFromStream
  27 stub ILSaveToStream@8
  28 stdcall SHILCreateFromPath (long long long) SHILCreateFromPathAW
  29 stdcall PathIsRoot(ptr) PathIsRootAW
  30 stdcall PathBuildRoot(ptr long) PathBuildRootA
  31 stdcall PathFindExtension(ptr) PathFindExtensionAW
  32 stdcall PathAddBackslash(ptr) PathAddBackslashAW
  33 stdcall PathRemoveBlanks(str) PathRemoveBlanksAW
  34 stdcall PathFindFilename(ptr) PathFindFilenameAW
  35 stdcall PathRemoveFileSpec(str) PathRemoveFileSpecA
  36 stdcall PathAppend(str str) PathAppendA
  37 stdcall PathCombine(ptr ptr ptr) PathCombineAW
  38 stub PathStripPath
  39 stdcall PathIsUNC (ptr) PathIsUNCAW
  40 stdcall PathIsRelative (ptr) PathIsRelativeAW
  41 stub Control_RunDLLA@16
  42 stub Control_RunDLLW@16
  43 stdcall PathIsExe (ptr) PathIsExeAW
  44 stub DoEnvironmentSubstA@8
  45 stdcall PathFileExists(str) PathFileExistsA
  46 stdcall PathMatchSpec (str str) PathMatchSpecAW
  47 stub PathMakeUniqueName@20
  48 stdcall PathSetDlgItemPath (long long ptr) PathSetDlgItemPathAW
  49 stdcall PathQualify (ptr) PathQualifyAW
  50 stub PathStripToRoot@4
  51 stdcall PathResolve(str long long) PathResolve
  52 stdcall PathGetArgs(str) PathGetArgsAW
  53 stdcall DoEnvironmentSubst (long long) DoEnvironmentSubstAW
  54 stdcall DragAcceptFiles(long long) DragAcceptFiles
  55 stdcall PathQuoteSpaces (ptr) PathQuoteSpacesAW
  56 stdcall PathUnquoteSpaces(str) PathUnquoteSpacesAW
  57 stdcall PathGetDriveNumber (str) PathGetDriveNumber
  58 stdcall ParseField(str long ptr long) ParseFieldA
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
  72 stdcall Shell_GetCachedImageIndex(ptr ptr long) Shell_GetCachedImageIndexA # ASCII!!!
  73 stdcall SHShellFolderView_Message(long long long) SHShellFolderView_Message 
  74 stub SHCreateStdEnumFmtEtc
  75 stdcall PathYetAnotherMakeUniqueName(ptr ptr) PathYetAnotherMakeUniqueNameA
  76 stub DragQueryInfo
  77 stdcall SHMapPIDLToSystemImageListIndex(long long long) SHMapPIDLToSystemImageListIndex
  78 stdcall OleStrToStrN(str long wstr long) OleStrToStrNAW
  79 stdcall StrToOleStrN(wstr long str long) StrToOleStrNAW
  80 stdcall DragFinish(long) DragFinish
  81 stdcall DragQueryFile(long long ptr long) DragQueryFileA
  82 stdcall DragQueryFileA(long long ptr long) DragQueryFileA
  83 stub CIDLData_CreateFromIDArray
  84 stub SHIsBadInterfacePtr
  85 stdcall OpenRegStream(long long long long) OpenRegStream
  86 stdcall SHRegisterDragDrop(long ptr) SHRegisterDragDrop
  87 stdcall SHRevokeDragDrop(long) SHRevokeDragDrop
  88 stdcall SHDoDragDrop(long long long long long long) SHDoDragDrop
  89 stdcall SHCloneSpecialIDList(long long long) SHCloneSpecialIDList
  90 stub SHFindFiles
  91 stub SHFindComputer
  92 stub PathGetShortPath
  93 stub Win32CreateDirectory
  94 stub Win32RemoveDirectory
  95 stdcall SHLogILFromFSIL (ptr) SHLogILFromFSIL
  96 stdcall StrRetToStrN (long long long long) StrRetToStrNAW
  97 stdcall SHWaitForFileToOpen (long long long) SHWaitForFileToOpen
  98 stdcall SHGetRealIDL (long long long) SHGetRealIDL
  99 stdcall SetAppStartingCursor (long long) SetAppStartingCursor
 100 stdcall SHRestricted(long) SHRestricted
 101 stub DragQueryFileAorW   # exported by name
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
 138 stub DuplicateIcon   # exported by name
 139 stub Desktop_UpdateBriefcaseOnEvent
 140 stdcall FileMenu_DeleteItemByIndex(long long) FileMenu_DeleteItemByIndex
 141 stdcall FileMenu_DeleteItemByFirstID(long long)FileMenu_DeleteItemByFirstID
 142 stdcall FileMenu_DeleteSeparator(long)FileMenu_DeleteSeparator
 143 stdcall FileMenu_EnableItemByCmd(long long long)FileMenu_EnableItemByCmd
 144 stdcall FileMenu_GetItemExtent (long long) FileMenu_GetItemExtent
 145 stdcall PathFindOnPath (ptr ptr) PathFindOnPathAW
 146 stdcall RLBuildListOfPaths()RLBuildListOfPaths
 147 stdcall SHCLSIDFromString(long long) SHCLSIDFromStringAW
 148 stdcall ExtractAssociatedIconA(long ptr long) ExtractAssociatedIconA   # exported by name
 149 stdcall SHFind_InitMenuPopup(long long long long) SHFind_InitMenuPopup
 150 stub ExtractAssociatedIconExA   # exported by name
 151 stdcall SHLoadOLE (long) SHLoadOLE
 152 stdcall ILGetSize(ptr) ILGetSize
 153 stdcall ILGetNext(ptr) ILGetNext
 154 stdcall ILAppend (long long long) ILAppend
 155 stdcall ILFree (ptr) ILFree
 156 stdcall ILGlobalFree (ptr) ILGlobalFree
 157 stdcall ILCreateFromPath (ptr) ILCreateFromPathAW
 158 stdcall PathGetExtension(str long long) PathGetExtensionAW
 159 stub PathIsDirectory
 160 stub SHNetConnectionDialog
 161 stdcall SHRunControlPanel (long long) SHRunControlPanel
 162 stdcall SHSimpleIDListFromPath (ptr) SHSimpleIDListFromPathAW
 163 stdcall StrToOleStr (wstr str) StrToOleStrAW
 164 stub Win32DeleteFile
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
 182 cdecl ShellMessageBoxW(long long long str long long) ShellMessageBoxW
 183 cdecl ShellMessageBoxA(long long long str long long) ShellMessageBoxA
 184 stdcall ArrangeWindows(long long long long long) ArrangeWindows
 185 stub SHHandleDiskFull
 186 stub ExtractAssociatedIconExW   # exported by name
 187 stub ExtractAssociatedIconW   # exported by name
 188 stdcall ExtractIconA(long str long) ExtractIconA   # exported by name
 189 stdcall ExtractIconEx(ptr long ptr ptr long) ExtractIconExAW
 190 stdcall ExtractIconExA(str long ptr ptr long) ExtractIconExA
 191 stdcall ExtractIconExW(wstr long ptr ptr long) ExtractIconExW
 192 stub ExtractIconResInfoA   # exported by name
 193 stub ExtractIconResInfoW   # exported by name
 194 stdcall ExtractIconW(long wstr long) ExtractIconW   # exported by name
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
 216 stub ExtractVersionResource16W   # exported by name
 217 stub FindExeDlgProc   # exported by name
 218 stdcall FindExecutableA(ptr ptr ptr) FindExecutableA   # exported by name
 219 stdcall FindExecutableW(wstr wstr wstr) FindExecutableW   # exported by name
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
 233 stub RegenerateUserEnvironment   # exported by name
 234 stdcall SHAddToRecentDocs (long ptr) SHAddToRecentDocs  # exported by name
 235 stdcall SHAppBarMessage(long ptr) SHAppBarMessage   # exported by name
 236 stdcall SHBrowseForFolder(ptr) SHBrowseForFolderA   # exported by name
 237 stdcall SHBrowseForFolderA(ptr) SHBrowseForFolderA   # exported by name
 238 stub SHBrowseForFolderW@4   # exported by name
 239 stdcall SHChangeNotify (long long ptr ptr) SHChangeNotify  # exported by name
 240 stub SHEmptyRecycleBinA@12   # exported by name
 241 stub SHEmptyRecycleBinW@12   # exported by name
 242 stdcall SHFileOperation (ptr) SHFileOperationAW   # exported by name
 243 stdcall shell32_243(long long) shell32_243
 244 stdcall SHInitRestricted(ptr ptr) SHInitRestricted # win98+ only, by ordinal
 245 stub SHFormatDrive@16   # exported by name
 246 stub SHFreeNameMappings@4   # exported by name
 247 stdcall SHGetDataFromIDListA (ptr ptr long ptr long) SHGetDataFromIDListA
 248 stdcall SHGetDataFromIDListW (ptr ptr long ptr long) SHGetDataFromIDListW
 249 stub PathParseIconLocation@4
 250 stub PathRemoveExtension@4
 251 stub PathRemoveArgs@4
 252 stdcall SHGetDesktopFolder(ptr) SHGetDesktopFolder   # exported by name
 253 stdcall SHGetFileInfo(ptr long ptr long long) SHGetFileInfoA   # exported by name
 254 stdcall SHGetFileInfoA(ptr long ptr long long) SHGetFileInfoA   # exported by name
 255 stdcall SHGetFileInfoW(ptr long ptr long long) SHGetFileInfoW # exported by name
 256 stdcall SHGetInstanceExplorer (long) SHGetInstanceExplorer
 257 stdcall SHGetMalloc(ptr) SHGetMalloc   # exported by name
 258 stub SHGetNewLinkInfo@20   # exported by name
 259 stdcall SHGetPathFromIDList(ptr ptr) SHGetPathFromIDListAW   # exported by name
 260 stub SHGetPathFromIDList@8 # exported by name
 261 stdcall SHGetPathFromIDListA (long long) SHGetPathFromIDListA # exported by name
 262 stdcall SHGetPathFromIDListW (long long) SHGetPathFromIDListW # exported by name
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
 295 stub ShellHookProc   # exported by name
 296 stdcall Shell_NotifyIcon(long ptr) Shell_NotifyIcon
 297 stdcall Shell_NotifyIconA(long ptr) Shell_NotifyIconA
 298 stub Shell_NotifyIconW   # exported by name
 299 stub Shl1632_ThunkData32
 300 stub Shl3216_ThunkData32
 301 stub StrChrA # proper ordinal unknown
 302 stub StrChrIA # proper ordinal unknown
 303 stub StrChrIW # proper ordinal unknown
 304 stdcall StrChrW (ptr ptr) StrChrW # proper ordinal unknown
 305 stub StrCmpNA # proper ordinal unknown
 306 stub StrCmpNIA # proper ordinal unknown
 307 stdcall StrCmpNIW (wstr wstr long) StrCmpNIW  # proper ordinal unknown
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
 320 stdcall StrRChrW (wstr wstr long) StrRChrW # proper ordinal unknown
 321 stub StrRStrA # proper ordinal unknown
 322 stub StrRStrIA # proper ordinal unknown
 323 stub StrRStrIW # proper ordinal unknown
 324 stub StrRStrW # proper ordinal unknown
 325 stub StrStrA # proper ordinal unknown
 326 stub StrStrIA # proper ordinal unknown
 327 stub StrStrIW # proper ordinal unknown
 328 stub StrStrW # proper ordinal unknown 
 329 stub WOWShellExecute # proper ordinal unknown

 505 stdcall SHRegCloseKey (long) SHRegCloseKey
 506 stdcall SHRegOpenKeyA (long str long) SHRegOpenKeyA
 507 stdcall SHRegOpenKeyW (long wstr long) SHRegOpenKeyW
 508 stub SHRegQueryValueA@16
 509 stdcall SHRegQueryValueExA(long str ptr ptr ptr ptr) SHRegQueryValueExA
 510 stdcall SHRegQueryValueW (long long long long) SHRegQueryValueW
 511 stdcall SHRegQueryValueExW (long wstr ptr ptr ptr ptr) SHRegQueryValueExW
 512 stub SHRegDeleteKeyW@8

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

 650 stub PathIsSameRoot@8

# nt40/win98
 651 stdcall ReadCabinetState (long long) ReadCabinetState 
 652 stdcall WriteCabinetState (long) WriteCabinetState
 653 stdcall PathProcessCommand (long long long long) PathProcessCommandAW

# win98
 654 stdcall shell32_654 (long long) shell32_654

 660 stdcall FileIconInit (long) FileIconInit

 680 stdcall IsUserAdmin () IsUserAdmin

1217 stub FOOBAR1217   # no joke! This is the real name!!

# later additions ... FIXME: incorrect ordinals
# win 98 uses 2...330, 505..511, 520..526, 640..654, 660, 680, 700..707, 711


1220 stdcall DllGetVersion (ptr) SHELL32_DllGetVersion	# win98:201
1221 stdcall SHGetSpecialFolderPathA(long ptr long long) SHGetSpecialFolderPathA	# win98:292
1222 stdcall DoEnvironmentSubstA (str str) DoEnvironmentSubstA		# win98:293
1223 stdcall DoEnvironmentSubstW (wstr wstr) DoEnvironmentSubstW	# win98:204

# by-name routines relocated in win98

1224 stdcall SHFileOperationA (ptr) SHFileOperationA # exported by name
1225 stdcall SHFileOperationW (ptr) SHFileOperationW # exported by name