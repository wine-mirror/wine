/*
 * Declarations for DBGHELP
 *
 * Copyright (C) 2003 Eric Pouech
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_DBGHELP_H
#define __WINE_DBGHELP_H

#include <minidumpapiset.h>

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#ifdef _WIN64
#ifndef _IMAGEHLP64
#define _IMAGEHLP64
#endif
#endif

#ifdef _IMAGEHLP_SOURCE_
#define IMAGEAPI WINAPI
#else
#define IMAGEAPI DECLSPEC_IMPORT WINAPI
#endif

#define DBHLPAPI IMAGEAPI

typedef struct _LOADED_IMAGE
{
    PSTR                        ModuleName;
    HANDLE                      hFile;
    PUCHAR                      MappedAddress;
    PIMAGE_NT_HEADERS           FileHeader;
    PIMAGE_SECTION_HEADER       LastRvaSection;
    ULONG                       NumberOfSections;
    PIMAGE_SECTION_HEADER       Sections;
    ULONG                       Characteristics;
    BOOLEAN                     fSystemImage;
    BOOLEAN                     fDOSImage;
    BOOLEAN                     fReadOnly;
    UCHAR                       Version;
    LIST_ENTRY                  Links;
    ULONG                       SizeOfImage;
} LOADED_IMAGE, *PLOADED_IMAGE;

/*************************
 *    IMAGEHLP equiv     *
 *************************/

typedef enum
{
    AddrMode1616,
    AddrMode1632,
    AddrModeReal,
    AddrModeFlat
} ADDRESS_MODE;

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define ADDRESS ADDRESS64
#define LPADDRESS LPADDRESS64
#else
typedef struct _tagADDRESS
{
    DWORD                       Offset;
    WORD                        Segment;
    ADDRESS_MODE                Mode;
} ADDRESS, *LPADDRESS;
#endif

typedef struct _tagADDRESS64
{
    DWORD64                     Offset;
    WORD                        Segment;
    ADDRESS_MODE                Mode;
} ADDRESS64, *LPADDRESS64;

#define SYMF_OMAP_GENERATED   0x00000001
#define SYMF_OMAP_MODIFIED    0x00000002
#define SYMF_USER_GENERATED   0x00000004
#define SYMF_REGISTER         0x00000008
#define SYMF_REGREL           0x00000010
#define SYMF_FRAMEREL         0x00000020
#define SYMF_PARAMETER        0x00000040
#define SYMF_LOCAL            0x00000080
#define SYMF_CONSTANT         0x00000100
#define SYMF_EXPORT           0x00000200
#define SYMF_FORWARDER        0x00000400
#define SYMF_FUNCTION         0x00000800
#define SYMF_VIRTUAL          0x00001000
#define SYMF_THUNK            0x00002000
#define SYMF_TLSREL           0x00004000

typedef enum 
{
    SymNone = 0,
    SymCoff,
    SymCv,
    SymPdb,
    SymExport,
    SymDeferred,
    SymSym,
    SymDia,
    SymVirtual,
    NumSymTypes
} SYM_TYPE;

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define IMAGEHLP_SYMBOL IMAGEHLP_SYMBOL64
#define IMAGEHLP_SYMBOLW IMAGEHLP_SYMBOLW64
#define PIMAGEHLP_SYMBOL PIMAGEHLP_SYMBOL64
#define PIMAGEHLP_SYMBOLW PIMAGEHLP_SYMBOLW64
#else
typedef struct _IMAGEHLP_SYMBOL
{
    DWORD                       SizeOfStruct;
    DWORD                       Address;
    DWORD                       Size;
    DWORD                       Flags;
    DWORD                       MaxNameLength;
    CHAR                        Name[1];
} IMAGEHLP_SYMBOL, *PIMAGEHLP_SYMBOL;

typedef struct _IMAGEHLP_SYMBOLW
{
    DWORD                       SizeOfStruct;
    DWORD                       Address;
    DWORD                       Size;
    DWORD                       Flags;
    DWORD                       MaxNameLength;
    WCHAR                       Name[1];
} IMAGEHLP_SYMBOLW, *PIMAGEHLP_SYMBOLW;
#endif

typedef struct _IMAGEHLP_SYMBOL64
{
    DWORD                       SizeOfStruct;
    DWORD64                     Address;
    DWORD                       Size;
    DWORD                       Flags;
    DWORD                       MaxNameLength;
    CHAR                        Name[1];
} IMAGEHLP_SYMBOL64, *PIMAGEHLP_SYMBOL64;

typedef struct _IMAGEHLP_SYMBOLW64
{
    DWORD                       SizeOfStruct;
    DWORD64                     Address;
    DWORD                       Size;
    DWORD                       Flags;
    DWORD                       MaxNameLength;
    WCHAR                       Name[1];
} IMAGEHLP_SYMBOLW64, *PIMAGEHLP_SYMBOLW64;

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define IMAGEHLP_MODULE IMAGEHLP_MODULE64
#define PIMAGEHLP_MODULE PIMAGEHLP_MODULE64
#define IMAGEHLP_MODULEW IMAGEHLP_MODULEW64
#define PIMAGEHLP_MODULEW PIMAGEHLP_MODULEW64
#else
typedef struct _IMAGEHLP_MODULE
{
    DWORD                       SizeOfStruct;
    DWORD                       BaseOfImage;
    DWORD                       ImageSize;
    DWORD                       TimeDateStamp;
    DWORD                       CheckSum;
    DWORD                       NumSyms;
    SYM_TYPE                    SymType;
    CHAR                        ModuleName[32];
    CHAR                        ImageName[256];
    CHAR                        LoadedImageName[256];
} IMAGEHLP_MODULE, *PIMAGEHLP_MODULE;

typedef struct _IMAGEHLP_MODULEW
{
    DWORD                       SizeOfStruct;
    DWORD                       BaseOfImage;
    DWORD                       ImageSize;
    DWORD                       TimeDateStamp;
    DWORD                       CheckSum;
    DWORD                       NumSyms;
    SYM_TYPE                    SymType;
    WCHAR                       ModuleName[32];
    WCHAR                       ImageName[256];
    WCHAR                       LoadedImageName[256];
} IMAGEHLP_MODULEW, *PIMAGEHLP_MODULEW;
#endif

typedef struct _IMAGEHLP_MODULE64
{
    DWORD                       SizeOfStruct;
    DWORD64                     BaseOfImage;
    DWORD                       ImageSize;
    DWORD                       TimeDateStamp;
    DWORD                       CheckSum;
    DWORD                       NumSyms;
    SYM_TYPE                    SymType;
    CHAR                        ModuleName[32];
    CHAR                        ImageName[256];
    CHAR                        LoadedImageName[256];
    CHAR                        LoadedPdbName[256];
    DWORD                       CVSig;
    CHAR                        CVData[MAX_PATH*3];
    DWORD                       PdbSig;
    GUID                        PdbSig70;
    DWORD                       PdbAge;
    BOOL                        PdbUnmatched;
    BOOL                        DbgUnmatched;
    BOOL                        LineNumbers;
    BOOL                        GlobalSymbols;
    BOOL                        TypeInfo;
    BOOL                        SourceIndexed;
    BOOL                        Publics;
    DWORD                       MachineType;
    DWORD                       Reserved;
} IMAGEHLP_MODULE64, *PIMAGEHLP_MODULE64;

typedef struct _IMAGEHLP_MODULEW64
{
    DWORD                       SizeOfStruct;
    DWORD64                     BaseOfImage;
    DWORD                       ImageSize;
    DWORD                       TimeDateStamp;
    DWORD                       CheckSum;
    DWORD                       NumSyms;
    SYM_TYPE                    SymType;
    WCHAR                       ModuleName[32];
    WCHAR                       ImageName[256];
    WCHAR                       LoadedImageName[256];
    WCHAR                       LoadedPdbName[256];
    DWORD                       CVSig;
    WCHAR                       CVData[MAX_PATH*3];
    DWORD                       PdbSig;
    GUID                        PdbSig70;
    DWORD                       PdbAge;
    BOOL                        PdbUnmatched;
    BOOL                        DbgUnmatched;
    BOOL                        LineNumbers;
    BOOL                        GlobalSymbols;
    BOOL                        TypeInfo;
    BOOL                        SourceIndexed;
    BOOL                        Publics;
    DWORD                       MachineType;
    DWORD                       Reserved;
} IMAGEHLP_MODULEW64, *PIMAGEHLP_MODULEW64;

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define IMAGEHLP_LINE IMAGEHLP_LINE64
#define PIMAGEHLP_LINE PIMAGEHLP_LINE64
#define IMAGEHLP_LINEW IMAGEHLP_LINEW64
#define PIMAGEHLP_LINEW PIMAGEHLP_LINEW64
#else
typedef struct _IMAGEHLP_LINE
{
    DWORD                       SizeOfStruct;
    PVOID                       Key;
    DWORD                       LineNumber;
    PCHAR                       FileName;
    DWORD                       Address;
} IMAGEHLP_LINE, *PIMAGEHLP_LINE;

