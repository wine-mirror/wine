/*
 *      imagehlp.h   -       Declarations for IMAGEHLP
 */

#ifndef __WINE_IMAGEHLP_H
#define __WINE_IMAGEHLP_H

#include "toolhelp.h"
#include "wintypes.h"

/***********************************************************************
 * Types
 */

#define ANYSIZE_ARRAY 1 /* FIXME: Move to wintypes.h */

typedef unsigned char UCHAR; /* FIXME: Move to wintypes.h */
typedef UCHAR *PUCHAR;  /* FIXME: Move to wintypes.h */
typedef CHAR *PCHAR;  /* FIXME: Move to wintypes.h */
typedef unsigned char boolean; /* FIXME: Move to wintypes.h */
typedef boolean BOOLEAN; /* FIXME: Move to wintypes.h */
typedef void *PVOID; /* FIXME: Move to wintypes.h */
typedef DWORD *PDWORD; /* FIXME: Move to wintypes.h */
typedef BYTE *PBYTE; /* FIXME: Move to wintypes.h */
typedef ULONG *PULONG; /* FIXME: Move to wintypes.h */
typedef unsigned short USHORT; /* FIXME: Move to wintypes.h */
typedef const void *PCVOID; /* FIXME: Move to wintypes.h */
typedef UINT32 *PUINT32; /* FIXME: Move to wintypes.h */

typedef struct _LIST_ENTRY32 {
  struct _LIST_ENTRY32 *Flink;
  struct _LIST_ENTRY32 *Blink;
} LIST_ENTRY32, *PLIST_ENTRY32; /* FIXME: Move to wintypes.h */

typedef struct _SINGLE_LIST_ENTRY32 {
  struct _SINGLE_LIST_ENTRY32 *Next;
} SINGLE_LIST_ENTRY32, *PSINGLE_LIST_ENTRY32; /* FIXME: Move to wintypes.h */

/* FIXME: Move to wintypes.h */
#define FIELD_OFFSET(type, field) \
  ((LONG)(INT32)&(((type *)0)->field))

/* FIXME: Move to wintypes.h */
#define CONTAINING_RECORD(address, type, field) \
  ((type *)((PCHAR)(address) - (PCHAR)(&((type *)0)->field)))

typedef PVOID DIGEST_HANDLE32; 

/***********************************************************************
 * Enums/Defines
 */

typedef enum _IMAGEHLP_STATUS_REASON32 {
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
} IMAGEHLP_STATUS_REASON32;

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

#define IMAGE_DEBUG_TYPE_UNKNOWN        0
#define IMAGE_DEBUG_TYPE_COFF           1
#define IMAGE_DEBUG_TYPE_CODEVIEW       2
#define IMAGE_DEBUG_TYPE_FPO            3
#define IMAGE_DEBUG_TYPE_MISC           4
#define IMAGE_DEBUG_TYPE_EXCEPTION      5
#define IMAGE_DEBUG_TYPE_FIXUP          6
#define IMAGE_DEBUG_TYPE_OMAP_TO_SRC    7
#define IMAGE_DEBUG_TYPE_OMAP_FROM_SRC  8
#define IMAGE_DEBUG_TYPE_BORLAND        9
#define IMAGE_DEBUG_TYPE_RESERVED10    10

#define FRAME_FPO    0
#define FRAME_TRAP   1
#define FRAME_TSS    2
#define FRAME_NONFPO 3

#define CHECKSUM_SUCCESS         0
#define CHECKSUM_OPEN_FAILURE    1
#define CHECKSUM_MAP_FAILURE     2
#define CHECKSUM_MAPVIEW_FAILURE 3
#define CHECKSUM_UNICODE_FAILURE 4

typedef enum _ADRESS_MODE32 {
  AddrMode1616,
  AddrMode1632,
  AddrModeReal,
  AddrModeFlat
} ADDRESS_MODE32;

#define SYMOPT_CASE_INSENSITIVE  0x00000001
#define SYMOPT_UNDNAME           0x00000002
#define SYMOPT_DEFERRED_LOADS    0x00000004
#define SYMOPT_NO_CPP            0x00000008
#define SYMOPT_LOAD_LINES        0x00000010
#define SYMOPT_OMAP_FIND_NEAREST 0x00000020

