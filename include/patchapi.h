/*
 * Copyright 2011 Hans Leidekker for CodeWeavers
 * Copyright 2019 Conor McCarthy
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

#ifndef _PATCHAPI_H_
#define _PATCHAPI_H_

#ifdef __cplusplus
extern "C" {
#endif

#define PATCH_OPTION_USE_LZX_A        0x00000001
#define PATCH_OPTION_USE_LZX_B        0x00000002
#define PATCH_OPTION_USE_LZX_LARGE    0x00000004 /* raise maximum window from 8 -> 32 Mb */

#define PATCH_OPTION_NO_BINDFIX        0x00010000
#define PATCH_OPTION_NO_LOCKFIX        0x00020000
#define PATCH_OPTION_NO_REBASE         0x00040000
#define PATCH_OPTION_FAIL_IF_SAME_FILE 0x00080000
#define PATCH_OPTION_FAIL_IF_BIGGER    0x00100000
#define PATCH_OPTION_NO_CHECKSUM       0x00200000
#define PATCH_OPTION_NO_RESTIMEFIX     0x00400000
#define PATCH_OPTION_NO_TIMESTAMP      0x00800000
#define PATCH_OPTION_INTERLEAVE_FILES  0x40000000
#define PATCH_OPTION_RESERVED1         0x80000000

#define APPLY_OPTION_FAIL_IF_EXACT  0x00000001
#define APPLY_OPTION_FAIL_IF_CLOSE  0x00000002
#define APPLY_OPTION_TEST_ONLY      0x00000004
#define APPLY_OPTION_VALID_FLAGS    0x00000007

#define ERROR_PATCH_DECODE_FAILURE 0xC00E4101
#define ERROR_PATCH_CORRUPT        0xC00E4102
#define ERROR_PATCH_NEWER_FORMAT   0xC00E4103
#define ERROR_PATCH_WRONG_FILE     0xC00E4104
#define ERROR_PATCH_NOT_NECESSARY  0xC00E4105
#define ERROR_PATCH_NOT_AVAILABLE  0xC00E4106

typedef struct _PATCH_IGNORE_RANGE
{
    ULONG OffsetInOldFile;
    ULONG LengthInBytes;
} PATCH_IGNORE_RANGE, *PPATCH_IGNORE_RANGE;

typedef struct _PATCH_RETAIN_RANGE
{
    ULONG OffsetInOldFile;
    ULONG LengthInBytes;
    ULONG OffsetInNewFile;
} PATCH_RETAIN_RANGE, *PPATCH_RETAIN_RANGE;

typedef struct _PATCH_INTERLEAVE_MAP {
    ULONG CountRanges;
    struct {
        ULONG OldOffset;
        ULONG OldLength;
        ULONG NewLength;
    } Range[1];
} PATCH_INTERLEAVE_MAP, *PPATCH_INTERLEAVE_MAP;

typedef BOOL(CALLBACK PATCH_SYMLOAD_CALLBACK)(ULONG, LPCSTR, ULONG, ULONG, ULONG, ULONG, ULONG, PVOID);

typedef PATCH_SYMLOAD_CALLBACK *PPATCH_SYMLOAD_CALLBACK;

typedef struct _PATCH_OPTION_DATA {
    ULONG SizeOfThisStruct;
    ULONG SymbolOptionFlags;
    LPCSTR NewFileSymbolPath;
    LPCSTR *OldFileSymbolPathArray;
    ULONG ExtendedOptionFlags;
    PPATCH_SYMLOAD_CALLBACK SymLoadCallback;
    PVOID SymLoadContext;
    PPATCH_INTERLEAVE_MAP* InterleaveMapArray;
    ULONG MaxLzxWindowSize;
} PATCH_OPTION_DATA, *PPATCH_OPTION_DATA;

typedef BOOL (CALLBACK PATCH_PROGRESS_CALLBACK)(PVOID, ULONG, ULONG);

typedef PATCH_PROGRESS_CALLBACK *PPATCH_PROGRESS_CALLBACK;

BOOL WINAPI ApplyPatchToFileA(LPCSTR,LPCSTR,LPCSTR,ULONG);
BOOL WINAPI ApplyPatchToFileW(LPCWSTR,LPCWSTR,LPCWSTR,ULONG);
#define     ApplyPatchToFile WINELIB_NAME_AW(ApplyPatchToFile)

BOOL WINAPI ApplyPatchToFileByHandles(HANDLE, HANDLE, HANDLE, ULONG);
BOOL WINAPI ApplyPatchToFileExA(LPCSTR, LPCSTR, LPCSTR, ULONG, PPATCH_PROGRESS_CALLBACK, PVOID);
BOOL WINAPI ApplyPatchToFileExW(LPCWSTR, LPCWSTR, LPCWSTR, ULONG, PPATCH_PROGRESS_CALLBACK, PVOID);
#define     ApplyPatchToFileEx WINELIB_NAME_AW(ApplyPatchToFileEx)
BOOL WINAPI ApplyPatchToFileByHandlesEx(HANDLE, HANDLE, HANDLE, ULONG, PPATCH_PROGRESS_CALLBACK, PVOID);
BOOL WINAPI ApplyPatchToFileByBuffers(PBYTE, ULONG, PBYTE, ULONG, PBYTE*, ULONG, ULONG*, FILETIME*, ULONG,
                                      PPATCH_PROGRESS_CALLBACK, PVOID);

BOOL WINAPI TestApplyPatchToFileA(LPCSTR, LPCSTR, ULONG);
BOOL WINAPI TestApplyPatchToFileW(LPCWSTR, LPCWSTR, ULONG);
#define     TestApplyPatchToFile WINELIB_NAME_AW(TestApplyPatchToFile)
BOOL WINAPI TestApplyPatchToFileByHandles(HANDLE, HANDLE, ULONG);
BOOL WINAPI TestApplyPatchToFileByBuffers(PBYTE, ULONG, PBYTE, ULONG, ULONG*, ULONG);

BOOL WINAPI GetFilePatchSignatureA(LPCSTR, ULONG, PVOID, ULONG, PPATCH_IGNORE_RANGE, ULONG,
                                   PPATCH_RETAIN_RANGE, ULONG, LPSTR);
BOOL WINAPI GetFilePatchSignatureW(LPCWSTR, ULONG, PVOID, ULONG, PPATCH_IGNORE_RANGE, ULONG,
                                   PPATCH_RETAIN_RANGE, ULONG, LPWSTR);
#define     GetFilePatchSignature WINELIB_NAME_AW(GetFilePatchSignature)

BOOL WINAPI GetFilePatchSignatureByHandle(HANDLE, ULONG, PVOID, ULONG, PPATCH_IGNORE_RANGE,
                                          ULONG, PPATCH_RETAIN_RANGE, ULONG, LPSTR);
BOOL WINAPI GetFilePatchSignatureByBuffer(PBYTE, ULONG, ULONG, PVOID, ULONG, PPATCH_IGNORE_RANGE, ULONG,
                                          PPATCH_RETAIN_RANGE, ULONG, LPSTR);
INT WINAPI NormalizeFileForPatchSignature(PVOID, ULONG, ULONG, PATCH_OPTION_DATA*, ULONG,
                                          ULONG, ULONG, PPATCH_IGNORE_RANGE, ULONG, PPATCH_RETAIN_RANGE);

#ifdef __cplusplus
}
#endif

#endif /* _PATCHAPI_H_ */
