/* File generated automatically from tools/winapi/test.dat; do not edit! */
/* This file can be copied, modified and distributed without restriction. */

/*
 * Unit tests for data structure packing
 */

#define WINVER 0x0501
#define WINE_NOWINSOCK

#include "windows.h"

#include "wine/test.h"

/***********************************************************************
 * Windows API extension
 */

#if (_MSC_VER >= 1300) && defined(__cplusplus)
# define FIELD_ALIGNMENT(type, field) __alignof(((type*)0)->field)
#elif defined(__GNUC__)
# define FIELD_ALIGNMENT(type, field) __alignof__(((type*)0)->field)
#else
/* FIXME: Not sure if is possible to do without compiler extension */
#endif

/***********************************************************************
 * Test helper macros
 */

#ifdef FIELD_ALIGNMENT
# define TEST_FIELD_ALIGNMENT(type, field, align) \
   ok(FIELD_ALIGNMENT(type, field) == align, \
       "FIELD_ALIGNMENT(" #type ", " #field ") == %d (expected " #align ")", \
           FIELD_ALIGNMENT(type, field))
#else
# define TEST_FIELD_ALIGNMENT(type, field, align) do { } while (0)
#endif

#define TEST_FIELD_OFFSET(type, field, offset) \
    ok(FIELD_OFFSET(type, field) == offset, \
        "FIELD_OFFSET(" #type ", " #field ") == %ld (expected " #offset ")", \
             FIELD_OFFSET(type, field))

