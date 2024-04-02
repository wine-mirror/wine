/*
 * Copyright (C) 2018 Alistair Leslie-Hughes
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
#ifndef __FLTKERNEL__
#define __FLTKERNEL__

#include "wine/winheader_enter.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <ddk/ntifs.h>

typedef struct _FLT_FILTER   *PFLT_FILTER;
typedef struct _FLT_INSTANCE *PFLT_INSTANCE;
typedef ULONG  FLT_CALLBACK_DATA_FLAGS;
typedef USHORT FLT_CONTEXT_REGISTRATION_FLAGS;
typedef USHORT FLT_CONTEXT_TYPE;
typedef ULONG  FLT_FILE_NAME_OPTIONS;
typedef ULONG  FLT_FILTER_UNLOAD_FLAGS;
typedef ULONG  FLT_INSTANCE_QUERY_TEARDOWN_FLAGS;
typedef ULONG  FLT_INSTANCE_SETUP_FLAGS;
typedef ULONG  FLT_INSTANCE_TEARDOWN_FLAGS;
typedef ULONG  FLT_NORMALIZE_NAME_FLAGS;
typedef ULONG  FLT_OPERATION_REGISTRATION_FLAGS;
typedef ULONG  FLT_POST_OPERATION_FLAGS;
typedef ULONG  FLT_REGISTRATION_FLAGS;
typedef void*  PFLT_CONTEXT;


#define FLT_VOLUME_CONTEXT       0x0001
#define FLT_INSTANCE_CONTEXT     0x0002
#define FLT_FILE_CONTEXT         0x0004
#define FLT_STREAM_CONTEXT       0x0008
#define FLT_STREAMHANDLE_CONTEXT 0x0010
#define FLT_TRANSACTION_CONTEXT  0x0020

#define FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO     0x00000001
#define FLTFL_OPERATION_REGISTRATION_SKIP_CACHED_IO     0x00000002
#define FLTFL_OPERATION_REGISTRATION_SKIP_NON_DASD_IO   0x00000004

#define FLTFL_INSTANCE_TEARDOWN_MANUAL                  0x00000001
#define FLTFL_INSTANCE_TEARDOWN_FILTER_UNLOAD           0x00000002
#define FLTFL_INSTANCE_TEARDOWN_MANDATORY_FILTER_UNLOAD 0x00000004
#define FLTFL_INSTANCE_TEARDOWN_VOLUME_DISMOUNT         0x00000008
#define FLTFL_INSTANCE_TEARDOWN_INTERNAL_ERROR          0x00000010

/* Belongs in fltuserstructures.h */
typedef enum _FLT_FILESYSTEM_TYPE
{
    FLT_FSTYPE_UNKNOWN,
    FLT_FSTYPE_RAW,
    FLT_FSTYPE_NTFS,
    FLT_FSTYPE_FAT,
    FLT_FSTYPE_CDFS,
    FLT_FSTYPE_UDFS,
    FLT_FSTYPE_LANMAN,
    FLT_FSTYPE_WEBDAV,
    FLT_FSTYPE_RDPDR,
    FLT_FSTYPE_NFS,
    FLT_FSTYPE_MS_NETWARE,
    FLT_FSTYPE_NETWARE,
    FLT_FSTYPE_BSUDF,
    FLT_FSTYPE_MUP,
    FLT_FSTYPE_RSFX,
    FLT_FSTYPE_ROXIO_UDF1,
    FLT_FSTYPE_ROXIO_UDF2,
    FLT_FSTYPE_ROXIO_UDF3,
    FLT_FSTYPE_TACIT,
    FLT_FSTYPE_FS_REC,
    FLT_FSTYPE_INCD,
    FLT_FSTYPE_INCD_FAT,
    FLT_FSTYPE_EXFAT,
    FLT_FSTYPE_PSFS,
    FLT_FSTYPE_GPFS,
    FLT_FSTYPE_NPFS,
    FLT_FSTYPE_MSFS,
    FLT_FSTYPE_CSVFS,
    FLT_FSTYPE_REFS,
    FLT_FSTYPE_OPENAFS
} FLT_FILESYSTEM_TYPE, *PFLT_FILESYSTEM_TYPE;

typedef struct _FLT_NAME_CONTROL
{
    UNICODE_STRING Name;
} FLT_NAME_CONTROL, *PFLT_NAME_CONTROL;

