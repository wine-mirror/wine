/*
 *      imagehlp.h   -       Declarations for IMAGEHLP
 */

#ifndef __WINE_IMAGEHLP_H
#define __WINE_IMAGEHLP_H

#include "windef.h"
#include "winbase.h"

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/***********************************************************************
 * Types
 */

typedef PVOID DIGEST_HANDLE; 

/***********************************************************************
 * Enums/Defines
 */

typedef enum _IMAGEHLP_STATUS_REASON {
  BindOutOfMemory,
  BindRvaToVaFailed,
  BindNoRoomInImage,
  BindImportModuleFailed,
  BindImportProcedureFailed,
  BindImportModule,
  BindImportProcedure,
  BindForwarder,
  BindForwarderNOT,
  BindImageModified,
  BindExpandFileHeaders,
  BindImageComplete,
  BindMismatchedSymbols,
  BindSymbolsNotUpdated
} IMAGEHLP_STATUS_REASON;

#define BIND_NO_BOUND_IMPORTS  0x00000001
#define BIND_NO_UPDATE         0x00000002
#define BIND_ALL_IMAGES        0x00000004
#define BIND_CACHE_IMPORT_DLLS 0x00000008

#define CERT_PE_IMAGE_DIGEST_DEBUG_INFO      0x01
#define CERT_PE_IMAGE_DIGEST_RESOURCES       0x02
#define CERT_PE_IMAGE_DIGEST_ALL_IMPORT_INFO 0x04
#define CERT_PE_IMAGE_DIGEST_NON_PE_INFO     0x08

#define CERT_SECTION_TYPE_ANY 0xFF

#define WIN_CERT_REVISION_1_0 0x0100
#define WIN_CERT_REVISION_2_0 0x0200

#define WIN_CERT_TYPE_X509             0x0001 /* X.509 Certificate */
#define WIN_CERT_TYPE_PKCS_SIGNED_DATA 0x0002 /* PKCS SignedData */
#define WIN_CERT_TYPE_RESERVED_1       0x0003 /* Reserved */

#define SPLITSYM_REMOVE_PRIVATE    0x00000001
#define SPLITSYM_EXTRACT_ALL       0x00000002
#define SPLITSYM_SYMBOLPATH_IS_SRC 0x00000004

#define FRAME_FPO    0
#define FRAME_TRAP   1
#define FRAME_TSS    2
#define FRAME_NONFPO 3

#define CHECKSUM_SUCCESS         0
#define CHECKSUM_OPEN_FAILURE    1
#define CHECKSUM_MAP_FAILURE     2
#define CHECKSUM_MAPVIEW_FAILURE 3
#define CHECKSUM_UNICODE_FAILURE 4

typedef enum _ADRESS_MODE {
  AddrMode1616,
  AddrMode1632,
  AddrModeReal,
  AddrModeFlat
} ADDRESS_MODE;

#define SYMOPT_CASE_INSENSITIVE  0x00000001
#define SYMOPT_UNDNAME           0x00000002
#define SYMOPT_DEFERRED_LOADS    0x00000004
#define SYMOPT_NO_CPP            0x00000008
#define SYMOPT_LOAD_LINES        0x00000010
#define SYMOPT_OMAP_FIND_NEAREST 0x00000020

#define SYMF_OMAP_GENERATED   0x00000001
#define SYMF_OMAP_MODIFIED    0x00000002

typedef enum _SYM_TYPE {
  SymNone,
  SymCoff,
  SymCv,
  SymPdb,
  SymExport,
  SymDeferred,
  SymSym        /* .sym file */
} SYM_TYPE32;

#define UNDNAME_COMPLETE               0x0000
#define UNDNAME_NO_LEADING_UNDERSCORES 0x0001
#define UNDNAME_NO_MS_KEYWORDS         0x0002
#define UNDNAME_NO_FUNCTION_RETURNS    0x0004
#define UNDNAME_NO_ALLOCATION_MODEL    0x0008
#define UNDNAME_NO_ALLOCATION_LANGUAGE 0x0010
#define UNDNAME_NO_MS_THISTYPE         0x0020
#define UNDNAME_NO_CV_THISTYPE         0x0040
#define UNDNAME_NO_THISTYPE            0x0060
#define UNDNAME_NO_ACCESS_SPECIFIERS   0x0080
#define UNDNAME_NO_THROW_SIGNATURES    0x0100
#define UNDNAME_NO_MEMBER_TYPE         0x0200
#define UNDNAME_NO_RETURN_UDT_MODEL    0x0400
#define UNDNAME_32_BIT_DECODE          0x0800
#define UNDNAME_NAME_ONLY              0x1000
#define UNDNAME_NO_ARGUMENTS           0x2000
#define UNDNAME_NO_SPECIAL_SYMS        0x4000

