/*
 * Copyright 2008 Francois Gouget for CodeWeavers
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

#ifndef _NTDDK_
#define _NTDDK_

/* Note: We will probably have to duplicate everything ultimately :-( */
#include <ddk/wdm.h>

#include <excpt.h>
/* FIXME: #include <ntdef.h> */
#include <ntstatus.h>
/* FIXME: #include <bugcodes.h> */
/* FIXME: #include <ntiologc.h> */


typedef enum _BUS_DATA_TYPE
{
    ConfigurationSpaceUndefined = -1,
    Cmos,
    EisaConfiguration,
    Pos,
    CbusConfiguration,
    PCIConfiguration,
    VMEConfiguration,
    NuBusConfiguration,
    PCMCIAConfiguration,
    MPIConfiguration,
    MPSAConfiguration,
    PNPISAConfiguration,
    MaximumBusDataType
} BUS_DATA_TYPE, *PBUS_DATA_TYPE;

typedef struct _CONFIGURATION_INFORMATION
{
    ULONG DiskCount;
    ULONG FloppyCount;
    ULONG CdRomCount;
    ULONG TapeCount;
    ULONG ScsiPortCount;
    ULONG SerialCount;
    ULONG ParallelCount;
    BOOLEAN AtDiskPrimaryAddressClaimed;
    BOOLEAN AtDiskSecondaryAddressClaimed;
    ULONG Version;
    ULONG MediumChangerCount;
} CONFIGURATION_INFORMATION, *PCONFIGURATION_INFORMATION;

typedef enum _CONFIGURATION_TYPE
{
    ArcSystem = 0,
    CentralProcessor,
    FloatingPointProcessor,
    PrimaryIcache,
    PrimaryDcache,
    SecondaryIcache,
    SecondaryDcache,
    SecondaryCache,
    EisaAdapter,
    TcAdapter,
    ScsiAdapter,
    DtiAdapter,
    MultiFunctionAdapter,
    DiskController,
    TapeController,
    CdromController,
    WormController,
    SerialController,
    NetworkController,
    DisplayController,
    ParallelController,
    PointerController,
    KeyboardController,
    AudioController,
    OtherController,
    DiskPeripheral,
    FloppyDiskPeripheral,
    TapePeripheral,
    ModemPeripheral,
    MonitorPeripheral,
    PrinterPeripheral,
    PointerPeripheral,
    KeyboardPeripheral,
    TerminalPeripheral,
    OtherPeripheral,
    LinePeripheral,
    NetworkPeripheral,
    SystemMemory,
    DockingInformation,
    RealModeIrqRoutingTable,
    RealModePCIEnumeration,
    MaximumType
} CONFIGURATION_TYPE, *PCONFIGURATION_TYPE;

#define IMAGE_ADDRESSING_MODE_32BIT 3