typedef struct _IMAGEHLP_LINEW
{
    DWORD                       SizeOfStruct;
    PVOID                       Key;
    DWORD                       LineNumber;
    PWSTR                       FileName;
    DWORD                       Address;
} IMAGEHLP_LINEW, *PIMAGEHLP_LINEW;
#endif

typedef struct _IMAGEHLP_LINE64
{
    DWORD                       SizeOfStruct;
    PVOID                       Key;
    DWORD                       LineNumber;
    PCHAR                       FileName;
    DWORD64                     Address;
} IMAGEHLP_LINE64, *PIMAGEHLP_LINE64;

typedef struct _IMAGEHLP_LINEW64
{
    DWORD                       SizeOfStruct;
    PVOID                       Key;
    DWORD                       LineNumber;
    PWSTR                       FileName;
    DWORD64                     Address;
} IMAGEHLP_LINEW64, *PIMAGEHLP_LINEW64;

typedef struct _SOURCEFILE
{
    DWORD64                     ModBase;
    PCHAR                       FileName;
} SOURCEFILE, *PSOURCEFILE;

typedef struct _SOURCEFILEW
{
    DWORD64                     ModBase;
    PWSTR                       FileName;
} SOURCEFILEW, *PSOURCEFILEW;

#define CBA_DEFERRED_SYMBOL_LOAD_START                  0x00000001
#define CBA_DEFERRED_SYMBOL_LOAD_COMPLETE               0x00000002
#define CBA_DEFERRED_SYMBOL_LOAD_FAILURE                0x00000003
#define CBA_SYMBOLS_UNLOADED                            0x00000004
#define CBA_DUPLICATE_SYMBOL                            0x00000005
#define CBA_READ_MEMORY                                 0x00000006
#define CBA_DEFERRED_SYMBOL_LOAD_CANCEL                 0x00000007
#define CBA_SET_OPTIONS                                 0x00000008
#define CBA_EVENT                                       0x00000010
#define CBA_DEFERRED_SYMBOL_LOAD_PARTIAL                0x00000020
#define CBA_DEBUG_INFO                                  0x10000000
#define CBA_SRCSRV_INFO                                 0x20000000
#define CBA_SRCSRV_EVENT                                0x40000000

typedef struct _IMAGEHLP_CBA_READ_MEMORY
{
    DWORD64   addr;
    PVOID     buf;
    DWORD     bytes;
    DWORD    *bytesread;
} IMAGEHLP_CBA_READ_MEMORY, *PIMAGEHLP_CBA_READ_MEMORY;

enum
{
    sevInfo = 0,
    sevProblem,
    sevAttn,
    sevFatal,
    sevMax
};

#define EVENT_SRCSPEW_START 100
#define EVENT_SRCSPEW       100
#define EVENT_SRCSPEW_END   199

typedef struct _IMAGEHLP_CBA_EVENT
{
    DWORD       severity;
    DWORD       code;
    PCHAR       desc;
    PVOID       object;
} IMAGEHLP_CBA_EVENT, *PIMAGEHLP_CBA_EVENT;

typedef struct _IMAGEHLP_CBA_EVENTW
{
    DWORD       severity;
    DWORD       code;
    PCWSTR      desc;
    PVOID       object;
} IMAGEHLP_CBA_EVENTW, *PIMAGEHLP_CBA_EVENTW;

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define IMAGEHLP_DEFERRED_SYMBOL_LOAD IMAGEHLP_DEFERRED_SYMBOL_LOAD64
#define PIMAGEHLP_DEFERRED_SYMBOL_LOAD PIMAGEHLP_DEFERRED_SYMBOL_LOAD64
#else
typedef struct _IMAGEHLP_DEFERRED_SYMBOL_LOAD
{
    DWORD                       SizeOfStruct;
    DWORD                       BaseOfImage;
    DWORD                       CheckSum;
    DWORD                       TimeDateStamp;
    CHAR                        FileName[MAX_PATH];
    BOOLEAN                     Reparse;
    HANDLE                      hFile;
} IMAGEHLP_DEFERRED_SYMBOL_LOAD, *PIMAGEHLP_DEFERRED_SYMBOL_LOAD;
#endif

typedef struct _IMAGEHLP_DEFERRED_SYMBOL_LOAD64
{
    DWORD                       SizeOfStruct;
    DWORD64                     BaseOfImage;
    DWORD                       CheckSum;
    DWORD                       TimeDateStamp;
    CHAR                        FileName[MAX_PATH];
    BOOLEAN                     Reparse;
    HANDLE                      hFile;
    DWORD                       Flags;
} IMAGEHLP_DEFERRED_SYMBOL_LOAD64, *PIMAGEHLP_DEFERRED_SYMBOL_LOAD64;

typedef struct _IMAGEHLP_DEFERRED_SYMBOL_LOADW64
{
    DWORD                       SizeOfStruct;
    DWORD64                     BaseOfImage;
    DWORD                       CheckSum;
    DWORD                       TimeDateStamp;
    WCHAR                       FileName[MAX_PATH + 1];
    BOOLEAN                     Reparse;
    HANDLE                      hFile;
    DWORD                       Flags;
} IMAGEHLP_DEFERRED_SYMBOL_LOADW64, *PIMAGEHLP_DEFERRED_SYMBOL_LOADW64;

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define IMAGEHLP_DUPLICATE_SYMBOL IMAGEHLP_DUPLICATE_SYMBOL64
#define PIMAGEHLP_DUPLICATE_SYMBOL PIMAGEHLP_DUPLICATE_SYMBOL64
#else
typedef struct _IMAGEHLP_DUPLICATE_SYMBOL
{
    DWORD                       SizeOfStruct;
    DWORD                       NumberOfDups;
    PIMAGEHLP_SYMBOL            Symbol;
    DWORD                       SelectedSymbol;
} IMAGEHLP_DUPLICATE_SYMBOL, *PIMAGEHLP_DUPLICATE_SYMBOL;
#endif

typedef struct _IMAGEHLP_DUPLICATE_SYMBOL64
{
    DWORD                       SizeOfStruct;
    DWORD                       NumberOfDups;
    PIMAGEHLP_SYMBOL64          Symbol;
    DWORD                       SelectedSymbol;
} IMAGEHLP_DUPLICATE_SYMBOL64, *PIMAGEHLP_DUPLICATE_SYMBOL64;

#define SYMOPT_CASE_INSENSITIVE         0x00000001
#define SYMOPT_UNDNAME                  0x00000002
#define SYMOPT_DEFERRED_LOADS           0x00000004
#define SYMOPT_NO_CPP                   0x00000008
#define SYMOPT_LOAD_LINES               0x00000010
#define SYMOPT_OMAP_FIND_NEAREST        0x00000020
#define SYMOPT_LOAD_ANYTHING            0x00000040
#define SYMOPT_IGNORE_CVREC             0x00000080
#define SYMOPT_NO_UNQUALIFIED_LOADS     0x00000100
#define SYMOPT_FAIL_CRITICAL_ERRORS     0x00000200
#define SYMOPT_EXACT_SYMBOLS            0x00000400
#define SYMOPT_WILD_UNDERSCORE          0x00000800
#define SYMOPT_USE_DEFAULTS             0x00001000
/* latest SDK defines:
#define SYMOPT_ALLOW_ABSOLUTE_SYMBOLS   0x00000800
#define SYMOPT_IGNORE_NT_SYMPATH        0x00001000
*/
#define SYMOPT_INCLUDE_32BIT_MODULES    0x00002000
#define SYMOPT_PUBLICS_ONLY             0x00004000
#define SYMOPT_NO_PUBLICS               0x00008000
#define SYMOPT_AUTO_PUBLICS             0x00010000
#define SYMOPT_NO_IMAGE_SEARCH          0x00020000
#define SYMOPT_SECURE                   0x00040000
#define SYMOPT_NO_PROMPTS               0x00080000
#define SYMOPT_OVERWRITE                0x00100000
#define SYMOPT_IGNORE_IMAGEDIR          0x00200000
#define SYMOPT_FLAT_DIRECTORY           0x00400000
#define SYMOPT_FAVOR_COMPRESSED         0x00800000
#define SYMOPT_ALLOW_ZERO_ADDRESS       0x01000000
#define SYMOPT_DISABLE_SYMSRV_AUTODETECT  0x02000000
#define SYMOPT_READONLY_CACHE             0x04000000
#define SYMOPT_SYMPATH_LAST               0x08000000
#define SYMOPT_DISABLE_FAST_SYMBOLS       0x10000000
#define SYMOPT_DISABLE_SYMSRV_TIMEOUT     0x20000000
#define SYMOPT_DISABLE_SRVSTAR_ON_STARTUP 0x40000000
#define SYMOPT_DEBUG                      0x80000000

typedef struct _IMAGEHLP_STACK_FRAME
{
    ULONG64     InstructionOffset;
    ULONG64     ReturnOffset;
    ULONG64     FrameOffset;
    ULONG64     StackOffset;
    ULONG64     BackingStoreOffset;
    ULONG64     FuncTableEntry;
    ULONG64     Params[4];
    ULONG64     Reserved[5];
    BOOL        Virtual;
    ULONG       Reserved2;
} IMAGEHLP_STACK_FRAME, *PIMAGEHLP_STACK_FRAME;

