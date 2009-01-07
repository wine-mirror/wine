/* File generated automatically from tools/winapi/tests.dat; do not edit! */
/* This file can be copied, modified and distributed without restriction. */

/*
 * Unit tests for data structure packing
 */

#define WINVER 0x0501
#define _WIN32_IE 0x0501
#define _WIN32_WINNT 0x0501

#define WINE_NOWINSOCK

#include "ntdll_test.h"

#include "wine/test.h"

/***********************************************************************
 * Compatibility macros
 */

#define DWORD_PTR UINT_PTR
#define LONG_PTR INT_PTR
#define ULONG_PTR UINT_PTR

/***********************************************************************
 * Windows API extension
 */

#if defined(_MSC_VER) && (_MSC_VER >= 1300) && defined(__cplusplus)
# define _TYPE_ALIGNMENT(type) __alignof(type)
#elif defined(__GNUC__)
# define _TYPE_ALIGNMENT(type) __alignof__(type)
#else
/*
 * FIXME: May not be possible without a compiler extension
 *        (if type is not just a name that is, otherwise the normal
 *         TYPE_ALIGNMENT can be used)
 */
#endif

#if defined(TYPE_ALIGNMENT) && defined(_MSC_VER) && _MSC_VER >= 800 && !defined(__cplusplus)
#pragma warning(disable:4116)
#endif

#if !defined(TYPE_ALIGNMENT) && defined(_TYPE_ALIGNMENT)
# define TYPE_ALIGNMENT _TYPE_ALIGNMENT
#endif

/***********************************************************************
 * Test helper macros
 */

#ifdef FIELD_ALIGNMENT
# define TEST_FIELD_ALIGNMENT(type, field, align) \
   ok(_TYPE_ALIGNMENT(((type*)0)->field) == align, \
       "FIELD_ALIGNMENT(" #type ", " #field ") == %d (expected " #align ")\n", \
           (int)_TYPE_ALIGNMENT(((type*)0)->field))
#else
# define TEST_FIELD_ALIGNMENT(type, field, align) do { } while (0)
#endif

#define TEST_FIELD_OFFSET(type, field, offset) \
    ok(FIELD_OFFSET(type, field) == offset, \
        "FIELD_OFFSET(" #type ", " #field ") == %ld (expected " #offset ")\n", \
             (long int)FIELD_OFFSET(type, field))