#define CBA_DEFERRED_SYMBOL_LOAD_START          0x00000001
#define CBA_DEFERRED_SYMBOL_LOAD_COMPLETE       0x00000002
#define CBA_DEFERRED_SYMBOL_LOAD_FAILURE        0x00000003
#define CBA_SYMBOLS_UNLOADED                    0x00000004
#define CBA_DUPLICATE_SYMBOL                    0x00000005

/***********************************************************************
 * Structures
 */

typedef struct _LOADED_IMAGE {
  LPSTR                   ModuleName;
  HANDLE                hFile;
  PUCHAR                  MappedAddress;
  PIMAGE_NT_HEADERS     FileHeader;
  PIMAGE_SECTION_HEADER LastRvaSection;
  ULONG                   NumberOfSections;
  PIMAGE_SECTION_HEADER Sections;
  ULONG                   Characteristics;
  BOOLEAN                 fSystemImage;
  BOOLEAN                 fDOSImage;
  LIST_ENTRY            Links;
  ULONG                   SizeOfImage;
} LOADED_IMAGE, *PLOADED_IMAGE;

typedef struct _WIN_CERTIFICATE {
  DWORD dwLength;
  WORD  wRevision;                   /*  WIN_CERT_REVISON_xxx */ 
  WORD  wCertificateType;            /*  WIN_CERT_TYPE_xxx */
  BYTE  bCertificate[ANYSIZE_ARRAY];
} WIN_CERTIFICATE, *PWIN_CERTIFICATE;

typedef struct _API_VERSION {
  USHORT  MajorVersion;
  USHORT  MinorVersion;
  USHORT  Revision;
  USHORT  Reserved;
} API_VERSION, *PAPI_VERSION;

typedef struct _IMAGE_DEBUG_INFORMATION {
  LIST_ENTRY List;
  DWORD        Size;
  PVOID        MappedBase;
  USHORT       Machine;
  USHORT       Characteristics;
  DWORD        CheckSum;
  DWORD        ImageBase;
  DWORD        SizeOfImage;
  
  DWORD NumberOfSections;
  PIMAGE_SECTION_HEADER Sections;

  DWORD ExportedNamesSize;
  LPSTR ExportedNames;

  DWORD NumberOfFunctionTableEntries;
  PIMAGE_FUNCTION_ENTRY FunctionTableEntries;
  DWORD LowestFunctionStartingAddress;
  DWORD HighestFunctionEndingAddress;

  DWORD NumberOfFpoTableEntries;
  PFPO_DATA FpoTableEntries;

  DWORD SizeOfCoffSymbols;
  PIMAGE_COFF_SYMBOLS_HEADER CoffSymbols;

  DWORD SizeOfCodeViewSymbols;
  PVOID CodeViewSymbols;

  LPSTR ImageFilePath;
  LPSTR ImageFileName;
  LPSTR DebugFilePath;

  DWORD TimeDateStamp;

  BOOL RomImage;
  PIMAGE_DEBUG_DIRECTORY DebugDirectory;
  DWORD  NumberOfDebugDirectories;

  DWORD Reserved[3];
} IMAGE_DEBUG_INFORMATION, *PIMAGE_DEBUG_INFORMATION;

typedef struct _ADDRESS {
    DWORD          Offset;
    WORD           Segment;
    ADDRESS_MODE Mode;
} ADDRESS, *PADDRESS;

typedef struct _KDHELP {
  DWORD Thread;
  DWORD ThCallbackStack;
  DWORD NextCallback;
  DWORD FramePointer;
  DWORD KiCallUserMode;
  DWORD KeUserCallbackDispatcher;
  DWORD SystemRangeStart;
} KDHELP, *PKDHELP;

typedef struct _STACKFRAME {
  ADDRESS AddrPC;
  ADDRESS AddrReturn;
  ADDRESS AddrFrame;
  ADDRESS AddrStack;
  PVOID     FuncTableEntry;
  DWORD     Params[4];
  BOOL    Far;
  BOOL    Virtual;
  DWORD     Reserved[3];
  KDHELP  KdHelp;
} STACKFRAME, *PSTACKFRAME;

typedef struct _IMAGEHLP_SYMBOL {
  DWORD SizeOfStruct;
  DWORD Address;
  DWORD Size;
  DWORD Flags;
  DWORD MaxNameLength;
  CHAR  Name[ANYSIZE_ARRAY];
} IMAGEHLP_SYMBOL, *PIMAGEHLP_SYMBOL;

