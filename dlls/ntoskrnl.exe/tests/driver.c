/*
 * ntoskrnl.exe testing framework
 *
 * Copyright 2015 Sebastian Lackner
 * Copyright 2015 Michael Müller
 * Copyright 2015 Christian Costa
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

#if 0
#pragma makedep testdll
#endif

#include <stdarg.h>
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"
#include "ddk/ntddk.h"
#include "ddk/ntifs.h"
#include "ddk/wdm.h"
#include "ddk/fltkernel.h"

#include "driver.h"

#include "utils.h"

/* memcmp() isn't exported from ntoskrnl on i386 */
static int kmemcmp( const void *ptr1, const void *ptr2, size_t n )
{
    const unsigned char *p1, *p2;

    for (p1 = ptr1, p2 = ptr2; n; n--, p1++, p2++)
    {
        if (*p1 < *p2) return -1;
        if (*p1 > *p2) return 1;
    }
    return 0;
}

static DRIVER_OBJECT *driver_obj;
static DEVICE_OBJECT *lower_device, *upper_device;

static POBJECT_TYPE *pExEventObjectType, *pIoFileObjectType, *pPsThreadType, *pIoDriverObjectType;
static PEPROCESS *pPsInitialSystemProcess;
static void *create_caller_thread;

static PETHREAD create_irp_thread;

NTSTATUS WINAPI ZwQueryInformationProcess(HANDLE,PROCESSINFOCLASS,void*,ULONG,ULONG*);

struct file_context
{
    DWORD id;
    ULONG namelen;
    WCHAR name[10];
};

static void *get_proc_address(const char *name)
{
    UNICODE_STRING name_u;
    ANSI_STRING name_a;
    NTSTATUS status;
    void *ret;

    RtlInitAnsiString(&name_a, name);
    status = RtlAnsiStringToUnicodeString(&name_u, &name_a, TRUE);
    ok (!status, "RtlAnsiStringToUnicodeString failed: %#lx\n", status);
    if (status) return NULL;

    ret = MmGetSystemRoutineAddress(&name_u);
    RtlFreeUnicodeString(&name_u);
    return ret;
}

static FILE_OBJECT *last_created_file;
static unsigned int create_count, close_count;

static NTSTATUS WINAPI test_irp_struct_completion_routine(DEVICE_OBJECT *reserved, IRP *irp, void *context)
{
    unsigned int *result = context;

    *result = 1;
    return STATUS_MORE_PROCESSING_REQUIRED;
}

static void test_irp_struct(IRP *irp, DEVICE_OBJECT *device)
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    unsigned int irp_completion_result;

    ok(device == upper_device, "Expected device %p, got %p.\n", upper_device, device);
    ok(last_created_file != NULL, "last_created_file = NULL\n");
    ok(irpsp->FileObject == last_created_file, "FileObject != last_created_file\n");
    ok(irpsp->DeviceObject == upper_device, "unexpected DeviceObject\n");
    ok(irpsp->FileObject->DeviceObject == lower_device, "unexpected FileObject->DeviceObject\n");
    ok(!irp->UserEvent, "UserEvent = %p\n", irp->UserEvent);
    ok(irp->Tail.Overlay.Thread == (PETHREAD)KeGetCurrentThread(),
       "IRP thread is not the current thread\n");

    ok(IoGetRequestorProcess(irp) == IoGetCurrentProcess(), "processes didn't match\n");

    irp = IoAllocateIrp(1, FALSE);
    ok(irp->AllocationFlags == IRP_ALLOCATED_FIXED_SIZE, "Got unexpected irp->AllocationFlags %#x.\n",
            irp->AllocationFlags);
    ok(irp->CurrentLocation == 2,
            "Got unexpected irp->CurrentLocation %u.\n", irp->CurrentLocation);
    IoSetCompletionRoutine(irp, test_irp_struct_completion_routine, &irp_completion_result,
            TRUE, TRUE, TRUE);

    irp_completion_result = 0;

    irp->IoStatus.Status = STATUS_SUCCESS;
    --irp->CurrentLocation;
    --irp->Tail.Overlay.CurrentStackLocation;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    ok(irp->CurrentLocation == 2,
            "Got unexpected irp->CurrentLocation %u.\n", irp->CurrentLocation);
    ok(irp_completion_result, "IRP completion was not called.\n");

    --irp->CurrentLocation;
    --irp->Tail.Overlay.CurrentStackLocation;
    IoReuseIrp(irp, STATUS_UNSUCCESSFUL);
    ok(irp->CurrentLocation == 2,
            "Got unexpected irp->CurrentLocation %u.\n", irp->CurrentLocation);
    ok(irp->AllocationFlags == IRP_ALLOCATED_FIXED_SIZE, "Got unexpected irp->AllocationFlags %#x.\n",
            irp->AllocationFlags);

    IoFreeIrp(irp);
}

static int cancel_queue_cnt;

static void WINAPI cancel_queued_irp(DEVICE_OBJECT *device, IRP *irp)
{
    IoReleaseCancelSpinLock(irp->CancelIrql);
    ok(irp->Cancel == TRUE, "Cancel = %x\n", irp->Cancel);
    ok(!irp->CancelRoutine, "CancelRoutine = %p\n", irp->CancelRoutine);
    irp->IoStatus.Status = STATUS_CANCELLED;
    irp->IoStatus.Information = 0;
    cancel_queue_cnt++;
}

static void test_queue(void)
{
    KDEVICE_QUEUE_ENTRY *entry;
    KDEVICE_QUEUE queue;
    BOOLEAN ret;
    KIRQL irql;
    IRP *irp;

    irp = IoAllocateIrp(1, FALSE);

    memset(&queue, 0xcd, sizeof(queue));
    KeInitializeDeviceQueue(&queue);
    ok(!queue.Busy, "unexpected Busy state\n");
    ok(queue.Size == sizeof(queue), "unexpected Size %x\n", queue.Size);
    ok(queue.Type == IO_TYPE_DEVICE_QUEUE, "unexpected Type %x\n", queue.Type);
    ok(IsListEmpty(&queue.DeviceListHead), "expected empty queue list\n");

    ret = KeInsertDeviceQueue(&queue, &irp->Tail.Overlay.DeviceQueueEntry);
    ok(!ret, "expected KeInsertDeviceQueue to not insert IRP\n");
    ok(queue.Busy, "expected Busy state\n");
    ok(IsListEmpty(&queue.DeviceListHead), "expected empty queue list\n");
    ok(!irp->Tail.Overlay.DeviceQueueEntry.Inserted, "expected not inserted\n");

    entry = KeRemoveDeviceQueue(&queue);
    ok(!entry, "expected KeRemoveDeviceQueue to return NULL\n");
    ok(!queue.Busy, "unexpected Busy state\n");
    ok(IsListEmpty(&queue.DeviceListHead), "expected empty queue list\n");
    ok(!irp->Tail.Overlay.DeviceQueueEntry.Inserted, "expected not inserted\n");

    ret = KeInsertDeviceQueue(&queue, &irp->Tail.Overlay.DeviceQueueEntry);
    ok(!ret, "expected KeInsertDeviceQueue to not insert IRP\n");
    ok(queue.Busy, "expected Busy state\n");
    ok(IsListEmpty(&queue.DeviceListHead), "expected empty queue list\n");
    ok(!irp->Tail.Overlay.DeviceQueueEntry.Inserted, "expected not inserted\n");

    ret = KeInsertDeviceQueue(&queue, &irp->Tail.Overlay.DeviceQueueEntry);
    ok(ret, "expected KeInsertDeviceQueue to insert IRP\n");
    ok(queue.Busy, "expected Busy state\n");
    ok(!IsListEmpty(&queue.DeviceListHead), "unexpected empty queue list\n");
    ok(irp->Tail.Overlay.DeviceQueueEntry.Inserted, "expected inserted\n");
    ok(queue.DeviceListHead.Flink == &irp->Tail.Overlay.DeviceQueueEntry.DeviceListEntry,
       "unexpected queue list head\n");

    entry = KeRemoveDeviceQueue(&queue);
    ok(entry != NULL, "expected KeRemoveDeviceQueue to return non-NULL\n");
    ok(CONTAINING_RECORD(entry, IRP, Tail.Overlay.DeviceQueueEntry) == irp,
       "unexpected IRP returned\n");
    ok(queue.Busy, "expected Busy state\n");
    ok(IsListEmpty(&queue.DeviceListHead), "expected empty queue list\n");
    ok(!irp->Tail.Overlay.DeviceQueueEntry.Inserted, "expected not inserted\n");

    entry = KeRemoveDeviceQueue(&queue);
    ok(entry == NULL, "expected KeRemoveDeviceQueue to return NULL\n");
    ok(!queue.Busy, "unexpected Busy state\n");
    ok(IsListEmpty(&queue.DeviceListHead), "expected empty queue list\n");
    ok(!irp->Tail.Overlay.DeviceQueueEntry.Inserted, "expected not inserted\n");

    IoCancelIrp(irp);
    ok(irp->Cancel, "unexpected non-cancelled state\n");

    ret = KeInsertDeviceQueue(&queue, &irp->Tail.Overlay.DeviceQueueEntry);
    ok(!ret, "expected KeInsertDeviceQueue to not insert IRP\n");
    ok(queue.Busy, "expected Busy state\n");
    ok(IsListEmpty(&queue.DeviceListHead), "expected empty queue list\n");
    ok(!irp->Tail.Overlay.DeviceQueueEntry.Inserted, "expected inserted\n");
    ret = KeInsertDeviceQueue(&queue, &irp->Tail.Overlay.DeviceQueueEntry);
    ok(ret, "expected KeInsertDeviceQueue to insert IRP\n");
    ok(queue.Busy, "expected Busy state\n");
    ok(!IsListEmpty(&queue.DeviceListHead), "unexpected empty queue list\n");
    ok(irp->Tail.Overlay.DeviceQueueEntry.Inserted, "expected inserted\n");
    ok(queue.DeviceListHead.Flink == &irp->Tail.Overlay.DeviceQueueEntry.DeviceListEntry,
       "unexpected queue list head\n");

    entry = KeRemoveDeviceQueue(&queue);
    ok(entry != NULL, "expected KeRemoveDeviceQueue to return non-NULL\n");
    ok(CONTAINING_RECORD(entry, IRP, Tail.Overlay.DeviceQueueEntry) == irp,
       "unexpected IRP returned\n");
    ok(queue.Busy, "expected Busy state\n");
    ok(IsListEmpty(&queue.DeviceListHead), "expected empty queue list\n");
    ok(!irp->Tail.Overlay.DeviceQueueEntry.Inserted, "expected inserted\n");
    ok(irp->Cancel, "unexpected non-cancelled state\n");

    IoFreeIrp(irp);

    irp = IoAllocateIrp(1, FALSE);

    IoAcquireCancelSpinLock(&irql);
    IoSetCancelRoutine(irp, cancel_queued_irp);
    IoReleaseCancelSpinLock(irql);

    ret = KeInsertDeviceQueue(&queue, &irp->Tail.Overlay.DeviceQueueEntry);
    ok(ret, "expected KeInsertDeviceQueue to insert IRP\n");
    ok(queue.Busy, "expected Busy state\n");
    ok(!IsListEmpty(&queue.DeviceListHead), "expected empty queue list\n");
    ok(irp->Tail.Overlay.DeviceQueueEntry.Inserted, "expected inserted\n");
    ok(queue.DeviceListHead.Flink == &irp->Tail.Overlay.DeviceQueueEntry.DeviceListEntry,
       "unexpected queue list head\n");

    IoCancelIrp(irp);
    ok(irp->Cancel, "unexpected non-cancelled state\n");
    ok(cancel_queue_cnt, "expected cancel routine to be called\n");

    entry = KeRemoveDeviceQueue(&queue);
    ok(entry != NULL, "expected KeRemoveDeviceQueue to return non-NULL\n");
    ok(CONTAINING_RECORD(entry, IRP, Tail.Overlay.DeviceQueueEntry) == irp,
       "unexpected IRP returned\n");
    ok(irp->Cancel, "unexpected non-cancelled state\n");
    ok(cancel_queue_cnt, "expected cancel routine to be called\n");
    ok(queue.Busy, "expected Busy state\n");
    ok(IsListEmpty(&queue.DeviceListHead), "expected empty queue list\n");
    ok(!irp->Tail.Overlay.DeviceQueueEntry.Inserted, "expected inserted\n");

    IoFreeIrp(irp);
}

static void test_mdl_map(void)
{
    char buffer[20] = "test buffer";
    void *addr;
    MDL *mdl;

    mdl = IoAllocateMdl(buffer, sizeof(buffer), FALSE, FALSE, NULL);
    ok(mdl != NULL, "IoAllocateMdl failed\n");

    MmProbeAndLockPages(mdl, KernelMode, IoReadAccess);

    addr = MmMapLockedPagesSpecifyCache(mdl, KernelMode, MmCached, NULL, FALSE, NormalPagePriority);
    todo_wine
    ok(addr != NULL, "MmMapLockedPagesSpecifyCache failed\n");

    MmUnmapLockedPages(addr, mdl);

    IoFreeMdl(mdl);
}

static void test_init_funcs(void)
{
    KTIMER timer, timer2;

    KeInitializeTimerEx(&timer, NotificationTimer);
    ok(timer.Header.Type == 8, "got: %u\n", timer.Header.Type);
    ok(timer.Header.Size == 0 || timer.Header.Size == 10, "got: %u\n", timer.Header.Size);
    ok(timer.Header.SignalState == 0, "got: %lu\n", timer.Header.SignalState);

    KeInitializeTimerEx(&timer2, SynchronizationTimer);
    ok(timer2.Header.Type == 9, "got: %u\n", timer2.Header.Type);
    ok(timer2.Header.Size == 0 || timer2.Header.Size == 10, "got: %u\n", timer2.Header.Size);
    ok(timer2.Header.SignalState == 0, "got: %lu\n", timer2.Header.SignalState);
}

static const WCHAR driver2_path[] =
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\WineTestDriver2";

static IMAGE_INFO test_image_info;
static int test_load_image_notify_count;
static WCHAR test_load_image_name[MAX_PATH];

static void WINAPI test_load_image_notify_routine(UNICODE_STRING *image_name, HANDLE process_id,
        IMAGE_INFO *image_info)
{
    if (image_name->Buffer && wcsstr(image_name->Buffer, L".tmp"))
    {
        ++test_load_image_notify_count;
        test_image_info = *image_info;
        wcscpy(test_load_image_name, image_name->Buffer);
    }
}

static void test_load_driver(void)
{
    char full_name_buffer[300];
    OBJECT_NAME_INFORMATION *full_name = (OBJECT_NAME_INFORMATION *)full_name_buffer;
    static WCHAR image_path_key_name[] = L"ImagePath";
    RTL_QUERY_REGISTRY_TABLE query_table[2];
    UNICODE_STRING name, image_path;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    NTSTATUS ret;
    HANDLE file;

    ret = PsSetLoadImageNotifyRoutine(test_load_image_notify_routine);
    ok(ret == STATUS_SUCCESS, "Got unexpected status %#lx.\n", ret);

    /* Routine gets registered twice on Windows. */
    ret = PsSetLoadImageNotifyRoutine(test_load_image_notify_routine);
    ok(ret == STATUS_SUCCESS, "Got unexpected status %#lx.\n", ret);

    RtlInitUnicodeString(&image_path, NULL);
    memset(query_table, 0, sizeof(query_table));
    query_table[0].QueryRoutine = NULL;
    query_table[0].Name = image_path_key_name;
    query_table[0].EntryContext = &image_path;
    query_table[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_TYPECHECK;
    query_table[0].DefaultType = REG_EXPAND_SZ << RTL_QUERY_REGISTRY_TYPECHECK_SHIFT;

    ret = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE, driver2_path, query_table, NULL, NULL);
    ok(ret == STATUS_SUCCESS, "Got unexpected status %#lx.\n", ret);
    ok(!!image_path.Buffer, "image_path.Buffer is NULL.\n");

    /* The image path name in the registry may contain NT symlinks (e.g. DOS
     * drives), which are resolved before the callback is called on Windows 10. */
    InitializeObjectAttributes(&attr, &image_path, OBJ_KERNEL_HANDLE, NULL, NULL);
    ret = ZwOpenFile(&file, SYNCHRONIZE, &attr, &io, 0, FILE_SYNCHRONOUS_IO_NONALERT);
    todo_wine ok(!ret, "Got unexpected status %#lx.\n", ret);
    if (!ret)
    {
        ret = ZwQueryObject(file, ObjectNameInformation, full_name_buffer, sizeof(full_name_buffer), NULL);
        ok(!ret, "Got unexpected status %#lx.\n", ret);
        ZwClose(file);
    }

    RtlInitUnicodeString(&name, driver2_path);

    ret = ZwLoadDriver(&name);
    ok(!ret, "got %#lx\n", ret);

    ok(test_load_image_notify_count == 2, "Got unexpected test_load_image_notify_count %u.\n",
            test_load_image_notify_count);
    ok(test_image_info.ImageAddressingMode == IMAGE_ADDRESSING_MODE_32BIT,
            "Got unexpected ImageAddressingMode %#x.\n", test_image_info.ImageAddressingMode);
    ok(test_image_info.SystemModeImage,
            "Got unexpected SystemModeImage %#x.\n", test_image_info.SystemModeImage);
    ok(!wcscmp(test_load_image_name, image_path.Buffer) /* Win < 10 */
            || !wcscmp(test_load_image_name, full_name->Name.Buffer),
            "Expected image path name %ls, got %ls.\n", full_name->Name.Buffer, test_load_image_name);

    test_load_image_notify_count = 0;

    ret = ZwLoadDriver(&name);
    ok(ret == STATUS_IMAGE_ALREADY_LOADED, "got %#lx\n", ret);

    ret = ZwUnloadDriver(&name);
    ok(!ret, "got %#lx\n", ret);

    ret = PsRemoveLoadImageNotifyRoutine(test_load_image_notify_routine);
    ok(ret == STATUS_SUCCESS, "Got unexpected status %#lx.\n", ret);
    ret = PsRemoveLoadImageNotifyRoutine(test_load_image_notify_routine);
    ok(ret == STATUS_SUCCESS, "Got unexpected status %#lx.\n", ret);
    ret = PsRemoveLoadImageNotifyRoutine(test_load_image_notify_routine);
    ok(ret == STATUS_PROCEDURE_NOT_FOUND, "Got unexpected status %#lx.\n", ret);

    ok(test_load_image_notify_count == 0, "Got unexpected test_load_image_notify_count %u.\n",
            test_load_image_notify_count);
    RtlFreeUnicodeString(&image_path);
}

