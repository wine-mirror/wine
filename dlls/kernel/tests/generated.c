/* File generated automatically from ../../../tools/winapi/test.dat; do not edit! */
/* This file can be copied, modified and distributed without restriction. */

/*
 * Unit tests for data structure packing
 */

#include <stdio.h>

#include "wine/test.h"
#include "winbase.h"

START_TEST(generated)
{
    /* BY_HANDLE_FILE_INFORMATION */
    ok(FIELD_OFFSET(BY_HANDLE_FILE_INFORMATION, dwFileAttributes) == 0,
       "FIELD_OFFSET(BY_HANDLE_FILE_INFORMATION, dwFileAttributes) == %ld (expected 0)",
       FIELD_OFFSET(BY_HANDLE_FILE_INFORMATION, dwFileAttributes)); /* DWORD */
    ok(FIELD_OFFSET(BY_HANDLE_FILE_INFORMATION, ftCreationTime) == 4,
       "FIELD_OFFSET(BY_HANDLE_FILE_INFORMATION, ftCreationTime) == %ld (expected 4)",
       FIELD_OFFSET(BY_HANDLE_FILE_INFORMATION, ftCreationTime)); /* FILETIME */
    ok(FIELD_OFFSET(BY_HANDLE_FILE_INFORMATION, ftLastAccessTime) == 12,
       "FIELD_OFFSET(BY_HANDLE_FILE_INFORMATION, ftLastAccessTime) == %ld (expected 12)",
       FIELD_OFFSET(BY_HANDLE_FILE_INFORMATION, ftLastAccessTime)); /* FILETIME */
    ok(FIELD_OFFSET(BY_HANDLE_FILE_INFORMATION, ftLastWriteTime) == 20,
       "FIELD_OFFSET(BY_HANDLE_FILE_INFORMATION, ftLastWriteTime) == %ld (expected 20)",
       FIELD_OFFSET(BY_HANDLE_FILE_INFORMATION, ftLastWriteTime)); /* FILETIME */
    ok(FIELD_OFFSET(BY_HANDLE_FILE_INFORMATION, dwVolumeSerialNumber) == 28,
       "FIELD_OFFSET(BY_HANDLE_FILE_INFORMATION, dwVolumeSerialNumber) == %ld (expected 28)",
       FIELD_OFFSET(BY_HANDLE_FILE_INFORMATION, dwVolumeSerialNumber)); /* DWORD */
    ok(FIELD_OFFSET(BY_HANDLE_FILE_INFORMATION, nFileSizeHigh) == 32,
       "FIELD_OFFSET(BY_HANDLE_FILE_INFORMATION, nFileSizeHigh) == %ld (expected 32)",
       FIELD_OFFSET(BY_HANDLE_FILE_INFORMATION, nFileSizeHigh)); /* DWORD */
    ok(FIELD_OFFSET(BY_HANDLE_FILE_INFORMATION, nFileSizeLow) == 36,
       "FIELD_OFFSET(BY_HANDLE_FILE_INFORMATION, nFileSizeLow) == %ld (expected 36)",
       FIELD_OFFSET(BY_HANDLE_FILE_INFORMATION, nFileSizeLow)); /* DWORD */
    ok(FIELD_OFFSET(BY_HANDLE_FILE_INFORMATION, nNumberOfLinks) == 40,
       "FIELD_OFFSET(BY_HANDLE_FILE_INFORMATION, nNumberOfLinks) == %ld (expected 40)",
       FIELD_OFFSET(BY_HANDLE_FILE_INFORMATION, nNumberOfLinks)); /* DWORD */
    ok(FIELD_OFFSET(BY_HANDLE_FILE_INFORMATION, nFileIndexHigh) == 44,
       "FIELD_OFFSET(BY_HANDLE_FILE_INFORMATION, nFileIndexHigh) == %ld (expected 44)",
       FIELD_OFFSET(BY_HANDLE_FILE_INFORMATION, nFileIndexHigh)); /* DWORD */
    ok(FIELD_OFFSET(BY_HANDLE_FILE_INFORMATION, nFileIndexLow) == 48,
       "FIELD_OFFSET(BY_HANDLE_FILE_INFORMATION, nFileIndexLow) == %ld (expected 48)",
       FIELD_OFFSET(BY_HANDLE_FILE_INFORMATION, nFileIndexLow)); /* DWORD */
    ok(sizeof(BY_HANDLE_FILE_INFORMATION) == 52, "sizeof(BY_HANDLE_FILE_INFORMATION) == %d (expected 52)", sizeof(BY_HANDLE_FILE_INFORMATION));

    /* COMMCONFIG */
    ok(FIELD_OFFSET(COMMCONFIG, dwSize) == 0,
       "FIELD_OFFSET(COMMCONFIG, dwSize) == %ld (expected 0)",
       FIELD_OFFSET(COMMCONFIG, dwSize)); /* DWORD */
    ok(FIELD_OFFSET(COMMCONFIG, wVersion) == 4,
       "FIELD_OFFSET(COMMCONFIG, wVersion) == %ld (expected 4)",
       FIELD_OFFSET(COMMCONFIG, wVersion)); /* WORD */
    ok(FIELD_OFFSET(COMMCONFIG, wReserved) == 6,
       "FIELD_OFFSET(COMMCONFIG, wReserved) == %ld (expected 6)",
       FIELD_OFFSET(COMMCONFIG, wReserved)); /* WORD */
    ok(FIELD_OFFSET(COMMCONFIG, dcb) == 8,
       "FIELD_OFFSET(COMMCONFIG, dcb) == %ld (expected 8)",
       FIELD_OFFSET(COMMCONFIG, dcb)); /* DCB */
    ok(FIELD_OFFSET(COMMCONFIG, dwProviderSubType) == 36,
       "FIELD_OFFSET(COMMCONFIG, dwProviderSubType) == %ld (expected 36)",
       FIELD_OFFSET(COMMCONFIG, dwProviderSubType)); /* DWORD */
    ok(FIELD_OFFSET(COMMCONFIG, dwProviderOffset) == 40,
       "FIELD_OFFSET(COMMCONFIG, dwProviderOffset) == %ld (expected 40)",
       FIELD_OFFSET(COMMCONFIG, dwProviderOffset)); /* DWORD */
    ok(FIELD_OFFSET(COMMCONFIG, dwProviderSize) == 44,
       "FIELD_OFFSET(COMMCONFIG, dwProviderSize) == %ld (expected 44)",
       FIELD_OFFSET(COMMCONFIG, dwProviderSize)); /* DWORD */
    ok(FIELD_OFFSET(COMMCONFIG, wcProviderData) == 48,
       "FIELD_OFFSET(COMMCONFIG, wcProviderData) == %ld (expected 48)",
       FIELD_OFFSET(COMMCONFIG, wcProviderData)); /* DWORD[1] */
    ok(sizeof(COMMCONFIG) == 52, "sizeof(COMMCONFIG) == %d (expected 52)", sizeof(COMMCONFIG));

    /* COMMPROP */
    ok(FIELD_OFFSET(COMMPROP, wPacketLength) == 0,
       "FIELD_OFFSET(COMMPROP, wPacketLength) == %ld (expected 0)",
       FIELD_OFFSET(COMMPROP, wPacketLength)); /* WORD */
    ok(FIELD_OFFSET(COMMPROP, wPacketVersion) == 2,
       "FIELD_OFFSET(COMMPROP, wPacketVersion) == %ld (expected 2)",
       FIELD_OFFSET(COMMPROP, wPacketVersion)); /* WORD */
    ok(FIELD_OFFSET(COMMPROP, dwServiceMask) == 4,
       "FIELD_OFFSET(COMMPROP, dwServiceMask) == %ld (expected 4)",
       FIELD_OFFSET(COMMPROP, dwServiceMask)); /* DWORD */
    ok(FIELD_OFFSET(COMMPROP, dwReserved1) == 8,
       "FIELD_OFFSET(COMMPROP, dwReserved1) == %ld (expected 8)",
       FIELD_OFFSET(COMMPROP, dwReserved1)); /* DWORD */
    ok(FIELD_OFFSET(COMMPROP, dwMaxTxQueue) == 12,
       "FIELD_OFFSET(COMMPROP, dwMaxTxQueue) == %ld (expected 12)",
       FIELD_OFFSET(COMMPROP, dwMaxTxQueue)); /* DWORD */
    ok(FIELD_OFFSET(COMMPROP, dwMaxRxQueue) == 16,
       "FIELD_OFFSET(COMMPROP, dwMaxRxQueue) == %ld (expected 16)",
       FIELD_OFFSET(COMMPROP, dwMaxRxQueue)); /* DWORD */
    ok(FIELD_OFFSET(COMMPROP, dwMaxBaud) == 20,
       "FIELD_OFFSET(COMMPROP, dwMaxBaud) == %ld (expected 20)",
       FIELD_OFFSET(COMMPROP, dwMaxBaud)); /* DWORD */
    ok(FIELD_OFFSET(COMMPROP, dwProvSubType) == 24,
       "FIELD_OFFSET(COMMPROP, dwProvSubType) == %ld (expected 24)",
       FIELD_OFFSET(COMMPROP, dwProvSubType)); /* DWORD */
    ok(FIELD_OFFSET(COMMPROP, dwProvCapabilities) == 28,
       "FIELD_OFFSET(COMMPROP, dwProvCapabilities) == %ld (expected 28)",
       FIELD_OFFSET(COMMPROP, dwProvCapabilities)); /* DWORD */
    ok(FIELD_OFFSET(COMMPROP, dwSettableParams) == 32,
       "FIELD_OFFSET(COMMPROP, dwSettableParams) == %ld (expected 32)",
       FIELD_OFFSET(COMMPROP, dwSettableParams)); /* DWORD */
    ok(FIELD_OFFSET(COMMPROP, dwSettableBaud) == 36,
       "FIELD_OFFSET(COMMPROP, dwSettableBaud) == %ld (expected 36)",
       FIELD_OFFSET(COMMPROP, dwSettableBaud)); /* DWORD */
    ok(FIELD_OFFSET(COMMPROP, wSettableData) == 40,
       "FIELD_OFFSET(COMMPROP, wSettableData) == %ld (expected 40)",
       FIELD_OFFSET(COMMPROP, wSettableData)); /* WORD */
    ok(FIELD_OFFSET(COMMPROP, wSettableStopParity) == 42,
       "FIELD_OFFSET(COMMPROP, wSettableStopParity) == %ld (expected 42)",
       FIELD_OFFSET(COMMPROP, wSettableStopParity)); /* WORD */
    ok(FIELD_OFFSET(COMMPROP, dwCurrentTxQueue) == 44,
       "FIELD_OFFSET(COMMPROP, dwCurrentTxQueue) == %ld (expected 44)",
       FIELD_OFFSET(COMMPROP, dwCurrentTxQueue)); /* DWORD */
    ok(FIELD_OFFSET(COMMPROP, dwCurrentRxQueue) == 48,
       "FIELD_OFFSET(COMMPROP, dwCurrentRxQueue) == %ld (expected 48)",
       FIELD_OFFSET(COMMPROP, dwCurrentRxQueue)); /* DWORD */
    ok(FIELD_OFFSET(COMMPROP, dwProvSpec1) == 52,
       "FIELD_OFFSET(COMMPROP, dwProvSpec1) == %ld (expected 52)",
       FIELD_OFFSET(COMMPROP, dwProvSpec1)); /* DWORD */
    ok(FIELD_OFFSET(COMMPROP, dwProvSpec2) == 56,
       "FIELD_OFFSET(COMMPROP, dwProvSpec2) == %ld (expected 56)",
       FIELD_OFFSET(COMMPROP, dwProvSpec2)); /* DWORD */
    ok(FIELD_OFFSET(COMMPROP, wcProvChar) == 60,
       "FIELD_OFFSET(COMMPROP, wcProvChar) == %ld (expected 60)",
       FIELD_OFFSET(COMMPROP, wcProvChar)); /* WCHAR[1] */
    ok(sizeof(COMMPROP) == 64, "sizeof(COMMPROP) == %d (expected 64)", sizeof(COMMPROP));

    /* COMMTIMEOUTS */
    ok(FIELD_OFFSET(COMMTIMEOUTS, ReadIntervalTimeout) == 0,
       "FIELD_OFFSET(COMMTIMEOUTS, ReadIntervalTimeout) == %ld (expected 0)",
       FIELD_OFFSET(COMMTIMEOUTS, ReadIntervalTimeout)); /* DWORD */
    ok(FIELD_OFFSET(COMMTIMEOUTS, ReadTotalTimeoutMultiplier) == 4,
       "FIELD_OFFSET(COMMTIMEOUTS, ReadTotalTimeoutMultiplier) == %ld (expected 4)",
       FIELD_OFFSET(COMMTIMEOUTS, ReadTotalTimeoutMultiplier)); /* DWORD */
    ok(FIELD_OFFSET(COMMTIMEOUTS, ReadTotalTimeoutConstant) == 8,
       "FIELD_OFFSET(COMMTIMEOUTS, ReadTotalTimeoutConstant) == %ld (expected 8)",
       FIELD_OFFSET(COMMTIMEOUTS, ReadTotalTimeoutConstant)); /* DWORD */
    ok(FIELD_OFFSET(COMMTIMEOUTS, WriteTotalTimeoutMultiplier) == 12,
       "FIELD_OFFSET(COMMTIMEOUTS, WriteTotalTimeoutMultiplier) == %ld (expected 12)",
       FIELD_OFFSET(COMMTIMEOUTS, WriteTotalTimeoutMultiplier)); /* DWORD */
    ok(FIELD_OFFSET(COMMTIMEOUTS, WriteTotalTimeoutConstant) == 16,
       "FIELD_OFFSET(COMMTIMEOUTS, WriteTotalTimeoutConstant) == %ld (expected 16)",
       FIELD_OFFSET(COMMTIMEOUTS, WriteTotalTimeoutConstant)); /* DWORD */
    ok(sizeof(COMMTIMEOUTS) == 20, "sizeof(COMMTIMEOUTS) == %d (expected 20)", sizeof(COMMTIMEOUTS));

    /* COMSTAT */
    ok(FIELD_OFFSET(COMSTAT, status) == 0,
       "FIELD_OFFSET(COMSTAT, status) == %ld (expected 0)",
       FIELD_OFFSET(COMSTAT, status)); /* DWORD */
    ok(FIELD_OFFSET(COMSTAT, cbInQue) == 4,
       "FIELD_OFFSET(COMSTAT, cbInQue) == %ld (expected 4)",
       FIELD_OFFSET(COMSTAT, cbInQue)); /* DWORD */
    ok(FIELD_OFFSET(COMSTAT, cbOutQue) == 8,
       "FIELD_OFFSET(COMSTAT, cbOutQue) == %ld (expected 8)",
       FIELD_OFFSET(COMSTAT, cbOutQue)); /* DWORD */
    ok(sizeof(COMSTAT) == 12, "sizeof(COMSTAT) == %d (expected 12)", sizeof(COMSTAT));

    /* CREATE_PROCESS_DEBUG_INFO */
    ok(FIELD_OFFSET(CREATE_PROCESS_DEBUG_INFO, hFile) == 0,
       "FIELD_OFFSET(CREATE_PROCESS_DEBUG_INFO, hFile) == %ld (expected 0)",
       FIELD_OFFSET(CREATE_PROCESS_DEBUG_INFO, hFile)); /* HANDLE */
    ok(FIELD_OFFSET(CREATE_PROCESS_DEBUG_INFO, hProcess) == 4,
       "FIELD_OFFSET(CREATE_PROCESS_DEBUG_INFO, hProcess) == %ld (expected 4)",
       FIELD_OFFSET(CREATE_PROCESS_DEBUG_INFO, hProcess)); /* HANDLE */
    ok(FIELD_OFFSET(CREATE_PROCESS_DEBUG_INFO, hThread) == 8,
       "FIELD_OFFSET(CREATE_PROCESS_DEBUG_INFO, hThread) == %ld (expected 8)",
       FIELD_OFFSET(CREATE_PROCESS_DEBUG_INFO, hThread)); /* HANDLE */
    ok(FIELD_OFFSET(CREATE_PROCESS_DEBUG_INFO, lpBaseOfImage) == 12,
       "FIELD_OFFSET(CREATE_PROCESS_DEBUG_INFO, lpBaseOfImage) == %ld (expected 12)",
       FIELD_OFFSET(CREATE_PROCESS_DEBUG_INFO, lpBaseOfImage)); /* LPVOID */
    ok(FIELD_OFFSET(CREATE_PROCESS_DEBUG_INFO, dwDebugInfoFileOffset) == 16,
       "FIELD_OFFSET(CREATE_PROCESS_DEBUG_INFO, dwDebugInfoFileOffset) == %ld (expected 16)",
       FIELD_OFFSET(CREATE_PROCESS_DEBUG_INFO, dwDebugInfoFileOffset)); /* DWORD */
    ok(FIELD_OFFSET(CREATE_PROCESS_DEBUG_INFO, nDebugInfoSize) == 20,
       "FIELD_OFFSET(CREATE_PROCESS_DEBUG_INFO, nDebugInfoSize) == %ld (expected 20)",
       FIELD_OFFSET(CREATE_PROCESS_DEBUG_INFO, nDebugInfoSize)); /* DWORD */
    ok(FIELD_OFFSET(CREATE_PROCESS_DEBUG_INFO, lpThreadLocalBase) == 24,
       "FIELD_OFFSET(CREATE_PROCESS_DEBUG_INFO, lpThreadLocalBase) == %ld (expected 24)",
       FIELD_OFFSET(CREATE_PROCESS_DEBUG_INFO, lpThreadLocalBase)); /* LPVOID */
    ok(FIELD_OFFSET(CREATE_PROCESS_DEBUG_INFO, lpStartAddress) == 28,
       "FIELD_OFFSET(CREATE_PROCESS_DEBUG_INFO, lpStartAddress) == %ld (expected 28)",
       FIELD_OFFSET(CREATE_PROCESS_DEBUG_INFO, lpStartAddress)); /* LPTHREAD_START_ROUTINE */
    ok(FIELD_OFFSET(CREATE_PROCESS_DEBUG_INFO, lpImageName) == 32,
       "FIELD_OFFSET(CREATE_PROCESS_DEBUG_INFO, lpImageName) == %ld (expected 32)",
       FIELD_OFFSET(CREATE_PROCESS_DEBUG_INFO, lpImageName)); /* LPVOID */
    ok(FIELD_OFFSET(CREATE_PROCESS_DEBUG_INFO, fUnicode) == 36,
       "FIELD_OFFSET(CREATE_PROCESS_DEBUG_INFO, fUnicode) == %ld (expected 36)",
       FIELD_OFFSET(CREATE_PROCESS_DEBUG_INFO, fUnicode)); /* WORD */
    ok(sizeof(CREATE_PROCESS_DEBUG_INFO) == 40, "sizeof(CREATE_PROCESS_DEBUG_INFO) == %d (expected 40)", sizeof(CREATE_PROCESS_DEBUG_INFO));

    /* CREATE_THREAD_DEBUG_INFO */
    ok(FIELD_OFFSET(CREATE_THREAD_DEBUG_INFO, hThread) == 0,
       "FIELD_OFFSET(CREATE_THREAD_DEBUG_INFO, hThread) == %ld (expected 0)",
       FIELD_OFFSET(CREATE_THREAD_DEBUG_INFO, hThread)); /* HANDLE */
    ok(FIELD_OFFSET(CREATE_THREAD_DEBUG_INFO, lpThreadLocalBase) == 4,
       "FIELD_OFFSET(CREATE_THREAD_DEBUG_INFO, lpThreadLocalBase) == %ld (expected 4)",
       FIELD_OFFSET(CREATE_THREAD_DEBUG_INFO, lpThreadLocalBase)); /* LPVOID */
    ok(FIELD_OFFSET(CREATE_THREAD_DEBUG_INFO, lpStartAddress) == 8,
       "FIELD_OFFSET(CREATE_THREAD_DEBUG_INFO, lpStartAddress) == %ld (expected 8)",
       FIELD_OFFSET(CREATE_THREAD_DEBUG_INFO, lpStartAddress)); /* LPTHREAD_START_ROUTINE */
    ok(sizeof(CREATE_THREAD_DEBUG_INFO) == 12, "sizeof(CREATE_THREAD_DEBUG_INFO) == %d (expected 12)", sizeof(CREATE_THREAD_DEBUG_INFO));

    /* DCB */
    ok(FIELD_OFFSET(DCB, DCBlength) == 0,
       "FIELD_OFFSET(DCB, DCBlength) == %ld (expected 0)",
       FIELD_OFFSET(DCB, DCBlength)); /* DWORD */
    ok(FIELD_OFFSET(DCB, BaudRate) == 4,
       "FIELD_OFFSET(DCB, BaudRate) == %ld (expected 4)",
       FIELD_OFFSET(DCB, BaudRate)); /* DWORD */
    ok(FIELD_OFFSET(DCB, wReserved) == 12,
       "FIELD_OFFSET(DCB, wReserved) == %ld (expected 12)",
       FIELD_OFFSET(DCB, wReserved)); /* WORD */
    ok(FIELD_OFFSET(DCB, XonLim) == 14,
       "FIELD_OFFSET(DCB, XonLim) == %ld (expected 14)",
       FIELD_OFFSET(DCB, XonLim)); /* WORD */
    ok(FIELD_OFFSET(DCB, XoffLim) == 16,
       "FIELD_OFFSET(DCB, XoffLim) == %ld (expected 16)",
       FIELD_OFFSET(DCB, XoffLim)); /* WORD */
    ok(FIELD_OFFSET(DCB, ByteSize) == 18,
       "FIELD_OFFSET(DCB, ByteSize) == %ld (expected 18)",
       FIELD_OFFSET(DCB, ByteSize)); /* BYTE */
    ok(FIELD_OFFSET(DCB, Parity) == 19,
       "FIELD_OFFSET(DCB, Parity) == %ld (expected 19)",
       FIELD_OFFSET(DCB, Parity)); /* BYTE */
    ok(FIELD_OFFSET(DCB, StopBits) == 20,
       "FIELD_OFFSET(DCB, StopBits) == %ld (expected 20)",
       FIELD_OFFSET(DCB, StopBits)); /* BYTE */
    ok(FIELD_OFFSET(DCB, XonChar) == 21,
       "FIELD_OFFSET(DCB, XonChar) == %ld (expected 21)",
       FIELD_OFFSET(DCB, XonChar)); /* char */
    ok(FIELD_OFFSET(DCB, XoffChar) == 22,
       "FIELD_OFFSET(DCB, XoffChar) == %ld (expected 22)",
       FIELD_OFFSET(DCB, XoffChar)); /* char */
    ok(FIELD_OFFSET(DCB, ErrorChar) == 23,
       "FIELD_OFFSET(DCB, ErrorChar) == %ld (expected 23)",
       FIELD_OFFSET(DCB, ErrorChar)); /* char */
    ok(FIELD_OFFSET(DCB, EofChar) == 24,
       "FIELD_OFFSET(DCB, EofChar) == %ld (expected 24)",
       FIELD_OFFSET(DCB, EofChar)); /* char */
    ok(FIELD_OFFSET(DCB, EvtChar) == 25,
       "FIELD_OFFSET(DCB, EvtChar) == %ld (expected 25)",
       FIELD_OFFSET(DCB, EvtChar)); /* char */
    ok(sizeof(DCB) == 28, "sizeof(DCB) == %d (expected 28)", sizeof(DCB));

    /* EXCEPTION_DEBUG_INFO */
    ok(FIELD_OFFSET(EXCEPTION_DEBUG_INFO, ExceptionRecord) == 0,
       "FIELD_OFFSET(EXCEPTION_DEBUG_INFO, ExceptionRecord) == %ld (expected 0)",
       FIELD_OFFSET(EXCEPTION_DEBUG_INFO, ExceptionRecord)); /* EXCEPTION_RECORD */
    ok(FIELD_OFFSET(EXCEPTION_DEBUG_INFO, dwFirstChance) == 80,
       "FIELD_OFFSET(EXCEPTION_DEBUG_INFO, dwFirstChance) == %ld (expected 80)",
       FIELD_OFFSET(EXCEPTION_DEBUG_INFO, dwFirstChance)); /* DWORD */
    ok(sizeof(EXCEPTION_DEBUG_INFO) == 84, "sizeof(EXCEPTION_DEBUG_INFO) == %d (expected 84)", sizeof(EXCEPTION_DEBUG_INFO));

    /* EXIT_PROCESS_DEBUG_INFO */
    ok(FIELD_OFFSET(EXIT_PROCESS_DEBUG_INFO, dwExitCode) == 0,
       "FIELD_OFFSET(EXIT_PROCESS_DEBUG_INFO, dwExitCode) == %ld (expected 0)",
       FIELD_OFFSET(EXIT_PROCESS_DEBUG_INFO, dwExitCode)); /* DWORD */
    ok(sizeof(EXIT_PROCESS_DEBUG_INFO) == 4, "sizeof(EXIT_PROCESS_DEBUG_INFO) == %d (expected 4)", sizeof(EXIT_PROCESS_DEBUG_INFO));

    /* EXIT_THREAD_DEBUG_INFO */
    ok(FIELD_OFFSET(EXIT_THREAD_DEBUG_INFO, dwExitCode) == 0,
       "FIELD_OFFSET(EXIT_THREAD_DEBUG_INFO, dwExitCode) == %ld (expected 0)",
       FIELD_OFFSET(EXIT_THREAD_DEBUG_INFO, dwExitCode)); /* DWORD */
    ok(sizeof(EXIT_THREAD_DEBUG_INFO) == 4, "sizeof(EXIT_THREAD_DEBUG_INFO) == %d (expected 4)", sizeof(EXIT_THREAD_DEBUG_INFO));

    /* LDT_ENTRY */
    ok(FIELD_OFFSET(LDT_ENTRY, LimitLow) == 0,
       "FIELD_OFFSET(LDT_ENTRY, LimitLow) == %ld (expected 0)",
       FIELD_OFFSET(LDT_ENTRY, LimitLow)); /* WORD */
    ok(FIELD_OFFSET(LDT_ENTRY, BaseLow) == 2,
       "FIELD_OFFSET(LDT_ENTRY, BaseLow) == %ld (expected 2)",
       FIELD_OFFSET(LDT_ENTRY, BaseLow)); /* WORD */
    ok(sizeof(LDT_ENTRY) == 8, "sizeof(LDT_ENTRY) == %d (expected 8)", sizeof(LDT_ENTRY));

    /* LOAD_DLL_DEBUG_INFO */
    ok(FIELD_OFFSET(LOAD_DLL_DEBUG_INFO, hFile) == 0,
       "FIELD_OFFSET(LOAD_DLL_DEBUG_INFO, hFile) == %ld (expected 0)",
       FIELD_OFFSET(LOAD_DLL_DEBUG_INFO, hFile)); /* HANDLE */
    ok(FIELD_OFFSET(LOAD_DLL_DEBUG_INFO, lpBaseOfDll) == 4,
       "FIELD_OFFSET(LOAD_DLL_DEBUG_INFO, lpBaseOfDll) == %ld (expected 4)",
       FIELD_OFFSET(LOAD_DLL_DEBUG_INFO, lpBaseOfDll)); /* LPVOID */
    ok(FIELD_OFFSET(LOAD_DLL_DEBUG_INFO, dwDebugInfoFileOffset) == 8,
       "FIELD_OFFSET(LOAD_DLL_DEBUG_INFO, dwDebugInfoFileOffset) == %ld (expected 8)",
       FIELD_OFFSET(LOAD_DLL_DEBUG_INFO, dwDebugInfoFileOffset)); /* DWORD */
    ok(FIELD_OFFSET(LOAD_DLL_DEBUG_INFO, nDebugInfoSize) == 12,
       "FIELD_OFFSET(LOAD_DLL_DEBUG_INFO, nDebugInfoSize) == %ld (expected 12)",
       FIELD_OFFSET(LOAD_DLL_DEBUG_INFO, nDebugInfoSize)); /* DWORD */
    ok(FIELD_OFFSET(LOAD_DLL_DEBUG_INFO, lpImageName) == 16,
       "FIELD_OFFSET(LOAD_DLL_DEBUG_INFO, lpImageName) == %ld (expected 16)",
       FIELD_OFFSET(LOAD_DLL_DEBUG_INFO, lpImageName)); /* LPVOID */
    ok(FIELD_OFFSET(LOAD_DLL_DEBUG_INFO, fUnicode) == 20,
       "FIELD_OFFSET(LOAD_DLL_DEBUG_INFO, fUnicode) == %ld (expected 20)",
       FIELD_OFFSET(LOAD_DLL_DEBUG_INFO, fUnicode)); /* WORD */
    ok(sizeof(LOAD_DLL_DEBUG_INFO) == 24, "sizeof(LOAD_DLL_DEBUG_INFO) == %d (expected 24)", sizeof(LOAD_DLL_DEBUG_INFO));

    /* MEMORYSTATUS */
    ok(FIELD_OFFSET(MEMORYSTATUS, dwLength) == 0,
       "FIELD_OFFSET(MEMORYSTATUS, dwLength) == %ld (expected 0)",
       FIELD_OFFSET(MEMORYSTATUS, dwLength)); /* DWORD */
    ok(FIELD_OFFSET(MEMORYSTATUS, dwMemoryLoad) == 4,
       "FIELD_OFFSET(MEMORYSTATUS, dwMemoryLoad) == %ld (expected 4)",
       FIELD_OFFSET(MEMORYSTATUS, dwMemoryLoad)); /* DWORD */
    ok(FIELD_OFFSET(MEMORYSTATUS, dwTotalPhys) == 8,
       "FIELD_OFFSET(MEMORYSTATUS, dwTotalPhys) == %ld (expected 8)",
       FIELD_OFFSET(MEMORYSTATUS, dwTotalPhys)); /* SIZE_T */
    ok(FIELD_OFFSET(MEMORYSTATUS, dwAvailPhys) == 12,
       "FIELD_OFFSET(MEMORYSTATUS, dwAvailPhys) == %ld (expected 12)",
       FIELD_OFFSET(MEMORYSTATUS, dwAvailPhys)); /* SIZE_T */
    ok(FIELD_OFFSET(MEMORYSTATUS, dwTotalPageFile) == 16,
       "FIELD_OFFSET(MEMORYSTATUS, dwTotalPageFile) == %ld (expected 16)",
       FIELD_OFFSET(MEMORYSTATUS, dwTotalPageFile)); /* SIZE_T */
    ok(FIELD_OFFSET(MEMORYSTATUS, dwAvailPageFile) == 20,
       "FIELD_OFFSET(MEMORYSTATUS, dwAvailPageFile) == %ld (expected 20)",
       FIELD_OFFSET(MEMORYSTATUS, dwAvailPageFile)); /* SIZE_T */
    ok(FIELD_OFFSET(MEMORYSTATUS, dwTotalVirtual) == 24,
       "FIELD_OFFSET(MEMORYSTATUS, dwTotalVirtual) == %ld (expected 24)",
       FIELD_OFFSET(MEMORYSTATUS, dwTotalVirtual)); /* SIZE_T */
    ok(FIELD_OFFSET(MEMORYSTATUS, dwAvailVirtual) == 28,
       "FIELD_OFFSET(MEMORYSTATUS, dwAvailVirtual) == %ld (expected 28)",
       FIELD_OFFSET(MEMORYSTATUS, dwAvailVirtual)); /* SIZE_T */
    ok(sizeof(MEMORYSTATUS) == 32, "sizeof(MEMORYSTATUS) == %d (expected 32)", sizeof(MEMORYSTATUS));

    /* OSVERSIONINFOA */
    ok(FIELD_OFFSET(OSVERSIONINFOA, dwOSVersionInfoSize) == 0,
       "FIELD_OFFSET(OSVERSIONINFOA, dwOSVersionInfoSize) == %ld (expected 0)",
       FIELD_OFFSET(OSVERSIONINFOA, dwOSVersionInfoSize)); /* DWORD */
    ok(FIELD_OFFSET(OSVERSIONINFOA, dwMajorVersion) == 4,
       "FIELD_OFFSET(OSVERSIONINFOA, dwMajorVersion) == %ld (expected 4)",
       FIELD_OFFSET(OSVERSIONINFOA, dwMajorVersion)); /* DWORD */
    ok(FIELD_OFFSET(OSVERSIONINFOA, dwMinorVersion) == 8,
       "FIELD_OFFSET(OSVERSIONINFOA, dwMinorVersion) == %ld (expected 8)",
       FIELD_OFFSET(OSVERSIONINFOA, dwMinorVersion)); /* DWORD */
    ok(FIELD_OFFSET(OSVERSIONINFOA, dwBuildNumber) == 12,
       "FIELD_OFFSET(OSVERSIONINFOA, dwBuildNumber) == %ld (expected 12)",
       FIELD_OFFSET(OSVERSIONINFOA, dwBuildNumber)); /* DWORD */
    ok(FIELD_OFFSET(OSVERSIONINFOA, dwPlatformId) == 16,
       "FIELD_OFFSET(OSVERSIONINFOA, dwPlatformId) == %ld (expected 16)",
       FIELD_OFFSET(OSVERSIONINFOA, dwPlatformId)); /* DWORD */
    ok(FIELD_OFFSET(OSVERSIONINFOA, szCSDVersion) == 20,
       "FIELD_OFFSET(OSVERSIONINFOA, szCSDVersion) == %ld (expected 20)",
       FIELD_OFFSET(OSVERSIONINFOA, szCSDVersion)); /* CHAR[128] */
    ok(sizeof(OSVERSIONINFOA) == 148, "sizeof(OSVERSIONINFOA) == %d (expected 148)", sizeof(OSVERSIONINFOA));

    /* OSVERSIONINFOEXA */
    ok(FIELD_OFFSET(OSVERSIONINFOEXA, dwOSVersionInfoSize) == 0,
       "FIELD_OFFSET(OSVERSIONINFOEXA, dwOSVersionInfoSize) == %ld (expected 0)",
       FIELD_OFFSET(OSVERSIONINFOEXA, dwOSVersionInfoSize)); /* DWORD */
    ok(FIELD_OFFSET(OSVERSIONINFOEXA, dwMajorVersion) == 4,
       "FIELD_OFFSET(OSVERSIONINFOEXA, dwMajorVersion) == %ld (expected 4)",
       FIELD_OFFSET(OSVERSIONINFOEXA, dwMajorVersion)); /* DWORD */
    ok(FIELD_OFFSET(OSVERSIONINFOEXA, dwMinorVersion) == 8,
       "FIELD_OFFSET(OSVERSIONINFOEXA, dwMinorVersion) == %ld (expected 8)",
       FIELD_OFFSET(OSVERSIONINFOEXA, dwMinorVersion)); /* DWORD */
    ok(FIELD_OFFSET(OSVERSIONINFOEXA, dwBuildNumber) == 12,
       "FIELD_OFFSET(OSVERSIONINFOEXA, dwBuildNumber) == %ld (expected 12)",
       FIELD_OFFSET(OSVERSIONINFOEXA, dwBuildNumber)); /* DWORD */
    ok(FIELD_OFFSET(OSVERSIONINFOEXA, dwPlatformId) == 16,
       "FIELD_OFFSET(OSVERSIONINFOEXA, dwPlatformId) == %ld (expected 16)",
       FIELD_OFFSET(OSVERSIONINFOEXA, dwPlatformId)); /* DWORD */
    ok(FIELD_OFFSET(OSVERSIONINFOEXA, szCSDVersion) == 20,
       "FIELD_OFFSET(OSVERSIONINFOEXA, szCSDVersion) == %ld (expected 20)",
       FIELD_OFFSET(OSVERSIONINFOEXA, szCSDVersion)); /* CHAR[128] */
    ok(FIELD_OFFSET(OSVERSIONINFOEXA, wServicePackMajor) == 148,
       "FIELD_OFFSET(OSVERSIONINFOEXA, wServicePackMajor) == %ld (expected 148)",
       FIELD_OFFSET(OSVERSIONINFOEXA, wServicePackMajor)); /* WORD */
    ok(FIELD_OFFSET(OSVERSIONINFOEXA, wServicePackMinor) == 150,
       "FIELD_OFFSET(OSVERSIONINFOEXA, wServicePackMinor) == %ld (expected 150)",
       FIELD_OFFSET(OSVERSIONINFOEXA, wServicePackMinor)); /* WORD */
    ok(FIELD_OFFSET(OSVERSIONINFOEXA, wSuiteMask) == 152,
       "FIELD_OFFSET(OSVERSIONINFOEXA, wSuiteMask) == %ld (expected 152)",
       FIELD_OFFSET(OSVERSIONINFOEXA, wSuiteMask)); /* WORD */
    ok(FIELD_OFFSET(OSVERSIONINFOEXA, wProductType) == 154,
       "FIELD_OFFSET(OSVERSIONINFOEXA, wProductType) == %ld (expected 154)",
       FIELD_OFFSET(OSVERSIONINFOEXA, wProductType)); /* BYTE */
    ok(FIELD_OFFSET(OSVERSIONINFOEXA, wReserved) == 155,
       "FIELD_OFFSET(OSVERSIONINFOEXA, wReserved) == %ld (expected 155)",
       FIELD_OFFSET(OSVERSIONINFOEXA, wReserved)); /* BYTE */
    ok(sizeof(OSVERSIONINFOEXA) == 156, "sizeof(OSVERSIONINFOEXA) == %d (expected 156)", sizeof(OSVERSIONINFOEXA));

    /* OSVERSIONINFOEXW */
    ok(FIELD_OFFSET(OSVERSIONINFOEXW, dwOSVersionInfoSize) == 0,
       "FIELD_OFFSET(OSVERSIONINFOEXW, dwOSVersionInfoSize) == %ld (expected 0)",
       FIELD_OFFSET(OSVERSIONINFOEXW, dwOSVersionInfoSize)); /* DWORD */
    ok(FIELD_OFFSET(OSVERSIONINFOEXW, dwMajorVersion) == 4,
       "FIELD_OFFSET(OSVERSIONINFOEXW, dwMajorVersion) == %ld (expected 4)",
       FIELD_OFFSET(OSVERSIONINFOEXW, dwMajorVersion)); /* DWORD */
    ok(FIELD_OFFSET(OSVERSIONINFOEXW, dwMinorVersion) == 8,
       "FIELD_OFFSET(OSVERSIONINFOEXW, dwMinorVersion) == %ld (expected 8)",
       FIELD_OFFSET(OSVERSIONINFOEXW, dwMinorVersion)); /* DWORD */
    ok(FIELD_OFFSET(OSVERSIONINFOEXW, dwBuildNumber) == 12,
       "FIELD_OFFSET(OSVERSIONINFOEXW, dwBuildNumber) == %ld (expected 12)",
       FIELD_OFFSET(OSVERSIONINFOEXW, dwBuildNumber)); /* DWORD */
    ok(FIELD_OFFSET(OSVERSIONINFOEXW, dwPlatformId) == 16,
       "FIELD_OFFSET(OSVERSIONINFOEXW, dwPlatformId) == %ld (expected 16)",
       FIELD_OFFSET(OSVERSIONINFOEXW, dwPlatformId)); /* DWORD */
    ok(FIELD_OFFSET(OSVERSIONINFOEXW, szCSDVersion) == 20,
       "FIELD_OFFSET(OSVERSIONINFOEXW, szCSDVersion) == %ld (expected 20)",
       FIELD_OFFSET(OSVERSIONINFOEXW, szCSDVersion)); /* WCHAR[128] */
    ok(FIELD_OFFSET(OSVERSIONINFOEXW, wServicePackMajor) == 276,
       "FIELD_OFFSET(OSVERSIONINFOEXW, wServicePackMajor) == %ld (expected 276)",
       FIELD_OFFSET(OSVERSIONINFOEXW, wServicePackMajor)); /* WORD */
    ok(FIELD_OFFSET(OSVERSIONINFOEXW, wServicePackMinor) == 278,
       "FIELD_OFFSET(OSVERSIONINFOEXW, wServicePackMinor) == %ld (expected 278)",
       FIELD_OFFSET(OSVERSIONINFOEXW, wServicePackMinor)); /* WORD */
    ok(FIELD_OFFSET(OSVERSIONINFOEXW, wSuiteMask) == 280,
       "FIELD_OFFSET(OSVERSIONINFOEXW, wSuiteMask) == %ld (expected 280)",
       FIELD_OFFSET(OSVERSIONINFOEXW, wSuiteMask)); /* WORD */
    ok(FIELD_OFFSET(OSVERSIONINFOEXW, wProductType) == 282,
       "FIELD_OFFSET(OSVERSIONINFOEXW, wProductType) == %ld (expected 282)",
       FIELD_OFFSET(OSVERSIONINFOEXW, wProductType)); /* BYTE */
    ok(FIELD_OFFSET(OSVERSIONINFOEXW, wReserved) == 283,
       "FIELD_OFFSET(OSVERSIONINFOEXW, wReserved) == %ld (expected 283)",
       FIELD_OFFSET(OSVERSIONINFOEXW, wReserved)); /* BYTE */
    ok(sizeof(OSVERSIONINFOEXW) == 284, "sizeof(OSVERSIONINFOEXW) == %d (expected 284)", sizeof(OSVERSIONINFOEXW));

    /* OSVERSIONINFOW */
    ok(FIELD_OFFSET(OSVERSIONINFOW, dwOSVersionInfoSize) == 0,
       "FIELD_OFFSET(OSVERSIONINFOW, dwOSVersionInfoSize) == %ld (expected 0)",
       FIELD_OFFSET(OSVERSIONINFOW, dwOSVersionInfoSize)); /* DWORD */
    ok(FIELD_OFFSET(OSVERSIONINFOW, dwMajorVersion) == 4,
       "FIELD_OFFSET(OSVERSIONINFOW, dwMajorVersion) == %ld (expected 4)",
       FIELD_OFFSET(OSVERSIONINFOW, dwMajorVersion)); /* DWORD */
    ok(FIELD_OFFSET(OSVERSIONINFOW, dwMinorVersion) == 8,
       "FIELD_OFFSET(OSVERSIONINFOW, dwMinorVersion) == %ld (expected 8)",
       FIELD_OFFSET(OSVERSIONINFOW, dwMinorVersion)); /* DWORD */
    ok(FIELD_OFFSET(OSVERSIONINFOW, dwBuildNumber) == 12,
       "FIELD_OFFSET(OSVERSIONINFOW, dwBuildNumber) == %ld (expected 12)",
       FIELD_OFFSET(OSVERSIONINFOW, dwBuildNumber)); /* DWORD */
    ok(FIELD_OFFSET(OSVERSIONINFOW, dwPlatformId) == 16,
       "FIELD_OFFSET(OSVERSIONINFOW, dwPlatformId) == %ld (expected 16)",
       FIELD_OFFSET(OSVERSIONINFOW, dwPlatformId)); /* DWORD */
    ok(FIELD_OFFSET(OSVERSIONINFOW, szCSDVersion) == 20,
       "FIELD_OFFSET(OSVERSIONINFOW, szCSDVersion) == %ld (expected 20)",
       FIELD_OFFSET(OSVERSIONINFOW, szCSDVersion)); /* WCHAR[128] */
    ok(sizeof(OSVERSIONINFOW) == 276, "sizeof(OSVERSIONINFOW) == %d (expected 276)", sizeof(OSVERSIONINFOW));

    /* OUTPUT_DEBUG_STRING_INFO */
    ok(FIELD_OFFSET(OUTPUT_DEBUG_STRING_INFO, lpDebugStringData) == 0,
       "FIELD_OFFSET(OUTPUT_DEBUG_STRING_INFO, lpDebugStringData) == %ld (expected 0)",
       FIELD_OFFSET(OUTPUT_DEBUG_STRING_INFO, lpDebugStringData)); /* LPSTR */
    ok(FIELD_OFFSET(OUTPUT_DEBUG_STRING_INFO, fUnicode) == 4,
       "FIELD_OFFSET(OUTPUT_DEBUG_STRING_INFO, fUnicode) == %ld (expected 4)",
       FIELD_OFFSET(OUTPUT_DEBUG_STRING_INFO, fUnicode)); /* WORD */
    ok(FIELD_OFFSET(OUTPUT_DEBUG_STRING_INFO, nDebugStringLength) == 6,
       "FIELD_OFFSET(OUTPUT_DEBUG_STRING_INFO, nDebugStringLength) == %ld (expected 6)",
       FIELD_OFFSET(OUTPUT_DEBUG_STRING_INFO, nDebugStringLength)); /* WORD */
    ok(sizeof(OUTPUT_DEBUG_STRING_INFO) == 8, "sizeof(OUTPUT_DEBUG_STRING_INFO) == %d (expected 8)", sizeof(OUTPUT_DEBUG_STRING_INFO));

    /* OVERLAPPED */
    ok(FIELD_OFFSET(OVERLAPPED, Internal) == 0,
       "FIELD_OFFSET(OVERLAPPED, Internal) == %ld (expected 0)",
       FIELD_OFFSET(OVERLAPPED, Internal)); /* DWORD */
    ok(FIELD_OFFSET(OVERLAPPED, InternalHigh) == 4,
       "FIELD_OFFSET(OVERLAPPED, InternalHigh) == %ld (expected 4)",
       FIELD_OFFSET(OVERLAPPED, InternalHigh)); /* DWORD */
    ok(FIELD_OFFSET(OVERLAPPED, Offset) == 8,
       "FIELD_OFFSET(OVERLAPPED, Offset) == %ld (expected 8)",
       FIELD_OFFSET(OVERLAPPED, Offset)); /* DWORD */
    ok(FIELD_OFFSET(OVERLAPPED, OffsetHigh) == 12,
       "FIELD_OFFSET(OVERLAPPED, OffsetHigh) == %ld (expected 12)",
       FIELD_OFFSET(OVERLAPPED, OffsetHigh)); /* DWORD */
    ok(FIELD_OFFSET(OVERLAPPED, hEvent) == 16,
       "FIELD_OFFSET(OVERLAPPED, hEvent) == %ld (expected 16)",
       FIELD_OFFSET(OVERLAPPED, hEvent)); /* HANDLE */
    ok(sizeof(OVERLAPPED) == 20, "sizeof(OVERLAPPED) == %d (expected 20)", sizeof(OVERLAPPED));

    /* PROCESS_INFORMATION */
    ok(FIELD_OFFSET(PROCESS_INFORMATION, hProcess) == 0,
       "FIELD_OFFSET(PROCESS_INFORMATION, hProcess) == %ld (expected 0)",
       FIELD_OFFSET(PROCESS_INFORMATION, hProcess)); /* HANDLE */
    ok(FIELD_OFFSET(PROCESS_INFORMATION, hThread) == 4,
       "FIELD_OFFSET(PROCESS_INFORMATION, hThread) == %ld (expected 4)",
       FIELD_OFFSET(PROCESS_INFORMATION, hThread)); /* HANDLE */
    ok(FIELD_OFFSET(PROCESS_INFORMATION, dwProcessId) == 8,
       "FIELD_OFFSET(PROCESS_INFORMATION, dwProcessId) == %ld (expected 8)",
       FIELD_OFFSET(PROCESS_INFORMATION, dwProcessId)); /* DWORD */
    ok(FIELD_OFFSET(PROCESS_INFORMATION, dwThreadId) == 12,
       "FIELD_OFFSET(PROCESS_INFORMATION, dwThreadId) == %ld (expected 12)",
       FIELD_OFFSET(PROCESS_INFORMATION, dwThreadId)); /* DWORD */
    ok(sizeof(PROCESS_INFORMATION) == 16, "sizeof(PROCESS_INFORMATION) == %d (expected 16)", sizeof(PROCESS_INFORMATION));

    /* RIP_INFO */
    ok(FIELD_OFFSET(RIP_INFO, dwError) == 0,
       "FIELD_OFFSET(RIP_INFO, dwError) == %ld (expected 0)",
       FIELD_OFFSET(RIP_INFO, dwError)); /* DWORD */
    ok(FIELD_OFFSET(RIP_INFO, dwType) == 4,
       "FIELD_OFFSET(RIP_INFO, dwType) == %ld (expected 4)",
       FIELD_OFFSET(RIP_INFO, dwType)); /* DWORD */
    ok(sizeof(RIP_INFO) == 8, "sizeof(RIP_INFO) == %d (expected 8)", sizeof(RIP_INFO));

    /* SECURITY_ATTRIBUTES */
    ok(FIELD_OFFSET(SECURITY_ATTRIBUTES, nLength) == 0,
       "FIELD_OFFSET(SECURITY_ATTRIBUTES, nLength) == %ld (expected 0)",
       FIELD_OFFSET(SECURITY_ATTRIBUTES, nLength)); /* DWORD */
    ok(FIELD_OFFSET(SECURITY_ATTRIBUTES, lpSecurityDescriptor) == 4,
       "FIELD_OFFSET(SECURITY_ATTRIBUTES, lpSecurityDescriptor) == %ld (expected 4)",
       FIELD_OFFSET(SECURITY_ATTRIBUTES, lpSecurityDescriptor)); /* LPVOID */
    ok(FIELD_OFFSET(SECURITY_ATTRIBUTES, bInheritHandle) == 8,
       "FIELD_OFFSET(SECURITY_ATTRIBUTES, bInheritHandle) == %ld (expected 8)",
       FIELD_OFFSET(SECURITY_ATTRIBUTES, bInheritHandle)); /* BOOL */
    ok(sizeof(SECURITY_ATTRIBUTES) == 12, "sizeof(SECURITY_ATTRIBUTES) == %d (expected 12)", sizeof(SECURITY_ATTRIBUTES));

    /* STARTUPINFOA */
    ok(FIELD_OFFSET(STARTUPINFOA, cb) == 0,
       "FIELD_OFFSET(STARTUPINFOA, cb) == %ld (expected 0)",
       FIELD_OFFSET(STARTUPINFOA, cb)); /* DWORD */
    ok(FIELD_OFFSET(STARTUPINFOA, lpReserved) == 4,
       "FIELD_OFFSET(STARTUPINFOA, lpReserved) == %ld (expected 4)",
       FIELD_OFFSET(STARTUPINFOA, lpReserved)); /* LPSTR */
    ok(FIELD_OFFSET(STARTUPINFOA, lpDesktop) == 8,
       "FIELD_OFFSET(STARTUPINFOA, lpDesktop) == %ld (expected 8)",
       FIELD_OFFSET(STARTUPINFOA, lpDesktop)); /* LPSTR */
    ok(FIELD_OFFSET(STARTUPINFOA, lpTitle) == 12,
       "FIELD_OFFSET(STARTUPINFOA, lpTitle) == %ld (expected 12)",
       FIELD_OFFSET(STARTUPINFOA, lpTitle)); /* LPSTR */
    ok(FIELD_OFFSET(STARTUPINFOA, dwX) == 16,
       "FIELD_OFFSET(STARTUPINFOA, dwX) == %ld (expected 16)",
       FIELD_OFFSET(STARTUPINFOA, dwX)); /* DWORD */
    ok(FIELD_OFFSET(STARTUPINFOA, dwY) == 20,
       "FIELD_OFFSET(STARTUPINFOA, dwY) == %ld (expected 20)",
       FIELD_OFFSET(STARTUPINFOA, dwY)); /* DWORD */
    ok(FIELD_OFFSET(STARTUPINFOA, dwXSize) == 24,
       "FIELD_OFFSET(STARTUPINFOA, dwXSize) == %ld (expected 24)",
       FIELD_OFFSET(STARTUPINFOA, dwXSize)); /* DWORD */
    ok(FIELD_OFFSET(STARTUPINFOA, dwYSize) == 28,
       "FIELD_OFFSET(STARTUPINFOA, dwYSize) == %ld (expected 28)",
       FIELD_OFFSET(STARTUPINFOA, dwYSize)); /* DWORD */
    ok(FIELD_OFFSET(STARTUPINFOA, dwXCountChars) == 32,
       "FIELD_OFFSET(STARTUPINFOA, dwXCountChars) == %ld (expected 32)",
       FIELD_OFFSET(STARTUPINFOA, dwXCountChars)); /* DWORD */
    ok(FIELD_OFFSET(STARTUPINFOA, dwYCountChars) == 36,
       "FIELD_OFFSET(STARTUPINFOA, dwYCountChars) == %ld (expected 36)",
       FIELD_OFFSET(STARTUPINFOA, dwYCountChars)); /* DWORD */
    ok(FIELD_OFFSET(STARTUPINFOA, dwFillAttribute) == 40,
       "FIELD_OFFSET(STARTUPINFOA, dwFillAttribute) == %ld (expected 40)",
       FIELD_OFFSET(STARTUPINFOA, dwFillAttribute)); /* DWORD */
    ok(FIELD_OFFSET(STARTUPINFOA, dwFlags) == 44,
       "FIELD_OFFSET(STARTUPINFOA, dwFlags) == %ld (expected 44)",
       FIELD_OFFSET(STARTUPINFOA, dwFlags)); /* DWORD */
    ok(FIELD_OFFSET(STARTUPINFOA, wShowWindow) == 48,
       "FIELD_OFFSET(STARTUPINFOA, wShowWindow) == %ld (expected 48)",
       FIELD_OFFSET(STARTUPINFOA, wShowWindow)); /* WORD */
    ok(FIELD_OFFSET(STARTUPINFOA, cbReserved2) == 50,
       "FIELD_OFFSET(STARTUPINFOA, cbReserved2) == %ld (expected 50)",
       FIELD_OFFSET(STARTUPINFOA, cbReserved2)); /* WORD */
    ok(FIELD_OFFSET(STARTUPINFOA, lpReserved2) == 52,
       "FIELD_OFFSET(STARTUPINFOA, lpReserved2) == %ld (expected 52)",
       FIELD_OFFSET(STARTUPINFOA, lpReserved2)); /* BYTE * */
    ok(FIELD_OFFSET(STARTUPINFOA, hStdInput) == 56,
       "FIELD_OFFSET(STARTUPINFOA, hStdInput) == %ld (expected 56)",
       FIELD_OFFSET(STARTUPINFOA, hStdInput)); /* HANDLE */
    ok(FIELD_OFFSET(STARTUPINFOA, hStdOutput) == 60,
       "FIELD_OFFSET(STARTUPINFOA, hStdOutput) == %ld (expected 60)",
       FIELD_OFFSET(STARTUPINFOA, hStdOutput)); /* HANDLE */
    ok(FIELD_OFFSET(STARTUPINFOA, hStdError) == 64,
       "FIELD_OFFSET(STARTUPINFOA, hStdError) == %ld (expected 64)",
       FIELD_OFFSET(STARTUPINFOA, hStdError)); /* HANDLE */
    ok(sizeof(STARTUPINFOA) == 68, "sizeof(STARTUPINFOA) == %d (expected 68)", sizeof(STARTUPINFOA));

    /* STARTUPINFOW */
    ok(FIELD_OFFSET(STARTUPINFOW, cb) == 0,
       "FIELD_OFFSET(STARTUPINFOW, cb) == %ld (expected 0)",
       FIELD_OFFSET(STARTUPINFOW, cb)); /* DWORD */
    ok(FIELD_OFFSET(STARTUPINFOW, lpReserved) == 4,
       "FIELD_OFFSET(STARTUPINFOW, lpReserved) == %ld (expected 4)",
       FIELD_OFFSET(STARTUPINFOW, lpReserved)); /* LPWSTR */
    ok(FIELD_OFFSET(STARTUPINFOW, lpDesktop) == 8,
       "FIELD_OFFSET(STARTUPINFOW, lpDesktop) == %ld (expected 8)",
       FIELD_OFFSET(STARTUPINFOW, lpDesktop)); /* LPWSTR */
    ok(FIELD_OFFSET(STARTUPINFOW, lpTitle) == 12,
       "FIELD_OFFSET(STARTUPINFOW, lpTitle) == %ld (expected 12)",
       FIELD_OFFSET(STARTUPINFOW, lpTitle)); /* LPWSTR */
    ok(FIELD_OFFSET(STARTUPINFOW, dwX) == 16,
       "FIELD_OFFSET(STARTUPINFOW, dwX) == %ld (expected 16)",
       FIELD_OFFSET(STARTUPINFOW, dwX)); /* DWORD */
    ok(FIELD_OFFSET(STARTUPINFOW, dwY) == 20,
       "FIELD_OFFSET(STARTUPINFOW, dwY) == %ld (expected 20)",
       FIELD_OFFSET(STARTUPINFOW, dwY)); /* DWORD */
    ok(FIELD_OFFSET(STARTUPINFOW, dwXSize) == 24,
       "FIELD_OFFSET(STARTUPINFOW, dwXSize) == %ld (expected 24)",
       FIELD_OFFSET(STARTUPINFOW, dwXSize)); /* DWORD */
    ok(FIELD_OFFSET(STARTUPINFOW, dwYSize) == 28,
       "FIELD_OFFSET(STARTUPINFOW, dwYSize) == %ld (expected 28)",
       FIELD_OFFSET(STARTUPINFOW, dwYSize)); /* DWORD */
    ok(FIELD_OFFSET(STARTUPINFOW, dwXCountChars) == 32,
       "FIELD_OFFSET(STARTUPINFOW, dwXCountChars) == %ld (expected 32)",
       FIELD_OFFSET(STARTUPINFOW, dwXCountChars)); /* DWORD */
    ok(FIELD_OFFSET(STARTUPINFOW, dwYCountChars) == 36,
       "FIELD_OFFSET(STARTUPINFOW, dwYCountChars) == %ld (expected 36)",
       FIELD_OFFSET(STARTUPINFOW, dwYCountChars)); /* DWORD */
    ok(FIELD_OFFSET(STARTUPINFOW, dwFillAttribute) == 40,
       "FIELD_OFFSET(STARTUPINFOW, dwFillAttribute) == %ld (expected 40)",
       FIELD_OFFSET(STARTUPINFOW, dwFillAttribute)); /* DWORD */
    ok(FIELD_OFFSET(STARTUPINFOW, dwFlags) == 44,
       "FIELD_OFFSET(STARTUPINFOW, dwFlags) == %ld (expected 44)",
       FIELD_OFFSET(STARTUPINFOW, dwFlags)); /* DWORD */
    ok(FIELD_OFFSET(STARTUPINFOW, wShowWindow) == 48,
       "FIELD_OFFSET(STARTUPINFOW, wShowWindow) == %ld (expected 48)",
       FIELD_OFFSET(STARTUPINFOW, wShowWindow)); /* WORD */
    ok(FIELD_OFFSET(STARTUPINFOW, cbReserved2) == 50,
       "FIELD_OFFSET(STARTUPINFOW, cbReserved2) == %ld (expected 50)",
       FIELD_OFFSET(STARTUPINFOW, cbReserved2)); /* WORD */
    ok(FIELD_OFFSET(STARTUPINFOW, lpReserved2) == 52,
       "FIELD_OFFSET(STARTUPINFOW, lpReserved2) == %ld (expected 52)",
       FIELD_OFFSET(STARTUPINFOW, lpReserved2)); /* BYTE * */
    ok(FIELD_OFFSET(STARTUPINFOW, hStdInput) == 56,
       "FIELD_OFFSET(STARTUPINFOW, hStdInput) == %ld (expected 56)",
       FIELD_OFFSET(STARTUPINFOW, hStdInput)); /* HANDLE */
    ok(FIELD_OFFSET(STARTUPINFOW, hStdOutput) == 60,
       "FIELD_OFFSET(STARTUPINFOW, hStdOutput) == %ld (expected 60)",
       FIELD_OFFSET(STARTUPINFOW, hStdOutput)); /* HANDLE */
    ok(FIELD_OFFSET(STARTUPINFOW, hStdError) == 64,
       "FIELD_OFFSET(STARTUPINFOW, hStdError) == %ld (expected 64)",
       FIELD_OFFSET(STARTUPINFOW, hStdError)); /* HANDLE */
    ok(sizeof(STARTUPINFOW) == 68, "sizeof(STARTUPINFOW) == %d (expected 68)", sizeof(STARTUPINFOW));

    /* SYSLEVEL */
    ok(FIELD_OFFSET(SYSLEVEL, crst) == 0,
       "FIELD_OFFSET(SYSLEVEL, crst) == %ld (expected 0)",
       FIELD_OFFSET(SYSLEVEL, crst)); /* CRITICAL_SECTION */
    ok(FIELD_OFFSET(SYSLEVEL, level) == 24,
       "FIELD_OFFSET(SYSLEVEL, level) == %ld (expected 24)",
       FIELD_OFFSET(SYSLEVEL, level)); /* INT */
    ok(sizeof(SYSLEVEL) == 28, "sizeof(SYSLEVEL) == %d (expected 28)", sizeof(SYSLEVEL));

    /* SYSTEMTIME */
    ok(FIELD_OFFSET(SYSTEMTIME, wYear) == 0,
       "FIELD_OFFSET(SYSTEMTIME, wYear) == %ld (expected 0)",
       FIELD_OFFSET(SYSTEMTIME, wYear)); /* WORD */
    ok(FIELD_OFFSET(SYSTEMTIME, wMonth) == 2,
       "FIELD_OFFSET(SYSTEMTIME, wMonth) == %ld (expected 2)",
       FIELD_OFFSET(SYSTEMTIME, wMonth)); /* WORD */
    ok(FIELD_OFFSET(SYSTEMTIME, wDayOfWeek) == 4,
       "FIELD_OFFSET(SYSTEMTIME, wDayOfWeek) == %ld (expected 4)",
       FIELD_OFFSET(SYSTEMTIME, wDayOfWeek)); /* WORD */
    ok(FIELD_OFFSET(SYSTEMTIME, wDay) == 6,
       "FIELD_OFFSET(SYSTEMTIME, wDay) == %ld (expected 6)",
       FIELD_OFFSET(SYSTEMTIME, wDay)); /* WORD */
    ok(FIELD_OFFSET(SYSTEMTIME, wHour) == 8,
       "FIELD_OFFSET(SYSTEMTIME, wHour) == %ld (expected 8)",
       FIELD_OFFSET(SYSTEMTIME, wHour)); /* WORD */
    ok(FIELD_OFFSET(SYSTEMTIME, wMinute) == 10,
       "FIELD_OFFSET(SYSTEMTIME, wMinute) == %ld (expected 10)",
       FIELD_OFFSET(SYSTEMTIME, wMinute)); /* WORD */
    ok(FIELD_OFFSET(SYSTEMTIME, wSecond) == 12,
       "FIELD_OFFSET(SYSTEMTIME, wSecond) == %ld (expected 12)",
       FIELD_OFFSET(SYSTEMTIME, wSecond)); /* WORD */
    ok(FIELD_OFFSET(SYSTEMTIME, wMilliseconds) == 14,
       "FIELD_OFFSET(SYSTEMTIME, wMilliseconds) == %ld (expected 14)",
       FIELD_OFFSET(SYSTEMTIME, wMilliseconds)); /* WORD */
    ok(sizeof(SYSTEMTIME) == 16, "sizeof(SYSTEMTIME) == %d (expected 16)", sizeof(SYSTEMTIME));

    /* SYSTEM_INFO */
    ok(FIELD_OFFSET(SYSTEM_INFO, dwPageSize) == 4,
       "FIELD_OFFSET(SYSTEM_INFO, dwPageSize) == %ld (expected 4)",
       FIELD_OFFSET(SYSTEM_INFO, dwPageSize)); /* DWORD */
    ok(FIELD_OFFSET(SYSTEM_INFO, lpMinimumApplicationAddress) == 8,
       "FIELD_OFFSET(SYSTEM_INFO, lpMinimumApplicationAddress) == %ld (expected 8)",
       FIELD_OFFSET(SYSTEM_INFO, lpMinimumApplicationAddress)); /* LPVOID */
    ok(FIELD_OFFSET(SYSTEM_INFO, lpMaximumApplicationAddress) == 12,
       "FIELD_OFFSET(SYSTEM_INFO, lpMaximumApplicationAddress) == %ld (expected 12)",
       FIELD_OFFSET(SYSTEM_INFO, lpMaximumApplicationAddress)); /* LPVOID */
    ok(FIELD_OFFSET(SYSTEM_INFO, dwActiveProcessorMask) == 16,
       "FIELD_OFFSET(SYSTEM_INFO, dwActiveProcessorMask) == %ld (expected 16)",
       FIELD_OFFSET(SYSTEM_INFO, dwActiveProcessorMask)); /* DWORD */
    ok(FIELD_OFFSET(SYSTEM_INFO, dwNumberOfProcessors) == 20,
       "FIELD_OFFSET(SYSTEM_INFO, dwNumberOfProcessors) == %ld (expected 20)",
       FIELD_OFFSET(SYSTEM_INFO, dwNumberOfProcessors)); /* DWORD */
    ok(FIELD_OFFSET(SYSTEM_INFO, dwProcessorType) == 24,
       "FIELD_OFFSET(SYSTEM_INFO, dwProcessorType) == %ld (expected 24)",
       FIELD_OFFSET(SYSTEM_INFO, dwProcessorType)); /* DWORD */
    ok(FIELD_OFFSET(SYSTEM_INFO, dwAllocationGranularity) == 28,
       "FIELD_OFFSET(SYSTEM_INFO, dwAllocationGranularity) == %ld (expected 28)",
       FIELD_OFFSET(SYSTEM_INFO, dwAllocationGranularity)); /* DWORD */
    ok(FIELD_OFFSET(SYSTEM_INFO, wProcessorLevel) == 32,
       "FIELD_OFFSET(SYSTEM_INFO, wProcessorLevel) == %ld (expected 32)",
       FIELD_OFFSET(SYSTEM_INFO, wProcessorLevel)); /* WORD */
    ok(FIELD_OFFSET(SYSTEM_INFO, wProcessorRevision) == 34,
       "FIELD_OFFSET(SYSTEM_INFO, wProcessorRevision) == %ld (expected 34)",
       FIELD_OFFSET(SYSTEM_INFO, wProcessorRevision)); /* WORD */
    ok(sizeof(SYSTEM_INFO) == 36, "sizeof(SYSTEM_INFO) == %d (expected 36)", sizeof(SYSTEM_INFO));

    /* SYSTEM_POWER_STATUS */
    ok(FIELD_OFFSET(SYSTEM_POWER_STATUS, ACLineStatus) == 0,
       "FIELD_OFFSET(SYSTEM_POWER_STATUS, ACLineStatus) == %ld (expected 0)",
       FIELD_OFFSET(SYSTEM_POWER_STATUS, ACLineStatus)); /* BYTE */
    ok(FIELD_OFFSET(SYSTEM_POWER_STATUS, BatteryFlag) == 1,
       "FIELD_OFFSET(SYSTEM_POWER_STATUS, BatteryFlag) == %ld (expected 1)",
       FIELD_OFFSET(SYSTEM_POWER_STATUS, BatteryFlag)); /* BYTE */
    ok(FIELD_OFFSET(SYSTEM_POWER_STATUS, BatteryLifePercent) == 2,
       "FIELD_OFFSET(SYSTEM_POWER_STATUS, BatteryLifePercent) == %ld (expected 2)",
       FIELD_OFFSET(SYSTEM_POWER_STATUS, BatteryLifePercent)); /* BYTE */
    ok(FIELD_OFFSET(SYSTEM_POWER_STATUS, reserved) == 3,
       "FIELD_OFFSET(SYSTEM_POWER_STATUS, reserved) == %ld (expected 3)",
       FIELD_OFFSET(SYSTEM_POWER_STATUS, reserved)); /* BYTE */
    ok(FIELD_OFFSET(SYSTEM_POWER_STATUS, BatteryLifeTime) == 4,
       "FIELD_OFFSET(SYSTEM_POWER_STATUS, BatteryLifeTime) == %ld (expected 4)",
       FIELD_OFFSET(SYSTEM_POWER_STATUS, BatteryLifeTime)); /* DWORD */
    ok(FIELD_OFFSET(SYSTEM_POWER_STATUS, BatteryFullLifeTime) == 8,
       "FIELD_OFFSET(SYSTEM_POWER_STATUS, BatteryFullLifeTime) == %ld (expected 8)",
       FIELD_OFFSET(SYSTEM_POWER_STATUS, BatteryFullLifeTime)); /* DWORD */
    ok(sizeof(SYSTEM_POWER_STATUS) == 12, "sizeof(SYSTEM_POWER_STATUS) == %d (expected 12)", sizeof(SYSTEM_POWER_STATUS));

    /* TIME_ZONE_INFORMATION */
    ok(FIELD_OFFSET(TIME_ZONE_INFORMATION, Bias) == 0,
       "FIELD_OFFSET(TIME_ZONE_INFORMATION, Bias) == %ld (expected 0)",
       FIELD_OFFSET(TIME_ZONE_INFORMATION, Bias)); /* LONG */
    ok(FIELD_OFFSET(TIME_ZONE_INFORMATION, StandardName) == 4,
       "FIELD_OFFSET(TIME_ZONE_INFORMATION, StandardName) == %ld (expected 4)",
       FIELD_OFFSET(TIME_ZONE_INFORMATION, StandardName)); /* WCHAR[32] */
    ok(FIELD_OFFSET(TIME_ZONE_INFORMATION, StandardDate) == 68,
       "FIELD_OFFSET(TIME_ZONE_INFORMATION, StandardDate) == %ld (expected 68)",
       FIELD_OFFSET(TIME_ZONE_INFORMATION, StandardDate)); /* SYSTEMTIME */
    ok(FIELD_OFFSET(TIME_ZONE_INFORMATION, StandardBias) == 84,
       "FIELD_OFFSET(TIME_ZONE_INFORMATION, StandardBias) == %ld (expected 84)",
       FIELD_OFFSET(TIME_ZONE_INFORMATION, StandardBias)); /* LONG */
    ok(FIELD_OFFSET(TIME_ZONE_INFORMATION, DaylightName) == 88,
       "FIELD_OFFSET(TIME_ZONE_INFORMATION, DaylightName) == %ld (expected 88)",
       FIELD_OFFSET(TIME_ZONE_INFORMATION, DaylightName)); /* WCHAR[32] */
    ok(FIELD_OFFSET(TIME_ZONE_INFORMATION, DaylightDate) == 152,
       "FIELD_OFFSET(TIME_ZONE_INFORMATION, DaylightDate) == %ld (expected 152)",
       FIELD_OFFSET(TIME_ZONE_INFORMATION, DaylightDate)); /* SYSTEMTIME */
    ok(FIELD_OFFSET(TIME_ZONE_INFORMATION, DaylightBias) == 168,
       "FIELD_OFFSET(TIME_ZONE_INFORMATION, DaylightBias) == %ld (expected 168)",
       FIELD_OFFSET(TIME_ZONE_INFORMATION, DaylightBias)); /* LONG */
    ok(sizeof(TIME_ZONE_INFORMATION) == 172, "sizeof(TIME_ZONE_INFORMATION) == %d (expected 172)", sizeof(TIME_ZONE_INFORMATION));

    /* UNLOAD_DLL_DEBUG_INFO */
    ok(FIELD_OFFSET(UNLOAD_DLL_DEBUG_INFO, lpBaseOfDll) == 0,
       "FIELD_OFFSET(UNLOAD_DLL_DEBUG_INFO, lpBaseOfDll) == %ld (expected 0)",
       FIELD_OFFSET(UNLOAD_DLL_DEBUG_INFO, lpBaseOfDll)); /* LPVOID */
    ok(sizeof(UNLOAD_DLL_DEBUG_INFO) == 4, "sizeof(UNLOAD_DLL_DEBUG_INFO) == %d (expected 4)", sizeof(UNLOAD_DLL_DEBUG_INFO));

    /* WIN32_FILE_ATTRIBUTE_DATA */
    ok(FIELD_OFFSET(WIN32_FILE_ATTRIBUTE_DATA, dwFileAttributes) == 0,
       "FIELD_OFFSET(WIN32_FILE_ATTRIBUTE_DATA, dwFileAttributes) == %ld (expected 0)",
       FIELD_OFFSET(WIN32_FILE_ATTRIBUTE_DATA, dwFileAttributes)); /* DWORD */
    ok(FIELD_OFFSET(WIN32_FILE_ATTRIBUTE_DATA, ftCreationTime) == 4,
       "FIELD_OFFSET(WIN32_FILE_ATTRIBUTE_DATA, ftCreationTime) == %ld (expected 4)",
       FIELD_OFFSET(WIN32_FILE_ATTRIBUTE_DATA, ftCreationTime)); /* FILETIME */
    ok(FIELD_OFFSET(WIN32_FILE_ATTRIBUTE_DATA, ftLastAccessTime) == 12,
       "FIELD_OFFSET(WIN32_FILE_ATTRIBUTE_DATA, ftLastAccessTime) == %ld (expected 12)",
       FIELD_OFFSET(WIN32_FILE_ATTRIBUTE_DATA, ftLastAccessTime)); /* FILETIME */
    ok(FIELD_OFFSET(WIN32_FILE_ATTRIBUTE_DATA, ftLastWriteTime) == 20,
       "FIELD_OFFSET(WIN32_FILE_ATTRIBUTE_DATA, ftLastWriteTime) == %ld (expected 20)",
       FIELD_OFFSET(WIN32_FILE_ATTRIBUTE_DATA, ftLastWriteTime)); /* FILETIME */
    ok(FIELD_OFFSET(WIN32_FILE_ATTRIBUTE_DATA, nFileSizeHigh) == 28,
       "FIELD_OFFSET(WIN32_FILE_ATTRIBUTE_DATA, nFileSizeHigh) == %ld (expected 28)",
       FIELD_OFFSET(WIN32_FILE_ATTRIBUTE_DATA, nFileSizeHigh)); /* DWORD */
    ok(FIELD_OFFSET(WIN32_FILE_ATTRIBUTE_DATA, nFileSizeLow) == 32,
       "FIELD_OFFSET(WIN32_FILE_ATTRIBUTE_DATA, nFileSizeLow) == %ld (expected 32)",
       FIELD_OFFSET(WIN32_FILE_ATTRIBUTE_DATA, nFileSizeLow)); /* DWORD */
    ok(sizeof(WIN32_FILE_ATTRIBUTE_DATA) == 36, "sizeof(WIN32_FILE_ATTRIBUTE_DATA) == %d (expected 36)", sizeof(WIN32_FILE_ATTRIBUTE_DATA));

    /* WIN32_FIND_DATAA */
    ok(FIELD_OFFSET(WIN32_FIND_DATAA, dwFileAttributes) == 0,
       "FIELD_OFFSET(WIN32_FIND_DATAA, dwFileAttributes) == %ld (expected 0)",
       FIELD_OFFSET(WIN32_FIND_DATAA, dwFileAttributes)); /* DWORD */
    ok(FIELD_OFFSET(WIN32_FIND_DATAA, ftCreationTime) == 4,
       "FIELD_OFFSET(WIN32_FIND_DATAA, ftCreationTime) == %ld (expected 4)",
       FIELD_OFFSET(WIN32_FIND_DATAA, ftCreationTime)); /* FILETIME */
    ok(FIELD_OFFSET(WIN32_FIND_DATAA, ftLastAccessTime) == 12,
       "FIELD_OFFSET(WIN32_FIND_DATAA, ftLastAccessTime) == %ld (expected 12)",
       FIELD_OFFSET(WIN32_FIND_DATAA, ftLastAccessTime)); /* FILETIME */
    ok(FIELD_OFFSET(WIN32_FIND_DATAA, ftLastWriteTime) == 20,
       "FIELD_OFFSET(WIN32_FIND_DATAA, ftLastWriteTime) == %ld (expected 20)",
       FIELD_OFFSET(WIN32_FIND_DATAA, ftLastWriteTime)); /* FILETIME */
    ok(FIELD_OFFSET(WIN32_FIND_DATAA, nFileSizeHigh) == 28,
       "FIELD_OFFSET(WIN32_FIND_DATAA, nFileSizeHigh) == %ld (expected 28)",
       FIELD_OFFSET(WIN32_FIND_DATAA, nFileSizeHigh)); /* DWORD */
    ok(FIELD_OFFSET(WIN32_FIND_DATAA, nFileSizeLow) == 32,
       "FIELD_OFFSET(WIN32_FIND_DATAA, nFileSizeLow) == %ld (expected 32)",
       FIELD_OFFSET(WIN32_FIND_DATAA, nFileSizeLow)); /* DWORD */
    ok(FIELD_OFFSET(WIN32_FIND_DATAA, dwReserved0) == 36,
       "FIELD_OFFSET(WIN32_FIND_DATAA, dwReserved0) == %ld (expected 36)",
       FIELD_OFFSET(WIN32_FIND_DATAA, dwReserved0)); /* DWORD */
    ok(FIELD_OFFSET(WIN32_FIND_DATAA, dwReserved1) == 40,
       "FIELD_OFFSET(WIN32_FIND_DATAA, dwReserved1) == %ld (expected 40)",
       FIELD_OFFSET(WIN32_FIND_DATAA, dwReserved1)); /* DWORD */
    ok(FIELD_OFFSET(WIN32_FIND_DATAA, cFileName) == 44,
       "FIELD_OFFSET(WIN32_FIND_DATAA, cFileName) == %ld (expected 44)",
       FIELD_OFFSET(WIN32_FIND_DATAA, cFileName)); /* CHAR[260] */
    ok(FIELD_OFFSET(WIN32_FIND_DATAA, cAlternateFileName) == 304,
       "FIELD_OFFSET(WIN32_FIND_DATAA, cAlternateFileName) == %ld (expected 304)",
       FIELD_OFFSET(WIN32_FIND_DATAA, cAlternateFileName)); /* CHAR[14] */
    ok(sizeof(WIN32_FIND_DATAA) == 320, "sizeof(WIN32_FIND_DATAA) == %d (expected 320)", sizeof(WIN32_FIND_DATAA));

    /* WIN32_FIND_DATAW */
    ok(FIELD_OFFSET(WIN32_FIND_DATAW, dwFileAttributes) == 0,
       "FIELD_OFFSET(WIN32_FIND_DATAW, dwFileAttributes) == %ld (expected 0)",
       FIELD_OFFSET(WIN32_FIND_DATAW, dwFileAttributes)); /* DWORD */
    ok(FIELD_OFFSET(WIN32_FIND_DATAW, ftCreationTime) == 4,
       "FIELD_OFFSET(WIN32_FIND_DATAW, ftCreationTime) == %ld (expected 4)",
       FIELD_OFFSET(WIN32_FIND_DATAW, ftCreationTime)); /* FILETIME */
    ok(FIELD_OFFSET(WIN32_FIND_DATAW, ftLastAccessTime) == 12,
       "FIELD_OFFSET(WIN32_FIND_DATAW, ftLastAccessTime) == %ld (expected 12)",
       FIELD_OFFSET(WIN32_FIND_DATAW, ftLastAccessTime)); /* FILETIME */
    ok(FIELD_OFFSET(WIN32_FIND_DATAW, ftLastWriteTime) == 20,
       "FIELD_OFFSET(WIN32_FIND_DATAW, ftLastWriteTime) == %ld (expected 20)",
       FIELD_OFFSET(WIN32_FIND_DATAW, ftLastWriteTime)); /* FILETIME */
    ok(FIELD_OFFSET(WIN32_FIND_DATAW, nFileSizeHigh) == 28,
       "FIELD_OFFSET(WIN32_FIND_DATAW, nFileSizeHigh) == %ld (expected 28)",
       FIELD_OFFSET(WIN32_FIND_DATAW, nFileSizeHigh)); /* DWORD */
    ok(FIELD_OFFSET(WIN32_FIND_DATAW, nFileSizeLow) == 32,
       "FIELD_OFFSET(WIN32_FIND_DATAW, nFileSizeLow) == %ld (expected 32)",
       FIELD_OFFSET(WIN32_FIND_DATAW, nFileSizeLow)); /* DWORD */
    ok(FIELD_OFFSET(WIN32_FIND_DATAW, dwReserved0) == 36,
       "FIELD_OFFSET(WIN32_FIND_DATAW, dwReserved0) == %ld (expected 36)",
       FIELD_OFFSET(WIN32_FIND_DATAW, dwReserved0)); /* DWORD */
    ok(FIELD_OFFSET(WIN32_FIND_DATAW, dwReserved1) == 40,
       "FIELD_OFFSET(WIN32_FIND_DATAW, dwReserved1) == %ld (expected 40)",
       FIELD_OFFSET(WIN32_FIND_DATAW, dwReserved1)); /* DWORD */
    ok(FIELD_OFFSET(WIN32_FIND_DATAW, cFileName) == 44,
       "FIELD_OFFSET(WIN32_FIND_DATAW, cFileName) == %ld (expected 44)",
       FIELD_OFFSET(WIN32_FIND_DATAW, cFileName)); /* WCHAR[260] */
    ok(FIELD_OFFSET(WIN32_FIND_DATAW, cAlternateFileName) == 564,
       "FIELD_OFFSET(WIN32_FIND_DATAW, cAlternateFileName) == %ld (expected 564)",
       FIELD_OFFSET(WIN32_FIND_DATAW, cAlternateFileName)); /* WCHAR[14] */
    ok(sizeof(WIN32_FIND_DATAW) == 592, "sizeof(WIN32_FIND_DATAW) == %d (expected 592)", sizeof(WIN32_FIND_DATAW));

    /* WIN32_STREAM_ID */
    ok(FIELD_OFFSET(WIN32_STREAM_ID, dwStreamID) == 0,
       "FIELD_OFFSET(WIN32_STREAM_ID, dwStreamID) == %ld (expected 0)",
       FIELD_OFFSET(WIN32_STREAM_ID, dwStreamID)); /* DWORD */
    ok(FIELD_OFFSET(WIN32_STREAM_ID, dwStreamAttributes) == 4,
       "FIELD_OFFSET(WIN32_STREAM_ID, dwStreamAttributes) == %ld (expected 4)",
       FIELD_OFFSET(WIN32_STREAM_ID, dwStreamAttributes)); /* DWORD */
    ok(FIELD_OFFSET(WIN32_STREAM_ID, Size) == 8,
       "FIELD_OFFSET(WIN32_STREAM_ID, Size) == %ld (expected 8)",
       FIELD_OFFSET(WIN32_STREAM_ID, Size)); /* LARGE_INTEGER */
    ok(FIELD_OFFSET(WIN32_STREAM_ID, dwStreamNameSize) == 16,
       "FIELD_OFFSET(WIN32_STREAM_ID, dwStreamNameSize) == %ld (expected 16)",
       FIELD_OFFSET(WIN32_STREAM_ID, dwStreamNameSize)); /* DWORD */
    ok(FIELD_OFFSET(WIN32_STREAM_ID, cStreamName) == 20,
       "FIELD_OFFSET(WIN32_STREAM_ID, cStreamName) == %ld (expected 20)",
       FIELD_OFFSET(WIN32_STREAM_ID, cStreamName)); /* WCHAR[ANYSIZE_ARRAY] */
    ok(sizeof(WIN32_STREAM_ID) == 24, "sizeof(WIN32_STREAM_ID) == %d (expected 24)", sizeof(WIN32_STREAM_ID));

}
