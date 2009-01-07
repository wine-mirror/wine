/* File generated automatically from tools/winapi/tests.dat; do not edit! */
/* This file can be copied, modified and distributed without restriction. */

/*
 * Unit tests for data structure packing
 */

#define WINVER 0x0501
#define _WIN32_IE 0x0501
#define _WIN32_WINNT 0x0501

#define WINE_NOWINSOCK

#include "windows.h"

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

static void test_pack_LPOSVERSIONINFOA(void)
{
    /* LPOSVERSIONINFOA */
    TEST_TYPE(LPOSVERSIONINFOA, 4, 4);
    TEST_TYPE_POINTER(LPOSVERSIONINFOA, 148, 4);
}

static void test_pack_LPOSVERSIONINFOEXA(void)
{
    /* LPOSVERSIONINFOEXA */
    TEST_TYPE(LPOSVERSIONINFOEXA, 4, 4);
    TEST_TYPE_POINTER(LPOSVERSIONINFOEXA, 156, 4);
}

static void test_pack_LPOSVERSIONINFOEXW(void)
{
    /* LPOSVERSIONINFOEXW */
    TEST_TYPE(LPOSVERSIONINFOEXW, 4, 4);
    TEST_TYPE_POINTER(LPOSVERSIONINFOEXW, 284, 4);
}

static void test_pack_LPOSVERSIONINFOW(void)
{
    /* LPOSVERSIONINFOW */
    TEST_TYPE(LPOSVERSIONINFOW, 4, 4);
    TEST_TYPE_POINTER(LPOSVERSIONINFOW, 276, 4);
}

static void test_pack_OSVERSIONINFOA(void)
{
    /* OSVERSIONINFOA (pack 4) */
    TEST_TYPE(OSVERSIONINFOA, 148, 4);
    TEST_FIELD(OSVERSIONINFOA, dwOSVersionInfoSize, 0, 4, 4);
    TEST_FIELD(OSVERSIONINFOA, dwMajorVersion, 4, 4, 4);
    TEST_FIELD(OSVERSIONINFOA, dwMinorVersion, 8, 4, 4);
    TEST_FIELD(OSVERSIONINFOA, dwBuildNumber, 12, 4, 4);
    TEST_FIELD(OSVERSIONINFOA, dwPlatformId, 16, 4, 4);
    TEST_FIELD(OSVERSIONINFOA, szCSDVersion, 20, 128, 1);
}

static void test_pack_OSVERSIONINFOEXA(void)
{
    /* OSVERSIONINFOEXA (pack 4) */
    TEST_TYPE(OSVERSIONINFOEXA, 156, 4);
    TEST_FIELD(OSVERSIONINFOEXA, dwOSVersionInfoSize, 0, 4, 4);
    TEST_FIELD(OSVERSIONINFOEXA, dwMajorVersion, 4, 4, 4);
    TEST_FIELD(OSVERSIONINFOEXA, dwMinorVersion, 8, 4, 4);
    TEST_FIELD(OSVERSIONINFOEXA, dwBuildNumber, 12, 4, 4);
    TEST_FIELD(OSVERSIONINFOEXA, dwPlatformId, 16, 4, 4);
    TEST_FIELD(OSVERSIONINFOEXA, szCSDVersion, 20, 128, 1);
    TEST_FIELD(OSVERSIONINFOEXA, wServicePackMajor, 148, 2, 2);
    TEST_FIELD(OSVERSIONINFOEXA, wServicePackMinor, 150, 2, 2);
    TEST_FIELD(OSVERSIONINFOEXA, wSuiteMask, 152, 2, 2);
    TEST_FIELD(OSVERSIONINFOEXA, wProductType, 154, 1, 1);
    TEST_FIELD(OSVERSIONINFOEXA, wReserved, 155, 1, 1);
}

static void test_pack_OSVERSIONINFOEXW(void)
{
    /* OSVERSIONINFOEXW (pack 4) */
    TEST_TYPE(OSVERSIONINFOEXW, 284, 4);
    TEST_FIELD(OSVERSIONINFOEXW, dwOSVersionInfoSize, 0, 4, 4);
    TEST_FIELD(OSVERSIONINFOEXW, dwMajorVersion, 4, 4, 4);
    TEST_FIELD(OSVERSIONINFOEXW, dwMinorVersion, 8, 4, 4);
    TEST_FIELD(OSVERSIONINFOEXW, dwBuildNumber, 12, 4, 4);
    TEST_FIELD(OSVERSIONINFOEXW, dwPlatformId, 16, 4, 4);
    TEST_FIELD(OSVERSIONINFOEXW, szCSDVersion, 20, 256, 2);
    TEST_FIELD(OSVERSIONINFOEXW, wServicePackMajor, 276, 2, 2);
    TEST_FIELD(OSVERSIONINFOEXW, wServicePackMinor, 278, 2, 2);
    TEST_FIELD(OSVERSIONINFOEXW, wSuiteMask, 280, 2, 2);
    TEST_FIELD(OSVERSIONINFOEXW, wProductType, 282, 1, 1);
    TEST_FIELD(OSVERSIONINFOEXW, wReserved, 283, 1, 1);
}

static void test_pack_OSVERSIONINFOW(void)
{
    /* OSVERSIONINFOW (pack 4) */
    TEST_TYPE(OSVERSIONINFOW, 276, 4);
    TEST_FIELD(OSVERSIONINFOW, dwOSVersionInfoSize, 0, 4, 4);
    TEST_FIELD(OSVERSIONINFOW, dwMajorVersion, 4, 4, 4);
    TEST_FIELD(OSVERSIONINFOW, dwMinorVersion, 8, 4, 4);
    TEST_FIELD(OSVERSIONINFOW, dwBuildNumber, 12, 4, 4);
    TEST_FIELD(OSVERSIONINFOW, dwPlatformId, 16, 4, 4);
    TEST_FIELD(OSVERSIONINFOW, szCSDVersion, 20, 256, 2);
}

static void test_pack_POSVERSIONINFOA(void)
{
    /* POSVERSIONINFOA */
    TEST_TYPE(POSVERSIONINFOA, 4, 4);
    TEST_TYPE_POINTER(POSVERSIONINFOA, 148, 4);
}

static void test_pack_POSVERSIONINFOEXA(void)
{
    /* POSVERSIONINFOEXA */
    TEST_TYPE(POSVERSIONINFOEXA, 4, 4);
    TEST_TYPE_POINTER(POSVERSIONINFOEXA, 156, 4);
}

static void test_pack_POSVERSIONINFOEXW(void)
{
    /* POSVERSIONINFOEXW */
    TEST_TYPE(POSVERSIONINFOEXW, 4, 4);
    TEST_TYPE_POINTER(POSVERSIONINFOEXW, 284, 4);
}

static void test_pack_POSVERSIONINFOW(void)
{
    /* POSVERSIONINFOW */
    TEST_TYPE(POSVERSIONINFOW, 4, 4);
    TEST_TYPE_POINTER(POSVERSIONINFOW, 276, 4);
}

static void test_pack_LPLONG(void)
{
    /* LPLONG */
    TEST_TYPE(LPLONG, 4, 4);
}

static void test_pack_LPVOID(void)
{
    /* LPVOID */
    TEST_TYPE(LPVOID, 4, 4);
}

static void test_pack_PHKEY(void)
{
    /* PHKEY */
    TEST_TYPE(PHKEY, 4, 4);
}

static void test_pack_ACTCTXA(void)
{
    /* ACTCTXA (pack 4) */
    TEST_TYPE(ACTCTXA, 32, 4);
    TEST_FIELD(ACTCTXA, cbSize, 0, 4, 4);
    TEST_FIELD(ACTCTXA, dwFlags, 4, 4, 4);
    TEST_FIELD(ACTCTXA, lpSource, 8, 4, 4);
    TEST_FIELD(ACTCTXA, wProcessorArchitecture, 12, 2, 2);
    TEST_FIELD(ACTCTXA, wLangId, 14, 2, 2);
    TEST_FIELD(ACTCTXA, lpAssemblyDirectory, 16, 4, 4);
    TEST_FIELD(ACTCTXA, lpResourceName, 20, 4, 4);
    TEST_FIELD(ACTCTXA, lpApplicationName, 24, 4, 4);
    TEST_FIELD(ACTCTXA, hModule, 28, 4, 4);
}