static NTSTATUS wait_single(void *obj, ULONGLONG timeout)
{
    LARGE_INTEGER integer;

    integer.QuadPart = timeout;
    return KeWaitForSingleObject(obj, Executive, KernelMode, FALSE, &integer);
}

static NTSTATUS wait_multiple(ULONG count, void *objs[], WAIT_TYPE wait_type, ULONGLONG timeout)
{
    LARGE_INTEGER integer;

    integer.QuadPart = timeout;
    return KeWaitForMultipleObjects(count, objs, wait_type, Executive, KernelMode, FALSE, &integer, NULL);
}

static NTSTATUS wait_single_handle(HANDLE handle, ULONGLONG timeout)
{
    LARGE_INTEGER integer;

    integer.QuadPart = timeout;
    return ZwWaitForSingleObject(handle, FALSE, &integer);
}

static void test_current_thread(BOOL is_system)
{
    PROCESS_BASIC_INFORMATION info;
    DISPATCHER_HEADER *header;
    HANDLE process_handle, id;
    PEPROCESS current;
    PETHREAD thread;
    NTSTATUS ret;

    current = IoGetCurrentProcess();
    ok(current != NULL, "Expected current process to be non-NULL\n");

    header = (DISPATCHER_HEADER*)current;
    ok(header->Type == 3, "header->Type != 3, = %u\n", header->Type);
    ret = wait_single(current, 0);
    ok(ret == STATUS_TIMEOUT, "got %#lx\n", ret);

    if (is_system)
        ok(current == *pPsInitialSystemProcess, "current != PsInitialSystemProcess\n");
    else
        ok(current != *pPsInitialSystemProcess, "current == PsInitialSystemProcess\n");

    ok(PsGetProcessId(current) == PsGetCurrentProcessId(), "process IDs don't match\n");
    ok(PsGetThreadProcessId((PETHREAD)KeGetCurrentThread()) == PsGetCurrentProcessId(), "process IDs don't match\n");

    thread = PsGetCurrentThread();
    ret = wait_single( thread, 0 );
    ok(ret == STATUS_TIMEOUT, "got %#lx\n", ret);

    ok(PsGetThreadId((PETHREAD)KeGetCurrentThread()) == PsGetCurrentThreadId(), "thread IDs don't match\n");
    ok(PsIsSystemThread((PETHREAD)KeGetCurrentThread()) == is_system, "unexpected system thread\n");
    ok(ExGetPreviousMode() == is_system ? KernelMode : UserMode, "previous mode is not correct\n");
    if (!is_system)
    {
        ok(create_caller_thread == KeGetCurrentThread(), "thread is not create caller thread\n");
        ok(create_irp_thread == (PETHREAD)KeGetCurrentThread(), "thread of create request is not current thread\n");
    }

    ret = ObOpenObjectByPointer(current, OBJ_KERNEL_HANDLE, NULL, PROCESS_QUERY_INFORMATION, NULL, KernelMode, &process_handle);
    ok(!ret, "ObOpenObjectByPointer failed: %#lx\n", ret);

    ret = ZwQueryInformationProcess(process_handle, ProcessBasicInformation, &info, sizeof(info), NULL);
    ok(!ret, "ZwQueryInformationProcess failed: %#lx\n", ret);

    id = PsGetProcessInheritedFromUniqueProcessId(current);
    ok(id == (HANDLE)info.InheritedFromUniqueProcessId, "unexpected process id %p\n", id);

    ret = ZwClose(process_handle);
    ok(!ret, "ZwClose failed: %#lx\n", ret);
}

static void test_critical_region(BOOL is_dispatcher)
{
    BOOLEAN result;

    KeEnterCriticalRegion();
    KeEnterCriticalRegion();

    result = KeAreApcsDisabled();
    ok(result == TRUE, "KeAreApcsDisabled returned %x\n", result);
    KeLeaveCriticalRegion();

    result = KeAreApcsDisabled();
    ok(result == TRUE, "KeAreApcsDisabled returned %x\n", result);
    KeLeaveCriticalRegion();

    result = KeAreApcsDisabled();
    ok(result == is_dispatcher || broken(is_dispatcher && !result),
       "KeAreApcsDisabled returned %x\n", result);
}

static void sleep_1ms(void)
{
    LARGE_INTEGER timeout;

    timeout.QuadPart = -1 * 10000;
    KeDelayExecutionThread( KernelMode, FALSE, &timeout );
}

static void sleep(void)
{
    LARGE_INTEGER timeout;
    timeout.QuadPart = -20 * 10000;
    KeDelayExecutionThread( KernelMode, FALSE, &timeout );
}

static HANDLE create_thread(PKSTART_ROUTINE proc, void *arg)
{
    OBJECT_ATTRIBUTES attr = {0};
    HANDLE thread;
    NTSTATUS ret;

    attr.Length = sizeof(attr);
    attr.Attributes = OBJ_KERNEL_HANDLE;
    ret = PsCreateSystemThread(&thread, THREAD_ALL_ACCESS, &attr, NULL, NULL, proc, arg);
    ok(!ret, "got %#lx\n", ret);

    return thread;
}

static void join_thread(HANDLE thread)
{
    NTSTATUS ret;

    ret = ZwWaitForSingleObject(thread, FALSE, NULL);
    ok(!ret, "got %#lx\n", ret);
    ret = ZwClose(thread);
    ok(!ret, "got %#lx\n", ret);
}

static void run_thread(PKSTART_ROUTINE proc, void *arg)
{
    HANDLE thread = create_thread(proc, arg);
    join_thread(thread);
}

static KMUTEX test_mutex;

static void WINAPI mutex_thread(void *arg)
{
    NTSTATUS ret, expect = (NTSTATUS)(DWORD_PTR)arg;

    ret = wait_single(&test_mutex, 0);
    ok(ret == expect, "expected %#lx, got %#lx\n", expect, ret);

    if (!ret) KeReleaseMutex(&test_mutex, FALSE);
    PsTerminateSystemThread(STATUS_SUCCESS);
}

static KEVENT remove_lock_ready;

static void WINAPI remove_lock_thread(void *arg)
{
    IO_REMOVE_LOCK *lock = arg;
    NTSTATUS ret;

    ret = IoAcquireRemoveLockEx(lock, NULL, "", 1, sizeof(IO_REMOVE_LOCK_COMMON_BLOCK));
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    KeSetEvent(&remove_lock_ready, 0, FALSE);

    IoReleaseRemoveLockAndWaitEx(lock, NULL, sizeof(IO_REMOVE_LOCK_COMMON_BLOCK));
    PsTerminateSystemThread(STATUS_SUCCESS);
}

struct test_sync_dpc_context
{
    BOOL called;
};

static void WINAPI test_sync_dpc(KDPC *dpc, void *context, void *system_argument1, void *system_argument2)
{
    struct test_sync_dpc_context *c = context;

    c->called = TRUE;
}