typedef VOID IMAGEHLP_CONTEXT, *PIMAGEHLP_CONTEXT;

#define DBHHEADER_DEBUGDIRS     0x1
typedef struct _DBGHELP_MODLOAD_DATA
{
    DWORD               ssize;
    DWORD               ssig;
    PVOID               data;
    DWORD               size;
    DWORD               flags;
} MODLOAD_DATA, *PMODLOAD_DATA;

/*************************
 *    MODULE handling    *
 *************************/

/* flags for SymLoadModuleEx */
#define SLMFLAG_VIRTUAL         0x1
#define SLMFLAG_NO_SYMBOLS      0x4

typedef BOOL (CALLBACK *PENUMLOADED_MODULES_CALLBACK64)(PCSTR, DWORD64, ULONG, PVOID);
BOOL    IMAGEAPI EnumerateLoadedModules64(HANDLE, PENUMLOADED_MODULES_CALLBACK64, PVOID);
typedef BOOL (CALLBACK *PENUMLOADED_MODULES_CALLBACKW64)(PCWSTR, DWORD64, ULONG, PVOID);
BOOL    IMAGEAPI EnumerateLoadedModulesW64(HANDLE, PENUMLOADED_MODULES_CALLBACKW64, PVOID);
BOOL    IMAGEAPI EnumerateLoadedModulesEx(HANDLE, PENUMLOADED_MODULES_CALLBACK64, PVOID);
BOOL    IMAGEAPI EnumerateLoadedModulesExW(HANDLE, PENUMLOADED_MODULES_CALLBACKW64, PVOID);
typedef BOOL (CALLBACK *PSYM_ENUMMODULES_CALLBACK64)(PCSTR, DWORD64, PVOID);
BOOL    IMAGEAPI SymEnumerateModules64(HANDLE, PSYM_ENUMMODULES_CALLBACK64, PVOID);
typedef BOOL (CALLBACK *PSYM_ENUMMODULES_CALLBACKW64)(PCWSTR, DWORD64, PVOID);
BOOL    IMAGEAPI SymEnumerateModulesW64(HANDLE, PSYM_ENUMMODULES_CALLBACKW64, PVOID);
BOOL    IMAGEAPI SymGetModuleInfo64(HANDLE, DWORD64, PIMAGEHLP_MODULE64);
BOOL    IMAGEAPI SymGetModuleInfoW64(HANDLE, DWORD64, PIMAGEHLP_MODULEW64);
DWORD64 IMAGEAPI SymGetModuleBase64(HANDLE, DWORD64);
DWORD64 IMAGEAPI SymLoadModule64(HANDLE, HANDLE, PCSTR, PCSTR, DWORD64, DWORD);
DWORD64 IMAGEAPI SymLoadModuleEx(HANDLE, HANDLE, PCSTR, PCSTR, DWORD64, DWORD, PMODLOAD_DATA, DWORD);
DWORD64 IMAGEAPI SymLoadModuleExW(HANDLE, HANDLE, PCWSTR, PCWSTR, DWORD64, DWORD, PMODLOAD_DATA, DWORD);
BOOL    IMAGEAPI SymUnloadModule64(HANDLE, DWORD64);
BOOL    IMAGEAPI SymRefreshModuleList(HANDLE);

/*************************
 *    Symbol Handling    *
 *************************/

#define IMAGEHLP_SYMBOL_INFO_VALUEPRESENT          1
#define IMAGEHLP_SYMBOL_INFO_REGISTER              SYMF_REGISTER        /*  0x08 */
#define IMAGEHLP_SYMBOL_INFO_REGRELATIVE           SYMF_REGREL          /*  0x10 */
#define IMAGEHLP_SYMBOL_INFO_FRAMERELATIVE         SYMF_FRAMEREL        /*  0x20 */
#define IMAGEHLP_SYMBOL_INFO_PARAMETER             SYMF_PARAMETER       /*  0x40 */
#define IMAGEHLP_SYMBOL_INFO_LOCAL                 SYMF_LOCAL           /*  0x80 */
#define IMAGEHLP_SYMBOL_INFO_CONSTANT              SYMF_CONSTANT        /* 0x100 */
#define IMAGEHLP_SYMBOL_FUNCTION                   SYMF_FUNCTION        /* 0x800 */

#define SYMFLAG_VALUEPRESENT     0x00000001
#define SYMFLAG_REGISTER         0x00000008
#define SYMFLAG_REGREL           0x00000010
#define SYMFLAG_FRAMEREL         0x00000020
#define SYMFLAG_PARAMETER        0x00000040
#define SYMFLAG_LOCAL            0x00000080
#define SYMFLAG_CONSTANT         0x00000100
#define SYMFLAG_EXPORT           0x00000200
#define SYMFLAG_FORWARDER        0x00000400
#define SYMFLAG_FUNCTION         0x00000800
#define SYMFLAG_VIRTUAL          0x00001000
#define SYMFLAG_THUNK            0x00002000
#define SYMFLAG_TLSREL           0x00004000
#define SYMFLAG_SLOT             0x00008000
#define SYMFLAG_ILREL            0x00010000
#define SYMFLAG_METADATA         0x00020000
#define SYMFLAG_CLR_TOKEN        0x00040000
#define SYMFLAG_NULL             0x00080000
#define SYMFLAG_FUNC_NO_RETURN   0x00100000
#define SYMFLAG_SYNTHETIC_ZEROBASE 0x00200000
#define SYMFLAG_PUBLIC_CODE      0x00400000

#define MAX_SYM_NAME    2000

typedef struct _SYMBOL_INFO
{
    ULONG       SizeOfStruct;
    ULONG       TypeIndex;
    ULONG64     Reserved[2];
    ULONG       Index;
    ULONG       Size;
    ULONG64     ModBase;
    ULONG       Flags;
    ULONG64     Value;
    ULONG64     Address;
    ULONG       Register;
    ULONG       Scope;
    ULONG       Tag;
    ULONG       NameLen;
    ULONG       MaxNameLen;
    CHAR        Name[1];
} SYMBOL_INFO, *PSYMBOL_INFO;

typedef struct _SYMBOL_INFOW
{
    ULONG       SizeOfStruct;
    ULONG       TypeIndex;
    ULONG64     Reserved[2];
    ULONG       Index;
    ULONG       Size;
    ULONG64     ModBase;
    ULONG       Flags;
    ULONG64     Value;
    ULONG64     Address;
    ULONG       Register;
    ULONG       Scope;
    ULONG       Tag;
    ULONG       NameLen;
    ULONG       MaxNameLen;
    WCHAR       Name[1];
} SYMBOL_INFOW, *PSYMBOL_INFOW;

typedef struct _SYMBOL_INFO_PACKAGE
{
    SYMBOL_INFO si;
    CHAR        name[MAX_SYM_NAME+1];
} SYMBOL_INFO_PACKAGE, *PSYMBOL_INFO_PACKAGE;

typedef struct _SYMBOL_INFO_PACKAGEW
{
    SYMBOL_INFOW si;
    WCHAR        name[MAX_SYM_NAME+1];
} SYMBOL_INFO_PACKAGEW, *PSYMBOL_INFO_PACKAGEW;

typedef struct _SYMSRV_INDEX_INFO
{
    DWORD sizeofstruct;
    CHAR  file[MAX_PATH + 1];
    BOOL  stripped;
    DWORD timestamp;
    DWORD size;
    CHAR  dbgfile[MAX_PATH + 1];
    CHAR  pdbfile[MAX_PATH + 1];
    GUID  guid;
    DWORD sig;
    DWORD age;
} SYMSRV_INDEX_INFO, *PSYMSRV_INDEX_INFO;

typedef struct
{
    DWORD sizeofstruct;
    WCHAR file[MAX_PATH + 1];
    BOOL  stripped;
    DWORD timestamp;
    DWORD size;
    WCHAR dbgfile[MAX_PATH + 1];
    WCHAR pdbfile[MAX_PATH + 1];
    GUID  guid;
    DWORD sig;
    DWORD age;
} SYMSRV_INDEX_INFOW, *PSYMSRV_INDEX_INFOW;

typedef enum _IMAGEHLP_SYMBOL_TYPE_INFO 
{
    TI_GET_SYMTAG,
    TI_GET_SYMNAME,
    TI_GET_LENGTH,
    TI_GET_TYPE,
    TI_GET_TYPEID,
    TI_GET_BASETYPE,
    TI_GET_ARRAYINDEXTYPEID,
    TI_FINDCHILDREN,
    TI_GET_DATAKIND,
    TI_GET_ADDRESSOFFSET,
    TI_GET_OFFSET,
    TI_GET_VALUE,
    TI_GET_COUNT,
    TI_GET_CHILDRENCOUNT,
    TI_GET_BITPOSITION,
    TI_GET_VIRTUALBASECLASS,
    TI_GET_VIRTUALTABLESHAPEID,
    TI_GET_VIRTUALBASEPOINTEROFFSET,
    TI_GET_CLASSPARENTID,
    TI_GET_NESTED,
    TI_GET_SYMINDEX,
    TI_GET_LEXICALPARENT,
    TI_GET_ADDRESS,
    TI_GET_THISADJUST,
    TI_GET_UDTKIND,
    TI_IS_EQUIV_TO,
    TI_GET_CALLING_CONVENTION,
    TI_IS_CLOSE_EQUIV_TO,
    TI_GTIEX_REQS_VALID,
    TI_GET_VIRTUALBASEOFFSET,
    TI_GET_VIRTUALBASEDISPINDEX,
    TI_GET_IS_REFERENCE,
    TI_GET_INDIRECTVIRTUALBASECLASS,
    TI_GET_VIRTUALBASETABLETYPE,
    TI_GET_OBJECTPOINTERTYPE,
    IMAGEHLP_SYMBOL_TYPE_INFO_MAX
} IMAGEHLP_SYMBOL_TYPE_INFO;