static void test_pack_ACTCTXW(void)
{
    /* ACTCTXW (pack 4) */
    TEST_TYPE(ACTCTXW, 32, 4);
    TEST_FIELD(ACTCTXW, cbSize, 0, 4, 4);
    TEST_FIELD(ACTCTXW, dwFlags, 4, 4, 4);
    TEST_FIELD(ACTCTXW, lpSource, 8, 4, 4);
    TEST_FIELD(ACTCTXW, wProcessorArchitecture, 12, 2, 2);
    TEST_FIELD(ACTCTXW, wLangId, 14, 2, 2);
    TEST_FIELD(ACTCTXW, lpAssemblyDirectory, 16, 4, 4);
    TEST_FIELD(ACTCTXW, lpResourceName, 20, 4, 4);
    TEST_FIELD(ACTCTXW, lpApplicationName, 24, 4, 4);
    TEST_FIELD(ACTCTXW, hModule, 28, 4, 4);
}

static void test_pack_ACTCTX_SECTION_KEYED_DATA(void)
{
    /* ACTCTX_SECTION_KEYED_DATA (pack 4) */
    TEST_TYPE(ACTCTX_SECTION_KEYED_DATA, 64, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA, cbSize, 0, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA, ulDataFormatVersion, 4, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA, lpData, 8, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA, ulLength, 12, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA, lpSectionGlobalData, 16, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA, ulSectionGlobalDataLength, 20, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA, lpSectionBase, 24, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA, ulSectionTotalLength, 28, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA, hActCtx, 32, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA, ulAssemblyRosterIndex, 36, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA, ulFlags, 40, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA, AssemblyMetadata, 44, 20, 4);
}

static void test_pack_ACTCTX_SECTION_KEYED_DATA_2600(void)
{
    /* ACTCTX_SECTION_KEYED_DATA_2600 (pack 4) */
    TEST_TYPE(ACTCTX_SECTION_KEYED_DATA_2600, 40, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA_2600, cbSize, 0, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA_2600, ulDataFormatVersion, 4, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA_2600, lpData, 8, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA_2600, ulLength, 12, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA_2600, lpSectionGlobalData, 16, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA_2600, ulSectionGlobalDataLength, 20, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA_2600, lpSectionBase, 24, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA_2600, ulSectionTotalLength, 28, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA_2600, hActCtx, 32, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA_2600, ulAssemblyRosterIndex, 36, 4, 4);
}

static void test_pack_ACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA(void)
{
    /* ACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA (pack 4) */
    TEST_TYPE(ACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA, 20, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA, lpInformation, 0, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA, lpSectionBase, 4, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA, ulSectionLength, 8, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA, lpSectionGlobalDataBase, 12, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA, ulSectionGlobalDataLength, 16, 4, 4);
}

static void test_pack_ACTIVATION_CONTEXT_BASIC_INFORMATION(void)
{
    /* ACTIVATION_CONTEXT_BASIC_INFORMATION (pack 4) */
    TEST_TYPE(ACTIVATION_CONTEXT_BASIC_INFORMATION, 8, 4);
    TEST_FIELD(ACTIVATION_CONTEXT_BASIC_INFORMATION, hActCtx, 0, 4, 4);
    TEST_FIELD(ACTIVATION_CONTEXT_BASIC_INFORMATION, dwFlags, 4, 4, 4);
}

static void test_pack_BY_HANDLE_FILE_INFORMATION(void)
{
    /* BY_HANDLE_FILE_INFORMATION (pack 4) */
    TEST_TYPE(BY_HANDLE_FILE_INFORMATION, 52, 4);
    TEST_FIELD(BY_HANDLE_FILE_INFORMATION, dwFileAttributes, 0, 4, 4);
    TEST_FIELD(BY_HANDLE_FILE_INFORMATION, ftCreationTime, 4, 8, 4);
    TEST_FIELD(BY_HANDLE_FILE_INFORMATION, ftLastAccessTime, 12, 8, 4);
    TEST_FIELD(BY_HANDLE_FILE_INFORMATION, ftLastWriteTime, 20, 8, 4);
    TEST_FIELD(BY_HANDLE_FILE_INFORMATION, dwVolumeSerialNumber, 28, 4, 4);
    TEST_FIELD(BY_HANDLE_FILE_INFORMATION, nFileSizeHigh, 32, 4, 4);
    TEST_FIELD(BY_HANDLE_FILE_INFORMATION, nFileSizeLow, 36, 4, 4);
    TEST_FIELD(BY_HANDLE_FILE_INFORMATION, nNumberOfLinks, 40, 4, 4);
    TEST_FIELD(BY_HANDLE_FILE_INFORMATION, nFileIndexHigh, 44, 4, 4);
    TEST_FIELD(BY_HANDLE_FILE_INFORMATION, nFileIndexLow, 48, 4, 4);
}

static void test_pack_COMMCONFIG(void)
{
    /* COMMCONFIG (pack 4) */
    TEST_TYPE(COMMCONFIG, 52, 4);
    TEST_FIELD(COMMCONFIG, dwSize, 0, 4, 4);
    TEST_FIELD(COMMCONFIG, wVersion, 4, 2, 2);
    TEST_FIELD(COMMCONFIG, wReserved, 6, 2, 2);
    TEST_FIELD(COMMCONFIG, dcb, 8, 28, 4);
    TEST_FIELD(COMMCONFIG, dwProviderSubType, 36, 4, 4);
    TEST_FIELD(COMMCONFIG, dwProviderOffset, 40, 4, 4);
    TEST_FIELD(COMMCONFIG, dwProviderSize, 44, 4, 4);
    TEST_FIELD(COMMCONFIG, wcProviderData, 48, 4, 4);
}

static void test_pack_COMMPROP(void)
{
    /* COMMPROP (pack 4) */
    TEST_TYPE(COMMPROP, 64, 4);
    TEST_FIELD(COMMPROP, wPacketLength, 0, 2, 2);
    TEST_FIELD(COMMPROP, wPacketVersion, 2, 2, 2);
    TEST_FIELD(COMMPROP, dwServiceMask, 4, 4, 4);
    TEST_FIELD(COMMPROP, dwReserved1, 8, 4, 4);
    TEST_FIELD(COMMPROP, dwMaxTxQueue, 12, 4, 4);
    TEST_FIELD(COMMPROP, dwMaxRxQueue, 16, 4, 4);
    TEST_FIELD(COMMPROP, dwMaxBaud, 20, 4, 4);
    TEST_FIELD(COMMPROP, dwProvSubType, 24, 4, 4);
    TEST_FIELD(COMMPROP, dwProvCapabilities, 28, 4, 4);
    TEST_FIELD(COMMPROP, dwSettableParams, 32, 4, 4);
    TEST_FIELD(COMMPROP, dwSettableBaud, 36, 4, 4);
    TEST_FIELD(COMMPROP, wSettableData, 40, 2, 2);
    TEST_FIELD(COMMPROP, wSettableStopParity, 42, 2, 2);
    TEST_FIELD(COMMPROP, dwCurrentTxQueue, 44, 4, 4);
    TEST_FIELD(COMMPROP, dwCurrentRxQueue, 48, 4, 4);
    TEST_FIELD(COMMPROP, dwProvSpec1, 52, 4, 4);
    TEST_FIELD(COMMPROP, dwProvSpec2, 56, 4, 4);
    TEST_FIELD(COMMPROP, wcProvChar, 60, 2, 2);
}