static void test_sync(void)
{
    static const ULONG wine_tag = 0x454e4957; /* WINE */
    struct test_sync_dpc_context dpc_context;
    KEVENT manual_event, auto_event, *event;
    KSEMAPHORE semaphore, semaphore2;
    IO_REMOVE_LOCK remove_lock;
    LARGE_INTEGER timeout;
    OBJECT_ATTRIBUTES attr;
    HANDLE handle, thread;
    void *objs[2];
    KTIMER timer;
    NTSTATUS ret;
    KDPC dpc;
    int i;

    KeInitializeEvent(&manual_event, NotificationEvent, FALSE);

    ret = wait_single(&manual_event, 0);
    ok(ret == STATUS_TIMEOUT, "got %#lx\n", ret);

    ret = KeReadStateEvent(&manual_event);
    ok(ret == 0, "got %ld\n", ret);

    KeSetEvent(&manual_event, 0, FALSE);

    ret = KeReadStateEvent(&manual_event);
    ok(ret == 1, "got %ld\n", ret);

    ret = wait_single(&manual_event, 0);
    ok(ret == 0, "got %#lx\n", ret);

    ret = wait_single(&manual_event, 0);
    ok(ret == 0, "got %#lx\n", ret);

    KeResetEvent(&manual_event);

    ret = wait_single(&manual_event, 0);
    ok(ret == STATUS_TIMEOUT, "got %#lx\n", ret);

    KeInitializeEvent(&auto_event, SynchronizationEvent, FALSE);

    ret = wait_single(&auto_event, 0);
    ok(ret == STATUS_TIMEOUT, "got %#lx\n", ret);

    KeSetEvent(&auto_event, 0, FALSE);

    ret = wait_single(&auto_event, 0);
    ok(ret == 0, "got %#lx\n", ret);

    ret = wait_single(&auto_event, 0);
    ok(ret == STATUS_TIMEOUT, "got %#lx\n", ret);

    KeInitializeEvent(&auto_event, SynchronizationEvent, TRUE);

    ret = wait_single(&auto_event, 0);
    ok(ret == 0, "got %#lx\n", ret);

    objs[0] = &manual_event;
    objs[1] = &auto_event;

    ret = wait_multiple(2, objs, WaitAny, 0);
    ok(ret == STATUS_TIMEOUT, "got %#lx\n", ret);

    KeSetEvent(&manual_event, 0, FALSE);
    KeSetEvent(&auto_event, 0, FALSE);

    ret = wait_multiple(2, objs, WaitAny, 0);
    ok(ret == 0, "got %#lx\n", ret);

    ret = wait_single(&auto_event, 0);
    ok(ret == 0, "got %#lx\n", ret);

    KeResetEvent(&manual_event);
    KeSetEvent(&auto_event, 0, FALSE);

    ret = wait_multiple(2, objs, WaitAny, 0);
    ok(ret == 1, "got %#lx\n", ret);

    ret = wait_multiple(2, objs, WaitAny, 0);
    ok(ret == STATUS_TIMEOUT, "got %#lx\n", ret);

    KeSetEvent(&manual_event, 0, FALSE);
    KeSetEvent(&auto_event, 0, FALSE);

    ret = wait_multiple(2, objs, WaitAll, 0);
    ok(ret == 0, "got %#lx\n", ret);

    ret = wait_multiple(2, objs, WaitAll, 0);
    ok(ret == STATUS_TIMEOUT, "got %#lx\n", ret);

    KeSetEvent(&auto_event, 0, FALSE);
    KeResetEvent(&manual_event);

    ret = wait_multiple(2, objs, WaitAll, 0);
    ok(ret == STATUS_TIMEOUT, "got %#lx\n", ret);

    ret = wait_single(&auto_event, 0);
    ok(ret == 0, "got %#lx\n", ret);

    objs[0] = &auto_event;
    objs[1] = &manual_event;
    KeSetEvent(&manual_event, 0, FALSE);
    KeSetEvent(&auto_event, 0, FALSE);

    ret = wait_multiple(2, objs, WaitAny, 0);
    ok(ret == 0, "got %#lx\n", ret);

    ret = wait_multiple(2, objs, WaitAny, 0);
    ok(ret == 1, "got %#lx\n", ret);

    ret = wait_multiple(2, objs, WaitAny, 0);
    ok(ret == 1, "got %#lx\n", ret);

    InitializeObjectAttributes(&attr, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
    ret = ZwCreateEvent(&handle, SYNCHRONIZE, &attr, NotificationEvent, TRUE);
    ok(!ret, "ZwCreateEvent failed: %#lx\n", ret);

    ret = ObReferenceObjectByHandle(handle, SYNCHRONIZE, *pExEventObjectType, KernelMode, (void **)&event, NULL);
    ok(!ret, "ObReferenceObjectByHandle failed: %#lx\n", ret);


    ret = wait_single(event, 0);
    ok(ret == 0, "got %#lx\n", ret);
    ret = KeReadStateEvent(event);
    ok(ret == 1, "got %ld\n", ret);
    KeResetEvent(event);
    ret = KeReadStateEvent(event);
    ok(ret == 0, "got %ld\n", ret);
    ret = wait_single(event, 0);
    ok(ret == STATUS_TIMEOUT, "got %#lx\n", ret);
    ret = wait_single_handle(handle, 0);
    ok(ret == STATUS_TIMEOUT, "got %#lx\n", ret);

    KeSetEvent(event, 0, FALSE);
    ret = wait_single(event, 0);
    ok(ret == 0, "got %#lx\n", ret);
    ret = wait_single_handle(handle, 0);
    ok(!ret, "got %#lx\n", ret);

    ZwClose(handle);
    ObDereferenceObject(event);

    event = IoCreateSynchronizationEvent(NULL, &handle);
    ok(event != NULL, "IoCreateSynchronizationEvent failed\n");

    ret = wait_single(event, 0);
    ok(ret == 0, "got %#lx\n", ret);
    KeResetEvent(event);
    ret = wait_single(event, 0);
    ok(ret == STATUS_TIMEOUT, "got %#lx\n", ret);
    ret = wait_single_handle(handle, 0);
    ok(ret == STATUS_TIMEOUT, "got %#lx\n", ret);

    ret = ZwSetEvent(handle, NULL);
    ok(!ret, "NtSetEvent returned %#lx\n", ret);
    ret = wait_single(event, 0);
    ok(ret == 0, "got %#lx\n", ret);
    ret = wait_single_handle(handle, 0);
    ok(ret == STATUS_TIMEOUT, "got %#lx\n", ret);

    KeSetEvent(event, 0, FALSE);
    ret = wait_single_handle(handle, 0);
    ok(!ret, "got %#lx\n", ret);
    ret = wait_single(event, 0);
    ok(ret == STATUS_TIMEOUT, "got %#lx\n", ret);

    ret = ZwClose(handle);
    ok(!ret, "ZwClose returned %#lx\n", ret);

    /* test semaphores */
    KeInitializeSemaphore(&semaphore, 0, 5);

    ret = wait_single(&semaphore, 0);
    ok(ret == STATUS_TIMEOUT, "got %lu\n", ret);

    ret = KeReleaseSemaphore(&semaphore, 0, 1, FALSE);
    ok(ret == 0, "got prev %ld\n", ret);

    ret = KeReleaseSemaphore(&semaphore, 0, 2, FALSE);
    ok(ret == 1, "got prev %ld\n", ret);

    ret = KeReleaseSemaphore(&semaphore, 0, 1, FALSE);
    ok(ret == 3, "got prev %ld\n", ret);

    for (i = 0; i < 4; i++)
    {
        ret = wait_single(&semaphore, 0);
        ok(ret == 0, "got %#lx\n", ret);
    }

    ret = wait_single(&semaphore, 0);
    ok(ret == STATUS_TIMEOUT, "got %#lx\n", ret);

    KeInitializeSemaphore(&semaphore2, 3, 5);

    ret = KeReleaseSemaphore(&semaphore2, 0, 1, FALSE);
    ok(ret == 3, "got prev %ld\n", ret);

    for (i = 0; i < 4; i++)
    {
        ret = wait_single(&semaphore2, 0);
        ok(ret == 0, "got %#lx\n", ret);
    }

    objs[0] = &semaphore;
    objs[1] = &semaphore2;

    ret = wait_multiple(2, objs, WaitAny, 0);
    ok(ret == STATUS_TIMEOUT, "got %#lx\n", ret);

    KeReleaseSemaphore(&semaphore, 0, 1, FALSE);
    KeReleaseSemaphore(&semaphore2, 0, 1, FALSE);

    ret = wait_multiple(2, objs, WaitAny, 0);
    ok(ret == 0, "got %#lx\n", ret);

    ret = wait_multiple(2, objs, WaitAny, 0);
    ok(ret == 1, "got %#lx\n", ret);

    ret = wait_multiple(2, objs, WaitAny, 0);
    ok(ret == STATUS_TIMEOUT, "got %#lx\n", ret);

    KeReleaseSemaphore(&semaphore, 0, 1, FALSE);
    KeReleaseSemaphore(&semaphore2, 0, 1, FALSE);

    ret = wait_multiple(2, objs, WaitAll, 0);
    ok(ret == 0, "got %#lx\n", ret);

    ret = wait_multiple(2, objs, WaitAny, 0);
    ok(ret == STATUS_TIMEOUT, "got %#lx\n", ret);

    /* test mutexes */
    KeInitializeMutex(&test_mutex, 0);

    for (i = 0; i < 10; i++)
    {
        ret = wait_single(&test_mutex, 0);
        ok(ret == 0, "got %#lx\n", ret);
    }

    for (i = 0; i < 10; i++)
    {
        ret = KeReleaseMutex(&test_mutex, FALSE);
        ok(ret == i - 9, "expected %d, got %ld\n", i - 9, ret);
    }

    run_thread(mutex_thread, (void *)0);

    ret = wait_single(&test_mutex, 0);
    ok(ret == 0, "got %#lx\n", ret);

    run_thread(mutex_thread, (void *)STATUS_TIMEOUT);

    ret = KeReleaseMutex(&test_mutex, 0);
    ok(ret == 0, "got %#lx\n", ret);

    run_thread(mutex_thread, (void *)0);

    /* test timers */
    KeInitializeTimerEx(&timer, NotificationTimer);

    timeout.QuadPart = -20 * 10000;
    KeSetTimerEx(&timer, timeout, 0, NULL);

    ret = wait_single(&timer, 0);
    ok(ret == WAIT_TIMEOUT, "got %#lx\n", ret);

    ret = wait_single(&timer, -40 * 10000);
    ok(ret == 0, "got %#lx\n", ret);

    ret = wait_single(&timer, 0);
    ok(ret == 0, "got %#lx\n", ret);

    KeCancelTimer(&timer);
    KeInitializeTimerEx(&timer, SynchronizationTimer);

    memset(&dpc_context, 0, sizeof(dpc_context));
    KeInitializeDpc(&dpc, test_sync_dpc, &dpc_context);

    KeSetTimerEx(&timer, timeout, 0, &dpc);

    ret = wait_single(&timer, 0);
    ok(ret == WAIT_TIMEOUT, "got %#lx\n", ret);
    ok(!dpc_context.called, "DPC was called unexpectedly.\n");

    ret = wait_single(&timer, -40 * 10000);
    ok(ret == 0, "got %#lx\n", ret);
    sleep_1ms();
    ok(dpc_context.called, "DPC was not called.\n");

    ret = wait_single(&timer, -40 * 10000);
    ok(ret == WAIT_TIMEOUT, "got %#lx\n", ret);

    KeCancelTimer(&timer);
    KeSetTimerEx(&timer, timeout, 20, NULL);

    ret = wait_single(&timer, 0);
    ok(ret == WAIT_TIMEOUT, "got %#lx\n", ret);

    ret = wait_single(&timer, -40 * 10000);
    ok(ret == 0, "got %#lx\n", ret);

    ret = wait_single(&timer, 0);
    /* aliasing makes it sometimes succeeds, try again in that case */
    if (ret == 0) ret = wait_single(&timer, 0);
    ok(ret == WAIT_TIMEOUT, "got %#lx\n", ret);

    ret = wait_single(&timer, -40 * 10000);
    ok(ret == 0, "got %#lx\n", ret);

    ret = wait_single(&timer, -40 * 10000);
    ok(ret == 0, "got %#lx\n", ret);

    KeCancelTimer(&timer);

    /* Test cancelling timer. */
    dpc_context.called = 0;
    KeSetTimerEx(&timer, timeout, 0, &dpc);

    ret = wait_single(&timer, 0);
    ok(ret == WAIT_TIMEOUT, "got %#lx\n", ret);
    ok(!dpc_context.called, "DPC was called.\n");

    KeCancelTimer(&timer);
    dpc_context.called = 0;
    ret = wait_single(&timer, -40 * 10000);
    ok(ret == WAIT_TIMEOUT, "got %#lx\n", ret);
    ok(!dpc_context.called, "DPC was called.\n");

    KeSetTimerEx(&timer, timeout, 20, &dpc);
    KeSetTimerEx(&timer, timeout, 0, &dpc);
    ret = wait_single(&timer, 0);
    ok(ret == WAIT_TIMEOUT, "got %#lx\n", ret);

    ret = wait_single(&timer, -40 * 10000);
    ok(ret == 0, "got %#lx\n", ret);

    ret = wait_single(&timer, 0);
    ok(ret == WAIT_TIMEOUT, "got %#lx\n", ret);

    ret = wait_single(&timer, -40 * 10000);
    ok(ret == WAIT_TIMEOUT, "got %#lx\n", ret);

    ret = wait_single(&timer, -40 * 10000);
    ok(ret == WAIT_TIMEOUT, "got %#lx\n", ret);

    KeCancelTimer(&timer);
    /* Test reinitializing timer. */
    KeSetTimerEx(&timer, timeout, 0, &dpc);
    KeInitializeTimerEx(&timer, SynchronizationTimer);
    dpc_context.called = 0;
    ret = wait_single(&timer, -40 * 10000);
    ok(ret == 0, "got %#lx\n", ret);
    sleep_1ms();
    todo_wine ok(dpc_context.called, "DPC was not called.\n");

    ret = wait_single(&timer, 0);
    ok(ret == WAIT_TIMEOUT, "got %#lx\n", ret);
    sleep_1ms();
    todo_wine ok(dpc_context.called, "DPC was not called.\n");

    dpc_context.called = 0;
    KeSetTimerEx(&timer, timeout, 0, &dpc);
    ret = wait_single(&timer, -40 * 10000);
    ok(ret == 0, "got %#lx\n", ret);
    sleep_1ms();
    ok(dpc_context.called, "DPC was not called.\n");

    KeCancelTimer(&timer);
    /* remove locks */

    IoInitializeRemoveLockEx(&remove_lock, wine_tag, 0, 0, sizeof(IO_REMOVE_LOCK_COMMON_BLOCK));

    ret = IoAcquireRemoveLockEx(&remove_lock, NULL, "", 1, sizeof(IO_REMOVE_LOCK_COMMON_BLOCK));
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    IoReleaseRemoveLockEx(&remove_lock, NULL, sizeof(IO_REMOVE_LOCK_COMMON_BLOCK));

    ret = IoAcquireRemoveLockEx(&remove_lock, NULL, "", 1, sizeof(IO_REMOVE_LOCK_COMMON_BLOCK));
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    ret = IoAcquireRemoveLockEx(&remove_lock, NULL, "", 1, sizeof(IO_REMOVE_LOCK_COMMON_BLOCK));
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    KeInitializeEvent(&remove_lock_ready, SynchronizationEvent, FALSE);
    thread = create_thread(remove_lock_thread, &remove_lock);
    ret = wait_single(&remove_lock_ready, -1000 * 10000);
    ok(!ret, "got %#lx\n", ret);
    ret = wait_single_handle(thread, -50 * 10000);
    ok(ret == STATUS_TIMEOUT, "got %#lx\n", ret);

    ret = IoAcquireRemoveLockEx(&remove_lock, NULL, "", 1, sizeof(IO_REMOVE_LOCK_COMMON_BLOCK));
    ok(ret == STATUS_DELETE_PENDING, "got %#lx\n", ret);

    IoReleaseRemoveLockEx(&remove_lock, NULL, sizeof(IO_REMOVE_LOCK_COMMON_BLOCK));
    ret = wait_single_handle(thread, 0);
    ok(ret == STATUS_TIMEOUT, "got %#lx\n", ret);

    IoReleaseRemoveLockEx(&remove_lock, NULL, sizeof(IO_REMOVE_LOCK_COMMON_BLOCK));
    ret = wait_single_handle(thread, -10000 * 10000);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    ret = IoAcquireRemoveLockEx(&remove_lock, NULL, "", 1, sizeof(IO_REMOVE_LOCK_COMMON_BLOCK));
    ok(ret == STATUS_DELETE_PENDING, "got %#lx\n", ret);
}

static void test_call_driver(DEVICE_OBJECT *device)
{
    IO_STACK_LOCATION *irpsp;
    IO_STATUS_BLOCK iosb;
    IRP *irp = NULL;
    KEVENT event;
    NTSTATUS status;

    iosb.Status = 0xdeadbeef;
    iosb.Information = 0xdeadbeef;
    irp = IoBuildAsynchronousFsdRequest(IRP_MJ_FLUSH_BUFFERS, device, NULL, 0, NULL, &iosb);
    ok(irp->UserIosb == &iosb, "unexpected UserIosb\n");
    ok(!irp->Cancel, "Cancel = %x\n", irp->Cancel);
    ok(!irp->CancelRoutine, "CancelRoutine = %p\n", irp->CancelRoutine);
    ok(!irp->UserEvent, "UserEvent = %p\n", irp->UserEvent);
    ok(irp->CurrentLocation == 2, "CurrentLocation = %u\n", irp->CurrentLocation);
    ok(irp->Tail.Overlay.Thread == (PETHREAD)KeGetCurrentThread(),
       "IRP thread is not the current thread\n");
    ok(!irp->IoStatus.Status, "got status %#lx\n", irp->IoStatus.Status);
    ok(!irp->IoStatus.Information, "got information %#I64x\n", (UINT64)irp->IoStatus.Information);
    ok(iosb.Status == 0xdeadbeef, "got status %#lx\n", iosb.Status);
    ok(iosb.Information == 0xdeadbeef, "got information %#I64x\n", (UINT64)iosb.Information);

    irpsp = IoGetNextIrpStackLocation(irp);
    ok(irpsp->MajorFunction == IRP_MJ_FLUSH_BUFFERS, "MajorFunction = %u\n", irpsp->MajorFunction);
    ok(!irpsp->DeviceObject, "DeviceObject = %p\n", irpsp->DeviceObject);
    ok(!irpsp->FileObject, "FileObject = %p\n", irpsp->FileObject);
    ok(!irpsp->CompletionRoutine, "CompletionRoutine = %p\n", irpsp->CompletionRoutine);

    status = IoCallDriver(device, irp);
    ok(status == STATUS_PENDING, "IoCallDriver returned %#lx\n", status);
    ok(!irp->IoStatus.Status, "got status %#lx\n", irp->IoStatus.Status);
    ok(!irp->IoStatus.Information, "got information %#I64x\n", (UINT64)irp->IoStatus.Information);
    ok(iosb.Status == 0xdeadbeef, "got status %#lx\n", iosb.Status);
    ok(iosb.Information == 0xdeadbeef, "got information %#I64x\n", (UINT64)iosb.Information);

    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = 123;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    ok(iosb.Status == STATUS_SUCCESS, "got status %#lx\n", iosb.Status);
    ok(iosb.Information == 123, "got information %#I64x\n", (UINT64)iosb.Information);

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildSynchronousFsdRequest(IRP_MJ_FLUSH_BUFFERS, device, NULL, 0, NULL, &event, &iosb);
    ok(irp->UserIosb == &iosb, "unexpected UserIosb\n");
    ok(!irp->Cancel, "Cancel = %x\n", irp->Cancel);
    ok(!irp->CancelRoutine, "CancelRoutine = %p\n", irp->CancelRoutine);
    ok(irp->UserEvent == &event, "UserEvent = %p\n", irp->UserEvent);
    ok(irp->CurrentLocation == 2, "CurrentLocation = %u\n", irp->CurrentLocation);
    ok(irp->Tail.Overlay.Thread == (PETHREAD)KeGetCurrentThread(),
       "IRP thread is not the current thread\n");

    irpsp = IoGetNextIrpStackLocation(irp);
    ok(irpsp->MajorFunction == IRP_MJ_FLUSH_BUFFERS, "MajorFunction = %u\n", irpsp->MajorFunction);
    ok(!irpsp->DeviceObject, "DeviceObject = %p\n", irpsp->DeviceObject);
    ok(!irpsp->FileObject, "FileObject = %p\n", irpsp->FileObject);
    ok(!irpsp->CompletionRoutine, "CompletionRoutine = %p\n", irpsp->CompletionRoutine);

    status = wait_single(&event, 0);
    ok(status == STATUS_TIMEOUT, "got %#lx\n", status);

    status = IoCallDriver(device, irp);
    ok(status == STATUS_PENDING, "IoCallDriver returned %#lx\n", status);

    status = wait_single(&event, 0);
    ok(status == STATUS_TIMEOUT, "got %#lx\n", status);

    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;
    IoCompleteRequest(irp, IO_NO_INCREMENT);

    status = wait_single(&event, 0);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);
}

static int cancel_cnt;

static void WINAPI cancel_irp(DEVICE_OBJECT *device, IRP *irp)
{
    IoReleaseCancelSpinLock(irp->CancelIrql);
    ok(irp->Cancel == TRUE, "Cancel = %x\n", irp->Cancel);
    ok(!irp->CancelRoutine, "CancelRoutine = %p\n", irp->CancelRoutine);
    irp->IoStatus.Status = STATUS_CANCELLED;
    irp->IoStatus.Information = 0;
    cancel_cnt++;
}

static void WINAPI cancel_ioctl_irp(DEVICE_OBJECT *device, IRP *irp)
{
    IoReleaseCancelSpinLock(irp->CancelIrql);
    irp->IoStatus.Status = STATUS_CANCELLED;
    irp->IoStatus.Information = 0;
    cancel_cnt++;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
}

static NTSTATUS WINAPI cancel_test_completion(DEVICE_OBJECT *device, IRP *irp, void *context)
{
    ok(cancel_cnt == 1, "cancel_cnt = %d\n", cancel_cnt);
    *(BOOL*)context = TRUE;
    return STATUS_SUCCESS;
}