typedef enum _FLT_PREOP_CALLBACK_STATUS
{
    FLT_PREOP_SUCCESS_WITH_CALLBACK,
    FLT_PREOP_SUCCESS_NO_CALLBACK,
    FLT_PREOP_PENDING,
    FLT_PREOP_DISALLOW_FASTIO,
    FLT_PREOP_COMPLETE,
    FLT_PREOP_SYNCHRONIZE,
    FLT_PREOP_DISALLOW_FSFILTER_IO
} FLT_PREOP_CALLBACK_STATUS, *PFLT_PREOP_CALLBACK_STATUS;

typedef enum _FLT_POSTOP_CALLBACK_STATUS
{
    FLT_POSTOP_FINISHED_PROCESSING,
    FLT_POSTOP_MORE_PROCESSING_REQUIRED,
    FLT_POSTOP_DISALLOW_FSFILTER_IO
} FLT_POSTOP_CALLBACK_STATUS, *PFLT_POSTOP_CALLBACK_STATUS;

typedef struct _FLT_RELATED_CONTEXTS
{
    PFLT_CONTEXT VolumeContext;
    PFLT_CONTEXT InstanceContext;
    PFLT_CONTEXT FileContext;
    PFLT_CONTEXT StreamContext;
    PFLT_CONTEXT StreamHandleContext;
    PFLT_CONTEXT TransactionContext;
} FLT_RELATED_CONTEXTS, *PFLT_RELATED_CONTEXTS;

typedef const struct _FLT_RELATED_OBJECTS *PCFLT_RELATED_OBJECTS;

