/*
 * Copyright  Eric Youngdale (1994)
 */
#ifndef __WINE_PEEXE_H
#define __WINE_PEEXE_H

#include "wintypes.h"
#include "neexe.h"

typedef struct _IMAGE_FILE_HEADER {
	WORD	Machine;			/* 00 */
	WORD	NumberOfSections;		/* 02 */
	DWORD	TimeDateStamp;			/* 04 */
	DWORD	PointerToSymbolTable;		/* 08 */
	DWORD	NumberOfSymbols;		/* 0c */
	WORD	SizeOfOptionalHeader;		/* 10 */
	WORD	Characteristics;		/* 12 */
} IMAGE_FILE_HEADER,*LPIMAGE_FILE_HEADER;

#define	IMAGE_SIZEOF_FILE_HEADER	20

/* These defines describe the meanings of the bits in the Characteristics
   field */

#define IMAGE_FILE_RELOCS_STRIPPED	1 /* No relocation info */
#define IMAGE_FILE_EXECUTABLE_IMAGE	2
#define IMAGE_FILE_LINE_NUMS_STRIPPED   4
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED  8
#define IMAGE_FILE_16BIT_MACHINE	0x40
#define IMAGE_FILE_BYTES_REVERSED_LO	0x80
#define IMAGE_FILE_32BIT_MACHINE	0x100
#define IMAGE_FILE_DEBUG_STRIPPED	0x200
#define IMAGE_FILE_SYSTEM		0x1000
#define IMAGE_FILE_DLL			0x2000
#define IMAGE_FILE_BYTES_REVERSED_HI	0x8000

/* These are the settings of the Machine field. */
#define	IMAGE_FILE_MACHINE_UNKNOWN	0
#define	IMAGE_FILE_MACHINE_I860		0x14d
#define	IMAGE_FILE_MACHINE_I386		0x14c
#define	IMAGE_FILE_MACHINE_R3000	0x162
#define	IMAGE_FILE_MACHINE_R4000	0x166
#define	IMAGE_FILE_MACHINE_R10000	0x168
#define	IMAGE_FILE_MACHINE_ALPHA	0x184
#define	IMAGE_FILE_MACHINE_POWERPC	0x1F0  

typedef struct _IMAGE_DATA_DIRECTORY
{
	DWORD	VirtualAddress;
	DWORD	Size;
} IMAGE_DATA_DIRECTORY,*LPIMAGE_DATA_DIRECTORY;

#define	IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16

