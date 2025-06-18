/*
 * Copyright (C) 2014 Alistair Leslie-Hughes
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

#ifndef __NTIFS_H__
#define __NTIFS_H__

#include "ntddk.h"

#define PHCM_ERROR_INVALID_PARAMETER    ((char)-1)
#define PHCM_ERROR_NO_TEB               ((char)-2)
#define PHCM_ERROR_NO_PEB               ((char)-3)
#define PHCM_APPLICATION_DEFAULT        ((char)0)
#define PHCM_DISGUISE_PLACEHOLDERS      ((char)1)
#define PHCM_EXPOSE_PLACEHOLDERS        ((char)2)
#define PHCM_DISGUISE_FULL_PLACEHOLDERS ((char)3)
#define PHCM_MAX                        ((char)3)

#define SYMLINK_FLAG_RELATIVE 0x00000001

typedef struct _EX_PUSH_LOCK EX_PUSH_LOCK, *PEX_PUSH_LOCK;

typedef enum _FS_FILTER_SECTION_SYNC_TYPE
{
    SyncTypeOther = 0,
    SyncTypeCreateSection
} FS_FILTER_SECTION_SYNC_TYPE, *PFS_FILTER_SECTION_SYNC_TYPE;

typedef struct _FS_FILTER_SECTION_SYNC_OUTPUT
{
    ULONG StructureSize;
    ULONG SizeReturned;
    ULONG Flags;
    ULONG DesiredReadAlignment;
} FS_FILTER_SECTION_SYNC_OUTPUT, *PFS_FILTER_SECTION_SYNC_OUTPUT;

typedef struct _KQUEUE
{
  DISPATCHER_HEADER Header;
  LIST_ENTRY EntryListHead;
  volatile ULONG CurrentCount;
  ULONG MaximumCount;
  LIST_ENTRY ThreadListHead;
} KQUEUE, *PKQUEUE, *RESTRICTED_POINTER PRKQUEUE;

typedef enum _FS_FILTER_STREAM_FO_NOTIFICATION_TYPE
{
    NotifyTypeCreate  = 0,
    NotifyTypeRetired
} FS_FILTER_STREAM_FO_NOTIFICATION_TYPE, *PFS_FILTER_STREAM_FO_NOTIFICATION_TYPE;

typedef union _FS_FILTER_PARAMETERS
{
    struct
    {
        PLARGE_INTEGER EndingOffset;
        PERESOURCE    *ResourceToRelease;
    } AcquireForModifiedPageWriter;

    struct
    {
        PERESOURCE ResourceToRelease;
    } ReleaseForModifiedPageWriter;

    struct
    {
        FS_FILTER_SECTION_SYNC_TYPE    SyncType;
        ULONG                          PageProtection;
        PFS_FILTER_SECTION_SYNC_OUTPUT OutputInformation;
    } AcquireForSectionSynchronization;

    struct
    {
        FS_FILTER_STREAM_FO_NOTIFICATION_TYPE NotificationType;
        BOOLEAN POINTER_ALIGNMENT             SafeToRecurse;
    } NotifyStreamFileObject;

    struct
    {
        PIRP                   Irp;
        void                  *FileInformation;
        PULONG                 Length;
        FILE_INFORMATION_CLASS FileInformationClass;
        NTSTATUS               CompletionStatus;
    } QueryOpen;

    struct
    {
        void *Argument1;
        void *Argument2;
        void *Argument3;
        void *Argument4;
        void *Argument5;
    } Others;

} FS_FILTER_PARAMETERS, *PFS_FILTER_PARAMETERS;

typedef struct _FS_FILTER_CALLBACK_DATA
{
    ULONG                  SizeOfFsFilterCallbackData;
    UCHAR                  Operation;
    UCHAR                  Reserved;
    struct _DEVICE_OBJECT *DeviceObject;
    struct _FILE_OBJECT   *FileObject;
    FS_FILTER_PARAMETERS   Parameters;
} FS_FILTER_CALLBACK_DATA, *PFS_FILTER_CALLBACK_DATA;

typedef NTSTATUS (WINAPI *PFS_FILTER_CALLBACK)(PFS_FILTER_CALLBACK_DATA, void **);
typedef void     (WINAPI *PFS_FILTER_COMPLETION_CALLBACK)(PFS_FILTER_CALLBACK_DATA, NTSTATUS, void *context);

typedef struct _FS_FILTER_CALLBACKS
{
    ULONG SizeOfFsFilterCallbacks;
    ULONG Reserved;
    PFS_FILTER_CALLBACK            PreAcquireForSectionSynchronization;
    PFS_FILTER_COMPLETION_CALLBACK PostAcquireForSectionSynchronization;
    PFS_FILTER_CALLBACK            PreReleaseForSectionSynchronization;
    PFS_FILTER_COMPLETION_CALLBACK PostReleaseForSectionSynchronization;
    PFS_FILTER_CALLBACK            PreAcquireForCcFlush;
    PFS_FILTER_COMPLETION_CALLBACK PostAcquireForCcFlush;
    PFS_FILTER_CALLBACK            PreReleaseForCcFlush;
    PFS_FILTER_COMPLETION_CALLBACK PostReleaseForCcFlush;
    PFS_FILTER_CALLBACK            PreAcquireForModifiedPageWriter;
    PFS_FILTER_COMPLETION_CALLBACK PostAcquireForModifiedPageWriter;
    PFS_FILTER_CALLBACK            PreReleaseForModifiedPageWriter;
    PFS_FILTER_COMPLETION_CALLBACK PostReleaseForModifiedPageWriter;
} FS_FILTER_CALLBACKS, *PFS_FILTER_CALLBACKS;

typedef struct _REPARSE_DATA_BUFFER
{
    ULONG  ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
    union
    {
        struct
        {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            ULONG  Flags;
            WCHAR  PathBuffer[1];
        } SymbolicLinkReparseBuffer;

        struct
        {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            WCHAR  PathBuffer[1];
        } MountPointReparseBuffer;

        struct
        {
            UCHAR DataBuffer[1];
        } GenericReparseBuffer;

    } DUMMYUNIONNAME;
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;

typedef struct _REPARSE_GUID_DATA_BUFFER
{
    DWORD ReparseTag;
    WORD  ReparseDataLength;
    WORD  Reserved;
    GUID  ReparseGuid;
    struct
    {
        BYTE DataBuffer[1];
    } GenericReparseBuffer;
} REPARSE_GUID_DATA_BUFFER, *PREPARSE_GUID_DATA_BUFFER;

#define REPARSE_GUID_DATA_BUFFER_HEADER_SIZE FIELD_OFFSET(REPARSE_GUID_DATA_BUFFER, GenericReparseBuffer)

#define COMPRESSION_FORMAT_NONE         0
#define COMPRESSION_FORMAT_DEFAULT      1
#define COMPRESSION_FORMAT_LZNT1        2
#define COMPRESSION_FORMAT_XPRESS       3
#define COMPRESSION_FORMAT_XPRESS_HUFF  4
#define COMPRESSION_FORMAT_MAX          4

#define COMPRESSION_ENGINE_STANDARD     0x0000
#define COMPRESSION_ENGINE_MAXIMUM      0x0100
#define COMPRESSION_ENGINE_HIBER        0x0200
#define COMPRESSION_ENGINE_MAX          0x0200

#define COMPRESSION_FORMAT_MASK         0x00ff
#define COMPRESSION_ENGINE_MASK         0xff00

BOOLEAN WINAPI FsRtlIsNameInExpression(PUNICODE_STRING, PUNICODE_STRING, BOOLEAN, PWCH);
DEVICE_OBJECT * WINAPI IoGetAttachedDevice(DEVICE_OBJECT*);
PEPROCESS WINAPI IoGetRequestorProcess(IRP*);
void WINAPI KeStackAttachProcess(KPROCESS*,KAPC_STATE*);
void WINAPI KeUnstackDetachProcess(KAPC_STATE*);
NTSTATUS WINAPI ObOpenObjectByPointer(void*,ULONG,PACCESS_STATE,ACCESS_MASK,POBJECT_TYPE,KPROCESSOR_MODE,HANDLE*);
NTSTATUS WINAPI ObQueryNameString(PVOID,POBJECT_NAME_INFORMATION,ULONG,PULONG);
BOOLEAN WINAPI PsIsSystemThread(PETHREAD);
NTSTATUS WINAPI PsLookupProcessByProcessId(HANDLE,PEPROCESS*);
NTSTATUS WINAPI PsLookupThreadByThreadId(HANDLE,PETHREAD*);
void WINAPI PsRevertToSelf(void);
char WINAPI RtlQueryProcessPlaceholderCompatibilityMode(void);

#endif