static void test_pack_COMMTIMEOUTS(void)
{
    /* COMMTIMEOUTS (pack 4) */
    TEST_TYPE(COMMTIMEOUTS, 20, 4);
    TEST_FIELD(COMMTIMEOUTS, ReadIntervalTimeout, 0, 4, 4);
    TEST_FIELD(COMMTIMEOUTS, ReadTotalTimeoutMultiplier, 4, 4, 4);
    TEST_FIELD(COMMTIMEOUTS, ReadTotalTimeoutConstant, 8, 4, 4);
    TEST_FIELD(COMMTIMEOUTS, WriteTotalTimeoutMultiplier, 12, 4, 4);
    TEST_FIELD(COMMTIMEOUTS, WriteTotalTimeoutConstant, 16, 4, 4);
}

static void test_pack_COMSTAT(void)
{
    /* COMSTAT (pack 4) */
    TEST_TYPE(COMSTAT, 12, 4);
    TEST_FIELD(COMSTAT, cbInQue, 4, 4, 4);
    TEST_FIELD(COMSTAT, cbOutQue, 8, 4, 4);
}

static void test_pack_CREATE_PROCESS_DEBUG_INFO(void)
{
    /* CREATE_PROCESS_DEBUG_INFO (pack 4) */
    TEST_TYPE(CREATE_PROCESS_DEBUG_INFO, 40, 4);
    TEST_FIELD(CREATE_PROCESS_DEBUG_INFO, hFile, 0, 4, 4);
    TEST_FIELD(CREATE_PROCESS_DEBUG_INFO, hProcess, 4, 4, 4);
    TEST_FIELD(CREATE_PROCESS_DEBUG_INFO, hThread, 8, 4, 4);
    TEST_FIELD(CREATE_PROCESS_DEBUG_INFO, lpBaseOfImage, 12, 4, 4);
    TEST_FIELD(CREATE_PROCESS_DEBUG_INFO, dwDebugInfoFileOffset, 16, 4, 4);
    TEST_FIELD(CREATE_PROCESS_DEBUG_INFO, nDebugInfoSize, 20, 4, 4);
    TEST_FIELD(CREATE_PROCESS_DEBUG_INFO, lpThreadLocalBase, 24, 4, 4);
    TEST_FIELD(CREATE_PROCESS_DEBUG_INFO, lpStartAddress, 28, 4, 4);
    TEST_FIELD(CREATE_PROCESS_DEBUG_INFO, lpImageName, 32, 4, 4);
    TEST_FIELD(CREATE_PROCESS_DEBUG_INFO, fUnicode, 36, 2, 2);
}

static void test_pack_CREATE_THREAD_DEBUG_INFO(void)
{
    /* CREATE_THREAD_DEBUG_INFO (pack 4) */
    TEST_TYPE(CREATE_THREAD_DEBUG_INFO, 12, 4);
    TEST_FIELD(CREATE_THREAD_DEBUG_INFO, hThread, 0, 4, 4);
    TEST_FIELD(CREATE_THREAD_DEBUG_INFO, lpThreadLocalBase, 4, 4, 4);
    TEST_FIELD(CREATE_THREAD_DEBUG_INFO, lpStartAddress, 8, 4, 4);
}

static void test_pack_CRITICAL_SECTION(void)
{
    /* CRITICAL_SECTION */
    TEST_TYPE(CRITICAL_SECTION, 24, 4);
}

static void test_pack_CRITICAL_SECTION_DEBUG(void)
{
    /* CRITICAL_SECTION_DEBUG */
}

static void test_pack_DCB(void)
{
    /* DCB (pack 4) */
    TEST_TYPE(DCB, 28, 4);
    TEST_FIELD(DCB, DCBlength, 0, 4, 4);
    TEST_FIELD(DCB, BaudRate, 4, 4, 4);
    TEST_FIELD(DCB, wReserved, 12, 2, 2);
    TEST_FIELD(DCB, XonLim, 14, 2, 2);
    TEST_FIELD(DCB, XoffLim, 16, 2, 2);
    TEST_FIELD(DCB, ByteSize, 18, 1, 1);
    TEST_FIELD(DCB, Parity, 19, 1, 1);
    TEST_FIELD(DCB, StopBits, 20, 1, 1);
    TEST_FIELD(DCB, XonChar, 21, 1, 1);
    TEST_FIELD(DCB, XoffChar, 22, 1, 1);
    TEST_FIELD(DCB, ErrorChar, 23, 1, 1);
    TEST_FIELD(DCB, EofChar, 24, 1, 1);
    TEST_FIELD(DCB, EvtChar, 25, 1, 1);
    TEST_FIELD(DCB, wReserved1, 26, 2, 2);
}

static void test_pack_DEBUG_EVENT(void)
{
    /* DEBUG_EVENT (pack 4) */
    TEST_FIELD(DEBUG_EVENT, dwDebugEventCode, 0, 4, 4);
    TEST_FIELD(DEBUG_EVENT, dwProcessId, 4, 4, 4);
    TEST_FIELD(DEBUG_EVENT, dwThreadId, 8, 4, 4);
}

static void test_pack_ENUMRESLANGPROCA(void)
{
    /* ENUMRESLANGPROCA */
    TEST_TYPE(ENUMRESLANGPROCA, 4, 4);
}

static void test_pack_ENUMRESLANGPROCW(void)
{
    /* ENUMRESLANGPROCW */
    TEST_TYPE(ENUMRESLANGPROCW, 4, 4);
}

static void test_pack_ENUMRESNAMEPROCA(void)
{
    /* ENUMRESNAMEPROCA */
    TEST_TYPE(ENUMRESNAMEPROCA, 4, 4);
}

static void test_pack_ENUMRESNAMEPROCW(void)
{
    /* ENUMRESNAMEPROCW */
    TEST_TYPE(ENUMRESNAMEPROCW, 4, 4);
}

static void test_pack_ENUMRESTYPEPROCA(void)
{
    /* ENUMRESTYPEPROCA */
    TEST_TYPE(ENUMRESTYPEPROCA, 4, 4);
}

static void test_pack_ENUMRESTYPEPROCW(void)
{
    /* ENUMRESTYPEPROCW */
    TEST_TYPE(ENUMRESTYPEPROCW, 4, 4);
}

static void test_pack_EXCEPTION_DEBUG_INFO(void)
{
    /* EXCEPTION_DEBUG_INFO (pack 4) */
    TEST_TYPE(EXCEPTION_DEBUG_INFO, 84, 4);
    TEST_FIELD(EXCEPTION_DEBUG_INFO, ExceptionRecord, 0, 80, 4);
    TEST_FIELD(EXCEPTION_DEBUG_INFO, dwFirstChance, 80, 4, 4);
}

static void test_pack_EXIT_PROCESS_DEBUG_INFO(void)
{
    /* EXIT_PROCESS_DEBUG_INFO (pack 4) */
    TEST_TYPE(EXIT_PROCESS_DEBUG_INFO, 4, 4);
    TEST_FIELD(EXIT_PROCESS_DEBUG_INFO, dwExitCode, 0, 4, 4);
}

static void test_pack_EXIT_THREAD_DEBUG_INFO(void)
{
    /* EXIT_THREAD_DEBUG_INFO (pack 4) */
    TEST_TYPE(EXIT_THREAD_DEBUG_INFO, 4, 4);
    TEST_FIELD(EXIT_THREAD_DEBUG_INFO, dwExitCode, 0, 4, 4);
}

static void test_pack_HW_PROFILE_INFOA(void)
{
    /* HW_PROFILE_INFOA (pack 4) */
    TEST_TYPE(HW_PROFILE_INFOA, 124, 4);
    TEST_FIELD(HW_PROFILE_INFOA, dwDockInfo, 0, 4, 4);
    TEST_FIELD(HW_PROFILE_INFOA, szHwProfileGuid, 4, 39, 1);
    TEST_FIELD(HW_PROFILE_INFOA, szHwProfileName, 43, 80, 1);
}

static void test_pack_HW_PROFILE_INFOW(void)
{
    /* HW_PROFILE_INFOW (pack 4) */
    TEST_TYPE(HW_PROFILE_INFOW, 244, 4);
    TEST_FIELD(HW_PROFILE_INFOW, dwDockInfo, 0, 4, 4);
    TEST_FIELD(HW_PROFILE_INFOW, szHwProfileGuid, 4, 78, 2);
    TEST_FIELD(HW_PROFILE_INFOW, szHwProfileName, 82, 160, 2);
}