static void test_cancel_irp(DEVICE_OBJECT *device)
{
    IO_STACK_LOCATION *irpsp;
    IO_STATUS_BLOCK iosb;
    IRP *irp = NULL;
    BOOL completion_called;
    BOOLEAN r;
    NTSTATUS status;

    /* cancel IRP with no cancel routine */
    irp = IoBuildAsynchronousFsdRequest(IRP_MJ_FLUSH_BUFFERS, device, NULL, 0, NULL, &iosb);

    r = IoCancelIrp(irp);
    ok(!r, "IoCancelIrp returned %x\n", r);
    ok(irp->Cancel == TRUE, "Cancel = %x\n", irp->Cancel);

    r = IoCancelIrp(irp);
    ok(!r, "IoCancelIrp returned %x\n", r);
    IoFreeIrp(irp);

    irp = IoBuildAsynchronousFsdRequest(IRP_MJ_FLUSH_BUFFERS, device, NULL, 0, NULL, &iosb);

    /* cancel IRP with cancel routine */
    status = IoCallDriver(device, irp);
    ok(status == STATUS_PENDING, "IoCallDriver returned %#lx\n", status);

    ok(irp->CurrentLocation == 1, "CurrentLocation = %u\n", irp->CurrentLocation);
    irpsp = IoGetCurrentIrpStackLocation(irp);
    ok(irpsp->DeviceObject == device, "DeviceObject = %p\n", irpsp->DeviceObject);

    IoSetCancelRoutine(irp, cancel_irp);
    cancel_cnt = 0;
    r = IoCancelIrp(irp);
    ok(r == TRUE, "IoCancelIrp returned %x\n", r);
    ok(irp->Cancel == TRUE, "Cancel = %x\n", irp->Cancel);
    ok(cancel_cnt == 1, "cancel_cnt = %d\n", cancel_cnt);

    cancel_cnt = 0;
    r = IoCancelIrp(irp);
    ok(!r, "IoCancelIrp returned %x\n", r);
    ok(irp->Cancel == TRUE, "Cancel = %x\n", irp->Cancel);
    ok(!cancel_cnt, "cancel_cnt = %d\n", cancel_cnt);

    IoCompleteRequest(irp, IO_NO_INCREMENT);

    /* cancel IRP with cancel and completion routines with no SL_INVOKE_ON_ERROR */
    irp = IoBuildAsynchronousFsdRequest(IRP_MJ_FLUSH_BUFFERS, device, NULL, 0, NULL, &iosb);
    IoSetCompletionRoutine(irp, cancel_test_completion, &completion_called, TRUE, FALSE, TRUE);

    status = IoCallDriver(device, irp);
    ok(status == STATUS_PENDING, "IoCallDriver returned %#lx\n", status);

    IoSetCancelRoutine(irp, cancel_irp);
    cancel_cnt = 0;
    r = IoCancelIrp(irp);
    ok(r == TRUE, "IoCancelIrp returned %x\n", r);
    ok(cancel_cnt == 1, "cancel_cnt = %d\n", cancel_cnt);
    ok(irp->Cancel == TRUE, "Cancel = %x\n", irp->Cancel);

    completion_called = FALSE;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    ok(completion_called, "completion not called\n");

    /* cancel IRP with cancel and completion routines with no SL_INVOKE_ON_CANCEL flag */
    irp = IoBuildAsynchronousFsdRequest(IRP_MJ_FLUSH_BUFFERS, device, NULL, 0, NULL, &iosb);
    IoSetCompletionRoutine(irp, cancel_test_completion, &completion_called, TRUE, TRUE, FALSE);

    status = IoCallDriver(device, irp);
    ok(status == STATUS_PENDING, "IoCallDriver returned %#lx\n", status);

    IoSetCancelRoutine(irp, cancel_irp);
    cancel_cnt = 0;
    r = IoCancelIrp(irp);
    ok(r == TRUE, "IoCancelIrp returned %x\n", r);
    ok(irp->Cancel == TRUE, "Cancel = %x\n", irp->Cancel);
    ok(cancel_cnt == 1, "cancel_cnt = %d\n", cancel_cnt);

    completion_called = FALSE;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    ok(completion_called, "completion not called\n");

    /* cancel IRP with cancel and completion routines, but no SL_INVOKE_ON_ERROR nor SL_INVOKE_ON_CANCEL flag */
    irp = IoBuildAsynchronousFsdRequest(IRP_MJ_FLUSH_BUFFERS, device, NULL, 0, NULL, &iosb);
    IoSetCompletionRoutine(irp, cancel_test_completion, &completion_called, TRUE, FALSE, FALSE);

    status = IoCallDriver(device, irp);
    ok(status == STATUS_PENDING, "IoCallDriver returned %#lx\n", status);

    IoSetCancelRoutine(irp, cancel_irp);
    cancel_cnt = 0;
    r = IoCancelIrp(irp);
    ok(r == TRUE, "IoCancelIrp returned %x\n", r);
    ok(irp->Cancel == TRUE, "Cancel = %x\n", irp->Cancel);
    ok(cancel_cnt == 1, "cancel_cnt = %d\n", cancel_cnt);

    completion_called = FALSE;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    ok(!completion_called, "completion not called\n");
}

static int callout_cnt;

static void WINAPI callout(void *parameter)
{
    ok(parameter == (void*)0xdeadbeef, "parameter = %p\n", parameter);
    callout_cnt++;
}

static void test_stack_callout(void)
{
    NTSTATUS (WINAPI *pKeExpandKernelStackAndCallout)(PEXPAND_STACK_CALLOUT,void*,SIZE_T);
    NTSTATUS (WINAPI *pKeExpandKernelStackAndCalloutEx)(PEXPAND_STACK_CALLOUT,void*,SIZE_T,BOOLEAN,void*);
    NTSTATUS ret;

    pKeExpandKernelStackAndCallout = get_proc_address("KeExpandKernelStackAndCallout");
    if (pKeExpandKernelStackAndCallout)
    {
        callout_cnt = 0;
        ret = pKeExpandKernelStackAndCallout(callout, (void*)0xdeadbeef, 4096);
        ok(ret == STATUS_SUCCESS, "KeExpandKernelStackAndCallout failed: %#lx\n", ret);
        ok(callout_cnt == 1, "callout_cnt = %u\n", callout_cnt);
    }
    else win_skip("KeExpandKernelStackAndCallout is not available\n");

    pKeExpandKernelStackAndCalloutEx = get_proc_address("KeExpandKernelStackAndCalloutEx");
    if (pKeExpandKernelStackAndCalloutEx)
    {
        callout_cnt = 0;
        ret = pKeExpandKernelStackAndCalloutEx(callout, (void*)0xdeadbeef, 4096, FALSE, NULL);
        ok(ret == STATUS_SUCCESS, "KeExpandKernelStackAndCalloutEx failed: %#lx\n", ret);
        ok(callout_cnt == 1, "callout_cnt = %u\n", callout_cnt);
    }
    else win_skip("KeExpandKernelStackAndCalloutEx is not available\n");
}

static void test_lookaside_list(void)
{
    NPAGED_LOOKASIDE_LIST list;
    PAGED_LOOKASIDE_LIST paged_list;
    ULONG tag = 0x454e4957; /* WINE */

    ExInitializeNPagedLookasideList(&list, NULL, NULL, POOL_NX_ALLOCATION, LOOKASIDE_MINIMUM_BLOCK_SIZE, tag, 0);
    ok(list.L.Depth == 4, "Expected 4 got %u\n", list.L.Depth);
    ok(list.L.MaximumDepth == 256, "Expected 256 got %u\n", list.L.MaximumDepth);
    ok(list.L.TotalAllocates == 0, "Expected 0 got %lu\n", list.L.TotalAllocates);
    ok(list.L.AllocateMisses == 0, "Expected 0 got %lu\n", list.L.AllocateMisses);
    ok(list.L.TotalFrees == 0, "Expected 0 got %lu\n", list.L.TotalFrees);
    ok(list.L.FreeMisses == 0, "Expected 0 got %lu\n", list.L.FreeMisses);
    ok(list.L.Type == (NonPagedPool|POOL_NX_ALLOCATION),
       "Expected NonPagedPool|POOL_NX_ALLOCATION got %u\n", list.L.Type);
    ok(list.L.Tag == tag, "Expected %lx got %lx\n", tag, list.L.Tag);
    ok(list.L.Size == LOOKASIDE_MINIMUM_BLOCK_SIZE, "got %lu\n", list.L.Size);
    ok(list.L.LastTotalAllocates == 0,"Expected 0 got %lu\n", list.L.LastTotalAllocates);
    ok(list.L.LastAllocateMisses == 0,"Expected 0 got %lu\n", list.L.LastAllocateMisses);
    ExDeleteNPagedLookasideList(&list);

    list.L.Depth = 0;
    ExInitializeNPagedLookasideList(&list, NULL, NULL, 0, LOOKASIDE_MINIMUM_BLOCK_SIZE, tag, 20);
    ok(list.L.Depth == 4, "Expected 4 got %u\n", list.L.Depth);
    ok(list.L.MaximumDepth == 256, "Expected 256 got %u\n", list.L.MaximumDepth);
    ok(list.L.Type == NonPagedPool, "Expected NonPagedPool got %u\n", list.L.Type);
    ExDeleteNPagedLookasideList(&list);

    ExInitializePagedLookasideList(&paged_list, NULL, NULL, POOL_NX_ALLOCATION, LOOKASIDE_MINIMUM_BLOCK_SIZE, tag, 0);
    ok(paged_list.L.Depth == 4, "Expected 4 got %u\n", paged_list.L.Depth);
    ok(paged_list.L.MaximumDepth == 256, "Expected 256 got %u\n", paged_list.L.MaximumDepth);
    ok(paged_list.L.TotalAllocates == 0, "Expected 0 got %lu\n", paged_list.L.TotalAllocates);
    ok(paged_list.L.AllocateMisses == 0, "Expected 0 got %lu\n", paged_list.L.AllocateMisses);
    ok(paged_list.L.TotalFrees == 0, "Expected 0 got %lu\n", paged_list.L.TotalFrees);
    ok(paged_list.L.FreeMisses == 0, "Expected 0 got %lu\n", paged_list.L.FreeMisses);
    ok(paged_list.L.Type == (PagedPool|POOL_NX_ALLOCATION),
       "Expected PagedPool|POOL_NX_ALLOCATION got %u\n", paged_list.L.Type);
    ok(paged_list.L.Tag == tag, "Expected %lx got %lx\n", tag, paged_list.L.Tag);
    ok(paged_list.L.Size == LOOKASIDE_MINIMUM_BLOCK_SIZE, "got %lu\n", paged_list.L.Size);
    ok(paged_list.L.LastTotalAllocates == 0,"Expected 0 got %lu\n", paged_list.L.LastTotalAllocates);
    ok(paged_list.L.LastAllocateMisses == 0,"Expected 0 got %lu\n", paged_list.L.LastAllocateMisses);
    ExDeletePagedLookasideList(&paged_list);

    paged_list.L.Depth = 0;
    ExInitializePagedLookasideList(&paged_list, NULL, NULL, 0, LOOKASIDE_MINIMUM_BLOCK_SIZE, tag, 20);
    ok(paged_list.L.Depth == 4, "Expected 4 got %u\n", paged_list.L.Depth);
    ok(paged_list.L.MaximumDepth == 256, "Expected 256 got %u\n", paged_list.L.MaximumDepth);
    ok(paged_list.L.Type == PagedPool, "Expected PagedPool got %u\n", paged_list.L.Type);
    ExDeletePagedLookasideList(&paged_list);
}

static void test_version(void)
{
    USHORT *pNtBuildNumber;
    ULONG build;

    pNtBuildNumber = get_proc_address("NtBuildNumber");
    ok(!!pNtBuildNumber, "Could not get pointer to NtBuildNumber\n");

    PsGetVersion(NULL, NULL, &build, NULL);
    ok(*pNtBuildNumber == build, "Expected build number %lu, got %u\n", build, *pNtBuildNumber);
}

static void WINAPI thread_proc(void *arg)
{
    PsTerminateSystemThread(STATUS_SUCCESS);
}

static void test_ob_reference(void)
{
    POBJECT_TYPE (WINAPI *pObGetObjectType)(void*);
    OBJECT_ATTRIBUTES attr = { sizeof(attr) };
    HANDLE event_handle, file_handle, file_handle2, thread_handle, handle;
    DISPATCHER_HEADER *header;
    FILE_OBJECT *file;
    void *obj1, *obj2;
    POBJECT_TYPE obj1_type;
    UNICODE_STRING pathU;
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    pObGetObjectType = get_proc_address("ObGetObjectType");
    if (!pObGetObjectType)
        win_skip("ObGetObjectType not found\n");

    InitializeObjectAttributes(&attr, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
    status = ZwCreateEvent(&event_handle, SYNCHRONIZE, &attr, NotificationEvent, TRUE);
    ok(!status, "ZwCreateEvent failed: %#lx\n", status);

    RtlInitUnicodeString(&pathU, L"\\??\\C:\\windows\\winetest_ntoskrnl_file.tmp");
    attr.ObjectName = &pathU;
    attr.Attributes = OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE;
    status = ZwCreateFile(&file_handle,  DELETE | FILE_WRITE_DATA | SYNCHRONIZE, &attr, &io, NULL, 0, 0, FILE_CREATE,
                          FILE_DELETE_ON_CLOSE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    ok(!status, "ZwCreateFile failed: %#lx\n", status);

    status = ZwDuplicateObject(NtCurrentProcess(), file_handle, NtCurrentProcess(), &file_handle2,
                               0, OBJ_KERNEL_HANDLE, DUPLICATE_SAME_ACCESS);
    ok(!status, "ZwDuplicateObject failed: %#lx\n", status);

    InitializeObjectAttributes(&attr, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
    status = PsCreateSystemThread(&thread_handle, SYNCHRONIZE, &attr, NULL, NULL, thread_proc, NULL);
    ok(!status, "PsCreateSystemThread returned: %#lx\n", status);

    status = ObReferenceObjectByHandle(NULL, SYNCHRONIZE, *pExEventObjectType, KernelMode, &obj1, NULL);
    ok(status == STATUS_INVALID_HANDLE, "ObReferenceObjectByHandle failed: %#lx\n", status);

    status = ObReferenceObjectByHandle(event_handle, SYNCHRONIZE, *pIoFileObjectType, KernelMode, &obj1, NULL);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH, "ObReferenceObjectByHandle returned: %#lx\n", status);

    status = ObReferenceObjectByHandle(event_handle, SYNCHRONIZE, NULL, KernelMode, &obj1, NULL);
    ok(!status, "ObReferenceObjectByHandle failed: %#lx\n", status);

    if (pObGetObjectType)
    {
        obj1_type = pObGetObjectType(obj1);
        ok(obj1_type == *pExEventObjectType, "ObGetObjectType returned %p\n", obj1_type);
    }

    if (sizeof(void *) != 4) /* avoid dealing with fastcall */
    {
        ObfReferenceObject(obj1);
        ObDereferenceObject(obj1);
    }

    status = ObReferenceObjectByHandle(event_handle, SYNCHRONIZE, *pIoFileObjectType, KernelMode, &obj2, NULL);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH, "ObReferenceObjectByHandle returned: %#lx\n", status);

    status = ObReferenceObjectByHandle(event_handle, SYNCHRONIZE, *pExEventObjectType, KernelMode, &obj2, NULL);
    ok(!status, "ObReferenceObjectByHandle failed: %#lx\n", status);
    ok(obj1 == obj2, "obj1 != obj2\n");

    ObDereferenceObject(obj2);

    status = ObReferenceObjectByHandle(event_handle, SYNCHRONIZE, NULL, KernelMode, &obj2, NULL);
    ok(!status, "ObReferenceObjectByHandle failed: %#lx\n", status);
    ok(obj1 == obj2, "obj1 != obj2\n");

    ObDereferenceObject(obj2);
    ObDereferenceObject(obj1);

    status = ObReferenceObjectByHandle(file_handle, SYNCHRONIZE, *pIoFileObjectType, KernelMode, &obj1, NULL);
    ok(!status, "ObReferenceObjectByHandle failed: %#lx\n", status);

    status = ObReferenceObjectByHandle(file_handle2, SYNCHRONIZE, *pIoFileObjectType, KernelMode, &obj2, NULL);
    ok(!status, "ObReferenceObjectByHandle failed: %#lx\n", status);
    ok(obj1 == obj2, "obj1 != obj2\n");

    file = obj1;
    ok(file->Type == 5, "Type = %u\n", file->Type);

    ObDereferenceObject(obj1);
    ObDereferenceObject(obj2);

    status = ObReferenceObjectByHandle(thread_handle, SYNCHRONIZE, *pPsThreadType, KernelMode, &obj1, NULL);
    ok(!status, "ObReferenceObjectByHandle failed: %#lx\n", status);

    status = ObReferenceObjectByHandle(thread_handle, SYNCHRONIZE, *pPsThreadType, KernelMode, &obj2, NULL);
    ok(!status, "ObReferenceObjectByHandle failed: %#lx\n", status);
    ok(obj1 == obj2, "obj1 != obj2\n");

    header = obj1;
    ok(header->Type == 6, "Type = %u\n", header->Type);

    status = wait_single(header, 0);
    ok(status == 0 || status == STATUS_TIMEOUT, "got %#lx\n", status);

    ObDereferenceObject(obj2);

    status = ObOpenObjectByPointer(obj1, OBJ_KERNEL_HANDLE, NULL, 0, NULL, KernelMode, &handle);
    ok(status == STATUS_SUCCESS, "ObOpenObjectByPointer failed: %#lx\n", status);

    status = ZwClose(handle);
    ok(!status, "ZwClose failed: %#lx\n", status);

    status = ObReferenceObjectByHandle(thread_handle, SYNCHRONIZE, *pPsThreadType, KernelMode, &obj2, NULL);
    ok(!status, "ObReferenceObjectByHandle failed: %#lx\n", status);
    ok(obj1 == obj2, "obj1 != obj2\n");
    ObDereferenceObject(obj2);

    status = ObOpenObjectByPointer(obj1, OBJ_KERNEL_HANDLE, NULL, 0, *pIoFileObjectType, KernelMode, &handle);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH, "ObOpenObjectByPointer returned: %#lx\n", status);

    ObDereferenceObject(obj1);

    status = ZwClose(thread_handle);
    ok(!status, "ZwClose failed: %#lx\n", status);

    status = ZwClose(event_handle);
    ok(!status, "ZwClose failed: %#lx\n", status);

    status = ZwClose(file_handle);
    ok(!status, "ZwClose failed: %#lx\n", status);

    status = ZwClose(file_handle2);
    ok(!status, "ZwClose failed: %#lx\n", status);
}

