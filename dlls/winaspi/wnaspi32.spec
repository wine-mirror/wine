# we have several ordinal clashes here, it seems...

1 cdecl GetASPI32SupportInfo() GetASPI32SupportInfo
2 cdecl SendASPI32Command(ptr) SendASPI32Command
4 cdecl GetASPI32DLLVersion() GetASPI32DLLVersion

# 5 is the ordinal used by Adaptec's WNASPI32 DLL
#5 cdecl GetASPI32DLLVersion() GetASPI32DLLVersion
6 stub RegisterWOWPost
7 cdecl TranslateASPI32Address(ptr ptr) TranslateASPI32Address
8 cdecl GetASPI32Buffer(ptr) GetASPI32Buffer
# FreeASPI32Buffer: ord clash with GetASPI32DLLVersion (4), so use 14 instead
14 cdecl FreeASPI32Buffer(ptr) FreeASPI32Buffer