static void test_pack_LOAD_DLL_DEBUG_INFO(void)
{
    /* LOAD_DLL_DEBUG_INFO (pack 4) */
    TEST_TYPE(LOAD_DLL_DEBUG_INFO, 24, 4);
    TEST_FIELD(LOAD_DLL_DEBUG_INFO, hFile, 0, 4, 4);
    TEST_FIELD(LOAD_DLL_DEBUG_INFO, lpBaseOfDll, 4, 4, 4);
    TEST_FIELD(LOAD_DLL_DEBUG_INFO, dwDebugInfoFileOffset, 8, 4, 4);
    TEST_FIELD(LOAD_DLL_DEBUG_INFO, nDebugInfoSize, 12, 4, 4);
    TEST_FIELD(LOAD_DLL_DEBUG_INFO, lpImageName, 16, 4, 4);
    TEST_FIELD(LOAD_DLL_DEBUG_INFO, fUnicode, 20, 2, 2);
}

static void test_pack_LPBY_HANDLE_FILE_INFORMATION(void)
{
    /* LPBY_HANDLE_FILE_INFORMATION */
    TEST_TYPE(LPBY_HANDLE_FILE_INFORMATION, 4, 4);
    TEST_TYPE_POINTER(LPBY_HANDLE_FILE_INFORMATION, 52, 4);
}

static void test_pack_LPCOMMCONFIG(void)
{
    /* LPCOMMCONFIG */
    TEST_TYPE(LPCOMMCONFIG, 4, 4);
    TEST_TYPE_POINTER(LPCOMMCONFIG, 52, 4);
}

static void test_pack_LPCOMMPROP(void)
{
    /* LPCOMMPROP */
    TEST_TYPE(LPCOMMPROP, 4, 4);
    TEST_TYPE_POINTER(LPCOMMPROP, 64, 4);
}

static void test_pack_LPCOMMTIMEOUTS(void)
{
    /* LPCOMMTIMEOUTS */
    TEST_TYPE(LPCOMMTIMEOUTS, 4, 4);
    TEST_TYPE_POINTER(LPCOMMTIMEOUTS, 20, 4);
}

static void test_pack_LPCOMSTAT(void)
{
    /* LPCOMSTAT */
    TEST_TYPE(LPCOMSTAT, 4, 4);
    TEST_TYPE_POINTER(LPCOMSTAT, 12, 4);
}

static void test_pack_LPCRITICAL_SECTION(void)
{
    /* LPCRITICAL_SECTION */
    TEST_TYPE(LPCRITICAL_SECTION, 4, 4);
}

static void test_pack_LPCRITICAL_SECTION_DEBUG(void)
{
    /* LPCRITICAL_SECTION_DEBUG */
    TEST_TYPE(LPCRITICAL_SECTION_DEBUG, 4, 4);
}

static void test_pack_LPDCB(void)
{
    /* LPDCB */
    TEST_TYPE(LPDCB, 4, 4);
    TEST_TYPE_POINTER(LPDCB, 28, 4);
}

static void test_pack_LPDEBUG_EVENT(void)
{
    /* LPDEBUG_EVENT */
    TEST_TYPE(LPDEBUG_EVENT, 4, 4);
}

static void test_pack_LPEXCEPTION_POINTERS(void)
{
    /* LPEXCEPTION_POINTERS */
    TEST_TYPE(LPEXCEPTION_POINTERS, 4, 4);
}

static void test_pack_LPEXCEPTION_RECORD(void)
{
    /* LPEXCEPTION_RECORD */
    TEST_TYPE(LPEXCEPTION_RECORD, 4, 4);
}

static void test_pack_LPFIBER_START_ROUTINE(void)
{
    /* LPFIBER_START_ROUTINE */
    TEST_TYPE(LPFIBER_START_ROUTINE, 4, 4);
}

static void test_pack_LPHW_PROFILE_INFOA(void)
{
    /* LPHW_PROFILE_INFOA */
    TEST_TYPE(LPHW_PROFILE_INFOA, 4, 4);
    TEST_TYPE_POINTER(LPHW_PROFILE_INFOA, 124, 4);
}

static void test_pack_LPHW_PROFILE_INFOW(void)
{
    /* LPHW_PROFILE_INFOW */
    TEST_TYPE(LPHW_PROFILE_INFOW, 4, 4);
    TEST_TYPE_POINTER(LPHW_PROFILE_INFOW, 244, 4);
}

static void test_pack_LPMEMORYSTATUS(void)
{
    /* LPMEMORYSTATUS */
    TEST_TYPE(LPMEMORYSTATUS, 4, 4);
    TEST_TYPE_POINTER(LPMEMORYSTATUS, 32, 4);
}

static void test_pack_LPMEMORYSTATUSEX(void)
{
    /* LPMEMORYSTATUSEX */
    TEST_TYPE(LPMEMORYSTATUSEX, 4, 4);
    TEST_TYPE_POINTER(LPMEMORYSTATUSEX, 64, 8);
}

static void test_pack_LPOFSTRUCT(void)
{
    /* LPOFSTRUCT */
    TEST_TYPE(LPOFSTRUCT, 4, 4);
    TEST_TYPE_POINTER(LPOFSTRUCT, 136, 2);
}

static void test_pack_LPOVERLAPPED(void)
{
    /* LPOVERLAPPED */
    TEST_TYPE(LPOVERLAPPED, 4, 4);
}

static void test_pack_LPOVERLAPPED_COMPLETION_ROUTINE(void)
{
    /* LPOVERLAPPED_COMPLETION_ROUTINE */
    TEST_TYPE(LPOVERLAPPED_COMPLETION_ROUTINE, 4, 4);
}

static void test_pack_LPPROCESS_HEAP_ENTRY(void)
{
    /* LPPROCESS_HEAP_ENTRY */
    TEST_TYPE(LPPROCESS_HEAP_ENTRY, 4, 4);
}

static void test_pack_LPPROCESS_INFORMATION(void)
{
    /* LPPROCESS_INFORMATION */
    TEST_TYPE(LPPROCESS_INFORMATION, 4, 4);
    TEST_TYPE_POINTER(LPPROCESS_INFORMATION, 16, 4);
}

static void test_pack_LPPROGRESS_ROUTINE(void)
{
    /* LPPROGRESS_ROUTINE */
    TEST_TYPE(LPPROGRESS_ROUTINE, 4, 4);
}

static void test_pack_LPSECURITY_ATTRIBUTES(void)
{
    /* LPSECURITY_ATTRIBUTES */
    TEST_TYPE(LPSECURITY_ATTRIBUTES, 4, 4);
    TEST_TYPE_POINTER(LPSECURITY_ATTRIBUTES, 12, 4);
}

static void test_pack_LPSTARTUPINFOA(void)
{
    /* LPSTARTUPINFOA */
    TEST_TYPE(LPSTARTUPINFOA, 4, 4);
    TEST_TYPE_POINTER(LPSTARTUPINFOA, 68, 4);
}

static void test_pack_LPSTARTUPINFOW(void)
{
    /* LPSTARTUPINFOW */
    TEST_TYPE(LPSTARTUPINFOW, 4, 4);
    TEST_TYPE_POINTER(LPSTARTUPINFOW, 68, 4);
}

static void test_pack_LPSYSTEMTIME(void)
{
    /* LPSYSTEMTIME */
    TEST_TYPE(LPSYSTEMTIME, 4, 4);
    TEST_TYPE_POINTER(LPSYSTEMTIME, 16, 2);
}

static void test_pack_LPSYSTEM_INFO(void)
{
    /* LPSYSTEM_INFO */
    TEST_TYPE(LPSYSTEM_INFO, 4, 4);
}

static void test_pack_LPSYSTEM_POWER_STATUS(void)
{
    /* LPSYSTEM_POWER_STATUS */
    TEST_TYPE(LPSYSTEM_POWER_STATUS, 4, 4);
    TEST_TYPE_POINTER(LPSYSTEM_POWER_STATUS, 12, 4);
}

