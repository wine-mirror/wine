name	shell32
type	win32
base	1

# Functions exported by the Win95 shell32.dll 
# (these need to have these exact ordinals, for some win95 dlls 
#  import shell32.dll by ordinal)

   1 stub CheckEscapesA
   4 stub CheckEscapesW
   5 stdcall CommandLineToArgvW(ptr ptr) CommandLineToArgvW
   6 stub Control_FillCache_RunDLL
  10 stub Control_RunDLL
  12 stub DllGetClassObject
  20 stub DoEnvironmentSubstA
  36 stub DoEnvironmentSubstW
  39 stdcall DragAcceptFiles(long long) DragAcceptFiles
  40 stub DragFinish
  42 stub DragQueryFile
  47 stub SHELL32_47
  48 stub DragQueryFileA
  51 stub DragQueryFileAorW
  52 stub DragQueryFileW
  74 stub DragQueryPoint
  78 stub DuplicateIcon
  80 stub ExtractAssociatedIconA
  99 stub ExtractAssociatedIconExA
 122 stub ExtractAssociatedIconExW
 123 stub ExtractAssociatedIconW
 131 stub ExtractIconA
 133 stub ExtractIconEx
 136 stub ExtractIconExA
 146 stub ExtractIconResInfoA
 148 stub ExtractIconResInfoW
 154 stub SHELL32_154
 155 stub SHELL32_155
 156 stub SHELL32_156
 158 stub SHELL32_158
 178 stub ExtractIconW
 180 stub ExtractVersionResource16W
 182 stub SHELL32_182
 183 stub SHELL32_183
 184 stdcall FindExecutableA(ptr ptr ptr) FindExecutable32A
 185 stub FindExecutableW
 186 return FreeIconList 4 0
 187 stub InternalExtractIconListA
 188 stub InternalExtractIconListW
 189 stub OpenAs_RunDLL
 190 stub PrintersGetCommand_RunDLL
 191 stub RealShellExecuteA
 192 stub RealShellExecuteExA
 201 stub RealShellExecuteExW
 202 stub RealShellExecuteW
 203 stub RegenerateUserEnvironment
 204 stub SHAddToRecentDocs
 205 stub SHAppBarMessage
 206 stub SHBrowseForFolder
 207 stub SHBrowseForFolderA
 208 stub SHChangeNotify
 209 stub SHFileOperation
 210 stub SHFileOperationA
 211 stub SHFormatDrive
 212 stub SHFreeNameMappings
 213 stub SHGetDataFromIDListA
 214 stub SHGetDesktopFolder
 215 stdcall SHGetFileInfo(ptr long ptr long long) SHGetFileInfo32A
 216 stdcall SHGetFileInfoA(ptr long ptr long long) SHGetFileInfo32A
 217 stub SHGetInstanceExplorer
 218 stub SHGetMalloc
 219 stub SHGetPathFromIDList
 220 stub SHGetPathFromIDListA
 221 stub SHGetSpecialFolderLocation
 222 stub SHHelpShortcuts_RunDLL
 223 stub SHLoadInProc
 224 stub SheChangeDirA
 225 stub SheChangeDirExA
 226 stub SheChangeDirExW
 227 stub SheChangeDirW
 228 stub SheConvertPathW
 229 stub SheFullPathA
 230 stub SheFullPathW
 231 stub SheGetCurDrive
 232 stub SheGetDirA
 233 stub SheGetDirExW
 234 stub SheGetDirW
 235 stub SheGetPathOffsetW
 236 stub SheRemoveQuotesA
 237 stub SheRemoveQuotesW
 238 stub SheSetCurDrive
 239 stub SheShortenPathA
 240 stub SheShortenPathW
 241 stdcall ShellAboutA(long ptr ptr long) ShellAbout32A
 242 stdcall ShellAboutW(long ptr ptr long) ShellAbout32W
 243 stdcall ShellExecuteA(long ptr ptr ptr ptr long) ShellExecute32A
 244 stub ShellExecuteEx
 245 stub ShellExecuteExA
 246 stub ShellExecuteW
 247 stub Shell_NotifyIcon
 248 stub Shell_NotifyIconA
 249 stub Shl1632_ThunkData32
 250 stub Shl3216_ThunkData32
1023 stub ExtractIconExW # proper ordinal unknown
1028 stub FindExeDlgProc # proper ordinal unknown
1041 stub RegisterShellHook # proper ordinal unknown
1046 stub SHBrowseForFolderW # proper ordinal unknown
1050 stub SHFileOperationW # proper ordinal unknown
1056 stub SHGetFileInfoW # proper ordinal unknown
1061 stub SHGetPathFromIDListW # proper ordinal unknown
1087 stub ShellExecuteExW # proper ordinal unknown
1089 stub ShellHookProc # proper ordinal unknown
1092 stub Shell_NotifyIconW # proper ordinal unknown
1093 stub StrChrA # proper ordinal unknown
1094 stub StrChrIA # proper ordinal unknown
1095 stub StrChrIW # proper ordinal unknown
1096 stub StrChrW # proper ordinal unknown
1097 stub StrCmpNA # proper ordinal unknown
1098 stub StrCmpNIA # proper ordinal unknown
1099 stub StrCmpNIW # proper ordinal unknown
1100 stub StrCmpNW # proper ordinal unknown
1101 stub StrCpyNA # proper ordinal unknown
1102 stub StrCpyNW # proper ordinal unknown
1103 stub StrNCmpA # proper ordinal unknown
1104 stub StrNCmpIA # proper ordinal unknown
1105 stub StrNCmpIW # proper ordinal unknown
1106 stub StrNCmpW # proper ordinal unknown
1107 stub StrNCpyA # proper ordinal unknown
1108 stub StrNCpyW # proper ordinal unknown
1109 stub StrRChrA # proper ordinal unknown
1110 stub StrRChrIA # proper ordinal unknown
1111 stub StrRChrIW # proper ordinal unknown
1112 stub StrRChrW # proper ordinal unknown
1113 stub StrRStrA # proper ordinal unknown
1114 stub StrRStrIA # proper ordinal unknown
1115 stub StrRStrIW # proper ordinal unknown
1116 stub StrRStrW # proper ordinal unknown
1117 stub StrStrA # proper ordinal unknown
1118 stub StrStrIA # proper ordinal unknown
1119 stub StrStrIW # proper ordinal unknown
1120 stub StrStrW # proper ordinal unknown
1121 stub WOWShellExecute # proper ordinal unknown