typedef union _FLT_PARAMETERS
{
    struct
    {
        PIO_SECURITY_CONTEXT     SecurityContext;
        ULONG                    Options;
        USHORT POINTER_ALIGNMENT FileAttributes;
        USHORT                   ShareAccess;
        ULONG POINTER_ALIGNMENT  EaLength;
        void                    *EaBuffer;
        LARGE_INTEGER            AllocationSize;
    } Create;

    struct
    {
        PIO_SECURITY_CONTEXT     SecurityContext;
        ULONG                    Options;
        USHORT POINTER_ALIGNMENT Reserved;
        USHORT                   ShareAccess;
        void                    *Parameters;
    } CreatePipe;

#undef CreateMailslot
    struct
    {
        PIO_SECURITY_CONTEXT     SecurityContext;
        ULONG                    Options;
        USHORT POINTER_ALIGNMENT Reserved;
        USHORT                   ShareAccess;
        void                    *Parameters;
    } CreateMailslot;

    struct
    {
        ULONG                   Length;
        ULONG POINTER_ALIGNMENT Key;
        LARGE_INTEGER           ByteOffset;
        void                   *ReadBuffer;
        PMDL                    MdlAddress;
    } Read;

    struct
    {
        ULONG                   Length;
        ULONG POINTER_ALIGNMENT Key;
        LARGE_INTEGER           ByteOffset;
        void                   *WriteBuffer;
        PMDL                    MdlAddress;
    } Write;

    struct
    {
        ULONG                   Length;
        FILE_INFORMATION_CLASS POINTER_ALIGNMENT FileInformationClass;
        void                   *InfoBuffer;
    } QueryFileInformation;

    struct
    {
        ULONG                   Length;
        FILE_INFORMATION_CLASS POINTER_ALIGNMENT FileInformationClass;
        PFILE_OBJECT            ParentOfTarget;
        union
        {
            struct
            {
                BOOLEAN ReplaceIfExists;
                BOOLEAN AdvanceOnly;
            } DUMMYSTRUCTNAME;
            ULONG ClusterCount;
            HANDLE DeleteHandle;
        } DUMMYUNIONNAME;
        void                    *InfoBuffer;
    } SetFileInformation;

    struct
    {
        ULONG                   Length;
        void                   *EaList;
        ULONG                   EaListLength;
        ULONG                   POINTER_ALIGNMENT EaIndex;
        void                   *EaBuffer;
        PMDL                    MdlAddress;
    } QueryEa;

    struct
    {
        ULONG                   Length;
        void                   *EaBuffer;
        PMDL                    MdlAddress;
    } SetEa;

    struct
    {
        ULONG                   Length;
        FS_INFORMATION_CLASS POINTER_ALIGNMENT FsInformationClass;
        void                   *VolumeBuffer;
    } QueryVolumeInformation;

    struct
    {
        ULONG                  Length;
        FS_INFORMATION_CLASS POINTER_ALIGNMENT FsInformationClass;
        void                  *VolumeBuffer;
    } SetVolumeInformation;

    union
    {
        struct
        {
            ULONG                   Length;
            PUNICODE_STRING         FileName;
            FILE_INFORMATION_CLASS  FileInformationClass;
            ULONG POINTER_ALIGNMENT FileIndex;
            void                   *DirectoryBuffer;
            PMDL                    MdlAddress;
        } QueryDirectory;

        struct
        {
            ULONG                   Length;
            ULONG POINTER_ALIGNMENT CompletionFilter;
            ULONG POINTER_ALIGNMENT Spare1;
            ULONG POINTER_ALIGNMENT Spare2;
            void                   *DirectoryBuffer;
            PMDL                    MdlAddress;
        } NotifyDirectory;

        struct
        {
            ULONG                    Length;
            ULONG POINTER_ALIGNMENT  CompletionFilter;
            DIRECTORY_NOTIFY_INFORMATION_CLASS POINTER_ALIGNMENT DirectoryNotifyInformationClass;
            ULONG POINTER_ALIGNMENT  Spare2;
            void                    *DirectoryBuffer;
            PMDL                     MdlAddress;
        } NotifyDirectoryEx;
    } DirectoryControl;

    union
    {
        struct
        {
            PVPB           Vpb;
            PDEVICE_OBJECT DeviceObject;
        } VerifyVolume;

        struct
        {
            ULONG                   OutputBufferLength;
            ULONG POINTER_ALIGNMENT InputBufferLength;
            ULONG POINTER_ALIGNMENT FsControlCode;
        } Common;

        struct
        {
            ULONG                   OutputBufferLength;
            ULONG POINTER_ALIGNMENT InputBufferLength;
            ULONG POINTER_ALIGNMENT FsControlCode;
            void                   *InputBuffer;
            void                   *OutputBuffer;
            PMDL                    OutputMdlAddress;
        } Neither;

        struct
        {
            ULONG                   OutputBufferLength;
            ULONG POINTER_ALIGNMENT InputBufferLength;
            ULONG POINTER_ALIGNMENT FsControlCode;
            void                   *SystemBuffer;
        } Buffered;

        struct
        {
            ULONG                   OutputBufferLength;
            ULONG POINTER_ALIGNMENT InputBufferLength;
            ULONG POINTER_ALIGNMENT FsControlCode;
            void                   *InputSystemBuffer;
            void                   *OutputBuffer;
            PMDL                    OutputMdlAddress;
        } Direct;
    } FileSystemControl;

    union
    {
        struct
        {
            ULONG                   OutputBufferLength;
            ULONG POINTER_ALIGNMENT InputBufferLength;
            ULONG POINTER_ALIGNMENT IoControlCode;
        } Common;

        struct
        {
            ULONG                   OutputBufferLength;
            ULONG POINTER_ALIGNMENT InputBufferLength;
            ULONG POINTER_ALIGNMENT IoControlCode;
            void                   *InputBuffer;
            void                   *OutputBuffer;
            PMDL                    OutputMdlAddress;
        } Neither;

        struct
        {
            ULONG                   OutputBufferLength;
            ULONG POINTER_ALIGNMENT InputBufferLength;
            ULONG POINTER_ALIGNMENT IoControlCode;
            void                   *SystemBuffer;
        } Buffered;

        struct
        {
            ULONG                   OutputBufferLength;
            ULONG POINTER_ALIGNMENT InputBufferLength;
            ULONG POINTER_ALIGNMENT IoControlCode;
            void                   *InputSystemBuffer;
            void                   *OutputBuffer;
            PMDL                    OutputMdlAddress;
        } Direct;

        struct
        {
            ULONG                   OutputBufferLength;
            ULONG POINTER_ALIGNMENT InputBufferLength;
            ULONG POINTER_ALIGNMENT IoControlCode;
            void                   *InputBuffer;
            void                   *OutputBuffer;
        } FastIo;
    } DeviceIoControl;

    struct
    {
        PLARGE_INTEGER              Length;
        ULONG POINTER_ALIGNMENT     Key;
        LARGE_INTEGER               ByteOffset;
        PEPROCESS                   ProcessId;
        BOOLEAN                     FailImmediately;
        BOOLEAN                     ExclusiveLock;
    } LockControl;

    struct
    {
        SECURITY_INFORMATION        SecurityInformation;
        ULONG POINTER_ALIGNMENT     Length;
        void                       *SecurityBuffer;
        PMDL                        MdlAddress;
    } QuerySecurity;

    struct
    {
        SECURITY_INFORMATION        SecurityInformation;
        PSECURITY_DESCRIPTOR        SecurityDescriptor;
    } SetSecurity;

    struct
    {
        ULONG_PTR                   ProviderId;
        void                       *DataPath;
        ULONG                       BufferSize;
        void                       *Buffer;
    } WMI;

    struct
    {
        ULONG                       Length;
        PSID                        StartSid;
        PFILE_GET_QUOTA_INFORMATION SidList;
        ULONG                       SidListLength;
        void                       *QuotaBuffer;
        PMDL                        MdlAddress;
    } QueryQuota;

    struct
    {
        ULONG                       Length;
        void                       *QuotaBuffer;
        PMDL                        MdlAddress;
    } SetQuota;

    union
    {
        struct
        {
            PCM_RESOURCE_LIST       AllocatedResources;
            PCM_RESOURCE_LIST       AllocatedResourcesTranslated;
        } StartDevice;

        struct
        {
            DEVICE_RELATION_TYPE    Type;
        } QueryDeviceRelations;

        struct
        {
            const GUID             *InterfaceType;
            USHORT                  Size;
            USHORT                  Version;
            PINTERFACE              Interface;
            void                   *InterfaceSpecificData;
        } QueryInterface;

        struct
        {
            PDEVICE_CAPABILITIES    Capabilities;
        } DeviceCapabilities;

        struct
        {
            PIO_RESOURCE_REQUIREMENTS_LIST IoResourceRequirementList;
        } FilterResourceRequirements;

        struct
        {
            ULONG                   WhichSpace;
            void                   *Buffer;
            ULONG                   Offset;
            ULONG POINTER_ALIGNMENT Length;
        } ReadWriteConfig;

        struct
        {
            BOOLEAN                 Lock;
        } SetLock;

        struct {
            BUS_QUERY_ID_TYPE       IdType;
        } QueryId;

        struct
        {
            DEVICE_TEXT_TYPE       DeviceTextType;
            LCID POINTER_ALIGNMENT LocaleId;
        } QueryDeviceText;

        struct
        {
            BOOLEAN                InPath;
            BOOLEAN                Reserved[3];
            DEVICE_USAGE_NOTIFICATION_TYPE POINTER_ALIGNMENT Type;
        } UsageNotification;
    } Pnp;

    struct
    {
        FS_FILTER_SECTION_SYNC_TYPE    SyncType;
        ULONG                          PageProtection;
        PFS_FILTER_SECTION_SYNC_OUTPUT OutputInformation;
    } AcquireForSectionSynchronization;

    struct
    {
        PLARGE_INTEGER                 EndingOffset;
        PERESOURCE                    *ResourceToRelease;
    } AcquireForModifiedPageWriter;

    struct
    {
        PERESOURCE                     ResourceToRelease;
    } ReleaseForModifiedPageWriter;

    struct
    {
        PIRP                           Irp;
        void                          *FileInformation;
        PULONG                         Length;
        FILE_INFORMATION_CLASS         FileInformationClass;
    } QueryOpen;

    struct
    {
        LARGE_INTEGER                  FileOffset;
        ULONG                          Length;
        ULONG POINTER_ALIGNMENT        LockKey;
        BOOLEAN POINTER_ALIGNMENT      CheckForReadOperation;
    } FastIoCheckIfPossible;

    struct
    {
        PIRP                           Irp;
        PFILE_NETWORK_OPEN_INFORMATION NetworkInformation;
    } NetworkQueryOpen;

    struct
    {
        LARGE_INTEGER                  FileOffset;
        ULONG POINTER_ALIGNMENT        Length;
        ULONG POINTER_ALIGNMENT        Key;
        PMDL                          *MdlChain;
    } MdlRead;

    struct
    {
        PMDL                           MdlChain;
    } MdlReadComplete;

    struct
    {
        LARGE_INTEGER                  FileOffset;
        ULONG POINTER_ALIGNMENT        Length;
        ULONG POINTER_ALIGNMENT        Key;
        PMDL                          *MdlChain;
    } PrepareMdlWrite;

    struct
    {
        LARGE_INTEGER                  FileOffset;
        PMDL                           MdlChain;
    } MdlWriteComplete;

    struct
    {
        ULONG                          DeviceType;
    } MountVolume;

    struct
    {
        void                          *Argument1;
        void                          *Argument2;
        void                          *Argument3;
        void                          *Argument4;
        void                          *Argument5;
        LARGE_INTEGER                  Argument6;
    } Others;
} FLT_PARAMETERS, *PFLT_PARAMETERS;

