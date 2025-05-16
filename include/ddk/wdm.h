/*
 * Copyright 2004-2005 Ivan Leo Puoti
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

#ifndef _WDMDDK_
#define _WDMDDK_
#define _NTDDK_

#include <ntstatus.h>
#include <devpropdef.h>

#ifdef _WIN64
#define POINTER_ALIGNMENT DECLSPEC_ALIGN(8)
#else
#define POINTER_ALIGNMENT
#endif

/* FIXME: We suppose that page size is 4096 */
#undef PAGE_SIZE
#undef PAGE_SHIFT
#define PAGE_SIZE   0x1000
#define PAGE_SHIFT  12

#define BYTE_OFFSET(va) ((ULONG)((ULONG_PTR)(va) & (PAGE_SIZE - 1)))
#define PAGE_ALIGN(va)  ((PVOID)((ULONG_PTR)(va) & ~(PAGE_SIZE - 1)))
#define ADDRESS_AND_SIZE_TO_SPAN_PAGES(va, length) \
    ((BYTE_OFFSET(va) + ((SIZE_T)(length)) + (PAGE_SIZE - 1)) >> PAGE_SHIFT)

#define LOW_PRIORITY             0
#define LOW_REALTIME_PRIORITY   16
#define HIGH_PRIORITY           31
#define MAXIMUM_PRIORITY        32

typedef LONG KPRIORITY;

typedef ULONG_PTR KSPIN_LOCK, *PKSPIN_LOCK;

typedef ULONG_PTR ERESOURCE_THREAD;
typedef ERESOURCE_THREAD *PERESOURCE_THREAD;
typedef struct _FILE_GET_QUOTA_INFORMATION *PFILE_GET_QUOTA_INFORMATION;

struct _KDPC;
struct _KAPC;
struct _IRP;
struct _DEVICE_OBJECT;
struct _DRIVER_OBJECT;
struct _KPROCESS;

typedef VOID (WINAPI *PKDEFERRED_ROUTINE)(struct _KDPC *, PVOID, PVOID, PVOID);
typedef VOID (WINAPI *PKSTART_ROUTINE)(PVOID);

typedef NTSTATUS (WINAPI *PDRIVER_INITIALIZE)(struct _DRIVER_OBJECT *, PUNICODE_STRING);
typedef NTSTATUS (WINAPI *PDRIVER_DISPATCH)(struct _DEVICE_OBJECT *, struct _IRP *);
typedef void (WINAPI *PDRIVER_STARTIO)(struct _DEVICE_OBJECT *, struct _IRP *);
typedef void (WINAPI *PDRIVER_UNLOAD)(struct _DRIVER_OBJECT *);
typedef NTSTATUS (WINAPI *PDRIVER_ADD_DEVICE)(struct _DRIVER_OBJECT *, struct _DEVICE_OBJECT *);
typedef void (WINAPI *PDEVICE_CHANGE_COMPLETE_CALLBACK)(void*);

typedef struct _DISPATCHER_HEADER {
  UCHAR  Type;
  UCHAR  Absolute;
  UCHAR  Size;
  UCHAR  Inserted;
  LONG  SignalState;
  LIST_ENTRY  WaitListHead;
} DISPATCHER_HEADER, *PDISPATCHER_HEADER;

typedef struct _KEVENT {
  DISPATCHER_HEADER  Header;
} KEVENT, *PKEVENT, *RESTRICTED_POINTER PRKEVENT;

typedef struct _KSEMAPHORE {
  DISPATCHER_HEADER  Header;
  LONG Limit;
} KSEMAPHORE, *PKSEMAPHORE, *PRKSEMAPHORE;

typedef struct _KDPC {
  union {
    ULONG TargetInfoAsUlong;
    struct {
      UCHAR  Type;
      UCHAR  Importance;
      volatile USHORT  Number;
    } DUMMYSTRUCTNAME;
  } DUMMYUNIONNAME;
  SINGLE_LIST_ENTRY  DpcListEntry;
  KAFFINITY  ProcessorHistory;
  PKDEFERRED_ROUTINE  DeferredRoutine;
  PVOID  DeferredContext;
  PVOID  SystemArgument1;
  PVOID  SystemArgument2;
  PVOID  DpcData;
} KDPC, *PKDPC, *RESTRICTED_POINTER PRKDPC;

typedef enum _KDPC_IMPORTANCE {
  LowImportance,
  MediumImportance,
  HighImportance,
  MediumHighImportance
} KDPC_IMPORTANCE;

typedef struct _KDEVICE_QUEUE_ENTRY {
  LIST_ENTRY  DeviceListEntry;
  ULONG  SortKey;
  BOOLEAN  Inserted;
} KDEVICE_QUEUE_ENTRY, *PKDEVICE_QUEUE_ENTRY,
*RESTRICTED_POINTER PRKDEVICE_QUEUE_ENTRY;

typedef struct _KDEVICE_QUEUE {
  CSHORT  Type;
  CSHORT  Size;
  LIST_ENTRY  DeviceListHead;
  KSPIN_LOCK  Lock;
  BOOLEAN  Busy;
} KDEVICE_QUEUE, *PKDEVICE_QUEUE, *RESTRICTED_POINTER PRKDEVICE_QUEUE;

typedef struct _KMUTANT {
    DISPATCHER_HEADER Header;
    LIST_ENTRY MutantListEntry;
    struct _KTHREAD *RESTRICTED_POINTER OwnerThread;
    BOOLEAN Abandoned;
    UCHAR ApcDisable;
} KMUTANT, *PKMUTANT, *RESTRICTED_POINTER PRKMUTANT, KMUTEX, *PKMUTEX, *RESTRICTED_POINTER PRKMUTEX;

typedef struct _DEFERRED_REVERSE_BARRIER
{
    ULONG Barrier;
    ULONG TotalProcessors;
} DEFERRED_REVERSE_BARRIER;

typedef enum _KWAIT_REASON
{
    Executive,
    FreePage,
    PageIn,
    PoolAllocation,
    DelayExecution,
    Suspended,
    UserRequest,
    WrExecutive,
    WrFreePage,
    WrPageIn,
    WrDelayExecution,
    WrSuspended,
    WrUserRequest,
    WrQueue,
    WrLpcReceive,
    WrLpcReply,
    WrVirtualMemory,
    WrPageOut,
    WrRendezvous,
    Spare2,
    Spare3,
    Spare4,
    Spare5,
    Spare6,
    WrKernel,
    MaximumWaitReason,
} KWAIT_REASON;

typedef struct _KWAIT_BLOCK {
    LIST_ENTRY WaitListEntry;
    struct _KTHREAD *RESTRICTED_POINTER Thread;
    PVOID Object;
    struct _KWAIT_BLOCK *RESTRICTED_POINTER NextWaitBlock;
    USHORT WaitKey;
    USHORT WaitType;
} KWAIT_BLOCK, *PKWAIT_BLOCK, *RESTRICTED_POINTER PRKWAIT_BLOCK;

typedef struct _OWNER_ENTRY
{
    ERESOURCE_THREAD OwnerThread;
    union
    {
        struct
        {
            ULONG IoPriorityBoosted : 1;
            ULONG OwnerReferenced : 1;
            ULONG IoQoSPriorityBoosted : 1;
            ULONG OwnerCount : 29;
        };
        ULONG TableSize;
    };
} OWNER_ENTRY, *POWNER_ENTRY;

#define ResourceNeverExclusive       0x0010
#define ResourceReleaseByOtherThread 0x0020
#define ResourceOwnedExclusive       0x0080

typedef struct _ERESOURCE
{
    LIST_ENTRY SystemResourcesList;
    OWNER_ENTRY *OwnerTable;
    SHORT ActiveCount;
    union
    {
        USHORT Flag;
        struct
        {
            UCHAR ReservedLowFlags;
            UCHAR WaiterPriority;
        };
    };
    KSEMAPHORE *SharedWaiters;
    KEVENT *ExclusiveWaiters;
    OWNER_ENTRY OwnerEntry;
    ULONG ActiveEntries;
    ULONG ContentionCount;
    ULONG NumberOfSharedWaiters;
    ULONG NumberOfExclusiveWaiters;
#ifdef _WIN64
    void *Reserved2;
#endif
    union
    {
        void *Address;
        ULONG_PTR CreatorBackTraceIndex;
    };
    KSPIN_LOCK SpinLock;
} ERESOURCE, *PERESOURCE;

typedef struct _IO_TIMER *PIO_TIMER;
typedef struct _IO_TIMER_ROUTINE *PIO_TIMER_ROUTINE;
typedef struct _ETHREAD *PETHREAD;
typedef struct _KTHREAD *PKTHREAD, *PRKTHREAD;
typedef struct _EPROCESS *PEPROCESS;
typedef struct _KPROCESS KPROCESS, *PKPROCESS, *PRKPROCESS;
typedef struct _IO_WORKITEM *PIO_WORKITEM;
typedef struct _OBJECT_TYPE *POBJECT_TYPE;
typedef struct _OBJECT_HANDLE_INFORMATION *POBJECT_HANDLE_INFORMATION;
typedef struct _ZONE_HEADER *PZONE_HEADER;
typedef struct _LOOKASIDE_LIST_EX *PLOOKASIDE_LIST_EX;

typedef struct _KAPC_STATE
{
     LIST_ENTRY ApcListHead[2];
     PKPROCESS Process;
     UCHAR KernelApcInProgress;
     UCHAR KernelApcPending;
     UCHAR UserApcPending;
} KAPC_STATE, *PKAPC_STATE;

#define FM_LOCK_BIT 0x1

typedef struct _FAST_MUTEX
{
    LONG Count;
    PKTHREAD Owner;
    ULONG Contention;
    KEVENT Event;
    ULONG OldIrql;
} FAST_MUTEX, *PFAST_MUTEX;

typedef struct _KGUARDED_MUTEX
{
     LONG Count;
     PKTHREAD Owner;
     ULONG Contention;
     KEVENT Event;
     union
     {
          struct
          {
               SHORT KernelApcDisable;
               SHORT SpecialApcDisable;
          };
          ULONG CombinedApcDisable;
     };
} KGUARDED_MUTEX, *PKGUARDED_MUTEX;

#define MAXIMUM_VOLUME_LABEL_LENGTH       (32 * sizeof(WCHAR))

typedef struct _VPB {
  CSHORT  Type;
  CSHORT  Size;
  USHORT  Flags;
  USHORT  VolumeLabelLength;
  struct _DEVICE_OBJECT  *DeviceObject;
  struct _DEVICE_OBJECT  *RealDevice;
  ULONG  SerialNumber;
  ULONG  ReferenceCount;
  WCHAR  VolumeLabel[MAXIMUM_VOLUME_LABEL_LENGTH / sizeof(WCHAR)];
} VPB, *PVPB;

#define POOL_QUOTA_FAIL_INSTEAD_OF_RAISE 0x0008
#define POOL_RAISE_IF_ALLOCATION_FAILURE 0x0010
#define POOL_COLD_ALLOCATION             0x0100
#define POOL_NX_ALLOCATION               0x0200

typedef enum _POOL_TYPE {
  NonPagedPool,
  PagedPool,
  NonPagedPoolMustSucceed,
  DontUseThisType,
  NonPagedPoolCacheAligned,
  PagedPoolCacheAligned,
  NonPagedPoolCacheAlignedMustS,
  MaxPoolType
} POOL_TYPE;

typedef ULONG64 POOL_FLAGS;

#define POOL_FLAG_REQUIRED_START        0x00000001
#define POOL_FLAG_USE_QUOTA             0x00000001
#define POOL_FLAG_UNINITIALIZED         0x00000002
#define POOL_FLAG_SESSION               0x00000004
#define POOL_FLAG_CACHE_ALIGNED         0x00000008
#define POOL_FLAG_RESERVED1             0x00000010
#define POOL_FLAG_RAISE_ON_FAILURE      0x00000020
#define POOL_FLAG_NON_PAGED             0x00000040
#define POOL_FLAG_NON_PAGED_EXECUTE     0x00000080
#define POOL_FLAG_PAGED                 0x00000100
#define POOL_FLAG_RESERVED2             0x00000200
#define POOL_FLAG_RESERVED3             0x00000400
#define POOL_FLAG_LAST_KNOWN_REQUIRED   POOL_FLAG_RESERVED3
#define POOL_FLAG_REQUIRED_END          0x80000000

typedef struct _WAIT_CONTEXT_BLOCK {
  KDEVICE_QUEUE_ENTRY  WaitQueueEntry;
  struct _DRIVER_CONTROL  *DeviceRoutine;
  PVOID  DeviceContext;
  ULONG  NumberOfMapRegisters;
  PVOID  DeviceObject;
  PVOID  CurrentIrp;
  PKDPC  BufferChainingDpc;
} WAIT_CONTEXT_BLOCK, *PWAIT_CONTEXT_BLOCK;

#define DO_BUFFERED_IO                  0x00000004
#define DO_EXCLUSIVE                    0x00000008
#define DO_DIRECT_IO                    0x00000010
#define DO_MAP_IO_BUFFER                0x00000020
#define DO_DEVICE_INITIALIZING          0x00000080
#define DO_SHUTDOWN_REGISTERED          0x00000800
#define DO_BUS_ENUMERATED_DEVICE        0x00001000
#define DO_POWER_PAGABLE                0x00002000
#define DO_POWER_INRUSH                 0x00004000

