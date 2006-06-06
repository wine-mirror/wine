3 stdcall HlinkCreateFromMoniker(ptr wstr wstr ptr long ptr ptr ptr)
4 stdcall HlinkCreateFromString(wstr wstr wstr ptr long ptr ptr ptr)
5 stdcall HlinkCreateFromData(ptr ptr long ptr ptr ptr)
6 stdcall HlinkCreateBrowseContext(ptr ptr ptr)
7 stub HlinkClone
8 stub HlinkNavigateToStringReference
9 stdcall HlinkOnNavigate(ptr ptr long ptr wstr wstr ptr)
10 stdcall HlinkNavigate(ptr ptr long ptr ptr ptr)
11 stub HlinkUpdateStackItem
12 stub HlinkOnRenameDocument
14 stub HlinkResolveMonikerForData
15 stub HlinkResolveStringForData
16 stub OleSaveToStreamEx
18 stub HlinkParseDisplayName
20 stub HlinkQueryCreateFromData
21 stub HlinkSetSpecialReference
22 stub HlinkGetSpecialReference
23 stub HlinkCreateShortcut
24 stub HlinkResolveShortcut
25 stub HlinkIsShortcut
26 stub HlinkResolveShortcutToString
27 stub HlinkCreateShortcutFromString
28 stub HlinkGetValueFromParams
29 stub HlinkCreateShortcutFromMoniker
30 stub HlinkResolveShortcutToMoniker
31 stub HlinkTranslateURL
32 stdcall HlinkCreateExtensionServices(wstr long wstr wstr ptr ptr ptr)
33 stub HlinkPreprocessMoniker

@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stub -private DllRegisterServer
@ stub -private DllUnregisterServer