static void check_resource_(int line, ERESOURCE *resource, ULONG exclusive_waiters,
        ULONG shared_waiters, BOOLEAN exclusive, ULONG shared_count)
{
    BOOLEAN ret;
    ULONG count;

    count = ExGetExclusiveWaiterCount(resource);
    ok_(__FILE__, line)(count == exclusive_waiters,
            "expected %lu exclusive waiters, got %lu\n", exclusive_waiters, count);
    count = ExGetSharedWaiterCount(resource);
    ok_(__FILE__, line)(count == shared_waiters,
            "expected %lu shared waiters, got %lu\n", shared_waiters, count);
    ret = ExIsResourceAcquiredExclusiveLite(resource);
    ok_(__FILE__, line)(ret == exclusive,
            "expected exclusive %u, got %u\n", exclusive, ret);
    count = ExIsResourceAcquiredSharedLite(resource);
    ok_(__FILE__, line)(count == shared_count,
            "expected shared %lu, got %lu\n", shared_count, count);
}
#define check_resource(a,b,c,d,e) check_resource_(__LINE__,a,b,c,d,e)

static KEVENT resource_shared_ready, resource_shared_done, resource_exclusive_ready, resource_exclusive_done;

static void WINAPI resource_shared_thread(void *arg)
{
    ERESOURCE *resource = arg;
    BOOLEAN ret;

    check_resource(resource, 0, 0, FALSE, 0);

    ret = ExAcquireResourceSharedLite(resource, TRUE);
    ok(ret == TRUE, "got ret %u\n", ret);

    check_resource(resource, 0, 0, FALSE, 1);

    KeSetEvent(&resource_shared_ready, IO_NO_INCREMENT, FALSE);
    KeWaitForSingleObject(&resource_shared_done, Executive, KernelMode, FALSE, NULL);

    ExReleaseResourceForThreadLite(resource, (ULONG_PTR)PsGetCurrentThread());

    PsTerminateSystemThread(STATUS_SUCCESS);
}

static void WINAPI resource_exclusive_thread(void *arg)
{
    ERESOURCE *resource = arg;
    BOOLEAN ret;

    check_resource(resource, 0, 0, FALSE, 0);

    ret = ExAcquireResourceExclusiveLite(resource, TRUE);
    ok(ret == TRUE, "got ret %u\n", ret);

    check_resource(resource, 0, 0, TRUE, 1);

    KeSetEvent(&resource_exclusive_ready, IO_NO_INCREMENT, FALSE);
    KeWaitForSingleObject(&resource_exclusive_done, Executive, KernelMode, FALSE, NULL);

    ExReleaseResourceForThreadLite(resource, (ULONG_PTR)PsGetCurrentThread());

    PsTerminateSystemThread(STATUS_SUCCESS);
}

static void test_resource(void)
{
    ERESOURCE resource;
    NTSTATUS status;
    BOOLEAN ret;
    HANDLE thread, thread2;

    memset(&resource, 0xcc, sizeof(resource));

    status = ExInitializeResourceLite(&resource);
    ok(status == STATUS_SUCCESS, "got status %#lx\n", status);
    check_resource(&resource, 0, 0, FALSE, 0);

    KeEnterCriticalRegion();

    ret = ExAcquireResourceExclusiveLite(&resource, FALSE);
    ok(ret == TRUE, "got ret %u\n", ret);
    check_resource(&resource, 0, 0, TRUE, 1);

    ret = ExAcquireResourceExclusiveLite(&resource, FALSE);
    ok(ret == TRUE, "got ret %u\n", ret);
    check_resource(&resource, 0, 0, TRUE, 2);

    ret = ExAcquireResourceSharedLite(&resource, FALSE);
    ok(ret == TRUE, "got ret %u\n", ret);
    check_resource(&resource, 0, 0, TRUE, 3);

    ExReleaseResourceForThreadLite(&resource, (ULONG_PTR)PsGetCurrentThread());
    check_resource(&resource, 0, 0, TRUE, 2);

    ExReleaseResourceForThreadLite(&resource, (ULONG_PTR)PsGetCurrentThread());
    check_resource(&resource, 0, 0, TRUE, 1);

    ExReleaseResourceForThreadLite(&resource, (ULONG_PTR)PsGetCurrentThread());
    check_resource(&resource, 0, 0, FALSE, 0);

    ret = ExAcquireResourceSharedLite(&resource, FALSE);
    ok(ret == TRUE, "got ret %u\n", ret);
    check_resource(&resource, 0, 0, FALSE, 1);

    ret = ExAcquireResourceSharedLite(&resource, FALSE);
    ok(ret == TRUE, "got ret %u\n", ret);
    check_resource(&resource, 0, 0, FALSE, 2);

    ret = ExAcquireResourceExclusiveLite(&resource, FALSE);
    ok(ret == FALSE, "got ret %u\n", ret);
    check_resource(&resource, 0, 0, FALSE, 2);

    ExReleaseResourceForThreadLite(&resource, (ULONG_PTR)PsGetCurrentThread());
    check_resource(&resource, 0, 0, FALSE, 1);

    ExReleaseResourceForThreadLite(&resource, (ULONG_PTR)PsGetCurrentThread());
    check_resource(&resource, 0, 0, FALSE, 0);

    ret = ExAcquireSharedStarveExclusive(&resource, FALSE);
    ok(ret == TRUE, "got ret %u\n", ret);
    check_resource(&resource, 0, 0, FALSE, 1);

    ExReleaseResourceForThreadLite(&resource, (ULONG_PTR)PsGetCurrentThread());
    check_resource(&resource, 0, 0, FALSE, 0);

    ret = ExAcquireSharedWaitForExclusive(&resource, FALSE);
    ok(ret == TRUE, "got ret %u\n", ret);
    check_resource(&resource, 0, 0, FALSE, 1);

    ExReleaseResourceForThreadLite(&resource, (ULONG_PTR)PsGetCurrentThread());
    check_resource(&resource, 0, 0, FALSE, 0);

    /* Do not acquire the resource ourselves, but spawn a shared thread holding it. */

    KeInitializeEvent(&resource_shared_ready, SynchronizationEvent, FALSE);
    KeInitializeEvent(&resource_shared_done, SynchronizationEvent, FALSE);
    thread = create_thread(resource_shared_thread, &resource);
    KeWaitForSingleObject(&resource_shared_ready, Executive, KernelMode, FALSE, NULL);
    check_resource(&resource, 0, 0, FALSE, 0);

    ret = ExAcquireResourceExclusiveLite(&resource, FALSE);
    ok(ret == FALSE, "got ret %u\n", ret);

    ret = ExAcquireResourceSharedLite(&resource, FALSE);
    ok(ret == TRUE, "got ret %u\n", ret);
    check_resource(&resource, 0, 0, FALSE, 1);

    ExReleaseResourceForThreadLite(&resource, (ULONG_PTR)PsGetCurrentThread());
    check_resource(&resource, 0, 0, FALSE, 0);

    ret = ExAcquireSharedStarveExclusive(&resource, FALSE);
    ok(ret == TRUE, "got ret %u\n", ret);
    check_resource(&resource, 0, 0, FALSE, 1);

    ExReleaseResourceForThreadLite(&resource, (ULONG_PTR)PsGetCurrentThread());
    check_resource(&resource, 0, 0, FALSE, 0);

    ret = ExAcquireSharedWaitForExclusive(&resource, FALSE);
    ok(ret == TRUE, "got ret %u\n", ret);
    check_resource(&resource, 0, 0, FALSE, 1);

    ExReleaseResourceForThreadLite(&resource, (ULONG_PTR)PsGetCurrentThread());
    check_resource(&resource, 0, 0, FALSE, 0);

    KeSetEvent(&resource_shared_done, IO_NO_INCREMENT, FALSE);
    join_thread(thread);
    check_resource(&resource, 0, 0, FALSE, 0);

    /* Acquire the resource as exclusive, and then spawn a shared thread. */

    ret = ExAcquireResourceExclusiveLite(&resource, FALSE);
    ok(ret == TRUE, "got ret %u\n", ret);
    check_resource(&resource, 0, 0, TRUE, 1);

    thread = create_thread(resource_shared_thread, &resource);
    sleep();
    check_resource(&resource, 0, 1, TRUE, 1);

    ret = ExAcquireResourceExclusiveLite(&resource, FALSE);
    ok(ret == TRUE, "got ret %u\n", ret);
    check_resource(&resource, 0, 1, TRUE, 2);

    ExReleaseResourceForThreadLite(&resource, (ULONG_PTR)PsGetCurrentThread());
    ExReleaseResourceForThreadLite(&resource, (ULONG_PTR)PsGetCurrentThread());
    KeWaitForSingleObject(&resource_shared_ready, Executive, KernelMode, FALSE, NULL);
    KeSetEvent(&resource_shared_done, IO_NO_INCREMENT, FALSE);
    join_thread(thread);
    check_resource(&resource, 0, 0, FALSE, 0);

    /* Do not acquire the resource ourselves, but spawn an exclusive thread holding it. */

    KeInitializeEvent(&resource_exclusive_ready, SynchronizationEvent, FALSE);
    KeInitializeEvent(&resource_exclusive_done, SynchronizationEvent, FALSE);
    thread = create_thread(resource_exclusive_thread, &resource);
    KeWaitForSingleObject(&resource_exclusive_ready, Executive, KernelMode, FALSE, NULL);
    check_resource(&resource, 0, 0, FALSE, 0);

    ret = ExAcquireResourceExclusiveLite(&resource, FALSE);
    ok(ret == FALSE, "got ret %u\n", ret);
    check_resource(&resource, 0, 0, FALSE, 0);

    ret = ExAcquireResourceSharedLite(&resource, FALSE);
    ok(ret == FALSE, "got ret %u\n", ret);
    check_resource(&resource, 0, 0, FALSE, 0);

    ret = ExAcquireSharedStarveExclusive(&resource, FALSE);
    ok(ret == FALSE, "got ret %u\n", ret);
    check_resource(&resource, 0, 0, FALSE, 0);

    ret = ExAcquireSharedWaitForExclusive(&resource, FALSE);
    ok(ret == FALSE, "got ret %u\n", ret);
    check_resource(&resource, 0, 0, FALSE, 0);

    KeSetEvent(&resource_exclusive_done, IO_NO_INCREMENT, FALSE);
    join_thread(thread);
    check_resource(&resource, 0, 0, FALSE, 0);

    /* Acquire the resource as shared, and then spawn an exclusive waiter. */

    ret = ExAcquireResourceSharedLite(&resource, FALSE);
    ok(ret == TRUE, "got ret %u\n", ret);
    check_resource(&resource, 0, 0, FALSE, 1);

    thread = create_thread(resource_exclusive_thread, &resource);
    sleep();
    check_resource(&resource, 1, 0, FALSE, 1);

    ret = ExAcquireResourceSharedLite(&resource, FALSE);
    ok(ret == TRUE, "got ret %u\n", ret);
    check_resource(&resource, 1, 0, FALSE, 2);
    ExReleaseResourceForThreadLite(&resource, (ULONG_PTR)PsGetCurrentThread());

    ret = ExAcquireSharedStarveExclusive(&resource, FALSE);
    ok(ret == TRUE, "got ret %u\n", ret);
    check_resource(&resource, 1, 0, FALSE, 2);
    ExReleaseResourceForThreadLite(&resource, (ULONG_PTR)PsGetCurrentThread());

    ret = ExAcquireSharedWaitForExclusive(&resource, FALSE);
    ok(ret == FALSE, "got ret %u\n", ret);
    check_resource(&resource, 1, 0, FALSE, 1);

    ExReleaseResourceForThreadLite(&resource, (ULONG_PTR)PsGetCurrentThread());
    KeWaitForSingleObject(&resource_exclusive_ready, Executive, KernelMode, FALSE, NULL);
    KeSetEvent(&resource_exclusive_done, IO_NO_INCREMENT, FALSE);
    join_thread(thread);
    check_resource(&resource, 0, 0, FALSE, 0);

    /* Spawn a shared and then exclusive waiter. */

    KeInitializeEvent(&resource_shared_ready, SynchronizationEvent, FALSE);
    KeInitializeEvent(&resource_shared_done, SynchronizationEvent, FALSE);
    thread = create_thread(resource_shared_thread, &resource);
    KeWaitForSingleObject(&resource_shared_ready, Executive, KernelMode, FALSE, NULL);
    check_resource(&resource, 0, 0, FALSE, 0);

    thread2 = create_thread(resource_exclusive_thread, &resource);
    sleep();
    check_resource(&resource, 1, 0, FALSE, 0);

    ret = ExAcquireResourceExclusiveLite(&resource, FALSE);
    ok(ret == FALSE, "got ret %u\n", ret);
    check_resource(&resource, 1, 0, FALSE, 0);

    ret = ExAcquireResourceSharedLite(&resource, FALSE);
    ok(ret == FALSE, "got ret %u\n", ret);
    check_resource(&resource, 1, 0, FALSE, 0);

    ret = ExAcquireSharedStarveExclusive(&resource, FALSE);
    ok(ret == TRUE, "got ret %u\n", ret);
    check_resource(&resource, 1, 0, FALSE, 1);
    ExReleaseResourceForThreadLite(&resource, (ULONG_PTR)PsGetCurrentThread());

    ret = ExAcquireSharedWaitForExclusive(&resource, FALSE);
    ok(ret == FALSE, "got ret %u\n", ret);
    check_resource(&resource, 1, 0, FALSE, 0);

    KeSetEvent(&resource_shared_done, IO_NO_INCREMENT, FALSE);
    join_thread(thread);
    KeWaitForSingleObject(&resource_exclusive_ready, Executive, KernelMode, FALSE, NULL);
    KeSetEvent(&resource_exclusive_done, IO_NO_INCREMENT, FALSE);
    join_thread(thread2);
    check_resource(&resource, 0, 0, FALSE, 0);

    KeLeaveCriticalRegion();

    status = ExDeleteResourceLite(&resource);
    ok(status == STATUS_SUCCESS, "got status %#lx\n", status);
}

static void test_lookup_thread(void)
{
    NTSTATUS status;
    PETHREAD thread = NULL;

    status = PsLookupThreadByThreadId(PsGetCurrentThreadId(), &thread);
    ok(!status, "PsLookupThreadByThreadId failed: %#lx\n", status);
    ok((PKTHREAD)thread == KeGetCurrentThread(), "thread != KeGetCurrentThread\n");
    if (thread) ObDereferenceObject(thread);

    status = PsLookupThreadByThreadId(NULL, &thread);
    ok(status == STATUS_INVALID_CID || broken(status == STATUS_INVALID_PARAMETER) /* winxp */,
       "PsLookupThreadByThreadId returned %#lx\n", status);
}

static void test_stack_limits(void)
{
    ULONG_PTR low = 0, high = 0;

    IoGetStackLimits(&low, &high);
    ok(low, "low = 0\n");
    ok(low < high, "low >= high\n");
    ok(low < (ULONG_PTR)&low && (ULONG_PTR)&low < high, "stack variable is not in stack limits\n");
}

static unsigned int got_completion, completion_lower_pending, completion_upper_pending;

static NTSTATUS WINAPI completion_cb(DEVICE_OBJECT *device, IRP *irp, void *context)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    ok(device == context, "Got device %p; expected %p.\n", device, context);

    if (device == upper_device)
    {
        ok(irp->PendingReturned == completion_lower_pending, "Got PendingReturned %u, expected %u.\n",
                irp->PendingReturned, completion_lower_pending);

        ok(irp->CurrentLocation == 2, "Got current location %u.\n", irp->CurrentLocation);
        ok(stack->Control == (SL_INVOKE_ON_CANCEL | SL_INVOKE_ON_ERROR | SL_INVOKE_ON_SUCCESS),
                "Got control flags %#x.\n", stack->Control);
        stack = IoGetNextIrpStackLocation(irp);
        ok(!stack->Control, "Got control flags %#x.\n", stack->Control);
        stack = irp->Tail.Overlay.CurrentStackLocation + 1; /* previous location */
        ok(!stack->Control, "Got control flags %#x.\n", stack->Control);

        if (irp->PendingReturned && completion_upper_pending)
            IoMarkIrpPending(irp);
    }
    else
    {
        ok(irp->PendingReturned == completion_upper_pending, "Got PendingReturned %u, expected %u.\n",
                irp->PendingReturned, completion_upper_pending);

        ok(irp->CurrentLocation == 3, "Got current location %u.\n", irp->CurrentLocation);
        ok(!stack->Control, "Got control flags %#x.\n", stack->Control);
        stack = IoGetNextIrpStackLocation(irp);
        ok(!stack->Control, "Got control flags %#x.\n", stack->Control);
        stack = irp->Tail.Overlay.CurrentStackLocation - 2; /* lowest location */
        ok(!stack->Control, "Got control flags %#x.\n", stack->Control);
    }

    ++got_completion;
    return STATUS_SUCCESS;
}