#ifdef _TYPE_ALIGNMENT
#define TEST__TYPE_ALIGNMENT(type, align) \
    ok(_TYPE_ALIGNMENT(type) == align, "TYPE_ALIGNMENT(" #type ") == %d (expected " #align ")\n", (int)_TYPE_ALIGNMENT(type))
#else
# define TEST__TYPE_ALIGNMENT(type, align) do { } while (0)
#endif

#ifdef TYPE_ALIGNMENT
#define TEST_TYPE_ALIGNMENT(type, align) \
    ok(TYPE_ALIGNMENT(type) == align, "TYPE_ALIGNMENT(" #type ") == %d (expected " #align ")\n", (int)TYPE_ALIGNMENT(type))
#else
# define TEST_TYPE_ALIGNMENT(type, align) do { } while (0)
#endif

#define TEST_TYPE_SIZE(type, size) \
    ok(sizeof(type) == size, "sizeof(" #type ") == %d (expected " #size ")\n", ((int) sizeof(type)))

/***********************************************************************
 * Test macros
 */

#define TEST_FIELD(type, field, field_offset, field_size, field_align) \
  TEST_TYPE_SIZE((((type*)0)->field), field_size); \
  TEST_FIELD_ALIGNMENT(type, field, field_align); \
  TEST_FIELD_OFFSET(type, field, field_offset)

#define TEST_TYPE(type, size, align) \
  TEST_TYPE_ALIGNMENT(type, align); \
  TEST_TYPE_SIZE(type, size)

#define TEST_TYPE_POINTER(type, size, align) \
    TEST__TYPE_ALIGNMENT(*(type)0, align); \
    TEST_TYPE_SIZE(*(type)0, size)

#define TEST_TYPE_SIGNED(type) \
    ok((type) -1 < 0, "(" #type ") -1 < 0\n");

#define TEST_TYPE_UNSIGNED(type) \
     ok((type) -1 > 0, "(" #type ") -1 > 0\n");

static void test_pack_DWORD32(void)
{
    /* DWORD32 */
    TEST_TYPE(DWORD32, 4, 4);
    TEST_TYPE_UNSIGNED(DWORD32);
}

static void test_pack_DWORD64(void)
{
    /* DWORD64 */
    TEST_TYPE(DWORD64, 8, 8);
    TEST_TYPE_UNSIGNED(DWORD64);
}

static void test_pack_DWORD_PTR(void)
{
    /* DWORD_PTR */
    TEST_TYPE(DWORD_PTR, 4, 4);
}

static void test_pack_HALF_PTR(void)
{
    /* HALF_PTR */
    TEST_TYPE(HALF_PTR, 2, 2);
    TEST_TYPE_SIGNED(HALF_PTR);
}

static void test_pack_INT16(void)
{
    /* INT16 */
    TEST_TYPE(INT16, 2, 2);
    TEST_TYPE_SIGNED(INT16);
}

static void test_pack_INT32(void)
{
    /* INT32 */
    TEST_TYPE(INT32, 4, 4);
    TEST_TYPE_SIGNED(INT32);
}

static void test_pack_INT64(void)
{
    /* INT64 */
    TEST_TYPE(INT64, 8, 8);
    TEST_TYPE_SIGNED(INT64);
}

static void test_pack_INT8(void)
{
    /* INT8 */
    TEST_TYPE(INT8, 1, 1);
    TEST_TYPE_SIGNED(INT8);
}

static void test_pack_INT_PTR(void)
{
    /* INT_PTR */
    TEST_TYPE(INT_PTR, 4, 4);
    TEST_TYPE_SIGNED(INT_PTR);
}

static void test_pack_LONG32(void)
{
    /* LONG32 */
    TEST_TYPE(LONG32, 4, 4);
    TEST_TYPE_SIGNED(LONG32);
}

static void test_pack_LONG64(void)
{
    /* LONG64 */
    TEST_TYPE(LONG64, 8, 8);
    TEST_TYPE_SIGNED(LONG64);
}

static void test_pack_LONG_PTR(void)
{
    /* LONG_PTR */
    TEST_TYPE(LONG_PTR, 4, 4);
    TEST_TYPE_SIGNED(LONG_PTR);
}

static void test_pack_SIZE_T(void)
{
    /* SIZE_T */
    TEST_TYPE(SIZE_T, 4, 4);
}

static void test_pack_SSIZE_T(void)
{
    /* SSIZE_T */
    TEST_TYPE(SSIZE_T, 4, 4);
}

static void test_pack_UHALF_PTR(void)
{
    /* UHALF_PTR */
    TEST_TYPE(UHALF_PTR, 2, 2);
    TEST_TYPE_UNSIGNED(UHALF_PTR);
}

static void test_pack_UINT16(void)
{
    /* UINT16 */
    TEST_TYPE(UINT16, 2, 2);
    TEST_TYPE_UNSIGNED(UINT16);
}

static void test_pack_UINT32(void)
{
    /* UINT32 */
    TEST_TYPE(UINT32, 4, 4);
    TEST_TYPE_UNSIGNED(UINT32);
}

static void test_pack_UINT64(void)
{
    /* UINT64 */
    TEST_TYPE(UINT64, 8, 8);
    TEST_TYPE_UNSIGNED(UINT64);
}

static void test_pack_UINT8(void)
{
    /* UINT8 */
    TEST_TYPE(UINT8, 1, 1);
    TEST_TYPE_UNSIGNED(UINT8);
}

static void test_pack_UINT_PTR(void)
{
    /* UINT_PTR */
    TEST_TYPE(UINT_PTR, 4, 4);
    TEST_TYPE_UNSIGNED(UINT_PTR);
}

static void test_pack_ULONG32(void)
{
    /* ULONG32 */
    TEST_TYPE(ULONG32, 4, 4);
    TEST_TYPE_UNSIGNED(ULONG32);
}

static void test_pack_ULONG64(void)
{
    /* ULONG64 */
    TEST_TYPE(ULONG64, 8, 8);
    TEST_TYPE_UNSIGNED(ULONG64);
}

static void test_pack_ULONG_PTR(void)
{
    /* ULONG_PTR */
    TEST_TYPE(ULONG_PTR, 4, 4);
    TEST_TYPE_UNSIGNED(ULONG_PTR);
}

static void test_pack_ACCESS_ALLOWED_ACE(void)
{
    /* ACCESS_ALLOWED_ACE (pack 4) */
    TEST_TYPE(ACCESS_ALLOWED_ACE, 12, 4);
    TEST_FIELD(ACCESS_ALLOWED_ACE, Header, 0, 4, 2);
    TEST_FIELD(ACCESS_ALLOWED_ACE, Mask, 4, 4, 4);
    TEST_FIELD(ACCESS_ALLOWED_ACE, SidStart, 8, 4, 4);
}

static void test_pack_ACCESS_DENIED_ACE(void)
{
    /* ACCESS_DENIED_ACE (pack 4) */
    TEST_TYPE(ACCESS_DENIED_ACE, 12, 4);
    TEST_FIELD(ACCESS_DENIED_ACE, Header, 0, 4, 2);
    TEST_FIELD(ACCESS_DENIED_ACE, Mask, 4, 4, 4);
    TEST_FIELD(ACCESS_DENIED_ACE, SidStart, 8, 4, 4);
}

static void test_pack_ACCESS_MASK(void)
{
    /* ACCESS_MASK */
    TEST_TYPE(ACCESS_MASK, 4, 4);
    TEST_TYPE_UNSIGNED(ACCESS_MASK);
}

static void test_pack_ACE_HEADER(void)
{
    /* ACE_HEADER (pack 4) */
    TEST_TYPE(ACE_HEADER, 4, 2);
    TEST_FIELD(ACE_HEADER, AceType, 0, 1, 1);
    TEST_FIELD(ACE_HEADER, AceFlags, 1, 1, 1);
    TEST_FIELD(ACE_HEADER, AceSize, 2, 2, 2);
}

static void test_pack_ACL(void)
{
    /* ACL (pack 4) */
    TEST_TYPE(ACL, 8, 2);
    TEST_FIELD(ACL, AclRevision, 0, 1, 1);
    TEST_FIELD(ACL, Sbz1, 1, 1, 1);
    TEST_FIELD(ACL, AclSize, 2, 2, 2);
    TEST_FIELD(ACL, AceCount, 4, 2, 2);
    TEST_FIELD(ACL, Sbz2, 6, 2, 2);
}

static void test_pack_ACL_REVISION_INFORMATION(void)
{
    /* ACL_REVISION_INFORMATION (pack 4) */
    TEST_TYPE(ACL_REVISION_INFORMATION, 4, 4);
    TEST_FIELD(ACL_REVISION_INFORMATION, AclRevision, 0, 4, 4);
}

static void test_pack_ACL_SIZE_INFORMATION(void)
{
    /* ACL_SIZE_INFORMATION (pack 4) */
    TEST_TYPE(ACL_SIZE_INFORMATION, 12, 4);
    TEST_FIELD(ACL_SIZE_INFORMATION, AceCount, 0, 4, 4);
    TEST_FIELD(ACL_SIZE_INFORMATION, AclBytesInUse, 4, 4, 4);
    TEST_FIELD(ACL_SIZE_INFORMATION, AclBytesFree, 8, 4, 4);
}

static void test_pack_BOOLEAN(void)
{
    /* BOOLEAN */
    TEST_TYPE(BOOLEAN, 1, 1);
    TEST_TYPE_UNSIGNED(BOOLEAN);
}

static void test_pack_CCHAR(void)
{
    /* CCHAR */
    TEST_TYPE(CCHAR, 1, 1);
    TEST_TYPE_SIGNED(CCHAR);
}

static void test_pack_CHAR(void)
{
    /* CHAR */
    TEST_TYPE(CHAR, 1, 1);
    TEST_TYPE_SIGNED(CHAR);
}

static void test_pack_DWORDLONG(void)
{
    /* DWORDLONG */
    TEST_TYPE(DWORDLONG, 8, 8);
    TEST_TYPE_UNSIGNED(DWORDLONG);
}

static void test_pack_EXCEPTION_POINTERS(void)
{
    /* EXCEPTION_POINTERS (pack 4) */
    TEST_TYPE(EXCEPTION_POINTERS, 8, 4);
    TEST_FIELD(EXCEPTION_POINTERS, ExceptionRecord, 0, 4, 4);
    TEST_FIELD(EXCEPTION_POINTERS, ContextRecord, 4, 4, 4);
}

static void test_pack_EXCEPTION_RECORD(void)
{
    /* EXCEPTION_RECORD (pack 4) */
    TEST_TYPE(EXCEPTION_RECORD, 80, 4);
    TEST_FIELD(EXCEPTION_RECORD, ExceptionCode, 0, 4, 4);
    TEST_FIELD(EXCEPTION_RECORD, ExceptionFlags, 4, 4, 4);
    TEST_FIELD(EXCEPTION_RECORD, ExceptionRecord, 8, 4, 4);
    TEST_FIELD(EXCEPTION_RECORD, ExceptionAddress, 12, 4, 4);
    TEST_FIELD(EXCEPTION_RECORD, NumberParameters, 16, 4, 4);
    TEST_FIELD(EXCEPTION_RECORD, ExceptionInformation, 20, 60, 4);
}

static void test_pack_EXECUTION_STATE(void)
{
    /* EXECUTION_STATE */
    TEST_TYPE(EXECUTION_STATE, 4, 4);
    TEST_TYPE_UNSIGNED(EXECUTION_STATE);
}

static void test_pack_FLOATING_SAVE_AREA(void)
{
    /* FLOATING_SAVE_AREA (pack 4) */
    TEST_TYPE(FLOATING_SAVE_AREA, 112, 4);
    TEST_FIELD(FLOATING_SAVE_AREA, ControlWord, 0, 4, 4);
    TEST_FIELD(FLOATING_SAVE_AREA, StatusWord, 4, 4, 4);
    TEST_FIELD(FLOATING_SAVE_AREA, TagWord, 8, 4, 4);
    TEST_FIELD(FLOATING_SAVE_AREA, ErrorOffset, 12, 4, 4);
    TEST_FIELD(FLOATING_SAVE_AREA, ErrorSelector, 16, 4, 4);
    TEST_FIELD(FLOATING_SAVE_AREA, DataOffset, 20, 4, 4);
    TEST_FIELD(FLOATING_SAVE_AREA, DataSelector, 24, 4, 4);
    TEST_FIELD(FLOATING_SAVE_AREA, RegisterArea, 28, 80, 1);
    TEST_FIELD(FLOATING_SAVE_AREA, Cr0NpxState, 108, 4, 4);
}

static void test_pack_FPO_DATA(void)
{
    /* FPO_DATA (pack 4) */
    TEST_TYPE(FPO_DATA, 16, 4);
    TEST_FIELD(FPO_DATA, ulOffStart, 0, 4, 4);
    TEST_FIELD(FPO_DATA, cbProcSize, 4, 4, 4);
    TEST_FIELD(FPO_DATA, cdwLocals, 8, 4, 4);
    TEST_FIELD(FPO_DATA, cdwParams, 12, 2, 2);
}

static void test_pack_GENERIC_MAPPING(void)
{
    /* GENERIC_MAPPING (pack 4) */
    TEST_TYPE(GENERIC_MAPPING, 16, 4);
    TEST_FIELD(GENERIC_MAPPING, GenericRead, 0, 4, 4);
    TEST_FIELD(GENERIC_MAPPING, GenericWrite, 4, 4, 4);
    TEST_FIELD(GENERIC_MAPPING, GenericExecute, 8, 4, 4);
    TEST_FIELD(GENERIC_MAPPING, GenericAll, 12, 4, 4);
}

static void test_pack_HANDLE(void)
{
    /* HANDLE */
    TEST_TYPE(HANDLE, 4, 4);
}

static void test_pack_HRESULT(void)
{
    /* HRESULT */
    TEST_TYPE(HRESULT, 4, 4);
}

static void test_pack_IMAGE_ARCHIVE_MEMBER_HEADER(void)
{
    /* IMAGE_ARCHIVE_MEMBER_HEADER (pack 4) */
    TEST_TYPE(IMAGE_ARCHIVE_MEMBER_HEADER, 60, 1);
    TEST_FIELD(IMAGE_ARCHIVE_MEMBER_HEADER, Name, 0, 16, 1);
    TEST_FIELD(IMAGE_ARCHIVE_MEMBER_HEADER, Date, 16, 12, 1);
    TEST_FIELD(IMAGE_ARCHIVE_MEMBER_HEADER, UserID, 28, 6, 1);
    TEST_FIELD(IMAGE_ARCHIVE_MEMBER_HEADER, GroupID, 34, 6, 1);
    TEST_FIELD(IMAGE_ARCHIVE_MEMBER_HEADER, Mode, 40, 8, 1);
    TEST_FIELD(IMAGE_ARCHIVE_MEMBER_HEADER, Size, 48, 10, 1);
    TEST_FIELD(IMAGE_ARCHIVE_MEMBER_HEADER, EndHeader, 58, 2, 1);
}

static void test_pack_IMAGE_AUX_SYMBOL(void)
{
    /* IMAGE_AUX_SYMBOL (pack 2) */
}

static void test_pack_IMAGE_BASE_RELOCATION(void)
{
    /* IMAGE_BASE_RELOCATION (pack 4) */
    TEST_TYPE(IMAGE_BASE_RELOCATION, 8, 4);
    TEST_FIELD(IMAGE_BASE_RELOCATION, VirtualAddress, 0, 4, 4);
    TEST_FIELD(IMAGE_BASE_RELOCATION, SizeOfBlock, 4, 4, 4);
}

static void test_pack_IMAGE_BOUND_FORWARDER_REF(void)
{
    /* IMAGE_BOUND_FORWARDER_REF (pack 4) */
    TEST_TYPE(IMAGE_BOUND_FORWARDER_REF, 8, 4);
    TEST_FIELD(IMAGE_BOUND_FORWARDER_REF, TimeDateStamp, 0, 4, 4);
    TEST_FIELD(IMAGE_BOUND_FORWARDER_REF, OffsetModuleName, 4, 2, 2);
    TEST_FIELD(IMAGE_BOUND_FORWARDER_REF, Reserved, 6, 2, 2);
}

static void test_pack_IMAGE_BOUND_IMPORT_DESCRIPTOR(void)
{
    /* IMAGE_BOUND_IMPORT_DESCRIPTOR (pack 4) */
    TEST_TYPE(IMAGE_BOUND_IMPORT_DESCRIPTOR, 8, 4);
    TEST_FIELD(IMAGE_BOUND_IMPORT_DESCRIPTOR, TimeDateStamp, 0, 4, 4);
    TEST_FIELD(IMAGE_BOUND_IMPORT_DESCRIPTOR, OffsetModuleName, 4, 2, 2);
    TEST_FIELD(IMAGE_BOUND_IMPORT_DESCRIPTOR, NumberOfModuleForwarderRefs, 6, 2, 2);
}

static void test_pack_IMAGE_COFF_SYMBOLS_HEADER(void)
{
    /* IMAGE_COFF_SYMBOLS_HEADER (pack 4) */
    TEST_TYPE(IMAGE_COFF_SYMBOLS_HEADER, 32, 4);
    TEST_FIELD(IMAGE_COFF_SYMBOLS_HEADER, NumberOfSymbols, 0, 4, 4);
    TEST_FIELD(IMAGE_COFF_SYMBOLS_HEADER, LvaToFirstSymbol, 4, 4, 4);
    TEST_FIELD(IMAGE_COFF_SYMBOLS_HEADER, NumberOfLinenumbers, 8, 4, 4);
    TEST_FIELD(IMAGE_COFF_SYMBOLS_HEADER, LvaToFirstLinenumber, 12, 4, 4);
    TEST_FIELD(IMAGE_COFF_SYMBOLS_HEADER, RvaToFirstByteOfCode, 16, 4, 4);
    TEST_FIELD(IMAGE_COFF_SYMBOLS_HEADER, RvaToLastByteOfCode, 20, 4, 4);
    TEST_FIELD(IMAGE_COFF_SYMBOLS_HEADER, RvaToFirstByteOfData, 24, 4, 4);
    TEST_FIELD(IMAGE_COFF_SYMBOLS_HEADER, RvaToLastByteOfData, 28, 4, 4);
}

static void test_pack_IMAGE_DATA_DIRECTORY(void)
{
    /* IMAGE_DATA_DIRECTORY (pack 4) */
    TEST_TYPE(IMAGE_DATA_DIRECTORY, 8, 4);
    TEST_FIELD(IMAGE_DATA_DIRECTORY, VirtualAddress, 0, 4, 4);
    TEST_FIELD(IMAGE_DATA_DIRECTORY, Size, 4, 4, 4);
}

static void test_pack_IMAGE_DEBUG_DIRECTORY(void)
{
    /* IMAGE_DEBUG_DIRECTORY (pack 4) */
    TEST_TYPE(IMAGE_DEBUG_DIRECTORY, 28, 4);
    TEST_FIELD(IMAGE_DEBUG_DIRECTORY, Characteristics, 0, 4, 4);
    TEST_FIELD(IMAGE_DEBUG_DIRECTORY, TimeDateStamp, 4, 4, 4);
    TEST_FIELD(IMAGE_DEBUG_DIRECTORY, MajorVersion, 8, 2, 2);
    TEST_FIELD(IMAGE_DEBUG_DIRECTORY, MinorVersion, 10, 2, 2);
    TEST_FIELD(IMAGE_DEBUG_DIRECTORY, Type, 12, 4, 4);
    TEST_FIELD(IMAGE_DEBUG_DIRECTORY, SizeOfData, 16, 4, 4);
    TEST_FIELD(IMAGE_DEBUG_DIRECTORY, AddressOfRawData, 20, 4, 4);
    TEST_FIELD(IMAGE_DEBUG_DIRECTORY, PointerToRawData, 24, 4, 4);
}

static void test_pack_IMAGE_DEBUG_MISC(void)
{
    /* IMAGE_DEBUG_MISC (pack 4) */
    TEST_TYPE(IMAGE_DEBUG_MISC, 16, 4);
    TEST_FIELD(IMAGE_DEBUG_MISC, DataType, 0, 4, 4);
    TEST_FIELD(IMAGE_DEBUG_MISC, Length, 4, 4, 4);
    TEST_FIELD(IMAGE_DEBUG_MISC, Unicode, 8, 1, 1);
    TEST_FIELD(IMAGE_DEBUG_MISC, Reserved, 9, 3, 1);
    TEST_FIELD(IMAGE_DEBUG_MISC, Data, 12, 1, 1);
}

static void test_pack_IMAGE_DOS_HEADER(void)
{
    /* IMAGE_DOS_HEADER (pack 2) */
    TEST_TYPE(IMAGE_DOS_HEADER, 64, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, e_magic, 0, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, e_cblp, 2, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, e_cp, 4, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, e_crlc, 6, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, e_cparhdr, 8, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, e_minalloc, 10, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, e_maxalloc, 12, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, e_ss, 14, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, e_sp, 16, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, e_csum, 18, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, e_ip, 20, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, e_cs, 22, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, e_lfarlc, 24, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, e_ovno, 26, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, e_res, 28, 8, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, e_oemid, 36, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, e_oeminfo, 38, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, e_res2, 40, 20, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, e_lfanew, 60, 4, 2);
}

static void test_pack_IMAGE_EXPORT_DIRECTORY(void)
{
    /* IMAGE_EXPORT_DIRECTORY (pack 4) */
    TEST_TYPE(IMAGE_EXPORT_DIRECTORY, 40, 4);
    TEST_FIELD(IMAGE_EXPORT_DIRECTORY, Characteristics, 0, 4, 4);
    TEST_FIELD(IMAGE_EXPORT_DIRECTORY, TimeDateStamp, 4, 4, 4);
    TEST_FIELD(IMAGE_EXPORT_DIRECTORY, MajorVersion, 8, 2, 2);
    TEST_FIELD(IMAGE_EXPORT_DIRECTORY, MinorVersion, 10, 2, 2);
    TEST_FIELD(IMAGE_EXPORT_DIRECTORY, Name, 12, 4, 4);
    TEST_FIELD(IMAGE_EXPORT_DIRECTORY, Base, 16, 4, 4);
    TEST_FIELD(IMAGE_EXPORT_DIRECTORY, NumberOfFunctions, 20, 4, 4);
    TEST_FIELD(IMAGE_EXPORT_DIRECTORY, NumberOfNames, 24, 4, 4);
    TEST_FIELD(IMAGE_EXPORT_DIRECTORY, AddressOfFunctions, 28, 4, 4);
    TEST_FIELD(IMAGE_EXPORT_DIRECTORY, AddressOfNames, 32, 4, 4);
    TEST_FIELD(IMAGE_EXPORT_DIRECTORY, AddressOfNameOrdinals, 36, 4, 4);
}

static void test_pack_IMAGE_FILE_HEADER(void)
{
    /* IMAGE_FILE_HEADER (pack 4) */
    TEST_TYPE(IMAGE_FILE_HEADER, 20, 4);
    TEST_FIELD(IMAGE_FILE_HEADER, Machine, 0, 2, 2);
    TEST_FIELD(IMAGE_FILE_HEADER, NumberOfSections, 2, 2, 2);
    TEST_FIELD(IMAGE_FILE_HEADER, TimeDateStamp, 4, 4, 4);
    TEST_FIELD(IMAGE_FILE_HEADER, PointerToSymbolTable, 8, 4, 4);
    TEST_FIELD(IMAGE_FILE_HEADER, NumberOfSymbols, 12, 4, 4);
    TEST_FIELD(IMAGE_FILE_HEADER, SizeOfOptionalHeader, 16, 2, 2);
    TEST_FIELD(IMAGE_FILE_HEADER, Characteristics, 18, 2, 2);
}

static void test_pack_IMAGE_FUNCTION_ENTRY(void)
{
    /* IMAGE_FUNCTION_ENTRY (pack 4) */
    TEST_TYPE(IMAGE_FUNCTION_ENTRY, 12, 4);
    TEST_FIELD(IMAGE_FUNCTION_ENTRY, StartingAddress, 0, 4, 4);
    TEST_FIELD(IMAGE_FUNCTION_ENTRY, EndingAddress, 4, 4, 4);
    TEST_FIELD(IMAGE_FUNCTION_ENTRY, EndOfPrologue, 8, 4, 4);
}

static void test_pack_IMAGE_IMPORT_BY_NAME(void)
{
    /* IMAGE_IMPORT_BY_NAME (pack 4) */
    TEST_TYPE(IMAGE_IMPORT_BY_NAME, 4, 2);
    TEST_FIELD(IMAGE_IMPORT_BY_NAME, Hint, 0, 2, 2);
    TEST_FIELD(IMAGE_IMPORT_BY_NAME, Name, 2, 1, 1);
}

static void test_pack_IMAGE_IMPORT_DESCRIPTOR(void)
{
    /* IMAGE_IMPORT_DESCRIPTOR (pack 4) */
}

static void test_pack_IMAGE_LINENUMBER(void)
{
    /* IMAGE_LINENUMBER (pack 2) */
}

static void test_pack_IMAGE_LOAD_CONFIG_DIRECTORY(void)
{
    /* IMAGE_LOAD_CONFIG_DIRECTORY (pack 4) */
    TEST_TYPE(IMAGE_LOAD_CONFIG_DIRECTORY, 72, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, Size, 0, 4, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, TimeDateStamp, 4, 4, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, MajorVersion, 8, 2, 2);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, MinorVersion, 10, 2, 2);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, GlobalFlagsClear, 12, 4, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, GlobalFlagsSet, 16, 4, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, CriticalSectionDefaultTimeout, 20, 4, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, DeCommitFreeBlockThreshold, 24, 4, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, DeCommitTotalFreeThreshold, 28, 4, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, LockPrefixTable, 32, 4, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, MaximumAllocationSize, 36, 4, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, VirtualMemoryThreshold, 40, 4, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, ProcessHeapFlags, 44, 4, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, ProcessAffinityMask, 48, 4, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, CSDVersion, 52, 2, 2);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, Reserved1, 54, 2, 2);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, EditList, 56, 4, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, SecurityCookie, 60, 4, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, SEHandlerTable, 64, 4, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, SEHandlerCount, 68, 4, 4);
}

static void test_pack_IMAGE_NT_HEADERS(void)
{
    /* IMAGE_NT_HEADERS */
    TEST_TYPE(IMAGE_NT_HEADERS, 248, 4);
}

static void test_pack_IMAGE_OS2_HEADER(void)
{
    /* IMAGE_OS2_HEADER (pack 2) */
    TEST_TYPE(IMAGE_OS2_HEADER, 64, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, ne_magic, 0, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, ne_ver, 2, 1, 1);
    TEST_FIELD(IMAGE_OS2_HEADER, ne_rev, 3, 1, 1);
    TEST_FIELD(IMAGE_OS2_HEADER, ne_enttab, 4, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, ne_cbenttab, 6, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, ne_crc, 8, 4, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, ne_flags, 12, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, ne_autodata, 14, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, ne_heap, 16, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, ne_stack, 18, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, ne_csip, 20, 4, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, ne_sssp, 24, 4, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, ne_cseg, 28, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, ne_cmod, 30, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, ne_cbnrestab, 32, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, ne_segtab, 34, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, ne_rsrctab, 36, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, ne_restab, 38, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, ne_modtab, 40, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, ne_imptab, 42, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, ne_nrestab, 44, 4, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, ne_cmovent, 48, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, ne_align, 50, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, ne_cres, 52, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, ne_exetyp, 54, 1, 1);
    TEST_FIELD(IMAGE_OS2_HEADER, ne_flagsothers, 55, 1, 1);
    TEST_FIELD(IMAGE_OS2_HEADER, ne_pretthunks, 56, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, ne_psegrefbytes, 58, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, ne_swaparea, 60, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, ne_expver, 62, 2, 2);
}

static void test_pack_IMAGE_RELOCATION(void)
{
    /* IMAGE_RELOCATION (pack 2) */
}

static void test_pack_IMAGE_RESOURCE_DATA_ENTRY(void)
{
    /* IMAGE_RESOURCE_DATA_ENTRY (pack 4) */
    TEST_TYPE(IMAGE_RESOURCE_DATA_ENTRY, 16, 4);
    TEST_FIELD(IMAGE_RESOURCE_DATA_ENTRY, OffsetToData, 0, 4, 4);
    TEST_FIELD(IMAGE_RESOURCE_DATA_ENTRY, Size, 4, 4, 4);
    TEST_FIELD(IMAGE_RESOURCE_DATA_ENTRY, CodePage, 8, 4, 4);
    TEST_FIELD(IMAGE_RESOURCE_DATA_ENTRY, Reserved, 12, 4, 4);
}

static void test_pack_IMAGE_RESOURCE_DIRECTORY(void)
{
    /* IMAGE_RESOURCE_DIRECTORY (pack 4) */
    TEST_TYPE(IMAGE_RESOURCE_DIRECTORY, 16, 4);
    TEST_FIELD(IMAGE_RESOURCE_DIRECTORY, Characteristics, 0, 4, 4);
    TEST_FIELD(IMAGE_RESOURCE_DIRECTORY, TimeDateStamp, 4, 4, 4);
    TEST_FIELD(IMAGE_RESOURCE_DIRECTORY, MajorVersion, 8, 2, 2);
    TEST_FIELD(IMAGE_RESOURCE_DIRECTORY, MinorVersion, 10, 2, 2);
    TEST_FIELD(IMAGE_RESOURCE_DIRECTORY, NumberOfNamedEntries, 12, 2, 2);
    TEST_FIELD(IMAGE_RESOURCE_DIRECTORY, NumberOfIdEntries, 14, 2, 2);
}

static void test_pack_IMAGE_RESOURCE_DIRECTORY_ENTRY(void)
{
    /* IMAGE_RESOURCE_DIRECTORY_ENTRY (pack 4) */
}

static void test_pack_IMAGE_RESOURCE_DIRECTORY_STRING(void)
{
    /* IMAGE_RESOURCE_DIRECTORY_STRING (pack 4) */
    TEST_TYPE(IMAGE_RESOURCE_DIRECTORY_STRING, 4, 2);
    TEST_FIELD(IMAGE_RESOURCE_DIRECTORY_STRING, Length, 0, 2, 2);
    TEST_FIELD(IMAGE_RESOURCE_DIRECTORY_STRING, NameString, 2, 1, 1);
}

static void test_pack_IMAGE_RESOURCE_DIR_STRING_U(void)
{
    /* IMAGE_RESOURCE_DIR_STRING_U (pack 4) */
    TEST_TYPE(IMAGE_RESOURCE_DIR_STRING_U, 4, 2);
    TEST_FIELD(IMAGE_RESOURCE_DIR_STRING_U, Length, 0, 2, 2);
    TEST_FIELD(IMAGE_RESOURCE_DIR_STRING_U, NameString, 2, 2, 2);
}

static void test_pack_IMAGE_SECTION_HEADER(void)
{
    /* IMAGE_SECTION_HEADER (pack 4) */
    TEST_FIELD(IMAGE_SECTION_HEADER, Name, 0, 8, 1);
}

static void test_pack_IMAGE_SEPARATE_DEBUG_HEADER(void)
{
    /* IMAGE_SEPARATE_DEBUG_HEADER (pack 4) */
    TEST_TYPE(IMAGE_SEPARATE_DEBUG_HEADER, 48, 4);
    TEST_FIELD(IMAGE_SEPARATE_DEBUG_HEADER, Signature, 0, 2, 2);
    TEST_FIELD(IMAGE_SEPARATE_DEBUG_HEADER, Flags, 2, 2, 2);
    TEST_FIELD(IMAGE_SEPARATE_DEBUG_HEADER, Machine, 4, 2, 2);
    TEST_FIELD(IMAGE_SEPARATE_DEBUG_HEADER, Characteristics, 6, 2, 2);
    TEST_FIELD(IMAGE_SEPARATE_DEBUG_HEADER, TimeDateStamp, 8, 4, 4);
    TEST_FIELD(IMAGE_SEPARATE_DEBUG_HEADER, CheckSum, 12, 4, 4);
    TEST_FIELD(IMAGE_SEPARATE_DEBUG_HEADER, ImageBase, 16, 4, 4);
    TEST_FIELD(IMAGE_SEPARATE_DEBUG_HEADER, SizeOfImage, 20, 4, 4);
    TEST_FIELD(IMAGE_SEPARATE_DEBUG_HEADER, NumberOfSections, 24, 4, 4);
    TEST_FIELD(IMAGE_SEPARATE_DEBUG_HEADER, ExportedNamesSize, 28, 4, 4);
    TEST_FIELD(IMAGE_SEPARATE_DEBUG_HEADER, DebugDirectorySize, 32, 4, 4);
    TEST_FIELD(IMAGE_SEPARATE_DEBUG_HEADER, SectionAlignment, 36, 4, 4);
    TEST_FIELD(IMAGE_SEPARATE_DEBUG_HEADER, Reserved, 40, 8, 4);
}

static void test_pack_IMAGE_SYMBOL(void)
{
    /* IMAGE_SYMBOL (pack 2) */
}

static void test_pack_IMAGE_THUNK_DATA(void)
{
    /* IMAGE_THUNK_DATA */
}

static void test_pack_IMAGE_TLS_DIRECTORY(void)
{
    /* IMAGE_TLS_DIRECTORY */
    TEST_TYPE(IMAGE_TLS_DIRECTORY, 24, 4);
}

static void test_pack_IMAGE_VXD_HEADER(void)
{
    /* IMAGE_VXD_HEADER (pack 2) */
    TEST_TYPE(IMAGE_VXD_HEADER, 196, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_magic, 0, 2, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_border, 2, 1, 1);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_worder, 3, 1, 1);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_level, 4, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_cpu, 8, 2, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_os, 10, 2, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_ver, 12, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_mflags, 16, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_mpages, 20, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_startobj, 24, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_eip, 28, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_stackobj, 32, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_esp, 36, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_pagesize, 40, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_lastpagesize, 44, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_fixupsize, 48, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_fixupsum, 52, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_ldrsize, 56, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_ldrsum, 60, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_objtab, 64, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_objcnt, 68, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_objmap, 72, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_itermap, 76, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_rsrctab, 80, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_rsrccnt, 84, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_restab, 88, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_enttab, 92, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_dirtab, 96, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_dircnt, 100, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_fpagetab, 104, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_frectab, 108, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_impmod, 112, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_impmodcnt, 116, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_impproc, 120, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_pagesum, 124, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_datapage, 128, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_preload, 132, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_nrestab, 136, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_cbnrestab, 140, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_nressum, 144, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_autodata, 148, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_debuginfo, 152, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_debuglen, 156, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_instpreload, 160, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_instdemand, 164, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_heapsize, 168, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_res3, 172, 12, 1);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_winresoff, 184, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_winreslen, 188, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_devid, 192, 2, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, e32_ddkver, 194, 2, 2);
}