static void test_pack_LPTHREAD_START_ROUTINE(void)
{
    /* LPTHREAD_START_ROUTINE */
    TEST_TYPE(LPTHREAD_START_ROUTINE, 4, 4);
}

static void test_pack_LPTIME_ZONE_INFORMATION(void)
{
    /* LPTIME_ZONE_INFORMATION */
    TEST_TYPE(LPTIME_ZONE_INFORMATION, 4, 4);
    TEST_TYPE_POINTER(LPTIME_ZONE_INFORMATION, 172, 4);
}

static void test_pack_LPWIN32_FILE_ATTRIBUTE_DATA(void)
{
    /* LPWIN32_FILE_ATTRIBUTE_DATA */
    TEST_TYPE(LPWIN32_FILE_ATTRIBUTE_DATA, 4, 4);
    TEST_TYPE_POINTER(LPWIN32_FILE_ATTRIBUTE_DATA, 36, 4);
}

static void test_pack_LPWIN32_FIND_DATAA(void)
{
    /* LPWIN32_FIND_DATAA */
    TEST_TYPE(LPWIN32_FIND_DATAA, 4, 4);
    TEST_TYPE_POINTER(LPWIN32_FIND_DATAA, 320, 4);
}

static void test_pack_LPWIN32_FIND_DATAW(void)
{
    /* LPWIN32_FIND_DATAW */
    TEST_TYPE(LPWIN32_FIND_DATAW, 4, 4);
    TEST_TYPE_POINTER(LPWIN32_FIND_DATAW, 592, 4);
}

static void test_pack_LPWIN32_STREAM_ID(void)
{
    /* LPWIN32_STREAM_ID */
    TEST_TYPE(LPWIN32_STREAM_ID, 4, 4);
    TEST_TYPE_POINTER(LPWIN32_STREAM_ID, 24, 8);
}

static void test_pack_MEMORYSTATUS(void)
{
    /* MEMORYSTATUS (pack 4) */
    TEST_TYPE(MEMORYSTATUS, 32, 4);
    TEST_FIELD(MEMORYSTATUS, dwLength, 0, 4, 4);
    TEST_FIELD(MEMORYSTATUS, dwMemoryLoad, 4, 4, 4);
    TEST_FIELD(MEMORYSTATUS, dwTotalPhys, 8, 4, 4);
    TEST_FIELD(MEMORYSTATUS, dwAvailPhys, 12, 4, 4);
    TEST_FIELD(MEMORYSTATUS, dwTotalPageFile, 16, 4, 4);
    TEST_FIELD(MEMORYSTATUS, dwAvailPageFile, 20, 4, 4);
    TEST_FIELD(MEMORYSTATUS, dwTotalVirtual, 24, 4, 4);
    TEST_FIELD(MEMORYSTATUS, dwAvailVirtual, 28, 4, 4);
}

static void test_pack_MEMORYSTATUSEX(void)
{
    /* MEMORYSTATUSEX (pack 8) */
    TEST_TYPE(MEMORYSTATUSEX, 64, 8);
    TEST_FIELD(MEMORYSTATUSEX, dwLength, 0, 4, 4);
    TEST_FIELD(MEMORYSTATUSEX, dwMemoryLoad, 4, 4, 4);
    TEST_FIELD(MEMORYSTATUSEX, ullTotalPhys, 8, 8, 8);
    TEST_FIELD(MEMORYSTATUSEX, ullAvailPhys, 16, 8, 8);
    TEST_FIELD(MEMORYSTATUSEX, ullTotalPageFile, 24, 8, 8);
    TEST_FIELD(MEMORYSTATUSEX, ullAvailPageFile, 32, 8, 8);
    TEST_FIELD(MEMORYSTATUSEX, ullTotalVirtual, 40, 8, 8);
    TEST_FIELD(MEMORYSTATUSEX, ullAvailVirtual, 48, 8, 8);
    TEST_FIELD(MEMORYSTATUSEX, ullAvailExtendedVirtual, 56, 8, 8);
}

static void test_pack_OFSTRUCT(void)
{
    /* OFSTRUCT (pack 4) */
    TEST_TYPE(OFSTRUCT, 136, 2);
    TEST_FIELD(OFSTRUCT, cBytes, 0, 1, 1);
    TEST_FIELD(OFSTRUCT, fFixedDisk, 1, 1, 1);
    TEST_FIELD(OFSTRUCT, nErrCode, 2, 2, 2);
    TEST_FIELD(OFSTRUCT, Reserved1, 4, 2, 2);
    TEST_FIELD(OFSTRUCT, Reserved2, 6, 2, 2);
    TEST_FIELD(OFSTRUCT, szPathName, 8, 128, 1);
}

static void test_pack_OUTPUT_DEBUG_STRING_INFO(void)
{
    /* OUTPUT_DEBUG_STRING_INFO (pack 4) */
    TEST_TYPE(OUTPUT_DEBUG_STRING_INFO, 8, 4);
    TEST_FIELD(OUTPUT_DEBUG_STRING_INFO, lpDebugStringData, 0, 4, 4);
    TEST_FIELD(OUTPUT_DEBUG_STRING_INFO, fUnicode, 4, 2, 2);
    TEST_FIELD(OUTPUT_DEBUG_STRING_INFO, nDebugStringLength, 6, 2, 2);
}

static void test_pack_PACTCTXA(void)
{
    /* PACTCTXA */
    TEST_TYPE(PACTCTXA, 4, 4);
    TEST_TYPE_POINTER(PACTCTXA, 32, 4);
}

static void test_pack_PACTCTXW(void)
{
    /* PACTCTXW */
    TEST_TYPE(PACTCTXW, 4, 4);
    TEST_TYPE_POINTER(PACTCTXW, 32, 4);
}

static void test_pack_PACTCTX_SECTION_KEYED_DATA(void)
{
    /* PACTCTX_SECTION_KEYED_DATA */
    TEST_TYPE(PACTCTX_SECTION_KEYED_DATA, 4, 4);
    TEST_TYPE_POINTER(PACTCTX_SECTION_KEYED_DATA, 64, 4);
}

static void test_pack_PACTCTX_SECTION_KEYED_DATA_2600(void)
{
    /* PACTCTX_SECTION_KEYED_DATA_2600 */
    TEST_TYPE(PACTCTX_SECTION_KEYED_DATA_2600, 4, 4);
    TEST_TYPE_POINTER(PACTCTX_SECTION_KEYED_DATA_2600, 40, 4);
}

static void test_pack_PACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA(void)
{
    /* PACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA */
    TEST_TYPE(PACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA, 4, 4);
    TEST_TYPE_POINTER(PACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA, 20, 4);
}

static void test_pack_PACTIVATION_CONTEXT_BASIC_INFORMATION(void)
{
    /* PACTIVATION_CONTEXT_BASIC_INFORMATION */
    TEST_TYPE(PACTIVATION_CONTEXT_BASIC_INFORMATION, 4, 4);
    TEST_TYPE_POINTER(PACTIVATION_CONTEXT_BASIC_INFORMATION, 8, 4);
}

static void test_pack_PAPCFUNC(void)
{
    /* PAPCFUNC */
    TEST_TYPE(PAPCFUNC, 4, 4);
}

static void test_pack_PBY_HANDLE_FILE_INFORMATION(void)
{
    /* PBY_HANDLE_FILE_INFORMATION */
    TEST_TYPE(PBY_HANDLE_FILE_INFORMATION, 4, 4);
    TEST_TYPE_POINTER(PBY_HANDLE_FILE_INFORMATION, 52, 4);
}

static void test_pack_PCACTCTXA(void)
{
    /* PCACTCTXA */
    TEST_TYPE(PCACTCTXA, 4, 4);
    TEST_TYPE_POINTER(PCACTCTXA, 32, 4);
}

static void test_pack_PCACTCTXW(void)
{
    /* PCACTCTXW */
    TEST_TYPE(PCACTCTXW, 4, 4);
    TEST_TYPE_POINTER(PCACTCTXW, 32, 4);
}