static void test_completion(void)
{
    IO_STATUS_BLOCK io;
    NTSTATUS ret;
    KEVENT event;
    IRP *irp;

    completion_lower_pending = completion_upper_pending = FALSE;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_WINETEST_COMPLETION, upper_device,
            NULL, 0, NULL, 0, FALSE, &event, &io);
    IoSetCompletionRoutine(irp, completion_cb, NULL, TRUE, TRUE, TRUE);
    ret = IoCallDriver(upper_device, irp);
    ok(ret == STATUS_SUCCESS, "IoCallDriver returned %#lx\n", ret);
    ok(got_completion == 2, "got %u calls to completion routine\n", got_completion);

    completion_lower_pending = TRUE;
    got_completion = 0;

    irp = IoBuildDeviceIoControlRequest(IOCTL_WINETEST_COMPLETION, upper_device,
            NULL, 0, NULL, 0, FALSE, &event, &io);
    IoSetCompletionRoutine(irp, completion_cb, NULL, TRUE, TRUE, TRUE);
    ret = IoCallDriver(upper_device, irp);
    ok(ret == STATUS_PENDING, "IoCallDriver returned %#lx\n", ret);
    ok(!got_completion, "got %u calls to completion routine\n", got_completion);

    ok(irp->CurrentLocation == 1, "Got current location %u.\n", irp->CurrentLocation);
    ok(!irp->PendingReturned, "Got pending flag %u.\n", irp->PendingReturned);

    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    ok(got_completion == 2, "got %u calls to completion routine\n", got_completion);

    completion_upper_pending = TRUE;
    got_completion = 0;

    irp = IoBuildDeviceIoControlRequest(IOCTL_WINETEST_COMPLETION, upper_device,
            NULL, 0, NULL, 0, FALSE, &event, &io);
    IoSetCompletionRoutine(irp, completion_cb, NULL, TRUE, TRUE, TRUE);
    ret = IoCallDriver(upper_device, irp);
    ok(ret == STATUS_PENDING, "IoCallDriver returned %#lx\n", ret);
    ok(!got_completion, "got %u calls to completion routine\n", got_completion);

    ok(irp->CurrentLocation == 1, "Got current location %u.\n", irp->CurrentLocation);
    ok(!irp->PendingReturned, "Got pending flag %u.\n", irp->PendingReturned);

    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    ok(got_completion == 2, "got %u calls to completion routine\n", got_completion);
}

static void test_IoAttachDeviceToDeviceStack(void)
{
    DEVICE_OBJECT *dev1, *dev2, *dev3, *ret;
    NTSTATUS status;

    status = IoCreateDevice(driver_obj, 0, NULL, FILE_DEVICE_UNKNOWN,
            FILE_DEVICE_SECURE_OPEN, FALSE, &dev1);
    ok(status == STATUS_SUCCESS, "IoCreateDevice failed\n");
    status = IoCreateDevice(driver_obj, 0, NULL, FILE_DEVICE_UNKNOWN,
            FILE_DEVICE_SECURE_OPEN, FALSE, &dev2);
    ok(status == STATUS_SUCCESS, "IoCreateDevice failed\n");
    status = IoCreateDevice(driver_obj, 0, NULL, FILE_DEVICE_UNKNOWN,
            FILE_DEVICE_SECURE_OPEN, FALSE, &dev3);
    ok(status == STATUS_SUCCESS, "IoCreateDevice failed\n");

    /* TODO: initialize devices properly */
    dev1->Flags &= ~DO_DEVICE_INITIALIZING;
    dev2->Flags &= ~DO_DEVICE_INITIALIZING;

    ret = IoAttachDeviceToDeviceStack(dev2, dev1);
    ok(ret == dev1, "IoAttachDeviceToDeviceStack returned %p, expected %p\n", ret, dev1);
    ok(dev1->AttachedDevice == dev2, "dev1->AttachedDevice = %p, expected %p\n",
            dev1->AttachedDevice, dev2);
    ok(!dev2->AttachedDevice, "dev2->AttachedDevice = %p\n", dev2->AttachedDevice);
    ok(dev1->StackSize == 1, "dev1->StackSize = %d\n", dev1->StackSize);
    ok(dev2->StackSize == 2, "dev2->StackSize = %d\n", dev2->StackSize);

    ret = IoAttachDeviceToDeviceStack(dev3, dev1);
    ok(ret == dev2, "IoAttachDeviceToDeviceStack returned %p, expected %p\n", ret, dev2);
    ok(dev1->AttachedDevice == dev2, "dev1->AttachedDevice = %p, expected %p\n",
            dev1->AttachedDevice, dev2);
    ok(dev2->AttachedDevice == dev3, "dev2->AttachedDevice = %p, expected %p\n",
            dev2->AttachedDevice, dev3);
    ok(!dev3->AttachedDevice, "dev3->AttachedDevice = %p\n", dev3->AttachedDevice);
    ok(dev1->StackSize == 1, "dev1->StackSize = %d\n", dev1->StackSize);
    ok(dev2->StackSize == 2, "dev2->StackSize = %d\n", dev2->StackSize);
    ok(dev3->StackSize == 3, "dev3->StackSize = %d\n", dev3->StackSize);

    IoDetachDevice(dev1);
    ok(!dev1->AttachedDevice, "dev1->AttachedDevice = %p\n", dev1->AttachedDevice);
    ok(dev2->AttachedDevice == dev3, "dev2->AttachedDevice = %p\n", dev2->AttachedDevice);

    IoDetachDevice(dev2);
    ok(!dev2->AttachedDevice, "dev2->AttachedDevice = %p\n", dev2->AttachedDevice);
    ok(dev1->StackSize == 1, "dev1->StackSize = %d\n", dev1->StackSize);
    ok(dev2->StackSize == 2, "dev2->StackSize = %d\n", dev2->StackSize);
    ok(dev3->StackSize == 3, "dev3->StackSize = %d\n", dev3->StackSize);

    IoDeleteDevice(dev1);
    IoDeleteDevice(dev2);
    IoDeleteDevice(dev3);
}

static void test_object_name(void)
{
    static const WCHAR event_nameW[] = L"\\wine_test_event";
    static const WCHAR device_nameW[] = L"\\Device\\WineTestDriver";
    char buffer[1024];
    OBJECT_NAME_INFORMATION *name = (OBJECT_NAME_INFORMATION *)buffer;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING string;
    ULONG ret_size;
    HANDLE handle;
    KEVENT *event;
    NTSTATUS ret;

    ret_size = 0;
    ret = ObQueryNameString(lower_device, name, 0, &ret_size);
    ok(ret == STATUS_INFO_LENGTH_MISMATCH, "got status %#lx\n", ret);
    ok(ret_size == sizeof(*name) + sizeof(device_nameW), "got size %lu\n", ret_size);

    ret_size = 0;
    ret = ObQueryNameString(lower_device, name, sizeof(buffer), &ret_size);
    ok(!ret, "got status %#lx\n", ret);
    ok(!wcscmp(name->Name.Buffer, device_nameW), "got name %ls\n", name->Name.Buffer);
    ok(ret_size == sizeof(*name) + sizeof(device_nameW), "got size %lu\n", ret_size);
    ok(name->Name.Length == wcslen(device_nameW) * sizeof(WCHAR), "got length %u\n", name->Name.Length);
    ok(name->Name.MaximumLength == sizeof(device_nameW), "got maximum length %u\n", name->Name.MaximumLength);

    event = IoCreateSynchronizationEvent(NULL, &handle);
    ok(!!event, "failed to create event\n");

    ret_size = 0;
    ret = ObQueryNameString(event, name, sizeof(buffer), &ret_size);
    ok(!ret, "got status %#lx\n", ret);
    ok(!name->Name.Buffer, "got name %ls\n", name->Name.Buffer);
    ok(ret_size == sizeof(*name), "got size %lu\n", ret_size);
    ok(!name->Name.Length, "got length %u\n", name->Name.Length);
    ok(!name->Name.MaximumLength, "got maximum length %u\n", name->Name.MaximumLength);

    ret = ZwClose(handle);
    ok(!ret, "got status %#lx\n", ret);

    RtlInitUnicodeString(&string, event_nameW);
    InitializeObjectAttributes(&attr, &string, OBJ_KERNEL_HANDLE, NULL, NULL);
    ret = ZwCreateEvent(&handle, 0, &attr, NotificationEvent, TRUE);
    ok(!ret, "got status %#lx\n", ret);
    ret = ObReferenceObjectByHandle(handle, 0, *pExEventObjectType, KernelMode, (void **)&event, NULL);
    ok(!ret, "got status %#lx\n", ret);

    ret_size = 0;
    ret = ObQueryNameString(event, name, sizeof(buffer), &ret_size);
    ok(!ret, "got status %#lx\n", ret);
    ok(!wcscmp(name->Name.Buffer, event_nameW), "got name %ls\n", name->Name.Buffer);
    ok(ret_size == sizeof(*name) + sizeof(event_nameW), "got size %lu\n", ret_size);
    ok(name->Name.Length == wcslen(event_nameW) * sizeof(WCHAR), "got length %u\n", name->Name.Length);
    ok(name->Name.MaximumLength == sizeof(event_nameW), "got maximum length %u\n", name->Name.MaximumLength);

    ObDereferenceObject(event);
    ret = ZwClose(handle);
    ok(!ret, "got status %#lx\n", ret);

    ret_size = 0;
    ret = ObQueryNameString(KeGetCurrentThread(), name, sizeof(buffer), &ret_size);
    ok(!ret, "got status %#lx\n", ret);
    ok(!name->Name.Buffer, "got name %ls\n", name->Name.Buffer);
    ok(ret_size == sizeof(*name), "got size %lu\n", ret_size);
    ok(!name->Name.Length, "got length %u\n", name->Name.Length);
    ok(!name->Name.MaximumLength, "got maximum length %u\n", name->Name.MaximumLength);

    ret_size = 0;
    ret = ObQueryNameString(IoGetCurrentProcess(), name, sizeof(buffer), &ret_size);
    ok(!ret, "got status %#lx\n", ret);
    ok(!name->Name.Buffer, "got name %ls\n", name->Name.Buffer);
    ok(ret_size == sizeof(*name), "got size %lu\n", ret_size);
    ok(!name->Name.Length, "got length %u\n", name->Name.Length);
    ok(!name->Name.MaximumLength, "got maximum length %u\n", name->Name.MaximumLength);
}

static PIO_WORKITEM work_item;

static void WINAPI main_test_task(DEVICE_OBJECT *device, void *context)
{
    IRP *irp = context;

    test_current_thread(TRUE);
    test_critical_region(FALSE);
    test_call_driver(device);
    test_cancel_irp(device);
    test_stack_limits();
    test_completion();

    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
}

#if defined(__i386__) || defined(__x86_64__)
static void test_executable_pool(void)
{
    static const unsigned char bytes[] =
            { 0xb8, 0xef, 0xbe, 0xad, 0xde, 0xc3 }; /* mov $0xdeadbeef,%eax ; ret */
    static const ULONG tag = 0x74736574; /* test */
    int (*func)(void);
    int ret;

    func = ExAllocatePoolWithTag(NonPagedPool, sizeof(bytes), tag);
    ok(!!func, "Got NULL memory.\n");

    memcpy(func, bytes, sizeof(bytes));
    ret = func();
    ok(ret == 0xdeadbeef, "Got %#x.\n", ret);

    ExFreePoolWithTag(func, tag);
}
#endif

static void test_affinity(void)
{
    KAFFINITY (WINAPI *pKeSetSystemAffinityThreadEx)(KAFFINITY affinity);
    void (WINAPI *pKeRevertToUserAffinityThreadEx)(KAFFINITY affinity);
    ULONG (WINAPI *pKeQueryActiveProcessorCountEx)(USHORT);
    KAFFINITY (WINAPI *pKeQueryActiveProcessors)(void);
    KAFFINITY mask, mask_all_cpus;
    ULONG cpu_count, count;

    pKeQueryActiveProcessorCountEx = get_proc_address("KeQueryActiveProcessorCountEx");
    if (!pKeQueryActiveProcessorCountEx)
    {
        win_skip("KeQueryActiveProcessorCountEx is not available.\n");
        return;
    }

    pKeQueryActiveProcessors = get_proc_address("KeQueryActiveProcessors");
    ok(!!pKeQueryActiveProcessors, "KeQueryActiveProcessors is not available.\n");

    pKeSetSystemAffinityThreadEx = get_proc_address("KeSetSystemAffinityThreadEx");
    ok(!!pKeSetSystemAffinityThreadEx, "KeSetSystemAffinityThreadEx is not available.\n");

    pKeRevertToUserAffinityThreadEx = get_proc_address("KeRevertToUserAffinityThreadEx");
    ok(!!pKeRevertToUserAffinityThreadEx, "KeRevertToUserAffinityThreadEx is not available.\n");

    count = pKeQueryActiveProcessorCountEx(1);
    ok(!count, "Got unexpected count %lu.\n", count);

    cpu_count = pKeQueryActiveProcessorCountEx(0);
    ok(cpu_count, "Got unexpected cpu_count %lu.\n", cpu_count);

    count = pKeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS);
    ok(count == cpu_count, "Got unexpected count %lu.\n", count);

    if (cpu_count >= 8 * sizeof(KAFFINITY))
        mask_all_cpus = ~(KAFFINITY)0;
    else
        mask_all_cpus = ((KAFFINITY)1 << cpu_count) - 1;

    mask = pKeQueryActiveProcessors();
    ok(mask == mask_all_cpus, "Got unexpected mask %#Ix.\n", mask);

    pKeRevertToUserAffinityThreadEx(0x2);

    mask = pKeSetSystemAffinityThreadEx(0);
    ok(!mask, "Got unexpected mask %#Ix.\n", mask);

    pKeRevertToUserAffinityThreadEx(0x2);

    mask = pKeSetSystemAffinityThreadEx(0x1);
    ok(mask == 0x2, "Got unexpected mask %#Ix.\n", mask);

    mask = pKeSetSystemAffinityThreadEx(~(KAFFINITY)0);
    ok(mask == 0x1, "Got unexpected mask %#Ix.\n", mask);

    pKeRevertToUserAffinityThreadEx(~(KAFFINITY)0);
    mask = pKeSetSystemAffinityThreadEx(0x1);
    ok(mask == mask_all_cpus, "Got unexpected mask %#Ix.\n", mask);

    pKeRevertToUserAffinityThreadEx(0);

    mask = pKeSetSystemAffinityThreadEx(0x1);
    ok(!mask, "Got unexpected mask %#Ix.\n", mask);

    KeRevertToUserAffinityThread();

    mask = pKeSetSystemAffinityThreadEx(0x1);
    ok(!mask, "Got unexpected mask %#Ix.\n", mask);

    KeRevertToUserAffinityThread();
}

struct test_dpc_func_context
{
    volatile LONG call_count;
    volatile LONG selected_count;
    volatile DEFERRED_REVERSE_BARRIER sync_barrier_start_value, sync_barrier_mid_value, sync_barrier_end_value;
    volatile LONG done_barrier_start_value;
};

static BOOLEAN (WINAPI *pKeSignalCallDpcSynchronize)(void *barrier);
static void (WINAPI *pKeSignalCallDpcDone)(void *barrier);

static void WINAPI test_dpc_func(PKDPC Dpc, void *context, void *cpu_count,
        void *reverse_barrier)
{
    DEFERRED_REVERSE_BARRIER *barrier = reverse_barrier;
    struct test_dpc_func_context *data = context;

    InterlockedIncrement(&data->call_count);

    InterlockedCompareExchange((volatile LONG*)&data->sync_barrier_start_value.Barrier,
            *(volatile LONG *)&barrier->Barrier, 0);
    InterlockedCompareExchange((volatile LONG*)&data->sync_barrier_start_value.TotalProcessors,
            *(volatile LONG *)&barrier->TotalProcessors, 0);

    if (pKeSignalCallDpcSynchronize(reverse_barrier))
        InterlockedIncrement(&data->selected_count);

    InterlockedCompareExchange((volatile LONG*)&data->sync_barrier_mid_value.Barrier,
            *(volatile LONG *)&barrier->Barrier, 0);
    InterlockedCompareExchange((volatile LONG*)&data->sync_barrier_mid_value.TotalProcessors,
            *(volatile LONG *)&barrier->TotalProcessors, 0);

    data->done_barrier_start_value =  *(volatile LONG *)cpu_count;

    if (pKeSignalCallDpcSynchronize(reverse_barrier))
        InterlockedIncrement(&data->selected_count);

    pKeSignalCallDpcSynchronize(reverse_barrier);
    pKeSignalCallDpcSynchronize(reverse_barrier);

    InterlockedCompareExchange((volatile LONG*)&data->sync_barrier_end_value.Barrier,
            *(volatile LONG *)&barrier->Barrier, 0);
    InterlockedCompareExchange((volatile LONG*)&data->sync_barrier_end_value.TotalProcessors,
            *(volatile LONG *)&barrier->TotalProcessors, 0);

    pKeSignalCallDpcDone(cpu_count);
}