static void test_pack_IO_COUNTERS(void)
{
    /* IO_COUNTERS (pack 8) */
    TEST_TYPE(IO_COUNTERS, 48, 8);
    TEST_FIELD(IO_COUNTERS, ReadOperationCount, 0, 8, 8);
    TEST_FIELD(IO_COUNTERS, WriteOperationCount, 8, 8, 8);
    TEST_FIELD(IO_COUNTERS, OtherOperationCount, 16, 8, 8);
    TEST_FIELD(IO_COUNTERS, ReadTransferCount, 24, 8, 8);
    TEST_FIELD(IO_COUNTERS, WriteTransferCount, 32, 8, 8);
    TEST_FIELD(IO_COUNTERS, OtherTransferCount, 40, 8, 8);
}

static void test_pack_LANGID(void)
{
    /* LANGID */
    TEST_TYPE(LANGID, 2, 2);
    TEST_TYPE_UNSIGNED(LANGID);
}

static void test_pack_LARGE_INTEGER(void)
{
    /* LARGE_INTEGER (pack 4) */
}

static void test_pack_LCID(void)
{
    /* LCID */
    TEST_TYPE(LCID, 4, 4);
    TEST_TYPE_UNSIGNED(LCID);
}

static void test_pack_LIST_ENTRY(void)
{
    /* LIST_ENTRY (pack 4) */
    TEST_TYPE(LIST_ENTRY, 8, 4);
    TEST_FIELD(LIST_ENTRY, Flink, 0, 4, 4);
    TEST_FIELD(LIST_ENTRY, Blink, 4, 4, 4);
}