typedef struct _FLT_IO_PARAMETER_BLOCK
{
    ULONG          IrpFlags;
    UCHAR          MajorFunction;
    UCHAR          MinorFunction;
    UCHAR          OperationFlags;
    UCHAR          Reserved;
    PFILE_OBJECT   TargetFileObject;
    PFLT_INSTANCE  TargetInstance;
    FLT_PARAMETERS Parameters;
} FLT_IO_PARAMETER_BLOCK, *PFLT_IO_PARAMETER_BLOCK;

typedef struct _FLT_CALLBACK_DATA
{
    FLT_CALLBACK_DATA_FLAGS       Flags;
    PETHREAD const                Thread;
    PFLT_IO_PARAMETER_BLOCK const Iopb;
    IO_STATUS_BLOCK               IoStatus;
    struct _FLT_TAG_DATA_BUFFER  *TagData;

    union
    {
        struct
        {
            LIST_ENTRY QueueLinks;
            void      *QueueContext[2];
        } DUMMYSTRUCTNAME;
        void *FilterContext[4];
    } DUMMYUNIONNAME;
    KPROCESSOR_MODE RequestorMode;
} FLT_CALLBACK_DATA, *PFLT_CALLBACK_DATA;

typedef void*    (WINAPI *PFLT_CONTEXT_ALLOCATE_CALLBACK)(POOL_TYPE,SIZE_T,FLT_CONTEXT_TYPE);
typedef void     (WINAPI *PFLT_CONTEXT_CLEANUP_CALLBACK)(PFLT_CONTEXT, FLT_CONTEXT_TYPE);
typedef void     (WINAPI *PFLT_CONTEXT_FREE_CALLBACK)(void *, FLT_CONTEXT_TYPE);
typedef NTSTATUS (WINAPI *PFLT_FILTER_UNLOAD_CALLBACK)(FLT_FILTER_UNLOAD_FLAGS);
typedef NTSTATUS (WINAPI *PFLT_INSTANCE_QUERY_TEARDOWN_CALLBACK)(PCFLT_RELATED_OBJECTS,
                FLT_INSTANCE_QUERY_TEARDOWN_FLAGS);
