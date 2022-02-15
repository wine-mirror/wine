@ stdcall ConvertDefaultLocale(long) kernelbase.ConvertDefaultLocale
@ stdcall FindNLSString(long long wstr long wstr long ptr) kernelbase.FindNLSString
@ stdcall FindNLSStringEx(wstr long wstr long wstr long ptr ptr ptr long) kernelbase.FindNLSStringEx
@ stdcall GetACP() kernelbase.GetACP
@ stub GetCPFileNameFromRegistry
@ stdcall GetCPInfo(long ptr) kernelbase.GetCPInfo
@ stdcall GetCPInfoExW(long long ptr) kernelbase.GetCPInfoExW
@ stdcall GetCalendarInfoEx(wstr long ptr long ptr long ptr) kernelbase.GetCalendarInfoEx
@ stdcall GetCalendarInfoW(long long long ptr long ptr) kernelbase.GetCalendarInfoW
@ stdcall GetFileMUIInfo(long wstr ptr ptr) kernelbase.GetFileMUIInfo
@ stdcall GetFileMUIPath(long wstr wstr ptr ptr ptr ptr) kernelbase.GetFileMUIPath
@ stdcall GetLocaleInfoEx(wstr long ptr long) kernelbase.GetLocaleInfoEx
@ stdcall GetLocaleInfoW(long long ptr long) kernelbase.GetLocaleInfoW
@ stdcall GetNLSVersion(long long ptr) kernelbase.GetNLSVersion
@ stdcall GetNLSVersionEx(long wstr ptr) kernelbase.GetNLSVersionEx
@ stdcall GetOEMCP() kernelbase.GetOEMCP
@ stdcall GetProcessPreferredUILanguages(long ptr ptr ptr) kernelbase.GetProcessPreferredUILanguages
@ stdcall GetSystemDefaultLCID() kernelbase.GetSystemDefaultLCID
@ stdcall GetSystemDefaultLangID() kernelbase.GetSystemDefaultLangID
@ stdcall GetSystemPreferredUILanguages(long ptr ptr ptr) kernelbase.GetSystemPreferredUILanguages
@ stdcall GetThreadLocale() kernelbase.GetThreadLocale
@ stdcall GetThreadPreferredUILanguages(long ptr ptr ptr) kernelbase.GetThreadPreferredUILanguages
@ stdcall GetThreadUILanguage() kernelbase.GetThreadUILanguage
@ stub GetUILanguageInfo
@ stdcall GetUserDefaultLCID() kernelbase.GetUserDefaultLCID
@ stdcall GetUserDefaultLangID() kernelbase.GetUserDefaultLangID
@ stdcall GetUserPreferredUILanguages(long ptr ptr ptr) kernelbase.GetUserPreferredUILanguages
@ stub IsNLSDefinedString
@ stdcall IsValidCodePage(long) kernelbase.IsValidCodePage
@ stdcall IsValidLanguageGroup(long long) kernelbase.IsValidLanguageGroup
@ stdcall IsValidLocale(long long) kernelbase.IsValidLocale
@ stdcall IsValidLocaleName(wstr) kernelbase.IsValidLocaleName
@ stdcall LCMapStringEx(wstr long wstr long ptr long ptr ptr long) kernelbase.LCMapStringEx
@ stdcall LCMapStringW(long long wstr long ptr long) kernelbase.LCMapStringW
@ stdcall LocaleNameToLCID(wstr long) kernelbase.LocaleNameToLCID
@ stub NlsCheckPolicy
@ stub NlsEventDataDescCreate
@ stub NlsGetCacheUpdateCount
@ stub NlsUpdateLocale
@ stub NlsUpdateSystemLocale
@ stub NlsWriteEtwEvent
@ stdcall ResolveLocaleName(wstr ptr long) kernelbase.ResolveLocaleName
@ stdcall SetCalendarInfoW(long long long wstr) kernelbase.SetCalendarInfoW
@ stdcall SetLocaleInfoW(long long wstr) kernelbase.SetLocaleInfoW
@ stdcall SetThreadLocale(long) kernelbase.SetThreadLocale
@ stdcall VerLanguageNameA(long str long) kernelbase.VerLanguageNameA
@ stdcall VerLanguageNameW(long wstr long) kernelbase.VerLanguageNameW