static void test_pack_LONG(void)
{
    /* LONG */
    TEST_TYPE(LONG, 4, 4);
    TEST_TYPE_SIGNED(LONG);
}

static void test_pack_LONGLONG(void)
{
    /* LONGLONG */
    TEST_TYPE(LONGLONG, 8, 8);
    TEST_TYPE_SIGNED(LONGLONG);
}

static void test_pack_LUID(void)
{
    /* LUID (pack 4) */
    TEST_TYPE(LUID, 8, 4);
    TEST_FIELD(LUID, LowPart, 0, 4, 4);
    TEST_FIELD(LUID, HighPart, 4, 4, 4);
}

static void test_pack_LUID_AND_ATTRIBUTES(void)
{
    /* LUID_AND_ATTRIBUTES (pack 4) */
    TEST_TYPE(LUID_AND_ATTRIBUTES, 12, 4);
    TEST_FIELD(LUID_AND_ATTRIBUTES, Luid, 0, 8, 4);
    TEST_FIELD(LUID_AND_ATTRIBUTES, Attributes, 8, 4, 4);
}

static void test_pack_MEMORY_BASIC_INFORMATION(void)
{
    /* MEMORY_BASIC_INFORMATION (pack 4) */
    TEST_TYPE(MEMORY_BASIC_INFORMATION, 28, 4);
    TEST_FIELD(MEMORY_BASIC_INFORMATION, BaseAddress, 0, 4, 4);
    TEST_FIELD(MEMORY_BASIC_INFORMATION, AllocationBase, 4, 4, 4);
    TEST_FIELD(MEMORY_BASIC_INFORMATION, AllocationProtect, 8, 4, 4);
    TEST_FIELD(MEMORY_BASIC_INFORMATION, RegionSize, 12, 4, 4);
    TEST_FIELD(MEMORY_BASIC_INFORMATION, State, 16, 4, 4);
    TEST_FIELD(MEMORY_BASIC_INFORMATION, Protect, 20, 4, 4);
    TEST_FIELD(MEMORY_BASIC_INFORMATION, Type, 24, 4, 4);
}

static void test_pack_MESSAGE_RESOURCE_BLOCK(void)
{
    /* MESSAGE_RESOURCE_BLOCK (pack 4) */
    TEST_TYPE(MESSAGE_RESOURCE_BLOCK, 12, 4);
    TEST_FIELD(MESSAGE_RESOURCE_BLOCK, LowId, 0, 4, 4);
    TEST_FIELD(MESSAGE_RESOURCE_BLOCK, HighId, 4, 4, 4);
    TEST_FIELD(MESSAGE_RESOURCE_BLOCK, OffsetToEntries, 8, 4, 4);
}

static void test_pack_MESSAGE_RESOURCE_DATA(void)
{
    /* MESSAGE_RESOURCE_DATA (pack 4) */
    TEST_TYPE(MESSAGE_RESOURCE_DATA, 16, 4);
    TEST_FIELD(MESSAGE_RESOURCE_DATA, NumberOfBlocks, 0, 4, 4);
    TEST_FIELD(MESSAGE_RESOURCE_DATA, Blocks, 4, 12, 4);
}

static void test_pack_MESSAGE_RESOURCE_ENTRY(void)
{
    /* MESSAGE_RESOURCE_ENTRY (pack 4) */
    TEST_TYPE(MESSAGE_RESOURCE_ENTRY, 6, 2);
    TEST_FIELD(MESSAGE_RESOURCE_ENTRY, Length, 0, 2, 2);
    TEST_FIELD(MESSAGE_RESOURCE_ENTRY, Flags, 2, 2, 2);
    TEST_FIELD(MESSAGE_RESOURCE_ENTRY, Text, 4, 1, 1);
}

static void test_pack_NT_TIB(void)
{
    /* NT_TIB (pack 4) */
    TEST_FIELD(NT_TIB, ExceptionList, 0, 4, 4);
    TEST_FIELD(NT_TIB, StackBase, 4, 4, 4);
    TEST_FIELD(NT_TIB, StackLimit, 8, 4, 4);
    TEST_FIELD(NT_TIB, SubSystemTib, 12, 4, 4);
}

static void test_pack_OBJECT_TYPE_LIST(void)
{
    /* OBJECT_TYPE_LIST (pack 4) */
    TEST_TYPE(OBJECT_TYPE_LIST, 8, 4);
    TEST_FIELD(OBJECT_TYPE_LIST, Level, 0, 2, 2);
    TEST_FIELD(OBJECT_TYPE_LIST, Sbz, 2, 2, 2);
    TEST_FIELD(OBJECT_TYPE_LIST, ObjectType, 4, 4, 4);
}

static void test_pack_PACCESS_ALLOWED_ACE(void)
{
    /* PACCESS_ALLOWED_ACE */
    TEST_TYPE(PACCESS_ALLOWED_ACE, 4, 4);
    TEST_TYPE_POINTER(PACCESS_ALLOWED_ACE, 12, 4);
}

static void test_pack_PACCESS_DENIED_ACE(void)
{
    /* PACCESS_DENIED_ACE */
    TEST_TYPE(PACCESS_DENIED_ACE, 4, 4);
    TEST_TYPE_POINTER(PACCESS_DENIED_ACE, 12, 4);
}

static void test_pack_PACCESS_TOKEN(void)
{
    /* PACCESS_TOKEN */
    TEST_TYPE(PACCESS_TOKEN, 4, 4);
}

static void test_pack_PACE_HEADER(void)
{
    /* PACE_HEADER */
    TEST_TYPE(PACE_HEADER, 4, 4);
    TEST_TYPE_POINTER(PACE_HEADER, 4, 2);
}

static void test_pack_PACL(void)
{
    /* PACL */
    TEST_TYPE(PACL, 4, 4);
    TEST_TYPE_POINTER(PACL, 8, 2);
}

static void test_pack_PACL_REVISION_INFORMATION(void)
{
    /* PACL_REVISION_INFORMATION */
    TEST_TYPE(PACL_REVISION_INFORMATION, 4, 4);
    TEST_TYPE_POINTER(PACL_REVISION_INFORMATION, 4, 4);
}

static void test_pack_PACL_SIZE_INFORMATION(void)
{
    /* PACL_SIZE_INFORMATION */
    TEST_TYPE(PACL_SIZE_INFORMATION, 4, 4);
    TEST_TYPE_POINTER(PACL_SIZE_INFORMATION, 12, 4);
}

static void test_pack_PCCH(void)
{
    /* PCCH */
    TEST_TYPE(PCCH, 4, 4);
    TEST_TYPE_POINTER(PCCH, 1, 1);
}

static void test_pack_PCH(void)
{
    /* PCH */
    TEST_TYPE(PCH, 4, 4);
    TEST_TYPE_POINTER(PCH, 1, 1);
}

static void test_pack_PCSTR(void)
{
    /* PCSTR */
    TEST_TYPE(PCSTR, 4, 4);
    TEST_TYPE_POINTER(PCSTR, 1, 1);
}

static void test_pack_PCTSTR(void)
{
    /* PCTSTR */
    TEST_TYPE(PCTSTR, 4, 4);
}

static void test_pack_PCWCH(void)
{
    /* PCWCH */
    TEST_TYPE(PCWCH, 4, 4);
    TEST_TYPE_POINTER(PCWCH, 2, 2);
}

static void test_pack_PCWSTR(void)
{
    /* PCWSTR */
    TEST_TYPE(PCWSTR, 4, 4);
    TEST_TYPE_POINTER(PCWSTR, 2, 2);
}

static void test_pack_PEXCEPTION_POINTERS(void)
{
    /* PEXCEPTION_POINTERS */
    TEST_TYPE(PEXCEPTION_POINTERS, 4, 4);
    TEST_TYPE_POINTER(PEXCEPTION_POINTERS, 8, 4);
}

static void test_pack_PEXCEPTION_RECORD(void)
{
    /* PEXCEPTION_RECORD */
    TEST_TYPE(PEXCEPTION_RECORD, 4, 4);
    TEST_TYPE_POINTER(PEXCEPTION_RECORD, 80, 4);
}

static void test_pack_PFLOATING_SAVE_AREA(void)
{
    /* PFLOATING_SAVE_AREA */
    TEST_TYPE(PFLOATING_SAVE_AREA, 4, 4);
    TEST_TYPE_POINTER(PFLOATING_SAVE_AREA, 112, 4);
}

static void test_pack_PFPO_DATA(void)
{
    /* PFPO_DATA */
    TEST_TYPE(PFPO_DATA, 4, 4);
    TEST_TYPE_POINTER(PFPO_DATA, 16, 4);
}

static void test_pack_PGENERIC_MAPPING(void)
{
    /* PGENERIC_MAPPING */
    TEST_TYPE(PGENERIC_MAPPING, 4, 4);
    TEST_TYPE_POINTER(PGENERIC_MAPPING, 16, 4);
}

static void test_pack_PHANDLE(void)
{
    /* PHANDLE */
    TEST_TYPE(PHANDLE, 4, 4);
    TEST_TYPE_POINTER(PHANDLE, 4, 4);
}

static void test_pack_PIMAGE_ARCHIVE_MEMBER_HEADER(void)
{
    /* PIMAGE_ARCHIVE_MEMBER_HEADER */
    TEST_TYPE(PIMAGE_ARCHIVE_MEMBER_HEADER, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_ARCHIVE_MEMBER_HEADER, 60, 1);
}

static void test_pack_PIMAGE_AUX_SYMBOL(void)
{
    /* PIMAGE_AUX_SYMBOL */
    TEST_TYPE(PIMAGE_AUX_SYMBOL, 4, 4);
}

static void test_pack_PIMAGE_BASE_RELOCATION(void)
{
    /* PIMAGE_BASE_RELOCATION */
    TEST_TYPE(PIMAGE_BASE_RELOCATION, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_BASE_RELOCATION, 8, 4);
}

static void test_pack_PIMAGE_BOUND_FORWARDER_REF(void)
{
    /* PIMAGE_BOUND_FORWARDER_REF */
    TEST_TYPE(PIMAGE_BOUND_FORWARDER_REF, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_BOUND_FORWARDER_REF, 8, 4);
}

static void test_pack_PIMAGE_BOUND_IMPORT_DESCRIPTOR(void)
{
    /* PIMAGE_BOUND_IMPORT_DESCRIPTOR */
    TEST_TYPE(PIMAGE_BOUND_IMPORT_DESCRIPTOR, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_BOUND_IMPORT_DESCRIPTOR, 8, 4);
}

static void test_pack_PIMAGE_COFF_SYMBOLS_HEADER(void)
{
    /* PIMAGE_COFF_SYMBOLS_HEADER */
    TEST_TYPE(PIMAGE_COFF_SYMBOLS_HEADER, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_COFF_SYMBOLS_HEADER, 32, 4);
}

static void test_pack_PIMAGE_DATA_DIRECTORY(void)
{
    /* PIMAGE_DATA_DIRECTORY */
    TEST_TYPE(PIMAGE_DATA_DIRECTORY, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_DATA_DIRECTORY, 8, 4);
}

static void test_pack_PIMAGE_DEBUG_DIRECTORY(void)
{
    /* PIMAGE_DEBUG_DIRECTORY */
    TEST_TYPE(PIMAGE_DEBUG_DIRECTORY, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_DEBUG_DIRECTORY, 28, 4);
}

static void test_pack_PIMAGE_DEBUG_MISC(void)
{
    /* PIMAGE_DEBUG_MISC */
    TEST_TYPE(PIMAGE_DEBUG_MISC, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_DEBUG_MISC, 16, 4);
}

static void test_pack_PIMAGE_DOS_HEADER(void)
{
    /* PIMAGE_DOS_HEADER */
    TEST_TYPE(PIMAGE_DOS_HEADER, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_DOS_HEADER, 64, 2);
}