static void test_dpc(void)
{
    void (WINAPI *pKeGenericCallDpc)(PKDEFERRED_ROUTINE routine, void *context);
    void (WINAPI *pKeInitializeDpc)(PKDPC dpc, PKDEFERRED_ROUTINE routine, void *context);
    struct test_dpc_func_context data;
    KAFFINITY cpu_mask;
    ULONG cpu_count;
    struct _KDPC dpc = {0};

    pKeInitializeDpc = get_proc_address("KeInitializeDpc");
    if(!pKeInitializeDpc)
    {
        win_skip("KeInitializeDpc is not available.\n");
        return;
    }

    pKeInitializeDpc(&dpc, test_dpc_func, &data);

    ok(dpc.Number == 0, "Got unexpected Dpc Number %u.\n", dpc.Number);
    todo_wine ok(dpc.Type == 0x13, "Got unexpected Dpc Type %u.\n", dpc.Type);
    todo_wine ok(dpc.Importance == MediumImportance, "Got unexpected Dpc Importance %u.\n", dpc.Importance);
    ok(dpc.DeferredRoutine == test_dpc_func, "Got unexpected Dpc DeferredRoutine %p.\n", dpc.DeferredRoutine);
    ok(dpc.DeferredContext == &data, "Got unexpected Dpc DeferredContext %p.\n", dpc.DeferredContext);

    pKeGenericCallDpc = get_proc_address("KeGenericCallDpc");
    if (!pKeGenericCallDpc)
    {
        win_skip("KeGenericCallDpc is not available.\n");
        return;
    }

    pKeSignalCallDpcDone = get_proc_address("KeSignalCallDpcDone");
    ok(!!pKeSignalCallDpcDone, "KeSignalCallDpcDone is not available.\n");
    pKeSignalCallDpcSynchronize = get_proc_address("KeSignalCallDpcSynchronize");
    ok(!!pKeSignalCallDpcSynchronize, "KeSignalCallDpcSynchronize is not available.\n");


    cpu_mask = KeQueryActiveProcessors();
    cpu_count = 0;
    while (cpu_mask)
    {
        if (cpu_mask & 1)
            ++cpu_count;

        cpu_mask >>= 1;
    }

    memset(&data, 0, sizeof(data));

    KeSetSystemAffinityThread(0x1);

    pKeGenericCallDpc(test_dpc_func, &data);
    ok(data.call_count == cpu_count, "Got unexpected call_count %lu.\n", data.call_count);
    ok(data.selected_count == 2, "Got unexpected selected_count %lu.\n", data.selected_count);
    ok(data.sync_barrier_start_value.Barrier == cpu_count,
            "Got unexpected sync_barrier_start_value.Barrier %ld.\n",
            data.sync_barrier_start_value.Barrier);
    ok(data.sync_barrier_start_value.TotalProcessors == cpu_count,
            "Got unexpected sync_barrier_start_value.TotalProcessors %ld.\n",
            data.sync_barrier_start_value.TotalProcessors);

    ok(data.sync_barrier_mid_value.Barrier == (0x80000000 | cpu_count),
            "Got unexpected sync_barrier_mid_value.Barrier %ld.\n",
            data.sync_barrier_mid_value.Barrier);
    ok(data.sync_barrier_mid_value.TotalProcessors == cpu_count,
            "Got unexpected sync_barrier_mid_value.TotalProcessors %ld.\n",
            data.sync_barrier_mid_value.TotalProcessors);

    ok(data.sync_barrier_end_value.Barrier == cpu_count,
            "Got unexpected sync_barrier_end_value.Barrier %ld.\n",
            data.sync_barrier_end_value.Barrier);
    ok(data.sync_barrier_end_value.TotalProcessors == cpu_count,
            "Got unexpected sync_barrier_end_value.TotalProcessors %ld.\n",
            data.sync_barrier_end_value.TotalProcessors);

    ok(data.done_barrier_start_value == cpu_count, "Got unexpected done_barrier_start_value %ld.\n", data.done_barrier_start_value);

    KeRevertToUserAffinityThread();
}

static void test_process_memory(const struct main_test_input *test_input)
{
    NTSTATUS (WINAPI *pMmCopyVirtualMemory)(PEPROCESS fromprocess, void *fromaddress, PEPROCESS toprocess,
            void *toaddress, SIZE_T bufsize, KPROCESSOR_MODE mode, SIZE_T *copied);
    char buffer[sizeof(teststr)];
    ULONG64 modified_value;
    PEPROCESS process;
    KAPC_STATE state;
    NTSTATUS status;
    SIZE_T size;
    BYTE *base;

    pMmCopyVirtualMemory = get_proc_address("MmCopyVirtualMemory");

    status = PsLookupProcessByProcessId((HANDLE)(ULONG_PTR)test_input->process_id, &process);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);

    if (status)
        return;

    if (0) /* Crashes on Windows. */
        PsGetProcessSectionBaseAddress(NULL);

    base = PsGetProcessSectionBaseAddress(process);
    ok(!!base, "Got NULL base address.\n");

    ok(process == PsGetCurrentProcess(), "Got unexpected process %p, PsGetCurrentProcess() %p.\n",
            process, PsGetCurrentProcess());

    modified_value = 0xdeadbeeffeedcafe;
    if (pMmCopyVirtualMemory)
    {
        size = 0xdeadbeef;
        status = pMmCopyVirtualMemory(process, base + test_input->teststr_offset, PsGetCurrentProcess(),
                buffer, sizeof(buffer), UserMode, &size);
        todo_wine ok(status == STATUS_ACCESS_VIOLATION, "Got unexpected status %#lx.\n", status);
        ok(!size, "Got unexpected size %#Ix.\n", size);

        memset(buffer, 0, sizeof(buffer));
        size = 0xdeadbeef;
        if (0)  /* Passing NULL for the copied size address hangs Windows. */
            pMmCopyVirtualMemory(process, base + test_input->teststr_offset, PsGetCurrentProcess(),
                                 buffer, sizeof(buffer), KernelMode, NULL);
        status = pMmCopyVirtualMemory(process, base + test_input->teststr_offset, PsGetCurrentProcess(),
                buffer, sizeof(buffer), KernelMode, &size);
        todo_wine ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
        todo_wine ok(size == sizeof(buffer), "Got unexpected size %Iu.\n", size);
        todo_wine ok(!strcmp(buffer, teststr), "Got unexpected test string.\n");
    }
    else
    {
       win_skip("MmCopyVirtualMemory is not available.\n");
    }

    if (!winetest_platform_is_wine)
    {
        KeStackAttachProcess((PKPROCESS)process, &state);
        todo_wine ok(!strcmp(teststr, (char *)(base + test_input->teststr_offset)),
                "Strings do not match.\n");
        *test_input->modified_value = modified_value;
        KeUnstackDetachProcess(&state);
    }
    ObDereferenceObject(process);
}

static void test_permanence(void)
{
    OBJECT_ATTRIBUTES attr;
    HANDLE handle, handle2;
    UNICODE_STRING str;
    NTSTATUS status;

    RtlInitUnicodeString(&str, L"\\BaseNamedObjects\\wine_test_dir");
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    status = ZwCreateDirectoryObject( &handle, GENERIC_ALL, &attr );
    ok(!status, "got %#lx\n", status);
    status = ZwClose( handle );
    ok(!status, "got %#lx\n", status);
    status = ZwOpenDirectoryObject( &handle, 0, &attr );
    ok(status == STATUS_OBJECT_NAME_NOT_FOUND, "got %#lx\n", status);

    attr.Attributes = OBJ_PERMANENT;
    status = ZwCreateDirectoryObject( &handle, GENERIC_ALL, &attr );
    ok(!status, "got %#lx\n", status);
    status = ZwClose( handle );
    ok(!status, "got %#lx\n", status);

    attr.Attributes = 0;
    status = ZwOpenDirectoryObject( &handle, 0, &attr );
    ok(!status, "got %#lx\n", status);
    status = ZwMakeTemporaryObject( handle );
    ok(!status, "got %#lx\n", status);
    status = ZwMakeTemporaryObject( handle );
    ok(!status, "got %#lx\n", status);
    status = ZwClose( handle );
    ok(!status, "got %#lx\n", status);
    status = ZwOpenDirectoryObject( &handle, 0, &attr );
    ok(status == STATUS_OBJECT_NAME_NOT_FOUND, "got %#lx\n", status);

    status = ZwCreateDirectoryObject( &handle, GENERIC_ALL, &attr );
    ok(!status, "got %#lx\n", status);
    attr.Attributes = OBJ_PERMANENT;
    status = ZwOpenDirectoryObject( &handle2, 0, &attr );
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);
    status = ZwClose( handle2 );
    ok(!status, "got %#lx\n", status);
    status = ZwClose( handle );
    ok(!status, "got %#lx\n", status);
    attr.Attributes = 0;
    status = ZwOpenDirectoryObject( &handle, 0, &attr );
    ok(status == STATUS_OBJECT_NAME_NOT_FOUND, "got %#lx\n", status);
}

static void test_driver_object_extension(void)
{
    NTSTATUS (WINAPI *pIoAllocateDriverObjectExtension)(PDRIVER_OBJECT, PVOID, ULONG, PVOID *);
    PVOID (WINAPI *pIoGetDriverObjectExtension)(PDRIVER_OBJECT, PVOID);
    NTSTATUS status;
    void *driver_obj_ext = NULL;
    void *get_obj_ext = NULL;

    pIoAllocateDriverObjectExtension = get_proc_address("IoAllocateDriverObjectExtension");
    pIoGetDriverObjectExtension = get_proc_address("IoGetDriverObjectExtension");

    if (!pIoAllocateDriverObjectExtension)
    {
        win_skip("IoAllocateDriverObjectExtension is not available.\n");
        return;
    }

    status = pIoAllocateDriverObjectExtension(driver_obj, NULL, 100, &driver_obj_ext);
    todo_wine ok(status == STATUS_SUCCESS, "got %#lx\n", status);
    todo_wine ok(driver_obj_ext != NULL, "got NULL\n");

    get_obj_ext = pIoGetDriverObjectExtension(driver_obj, NULL);
    todo_wine ok(get_obj_ext == driver_obj_ext && get_obj_ext != NULL, "got %p != %p\n", get_obj_ext, driver_obj_ext);

    status = pIoAllocateDriverObjectExtension(driver_obj, NULL, 100, &driver_obj_ext);
    todo_wine ok(status == STATUS_OBJECT_NAME_COLLISION, "got %#lx\n", status);
    ok(driver_obj_ext == NULL, "got %p\n", driver_obj_ext);

    get_obj_ext = pIoGetDriverObjectExtension(driver_obj, (void *)0xdead);
    ok(get_obj_ext == NULL, "got %p\n", get_obj_ext);
}

static void test_default_security(void)
{
    PSECURITY_DESCRIPTOR sd = NULL;
    NTSTATUS status;
    PSID group = NULL, owner = NULL;
    BOOLEAN isdefault, present;
    PACL acl = NULL;
    PACCESS_ALLOWED_ACE ace;
    SID_IDENTIFIER_AUTHORITY auth = { SECURITY_NULL_SID_AUTHORITY };
    SID_IDENTIFIER_AUTHORITY authwine7 = { SECURITY_NT_AUTHORITY };
    PSID sid1, sid2, sidwin7;
    BOOL ret;

    status = FltBuildDefaultSecurityDescriptor(&sd, STANDARD_RIGHTS_ALL);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);
    if (status != STATUS_SUCCESS)
    {
        win_skip("Skipping FltBuildDefaultSecurityDescriptor tests\n");
        return;
    }
    ok(sd != NULL, "Failed to return descriptor\n");

    status = RtlGetGroupSecurityDescriptor(sd, &group, &isdefault);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);
    ok(group == NULL, "group isn't NULL\n");

    status = RtlGetOwnerSecurityDescriptor(sd, &owner, &isdefault);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);
    ok(owner == NULL, "owner isn't NULL\n");

    status = RtlGetDaclSecurityDescriptor(sd, &present, &acl, &isdefault);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);
    ok(acl != NULL, "acl is NULL\n");
    ok(acl->AceCount == 2, "got %d\n", acl->AceCount);

    sid1 = ExAllocatePool(NonPagedPool, RtlLengthRequiredSid(2));
    status = RtlInitializeSid(sid1, &auth, 2);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);
    *RtlSubAuthoritySid(sid1, 0)  = SECURITY_BUILTIN_DOMAIN_RID;
    *RtlSubAuthoritySid(sid1, 1) = DOMAIN_GROUP_RID_ADMINS;

    sidwin7 = ExAllocatePool(NonPagedPool, RtlLengthRequiredSid(2));
    status = RtlInitializeSid(sidwin7, &authwine7, 2);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);
    *RtlSubAuthoritySid(sidwin7, 0)  = SECURITY_BUILTIN_DOMAIN_RID;
    *RtlSubAuthoritySid(sidwin7, 1) = DOMAIN_ALIAS_RID_ADMINS;

    sid2 = ExAllocatePool(NonPagedPool, RtlLengthRequiredSid(1));
    RtlInitializeSid(sid2, &auth, 1);
    *RtlSubAuthoritySid(sid2, 0)  = SECURITY_LOCAL_SYSTEM_RID;

    /* SECURITY_BUILTIN_DOMAIN_RID */
    status = RtlGetAce(acl, 0, (void**)&ace);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);

    ok(ace->Header.AceType == ACCESS_ALLOWED_ACE_TYPE, "got %#x\n", ace->Header.AceType);
    ok(ace->Header.AceFlags == 0, "got %#x\n", ace->Header.AceFlags);
    ok(ace->Mask == STANDARD_RIGHTS_ALL, "got %#lx\n", ace->Mask);

    ret = RtlEqualSid(sid1, (PSID)&ace->SidStart) || RtlEqualSid(sidwin7, (PSID)&ace->SidStart);
    ok(ret, "SID not equal\n");

    /* SECURITY_LOCAL_SYSTEM_RID */
    status = RtlGetAce(acl, 1, (void**)&ace);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);

    ok(ace->Header.AceType == ACCESS_ALLOWED_ACE_TYPE, "got %#x\n", ace->Header.AceType);
    ok(ace->Header.AceFlags == 0, "got %#x\n", ace->Header.AceFlags);
    ok(ace->Mask == STANDARD_RIGHTS_ALL, "got %#lx\n", ace->Mask);

    ret = RtlEqualSid(sid2, (PSID)&ace->SidStart) || RtlEqualSid(sidwin7, (PSID)&ace->SidStart);
    ok(ret, "SID not equal\n");

    ExFreePool(sid1);
    ExFreePool(sid2);

    FltFreeSecurityDescriptor(sd);
}

static NTSTATUS main_test(DEVICE_OBJECT *device, IRP *irp, IO_STACK_LOCATION *stack)
{
    void *buffer = irp->AssociatedIrp.SystemBuffer;
    struct main_test_input *test_input = buffer;

    if (!buffer)
        return STATUS_ACCESS_VIOLATION;

    pExEventObjectType = get_proc_address("ExEventObjectType");
    ok(!!pExEventObjectType, "ExEventObjectType not found\n");

    pIoFileObjectType = get_proc_address("IoFileObjectType");
    ok(!!pIoFileObjectType, "IofileObjectType not found\n");

    pPsThreadType = get_proc_address("PsThreadType");
    ok(!!pPsThreadType, "IofileObjectType not found\n");

    pPsInitialSystemProcess = get_proc_address("PsInitialSystemProcess");
    ok(!!pPsInitialSystemProcess, "PsInitialSystemProcess not found\n");

    test_irp_struct(irp, device);
    test_current_thread(FALSE);
    test_critical_region(TRUE);
    test_mdl_map();
    test_init_funcs();
    test_load_driver();
    test_sync();
    test_queue();
    test_version();
    test_stack_callout();
    test_lookaside_list();
    test_ob_reference();
    test_resource();
    test_lookup_thread();
    test_IoAttachDeviceToDeviceStack();
    test_object_name();
#if defined(__i386__) || defined(__x86_64__)
    test_executable_pool();
#endif
    test_affinity();
    test_dpc();
    test_process_memory(test_input);
    test_permanence();
    test_driver_object_extension();
    test_default_security();

    IoMarkIrpPending(irp);
    IoQueueWorkItem(work_item, main_test_task, DelayedWorkQueue, irp);

    return STATUS_PENDING;
}

static NTSTATUS test_basic_ioctl(IRP *irp, IO_STACK_LOCATION *stack, ULONG_PTR *info)
{
    ULONG length = min(stack->Parameters.DeviceIoControl.OutputBufferLength, sizeof(teststr));
    char *buffer = irp->AssociatedIrp.SystemBuffer;

    if (!buffer)
        return STATUS_ACCESS_VIOLATION;

    memcpy(buffer, teststr, length);
    *info = length;

    return STATUS_SUCCESS;
}