#define IO_NO_INCREMENT                     0
#define IO_CD_ROM_INCREMENT                 1
#define IO_DISK_INCREMENT                   1
#define IO_KEYBOARD_INCREMENT               6
#define IO_MAILSLOT_INCREMENT               2
#define IO_MOUSE_INCREMENT                  6
#define IO_NAMED_PIPE_INCREMENT             2
#define IO_NETWORK_INCREMENT                2
#define IO_PARALLEL_INCREMENT               1
#define IO_SERIAL_INCREMENT                 2
#define IO_SOUND_INCREMENT                  8
#define IO_VIDEO_INCREMENT                  1

#ifndef DEVICE_TYPE
#define DEVICE_TYPE ULONG
#endif
#define IRP_MJ_MAXIMUM_FUNCTION           0x1b
#define IRP_MJ_CREATE                     0x00
#define IRP_MJ_CREATE_NAMED_PIPE          0x01
#define IRP_MJ_CLOSE                      0x02
#define IRP_MJ_READ                       0x03
#define IRP_MJ_WRITE                      0x04
#define IRP_MJ_QUERY_INFORMATION          0x05
#define IRP_MJ_SET_INFORMATION            0x06
#define IRP_MJ_QUERY_EA                   0x07
#define IRP_MJ_SET_EA                     0x08
#define IRP_MJ_FLUSH_BUFFERS              0x09
#define IRP_MJ_QUERY_VOLUME_INFORMATION   0x0a
#define IRP_MJ_SET_VOLUME_INFORMATION     0x0b
#define IRP_MJ_DIRECTORY_CONTROL          0x0c
#define IRP_MJ_FILE_SYSTEM_CONTROL        0x0d
#define IRP_MJ_DEVICE_CONTROL             0x0e
#define IRP_MJ_INTERNAL_DEVICE_CONTROL    0x0f
#define IRP_MJ_SHUTDOWN                   0x10
#define IRP_MJ_LOCK_CONTROL               0x11
#define IRP_MJ_CLEANUP                    0x12
#define IRP_MJ_CREATE_MAILSLOT            0x13
#define IRP_MJ_QUERY_SECURITY             0x14
#define IRP_MJ_SET_SECURITY               0x15
#define IRP_MJ_POWER                      0x16
#define IRP_MJ_SYSTEM_CONTROL             0x17
#define IRP_MJ_DEVICE_CHANGE              0x18
#define IRP_MJ_QUERY_QUOTA                0x19
#define IRP_MJ_SET_QUOTA                  0x1a
#define IRP_MJ_PNP                        0x1b

#define IRP_MN_START_DEVICE                 0x00
#define IRP_MN_QUERY_REMOVE_DEVICE          0x01
#define IRP_MN_REMOVE_DEVICE                0x02
#define IRP_MN_CANCEL_REMOVE_DEVICE         0x03
#define IRP_MN_STOP_DEVICE                  0x04
#define IRP_MN_QUERY_STOP_DEVICE            0x05
#define IRP_MN_CANCEL_STOP_DEVICE           0x06
#define IRP_MN_QUERY_DEVICE_RELATIONS       0x07
#define IRP_MN_QUERY_INTERFACE              0x08
#define IRP_MN_QUERY_CAPABILITIES           0x09
#define IRP_MN_QUERY_RESOURCES              0x0A
#define IRP_MN_QUERY_RESOURCE_REQUIREMENTS  0x0B
#define IRP_MN_QUERY_DEVICE_TEXT            0x0C
#define IRP_MN_FILTER_RESOURCE_REQUIREMENTS 0x0D
#define IRP_MN_READ_CONFIG                  0x0F
#define IRP_MN_WRITE_CONFIG                 0x10
#define IRP_MN_EJECT                        0x11
#define IRP_MN_SET_LOCK                     0x12
#define IRP_MN_QUERY_ID                     0x13
#define IRP_MN_QUERY_PNP_DEVICE_STATE       0x14
#define IRP_MN_QUERY_BUS_INFORMATION        0x15
#define IRP_MN_DEVICE_USAGE_NOTIFICATION    0x16
#define IRP_MN_SURPRISE_REMOVAL             0x17
#define IRP_MN_QUERY_LEGACY_BUS_INFORMATION 0x18

#define IRP_MN_WAIT_WAKE                    0x00
#define IRP_MN_POWER_SEQUENCE               0x01
#define IRP_MN_SET_POWER                    0x02
#define IRP_MN_QUERY_POWER                  0x03

#define IRP_QUOTA_CHARGED               0x01
#define IRP_ALLOCATED_MUST_SUCCEED      0x02
#define IRP_ALLOCATED_FIXED_SIZE        0x04
#define IRP_LOOKASIDE_ALLOCATION        0x08

#define IO_TYPE_ADAPTER                 0x01
#define IO_TYPE_CONTROLLER              0x02
#define IO_TYPE_DEVICE                  0x03
#define IO_TYPE_DRIVER                  0x04
#define IO_TYPE_FILE                    0x05
#define IO_TYPE_IRP                     0x06
#define IO_TYPE_MASTER_ADAPTER          0x07
#define IO_TYPE_OPEN_PACKET             0x08
#define IO_TYPE_TIMER                   0x09
#define IO_TYPE_VPB                     0x0a
#define IO_TYPE_ERROR_LOG               0x0b
#define IO_TYPE_ERROR_MESSAGE           0x0c
#define IO_TYPE_DEVICE_OBJECT_EXTENSION 0x0d
#define IO_TYPE_DEVICE_QUEUE            0x14

typedef struct _DEVICE_OBJECT {
  CSHORT  Type;
  USHORT  Size;
  LONG  ReferenceCount;
  struct _DRIVER_OBJECT  *DriverObject;
  struct _DEVICE_OBJECT  *NextDevice;
  struct _DEVICE_OBJECT  *AttachedDevice;
  struct _IRP  *CurrentIrp;
  PIO_TIMER  Timer;
  ULONG  Flags;
  ULONG  Characteristics;
  PVPB  Vpb;
  PVOID  DeviceExtension;
  DEVICE_TYPE  DeviceType;
  CCHAR  StackSize;
  union {
    LIST_ENTRY  ListEntry;
    WAIT_CONTEXT_BLOCK  Wcb;
  } Queue;
  ULONG  AlignmentRequirement;
  KDEVICE_QUEUE  DeviceQueue;
  KDPC  Dpc;
  ULONG  ActiveThreadCount;
  PSECURITY_DESCRIPTOR  SecurityDescriptor;
  KEVENT  DeviceLock;
  USHORT  SectorSize;
  USHORT  Spare1;
  struct _DEVOBJ_EXTENSION  *DeviceObjectExtension;
  PVOID  Reserved;
} DEVICE_OBJECT;
typedef struct _DEVICE_OBJECT *PDEVICE_OBJECT;

typedef struct _DEVICE_RELATIONS {
  ULONG Count;
  PDEVICE_OBJECT Objects[1];
} DEVICE_RELATIONS;
typedef struct _DEVICE_RELATIONS *PDEVICE_RELATIONS;

typedef struct _DRIVER_EXTENSION {
  struct _DRIVER_OBJECT  *DriverObject;
  PDRIVER_ADD_DEVICE AddDevice;
  ULONG  Count;
  UNICODE_STRING  ServiceKeyName;
} DRIVER_EXTENSION, *PDRIVER_EXTENSION;

typedef struct _DRIVER_OBJECT {
  CSHORT  Type;
  CSHORT  Size;
  PDEVICE_OBJECT  DeviceObject;
  ULONG  Flags;
  PVOID  DriverStart;
  ULONG  DriverSize;
  PVOID  DriverSection;
  PDRIVER_EXTENSION  DriverExtension;
  UNICODE_STRING  DriverName;
  PUNICODE_STRING  HardwareDatabase;
  PVOID  FastIoDispatch;
  PDRIVER_INITIALIZE DriverInit;
  PDRIVER_STARTIO    DriverStartIo;
  PDRIVER_UNLOAD     DriverUnload;
  PDRIVER_DISPATCH   MajorFunction[IRP_MJ_MAXIMUM_FUNCTION + 1];
} DRIVER_OBJECT;
typedef struct _DRIVER_OBJECT *PDRIVER_OBJECT;

/* Irp definitions */
typedef UCHAR KIRQL, *PKIRQL;
typedef CCHAR KPROCESSOR_MODE;
typedef enum _KAPC_ENVIRONMENT
{
    OriginalApcEnvironment,
    AttachedApcEnvironment,
    CurrentApcEnvironment,
    InsertApcEnvironment
} KAPC_ENVIRONMENT, *PKAPC_ENVIRONMENT;

typedef VOID (WINAPI *PDRIVER_CANCEL)(
  IN struct _DEVICE_OBJECT  *DeviceObject,
  IN struct _IRP  *Irp);

typedef VOID (WINAPI *PKNORMAL_ROUTINE)(
  IN PVOID  NormalContext,
  IN PVOID  SystemArgument1,
  IN PVOID  SystemArgument2);

typedef VOID (WINAPI *PKKERNEL_ROUTINE)(
  IN struct _KAPC  *Apc,
  IN OUT PKNORMAL_ROUTINE  *NormalRoutine,
  IN OUT PVOID  *NormalContext,
  IN OUT PVOID  *SystemArgument1,
  IN OUT PVOID  *SystemArgument2);

typedef VOID (WINAPI *PKRUNDOWN_ROUTINE)(
  IN struct _KAPC  *Apc);

typedef struct _KAPC {
  CSHORT  Type;
  CSHORT  Size;
  ULONG  Spare0;
  struct _KTHREAD  *Thread;
  LIST_ENTRY  ApcListEntry;
  PKKERNEL_ROUTINE  KernelRoutine;
  PKRUNDOWN_ROUTINE  RundownRoutine;
  PKNORMAL_ROUTINE  NormalRoutine;
  PVOID  NormalContext;
  PVOID  SystemArgument1;
  PVOID  SystemArgument2;
  CCHAR  ApcStateIndex;
  KPROCESSOR_MODE  ApcMode;
  BOOLEAN  Inserted;
} KAPC, *PKAPC, *RESTRICTED_POINTER PRKAPC;

typedef struct _IRP {
  CSHORT  Type;
  USHORT  Size;
  struct _MDL  *MdlAddress;
  ULONG  Flags;
  union {
    struct _IRP  *MasterIrp;
    LONG  IrpCount;
    PVOID  SystemBuffer;
  } AssociatedIrp;
  LIST_ENTRY  ThreadListEntry;
  IO_STATUS_BLOCK  IoStatus;
  KPROCESSOR_MODE  RequestorMode;
  BOOLEAN  PendingReturned;
  CHAR  StackCount;
  CHAR  CurrentLocation;
  BOOLEAN  Cancel;
  KIRQL  CancelIrql;
  CCHAR  ApcEnvironment;
  UCHAR  AllocationFlags;
  PIO_STATUS_BLOCK  UserIosb;
  PKEVENT  UserEvent;
  union {
    struct {
      PIO_APC_ROUTINE  UserApcRoutine;
      PVOID  UserApcContext;
    } AsynchronousParameters;
    LARGE_INTEGER  AllocationSize;
  } Overlay;
  PDRIVER_CANCEL  CancelRoutine;
  PVOID  UserBuffer;
  union {
    struct {
       union {
        KDEVICE_QUEUE_ENTRY  DeviceQueueEntry;
        struct {
          PVOID  DriverContext[4];
        } DUMMYSTRUCTNAME;
      } DUMMYUNIONNAME1;
      PETHREAD  Thread;
      PCHAR  AuxiliaryBuffer;
      struct {
        LIST_ENTRY  ListEntry;
        union {
          struct _IO_STACK_LOCATION  *CurrentStackLocation;
          ULONG  PacketType;
        } DUMMYUNIONNAME2;
      } DUMMYSTRUCTNAME;
      struct _FILE_OBJECT  *OriginalFileObject;
    } Overlay;
    KAPC  Apc;
    PVOID  CompletionKey;
  } Tail;
} IRP;
typedef struct _IRP *PIRP;

#define IRP_NOCACHE               0x0001
#define IRP_PAGING_IO             0x0002
#define IRP_MOUNT_COMPLETION      0x0002
#define IRP_SYNCHRONOUS_API       0x0004
#define IRP_ASSOCIATED_IRP        0x0008
#define IRP_BUFFERED_IO           0x0010
#define IRP_DEALLOCATE_BUFFER     0x0020
#define IRP_INPUT_OPERATION       0x0040
#define IRP_SYNCHRONOUS_PAGING_IO 0x0040
#define IRP_CREATE_OPERATION      0x0080
#define IRP_READ_OPERATION        0x0100
#define IRP_WRITE_OPERATION       0x0200
#define IRP_CLOSE_OPERATION       0x0400
#define IRP_DEFER_IO_COMPLETION   0x0800
#define IRP_OB_QUERY_NAME         0x1000
#define IRP_HOLD_DEVICE_QUEUE     0x2000

typedef VOID (WINAPI *PINTERFACE_REFERENCE)(
  PVOID  Context);

typedef VOID (WINAPI *PINTERFACE_DEREFERENCE)(
  PVOID Context);

typedef struct _INTERFACE {
  USHORT  Size;
  USHORT  Version;
  PVOID  Context;
  PINTERFACE_REFERENCE  InterfaceReference;
  PINTERFACE_DEREFERENCE  InterfaceDereference;
} INTERFACE, *PINTERFACE;

typedef struct _SECTION_OBJECT_POINTERS {
  PVOID  DataSectionObject;
  PVOID  SharedCacheMap;
  PVOID  ImageSectionObject;
} SECTION_OBJECT_POINTERS, *PSECTION_OBJECT_POINTERS;

typedef struct _IO_COMPLETION_CONTEXT {
  PVOID  Port;
  PVOID  Key;
} IO_COMPLETION_CONTEXT, *PIO_COMPLETION_CONTEXT;