static void test_pack_PIMAGE_EXPORT_DIRECTORY(void)
{
    /* PIMAGE_EXPORT_DIRECTORY */
    TEST_TYPE(PIMAGE_EXPORT_DIRECTORY, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_EXPORT_DIRECTORY, 40, 4);
}

static void test_pack_PIMAGE_FILE_HEADER(void)
{
    /* PIMAGE_FILE_HEADER */
    TEST_TYPE(PIMAGE_FILE_HEADER, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_FILE_HEADER, 20, 4);
}

static void test_pack_PIMAGE_FUNCTION_ENTRY(void)
{
    /* PIMAGE_FUNCTION_ENTRY */
    TEST_TYPE(PIMAGE_FUNCTION_ENTRY, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_FUNCTION_ENTRY, 12, 4);
}

static void test_pack_PIMAGE_IMPORT_BY_NAME(void)
{
    /* PIMAGE_IMPORT_BY_NAME */
    TEST_TYPE(PIMAGE_IMPORT_BY_NAME, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_IMPORT_BY_NAME, 4, 2);
}

static void test_pack_PIMAGE_IMPORT_DESCRIPTOR(void)
{
    /* PIMAGE_IMPORT_DESCRIPTOR */
    TEST_TYPE(PIMAGE_IMPORT_DESCRIPTOR, 4, 4);
}

static void test_pack_PIMAGE_LINENUMBER(void)
{
    /* PIMAGE_LINENUMBER */
    TEST_TYPE(PIMAGE_LINENUMBER, 4, 4);
}

static void test_pack_PIMAGE_LOAD_CONFIG_DIRECTORY(void)
{
    /* PIMAGE_LOAD_CONFIG_DIRECTORY */
    TEST_TYPE(PIMAGE_LOAD_CONFIG_DIRECTORY, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_LOAD_CONFIG_DIRECTORY, 72, 4);
}

static void test_pack_PIMAGE_NT_HEADERS(void)
{
    /* PIMAGE_NT_HEADERS */
    TEST_TYPE(PIMAGE_NT_HEADERS, 4, 4);
}

static void test_pack_PIMAGE_OPTIONAL_HEADER(void)
{
    /* PIMAGE_OPTIONAL_HEADER */
    TEST_TYPE(PIMAGE_OPTIONAL_HEADER, 4, 4);
}

static void test_pack_PIMAGE_OS2_HEADER(void)
{
    /* PIMAGE_OS2_HEADER */
    TEST_TYPE(PIMAGE_OS2_HEADER, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_OS2_HEADER, 64, 2);
}

static void test_pack_PIMAGE_RELOCATION(void)
{
    /* PIMAGE_RELOCATION */
    TEST_TYPE(PIMAGE_RELOCATION, 4, 4);
}

static void test_pack_PIMAGE_RESOURCE_DATA_ENTRY(void)
{
    /* PIMAGE_RESOURCE_DATA_ENTRY */
    TEST_TYPE(PIMAGE_RESOURCE_DATA_ENTRY, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_RESOURCE_DATA_ENTRY, 16, 4);
}

static void test_pack_PIMAGE_RESOURCE_DIRECTORY(void)
{
    /* PIMAGE_RESOURCE_DIRECTORY */
    TEST_TYPE(PIMAGE_RESOURCE_DIRECTORY, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_RESOURCE_DIRECTORY, 16, 4);
}

static void test_pack_PIMAGE_RESOURCE_DIRECTORY_ENTRY(void)
{
    /* PIMAGE_RESOURCE_DIRECTORY_ENTRY */
    TEST_TYPE(PIMAGE_RESOURCE_DIRECTORY_ENTRY, 4, 4);
}

static void test_pack_PIMAGE_RESOURCE_DIRECTORY_STRING(void)
{
    /* PIMAGE_RESOURCE_DIRECTORY_STRING */
    TEST_TYPE(PIMAGE_RESOURCE_DIRECTORY_STRING, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_RESOURCE_DIRECTORY_STRING, 4, 2);
}

static void test_pack_PIMAGE_RESOURCE_DIR_STRING_U(void)
{
    /* PIMAGE_RESOURCE_DIR_STRING_U */
    TEST_TYPE(PIMAGE_RESOURCE_DIR_STRING_U, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_RESOURCE_DIR_STRING_U, 4, 2);
}

static void test_pack_PIMAGE_SECTION_HEADER(void)
{
    /* PIMAGE_SECTION_HEADER */
    TEST_TYPE(PIMAGE_SECTION_HEADER, 4, 4);
}

static void test_pack_PIMAGE_SEPARATE_DEBUG_HEADER(void)
{
    /* PIMAGE_SEPARATE_DEBUG_HEADER */
    TEST_TYPE(PIMAGE_SEPARATE_DEBUG_HEADER, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_SEPARATE_DEBUG_HEADER, 48, 4);
}

static void test_pack_PIMAGE_SYMBOL(void)
{
    /* PIMAGE_SYMBOL */
    TEST_TYPE(PIMAGE_SYMBOL, 4, 4);
}

static void test_pack_PIMAGE_THUNK_DATA(void)
{
    /* PIMAGE_THUNK_DATA */
    TEST_TYPE(PIMAGE_THUNK_DATA, 4, 4);
}

static void test_pack_PIMAGE_TLS_CALLBACK(void)
{
    /* PIMAGE_TLS_CALLBACK */
    TEST_TYPE(PIMAGE_TLS_CALLBACK, 4, 4);
}

static void test_pack_PIMAGE_TLS_DIRECTORY(void)
{
    /* PIMAGE_TLS_DIRECTORY */
    TEST_TYPE(PIMAGE_TLS_DIRECTORY, 4, 4);
}

static void test_pack_PIMAGE_VXD_HEADER(void)
{
    /* PIMAGE_VXD_HEADER */
    TEST_TYPE(PIMAGE_VXD_HEADER, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_VXD_HEADER, 196, 2);
}

static void test_pack_PIO_COUNTERS(void)
{
    /* PIO_COUNTERS */
    TEST_TYPE(PIO_COUNTERS, 4, 4);
    TEST_TYPE_POINTER(PIO_COUNTERS, 48, 8);
}

static void test_pack_PISECURITY_DESCRIPTOR(void)
{
    /* PISECURITY_DESCRIPTOR */
    TEST_TYPE(PISECURITY_DESCRIPTOR, 4, 4);
    TEST_TYPE_POINTER(PISECURITY_DESCRIPTOR, 20, 4);
}

static void test_pack_PISECURITY_DESCRIPTOR_RELATIVE(void)
{
    /* PISECURITY_DESCRIPTOR_RELATIVE */
    TEST_TYPE(PISECURITY_DESCRIPTOR_RELATIVE, 4, 4);
    TEST_TYPE_POINTER(PISECURITY_DESCRIPTOR_RELATIVE, 20, 4);
}

static void test_pack_PISID(void)
{
    /* PISID */
    TEST_TYPE(PISID, 4, 4);
    TEST_TYPE_POINTER(PISID, 12, 4);
}

static void test_pack_PLARGE_INTEGER(void)
{
    /* PLARGE_INTEGER */
    TEST_TYPE(PLARGE_INTEGER, 4, 4);
}

static void test_pack_PLIST_ENTRY(void)
{
    /* PLIST_ENTRY */
    TEST_TYPE(PLIST_ENTRY, 4, 4);
    TEST_TYPE_POINTER(PLIST_ENTRY, 8, 4);
}

static void test_pack_PLUID(void)
{
    /* PLUID */
    TEST_TYPE(PLUID, 4, 4);
    TEST_TYPE_POINTER(PLUID, 8, 4);
}

static void test_pack_PLUID_AND_ATTRIBUTES(void)
{
    /* PLUID_AND_ATTRIBUTES */
    TEST_TYPE(PLUID_AND_ATTRIBUTES, 4, 4);
    TEST_TYPE_POINTER(PLUID_AND_ATTRIBUTES, 12, 4);
}

static void test_pack_PMEMORY_BASIC_INFORMATION(void)
{
    /* PMEMORY_BASIC_INFORMATION */
    TEST_TYPE(PMEMORY_BASIC_INFORMATION, 4, 4);
    TEST_TYPE_POINTER(PMEMORY_BASIC_INFORMATION, 28, 4);
}

static void test_pack_PMESSAGE_RESOURCE_BLOCK(void)
{
    /* PMESSAGE_RESOURCE_BLOCK */
    TEST_TYPE(PMESSAGE_RESOURCE_BLOCK, 4, 4);
    TEST_TYPE_POINTER(PMESSAGE_RESOURCE_BLOCK, 12, 4);
}

static void test_pack_PMESSAGE_RESOURCE_DATA(void)
{
    /* PMESSAGE_RESOURCE_DATA */
    TEST_TYPE(PMESSAGE_RESOURCE_DATA, 4, 4);
    TEST_TYPE_POINTER(PMESSAGE_RESOURCE_DATA, 16, 4);
}

static void test_pack_PMESSAGE_RESOURCE_ENTRY(void)
{
    /* PMESSAGE_RESOURCE_ENTRY */
    TEST_TYPE(PMESSAGE_RESOURCE_ENTRY, 4, 4);
    TEST_TYPE_POINTER(PMESSAGE_RESOURCE_ENTRY, 6, 2);
}

static void test_pack_PNT_TIB(void)
{
    /* PNT_TIB */
    TEST_TYPE(PNT_TIB, 4, 4);
}

static void test_pack_POBJECT_TYPE_LIST(void)
{
    /* POBJECT_TYPE_LIST */
    TEST_TYPE(POBJECT_TYPE_LIST, 4, 4);
    TEST_TYPE_POINTER(POBJECT_TYPE_LIST, 8, 4);
}

static void test_pack_PPRIVILEGE_SET(void)
{
    /* PPRIVILEGE_SET */
    TEST_TYPE(PPRIVILEGE_SET, 4, 4);
    TEST_TYPE_POINTER(PPRIVILEGE_SET, 20, 4);
}

static void test_pack_PRIVILEGE_SET(void)
{
    /* PRIVILEGE_SET (pack 4) */
    TEST_TYPE(PRIVILEGE_SET, 20, 4);
    TEST_FIELD(PRIVILEGE_SET, PrivilegeCount, 0, 4, 4);
    TEST_FIELD(PRIVILEGE_SET, Control, 4, 4, 4);
    TEST_FIELD(PRIVILEGE_SET, Privilege, 8, 12, 4);
}

static void test_pack_PRLIST_ENTRY(void)
{
    /* PRLIST_ENTRY */
    TEST_TYPE(PRLIST_ENTRY, 4, 4);
    TEST_TYPE_POINTER(PRLIST_ENTRY, 8, 4);
}

static void test_pack_PRTL_CRITICAL_SECTION(void)
{
    /* PRTL_CRITICAL_SECTION */
    TEST_TYPE(PRTL_CRITICAL_SECTION, 4, 4);
    TEST_TYPE_POINTER(PRTL_CRITICAL_SECTION, 24, 4);
}

static void test_pack_PRTL_CRITICAL_SECTION_DEBUG(void)
{
    /* PRTL_CRITICAL_SECTION_DEBUG */
    TEST_TYPE(PRTL_CRITICAL_SECTION_DEBUG, 4, 4);
}

static void test_pack_PRTL_OSVERSIONINFOEXW(void)
{
    /* PRTL_OSVERSIONINFOEXW */
    TEST_TYPE(PRTL_OSVERSIONINFOEXW, 4, 4);
    TEST_TYPE_POINTER(PRTL_OSVERSIONINFOEXW, 284, 4);
}

static void test_pack_PRTL_OSVERSIONINFOW(void)
{
    /* PRTL_OSVERSIONINFOW */
    TEST_TYPE(PRTL_OSVERSIONINFOW, 4, 4);
    TEST_TYPE_POINTER(PRTL_OSVERSIONINFOW, 276, 4);
}

static void test_pack_PRTL_RESOURCE_DEBUG(void)
{
    /* PRTL_RESOURCE_DEBUG */
    TEST_TYPE(PRTL_RESOURCE_DEBUG, 4, 4);
}

static void test_pack_PSECURITY_DESCRIPTOR(void)
{
    /* PSECURITY_DESCRIPTOR */
    TEST_TYPE(PSECURITY_DESCRIPTOR, 4, 4);
}

static void test_pack_PSECURITY_QUALITY_OF_SERVICE(void)
{
    /* PSECURITY_QUALITY_OF_SERVICE */
    TEST_TYPE(PSECURITY_QUALITY_OF_SERVICE, 4, 4);
}

static void test_pack_PSID(void)
{
    /* PSID */
    TEST_TYPE(PSID, 4, 4);
}

static void test_pack_PSID_IDENTIFIER_AUTHORITY(void)
{
    /* PSID_IDENTIFIER_AUTHORITY */
    TEST_TYPE(PSID_IDENTIFIER_AUTHORITY, 4, 4);
    TEST_TYPE_POINTER(PSID_IDENTIFIER_AUTHORITY, 6, 1);
}

