@ stdcall GetAcceptLanguagesW(ptr ptr) kernelbase.GetAcceptLanguagesW
@ stdcall HashData(ptr long ptr long) kernelbase.HashData
@ stdcall IsInternetESCEnabled() kernelbase.IsInternetESCEnabled
@ stdcall ParseURLW(wstr ptr) kernelbase.ParseURLW
@ stdcall PathAddBackslashA(str) kernelbase.PathAddBackslashA
@ stdcall PathAddBackslashW(wstr) kernelbase.PathAddBackslashW
@ stdcall PathAddExtensionA(str str) kernelbase.PathAddExtensionA
@ stdcall PathAddExtensionW(wstr wstr) kernelbase.PathAddExtensionW
@ stdcall PathAppendA(str str) kernelbase.PathAppendA
@ stdcall PathAppendW(wstr wstr) kernelbase.PathAppendW
@ stdcall PathCanonicalizeA(ptr str) kernelbase.PathCanonicalizeA
@ stdcall PathCanonicalizeW(ptr wstr) kernelbase.PathCanonicalizeW
@ stdcall PathCommonPrefixA(str str ptr) kernelbase.PathCommonPrefixA
@ stdcall PathCommonPrefixW(wstr wstr ptr) kernelbase.PathCommonPrefixW
@ stdcall PathCreateFromUrlAlloc(wstr ptr long) kernelbase.PathCreateFromUrlAlloc
@ stdcall PathCreateFromUrlW(wstr ptr ptr long) kernelbase.PathCreateFromUrlW
@ stdcall PathFileExistsA(str) kernelbase.PathFileExistsA
@ stdcall PathFileExistsW(wstr) kernelbase.PathFileExistsW
@ stdcall PathFindExtensionA(str) kernelbase.PathFindExtensionA
@ stdcall PathFindExtensionW(wstr) kernelbase.PathFindExtensionW
@ stdcall PathFindFileNameA(str) kernelbase.PathFindFileNameA
@ stdcall PathFindFileNameW(wstr) kernelbase.PathFindFileNameW
@ stdcall PathFindNextComponentA(str) kernelbase.PathFindNextComponentA
@ stdcall PathFindNextComponentW(wstr) kernelbase.PathFindNextComponentW
@ stdcall PathGetArgsA(str) kernelbase.PathGetArgsA
@ stdcall PathGetArgsW(wstr) kernelbase.PathGetArgsW
@ stdcall PathGetDriveNumberA(str) kernelbase.PathGetDriveNumberA
@ stdcall PathGetDriveNumberW(wstr) kernelbase.PathGetDriveNumberW
@ stdcall PathIsFileSpecA(str) kernelbase.PathIsFileSpecA
@ stdcall PathIsFileSpecW(wstr) kernelbase.PathIsFileSpecW
@ stdcall PathIsLFNFileSpecA(str) kernelbase.PathIsLFNFileSpecA
@ stdcall PathIsLFNFileSpecW(wstr) kernelbase.PathIsLFNFileSpecW
@ stdcall PathIsPrefixA(str str) kernelbase.PathIsPrefixA
@ stdcall PathIsPrefixW(wstr wstr) kernelbase.PathIsPrefixW
@ stdcall PathIsRelativeA(str) kernelbase.PathIsRelativeA
@ stdcall PathIsRelativeW(wstr) kernelbase.PathIsRelativeW
@ stdcall PathIsRootA(str) kernelbase.PathIsRootA
@ stdcall PathIsRootW(wstr) kernelbase.PathIsRootW
@ stdcall PathIsSameRootA(str str) kernelbase.PathIsSameRootA
@ stdcall PathIsSameRootW(wstr wstr) kernelbase.PathIsSameRootW
@ stdcall PathIsUNCA(str) kernelbase.PathIsUNCA
@ stdcall PathIsUNCServerA(str) kernelbase.PathIsUNCServerA
@ stdcall PathIsUNCServerShareA(str) kernelbase.PathIsUNCServerShareA
@ stdcall PathIsUNCServerShareW(wstr) kernelbase.PathIsUNCServerShareW
@ stdcall PathIsUNCServerW(wstr) kernelbase.PathIsUNCServerW
@ stdcall PathIsUNCW(wstr) kernelbase.PathIsUNCW
@ stdcall PathIsURLW(wstr) kernelbase.PathIsURLW
@ stdcall PathParseIconLocationA(str) kernelbase.PathParseIconLocationA
@ stdcall PathParseIconLocationW(wstr) kernelbase.PathParseIconLocationW
@ stdcall PathRelativePathToA(ptr str long str long) kernelbase.PathRelativePathToA
@ stdcall PathRelativePathToW(ptr wstr long wstr long) kernelbase.PathRelativePathToW
@ stdcall PathRemoveBackslashA(str) kernelbase.PathRemoveBackslashA
@ stdcall PathRemoveBackslashW(wstr) kernelbase.PathRemoveBackslashW
@ stdcall PathRemoveBlanksA(str) kernelbase.PathRemoveBlanksA
@ stdcall PathRemoveBlanksW(wstr) kernelbase.PathRemoveBlanksW
@ stdcall PathRemoveExtensionA(str) kernelbase.PathRemoveExtensionA
@ stdcall PathRemoveExtensionW(wstr) kernelbase.PathRemoveExtensionW
@ stdcall PathRemoveFileSpecA(str) kernelbase.PathRemoveFileSpecA
@ stdcall PathRemoveFileSpecW(wstr) kernelbase.PathRemoveFileSpecW
@ stdcall PathRenameExtensionA(str str) kernelbase.PathRenameExtensionA
@ stdcall PathRenameExtensionW(wstr wstr) kernelbase.PathRenameExtensionW
@ stdcall PathSkipRootA(str) kernelbase.PathSkipRootA
@ stdcall PathSkipRootW(wstr) kernelbase.PathSkipRootW
@ stdcall PathStripPathA(str) kernelbase.PathStripPathA
@ stdcall PathStripPathW(wstr) kernelbase.PathStripPathW
@ stdcall PathStripToRootA(str) kernelbase.PathStripToRootA
@ stdcall PathStripToRootW(wstr) kernelbase.PathStripToRootW
@ stdcall PathUnquoteSpacesA(str) kernelbase.PathUnquoteSpacesA
@ stdcall PathUnquoteSpacesW(wstr) kernelbase.PathUnquoteSpacesW
@ stdcall QISearch(ptr ptr ptr ptr) kernelbase.QISearch
@ stdcall SHLoadIndirectString(wstr ptr long ptr) kernelbase.SHLoadIndirectString
@ stdcall SHRegCloseUSKey(ptr) kernelbase.SHRegCloseUSKey
@ stdcall SHRegDeleteUSValueA(long str long) kernelbase.SHRegDeleteUSValueA
@ stdcall SHRegDeleteUSValueW(long wstr long) kernelbase.SHRegDeleteUSValueW
@ stdcall SHRegEnumUSKeyA(long long str ptr long) kernelbase.SHRegEnumUSKeyA
@ stdcall SHRegEnumUSKeyW(long long wstr ptr long) kernelbase.SHRegEnumUSKeyW
@ stdcall SHRegGetBoolUSValueA(str str long long) kernelbase.SHRegGetBoolUSValueA
@ stdcall SHRegGetBoolUSValueW(wstr wstr long long) kernelbase.SHRegGetBoolUSValueW
@ stdcall SHRegGetUSValueA(str str ptr ptr ptr long ptr long) kernelbase.SHRegGetUSValueA
@ stdcall SHRegGetUSValueW(wstr wstr ptr ptr ptr long ptr long) kernelbase.SHRegGetUSValueW
@ stdcall SHRegOpenUSKeyA(str long long ptr long) kernelbase.SHRegOpenUSKeyA
@ stdcall SHRegOpenUSKeyW(wstr long long ptr long) kernelbase.SHRegOpenUSKeyW
@ stdcall SHRegQueryUSValueA(long str ptr ptr ptr long ptr long) kernelbase.SHRegQueryUSValueA
@ stdcall SHRegQueryUSValueW(long wstr ptr ptr ptr long ptr long) kernelbase.SHRegQueryUSValueW
@ stdcall SHRegSetUSValueA(str str long ptr long long) kernelbase.SHRegSetUSValueA
@ stdcall SHRegSetUSValueW(wstr wstr long ptr long long) kernelbase.SHRegSetUSValueW
@ stdcall StrCSpnA(str str) kernelbase.StrCSpnA
@ stdcall StrCSpnIA(str str) kernelbase.StrCSpnIA
@ stdcall StrCSpnIW(wstr wstr) kernelbase.StrCSpnIW
@ stdcall StrCSpnW(wstr wstr) kernelbase.StrCSpnW
@ stdcall StrCatBuffA(str str long) kernelbase.StrCatBuffA
@ stdcall StrCatBuffW(wstr wstr long) kernelbase.StrCatBuffW
@ stdcall StrCatChainW(ptr long long wstr) kernelbase.StrCatChainW
@ stdcall StrChrA(str long) kernelbase.StrChrA
@ stdcall StrChrIA(str long) kernelbase.StrChrIA
@ stdcall StrChrIW(wstr long) kernelbase.StrChrIW
@ stub StrChrNIW
@ stdcall StrChrNW(wstr long long) kernelbase.StrChrNW
@ stdcall StrChrW(wstr long) kernelbase.StrChrW
@ stdcall StrCmpCA(str str) kernelbase.StrCmpCA
@ stdcall StrCmpCW(wstr wstr) kernelbase.StrCmpCW
@ stdcall StrCmpICA(str str) kernelbase.StrCmpICA
@ stdcall StrCmpICW(wstr wstr) kernelbase.StrCmpICW
@ stdcall StrCmpIW(wstr wstr) kernelbase.StrCmpIW
@ stdcall StrCmpLogicalW(wstr wstr) kernelbase.StrCmpLogicalW
@ stdcall StrCmpNA(str str long) kernelbase.StrCmpNA
@ stdcall StrCmpNCA(str str long) kernelbase.StrCmpNCA
@ stdcall StrCmpNCW(wstr wstr long) kernelbase.StrCmpNCW
@ stdcall StrCmpNIA(str str long) kernelbase.StrCmpNIA
@ stdcall StrCmpNICA(str str long) kernelbase.StrCmpNICA
@ stdcall StrCmpNICW(wstr wstr long) kernelbase.StrCmpNICW
@ stdcall StrCmpNIW(wstr wstr long) kernelbase.StrCmpNIW
@ stdcall StrCmpNW(wstr wstr long) kernelbase.StrCmpNW
@ stdcall StrCmpW(wstr wstr) kernelbase.StrCmpW
@ stdcall StrCpyNW(ptr wstr long) kernelbase.StrCpyNW
@ stdcall StrDupA(str) kernelbase.StrDupA
@ stdcall StrDupW(wstr) kernelbase.StrDupW
@ stdcall StrIsIntlEqualA(long str str long) kernelbase.StrIsIntlEqualA
@ stdcall StrIsIntlEqualW(long wstr wstr long) kernelbase.StrIsIntlEqualW
@ stdcall StrPBrkA(str str) kernelbase.StrPBrkA
@ stdcall StrPBrkW(wstr wstr) kernelbase.StrPBrkW
@ stdcall StrRChrA(str str long) kernelbase.StrRChrA
@ stdcall StrRChrIA(str str long) kernelbase.StrRChrIA
@ stdcall StrRChrIW(wstr wstr long) kernelbase.StrRChrIW
@ stdcall StrRChrW(wstr wstr long) kernelbase.StrRChrW
@ stdcall StrRStrIA(str str str) kernelbase.StrRStrIA
@ stdcall StrRStrIW(wstr wstr wstr) kernelbase.StrRStrIW
@ stdcall StrSpnA(str str) kernelbase.StrSpnA
@ stdcall StrSpnW(wstr wstr) kernelbase.StrSpnW
@ stdcall StrStrA(str str) kernelbase.StrStrA
@ stdcall StrStrIA(str str) kernelbase.StrStrIA
@ stdcall StrStrIW(wstr wstr) kernelbase.StrStrIW
@ stdcall StrStrNIW(wstr wstr long) kernelbase.StrStrNIW
@ stdcall StrStrNW(wstr wstr long) kernelbase.StrStrNW
@ stdcall StrStrW(wstr wstr) kernelbase.StrStrW
@ stdcall StrToInt64ExA(str long ptr) kernelbase.StrToInt64ExA
@ stdcall StrToInt64ExW(wstr long ptr) kernelbase.StrToInt64ExW
@ stdcall StrToIntA(str) kernelbase.StrToIntA
@ stdcall StrToIntExA(str long ptr) kernelbase.StrToIntExA
@ stdcall StrToIntExW(wstr long ptr) kernelbase.StrToIntExW
@ stdcall StrToIntW(wstr) kernelbase.StrToIntW
@ stdcall StrTrimA(str str) kernelbase.StrTrimA
@ stdcall StrTrimW(wstr wstr) kernelbase.StrTrimW
@ stdcall UrlApplySchemeW(wstr ptr ptr long) kernelbase.UrlApplySchemeW
@ stdcall UrlCanonicalizeW(wstr ptr ptr long) kernelbase.UrlCanonicalizeW
@ stdcall UrlCombineA(str str ptr ptr long) kernelbase.UrlCombineA
@ stdcall UrlCombineW(wstr wstr ptr ptr long) kernelbase.UrlCombineW
@ stdcall UrlCreateFromPathW(wstr ptr ptr long) kernelbase.UrlCreateFromPathW
@ stdcall UrlEscapeW(wstr ptr ptr long) kernelbase.UrlEscapeW
@ stdcall UrlFixupW(wstr wstr long) kernelbase.UrlFixupW
@ stdcall UrlGetLocationW(wstr) kernelbase.UrlGetLocationW
@ stdcall UrlGetPartW(wstr ptr ptr long long) kernelbase.UrlGetPartW
@ stdcall UrlIsW(wstr long) kernelbase.UrlIsW
@ stdcall UrlUnescapeA(str ptr ptr long) kernelbase.UrlUnescapeA
@ stdcall UrlUnescapeW(wstr ptr ptr long) kernelbase.UrlUnescapeW
