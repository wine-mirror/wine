101 stub -noname DoConnectoidsExist
102 stub -noname GetDiskInfoA
103 stub -noname PerformOperationOverUrlCacheA
104 stub -noname HttpCheckDavComplianceA
105 stub -noname HttpCheckDavComplianceW
108 stub -noname ImportCookieFileA
109 stub -noname ExportCookieFileA
110 stub -noname ImportCookieFileW
111 stub -noname ExportCookieFileW
112 stub -noname IsProfilesEnabled
116 stub -noname IsDomainlegalCookieDomainA
117 stub -noname IsDomainLegalCookieDomainW
118 stub -noname FindP3PPolicySymbol
120 stub -noname MapResourceToPolicy
121 stub -noname GetP3PPolicy
122 stub -noname FreeP3PObject
123 stub -noname GetP3PRequestStatus

@ stdcall InternetInitializeAutoProxyDll(long)
@ stub ShowCertificate
@ stdcall CommitUrlCacheEntryA(str str double double long str long str str)
@ stub CommitUrlCacheEntryW
@ stub CreateUrlCacheContainerA
@ stub CreateUrlCacheContainerW
@ stdcall CreateUrlCacheEntryA(str long str ptr long)
@ stub CreateUrlCacheEntryW
@ stdcall CreateUrlCacheGroup(long ptr)
@ stub DeleteIE3Cache
@ stub DeleteUrlCacheContainerA
@ stub DeleteUrlCacheContainerW
@ stdcall DeleteUrlCacheEntry(str) DeleteUrlCacheEntryA
@ stdcall DeleteUrlCacheEntryA(str)
@ stub DeleteUrlCacheEntryW
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
@ stdcall FtpFindFirstFileA(ptr str ptr long long)
@ stdcall FtpFindFirstFileW(ptr wstr ptr long long)
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
@ stdcall GopherCreateLocatorA(str long str str long str ptr)
@ stdcall GopherCreateLocatorW(wstr long wstr wstr long wstr ptr)
@ stdcall GopherFindFirstFileA(ptr str str ptr long long)
@ stdcall GopherFindFirstFileW(ptr wstr wstr ptr long long)
@ stdcall GopherGetAttributeA(ptr str str ptr long ptr ptr long)
@ stdcall GopherGetAttributeW(ptr wstr wstr ptr long ptr ptr long)
@ stdcall GopherGetLocatorTypeA(str ptr)
@ stdcall GopherGetLocatorTypeW(wstr ptr)
@ stdcall GopherOpenFileA(ptr str str long long)
@ stdcall GopherOpenFileW(ptr wstr wstr long long)
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
@ stdcall InternetCreateUrlA(ptr long ptr ptr)
@ stdcall InternetCreateUrlW(ptr long ptr ptr)
@ stub InternetDebugGetLocalTime
@ stub InternetDial
@ stdcall InternetErrorDlg(long long long long ptr)
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
@ stdcall InternetSetFilePointer(ptr long ptr long long)
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
@ stdcall ReadUrlCacheEntryStream(ptr long ptr ptr long)
@ stdcall RetrieveUrlCacheEntryFileA(str ptr ptr long)
@ stub RetrieveUrlCacheEntryFileW
@ stdcall RetrieveUrlCacheEntryStreamA(str ptr ptr long long)
@ stub RetrieveUrlCacheEntryStreamW
@ stub RunOnceUrlCache
@ stub SetUrlCacheConfigInfoA
@ stub SetUrlCacheConfigInfoW
@ stdcall SetUrlCacheEntryGroup(str long double ptr long ptr)
@ stdcall SetUrlCacheEntryInfoA(str ptr long)
@ stdcall SetUrlCacheEntryInfoW(wstr ptr long)
@ stub SetUrlCacheHeaderData
@ stub ShowClientAuthCerts
@ stub ShowSecurityInfo
@ stub ShowX509EncodedCertificate
@ stdcall UnlockUrlCacheEntryFile(str long) UnlockUrlCacheEntryFileA
@ stdcall UnlockUrlCacheEntryFileA(str long)
@ stub UnlockUrlCacheEntryFileW
@ stdcall UnlockUrlCacheEntryStream(ptr long)
@ stub UpdateUrlCacheContentPath