static void test_pack_PSINGLE_LIST_ENTRY(void)
{
    /* PSINGLE_LIST_ENTRY */
    TEST_TYPE(PSINGLE_LIST_ENTRY, 4, 4);
    TEST_TYPE_POINTER(PSINGLE_LIST_ENTRY, 4, 4);
}

static void test_pack_PSTR(void)
{
    /* PSTR */
    TEST_TYPE(PSTR, 4, 4);
    TEST_TYPE_POINTER(PSTR, 1, 1);
}

static void test_pack_PSYSTEM_ALARM_ACE(void)
{
    /* PSYSTEM_ALARM_ACE */
    TEST_TYPE(PSYSTEM_ALARM_ACE, 4, 4);
    TEST_TYPE_POINTER(PSYSTEM_ALARM_ACE, 12, 4);
}

static void test_pack_PSYSTEM_AUDIT_ACE(void)
{
    /* PSYSTEM_AUDIT_ACE */
    TEST_TYPE(PSYSTEM_AUDIT_ACE, 4, 4);
    TEST_TYPE_POINTER(PSYSTEM_AUDIT_ACE, 12, 4);
}

static void test_pack_PTOKEN_GROUPS(void)
{
    /* PTOKEN_GROUPS */
    TEST_TYPE(PTOKEN_GROUPS, 4, 4);
    TEST_TYPE_POINTER(PTOKEN_GROUPS, 12, 4);
}

static void test_pack_PTOKEN_PRIVILEGES(void)
{
    /* PTOKEN_PRIVILEGES */
    TEST_TYPE(PTOKEN_PRIVILEGES, 4, 4);
    TEST_TYPE_POINTER(PTOKEN_PRIVILEGES, 16, 4);
}

static void test_pack_PTOKEN_USER(void)
{
    /* PTOKEN_USER */
    TEST_TYPE(PTOKEN_USER, 4, 4);
    TEST_TYPE_POINTER(PTOKEN_USER, 8, 4);
}

static void test_pack_PTSTR(void)
{
    /* PTSTR */
    TEST_TYPE(PTSTR, 4, 4);
}

static void test_pack_PULARGE_INTEGER(void)
{
    /* PULARGE_INTEGER */
    TEST_TYPE(PULARGE_INTEGER, 4, 4);
}

static void test_pack_PVECTORED_EXCEPTION_HANDLER(void)
{
    /* PVECTORED_EXCEPTION_HANDLER */
    TEST_TYPE(PVECTORED_EXCEPTION_HANDLER, 4, 4);
}

static void test_pack_PVOID(void)
{
    /* PVOID */
    TEST_TYPE(PVOID, 4, 4);
}

static void test_pack_PWCH(void)
{
    /* PWCH */
    TEST_TYPE(PWCH, 4, 4);
    TEST_TYPE_POINTER(PWCH, 2, 2);
}

static void test_pack_PWSTR(void)
{
    /* PWSTR */
    TEST_TYPE(PWSTR, 4, 4);
    TEST_TYPE_POINTER(PWSTR, 2, 2);
}

static void test_pack_RTL_CRITICAL_SECTION(void)
{
    /* RTL_CRITICAL_SECTION (pack 4) */
    TEST_TYPE(RTL_CRITICAL_SECTION, 24, 4);
    TEST_FIELD(RTL_CRITICAL_SECTION, DebugInfo, 0, 4, 4);
    TEST_FIELD(RTL_CRITICAL_SECTION, LockCount, 4, 4, 4);
    TEST_FIELD(RTL_CRITICAL_SECTION, RecursionCount, 8, 4, 4);
    TEST_FIELD(RTL_CRITICAL_SECTION, OwningThread, 12, 4, 4);
    TEST_FIELD(RTL_CRITICAL_SECTION, LockSemaphore, 16, 4, 4);
    TEST_FIELD(RTL_CRITICAL_SECTION, SpinCount, 20, 4, 4);
}

static void test_pack_RTL_CRITICAL_SECTION_DEBUG(void)
{
    /* RTL_CRITICAL_SECTION_DEBUG (pack 4) */
    TEST_FIELD(RTL_CRITICAL_SECTION_DEBUG, Type, 0, 2, 2);
    TEST_FIELD(RTL_CRITICAL_SECTION_DEBUG, CreatorBackTraceIndex, 2, 2, 2);
    TEST_FIELD(RTL_CRITICAL_SECTION_DEBUG, CriticalSection, 4, 4, 4);
    TEST_FIELD(RTL_CRITICAL_SECTION_DEBUG, ProcessLocksList, 8, 8, 4);
    TEST_FIELD(RTL_CRITICAL_SECTION_DEBUG, EntryCount, 16, 4, 4);
    TEST_FIELD(RTL_CRITICAL_SECTION_DEBUG, ContentionCount, 20, 4, 4);
}

static void test_pack_RTL_OSVERSIONINFOEXW(void)
{
    /* RTL_OSVERSIONINFOEXW (pack 4) */
    TEST_TYPE(RTL_OSVERSIONINFOEXW, 284, 4);
    TEST_FIELD(RTL_OSVERSIONINFOEXW, dwOSVersionInfoSize, 0, 4, 4);
    TEST_FIELD(RTL_OSVERSIONINFOEXW, dwMajorVersion, 4, 4, 4);
    TEST_FIELD(RTL_OSVERSIONINFOEXW, dwMinorVersion, 8, 4, 4);
    TEST_FIELD(RTL_OSVERSIONINFOEXW, dwBuildNumber, 12, 4, 4);
    TEST_FIELD(RTL_OSVERSIONINFOEXW, dwPlatformId, 16, 4, 4);
    TEST_FIELD(RTL_OSVERSIONINFOEXW, szCSDVersion, 20, 256, 2);
    TEST_FIELD(RTL_OSVERSIONINFOEXW, wServicePackMajor, 276, 2, 2);
    TEST_FIELD(RTL_OSVERSIONINFOEXW, wServicePackMinor, 278, 2, 2);
    TEST_FIELD(RTL_OSVERSIONINFOEXW, wSuiteMask, 280, 2, 2);
    TEST_FIELD(RTL_OSVERSIONINFOEXW, wProductType, 282, 1, 1);
    TEST_FIELD(RTL_OSVERSIONINFOEXW, wReserved, 283, 1, 1);
}

static void test_pack_RTL_OSVERSIONINFOW(void)
{
    /* RTL_OSVERSIONINFOW (pack 4) */
    TEST_TYPE(RTL_OSVERSIONINFOW, 276, 4);
    TEST_FIELD(RTL_OSVERSIONINFOW, dwOSVersionInfoSize, 0, 4, 4);
    TEST_FIELD(RTL_OSVERSIONINFOW, dwMajorVersion, 4, 4, 4);
    TEST_FIELD(RTL_OSVERSIONINFOW, dwMinorVersion, 8, 4, 4);
    TEST_FIELD(RTL_OSVERSIONINFOW, dwBuildNumber, 12, 4, 4);
    TEST_FIELD(RTL_OSVERSIONINFOW, dwPlatformId, 16, 4, 4);
    TEST_FIELD(RTL_OSVERSIONINFOW, szCSDVersion, 20, 256, 2);
}

static void test_pack_RTL_RESOURCE_DEBUG(void)
{
    /* RTL_RESOURCE_DEBUG (pack 4) */
    TEST_FIELD(RTL_RESOURCE_DEBUG, Type, 0, 2, 2);
    TEST_FIELD(RTL_RESOURCE_DEBUG, CreatorBackTraceIndex, 2, 2, 2);
    TEST_FIELD(RTL_RESOURCE_DEBUG, CriticalSection, 4, 4, 4);
    TEST_FIELD(RTL_RESOURCE_DEBUG, ProcessLocksList, 8, 8, 4);
    TEST_FIELD(RTL_RESOURCE_DEBUG, EntryCount, 16, 4, 4);
    TEST_FIELD(RTL_RESOURCE_DEBUG, ContentionCount, 20, 4, 4);
}

static void test_pack_SECURITY_CONTEXT_TRACKING_MODE(void)
{
    /* SECURITY_CONTEXT_TRACKING_MODE */
    TEST_TYPE(SECURITY_CONTEXT_TRACKING_MODE, 1, 1);
}

static void test_pack_SECURITY_DESCRIPTOR(void)
{
    /* SECURITY_DESCRIPTOR (pack 4) */
    TEST_TYPE(SECURITY_DESCRIPTOR, 20, 4);
    TEST_FIELD(SECURITY_DESCRIPTOR, Revision, 0, 1, 1);
    TEST_FIELD(SECURITY_DESCRIPTOR, Sbz1, 1, 1, 1);
    TEST_FIELD(SECURITY_DESCRIPTOR, Control, 2, 2, 2);
    TEST_FIELD(SECURITY_DESCRIPTOR, Owner, 4, 4, 4);
    TEST_FIELD(SECURITY_DESCRIPTOR, Group, 8, 4, 4);
    TEST_FIELD(SECURITY_DESCRIPTOR, Sacl, 12, 4, 4);
    TEST_FIELD(SECURITY_DESCRIPTOR, Dacl, 16, 4, 4);
}

static void test_pack_SECURITY_DESCRIPTOR_CONTROL(void)
{
    /* SECURITY_DESCRIPTOR_CONTROL */
    TEST_TYPE(SECURITY_DESCRIPTOR_CONTROL, 2, 2);
    TEST_TYPE_UNSIGNED(SECURITY_DESCRIPTOR_CONTROL);
}

static void test_pack_SECURITY_DESCRIPTOR_RELATIVE(void)
{
    /* SECURITY_DESCRIPTOR_RELATIVE (pack 4) */
    TEST_TYPE(SECURITY_DESCRIPTOR_RELATIVE, 20, 4);
    TEST_FIELD(SECURITY_DESCRIPTOR_RELATIVE, Revision, 0, 1, 1);
    TEST_FIELD(SECURITY_DESCRIPTOR_RELATIVE, Sbz1, 1, 1, 1);
    TEST_FIELD(SECURITY_DESCRIPTOR_RELATIVE, Control, 2, 2, 2);
    TEST_FIELD(SECURITY_DESCRIPTOR_RELATIVE, Owner, 4, 4, 4);
    TEST_FIELD(SECURITY_DESCRIPTOR_RELATIVE, Group, 8, 4, 4);
    TEST_FIELD(SECURITY_DESCRIPTOR_RELATIVE, Sacl, 12, 4, 4);
    TEST_FIELD(SECURITY_DESCRIPTOR_RELATIVE, Dacl, 16, 4, 4);
}

static void test_pack_SECURITY_INFORMATION(void)
{
    /* SECURITY_INFORMATION */
    TEST_TYPE(SECURITY_INFORMATION, 4, 4);
    TEST_TYPE_UNSIGNED(SECURITY_INFORMATION);
}

static void test_pack_SECURITY_QUALITY_OF_SERVICE(void)
{
    /* SECURITY_QUALITY_OF_SERVICE (pack 4) */
    TEST_FIELD(SECURITY_QUALITY_OF_SERVICE, Length, 0, 4, 4);
}

static void test_pack_SHORT(void)
{
    /* SHORT */
    TEST_TYPE(SHORT, 2, 2);
    TEST_TYPE_SIGNED(SHORT);
}

static void test_pack_SID(void)
{
    /* SID (pack 4) */
    TEST_TYPE(SID, 12, 4);
    TEST_FIELD(SID, Revision, 0, 1, 1);
    TEST_FIELD(SID, SubAuthorityCount, 1, 1, 1);
    TEST_FIELD(SID, IdentifierAuthority, 2, 6, 1);
    TEST_FIELD(SID, SubAuthority, 8, 4, 4);
}

static void test_pack_SID_AND_ATTRIBUTES(void)
{
    /* SID_AND_ATTRIBUTES (pack 4) */
    TEST_TYPE(SID_AND_ATTRIBUTES, 8, 4);
    TEST_FIELD(SID_AND_ATTRIBUTES, Sid, 0, 4, 4);
    TEST_FIELD(SID_AND_ATTRIBUTES, Attributes, 4, 4, 4);
}

static void test_pack_SID_IDENTIFIER_AUTHORITY(void)
{
    /* SID_IDENTIFIER_AUTHORITY (pack 4) */
    TEST_TYPE(SID_IDENTIFIER_AUTHORITY, 6, 1);
    TEST_FIELD(SID_IDENTIFIER_AUTHORITY, Value, 0, 6, 1);
}

static void test_pack_SINGLE_LIST_ENTRY(void)
{
    /* SINGLE_LIST_ENTRY (pack 4) */
    TEST_TYPE(SINGLE_LIST_ENTRY, 4, 4);
    TEST_FIELD(SINGLE_LIST_ENTRY, Next, 0, 4, 4);
}

