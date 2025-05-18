@ stub DbgHelpCreateUserDump
@ stub DbgHelpCreateUserDumpW
@ stdcall EnumDirTree(long str str ptr ptr ptr)
@ stdcall EnumDirTreeW(long wstr wstr ptr ptr ptr)
@ stdcall -arch=win32 EnumerateLoadedModules(long ptr ptr)
@ stdcall -arch=win64 EnumerateLoadedModules(long ptr ptr) EnumerateLoadedModules64
@ stdcall EnumerateLoadedModules64(long ptr ptr)
@ stdcall EnumerateLoadedModulesEx(long ptr ptr) EnumerateLoadedModules64
@ stdcall EnumerateLoadedModulesExW(long ptr ptr) EnumerateLoadedModulesW64
@ stdcall EnumerateLoadedModulesW64(long ptr ptr)
@ stdcall ExtensionApiVersion()
@ stdcall FindDebugInfoFile(str str ptr)
@ stdcall FindDebugInfoFileEx(str str ptr ptr ptr)
@ stub FindDebugInfoFileExW
@ stdcall FindExecutableImage(str str str)
@ stdcall FindExecutableImageEx(str str ptr ptr ptr)
@ stdcall FindExecutableImageExW(wstr wstr ptr ptr ptr)
@ stub FindFileInPath
@ stub FindFileInSearchPath
@ stdcall GetTimestampForLoadedLibrary(long)
@ stdcall ImageDirectoryEntryToData(ptr long long ptr)
@ stdcall ImageDirectoryEntryToDataEx(ptr long long ptr ptr)
@ stdcall -import ImageNtHeader(ptr) RtlImageNtHeader
@ stdcall -import ImageRvaToSection(ptr ptr long) RtlImageRvaToSection
@ stdcall -import ImageRvaToVa(ptr ptr long ptr) RtlImageRvaToVa
@ stdcall ImagehlpApiVersion()
@ stdcall ImagehlpApiVersionEx(ptr)
@ stdcall MakeSureDirectoryPathExists(str)
@ stdcall -arch=win32 MapDebugInformation(long str str long)
@ stdcall MiniDumpReadDumpStream(ptr long ptr ptr ptr)
@ stdcall MiniDumpWriteDump(ptr long ptr long ptr ptr ptr)
@ stdcall SearchTreeForFile(str str ptr)
@ stdcall SearchTreeForFileW(wstr wstr ptr)
@ stdcall -arch=win32 StackWalk(long long long ptr ptr ptr ptr ptr ptr)
@ stdcall -arch=win64 StackWalk(long long long ptr ptr ptr ptr ptr ptr) StackWalk64
@ stdcall StackWalk64(long long long ptr ptr ptr ptr ptr ptr)
@ stdcall StackWalkEx(long long long ptr ptr ptr ptr ptr ptr long)
@ stub SymAddSourceStream
@ stub SymAddSourceStreamA
@ stub SymAddSourceStreamW
@ stdcall SymAddSymbol(ptr int64 str int64 long long)
@ stdcall SymAddSymbolW(ptr int64 wstr int64 long long)
@ stdcall SymAddrIncludeInlineTrace(long int64)
@ stdcall SymCleanup(long)
@ stub SymCompareInlineTrace
@ stub SymDeleteSymbol
@ stub SymDeleteSymbolW
@ stdcall SymEnumLines(ptr int64 str str ptr ptr)
@ stub SymEnumLinesW
@ stub SymEnumProcesses
@ stub SymEnumSourceFileTokens
@ stdcall SymEnumSourceFiles(ptr int64 str ptr ptr)
@ stdcall SymEnumSourceFilesW(ptr int64 wstr ptr ptr)
@ stdcall SymEnumSourceLines(ptr int64 str str long long ptr ptr)
@ stdcall SymEnumSourceLinesW(ptr int64 wstr wstr long long ptr ptr)
@ stub SymEnumSym
@ stdcall SymEnumSymbols(ptr int64 str ptr ptr)
@ stub SymEnumSymbolsForAddr
@ stub SymEnumSymbolsForAddrW
@ stdcall SymEnumSymbolsW(ptr int64 wstr ptr ptr)
@ stdcall SymEnumTypes(ptr int64 ptr ptr)
@ stdcall SymEnumTypesByName(ptr int64 str ptr ptr)
@ stdcall SymEnumTypesByNameW(ptr int64 wstr ptr ptr)
@ stdcall SymEnumTypesW(ptr int64 ptr ptr)
@ stdcall -arch=win32 SymEnumerateModules(long ptr ptr)
@ stdcall -arch=win64 SymEnumerateModules(long ptr ptr) SymEnumerateModules64
@ stdcall SymEnumerateModules64(long ptr ptr)
@ stdcall SymEnumerateModulesW64(long ptr ptr)
@ stdcall -arch=win32 SymEnumerateSymbols(long long ptr ptr)
@ stdcall -arch=win64 SymEnumerateSymbols(long int64 ptr ptr) SymEnumerateSymbols64
@ stdcall SymEnumerateSymbols64(long int64 ptr ptr)
@ stub SymEnumerateSymbolsW
@ stub SymEnumerateSymbolsW64
@ stub SymFindDebugInfoFile
@ stub SymFindDebugInfoFileW
@ stub SymFindExecutableImage
@ stub SymFindExecutableImageW
@ stdcall SymFindFileInPath(long str str ptr long long long ptr ptr ptr)
@ stdcall SymFindFileInPathW(long wstr wstr ptr long long long ptr ptr ptr)
@ stdcall SymFromAddr(ptr int64 ptr ptr)
@ stdcall SymFromAddrW(ptr int64 ptr ptr)
@ stdcall SymFromIndex(long int64 long ptr)
@ stdcall SymFromIndexW(long int64 long ptr)
@ stdcall SymFromInlineContext(long int64 long ptr ptr)
@ stdcall SymFromInlineContextW(long int64 long ptr ptr)
@ stdcall SymFromName(long str ptr)
@ stdcall SymFromNameW(long wstr ptr)
@ stub SymFromToken
@ stub SymFromTokenW
@ stdcall -arch=win32 SymFunctionTableAccess(long long)
@ stdcall -arch=win64 SymFunctionTableAccess(long int64) SymFunctionTableAccess64
@ stdcall SymFunctionTableAccess64(long int64)
@ stub SymGetFileLineOffsets64
@ stub SymGetHomeDirectory
@ stub SymGetHomeDirectoryW
@ stdcall SymGetExtendedOption(long)
@ stdcall -arch=win32 SymGetLineFromAddr(long long ptr ptr)
@ stdcall -arch=win64 SymGetLineFromAddr(long int64 ptr ptr) SymGetLineFromAddr64
@ stdcall SymGetLineFromAddr64(long int64 ptr ptr)
@ stub SymGetLineFromAddrW
@ stdcall SymGetLineFromAddrW64(long int64 ptr ptr)
@ stdcall SymGetLineFromInlineContext(long int64 long int64 ptr ptr)
@ stdcall SymGetLineFromInlineContextW(long int64 long int64 ptr ptr)
@ stdcall -arch=win32 SymGetLineFromName(long str str long ptr ptr)
@ stdcall -arch=win64 SymGetLineFromName(long str str long ptr ptr) SymGetLineFromName64
@ stdcall SymGetLineFromName64(long str str long ptr ptr)
@ stdcall SymGetLineFromNameW64(long wstr wstr long ptr ptr)
@ stdcall -arch=win32 SymGetLineNext(long ptr)
@ stdcall -arch=win64 SymGetLineNext(long ptr) SymGetLineNext64
@ stdcall SymGetLineNext64(long ptr)
@ stdcall SymGetLineNextW64(long ptr)
@ stdcall -arch=win32 SymGetLinePrev(long ptr)
@ stdcall -arch=win64 SymGetLinePrev(long ptr) SymGetLinePrev64
@ stdcall SymGetLinePrev64(long ptr)
@ stdcall SymGetLinePrevW64(long ptr)
@ stdcall -arch=win32 SymGetModuleBase(long long)
@ stdcall -arch=win64 SymGetModuleBase(long int64) SymGetModuleBase64
@ stdcall SymGetModuleBase64(long int64)
@ stdcall -arch=win32 SymGetModuleInfo(long ptr ptr)
@ stdcall -arch=win64 SymGetModuleInfo(long int64 ptr) SymGetModuleInfo64
@ stdcall SymGetModuleInfo64(long int64 ptr)
@ stdcall -arch=win32 SymGetModuleInfoW(long long ptr)
@ stdcall -arch=win64 SymGetModuleInfoW(long int64 ptr) SymGetModuleInfoW64
@ stdcall SymGetModuleInfoW64(long int64 ptr)
@ stub SymGetOmapBlockBase
@ stdcall SymGetOptions()
@ stub SymGetScope
@ stub SymGetScopeW
@ stdcall SymGetSearchPath(long ptr long)
@ stdcall SymGetSearchPathW(long ptr long)
@ stub SymGetSourceFile
@ stub SymGetSourceFileFromToken
@ stub SymGetSourceFileFromTokenW
@ stdcall SymGetSourceFileToken(ptr int64 str ptr ptr)
@ stdcall SymGetSourceFileTokenW(ptr int64 wstr ptr ptr)
@ stub SymGetSourceFileW
@ stub SymGetSourceVarFromToken
@ stub SymGetSourceVarFromTokenW
@ stdcall -arch=win32 SymGetSymFromAddr(long long ptr ptr)
@ stdcall -arch=win64 SymGetSymFromAddr(long int64 ptr ptr) SymGetSymFromAddr64
@ stdcall SymGetSymFromAddr64(long int64 ptr ptr)
@ stdcall -arch=win32 SymGetSymFromName(long str ptr)
@ stdcall -arch=win64 SymGetSymFromName(long str ptr) SymGetSymFromName64
@ stdcall SymGetSymFromName64(long str ptr)
@ stdcall -arch=win32 SymGetSymNext(long ptr)
@ stdcall -arch=win64 SymGetSymNext(long ptr) SymGetSymNext64
@ stdcall SymGetSymNext64(long ptr)
@ stdcall -arch=win32 SymGetSymPrev(long ptr)
@ stdcall -arch=win64 SymGetSymPrev(long ptr) SymGetSymPrev64
@ stdcall SymGetSymPrev64(long ptr)
@ stub SymGetSymbolFile
@ stub SymGetSymbolFileW
@ stdcall SymGetTypeFromName(ptr int64 str ptr)
@ stub SymGetTypeFromNameW
@ stdcall SymGetTypeInfo(ptr int64 long long ptr)
@ stub SymGetTypeInfoEx
@ stub SymGetUnwindInfo
@ stdcall SymInitialize(long str long)
@ stdcall SymInitializeW(long wstr long)
@ stdcall -arch=win32 SymLoadModule(long long str str long long)
@ stdcall -arch=win64 SymLoadModule(long long str str int64 long) SymLoadModule64
@ stdcall SymLoadModule64(long long str str int64 long)
@ stdcall SymLoadModuleEx(long long str str int64 long ptr long)
@ stdcall SymLoadModuleExW(long long wstr wstr int64 long ptr long)
@ stdcall SymMatchFileName(str str ptr ptr)
@ stdcall SymMatchFileNameW(wstr wstr ptr ptr)
@ stdcall SymMatchString(str str long) SymMatchStringA
@ stdcall SymMatchStringA(str str long)
@ stdcall SymMatchStringW(wstr wstr long)
@ stub SymNext
@ stub SymNextW
@ stub SymPrev
@ stub SymPrevW
@ stdcall SymQueryInlineTrace(long int64 long int64 int64 ptr ptr)
@ stdcall SymRefreshModuleList(long)
@ stdcall -arch=win32 SymRegisterCallback(long ptr ptr)
@ stdcall -arch=win64 SymRegisterCallback(long ptr ptr) SymRegisterCallback64
@ stdcall SymRegisterCallback64(long ptr int64)
@ stdcall SymRegisterCallbackW64(long ptr int64)
@ stdcall -arch=win32 SymRegisterFunctionEntryCallback(long ptr ptr)
@ stdcall -arch=win64 SymRegisterFunctionEntryCallback(long ptr ptr) SymRegisterFunctionEntryCallback64
@ stdcall SymRegisterFunctionEntryCallback64(ptr ptr int64)
@ stdcall SymSearch(long int64 long long str int64 ptr ptr long)
@ stdcall SymSearchW(long int64 long long wstr int64 ptr ptr long)
@ stdcall SymSetContext(long ptr ptr)
@ stdcall SymSetExtendedOption(long long)
@ stdcall SymSetHomeDirectory(long str)
@ stdcall SymSetHomeDirectoryW(long wstr)
@ stdcall SymSetOptions(long)
@ stdcall SymSetParentWindow(long)
@ stdcall SymSetScopeFromAddr(ptr int64)
@ stdcall SymSetScopeFromIndex(ptr int64 long)
@ stdcall SymSetScopeFromInlineContext(ptr int64 long)
@ stdcall SymSetSearchPath(long str)
@ stdcall SymSetSearchPathW(long wstr)
@ stub SymSrvDeltaName
@ stub SymSrvDeltaNameW
@ stdcall SymSrvGetFileIndexInfo(str ptr long)
@ stdcall SymSrvGetFileIndexInfoW(wstr ptr long)
@ stub SymSrvGetFileIndexString
@ stub SymSrvGetFileIndexStringW
@ stdcall SymSrvGetFileIndexes(str ptr ptr ptr long)
@ stdcall SymSrvGetFileIndexesW(wstr ptr ptr ptr long)
@ stub SymSrvGetSupplement
@ stub SymSrvGetSupplementW
@ stub SymSrvIsStore
@ stub SymSrvIsStoreW
@ stub SymSrvStoreFile
@ stub SymSrvStoreFileW
@ stub SymSrvStoreSupplement
@ stub SymSrvStoreSupplementW
# @ stub SymSetSymWithAddr64 no longer present ??
@ stub SymSetSymWithAddr64
@ stdcall -arch=win32 SymUnDName(ptr str long)
@ stdcall -arch=win64 SymUnDName(ptr str long) SymUnDName64
@ stdcall SymUnDName64(ptr str long)
@ stdcall -arch=win32 SymUnloadModule(long long)
@ stdcall -arch=win64 SymUnloadModule(long int64) SymUnloadModule64
@ stdcall SymUnloadModule64(long int64)
@ stdcall UnDecorateSymbolName(str ptr long long)
@ stdcall UnDecorateSymbolNameW(wstr ptr long long)
@ stdcall -arch=win32 UnmapDebugInformation(ptr)
@ stdcall WinDbgExtensionDllInit(ptr long long)
#@ stub block
#@ stub chksym
#@ stub dbghelp
#@ stub dh
#@ stub fptr
#@ stub homedir
#@ stub itoldyouso
#@ stub lmi
#@ stub lminfo
#@ stub omap
#@ stub srcfiles
#@ stub stack_force_ebp
#@ stub stackdbg
#@ stub sym
#@ stub symsrv
#@ stub vc7fpo

# wine extensions
@ stdcall wine_get_module_information(long int64 ptr long)