static void test_pack_PCACTCTX_SECTION_KEYED_DATA(void)
{
    /* PCACTCTX_SECTION_KEYED_DATA */
    TEST_TYPE(PCACTCTX_SECTION_KEYED_DATA, 4, 4);
    TEST_TYPE_POINTER(PCACTCTX_SECTION_KEYED_DATA, 64, 4);
}

static void test_pack_PCACTCTX_SECTION_KEYED_DATA_2600(void)
{
    /* PCACTCTX_SECTION_KEYED_DATA_2600 */
    TEST_TYPE(PCACTCTX_SECTION_KEYED_DATA_2600, 4, 4);
    TEST_TYPE_POINTER(PCACTCTX_SECTION_KEYED_DATA_2600, 40, 4);
}

static void test_pack_PCACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA(void)
{
    /* PCACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA */
    TEST_TYPE(PCACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA, 4, 4);
    TEST_TYPE_POINTER(PCACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA, 20, 4);
}

static void test_pack_PCRITICAL_SECTION(void)
{
    /* PCRITICAL_SECTION */
    TEST_TYPE(PCRITICAL_SECTION, 4, 4);
}

static void test_pack_PCRITICAL_SECTION_DEBUG(void)
{
    /* PCRITICAL_SECTION_DEBUG */
    TEST_TYPE(PCRITICAL_SECTION_DEBUG, 4, 4);
}

static void test_pack_PFIBER_START_ROUTINE(void)
{
    /* PFIBER_START_ROUTINE */
    TEST_TYPE(PFIBER_START_ROUTINE, 4, 4);
}

static void test_pack_POFSTRUCT(void)
{
    /* POFSTRUCT */
    TEST_TYPE(POFSTRUCT, 4, 4);
    TEST_TYPE_POINTER(POFSTRUCT, 136, 2);
}

static void test_pack_PPROCESS_HEAP_ENTRY(void)
{
    /* PPROCESS_HEAP_ENTRY */
    TEST_TYPE(PPROCESS_HEAP_ENTRY, 4, 4);
}

static void test_pack_PPROCESS_INFORMATION(void)
{
    /* PPROCESS_INFORMATION */
    TEST_TYPE(PPROCESS_INFORMATION, 4, 4);
    TEST_TYPE_POINTER(PPROCESS_INFORMATION, 16, 4);
}

static void test_pack_PQUERYACTCTXW_FUNC(void)
{
    /* PQUERYACTCTXW_FUNC */
    TEST_TYPE(PQUERYACTCTXW_FUNC, 4, 4);
}

static void test_pack_PROCESS_HEAP_ENTRY(void)
{
    /* PROCESS_HEAP_ENTRY (pack 4) */
    TEST_FIELD(PROCESS_HEAP_ENTRY, lpData, 0, 4, 4);
    TEST_FIELD(PROCESS_HEAP_ENTRY, cbData, 4, 4, 4);
    TEST_FIELD(PROCESS_HEAP_ENTRY, cbOverhead, 8, 1, 1);
    TEST_FIELD(PROCESS_HEAP_ENTRY, iRegionIndex, 9, 1, 1);
    TEST_FIELD(PROCESS_HEAP_ENTRY, wFlags, 10, 2, 2);
}

static void test_pack_PROCESS_INFORMATION(void)
{
    /* PROCESS_INFORMATION (pack 4) */
    TEST_TYPE(PROCESS_INFORMATION, 16, 4);
    TEST_FIELD(PROCESS_INFORMATION, hProcess, 0, 4, 4);
    TEST_FIELD(PROCESS_INFORMATION, hThread, 4, 4, 4);
    TEST_FIELD(PROCESS_INFORMATION, dwProcessId, 8, 4, 4);
    TEST_FIELD(PROCESS_INFORMATION, dwThreadId, 12, 4, 4);
}

static void test_pack_PSECURITY_ATTRIBUTES(void)
{
    /* PSECURITY_ATTRIBUTES */
    TEST_TYPE(PSECURITY_ATTRIBUTES, 4, 4);
    TEST_TYPE_POINTER(PSECURITY_ATTRIBUTES, 12, 4);
}

static void test_pack_PSYSTEMTIME(void)
{
    /* PSYSTEMTIME */
    TEST_TYPE(PSYSTEMTIME, 4, 4);
    TEST_TYPE_POINTER(PSYSTEMTIME, 16, 2);
}

static void test_pack_PTIMERAPCROUTINE(void)
{
    /* PTIMERAPCROUTINE */
    TEST_TYPE(PTIMERAPCROUTINE, 4, 4);
}

static void test_pack_PTIME_ZONE_INFORMATION(void)
{
    /* PTIME_ZONE_INFORMATION */
    TEST_TYPE(PTIME_ZONE_INFORMATION, 4, 4);
    TEST_TYPE_POINTER(PTIME_ZONE_INFORMATION, 172, 4);
}

static void test_pack_PWIN32_FIND_DATAA(void)
{
    /* PWIN32_FIND_DATAA */
    TEST_TYPE(PWIN32_FIND_DATAA, 4, 4);
    TEST_TYPE_POINTER(PWIN32_FIND_DATAA, 320, 4);
}

static void test_pack_PWIN32_FIND_DATAW(void)
{
    /* PWIN32_FIND_DATAW */
    TEST_TYPE(PWIN32_FIND_DATAW, 4, 4);
    TEST_TYPE_POINTER(PWIN32_FIND_DATAW, 592, 4);
}

static void test_pack_RIP_INFO(void)
{
    /* RIP_INFO (pack 4) */
    TEST_TYPE(RIP_INFO, 8, 4);
    TEST_FIELD(RIP_INFO, dwError, 0, 4, 4);
    TEST_FIELD(RIP_INFO, dwType, 4, 4, 4);
}

static void test_pack_SECURITY_ATTRIBUTES(void)
{
    /* SECURITY_ATTRIBUTES (pack 4) */
    TEST_TYPE(SECURITY_ATTRIBUTES, 12, 4);
    TEST_FIELD(SECURITY_ATTRIBUTES, nLength, 0, 4, 4);
    TEST_FIELD(SECURITY_ATTRIBUTES, lpSecurityDescriptor, 4, 4, 4);
    TEST_FIELD(SECURITY_ATTRIBUTES, bInheritHandle, 8, 4, 4);
}

static void test_pack_STARTUPINFOA(void)
{
    /* STARTUPINFOA (pack 4) */
    TEST_TYPE(STARTUPINFOA, 68, 4);
    TEST_FIELD(STARTUPINFOA, cb, 0, 4, 4);
    TEST_FIELD(STARTUPINFOA, lpReserved, 4, 4, 4);
    TEST_FIELD(STARTUPINFOA, lpDesktop, 8, 4, 4);
    TEST_FIELD(STARTUPINFOA, lpTitle, 12, 4, 4);
    TEST_FIELD(STARTUPINFOA, dwX, 16, 4, 4);
    TEST_FIELD(STARTUPINFOA, dwY, 20, 4, 4);
    TEST_FIELD(STARTUPINFOA, dwXSize, 24, 4, 4);
    TEST_FIELD(STARTUPINFOA, dwYSize, 28, 4, 4);
    TEST_FIELD(STARTUPINFOA, dwXCountChars, 32, 4, 4);
    TEST_FIELD(STARTUPINFOA, dwYCountChars, 36, 4, 4);
    TEST_FIELD(STARTUPINFOA, dwFillAttribute, 40, 4, 4);
    TEST_FIELD(STARTUPINFOA, dwFlags, 44, 4, 4);
    TEST_FIELD(STARTUPINFOA, wShowWindow, 48, 2, 2);
    TEST_FIELD(STARTUPINFOA, cbReserved2, 50, 2, 2);
    TEST_FIELD(STARTUPINFOA, lpReserved2, 52, 4, 4);
    TEST_FIELD(STARTUPINFOA, hStdInput, 56, 4, 4);
    TEST_FIELD(STARTUPINFOA, hStdOutput, 60, 4, 4);
    TEST_FIELD(STARTUPINFOA, hStdError, 64, 4, 4);
}