#define IMAGEHLP_GET_TYPE_INFO_UNCACHED            0x00000001
#define IMAGEHLP_GET_TYPE_INFO_CHILDREN            0x00000002
typedef struct _IMAGEHLP_GET_TYPE_INFO_PARAMS
{
    ULONG       SizeOfStruct;
    ULONG       Flags;
    ULONG       NumIds;
    PULONG      TypeIds;
    ULONG64     TagFilter;
    ULONG       NumReqs;
    IMAGEHLP_SYMBOL_TYPE_INFO* ReqKinds;
    PULONG_PTR  ReqOffsets;
    PULONG      ReqSizes;
    ULONG_PTR   ReqStride;
    ULONG_PTR   BufferSize;
    PVOID       Buffer;
    ULONG       EntriesMatched;
    ULONG       EntriesFilled;
    ULONG64     TagsFound;
    ULONG64     AllReqsValid;
    ULONG       NumReqsValid;
    PULONG64    ReqsValid;
} IMAGEHLP_GET_TYPE_INFO_PARAMS, *PIMAGEHLP_GET_TYPE_INFO_PARAMS;

typedef struct _TI_FINDCHILDREN_PARAMS 
{
    ULONG Count;
    ULONG Start;
    ULONG ChildId[1];
} TI_FINDCHILDREN_PARAMS;

#define UNDNAME_COMPLETE                 (0x0000)
#define UNDNAME_NO_LEADING_UNDERSCORES   (0x0001)
#define UNDNAME_NO_MS_KEYWORDS           (0x0002)
#define UNDNAME_NO_FUNCTION_RETURNS      (0x0004)
#define UNDNAME_NO_ALLOCATION_MODEL      (0x0008)
#define UNDNAME_NO_ALLOCATION_LANGUAGE   (0x0010)
#define UNDNAME_NO_MS_THISTYPE           (0x0020)
#define UNDNAME_NO_CV_THISTYPE           (0x0040)
#define UNDNAME_NO_THISTYPE              (0x0060)
#define UNDNAME_NO_ACCESS_SPECIFIERS     (0x0080)
#define UNDNAME_NO_THROW_SIGNATURES      (0x0100)
#define UNDNAME_NO_MEMBER_TYPE           (0x0200)
#define UNDNAME_NO_RETURN_UDT_MODEL      (0x0400)
#define UNDNAME_32_BIT_DECODE            (0x0800)
#define UNDNAME_NAME_ONLY                (0x1000)
#define UNDNAME_NO_ARGUMENTS             (0x2000)
#define UNDNAME_NO_SPECIAL_SYMS          (0x4000)

#define SYMSEARCH_MASKOBJS              0x01
#define SYMSEARCH_RECURSE               0x02
#define SYMSEARCH_GLOBALSONLY           0x04

BOOL    IMAGEAPI SymGetTypeInfo(HANDLE, DWORD64, ULONG, IMAGEHLP_SYMBOL_TYPE_INFO, PVOID);
BOOL    IMAGEAPI SymGetTypeInfoEx(HANDLE, DWORD64, PIMAGEHLP_GET_TYPE_INFO_PARAMS);
typedef BOOL (CALLBACK *PSYM_ENUMERATESYMBOLS_CALLBACK)(PSYMBOL_INFO, ULONG, PVOID);
typedef BOOL (CALLBACK *PSYM_ENUMERATESYMBOLS_CALLBACKW)(PSYMBOL_INFOW, ULONG, PVOID);
BOOL    IMAGEAPI SymEnumTypes(HANDLE, ULONG64, PSYM_ENUMERATESYMBOLS_CALLBACK, PVOID);
BOOL    IMAGEAPI SymEnumTypesW(HANDLE, ULONG64, PSYM_ENUMERATESYMBOLS_CALLBACKW, PVOID);
BOOL    IMAGEAPI SymEnumTypesByName(HANDLE, ULONG64, PCSTR, PSYM_ENUMERATESYMBOLS_CALLBACK, PVOID);
BOOL    IMAGEAPI SymEnumTypesByNameW(HANDLE, ULONG64, PCWSTR, PSYM_ENUMERATESYMBOLS_CALLBACKW, PVOID);
BOOL    IMAGEAPI SymFromAddr(HANDLE, DWORD64, DWORD64*, SYMBOL_INFO*);
BOOL    IMAGEAPI SymFromAddrW(HANDLE, DWORD64, DWORD64*, SYMBOL_INFOW*);
BOOL    IMAGEAPI SymFromInlineContext(HANDLE, DWORD64, ULONG, PDWORD64, PSYMBOL_INFO);
BOOL    IMAGEAPI SymFromInlineContextW(HANDLE, DWORD64, ULONG, PDWORD64, PSYMBOL_INFOW);
BOOL    IMAGEAPI SymFromToken(HANDLE, DWORD64, DWORD, PSYMBOL_INFO);
BOOL    IMAGEAPI SymFromTokenW(HANDLE, DWORD64, DWORD, PSYMBOL_INFOW);
BOOL    IMAGEAPI SymFromName(HANDLE, PCSTR, PSYMBOL_INFO);
BOOL    IMAGEAPI SymFromNameW(HANDLE, PCWSTR, PSYMBOL_INFOW);
BOOL    IMAGEAPI SymGetSymFromAddr64(HANDLE, DWORD64, PDWORD64, PIMAGEHLP_SYMBOL64);
BOOL    IMAGEAPI SymGetSymFromName64(HANDLE, PCSTR, PIMAGEHLP_SYMBOL64);
BOOL    IMAGEAPI SymGetTypeFromName(HANDLE, ULONG64, PCSTR, PSYMBOL_INFO);
BOOL    IMAGEAPI SymGetTypeFromNameW(HANDLE, ULONG64, PCWSTR, PSYMBOL_INFOW);
BOOL    IMAGEAPI SymGetSymNext64(HANDLE, PIMAGEHLP_SYMBOL64);
BOOL    IMAGEAPI SymGetSymNextW64(HANDLE, PIMAGEHLP_SYMBOLW64);
BOOL    IMAGEAPI SymGetSymPrev64(HANDLE, PIMAGEHLP_SYMBOL64);
BOOL    IMAGEAPI SymGetSymPrevW64(HANDLE, PIMAGEHLP_SYMBOLW64);
BOOL    IMAGEAPI SymEnumSym(HANDLE,ULONG64,PSYM_ENUMERATESYMBOLS_CALLBACK,PVOID);
BOOL    IMAGEAPI SymEnumSymbols(HANDLE, ULONG64, PCSTR, PSYM_ENUMERATESYMBOLS_CALLBACK, PVOID);
BOOL    IMAGEAPI SymEnumSymbolsW(HANDLE, ULONG64, PCWSTR, PSYM_ENUMERATESYMBOLS_CALLBACKW, PVOID);
typedef BOOL (CALLBACK *PSYM_ENUMSYMBOLS_CALLBACK64)(PCSTR, DWORD64, ULONG, PVOID);
typedef BOOL (CALLBACK *PSYM_ENUMSYMBOLS_CALLBACK64W)(PCWSTR, DWORD64, ULONG, PVOID);
BOOL    IMAGEAPI SymEnumerateSymbols64(HANDLE, ULONG64, PSYM_ENUMSYMBOLS_CALLBACK64, PVOID);
BOOL    IMAGEAPI SymEnumerateSymbolsW64(HANDLE, ULONG64, PSYM_ENUMSYMBOLS_CALLBACK64W, PVOID);
BOOL    IMAGEAPI SymEnumSymbolsForAddr(HANDLE, DWORD64, PSYM_ENUMERATESYMBOLS_CALLBACK, PVOID);
BOOL    IMAGEAPI SymEnumSymbolsForAddrW(HANDLE, DWORD64, PSYM_ENUMERATESYMBOLS_CALLBACKW, PVOID);
typedef BOOL (CALLBACK *PSYMBOL_REGISTERED_CALLBACK64)(HANDLE, ULONG, ULONG64, ULONG64);
BOOL    IMAGEAPI SymRegisterCallback64(HANDLE, PSYMBOL_REGISTERED_CALLBACK64, ULONG64);
BOOL    IMAGEAPI SymRegisterCallbackW64(HANDLE, PSYMBOL_REGISTERED_CALLBACK64, ULONG64);
BOOL    IMAGEAPI SymUnDName64(PIMAGEHLP_SYMBOL64, PSTR, DWORD);
BOOL    IMAGEAPI SymMatchString(PCSTR, PCSTR, BOOL);
BOOL    IMAGEAPI SymMatchStringA(PCSTR, PCSTR, BOOL);
BOOL    IMAGEAPI SymMatchStringW(PCWSTR, PCWSTR, BOOL);
BOOL    IMAGEAPI SymSearch(HANDLE, ULONG64, DWORD, DWORD, PCSTR, DWORD64, PSYM_ENUMERATESYMBOLS_CALLBACK, PVOID, DWORD);
BOOL    IMAGEAPI SymSearchW(HANDLE, ULONG64, DWORD, DWORD, PCWSTR, DWORD64, PSYM_ENUMERATESYMBOLS_CALLBACKW, PVOID, DWORD);
DWORD   IMAGEAPI UnDecorateSymbolName(PCSTR, PSTR, DWORD, DWORD);
DWORD   IMAGEAPI UnDecorateSymbolNameW(PCWSTR, PWSTR, DWORD, DWORD);
BOOL    IMAGEAPI SymGetScope(HANDLE, ULONG64, DWORD, PSYMBOL_INFO);
BOOL    IMAGEAPI SymGetScopeW(HANDLE, ULONG64, DWORD, PSYMBOL_INFOW);
BOOL    IMAGEAPI SymFromIndex(HANDLE, ULONG64, DWORD, PSYMBOL_INFO);
BOOL    IMAGEAPI SymFromIndexW(HANDLE, ULONG64, DWORD, PSYMBOL_INFOW);
BOOL    IMAGEAPI SymAddSymbol(HANDLE, ULONG64, PCSTR, DWORD64, DWORD, DWORD);
BOOL    IMAGEAPI SymAddSymbolW(HANDLE, ULONG64, PCWSTR, DWORD64, DWORD, DWORD);
BOOL    IMAGEAPI SymDeleteSymbol(HANDLE, ULONG64, PCSTR, DWORD64, DWORD);
BOOL    IMAGEAPI SymDeleteSymbolW(HANDLE, ULONG64, PCWSTR, DWORD64, DWORD);

