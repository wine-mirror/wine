@ stdcall InternetInitializeAutoProxyDll(long) InternetInitializeAutoProxyDll
@ stub ShowCertificate
@ stdcall CommitUrlCacheEntryA(str str long long long str long str str) CommitUrlCacheEntryA
@ stub CommitUrlCacheEntryW
@ stub CreateUrlCacheContainerA
@ stub CreateUrlCacheContainerW
@ stub CreateUrlCacheEntryA
@ stub CreateUrlCacheEntryW
@ stdcall CreateUrlCacheGroup(long ptr) CreateUrlCacheGroup
@ stub DeleteIE3Cache
@ stub DeleteUrlCacheContainerA
@ stub DeleteUrlCacheContainerW
@ stdcall DeleteUrlCacheEntry(str) DeleteUrlCacheEntry
@ stdcall DeleteUrlCacheGroup(long long ptr) DeleteUrlCacheGroup
@ stdcall DllInstall(long ptr) WININET_DllInstall
@ stub FindCloseUrlCache
@ stub FindFirstUrlCacheContainerA
@ stub FindFirstUrlCacheContainerW
@ stdcall FindFirstUrlCacheEntryA(str ptr ptr) FindFirstUrlCacheEntryA
@ stub FindFirstUrlCacheEntryExA
@ stub FindFirstUrlCacheEntryExW
@ stdcall FindFirstUrlCacheEntryW(wstr ptr ptr) FindFirstUrlCacheEntryW
@ stub FindNextUrlCacheContainerA
@ stub FindNextUrlCacheContainerW
@ stub FindNextUrlCacheEntryA
@ stub FindNextUrlCacheEntryExA
@ stub FindNextUrlCacheEntryExW
@ stub FindNextUrlCacheEntryW
@ stub FreeUrlCacheSpaceA
@ stub FreeUrlCacheSpaceW
@ stub FtpCommandA
@ stub FtpCommandW
@ stdcall FtpCreateDirectoryA(ptr str) FtpCreateDirectoryA
@ stdcall FtpCreateDirectoryW(ptr wstr) FtpCreateDirectoryW
@ stdcall FtpDeleteFileA(ptr str) FtpDeleteFileA
@ stub FtpDeleteFileW
@ stdcall FtpFindFirstFileA(ptr str str long long) FtpFindFirstFileA
@ stdcall FtpFindFirstFileW(ptr wstr wstr long long) FtpFindFirstFileW
@ stdcall FtpGetCurrentDirectoryA(ptr str ptr) FtpGetCurrentDirectoryA
@ stdcall FtpGetCurrentDirectoryW(ptr wstr ptr) FtpGetCurrentDirectoryW
@ stdcall FtpGetFileA(ptr str str long long long long) FtpGetFileA
@ stdcall FtpGetFileW(ptr wstr wstr long long long long) FtpGetFileW
@ stdcall FtpOpenFileA(ptr str long long long) FtpOpenFileA
@ stdcall FtpOpenFileW(ptr wstr long long long) FtpOpenFileW
@ stdcall FtpPutFileA(ptr str str long long) FtpPutFileA
@ stub FtpPutFileW
@ stdcall FtpRemoveDirectoryA(ptr str) FtpRemoveDirectoryA
@ stub FtpRemoveDirectoryW
@ stdcall FtpRenameFileA(ptr str str) FtpRenameFileA
@ stub FtpRenameFileW
@ stdcall FtpSetCurrentDirectoryA(ptr str) FtpSetCurrentDirectoryA
@ stdcall FtpSetCurrentDirectoryW(ptr wstr) FtpSetCurrentDirectoryW
@ stub GetUrlCacheConfigInfoA
@ stub GetUrlCacheConfigInfoW
@ stdcall GetUrlCacheEntryInfoA(str ptr long) GetUrlCacheEntryInfoA
@ stdcall GetUrlCacheEntryInfoExA(str ptr ptr str ptr ptr long) GetUrlCacheEntryInfoExA
@ stdcall GetUrlCacheEntryInfoExW(wstr ptr ptr wstr ptr ptr long) GetUrlCacheEntryInfoExW
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
@ stdcall HttpEndRequestA(ptr ptr long long) HttpEndRequestA
@ stdcall HttpEndRequestW(ptr ptr long long) HttpEndRequestW
@ stdcall HttpOpenRequestA(ptr str str str str ptr long long) HttpOpenRequestA
@ stdcall HttpOpenRequestW(ptr wstr wstr wstr wstr ptr long long) HttpOpenRequestW
@ stdcall HttpQueryInfoA(ptr long ptr ptr ptr) HttpQueryInfoA
@ stdcall HttpQueryInfoW(ptr long ptr ptr ptr) HttpQueryInfoW
@ stdcall HttpSendRequestA(ptr str long ptr long) HttpSendRequestA
@ stdcall HttpSendRequestExA(long ptr ptr long long) HttpSendRequestExA
@ stub HttpSendRequestExW
@ stdcall HttpSendRequestW(ptr wstr long ptr long) HttpSendRequestW
@ stub IncrementUrlCacheHeaderData
@ stdcall InternetAttemptConnect(long) InternetAttemptConnect
@ stdcall InternetAutodial(long ptr) InternetAutoDial
@ stub InternetAutodialCallback
@ stdcall InternetAutodialHangup(long) InternetAutodialHangup
@ stdcall InternetCanonicalizeUrlA(str str ptr long) InternetCanonicalizeUrlA
@ stdcall InternetCanonicalizeUrlW(wstr wstr ptr long) InternetCanonicalizeUrlW
@ stdcall InternetCheckConnectionA(ptr long long) InternetCheckConnectionA
@ stdcall InternetCheckConnectionW(ptr long long) InternetCheckConnectionW
@ stdcall InternetCloseHandle(long) InternetCloseHandle
@ stdcall InternetCombineUrlA(str str str ptr long) InternetCombineUrlA
@ stdcall InternetCombineUrlW(wstr wstr wstr ptr long) InternetCombineUrlW
@ stub InternetConfirmZoneCrossing
@ stdcall InternetConnectA(ptr str long str str long long long) InternetConnectA
@ stdcall InternetConnectW(ptr wstr long wstr wstr long long long) InternetConnectW
@ stdcall InternetCrackUrlA(str long long ptr) InternetCrackUrlA
@ stdcall InternetCrackUrlW(wstr long long ptr) InternetCrackUrlW
@ stub InternetCreateUrlA
@ stub InternetCreateUrlW
@ stub InternetDebugGetLocalTime
@ stub InternetDial
@ stub InternetErrorDlg
@ stdcall InternetFindNextFileA(ptr ptr) InternetFindNextFileA
@ stub InternetFindNextFileW
@ stub InternetGetCertByURL
@ stdcall InternetGetConnectedState(ptr long) InternetGetConnectedState
@ stdcall InternetGetCookieA(str str ptr long) InternetGetCookieA
@ stdcall InternetGetCookieW(wstr wstr ptr long) InternetGetCookieW
@ stdcall InternetGetLastResponseInfoA(ptr str ptr) InternetGetLastResponseInfoA
@ stub InternetGetLastResponseInfoW
@ stub InternetGoOnline
@ stub InternetHangUp
@ stdcall InternetLockRequestFile(ptr ptr) InternetLockRequestFile
@ stdcall InternetOpenA(str long str str long) InternetOpenA
@ stdcall InternetOpenW(wstr long wstr wstr long) InternetOpenW
@ stub InternetOpenServerPushParse
@ stdcall InternetOpenUrlA(ptr str str long long long) InternetOpenUrlA
@ stdcall InternetOpenUrlW(ptr wstr wstr long long long) InternetOpenUrlW
@ stdcall InternetQueryDataAvailable(ptr ptr long long) InternetQueryDataAvailable
@ stdcall InternetQueryOptionA(ptr long ptr ptr) InternetQueryOptionA
@ stdcall InternetQueryOptionW(ptr long ptr ptr) InternetQueryOptionW
@ stdcall InternetReadFile(ptr ptr long ptr) InternetReadFile
@ stdcall InternetReadFileExA(ptr ptr long long) InternetReadFileExA
@ stdcall InternetReadFileExW(ptr ptr long long) InternetReadFileExW
@ stub InternetServerPushParse
@ stdcall InternetSetCookieA(str str str) InternetSetCookieA
@ stdcall InternetSetCookieW(wstr wstr wstr) InternetSetCookieW
@ stub InternetSetDialState
@ stub InternetSetFilePointer
@ stdcall InternetSetOptionA(ptr long ptr long) InternetSetOptionA
@ stdcall InternetSetOptionW(ptr long ptr long) InternetSetOptionW
@ stub InternetSetOptionExA
@ stub InternetSetOptionExW
@ stdcall InternetSetStatusCallback(ptr ptr) InternetSetStatusCallback
@ stub InternetShowSecurityInfoByURL
@ stub InternetTimeFromSystemTime
@ stub InternetTimeToSystemTime
@ stdcall InternetUnlockRequestFile(ptr) InternetUnlockRequestFile
@ stdcall InternetWriteFile(ptr ptr long ptr) InternetWriteFile
@ stub InternetWriteFileExA
@ stub InternetWriteFileExW
@ stdcall IsHostInProxyBypassList(long str long) IsHostInProxyBypassList
@ stub LoadUrlCacheContent
@ stub ParseX509EncodedCertificateForListBoxEntry
@ stub ReadUrlCacheEntryStream
@ stdcall RetrieveUrlCacheEntryFileA(str ptr ptr long) RetrieveUrlCacheEntryFileA
@ stub RetrieveUrlCacheEntryFileW
@ stub RetrieveUrlCacheEntryStreamA
@ stub RetrieveUrlCacheEntryStreamW
@ stub RunOnceUrlCache
@ stub SetUrlCacheConfigInfoA
@ stub SetUrlCacheConfigInfoW
@ stdcall SetUrlCacheEntryGroup(str long long ptr long ptr) SetUrlCacheEntryGroup
@ stub SetUrlCacheEntryInfoA
@ stub SetUrlCacheEntryInfoW
@ stub SetUrlCacheHeaderData
@ stub ShowClientAuthCerts
@ stub ShowSecurityInfo
@ stub ShowX509EncodedCertificate
@ stub UnlockUrlCacheEntryFile
@ stub UnlockUrlCacheEntryStream
@ stub UpdateUrlCacheContentPath