static NTSTATUS get_dword(IRP *irp, IO_STACK_LOCATION *stack, ULONG_PTR *info, DWORD value)
{
    ULONG length = stack->Parameters.DeviceIoControl.OutputBufferLength;
    char *buffer = irp->AssociatedIrp.SystemBuffer;

    if (!buffer)
        return STATUS_ACCESS_VIOLATION;

    if (length < sizeof(DWORD))
        return STATUS_BUFFER_TOO_SMALL;

    *(DWORD*)buffer = value;
    *info = sizeof(DWORD);
    return STATUS_SUCCESS;
}

static NTSTATUS get_fscontext(IRP *irp, IO_STACK_LOCATION *stack, ULONG_PTR *info)
{
    ULONG length = stack->Parameters.DeviceIoControl.OutputBufferLength;
    char *buffer = irp->AssociatedIrp.SystemBuffer;
    struct file_context *context = stack->FileObject->FsContext;

    if (!buffer)
        return STATUS_ACCESS_VIOLATION;

    if (length < sizeof(DWORD))
        return STATUS_BUFFER_TOO_SMALL;

    *(DWORD*)buffer = context->id;
    *info = sizeof(DWORD);
    return STATUS_SUCCESS;
}

static NTSTATUS return_status(IRP *irp, IO_STACK_LOCATION *stack, ULONG code)
{
    ULONG input_length = stack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG output_length = stack->Parameters.DeviceIoControl.OutputBufferLength;
    const struct return_status_params *input_buffer;
    struct return_status_params params;
    void *output_buffer;

    if (code == IOCTL_WINETEST_RETURN_STATUS_NEITHER)
    {
        input_buffer = stack->Parameters.DeviceIoControl.Type3InputBuffer;
        output_buffer = irp->UserBuffer;
    }
    else if (code == IOCTL_WINETEST_RETURN_STATUS_DIRECT)
    {
        input_buffer = irp->AssociatedIrp.SystemBuffer;
        output_buffer = MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority);
    }
    else
    {
        input_buffer = irp->AssociatedIrp.SystemBuffer;
        output_buffer = irp->AssociatedIrp.SystemBuffer;
    }

    if (!input_buffer || !output_buffer)
    {
        irp->IoStatus.Status = STATUS_ACCESS_VIOLATION;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return STATUS_ACCESS_VIOLATION;
    }

    if (input_length < sizeof(params) || output_length < 6)
    {
        irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return STATUS_BUFFER_TOO_SMALL;
    }

    params = *input_buffer;

    if (params.ret_status == STATUS_PENDING && !params.pending)
    {
        /* this causes kernel hangs under certain conditions */
        irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return STATUS_INVALID_PARAMETER;
    }

    if (params.pending)
        IoMarkIrpPending(irp);

    /* intentionally report the wrong information (and status) */
    memcpy(output_buffer, "ghijkl", 6);
    irp->IoStatus.Information = 3;
    irp->IoStatus.Status = params.iosb_status;
    IoCompleteRequest(irp, IO_NO_INCREMENT);

    return params.ret_status;
}

static NTSTATUS test_load_driver_ioctl(IRP *irp, IO_STACK_LOCATION *stack, ULONG_PTR *info)
{
    BOOL *load = irp->AssociatedIrp.SystemBuffer;
    UNICODE_STRING name;

    if (!load)
        return STATUS_ACCESS_VIOLATION;

    *info = 0;

    RtlInitUnicodeString(&name, driver2_path);
    if (*load)
        return ZwLoadDriver(&name);
    else
        return ZwUnloadDriver(&name);
}

static NTSTATUS completion_ioctl(DEVICE_OBJECT *device, IRP *irp, IO_STACK_LOCATION *stack)
{
    if (device == upper_device)
    {
        ok(irp->CurrentLocation == 2, "Got current location %u.\n", irp->CurrentLocation);
        ok(!irp->PendingReturned, "Got pending flag %u.\n", irp->PendingReturned);
        ok(stack->Control == (SL_INVOKE_ON_CANCEL | SL_INVOKE_ON_ERROR | SL_INVOKE_ON_SUCCESS),
                "Got control flags %#x.\n", stack->Control);
        stack = IoGetNextIrpStackLocation(irp);
        ok(!stack->Control, "Got control flags %#x.\n", stack->Control);
        stack = irp->Tail.Overlay.CurrentStackLocation + 1; /* previous location */
        ok(!stack->Control, "Got control flags %#x.\n", stack->Control);

        IoCopyCurrentIrpStackLocationToNext(irp);
        IoSetCompletionRoutine(irp, completion_cb, upper_device, TRUE, TRUE, TRUE);
        return IoCallDriver(lower_device, irp);
    }
    else
    {
        ok(device == lower_device, "Got wrong device.\n");
        ok(irp->CurrentLocation == 1, "Got current location %u.\n", irp->CurrentLocation);
        ok(!irp->PendingReturned, "Got pending flag %u.\n", irp->PendingReturned);
        ok(stack->Control == (SL_INVOKE_ON_CANCEL | SL_INVOKE_ON_ERROR | SL_INVOKE_ON_SUCCESS),
                "Got control flags %#x.\n", stack->Control);
        stack = irp->Tail.Overlay.CurrentStackLocation + 1; /* previous location */
        ok(stack->Control == (SL_INVOKE_ON_CANCEL | SL_INVOKE_ON_ERROR | SL_INVOKE_ON_SUCCESS),
                "Got control flags %#x.\n", stack->Control);
        stack = irp->Tail.Overlay.CurrentStackLocation + 2; /* top location */
        ok(!stack->Control, "Got control flags %#x.\n", stack->Control);

        if (completion_lower_pending)
        {
            IoMarkIrpPending(irp);
            return STATUS_PENDING;
        }
        else
        {
            irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;
        }
    }
}

static NTSTATUS WINAPI driver_Create(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    struct file_context *context = ExAllocatePool(PagedPool, sizeof(*context));

    if (!context)
    {
        irp->IoStatus.Status = STATUS_NO_MEMORY;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return STATUS_NO_MEMORY;
    }

    context->id = ++create_count;
    context->namelen = min(irpsp->FileObject->FileName.Length, sizeof(context->name));
    memcpy(context->name, irpsp->FileObject->FileName.Buffer, context->namelen);
    irpsp->FileObject->FsContext = context;

    last_created_file = irpsp->FileObject;
    create_caller_thread = KeGetCurrentThread();
    create_irp_thread = irp->Tail.Overlay.Thread;

    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static NTSTATUS WINAPI driver_IoControl(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    NTSTATUS status = STATUS_NOT_SUPPORTED;

    switch (stack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_WINETEST_BASIC_IOCTL:
            status = test_basic_ioctl(irp, stack, &irp->IoStatus.Information);
            break;
        case IOCTL_WINETEST_MAIN_TEST:
            status = main_test(device, irp, stack);
            break;
        case IOCTL_WINETEST_LOAD_DRIVER:
            status = test_load_driver_ioctl(irp, stack, &irp->IoStatus.Information);
            break;
        case IOCTL_WINETEST_RESET_CANCEL:
            cancel_cnt = 0;
            status = STATUS_SUCCESS;
            break;
        case IOCTL_WINETEST_TEST_CANCEL:
            IoSetCancelRoutine(irp, cancel_ioctl_irp);
            IoMarkIrpPending(irp);
            return STATUS_PENDING;
        case IOCTL_WINETEST_GET_CANCEL_COUNT:
            status = get_dword(irp, stack, &irp->IoStatus.Information, cancel_cnt);
            break;
        case IOCTL_WINETEST_GET_CREATE_COUNT:
            status = get_dword(irp, stack, &irp->IoStatus.Information, create_count);
            break;
        case IOCTL_WINETEST_GET_CLOSE_COUNT:
            status = get_dword(irp, stack, &irp->IoStatus.Information, close_count);
            break;
        case IOCTL_WINETEST_GET_FSCONTEXT:
            status = get_fscontext(irp, stack, &irp->IoStatus.Information);
            break;
        case IOCTL_WINETEST_RETURN_STATUS_BUFFERED:
        case IOCTL_WINETEST_RETURN_STATUS_DIRECT:
        case IOCTL_WINETEST_RETURN_STATUS_NEITHER:
            return return_status(irp, stack, stack->Parameters.DeviceIoControl.IoControlCode);
        case IOCTL_WINETEST_DETACH:
            IoDetachDevice(lower_device);
            status = STATUS_SUCCESS;
            break;
        case IOCTL_WINETEST_COMPLETION:
            return completion_ioctl(device, irp, stack);
        default:
            break;
    }

    if (status != STATUS_PENDING)
    {
        irp->IoStatus.Status = status;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }
    return status;
}

static NTSTATUS WINAPI driver_FlushBuffers(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation(irp);
    ok(device == lower_device, "Expected device %p, got %p.\n", lower_device, device);
    ok(irpsp->DeviceObject == device, "device != DeviceObject\n");
    ok(irp->Tail.Overlay.Thread == (PETHREAD)KeGetCurrentThread(),
       "IRP thread is not the current thread\n");
    IoMarkIrpPending(irp);
    return STATUS_PENDING;
}

static void WINAPI blocking_irp_task(DEVICE_OBJECT *device, void *context)
{
    LARGE_INTEGER timeout;
    IRP *irp = context;

    timeout.QuadPart = -100 * 10000;
    KeDelayExecutionThread( KernelMode, FALSE, &timeout );

    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
}

static void WINAPI blocking_irp_failure_task(DEVICE_OBJECT *device, void *context)
{
    LARGE_INTEGER timeout;
    IRP *irp = context;

    timeout.QuadPart = -100 * 10000;
    KeDelayExecutionThread( KernelMode, FALSE, &timeout );

    irp->IoStatus.Status = STATUS_DEVICE_NOT_READY;
    irp->IoStatus.Information = 0;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
}

static BOOL compare_file_name(const struct file_context *context, const WCHAR *expect)
{
    return context->namelen == wcslen(expect) * sizeof(WCHAR)
            && !kmemcmp(context->name, expect, context->namelen);
}

static NTSTATUS WINAPI driver_QueryInformation(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    NTSTATUS ret;

    switch (stack->Parameters.QueryFile.FileInformationClass)
    {
    case FileNameInformation:
    {
        const struct file_context *context = stack->FileObject->FsContext;
        FILE_NAME_INFORMATION *info = irp->AssociatedIrp.SystemBuffer;
        ULONG len;

        if (stack->Parameters.QueryFile.Length < sizeof(*info))
        {
            ret = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        if (compare_file_name(context, L"\\notimpl"))
        {
            ret = STATUS_NOT_IMPLEMENTED;
            break;
        }
        else if (compare_file_name(context, L""))
        {
            ret = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
        else if (compare_file_name(context, L"\\badparam"))
        {
            ret = STATUS_INVALID_PARAMETER;
            break;
        }
        else if (compare_file_name(context, L"\\genfail"))
        {
            ret = STATUS_UNSUCCESSFUL;
            break;
        }
        else if (compare_file_name(context, L"\\badtype"))
        {
            ret = STATUS_OBJECT_TYPE_MISMATCH;
            break;
        }

        len = stack->Parameters.QueryFile.Length - FIELD_OFFSET(FILE_NAME_INFORMATION, FileName);
        if (len < context->namelen)
            ret = STATUS_BUFFER_OVERFLOW;
        else
        {
            len = context->namelen;
            ret = STATUS_SUCCESS;
        }
        irp->IoStatus.Information = FIELD_OFFSET(FILE_NAME_INFORMATION, FileName) + len;
        info->FileNameLength = context->namelen;
        memcpy(info->FileName, context->name, len);
        break;
    }

    default:
        ret = STATUS_NOT_IMPLEMENTED;
        break;
    }

    irp->IoStatus.Status = ret;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return ret;
}

static NTSTATUS WINAPI driver_QueryVolumeInformation(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    ULONG length = stack->Parameters.QueryVolume.Length;
    NTSTATUS ret;

    switch (stack->Parameters.QueryVolume.FsInformationClass)
    {
    case FileFsVolumeInformation:
    {
        FILE_FS_VOLUME_INFORMATION *info = irp->AssociatedIrp.SystemBuffer;
        static const WCHAR label[] = L"WineTestDriver";
        ULONG serial = 0xdeadbeef;

        if (length < sizeof(FILE_FS_VOLUME_INFORMATION))
        {
            ret = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        info->VolumeCreationTime.QuadPart = 0;
        info->VolumeSerialNumber = serial;
        info->VolumeLabelLength = min( lstrlenW(label) * sizeof(WCHAR),
                                       length - offsetof( FILE_FS_VOLUME_INFORMATION, VolumeLabel ) );
        info->SupportsObjects = TRUE;
        memcpy( info->VolumeLabel, label, info->VolumeLabelLength );

        irp->IoStatus.Information = offsetof( FILE_FS_VOLUME_INFORMATION, VolumeLabel ) + info->VolumeLabelLength;
        ret = STATUS_SUCCESS;
        break;
    }

    case FileFsSizeInformation:
    {
        IoMarkIrpPending(irp);
        IoQueueWorkItem(work_item, blocking_irp_task, DelayedWorkQueue, irp);
        return STATUS_PENDING;
    }

    case FileFsFullSizeInformation:
    {
        IoMarkIrpPending(irp);
        IoQueueWorkItem(work_item, blocking_irp_failure_task, DelayedWorkQueue, irp);
        return STATUS_PENDING;
    }

    default:
        ret = STATUS_NOT_IMPLEMENTED;
        break;
    }

    irp->IoStatus.Status = ret;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return ret;
}

static NTSTATUS WINAPI driver_Close(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    ++close_count;
    if (stack->FileObject->FsContext)
        ExFreePool(stack->FileObject->FsContext);
    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static VOID WINAPI driver_Unload(DRIVER_OBJECT *driver)
{
    UNICODE_STRING linkW;

    DbgPrint("unloading driver\n");

    IoFreeWorkItem(work_item);
    work_item = NULL;

    RtlInitUnicodeString(&linkW, L"\\DosDevices\\WineTestDriver");
    IoDeleteSymbolicLink(&linkW);

    IoDeleteDevice(upper_device);
    IoDeleteDevice(lower_device);

    winetest_cleanup();
}

NTSTATUS WINAPI DriverEntry(DRIVER_OBJECT *driver, PUNICODE_STRING registry)
{
    UNICODE_STRING nameW, linkW;
    NTSTATUS status;
    void *obj;

    if ((status = winetest_init()))
        return status;

    DbgPrint("loading driver\n");

    driver_obj = driver;

    /* Allow unloading of the driver */
    driver->DriverUnload = driver_Unload;

    /* Set driver functions */
    driver->MajorFunction[IRP_MJ_CREATE]            = driver_Create;
    driver->MajorFunction[IRP_MJ_DEVICE_CONTROL]    = driver_IoControl;
    driver->MajorFunction[IRP_MJ_FLUSH_BUFFERS]     = driver_FlushBuffers;
    driver->MajorFunction[IRP_MJ_QUERY_INFORMATION] = driver_QueryInformation;
    driver->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = driver_QueryVolumeInformation;
    driver->MajorFunction[IRP_MJ_CLOSE]             = driver_Close;

    RtlInitUnicodeString(&nameW, L"IoDriverObjectType");
    pIoDriverObjectType = MmGetSystemRoutineAddress(&nameW);

    RtlInitUnicodeString(&nameW, L"\\Driver\\WineTestDriver");
    status = ObReferenceObjectByName(&nameW, 0, NULL, 0, *pIoDriverObjectType, KernelMode, NULL, &obj);
    ok(!status, "got %#lx\n", status);
    ok(obj == driver, "expected %p, got %p\n", driver, obj);
    ObDereferenceObject(obj);

    RtlInitUnicodeString(&nameW, L"\\Device\\WineTestDriver");
    RtlInitUnicodeString(&linkW, L"\\DosDevices\\WineTestDriver");

    status = IoCreateDevice(driver, 0, &nameW, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &lower_device);
    ok(!status, "failed to create device, status %#lx\n", status);
    status = IoCreateSymbolicLink(&linkW, &nameW);
    ok(!status, "failed to create link, status %#lx\n", status);
    lower_device->Flags &= ~DO_DEVICE_INITIALIZING;

    RtlInitUnicodeString(&nameW, L"\\Device\\WineTestUpper");
    status = IoCreateDevice(driver, 0, &nameW, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &upper_device);
    ok(!status, "failed to create device, status %#lx\n", status);

    IoAttachDeviceToDeviceStack(upper_device, lower_device);
    upper_device->Flags &= ~DO_DEVICE_INITIALIZING;

    work_item = IoAllocateWorkItem(lower_device);
    ok(work_item != NULL, "work_item = NULL\n");

    return STATUS_SUCCESS;
}
