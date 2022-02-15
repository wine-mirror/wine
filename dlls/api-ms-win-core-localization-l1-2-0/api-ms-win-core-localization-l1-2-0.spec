@ stdcall ConvertDefaultLocale(long) kernelbase.ConvertDefaultLocale
@ stdcall EnumSystemGeoID(long long ptr) kernelbase.EnumSystemGeoID
@ stdcall EnumSystemLocalesA(ptr long) kernelbase.EnumSystemLocalesA
@ stdcall EnumSystemLocalesW(ptr long) kernelbase.EnumSystemLocalesW
@ stdcall FindNLSString(long long wstr long wstr long ptr) kernelbase.FindNLSString
@ stdcall FindNLSStringEx(wstr long wstr long wstr long ptr ptr ptr long) kernelbase.FindNLSStringEx
@ stdcall FormatMessageA(long ptr long long ptr long ptr) kernelbase.FormatMessageA
@ stdcall FormatMessageW(long ptr long long ptr long ptr) kernelbase.FormatMessageW
@ stdcall GetACP() kernelbase.GetACP
@ stdcall GetCPInfo(long ptr) kernelbase.GetCPInfo
@ stdcall GetCPInfoExW(long long ptr) kernelbase.GetCPInfoExW
@ stdcall GetCalendarInfoEx(wstr long ptr long ptr long ptr) kernelbase.GetCalendarInfoEx
@ stdcall GetCalendarInfoW(long long long ptr long ptr) kernelbase.GetCalendarInfoW
@ stdcall GetFileMUIInfo(long wstr ptr ptr) kernelbase.GetFileMUIInfo
@ stdcall GetFileMUIPath(long wstr wstr ptr ptr ptr ptr) kernelbase.GetFileMUIPath
@ stdcall GetGeoInfoW(long long ptr long long) kernelbase.GetGeoInfoW
@ stdcall GetLocaleInfoA(long long ptr long) kernelbase.GetLocaleInfoA
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
@ stdcall GetUserDefaultLocaleName(ptr long) kernelbase.GetUserDefaultLocaleName
@ stdcall GetUserGeoID(long) kernelbase.GetUserGeoID
@ stdcall GetUserPreferredUILanguages(long ptr ptr ptr) kernelbase.GetUserPreferredUILanguages
@ stdcall IdnToAscii(long wstr long ptr long) kernelbase.IdnToAscii
@ stdcall IdnToUnicode(long wstr long ptr long) kernelbase.IdnToUnicode
@ stdcall IsDBCSLeadByte(long) kernelbase.IsDBCSLeadByte
@ stdcall IsDBCSLeadByteEx(long long) kernelbase.IsDBCSLeadByteEx
@ stub IsNLSDefinedString
@ stdcall IsValidCodePage(long) kernelbase.IsValidCodePage
@ stdcall IsValidLanguageGroup(long long) kernelbase.IsValidLanguageGroup
@ stdcall IsValidLocale(long long) kernelbase.IsValidLocale
@ stdcall IsValidLocaleName(wstr) kernelbase.IsValidLocaleName
@ stdcall IsValidNLSVersion(long wstr ptr) kernelbase.IsValidNLSVersion
@ stdcall LCMapStringA(long long str long ptr long) kernelbase.LCMapStringA
@ stdcall LCMapStringEx(wstr long wstr long ptr long ptr ptr long) kernelbase.LCMapStringEx
@ stdcall LCMapStringW(long long wstr long ptr long) kernelbase.LCMapStringW
@ stdcall LocaleNameToLCID(wstr long) kernelbase.LocaleNameToLCID
@ stdcall ResolveLocaleName(wstr ptr long) kernelbase.ResolveLocaleName
@ stdcall SetCalendarInfoW(long long long wstr) kernelbase.SetCalendarInfoW
@ stdcall SetLocaleInfoW(long long wstr) kernelbase.SetLocaleInfoW
@ stdcall SetProcessPreferredUILanguages(long ptr ptr) kernelbase.SetProcessPreferredUILanguages
@ stdcall SetThreadLocale(long) kernelbase.SetThreadLocale
@ stdcall SetThreadPreferredUILanguages(long ptr ptr) kernelbase.SetThreadPreferredUILanguages
@ stdcall SetThreadUILanguage(long) kernelbase.SetThreadUILanguage
@ stdcall SetUserGeoID(long) kernelbase.SetUserGeoID
@ stdcall VerLanguageNameA(long str long) kernelbase.VerLanguageNameA
@ stdcall VerLanguageNameW(long wstr long) kernelbase.VerLanguageNameW