typedef enum _DEVICE_RELATION_TYPE {
  BusRelations,
  EjectionRelations,
  PowerRelations,
  RemovalRelations,
  TargetDeviceRelation,
  SingleBusRelations
} DEVICE_RELATION_TYPE, *PDEVICE_RELATION_TYPE;

typedef struct _FILE_OBJECT {
  CSHORT  Type;
  CSHORT  Size;
  PDEVICE_OBJECT  DeviceObject;
  PVPB  Vpb;
  PVOID  FsContext;
  PVOID  FsContext2;
  PSECTION_OBJECT_POINTERS  SectionObjectPointer;
  PVOID  PrivateCacheMap;
  NTSTATUS  FinalStatus;
  struct _FILE_OBJECT  *RelatedFileObject;
  BOOLEAN  LockOperation;
  BOOLEAN  DeletePending;
  BOOLEAN  ReadAccess;
  BOOLEAN  WriteAccess;
  BOOLEAN  DeleteAccess;
  BOOLEAN  SharedRead;
  BOOLEAN  SharedWrite;
  BOOLEAN  SharedDelete;
  ULONG  Flags;
  UNICODE_STRING  FileName;
  LARGE_INTEGER  CurrentByteOffset;
  ULONG  Waiters;
  ULONG  Busy;
  PVOID  LastLock;
  KEVENT  Lock;
  KEVENT  Event;
  PIO_COMPLETION_CONTEXT  CompletionContext;
} FILE_OBJECT;
typedef struct _FILE_OBJECT *PFILE_OBJECT;

#define INITIAL_PRIVILEGE_COUNT           3

typedef struct _INITIAL_PRIVILEGE_SET {
  ULONG  PrivilegeCount;
  ULONG  Control;
  LUID_AND_ATTRIBUTES  Privilege[INITIAL_PRIVILEGE_COUNT];
} INITIAL_PRIVILEGE_SET, * PINITIAL_PRIVILEGE_SET;

typedef struct _SECURITY_SUBJECT_CONTEXT {
  PACCESS_TOKEN  ClientToken;
  SECURITY_IMPERSONATION_LEVEL  ImpersonationLevel;
  PACCESS_TOKEN  PrimaryToken;
  PVOID  ProcessAuditId;
} SECURITY_SUBJECT_CONTEXT, *PSECURITY_SUBJECT_CONTEXT;

typedef struct _ACCESS_STATE {
  LUID  OperationID;
  BOOLEAN  SecurityEvaluated;
  BOOLEAN  GenerateAudit;
  BOOLEAN  GenerateOnClose;
  BOOLEAN  PrivilegesAllocated;
  ULONG  Flags;
  ACCESS_MASK  RemainingDesiredAccess;
  ACCESS_MASK  PreviouslyGrantedAccess;
  ACCESS_MASK  OriginalDesiredAccess;
  SECURITY_SUBJECT_CONTEXT  SubjectSecurityContext;
  PSECURITY_DESCRIPTOR  SecurityDescriptor;
  PVOID  AuxData;
  union {
    INITIAL_PRIVILEGE_SET  InitialPrivilegeSet;
    PRIVILEGE_SET  PrivilegeSet;
  } Privileges;

  BOOLEAN  AuditPrivileges;
  UNICODE_STRING  ObjectName;
  UNICODE_STRING  ObjectTypeName;
} ACCESS_STATE, *PACCESS_STATE;

typedef struct _IO_SECURITY_CONTEXT {
  PSECURITY_QUALITY_OF_SERVICE  SecurityQos;
  PACCESS_STATE  AccessState;
  ACCESS_MASK  DesiredAccess;
  ULONG  FullCreateOptions;
} IO_SECURITY_CONTEXT, *PIO_SECURITY_CONTEXT;

typedef struct _DEVICE_CAPABILITIES {
  USHORT  Size;
  USHORT  Version;
  ULONG  DeviceD1 : 1;
  ULONG  DeviceD2 : 1;
  ULONG  LockSupported : 1;
  ULONG  EjectSupported : 1;
  ULONG  Removable : 1;
  ULONG  DockDevice : 1;
  ULONG  UniqueID : 1;
  ULONG  SilentInstall : 1;
  ULONG  RawDeviceOK : 1;
  ULONG  SurpriseRemovalOK : 1;
  ULONG  WakeFromD0 : 1;
  ULONG  WakeFromD1 : 1;
  ULONG  WakeFromD2 : 1;
  ULONG  WakeFromD3 : 1;
  ULONG  HardwareDisabled : 1;
  ULONG  NonDynamic : 1;
  ULONG  WarmEjectSupported : 1;
  ULONG  NoDisplayInUI : 1;
  ULONG  Reserved : 14;
  ULONG  Address;
  ULONG  UINumber;
  DEVICE_POWER_STATE  DeviceState[PowerSystemMaximum];
  SYSTEM_POWER_STATE  SystemWake;
  DEVICE_POWER_STATE  DeviceWake;
  ULONG  D1Latency;
  ULONG  D2Latency;
  ULONG  D3Latency;
} DEVICE_CAPABILITIES, *PDEVICE_CAPABILITIES;

typedef struct _DEVICE_INTERFACE_CHANGE_NOTIFICATION {
  USHORT Version;
  USHORT Size;
  GUID Event;
  GUID InterfaceClassGuid;
  PUNICODE_STRING SymbolicLinkName;
} DEVICE_INTERFACE_CHANGE_NOTIFICATION, *PDEVICE_INTERFACE_CHANGE_NOTIFICATION;

typedef enum _INTERFACE_TYPE {
  InterfaceTypeUndefined = -1,
  Internal,
  Isa,
  Eisa,
  MicroChannel,
  TurboChannel,
  PCIBus,
  VMEBus,
  NuBus,
  PCMCIABus,
  CBus,
  MPIBus,
  MPSABus,
  ProcessorInternal,
  InternalPowerBus,
  PNPISABus,
  PNPBus,
  MaximumInterfaceType
} INTERFACE_TYPE, *PINTERFACE_TYPE;

typedef LARGE_INTEGER PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;

#define IO_RESOURCE_PREFERRED             0x01
#define IO_RESOURCE_DEFAULT               0x02
#define IO_RESOURCE_ALTERNATIVE           0x08

typedef struct _IO_RESOURCE_DESCRIPTOR {
  UCHAR  Option;
  UCHAR  Type;
  UCHAR  ShareDisposition;
  UCHAR  Spare1;
  USHORT  Flags;
  USHORT  Spare2;
  union {
    struct {
      ULONG  Length;
      ULONG  Alignment;
      PHYSICAL_ADDRESS  MinimumAddress;
      PHYSICAL_ADDRESS  MaximumAddress;
    } Port;
    struct {
      ULONG  Length;
      ULONG  Alignment;
      PHYSICAL_ADDRESS  MinimumAddress;
      PHYSICAL_ADDRESS  MaximumAddress;
    } Memory;
    struct {
      ULONG  MinimumVector;
      ULONG  MaximumVector;
    } Interrupt;
    struct {
      ULONG  MinimumChannel;
      ULONG  MaximumChannel;
    } Dma;
    struct {
      ULONG  Length;
      ULONG  Alignment;
      PHYSICAL_ADDRESS  MinimumAddress;
      PHYSICAL_ADDRESS  MaximumAddress;
    } Generic;
    struct {
      ULONG  Data[3];
    } DevicePrivate;
    struct {
      ULONG  Length;
      ULONG  MinBusNumber;
      ULONG  MaxBusNumber;
      ULONG  Reserved;
    } BusNumber;
    struct {
      ULONG  Priority;
      ULONG  Reserved1;
      ULONG  Reserved2;
    } ConfigData;
  } u;
} IO_RESOURCE_DESCRIPTOR, *PIO_RESOURCE_DESCRIPTOR;

typedef struct _IO_RESOURCE_LIST {
  USHORT  Version;
  USHORT  Revision;
  ULONG  Count;
  IO_RESOURCE_DESCRIPTOR  Descriptors[1];
} IO_RESOURCE_LIST, *PIO_RESOURCE_LIST;

typedef struct _IO_RESOURCE_REQUIREMENTS_LIST {
  ULONG  ListSize;
  INTERFACE_TYPE  InterfaceType;
  ULONG  BusNumber;
  ULONG  SlotNumber;
  ULONG  Reserved[3];
  ULONG  AlternativeLists;
  IO_RESOURCE_LIST  List[1];
} IO_RESOURCE_REQUIREMENTS_LIST, *PIO_RESOURCE_REQUIREMENTS_LIST;

typedef enum _BUS_QUERY_ID_TYPE {
  BusQueryDeviceID,
  BusQueryHardwareIDs,
  BusQueryCompatibleIDs,
  BusQueryInstanceID,
  BusQueryDeviceSerialNumber,
  BusQueryContainerID,
} BUS_QUERY_ID_TYPE, *PBUS_QUERY_ID_TYPE;

typedef enum _CREATE_FILE_TYPE {
  CreateFileTypeNone,
  CreateFileTypeNamedPipe,
  CreateFileTypeMailslot
} CREATE_FILE_TYPE;

typedef enum {
  DevicePropertyDeviceDescription,
  DevicePropertyHardwareID,
  DevicePropertyCompatibleIDs,
  DevicePropertyBootConfiguration,
  DevicePropertyBootConfigurationTranslated,
  DevicePropertyClassName,
  DevicePropertyClassGuid,
  DevicePropertyDriverKeyName,
  DevicePropertyManufacturer,
  DevicePropertyFriendlyName,
  DevicePropertyLocationInformation,
  DevicePropertyPhysicalDeviceObjectName,
  DevicePropertyBusTypeGuid,
  DevicePropertyLegacyBusType,
  DevicePropertyBusNumber,
  DevicePropertyEnumeratorName,
  DevicePropertyAddress,
  DevicePropertyUINumber,
  DevicePropertyInstallState,
  DevicePropertyRemovalPolicy
} DEVICE_REGISTRY_PROPERTY;

typedef enum _DEVICE_TEXT_TYPE {
  DeviceTextDescription,
  DeviceTextLocationInformation
} DEVICE_TEXT_TYPE, *PDEVICE_TEXT_TYPE;

typedef enum _DEVICE_USAGE_NOTIFICATION_TYPE {
  DeviceUsageTypeUndefined,
  DeviceUsageTypePaging,
  DeviceUsageTypeHibernation,
  DeviceUsageTypeDumpFile
} DEVICE_USAGE_NOTIFICATION_TYPE;

typedef struct _POWER_SEQUENCE {
  ULONG  SequenceD1;
  ULONG  SequenceD2;
  ULONG  SequenceD3;
} POWER_SEQUENCE, *PPOWER_SEQUENCE;

typedef enum _POWER_STATE_TYPE {
  SystemPowerState,
  DevicePowerState
} POWER_STATE_TYPE, *PPOWER_STATE_TYPE;

typedef union _POWER_STATE {
  SYSTEM_POWER_STATE  SystemState;
  DEVICE_POWER_STATE  DeviceState;
} POWER_STATE, *PPOWER_STATE;

typedef struct _CM_PARTIAL_RESOURCE_DESCRIPTOR {
  UCHAR Type;
  UCHAR ShareDisposition;
  USHORT Flags;
  union {
    struct {
      PHYSICAL_ADDRESS Start;
      ULONG Length;
    } Generic;
    struct {
      PHYSICAL_ADDRESS Start;
      ULONG Length;
    } Port;
    struct {
      ULONG Level;
      ULONG Vector;
      ULONG Affinity;
    } Interrupt;
    struct {
      PHYSICAL_ADDRESS Start;
      ULONG Length;
    } Memory;
    struct {
      ULONG Channel;
      ULONG Port;
      ULONG Reserved1;
    } Dma;
    struct {
      ULONG Data[3];
    } DevicePrivate;
    struct {
      ULONG Start;
      ULONG Length;
      ULONG Reserved;
    } BusNumber;
    struct {
      ULONG DataSize;
      ULONG Reserved1;
      ULONG Reserved2;
    } DeviceSpecificData;
  } u;
} CM_PARTIAL_RESOURCE_DESCRIPTOR, *PCM_PARTIAL_RESOURCE_DESCRIPTOR;

typedef struct _CM_PARTIAL_RESOURCE_LIST {
  USHORT  Version;
  USHORT  Revision;
  ULONG  Count;
  CM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptors[1];
} CM_PARTIAL_RESOURCE_LIST, *PCM_PARTIAL_RESOURCE_LIST;

typedef struct _CM_FULL_RESOURCE_DESCRIPTOR {
  INTERFACE_TYPE  InterfaceType;
  ULONG  BusNumber;
  CM_PARTIAL_RESOURCE_LIST  PartialResourceList;
} CM_FULL_RESOURCE_DESCRIPTOR, *PCM_FULL_RESOURCE_DESCRIPTOR;

typedef struct _CM_RESOURCE_LIST {
  ULONG  Count;
  CM_FULL_RESOURCE_DESCRIPTOR  List[1];
} CM_RESOURCE_LIST, *PCM_RESOURCE_LIST;

typedef NTSTATUS (WINAPI *PIO_COMPLETION_ROUTINE)(
  IN struct _DEVICE_OBJECT  *DeviceObject,
  IN struct _IRP  *Irp,
  IN PVOID  Context);

#define SL_PENDING_RETURNED             0x01
#define SL_INVOKE_ON_CANCEL             0x20
#define SL_INVOKE_ON_SUCCESS            0x40
#define SL_INVOKE_ON_ERROR              0x80

