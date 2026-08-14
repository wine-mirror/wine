/*
 * Copyright 2024 Vijay Kiran Kamuju
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

#ifndef _MSDELTA_H_
#define _MSDELTA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include <wincrypt.h>

typedef INT64 DELTA_FILE_TYPE;
typedef INT64 DELTA_FLAG_TYPE;

#define DELTA_FILE_SIZE_LIMIT         (32*1024*1024)
#define DELTA_OPTIONS_SIZE_LIMIT      (128*1024*1024)
#define DELTA_MAX_HASH_SIZE           32

#define DELTA_FILE_TYPE_RAW                  ((DELTA_FILE_TYPE)0x00000001)
#define DELTA_FILE_TYPE_I386                 ((DELTA_FILE_TYPE)0x00000002)
#define DELTA_FILE_TYPE_IA64                 ((DELTA_FILE_TYPE)0x00000004)
#define DELTA_FILE_TYPE_AMD64                ((DELTA_FILE_TYPE)0x00000008)
#define DELTA_FILE_TYPE_CLI_I386             ((DELTA_FILE_TYPE)0x00000010)
#define DELTA_FILE_TYPE_CLI_AMD64            ((DELTA_FILE_TYPE)0x00000020)
#define DELTA_FILE_TYPE_CLI_ARM              ((DELTA_FILE_TYPE)0x00000040)
#define DELTA_FILE_TYPE_CLI_ARM64            ((DELTA_FILE_TYPE)0x00000080)

#define DELTA_FILE_TYPE_SET_RAW_ONLY           (DELTA_FILE_TYPE_RAW)
#define DELTA_FILE_TYPE_SET_EXECUTABLES_1      (DELTA_FILE_TYPE_RAW|DELTA_FILE_TYPE_I386| \
                                                DELTA_FILE_TYPE_IA64|DELTA_FILE_TYPE_AMD64)
#define DELTA_FILE_TYPE_SET_EXECUTABLES        (DELTA_FILE_TYPE_SET_EXECUTABLES_1)
#define DELTA_FILE_TYPE_SET_EXECUTABLES_2      (DELTA_FILE_TYPE_RAW|DELTA_FILE_TYPE_CLI_I386| \
                                                DELTA_FILE_TYPE_IA64|DELTA_FILE_TYPE_CLI_AMD64| \
                                                DELTA_FILE_TYPE_CLI_ARM)
#define DELTA_FILE_TYPE_SET_EXECUTABLES_3      (DELTA_FILE_TYPE_RAW|DELTA_FILE_TYPE_CLI_I386| \
                                                DELTA_FILE_TYPE_IA64|DELTA_FILE_TYPE_CLI_AMD64| \
                                                DELTA_FILE_TYPE_CLI_ARM|DELTA_FILE_TYPE_CLI_ARM64)
#define DELTA_FILE_TYPE_SET_EXECUTABLES_LATEST (DELTA_FILE_TYPE_SET_EXECUTABLES_3)

#define DELTA_FLAG_NONE                      ((DELTA_FLAG_TYPE)0x00000000)
#define DELTA_APPLY_FLAG_ALLOW_PA19          ((DELTA_FLAG_TYPE)0x00000001)
#define DELTA_FLAG_E8                        ((DELTA_FLAG_TYPE)0x00000001)
#define DELTA_FLAG_MARK                      ((DELTA_FLAG_TYPE)0x00000002)
#define DELTA_FLAG_IMPORTS                   ((DELTA_FLAG_TYPE)0x00000004)
#define DELTA_FLAG_EXPORTS                   ((DELTA_FLAG_TYPE)0x00000008)
#define DELTA_FLAG_RESOURCES                 ((DELTA_FLAG_TYPE)0x00000010)
#define DELTA_FLAG_RELOCS                    ((DELTA_FLAG_TYPE)0x00000020)
#define DELTA_FLAG_I386_SMASHLOCK            ((DELTA_FLAG_TYPE)0x00000040)
#define DELTA_FLAG_I386_JMPS                 ((DELTA_FLAG_TYPE)0x00000080)
#define DELTA_FLAG_I386_CALLS                ((DELTA_FLAG_TYPE)0x00000100)
#define DELTA_FLAG_AMD64_DISASM              ((DELTA_FLAG_TYPE)0x00000200)
#define DELTA_FLAG_AMD64_PDATA               ((DELTA_FLAG_TYPE)0x00000400)
#define DELTA_FLAG_IA64_DISASM               ((DELTA_FLAG_TYPE)0x00000800)
#define DELTA_FLAG_IA64_PDATA                ((DELTA_FLAG_TYPE)0x00001000)
#define DELTA_FLAG_UNBIND                    ((DELTA_FLAG_TYPE)0x00002000)
#define DELTA_FLAG_CLI_DISASM                ((DELTA_FLAG_TYPE)0x00004000)
#define DELTA_FLAG_CLI_METADATA              ((DELTA_FLAG_TYPE)0x00008000)
#define DELTA_FLAG_CLI_HEADERS               ((DELTA_FLAG_TYPE)0x00010000)
#define DELTA_FLAG_IGNORE_FILE_SIZE_LIMIT    ((DELTA_FLAG_TYPE)0x00020000)
#define DELTA_FLAG_IGNORE_OPTIONS_SIZE_LIMIT ((DELTA_FLAG_TYPE)0x00040000)
#define DELTA_FLAG_ARM_DISASM                ((DELTA_FLAG_TYPE)0x00080000)
#define DELTA_FLAG_ARM_PDATA                 ((DELTA_FLAG_TYPE)0x00100000)
#define DELTA_FLAG_CLI4_METADATA             ((DELTA_FLAG_TYPE)0x00200000)
#define DELTA_FLAG_CLI4_DISASM               ((DELTA_FLAG_TYPE)0x00400000)
#define DELTA_FLAG_ARM64_DISASM              ((DELTA_FLAG_TYPE)0x00800000)
#define DELTA_FLAG_ARM64_PDATA               ((DELTA_FLAG_TYPE)0x01000000)

#define DELTA_DEFAULT_FLAGS_RAW              (DELTA_FLAG_NONE)
#define DELTA_DEFAULT_FLAGS_I386             (DELTA_FLAG_MARK|DELTA_FLAG_IMPORTS| \
                                              DELTA_FLAG_EXPORTS|DELTA_FLAG_RESOURCES| \
                                              DELTA_FLAG_RELOCS|DELTA_FLAG_I386_SMASHLOCK| \
                                              DELTA_FLAG_I386_JMPS|DELTA_FLAG_I386_CALLS| \
                                              DELTA_FLAG_UNBIND|DELTA_FLAG_CLI_DISASM| \
                                              DELTA_FLAG_CLI_METADATA)
#define DELTA_DEFAULT_FLAGS_IA64             (DELTA_FLAG_MARK|DELTA_FLAG_IMPORTS| \
                                              DELTA_FLAG_EXPORTS|DELTA_FLAG_RESOURCES| \
                                              DELTA_FLAG_RELOCS|DELTA_FLAG_IA64_DISASM| \
                                              DELTA_FLAG_IA64_PDATA| DELTA_FLAG_UNBIND| \
                                              DELTA_FLAG_CLI_DISASM|DELTA_FLAG_CLI_METADATA)
#define DELTA_DEFAULT_FLAGS_AMD64            (DELTA_FLAG_MARK|DELTA_FLAG_IMPORTS| \
                                              DELTA_FLAG_EXPORTS|DELTA_FLAG_RESOURCES| \
                                              DELTA_FLAG_RELOCS|DELTA_FLAG_AMD64_DISASM| \
                                              DELTA_FLAG_AMD64_PDATA| DELTA_FLAG_UNBIND| \
                                              DELTA_FLAG_CLI_DISASM|DELTA_FLAG_CLI_METADATA)
#define DELTA_CLI4_FLAGS_I386                (DELTA_FLAG_MARK|DELTA_FLAG_IMPORTS| \
                                              DELTA_FLAG_EXPORTS|DELTA_FLAG_RESOURCES| \
                                              DELTA_FLAG_RELOCS|DELTA_FLAG_I386_SMASHLOCK| \
                                              DELTA_FLAG_I386_JMPS|DELTA_FLAG_I386_CALLS| \
                                              DELTA_FLAG_UNBIND|DELTA_FLAG_CLI4_DISASM| \
                                              DELTA_FLAG_CLI4_METADATA)
#define DELTA_CLI4_FLAGS_AMD64               (DELTA_FLAG_MARK|DELTA_FLAG_IMPORTS| \
                                              DELTA_FLAG_EXPORTS|DELTA_FLAG_RESOURCES| \
                                              DELTA_FLAG_RELOCS|DELTA_FLAG_AMD64_DISASM| \
                                              DELTA_FLAG_AMD64_PDATA| DELTA_FLAG_UNBIND| \
                                              DELTA_FLAG_CLI4_DISASM|DELTA_FLAG_CLI4_METADATA)
#define DELTA_CLI4_FLAGS_ARM                 (DELTA_FLAG_MARK|DELTA_FLAG_IMPORTS| \
                                              DELTA_FLAG_EXPORTS|DELTA_FLAG_RESOURCES| \
                                              DELTA_FLAG_RELOCS|DELTA_FLAG_ARM_DISASM| \
                                              DELTA_FLAG_ARM_PDATA| DELTA_FLAG_UNBIND| \
                                              DELTA_FLAG_CLI4_DISASM|DELTA_FLAG_CLI4_METADATA)
#define DELTA_CLI4_FLAGS_ARM64               (DELTA_FLAG_MARK|DELTA_FLAG_IMPORTS| \
                                              DELTA_FLAG_EXPORTS|DELTA_FLAG_RESOURCES| \
                                              DELTA_FLAG_RELOCS|DELTA_FLAG_ARM64_DISASM| \
                                              DELTA_FLAG_ARM64_PDATA| DELTA_FLAG_UNBIND| \
                                              DELTA_FLAG_CLI4_DISASM|DELTA_FLAG_CLI4_METADATA)

typedef struct _DELTA_INPUT
{
    union {
        LPCVOID lpcStart;
        LPVOID lpStart;
    };
    SIZE_T uSize;
    BOOL Editable;
} DELTA_INPUT;
typedef DELTA_INPUT *LPDELTA_INPUT;
typedef const DELTA_INPUT *LPCDELTA_INPUT;

typedef struct _DELTA_OUTPUT
{
    LPVOID lpStart;
    SIZE_T uSize;
} DELTA_OUTPUT;
typedef DELTA_OUTPUT *LPDELTA_OUTPUT;
typedef const DELTA_OUTPUT *LPCDELTA_OUTPUT;

typedef struct _DELTA_HASH
{
    DWORD HashSize;
    UCHAR HashValue[DELTA_MAX_HASH_SIZE];
} DELTA_HASH;
typedef DELTA_HASH *LPDELTA_HASH;
typedef const DELTA_HASH *LPCDELTA_HASH;

typedef struct _DELTA_HEADER_INFO
{
    DELTA_FILE_TYPE FileTypeSet;
    DELTA_FILE_TYPE FileType;
    DELTA_FLAG_TYPE Flags;
    SIZE_T TargetSize;
    FILETIME TargetFileTime;
    ALG_ID TargetHashAlgId;
    DELTA_HASH TargetHash;
} DELTA_HEADER_INFO;
typedef DELTA_HEADER_INFO *LPDELTA_HEADER_INFO;
typedef const DELTA_HEADER_INFO *LPCDELTA_HEADER_INFO;

#ifdef __cplusplus
}
#endif

#endif /* _MSDELTA_H_ */