static void test_pack_SYSTEM_ALARM_ACE(void)
{
    /* SYSTEM_ALARM_ACE (pack 4) */
    TEST_TYPE(SYSTEM_ALARM_ACE, 12, 4);
    TEST_FIELD(SYSTEM_ALARM_ACE, Header, 0, 4, 2);
    TEST_FIELD(SYSTEM_ALARM_ACE, Mask, 4, 4, 4);
    TEST_FIELD(SYSTEM_ALARM_ACE, SidStart, 8, 4, 4);
}

static void test_pack_SYSTEM_AUDIT_ACE(void)
{
    /* SYSTEM_AUDIT_ACE (pack 4) */
    TEST_TYPE(SYSTEM_AUDIT_ACE, 12, 4);
    TEST_FIELD(SYSTEM_AUDIT_ACE, Header, 0, 4, 2);
    TEST_FIELD(SYSTEM_AUDIT_ACE, Mask, 4, 4, 4);
    TEST_FIELD(SYSTEM_AUDIT_ACE, SidStart, 8, 4, 4);
}

static void test_pack_TCHAR(void)
{
    /* TCHAR */
    TEST_TYPE(TCHAR, 1, 1);
}

static void test_pack_TOKEN_DEFAULT_DACL(void)
{
    /* TOKEN_DEFAULT_DACL (pack 4) */
    TEST_TYPE(TOKEN_DEFAULT_DACL, 4, 4);
    TEST_FIELD(TOKEN_DEFAULT_DACL, DefaultDacl, 0, 4, 4);
}

static void test_pack_TOKEN_GROUPS(void)
{
    /* TOKEN_GROUPS (pack 4) */
    TEST_TYPE(TOKEN_GROUPS, 12, 4);
    TEST_FIELD(TOKEN_GROUPS, GroupCount, 0, 4, 4);
    TEST_FIELD(TOKEN_GROUPS, Groups, 4, 8, 4);
}

static void test_pack_TOKEN_OWNER(void)
{
    /* TOKEN_OWNER (pack 4) */
    TEST_TYPE(TOKEN_OWNER, 4, 4);
    TEST_FIELD(TOKEN_OWNER, Owner, 0, 4, 4);
}

static void test_pack_TOKEN_PRIMARY_GROUP(void)
{
    /* TOKEN_PRIMARY_GROUP (pack 4) */
    TEST_TYPE(TOKEN_PRIMARY_GROUP, 4, 4);
    TEST_FIELD(TOKEN_PRIMARY_GROUP, PrimaryGroup, 0, 4, 4);
}

static void test_pack_TOKEN_PRIVILEGES(void)
{
    /* TOKEN_PRIVILEGES (pack 4) */
    TEST_TYPE(TOKEN_PRIVILEGES, 16, 4);
    TEST_FIELD(TOKEN_PRIVILEGES, PrivilegeCount, 0, 4, 4);
    TEST_FIELD(TOKEN_PRIVILEGES, Privileges, 4, 12, 4);
}

static void test_pack_TOKEN_SOURCE(void)
{
    /* TOKEN_SOURCE (pack 4) */
    TEST_TYPE(TOKEN_SOURCE, 16, 4);
    TEST_FIELD(TOKEN_SOURCE, SourceName, 0, 8, 1);
    TEST_FIELD(TOKEN_SOURCE, SourceIdentifier, 8, 8, 4);
}

static void test_pack_TOKEN_STATISTICS(void)
{
    /* TOKEN_STATISTICS (pack 4) */
    TEST_FIELD(TOKEN_STATISTICS, TokenId, 0, 8, 4);
    TEST_FIELD(TOKEN_STATISTICS, AuthenticationId, 8, 8, 4);
    TEST_FIELD(TOKEN_STATISTICS, ExpirationTime, 16, 8, 4);
}

static void test_pack_TOKEN_USER(void)
{
    /* TOKEN_USER (pack 4) */
    TEST_TYPE(TOKEN_USER, 8, 4);
    TEST_FIELD(TOKEN_USER, User, 0, 8, 4);
}

static void test_pack_ULARGE_INTEGER(void)
{
    /* ULARGE_INTEGER (pack 4) */
}

static void test_pack_ULONGLONG(void)
{
    /* ULONGLONG */
    TEST_TYPE(ULONGLONG, 8, 8);
    TEST_TYPE_UNSIGNED(ULONGLONG);
}

static void test_pack_WAITORTIMERCALLBACKFUNC(void)
{
    /* WAITORTIMERCALLBACKFUNC */
    TEST_TYPE(WAITORTIMERCALLBACKFUNC, 4, 4);
}

static void test_pack_WCHAR(void)
{
    /* WCHAR */
    TEST_TYPE(WCHAR, 2, 2);
    TEST_TYPE_UNSIGNED(WCHAR);
}

static void test_pack_ATOM(void)
{
    /* ATOM */
    TEST_TYPE(ATOM, 2, 2);
    TEST_TYPE_UNSIGNED(ATOM);
}

static void test_pack_BOOL(void)
{
    /* BOOL */
    TEST_TYPE(BOOL, 4, 4);
    TEST_TYPE_SIGNED(BOOL);
}

static void test_pack_BYTE(void)
{
    /* BYTE */
    TEST_TYPE(BYTE, 1, 1);
    TEST_TYPE_UNSIGNED(BYTE);
}

static void test_pack_COLORREF(void)
{
    /* COLORREF */
    TEST_TYPE(COLORREF, 4, 4);
    TEST_TYPE_UNSIGNED(COLORREF);
}

static void test_pack_DWORD(void)
{
    /* DWORD */
    TEST_TYPE(DWORD, 4, 4);
    TEST_TYPE_UNSIGNED(DWORD);
}

static void test_pack_FARPROC(void)
{
    /* FARPROC */
    TEST_TYPE(FARPROC, 4, 4);
}

static void test_pack_FLOAT(void)
{
    /* FLOAT */
    TEST_TYPE(FLOAT, 4, 4);
}

static void test_pack_GLOBALHANDLE(void)
{
    /* GLOBALHANDLE */
    TEST_TYPE(GLOBALHANDLE, 4, 4);
}

static void test_pack_HCURSOR(void)
{
    /* HCURSOR */
    TEST_TYPE(HCURSOR, 4, 4);
    TEST_TYPE_UNSIGNED(HCURSOR);
}

static void test_pack_HFILE(void)
{
    /* HFILE */
    TEST_TYPE(HFILE, 4, 4);
    TEST_TYPE_SIGNED(HFILE);
}

static void test_pack_HGDIOBJ(void)
{
    /* HGDIOBJ */
    TEST_TYPE(HGDIOBJ, 4, 4);
}

static void test_pack_HGLOBAL(void)
{
    /* HGLOBAL */
    TEST_TYPE(HGLOBAL, 4, 4);
}

static void test_pack_HLOCAL(void)
{
    /* HLOCAL */
    TEST_TYPE(HLOCAL, 4, 4);
}

static void test_pack_HMODULE(void)
{
    /* HMODULE */
    TEST_TYPE(HMODULE, 4, 4);
    TEST_TYPE_UNSIGNED(HMODULE);
}

static void test_pack_INT(void)
{
    /* INT */
    TEST_TYPE(INT, 4, 4);
    TEST_TYPE_SIGNED(INT);
}

static void test_pack_LOCALHANDLE(void)
{
    /* LOCALHANDLE */
    TEST_TYPE(LOCALHANDLE, 4, 4);
}

static void test_pack_LPARAM(void)
{
    /* LPARAM */
    TEST_TYPE(LPARAM, 4, 4);
}

static void test_pack_LPCRECT(void)
{
    /* LPCRECT */
    TEST_TYPE(LPCRECT, 4, 4);
    TEST_TYPE_POINTER(LPCRECT, 16, 4);
}

static void test_pack_LPCRECTL(void)
{
    /* LPCRECTL */
    TEST_TYPE(LPCRECTL, 4, 4);
    TEST_TYPE_POINTER(LPCRECTL, 16, 4);
}

static void test_pack_LPCVOID(void)
{
    /* LPCVOID */
    TEST_TYPE(LPCVOID, 4, 4);
}

static void test_pack_LPPOINT(void)
{
    /* LPPOINT */
    TEST_TYPE(LPPOINT, 4, 4);
    TEST_TYPE_POINTER(LPPOINT, 8, 4);
}

static void test_pack_LPRECT(void)
{
    /* LPRECT */
    TEST_TYPE(LPRECT, 4, 4);
    TEST_TYPE_POINTER(LPRECT, 16, 4);
}

static void test_pack_LPRECTL(void)
{
    /* LPRECTL */
    TEST_TYPE(LPRECTL, 4, 4);
    TEST_TYPE_POINTER(LPRECTL, 16, 4);
}

static void test_pack_LPSIZE(void)
{
    /* LPSIZE */
    TEST_TYPE(LPSIZE, 4, 4);
    TEST_TYPE_POINTER(LPSIZE, 8, 4);
}

static void test_pack_LRESULT(void)
{
    /* LRESULT */
    TEST_TYPE(LRESULT, 4, 4);
}

static void test_pack_POINT(void)
{
    /* POINT (pack 4) */
    TEST_TYPE(POINT, 8, 4);
    TEST_FIELD(POINT, x, 0, 4, 4);
    TEST_FIELD(POINT, y, 4, 4, 4);
}

static void test_pack_POINTL(void)
{
    /* POINTL (pack 4) */
    TEST_TYPE(POINTL, 8, 4);
    TEST_FIELD(POINTL, x, 0, 4, 4);
    TEST_FIELD(POINTL, y, 4, 4, 4);
}

static void test_pack_PPOINT(void)
{
    /* PPOINT */
    TEST_TYPE(PPOINT, 4, 4);
    TEST_TYPE_POINTER(PPOINT, 8, 4);
}

static void test_pack_PPOINTL(void)
{
    /* PPOINTL */
    TEST_TYPE(PPOINTL, 4, 4);
    TEST_TYPE_POINTER(PPOINTL, 8, 4);
}

static void test_pack_PRECT(void)
{
    /* PRECT */
    TEST_TYPE(PRECT, 4, 4);
    TEST_TYPE_POINTER(PRECT, 16, 4);
}

static void test_pack_PRECTL(void)
{
    /* PRECTL */
    TEST_TYPE(PRECTL, 4, 4);
    TEST_TYPE_POINTER(PRECTL, 16, 4);
}

static void test_pack_PROC(void)
{
    /* PROC */
    TEST_TYPE(PROC, 4, 4);
}

static void test_pack_PSIZE(void)
{
    /* PSIZE */
    TEST_TYPE(PSIZE, 4, 4);
    TEST_TYPE_POINTER(PSIZE, 8, 4);
}

static void test_pack_PSZ(void)
{
    /* PSZ */
    TEST_TYPE(PSZ, 4, 4);
}

static void test_pack_RECT(void)
{
    /* RECT (pack 4) */
    TEST_TYPE(RECT, 16, 4);
    TEST_FIELD(RECT, left, 0, 4, 4);
    TEST_FIELD(RECT, top, 4, 4, 4);
    TEST_FIELD(RECT, right, 8, 4, 4);
    TEST_FIELD(RECT, bottom, 12, 4, 4);
}

static void test_pack_RECTL(void)
{
    /* RECTL (pack 4) */
    TEST_TYPE(RECTL, 16, 4);
    TEST_FIELD(RECTL, left, 0, 4, 4);
    TEST_FIELD(RECTL, top, 4, 4, 4);
    TEST_FIELD(RECTL, right, 8, 4, 4);
    TEST_FIELD(RECTL, bottom, 12, 4, 4);
}

static void test_pack_SIZE(void)
{
    /* SIZE (pack 4) */
    TEST_TYPE(SIZE, 8, 4);
    TEST_FIELD(SIZE, cx, 0, 4, 4);
    TEST_FIELD(SIZE, cy, 4, 4, 4);
}

static void test_pack_SIZEL(void)
{
    /* SIZEL */
    TEST_TYPE(SIZEL, 8, 4);
}

static void test_pack_UCHAR(void)
{
    /* UCHAR */
    TEST_TYPE(UCHAR, 1, 1);
    TEST_TYPE_UNSIGNED(UCHAR);
}

static void test_pack_UINT(void)
{
    /* UINT */
    TEST_TYPE(UINT, 4, 4);
    TEST_TYPE_UNSIGNED(UINT);
}

static void test_pack_ULONG(void)
{
    /* ULONG */
    TEST_TYPE(ULONG, 4, 4);
    TEST_TYPE_UNSIGNED(ULONG);
}

static void test_pack_USHORT(void)
{
    /* USHORT */
    TEST_TYPE(USHORT, 2, 2);
    TEST_TYPE_UNSIGNED(USHORT);
}

