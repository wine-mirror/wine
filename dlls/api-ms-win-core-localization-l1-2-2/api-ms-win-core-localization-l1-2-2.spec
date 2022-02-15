@ stdcall EnumSystemGeoID(long long ptr) kernelbase.EnumSystemGeoID
@ stdcall EnumSystemLocalesA(ptr long) kernelbase.EnumSystemLocalesA
@ stdcall EnumSystemLocalesEx(ptr long long ptr) kernelbase.EnumSystemLocalesEx
@ stdcall EnumSystemLocalesW(ptr long) kernelbase.EnumSystemLocalesW
@ stdcall FindNLSStringEx(wstr long wstr long wstr long ptr ptr ptr long) kernelbase.FindNLSStringEx
@ stdcall FormatMessageA(long ptr long long ptr long ptr) kernelbase.FormatMessageA
@ stdcall FormatMessageW(long ptr long long ptr long ptr) kernelbase.FormatMessageW
@ stdcall GetACP() kernelbase.GetACP
@ stdcall GetCalendarInfoEx(wstr long ptr long ptr long ptr) kernelbase.GetCalendarInfoEx
@ stdcall GetCPInfo(long ptr) kernelbase.GetCPInfo
@ stdcall GetCPInfoExW(long long ptr) kernelbase.GetCPInfoExW
@ stdcall GetGeoInfoW(long long ptr long long) kernelbase.GetGeoInfoW
@ stdcall GetLocaleInfoA(long long ptr long) kernelbase.GetLocaleInfoA
@ stdcall GetLocaleInfoEx(wstr long ptr long) kernelbase.GetLocaleInfoEx
@ stdcall GetLocaleInfoW(long long ptr long) kernelbase.GetLocaleInfoW
@ stdcall GetNLSVersionEx(long wstr ptr) kernelbase.GetNLSVersionEx
@ stdcall GetOEMCP() kernelbase.GetOEMCP
@ stdcall GetSystemDefaultLangID() kernelbase.GetSystemDefaultLangID
@ stdcall GetSystemDefaultLCID() kernelbase.GetSystemDefaultLCID
@ stdcall GetSystemDefaultLocaleName(ptr long) kernelbase.GetSystemDefaultLocaleName
@ stdcall GetThreadLocale() kernelbase.GetThreadLocale
@ stdcall GetUserDefaultLangID() kernelbase.GetUserDefaultLangID
@ stdcall GetUserDefaultLCID() kernelbase.GetUserDefaultLCID
@ stdcall GetUserDefaultLocaleName(ptr long) kernelbase.GetUserDefaultLocaleName
@ stdcall GetUserGeoID(long) kernelbase.GetUserGeoID
@ stdcall IdnToAscii(long wstr long ptr long) kernelbase.IdnToAscii
@ stdcall IdnToUnicode(long wstr long ptr long) kernelbase.IdnToUnicode
@ stdcall IsDBCSLeadByte(long) kernelbase.IsDBCSLeadByte
@ stdcall IsDBCSLeadByteEx(long long) kernelbase.IsDBCSLeadByteEx
@ stub IsNLSDefinedString
@ stdcall IsValidCodePage(long) kernelbase.IsValidCodePage
@ stdcall IsValidLocale(long long) kernelbase.IsValidLocale
@ stdcall IsValidLocaleName(wstr) kernelbase.IsValidLocaleName
@ stdcall IsValidNLSVersion(long wstr ptr) kernelbase.IsValidNLSVersion
@ stdcall LCIDToLocaleName(long ptr long long) kernelbase.LCIDToLocaleName
@ stdcall LCMapStringA(long long str long ptr long) kernelbase.LCMapStringA
@ stdcall LCMapStringEx(wstr long wstr long ptr long ptr ptr long) kernelbase.LCMapStringEx
@ stdcall LCMapStringW(long long wstr long ptr long) kernelbase.LCMapStringW
@ stdcall LocaleNameToLCID(wstr long) kernelbase.LocaleNameToLCID
@ stdcall ResolveLocaleName(wstr ptr long) kernelbase.ResolveLocaleName
@ stdcall VerLanguageNameA(long str long) kernelbase.VerLanguageNameA
@ stdcall VerLanguageNameW(long wstr long) kernelbase.VerLanguageNameW