typedef struct _OMAP
{
    ULONG  rva;
    ULONG  rvaTo;
} OMAP, *POMAP;

BOOL IMAGEAPI SymGetOmaps(HANDLE, DWORD64, POMAP*, PDWORD64, POMAP*, PDWORD64);

/*************************
 *      Source Files     *
 *************************/
typedef BOOL (CALLBACK *PSYM_ENUMSOURCEFILES_CALLBACK)(PSOURCEFILE, PVOID);
typedef BOOL (CALLBACK *PSYM_ENUMSOURCEFILES_CALLBACKW)(PSOURCEFILEW, PVOID);

BOOL    IMAGEAPI SymEnumSourceFiles(HANDLE, ULONG64, PCSTR, PSYM_ENUMSOURCEFILES_CALLBACK, PVOID);
BOOL    IMAGEAPI SymEnumSourceFilesW(HANDLE, ULONG64, PCWSTR, PSYM_ENUMSOURCEFILES_CALLBACKW, PVOID);
BOOL    IMAGEAPI SymGetLineFromAddr64(HANDLE, DWORD64, PDWORD, PIMAGEHLP_LINE64);
BOOL    IMAGEAPI SymGetLineFromAddrW64(HANDLE, DWORD64, PDWORD, PIMAGEHLP_LINEW64);
BOOL    IMAGEAPI SymGetLineFromInlineContext(HANDLE, DWORD64, ULONG, DWORD64, PDWORD, PIMAGEHLP_LINE64);
BOOL    IMAGEAPI SymGetLineFromInlineContextW(HANDLE, DWORD64, ULONG, DWORD64, PDWORD, PIMAGEHLP_LINEW64);
BOOL    IMAGEAPI SymGetLinePrev64(HANDLE, PIMAGEHLP_LINE64);
BOOL    IMAGEAPI SymGetLinePrevW64(HANDLE, PIMAGEHLP_LINEW64);
BOOL    IMAGEAPI SymGetLineNext64(HANDLE, PIMAGEHLP_LINE64);
BOOL    IMAGEAPI SymGetLineNextW64(HANDLE, PIMAGEHLP_LINEW64);
BOOL    IMAGEAPI SymGetLineFromName64(HANDLE, PCSTR, PCSTR, DWORD, PLONG, PIMAGEHLP_LINE64);
BOOL    IMAGEAPI SymGetLineFromNameW64(HANDLE, PCWSTR, PCWSTR, DWORD, PLONG, PIMAGEHLP_LINEW64);
ULONG   IMAGEAPI SymGetFileLineOffsets64(HANDLE, PCSTR, PCSTR, PDWORD64, ULONG);
BOOL    IMAGEAPI SymGetSourceFile(HANDLE, ULONG64, PCSTR, PCSTR, PSTR, DWORD);
BOOL    IMAGEAPI SymGetSourceFileW(HANDLE, ULONG64, PCWSTR, PCWSTR, PWSTR, DWORD);
BOOL    IMAGEAPI SymGetSourceFileToken(HANDLE, ULONG64, PCSTR, PVOID*, DWORD*);
BOOL    IMAGEAPI SymGetSourceFileTokenW(HANDLE, ULONG64, PCWSTR, PVOID*, DWORD*);
BOOL    IMAGEAPI SymGetSourceFileFromToken(HANDLE, PVOID, PCSTR, PSTR, DWORD);
BOOL    IMAGEAPI SymGetSourceFileFromTokenW(HANDLE, PVOID, PCWSTR, PWSTR, DWORD);
BOOL    IMAGEAPI SymGetSourceVarFromToken(HANDLE, PVOID, PCSTR, PCSTR, PSTR, DWORD);
BOOL    IMAGEAPI SymGetSourceVarFromTokenW(HANDLE, PVOID, PCWSTR, PCWSTR, PWSTR, DWORD);

typedef struct _SRCCODEINFO
{
    DWORD       SizeOfStruct;
    PVOID       Key;
    DWORD64     ModBase;
    CHAR        Obj[MAX_PATH+1];
    CHAR        FileName[MAX_PATH+1];
    DWORD       LineNumber;
    DWORD64     Address;
} SRCCODEINFO, *PSRCCODEINFO;

typedef struct _SRCCODEINFOW
{
    DWORD       SizeOfStruct;
    PVOID       Key;
    DWORD64     ModBase;
    WCHAR       Obj[MAX_PATH+1];
    WCHAR       FileName[MAX_PATH+1];
    DWORD       LineNumber;
    DWORD64     Address;
} SRCCODEINFOW, *PSRCCODEINFOW;

typedef BOOL (CALLBACK* PSYM_ENUMLINES_CALLBACK)(PSRCCODEINFO, PVOID);
typedef BOOL (CALLBACK* PSYM_ENUMLINES_CALLBACKW)(PSRCCODEINFOW, PVOID);
BOOL IMAGEAPI SymEnumLines(HANDLE, ULONG64, PCSTR, PCSTR, PSYM_ENUMLINES_CALLBACK, PVOID);
BOOL IMAGEAPI SymEnumLinesW(HANDLE, ULONG64, PCWSTR, PCWSTR, PSYM_ENUMLINES_CALLBACKW, PVOID);
BOOL IMAGEAPI SymEnumSourceLines(HANDLE, ULONG64, PCSTR, PCSTR, DWORD, DWORD, PSYM_ENUMLINES_CALLBACK, PVOID);
BOOL IMAGEAPI SymEnumSourceLinesW(HANDLE, ULONG64, PCWSTR, PCWSTR, DWORD, DWORD, PSYM_ENUMLINES_CALLBACKW, PVOID);

/*************************
 * File & image handling *
 *************************/
BOOL IMAGEAPI SymInitialize(HANDLE, PCSTR, BOOL);
BOOL IMAGEAPI SymInitializeW(HANDLE, PCWSTR, BOOL);
BOOL IMAGEAPI SymCleanup(HANDLE);

