name	shell32
type	win32

# Functions exported by the Win95 shell32.dll 
# (these need to have these exact ordinals, for some win95 dlls 
#  import shell32.dll by ordinal)

   2 stdcall SHELL32_2(long long long long long long) SHELL32_2
   3 stub CheckEscapesA
   4 stub SHELL32_4
   5 stub SHELL32_5
   6 stub CheckEscapesW
   7 stdcall CommandLineToArgvW(wstr ptr) CommandLineToArgvW
   8 stub Control_FillCache_RunDLL
  12 stdcall Control_RunDLL(long long long long) Control_RunDLL
  14 stdcall DllGetClassObject(long long ptr) SHELL32_DllGetClassObject
  15 stub SHELL32_15
  16 stdcall SHELL32_16(ptr) SHELL32_16
  17 stub SHELL32_17
  18 stdcall SHELL32_18(ptr) SHELL32_18
  19 stub SHELL32_19
  20 stub SHELL32_20
  21 stub SHELL32_21
  22 stub DoEnvironmentSubstA
  23 stub SHELL32_23
  24 stub SHELL32_24
  25 stdcall SHELL32_25(ptr ptr) SHELL32_25
  26 stub SHELL32_26
  27 stub SHELL32_27
  28 stub SHELL32_28
  29 stdcall SHELL32_29(str) SHELL32_29
  30 stdcall SHELL32_30(ptr long) SHELL32_30
  31 stdcall SHELL32_31(str) SHELL32_31
  32 stdcall SHELL32_32(str) SHELL32_32
  33 stdcall SHELL32_33(str) SHELL32_33
  34 stdcall SHELL32_34(str) SHELL32_34
  35 stdcall SHELL32_35(str) SHELL32_35
  36 stdcall SHELL32_36(str str) SHELL32_36
  37 stdcall SHELL32_37(ptr str str) SHELL32_37
  38 stub DoEnvironmentSubstW
  39 stub SHELL32_39
  40 stub SHELL32_40
  41 stdcall DragAcceptFiles(long long) DragAcceptFiles
  42 stub DragFinish
  43 stub SHELL32_43
  44 stub DragQueryFile
  45 stdcall SHELL32_45(str) SHELL32_45
  46 stub SHELL32_46
  47 stub SHELL32_47
  48 stub SHELL32_48
  49 stub SHELL32_49
  50 stub DragQueryFileA
  51 stdcall SHELL32_51(str long long) SHELL32_51
  52 stdcall SHELL32_52(str) SHELL32_52
  53 stub DragQueryFileAorW
  54 stub DragQueryFileW
  55 stub SHELL32_55
  56 stdcall SHELL32_56(str) SHELL32_56
  57 stub SHELL32_57
  58 stdcall SHELL32_58(long long long long) SHELL32_58
  59 stub SHELL32_59
  60 stub SHELL32_60
  61 stub SHELL32_61
  62 stdcall SHELL32_62(long long long long) SHELL32_62
  63 stdcall SHELL32_63(long long long long str str str) SHELL32_63
  64 stub SHELL32_64
  65 stub SHELL32_65
  66 stub SHELL32_66
  67 stub SHELL32_67
  68 stdcall SHELL32_68(long long long) SHELL32_68
  69 stub SHELL32_69
  70 stub SHELL32_70
  71 stdcall SHELL32_71(ptr ptr) SHELL32_71
  72 stdcall SHELL32_72(ptr ptr long) SHELL32_72
  73 stub SHELL32_73
  74 stub SHELL32_74
  75 stub SHELL32_75
  76 stub DragQueryPoint
  77 stdcall SHELL32_77(long long long) SHELL32_77
  78 stub SHELL32_78
  79 stdcall SHELL32_79(str ptr) SHELL32_79
  80 stub DuplicateIcon
  81 stub SHELL32_81
  82 stdcall ExtractAssociatedIconA(long ptr long) ExtractAssociatedIcon32A
  83 stub SHELL32_83
  84 stub SHELL32_84
  85 stdcall SHELL32_85(long long long long) SHELL32_85
  86 stdcall SHELL32_86(long ptr) SHELL32_86
  87 stdcall SHELL32_87(long) SHELL32_87
  88 stub SHELL32_88
  89 stdcall SHELL32_89(long long long) SHELL32_89
  90 stub SHELL32_90
  91 stub SHELL32_91
  92 stub SHELL32_92
  93 stub SHELL32_93
  94 stub SHELL32_94
  95 stub SHELL32_95
  96 stub SHELL32_96
  97 stub SHELL32_97
  98 stub SHELL32_98
  99 stub SHELL32_99
 100 stdcall SHELL32_100(long) SHELL32_100
 101 stub ExtractAssociatedIconExA
 102 stdcall SHELL32_102(ptr ptr long ptr ptr) SHELL32_102
 103 stub SHELL32_103
 104 stub SHELL32_104
 105 stub SHELL32_105
 106 stub SHELL32_106
 107 stub SHELL32_107
 108 stub SHELL32_108
 109 stub SHELL32_109
 110 stub SHELL32_110
 111 stub SHELL32_111
 112 stub SHELL32_112
 113 stub SHELL32_113
 114 stub SHELL32_114
 115 stub SHELL32_115
 116 stub SHELL32_116
 117 stub SHELL32_117
 118 stub SHELL32_118
 119 stdcall SHELL32_119(ptr) SHELL32_119
 120 stub SHELL32_120
 121 stub SHELL32_121
 122 stub SHELL32_122
 123 stub SHELL32_123
 124 stub ExtractAssociatedIconExW
 125 stub ExtractAssociatedIconW
 126 stub SHELL32_126
 127 stub SHELL32_127
 128 stdcall DllGetClassObject(long long ptr) SHELL32_DllGetClassObject
 129 stub SHELL32_129
 130 stub SHELL32_130
 131 stub SHELL32_131
 132 stub SHELL32_132
 133 stdcall ExtractIconA(long str long) ExtractIcon32A
 134 stub SHELL32_134
 135 stub ExtractIconEx
 136 stub SHELL32_136
 137 stub SHELL32_137
 138 stub ExtractIconExA
 139 stub SHELL32_139
 140 stub SHELL32_140
 141 stub SHELL32_141
 142 stub SHELL32_142
 143 stub SHELL32_143
 144 stub SHELL32_144
 145 stub SHELL32_145
 146 stub SHELL32_146
 147 stub SHELL32_147
 148 stub ExtractIconResInfoA
 149 stub SHELL32_149
 150 stub ExtractIconResInfoW
 151 stub SHELL32_151
 152 stdcall SHELL32_152(ptr) SHELL32_152
 153 stub SHELL32_153
 154 stub SHELL32_154
 155 stdcall SHELL32_155(ptr) SHELL32_155
 156 stub SHELL32_156
 157 stub SHELL32_157
 158 stub SHELL32_158
 159 stub SHELL32_159
 160 stub SHELL32_160
 161 stub SHELL32_161
 162 stub SHELL32_162
 163 stub SHELL32_163
 164 stub SHELL32_164
 165 stdcall SHELL32_165(long long) SHELL32_165
 166 stub SHELL32_166
 167 stub SHELL32_167
 168 stub SHELL32_168
 169 stub SHELL32_169
 170 stub SHELL32_170
 171 stub SHELL32_171
 172 stub SHELL32_172
 173 stub SHELL32_173
 174 stub SHELL32_174
 175 stdcall SHELL32_175(long long long long) SHELL32_175
 176 stub SHELL32_176
 177 stub SHELL32_177
 178 stub SHELL32_178
 179 stub SHELL32_179
 180 stdcall ExtractIconW(long wstr long) ExtractIcon32W
 181 stdcall SHELL32_181(long long) SHELL32_181
 182 stub ExtractVersionResource16W
 183 cdecl SHELL32_183(long long long long long long) SHELL32_183
 184 stub SHELL32_184
 185 stub SHELL32_185
 186 stdcall FindExecutableA(ptr ptr ptr) FindExecutable32A
 187 stub FindExecutableW
 188 stdcall FreeIconList(long) FreeIconList
 189 stub InternalExtractIconListA
 190 stub InternalExtractIconListW
 191 stub OpenAs_RunDLL
 192 stub PrintersGetCommand_RunDLL
 193 stub RealShellExecuteA
 194 stub RealShellExecuteExA
 195 stdcall SHELL32_195(ptr) SHELL32_195
 196 stdcall SHELL32_196(long) SHELL32_196
 197 stub SHELL32_197
 198 stub SHELL32_198
 199 stub SHELL32_199
 200 stub SHELL32_200
 201 stub SHELL32_201
 202 stub SHELL32_202
 203 stub RealShellExecuteExW
 204 stub RealShellExecuteW
 205 stub RegenerateUserEnvironment
 206 stub SHAddToRecentDocs
 207 stdcall SHAppBarMessage(long ptr) SHAppBarMessage32
 208 stub SHBrowseForFolder
 209 stub SHBrowseForFolderA
 210 stub SHChangeNotify
 211 stub SHFileOperation
 212 stub SHFileOperationA
 213 stub SHFormatDrive
 214 stub SHFreeNameMappings
 215 stub SHGetDataFromIDListA
 216 stdcall SHGetDesktopFolder(ptr) SHGetDesktopFolder
 217 stdcall SHGetFileInfo(ptr long ptr long long) SHGetFileInfo32A
 218 stdcall SHGetFileInfoA(ptr long ptr long long) SHGetFileInfo32A
 219 stub SHGetInstanceExplorer
 220 stdcall SHGetMalloc(ptr) SHGetMalloc
 221 stdcall SHGetPathFromIDList(ptr ptr) SHGetPathFromIDList
 222 stub SHGetPathFromIDListA
 223 stdcall SHGetSpecialFolderLocation(long long ptr) SHGetSpecialFolderLocation
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
 243 stdcall ShellAboutA(long str str long) ShellAbout32A
 244 stdcall ShellAboutW(long wstr wstr long) ShellAbout32W
 245 stdcall ShellExecuteA(long str str str str long) ShellExecute32A
 246 stub ShellExecuteEx
 247 stub ShellExecuteExA
 248 stub ShellExecuteW
 249 stdcall Shell_NotifyIcon(long ptr) Shell_NotifyIcon
 250 stdcall Shell_NotifyIconA(long ptr) Shell_NotifyIconA
 251 stub Shl1632_ThunkData32
 252 stub Shl3216_ThunkData32
 505 stub SHELL32_505
 507 stub SHELL32_507
 510 stub SHELL32_510
 511 stub SHELL32_511
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
