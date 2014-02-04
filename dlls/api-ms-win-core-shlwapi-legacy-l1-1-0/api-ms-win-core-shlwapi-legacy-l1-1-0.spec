@ stdcall IsCharBlankW(long) shlwapi.IsCharBlankW
@ stdcall IsCharCntrlW(ptr) shlwapi.IsCharCntrlW
@ stdcall IsCharDigitW(long) shlwapi.IsCharDigitW
@ stdcall IsCharPunctW(long) shlwapi.IsCharPunctW
@ stdcall IsCharSpaceA(long) shlwapi.IsCharSpaceA
@ stdcall IsCharSpaceW(long) shlwapi.IsCharSpaceW
@ stdcall IsCharXDigitW(long) shlwapi.IsCharXDigitW
@ stdcall PathAddBackslashA(str) shlwapi.PathAddBackslashA
@ stdcall PathAddBackslashW(wstr) shlwapi.PathAddBackslashW
@ stdcall PathAddExtensionA(str str) shlwapi.PathAddExtensionA
@ stdcall PathAddExtensionW(wstr wstr) shlwapi.PathAddExtensionW
@ stdcall PathAppendA(str str) shlwapi.PathAppendA
@ stdcall PathAppendW(wstr wstr) shlwapi.PathAppendW
@ stdcall PathCanonicalizeA(ptr str) shlwapi.PathCanonicalizeA
@ stdcall PathCanonicalizeW(ptr wstr) shlwapi.PathCanonicalizeW
@ stdcall PathCombineA(ptr str str) shlwapi.PathCombineA
@ stdcall PathCombineW(ptr wstr wstr) shlwapi.PathCombineW
@ stdcall PathCommonPrefixA(str str ptr) shlwapi.PathCommonPrefixA
@ stdcall PathCommonPrefixW(wstr wstr ptr) shlwapi.PathCommonPrefixW
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
@ stdcall PathGetCharTypeA(long) shlwapi.PathGetCharTypeA
@ stdcall PathGetCharTypeW(long) shlwapi.PathGetCharTypeW
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
@ stdcall PathIsValidCharA(long long) shlwapi.PathIsValidCharA
@ stdcall PathIsValidCharW(long long) shlwapi.PathIsValidCharW
@ stdcall PathMatchSpecA(str str) shlwapi.PathMatchSpecA
@ stub PathMatchSpecExA
@ stub PathMatchSpecExW
@ stdcall PathMatchSpecW(wstr wstr) shlwapi.PathMatchSpecW
@ stdcall PathParseIconLocationA(str) shlwapi.PathParseIconLocationA
@ stdcall PathParseIconLocationW(wstr) shlwapi.PathParseIconLocationW
@ stdcall PathQuoteSpacesA(str) shlwapi.PathQuoteSpacesA
@ stdcall PathQuoteSpacesW(wstr) shlwapi.PathQuoteSpacesW
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
@ stdcall PathSearchAndQualifyA(str ptr long) shlwapi.PathSearchAndQualifyA
@ stdcall PathSearchAndQualifyW(wstr ptr long) shlwapi.PathSearchAndQualifyW
@ stdcall PathSkipRootA(str) shlwapi.PathSkipRootA
@ stdcall PathSkipRootW(wstr) shlwapi.PathSkipRootW
@ stdcall PathStripPathA(str) shlwapi.PathStripPathA
@ stdcall PathStripPathW(wstr) shlwapi.PathStripPathW
@ stdcall PathStripToRootA(str) shlwapi.PathStripToRootA
@ stdcall PathStripToRootW(wstr) shlwapi.PathStripToRootW
@ stdcall PathUnExpandEnvStringsA(str ptr long) shlwapi.PathUnExpandEnvStringsA
@ stdcall PathUnExpandEnvStringsW(wstr ptr long) shlwapi.PathUnExpandEnvStringsW
@ stdcall PathUnquoteSpacesA(str) shlwapi.PathUnquoteSpacesA
@ stdcall PathUnquoteSpacesW(wstr) shlwapi.PathUnquoteSpacesW
@ stdcall SHExpandEnvironmentStringsA(str ptr long) shlwapi.SHExpandEnvironmentStringsA
@ stdcall SHExpandEnvironmentStringsW(wstr ptr long) shlwapi.SHExpandEnvironmentStringsW
@ stdcall SHTruncateString(str long) shlwapi.SHTruncateString