HANDLE  IMAGEAPI FindDebugInfoFile(PCSTR, PCSTR, PSTR);
typedef BOOL (CALLBACK *PFIND_DEBUG_FILE_CALLBACK)(HANDLE, PCSTR, PVOID);
typedef BOOL (CALLBACK *PFIND_DEBUG_FILE_CALLBACKW)(HANDLE, PCWSTR, PVOID);
HANDLE  IMAGEAPI FindDebugInfoFileEx(PCSTR, PCSTR, PSTR, PFIND_DEBUG_FILE_CALLBACK, PVOID);
HANDLE  IMAGEAPI FindDebugInfoFileExW(PCWSTR, PCWSTR, PWSTR, PFIND_DEBUG_FILE_CALLBACKW, PVOID);
HANDLE  IMAGEAPI SymFindDebugInfoFile(HANDLE, PCSTR, PSTR, PFIND_DEBUG_FILE_CALLBACK, PVOID);
HANDLE  IMAGEAPI SymFindDebugInfoFileW(HANDLE, PCWSTR, PWSTR, PFIND_DEBUG_FILE_CALLBACKW, PVOID);
typedef BOOL (CALLBACK *PFINDFILEINPATHCALLBACK)(PCSTR, PVOID);
typedef BOOL (CALLBACK *PFINDFILEINPATHCALLBACKW)(PCWSTR, PVOID);
BOOL    IMAGEAPI FindFileInPath(HANDLE, PCSTR, PCSTR, PVOID, DWORD, DWORD, DWORD, PSTR, PFINDFILEINPATHCALLBACK, PVOID);
BOOL    IMAGEAPI SymFindFileInPath(HANDLE, PCSTR, PCSTR, PVOID, DWORD, DWORD, DWORD, PSTR, PFINDFILEINPATHCALLBACK, PVOID);
BOOL    IMAGEAPI SymFindFileInPathW(HANDLE, PCWSTR, PCWSTR, PVOID, DWORD, DWORD, DWORD, PWSTR, PFINDFILEINPATHCALLBACKW, PVOID);
HANDLE  IMAGEAPI FindExecutableImage(PCSTR, PCSTR, PSTR);
typedef BOOL (CALLBACK *PFIND_EXE_FILE_CALLBACK)(HANDLE, PCSTR, PVOID);
typedef BOOL (CALLBACK *PFIND_EXE_FILE_CALLBACKW)(HANDLE, PCWSTR, PVOID);
HANDLE  IMAGEAPI FindExecutableImageEx(PCSTR, PCSTR, PSTR, PFIND_EXE_FILE_CALLBACK, PVOID);
HANDLE  IMAGEAPI FindExecutableImageExW(PCWSTR, PCWSTR, PWSTR, PFIND_EXE_FILE_CALLBACKW, PVOID);
HANDLE  IMAGEAPI SymFindExecutableImage(HANDLE, PCSTR, PSTR, PFIND_EXE_FILE_CALLBACK, PVOID);
HANDLE  IMAGEAPI SymFindExecutableImageW(HANDLE, PCWSTR, PWSTR, PFIND_EXE_FILE_CALLBACKW, PVOID);
PIMAGE_NT_HEADERS IMAGEAPI ImageNtHeader(PVOID);
PVOID   IMAGEAPI ImageDirectoryEntryToDataEx(PVOID, BOOLEAN, USHORT, PULONG, PIMAGE_SECTION_HEADER *);
PVOID   IMAGEAPI ImageDirectoryEntryToData(PVOID, BOOLEAN, USHORT, PULONG);
PIMAGE_SECTION_HEADER IMAGEAPI ImageRvaToSection(PIMAGE_NT_HEADERS, PVOID, ULONG);
PVOID   IMAGEAPI ImageRvaToVa(PIMAGE_NT_HEADERS, PVOID, ULONG, PIMAGE_SECTION_HEADER*);
BOOL    IMAGEAPI SymGetSearchPath(HANDLE, PSTR, DWORD);
BOOL    IMAGEAPI SymGetSearchPathW(HANDLE, PWSTR, DWORD);
BOOL    IMAGEAPI SymSetSearchPath(HANDLE, PCSTR);
BOOL    IMAGEAPI SymSetSearchPathW(HANDLE, PCWSTR);
DWORD   IMAGEAPI GetTimestampForLoadedLibrary(HMODULE);
BOOL    IMAGEAPI MakeSureDirectoryPathExists(PCSTR);
BOOL    IMAGEAPI SearchTreeForFile(PCSTR, PCSTR, PSTR);
BOOL    IMAGEAPI SearchTreeForFileW(PCWSTR, PCWSTR, PWSTR);
typedef BOOL (CALLBACK *PENUMDIRTREE_CALLBACK)(PCSTR, PVOID);
typedef BOOL (CALLBACK *PENUMDIRTREE_CALLBACKW)(PCWSTR, PVOID);
BOOL    IMAGEAPI EnumDirTree(HANDLE, PCSTR, PCSTR, PSTR, PENUMDIRTREE_CALLBACK, PVOID);
BOOL    IMAGEAPI EnumDirTreeW(HANDLE, PCWSTR, PCWSTR, PWSTR, PENUMDIRTREE_CALLBACKW, PVOID);
BOOL    IMAGEAPI SymMatchFileName(PCSTR, PCSTR, PSTR*, PSTR*);
BOOL    IMAGEAPI SymMatchFileNameW(PCWSTR, PCWSTR, PWSTR*, PWSTR*);
PCHAR   IMAGEAPI SymSetHomeDirectory(HANDLE, PCSTR);
PWSTR   IMAGEAPI SymSetHomeDirectoryW(HANDLE, PCWSTR);
PCHAR   IMAGEAPI SymGetHomeDirectory(DWORD, PSTR, size_t);
PWSTR   IMAGEAPI SymGetHomeDirectoryW(DWORD, PWSTR, size_t);
#define hdBase  0
#define hdSym   1
#define hdSrc   2
#define hdMax   3

/*************************
 *   Context management  *
 *************************/
BOOL IMAGEAPI SymSetContext(HANDLE, PIMAGEHLP_STACK_FRAME, PIMAGEHLP_CONTEXT);
BOOL IMAGEAPI SymSetScopeFromIndex(HANDLE, ULONG64, ULONG);
BOOL IMAGEAPI SymSetScopeFromAddr(HANDLE, ULONG64);
BOOL IMAGEAPI SymSetScopeFromInlineContext(HANDLE, ULONG64, ULONG);

/*************************
 *    Stack management   *
 *************************/

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define KDHELP KDHELP64
#define PKDHELP PKDHELP64
#else
typedef struct _KDHELP
{
    DWORD       Thread;
    DWORD       ThCallbackStack;
    DWORD       NextCallback;
    DWORD       FramePointer;
    DWORD       KiCallUserMode;
    DWORD       KeUserCallbackDispatcher;
    DWORD       SystemRangeStart;
} KDHELP, *PKDHELP;
#endif

typedef struct _KDHELP64
{
    DWORD64     Thread;
    DWORD       ThCallbackStack;
    DWORD       ThCallbackBStore;
    DWORD       NextCallback;
    DWORD       FramePointer;
    DWORD64     KiCallUserMode;
    DWORD64     KeUserCallbackDispatcher;
    DWORD64     SystemRangeStart;
    DWORD64     Reserved[8];
} KDHELP64, *PKDHELP64;

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define STACKFRAME STACKFRAME64
#define LPSTACKFRAME LPSTACKFRAME64
#else
typedef struct _STACKFRAME
{
    ADDRESS     AddrPC;
    ADDRESS     AddrReturn;
    ADDRESS     AddrFrame;
    ADDRESS     AddrStack;
    PVOID       FuncTableEntry;
    DWORD       Params[4];
    BOOL        Far;
    BOOL        Virtual;
    DWORD       Reserved[3];
    KDHELP      KdHelp;
    ADDRESS     AddrBStore;
} STACKFRAME, *LPSTACKFRAME;
#endif

typedef struct _STACKFRAME64
{
    ADDRESS64   AddrPC;
    ADDRESS64   AddrReturn;
    ADDRESS64   AddrFrame;
    ADDRESS64   AddrStack;
    ADDRESS64   AddrBStore;
    PVOID       FuncTableEntry;
    DWORD64     Params[4];
    BOOL        Far;
    BOOL        Virtual;
    DWORD64     Reserved[3];
    KDHELP64    KdHelp;
} STACKFRAME64, *LPSTACKFRAME64;

#define INLINE_FRAME_CONTEXT_INIT   0
#define INLINE_FRAME_CONTEXT_IGNORE 0xFFFFFFFF

typedef struct _tagSTACKFRAME_EX
{
    ADDRESS64   AddrPC;
    ADDRESS64   AddrReturn;
    ADDRESS64   AddrFrame;
    ADDRESS64   AddrStack;
    ADDRESS64   AddrBStore;
    PVOID       FuncTableEntry;
    DWORD64     Params[4];
    BOOL        Far;
    BOOL        Virtual;
    DWORD64     Reserved[3];
    KDHELP64    KdHelp;

    DWORD       StackFrameSize;
    DWORD       InlineFrameContext;
} STACKFRAME_EX, *LPSTACKFRAME_EX;


typedef BOOL (CALLBACK *PREAD_PROCESS_MEMORY_ROUTINE64)
    (HANDLE, DWORD64, PVOID, DWORD, PDWORD);
typedef PVOID (CALLBACK *PFUNCTION_TABLE_ACCESS_ROUTINE64)(HANDLE, DWORD64);
typedef DWORD64 (CALLBACK *PGET_MODULE_BASE_ROUTINE64)(HANDLE, DWORD64);
typedef DWORD64 (CALLBACK *PTRANSLATE_ADDRESS_ROUTINE64)(HANDLE, HANDLE, LPADDRESS64);
BOOL IMAGEAPI StackWalk64(DWORD, HANDLE, HANDLE, LPSTACKFRAME64, PVOID,
                        PREAD_PROCESS_MEMORY_ROUTINE64,
                        PFUNCTION_TABLE_ACCESS_ROUTINE64,
                        PGET_MODULE_BASE_ROUTINE64,
                        PTRANSLATE_ADDRESS_ROUTINE64);
