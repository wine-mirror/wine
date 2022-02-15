@ stdcall IsCharBlankW(long) kernelbase.IsCharBlankW
@ stdcall IsCharCntrlW(long) kernelbase.IsCharCntrlW
@ stdcall IsCharDigitW(long) kernelbase.IsCharDigitW
@ stdcall IsCharPunctW(long) kernelbase.IsCharPunctW
@ stdcall IsCharSpaceA(long) kernelbase.IsCharSpaceA
@ stdcall IsCharSpaceW(long) kernelbase.IsCharSpaceW
@ stdcall IsCharXDigitW(long) kernelbase.IsCharXDigitW
@ stdcall PathAddBackslashA(str) kernelbase.PathAddBackslashA
@ stdcall PathAddBackslashW(wstr) kernelbase.PathAddBackslashW
@ stdcall PathAddExtensionA(str str) kernelbase.PathAddExtensionA
@ stdcall PathAddExtensionW(wstr wstr) kernelbase.PathAddExtensionW
@ stdcall PathAppendA(str str) kernelbase.PathAppendA
@ stdcall PathAppendW(wstr wstr) kernelbase.PathAppendW
@ stdcall PathCanonicalizeA(ptr str) kernelbase.PathCanonicalizeA
@ stdcall PathCanonicalizeW(ptr wstr) kernelbase.PathCanonicalizeW
@ stdcall PathCombineA(ptr str str) kernelbase.PathCombineA
@ stdcall PathCombineW(ptr wstr wstr) kernelbase.PathCombineW
@ stdcall PathCommonPrefixA(str str ptr) kernelbase.PathCommonPrefixA
@ stdcall PathCommonPrefixW(wstr wstr ptr) kernelbase.PathCommonPrefixW
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
@ stdcall PathGetCharTypeA(long) kernelbase.PathGetCharTypeA
@ stdcall PathGetCharTypeW(long) kernelbase.PathGetCharTypeW
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
@ stdcall PathIsValidCharA(long long) kernelbase.PathIsValidCharA
@ stdcall PathIsValidCharW(long long) kernelbase.PathIsValidCharW
@ stdcall PathMatchSpecA(str str) kernelbase.PathMatchSpecA
@ stub PathMatchSpecExA
@ stub PathMatchSpecExW
@ stdcall PathMatchSpecW(wstr wstr) kernelbase.PathMatchSpecW
@ stdcall PathParseIconLocationA(str) kernelbase.PathParseIconLocationA
@ stdcall PathParseIconLocationW(wstr) kernelbase.PathParseIconLocationW
@ stdcall PathQuoteSpacesA(str) kernelbase.PathQuoteSpacesA
@ stdcall PathQuoteSpacesW(wstr) kernelbase.PathQuoteSpacesW
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
@ stdcall PathSearchAndQualifyA(str ptr long) kernelbase.PathSearchAndQualifyA
@ stdcall PathSearchAndQualifyW(wstr ptr long) kernelbase.PathSearchAndQualifyW
@ stdcall PathSkipRootA(str) kernelbase.PathSkipRootA
@ stdcall PathSkipRootW(wstr) kernelbase.PathSkipRootW
@ stdcall PathStripPathA(str) kernelbase.PathStripPathA
@ stdcall PathStripPathW(wstr) kernelbase.PathStripPathW
@ stdcall PathStripToRootA(str) kernelbase.PathStripToRootA
@ stdcall PathStripToRootW(wstr) kernelbase.PathStripToRootW
@ stdcall PathUnExpandEnvStringsA(str ptr long) kernelbase.PathUnExpandEnvStringsA
@ stdcall PathUnExpandEnvStringsW(wstr ptr long) kernelbase.PathUnExpandEnvStringsW
@ stdcall PathUnquoteSpacesA(str) kernelbase.PathUnquoteSpacesA
@ stdcall PathUnquoteSpacesW(wstr) kernelbase.PathUnquoteSpacesW
@ stdcall SHExpandEnvironmentStringsA(str ptr long) kernelbase.SHExpandEnvironmentStringsA
@ stdcall SHExpandEnvironmentStringsW(wstr ptr long) kernelbase.SHExpandEnvironmentStringsW
@ stdcall SHTruncateString(str long) kernelbase.SHTruncateString
