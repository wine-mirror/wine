@ stdcall GetAcceptLanguagesW(ptr ptr) shlwapi.GetAcceptLanguagesW
@ stdcall HashData(ptr long ptr long) shlwapi.HashData
@ stdcall IsInternetESCEnabled() shlwapi.IsInternetESCEnabled
@ stdcall ParseURLW(wstr ptr) shlwapi.ParseURLW
@ stdcall PathAddBackslashA(str) shlwapi.PathAddBackslashA
@ stdcall PathAddBackslashW(wstr) shlwapi.PathAddBackslashW
@ stdcall PathAddExtensionA(str str) shlwapi.PathAddExtensionA
@ stdcall PathAddExtensionW(wstr wstr) shlwapi.PathAddExtensionW
@ stdcall PathAppendA(str str) shlwapi.PathAppendA
@ stdcall PathAppendW(wstr wstr) shlwapi.PathAppendW
@ stdcall PathCanonicalizeA(ptr str) shlwapi.PathCanonicalizeA
@ stdcall PathCanonicalizeW(ptr wstr) shlwapi.PathCanonicalizeW
@ stdcall PathCommonPrefixA(str str ptr) shlwapi.PathCommonPrefixA
@ stdcall PathCommonPrefixW(wstr wstr ptr) shlwapi.PathCommonPrefixW
@ stdcall PathCreateFromUrlAlloc(wstr ptr long) shlwapi.PathCreateFromUrlAlloc
@ stdcall PathCreateFromUrlW(wstr ptr ptr long) shlwapi.PathCreateFromUrlW
@ stdcall PathFileExistsA(str) shlwapi.PathFileExistsA
@ stdcall PathFileExistsW(wstr) shlwapi.PathFileExistsW
@ stdcall PathFindExtensionA(str) shlwapi.PathFindExtensionA
@ stdcall PathFindExtensionW(wstr) shlwapi.PathFindExtensionW
@ stdcall PathFindFileNameA(str) shlwapi.PathFindFileNameA
@ stdcall PathFindFileNameW(wstr) shlwapi.PathFindFileNameW
@ stdcall PathFindNextComponentA(str) shlwapi.PathFindNextComponentA
@ stdcall PathFindNextComponentW(wstr) shlwapi.PathFindNextComponentW
@ stdcall PathGetArgsA(str) shlwapi.PathGetArgsA
@ stdcall PathGetArgsW(wstr) shlwapi.PathGetArgsW
@ stdcall PathGetDriveNumberA(str) shlwapi.PathGetDriveNumberA
@ stdcall PathGetDriveNumberW(wstr) shlwapi.PathGetDriveNumberW
@ stdcall PathIsFileSpecA(str) shlwapi.PathIsFileSpecA
@ stdcall PathIsFileSpecW(wstr) shlwapi.PathIsFileSpecW
@ stdcall PathIsLFNFileSpecA(str) shlwapi.PathIsLFNFileSpecA
@ stdcall PathIsLFNFileSpecW(wstr) shlwapi.PathIsLFNFileSpecW
@ stdcall PathIsPrefixA(str str) shlwapi.PathIsPrefixA
@ stdcall PathIsPrefixW(wstr wstr) shlwapi.PathIsPrefixW
@ stdcall PathIsRelativeA(str) shlwapi.PathIsRelativeA
@ stdcall PathIsRelativeW(wstr) shlwapi.PathIsRelativeW
@ stdcall PathIsRootA(str) shlwapi.PathIsRootA
@ stdcall PathIsRootW(wstr) shlwapi.PathIsRootW
@ stdcall PathIsSameRootA(str str) shlwapi.PathIsSameRootA
@ stdcall PathIsSameRootW(wstr wstr) shlwapi.PathIsSameRootW
@ stdcall PathIsUNCA(str) shlwapi.PathIsUNCA
@ stdcall PathIsUNCServerA(str) shlwapi.PathIsUNCServerA
@ stdcall PathIsUNCServerShareA(str) shlwapi.PathIsUNCServerShareA
@ stdcall PathIsUNCServerShareW(wstr) shlwapi.PathIsUNCServerShareW
@ stdcall PathIsUNCServerW(wstr) shlwapi.PathIsUNCServerW
@ stdcall PathIsUNCW(wstr) shlwapi.PathIsUNCW
@ stdcall PathIsURLW(wstr) shlwapi.PathIsURLW
@ stdcall PathParseIconLocationA(str) shlwapi.PathParseIconLocationA
@ stdcall PathParseIconLocationW(wstr) shlwapi.PathParseIconLocationW
@ stdcall PathRelativePathToA(ptr str long str long) shlwapi.PathRelativePathToA
@ stdcall PathRelativePathToW(ptr wstr long wstr long) shlwapi.PathRelativePathToW
@ stdcall PathRemoveBackslashA(str) shlwapi.PathRemoveBackslashA
@ stdcall PathRemoveBackslashW(wstr) shlwapi.PathRemoveBackslashW
@ stdcall PathRemoveBlanksA(str) shlwapi.PathRemoveBlanksA
@ stdcall PathRemoveBlanksW(wstr) shlwapi.PathRemoveBlanksW
@ stdcall PathRemoveExtensionA(str) shlwapi.PathRemoveExtensionA
@ stdcall PathRemoveExtensionW(wstr) shlwapi.PathRemoveExtensionW
@ stdcall PathRemoveFileSpecA(str) shlwapi.PathRemoveFileSpecA
@ stdcall PathRemoveFileSpecW(wstr) shlwapi.PathRemoveFileSpecW
@ stdcall PathRenameExtensionA(str str) shlwapi.PathRenameExtensionA
@ stdcall PathRenameExtensionW(wstr wstr) shlwapi.PathRenameExtensionW
@ stdcall PathSkipRootA(str) shlwapi.PathSkipRootA
@ stdcall PathSkipRootW(wstr) shlwapi.PathSkipRootW
@ stdcall PathStripPathA(str) shlwapi.PathStripPathA
@ stdcall PathStripPathW(wstr) shlwapi.PathStripPathW
@ stdcall PathStripToRootA(str) shlwapi.PathStripToRootA
@ stdcall PathStripToRootW(wstr) shlwapi.PathStripToRootW
@ stdcall PathUnquoteSpacesA(str) shlwapi.PathUnquoteSpacesA
@ stdcall PathUnquoteSpacesW(wstr) shlwapi.PathUnquoteSpacesW
@ stdcall QISearch(ptr ptr ptr ptr) shlwapi.QISearch
@ stdcall SHLoadIndirectString(wstr ptr long ptr) shlwapi.SHLoadIndirectString
@ stdcall SHRegCloseUSKey(ptr) shlwapi.SHRegCloseUSKey
@ stdcall SHRegDeleteUSValueA(long str long) shlwapi.SHRegDeleteUSValueA
@ stdcall SHRegDeleteUSValueW(long wstr long) shlwapi.SHRegDeleteUSValueW
@ stdcall SHRegEnumUSKeyA(long long str ptr long) shlwapi.SHRegEnumUSKeyA
@ stdcall SHRegEnumUSKeyW(long long wstr ptr long) shlwapi.SHRegEnumUSKeyW
@ stdcall SHRegGetBoolUSValueA(str str long long) shlwapi.SHRegGetBoolUSValueA
@ stdcall SHRegGetBoolUSValueW(wstr wstr long long) shlwapi.SHRegGetBoolUSValueW
@ stdcall SHRegGetUSValueA( str str ptr ptr ptr long ptr long ) shlwapi.SHRegGetUSValueA
@ stdcall SHRegGetUSValueW( wstr wstr ptr ptr ptr long ptr long ) shlwapi.SHRegGetUSValueW
@ stdcall SHRegOpenUSKeyA( str long long ptr long ) shlwapi.SHRegOpenUSKeyA
@ stdcall SHRegOpenUSKeyW( wstr long long ptr long ) shlwapi.SHRegOpenUSKeyW
@ stdcall SHRegQueryUSValueA( long str ptr ptr ptr long ptr long ) shlwapi.SHRegQueryUSValueA
@ stdcall SHRegQueryUSValueW( long wstr ptr ptr ptr long ptr long ) shlwapi.SHRegQueryUSValueW
@ stdcall SHRegSetUSValueA( str str long ptr long long) shlwapi.SHRegSetUSValueA
@ stdcall SHRegSetUSValueW( wstr wstr long ptr long long) shlwapi.SHRegSetUSValueW
@ stdcall StrCSpnA(str str) shlwapi.StrCSpnA
@ stdcall StrCSpnIA(str str) shlwapi.StrCSpnIA
@ stdcall StrCSpnIW(wstr wstr) shlwapi.StrCSpnIW
@ stdcall StrCSpnW(wstr wstr) shlwapi.StrCSpnW
@ stdcall StrCatBuffA(str str long) shlwapi.StrCatBuffA
@ stdcall StrCatBuffW(wstr wstr long) shlwapi.StrCatBuffW
@ stdcall StrCatChainW(ptr long long wstr) shlwapi.StrCatChainW
@ stdcall StrChrA(str long) shlwapi.StrChrA
@ stdcall StrChrIA(str long) shlwapi.StrChrIA
@ stdcall StrChrIW(wstr long) shlwapi.StrChrIW
@ stub StrChrNIW
@ stdcall StrChrNW(wstr long long) shlwapi.StrChrNW
@ stdcall StrChrW(wstr long) shlwapi.StrChrW
@ stdcall StrCmpCA(str str) shlwapi.StrCmpCA
@ stdcall StrCmpCW(wstr wstr) shlwapi.StrCmpCW
@ stdcall StrCmpICA(str str) shlwapi.StrCmpICA
@ stdcall StrCmpICW(wstr wstr) shlwapi.StrCmpICW
@ stdcall StrCmpIW(wstr wstr) shlwapi.StrCmpIW
@ stdcall StrCmpLogicalW(wstr wstr) shlwapi.StrCmpLogicalW
@ stdcall StrCmpNA(str str long) shlwapi.StrCmpNA
@ stdcall StrCmpNCA(str str long) shlwapi.StrCmpNCA
@ stdcall StrCmpNCW(wstr wstr long) shlwapi.StrCmpNCW
@ stdcall StrCmpNIA(str str long) shlwapi.StrCmpNIA
@ stdcall StrCmpNICA(str str long) shlwapi.StrCmpNICA
@ stdcall StrCmpNICW(wstr wstr long) shlwapi.StrCmpNICW
@ stdcall StrCmpNIW(wstr wstr long) shlwapi.StrCmpNIW
@ stdcall StrCmpNW(wstr wstr long) shlwapi.StrCmpNW
@ stdcall StrCmpW(wstr wstr) shlwapi.StrCmpW
@ stdcall StrCpyNW(ptr wstr long) shlwapi.StrCpyNW
@ stdcall StrDupA(str) shlwapi.StrDupA
@ stdcall StrDupW(wstr) shlwapi.StrDupW
@ stdcall StrIsIntlEqualA(long str str long) shlwapi.StrIsIntlEqualA
@ stdcall StrIsIntlEqualW(long wstr wstr long) shlwapi.StrIsIntlEqualW
@ stdcall StrPBrkA(str str) shlwapi.StrPBrkA
@ stdcall StrPBrkW(wstr wstr) shlwapi.StrPBrkW
@ stdcall StrRChrA(str str long) shlwapi.StrRChrA
@ stdcall StrRChrIA(str str long) shlwapi.StrRChrIA
@ stdcall StrRChrIW(wstr wstr long) shlwapi.StrRChrIW
@ stdcall StrRChrW(wstr wstr long) shlwapi.StrRChrW
@ stdcall StrRStrIA(str str str) shlwapi.StrRStrIA
@ stdcall StrRStrIW(wstr wstr wstr) shlwapi.StrRStrIW
@ stdcall StrSpnA(str str) shlwapi.StrSpnA
@ stdcall StrSpnW(wstr wstr) shlwapi.StrSpnW
@ stdcall StrStrA(str str) shlwapi.StrStrA
@ stdcall StrStrIA(str str) shlwapi.StrStrIA
@ stdcall StrStrIW(wstr wstr) shlwapi.StrStrIW
@ stdcall StrStrNIW(wstr wstr long) shlwapi.StrStrNIW
@ stdcall StrStrNW(wstr wstr long) shlwapi.StrStrNW
@ stdcall StrStrW(wstr wstr) shlwapi.StrStrW
@ stdcall StrToInt64ExA(str long ptr) shlwapi.StrToInt64ExA
@ stdcall StrToInt64ExW(wstr long ptr) shlwapi.StrToInt64ExW
@ stdcall StrToIntA(str) shlwapi.StrToIntA
@ stdcall StrToIntExA(str long ptr) shlwapi.StrToIntExA
@ stdcall StrToIntExW(wstr long ptr) shlwapi.StrToIntExW
@ stdcall StrToIntW(wstr) shlwapi.StrToIntW
@ stdcall StrTrimA(str str) shlwapi.StrTrimA
@ stdcall StrTrimW(wstr wstr) shlwapi.StrTrimW
@ stdcall UrlApplySchemeW(wstr ptr ptr long) shlwapi.UrlApplySchemeW
@ stdcall UrlCanonicalizeW(wstr ptr ptr long) shlwapi.UrlCanonicalizeW
@ stdcall UrlCombineA(str str ptr ptr long) shlwapi.UrlCombineA
@ stdcall UrlCombineW(wstr wstr ptr ptr long) shlwapi.UrlCombineW
@ stdcall UrlCreateFromPathW(wstr ptr ptr long) shlwapi.UrlCreateFromPathW
@ stdcall UrlEscapeW(wstr ptr ptr long) shlwapi.UrlEscapeW
@ stdcall UrlFixupW(wstr wstr long) shlwapi.UrlFixupW
@ stdcall UrlGetLocationW(wstr) shlwapi.UrlGetLocationW
@ stdcall UrlGetPartW(wstr ptr ptr long long) shlwapi.UrlGetPartW
@ stdcall UrlIsW(wstr long) shlwapi.UrlIsW
@ stdcall UrlUnescapeA(str ptr ptr long) shlwapi.UrlUnescapeA
@ stdcall UrlUnescapeW(wstr ptr ptr long) shlwapi.UrlUnescapeW
