name    wininet
type    win32
init    WININET_LibMain

@ stub InternetInitializeAutoProxyDll
@ stub ShowCertificate
@ stub CommitUrlCacheEntryA
@ stub CommitUrlCacheEntryW
@ stub CreateUrlCacheContainerA
@ stub CreateUrlCacheContainerW
@ stub CreateUrlCacheEntryA
@ stub CreateUrlCacheEntryW
@ stub CreateUrlCacheGroup
@ stub DeleteIE3Cache
@ stub DeleteUrlCacheContainerA
@ stub DeleteUrlCacheContainerW
@ stub DeleteUrlCacheEntry
@ stub DeleteUrlCacheGroup
@ stdcall DllInstall(long ptr) WININET_DllInstall
@ stub FindCloseUrlCache
@ stub FindFirstUrlCacheContainerA
@ stub FindFirstUrlCacheContainerW
@ stub FindFirstUrlCacheEntryA
@ stub FindFirstUrlCacheEntryExA
@ stub FindFirstUrlCacheEntryExW
@ stub FindFirstUrlCacheEntryW
@ stub FindNextUrlCacheContainerA
@ stub FindNextUrlCacheContainerW
@ stub FindNextUrlCacheEntryA
@ stub FindNextUrlCacheEntryExA
@ stub FindNextUrlCacheEntryExW
@ stub FindNextUrlCacheEntryW
@ stub FreeUrlCacheSpaceA
@ stub FreeUrlCacheSpaceW
@ stdcall FtpCreateDirectoryA(ptr str)  FtpCreateDirectoryA
@ stub FtpCreateDirectoryW
@ stdcall FtpDeleteFileA(ptr str) FtpDeleteFileA
@ stub FtpDeleteFileW
@ stdcall FtpFindFirstFileA(ptr str str long long) FtpFindFirstFileA
@ stub FtpFindFirstFileW
@ stdcall FtpGetCurrentDirectoryA(ptr str ptr) FtpGetCurrentDirectoryA
@ stub FtpGetCurrentDirectoryW
@ stdcall FtpGetFileA(ptr str str long long long long) FtpGetFileA
@ stub FtpGetFileW
@ stdcall FtpOpenFileA(ptr str long long long) FtpOpenFileA
@ stub FtpOpenFileW
@ stdcall FtpPutFileA(ptr str str long long) FtpPutFileA 
@ stub FtpPutFileW
@ stdcall FtpRemoveDirectoryA(ptr str) FtpRemoveDirectoryA
@ stub FtpRemoveDirectoryW
@ stdcall FtpRenameFileA(ptr str str) FtpRenameFileA
@ stub FtpRenameFileW
@ stdcall FtpSetCurrentDirectoryA(ptr str) FtpSetCurrentDirectoryA
@ stub FtpSetCurrentDirectoryW
@ stub GetUrlCacheConfigInfoA
@ stub GetUrlCacheConfigInfoW
@ stub GetUrlCacheEntryInfoA
@ stub GetUrlCacheEntryInfoExA
@ stub GetUrlCacheEntryInfoExW
@ stub GetUrlCacheEntryInfoW
@ stub GetUrlCacheHeaderData
@ stub GopherCreateLocatorA
@ stub GopherCreateLocatorW
@ stub GopherFindFirstFileA
@ stub GopherFindFirstFileW
@ stub GopherGetAttributeA
@ stub GopherGetAttributeW
@ stub GopherGetLocatorTypeA
@ stub GopherGetLocatorTypeW
@ stub GopherOpenFileA
@ stub GopherOpenFileW
@ stdcall HttpAddRequestHeadersA(ptr str long long) HttpAddRequestHeadersA
@ stub HttpAddRequestHeadersW
@ stub HttpEndRequestA
@ stub HttpEndRequestW
@ stdcall HttpOpenRequestA(ptr str str str str ptr long long) HttpOpenRequestA
@ stub HttpOpenRequestW
@ stdcall HttpQueryInfoA(ptr long ptr ptr ptr) HttpQueryInfoA
@ stub HttpQueryInfoW
@ stdcall HttpSendRequestA(ptr str long ptr long) HttpSendRequestA
@ stub HttpSendRequestExA
@ stub HttpSendRequestExW
@ stub HttpSendRequestW
@ stub IncrementUrlCacheHeaderData
@ stdcall InternetAttemptConnect(long) InternetAttemptConnect
@ stub InternetAutodial
@ stub InternetAutodialCallback
@ stub InternetAutodialHangup
@ stdcall InternetCanonicalizeUrlA(str str ptr long) InternetCanonicalizeUrlA
@ stub InternetCanonicalizeUrlW
@ stdcall InternetCheckConnectionA(ptr long long) InternetCheckConnectionA
@ stub InternetCheckConnectionW
@ stdcall InternetCloseHandle(long) InternetCloseHandle
@ stub InternetCombineUrlA
@ stub InternetCombineUrlW
@ stub InternetConfirmZoneCrossing
@ stdcall InternetConnectA(ptr str long str str long long long) InternetConnectA
@ stub InternetConnectW
@ stdcall InternetCrackUrlA(str long long ptr) InternetCrackUrlA
@ stub InternetCrackUrlW
@ stub InternetCreateUrlA
@ stub InternetCreateUrlW
@ stub InternetDebugGetLocalTime
@ stub InternetDial
@ stub InternetErrorDlg
@ stdcall InternetFindNextFileA(ptr ptr) InternetFindNextFileA 
@ stub InternetFindNextFileW
@ stub InternetGetCertByURL
@ stdcall InternetGetConnectedState(ptr long) InternetGetConnectedState
@ stub InternetGetCookieA
@ stub InternetGetCookieW
@ stdcall InternetGetLastResponseInfoA(ptr str ptr) InternetGetLastResponseInfoA
@ stub InternetGetLastResponseInfoW
@ stub InternetGoOnline
@ stub InternetHangUp
@ stub InternetLockRequestFile
@ stdcall InternetOpenA(str long str str long) InternetOpenA
@ stub InternetOpenServerPushParse
@ stub InternetOpenUrlA
@ stub InternetOpenUrlW
@ stub InternetOpenW
@ stub InternetQueryDataAvailable
@ stdcall InternetQueryOptionA(ptr long ptr ptr) InternetQueryOptionA
@ stub InternetQueryOptionW
@ stdcall InternetReadFile(ptr ptr long ptr) InternetReadFile  
@ stub InternetReadFileExA
@ stub InternetReadFileExW
@ stub InternetServerPushParse
@ stub InternetSetCookieA
@ stub InternetSetCookieW
@ stub InternetSetDialState
@ stub InternetSetFilePointer
@ stub InternetSetOptionA
@ stub InternetSetOptionExA
@ stub InternetSetOptionExW
@ stub InternetSetOptionW
@ stdcall InternetSetStatusCallback(ptr ptr) InternetSetStatusCallback
@ stub InternetShowSecurityInfoByURL
@ stub InternetTimeFromSystemTime
@ stub InternetTimeToSystemTime
@ stub InternetUnlockRequestFile
@ stdcall InternetWriteFile(ptr ptr long ptr) InternetWriteFile
@ stub InternetWriteFileExA
@ stub InternetWriteFileExW
@ stub IsHostInProxyBypassList
@ stub LoadUrlCacheContent
@ stub ParseX509EncodedCertificateForListBoxEntry
@ stub ReadUrlCacheEntryStream
@ stub RetrieveUrlCacheEntryFileA
@ stub RetrieveUrlCacheEntryFileW
@ stub RetrieveUrlCacheEntryStreamA
@ stub RetrieveUrlCacheEntryStreamW
@ stub RunOnceUrlCache
@ stub SetUrlCacheConfigInfoA
@ stub SetUrlCacheConfigInfoW
@ stub SetUrlCacheEntryGroup
@ stub SetUrlCacheEntryInfoA
@ stub SetUrlCacheEntryInfoW
@ stub SetUrlCacheHeaderData
@ stub ShowClientAuthCerts
@ stub ShowSecurityInfo
@ stub ShowX509EncodedCertificate
@ stub UnlockUrlCacheEntryFile
@ stub UnlockUrlCacheEntryStream
@ stub UpdateUrlCacheContentPath
