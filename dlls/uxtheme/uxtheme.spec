# Undocumented functions - Names derived from debug symbols
1  stdcall QueryThemeServices()
2  stdcall OpenThemeFile(wstr wstr wstr ptr long)
3  stdcall CloseThemeFile(ptr)
4  stdcall ApplyTheme(ptr ptr ptr)
7  stdcall GetThemeDefaults(wstr wstr long wstr long)
8  stdcall EnumThemes(wstr ptr ptr)
9  stdcall EnumThemeColors(wstr wstr long wstr)
10 stdcall EnumThemeSizes(wstr wstr long wstr)
11 stdcall ParseThemeIniFile(wstr wstr ptr ptr)
13 stub DrawNCPreview
14 stub RegisterDefaultTheme
15 stub DumpLoadedThemeToTextFile
16 stub OpenThemeDataFromFile
17 stub OpenThemeFileFromData
18 stub GetThemeSysSize96
19 stub GetThemeSysFont96
20 stub SessionAllocate
21 stub SessionFree
22 stub ThemeHooksOn
23 stub ThemeHooksOff
24 stub AreThemeHooksActive
25 stub GetCurrentChangeNumber
26 stub GetNewChangeNumber
27 stub SetGlobalTheme
28 stub GetGlobalTheme
29 stub CheckThemeSignature
30 stub LoadTheme
31 stub InitUserTheme
32 stub InitUserRegistry
33 stub ReestablishServerConnection
34 stub ThemeHooksInstall
35 stub ThemeHooksRemove
36 stub RefreshThemeForTS
43 stub ClassicGetSystemMetrics
44 stub ClassicSystemParametersInfoA
45 stub ClassicSystemParametersInfoW
46 stub ClassicAdjustWindowRectEx
48 stub GetThemeParseErrorInfo
60 stub CreateThemeDataFromObjects
61 stub OpenThemeDataEx
62 stub ServerClearStockObjects
63 stub MarkSelection

# Standard functions
@ stdcall CloseThemeData(ptr)
@ stdcall DrawThemeBackground(ptr ptr long long ptr ptr)
@ stdcall DrawThemeBackgroundEx(ptr ptr long long ptr ptr)
@ stdcall DrawThemeEdge(ptr ptr long long ptr long long ptr)
@ stdcall DrawThemeIcon(ptr ptr long long ptr ptr long)
@ stdcall DrawThemeParentBackground(ptr ptr ptr)
@ stdcall DrawThemeText(ptr ptr long long wstr long long long ptr)
@ stdcall EnableThemeDialogTexture(ptr long)
@ stdcall EnableTheming(long)
@ stdcall GetCurrentThemeName(wstr long wstr long wstr long)
@ stdcall GetThemeAppProperties()
@ stdcall GetThemeBackgroundContentRect(ptr ptr long long ptr ptr)
@ stdcall GetThemeBackgroundExtent(ptr ptr long long ptr ptr)
@ stdcall GetThemeBackgroundRegion(ptr ptr long long ptr ptr)
@ stdcall GetThemeBool(ptr long long long ptr)
@ stdcall GetThemeColor(ptr long long long ptr)
@ stdcall GetThemeDocumentationProperty(wstr wstr wstr long)
@ stdcall GetThemeEnumValue(ptr long long long ptr)
@ stdcall GetThemeFilename(ptr long long long wstr long)
@ stdcall GetThemeFont(ptr ptr long long long ptr)
@ stdcall GetThemeInt(ptr long long long ptr)
@ stdcall GetThemeIntList(ptr long long long ptr)
@ stdcall GetThemeMargins(ptr ptr long long long ptr ptr)
@ stdcall GetThemeMetric(ptr ptr long long long ptr)
@ stdcall GetThemePartSize(ptr ptr long long ptr long ptr)
@ stdcall GetThemePosition(ptr long long long ptr)
@ stdcall GetThemePropertyOrigin(ptr long long long ptr)
@ stdcall GetThemeRect(ptr long long long ptr)
@ stdcall GetThemeString(ptr long long long wstr long)
@ stdcall GetThemeSysBool(ptr long)
@ stdcall GetThemeSysColor(ptr long)
@ stdcall GetThemeSysColorBrush(ptr long)
@ stdcall GetThemeSysFont(ptr long ptr)
@ stdcall GetThemeSysInt(ptr long ptr)
@ stdcall GetThemeSysSize(ptr long)
@ stdcall GetThemeSysString(ptr long wstr long)
@ stdcall GetThemeTextExtent(ptr ptr long long wstr long long ptr ptr)
@ stdcall GetThemeTextMetrics(ptr ptr long long ptr)
@ stdcall GetWindowTheme(ptr)
@ stdcall HitTestThemeBackground(ptr ptr long long long ptr ptr long ptr)
@ stdcall IsAppThemed()
@ stdcall IsThemeActive()
@ stdcall IsThemeBackgroundPartiallyTransparent(ptr long long)
@ stdcall IsThemeDialogTextureEnabled()
@ stdcall IsThemePartDefined(ptr long long)
@ stdcall OpenThemeData(ptr wstr)
@ stdcall SetThemeAppProperties(long)
@ stdcall SetWindowTheme(ptr wstr wstr)
