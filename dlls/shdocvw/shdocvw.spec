name	shdocvw
type	win32
init	COMIMPL_DllMain

import	ole32.dll
import	kernel32.dll
import	ntdll.dll

debug_channels (shdocvw comimpl)

108 stub AddUrlToFavorites
109 stdcall DllCanUnloadNow() COMIMPL_DllCanUnloadNow
312 stdcall DllGetClassObject(long long ptr) COMIMPL_DllGetClassObject
113 stdcall DllGetVersion(ptr) SHDOCVW_DllGetVersion
114 stdcall DllInstall(long wstr) SHDOCVW_DllInstall
124 stdcall DllRegisterServer() SHDOCVW_DllRegisterServer
126 stub DllRegisterWindowClasses
127 stdcall DllUnregisterServer() SHDOCVW_DllUnregisterServer
128 stub DoAddToFavDlg
129 stub DoAddToFavDlgW
132 stub DoFileDownload
133 stub DoFileDownloadEx
134 stub DoOrganizeFavDlg
144 stub DoOrganizeFavDlgW
106 stub HlinkFindFrame
154 stub HlinkFrameNavigate
155 stub HlinkFrameNavigateNHL
156 stub IEWriteErrorLog
157 stub OpenURL
163 stub SHAddSubscribeFavorite
166 stub SHGetIDispatchForFolder
168 stub SetQueryNetSessionCount
107 stub SetShellOfflineState
182 stub SoftwareUpdateMessageBox
184 stub URLQualifyA
186 stub URLQualifyW