/* Optional coff header - used by NT to provide additional information. */
typedef struct _IMAGE_OPTIONAL_HEADER
{
	/*
	 * Standard fields.
	 */

	WORD	Magic;				/* 00 */
	BYTE	MajorLinkerVersion;		/* 02 */
	BYTE	MinorLinkerVersion;		/* 03 */
	DWORD	SizeOfCode;			/* 04 */
	DWORD	SizeOfInitializedData;		/* 08 */
	DWORD	SizeOfUninitializedData;	/* 0C */
	DWORD	AddressOfEntryPoint;		/* 10 */
	DWORD	BaseOfCode;			/* 14 */
	DWORD	BaseOfData;			/* 18 */

	/*
	 * NT additional fields.
	 */

	DWORD	ImageBase;			/* 1C */
	DWORD	SectionAlignment;		/* 20 */
	DWORD	FileAlignment;			/* 24 */
	WORD	MajorOperatingSystemVersion;	/* 28 */
	WORD	MinorOperatingSystemVersion;	/* 2A */
	WORD	MajorImageVersion;		/* 2C */
	WORD	MinorImageVersion;		/* 2E */
	WORD	MajorSubsystemVersion;		/* 30 */
	WORD	MinorSubsystemVersion;		/* 32 */
	DWORD	Reserved1;			/* 34 */
	DWORD	SizeOfImage;			/* 38 */
	DWORD	SizeOfHeaders;			/* 3C */
	DWORD	CheckSum;			/* 40 */
	WORD	Subsystem;			/* 44 */
	WORD	DllCharacteristics;		/* 46 */
	DWORD	SizeOfStackReserve;		/* 48 */
	DWORD	SizeOfStackCommit;		/* 4C */
	DWORD	SizeOfHeapReserve;		/* 50 */
	DWORD	SizeOfHeapCommit;		/* 54 */
	DWORD	LoaderFlags;			/* 58 */
	DWORD	NumberOfRvaAndSizes;		/* 5C */
						/* 60: */
	IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER,*LPIMAGE_OPTIONAL_HEADER;

/* Possible Magic values */
#define IMAGE_NT_OPTIONAL_HDR_MAGIC        0x10b
#define IMAGE_ROM_OPTIONAL_HDR_MAGIC       0x107

/* These are indexes into the DataDirectory array */
#define IMAGE_FILE_EXPORT_DIRECTORY		0
#define IMAGE_FILE_IMPORT_DIRECTORY		1
#define IMAGE_FILE_RESOURCE_DIRECTORY		2
#define IMAGE_FILE_EXCEPTION_DIRECTORY		3
#define IMAGE_FILE_SECURITY_DIRECTORY		4
#define IMAGE_FILE_BASE_RELOCATION_TABLE	5
#define IMAGE_FILE_DEBUG_DIRECTORY		6
#define IMAGE_FILE_DESCRIPTION_STRING		7
#define IMAGE_FILE_MACHINE_VALUE		8  /* Mips */
#define IMAGE_FILE_THREAD_LOCAL_STORAGE		9
#define IMAGE_FILE_CALLBACK_DIRECTORY		10

/* Directory Entries, indices into the DataDirectory array */

#define	IMAGE_DIRECTORY_ENTRY_EXPORT		0
#define	IMAGE_DIRECTORY_ENTRY_IMPORT		1
#define	IMAGE_DIRECTORY_ENTRY_RESOURCE		2
#define	IMAGE_DIRECTORY_ENTRY_EXCEPTION		3
#define	IMAGE_DIRECTORY_ENTRY_SECURITY		4
#define	IMAGE_DIRECTORY_ENTRY_BASERELOC		5
#define	IMAGE_DIRECTORY_ENTRY_DEBUG		6
#define	IMAGE_DIRECTORY_ENTRY_COPYRIGHT		7
#define	IMAGE_DIRECTORY_ENTRY_GLOBALPTR		8   /* (MIPS GP) */
#define	IMAGE_DIRECTORY_ENTRY_TLS		9
#define	IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG	10
#define	IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT	11
#define	IMAGE_DIRECTORY_ENTRY_IAT		12  /* Import Address Table */

/* Subsystem Values */

#define	IMAGE_SUBSYSTEM_UNKNOWN		0
#define	IMAGE_SUBSYSTEM_NATIVE		1
#define	IMAGE_SUBSYSTEM_WINDOWS_GUI	2	/* Windows GUI subsystem */
#define	IMAGE_SUBSYSTEM_WINDOWS_CUI	3	/* Windows character subsystem*/
#define	IMAGE_SUBSYSTEM_OS2_CUI		5
#define	IMAGE_SUBSYSTEM_POSIX_CUI	7

typedef struct _IMAGE_NT_HEADERS {
	DWORD			Signature;	/* 00: PE\0\0 */
	IMAGE_FILE_HEADER	FileHeader;	/* 04: Fileheader */
	IMAGE_OPTIONAL_HEADER	OptionalHeader;	/* 18: Optional Header */
} IMAGE_NT_HEADERS,*LPIMAGE_NT_HEADERS;


#define PE_HEADER(module) \
    ((IMAGE_NT_HEADERS*)((LPBYTE)(module) + \
                         (((IMAGE_DOS_HEADER*)(module))->e_lfanew)))

#define PE_SECTIONS(module) \
    ((IMAGE_SECTION_HEADER*)((LPBYTE)&PE_HEADER(module)->OptionalHeader + \
                           PE_HEADER(module)->FileHeader.SizeOfOptionalHeader))

#define RVA_PTR(module,field) ((LPBYTE)(module) + PE_HEADER(module)->field)

/* Section header format */

#define	IMAGE_SIZEOF_SHORT_NAME	8

typedef struct _IMAGE_SECTION_HEADER {
	BYTE	Name[IMAGE_SIZEOF_SHORT_NAME];	/* 00: */
	union {
		DWORD	PhysicalAddress;	/* 04: */
		DWORD	VirtualSize;		/* 04: */
	} Misc;
	DWORD	VirtualAddress;			/* 08: */
	DWORD	SizeOfRawData;			/* 0C: */
	DWORD	PointerToRawData;		/* 10: */
	DWORD	PointerToRelocations;		/* 14: */
	DWORD	PointerToLinenumbers;		/* 18: */
	WORD	NumberOfRelocations;		/* 1C: */
	WORD	NumberOfLinenumbers;		/* 1E: */
	DWORD	Characteristics;		/* 20: */
						/* 24: */
} IMAGE_SECTION_HEADER,*LPIMAGE_SECTION_HEADER;

#define	IMAGE_SIZEOF_SECTION_HEADER 40

/* These defines are for the Characteristics bitfield. */
/* #define IMAGE_SCN_TYPE_REG			0x00000000 - Reserved */
/* #define IMAGE_SCN_TYPE_DSECT			0x00000001 - Reserved */
/* #define IMAGE_SCN_TYPE_NOLOAD		0x00000002 - Reserved */
/* #define IMAGE_SCN_TYPE_GROUP			0x00000004 - Reserved */
/* #define IMAGE_SCN_TYPE_NO_PAD		0x00000008 - Reserved */
/* #define IMAGE_SCN_TYPE_COPY			0x00000010 - Reserved */

#define IMAGE_SCN_CNT_CODE			0x00000020
#define IMAGE_SCN_CNT_INITIALIZED_DATA		0x00000040
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA	0x00000080

#define	IMAGE_SCN_LNK_OTHER			0x00000100 
#define	IMAGE_SCN_LNK_INFO			0x00000200  
/* #define	IMAGE_SCN_TYPE_OVER		0x00000400 - Reserved */
#define	IMAGE_SCN_LNK_REMOVE			0x00000800
#define	IMAGE_SCN_LNK_COMDAT			0x00001000

/* 						0x00002000 - Reserved */
/* #define IMAGE_SCN_MEM_PROTECTED 		0x00004000 - Obsolete */
#define	IMAGE_SCN_MEM_FARDATA			0x00008000

/* #define IMAGE_SCN_MEM_SYSHEAP		0x00010000 - Obsolete */
#define	IMAGE_SCN_MEM_PURGEABLE			0x00020000
#define	IMAGE_SCN_MEM_16BIT			0x00020000
#define	IMAGE_SCN_MEM_LOCKED			0x00040000
#define	IMAGE_SCN_MEM_PRELOAD			0x00080000

#define	IMAGE_SCN_ALIGN_1BYTES			0x00100000
#define	IMAGE_SCN_ALIGN_2BYTES			0x00200000
#define	IMAGE_SCN_ALIGN_4BYTES			0x00300000
#define	IMAGE_SCN_ALIGN_8BYTES			0x00400000
#define	IMAGE_SCN_ALIGN_16BYTES			0x00500000  /* Default */
#define IMAGE_SCN_ALIGN_32BYTES			0x00600000
#define IMAGE_SCN_ALIGN_64BYTES			0x00700000
/* 						0x00800000 - Unused */

#define IMAGE_SCN_LNK_NRELOC_OVFL		0x01000000


#define IMAGE_SCN_MEM_DISCARDABLE		0x02000000
#define IMAGE_SCN_MEM_NOT_CACHED		0x04000000
#define IMAGE_SCN_MEM_NOT_PAGED			0x08000000
#define IMAGE_SCN_MEM_SHARED			0x10000000
#define IMAGE_SCN_MEM_EXECUTE			0x20000000
#define IMAGE_SCN_MEM_READ			0x40000000
#define IMAGE_SCN_MEM_WRITE			0x80000000


/* Import name entry */
typedef struct _IMAGE_IMPORT_BY_NAME {
	WORD	Hint;
	BYTE	Name[1];
} IMAGE_IMPORT_BY_NAME,*LPIMAGE_IMPORT_BY_NAME;

/* Import thunk */
typedef struct _IMAGE_THUNK_DATA {
	union {
		LPBYTE    ForwarderString;
		FARPROC32 Function;
		DWORD     Ordinal;
		LPIMAGE_IMPORT_BY_NAME	AddressOfData;
	} u1;
} IMAGE_THUNK_DATA,*LPIMAGE_THUNK_DATA;

/* Import module directory */

typedef struct _IMAGE_IMPORT_DESCRIPTOR {
	union {
		DWORD	Characteristics; /* 0 for terminating null import descriptor  */
		LPIMAGE_THUNK_DATA OriginalFirstThunk;	/* RVA to original unbound IAT */
	} u;
	DWORD	TimeDateStamp;	/* 0 if not bound,
				 * -1 if bound, and real date\time stamp
				 *    in IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT
				 * (new BIND)
				 * otherwise date/time stamp of DLL bound to
				 * (Old BIND)
				 */
	DWORD	ForwarderChain;	/* -1 if no forwarders */
	DWORD	Name;
	/* RVA to IAT (if bound this IAT has actual addresses) */
	LPIMAGE_THUNK_DATA FirstThunk;	
} IMAGE_IMPORT_DESCRIPTOR,*LPIMAGE_IMPORT_DESCRIPTOR;

#define	IMAGE_ORDINAL_FLAG		0x80000000
#define	IMAGE_SNAP_BY_ORDINAL(Ordinal)	((Ordinal & IMAGE_ORDINAL_FLAG) != 0)
#define	IMAGE_ORDINAL(Ordinal)		(Ordinal & 0xffff)

/* Export module directory */

typedef struct _IMAGE_EXPORT_DIRECTORY {
	DWORD	Characteristics;
	DWORD	TimeDateStamp;
	WORD	MajorVersion;
	WORD	MinorVersion;
	DWORD	Name;
	DWORD	Base;
	DWORD	NumberOfFunctions;
	DWORD	NumberOfNames;
	LPDWORD	*AddressOfFunctions;
	LPDWORD	*AddressOfNames;
	LPWORD	*AddressOfNameOrdinals;
/*  u_char ModuleName[1]; */
} IMAGE_EXPORT_DIRECTORY,*LPIMAGE_EXPORT_DIRECTORY;


/*
 * Resource directory stuff
 */
typedef struct _IMAGE_RESOURCE_DIRECTORY {
	DWORD	Characteristics;
	DWORD	TimeDateStamp;
	WORD	MajorVersion;
	WORD	MinorVersion;
	WORD	NumberOfNamedEntries;
	WORD	NumberOfIdEntries;
	/*  IMAGE_RESOURCE_DIRECTORY_ENTRY DirectoryEntries[]; */
} IMAGE_RESOURCE_DIRECTORY,*LPIMAGE_RESOURCE_DIRECTORY;

#define	IMAGE_RESOURCE_NAME_IS_STRING		0x80000000
#define	IMAGE_RESOURCE_DATA_IS_DIRECTORY	0x80000000

typedef struct _IMAGE_RESOURCE_DIRECTORY_ENTRY {
	union {
		struct {
			DWORD NameOffset:31;
			DWORD NameIsString:1;
		} s;
		DWORD   Name;
		WORD    Id;
	} u1;
	union {
		DWORD   OffsetToData;
		struct {
			DWORD   OffsetToDirectory:31;
			DWORD   DataIsDirectory:1;
		} s;
	} u2;
} IMAGE_RESOURCE_DIRECTORY_ENTRY,*LPIMAGE_RESOURCE_DIRECTORY_ENTRY;


typedef struct _IMAGE_RESOURCE_DIRECTORY_STRING {
	WORD	Length;
	CHAR	NameString[ 1 ];
} IMAGE_RESOURCE_DIRECTORY_STRING,*LPIMAGE_RESOURCE_DIRECTORY_STRING;

typedef struct _IMAGE_RESOURCE_DIR_STRING_U {
	WORD	Length;
	WCHAR	NameString[ 1 ];
} IMAGE_RESOURCE_DIR_STRING_U,*LPIMAGE_RESOURCE_DIR_STRING_U;

typedef struct _IMAGE_RESOURCE_DATA_ENTRY {
	DWORD	OffsetToData;
	DWORD	Size;
	DWORD	CodePage;
	DWORD	ResourceHandle;
} IMAGE_RESOURCE_DATA_ENTRY,*LPIMAGE_RESOURCE_DATA_ENTRY;

typedef struct _IMAGE_BASE_RELOCATION
{
	DWORD	VirtualAddress;
	DWORD	SizeOfBlock;
	WORD	TypeOffset[1];
} IMAGE_BASE_RELOCATION,*LPIMAGE_BASE_RELOCATION;

typedef struct _IMAGE_LOAD_CONFIG_DIRECTORY {
	DWORD	Characteristics;
	DWORD	TimeDateStamp;
	WORD	MajorVersion;
	WORD	MinorVersion;
	DWORD	GlobalFlagsClear;
	DWORD	GlobalFlagsSet;
	DWORD	CriticalSectionDefaultTimeout;
	DWORD	DeCommitFreeBlockThreshold;
	DWORD	DeCommitTotalFreeThreshold;
	LPVOID	LockPrefixTable;
	DWORD	MaximumAllocationSize;
	DWORD	VirtualMemoryThreshold;
	DWORD	ProcessHeapFlags;
	DWORD	Reserved[ 4 ];
} IMAGE_LOAD_CONFIG_DIRECTORY,*LPIMAGE_LOAD_CONFIG_DIRECTORY;

typedef VOID (CALLBACK *LPIMAGE_TLS_CALLBACK)(
	LPVOID DllHandle,DWORD Reason,LPVOID Reserved
);

typedef struct _IMAGE_TLS_DIRECTORY {
	DWORD	StartAddressOfRawData;
	DWORD	EndAddressOfRawData;
	LPDWORD	AddressOfIndex;
	LPIMAGE_TLS_CALLBACK *AddressOfCallBacks;
	DWORD	SizeOfZeroFill;
	DWORD	Characteristics;
} IMAGE_TLS_DIRECTORY,*LPIMAGE_TLS_DIRECTORY;

/*
 * The IMAGE_DEBUG_DIRECTORY data directory points to an array of
 * these structures.
 */
typedef struct _IMAGE_DEBUG_DIRECTORY {
	DWORD	Characteristics;
	DWORD	TimeDateStamp;
	WORD	MajorVersion;
	WORD	MinorVersion;
	DWORD	Type;
	DWORD	SizeOfData;
	DWORD	AddressOfRawData;
	DWORD	PointerToRawData;
} IMAGE_DEBUG_DIRECTORY,*LPIMAGE_DEBUG_DIRECTORY;

/*
 * The type field above can take these (plus a few other
 * irrelevant) values.
 */
#define IMAGE_DEBUG_TYPE_UNKNOWN	0
#define IMAGE_DEBUG_TYPE_COFF		1
#define IMAGE_DEBUG_TYPE_CODEVIEW	2
#define IMAGE_DEBUG_TYPE_FPO		3
#define IMAGE_DEBUG_TYPE_MISC		4
#define IMAGE_DEBUG_TYPE_EXCEPTION	5
#define IMAGE_DEBUG_TYPE_FIXUP		6
#define IMAGE_DEBUG_TYPE_OMAP_TO_SRC	7
#define IMAGE_DEBUG_TYPE_OMAP_FROM_SRC	8


#define IMAGE_REL_BASED_ABSOLUTE 		0
#define IMAGE_REL_BASED_HIGH			1
#define IMAGE_REL_BASED_LOW			2
#define IMAGE_REL_BASED_HIGHLOW			3
#define IMAGE_REL_BASED_HIGHADJ			4
#define IMAGE_REL_BASED_MIPS_JMPADDR		5

/*
 * This is the structure that appears at the very start of a .DBG file.
 */
typedef struct _IMAGE_SEPARATE_DEBUG_HEADER {
	WORD	Signature;
	WORD	Flags;
	WORD	Machine;
	WORD	Characteristics;
	DWORD	TimeDateStamp;
	DWORD	CheckSum;
	DWORD	ImageBase;
	DWORD	SizeOfImage;
	DWORD	NumberOfSections;
	DWORD	ExportedNamesSize;
	DWORD	DebugDirectorySize;
	DWORD	Reserved[ 3 ];
} IMAGE_SEPARATE_DEBUG_HEADER,*LPIMAGE_SEPARATE_DEBUG_HEADER;

#define IMAGE_SEPARATE_DEBUG_SIGNATURE 0x4944

#endif /* __WINE_PEEXE_H */