#if !defined(_WIN64)
#pragma pack(push,4)
#endif
typedef struct _IO_STACK_LOCATION {
  UCHAR  MajorFunction;
  UCHAR  MinorFunction;
  UCHAR  Flags;
  UCHAR  Control;
  union {
    struct {
      PIO_SECURITY_CONTEXT  SecurityContext;
      ULONG  Options;
      USHORT POINTER_ALIGNMENT  FileAttributes;
      USHORT  ShareAccess;
      ULONG POINTER_ALIGNMENT  EaLength;
    } Create;
    struct {
      ULONG  Length;
      ULONG POINTER_ALIGNMENT  Key;
      LARGE_INTEGER  ByteOffset;
    } Read;
    struct {
      ULONG  Length;
      ULONG POINTER_ALIGNMENT  Key;
      LARGE_INTEGER  ByteOffset;
    } Write;
    struct {
      ULONG  Length;
      FILE_INFORMATION_CLASS POINTER_ALIGNMENT  FileInformationClass;
    } QueryFile;
    struct {
      ULONG  Length;
      FILE_INFORMATION_CLASS POINTER_ALIGNMENT  FileInformationClass;
      PFILE_OBJECT  FileObject;
      union {
        struct {
          BOOLEAN  ReplaceIfExists;
          BOOLEAN  AdvanceOnly;
        } DUMMYSTRUCTNAME;
        ULONG  ClusterCount;
        HANDLE  DeleteHandle;
      } DUMMYUNIONNAME;
    } SetFile;
    struct {
      ULONG  Length;
      FS_INFORMATION_CLASS POINTER_ALIGNMENT  FsInformationClass;
    } QueryVolume;
    struct {
      ULONG  OutputBufferLength;
      ULONG POINTER_ALIGNMENT  InputBufferLength;
      ULONG POINTER_ALIGNMENT  IoControlCode;
      PVOID  Type3InputBuffer;
    } DeviceIoControl;
    struct {
      SECURITY_INFORMATION  SecurityInformation;
      ULONG POINTER_ALIGNMENT  Length;
    } QuerySecurity;
    struct {
      SECURITY_INFORMATION  SecurityInformation;
      PSECURITY_DESCRIPTOR  SecurityDescriptor;
    } SetSecurity;
    struct {
      PVPB  Vpb;
      PDEVICE_OBJECT  DeviceObject;
    } MountVolume;
    struct {
      PVPB  Vpb;
      PDEVICE_OBJECT  DeviceObject;
    } VerifyVolume;
    struct {
      struct _SCSI_REQUEST_BLOCK  *Srb;
    } Scsi;
    struct {
      DEVICE_RELATION_TYPE  Type;
    } QueryDeviceRelations;
    struct {
      const GUID  *InterfaceType;
      USHORT  Size;
      USHORT  Version;
      PINTERFACE  Interface;
      PVOID  InterfaceSpecificData;
    } QueryInterface;
    struct {
      PDEVICE_CAPABILITIES  Capabilities;
    } DeviceCapabilities;
    struct {
      PIO_RESOURCE_REQUIREMENTS_LIST  IoResourceRequirementList;
    } FilterResourceRequirements;
    struct {
      ULONG  WhichSpace;
      PVOID  Buffer;
      ULONG  Offset;
      ULONG POINTER_ALIGNMENT  Length;
    } ReadWriteConfig;
    struct {
      BOOLEAN  Lock;
    } SetLock;
    struct {
      BUS_QUERY_ID_TYPE  IdType;
    } QueryId;
    struct {
      DEVICE_TEXT_TYPE  DeviceTextType;
      LCID POINTER_ALIGNMENT  LocaleId;
    } QueryDeviceText;
    struct {
      BOOLEAN  InPath;
      BOOLEAN  Reserved[3];
      DEVICE_USAGE_NOTIFICATION_TYPE POINTER_ALIGNMENT  Type;
    } UsageNotification;
    struct {
      SYSTEM_POWER_STATE  PowerState;
    } WaitWake;
    struct {
      PPOWER_SEQUENCE  PowerSequence;
    } PowerSequence;
    struct {
      ULONG  SystemContext;
      POWER_STATE_TYPE POINTER_ALIGNMENT  Type;
      POWER_STATE POINTER_ALIGNMENT  State;
      POWER_ACTION POINTER_ALIGNMENT  ShutdownType;
    } Power;
    struct {
      PCM_RESOURCE_LIST  AllocatedResources;
      PCM_RESOURCE_LIST  AllocatedResourcesTranslated;
    } StartDevice;
    struct {
      ULONG_PTR  ProviderId;
      PVOID  DataPath;
      ULONG  BufferSize;
      PVOID  Buffer;
    } WMI;
    struct {
      PVOID  Argument1;
      PVOID  Argument2;
      PVOID  Argument3;
      PVOID  Argument4;
    } Others;
  } Parameters;
  PDEVICE_OBJECT  DeviceObject;
  PFILE_OBJECT  FileObject;
  PIO_COMPLETION_ROUTINE  CompletionRoutine;
  PVOID  Context;
} IO_STACK_LOCATION, *PIO_STACK_LOCATION;
#if !defined(_WIN64)
#pragma pack(pop)
#endif

/* MDL definitions */

#define MDL_MAPPED_TO_SYSTEM_VA     0x0001
#define MDL_PAGES_LOCKED            0x0002
#define MDL_SOURCE_IS_NONPAGED_POOL 0x0004
#define MDL_ALLOCATED_FIXED_SIZE    0x0008
#define MDL_PARTIAL                 0x0010
#define MDL_PARTIAL_HAS_BEEN_MAPPED 0x0020
#define MDL_IO_PAGE_READ            0x0040
#define MDL_WRITE_OPERATION         0x0080
#define MDL_PARENT_MAPPED_SYSTEM_VA 0x0100
#define MDL_FREE_EXTRA_PTES         0x0200
#define MDL_DESCRIBES_AWE           0x0400
#define MDL_IO_SPACE                0x0800
#define MDL_NETWORK_HEADER          0x1000
#define MDL_MAPPING_CAN_FAIL        0x2000
#define MDL_ALLOCATED_MUST_SUCCEED  0x4000
#define MDL_INTERNAL                0x8000

#define MDL_MAPPING_FLAGS (MDL_MAPPED_TO_SYSTEM_VA     | \
                           MDL_PAGES_LOCKED            | \
                           MDL_SOURCE_IS_NONPAGED_POOL | \
                           MDL_PARTIAL_HAS_BEEN_MAPPED | \
                           MDL_PARENT_MAPPED_SYSTEM_VA | \
                           MDL_SYSTEM_VA               | \
                           MDL_IO_SPACE )

typedef struct _MDL {
  struct _MDL  *Next;
  CSHORT  Size;
  CSHORT  MdlFlags;
  struct _EPROCESS  *Process;
  PVOID  MappedSystemVa;
  PVOID  StartVa;
  ULONG  ByteCount;
  ULONG  ByteOffset;
} MDL, *PMDL;

typedef MDL *PMDLX;
typedef ULONG PFN_NUMBER, *PPFN_NUMBER;

static inline void MmInitializeMdl(MDL *mdl, void *va, SIZE_T length)
{
    mdl->Next       = NULL;
    mdl->Size       = sizeof(MDL) + sizeof(PFN_NUMBER) * ADDRESS_AND_SIZE_TO_SPAN_PAGES(va, length);
    mdl->MdlFlags   = 0;
    mdl->StartVa    = (void *)PAGE_ALIGN(va);
    mdl->ByteOffset = BYTE_OFFSET(va);
    mdl->ByteCount  = length;
}

typedef struct _KTIMER {
    DISPATCHER_HEADER Header;
    ULARGE_INTEGER DueTime;
    LIST_ENTRY TimerListEntry;
    struct _KDPC *Dpc;
    LONG Period;
} KTIMER, *PKTIMER;

typedef struct _KSYSTEM_TIME {
    ULONG LowPart;
    LONG High1Time;
    LONG High2Time;
} KSYSTEM_TIME, *PKSYSTEM_TIME;

typedef enum _NT_PRODUCT_TYPE {
    NtProductWinNt = 1,
    NtProductLanManNt,
    NtProductServer
} NT_PRODUCT_TYPE, *PNT_PRODUCT_TYPE;

#define PROCESSOR_FEATURE_MAX 64

typedef enum _ALTERNATIVE_ARCHITECTURE_TYPE
{
   StandardDesign,
   NEC98x86,
   EndAlternatives
} ALTERNATIVE_ARCHITECTURE_TYPE;

#define NX_SUPPORT_POLICY_ALWAYSOFF     0
#define NX_SUPPORT_POLICY_ALWAYSON      1
#define NX_SUPPORT_POLICY_OPTIN         2
#define NX_SUPPORT_POLICY_OPTOUT        3

