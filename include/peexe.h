/*
 * Copyright  Eric Youngdale (1994)
 */
#ifndef __WINE_PEEXE_H
#define __WINE_PEEXE_H



struct coff_header
{
	u_short Machine;
	u_short NumberOfSections;
        u_long TimeDateStamp;
        u_long PointerToSymbolTable;
        u_long NumberOfSymbols;
        u_short SizeOfOptionalHeader;
        u_short Characteristics;
};


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
#define IMAGE_FILE_MACHINE_UNKNOWN 0
#define IMAGE_FILE_MACHINE_I860    0x14d
#define IMAGE_FILE_MACHINE_I386    0x14c
#define IMAGE_FILE_MACHINE_R3000	0x162
#define IMAGE_FILE_MACHINE_R4000	0x166
#define IMAGE_FILE_MACHINE_ALPHA	0x184

struct Directory
{
  u_long Virtual_address;
  u_long Size;
};


/* Optional coff header - used by NT to provide additional information. */

struct ocoffhdr
{
	u_short Magic;  		/* Good old COFF magic 0413 */
	u_char MajorLinkerVersion;
	u_char MinorLinkerVersion;
	u_long SizeOfCode;
	u_long SizeOfInitializedData;
	u_long SizeOfUninitializedData;
	u_long AddressOfEntryPoint;
	u_long BaseOfCode;
	u_long BaseOfData;
	u_long BaseOfImage;
	u_long SectionAlignment;
	u_long FileAlignment;
	u_short MajorOperatingSystemVersion;
	u_short MinorOperatingSystemVersion;
	u_short MajorImageVersion;
	u_short MinorImageVersion;
	u_short MajorSubsystemVersion;
	u_short MinorSubsystemVersion;
	u_long Unknown1;
	u_long SizeOfImage;
	u_long SizeOfHeaders;
	u_long CheckSum;
	u_short Subsystem;
	u_short DllCharacteristics;
	u_long SizeOfStackReserve;
	u_long SizeOfStackCommit;
	u_long SizeOfHeapReserve;
	u_long SizeOfHeapCommit;
	u_long LoaderFlags;
	u_long NumberOfRvaAndSizes;
	struct Directory DataDirectory[16];
};

/* These are indexes into the DataDirectory array */
#define IMAGE_FILE_EXPORT_DIRECTORY 0
#define IMAGE_FILE_IMPORT_DIRECTORY 1
#define IMAGE_FILE_RESOURCE_DIRECTORY 2
#define IMAGE_FILE_EXCEPTION_DIRECTORY 3
#define IMAGE_FILE_SECURITY_DIRECTORY 4
#define IMAGE_FILE_BASE_RELOCATION_TABLE 5
#define IMAGE_FILE_DEBUG_DIRECTORY 6
#define IMAGE_FILE_DESCRIPTION_STRING 7
#define IMAGE_FILE_MACHINE_VALUE 8  /* Mips */
#define IMAGE_FILE_THREAD_LOCAL_STORAGE 9
#define IMAGE_FILE_CALLBACK_DIRECTORY 10

struct pe_header_s
{
	char magic[4];  /* Must be 'P', 'E', 0, 0 */
	struct coff_header coff;
	struct ocoffhdr    opt_coff;
};


struct pe_segment_table
{
  u_char Name[8];
  u_long Virtual_Size;
  u_long Virtual_Address;
  u_long Size_Of_Raw_Data;
  u_long   PointerToRawData;
  u_long   PointerToRelocations;
  u_long   PointerToLinenumbers;
  u_short  NumberOfRelocations;
  u_short  NumberOfLinenumbers;
  u_long   Characteristics;
};

/* These defines are for the Characteristics bitfield. */

#define IMAGE_SCN_TYPE_CNT_CODE 0x20
#define IMAGE_SCN_TYPE_CNT_INITIALIZED_DATA 0x40
#define IMAGE_SCN_TYPE_CNT_UNINITIALIZED_DATA 0x80
#define IMAGE_SCN_MEM_DISCARDABLE  0x2000000
#define IMAGE_SCN_MEM_SHARED 	  0x10000000
#define IMAGE_SCN_MEM_EXECUTE	  0x20000000
#define IMAGE_SCN_MEM_READ	  0x40000000
#define IMAGE_SCN_MEM_WRITE	  0x80000000

/*
 * Import module directory stuff
 */

struct PE_Import_Directory
{
  u_int Import_List;
  u_int TimeDate;
  u_int Forwarder;
  u_int ModuleName;
  u_int Thunk_List;
};

struct pe_import_name
{
  u_short Hint;
  u_char Name[1];
};

/*
 * Export module directory stuff
 */

struct PE_Export_Directory
{
  u_long Characteristics;
  u_long TimeDateStamp;
  u_short Major_version;
  u_short Minor_version;
  u_long Name;
  u_long Base;
  u_long Number_Of_Functions;
  u_long Number_Of_Names;
  u_long * AddressOfFunctions;
  u_long * AddressOfNames;
  u_short * Address_Of_Name_Ordinals;
/*  u_char ModuleName[1]; */
};

/*
 * Resource directory stuff
 */

struct PE_Resource_Directory
{
  u_long Characteristics;
  u_long TimeDateStamp;
  u_short MajorVersion;
  u_short MinorVersion;
  u_short NumberOfNamedEntries;
  u_short NumberOfIdEntries;
};

struct PE_Directory_Entry
{
  u_long Name;
  u_long OffsetToData;
};

struct PE_Directory_Name_String
{
  u_short Length;
  char NameString[1];
};

struct PE_Directory_Name_String_U
{
  u_short Length;
  u_short NameString[1];
};

struct PE_Resource_Leaf_Entry
{
  u_long OffsetToData;
  u_long Size;
  u_long CodePage;
  u_long Reserved;
};

#define IMAGE_RESOURCE_NAME_IS_STRING    0x80000000
#define IMAGE_RESOURCE_DATA_IS_DIRECTORY 0x80000000

struct PE_Reloc_Block
{
	u_long PageRVA;
	u_long BlockSize;
	short Relocations[1];
};

#define IMAGE_REL_BASED_ABSOLUTE 		0
#define IMAGE_REL_BASED_HIGH			1
#define IMAGE_REL_BASED_LOW				2
#define IMAGE_REL_BASED_HIGHLOW			3
#define IMAGE_REL_BASED_HIGHADJ			4
#define IMAGE_REL_BASED_MIPS_JMPADDR	5

#endif /* __WINE_PEEXE_H */
