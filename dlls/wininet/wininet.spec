@ stdcall InternetInitializeAutoProxyDll(long)
@ stub ShowCertificate
@ stdcall CommitUrlCacheEntryA(str str double double long str long str str)
@ stub CommitUrlCacheEntryW
@ stub CreateUrlCacheContainerA
@ stub CreateUrlCacheContainerW
@ stub CreateUrlCacheEntryA
@ stub CreateUrlCacheEntryW
@ stdcall CreateUrlCacheGroup(long ptr)
@ stub DeleteIE3Cache
@ stub DeleteUrlCacheContainerA
@ stub DeleteUrlCacheContainerW
@ stdcall DeleteUrlCacheEntry(str)
@ stdcall DeleteUrlCacheGroup(double long ptr)
@ stdcall DetectAutoProxyUrl(str long long)
@ stdcall DllInstall(long ptr) WININET_DllInstall
@ stub FindCloseUrlCache
@ stub FindFirstUrlCacheContainerA
@ stub FindFirstUrlCacheContainerW
@ stdcall FindFirstUrlCacheEntryA(str ptr ptr)
@ stub FindFirstUrlCacheEntryExA
@ stub FindFirstUrlCacheEntryExW
@ stdcall FindFirstUrlCacheEntryW(wstr ptr ptr)
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
@ stdcall FtpCreateDirectoryA(ptr str)
@ stdcall FtpCreateDirectoryW(ptr wstr)
@ stdcall FtpDeleteFileA(ptr str)
@ stub FtpDeleteFileW
@ stdcall FtpFindFirstFileA(ptr str str long long)
@ stdcall FtpFindFirstFileW(ptr wstr wstr long long)
@ stdcall FtpGetCurrentDirectoryA(ptr str ptr)
@ stdcall FtpGetCurrentDirectoryW(ptr wstr ptr)
@ stdcall FtpGetFileA(ptr str str long long long long)
@ stdcall FtpGetFileW(ptr wstr wstr long long long long)
@ stdcall FtpOpenFileA(ptr str long long long)
@ stdcall FtpOpenFileW(ptr wstr long long long)
@ stdcall FtpPutFileA(ptr str str long long)
@ stub FtpPutFileW
@ stdcall FtpRemoveDirectoryA(ptr str)
@ stub FtpRemoveDirectoryW
@ stdcall FtpRenameFileA(ptr str str)
@ stub FtpRenameFileW
@ stdcall FtpSetCurrentDirectoryA(ptr str)
@ stdcall FtpSetCurrentDirectoryW(ptr wstr)
@ stdcall GetUrlCacheConfigInfoA(ptr ptr long)
@ stdcall GetUrlCacheConfigInfoW(ptr ptr long)
@ stdcall GetUrlCacheEntryInfoA(str ptr long)
@ stdcall GetUrlCacheEntryInfoExA(str ptr ptr str ptr ptr long)
@ stdcall GetUrlCacheEntryInfoExW(wstr ptr ptr wstr ptr ptr long)
@ stdcall GetUrlCacheEntryInfoW(wstr ptr long)
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
@ stdcall HttpAddRequestHeadersA(ptr str long long)
@ stub HttpAddRequestHeadersW
@ stdcall HttpEndRequestA(ptr ptr long long)
@ stdcall HttpEndRequestW(ptr ptr long long)
@ stdcall HttpOpenRequestA(ptr str str str str ptr long long)
@ stdcall HttpOpenRequestW(ptr wstr wstr wstr wstr ptr long long)
@ stdcall HttpQueryInfoA(ptr long ptr ptr ptr)
@ stdcall HttpQueryInfoW(ptr long ptr ptr ptr)
@ stdcall HttpSendRequestA(ptr str long ptr long)
@ stdcall HttpSendRequestExA(long ptr ptr long long)
@ stub HttpSendRequestExW
@ stdcall HttpSendRequestW(ptr wstr long ptr long)
@ stub IncrementUrlCacheHeaderData
@ stdcall InternetAttemptConnect(long)
@ stdcall InternetAutodial(long ptr)
@ stub InternetAutodialCallback
@ stdcall InternetAutodialHangup(long)
@ stdcall InternetCanonicalizeUrlA(str str ptr long)
@ stdcall InternetCanonicalizeUrlW(wstr wstr ptr long)
@ stdcall InternetCheckConnectionA(ptr long long)
@ stdcall InternetCheckConnectionW(ptr long long)
@ stdcall InternetCloseHandle(long)
@ stdcall InternetCombineUrlA(str str str ptr long)
@ stdcall InternetCombineUrlW(wstr wstr wstr ptr long)
@ stub InternetConfirmZoneCrossing
@ stdcall InternetConnectA(ptr str long str str long long long)
@ stdcall InternetConnectW(ptr wstr long wstr wstr long long long)
@ stdcall InternetCrackUrlA(str long long ptr)
@ stdcall InternetCrackUrlW(wstr long long ptr)
@ stub InternetCreateUrlA
@ stub InternetCreateUrlW
@ stub InternetDebugGetLocalTime
@ stub InternetDial
@ stub InternetErrorDlg
@ stdcall InternetFindNextFileA(ptr ptr)
@ stub InternetFindNextFileW
@ stub InternetGetCertByURL
@ stdcall InternetGetConnectedState(ptr long)
@ stdcall InternetGetConnectedStateExW(ptr ptr long long)
@ stdcall InternetGetCookieA(str str ptr long)
@ stdcall InternetGetCookieW(wstr wstr ptr long)
@ stdcall InternetGetLastResponseInfoA(ptr str ptr)
@ stub InternetGetLastResponseInfoW
@ stub InternetGoOnline
@ stub InternetHangUp
@ stdcall InternetLockRequestFile(ptr ptr)
@ stdcall InternetOpenA(str long str str long)
@ stdcall InternetOpenW(wstr long wstr wstr long)
@ stub InternetOpenServerPushParse
@ stdcall InternetOpenUrlA(ptr str str long long long)
@ stdcall InternetOpenUrlW(ptr wstr wstr long long long)
@ stdcall InternetQueryDataAvailable(ptr ptr long long)
@ stdcall InternetQueryOptionA(ptr long ptr ptr)
@ stdcall InternetQueryOptionW(ptr long ptr ptr)
@ stdcall InternetReadFile(ptr ptr long ptr)
@ stdcall InternetReadFileExA(ptr ptr long long)
@ stdcall InternetReadFileExW(ptr ptr long long)
@ stub InternetServerPushParse
@ stdcall InternetSetCookieA(str str str)
@ stdcall InternetSetCookieW(wstr wstr wstr)
@ stub InternetSetDialState
@ stub InternetSetFilePointer
@ stdcall InternetSetOptionA(ptr long ptr long)
@ stdcall InternetSetOptionW(ptr long ptr long)
@ stdcall InternetSetOptionExA(ptr long ptr long long)
@ stdcall InternetSetOptionExW(ptr long ptr long long)
@ stdcall InternetSetStatusCallback(ptr ptr) InternetSetStatusCallbackA
@ stdcall InternetSetStatusCallbackA(ptr ptr)
@ stub InternetSetStatusCallbackW
@ stub InternetShowSecurityInfoByURL
@ stub InternetTimeFromSystemTime
@ stub InternetTimeToSystemTime
@ stdcall InternetUnlockRequestFile(ptr)
@ stdcall InternetWriteFile(ptr ptr long ptr)
@ stub InternetWriteFileExA
@ stub InternetWriteFileExW
@ stdcall IsHostInProxyBypassList(long str long)
@ stub LoadUrlCacheContent
@ stub ParseX509EncodedCertificateForListBoxEntry
@ stub ReadUrlCacheEntryStream
@ stdcall RetrieveUrlCacheEntryFileA(str ptr ptr long)
@ stub RetrieveUrlCacheEntryFileW
@ stub RetrieveUrlCacheEntryStreamA
@ stub RetrieveUrlCacheEntryStreamW
@ stub RunOnceUrlCache
@ stub SetUrlCacheConfigInfoA
@ stub SetUrlCacheConfigInfoW
@ stdcall SetUrlCacheEntryGroup(str long double ptr long ptr)
@ stub SetUrlCacheEntryInfoA
@ stub SetUrlCacheEntryInfoW
@ stub SetUrlCacheHeaderData
@ stub ShowClientAuthCerts
@ stub ShowSecurityInfo
@ stub ShowX509EncodedCertificate
@ stub UnlockUrlCacheEntryFile
@ stub UnlockUrlCacheEntryStream
@ stub UpdateUrlCacheContentPath