#define SYMF_OMAP_GENERATED   0x00000001
#define SYMF_OMAP_MODIFIED    0x00000002

typedef enum _SYM_TYPE32 {
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

#define IMAGE_DOS_SIGNATURE    0x5A4D     /* MZ   */
#define IMAGE_OS2_SIGNATURE    0x454E     /* NE   */
#define IMAGE_OS2_SIGNATURE_LE 0x454C     /* LE   */
#define IMAGE_VXD_SIGNATURE    0x454C     /* LE   */
#define IMAGE_NT_SIGNATURE     0x00004550 /* PE00 */

/***********************************************************************
 * Structures
 */

typedef struct _IMAGE_DATA_DIRECTORY32 {
  DWORD VirtualAddress;
  DWORD Size;
} IMAGE_DATA_DIRECTORY32, *PIMAGE_DATA_DIRECTORY32;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16

typedef struct _IMAGE_OPTIONAL_HEADER32 {

  /* Standard fields */

  WORD  Magic;
  BYTE  MajorLinkerVersion;
  BYTE  MinorLinkerVersion;
  DWORD SizeOfCode;
  DWORD SizeOfInitializedData;
  DWORD SizeOfUninitializedData;
  DWORD AddressOfEntryPoint;
  DWORD BaseOfCode;
  DWORD BaseOfData;

  /* NT additional fields */

  DWORD ImageBase;
  DWORD SectionAlignment;
  DWORD FileAlignment;
  WORD  MajorOperatingSystemVersion;
  WORD  MinorOperatingSystemVersion;
  WORD  MajorImageVersion;
  WORD  MinorImageVersion;
  WORD  MajorSubsystemVersion;
  WORD  MinorSubsystemVersion;
  DWORD Win32VersionValue;
  DWORD SizeOfImage;
  DWORD SizeOfHeaders;
  DWORD CheckSum;
  WORD  Subsystem;
  WORD  DllCharacteristics;
  DWORD SizeOfStackReserve;
  DWORD SizeOfStackCommit;
  DWORD SizeOfHeapReserve;
  DWORD SizeOfHeapCommit;
  DWORD LoaderFlags;
  DWORD NumberOfRvaAndSizes;
  IMAGE_DATA_DIRECTORY32 DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER32, *PIMAGE_OPTIONAL_HEADER32;

typedef struct _IMAGE_FILE_HEADER32 {
  WORD  Machine;
  WORD  NumberOfSections;
  DWORD TimeDateStamp;
  DWORD PointerToSymbolTable;
  DWORD NumberOfSymbols;
  WORD  SizeOfOptionalHeader;
  WORD  Characteristics;
} IMAGE_FILE_HEADER32, *PIMAGE_FILE_HEADER32;

typedef struct _IMAGE_NT_HEADERS32 {
  DWORD Signature;
  IMAGE_FILE_HEADER32 FileHeader;
  IMAGE_OPTIONAL_HEADER32 OptionalHeader;
} IMAGE_NT_HEADERS32, *PIMAGE_NT_HEADERS32;

#define IMAGE_SIZEOF_SHORT_NAME 8

typedef struct _IMAGE_SECTION_HEADER32 {
  BYTE  Name[IMAGE_SIZEOF_SHORT_NAME];
  union {
    DWORD PhysicalAddress;
    DWORD VirtualSize;
  } Misc;
  DWORD VirtualAddress;
  DWORD SizeOfRawData;
  DWORD PointerToRawData;
  DWORD PointerToRelocations;
  DWORD PointerToLinenumbers;
  WORD  NumberOfRelocations;
  WORD  NumberOfLinenumbers;
  DWORD Characteristics;
} IMAGE_SECTION_HEADER32, *PIMAGE_SECTION_HEADER32;

typedef struct _LOADED_IMAGE32 {
  LPSTR                   ModuleName;
  HANDLE32                hFile;
  PUCHAR                  MappedAddress;
  PIMAGE_NT_HEADERS32     FileHeader;
  PIMAGE_SECTION_HEADER32 LastRvaSection;
  ULONG                   NumberOfSections;
  PIMAGE_SECTION_HEADER32 Sections;
  ULONG                   Characteristics;
  BOOLEAN                 fSystemImage;
  BOOLEAN                 fDOSImage;
  LIST_ENTRY32            Links;
  ULONG                   SizeOfImage;
} LOADED_IMAGE32, *PLOADED_IMAGE32;

typedef struct _IMAGE_LOAD_CONFIG_DIRECTORY32 {
  DWORD Characteristics;
  DWORD TimeDateStamp;
  WORD  MajorVersion;
  WORD  MinorVersion;
  DWORD GlobalFlagsClear;
  DWORD GlobalFlagsSet;
  DWORD CriticalSectionDefaultTimeout;
  DWORD DeCommitFreeBlockThreshold;
  DWORD DeCommitTotalFreeThreshold;
  PVOID LockPrefixTable;
  DWORD MaximumAllocationSize;
  DWORD VirtualMemoryThreshold;
  DWORD ProcessHeapFlags;
  DWORD ProcessAffinityMask;
  WORD  CSDVersion;
  WORD  Reserved1;
  PVOID EditList;
  DWORD Reserved[1];
} IMAGE_LOAD_CONFIG_DIRECTORY32, *PIMAGE_LOAD_CONFIG_DIRECTORY32;

typedef struct _WIN_CERTIFICATE32 {
  DWORD dwLength;
  WORD  wRevision;                   /*  WIN_CERT_REVISON_xxx */ 
  WORD  wCertificateType;            /*  WIN_CERT_TYPE_xxx */
  BYTE  bCertificate[ANYSIZE_ARRAY];
} WIN_CERTIFICATE32, *PWIN_CERTIFICATE32;

typedef struct _API_VERSION32 {
  USHORT  MajorVersion;
  USHORT  MinorVersion;
  USHORT  Revision;
  USHORT  Reserved;
} API_VERSION32, *PAPI_VERSION32;

typedef struct _IMAGE_FUNCTION_ENTRY32 {
  DWORD StartingAddress;
  DWORD EndingAddress;
  DWORD EndOfPrologue;
} IMAGE_FUNCTION_ENTRY32, *PIMAGE_FUNCTION_ENTRY32;

typedef struct _IMAGE_DEBUG_DIRECTORY32 {
  DWORD Characteristics;
  DWORD TimeDateStamp;
  WORD  MajorVersion;
  WORD  MinorVersion;
  DWORD Type;
  DWORD SizeOfData;
  DWORD AddressOfRawData;
  DWORD PointerToRawData;
} IMAGE_DEBUG_DIRECTORY32, *PIMAGE_DEBUG_DIRECTORY32;

typedef struct _IMAGE_COFF_SYMBOLS_HEADER32 {
  DWORD NumberOfSymbols;
  DWORD LvaToFirstSymbol;
  DWORD NumberOfLinenumbers;
  DWORD LvaToFirstLinenumber;
  DWORD RvaToFirstByteOfCode;
  DWORD RvaToLastByteOfCode;
  DWORD RvaToFirstByteOfData;
  DWORD RvaToLastByteOfData;
} IMAGE_COFF_SYMBOLS_HEADER32, *PIMAGE_COFF_SYMBOLS_HEADER32;

typedef struct _FPO_DATA32 {
  DWORD ulOffStart;
  DWORD cbProcSize;
  DWORD cdwLocals;
  WORD  cdwParams;
  WORD  cbProlog : 8;
  WORD  cbRegs   : 3;
  WORD  fHasSEH  : 1;
  WORD  fUseBP   : 1;
  WORD  reserved : 1;
  WORD  cbFrame  : 2;
} FPO_DATA32, *PFPO_DATA32;

typedef struct _IMAGE_DEBUG_INFORMATION32 {
  LIST_ENTRY32 List;
  DWORD        Size;
  PVOID        MappedBase;
  USHORT       Machine;
  USHORT       Characteristics;
  DWORD        CheckSum;
  DWORD        ImageBase;
  DWORD        SizeOfImage;
  
  DWORD NumberOfSections;
  PIMAGE_SECTION_HEADER32 Sections;

  DWORD ExportedNamesSize;
  LPSTR ExportedNames;

  DWORD NumberOfFunctionTableEntries;
  PIMAGE_FUNCTION_ENTRY32 FunctionTableEntries;
  DWORD LowestFunctionStartingAddress;
  DWORD HighestFunctionEndingAddress;

  DWORD NumberOfFpoTableEntries;
  PFPO_DATA32 FpoTableEntries;

  DWORD SizeOfCoffSymbols;
  PIMAGE_COFF_SYMBOLS_HEADER32 CoffSymbols;

  DWORD SizeOfCodeViewSymbols;
  PVOID CodeViewSymbols;

  LPSTR ImageFilePath;
  LPSTR ImageFileName;
  LPSTR DebugFilePath;

  DWORD TimeDateStamp;

  BOOL32 RomImage;
  PIMAGE_DEBUG_DIRECTORY32 DebugDirectory;
  DWORD  NumberOfDebugDirectories;

  DWORD Reserved[3];
} IMAGE_DEBUG_INFORMATION32, *PIMAGE_DEBUG_INFORMATION32;

typedef struct _ADDRESS32 {
    DWORD          Offset;
    WORD           Segment;
    ADDRESS_MODE32 Mode;
} ADDRESS32, *PADDRESS32;

typedef struct _KDHELP32 {
  DWORD Thread;
  DWORD ThCallbackStack;
  DWORD NextCallback;
  DWORD FramePointer;
  DWORD KiCallUserMode;
  DWORD KeUserCallbackDispatcher;
  DWORD SystemRangeStart;
} KDHELP32, *PKDHELP32;

typedef struct _STACKFRAME32 {
  ADDRESS32 AddrPC;
  ADDRESS32 AddrReturn;
  ADDRESS32 AddrFrame;
  ADDRESS32 AddrStack;
  PVOID     FuncTableEntry;
  DWORD     Params[4];
  BOOL32    Far;
  BOOL32    Virtual;
  DWORD     Reserved[3];
  KDHELP32  KdHelp;
} STACKFRAME32, *PSTACKFRAME32;

typedef struct _IMAGEHLP_SYMBOL32 {
  DWORD SizeOfStruct;
  DWORD Address;
  DWORD Size;
  DWORD Flags;
  DWORD MaxNameLength;
  CHAR  Name[ANYSIZE_ARRAY];
} IMAGEHLP_SYMBOL32, *PIMAGEHLP_SYMBOL32;

typedef struct _IMAGEHLP_MODULE32 {
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
} IMAGEHLP_MODULE32, *PIMAGEHLP_MODULE32;

typedef struct _IMAGEHLP_LINE32 {
  DWORD SizeOfStruct;
  DWORD Key;
  DWORD LineNumber;
  PCHAR FileName;
  DWORD Address;
} IMAGEHLP_LINE32, *PIMAGEHLP_LINE32;

typedef struct _IMAGEHLP_DEFERRED_SYMBOL_LOAD32 {
  DWORD   SizeOfStruct;
  DWORD   BaseOfImage;
  DWORD   CheckSum;
  DWORD   TimeDateStamp;
  CHAR    FileName[MAX_PATH];
  BOOLEAN Reparse;
} IMAGEHLP_DEFERRED_SYMBOL_LOAD32, *PIMAGEHLP_DEFERRED_SYMBOL_LOAD32;

typedef struct _IMAGEHLP_DUPLICATE_SYMBOL32 {
  DWORD              SizeOfStruct;
  DWORD              NumberOfDups;
  PIMAGEHLP_SYMBOL32 Symbol;
  ULONG              SelectedSymbol;
} IMAGEHLP_DUPLICATE_SYMBOL32, *PIMAGEHLP_DUPLICATE_SYMBOL32;

typedef struct _IMAGE_DOS_HEADER32 {
  WORD e_magic;
  WORD e_cblp;
  WORD e_cp;
  WORD e_crlc;
  WORD e_cparhdr;
  WORD e_minalloc;
  WORD e_maxalloc;
  WORD e_ss;
  WORD e_sp;
  WORD e_csum;
  WORD e_ip;
  WORD e_cs;
  WORD e_lfarlc;
  WORD e_ovno;
  WORD e_res[4];
  WORD e_oemid;
  WORD e_oeminfo;
  WORD e_res2[10];
  LONG e_lfanew;
} IMAGE_DOS_HEADER32, *PIMAGE_DOS_HEADER32;

typedef struct _IMAGE_OS2_HEADER32 {
  WORD ne_magic;
  CHAR ne_ver;
  CHAR ne_rev;
  WORD ne_enttab;
  WORD ne_cbenttab;
  LONG ne_crc;
  WORD ne_flags;
  WORD ne_autodata;
  WORD ne_heap;
  WORD ne_stack;
  LONG ne_csip;
  LONG ne_sssp;
  WORD ne_cseg;
  WORD ne_cmod;
  WORD ne_cbnrestab;
  WORD ne_segtab;
  WORD ne_rsrctab;
  WORD ne_restab;
  WORD ne_modtab;
  WORD ne_imptab;
  LONG ne_nrestab;
  WORD ne_cmovent;
  WORD ne_align;
  WORD ne_cres;
  BYTE ne_exetyp;
  BYTE ne_flagsothers;
  WORD ne_pretthunks;
  WORD ne_psegrefbytes;
  WORD ne_swaparea;
  WORD ne_expver;
} IMAGE_OS2_HEADER32, *PIMAGE_OS2_HEADER32;

typedef struct _IMAGE_VXD_HEADER32 {
  WORD  e32_magic;
  BYTE  e32_border;
  BYTE  e32_worder;
  DWORD e32_level;
  WORD  e32_cpu;
  WORD  e32_os;
  DWORD e32_ver;
  DWORD e32_mflags;
  DWORD e32_mpages;
  DWORD e32_startobj;
  DWORD e32_eip;
  DWORD e32_stackobj;
  DWORD e32_esp;
  DWORD e32_pagesize;
  DWORD e32_lastpagesize;
  DWORD e32_fixupsize;
  DWORD e32_fixupsum;
  DWORD e32_ldrsize;
  DWORD e32_ldrsum;
  DWORD e32_objtab;
  DWORD e32_objcnt;
  DWORD e32_objmap;
  DWORD e32_itermap;
  DWORD e32_rsrctab;
  DWORD e32_rsrccnt;
  DWORD e32_restab;
  DWORD e32_enttab;
  DWORD e32_dirtab;
  DWORD e32_dircnt;
  DWORD e32_fpagetab;
  DWORD e32_frectab;
  DWORD e32_impmod;
  DWORD e32_impmodcnt;
  DWORD e32_impproc;
  DWORD e32_pagesum;
  DWORD e32_datapage;
  DWORD e32_preload;
  DWORD e32_nrestab;
  DWORD e32_cbnrestab;
  DWORD e32_nressum;
  DWORD e32_autodata;
  DWORD e32_debuginfo;
  DWORD e32_debuglen;
  DWORD e32_instpreload;
  DWORD e32_instdemand;
  DWORD e32_heapsize;
  BYTE   e32_res3[12];
  DWORD e32_winresoff;
  DWORD e32_winreslen;
  WORD  e32_devid;
  WORD  e32_ddkver;
} IMAGE_VXD_HEADER32, *PIMAGE_VXD_HEADER32;

/***********************************************************************
 * Callbacks
 */

typedef BOOL32 (CALLBACK *PIMAGEHLP_STATUS_ROUTINE32)(
  IMAGEHLP_STATUS_REASON32 Reason, LPSTR ImageName, LPSTR DllName,
  ULONG Va, ULONG Parameter
);

typedef BOOL32 (CALLBACK *PSYM_ENUMMODULES_CALLBACK32)(
  LPSTR ModuleName, ULONG BaseOfDll, PVOID UserContext
);

typedef BOOL32 (CALLBACK *PSYM_ENUMSYMBOLS_CALLBACK32)(
  LPSTR SymbolName, ULONG SymbolAddress, ULONG SymbolSize, 
  PVOID UserContext
);

typedef BOOL32 (CALLBACK *PENUMLOADED_MODULES_CALLBACK32)(
  LPSTR ModuleName, ULONG ModuleBase, ULONG ModuleSize, 
  PVOID UserContext
);

typedef BOOL32 (CALLBACK *PSYMBOL_REGISTERED_CALLBACK32)(
  HANDLE32 hProcess, ULONG ActionCode, PVOID CallbackData,
  PVOID UserContext
);

typedef BOOL32 (CALLBACK *DIGEST_FUNCTION32)(
  DIGEST_HANDLE32 refdata, PBYTE pData, DWORD dwLength
);

typedef BOOL32 (CALLBACK *PREAD_PROCESS_MEMORY_ROUTINE32)(
  HANDLE32  hProcess, PCVOID lpBaseAddress, PVOID lpBuffer,
  DWORD nSize, PDWORD lpNumberOfBytesRead
);

typedef PVOID (CALLBACK *PFUNCTION_TABLE_ACCESS_ROUTINE32)(
  HANDLE32 hProcess, DWORD AddrBase
);

typedef DWORD (CALLBACK *PGET_MODULE_BASE_ROUTINE32)(
  HANDLE32 hProcess, DWORD ReturnAddress);

typedef DWORD (CALLBACK *PTRANSLATE_ADDRESS_ROUTINE32)(
  HANDLE32 hProcess, HANDLE32 hThread, PADDRESS32 lpaddr
);

/***********************************************************************
 * Functions
 */

BOOL32 WINAPI BindImage32(
  LPSTR ImageName, LPSTR DllPath, LPSTR SymbolPath
);
BOOL32 WINAPI BindImageEx32(
  DWORD Flags, LPSTR ImageName, LPSTR DllPath, LPSTR SymbolPath,
  PIMAGEHLP_STATUS_ROUTINE32 StatusRoutine
);
PIMAGE_NT_HEADERS32 WINAPI CheckSumMappedFile32(
  LPVOID BaseAddress, DWORD FileLength, 
  LPDWORD HeaderSum, LPDWORD CheckSum
);
BOOL32 WINAPI EnumerateLoadedModules32(
  HANDLE32 hProcess,
  PENUMLOADED_MODULES_CALLBACK32 EnumLoadedModulesCallback,
  PVOID UserContext
);
HANDLE32 WINAPI FindDebugInfoFile32(
  LPSTR FileName, LPSTR SymbolPath, LPSTR DebugFilePath
);
HANDLE32 WINAPI FindExecutableImage32(
  LPSTR FileName, LPSTR SymbolPath, LPSTR ImageFilePath
);
BOOL32 WINAPI GetImageConfigInformation32(
  PLOADED_IMAGE32 LoadedImage, 
  PIMAGE_LOAD_CONFIG_DIRECTORY32 ImageConfigInformation
);
DWORD WINAPI GetImageUnusedHeaderBytes32(
  PLOADED_IMAGE32 LoadedImage,
  LPDWORD SizeUnusedHeaderBytes
);
DWORD WINAPI GetTimestampForLoadedLibrary32(
  HMODULE32 Module
);
BOOL32 WINAPI ImageAddCertificate32(
  HANDLE32 FileHandle, PWIN_CERTIFICATE32 Certificate, PDWORD Index
);
PVOID WINAPI ImageDirectoryEntryToData32(
  PVOID Base, BOOLEAN MappedAsImage, USHORT DirectoryEntry, PULONG Size
);
BOOL32 WINAPI ImageEnumerateCertificates32(
  HANDLE32 FileHandle, WORD TypeFilter, PDWORD CertificateCount,
  PDWORD Indices, DWORD IndexCount
);
BOOL32 WINAPI ImageGetCertificateData32(
  HANDLE32 FileHandle, DWORD CertificateIndex,
  PWIN_CERTIFICATE32 Certificate, PDWORD RequiredLength
);
BOOL32 WINAPI ImageGetCertificateHeader32(
  HANDLE32 FileHandle, DWORD CertificateIndex,
  PWIN_CERTIFICATE32 Certificateheader
);
BOOL32 WINAPI ImageGetDigestStream32(
  HANDLE32 FileHandle, DWORD DigestLevel,
  DIGEST_FUNCTION32 DigestFunction, DIGEST_HANDLE32 DigestHandle
);
PLOADED_IMAGE32 WINAPI ImageLoad32(
  LPSTR DllName, LPSTR DllPath
);
PIMAGE_NT_HEADERS32 WINAPI ImageNtHeader32(
  PVOID Base
);
BOOL32 WINAPI ImageRemoveCertificate32(
  HANDLE32 FileHandle, DWORD Index
);
PIMAGE_SECTION_HEADER32 WINAPI ImageRvaToSection32(
  PIMAGE_NT_HEADERS32 NtHeaders, PVOID Base, ULONG Rva
);
PVOID WINAPI ImageRvaToVa32(
  PIMAGE_NT_HEADERS32 NtHeaders, PVOID Base, ULONG Rva,
  PIMAGE_SECTION_HEADER32 *LastRvaSection
);
BOOL32 WINAPI ImageUnload32(
  PLOADED_IMAGE32 LoadedImage
);
PAPI_VERSION32 WINAPI ImagehlpApiVersion32(
);
PAPI_VERSION32 WINAPI ImagehlpApiVersionEx32(
  PAPI_VERSION32 AppVersion
);
BOOL32 WINAPI MakeSureDirectoryPathExists32(
  LPCSTR DirPath
);
BOOL32 WINAPI MapAndLoad32(
  LPSTR ImageName, LPSTR DllPath, PLOADED_IMAGE32 LoadedImage,
  BOOL32 DotDll, BOOL32 ReadOnly
);
PIMAGE_DEBUG_INFORMATION32 WINAPI MapDebugInformation32(
  HANDLE32 FileHandle, LPSTR FileName,
  LPSTR SymbolPath, DWORD ImageBase
);
DWORD WINAPI MapFileAndCheckSum32A(
  LPSTR Filename, LPDWORD HeaderSum, LPDWORD CheckSum
);
DWORD WINAPI MapFileAndCheckSum32W(
  LPWSTR Filename, LPDWORD HeaderSum, LPDWORD CheckSum
);
BOOL32 WINAPI ReBaseImage32(
  LPSTR CurrentImageName, LPSTR SymbolPath, BOOL32 fReBase,
  BOOL32 fRebaseSysfileOk, BOOL32 fGoingDown, ULONG CheckImageSize,
  ULONG *OldImageSize, ULONG *OldImageBase, ULONG *NewImageSize,
  ULONG *NewImageBase, ULONG TimeStamp
);
BOOL32 WINAPI RemovePrivateCvSymbolic32(
  PCHAR DebugData, PCHAR *NewDebugData, ULONG *NewDebugSize
);
VOID WINAPI RemoveRelocations32(
  PCHAR ImageName
);
BOOL32 WINAPI SearchTreeForFile32(
  LPSTR RootPath, LPSTR InputPathName, LPSTR OutputPathBuffer
);
BOOL32 WINAPI SetImageConfigInformation32(
  PLOADED_IMAGE32 LoadedImage,
  PIMAGE_LOAD_CONFIG_DIRECTORY32 ImageConfigInformation
);
BOOL32 WINAPI SplitSymbols32(
  LPSTR ImageName, LPSTR SymbolsPath, 
  LPSTR SymbolFilePath, DWORD Flags
);
BOOL32 WINAPI StackWalk32(
  DWORD MachineType, HANDLE32 hProcess, HANDLE32 hThread,
  PSTACKFRAME32 StackFrame, PVOID ContextRecord,
  PREAD_PROCESS_MEMORY_ROUTINE32 ReadMemoryRoutine,
  PFUNCTION_TABLE_ACCESS_ROUTINE32 FunctionTableAccessRoutine,
  PGET_MODULE_BASE_ROUTINE32 GetModuleBaseRoutine,
  PTRANSLATE_ADDRESS_ROUTINE32 TranslateAddress
);
BOOL32 WINAPI SymCleanup32(
  HANDLE32 hProcess
);
BOOL32 WINAPI SymEnumerateModules32(
  HANDLE32 hProcess, PSYM_ENUMMODULES_CALLBACK32 EnumModulesCallback,
  PVOID UserContext
);
BOOL32 WINAPI SymEnumerateSymbols32(
  HANDLE32 hProcess, DWORD BaseOfDll,
  PSYM_ENUMSYMBOLS_CALLBACK32 EnumSymbolsCallback, PVOID UserContext
);
PVOID WINAPI SymFunctionTableAccess32(
  HANDLE32 hProcess, DWORD AddrBase
);
DWORD WINAPI SymGetModuleBase32(
  HANDLE32 hProcess, DWORD dwAddr
);
BOOL32 WINAPI SymGetModuleInfo32(
  HANDLE32 hProcess, DWORD dwAddr,
  PIMAGEHLP_MODULE32 ModuleInfo
);
DWORD WINAPI SymGetOptions32(
);
BOOL32 WINAPI SymGetSearchPath32(
  HANDLE32 hProcess, LPSTR szSearchPath, DWORD SearchPathLength
);
BOOL32 WINAPI SymGetSymFromAddr32(
  HANDLE32 hProcess, DWORD dwAddr,
  PDWORD pdwDisplacement, PIMAGEHLP_SYMBOL32 Symbol
);
BOOL32 WINAPI SymGetSymFromName32(
  HANDLE32 hProcess, LPSTR Name, PIMAGEHLP_SYMBOL32 Symbol
);
BOOL32 WINAPI SymGetSymNext32(
  HANDLE32 hProcess, PIMAGEHLP_SYMBOL32 Symbol
);
BOOL32 WINAPI SymGetSymPrev32(
  HANDLE32 hProcess, PIMAGEHLP_SYMBOL32 Symbol
);
BOOL32 WINAPI SymInitialize32(
  HANDLE32 hProcess, LPSTR UserSearchPath, BOOL32 fInvadeProcess
);
BOOL32 WINAPI SymLoadModule32(
  HANDLE32 hProcess, HANDLE32 hFile, LPSTR ImageName, LPSTR ModuleName,
  DWORD BaseOfDll, DWORD SizeOfDll
);
BOOL32 WINAPI SymRegisterCallback32(
  HANDLE32 hProcess, PSYMBOL_REGISTERED_CALLBACK32 CallbackFunction,
  PVOID UserContext
);
DWORD WINAPI SymSetOptions32(
  DWORD SymOptions
);
BOOL32 WINAPI SymSetSearchPath32(
  HANDLE32 hProcess, LPSTR szSearchPath
);
BOOL32 WINAPI SymUnDName32(
  PIMAGEHLP_SYMBOL32 sym, LPSTR UnDecName, DWORD UnDecNameLength
);
BOOL32 WINAPI SymUnloadModule32(
  HANDLE32 hProcess, DWORD BaseOfDll
);
BOOL32 WINAPI TouchFileTimes32(
  HANDLE32 FileHandle, LPSYSTEMTIME lpSystemTime
);
DWORD WINAPI UnDecorateSymbolName32(
  LPCSTR DecoratedName, LPSTR UnDecoratedName,
  DWORD UndecoratedLength, DWORD Flags
);
BOOL32 WINAPI UnMapAndLoad32(
  PLOADED_IMAGE32 LoadedImage
);
BOOL32 WINAPI UnmapDebugInformation32(
  PIMAGE_DEBUG_INFORMATION32 DebugInfo
);
BOOL32 WINAPI UpdateDebugInfoFile32(
  LPSTR ImageFileName, LPSTR SymbolPath,
  LPSTR DebugFilePath, PIMAGE_NT_HEADERS32 NtHeaders
);
BOOL32 WINAPI UpdateDebugInfoFileEx32(
  LPSTR ImageFileName, LPSTR SymbolPath, LPSTR DebugFilePath,
  PIMAGE_NT_HEADERS32 NtHeaders, DWORD OldChecksum
);

/***********************************************************************
 * Wine specific
 */

extern HANDLE32 IMAGEHLP_hHeap32;

#endif  /* __WINE_IMAGEHLP_H */