typedef struct _IMAGEHLP_MODULE {
  DWORD      SizeOfStruct;
  DWORD      BaseOfImage;
  DWORD      ImageSize;
  DWORD      TimeDateStamp;
  DWORD      CheckSum;
  DWORD      NumSyms;
  SYM_TYPE32 SymType;
  CHAR       ModuleName[32];
  CHAR       ImageName[256];
  CHAR       LoadedImageName[256];
} IMAGEHLP_MODULE, *PIMAGEHLP_MODULE;

typedef struct _IMAGEHLP_LINE {
  DWORD SizeOfStruct;
  DWORD Key;
  DWORD LineNumber;
  PCHAR FileName;
  DWORD Address;
} IMAGEHLP_LINE, *PIMAGEHLP_LINE;

typedef struct _IMAGEHLP_DEFERRED_SYMBOL_LOAD {
  DWORD   SizeOfStruct;
  DWORD   BaseOfImage;
  DWORD   CheckSum;
  DWORD   TimeDateStamp;
  CHAR    FileName[MAX_PATH];
  BOOLEAN Reparse;
} IMAGEHLP_DEFERRED_SYMBOL_LOAD, *PIMAGEHLP_DEFERRED_SYMBOL_LOAD;

typedef struct _IMAGEHLP_DUPLICATE_SYMBOL {
  DWORD              SizeOfStruct;
  DWORD              NumberOfDups;
  PIMAGEHLP_SYMBOL Symbol;
  ULONG              SelectedSymbol;
} IMAGEHLP_DUPLICATE_SYMBOL, *PIMAGEHLP_DUPLICATE_SYMBOL;

/***********************************************************************
 * Callbacks
 */

typedef BOOL CALLBACK (*PIMAGEHLP_STATUS_ROUTINE)(
  IMAGEHLP_STATUS_REASON Reason, LPSTR ImageName, LPSTR DllName,
  ULONG Va, ULONG Parameter
);

typedef BOOL CALLBACK (*PSYM_ENUMMODULES_CALLBACK)(
  LPSTR ModuleName, ULONG BaseOfDll, PVOID UserContext
);

typedef BOOL CALLBACK (*PSYM_ENUMSYMBOLS_CALLBACK)(
  LPSTR SymbolName, ULONG SymbolAddress, ULONG SymbolSize, 
  PVOID UserContext
);

typedef BOOL CALLBACK (*PENUMLOADED_MODULES_CALLBACK)(
  LPSTR ModuleName, ULONG ModuleBase, ULONG ModuleSize, 
  PVOID UserContext
);

typedef BOOL CALLBACK (*PSYMBOL_REGISTERED_CALLBACK)(
  HANDLE hProcess, ULONG ActionCode, PVOID CallbackData,
  PVOID UserContext
);

typedef BOOL CALLBACK (*DIGEST_FUNCTION)(
  DIGEST_HANDLE refdata, PBYTE pData, DWORD dwLength
);

typedef BOOL CALLBACK (*PREAD_PROCESS_MEMORY_ROUTINE)(
  HANDLE  hProcess, PCVOID lpBaseAddress, PVOID lpBuffer,
  DWORD nSize, PDWORD lpNumberOfBytesRead
);

typedef PVOID CALLBACK (*PFUNCTION_TABLE_ACCESS_ROUTINE)(
  HANDLE hProcess, DWORD AddrBase
);

typedef DWORD CALLBACK (*PGET_MODULE_BASE_ROUTINE)(
  HANDLE hProcess, DWORD ReturnAddress);

typedef DWORD CALLBACK (*PTRANSLATE_ADDRESS_ROUTINE)(
  HANDLE hProcess, HANDLE hThread, PADDRESS lpaddr
);

/***********************************************************************
 * Functions
 */