typedef NTSTATUS (WINAPI *PFLT_GENERATE_FILE_NAME)(PFLT_INSTANCE, PFILE_OBJECT,PFLT_CALLBACK_DATA,
                FLT_FILE_NAME_OPTIONS,PBOOLEAN, PFLT_NAME_CONTROL);
typedef NTSTATUS (WINAPI *PFLT_INSTANCE_SETUP_CALLBACK)(PCFLT_RELATED_OBJECTS,FLT_INSTANCE_SETUP_FLAGS,
                DEVICE_TYPE,FLT_FILESYSTEM_TYPE);
typedef void     (WINAPI *PFLT_INSTANCE_TEARDOWN_CALLBACK)(PCFLT_RELATED_OBJECTS, FLT_INSTANCE_TEARDOWN_FLAGS);
typedef void     (WINAPI *PFLT_NORMALIZE_CONTEXT_CLEANUP)(void**);
typedef NTSTATUS (WINAPI *PFLT_NORMALIZE_NAME_COMPONENT)(PFLT_INSTANCE, PCUNICODE_STRING, USHORT,
                PCUNICODE_STRING,PFILE_NAMES_INFORMATION,ULONG,FLT_NORMALIZE_NAME_FLAGS, void **);
typedef NTSTATUS (WINAPI *PFLT_NORMALIZE_NAME_COMPONENT_EX)(PFLT_INSTANCE, PFILE_OBJECT, PCUNICODE_STRING,
                USHORT, PCUNICODE_STRING,PFILE_NAMES_INFORMATION, ULONG, FLT_NORMALIZE_NAME_FLAGS Flags,
                void **NormalizationContext);
