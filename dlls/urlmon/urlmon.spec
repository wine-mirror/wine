init URLMON_DllEntryPoint

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
@ stub CreateFormatEnumerator
@ stdcall CreateURLMoniker(ptr wstr ptr) CreateURLMoniker
@ stdcall DllCanUnloadNow() URLMON_DllCanUnloadNow
@ stdcall DllGetClassObject(ptr ptr ptr) URLMON_DllGetClassObject
@ stdcall DllInstall(long ptr) URLMON_DllInstall
@ stdcall DllRegisterServer() URLMON_DllRegisterServer
@ stdcall DllRegisterServerEx() URLMON_DllRegisterServerEx
@ stdcall DllUnregisterServer() URLMON_DllUnregisterServer
@ stdcall Extract(long str) Extract
@ stub FaultInIEFeature
@ stub FindMediaType
@ stub FindMediaTypeClass
@ stdcall FindMimeFromData(long ptr ptr long ptr long ptr long) FindMimeFromData 
@ stub GetClassFileOrMime
@ stub GetClassURL
@ stub GetComponentIDFromCLSSPEC
@ stub GetMarkOfTheWeb
@ stub GetSoftwareUpdateInfo
@ stub HlinkGoBack
@ stub HlinkGoForward
@ stub HlinkNavigateMoniker
@ stub HlinkNavigateString
@ stub HlinkSimpleNavigateToMoniker
@ stub HlinkSimpleNavigateToString
@ stub IsAsyncMoniker
@ stub IsLoggingEnabledA
@ stub IsLoggingEnabledW
@ stub IsValidURL
@ forward MkParseDisplayNameEx ole32.MkParseDisplayName
@ stdcall ObtainUserAgentString(long str ptr) ObtainUserAgentString
@ stub PrivateCoInstall
@ stdcall RegisterBindStatusCallback(ptr ptr ptr long) RegisterBindStatusCallback
@ stub RegisterFormatEnumerator
@ stub RegisterMediaTypeClass
@ stub RegisterMediaTypes
@ stdcall ReleaseBindInfo(ptr) ReleaseBindInfo
@ stdcall RevokeBindStatusCallback(ptr ptr) RevokeBindStatusCallback
@ stub RevokeFormatEnumerator
@ stub SetSoftwareUpdateAdvertisementState
@ stub URLDownloadA
@ stub URLDownloadToCacheFileA
@ stub URLDownloadToCacheFileW
@ stub URLDownloadToFileA
@ stub URLDownloadToFileW
@ stub URLDownloadW
@ stub URLOpenBlockingStreamA
@ stub URLOpenBlockingStreamW
@ stub URLOpenPullStreamA
@ stub URLOpenPullStreamW
@ stub URLOpenStreamA
@ stub URLOpenStreamW
@ stub UrlMkBuildVersion
@ stub UrlMkGetSessionOption
@ stdcall UrlMkSetSessionOption(long ptr long long) UrlMkSetSessionOption
@ stub WriteHitLogging
@ stub ZonesReInit
