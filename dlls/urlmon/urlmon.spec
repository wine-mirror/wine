name    urlmon
type    win32
init	URLMON_DllMain

import	ole32.dll
import	kernel32.dll
import	ntdll.dll

debug_channels (comimpl urlmon)

1 stub CDLGetLongPathNameA
2 stub CDLGetLongPathNameW
@ stub AsyncGetClassBits
@ stub AsyncInstallDistributionUnit
@ stub BindAsyncMoniker
@ stub CoGetClassObjectFromURL
@ stub CoInstall
@ stub CoInternetCombineUrl
@ stub CoInternetCompareUrl
@ stub CoInternetCreateSecurityManager
@ stub CoInternetCreateZoneManager
@ stub CoInternetGetProtocolFlags
@ stub CoInternetGetSecurityUrl
@ stdcall CoInternetGetSession(long ptr long) CoInternetGetSession
@ stub CoInternetParseUrl
@ stub CoInternetQueryInfo
@ stub CopyBindInfo
@ stub CopyStgMedium
@ stub CreateAsyncBindCtx
@ stdcall CreateAsyncBindCtxEx(ptr long ptr ptr ptr long) CreateAsyncBindCtxEx
@ stdcall CreateFormatEnumerator(long ptr ptr) CreateFormatEnumerator
@ stdcall CreateURLMoniker(ptr str ptr) CreateURLMoniker
@ stdcall DllCanUnloadNow() URLMON_DllCanUnloadNow
@ stdcall DllGetClassObject(ptr ptr ptr) URLMON_DllGetClassObject
@ stdcall DllInstall(long ptr) URLMON_DllInstall
@ stdcall DllRegisterServer() URLMON_DllRegisterServer
@ stdcall DllRegisterServerEx() URLMON_DllRegisterServerEx
@ stdcall DllUnregisterServer() URLMON_DllUnregisterServer
@ stdcall Extract(long long) Extract
@ stub FaultInIEFeature
@ stdcall FindMediaType(str ptr) FindMediaType
@ stdcall FindMediaTypeClass(ptr str ptr long) FindMediaTypeClass
@ stdcall FindMimeFromData(ptr wstr ptr long wstr long ptr long) FindMimeFromData
@ stdcall GetClassFileOrMime(ptr wstr ptr long wstr long ptr) GetClassFileOrMime
@ stdcall GetClassURL(wstr ptr) GetClassURL
@ stub GetComponentIDFromCLSSPEC
@ stub GetMarkOfTheWeb
@ stdcall GetSoftwareUpdateInfo(wstr ptr) GetSoftwareUpdateInfo
@ stdcall HlinkGoBack(ptr) HlinkGoBack
@ stdcall HlinkGoForward(ptr) HlinkGoForward
@ stdcall HlinkNavigateMoniker(ptr ptr) HlinkNavigateMoniker
@ stdcall HlinkNavigateString(ptr wstr) HlinkNavigateString
@ stdcall HlinkSimpleNavigateToMoniker(ptr wstr wstr ptr ptr ptr long long) HlinkSimpleNavigateToMoniker
@ stdcall HlinkSimpleNavigateToString(wstr wstr wstr ptr ptr ptr long long) HlinkSimpleNavigateToString
@ stdcall IsAsyncMoniker(ptr) IsAsyncMoniker
@ stdcall IsLoggingEnabledA(str) IsLoggingEnabledA
@ stdcall IsLoggingEnabledW(wstr) IsLoggingEnabledW
@ stdcall IsValidURL(ptr wstr long) IsValidURL
@ stdcall MkParseDisplayNameEx(ptr wstr ptr ptr) MkParseDisplayNameEx
@ stdcall ObtainUserAgentString(long str ptr) ObtainUserAgentString
@ stub PrivateCoInstall
@ stdcall RegisterBindStatusCallback(ptr ptr ptr long) RegisterBindStatusCallback
@ stdcall RegisterFormatEnumerator(ptr ptr long) RegisterFormatEnumerator
@ stdcall RegisterMediaTypeClass(ptr long ptr ptr long) RegisterMediaTypeClass
@ stdcall RegisterMediaTypes(long ptr ptr) RegisterMediaTypes
@ stdcall ReleaseBindInfo(ptr) ReleaseBindInfo
@ stdcall RevokeBindStatusCallback(ptr ptr) RevokeBindStatusCallback
@ stdcall RevokeFormatEnumerator(ptr ptr) RevokeFormatEnumerator
@ stdcall SetSoftwareUpdateAdvertisementState(wstr long long long) SetSoftwareUpdateAdvertisementState
@ stub URLDownloadA
@ stdcall URLDownloadToCacheFileA(ptr str ptr long long ptr) URLDownloadToCacheFileA
@ stdcall URLDownloadToCacheFileW(ptr wstr ptr long long ptr) URLDownloadToCacheFileW
@ stdcall URLDownloadToFileA(ptr str str long ptr) URLDownloadToFileA
@ stdcall URLDownloadToFileW(ptr wstr wstr long ptr) URLDownloadToFileW
@ stub URLDownloadW
@ stdcall URLOpenBlockingStreamA(ptr str ptr long ptr) URLOpenBlockingStreamA
@ stdcall URLOpenBlockingStreamW(ptr wstr ptr long ptr) URLOpenBlockingStreamW
@ stdcall URLOpenPullStreamA(ptr str long ptr) URLOpenPullStreamA
@ stdcall URLOpenPullStreamW(ptr wstr long ptr) URLOpenPullStreamW
@ stdcall URLOpenStreamA(ptr str long ptr) URLOpenStreamA
@ stdcall URLOpenStreamW(ptr wstr long ptr) URLOpenStreamW
@ stub UrlMkBuildVersion
@ stdcall UrlMkGetSessionOption(long ptr long ptr long) UrlMkGetSessionOption
@ stdcall UrlMkSetSessionOption(long ptr long long) UrlMkSetSessionOption
@ stdcall WriteHitLogging(ptr) WriteHitLogging
@ stub ZonesReInit

