@ stub DbgHelpCreateUserDump
@ stub DbgHelpCreateUserDumpW
@ stdcall EnumDirTree(long str str ptr ptr ptr)
@ stub EnumerateLoadedModules64
@ stdcall EnumerateLoadedModules(long ptr ptr)
@ stdcall ExtensionApiVersion()
@ stdcall FindDebugInfoFile(str str ptr)
@ stdcall FindDebugInfoFileEx(str str ptr ptr ptr)
@ stdcall FindExecutableImage(str str str)
@ stub FindExecutableImageEx
@ stub FindFileInPath
@ stub FindFileInSearchPath
@ stdcall GetTimestampForLoadedLibrary(long)
@ stdcall ImageDirectoryEntryToData(ptr long long ptr) ntdll.RtlImageDirectoryEntryToData
@ stub ImageDirectoryEntryToDataEx
@ stdcall ImageNtHeader(ptr) ntdll.RtlImageNtHeader
@ stdcall ImageRvaToSection(ptr ptr long) ntdll.RtlImageRvaToSection
@ stdcall ImageRvaToVa(ptr ptr long ptr) ntdll.RtlImageRvaToVa
@ stdcall ImagehlpApiVersion()
@ stdcall ImagehlpApiVersionEx(ptr)
@ stdcall MakeSureDirectoryPathExists(str)
@ stdcall MapDebugInformation(long str str long)
@ stdcall MiniDumpReadDumpStream(ptr long ptr ptr ptr)
@ stdcall MiniDumpWriteDump(ptr long ptr long long long long)
@ stdcall SearchTreeForFile(str str str)
@ stdcall StackWalk64(long long long ptr ptr ptr ptr ptr ptr)
@ stdcall StackWalk(long long long ptr ptr ptr ptr ptr ptr)
@ stub SymAddSymbol
@ stdcall SymCleanup(long)
@ stdcall SymEnumLines(ptr double str str ptr ptr)
@ stdcall SymEnumSourceFiles(ptr double str ptr ptr)
@ stub SymEnumSym
@ stdcall SymEnumSymbols(ptr double str ptr ptr)
@ stub SymEnumSymbolsForAddr
@ stdcall SymEnumTypes(ptr double ptr ptr)
@ stub SymEnumerateModules64
@ stdcall SymEnumerateModules(long ptr ptr)
@ stub SymEnumerateSymbols64
@ stdcall SymEnumerateSymbols(long long ptr ptr)
@ stub SymEnumerateSymbolsW64
@ stub SymEnumerateSymbolsW
@ stdcall SymFindFileInPath(long str str ptr long long long ptr ptr ptr)
@ stdcall SymFromAddr(ptr double ptr ptr)
@ stdcall SymFromName(long str ptr)
@ stub SymFromToken
@ stdcall SymFunctionTableAccess64(long double)
@ stdcall SymFunctionTableAccess(long long)
@ stub SymGetFileLineOffsets64
@ stub SymGetHomeDirectory
@ stdcall SymGetLineFromAddr64(long double ptr ptr)
@ stdcall SymGetLineFromAddr(long long ptr ptr)
@ stub SymGetLineFromName64
@ stub SymGetLineFromName
@ stdcall SymGetLineNext64(long ptr)
@ stdcall SymGetLineNext(long ptr)
@ stdcall SymGetLinePrev64(long ptr)
@ stdcall SymGetLinePrev(long ptr)
@ stdcall SymGetModuleBase64(long double)
@ stdcall SymGetModuleBase(long long)
@ stdcall SymGetModuleInfo64(long double ptr)
@ stdcall SymGetModuleInfo(long long ptr)
@ stub SymGetModuleInfoW64
@ stub SymGetModuleInfoW
@ stdcall SymGetOptions()
@ stdcall SymGetSearchPath(long str long)
@ stub SymGetSourceFileFromToken
@ stdcall SymGetSourceFileToken(ptr double str ptr ptr)
@ stub SymGetSourceVarFromToken
@ stub SymGetSymFromAddr64
@ stdcall SymGetSymFromAddr(long long ptr ptr)
@ stub SymGetSymFromName64
@ stdcall SymGetSymFromName(long str ptr)
@ stub SymGetSymNext64
@ stdcall SymGetSymNext(long ptr)
@ stub SymGetSymPrev64
@ stdcall SymGetSymPrev(long ptr)
@ stdcall SymGetTypeFromName(ptr double str ptr)
@ stdcall SymGetTypeInfo(ptr double long long ptr)
@ stdcall SymInitialize(long str long)
@ stdcall SymLoadModule64(long long str str double long)
@ stdcall SymLoadModule(long long str str long long)
@ stdcall SymLoadModuleEx(long long str str double long ptr long)
@ stdcall SymMatchFileName(str str ptr ptr)
@ stdcall SymMatchString(str str long)
@ stdcall SymRegisterCallback64(long ptr double)
@ stdcall SymRegisterCallback(long ptr ptr)
@ stdcall SymRegisterFunctionEntryCallback64(ptr ptr double)
@ stdcall SymRegisterFunctionEntryCallback(ptr ptr ptr)
@ stdcall SymSearch(long double long long str double ptr ptr long)
@ stdcall SymSetContext(long ptr ptr)
@ stdcall SymSetOptions(long)
@ stdcall SymSetParentWindow(long)
@ stdcall SymSetSearchPath(long str)
@ stub SymSetSymWithAddr64
@ stub SymUnDName64
@ stdcall SymUnDName(ptr str long)
@ stdcall SymUnloadModule64(long double)
@ stdcall SymUnloadModule(long long)
@ stdcall UnDecorateSymbolName(str str long long)
@ stdcall UnmapDebugInformation(ptr)
@ stdcall WinDbgExtensionDllInit(ptr long long)
#@ stub dbghelp
#@ stub dh
#@ stub fptr
#@ stub lm
#@ stub lmi
#@ stub omap
#@ stub srcfiles
#@ stub sym
#@ stub symsvr
#@ stub vc7fpo