typedef struct _IMAGE_INFO
{
    union
    {
        ULONG Properties;
        struct
        {
            ULONG ImageAddressingMode  : 8;
            ULONG SystemModeImage      : 1;
            ULONG ImageMappedToAllPids : 1;
            ULONG ExtendedInfoPresent  : 1;
            ULONG MachineTypeMismatch : 1;
            ULONG ImageSignatureLevel : 4;
            ULONG ImageSignatureType : 3;
            ULONG ImagePartialMap : 1;
            ULONG Reserved : 12;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    PVOID  ImageBase;
    ULONG  ImageSelector;
    SIZE_T ImageSize;
    ULONG  ImageSectionNumber;
} IMAGE_INFO, *PIMAGE_INFO;

typedef enum _IO_NOTIFICATION_EVENT_CATEGORY
{
    EventCategoryReserved,
    EventCategoryHardwareProfileChange,
    EventCategoryDeviceInterfaceChange,
    EventCategoryTargetDeviceChange
} IO_NOTIFICATION_EVENT_CATEGORY;

typedef struct _FILE_VALID_DATA_LENGTH_INFORMATION
{
  LARGE_INTEGER ValidDataLength;
} FILE_VALID_DATA_LENGTH_INFORMATION, *PFILE_VALID_DATA_LENGTH_INFORMATION;

typedef enum _RTL_GENERIC_COMPARE_RESULTS
{
    GenericLessThan,
    GenericGreaterThan,
    GenericEqual
} RTL_GENERIC_COMPARE_RESULTS;

typedef struct _RTL_SPLAY_LINKS
{
    struct _RTL_SPLAY_LINKS *Parent;
    struct _RTL_SPLAY_LINKS *LeftChild;
    struct _RTL_SPLAY_LINKS *RightChild;
} RTL_SPLAY_LINKS, *PRTL_SPLAY_LINKS;

struct _RTL_GENERIC_TABLE;

typedef RTL_GENERIC_COMPARE_RESULTS (WINAPI *PRTL_GENERIC_COMPARE_ROUTINE)(struct _RTL_GENERIC_TABLE *, void *, void *);
typedef void * (__WINE_ALLOC_SIZE(2) WINAPI *PRTL_GENERIC_ALLOCATE_ROUTINE)(struct _RTL_GENERIC_TABLE *, LONG);
typedef void (WINAPI *PRTL_GENERIC_FREE_ROUTINE)(struct _RTL_GENERIC_TABLE *Table, void *);

typedef struct _RTL_GENERIC_TABLE
{
    PRTL_SPLAY_LINKS TableRoot;
    LIST_ENTRY InsertOrderList;
    LIST_ENTRY *OrderedPointer;
    ULONG WhichOrderedElement;
    ULONG NumberGenericTableElements;
    PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine;
    PRTL_GENERIC_ALLOCATE_ROUTINE AllocateRoutine;
    PRTL_GENERIC_FREE_ROUTINE FreeRoutine;
    void *TableContext;
} RTL_GENERIC_TABLE;
typedef RTL_GENERIC_TABLE *PRTL_GENERIC_TABLE;

typedef struct _RTL_BALANCED_LINKS {
    struct _RTL_BALANCED_LINKS *Parent;
    struct _RTL_BALANCED_LINKS *LeftChild;
    struct _RTL_BALANCED_LINKS *RightChild;
    CHAR Balance;
    UCHAR Reserved[3];
} RTL_BALANCED_LINKS;
typedef RTL_BALANCED_LINKS *PRTL_BALANCED_LINKS;

struct _RTL_AVL_TABLE;

typedef RTL_GENERIC_COMPARE_RESULTS (WINAPI *PRTL_AVL_COMPARE_ROUTINE)(struct _RTL_AVL_TABLE *, void *, void *);

typedef void * (__WINE_ALLOC_SIZE(2) WINAPI *PRTL_AVL_ALLOCATE_ROUTINE)(struct _RTL_AVL_TABLE *, LONG);

typedef void (WINAPI *PRTL_AVL_FREE_ROUTINE )(struct _RTL_AVL_TABLE *, void *buffer);

typedef struct _RTL_AVL_TABLE {
    RTL_BALANCED_LINKS BalancedRoot;
    void *OrderedPointer;
    ULONG WhichOrderedElement;
    ULONG NumberGenericTableElements;
    ULONG DepthOfTree;
    PRTL_BALANCED_LINKS RestartKey;
    ULONG DeleteCount;
    PRTL_AVL_COMPARE_ROUTINE CompareRoutine;
    PRTL_AVL_ALLOCATE_ROUTINE AllocateRoutine;
    PRTL_AVL_FREE_ROUTINE FreeRoutine;
    void *TableContext;
} RTL_AVL_TABLE, *PRTL_AVL_TABLE;

typedef struct _PS_CREATE_NOTIFY_INFO {
    SIZE_T Size;
    union {
        ULONG Flags;
        struct {
            ULONG FileOpenNameAvailable :1;
            ULONG IsSubsystemProcess :1;
            ULONG Reserved :30;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    HANDLE               ParentProcessId;
    CLIENT_ID            CreatingThreadId;
    struct _FILE_OBJECT *FileObject;
    PCUNICODE_STRING     ImageFileName;
    PCUNICODE_STRING     CommandLine;
    NTSTATUS             CreationStatus;
} PS_CREATE_NOTIFY_INFO, *PPS_CREATE_NOTIFY_INFO;

typedef void (WINAPI *PCREATE_PROCESS_NOTIFY_ROUTINE)(HANDLE,HANDLE,BOOLEAN);
typedef void (WINAPI *PCREATE_PROCESS_NOTIFY_ROUTINE_EX)(PEPROCESS,HANDLE,PS_CREATE_NOTIFY_INFO*);
typedef void (WINAPI *PCREATE_THREAD_NOTIFY_ROUTINE)(HANDLE,HANDLE,BOOLEAN);
typedef VOID (WINAPI *PDRIVER_NOTIFICATION_CALLBACK_ROUTINE)(PVOID,PVOID);
typedef VOID (WINAPI *PDRIVER_REINITIALIZE)(PDRIVER_OBJECT,PVOID,ULONG);
typedef VOID (WINAPI *PLOAD_IMAGE_NOTIFY_ROUTINE)(PUNICODE_STRING,HANDLE,PIMAGE_INFO);
typedef NTSTATUS (WINAPI *PIO_QUERY_DEVICE_ROUTINE)(PVOID,PUNICODE_STRING,INTERFACE_TYPE,ULONG,
            PKEY_VALUE_FULL_INFORMATION*,CONFIGURATION_TYPE,ULONG,PKEY_VALUE_FULL_INFORMATION*);
typedef void (NTAPI EXPAND_STACK_CALLOUT)(void*);
typedef EXPAND_STACK_CALLOUT *PEXPAND_STACK_CALLOUT;

#ifndef UUID_DEFINED
#define UUID_DEFINED
typedef GUID UUID;
#endif

NTSTATUS  WINAPI ExUuidCreate(UUID*);
NTSTATUS  WINAPI IoQueryDeviceDescription(PINTERFACE_TYPE,PULONG,PCONFIGURATION_TYPE,PULONG,
                                  PCONFIGURATION_TYPE,PULONG,PIO_QUERY_DEVICE_ROUTINE,PVOID);
void      WINAPI IoRegisterBootDriverReinitialization(DRIVER_OBJECT*,PDRIVER_REINITIALIZE,void*);
void      WINAPI IoRegisterDriverReinitialization(PDRIVER_OBJECT,PDRIVER_REINITIALIZE,PVOID);
NTSTATUS  WINAPI IoRegisterShutdownNotification(PDEVICE_OBJECT);
BOOLEAN   WINAPI KeAreApcsDisabled(void);
void      WINAPI DECLSPEC_NORETURN KeBugCheck(ULONG);
NTSTATUS  WINAPI KeExpandKernelStackAndCallout(PEXPAND_STACK_CALLOUT,void*,SIZE_T);
void      WINAPI KeSetTargetProcessorDpc(PRKDPC,CCHAR);
BOOLEAN   WINAPI MmIsAddressValid(void *);
HANDLE    WINAPI PsGetProcessId(PEPROCESS);
void *    WINAPI PsGetProcessSectionBaseAddress(PEPROCESS);
HANDLE    WINAPI PsGetThreadId(PETHREAD);
HANDLE    WINAPI PsGetThreadProcessId(PETHREAD);
NTSTATUS  WINAPI PsRemoveLoadImageNotifyRoutine(PLOAD_IMAGE_NOTIFY_ROUTINE);
NTSTATUS  WINAPI PsSetCreateProcessNotifyRoutine(PCREATE_PROCESS_NOTIFY_ROUTINE,BOOLEAN);
NTSTATUS  WINAPI PsSetCreateProcessNotifyRoutineEx(PCREATE_PROCESS_NOTIFY_ROUTINE_EX,BOOLEAN);
NTSTATUS  WINAPI PsSetCreateThreadNotifyRoutine(PCREATE_THREAD_NOTIFY_ROUTINE);
NTSTATUS  WINAPI PsSetLoadImageNotifyRoutine(PLOAD_IMAGE_NOTIFY_ROUTINE);
NTSTATUS  WINAPI PsSetLoadImageNotifyRoutineEx(PLOAD_IMAGE_NOTIFY_ROUTINE,ULONG_PTR);
LONG      WINAPI RtlCompareString(const STRING*,const STRING*,BOOLEAN);
void      WINAPI RtlCopyString(STRING*,const STRING*);
BOOLEAN   WINAPI RtlEqualString(const STRING*,const STRING*,BOOLEAN);
void *    WINAPI RtlGetElementGenericTable(PRTL_GENERIC_TABLE,ULONG);
void      WINAPI RtlInitializeGenericTable(PRTL_GENERIC_TABLE,PRTL_GENERIC_COMPARE_ROUTINE,PRTL_GENERIC_ALLOCATE_ROUTINE,PRTL_GENERIC_FREE_ROUTINE,void *);
void      WINAPI RtlInitializeGenericTableAvl(PRTL_AVL_TABLE,PRTL_AVL_COMPARE_ROUTINE,PRTL_AVL_ALLOCATE_ROUTINE, PRTL_AVL_FREE_ROUTINE,void *);
void      WINAPI RtlInsertElementGenericTableAvl(PRTL_AVL_TABLE,void *,ULONG,BOOL*);
void *    WINAPI RtlLookupElementGenericTable(PRTL_GENERIC_TABLE,void *);
void      WINAPI RtlMapGenericMask(ACCESS_MASK*,const GENERIC_MAPPING*);
ULONG     WINAPI RtlNumberGenericTableElements(PRTL_GENERIC_TABLE);
BOOLEAN   WINAPI RtlPrefixUnicodeString(const UNICODE_STRING*,const UNICODE_STRING*,BOOLEAN);
NTSTATUS  WINAPI RtlUpcaseUnicodeString(UNICODE_STRING*,const UNICODE_STRING*,BOOLEAN);
char      WINAPI RtlUpperChar(char);
void      WINAPI RtlUpperString(STRING*,const STRING*);

#ifndef _WIN64
ULONGLONG WINAPI RtlLargeIntegerDivide(ULONGLONG,ULONGLONG,ULONGLONG*);
#endif

#endif