static void test_pack_WORD(void)
{
    /* WORD */
    TEST_TYPE(WORD, 2, 2);
    TEST_TYPE_UNSIGNED(WORD);
}

static void test_pack_WPARAM(void)
{
    /* WPARAM */
    TEST_TYPE(WPARAM, 4, 4);
}

static void test_pack(void)
{
    test_pack_ACCESS_ALLOWED_ACE();
    test_pack_ACCESS_DENIED_ACE();
    test_pack_ACCESS_MASK();
    test_pack_ACE_HEADER();
    test_pack_ACL();
    test_pack_ACL_REVISION_INFORMATION();
    test_pack_ACL_SIZE_INFORMATION();
    test_pack_ATOM();
    test_pack_BOOL();
    test_pack_BOOLEAN();
    test_pack_BYTE();
    test_pack_CCHAR();
    test_pack_CHAR();
    test_pack_COLORREF();
    test_pack_DWORD();
    test_pack_DWORD32();
    test_pack_DWORD64();
    test_pack_DWORDLONG();
    test_pack_DWORD_PTR();
    test_pack_EXCEPTION_POINTERS();
    test_pack_EXCEPTION_RECORD();
    test_pack_EXECUTION_STATE();
    test_pack_FARPROC();
    test_pack_FLOAT();
    test_pack_FLOATING_SAVE_AREA();
    test_pack_FPO_DATA();
    test_pack_GENERIC_MAPPING();
    test_pack_GLOBALHANDLE();
    test_pack_HALF_PTR();
    test_pack_HANDLE();
    test_pack_HCURSOR();
    test_pack_HFILE();
    test_pack_HGDIOBJ();
    test_pack_HGLOBAL();
    test_pack_HLOCAL();
    test_pack_HMODULE();
    test_pack_HRESULT();
    test_pack_IMAGE_ARCHIVE_MEMBER_HEADER();
    test_pack_IMAGE_AUX_SYMBOL();
    test_pack_IMAGE_BASE_RELOCATION();
    test_pack_IMAGE_BOUND_FORWARDER_REF();
    test_pack_IMAGE_BOUND_IMPORT_DESCRIPTOR();
    test_pack_IMAGE_COFF_SYMBOLS_HEADER();
    test_pack_IMAGE_DATA_DIRECTORY();
    test_pack_IMAGE_DEBUG_DIRECTORY();
    test_pack_IMAGE_DEBUG_MISC();
    test_pack_IMAGE_DOS_HEADER();
    test_pack_IMAGE_EXPORT_DIRECTORY();
    test_pack_IMAGE_FILE_HEADER();
    test_pack_IMAGE_FUNCTION_ENTRY();
    test_pack_IMAGE_IMPORT_BY_NAME();
    test_pack_IMAGE_IMPORT_DESCRIPTOR();
    test_pack_IMAGE_LINENUMBER();
    test_pack_IMAGE_LOAD_CONFIG_DIRECTORY();
    test_pack_IMAGE_NT_HEADERS();
    test_pack_IMAGE_OS2_HEADER();
    test_pack_IMAGE_RELOCATION();
    test_pack_IMAGE_RESOURCE_DATA_ENTRY();
    test_pack_IMAGE_RESOURCE_DIRECTORY();
    test_pack_IMAGE_RESOURCE_DIRECTORY_ENTRY();
    test_pack_IMAGE_RESOURCE_DIRECTORY_STRING();
    test_pack_IMAGE_RESOURCE_DIR_STRING_U();
    test_pack_IMAGE_SECTION_HEADER();
    test_pack_IMAGE_SEPARATE_DEBUG_HEADER();
    test_pack_IMAGE_SYMBOL();
    test_pack_IMAGE_THUNK_DATA();
    test_pack_IMAGE_TLS_DIRECTORY();
    test_pack_IMAGE_VXD_HEADER();
    test_pack_INT();
    test_pack_INT16();
    test_pack_INT32();
    test_pack_INT64();
    test_pack_INT8();
    test_pack_INT_PTR();
    test_pack_IO_COUNTERS();
    test_pack_LANGID();
    test_pack_LARGE_INTEGER();
    test_pack_LCID();
    test_pack_LIST_ENTRY();
    test_pack_LOCALHANDLE();
    test_pack_LONG();
    test_pack_LONG32();
    test_pack_LONG64();
    test_pack_LONGLONG();
    test_pack_LONG_PTR();
    test_pack_LPARAM();
    test_pack_LPCRECT();
    test_pack_LPCRECTL();
    test_pack_LPCVOID();
    test_pack_LPPOINT();
    test_pack_LPRECT();
    test_pack_LPRECTL();
    test_pack_LPSIZE();
    test_pack_LRESULT();
    test_pack_LUID();
    test_pack_LUID_AND_ATTRIBUTES();
    test_pack_MEMORY_BASIC_INFORMATION();
    test_pack_MESSAGE_RESOURCE_BLOCK();
    test_pack_MESSAGE_RESOURCE_DATA();
    test_pack_MESSAGE_RESOURCE_ENTRY();
    test_pack_NT_TIB();
    test_pack_OBJECT_TYPE_LIST();
    test_pack_PACCESS_ALLOWED_ACE();
    test_pack_PACCESS_DENIED_ACE();
    test_pack_PACCESS_TOKEN();
    test_pack_PACE_HEADER();
    test_pack_PACL();
    test_pack_PACL_REVISION_INFORMATION();
    test_pack_PACL_SIZE_INFORMATION();
    test_pack_PCCH();
    test_pack_PCH();
    test_pack_PCSTR();
    test_pack_PCTSTR();
    test_pack_PCWCH();
    test_pack_PCWSTR();
    test_pack_PEXCEPTION_POINTERS();
    test_pack_PEXCEPTION_RECORD();
    test_pack_PFLOATING_SAVE_AREA();
    test_pack_PFPO_DATA();
    test_pack_PGENERIC_MAPPING();
    test_pack_PHANDLE();
    test_pack_PIMAGE_ARCHIVE_MEMBER_HEADER();
    test_pack_PIMAGE_AUX_SYMBOL();
    test_pack_PIMAGE_BASE_RELOCATION();
    test_pack_PIMAGE_BOUND_FORWARDER_REF();
    test_pack_PIMAGE_BOUND_IMPORT_DESCRIPTOR();
    test_pack_PIMAGE_COFF_SYMBOLS_HEADER();
    test_pack_PIMAGE_DATA_DIRECTORY();
    test_pack_PIMAGE_DEBUG_DIRECTORY();
    test_pack_PIMAGE_DEBUG_MISC();
    test_pack_PIMAGE_DOS_HEADER();
    test_pack_PIMAGE_EXPORT_DIRECTORY();
    test_pack_PIMAGE_FILE_HEADER();
    test_pack_PIMAGE_FUNCTION_ENTRY();
    test_pack_PIMAGE_IMPORT_BY_NAME();
    test_pack_PIMAGE_IMPORT_DESCRIPTOR();
    test_pack_PIMAGE_LINENUMBER();
    test_pack_PIMAGE_LOAD_CONFIG_DIRECTORY();
    test_pack_PIMAGE_NT_HEADERS();
    test_pack_PIMAGE_OPTIONAL_HEADER();
    test_pack_PIMAGE_OS2_HEADER();
    test_pack_PIMAGE_RELOCATION();
    test_pack_PIMAGE_RESOURCE_DATA_ENTRY();
    test_pack_PIMAGE_RESOURCE_DIRECTORY();
    test_pack_PIMAGE_RESOURCE_DIRECTORY_ENTRY();
    test_pack_PIMAGE_RESOURCE_DIRECTORY_STRING();
    test_pack_PIMAGE_RESOURCE_DIR_STRING_U();
    test_pack_PIMAGE_SECTION_HEADER();
    test_pack_PIMAGE_SEPARATE_DEBUG_HEADER();
    test_pack_PIMAGE_SYMBOL();
    test_pack_PIMAGE_THUNK_DATA();
    test_pack_PIMAGE_TLS_CALLBACK();
    test_pack_PIMAGE_TLS_DIRECTORY();
    test_pack_PIMAGE_VXD_HEADER();
    test_pack_PIO_COUNTERS();
    test_pack_PISECURITY_DESCRIPTOR();
    test_pack_PISECURITY_DESCRIPTOR_RELATIVE();
    test_pack_PISID();
    test_pack_PLARGE_INTEGER();
    test_pack_PLIST_ENTRY();
    test_pack_PLUID();
    test_pack_PLUID_AND_ATTRIBUTES();
    test_pack_PMEMORY_BASIC_INFORMATION();
    test_pack_PMESSAGE_RESOURCE_BLOCK();
    test_pack_PMESSAGE_RESOURCE_DATA();
    test_pack_PMESSAGE_RESOURCE_ENTRY();
    test_pack_PNT_TIB();
    test_pack_POBJECT_TYPE_LIST();
    test_pack_POINT();
    test_pack_POINTL();
    test_pack_PPOINT();
    test_pack_PPOINTL();
    test_pack_PPRIVILEGE_SET();
    test_pack_PRECT();
    test_pack_PRECTL();
    test_pack_PRIVILEGE_SET();
    test_pack_PRLIST_ENTRY();
    test_pack_PROC();
    test_pack_PRTL_CRITICAL_SECTION();
    test_pack_PRTL_CRITICAL_SECTION_DEBUG();
    test_pack_PRTL_OSVERSIONINFOEXW();
    test_pack_PRTL_OSVERSIONINFOW();
    test_pack_PRTL_RESOURCE_DEBUG();
    test_pack_PSECURITY_DESCRIPTOR();
    test_pack_PSECURITY_QUALITY_OF_SERVICE();
    test_pack_PSID();
    test_pack_PSID_IDENTIFIER_AUTHORITY();
    test_pack_PSINGLE_LIST_ENTRY();
    test_pack_PSIZE();
    test_pack_PSTR();
    test_pack_PSYSTEM_ALARM_ACE();
    test_pack_PSYSTEM_AUDIT_ACE();
    test_pack_PSZ();
    test_pack_PTOKEN_GROUPS();
    test_pack_PTOKEN_PRIVILEGES();
    test_pack_PTOKEN_USER();
    test_pack_PTSTR();
    test_pack_PULARGE_INTEGER();
    test_pack_PVECTORED_EXCEPTION_HANDLER();
    test_pack_PVOID();
    test_pack_PWCH();
    test_pack_PWSTR();
    test_pack_RECT();
    test_pack_RECTL();
    test_pack_RTL_CRITICAL_SECTION();
    test_pack_RTL_CRITICAL_SECTION_DEBUG();
    test_pack_RTL_OSVERSIONINFOEXW();
    test_pack_RTL_OSVERSIONINFOW();
    test_pack_RTL_RESOURCE_DEBUG();
    test_pack_SECURITY_CONTEXT_TRACKING_MODE();
    test_pack_SECURITY_DESCRIPTOR();
    test_pack_SECURITY_DESCRIPTOR_CONTROL();
    test_pack_SECURITY_DESCRIPTOR_RELATIVE();
    test_pack_SECURITY_INFORMATION();
    test_pack_SECURITY_QUALITY_OF_SERVICE();
    test_pack_SHORT();
    test_pack_SID();
    test_pack_SID_AND_ATTRIBUTES();
    test_pack_SID_IDENTIFIER_AUTHORITY();
    test_pack_SINGLE_LIST_ENTRY();
    test_pack_SIZE();
    test_pack_SIZEL();
    test_pack_SIZE_T();
    test_pack_SSIZE_T();
    test_pack_SYSTEM_ALARM_ACE();
    test_pack_SYSTEM_AUDIT_ACE();
    test_pack_TCHAR();
    test_pack_TOKEN_DEFAULT_DACL();
    test_pack_TOKEN_GROUPS();
    test_pack_TOKEN_OWNER();
    test_pack_TOKEN_PRIMARY_GROUP();
    test_pack_TOKEN_PRIVILEGES();
    test_pack_TOKEN_SOURCE();
    test_pack_TOKEN_STATISTICS();
    test_pack_TOKEN_USER();
    test_pack_UCHAR();
    test_pack_UHALF_PTR();
    test_pack_UINT();
    test_pack_UINT16();
    test_pack_UINT32();
    test_pack_UINT64();
    test_pack_UINT8();
    test_pack_UINT_PTR();
    test_pack_ULARGE_INTEGER();
    test_pack_ULONG();
    test_pack_ULONG32();
    test_pack_ULONG64();
    test_pack_ULONGLONG();
    test_pack_ULONG_PTR();
    test_pack_USHORT();
    test_pack_WAITORTIMERCALLBACKFUNC();
    test_pack_WCHAR();
    test_pack_WORD();
    test_pack_WPARAM();
}

START_TEST(generated)
{
    test_pack();
}