typedef struct _KUSER_SHARED_DATA {
    ULONG TickCountLowDeprecated;                          /* 0x000 */
    ULONG TickCountMultiplier;                             /* 0x004 */
    volatile KSYSTEM_TIME InterruptTime;                   /* 0x008 */
    volatile KSYSTEM_TIME SystemTime;                      /* 0x014 */
    volatile KSYSTEM_TIME TimeZoneBias;                    /* 0x020 */
    USHORT ImageNumberLow;                                 /* 0x02c */
    USHORT ImageNumberHigh;                                /* 0x02e */
    WCHAR NtSystemRoot[260];                               /* 0x030 */
    ULONG MaxStackTraceDepth;                              /* 0x238 */
    ULONG CryptoExponent;                                  /* 0x23c */
    ULONG TimeZoneId;                                      /* 0x240 */
    ULONG LargePageMinimum;                                /* 0x244 */
    ULONG AitSamplingValue;                                /* 0x248 */
    ULONG AppCompatFlag;                                   /* 0x24c */
    ULONGLONG RNGSeedVersion;                              /* 0x250 */
    ULONG GlobalValidationRunLevel;                        /* 0x258 */
    volatile ULONG TimeZoneBiasStamp;                      /* 0x25c */
    ULONG NtBuildNumber;                                   /* 0x260 */
    NT_PRODUCT_TYPE NtProductType;                         /* 0x264 */
    BOOLEAN ProductTypeIsValid;                            /* 0x268 */
    USHORT NativeProcessorArchitecture;                    /* 0x26a */
    ULONG NtMajorVersion;                                  /* 0x26c */
    ULONG NtMinorVersion;                                  /* 0x270 */
    BOOLEAN ProcessorFeatures[PROCESSOR_FEATURE_MAX];      /* 0x274 */
    ULONG Reserved1;                                       /* 0x2b4 */
    ULONG Reserved3;                                       /* 0x2b8 */
    volatile ULONG TimeSlip;                               /* 0x2bc */
    ALTERNATIVE_ARCHITECTURE_TYPE AlternativeArchitecture; /* 0x2c0 */
    ULONG BootId;                                          /* 0x2c4 */
    LARGE_INTEGER SystemExpirationDate;                    /* 0x2c8 */
    ULONG SuiteMask;                                       /* 0x2d0 */
    BOOLEAN KdDebuggerEnabled;                             /* 0x2d4 */
    UCHAR NXSupportPolicy;                                 /* 0x2d5 */
    USHORT CyclesPerYield;                                 /* 0x2d6 */
    volatile ULONG ActiveConsoleId;                        /* 0x2d8 */
    volatile ULONG DismountCount;                          /* 0x2dc */
    ULONG ComPlusPackage;                                  /* 0x2e0 */
    ULONG LastSystemRITEventTickCount;                     /* 0x2e4 */
    ULONG NumberOfPhysicalPages;                           /* 0x2e8 */
    BOOLEAN SafeBootMode;                                  /* 0x2ec */
    UCHAR VirtualizationFlags;                             /* 0x2ed */
    union {
        ULONG SharedDataFlags;                             /* 0x2f0 */
        struct {
            ULONG DbgErrorPortPresent       : 1;
            ULONG DbgElevationEnabed        : 1;
            ULONG DbgVirtEnabled            : 1;
            ULONG DbgInstallerDetectEnabled : 1;
            ULONG DbgLkgEnabled             : 1;
            ULONG DbgDynProcessorEnabled    : 1;
            ULONG DbgConsoleBrokerEnabled   : 1;
            ULONG DbgSecureBootEnabled      : 1;
            ULONG DbgMultiSessionSku        : 1;
            ULONG DbgMultiUsersInSessionSku : 1;
            ULONG DbgStateSeparationEnabled : 1;
            ULONG SpareBits                 : 21;
        } DUMMYSTRUCTNAME2;
    } DUMMYUNIONNAME2;
    ULONG DataFlagsPad[1];                                 /* 0x2f4 */
    ULONGLONG TestRetInstruction;                          /* 0x2f8 */
    LONGLONG QpcFrequency;                                 /* 0x300 */
    ULONG SystemCall;                                      /* 0x308 */
    union {
        ULONG AllFlags;                                    /* 0x30c */
        struct {
            ULONG Win32Process            : 1;
            ULONG Sgx2Enclave             : 1;
            ULONG VbsBasicEnclave         : 1;
            ULONG SpareBits               : 29;
        } DUMMYSTRUCTNAME;
    } UserCetAvailableEnvironments;
    ULONGLONG SystemCallPad[2];                            /* 0x310 */
    union {
        volatile KSYSTEM_TIME TickCount;                   /* 0x320 */
        volatile ULONG64 TickCountQuad;
    } DUMMYUNIONNAME;
    ULONG Cookie;                                          /* 0x330 */
    ULONG CookiePad[1];                                    /* 0x334 */
    LONGLONG ConsoleSessionForegroundProcessId;            /* 0x338 */
    ULONGLONG TimeUpdateLock;                              /* 0x340 */
    ULONGLONG BaselineSystemTimeQpc;                       /* 0x348 */
    ULONGLONG BaselineInterruptTimeQpc;                    /* 0x350 */
    ULONGLONG QpcSystemTimeIncrement;                      /* 0x358 */
    ULONGLONG QpcInterruptTimeIncrement;                   /* 0x360 */
    UCHAR QpcSystemTimeIncrementShift;                     /* 0x368 */
    UCHAR QpcInterruptTimeIncrementShift;                  /* 0x369 */
    USHORT UnparkedProcessorCount;                         /* 0x36a */
    ULONG EnclaveFeatureMask[4];                           /* 0x36c */
    ULONG TelemetryCoverageRound;                          /* 0x37c */
    USHORT UserModeGlobalLogger[16];                       /* 0x380 */
    ULONG ImageFileExecutionOptions;                       /* 0x3a0 */
    ULONG LangGenerationCount;                             /* 0x3a4 */
    ULONG ActiveProcessorAffinity;                         /* 0x3a8 */
    volatile ULONGLONG InterruptTimeBias;                  /* 0x3b0 */
    volatile ULONGLONG QpcBias;                            /* 0x3b8 */
    ULONG ActiveProcessorCount;                            /* 0x3c0 */
    volatile UCHAR ActiveGroupCount;                       /* 0x3c4 */
    union {
        USHORT QpcData;                                    /* 0x3c6 */
        struct {
            UCHAR volatile QpcBypassEnabled;
            UCHAR QpcShift;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME3;
    LARGE_INTEGER TimeZoneBiasEffectiveStart;              /* 0x3c8 */
    LARGE_INTEGER TimeZoneBiasEffectiveEnd;                /* 0x3d0 */
    XSTATE_CONFIGURATION XState;                           /* 0x3d8 */
    KSYSTEM_TIME FeatureConfigurationChangeStamp;          /* 0x720 */
    ULONG Spare;
    ULONG64 UserPointerAuthMask;                           /* 0x730 */
} KSHARED_USER_DATA, *PKSHARED_USER_DATA;

C_ASSERT( sizeof(KSHARED_USER_DATA) == 0x738 );

#define SHARED_GLOBAL_FLAGS_QPC_BYPASS_ENABLED 0x01
#define SHARED_GLOBAL_FLAGS_QPC_BYPASS_USE_HV_PAGE 0x02
#define SHARED_GLOBAL_FLAGS_QPC_BYPASS_DISABLE_32BIT 0x04
#define SHARED_GLOBAL_FLAGS_QPC_BYPASS_USE_MFENCE 0x10
#define SHARED_GLOBAL_FLAGS_QPC_BYPASS_USE_LFENCE 0x20
#define SHARED_GLOBAL_FLAGS_QPC_BYPASS_A73_ERRATA 0x40
#define SHARED_GLOBAL_FLAGS_QPC_BYPASS_USE_RDTSCP 0x80

typedef enum _MEMORY_CACHING_TYPE {
    MmNonCached = 0,
    MmCached = 1,
    MmWriteCombined = 2,
    MmHardwareCoherentCached = 3,
    MmNonCachedUnordered = 4,
    MmUSWCCached = 5,
    MmMaximumCacheType = 6
} MEMORY_CACHING_TYPE;

typedef enum _MM_PAGE_PRIORITY {
    LowPagePriority,
    NormalPagePriority = 16,
    HighPagePriority = 32
} MM_PAGE_PRIORITY;

typedef enum _MM_SYSTEM_SIZE
{
    MmSmallSystem,
    MmMediumSystem,
    MmLargeSystem
} MM_SYSTEMSIZE;

typedef struct _IO_REMOVE_LOCK_COMMON_BLOCK {
    BOOLEAN Removed;
    BOOLEAN Reserved[3];
    LONG IoCount;
    KEVENT RemoveEvent;
} IO_REMOVE_LOCK_COMMON_BLOCK;

typedef struct _IO_REMOVE_LOCK_TRACKING_BLOCK *PIO_REMOVE_LOCK_TRACKING_BLOCK;

typedef struct _IO_REMOVE_LOCK_DBG_BLOCK {
    LONG Signature;
    LONG HighWatermark;
    LONGLONG MaxLockedTicks;
    LONG AllocateTag;
    LIST_ENTRY LockList;
    KSPIN_LOCK Spin;
    LONG LowMemoryCount;
    ULONG Reserved1[4];
    PVOID Reserved2;
    PIO_REMOVE_LOCK_TRACKING_BLOCK Blocks;
} IO_REMOVE_LOCK_DBG_BLOCK;

typedef struct _IO_REMOVE_LOCK {
    IO_REMOVE_LOCK_COMMON_BLOCK Common;
    IO_REMOVE_LOCK_DBG_BLOCK Dbg;
} IO_REMOVE_LOCK, *PIO_REMOVE_LOCK;

typedef enum  {
    IoReadAccess,
    IoWriteAccess,
    IoModifyAccess
} LOCK_OPERATION;

typedef struct _CALLBACK_OBJECT
{
    ULONG Signature;
    KSPIN_LOCK Lock;
    LIST_ENTRY RegisteredCallbacks;
    BOOLEAN AllowMultipleCallbacks;
    UCHAR reserved[3];
} CALLBACK_OBJECT, *PCALLBACK_OBJECT;

typedef struct _KSPIN_LOCK_QUEUE {
    struct _KSPIN_LOCK_QUEUE * volatile Next;
    volatile PKSPIN_LOCK Lock;
} KSPIN_LOCK_QUEUE, *PKSPIN_LOCK_QUEUE;

typedef struct _KLOCK_QUEUE_HANDLE {
    KSPIN_LOCK_QUEUE LockQueue;
    KIRQL OldIrql;
} KLOCK_QUEUE_HANDLE, *PKLOCK_QUEUE_HANDLE;

typedef void * (__WINE_ALLOC_SIZE(2) NTAPI *PALLOCATE_FUNCTION)(POOL_TYPE, SIZE_T, ULONG);
typedef void * (__WINE_ALLOC_SIZE(2) NTAPI *PALLOCATE_FUNCTION_EX)(POOL_TYPE, SIZE_T, ULONG, PLOOKASIDE_LIST_EX);
typedef void (NTAPI *PFREE_FUNCTION)(void *);
typedef void (NTAPI *PFREE_FUNCTION_EX)(void *, PLOOKASIDE_LIST_EX);
typedef void (NTAPI *PCALLBACK_FUNCTION)(void *, void *, void *);

#ifdef _WIN64
#define LOOKASIDE_ALIGN DECLSPEC_CACHEALIGN
#else
#define LOOKASIDE_ALIGN
#endif

#define LOOKASIDE_MINIMUM_BLOCK_SIZE (RTL_SIZEOF_THROUGH_FIELD(SLIST_ENTRY, Next))

#define GENERAL_LOOKASIDE_LAYOUT           \
    union                                  \
    {                                      \
        SLIST_HEADER ListHead;             \
        SINGLE_LIST_ENTRY SingleListHead;  \
    } DUMMYUNIONNAME;                      \
    USHORT Depth;                          \
    USHORT MaximumDepth;                   \
    ULONG TotalAllocates;                  \
    union                                  \
    {                                      \
        ULONG AllocateMisses;              \
        ULONG AllocateHits;                \
    } DUMMYUNIONNAME2;                     \
    ULONG TotalFrees;                      \
    union                                  \
    {                                      \
        ULONG FreeMisses;                  \
        ULONG FreeHits;                    \
    } DUMMYUNIONNAME3;                     \
    POOL_TYPE Type;                        \
    ULONG Tag;                             \
    ULONG Size;                            \
    union                                  \
    {                                      \
        PALLOCATE_FUNCTION_EX AllocateEx;  \
        PALLOCATE_FUNCTION Allocate;       \
    } DUMMYUNIONNAME4;                     \
    union                                  \
    {                                      \
        PFREE_FUNCTION_EX FreeEx;          \
        PFREE_FUNCTION Free;               \
    } DUMMYUNIONNAME5;                     \
    LIST_ENTRY ListEntry;                  \
    ULONG LastTotalAllocates;              \
    union                                  \
    {                                      \
        ULONG LastAllocateMisses;          \
        ULONG LastAllocateHits;            \
    } DUMMYUNIONNAME6;                     \
    ULONG Future[2];

typedef struct LOOKASIDE_ALIGN _GENERAL_LOOKASIDE
{
    GENERAL_LOOKASIDE_LAYOUT
} GENERAL_LOOKASIDE;

typedef struct _GENERAL_LOOKASIDE_POOL
{
    GENERAL_LOOKASIDE_LAYOUT
} GENERAL_LOOKASIDE_POOL, *PGENERAL_LOOKASIDE_POOL;

typedef struct _LOOKASIDE_LIST_EX
{
    GENERAL_LOOKASIDE_POOL L;
} LOOKASIDE_LIST_EX;

typedef struct LOOKASIDE_ALIGN _NPAGED_LOOKASIDE_LIST
{
    GENERAL_LOOKASIDE L;
#if defined(__i386__)
    KSPIN_LOCK Lock__ObsoleteButDoNotDelete;
#endif
} NPAGED_LOOKASIDE_LIST, *PNPAGED_LOOKASIDE_LIST;

typedef struct LOOKASIDE_ALIGN _PAGED_LOOKASIDE_LIST
{
    GENERAL_LOOKASIDE L;
#if defined(__i386__)
    FAST_MUTEX Lock__ObsoleteButDoNotDelete;
#endif
} PAGED_LOOKASIDE_LIST, *PPAGED_LOOKASIDE_LIST;

typedef NTSTATUS (NTAPI EX_CALLBACK_FUNCTION)(void *CallbackContext, void *Argument1, void *Argument2);
typedef EX_CALLBACK_FUNCTION *PEX_CALLBACK_FUNCTION;

typedef ULONG OB_OPERATION;

typedef struct _OB_PRE_CREATE_HANDLE_INFORMATION {
    ACCESS_MASK DesiredAccess;
    ACCESS_MASK OriginalDesiredAccess;
} OB_PRE_CREATE_HANDLE_INFORMATION, *POB_PRE_CREATE_HANDLE_INFORMATION;

typedef struct _OB_PRE_DUPLICATE_HANDLE_INFORMATION {
    ACCESS_MASK DesiredAccess;
    ACCESS_MASK OriginalDesiredAccess;
    PVOID SourceProcess;
    PVOID TargetProcess;
} OB_PRE_DUPLICATE_HANDLE_INFORMATION, *POB_PRE_DUPLICATE_HANDLE_INFORMATION;

typedef union _OB_PRE_OPERATION_PARAMETERS {
    OB_PRE_CREATE_HANDLE_INFORMATION CreateHandleInformation;
    OB_PRE_DUPLICATE_HANDLE_INFORMATION DuplicateHandleInformation;
} OB_PRE_OPERATION_PARAMETERS, *POB_PRE_OPERATION_PARAMETERS;

typedef struct _OB_PRE_OPERATION_INFORMATION {
    OB_OPERATION Operation;
    union {
        ULONG Flags;
        struct {
            ULONG KernelHandle:1;
            ULONG Reserved:31;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    PVOID Object;
    POBJECT_TYPE ObjectType;
    PVOID CallContext;
    POB_PRE_OPERATION_PARAMETERS Parameters;
} OB_PRE_OPERATION_INFORMATION, *POB_PRE_OPERATION_INFORMATION;

typedef struct _OB_POST_CREATE_HANDLE_INFORMATION {
    ACCESS_MASK GrantedAccess;
} OB_POST_CREATE_HANDLE_INFORMATION, *POB_POST_CREATE_HANDLE_INFORMATION;

typedef struct _OB_POST_DUPLICATE_HANDLE_INFORMATION {
    ACCESS_MASK GrantedAccess;
} OB_POST_DUPLICATE_HANDLE_INFORMATION, *POB_POST_DUPLICATE_HANDLE_INFORMATION;

typedef union _OB_POST_OPERATION_PARAMETERS {
    OB_POST_CREATE_HANDLE_INFORMATION CreateHandleInformation;
    OB_POST_DUPLICATE_HANDLE_INFORMATION DuplicateHandleInformation;
} OB_POST_OPERATION_PARAMETERS, *POB_POST_OPERATION_PARAMETERS;

typedef struct _OB_POST_OPERATION_INFORMATION {
    OB_OPERATION Operation;
    union {
        ULONG Flags;
        struct {
            ULONG KernelHandle:1;
            ULONG Reserved:31;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    PVOID Object;
    POBJECT_TYPE ObjectType;
    PVOID CallContext;
    NTSTATUS ReturnStatus;
    POB_POST_OPERATION_PARAMETERS Parameters;
} OB_POST_OPERATION_INFORMATION,*POB_POST_OPERATION_INFORMATION;

typedef enum _OB_PREOP_CALLBACK_STATUS {
    OB_PREOP_SUCCESS
} OB_PREOP_CALLBACK_STATUS, *POB_PREOP_CALLBACK_STATUS;

typedef OB_PREOP_CALLBACK_STATUS (WINAPI *POB_PRE_OPERATION_CALLBACK)(void *context, POB_PRE_OPERATION_INFORMATION information);
typedef void (WINAPI *POB_POST_OPERATION_CALLBACK)(void *context, POB_POST_OPERATION_INFORMATION information);

typedef struct _OB_OPERATION_REGISTRATION {
    POBJECT_TYPE *ObjectType;
    OB_OPERATION Operations;
    POB_PRE_OPERATION_CALLBACK PreOperation;
    POB_POST_OPERATION_CALLBACK PostOperation;
} OB_OPERATION_REGISTRATION, *POB_OPERATION_REGISTRATION;

typedef struct _OB_CALLBACK_REGISTRATION {
    USHORT Version;
    USHORT OperationRegistrationCount;
    UNICODE_STRING Altitude;
    PVOID RegistrationContext;
    OB_OPERATION_REGISTRATION *OperationRegistration;
} OB_CALLBACK_REGISTRATION, *POB_CALLBACK_REGISTRATION;

#define OB_FLT_REGISTRATION_VERSION_0100  0x0100
#define OB_FLT_REGISTRATION_VERSION OB_FLT_REGISTRATION_VERSION_0100

typedef enum _DIRECTORY_NOTIFY_INFORMATION_CLASS {
    DirectoryNotifyInformation = 1,
    DirectoryNotifyExtendedInformation
} DIRECTORY_NOTIFY_INFORMATION_CLASS, *PDIRECTORY_NOTIFY_INFORMATION_CLASS;

typedef struct _TARGET_DEVICE_CUSTOM_NOTIFICATION
{
    USHORT Version;
    USHORT Size;
    GUID Event;
    FILE_OBJECT *FileObject;
    LONG NameBufferOffset;
    UCHAR CustomDataBuffer[1];
} TARGET_DEVICE_CUSTOM_NOTIFICATION, *PTARGET_DEVICE_CUSTOM_NOTIFICATION;

typedef enum _WORK_QUEUE_TYPE {
    CriticalWorkQueue,
    DelayedWorkQueue,
    HyperCriticalWorkQueue,
    MaximumWorkQueue
} WORK_QUEUE_TYPE;

typedef void (WINAPI *PIO_WORKITEM_ROUTINE)(PDEVICE_OBJECT,void*);

NTSTATUS WINAPI ObCloseHandle(IN HANDLE handle);

#ifdef NONAMELESSUNION
# ifdef NONAMELESSSTRUCT
#  define IoGetCurrentIrpStackLocation(_Irp) ((_Irp)->Tail.Overlay.s.u2.CurrentStackLocation)
#  define IoGetNextIrpStackLocation(_Irp) ((_Irp)->Tail.Overlay.s.u2.CurrentStackLocation - 1)
   static inline void IoSkipCurrentIrpStackLocation(IRP *irp) {irp->Tail.Overlay.s.u2.CurrentStackLocation++; irp->CurrentLocation++;}
# else
#  define IoGetCurrentIrpStackLocation(_Irp) ((_Irp)->Tail.Overlay.u2.CurrentStackLocation)
#  define IoGetNextIrpStackLocation(_Irp) ((_Irp)->Tail.Overlay.u2.CurrentStackLocation - 1)
   static inline void IoSkipCurrentIrpStackLocation(IRP *irp) {irp->Tail.Overlay.u2.CurrentStackLocation++; irp->CurrentLocation++;}
# endif
#else
# ifdef NONAMELESSSTRUCT
#  define IoGetCurrentIrpStackLocation(_Irp) ((_Irp)->Tail.Overlay.s.CurrentStackLocation)
#  define IoGetNextIrpStackLocation(_Irp) ((_Irp)->Tail.Overlay.s.CurrentStackLocation - 1)
    static inline void IoSkipCurrentIrpStackLocation(IRP *irp) {irp->Tail.Overlay.s.CurrentStackLocation++; irp->CurrentLocation++;}
# else
#  define IoGetCurrentIrpStackLocation(_Irp) ((_Irp)->Tail.Overlay.CurrentStackLocation)
#  define IoGetNextIrpStackLocation(_Irp) ((_Irp)->Tail.Overlay.CurrentStackLocation - 1)
    static inline void IoSkipCurrentIrpStackLocation(IRP *irp) {irp->Tail.Overlay.CurrentStackLocation++; irp->CurrentLocation++;}
# endif
#endif

#define IoSetCancelRoutine(irp, routine) \
    ((PDRIVER_CANCEL)InterlockedExchangePointer((void **)&(irp)->CancelRoutine, routine))

static inline void IoSetCompletionRoutine(IRP *irp, PIO_COMPLETION_ROUTINE routine, void *context,
                                          BOOLEAN on_success, BOOLEAN on_error, BOOLEAN on_cancel)
{
    IO_STACK_LOCATION *irpsp = IoGetNextIrpStackLocation(irp);
    irpsp->CompletionRoutine = routine;
    irpsp->Context = context;
    irpsp->Control = 0;
    if (on_success) irpsp->Control |= SL_INVOKE_ON_SUCCESS;
    if (on_error)   irpsp->Control |= SL_INVOKE_ON_ERROR;
    if (on_cancel)  irpsp->Control |= SL_INVOKE_ON_CANCEL;
}

static inline void IoMarkIrpPending(IRP *irp)
{
    IoGetCurrentIrpStackLocation(irp)->Control |= SL_PENDING_RETURNED;
}

static inline void IoCopyCurrentIrpStackLocationToNext(IRP *irp)
{
    IO_STACK_LOCATION *current = IoGetCurrentIrpStackLocation(irp);
    IO_STACK_LOCATION *next = IoGetNextIrpStackLocation(irp);
    memcpy(next, current, FIELD_OFFSET(IO_STACK_LOCATION, CompletionRoutine));
    next->Control = 0;
}

typedef enum _MODE
{
    KernelMode,
    UserMode,
    MaximumMode
} MODE;

/* directory object access rights */
#define DIRECTORY_QUERY                 0x0001
#define DIRECTORY_TRAVERSE              0x0002
#define DIRECTORY_CREATE_OBJECT         0x0004
#define DIRECTORY_CREATE_SUBDIRECTORY   0x0008
#define DIRECTORY_ALL_ACCESS            (STANDARD_RIGHTS_REQUIRED | 0xF)

/* symbolic link access rights */
#define SYMBOLIC_LINK_QUERY             0x0001
#define SYMBOLIC_LINK_ALL_ACCESS        (STANDARD_RIGHTS_REQUIRED | 0x1)

typedef void (WINAPI *PREQUEST_POWER_COMPLETE)(DEVICE_OBJECT *device, UCHAR minor, POWER_STATE state, void *ctx, IO_STATUS_BLOCK *iosb);

#ifndef WINE_UNIX_LIB

NTSTATUS  WINAPI DbgQueryDebugFilterState(ULONG, ULONG);

void    FASTCALL ExAcquireFastMutex(FAST_MUTEX*);
void    FASTCALL ExAcquireFastMutexUnsafe(PFAST_MUTEX);
BOOLEAN   WINAPI ExAcquireResourceExclusiveLite(ERESOURCE*,BOOLEAN);
BOOLEAN   WINAPI ExAcquireResourceSharedLite(ERESOURCE*,BOOLEAN);
BOOLEAN   WINAPI ExAcquireSharedStarveExclusive(ERESOURCE*,BOOLEAN);
BOOLEAN   WINAPI ExAcquireSharedWaitForExclusive(ERESOURCE*,BOOLEAN);
void      WINAPI ExFreePool(PVOID);
PVOID     WINAPI ExAllocatePool(POOL_TYPE,SIZE_T) __WINE_ALLOC_SIZE(2) __WINE_DEALLOC(ExFreePool) __WINE_MALLOC;
PVOID     WINAPI ExAllocatePool2(POOL_FLAGS,SIZE_T,ULONG) __WINE_ALLOC_SIZE(2) __WINE_DEALLOC(ExFreePool) __WINE_MALLOC;
PVOID     WINAPI ExAllocatePoolWithQuota(POOL_TYPE,SIZE_T) __WINE_ALLOC_SIZE(2) __WINE_DEALLOC(ExFreePool) __WINE_MALLOC;
void      WINAPI ExFreePoolWithTag(PVOID,ULONG);
PVOID     WINAPI ExAllocatePoolWithTag(POOL_TYPE,SIZE_T,ULONG) __WINE_ALLOC_SIZE(2) __WINE_DEALLOC(ExFreePoolWithTag) __WINE_MALLOC;
PVOID     WINAPI ExAllocatePoolWithQuotaTag(POOL_TYPE,SIZE_T,ULONG) __WINE_ALLOC_SIZE(2) __WINE_DEALLOC(ExFreePoolWithTag) __WINE_MALLOC;
void      WINAPI ExDeleteNPagedLookasideList(PNPAGED_LOOKASIDE_LIST);
void      WINAPI ExDeletePagedLookasideList(PPAGED_LOOKASIDE_LIST);
NTSTATUS  WINAPI ExDeleteResourceLite(ERESOURCE*);
ULONG     WINAPI ExGetExclusiveWaiterCount(ERESOURCE*);
KPROCESSOR_MODE WINAPI ExGetPreviousMode(void);
ULONG     WINAPI ExGetSharedWaiterCount(ERESOURCE*);
void      WINAPI ExInitializeNPagedLookasideList(PNPAGED_LOOKASIDE_LIST,PALLOCATE_FUNCTION,PFREE_FUNCTION,ULONG,SIZE_T,ULONG,USHORT);
void      WINAPI ExInitializePagedLookasideList(PPAGED_LOOKASIDE_LIST,PALLOCATE_FUNCTION,PFREE_FUNCTION,ULONG,SIZE_T,ULONG,USHORT);
NTSTATUS  WINAPI ExInitializeResourceLite(ERESOURCE*);
PSLIST_ENTRY WINAPI ExInterlockedFlushSList(PSLIST_HEADER);
PSLIST_ENTRY WINAPI ExInterlockedPopEntrySList(PSLIST_HEADER,PKSPIN_LOCK);
PSLIST_ENTRY WINAPI ExInterlockedPushEntrySList(PSLIST_HEADER,PSLIST_ENTRY,PKSPIN_LOCK);
LIST_ENTRY * WINAPI ExInterlockedRemoveHeadList(LIST_ENTRY*,KSPIN_LOCK*);
BOOLEAN   WINAPI ExIsResourceAcquiredExclusiveLite(ERESOURCE*);
ULONG     WINAPI ExIsResourceAcquiredSharedLite(ERESOURCE*);
void *    WINAPI ExRegisterCallback(PCALLBACK_OBJECT,PCALLBACK_FUNCTION,void*);
void    FASTCALL ExReleaseFastMutex(FAST_MUTEX*);
void    FASTCALL ExReleaseFastMutexUnsafe(PFAST_MUTEX);
void      WINAPI ExReleaseResourceForThreadLite(ERESOURCE*,ERESOURCE_THREAD);
ULONG     WINAPI ExSetTimerResolution(ULONG,BOOLEAN);
void      WINAPI ExUnregisterCallback(void*);

#define PLUGPLAY_REGKEY_DEVICE            1
#define PLUGPLAY_REGKEY_DRIVER            2
#define PLUGPLAY_REGKEY_CURRENT_HWPROFILE 4

#define PLUGPLAY_PROPERTY_PERSISTENT 0x0001

void      WINAPI IoFreeErrorLogEntry(void*);
void      WINAPI IoFreeIrp(IRP*);
void      WINAPI IoFreeMdl(MDL*);
void      WINAPI IoFreeWorkItem(PIO_WORKITEM);
void      WINAPI IoAcquireCancelSpinLock(KIRQL*);
NTSTATUS  WINAPI IoAcquireRemoveLockEx(IO_REMOVE_LOCK*,void*,const char*,ULONG, ULONG);
NTSTATUS  WINAPI IoAllocateDriverObjectExtension(PDRIVER_OBJECT,PVOID,ULONG,PVOID*);
void*     WINAPI IoAllocateErrorLogEntry(void*,unsigned char) __WINE_ALLOC_SIZE(2) __WINE_DEALLOC(IoFreeErrorLogEntry) __WINE_MALLOC;
IRP*      WINAPI IoAllocateIrp(char,BOOLEAN) __WINE_DEALLOC(IoFreeIrp);
MDL*      WINAPI IoAllocateMdl(void*,ULONG,BOOLEAN,BOOLEAN,IRP*) __WINE_DEALLOC(IoFreeMdl);
PIO_WORKITEM WINAPI IoAllocateWorkItem(DEVICE_OBJECT*) __WINE_DEALLOC(IoFreeWorkItem);
void      WINAPI IoDetachDevice(PDEVICE_OBJECT);
PDEVICE_OBJECT WINAPI IoAttachDeviceToDeviceStack(PDEVICE_OBJECT,PDEVICE_OBJECT);
PIRP      WINAPI IoBuildAsynchronousFsdRequest(ULONG,DEVICE_OBJECT*,void*,ULONG,LARGE_INTEGER*,IO_STATUS_BLOCK*);
PIRP      WINAPI IoBuildDeviceIoControlRequest(ULONG,DEVICE_OBJECT*,PVOID,ULONG,PVOID,ULONG,BOOLEAN,PKEVENT,IO_STATUS_BLOCK*);
PIRP      WINAPI IoBuildSynchronousFsdRequest(ULONG,DEVICE_OBJECT*,PVOID,ULONG,PLARGE_INTEGER,PKEVENT,IO_STATUS_BLOCK*);
NTSTATUS  WINAPI IoCallDriver(DEVICE_OBJECT*,IRP*);
BOOLEAN   WINAPI IoCancelIrp(IRP*);
VOID      WINAPI IoCompleteRequest(IRP*,UCHAR);
NTSTATUS  WINAPI IoCreateDevice(DRIVER_OBJECT*,ULONG,UNICODE_STRING*,DEVICE_TYPE,ULONG,BOOLEAN,DEVICE_OBJECT**);
NTSTATUS  WINAPI IoCreateDeviceSecure(DRIVER_OBJECT*,ULONG,UNICODE_STRING*,DEVICE_TYPE,ULONG,BOOLEAN,PCUNICODE_STRING,LPCGUID,DEVICE_OBJECT**);
NTSTATUS  WINAPI IoCreateDriver(UNICODE_STRING*,PDRIVER_INITIALIZE);
NTSTATUS  WINAPI IoCreateSymbolicLink(UNICODE_STRING*,UNICODE_STRING*);
PKEVENT   WINAPI IoCreateSynchronizationEvent(UNICODE_STRING*,HANDLE*);
void      WINAPI IoDeleteDevice(DEVICE_OBJECT*);
void      WINAPI IoDeleteDriver(DRIVER_OBJECT*);
NTSTATUS  WINAPI IoDeleteSymbolicLink(UNICODE_STRING*);
DEVICE_OBJECT * WINAPI IoGetAttachedDeviceReference(DEVICE_OBJECT*);
PEPROCESS WINAPI IoGetCurrentProcess(void);
NTSTATUS  WINAPI IoGetDeviceInterfaces(const GUID*,PDEVICE_OBJECT,ULONG,PWSTR*);
NTSTATUS  WINAPI IoGetDeviceObjectPointer(UNICODE_STRING*,ACCESS_MASK,PFILE_OBJECT*,PDEVICE_OBJECT*);
NTSTATUS  WINAPI IoGetDeviceProperty(PDEVICE_OBJECT,DEVICE_REGISTRY_PROPERTY,ULONG,PVOID,PULONG);
NTSTATUS  WINAPI IoGetDevicePropertyData(PDEVICE_OBJECT,const DEVPROPKEY*,LCID,ULONG,ULONG,void*,ULONG*,DEVPROPTYPE*);
PVOID     WINAPI IoGetDriverObjectExtension(PDRIVER_OBJECT,PVOID);
PDEVICE_OBJECT WINAPI IoGetRelatedDeviceObject(PFILE_OBJECT);
void      WINAPI IoGetStackLimits(ULONG_PTR*,ULONG_PTR*);
void      WINAPI IoInitializeIrp(IRP*,USHORT,CCHAR);
VOID      WINAPI IoInitializeRemoveLockEx(PIO_REMOVE_LOCK,ULONG,ULONG,ULONG,ULONG);
void      WINAPI IoInvalidateDeviceRelations(PDEVICE_OBJECT,DEVICE_RELATION_TYPE);
#ifdef _WIN64
BOOLEAN   WINAPI IoIs32bitProcess(IRP*);
#endif
NTSTATUS  WINAPI IoOpenDeviceRegistryKey(DEVICE_OBJECT*,ULONG,ACCESS_MASK,HANDLE*);
void      WINAPI IoQueueWorkItem(PIO_WORKITEM,PIO_WORKITEM_ROUTINE,WORK_QUEUE_TYPE,void*);
NTSTATUS  WINAPI IoRegisterDeviceInterface(PDEVICE_OBJECT,const GUID*,PUNICODE_STRING,PUNICODE_STRING);
void      WINAPI IoReleaseCancelSpinLock(KIRQL);
void      WINAPI IoReleaseRemoveLockAndWaitEx(IO_REMOVE_LOCK*,void*,ULONG);
void      WINAPI IoReleaseRemoveLockEx(IO_REMOVE_LOCK*,void*,ULONG);
NTSTATUS  WINAPI IoReportTargetDeviceChange(DEVICE_OBJECT*,void*);
NTSTATUS  WINAPI IoReportTargetDeviceChangeAsynchronous(DEVICE_OBJECT*,void*,PDEVICE_CHANGE_COMPLETE_CALLBACK,void*);
void      WINAPI IoReuseIrp(IRP*,NTSTATUS);
NTSTATUS  WINAPI IoSetDeviceInterfaceState(UNICODE_STRING*,BOOLEAN);
NTSTATUS  WINAPI IoSetDevicePropertyData(DEVICE_OBJECT*,const DEVPROPKEY*,LCID,ULONG,DEVPROPTYPE,ULONG,void*);
NTSTATUS  WINAPI IoWMIRegistrationControl(PDEVICE_OBJECT,ULONG);

void    FASTCALL KeAcquireInStackQueuedSpinLockAtDpcLevel(KSPIN_LOCK*,KLOCK_QUEUE_HANDLE*);
#ifdef __i386__
void      WINAPI KeAcquireSpinLock(KSPIN_LOCK*,KIRQL*);
#else
#define KeAcquireSpinLock( lock, irql ) *(irql) = KeAcquireSpinLockRaiseToDpc( lock )
KIRQL     WINAPI KeAcquireSpinLockRaiseToDpc(KSPIN_LOCK*);
#endif
void      WINAPI KeAcquireSpinLockAtDpcLevel(KSPIN_LOCK*);
void      WINAPI DECLSPEC_NORETURN KeBugCheckEx(ULONG,ULONG_PTR,ULONG_PTR,ULONG_PTR,ULONG_PTR);
BOOLEAN   WINAPI KeCancelTimer(KTIMER*);
void      WINAPI KeClearEvent(PRKEVENT);
NTSTATUS  WINAPI KeDelayExecutionThread(KPROCESSOR_MODE,BOOLEAN,LARGE_INTEGER*);
void      WINAPI KeEnterCriticalRegion(void);
void      WINAPI KeGenericCallDpc(PKDEFERRED_ROUTINE,PVOID);
ULONG     WINAPI KeGetCurrentProcessorNumber(void);
PKTHREAD  WINAPI KeGetCurrentThread(void);
void      WINAPI KeInitializeDeviceQueue(KDEVICE_QUEUE*);
void      WINAPI KeInitializeDpc(KDPC*,PKDEFERRED_ROUTINE,void*);
void      WINAPI KeInitializeEvent(PRKEVENT,EVENT_TYPE,BOOLEAN);
void      WINAPI KeInitializeMutex(PRKMUTEX,ULONG);
void      WINAPI KeInitializeSemaphore(PRKSEMAPHORE,LONG,LONG);
void      WINAPI KeInitializeTimerEx(PKTIMER,TIMER_TYPE);
void      WINAPI KeInitializeTimer(KTIMER*);
BOOLEAN   WINAPI KeInsertDeviceQueue(KDEVICE_QUEUE*,KDEVICE_QUEUE_ENTRY*);
void      WINAPI KeLeaveCriticalRegion(void);
ULONG     WINAPI KeQueryActiveProcessorCountEx(USHORT);
KAFFINITY WINAPI KeQueryActiveProcessors(void);
void      WINAPI KeQuerySystemTime(LARGE_INTEGER*);
void      WINAPI KeQueryTickCount(LARGE_INTEGER*);
ULONG     WINAPI KeQueryTimeIncrement(void);
LONG      WINAPI KeReadStateEvent(PRKEVENT);
void    FASTCALL KeReleaseInStackQueuedSpinLockFromDpcLevel(KLOCK_QUEUE_HANDLE*);
LONG      WINAPI KeReleaseMutex(PRKMUTEX,BOOLEAN);
LONG      WINAPI KeReleaseSemaphore(PRKSEMAPHORE,KPRIORITY,LONG,BOOLEAN);
void      WINAPI KeReleaseSpinLock(KSPIN_LOCK*,KIRQL);
void      WINAPI KeReleaseSpinLockFromDpcLevel(KSPIN_LOCK*);
KDEVICE_QUEUE_ENTRY * WINAPI KeRemoveDeviceQueue(KDEVICE_QUEUE*);
LONG      WINAPI KeResetEvent(PRKEVENT);
void      WINAPI KeRevertToUserAffinityThread(void);
void      WINAPI KeRevertToUserAffinityThreadEx(KAFFINITY affinity);
LONG      WINAPI KeSetEvent(PRKEVENT,KPRIORITY,BOOLEAN);
KPRIORITY WINAPI KeSetPriorityThread(PKTHREAD,KPRIORITY);
void      WINAPI KeSetSystemAffinityThread(KAFFINITY);
KAFFINITY WINAPI KeSetSystemAffinityThreadEx(KAFFINITY affinity);
BOOLEAN   WINAPI KeSetTimer(KTIMER*,LARGE_INTEGER,KDPC*);
BOOLEAN   WINAPI KeSetTimerEx(KTIMER*,LARGE_INTEGER,LONG,KDPC*);
void      WINAPI KeSignalCallDpcDone(void*);
BOOLEAN   WINAPI KeSignalCallDpcSynchronize(void*);
NTSTATUS  WINAPI KeWaitForMultipleObjects(ULONG,void*[],WAIT_TYPE,KWAIT_REASON,KPROCESSOR_MODE,BOOLEAN,LARGE_INTEGER*,KWAIT_BLOCK*);
NTSTATUS  WINAPI KeWaitForSingleObject(void*,KWAIT_REASON,KPROCESSOR_MODE,BOOLEAN,LARGE_INTEGER*);

PVOID     WINAPI MmAllocateContiguousMemory(SIZE_T,PHYSICAL_ADDRESS) __WINE_ALLOC_SIZE(1) __WINE_MALLOC;
void      WINAPI MmFreeNonCachedMemory(PVOID,SIZE_T);
PVOID     WINAPI MmAllocateNonCachedMemory(SIZE_T) __WINE_ALLOC_SIZE(1) __WINE_DEALLOC(MmFreeNonCachedMemory) __WINE_MALLOC;
PMDL      WINAPI MmAllocatePagesForMdl(PHYSICAL_ADDRESS,PHYSICAL_ADDRESS,PHYSICAL_ADDRESS,SIZE_T);
void      WINAPI MmBuildMdlForNonPagedPool(MDL*);
NTSTATUS  WINAPI MmCopyVirtualMemory(PEPROCESS,void*,PEPROCESS,void*,SIZE_T,KPROCESSOR_MODE,SIZE_T*);
void *    WINAPI MmGetSystemRoutineAddress(UNICODE_STRING*);
PVOID     WINAPI MmMapLockedPages(MDL*,KPROCESSOR_MODE);
PVOID     WINAPI MmMapLockedPagesSpecifyCache(PMDLX,KPROCESSOR_MODE,MEMORY_CACHING_TYPE,PVOID,ULONG,MM_PAGE_PRIORITY);
MM_SYSTEMSIZE WINAPI MmQuerySystemSize(void);
void      WINAPI MmProbeAndLockPages(PMDLX, KPROCESSOR_MODE, LOCK_OPERATION);
void      WINAPI MmUnmapLockedPages(void*, PMDL);
void      WINAPI MmUnlockPages(PMDLX);

void    FASTCALL ObfReferenceObject(void*);
void      WINAPI ObDereferenceObject(void*);
USHORT    WINAPI ObGetFilterVersion(void);
NTSTATUS  WINAPI ObRegisterCallbacks(POB_CALLBACK_REGISTRATION, void**);
NTSTATUS  WINAPI ObReferenceObjectByHandle(HANDLE,ACCESS_MASK,POBJECT_TYPE,KPROCESSOR_MODE,PVOID*,POBJECT_HANDLE_INFORMATION);
NTSTATUS  WINAPI ObReferenceObjectByName(UNICODE_STRING*,ULONG,ACCESS_STATE*,ACCESS_MASK,POBJECT_TYPE,KPROCESSOR_MODE,void*,void**);
NTSTATUS  WINAPI ObReferenceObjectByPointer(void*,ACCESS_MASK,POBJECT_TYPE,KPROCESSOR_MODE);
void      WINAPI ObUnRegisterCallbacks(void*);

NTSTATUS  WINAPI PoCallDriver(DEVICE_OBJECT*,IRP*);
NTSTATUS  WINAPI PoRequestPowerIrp(DEVICE_OBJECT *device, UCHAR minor, POWER_STATE state, PREQUEST_POWER_COMPLETE complete_cb, void *ctx, IRP **irp);
POWER_STATE WINAPI PoSetPowerState(PDEVICE_OBJECT,POWER_STATE_TYPE,POWER_STATE);
void      WINAPI PoStartNextPowerIrp(IRP*);

NTSTATUS  WINAPI PsCreateSystemThread(PHANDLE,ULONG,POBJECT_ATTRIBUTES,HANDLE,PCLIENT_ID,PKSTART_ROUTINE,PVOID);
#define          PsGetCurrentProcess() IoGetCurrentProcess()
#define          PsGetCurrentThread() ((PETHREAD)KeGetCurrentThread())
HANDLE    WINAPI PsGetCurrentProcessId(void);
HANDLE    WINAPI PsGetCurrentThreadId(void);
HANDLE    WINAPI PsGetProcessInheritedFromUniqueProcessId(PEPROCESS);
BOOLEAN   WINAPI PsGetVersion(ULONG*,ULONG*,ULONG*,UNICODE_STRING*);
NTSTATUS  WINAPI PsTerminateSystemThread(NTSTATUS);

#ifdef __x86_64__
NTSYSAPI void WINAPI RtlCopyMemoryNonTemporal(void*,const void*,SIZE_T);
#else
#define RtlCopyMemoryNonTemporal RtlCopyMemory
#endif
BOOLEAN   WINAPI RtlIsNtDdiVersionAvailable(ULONG);

NTSTATUS  WINAPI ZwAddBootEntry(PUNICODE_STRING,PUNICODE_STRING);
NTSTATUS  WINAPI ZwAccessCheckAndAuditAlarm(PUNICODE_STRING,HANDLE,PUNICODE_STRING,PUNICODE_STRING,PSECURITY_DESCRIPTOR,ACCESS_MASK,PGENERIC_MAPPING,BOOLEAN,PACCESS_MASK,PBOOLEAN,PBOOLEAN);
NTSTATUS  WINAPI ZwAdjustPrivilegesToken(HANDLE,BOOLEAN,PTOKEN_PRIVILEGES,DWORD,PTOKEN_PRIVILEGES,PDWORD);
NTSTATUS  WINAPI ZwAlertThread(HANDLE ThreadHandle);
NTSTATUS  WINAPI ZwAllocateVirtualMemory(HANDLE,PVOID*,ULONG,SIZE_T*,ULONG,ULONG);
NTSTATUS  WINAPI ZwCancelIoFile(HANDLE,PIO_STATUS_BLOCK);
NTSTATUS  WINAPI ZwCancelTimer(HANDLE, BOOLEAN*);
NTSTATUS  WINAPI ZwClearEvent(HANDLE);
NTSTATUS  WINAPI ZwClose(HANDLE);
NTSTATUS  WINAPI ZwCloseObjectAuditAlarm(PUNICODE_STRING,HANDLE,BOOLEAN);
NTSTATUS  WINAPI ZwConnectPort(PHANDLE,PUNICODE_STRING,PSECURITY_QUALITY_OF_SERVICE,PLPC_SECTION_WRITE,PLPC_SECTION_READ,PULONG,PVOID,PULONG);
NTSTATUS  WINAPI ZwCreateDirectoryObject(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSTATUS  WINAPI ZwCreateEvent(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES *,BOOLEAN,BOOLEAN);
NTSTATUS  WINAPI ZwCreateFile(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK,PLARGE_INTEGER,ULONG,ULONG,ULONG,ULONG,PVOID,ULONG);
NTSTATUS  WINAPI ZwCreateKey(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES*,ULONG,const UNICODE_STRING*,ULONG,PULONG);
NTSTATUS  WINAPI ZwCreateSection(HANDLE*,ACCESS_MASK,const OBJECT_ATTRIBUTES*,const LARGE_INTEGER*,ULONG,ULONG,HANDLE);
NTSTATUS  WINAPI ZwCreateSymbolicLinkObject(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PUNICODE_STRING);
NTSTATUS  WINAPI ZwCreateTimer(HANDLE*, ACCESS_MASK, const OBJECT_ATTRIBUTES*, TIMER_TYPE);
NTSTATUS  WINAPI ZwDeleteAtom(RTL_ATOM);
NTSTATUS  WINAPI ZwDeleteFile(POBJECT_ATTRIBUTES);
NTSTATUS  WINAPI ZwDeleteKey(HANDLE);
NTSTATUS  WINAPI ZwDeleteValueKey(HANDLE,const UNICODE_STRING *);
NTSTATUS  WINAPI ZwDeviceIoControlFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,ULONG,PVOID,ULONG,PVOID,ULONG);
NTSTATUS  WINAPI ZwDisplayString(PUNICODE_STRING);
NTSTATUS  WINAPI ZwDuplicateObject(HANDLE,HANDLE,HANDLE,PHANDLE,ACCESS_MASK,ULONG,ULONG);
NTSTATUS  WINAPI ZwDuplicateToken(HANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,SECURITY_IMPERSONATION_LEVEL,TOKEN_TYPE,PHANDLE);
NTSTATUS  WINAPI ZwEnumerateKey(HANDLE,ULONG,KEY_INFORMATION_CLASS,void *,DWORD,DWORD *);
NTSTATUS  WINAPI ZwEnumerateValueKey(HANDLE,ULONG,KEY_VALUE_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS  WINAPI ZwFlushInstructionCache(HANDLE,LPCVOID,SIZE_T);
NTSTATUS  WINAPI ZwFlushKey(HANDLE);
NTSTATUS  WINAPI ZwFlushVirtualMemory(HANDLE,LPCVOID*,SIZE_T*,ULONG);
NTSTATUS  WINAPI ZwFreeVirtualMemory(HANDLE,PVOID*,SIZE_T*,ULONG);
NTSTATUS  WINAPI ZwFsControlFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,ULONG,PVOID,ULONG,PVOID,ULONG);
NTSTATUS  WINAPI ZwInitiatePowerAction(POWER_ACTION,SYSTEM_POWER_STATE,ULONG,BOOLEAN);
NTSTATUS  WINAPI ZwLoadDriver(const UNICODE_STRING *);
NTSTATUS  WINAPI ZwLoadKey(const OBJECT_ATTRIBUTES *,OBJECT_ATTRIBUTES *);
NTSTATUS  WINAPI ZwLockVirtualMemory(HANDLE,PVOID*,SIZE_T*,ULONG);
NTSTATUS  WINAPI ZwMakeTemporaryObject(HANDLE);
NTSTATUS  WINAPI ZwMapViewOfSection(HANDLE,HANDLE,PVOID*,ULONG,SIZE_T,const LARGE_INTEGER*,SIZE_T*,SECTION_INHERIT,ULONG,ULONG);
NTSTATUS  WINAPI ZwNotifyChangeKey(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,ULONG,BOOLEAN,PVOID,ULONG,BOOLEAN);
NTSTATUS  WINAPI ZwOpenDirectoryObject(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSTATUS  WINAPI ZwOpenEvent(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES *);
NTSTATUS  WINAPI ZwOpenFile(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK,ULONG,ULONG);
NTSTATUS  WINAPI ZwOpenKey(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES *);
NTSTATUS  WINAPI ZwOpenProcess(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES*,const CLIENT_ID*);
NTSTATUS  WINAPI ZwOpenProcessToken(HANDLE,DWORD,HANDLE *);
NTSTATUS  WINAPI ZwOpenSection(HANDLE*,ACCESS_MASK,const OBJECT_ATTRIBUTES*);
NTSTATUS  WINAPI ZwOpenSymbolicLinkObject(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSTATUS  WINAPI ZwOpenThread(HANDLE*,ACCESS_MASK,const OBJECT_ATTRIBUTES*,const CLIENT_ID*);
NTSTATUS  WINAPI ZwOpenThreadToken(HANDLE,DWORD,BOOLEAN,HANDLE *);
NTSTATUS  WINAPI ZwOpenTimer(HANDLE*, ACCESS_MASK, const OBJECT_ATTRIBUTES*);
NTSTATUS  WINAPI ZwPowerInformation(POWER_INFORMATION_LEVEL,PVOID,ULONG,PVOID,ULONG);
NTSTATUS  WINAPI ZwPulseEvent(HANDLE,PULONG);
NTSTATUS  WINAPI ZwQueryDefaultLocale(BOOLEAN,LCID*);
NTSTATUS  WINAPI ZwQueryDefaultUILanguage(LANGID*);
NTSTATUS  WINAPI ZwQueryDirectoryFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,PVOID,ULONG,FILE_INFORMATION_CLASS,BOOLEAN,PUNICODE_STRING,BOOLEAN);
NTSTATUS  WINAPI ZwQueryDirectoryObject(HANDLE,PDIRECTORY_BASIC_INFORMATION,ULONG,BOOLEAN,BOOLEAN,PULONG,PULONG);
NTSTATUS  WINAPI ZwQueryEaFile(HANDLE,PIO_STATUS_BLOCK,PVOID,ULONG,BOOLEAN,PVOID,ULONG,PVOID,BOOLEAN);
NTSTATUS  WINAPI ZwQueryFullAttributesFile(const OBJECT_ATTRIBUTES*,FILE_NETWORK_OPEN_INFORMATION*);
NTSTATUS  WINAPI ZwQueryInformationFile(HANDLE,PIO_STATUS_BLOCK,PVOID,LONG,FILE_INFORMATION_CLASS);
NTSTATUS  WINAPI ZwQueryInformationThread(HANDLE,THREADINFOCLASS,PVOID,ULONG,PULONG);
NTSTATUS  WINAPI ZwQueryInformationToken(HANDLE,DWORD,PVOID,DWORD,LPDWORD);
NTSTATUS  WINAPI ZwQueryInstallUILanguage(LANGID*);
NTSTATUS  WINAPI ZwQueryKey(HANDLE,KEY_INFORMATION_CLASS,void *,DWORD,DWORD *);
NTSTATUS  WINAPI ZwQueryObject(HANDLE, OBJECT_INFORMATION_CLASS, PVOID, ULONG, PULONG);
NTSTATUS  WINAPI ZwQuerySecurityObject(HANDLE,SECURITY_INFORMATION,PSECURITY_DESCRIPTOR,ULONG,PULONG);
NTSTATUS  WINAPI ZwQuerySection(HANDLE,SECTION_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS  WINAPI ZwQuerySymbolicLinkObject(HANDLE,PUNICODE_STRING,PULONG);
NTSTATUS  WINAPI ZwQuerySystemInformation(SYSTEM_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS  WINAPI ZwQueryTimerResolution(PULONG,PULONG,PULONG);
NTSTATUS  WINAPI ZwQueryValueKey(HANDLE,const UNICODE_STRING *,KEY_VALUE_INFORMATION_CLASS,void *,DWORD,DWORD *);
NTSTATUS  WINAPI ZwQueryVolumeInformationFile(HANDLE,PIO_STATUS_BLOCK,PVOID,ULONG,FS_INFORMATION_CLASS);
NTSTATUS  WINAPI ZwReadFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,PVOID,ULONG,PLARGE_INTEGER,PULONG);
NTSTATUS  WINAPI ZwReplaceKey(POBJECT_ATTRIBUTES,HANDLE,POBJECT_ATTRIBUTES);
NTSTATUS  WINAPI ZwRequestWaitReplyPort(HANDLE,PLPC_MESSAGE,PLPC_MESSAGE);
NTSTATUS  WINAPI ZwResetEvent(HANDLE,PULONG);
NTSTATUS  WINAPI ZwRestoreKey(HANDLE,HANDLE,ULONG);
NTSTATUS  WINAPI ZwSaveKey(HANDLE,HANDLE);
NTSTATUS  WINAPI ZwSecureConnectPort(PHANDLE,PUNICODE_STRING,PSECURITY_QUALITY_OF_SERVICE,PLPC_SECTION_WRITE,PSID,PLPC_SECTION_READ,PULONG,PVOID,PULONG);
NTSTATUS  WINAPI ZwSetDefaultLocale(BOOLEAN,LCID);
NTSTATUS  WINAPI ZwSetDefaultUILanguage(LANGID);
NTSTATUS  WINAPI ZwSetEaFile(HANDLE,PIO_STATUS_BLOCK,PVOID,ULONG);
NTSTATUS  WINAPI ZwSetEvent(HANDLE,PULONG);
NTSTATUS  WINAPI ZwSetInformationFile(HANDLE,PIO_STATUS_BLOCK,PVOID,ULONG,FILE_INFORMATION_CLASS);
NTSTATUS  WINAPI ZwSetInformationKey(HANDLE,const int,PVOID,ULONG);
NTSTATUS  WINAPI ZwSetInformationObject(HANDLE, OBJECT_INFORMATION_CLASS, PVOID, ULONG);
NTSTATUS  WINAPI ZwSetInformationProcess(HANDLE,PROCESSINFOCLASS,PVOID,ULONG);
NTSTATUS  WINAPI ZwSetInformationThread(HANDLE,THREADINFOCLASS,LPCVOID,ULONG);
NTSTATUS  WINAPI ZwSetIoCompletion(HANDLE,ULONG,ULONG,NTSTATUS,ULONG);
NTSTATUS  WINAPI ZwSetLdtEntries(ULONG,ULONG,ULONG,ULONG,ULONG,ULONG);
NTSTATUS  WINAPI ZwSetSecurityObject(HANDLE,SECURITY_INFORMATION,PSECURITY_DESCRIPTOR);
NTSTATUS  WINAPI ZwSetSystemInformation(SYSTEM_INFORMATION_CLASS,PVOID,ULONG);
NTSTATUS  WINAPI ZwSetSystemTime(const LARGE_INTEGER*,LARGE_INTEGER*);
NTSTATUS  WINAPI ZwSetTimer(HANDLE, const LARGE_INTEGER*, PTIMER_APC_ROUTINE, PVOID, BOOLEAN, ULONG, BOOLEAN*);
NTSTATUS  WINAPI ZwSetValueKey(HANDLE,const UNICODE_STRING *,ULONG,ULONG,const void *,ULONG);
NTSTATUS  WINAPI ZwSetVolumeInformationFile(HANDLE,PIO_STATUS_BLOCK,PVOID,ULONG,FS_INFORMATION_CLASS);
NTSTATUS  WINAPI ZwSuspendThread(HANDLE,PULONG);
NTSTATUS  WINAPI ZwTerminateProcess(HANDLE,LONG);
NTSTATUS  WINAPI ZwUnloadDriver(const UNICODE_STRING *);
NTSTATUS  WINAPI ZwUnloadKey(HANDLE);
NTSTATUS  WINAPI ZwUnmapViewOfSection(HANDLE,PVOID);
NTSTATUS  WINAPI ZwWaitForSingleObject(HANDLE,BOOLEAN,const LARGE_INTEGER*);
NTSTATUS  WINAPI ZwWaitForMultipleObjects(ULONG,const HANDLE*,BOOLEAN,BOOLEAN,const LARGE_INTEGER*);
NTSTATUS  WINAPI ZwWriteFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,const void*,ULONG,PLARGE_INTEGER,PULONG);
NTSTATUS  WINAPI ZwYieldExecution(void);

static inline void ExInitializeFastMutex( FAST_MUTEX *mutex )
{
    mutex->Count = FM_LOCK_BIT;
    mutex->Owner = NULL;
    mutex->Contention = 0;
    KeInitializeEvent( &mutex->Event, SynchronizationEvent, FALSE );
}

static FORCEINLINE void WINAPI KeInitializeSpinLock( KSPIN_LOCK *lock )
{
  *lock = 0;
}

static inline void *MmGetSystemAddressForMdlSafe(MDL *mdl, ULONG priority)
{
    if (mdl->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA | MDL_SOURCE_IS_NONPAGED_POOL))
        return mdl->MappedSystemVa;
    else
        return MmMapLockedPagesSpecifyCache(mdl, KernelMode, MmCached, NULL, FALSE, priority);
}

#endif /* WINE_UNIX_LIB */

#endif