#define SYM_STKWALK_DEFAULT        0x00000000
#define SYM_STKWALK_FORCE_FRAMEPTR 0x00000001
BOOL IMAGEAPI StackWalkEx(DWORD, HANDLE, HANDLE, LPSTACKFRAME_EX, PVOID,
                        PREAD_PROCESS_MEMORY_ROUTINE64,
                        PFUNCTION_TABLE_ACCESS_ROUTINE64,
                        PGET_MODULE_BASE_ROUTINE64,
                        PTRANSLATE_ADDRESS_ROUTINE64,
                        DWORD);
PVOID IMAGEAPI SymFunctionTableAccess64(HANDLE, DWORD64);

typedef PVOID (CALLBACK *PSYMBOL_FUNCENTRY_CALLBACK64)(HANDLE, ULONG64, ULONG64);

BOOL IMAGEAPI SymRegisterFunctionEntryCallback64(HANDLE, PSYMBOL_FUNCENTRY_CALLBACK64, ULONG64);
BOOL IMAGEAPI SymGetUnwindInfo(HANDLE, DWORD64, PVOID, PULONG);

/* Inline context related APIs */
DWORD IMAGEAPI SymAddrIncludeInlineTrace(HANDLE, DWORD64);

#define SYM_INLINE_COMP_ERROR     0
#define SYM_INLINE_COMP_IDENTICAL 1
#define SYM_INLINE_COMP_STEPIN    2
#define SYM_INLINE_COMP_STEPOUT   3
#define SYM_INLINE_COMP_STEPOVER  4
#define SYM_INLINE_COMP_DIFFERENT 5

DWORD IMAGEAPI SymCompareInlineTrace(HANDLE, DWORD64, DWORD, DWORD64, DWORD64, DWORD64);
BOOL  IMAGEAPI SymQueryInlineTrace(HANDLE, DWORD64, DWORD, DWORD64, DWORD64, LPDWORD, LPDWORD);

/*************************
 * Version, global stuff *
 *************************/

#define API_VERSION_NUMBER 9

typedef struct API_VERSION
{
    USHORT  MajorVersion;
    USHORT  MinorVersion;
    USHORT  Revision;
    USHORT  Reserved;
} API_VERSION, *LPAPI_VERSION;

LPAPI_VERSION IMAGEAPI ImagehlpApiVersion(void);
LPAPI_VERSION IMAGEAPI ImagehlpApiVersionEx(LPAPI_VERSION);

#ifndef _WIN64
typedef struct _IMAGE_DEBUG_INFORMATION
{
    LIST_ENTRY                  List;
    DWORD                       ReservedSize;
    PVOID                       ReservedMappedBase;
    USHORT                      ReservedMachine;
    USHORT                      ReservedCharacteristics;
    DWORD                       ReservedCheckSum;
    DWORD                       ImageBase;
    DWORD                       SizeOfImage;
    DWORD                       ReservedNumberOfSections;
    PIMAGE_SECTION_HEADER       ReservedSections;
    DWORD                       ReservedExportedNamesSize;
    PSTR                        ReservedExportedNames;
    DWORD                       ReservedNumberOfFunctionTableEntries;
    PIMAGE_FUNCTION_ENTRY       ReservedFunctionTableEntries;
    DWORD                       ReservedLowestFunctionStartingAddress;
    DWORD                       ReservedHighestFunctionEndingAddress;
    DWORD                       ReservedNumberOfFpoTableEntries;
    PFPO_DATA                   ReservedFpoTableEntries;
    DWORD                       SizeOfCoffSymbols;
    PIMAGE_COFF_SYMBOLS_HEADER  CoffSymbols;
    DWORD                       ReservedSizeOfCodeViewSymbols;
    PVOID                       ReservedCodeViewSymbols;
    PSTR                        ImageFilePath;
    PSTR                        ImageFileName;
    PSTR                        ReservedDebugFilePath;
    DWORD                       ReservedTimeDateStamp;
    BOOL                        ReservedRomImage;
    PIMAGE_DEBUG_DIRECTORY      ReservedDebugDirectory;
    DWORD                       ReservedNumberOfDebugDirectories;
    DWORD                       ReservedOriginalFunctionTableBaseAddress;
    DWORD                       Reserved[ 2 ];
} IMAGE_DEBUG_INFORMATION, *PIMAGE_DEBUG_INFORMATION;

PIMAGE_DEBUG_INFORMATION IMAGEAPI MapDebugInformation(HANDLE, PCSTR, PCSTR, ULONG);
BOOL IMAGEAPI UnmapDebugInformation(PIMAGE_DEBUG_INFORMATION);
#endif

typedef enum
{
    SYMOPT_EX_DISABLEACCESSTIMEUPDATE,
    SYMOPT_EX_MAX,

#ifdef __WINESRC__
    /* Include ELF/Mach-O modules in module operations */
    SYMOPT_EX_WINE_NATIVE_MODULES = 1000,
    /* Enable Wine's extension APIs */
    SYMOPT_EX_WINE_EXTENSION_API,
    /* Return the Unix actual path of loaded module */
    SYMOPT_EX_WINE_MODULE_REAL_PATH,
    /* Return the raw source file path from debug info (not always mapped to DOS) */
    SYMOPT_EX_WINE_SOURCE_ACTUAL_PATH,
#endif
} IMAGEHLP_EXTENDED_OPTIONS;

DWORD   IMAGEAPI SymGetOptions(void);
DWORD   IMAGEAPI SymSetOptions(DWORD);
BOOL    IMAGEAPI SymGetExtendedOption(IMAGEHLP_EXTENDED_OPTIONS option);
BOOL    IMAGEAPI SymSetExtendedOption(IMAGEHLP_EXTENDED_OPTIONS option, BOOL value);
BOOL    IMAGEAPI SymSetParentWindow(HWND);

/***************************
 * Symbol servers & stores *
 ***************************/
BOOL IMAGEAPI SymSrvGetFileIndexes(PCSTR, GUID *, PDWORD, PDWORD, DWORD);
BOOL IMAGEAPI SymSrvGetFileIndexesW(PCWSTR, GUID *, PDWORD, PDWORD, DWORD);
BOOL IMAGEAPI SymSrvGetFileIndexInfo(PCSTR, PSYMSRV_INDEX_INFO, DWORD);
BOOL IMAGEAPI SymSrvGetFileIndexInfoW(PCWSTR, PSYMSRV_INDEX_INFOW, DWORD);

typedef BOOL     (WINAPI* PSYMBOLSERVERPROC)(PCSTR, PCSTR, PVOID, DWORD, DWORD, PSTR);
typedef BOOL     (WINAPI* PSYMBOLSERVERPROCA)(PCSTR, PCSTR, PVOID, DWORD, DWORD, PSTR);
typedef BOOL     (WINAPI* PSYMBOLSERVERPROCW)(PCWSTR, PCWSTR, PVOID, DWORD, DWORD, PWSTR);
typedef BOOL     (WINAPI* PSYMBOLSERVEROPENPROC)(void);
typedef BOOL     (WINAPI* PSYMBOLSERVERCLOSEPROC)(void);
typedef BOOL     (WINAPI* PSYMBOLSERVERSETOPTIONSPROC)(UINT_PTR, ULONG64);
typedef BOOL     (CALLBACK* PSYMBOLSERVERCALLBACKPROC)(UINT_PTR, ULONG64, ULONG64);
typedef UINT_PTR (WINAPI* PSYMBOLSERVERGETOPTIONSPROC)(void);
typedef BOOL     (WINAPI* PSYMBOLSERVERPINGPROC)(PCSTR);
typedef BOOL     (WINAPI* PSYMBOLSERVERPINGPROCA)(PCSTR);
typedef BOOL     (WINAPI* PSYMBOLSERVERPINGPROCW)(PCWSTR);

#define SSRVOPT_CALLBACK            0x0001
#define SSRVOPT_DWORD               0x0002
#define SSRVOPT_DWORDPTR            0x0004
#define SSRVOPT_GUIDPTR             0x0008
#define SSRVOPT_OLDGUIDPTR          0x0010
#define SSRVOPT_UNATTENDED          0x0020
#define SSRVOPT_NOCOPY              0x0040
#define SSRVOPT_PARENTWIN           0x0080
#define SSRVOPT_PARAMTYPE           0x0100
#define SSRVOPT_SECURE              0x0200
#define SSRVOPT_TRACE               0x0400
#define SSRVOPT_SETCONTEXT          0x0800
#define SSRVOPT_PROXY               0x1000
#define SSRVOPT_DOWNSTREAM_STORE    0x2000
#define SSRVOPT_RESET               ((ULONG_PTR)-1)

#define SSRVACTION_TRACE        1
#define SSRVACTION_QUERYCANCEL  2
#define SSRVACTION_EVENT        3
#define SSRVACTION_EVENTW       4
#define SSRVACTION_SIZE         5
#define SSRVACTION_HTTPSTATUS   6

/* 32-bit functions */

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)

