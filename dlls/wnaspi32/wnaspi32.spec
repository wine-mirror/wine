# we have several ordinal clashes here, it seems...

1 cdecl GetASPI32SupportInfo()
2 cdecl SendASPI32Command(ptr)
4 cdecl GetASPI32DLLVersion()

# 5 is the ordinal used by Adaptec's WNASPI32 DLL
#5 cdecl GetASPI32DLLVersion()
6 stub RegisterWOWPost
7 cdecl TranslateASPI32Address(ptr ptr)
8 cdecl GetASPI32Buffer(ptr)
# FreeASPI32Buffer: ord clash with GetASPI32DLLVersion (4), so use 14 instead
14 cdecl FreeASPI32Buffer(ptr)
