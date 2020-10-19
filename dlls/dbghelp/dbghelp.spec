@ stub DbgHelpCreateUserDump
@ stub DbgHelpCreateUserDumpW
@ stdcall EnumDirTree(long str str ptr ptr ptr)
@ stdcall EnumDirTreeW(long wstr wstr ptr ptr ptr)
@ stdcall EnumerateLoadedModules(long ptr ptr)
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
@ stdcall MapDebugInformation(long str str long)
@ stdcall MiniDumpReadDumpStream(ptr long ptr ptr ptr)
@ stdcall MiniDumpWriteDump(ptr long ptr long ptr ptr ptr)
@ stdcall SearchTreeForFile(str str ptr)
@ stdcall SearchTreeForFileW(wstr wstr ptr)
@ stdcall StackWalk(long long long ptr ptr ptr ptr ptr ptr)
@ stdcall StackWalk64(long long long ptr ptr ptr ptr ptr ptr)
@ stub SymAddSourceStream
@ stub SymAddSourceStreamA
@ stub SymAddSourceStreamW
@ stdcall SymAddSymbol(ptr int64 str int64 long long)
@ stdcall SymAddSymbolW(ptr int64 wstr int64 long long)
@ stdcall SymCleanup(long)
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
@ stub SymEnumTypesByName
@ stub SymEnumTypesByNameW
@ stdcall SymEnumTypesW(ptr int64 ptr ptr)
@ stdcall SymEnumerateModules(long ptr ptr)
@ stdcall SymEnumerateModules64(long ptr ptr)
@ stdcall SymEnumerateModulesW64(long ptr ptr)
@ stdcall SymEnumerateSymbols(long long ptr ptr)
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
@ stdcall SymFromName(long str ptr)
@ stub SymFromNameW
@ stub SymFromToken
@ stub SymFromTokenW
@ stdcall SymFunctionTableAccess(long long)
@ stdcall SymFunctionTableAccess64(long int64)
@ stub SymGetFileLineOffsets64
@ stub SymGetHomeDirectory
@ stub SymGetHomeDirectoryW
@ stdcall SymGetExtendedOption(long)
@ stdcall SymGetLineFromAddr(long long ptr ptr)
@ stdcall SymGetLineFromAddr64(long int64 ptr ptr)
@ stdcall SymGetLineFromAddrW64(long int64 ptr ptr)
@ stdcall SymGetLineFromName(long str str long ptr ptr)
@ stdcall SymGetLineFromName64(long str str long ptr ptr)
@ stdcall SymGetLineFromNameW64(long wstr wstr long ptr ptr)
@ stdcall SymGetLineNext(long ptr)
@ stdcall SymGetLineNext64(long ptr)
@ stub SymGetLineNextW64
@ stdcall SymGetLinePrev(long ptr)
@ stdcall SymGetLinePrev64(long ptr)
@ stub SymGetLinePrevW64
@ stdcall SymGetModuleBase(long long)
@ stdcall SymGetModuleBase64(long int64)
@ stdcall SymGetModuleInfo(long long ptr)
@ stdcall SymGetModuleInfo64(long int64 ptr)
@ stdcall SymGetModuleInfoW(long long ptr)
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
@ stdcall SymGetSymFromAddr(long long ptr ptr)
@ stdcall SymGetSymFromAddr64(long int64 ptr ptr)
@ stdcall SymGetSymFromName(long str ptr)
@ stdcall SymGetSymFromName64(long str ptr)
@ stdcall SymGetSymNext(long ptr)
@ stdcall SymGetSymNext64(long ptr)
@ stdcall SymGetSymPrev(long ptr)
@ stdcall SymGetSymPrev64(long ptr)
@ stub SymGetSymbolFile
@ stub SymGetSymbolFileW
@ stdcall SymGetTypeFromName(ptr int64 str ptr)
@ stub SymGetTypeFromNameW
@ stdcall SymGetTypeInfo(ptr int64 long long ptr)
@ stub SymGetTypeInfoEx
@ stdcall SymInitialize(long str long)
@ stdcall SymInitializeW(long wstr long)
@ stdcall SymLoadModule(long long str str long long)
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
@ stdcall SymRefreshModuleList(long)
@ stdcall SymRegisterCallback(long ptr ptr)
@ stdcall SymRegisterCallback64(long ptr int64)
@ stdcall SymRegisterCallbackW64(long ptr int64)
@ stdcall SymRegisterFunctionEntryCallback(ptr ptr ptr)
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
@ stub SymSetScopeFromIndex
@ stdcall SymSetSearchPath(long str)
@ stdcall SymSetSearchPathW(long wstr)
@ stub SymSrvDeltaName
@ stub SymSrvDeltaNameW
@ stub SymSrvGetFileIndexInfo
@ stub SymSrvGetFileIndexInfoW
@ stub SymSrvGetFileIndexString
@ stub SymSrvGetFileIndexStringW
@ stub SymSrvGetFileIndexes
@ stub SymSrvGetFileIndexesW
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
@ stdcall SymUnDName(ptr str long)
@ stdcall SymUnDName64(ptr str long)
@ stdcall SymUnloadModule(long long)
@ stdcall SymUnloadModule64(long int64)
@ stdcall UnDecorateSymbolName(str ptr long long)
@ stdcall UnDecorateSymbolNameW(wstr ptr long long)
@ stdcall UnmapDebugInformation(ptr)
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