#define PENUMLOADED_MODULES_CALLBACK PENUMLOADED_MODULES_CALLBACK64
#define PFUNCTION_TABLE_ACCESS_ROUTINE PFUNCTION_TABLE_ACCESS_ROUTINE64
#define PGET_MODULE_BASE_ROUTINE PGET_MODULE_BASE_ROUTINE64
#define PREAD_PROCESS_MEMORY_ROUTINE PREAD_PROCESS_MEMORY_ROUTINE64
#define PSYMBOL_FUNCENTRY_CALLBACK PSYMBOL_FUNCENTRY_CALLBACK64
#define PSYMBOL_REGISTERED_CALLBACK PSYMBOL_REGISTERED_CALLBACK64
#define PSYM_ENUMMODULES_CALLBACK PSYM_ENUMMODULES_CALLBACK64
#define PSYM_ENUMSYMBOLS_CALLBACK PSYM_ENUMSYMBOLS_CALLBACK64
#define PSYM_ENUMSYMBOLS_CALLBACKW PSYM_ENUMSYMBOLS_CALLBACKW64
#define PTRANSLATE_ADDRESS_ROUTINE PTRANSLATE_ADDRESS_ROUTINE64

#define EnumerateLoadedModules EnumerateLoadedModules64
#define StackWalk StackWalk64
#define SymEnumerateModules SymEnumerateModules64
#define SymEnumerateSymbols SymEnumerateSymbols64
#define SymEnumerateSymbolsW SymEnumerateSymbolsW64
#define SymFunctionTableAccess SymFunctionTableAccess64
#define SymGetLineFromAddr SymGetLineFromAddr64
#define SymGetLineFromAddrW SymGetLineFromAddrW64
#define SymGetLineFromName SymGetLineFromName64
#define SymGetLineNext SymGetLineNext64
#define SymGetLineNextW SymGetLineNextW64
#define SymGetLinePrev SymGetLinePrev64
#define SymGetLinePrevW SymGetLinePrevW64
#define SymGetModuleBase SymGetModuleBase64
#define SymGetModuleInfo SymGetModuleInfo64
#define SymGetModuleInfoW SymGetModuleInfoW64
#define SymGetSymFromAddr SymGetSymFromAddr64
#define SymGetSymFromName SymGetSymFromName64
#define SymGetSymNext SymGetSymNext64
#define SymGetSymNextW SymGetSymNextW64
#define SymGetSymPrev SymGetSymPrev64
#define SymGetSymPrevW SymGetSymPrevW64
#define SymLoadModule SymLoadModule64
#define SymRegisterCallback SymRegisterCallback64
#define SymRegisterFunctionEntryCallback SymRegisterFunctionEntryCallback64
#define SymUnDName SymUnDName64
#define SymUnloadModule SymUnloadModule64

#else

typedef BOOL  (CALLBACK *PENUMLOADED_MODULES_CALLBACK)(PCSTR, ULONG, ULONG, PVOID);
typedef PVOID (CALLBACK *PFUNCTION_TABLE_ACCESS_ROUTINE)(HANDLE, DWORD);
typedef DWORD (CALLBACK *PGET_MODULE_BASE_ROUTINE)(HANDLE, DWORD);
typedef BOOL  (CALLBACK *PREAD_PROCESS_MEMORY_ROUTINE)(HANDLE, DWORD, PVOID, DWORD, PDWORD);
typedef BOOL  (CALLBACK *PSYM_ENUMMODULES_CALLBACK)(PCSTR, ULONG, PVOID);
typedef BOOL  (CALLBACK *PSYM_ENUMSYMBOLS_CALLBACK)(PCSTR, ULONG, ULONG, PVOID);
typedef BOOL  (CALLBACK *PSYM_ENUMSYMBOLS_CALLBACKW)(PCWSTR, ULONG, ULONG, PVOID);
typedef BOOL  (CALLBACK *PSYMBOL_REGISTERED_CALLBACK)(HANDLE, ULONG, PVOID, PVOID);
typedef PVOID (CALLBACK *PSYMBOL_FUNCENTRY_CALLBACK)(HANDLE, DWORD, PVOID);
typedef DWORD (CALLBACK *PTRANSLATE_ADDRESS_ROUTINE)(HANDLE, HANDLE, LPADDRESS);

BOOL    IMAGEAPI EnumerateLoadedModules(HANDLE, PENUMLOADED_MODULES_CALLBACK, PVOID);
BOOL    IMAGEAPI StackWalk(DWORD, HANDLE, HANDLE, LPSTACKFRAME, PVOID, PREAD_PROCESS_MEMORY_ROUTINE, PFUNCTION_TABLE_ACCESS_ROUTINE, PGET_MODULE_BASE_ROUTINE, PTRANSLATE_ADDRESS_ROUTINE);
BOOL    IMAGEAPI SymEnumerateModules(HANDLE, PSYM_ENUMMODULES_CALLBACK, PVOID);
BOOL    IMAGEAPI SymEnumerateSymbols(HANDLE, ULONG, PSYM_ENUMSYMBOLS_CALLBACK, PVOID);
BOOL    IMAGEAPI SymEnumerateSymbolsW(HANDLE, ULONG, PSYM_ENUMSYMBOLS_CALLBACKW, PVOID);
PVOID   IMAGEAPI SymFunctionTableAccess(HANDLE, DWORD);
BOOL    IMAGEAPI SymGetLineFromAddr(HANDLE, DWORD, PDWORD, PIMAGEHLP_LINE);
BOOL    IMAGEAPI SymGetLineFromAddrW(HANDLE, DWORD, PDWORD, PIMAGEHLP_LINEW);
BOOL    IMAGEAPI SymGetLineFromName(HANDLE, PCSTR, PCSTR, DWORD, PLONG, PIMAGEHLP_LINE);
BOOL    IMAGEAPI SymGetLineNext(HANDLE, PIMAGEHLP_LINE);
BOOL    IMAGEAPI SymGetLineNextW(HANDLE, PIMAGEHLP_LINEW);
BOOL    IMAGEAPI SymGetLinePrev(HANDLE, PIMAGEHLP_LINE);
BOOL    IMAGEAPI SymGetLinePrevW(HANDLE, PIMAGEHLP_LINEW);
DWORD   IMAGEAPI SymGetModuleBase(HANDLE, DWORD);
BOOL    IMAGEAPI SymGetModuleInfo(HANDLE, DWORD, PIMAGEHLP_MODULE);
BOOL    IMAGEAPI SymGetModuleInfoW(HANDLE, DWORD, PIMAGEHLP_MODULEW);
BOOL    IMAGEAPI SymGetSymFromAddr(HANDLE, DWORD, PDWORD, PIMAGEHLP_SYMBOL);
BOOL    IMAGEAPI SymGetSymFromName(HANDLE, PCSTR, PIMAGEHLP_SYMBOL);
BOOL    IMAGEAPI SymGetSymNext(HANDLE, PIMAGEHLP_SYMBOL);
BOOL    IMAGEAPI SymGetSymNextW(HANDLE, PIMAGEHLP_SYMBOLW);
BOOL    IMAGEAPI SymGetSymPrev(HANDLE, PIMAGEHLP_SYMBOL);
BOOL    IMAGEAPI SymGetSymPrevW(HANDLE, PIMAGEHLP_SYMBOLW);
DWORD   IMAGEAPI SymLoadModule(HANDLE, HANDLE, PCSTR, PCSTR, DWORD, DWORD);
BOOL    IMAGEAPI SymRegisterCallback(HANDLE, PSYMBOL_REGISTERED_CALLBACK, PVOID);
BOOL    IMAGEAPI SymRegisterFunctionEntryCallback(HANDLE, PSYMBOL_FUNCENTRY_CALLBACK, PVOID);
BOOL    IMAGEAPI SymUnDName(PIMAGEHLP_SYMBOL, PSTR, DWORD);
BOOL    IMAGEAPI SymUnloadModule(HANDLE, DWORD);

#endif

#ifdef __WINESRC__

/* Wine extensions to dbghelp */
enum dhext_module_type
{
    DMT_UNKNOWN,        /* for lookup, not actually used for a module */
    DMT_ELF,            /* a real ELF shared module */
    DMT_MACHO,          /* a real Mach-O shared module */
    DMT_PE,             /* a native or builtin PE module */
};

/* only reporting the formats not exposed in regular IMAGHELP_MODULE_INFO */
enum dhext_debug_format
{
    DHEXT_FORMAT_DWARF2     = 0x0001,
    DHEXT_FORMAT_DWARF3     = 0x0002,
    DHEXT_FORMAT_DWARF4     = 0x0004,
    DHEXT_FORMAT_DWARF5     = 0x0008,
    DHEXT_FORMAT_STABS      = 0x0010,
};

struct dhext_module_information
{
    enum dhext_module_type      type;
    unsigned                    is_wine_builtin : 1,
                                is_virtual : 1,
                                has_file_image : 1;
    unsigned                    debug_format_bitmask;
};

extern BOOL WINAPI wine_get_module_information(HANDLE, DWORD64 base, struct dhext_module_information*, unsigned len);

#endif /*  __WINESRC__ */

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif  /* __WINE_DBGHELP_H */