#define TEST_TYPE_ALIGNMENT(type, align) \
    ok(TYPE_ALIGNMENT(type) == align, "TYPE_ALIGNMENT(" #type ") == %d (expected " #align ")", TYPE_ALIGNMENT(type))

#define TEST_TYPE_SIZE(type, size) \
    ok(sizeof(type) == size, "sizeof(" #type ") == %d (expected " #size ")", sizeof(type))

/***********************************************************************
 * Test macros
 */

#define TEST_FIELD(type, field_type, field_name, field_offset, field_size, field_align) \
  TEST_TYPE_SIZE(field_type, field_size); \
  TEST_FIELD_ALIGNMENT(type, field_name, field_align); \
  TEST_FIELD_OFFSET(type, field_name, field_offset); \

#define TEST_TYPE(type, size, align) \
  TEST_TYPE_ALIGNMENT(type, align); \
  TEST_TYPE_SIZE(type, size)

void test_pack(void)
{
    /* BY_HANDLE_FILE_INFORMATION (pack 4) */
    TEST_TYPE(BY_HANDLE_FILE_INFORMATION, 52, 4);
    TEST_FIELD(BY_HANDLE_FILE_INFORMATION, DWORD, dwFileAttributes, 0, 4, 4);
    TEST_FIELD(BY_HANDLE_FILE_INFORMATION, FILETIME, ftCreationTime, 4, 8, 4);
    TEST_FIELD(BY_HANDLE_FILE_INFORMATION, FILETIME, ftLastAccessTime, 12, 8, 4);
    TEST_FIELD(BY_HANDLE_FILE_INFORMATION, FILETIME, ftLastWriteTime, 20, 8, 4);
    TEST_FIELD(BY_HANDLE_FILE_INFORMATION, DWORD, dwVolumeSerialNumber, 28, 4, 4);
    TEST_FIELD(BY_HANDLE_FILE_INFORMATION, DWORD, nFileSizeHigh, 32, 4, 4);
    TEST_FIELD(BY_HANDLE_FILE_INFORMATION, DWORD, nFileSizeLow, 36, 4, 4);
    TEST_FIELD(BY_HANDLE_FILE_INFORMATION, DWORD, nNumberOfLinks, 40, 4, 4);
    TEST_FIELD(BY_HANDLE_FILE_INFORMATION, DWORD, nFileIndexHigh, 44, 4, 4);
    TEST_FIELD(BY_HANDLE_FILE_INFORMATION, DWORD, nFileIndexLow, 48, 4, 4);

    /* COMMCONFIG (pack 4) */
    TEST_TYPE(COMMCONFIG, 52, 4);
    TEST_FIELD(COMMCONFIG, DWORD, dwSize, 0, 4, 4);
    TEST_FIELD(COMMCONFIG, WORD, wVersion, 4, 2, 2);
    TEST_FIELD(COMMCONFIG, WORD, wReserved, 6, 2, 2);
    TEST_FIELD(COMMCONFIG, DCB, dcb, 8, 28, 4);
    TEST_FIELD(COMMCONFIG, DWORD, dwProviderSubType, 36, 4, 4);
    TEST_FIELD(COMMCONFIG, DWORD, dwProviderOffset, 40, 4, 4);
    TEST_FIELD(COMMCONFIG, DWORD, dwProviderSize, 44, 4, 4);
    TEST_FIELD(COMMCONFIG, DWORD[1], wcProviderData, 48, 4, 4);

    /* COMMPROP (pack 4) */
    TEST_TYPE(COMMPROP, 64, 4);
    TEST_FIELD(COMMPROP, WORD, wPacketLength, 0, 2, 2);
    TEST_FIELD(COMMPROP, WORD, wPacketVersion, 2, 2, 2);
    TEST_FIELD(COMMPROP, DWORD, dwServiceMask, 4, 4, 4);
    TEST_FIELD(COMMPROP, DWORD, dwReserved1, 8, 4, 4);
    TEST_FIELD(COMMPROP, DWORD, dwMaxTxQueue, 12, 4, 4);
    TEST_FIELD(COMMPROP, DWORD, dwMaxRxQueue, 16, 4, 4);
    TEST_FIELD(COMMPROP, DWORD, dwMaxBaud, 20, 4, 4);
    TEST_FIELD(COMMPROP, DWORD, dwProvSubType, 24, 4, 4);
    TEST_FIELD(COMMPROP, DWORD, dwProvCapabilities, 28, 4, 4);
    TEST_FIELD(COMMPROP, DWORD, dwSettableParams, 32, 4, 4);
    TEST_FIELD(COMMPROP, DWORD, dwSettableBaud, 36, 4, 4);
    TEST_FIELD(COMMPROP, WORD, wSettableData, 40, 2, 2);
    TEST_FIELD(COMMPROP, WORD, wSettableStopParity, 42, 2, 2);
    TEST_FIELD(COMMPROP, DWORD, dwCurrentTxQueue, 44, 4, 4);
    TEST_FIELD(COMMPROP, DWORD, dwCurrentRxQueue, 48, 4, 4);
    TEST_FIELD(COMMPROP, DWORD, dwProvSpec1, 52, 4, 4);
    TEST_FIELD(COMMPROP, DWORD, dwProvSpec2, 56, 4, 4);
    TEST_FIELD(COMMPROP, WCHAR[1], wcProvChar, 60, 2, 2);

    /* COMMTIMEOUTS (pack 4) */
    TEST_TYPE(COMMTIMEOUTS, 20, 4);
    TEST_FIELD(COMMTIMEOUTS, DWORD, ReadIntervalTimeout, 0, 4, 4);
    TEST_FIELD(COMMTIMEOUTS, DWORD, ReadTotalTimeoutMultiplier, 4, 4, 4);
    TEST_FIELD(COMMTIMEOUTS, DWORD, ReadTotalTimeoutConstant, 8, 4, 4);
    TEST_FIELD(COMMTIMEOUTS, DWORD, WriteTotalTimeoutMultiplier, 12, 4, 4);
    TEST_FIELD(COMMTIMEOUTS, DWORD, WriteTotalTimeoutConstant, 16, 4, 4);

    /* COMSTAT (pack 4) */
    TEST_TYPE(COMSTAT, 12, 4);
    TEST_FIELD(COMSTAT, DWORD, cbInQue, 4, 4, 4);
    TEST_FIELD(COMSTAT, DWORD, cbOutQue, 8, 4, 4);

    /* CREATE_PROCESS_DEBUG_INFO (pack 4) */
    TEST_TYPE(CREATE_PROCESS_DEBUG_INFO, 40, 4);
    TEST_FIELD(CREATE_PROCESS_DEBUG_INFO, HANDLE, hFile, 0, 4, 4);
    TEST_FIELD(CREATE_PROCESS_DEBUG_INFO, HANDLE, hProcess, 4, 4, 4);
    TEST_FIELD(CREATE_PROCESS_DEBUG_INFO, HANDLE, hThread, 8, 4, 4);
    TEST_FIELD(CREATE_PROCESS_DEBUG_INFO, LPVOID, lpBaseOfImage, 12, 4, 4);
    TEST_FIELD(CREATE_PROCESS_DEBUG_INFO, DWORD, dwDebugInfoFileOffset, 16, 4, 4);
    TEST_FIELD(CREATE_PROCESS_DEBUG_INFO, DWORD, nDebugInfoSize, 20, 4, 4);
    TEST_FIELD(CREATE_PROCESS_DEBUG_INFO, LPVOID, lpThreadLocalBase, 24, 4, 4);
    TEST_FIELD(CREATE_PROCESS_DEBUG_INFO, LPTHREAD_START_ROUTINE, lpStartAddress, 28, 4, 4);
    TEST_FIELD(CREATE_PROCESS_DEBUG_INFO, LPVOID, lpImageName, 32, 4, 4);
    TEST_FIELD(CREATE_PROCESS_DEBUG_INFO, WORD, fUnicode, 36, 2, 2);

    /* CREATE_THREAD_DEBUG_INFO (pack 4) */
    TEST_TYPE(CREATE_THREAD_DEBUG_INFO, 12, 4);
    TEST_FIELD(CREATE_THREAD_DEBUG_INFO, HANDLE, hThread, 0, 4, 4);
    TEST_FIELD(CREATE_THREAD_DEBUG_INFO, LPVOID, lpThreadLocalBase, 4, 4, 4);
    TEST_FIELD(CREATE_THREAD_DEBUG_INFO, LPTHREAD_START_ROUTINE, lpStartAddress, 8, 4, 4);

    /* DCB (pack 4) */
    TEST_TYPE(DCB, 28, 4);
    TEST_FIELD(DCB, DWORD, DCBlength, 0, 4, 4);
    TEST_FIELD(DCB, DWORD, BaudRate, 4, 4, 4);
    TEST_FIELD(DCB, WORD, wReserved, 12, 2, 2);
    TEST_FIELD(DCB, WORD, XonLim, 14, 2, 2);
    TEST_FIELD(DCB, WORD, XoffLim, 16, 2, 2);
    TEST_FIELD(DCB, BYTE, ByteSize, 18, 1, 1);
    TEST_FIELD(DCB, BYTE, Parity, 19, 1, 1);
    TEST_FIELD(DCB, BYTE, StopBits, 20, 1, 1);
    TEST_FIELD(DCB, char, XonChar, 21, 1, 1);
    TEST_FIELD(DCB, char, XoffChar, 22, 1, 1);
    TEST_FIELD(DCB, char, ErrorChar, 23, 1, 1);
    TEST_FIELD(DCB, char, EofChar, 24, 1, 1);
    TEST_FIELD(DCB, char, EvtChar, 25, 1, 1);

    /* DEBUG_EVENT (pack 4) */
    TEST_FIELD(DEBUG_EVENT, DWORD, dwDebugEventCode, 0, 4, 4);
    TEST_FIELD(DEBUG_EVENT, DWORD, dwProcessId, 4, 4, 4);
    TEST_FIELD(DEBUG_EVENT, DWORD, dwThreadId, 8, 4, 4);

    /* EXCEPTION_DEBUG_INFO (pack 4) */
    TEST_TYPE(EXCEPTION_DEBUG_INFO, 84, 4);
    TEST_FIELD(EXCEPTION_DEBUG_INFO, EXCEPTION_RECORD, ExceptionRecord, 0, 80, 4);
    TEST_FIELD(EXCEPTION_DEBUG_INFO, DWORD, dwFirstChance, 80, 4, 4);

    /* EXIT_PROCESS_DEBUG_INFO (pack 4) */
    TEST_TYPE(EXIT_PROCESS_DEBUG_INFO, 4, 4);
    TEST_FIELD(EXIT_PROCESS_DEBUG_INFO, DWORD, dwExitCode, 0, 4, 4);

    /* EXIT_THREAD_DEBUG_INFO (pack 4) */
    TEST_TYPE(EXIT_THREAD_DEBUG_INFO, 4, 4);
    TEST_FIELD(EXIT_THREAD_DEBUG_INFO, DWORD, dwExitCode, 0, 4, 4);

    /* HW_PROFILE_INFOA (pack 4) */
    TEST_TYPE(HW_PROFILE_INFOA, 124, 4);
    TEST_FIELD(HW_PROFILE_INFOA, DWORD, dwDockInfo, 0, 4, 4);
    TEST_FIELD(HW_PROFILE_INFOA, CHAR[HW_PROFILE_GUIDLEN], szHwProfileGuid, 4, 39, 1);
    TEST_FIELD(HW_PROFILE_INFOA, CHAR[MAX_PROFILE_LEN], szHwProfileName, 43, 80, 1);

    /* LDT_ENTRY (pack 4) */
    TEST_FIELD(LDT_ENTRY, WORD, LimitLow, 0, 2, 2);
    TEST_FIELD(LDT_ENTRY, WORD, BaseLow, 2, 2, 2);

    /* LOAD_DLL_DEBUG_INFO (pack 4) */
    TEST_TYPE(LOAD_DLL_DEBUG_INFO, 24, 4);
    TEST_FIELD(LOAD_DLL_DEBUG_INFO, HANDLE, hFile, 0, 4, 4);
    TEST_FIELD(LOAD_DLL_DEBUG_INFO, LPVOID, lpBaseOfDll, 4, 4, 4);
    TEST_FIELD(LOAD_DLL_DEBUG_INFO, DWORD, dwDebugInfoFileOffset, 8, 4, 4);
    TEST_FIELD(LOAD_DLL_DEBUG_INFO, DWORD, nDebugInfoSize, 12, 4, 4);
    TEST_FIELD(LOAD_DLL_DEBUG_INFO, LPVOID, lpImageName, 16, 4, 4);
    TEST_FIELD(LOAD_DLL_DEBUG_INFO, WORD, fUnicode, 20, 2, 2);

    /* MEMORYSTATUS (pack 4) */
    TEST_TYPE(MEMORYSTATUS, 32, 4);
    TEST_FIELD(MEMORYSTATUS, DWORD, dwLength, 0, 4, 4);
    TEST_FIELD(MEMORYSTATUS, DWORD, dwMemoryLoad, 4, 4, 4);
    TEST_FIELD(MEMORYSTATUS, SIZE_T, dwTotalPhys, 8, 4, 4);
    TEST_FIELD(MEMORYSTATUS, SIZE_T, dwAvailPhys, 12, 4, 4);
    TEST_FIELD(MEMORYSTATUS, SIZE_T, dwTotalPageFile, 16, 4, 4);
    TEST_FIELD(MEMORYSTATUS, SIZE_T, dwAvailPageFile, 20, 4, 4);
    TEST_FIELD(MEMORYSTATUS, SIZE_T, dwTotalVirtual, 24, 4, 4);
    TEST_FIELD(MEMORYSTATUS, SIZE_T, dwAvailVirtual, 28, 4, 4);

    /* OFSTRUCT (pack 4) */
    TEST_TYPE(OFSTRUCT, 136, 2);
    TEST_FIELD(OFSTRUCT, BYTE, cBytes, 0, 1, 1);
    TEST_FIELD(OFSTRUCT, BYTE, fFixedDisk, 1, 1, 1);
    TEST_FIELD(OFSTRUCT, WORD, nErrCode, 2, 2, 2);
    TEST_FIELD(OFSTRUCT, WORD, Reserved1, 4, 2, 2);
    TEST_FIELD(OFSTRUCT, WORD, Reserved2, 6, 2, 2);
    TEST_FIELD(OFSTRUCT, BYTE[OFS_MAXPATHNAME], szPathName, 8, 128, 1);

    /* OSVERSIONINFOA (pack 4) */
    TEST_TYPE(OSVERSIONINFOA, 148, 4);
    TEST_FIELD(OSVERSIONINFOA, DWORD, dwOSVersionInfoSize, 0, 4, 4);
    TEST_FIELD(OSVERSIONINFOA, DWORD, dwMajorVersion, 4, 4, 4);
    TEST_FIELD(OSVERSIONINFOA, DWORD, dwMinorVersion, 8, 4, 4);
    TEST_FIELD(OSVERSIONINFOA, DWORD, dwBuildNumber, 12, 4, 4);
    TEST_FIELD(OSVERSIONINFOA, DWORD, dwPlatformId, 16, 4, 4);
    TEST_FIELD(OSVERSIONINFOA, CHAR[128], szCSDVersion, 20, 128, 1);

    /* OSVERSIONINFOEXA (pack 4) */
    TEST_TYPE(OSVERSIONINFOEXA, 156, 4);
    TEST_FIELD(OSVERSIONINFOEXA, DWORD, dwOSVersionInfoSize, 0, 4, 4);
    TEST_FIELD(OSVERSIONINFOEXA, DWORD, dwMajorVersion, 4, 4, 4);
    TEST_FIELD(OSVERSIONINFOEXA, DWORD, dwMinorVersion, 8, 4, 4);
    TEST_FIELD(OSVERSIONINFOEXA, DWORD, dwBuildNumber, 12, 4, 4);
    TEST_FIELD(OSVERSIONINFOEXA, DWORD, dwPlatformId, 16, 4, 4);
    TEST_FIELD(OSVERSIONINFOEXA, CHAR[128], szCSDVersion, 20, 128, 1);
    TEST_FIELD(OSVERSIONINFOEXA, WORD, wServicePackMajor, 148, 2, 2);
    TEST_FIELD(OSVERSIONINFOEXA, WORD, wServicePackMinor, 150, 2, 2);
    TEST_FIELD(OSVERSIONINFOEXA, WORD, wSuiteMask, 152, 2, 2);
    TEST_FIELD(OSVERSIONINFOEXA, BYTE, wProductType, 154, 1, 1);
    TEST_FIELD(OSVERSIONINFOEXA, BYTE, wReserved, 155, 1, 1);

    /* OSVERSIONINFOEXW (pack 4) */
    TEST_TYPE(OSVERSIONINFOEXW, 284, 4);
    TEST_FIELD(OSVERSIONINFOEXW, DWORD, dwOSVersionInfoSize, 0, 4, 4);
    TEST_FIELD(OSVERSIONINFOEXW, DWORD, dwMajorVersion, 4, 4, 4);
    TEST_FIELD(OSVERSIONINFOEXW, DWORD, dwMinorVersion, 8, 4, 4);
    TEST_FIELD(OSVERSIONINFOEXW, DWORD, dwBuildNumber, 12, 4, 4);
    TEST_FIELD(OSVERSIONINFOEXW, DWORD, dwPlatformId, 16, 4, 4);
    TEST_FIELD(OSVERSIONINFOEXW, WCHAR[128], szCSDVersion, 20, 256, 2);
    TEST_FIELD(OSVERSIONINFOEXW, WORD, wServicePackMajor, 276, 2, 2);
    TEST_FIELD(OSVERSIONINFOEXW, WORD, wServicePackMinor, 278, 2, 2);
    TEST_FIELD(OSVERSIONINFOEXW, WORD, wSuiteMask, 280, 2, 2);
    TEST_FIELD(OSVERSIONINFOEXW, BYTE, wProductType, 282, 1, 1);
    TEST_FIELD(OSVERSIONINFOEXW, BYTE, wReserved, 283, 1, 1);

    /* OSVERSIONINFOW (pack 4) */
    TEST_TYPE(OSVERSIONINFOW, 276, 4);
    TEST_FIELD(OSVERSIONINFOW, DWORD, dwOSVersionInfoSize, 0, 4, 4);
    TEST_FIELD(OSVERSIONINFOW, DWORD, dwMajorVersion, 4, 4, 4);
    TEST_FIELD(OSVERSIONINFOW, DWORD, dwMinorVersion, 8, 4, 4);
    TEST_FIELD(OSVERSIONINFOW, DWORD, dwBuildNumber, 12, 4, 4);
    TEST_FIELD(OSVERSIONINFOW, DWORD, dwPlatformId, 16, 4, 4);
    TEST_FIELD(OSVERSIONINFOW, WCHAR[128], szCSDVersion, 20, 256, 2);

    /* OUTPUT_DEBUG_STRING_INFO (pack 4) */
    TEST_TYPE(OUTPUT_DEBUG_STRING_INFO, 8, 4);
    TEST_FIELD(OUTPUT_DEBUG_STRING_INFO, LPSTR, lpDebugStringData, 0, 4, 4);
    TEST_FIELD(OUTPUT_DEBUG_STRING_INFO, WORD, fUnicode, 4, 2, 2);
    TEST_FIELD(OUTPUT_DEBUG_STRING_INFO, WORD, nDebugStringLength, 6, 2, 2);

    /* OVERLAPPED (pack 4) */
    TEST_TYPE(OVERLAPPED, 20, 4);
    TEST_FIELD(OVERLAPPED, DWORD, Internal, 0, 4, 4);
    TEST_FIELD(OVERLAPPED, DWORD, InternalHigh, 4, 4, 4);
    TEST_FIELD(OVERLAPPED, DWORD, Offset, 8, 4, 4);
    TEST_FIELD(OVERLAPPED, DWORD, OffsetHigh, 12, 4, 4);
    TEST_FIELD(OVERLAPPED, HANDLE, hEvent, 16, 4, 4);

    /* PROCESS_HEAP_ENTRY (pack 4) */
    TEST_FIELD(PROCESS_HEAP_ENTRY, LPVOID, lpData, 0, 4, 4);
    TEST_FIELD(PROCESS_HEAP_ENTRY, DWORD, cbData, 4, 4, 4);
    TEST_FIELD(PROCESS_HEAP_ENTRY, BYTE, cbOverhead, 8, 1, 1);
    TEST_FIELD(PROCESS_HEAP_ENTRY, BYTE, iRegionIndex, 9, 1, 1);
    TEST_FIELD(PROCESS_HEAP_ENTRY, WORD, wFlags, 10, 2, 2);

    /* PROCESS_INFORMATION (pack 4) */
    TEST_TYPE(PROCESS_INFORMATION, 16, 4);
    TEST_FIELD(PROCESS_INFORMATION, HANDLE, hProcess, 0, 4, 4);
    TEST_FIELD(PROCESS_INFORMATION, HANDLE, hThread, 4, 4, 4);
    TEST_FIELD(PROCESS_INFORMATION, DWORD, dwProcessId, 8, 4, 4);
    TEST_FIELD(PROCESS_INFORMATION, DWORD, dwThreadId, 12, 4, 4);

    /* RIP_INFO (pack 4) */
    TEST_TYPE(RIP_INFO, 8, 4);
    TEST_FIELD(RIP_INFO, DWORD, dwError, 0, 4, 4);
    TEST_FIELD(RIP_INFO, DWORD, dwType, 4, 4, 4);

    /* SECURITY_ATTRIBUTES (pack 4) */
    TEST_TYPE(SECURITY_ATTRIBUTES, 12, 4);
    TEST_FIELD(SECURITY_ATTRIBUTES, DWORD, nLength, 0, 4, 4);
    TEST_FIELD(SECURITY_ATTRIBUTES, LPVOID, lpSecurityDescriptor, 4, 4, 4);
    TEST_FIELD(SECURITY_ATTRIBUTES, BOOL, bInheritHandle, 8, 4, 4);

    /* STARTUPINFOA (pack 4) */
    TEST_TYPE(STARTUPINFOA, 68, 4);
    TEST_FIELD(STARTUPINFOA, DWORD, cb, 0, 4, 4);
    TEST_FIELD(STARTUPINFOA, LPSTR, lpReserved, 4, 4, 4);
    TEST_FIELD(STARTUPINFOA, LPSTR, lpDesktop, 8, 4, 4);
    TEST_FIELD(STARTUPINFOA, LPSTR, lpTitle, 12, 4, 4);
    TEST_FIELD(STARTUPINFOA, DWORD, dwX, 16, 4, 4);
    TEST_FIELD(STARTUPINFOA, DWORD, dwY, 20, 4, 4);
    TEST_FIELD(STARTUPINFOA, DWORD, dwXSize, 24, 4, 4);
    TEST_FIELD(STARTUPINFOA, DWORD, dwYSize, 28, 4, 4);
    TEST_FIELD(STARTUPINFOA, DWORD, dwXCountChars, 32, 4, 4);
    TEST_FIELD(STARTUPINFOA, DWORD, dwYCountChars, 36, 4, 4);
    TEST_FIELD(STARTUPINFOA, DWORD, dwFillAttribute, 40, 4, 4);
    TEST_FIELD(STARTUPINFOA, DWORD, dwFlags, 44, 4, 4);
    TEST_FIELD(STARTUPINFOA, WORD, wShowWindow, 48, 2, 2);
    TEST_FIELD(STARTUPINFOA, WORD, cbReserved2, 50, 2, 2);
    TEST_FIELD(STARTUPINFOA, BYTE *, lpReserved2, 52, 4, 4);
    TEST_FIELD(STARTUPINFOA, HANDLE, hStdInput, 56, 4, 4);
    TEST_FIELD(STARTUPINFOA, HANDLE, hStdOutput, 60, 4, 4);
    TEST_FIELD(STARTUPINFOA, HANDLE, hStdError, 64, 4, 4);

    /* STARTUPINFOW (pack 4) */
    TEST_TYPE(STARTUPINFOW, 68, 4);
    TEST_FIELD(STARTUPINFOW, DWORD, cb, 0, 4, 4);
    TEST_FIELD(STARTUPINFOW, LPWSTR, lpReserved, 4, 4, 4);
    TEST_FIELD(STARTUPINFOW, LPWSTR, lpDesktop, 8, 4, 4);
    TEST_FIELD(STARTUPINFOW, LPWSTR, lpTitle, 12, 4, 4);
    TEST_FIELD(STARTUPINFOW, DWORD, dwX, 16, 4, 4);
    TEST_FIELD(STARTUPINFOW, DWORD, dwY, 20, 4, 4);
    TEST_FIELD(STARTUPINFOW, DWORD, dwXSize, 24, 4, 4);
    TEST_FIELD(STARTUPINFOW, DWORD, dwYSize, 28, 4, 4);
    TEST_FIELD(STARTUPINFOW, DWORD, dwXCountChars, 32, 4, 4);
    TEST_FIELD(STARTUPINFOW, DWORD, dwYCountChars, 36, 4, 4);
    TEST_FIELD(STARTUPINFOW, DWORD, dwFillAttribute, 40, 4, 4);
    TEST_FIELD(STARTUPINFOW, DWORD, dwFlags, 44, 4, 4);
    TEST_FIELD(STARTUPINFOW, WORD, wShowWindow, 48, 2, 2);
    TEST_FIELD(STARTUPINFOW, WORD, cbReserved2, 50, 2, 2);
    TEST_FIELD(STARTUPINFOW, BYTE *, lpReserved2, 52, 4, 4);
    TEST_FIELD(STARTUPINFOW, HANDLE, hStdInput, 56, 4, 4);
    TEST_FIELD(STARTUPINFOW, HANDLE, hStdOutput, 60, 4, 4);
    TEST_FIELD(STARTUPINFOW, HANDLE, hStdError, 64, 4, 4);

    /* SYSTEMTIME (pack 4) */
    TEST_TYPE(SYSTEMTIME, 16, 2);
    TEST_FIELD(SYSTEMTIME, WORD, wYear, 0, 2, 2);
    TEST_FIELD(SYSTEMTIME, WORD, wMonth, 2, 2, 2);
    TEST_FIELD(SYSTEMTIME, WORD, wDayOfWeek, 4, 2, 2);
    TEST_FIELD(SYSTEMTIME, WORD, wDay, 6, 2, 2);
    TEST_FIELD(SYSTEMTIME, WORD, wHour, 8, 2, 2);
    TEST_FIELD(SYSTEMTIME, WORD, wMinute, 10, 2, 2);
    TEST_FIELD(SYSTEMTIME, WORD, wSecond, 12, 2, 2);
    TEST_FIELD(SYSTEMTIME, WORD, wMilliseconds, 14, 2, 2);

    /* SYSTEM_POWER_STATUS (pack 4) */
    TEST_TYPE(SYSTEM_POWER_STATUS, 12, 4);
    TEST_FIELD(SYSTEM_POWER_STATUS, BYTE, ACLineStatus, 0, 1, 1);
    TEST_FIELD(SYSTEM_POWER_STATUS, BYTE, BatteryFlag, 1, 1, 1);
    TEST_FIELD(SYSTEM_POWER_STATUS, BYTE, BatteryLifePercent, 2, 1, 1);
    TEST_FIELD(SYSTEM_POWER_STATUS, BYTE, Reserved1, 3, 1, 1);
    TEST_FIELD(SYSTEM_POWER_STATUS, DWORD, BatteryLifeTime, 4, 4, 4);
    TEST_FIELD(SYSTEM_POWER_STATUS, DWORD, BatteryFullLifeTime, 8, 4, 4);

    /* TIME_ZONE_INFORMATION (pack 4) */
    TEST_TYPE(TIME_ZONE_INFORMATION, 172, 4);
    TEST_FIELD(TIME_ZONE_INFORMATION, LONG, Bias, 0, 4, 4);
    TEST_FIELD(TIME_ZONE_INFORMATION, WCHAR[32], StandardName, 4, 64, 2);
    TEST_FIELD(TIME_ZONE_INFORMATION, SYSTEMTIME, StandardDate, 68, 16, 2);
    TEST_FIELD(TIME_ZONE_INFORMATION, LONG, StandardBias, 84, 4, 4);
    TEST_FIELD(TIME_ZONE_INFORMATION, WCHAR[32], DaylightName, 88, 64, 2);
    TEST_FIELD(TIME_ZONE_INFORMATION, SYSTEMTIME, DaylightDate, 152, 16, 2);
    TEST_FIELD(TIME_ZONE_INFORMATION, LONG, DaylightBias, 168, 4, 4);

    /* UNLOAD_DLL_DEBUG_INFO (pack 4) */
    TEST_TYPE(UNLOAD_DLL_DEBUG_INFO, 4, 4);
    TEST_FIELD(UNLOAD_DLL_DEBUG_INFO, LPVOID, lpBaseOfDll, 0, 4, 4);

    /* WIN32_FILE_ATTRIBUTE_DATA (pack 4) */
    TEST_TYPE(WIN32_FILE_ATTRIBUTE_DATA, 36, 4);
    TEST_FIELD(WIN32_FILE_ATTRIBUTE_DATA, DWORD, dwFileAttributes, 0, 4, 4);
    TEST_FIELD(WIN32_FILE_ATTRIBUTE_DATA, FILETIME, ftCreationTime, 4, 8, 4);
    TEST_FIELD(WIN32_FILE_ATTRIBUTE_DATA, FILETIME, ftLastAccessTime, 12, 8, 4);
    TEST_FIELD(WIN32_FILE_ATTRIBUTE_DATA, FILETIME, ftLastWriteTime, 20, 8, 4);
    TEST_FIELD(WIN32_FILE_ATTRIBUTE_DATA, DWORD, nFileSizeHigh, 28, 4, 4);
    TEST_FIELD(WIN32_FILE_ATTRIBUTE_DATA, DWORD, nFileSizeLow, 32, 4, 4);

    /* WIN32_FIND_DATAA (pack 4) */
    TEST_TYPE(WIN32_FIND_DATAA, 320, 4);
    TEST_FIELD(WIN32_FIND_DATAA, DWORD, dwFileAttributes, 0, 4, 4);
    TEST_FIELD(WIN32_FIND_DATAA, FILETIME, ftCreationTime, 4, 8, 4);
    TEST_FIELD(WIN32_FIND_DATAA, FILETIME, ftLastAccessTime, 12, 8, 4);
    TEST_FIELD(WIN32_FIND_DATAA, FILETIME, ftLastWriteTime, 20, 8, 4);
    TEST_FIELD(WIN32_FIND_DATAA, DWORD, nFileSizeHigh, 28, 4, 4);
    TEST_FIELD(WIN32_FIND_DATAA, DWORD, nFileSizeLow, 32, 4, 4);
    TEST_FIELD(WIN32_FIND_DATAA, DWORD, dwReserved0, 36, 4, 4);
    TEST_FIELD(WIN32_FIND_DATAA, DWORD, dwReserved1, 40, 4, 4);
    TEST_FIELD(WIN32_FIND_DATAA, CHAR[260], cFileName, 44, 260, 1);
    TEST_FIELD(WIN32_FIND_DATAA, CHAR[14], cAlternateFileName, 304, 14, 1);

    /* WIN32_FIND_DATAW (pack 4) */
    TEST_TYPE(WIN32_FIND_DATAW, 592, 4);
    TEST_FIELD(WIN32_FIND_DATAW, DWORD, dwFileAttributes, 0, 4, 4);
    TEST_FIELD(WIN32_FIND_DATAW, FILETIME, ftCreationTime, 4, 8, 4);
    TEST_FIELD(WIN32_FIND_DATAW, FILETIME, ftLastAccessTime, 12, 8, 4);
    TEST_FIELD(WIN32_FIND_DATAW, FILETIME, ftLastWriteTime, 20, 8, 4);
    TEST_FIELD(WIN32_FIND_DATAW, DWORD, nFileSizeHigh, 28, 4, 4);
    TEST_FIELD(WIN32_FIND_DATAW, DWORD, nFileSizeLow, 32, 4, 4);
    TEST_FIELD(WIN32_FIND_DATAW, DWORD, dwReserved0, 36, 4, 4);
    TEST_FIELD(WIN32_FIND_DATAW, DWORD, dwReserved1, 40, 4, 4);
    TEST_FIELD(WIN32_FIND_DATAW, WCHAR[260], cFileName, 44, 520, 2);
    TEST_FIELD(WIN32_FIND_DATAW, WCHAR[14], cAlternateFileName, 564, 28, 2);

    /* WIN32_STREAM_ID (pack 4) */
    TEST_TYPE(WIN32_STREAM_ID, 24, 4);
    TEST_FIELD(WIN32_STREAM_ID, DWORD, dwStreamId, 0, 4, 4);
    TEST_FIELD(WIN32_STREAM_ID, DWORD, dwStreamAttributes, 4, 4, 4);
    TEST_FIELD(WIN32_STREAM_ID, LARGE_INTEGER, Size, 8, 8, 4);
    TEST_FIELD(WIN32_STREAM_ID, DWORD, dwStreamNameSize, 16, 4, 4);
    TEST_FIELD(WIN32_STREAM_ID, WCHAR[ANYSIZE_ARRAY], cStreamName, 20, 2, 2);

}

START_TEST(generated)
{
    test_pack();
}