typedef FLT_PREOP_CALLBACK_STATUS (WINAPI *PFLT_PRE_OPERATION_CALLBACK)(PFLT_CALLBACK_DATA,
                PCFLT_RELATED_OBJECTS, void**);
typedef FLT_POSTOP_CALLBACK_STATUS (WINAPI *PFLT_POST_OPERATION_CALLBACK)(PFLT_CALLBACK_DATA,
                PCFLT_RELATED_OBJECTS, void*, FLT_POST_OPERATION_FLAGS);
typedef NTSTATUS (WINAPI *PFLT_SECTION_CONFLICT_NOTIFICATION_CALLBACK)(PFLT_INSTANCE, PFLT_CONTEXT,
                PFLT_CALLBACK_DATA);
typedef NTSTATUS (WINAPI *PFLT_TRANSACTION_NOTIFICATION_CALLBACK)(PCFLT_RELATED_OBJECTS, PFLT_CONTEXT, ULONG);

typedef struct _FLT_CONTEXT_REGISTRATION
{
    FLT_CONTEXT_TYPE               ContextType;
    FLT_CONTEXT_REGISTRATION_FLAGS Flags;
    PFLT_CONTEXT_CLEANUP_CALLBACK  ContextCleanupCallback;
    SIZE_T                         Size;
    ULONG                          PoolTag;
    PFLT_CONTEXT_ALLOCATE_CALLBACK ContextAllocateCallback;
    PFLT_CONTEXT_FREE_CALLBACK     ContextFreeCallback;
    void                          *Reserved1;
} FLT_CONTEXT_REGISTRATION, *PFLT_CONTEXT_REGISTRATION;

typedef const FLT_CONTEXT_REGISTRATION *PCFLT_CONTEXT_REGISTRATION;

typedef struct _FLT_OPERATION_REGISTRATION
{
    UCHAR                            MajorFunction;
    FLT_OPERATION_REGISTRATION_FLAGS Flags;
    PFLT_PRE_OPERATION_CALLBACK      PreOperation;
    PFLT_POST_OPERATION_CALLBACK     PostOperation;
    void                            *Reserved1;
} FLT_OPERATION_REGISTRATION, *PFLT_OPERATION_REGISTRATION;

typedef struct _FLT_REGISTRATION
{
    USHORT                                      Size;
    USHORT                                      Version;
    FLT_REGISTRATION_FLAGS                      Flags;
    const FLT_CONTEXT_REGISTRATION             *ContextRegistration;
    const FLT_OPERATION_REGISTRATION           *OperationRegistration;
    PFLT_FILTER_UNLOAD_CALLBACK                 FilterUnloadCallback;
    PFLT_INSTANCE_SETUP_CALLBACK                InstanceSetupCallback;
    PFLT_INSTANCE_QUERY_TEARDOWN_CALLBACK       InstanceQueryTeardownCallback;
    PFLT_INSTANCE_TEARDOWN_CALLBACK             InstanceTeardownStartCallback;
    PFLT_INSTANCE_TEARDOWN_CALLBACK             InstanceTeardownCompleteCallback;
    PFLT_GENERATE_FILE_NAME                     GenerateFileNameCallback;
    PFLT_NORMALIZE_NAME_COMPONENT               NormalizeNameComponentCallback;
    PFLT_NORMALIZE_CONTEXT_CLEANUP              NormalizeContextCleanupCallback;
    PFLT_TRANSACTION_NOTIFICATION_CALLBACK      TransactionNotificationCallback;
    PFLT_NORMALIZE_NAME_COMPONENT_EX            NormalizeNameComponentExCallback;
    PFLT_SECTION_CONFLICT_NOTIFICATION_CALLBACK SectionNotificationCallback;
} FLT_REGISTRATION, *PFLT_REGISTRATION;


void*    WINAPI FltGetRoutineAddress(LPCSTR name);
NTSTATUS WINAPI FltRegisterFilter(PDRIVER_OBJECT, const FLT_REGISTRATION *, PFLT_FILTER *);
NTSTATUS WINAPI FltStartFiltering(PFLT_FILTER);
void     WINAPI FltUnregisterFilter(PFLT_FILTER);

#ifdef __cplusplus
}
#endif

#include "wine/winheader_exit.h"

#endif
