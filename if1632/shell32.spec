name	shell32
type	win32

# Functions exported by the Win95 shell32.dll 
# (these need to have these exact ordinals, for some win95 dlls 
#  import shell32.dll by ordinal)

   3 stub CheckEscapesA
   6 stub CheckEscapesW
   7 stdcall CommandLineToArgvW(ptr ptr) CommandLineToArgvW
   8 stub Control_FillCache_RunDLL
  12 stdcall Control_RunDLL(long long long long) Control_RunDLL
  14 stub DllGetClassObject
  22 stub DoEnvironmentSubstA
  33 stub SHELL32_33
  34 stub SHELL32_34
  35 stub SHELL32_35
  38 stub DoEnvironmentSubstW
  41 stdcall DragAcceptFiles(long long) DragAcceptFiles
  42 stub DragFinish
  44 stub DragQueryFile
  46 stub SHELL32_46
  47 stub SHELL32_47
  48 stub SHELL32_48
  49 stub SHELL32_49
  50 stub DragQueryFileA
  53 stub DragQueryFileAorW
  54 stub DragQueryFileW
  56 stub SHELL32_56
  57 stub SHELL32_57
  58 stub SHELL32_58
  62 stub SHELL32_62
  63 stub SHELL32_63
  64 stub SHELL32_64
  65 stub SHELL32_65
  76 stub DragQueryPoint
  80 stub DuplicateIcon
  82 stub ExtractAssociatedIconA
 101 stub ExtractAssociatedIconExA
 124 stub ExtractAssociatedIconExW
 125 stub ExtractAssociatedIconW
 133 stub ExtractIconA
 135 stub ExtractIconEx
 138 stub ExtractIconExA
 148 stub ExtractIconResInfoA
 150 stub ExtractIconResInfoW
 156 stub SHELL32_156
 157 stub SHELL32_157
 158 stub SHELL32_158
 159 stub SHELL32_159
 160 stub SHELL32_160
 180 stub ExtractIconW
 182 stub ExtractVersionResource16W
 184 stub SHELL32_184
 185 stub SHELL32_185
 186 stdcall FindExecutableA(ptr ptr ptr) FindExecutable32A
 187 stub FindExecutableW
 188 return FreeIconList 4 0
 189 stub InternalExtractIconListA
 190 stub InternalExtractIconListW
 191 stub OpenAs_RunDLL
 192 stub PrintersGetCommand_RunDLL
 193 stub RealShellExecuteA
 194 stub RealShellExecuteExA
 203 stub RealShellExecuteExW
 204 stub RealShellExecuteW
 205 stub RegenerateUserEnvironment
 206 stub SHAddToRecentDocs
 207 stub SHAppBarMessage
 208 stub SHBrowseForFolder
 209 stub SHBrowseForFolderA
 210 stub SHChangeNotify
 211 stub SHFileOperation
 212 stub SHFileOperationA
 213 stub SHFormatDrive
 214 stub SHFreeNameMappings
 215 stub SHGetDataFromIDListA
 216 stub SHGetDesktopFolder
 217 stdcall SHGetFileInfo(ptr long ptr long long) SHGetFileInfo32A
 218 stdcall SHGetFileInfoA(ptr long ptr long long) SHGetFileInfo32A
 219 stub SHGetInstanceExplorer
 220 stub SHGetMalloc
 221 stub SHGetPathFromIDList
 222 stub SHGetPathFromIDListA
 223 stub SHGetSpecialFolderLocation
 224 stub SHHelpShortcuts_RunDLL
 225 stub SHLoadInProc
 226 stub SheChangeDirA
 227 stub SheChangeDirExA
 228 stub SheChangeDirExW
 229 stub SheChangeDirW
 230 stub SheConvertPathW
 231 stub SheFullPathA
 232 stub SheFullPathW
 233 stub SheGetCurDrive
 234 stub SheGetDirA
 235 stub SheGetDirExW
 236 stub SheGetDirW
 237 stub SheGetPathOffsetW
 238 stub SheRemoveQuotesA
 239 stub SheRemoveQuotesW
 240 stub SheSetCurDrive
 241 stub SheShortenPathA
 242 stub SheShortenPathW
 243 stdcall ShellAboutA(long ptr ptr long) ShellAbout32A
 244 stdcall ShellAboutW(long ptr ptr long) ShellAbout32W
 245 stdcall ShellExecuteA(long ptr ptr ptr ptr long) ShellExecute32A
 246 stub ShellExecuteEx
 247 stub ShellExecuteExA
 248 stub ShellExecuteW
 249 stub Shell_NotifyIcon
 250 stub Shell_NotifyIconA
 251 stub Shl1632_ThunkData32
 252 stub Shl3216_ThunkData32
1025 stub ExtractIconExW # proper ordinal unknown
1030 stub FindExeDlgProc # proper ordinal unknown
1043 stub RegisterShellHook # proper ordinal unknown
1048 stub SHBrowseForFolderW # proper ordinal unknown
1052 stub SHFileOperationW # proper ordinal unknown
1058 stub SHGetFileInfoW # proper ordinal unknown
1063 stub SHGetPathFromIDListW # proper ordinal unknown
1089 stub ShellExecuteExW # proper ordinal unknown
1091 stub ShellHookProc # proper ordinal unknown
1094 stub Shell_NotifyIconW # proper ordinal unknown
1095 stub StrChrA # proper ordinal unknown
1096 stub StrChrIA # proper ordinal unknown
1097 stub StrChrIW # proper ordinal unknown
1098 stub StrChrW # proper ordinal unknown
1099 stub StrCmpNA # proper ordinal unknown
1100 stub StrCmpNIA # proper ordinal unknown
1101 stub StrCmpNIW # proper ordinal unknown
1102 stub StrCmpNW # proper ordinal unknown
1103 stub StrCpyNA # proper ordinal unknown
1104 stub StrCpyNW # proper ordinal unknown
1105 stub StrNCmpA # proper ordinal unknown
1106 stub StrNCmpIA # proper ordinal unknown
1107 stub StrNCmpIW # proper ordinal unknown
1108 stub StrNCmpW # proper ordinal unknown
1109 stub StrNCpyA # proper ordinal unknown
1110 stub StrNCpyW # proper ordinal unknown
1111 stub StrRChrA # proper ordinal unknown
1112 stub StrRChrIA # proper ordinal unknown
1113 stub StrRChrIW # proper ordinal unknown
1114 stub StrRChrW # proper ordinal unknown
1115 stub StrRStrA # proper ordinal unknown
1116 stub StrRStrIA # proper ordinal unknown
1117 stub StrRStrIW # proper ordinal unknown
1118 stub StrRStrW # proper ordinal unknown
1119 stub StrStrA # proper ordinal unknown
1120 stub StrStrIA # proper ordinal unknown
1121 stub StrStrIW # proper ordinal unknown
1122 stub StrStrW # proper ordinal unknown
1123 stub WOWShellExecute # proper ordinal unknown