static void test_pack_STARTUPINFOW(void)
{
    /* STARTUPINFOW (pack 4) */
    TEST_TYPE(STARTUPINFOW, 68, 4);
    TEST_FIELD(STARTUPINFOW, cb, 0, 4, 4);
    TEST_FIELD(STARTUPINFOW, lpReserved, 4, 4, 4);
    TEST_FIELD(STARTUPINFOW, lpDesktop, 8, 4, 4);
    TEST_FIELD(STARTUPINFOW, lpTitle, 12, 4, 4);
    TEST_FIELD(STARTUPINFOW, dwX, 16, 4, 4);
    TEST_FIELD(STARTUPINFOW, dwY, 20, 4, 4);
    TEST_FIELD(STARTUPINFOW, dwXSize, 24, 4, 4);
    TEST_FIELD(STARTUPINFOW, dwYSize, 28, 4, 4);
    TEST_FIELD(STARTUPINFOW, dwXCountChars, 32, 4, 4);
    TEST_FIELD(STARTUPINFOW, dwYCountChars, 36, 4, 4);
    TEST_FIELD(STARTUPINFOW, dwFillAttribute, 40, 4, 4);
    TEST_FIELD(STARTUPINFOW, dwFlags, 44, 4, 4);
    TEST_FIELD(STARTUPINFOW, wShowWindow, 48, 2, 2);
    TEST_FIELD(STARTUPINFOW, cbReserved2, 50, 2, 2);
    TEST_FIELD(STARTUPINFOW, lpReserved2, 52, 4, 4);
    TEST_FIELD(STARTUPINFOW, hStdInput, 56, 4, 4);
    TEST_FIELD(STARTUPINFOW, hStdOutput, 60, 4, 4);
    TEST_FIELD(STARTUPINFOW, hStdError, 64, 4, 4);
}

static void test_pack_SYSTEMTIME(void)
{
    /* SYSTEMTIME (pack 4) */
    TEST_TYPE(SYSTEMTIME, 16, 2);
    TEST_FIELD(SYSTEMTIME, wYear, 0, 2, 2);
    TEST_FIELD(SYSTEMTIME, wMonth, 2, 2, 2);
    TEST_FIELD(SYSTEMTIME, wDayOfWeek, 4, 2, 2);
    TEST_FIELD(SYSTEMTIME, wDay, 6, 2, 2);
    TEST_FIELD(SYSTEMTIME, wHour, 8, 2, 2);
    TEST_FIELD(SYSTEMTIME, wMinute, 10, 2, 2);
    TEST_FIELD(SYSTEMTIME, wSecond, 12, 2, 2);
    TEST_FIELD(SYSTEMTIME, wMilliseconds, 14, 2, 2);
}

static void test_pack_SYSTEM_INFO(void)
{
    /* SYSTEM_INFO (pack 4) */
}

static void test_pack_SYSTEM_POWER_STATUS(void)
{
    /* SYSTEM_POWER_STATUS (pack 4) */
    TEST_TYPE(SYSTEM_POWER_STATUS, 12, 4);
    TEST_FIELD(SYSTEM_POWER_STATUS, ACLineStatus, 0, 1, 1);
    TEST_FIELD(SYSTEM_POWER_STATUS, BatteryFlag, 1, 1, 1);
    TEST_FIELD(SYSTEM_POWER_STATUS, BatteryLifePercent, 2, 1, 1);
    TEST_FIELD(SYSTEM_POWER_STATUS, Reserved1, 3, 1, 1);
    TEST_FIELD(SYSTEM_POWER_STATUS, BatteryLifeTime, 4, 4, 4);
    TEST_FIELD(SYSTEM_POWER_STATUS, BatteryFullLifeTime, 8, 4, 4);
}

static void test_pack_TIME_ZONE_INFORMATION(void)
{
    /* TIME_ZONE_INFORMATION (pack 4) */
    TEST_TYPE(TIME_ZONE_INFORMATION, 172, 4);
    TEST_FIELD(TIME_ZONE_INFORMATION, Bias, 0, 4, 4);
    TEST_FIELD(TIME_ZONE_INFORMATION, StandardName, 4, 64, 2);
    TEST_FIELD(TIME_ZONE_INFORMATION, StandardDate, 68, 16, 2);
    TEST_FIELD(TIME_ZONE_INFORMATION, StandardBias, 84, 4, 4);
    TEST_FIELD(TIME_ZONE_INFORMATION, DaylightName, 88, 64, 2);
    TEST_FIELD(TIME_ZONE_INFORMATION, DaylightDate, 152, 16, 2);
    TEST_FIELD(TIME_ZONE_INFORMATION, DaylightBias, 168, 4, 4);
}

static void test_pack_UNLOAD_DLL_DEBUG_INFO(void)
{
    /* UNLOAD_DLL_DEBUG_INFO (pack 4) */
    TEST_TYPE(UNLOAD_DLL_DEBUG_INFO, 4, 4);
    TEST_FIELD(UNLOAD_DLL_DEBUG_INFO, lpBaseOfDll, 0, 4, 4);
}

static void test_pack_WAITORTIMERCALLBACK(void)
{
    /* WAITORTIMERCALLBACK */
    TEST_TYPE(WAITORTIMERCALLBACK, 4, 4);
}

static void test_pack_WIN32_FILE_ATTRIBUTE_DATA(void)
{
    /* WIN32_FILE_ATTRIBUTE_DATA (pack 4) */
    TEST_TYPE(WIN32_FILE_ATTRIBUTE_DATA, 36, 4);
    TEST_FIELD(WIN32_FILE_ATTRIBUTE_DATA, dwFileAttributes, 0, 4, 4);
    TEST_FIELD(WIN32_FILE_ATTRIBUTE_DATA, ftCreationTime, 4, 8, 4);
    TEST_FIELD(WIN32_FILE_ATTRIBUTE_DATA, ftLastAccessTime, 12, 8, 4);
    TEST_FIELD(WIN32_FILE_ATTRIBUTE_DATA, ftLastWriteTime, 20, 8, 4);
    TEST_FIELD(WIN32_FILE_ATTRIBUTE_DATA, nFileSizeHigh, 28, 4, 4);
    TEST_FIELD(WIN32_FILE_ATTRIBUTE_DATA, nFileSizeLow, 32, 4, 4);
}

static void test_pack_WIN32_FIND_DATAA(void)
{
    /* WIN32_FIND_DATAA (pack 4) */
    TEST_TYPE(WIN32_FIND_DATAA, 320, 4);
    TEST_FIELD(WIN32_FIND_DATAA, dwFileAttributes, 0, 4, 4);
    TEST_FIELD(WIN32_FIND_DATAA, ftCreationTime, 4, 8, 4);
    TEST_FIELD(WIN32_FIND_DATAA, ftLastAccessTime, 12, 8, 4);
    TEST_FIELD(WIN32_FIND_DATAA, ftLastWriteTime, 20, 8, 4);
    TEST_FIELD(WIN32_FIND_DATAA, nFileSizeHigh, 28, 4, 4);
    TEST_FIELD(WIN32_FIND_DATAA, nFileSizeLow, 32, 4, 4);
    TEST_FIELD(WIN32_FIND_DATAA, dwReserved0, 36, 4, 4);
    TEST_FIELD(WIN32_FIND_DATAA, dwReserved1, 40, 4, 4);
    TEST_FIELD(WIN32_FIND_DATAA, cFileName, 44, 260, 1);
    TEST_FIELD(WIN32_FIND_DATAA, cAlternateFileName, 304, 14, 1);
}