BOOL WINAPI BindImage(
  LPSTR ImageName, LPSTR DllPath, LPSTR SymbolPath
);
BOOL WINAPI BindImageEx(
  DWORD Flags, LPSTR ImageName, LPSTR DllPath, LPSTR SymbolPath,
  PIMAGEHLP_STATUS_ROUTINE StatusRoutine
);
PIMAGE_NT_HEADERS WINAPI CheckSumMappedFile(
  LPVOID BaseAddress, DWORD FileLength, 
  LPDWORD HeaderSum, LPDWORD CheckSum
);
BOOL WINAPI EnumerateLoadedModules(
  HANDLE hProcess,
  PENUMLOADED_MODULES_CALLBACK EnumLoadedModulesCallback,
  PVOID UserContext
);
HANDLE WINAPI FindDebugInfoFile(
  LPSTR FileName, LPSTR SymbolPath, LPSTR DebugFilePath
);
HANDLE WINAPI FindExecutableImage(
  LPSTR FileName, LPSTR SymbolPath, LPSTR ImageFilePath
);
BOOL WINAPI GetImageConfigInformation(
  PLOADED_IMAGE LoadedImage, 
  PIMAGE_LOAD_CONFIG_DIRECTORY ImageConfigInformation
);
DWORD WINAPI GetImageUnusedHeaderBytes(
  PLOADED_IMAGE LoadedImage,
  LPDWORD SizeUnusedHeaderBytes
);
DWORD WINAPI GetTimestampForLoadedLibrary(
  HMODULE Module
);
BOOL WINAPI ImageAddCertificate(
  HANDLE FileHandle, PWIN_CERTIFICATE Certificate, PDWORD Index
);
PVOID WINAPI ImageDirectoryEntryToData(
  PVOID Base, BOOLEAN MappedAsImage, USHORT DirectoryEntry, PULONG Size
);
BOOL WINAPI ImageEnumerateCertificates(
  HANDLE FileHandle, WORD TypeFilter, PDWORD CertificateCount,
  PDWORD Indices, DWORD IndexCount
);
BOOL WINAPI ImageGetCertificateData(
  HANDLE FileHandle, DWORD CertificateIndex,
  PWIN_CERTIFICATE Certificate, PDWORD RequiredLength
);
BOOL WINAPI ImageGetCertificateHeader(
  HANDLE FileHandle, DWORD CertificateIndex,
  PWIN_CERTIFICATE Certificateheader
);
BOOL WINAPI ImageGetDigestStream(
  HANDLE FileHandle, DWORD DigestLevel,
  DIGEST_FUNCTION DigestFunction, DIGEST_HANDLE DigestHandle
);
PLOADED_IMAGE WINAPI ImageLoad(
  LPSTR DllName, LPSTR DllPath
);
PIMAGE_NT_HEADERS WINAPI ImageNtHeader(
  PVOID Base
);
BOOL WINAPI ImageRemoveCertificate(
  HANDLE FileHandle, DWORD Index
);
PIMAGE_SECTION_HEADER WINAPI ImageRvaToSection(
  PIMAGE_NT_HEADERS NtHeaders, PVOID Base, ULONG Rva
);
PVOID WINAPI ImageRvaToVa(
  PIMAGE_NT_HEADERS NtHeaders, PVOID Base, ULONG Rva,
  PIMAGE_SECTION_HEADER *LastRvaSection
);
BOOL WINAPI ImageUnload(
  PLOADED_IMAGE LoadedImage
);
PAPI_VERSION WINAPI ImagehlpApiVersion(
  void 
);
PAPI_VERSION WINAPI ImagehlpApiVersionEx(
  PAPI_VERSION AppVersion
);
BOOL WINAPI MakeSureDirectoryPathExists(
  LPCSTR DirPath
);
BOOL WINAPI MapAndLoad(
  LPSTR ImageName, LPSTR DllPath, PLOADED_IMAGE LoadedImage,
  BOOL DotDll, BOOL ReadOnly
);
PIMAGE_DEBUG_INFORMATION WINAPI MapDebugInformation(
  HANDLE FileHandle, LPSTR FileName,
  LPSTR SymbolPath, DWORD ImageBase
);
DWORD WINAPI MapFileAndCheckSumA(
  LPSTR Filename, LPDWORD HeaderSum, LPDWORD CheckSum
);
DWORD WINAPI MapFileAndCheckSumW(
  LPWSTR Filename, LPDWORD HeaderSum, LPDWORD CheckSum
);
BOOL WINAPI ReBaseImage(
  LPSTR CurrentImageName, LPSTR SymbolPath, BOOL fReBase,
  BOOL fRebaseSysfileOk, BOOL fGoingDown, ULONG CheckImageSize,
  ULONG *OldImageSize, ULONG *OldImageBase, ULONG *NewImageSize,
  ULONG *NewImageBase, ULONG TimeStamp
);
BOOL WINAPI RemovePrivateCvSymbolic(
  PCHAR DebugData, PCHAR *NewDebugData, ULONG *NewDebugSize
);
VOID WINAPI RemoveRelocations(
  PCHAR ImageName
);
BOOL WINAPI SearchTreeForFile(
  LPSTR RootPath, LPSTR InputPathName, LPSTR OutputPathBuffer
);
BOOL WINAPI SetImageConfigInformation(
  PLOADED_IMAGE LoadedImage,
  PIMAGE_LOAD_CONFIG_DIRECTORY ImageConfigInformation
);
BOOL WINAPI SplitSymbols(
  LPSTR ImageName, LPSTR SymbolsPath, 
  LPSTR SymbolFilePath, DWORD Flags
);
BOOL WINAPI StackWalk(
  DWORD MachineType, HANDLE hProcess, HANDLE hThread,
  PSTACKFRAME StackFrame, PVOID ContextRecord,
  PREAD_PROCESS_MEMORY_ROUTINE ReadMemoryRoutine,
  PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,
  PGET_MODULE_BASE_ROUTINE GetModuleBaseRoutine,
  PTRANSLATE_ADDRESS_ROUTINE TranslateAddress
);
BOOL WINAPI SymCleanup(
  HANDLE hProcess
);
BOOL WINAPI SymEnumerateModules(
  HANDLE hProcess, PSYM_ENUMMODULES_CALLBACK EnumModulesCallback,
  PVOID UserContext
);
BOOL WINAPI SymEnumerateSymbols(
  HANDLE hProcess, DWORD BaseOfDll,
  PSYM_ENUMSYMBOLS_CALLBACK EnumSymbolsCallback, PVOID UserContext
);
PVOID WINAPI SymFunctionTableAccess(
  HANDLE hProcess, DWORD AddrBase
);
DWORD WINAPI SymGetModuleBase(
  HANDLE hProcess, DWORD dwAddr
);
BOOL WINAPI SymGetModuleInfo(
  HANDLE hProcess, DWORD dwAddr,
  PIMAGEHLP_MODULE ModuleInfo
);
DWORD WINAPI SymGetOptions(
  void
);
BOOL WINAPI SymGetSearchPath(
  HANDLE hProcess, LPSTR szSearchPath, DWORD SearchPathLength
);
BOOL WINAPI SymGetSymFromAddr(
  HANDLE hProcess, DWORD dwAddr,
  PDWORD pdwDisplacement, PIMAGEHLP_SYMBOL Symbol
);
BOOL WINAPI SymGetSymFromName(
  HANDLE hProcess, LPSTR Name, PIMAGEHLP_SYMBOL Symbol
);
BOOL WINAPI SymGetSymNext(
  HANDLE hProcess, PIMAGEHLP_SYMBOL Symbol
);
BOOL WINAPI SymGetSymPrev(
  HANDLE hProcess, PIMAGEHLP_SYMBOL Symbol
);
BOOL WINAPI SymInitialize(
  HANDLE hProcess, LPSTR UserSearchPath, BOOL fInvadeProcess
);
BOOL WINAPI SymLoadModule(
  HANDLE hProcess, HANDLE hFile, LPSTR ImageName, LPSTR ModuleName,
  DWORD BaseOfDll, DWORD SizeOfDll
);
BOOL WINAPI SymRegisterCallback(
  HANDLE hProcess, PSYMBOL_REGISTERED_CALLBACK CallbackFunction,
  PVOID UserContext
);
DWORD WINAPI SymSetOptions(
  DWORD SymOptions
);
BOOL WINAPI SymSetSearchPath(
  HANDLE hProcess, LPSTR szSearchPath
);
BOOL WINAPI SymUnDName(
  PIMAGEHLP_SYMBOL sym, LPSTR UnDecName, DWORD UnDecNameLength
);
BOOL WINAPI SymUnloadModule(
  HANDLE hProcess, DWORD BaseOfDll
);
BOOL WINAPI TouchFileTimes(
  HANDLE FileHandle, LPSYSTEMTIME lpSystemTime
);
DWORD WINAPI UnDecorateSymbolName(
  LPCSTR DecoratedName, LPSTR UnDecoratedName,
  DWORD UndecoratedLength, DWORD Flags
);
BOOL WINAPI UnMapAndLoad(
  PLOADED_IMAGE LoadedImage
);
BOOL WINAPI UnmapDebugInformation(
  PIMAGE_DEBUG_INFORMATION DebugInfo
);
BOOL WINAPI UpdateDebugInfoFile(
  LPSTR ImageFileName, LPSTR SymbolPath,
  LPSTR DebugFilePath, PIMAGE_NT_HEADERS NtHeaders
);
BOOL WINAPI UpdateDebugInfoFileEx(
  LPSTR ImageFileName, LPSTR SymbolPath, LPSTR DebugFilePath,
  PIMAGE_NT_HEADERS NtHeaders, DWORD OldChecksum
);

/***********************************************************************
 * Wine specific
 */

extern HANDLE IMAGEHLP_hHeap;

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif  /* __WINE_IMAGEHLP_H */