static void test_pack_WIN32_FIND_DATAW(void)
{
    /* WIN32_FIND_DATAW (pack 4) */
    TEST_TYPE(WIN32_FIND_DATAW, 592, 4);
    TEST_FIELD(WIN32_FIND_DATAW, dwFileAttributes, 0, 4, 4);
    TEST_FIELD(WIN32_FIND_DATAW, ftCreationTime, 4, 8, 4);
    TEST_FIELD(WIN32_FIND_DATAW, ftLastAccessTime, 12, 8, 4);
    TEST_FIELD(WIN32_FIND_DATAW, ftLastWriteTime, 20, 8, 4);
    TEST_FIELD(WIN32_FIND_DATAW, nFileSizeHigh, 28, 4, 4);
    TEST_FIELD(WIN32_FIND_DATAW, nFileSizeLow, 32, 4, 4);
    TEST_FIELD(WIN32_FIND_DATAW, dwReserved0, 36, 4, 4);
    TEST_FIELD(WIN32_FIND_DATAW, dwReserved1, 40, 4, 4);
    TEST_FIELD(WIN32_FIND_DATAW, cFileName, 44, 520, 2);
    TEST_FIELD(WIN32_FIND_DATAW, cAlternateFileName, 564, 28, 2);
}

static void test_pack_WIN32_STREAM_ID(void)
{
    /* WIN32_STREAM_ID (pack 8) */
    TEST_TYPE(WIN32_STREAM_ID, 24, 8);
    TEST_FIELD(WIN32_STREAM_ID, dwStreamId, 0, 4, 4);
    TEST_FIELD(WIN32_STREAM_ID, dwStreamAttributes, 4, 4, 4);
    TEST_FIELD(WIN32_STREAM_ID, Size, 8, 8, 8);
    TEST_FIELD(WIN32_STREAM_ID, dwStreamNameSize, 16, 4, 4);
    TEST_FIELD(WIN32_STREAM_ID, cStreamName, 20, 2, 2);
}

static void test_pack(void)
{
    test_pack_ACTCTXA();
    test_pack_ACTCTXW();
    test_pack_ACTCTX_SECTION_KEYED_DATA();
    test_pack_ACTCTX_SECTION_KEYED_DATA_2600();
    test_pack_ACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA();
    test_pack_ACTIVATION_CONTEXT_BASIC_INFORMATION();
    test_pack_BY_HANDLE_FILE_INFORMATION();
    test_pack_COMMCONFIG();
    test_pack_COMMPROP();
    test_pack_COMMTIMEOUTS();
    test_pack_COMSTAT();
    test_pack_CREATE_PROCESS_DEBUG_INFO();
    test_pack_CREATE_THREAD_DEBUG_INFO();
    test_pack_CRITICAL_SECTION();
    test_pack_CRITICAL_SECTION_DEBUG();
    test_pack_DCB();
    test_pack_DEBUG_EVENT();
    test_pack_ENUMRESLANGPROCA();
    test_pack_ENUMRESLANGPROCW();
    test_pack_ENUMRESNAMEPROCA();
    test_pack_ENUMRESNAMEPROCW();
    test_pack_ENUMRESTYPEPROCA();
    test_pack_ENUMRESTYPEPROCW();
    test_pack_EXCEPTION_DEBUG_INFO();
    test_pack_EXIT_PROCESS_DEBUG_INFO();
    test_pack_EXIT_THREAD_DEBUG_INFO();
    test_pack_HW_PROFILE_INFOA();
    test_pack_HW_PROFILE_INFOW();
    test_pack_LOAD_DLL_DEBUG_INFO();
    test_pack_LPBY_HANDLE_FILE_INFORMATION();
    test_pack_LPCOMMCONFIG();
    test_pack_LPCOMMPROP();
    test_pack_LPCOMMTIMEOUTS();
    test_pack_LPCOMSTAT();
    test_pack_LPCRITICAL_SECTION();
    test_pack_LPCRITICAL_SECTION_DEBUG();
    test_pack_LPDCB();
    test_pack_LPDEBUG_EVENT();
    test_pack_LPEXCEPTION_POINTERS();
    test_pack_LPEXCEPTION_RECORD();
    test_pack_LPFIBER_START_ROUTINE();
    test_pack_LPHW_PROFILE_INFOA();
    test_pack_LPHW_PROFILE_INFOW();
    test_pack_LPLONG();
    test_pack_LPMEMORYSTATUS();
    test_pack_LPMEMORYSTATUSEX();
    test_pack_LPOFSTRUCT();
    test_pack_LPOSVERSIONINFOA();
    test_pack_LPOSVERSIONINFOEXA();
    test_pack_LPOSVERSIONINFOEXW();
    test_pack_LPOSVERSIONINFOW();
    test_pack_LPOVERLAPPED();
    test_pack_LPOVERLAPPED_COMPLETION_ROUTINE();
    test_pack_LPPROCESS_HEAP_ENTRY();
    test_pack_LPPROCESS_INFORMATION();
    test_pack_LPPROGRESS_ROUTINE();
    test_pack_LPSECURITY_ATTRIBUTES();
    test_pack_LPSTARTUPINFOA();
    test_pack_LPSTARTUPINFOW();
    test_pack_LPSYSTEMTIME();
    test_pack_LPSYSTEM_INFO();
    test_pack_LPSYSTEM_POWER_STATUS();
    test_pack_LPTHREAD_START_ROUTINE();
    test_pack_LPTIME_ZONE_INFORMATION();
    test_pack_LPVOID();
    test_pack_LPWIN32_FILE_ATTRIBUTE_DATA();
    test_pack_LPWIN32_FIND_DATAA();
    test_pack_LPWIN32_FIND_DATAW();
    test_pack_LPWIN32_STREAM_ID();
    test_pack_MEMORYSTATUS();
    test_pack_MEMORYSTATUSEX();
    test_pack_OFSTRUCT();
    test_pack_OSVERSIONINFOA();
    test_pack_OSVERSIONINFOEXA();
    test_pack_OSVERSIONINFOEXW();
    test_pack_OSVERSIONINFOW();
    test_pack_OUTPUT_DEBUG_STRING_INFO();
    test_pack_PACTCTXA();
    test_pack_PACTCTXW();
    test_pack_PACTCTX_SECTION_KEYED_DATA();
    test_pack_PACTCTX_SECTION_KEYED_DATA_2600();
    test_pack_PACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA();
    test_pack_PACTIVATION_CONTEXT_BASIC_INFORMATION();
    test_pack_PAPCFUNC();
    test_pack_PBY_HANDLE_FILE_INFORMATION();
    test_pack_PCACTCTXA();
    test_pack_PCACTCTXW();
    test_pack_PCACTCTX_SECTION_KEYED_DATA();
    test_pack_PCACTCTX_SECTION_KEYED_DATA_2600();
    test_pack_PCACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA();
    test_pack_PCRITICAL_SECTION();
    test_pack_PCRITICAL_SECTION_DEBUG();
    test_pack_PFIBER_START_ROUTINE();
    test_pack_PHKEY();
    test_pack_POFSTRUCT();
    test_pack_POSVERSIONINFOA();
    test_pack_POSVERSIONINFOEXA();
    test_pack_POSVERSIONINFOEXW();
    test_pack_POSVERSIONINFOW();
    test_pack_PPROCESS_HEAP_ENTRY();
    test_pack_PPROCESS_INFORMATION();
    test_pack_PQUERYACTCTXW_FUNC();
    test_pack_PROCESS_HEAP_ENTRY();
    test_pack_PROCESS_INFORMATION();
    test_pack_PSECURITY_ATTRIBUTES();
    test_pack_PSYSTEMTIME();
    test_pack_PTIMERAPCROUTINE();
    test_pack_PTIME_ZONE_INFORMATION();
    test_pack_PWIN32_FIND_DATAA();
    test_pack_PWIN32_FIND_DATAW();
    test_pack_RIP_INFO();
    test_pack_SECURITY_ATTRIBUTES();
    test_pack_STARTUPINFOA();
    test_pack_STARTUPINFOW();
    test_pack_SYSTEMTIME();
    test_pack_SYSTEM_INFO();
    test_pack_SYSTEM_POWER_STATUS();
    test_pack_TIME_ZONE_INFORMATION();
    test_pack_UNLOAD_DLL_DEBUG_INFO();
    test_pack_WAITORTIMERCALLBACK();
    test_pack_WIN32_FILE_ATTRIBUTE_DATA();
    test_pack_WIN32_FIND_DATAA();
    test_pack_WIN32_FIND_DATAW();
    test_pack_WIN32_STREAM_ID();
}

START_TEST(generated)
{
    test_pack();
}
