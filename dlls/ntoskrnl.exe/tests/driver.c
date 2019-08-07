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

#include "driver.h"

static const WCHAR device_name[] = {'\\','D','e','v','i','c','e',
                                    '\\','W','i','n','e','T','e','s','t','D','r','i','v','e','r',0};
static const WCHAR upper_name[] = {'\\','D','e','v','i','c','e',
                                   '\\','W','i','n','e','T','e','s','t','U','p','p','e','r',0};
static const WCHAR driver_link[] = {'\\','D','o','s','D','e','v','i','c','e','s',
                                    '\\','W','i','n','e','T','e','s','t','D','r','i','v','e','r',0};

static DRIVER_OBJECT *driver_obj;
static DEVICE_OBJECT *lower_device, *upper_device;

static HANDLE okfile;
static LONG successes;
static LONG failures;
static LONG skipped;
static LONG todo_successes;
static LONG todo_failures;
static int todo_level, todo_do_loop;
static int running_under_wine;
static int winetest_debug;
static int winetest_report_success;

static POBJECT_TYPE *pExEventObjectType, *pIoFileObjectType, *pPsThreadType;
static PEPROCESS *pPsInitialSystemProcess;
static void *create_caller_thread;

void WINAPI ObfReferenceObject( void *obj );

NTSTATUS WINAPI ZwQueryInformationProcess(HANDLE,PROCESSINFOCLASS,void*,ULONG,ULONG*);

static void kvprintf(const char *format, __ms_va_list ap)
{
    static char buffer[512];
    IO_STATUS_BLOCK io;
    int len = vsnprintf(buffer, sizeof(buffer), format, ap);
    ZwWriteFile(okfile, NULL, NULL, NULL, &io, buffer, len, NULL, NULL);
}

static void WINAPIV kprintf(const char *format, ...)
{
    __ms_va_list valist;

    __ms_va_start(valist, format);
    kvprintf(format, valist);
    __ms_va_end(valist);
}

static void WINAPIV vok_(const char *file, int line, int condition, const char *msg,  __ms_va_list args)
{
    const char *current_file;

    if (!(current_file = drv_strrchr(file, '/')) &&
        !(current_file = drv_strrchr(file, '\\')))
        current_file = file;
    else
        current_file++;

    if (todo_level)
    {
        if (condition)
        {
            kprintf("%s:%d: Test succeeded inside todo block: ", current_file, line);
            kvprintf(msg, args);
            InterlockedIncrement(&todo_failures);
        }
        else
        {
            if (winetest_debug > 0)
            {
                kprintf("%s:%d: Test marked todo: ", current_file, line);
                kvprintf(msg, args);
            }
            InterlockedIncrement(&todo_successes);
        }
    }
    else
    {
        if (!condition)
        {
            kprintf("%s:%d: Test failed: ", current_file, line);
            kvprintf(msg, args);
            InterlockedIncrement(&failures);
        }
        else
        {
            if (winetest_report_success)
                kprintf("%s:%d: Test succeeded\n", current_file, line);
            InterlockedIncrement(&successes);
        }
    }
}

static void WINAPIV ok_(const char *file, int line, int condition, const char *msg, ...)
{
    __ms_va_list args;
    __ms_va_start(args, msg);
    vok_(file, line, condition, msg, args);
    __ms_va_end(args);
}

static void vskip_(const char *file, int line, const char *msg, __ms_va_list args)
{
    const char *current_file;

    if (!(current_file = drv_strrchr(file, '/')) &&
        !(current_file = drv_strrchr(file, '\\')))
        current_file = file;
    else
        current_file++;

    kprintf("%s:%d: Tests skipped: ", current_file, line);
    kvprintf(msg, args);
    skipped++;
}

static void WINAPIV win_skip_(const char *file, int line, const char *msg, ...)
{
    __ms_va_list args;
    __ms_va_start(args, msg);
    if (running_under_wine)
        vok_(file, line, 0, msg, args);
    else
        vskip_(file, line, msg, args);
    __ms_va_end(args);
}

static void winetest_start_todo( int is_todo )
{
    todo_level = (todo_level << 1) | (is_todo != 0);
    todo_do_loop=1;
}

static int winetest_loop_todo(void)
{
    int do_loop=todo_do_loop;
    todo_do_loop=0;
    return do_loop;
}

static void winetest_end_todo(void)
{
    todo_level >>= 1;
}

static int broken(int condition)
{
    return !running_under_wine && condition;
}

#define ok(condition, ...)  ok_(__FILE__, __LINE__, condition, __VA_ARGS__)
#define todo_if(is_todo) for (winetest_start_todo(is_todo); \
                              winetest_loop_todo(); \
                              winetest_end_todo())
#define todo_wine               todo_if(running_under_wine)
#define todo_wine_if(is_todo)   todo_if((is_todo) && running_under_wine)
#define win_skip(...)           win_skip_(__FILE__, __LINE__, __VA_ARGS__)

static void *get_proc_address(const char *name)
{
    UNICODE_STRING name_u;
    ANSI_STRING name_a;
    NTSTATUS status;
    void *ret;

    RtlInitAnsiString(&name_a, name);
    status = RtlAnsiStringToUnicodeString(&name_u, &name_a, TRUE);
    ok (!status, "RtlAnsiStringToUnicodeString failed: %#x\n", status);
    if (status) return NULL;

    ret = MmGetSystemRoutineAddress(&name_u);
    RtlFreeUnicodeString(&name_u);
    return ret;
}

static FILE_OBJECT *last_created_file;
static unsigned int create_count, close_count;

static void test_irp_struct(IRP *irp, DEVICE_OBJECT *device)
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );

    ok(device == upper_device, "Expected device %p, got %p.\n", upper_device, device);
    ok(last_created_file != NULL, "last_created_file = NULL\n");
    ok(irpsp->FileObject == last_created_file, "FileObject != last_created_file\n");
    ok(irpsp->DeviceObject == upper_device, "unexpected DeviceObject\n");
    ok(irpsp->FileObject->DeviceObject == lower_device, "unexpected FileObject->DeviceObject\n");
    ok(!irp->UserEvent, "UserEvent = %p\n", irp->UserEvent);
    ok(irp->Tail.Overlay.Thread == (PETHREAD)KeGetCurrentThread(),
       "IRP thread is not the current thread\n");
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
    ok(timer.Header.SignalState == 0, "got: %u\n", timer.Header.SignalState);

    KeInitializeTimerEx(&timer2, SynchronizationTimer);
    ok(timer2.Header.Type == 9, "got: %u\n", timer2.Header.Type);
    ok(timer2.Header.Size == 0 || timer2.Header.Size == 10, "got: %u\n", timer2.Header.Size);
    ok(timer2.Header.SignalState == 0, "got: %u\n", timer2.Header.SignalState);
}

static const WCHAR driver2_path[] = {
    '\\','R','e','g','i','s','t','r','y',
    '\\','M','a','c','h','i','n','e',
    '\\','S','y','s','t','e','m',
    '\\','C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t',
    '\\','S','e','r','v','i','c','e','s',
    '\\','W','i','n','e','T','e','s','t','D','r','i','v','e','r','2',0
};

static void test_load_driver(void)
{
    UNICODE_STRING name;
    NTSTATUS ret;

    RtlInitUnicodeString(&name, driver2_path);

    ret = ZwLoadDriver(&name);
    ok(!ret, "got %#x\n", ret);

    ret = ZwLoadDriver(&name);
    ok(ret == STATUS_IMAGE_ALREADY_LOADED, "got %#x\n", ret);

    ret = ZwUnloadDriver(&name);
    ok(!ret, "got %#x\n", ret);
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
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    if (is_system)
        ok(current == *pPsInitialSystemProcess, "current != PsInitialSystemProcess\n");
    else
        ok(current != *pPsInitialSystemProcess, "current == PsInitialSystemProcess\n");

    ok(PsGetProcessId(current) == PsGetCurrentProcessId(), "process IDs don't match\n");
    ok(PsGetThreadProcessId((PETHREAD)KeGetCurrentThread()) == PsGetCurrentProcessId(), "process IDs don't match\n");

    thread = PsGetCurrentThread();
    ret = wait_single( thread, 0 );
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    ok(PsGetThreadId((PETHREAD)KeGetCurrentThread()) == PsGetCurrentThreadId(), "thread IDs don't match\n");
    ok(PsIsSystemThread((PETHREAD)KeGetCurrentThread()) == is_system, "unexpected system thread\n");
    if (!is_system)
        ok(create_caller_thread == KeGetCurrentThread(), "thread is not create caller thread\n");

    ret = ObOpenObjectByPointer(current, OBJ_KERNEL_HANDLE, NULL, PROCESS_QUERY_INFORMATION, NULL, KernelMode, &process_handle);
    ok(!ret, "ObOpenObjectByPointer failed: %#x\n", ret);

    ret = ZwQueryInformationProcess(process_handle, ProcessBasicInformation, &info, sizeof(info), NULL);
    ok(!ret, "ZwQueryInformationProcess failed: %#x\n", ret);

    id = PsGetProcessInheritedFromUniqueProcessId(current);
    ok(id == (HANDLE)info.InheritedFromUniqueProcessId, "unexpected process id %p\n", id);

    ret = ZwClose(process_handle);
    ok(!ret, "ZwClose failed: %#x\n", ret);
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
    ok(!ret, "got %#x\n", ret);

    return thread;
}

static void join_thread(HANDLE thread)
{
    NTSTATUS ret;

    ret = ZwWaitForSingleObject(thread, FALSE, NULL);
    ok(!ret, "got %#x\n", ret);
    ret = ZwClose(thread);
    ok(!ret, "got %#x\n", ret);
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
    ok(ret == expect, "expected %#x, got %#x\n", expect, ret);

    if (!ret) KeReleaseMutex(&test_mutex, FALSE);
    PsTerminateSystemThread(STATUS_SUCCESS);
}

static void test_sync(void)
{
    KSEMAPHORE semaphore, semaphore2;
    KEVENT manual_event, auto_event, *event;
    KTIMER timer;
    LARGE_INTEGER timeout;
    OBJECT_ATTRIBUTES attr;
    void *objs[2];
    NTSTATUS ret;
    HANDLE handle;
    int i;

    KeInitializeEvent(&manual_event, NotificationEvent, FALSE);

    ret = wait_single(&manual_event, 0);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    KeSetEvent(&manual_event, 0, FALSE);

    ret = wait_single(&manual_event, 0);
    ok(ret == 0, "got %#x\n", ret);

    ret = wait_single(&manual_event, 0);
    ok(ret == 0, "got %#x\n", ret);

    KeResetEvent(&manual_event);

    ret = wait_single(&manual_event, 0);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    KeInitializeEvent(&auto_event, SynchronizationEvent, FALSE);

    ret = wait_single(&auto_event, 0);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    KeSetEvent(&auto_event, 0, FALSE);

    ret = wait_single(&auto_event, 0);
    ok(ret == 0, "got %#x\n", ret);

    ret = wait_single(&auto_event, 0);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    KeInitializeEvent(&auto_event, SynchronizationEvent, TRUE);

    ret = wait_single(&auto_event, 0);
    ok(ret == 0, "got %#x\n", ret);

    objs[0] = &manual_event;
    objs[1] = &auto_event;

    ret = wait_multiple(2, objs, WaitAny, 0);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    KeSetEvent(&manual_event, 0, FALSE);
    KeSetEvent(&auto_event, 0, FALSE);

    ret = wait_multiple(2, objs, WaitAny, 0);
    ok(ret == 0, "got %#x\n", ret);

    ret = wait_single(&auto_event, 0);
    ok(ret == 0, "got %#x\n", ret);

    KeResetEvent(&manual_event);
    KeSetEvent(&auto_event, 0, FALSE);

    ret = wait_multiple(2, objs, WaitAny, 0);
    ok(ret == 1, "got %#x\n", ret);

    ret = wait_multiple(2, objs, WaitAny, 0);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    KeSetEvent(&manual_event, 0, FALSE);
    KeSetEvent(&auto_event, 0, FALSE);

    ret = wait_multiple(2, objs, WaitAll, 0);
    ok(ret == 0, "got %#x\n", ret);

    ret = wait_multiple(2, objs, WaitAll, 0);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    KeSetEvent(&auto_event, 0, FALSE);
    KeResetEvent(&manual_event);

    ret = wait_multiple(2, objs, WaitAll, 0);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    ret = wait_single(&auto_event, 0);
    ok(ret == 0, "got %#x\n", ret);

    objs[0] = &auto_event;
    objs[1] = &manual_event;
    KeSetEvent(&manual_event, 0, FALSE);
    KeSetEvent(&auto_event, 0, FALSE);

    ret = wait_multiple(2, objs, WaitAny, 0);
    ok(ret == 0, "got %#x\n", ret);

    ret = wait_multiple(2, objs, WaitAny, 0);
    ok(ret == 1, "got %#x\n", ret);

    ret = wait_multiple(2, objs, WaitAny, 0);
    ok(ret == 1, "got %#x\n", ret);

    InitializeObjectAttributes(&attr, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
    ret = ZwCreateEvent(&handle, SYNCHRONIZE, &attr, NotificationEvent, TRUE);
    ok(!ret, "ZwCreateEvent failed: %#x\n", ret);

    ret = ObReferenceObjectByHandle(handle, SYNCHRONIZE, *pExEventObjectType, KernelMode, (void **)&event, NULL);
    ok(!ret, "ObReferenceObjectByHandle failed: %#x\n", ret);

    ret = wait_single(event, 0);
    ok(ret == 0, "got %#x\n", ret);
    KeResetEvent(event);
    ret = wait_single(event, 0);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);
    ret = wait_single_handle(handle, 0);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    KeSetEvent(event, 0, FALSE);
    ret = wait_single(event, 0);
    ok(ret == 0, "got %#x\n", ret);
    ret = wait_single_handle(handle, 0);
    ok(!ret, "got %#x\n", ret);

    ZwClose(handle);
    ObDereferenceObject(event);

    event = IoCreateSynchronizationEvent(NULL, &handle);
    ok(event != NULL, "IoCreateSynchronizationEvent failed\n");

    ret = wait_single(event, 0);
    ok(ret == 0, "got %#x\n", ret);
    KeResetEvent(event);
    ret = wait_single(event, 0);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);
    ret = wait_single_handle(handle, 0);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    ret = ZwSetEvent(handle, NULL);
    ok(!ret, "NtSetEvent returned %#x\n", ret);
    ret = wait_single(event, 0);
    ok(ret == 0, "got %#x\n", ret);
    ret = wait_single_handle(handle, 0);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    KeSetEvent(event, 0, FALSE);
    ret = wait_single_handle(handle, 0);
    ok(!ret, "got %#x\n", ret);
    ret = wait_single(event, 0);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    ret = ZwClose(handle);
    ok(!ret, "ZwClose returned %#x\n", ret);

    /* test semaphores */
    KeInitializeSemaphore(&semaphore, 0, 5);

    ret = wait_single(&semaphore, 0);
    ok(ret == STATUS_TIMEOUT, "got %u\n", ret);

    ret = KeReleaseSemaphore(&semaphore, 0, 1, FALSE);
    ok(ret == 0, "got prev %d\n", ret);

    ret = KeReleaseSemaphore(&semaphore, 0, 2, FALSE);
    ok(ret == 1, "got prev %d\n", ret);

    ret = KeReleaseSemaphore(&semaphore, 0, 1, FALSE);
    ok(ret == 3, "got prev %d\n", ret);

    for (i = 0; i < 4; i++)
    {
        ret = wait_single(&semaphore, 0);
        ok(ret == 0, "got %#x\n", ret);
    }

    ret = wait_single(&semaphore, 0);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    KeInitializeSemaphore(&semaphore2, 3, 5);

    ret = KeReleaseSemaphore(&semaphore2, 0, 1, FALSE);
    ok(ret == 3, "got prev %d\n", ret);

    for (i = 0; i < 4; i++)
    {
        ret = wait_single(&semaphore2, 0);
        ok(ret == 0, "got %#x\n", ret);
    }

    objs[0] = &semaphore;
    objs[1] = &semaphore2;

    ret = wait_multiple(2, objs, WaitAny, 0);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    KeReleaseSemaphore(&semaphore, 0, 1, FALSE);
    KeReleaseSemaphore(&semaphore2, 0, 1, FALSE);

    ret = wait_multiple(2, objs, WaitAny, 0);
    ok(ret == 0, "got %#x\n", ret);

    ret = wait_multiple(2, objs, WaitAny, 0);
    ok(ret == 1, "got %#x\n", ret);

    ret = wait_multiple(2, objs, WaitAny, 0);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    KeReleaseSemaphore(&semaphore, 0, 1, FALSE);
    KeReleaseSemaphore(&semaphore2, 0, 1, FALSE);

    ret = wait_multiple(2, objs, WaitAll, 0);
    ok(ret == 0, "got %#x\n", ret);

    ret = wait_multiple(2, objs, WaitAny, 0);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    /* test mutexes */
    KeInitializeMutex(&test_mutex, 0);

    for (i = 0; i < 10; i++)
    {
        ret = wait_single(&test_mutex, 0);
        ok(ret == 0, "got %#x\n", ret);
    }

    for (i = 0; i < 10; i++)
    {
        ret = KeReleaseMutex(&test_mutex, FALSE);
        ok(ret == i - 9, "expected %d, got %d\n", i - 9, ret);
    }

    run_thread(mutex_thread, (void *)0);

    ret = wait_single(&test_mutex, 0);
    ok(ret == 0, "got %#x\n", ret);

    run_thread(mutex_thread, (void *)STATUS_TIMEOUT);

    ret = KeReleaseMutex(&test_mutex, 0);
    ok(ret == 0, "got %#x\n", ret);

    run_thread(mutex_thread, (void *)0);

    /* test timers */
    KeInitializeTimerEx(&timer, NotificationTimer);

    timeout.QuadPart = -20 * 10000;
    KeSetTimerEx(&timer, timeout, 0, NULL);

    ret = wait_single(&timer, 0);
    ok(ret == WAIT_TIMEOUT, "got %#x\n", ret);

    ret = wait_single(&timer, -40 * 10000);
    ok(ret == 0, "got %#x\n", ret);

    ret = wait_single(&timer, 0);
    ok(ret == 0, "got %#x\n", ret);

    KeCancelTimer(&timer);
    KeInitializeTimerEx(&timer, SynchronizationTimer);

    KeSetTimerEx(&timer, timeout, 0, NULL);

    ret = wait_single(&timer, 0);
    ok(ret == WAIT_TIMEOUT, "got %#x\n", ret);

    ret = wait_single(&timer, -40 * 10000);
    ok(ret == 0, "got %#x\n", ret);

    ret = wait_single(&timer, -40 * 10000);
    ok(ret == WAIT_TIMEOUT, "got %#x\n", ret);

    KeCancelTimer(&timer);
    KeSetTimerEx(&timer, timeout, 20, NULL);

    ret = wait_single(&timer, 0);
    ok(ret == WAIT_TIMEOUT, "got %#x\n", ret);

    ret = wait_single(&timer, -40 * 10000);
    ok(ret == 0, "got %#x\n", ret);

    ret = wait_single(&timer, 0);
    ok(ret == WAIT_TIMEOUT, "got %#x\n", ret);

    ret = wait_single(&timer, -40 * 10000);
    ok(ret == 0, "got %#x\n", ret);

    ret = wait_single(&timer, -40 * 10000);
    ok(ret == 0, "got %#x\n", ret);

    KeCancelTimer(&timer);
}

static void test_call_driver(DEVICE_OBJECT *device)
{
    IO_STACK_LOCATION *irpsp;
    IO_STATUS_BLOCK iosb;
    IRP *irp = NULL;
    KEVENT event;
    NTSTATUS status;

    irp = IoBuildAsynchronousFsdRequest(IRP_MJ_FLUSH_BUFFERS, device, NULL, 0, NULL, &iosb);
    ok(irp->UserIosb == &iosb, "unexpected UserIosb\n");
    ok(!irp->Cancel, "Cancel = %x\n", irp->Cancel);
    ok(!irp->CancelRoutine, "CancelRoutine = %x\n", irp->CancelRoutine);
    ok(!irp->UserEvent, "UserEvent = %p\n", irp->UserEvent);
    ok(irp->CurrentLocation == 2, "CurrentLocation = %u\n", irp->CurrentLocation);
    ok(irp->Tail.Overlay.Thread == (PETHREAD)KeGetCurrentThread(),
       "IRP thread is not the current thread\n");

    irpsp = IoGetNextIrpStackLocation(irp);
    ok(irpsp->MajorFunction == IRP_MJ_FLUSH_BUFFERS, "MajorFunction = %u\n", irpsp->MajorFunction);
    ok(!irpsp->DeviceObject, "DeviceObject = %u\n", irpsp->DeviceObject);
    ok(!irpsp->FileObject, "FileObject = %u\n", irpsp->FileObject);
    ok(!irpsp->CompletionRoutine, "CompletionRouptine = %p\n", irpsp->CompletionRoutine);

    status = IoCallDriver(device, irp);
    ok(status == STATUS_PENDING, "IoCallDriver returned %#x\n", status);

    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;
    IoCompleteRequest(irp, IO_NO_INCREMENT);

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildSynchronousFsdRequest(IRP_MJ_FLUSH_BUFFERS, device, NULL, 0, NULL, &event, &iosb);
    ok(irp->UserIosb == &iosb, "unexpected UserIosb\n");
    ok(!irp->Cancel, "Cancel = %x\n", irp->Cancel);
    ok(!irp->CancelRoutine, "CancelRoutine = %x\n", irp->CancelRoutine);
    ok(irp->UserEvent == &event, "UserEvent = %p\n", irp->UserEvent);
    ok(irp->CurrentLocation == 2, "CurrentLocation = %u\n", irp->CurrentLocation);
    ok(irp->Tail.Overlay.Thread == (PETHREAD)KeGetCurrentThread(),
       "IRP thread is not the current thread\n");

    irpsp = IoGetNextIrpStackLocation(irp);
    ok(irpsp->MajorFunction == IRP_MJ_FLUSH_BUFFERS, "MajorFunction = %u\n", irpsp->MajorFunction);
    ok(!irpsp->DeviceObject, "DeviceObject = %u\n", irpsp->DeviceObject);
    ok(!irpsp->FileObject, "FileObject = %u\n", irpsp->FileObject);
    ok(!irpsp->CompletionRoutine, "CompletionRouptine = %p\n", irpsp->CompletionRoutine);

    status = wait_single(&event, 0);
    ok(status == STATUS_TIMEOUT, "got %#x\n", status);

    status = IoCallDriver(device, irp);
    ok(status == STATUS_PENDING, "IoCallDriver returned %#x\n", status);

    status = wait_single(&event, 0);
    ok(status == STATUS_TIMEOUT, "got %#x\n", status);

    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;
    IoCompleteRequest(irp, IO_NO_INCREMENT);

    status = wait_single(&event, 0);
    ok(status == STATUS_SUCCESS, "got %#x\n", status);
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
    ok(status == STATUS_PENDING, "IoCallDriver returned %#x\n", status);

    ok(irp->CurrentLocation == 1, "CurrentLocation = %u\n", irp->CurrentLocation);
    irpsp = IoGetCurrentIrpStackLocation(irp);
    ok(irpsp->DeviceObject == device, "DeviceObject = %u\n", irpsp->DeviceObject);

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
    ok(status == STATUS_PENDING, "IoCallDriver returned %#x\n", status);

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
    ok(status == STATUS_PENDING, "IoCallDriver returned %#x\n", status);

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
    ok(status == STATUS_PENDING, "IoCallDriver returned %#x\n", status);

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
        ok(ret == STATUS_SUCCESS, "KeExpandKernelStackAndCallout failed: %#x\n", ret);
        ok(callout_cnt == 1, "callout_cnt = %u\n", callout_cnt);
    }
    else win_skip("KeExpandKernelStackAndCallout is not available\n");

    pKeExpandKernelStackAndCalloutEx = get_proc_address("KeExpandKernelStackAndCalloutEx");
    if (pKeExpandKernelStackAndCalloutEx)
    {
        callout_cnt = 0;
        ret = pKeExpandKernelStackAndCalloutEx(callout, (void*)0xdeadbeef, 4096, FALSE, NULL);
        ok(ret == STATUS_SUCCESS, "KeExpandKernelStackAndCalloutEx failed: %#x\n", ret);
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
    ok(list.L.TotalAllocates == 0, "Expected 0 got %u\n", list.L.TotalAllocates);
    ok(list.L.AllocateMisses == 0, "Expected 0 got %u\n", list.L.AllocateMisses);
    ok(list.L.TotalFrees == 0, "Expected 0 got %u\n", list.L.TotalFrees);
    ok(list.L.FreeMisses == 0, "Expected 0 got %u\n", list.L.FreeMisses);
    ok(list.L.Type == (NonPagedPool|POOL_NX_ALLOCATION),
       "Expected NonPagedPool|POOL_NX_ALLOCATION got %u\n", list.L.Type);
    ok(list.L.Tag == tag, "Expected %x got %x\n", tag, list.L.Tag);
    ok(list.L.Size == LOOKASIDE_MINIMUM_BLOCK_SIZE,
       "Expected %u got %u\n", LOOKASIDE_MINIMUM_BLOCK_SIZE, list.L.Size);
    ok(list.L.LastTotalAllocates == 0,"Expected 0 got %u\n", list.L.LastTotalAllocates);
    ok(list.L.LastAllocateMisses == 0,"Expected 0 got %u\n", list.L.LastAllocateMisses);
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
    ok(paged_list.L.TotalAllocates == 0, "Expected 0 got %u\n", paged_list.L.TotalAllocates);
    ok(paged_list.L.AllocateMisses == 0, "Expected 0 got %u\n", paged_list.L.AllocateMisses);
    ok(paged_list.L.TotalFrees == 0, "Expected 0 got %u\n", paged_list.L.TotalFrees);
    ok(paged_list.L.FreeMisses == 0, "Expected 0 got %u\n", paged_list.L.FreeMisses);
    ok(paged_list.L.Type == (PagedPool|POOL_NX_ALLOCATION),
       "Expected PagedPool|POOL_NX_ALLOCATION got %u\n", paged_list.L.Type);
    ok(paged_list.L.Tag == tag, "Expected %x got %x\n", tag, paged_list.L.Tag);
    ok(paged_list.L.Size == LOOKASIDE_MINIMUM_BLOCK_SIZE,
       "Expected %u got %u\n", LOOKASIDE_MINIMUM_BLOCK_SIZE, paged_list.L.Size);
    ok(paged_list.L.LastTotalAllocates == 0,"Expected 0 got %u\n", paged_list.L.LastTotalAllocates);
    ok(paged_list.L.LastAllocateMisses == 0,"Expected 0 got %u\n", paged_list.L.LastAllocateMisses);
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
    ok(*pNtBuildNumber == build, "Expected build number %u, got %u\n", build, *pNtBuildNumber);
}

static void WINAPI thread_proc(void *arg)
{
    PsTerminateSystemThread(STATUS_SUCCESS);
}

static void test_ob_reference(const WCHAR *test_path)
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
    WCHAR *tmp_path;
    SIZE_T len;
    NTSTATUS status;

    static const WCHAR tmpW[] = {'.','t','m','p',0};

    pObGetObjectType = get_proc_address("ObGetObjectType");
    if (!pObGetObjectType)
        win_skip("ObGetObjectType not found\n");

    InitializeObjectAttributes(&attr, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
    status = ZwCreateEvent(&event_handle, SYNCHRONIZE, &attr, NotificationEvent, TRUE);
    ok(!status, "ZwCreateEvent failed: %#x\n", status);

    len = wcslen(test_path);
    tmp_path = ExAllocatePool(PagedPool, len * sizeof(WCHAR) + sizeof(tmpW));
    memcpy(tmp_path, test_path, len * sizeof(WCHAR));
    memcpy(tmp_path + len, tmpW, sizeof(tmpW));

    RtlInitUnicodeString(&pathU, tmp_path);
    attr.ObjectName = &pathU;
    attr.Attributes = OBJ_KERNEL_HANDLE;
    status = ZwCreateFile(&file_handle,  DELETE | FILE_WRITE_DATA | SYNCHRONIZE, &attr, &io, NULL, 0, 0, FILE_CREATE,
                          FILE_DELETE_ON_CLOSE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    ok(!status, "ZwCreateFile failed: %#x\n", status);
    ExFreePool(tmp_path);

    status = ZwDuplicateObject(NtCurrentProcess(), file_handle, NtCurrentProcess(), &file_handle2,
                               0, OBJ_KERNEL_HANDLE, DUPLICATE_SAME_ACCESS);
    ok(!status, "ZwDuplicateObject failed: %#x\n", status);

    InitializeObjectAttributes(&attr, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
    status = PsCreateSystemThread(&thread_handle, SYNCHRONIZE, &attr, NULL, NULL, thread_proc, NULL);
    ok(!status, "PsCreateSystemThread returned: %#x\n", status);

    status = ObReferenceObjectByHandle(NULL, SYNCHRONIZE, *pExEventObjectType, KernelMode, &obj1, NULL);
    ok(status == STATUS_INVALID_HANDLE, "ObReferenceObjectByHandle failed: %#x\n", status);

    status = ObReferenceObjectByHandle(event_handle, SYNCHRONIZE, *pIoFileObjectType, KernelMode, &obj1, NULL);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH, "ObReferenceObjectByHandle returned: %#x\n", status);

    status = ObReferenceObjectByHandle(event_handle, SYNCHRONIZE, NULL, KernelMode, &obj1, NULL);
    ok(!status, "ObReferenceObjectByHandle failed: %#x\n", status);

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
    ok(status == STATUS_OBJECT_TYPE_MISMATCH, "ObReferenceObjectByHandle returned: %#x\n", status);

    status = ObReferenceObjectByHandle(event_handle, SYNCHRONIZE, *pExEventObjectType, KernelMode, &obj2, NULL);
    ok(!status, "ObReferenceObjectByHandle failed: %#x\n", status);
    ok(obj1 == obj2, "obj1 != obj2\n");

    ObDereferenceObject(obj2);

    status = ObReferenceObjectByHandle(event_handle, SYNCHRONIZE, NULL, KernelMode, &obj2, NULL);
    ok(!status, "ObReferenceObjectByHandle failed: %#x\n", status);
    ok(obj1 == obj2, "obj1 != obj2\n");

    ObDereferenceObject(obj2);
    ObDereferenceObject(obj1);

    status = ObReferenceObjectByHandle(file_handle, SYNCHRONIZE, *pIoFileObjectType, KernelMode, &obj1, NULL);
    ok(!status, "ObReferenceObjectByHandle failed: %#x\n", status);

    status = ObReferenceObjectByHandle(file_handle2, SYNCHRONIZE, *pIoFileObjectType, KernelMode, &obj2, NULL);
    ok(!status, "ObReferenceObjectByHandle failed: %#x\n", status);
    ok(obj1 == obj2, "obj1 != obj2\n");

    file = obj1;
    ok(file->Type == 5, "Type = %u\n", file->Type);

    ObDereferenceObject(obj1);
    ObDereferenceObject(obj2);

    status = ObReferenceObjectByHandle(thread_handle, SYNCHRONIZE, *pPsThreadType, KernelMode, &obj1, NULL);
    ok(!status, "ObReferenceObjectByHandle failed: %#x\n", status);

    status = ObReferenceObjectByHandle(thread_handle, SYNCHRONIZE, *pPsThreadType, KernelMode, &obj2, NULL);
    ok(!status, "ObReferenceObjectByHandle failed: %#x\n", status);
    ok(obj1 == obj2, "obj1 != obj2\n");

    header = obj1;
    ok(header->Type == 6, "Type = %u\n", header->Type);

    status = wait_single(header, 0);
    ok(status == 0 || status == STATUS_TIMEOUT, "got %#x\n", status);

    ObDereferenceObject(obj2);

    status = ObOpenObjectByPointer(obj1, OBJ_KERNEL_HANDLE, NULL, 0, NULL, KernelMode, &handle);
    ok(status == STATUS_SUCCESS, "ObOpenObjectByPointer failed: %#x\n", status);

    status = ZwClose(handle);
    ok(!status, "ZwClose failed: %#x\n", status);

    status = ObReferenceObjectByHandle(thread_handle, SYNCHRONIZE, *pPsThreadType, KernelMode, &obj2, NULL);
    ok(!status, "ObReferenceObjectByHandle failed: %#x\n", status);
    ok(obj1 == obj2, "obj1 != obj2\n");
    ObDereferenceObject(obj2);

    status = ObOpenObjectByPointer(obj1, OBJ_KERNEL_HANDLE, NULL, 0, *pIoFileObjectType, KernelMode, &handle);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH, "ObOpenObjectByPointer returned: %#x\n", status);

    ObDereferenceObject(obj1);

    status = ZwClose(thread_handle);
    ok(!status, "ZwClose failed: %#x\n", status);

    status = ZwClose(event_handle);
    ok(!status, "ZwClose failed: %#x\n", status);

    status = ZwClose(file_handle);
    ok(!status, "ZwClose failed: %#x\n", status);

    status = ZwClose(file_handle2);
    ok(!status, "ZwClose failed: %#x\n", status);
}

static void check_resource_(int line, ERESOURCE *resource, ULONG exclusive_waiters,
        ULONG shared_waiters, BOOLEAN exclusive, ULONG shared_count)
{
    BOOLEAN ret;
    ULONG count;

    count = ExGetExclusiveWaiterCount(resource);
    ok_(__FILE__, line, count == exclusive_waiters,
            "expected %u exclusive waiters, got %u\n", exclusive_waiters, count);
    count = ExGetSharedWaiterCount(resource);
    ok_(__FILE__, line, count == shared_waiters,
            "expected %u shared waiters, got %u\n", shared_waiters, count);
    ret = ExIsResourceAcquiredExclusiveLite(resource);
    ok_(__FILE__, line, ret == exclusive,
            "expected exclusive %u, got %u\n", exclusive, ret);
    count = ExIsResourceAcquiredSharedLite(resource);
    ok_(__FILE__, line, count == shared_count,
            "expected shared %u, got %u\n", shared_count, count);
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
    ok(status == STATUS_SUCCESS, "got status %#x\n", status);
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
    ok(status == STATUS_SUCCESS, "got status %#x\n", status);
}

static void test_lookup_thread(void)
{
    NTSTATUS status;
    PETHREAD thread = NULL;

    status = PsLookupThreadByThreadId(PsGetCurrentThreadId(), &thread);
    ok(!status, "PsLookupThreadByThreadId failed: %#x\n", status);
    ok((PKTHREAD)thread == KeGetCurrentThread(), "thread != KeGetCurrentThread\n");
    if (thread) ObDereferenceObject(thread);

    status = PsLookupThreadByThreadId(NULL, &thread);
    ok(status == STATUS_INVALID_CID || broken(status == STATUS_INVALID_PARAMETER) /* winxp */,
       "PsLookupThreadByThreadId returned %#x\n", status);
}

static void test_stack_limits(void)
{
    ULONG_PTR low = 0, high = 0;

    IoGetStackLimits(&low, &high);
    ok(low, "low = 0\n");
    ok(low < high, "low >= high\n");
    ok(low < (ULONG_PTR)&low && (ULONG_PTR)&low < high, "stack variable is not in stack limits\n");
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

static PIO_WORKITEM main_test_work_item;

static void WINAPI main_test_task(DEVICE_OBJECT *device, void *context)
{
    IRP *irp = context;
    void *buffer = irp->AssociatedIrp.SystemBuffer;

    IoFreeWorkItem(main_test_work_item);
    main_test_work_item = NULL;

    test_current_thread(TRUE);
    test_critical_region(FALSE);
    test_call_driver(device);
    test_cancel_irp(device);
    test_stack_limits();

    /* print process report */
    if (winetest_debug)
    {
        kprintf("%04x:ntoskrnl: %d tests executed (%d marked as todo, %d %s), %d skipped.\n",
            PsGetCurrentProcessId(), successes + failures + todo_successes + todo_failures,
            todo_successes, failures + todo_failures,
            (failures + todo_failures != 1) ? "failures" : "failure", skipped );
    }
    ZwClose(okfile);

    *((LONG *)buffer) = failures;
    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = sizeof(failures);
    IoCompleteRequest(irp, IO_NO_INCREMENT);
}

static NTSTATUS main_test(DEVICE_OBJECT *device, IRP *irp, IO_STACK_LOCATION *stack)
{
    ULONG length = stack->Parameters.DeviceIoControl.OutputBufferLength;
    void *buffer = irp->AssociatedIrp.SystemBuffer;
    struct test_input *test_input = (struct test_input *)buffer;
    OBJECT_ATTRIBUTES attr = {0};
    UNICODE_STRING pathU;
    IO_STATUS_BLOCK io;

    if (!buffer)
        return STATUS_ACCESS_VIOLATION;
    if (length < sizeof(failures))
        return STATUS_BUFFER_TOO_SMALL;

    attr.Length = sizeof(attr);
    RtlInitUnicodeString(&pathU, test_input->path);
    running_under_wine = test_input->running_under_wine;
    winetest_debug = test_input->winetest_debug;
    winetest_report_success = test_input->winetest_report_success;
    attr.ObjectName = &pathU;
    attr.Attributes = OBJ_KERNEL_HANDLE; /* needed to be accessible from system threads */
    ZwOpenFile(&okfile, FILE_APPEND_DATA | SYNCHRONIZE, &attr, &io, 0, FILE_SYNCHRONOUS_IO_NONALERT);

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
    test_version();
    test_stack_callout();
    test_lookaside_list();
    test_ob_reference(test_input->path);
    test_resource();
    test_lookup_thread();
    test_IoAttachDeviceToDeviceStack();

    if (main_test_work_item) return STATUS_UNEXPECTED_IO_ERROR;

    main_test_work_item = IoAllocateWorkItem(lower_device);
    ok(main_test_work_item != NULL, "main_test_work_item = NULL\n");

    IoQueueWorkItem(main_test_work_item, main_test_task, DelayedWorkQueue, irp);
    return STATUS_PENDING;
}

static NTSTATUS test_basic_ioctl(IRP *irp, IO_STACK_LOCATION *stack, ULONG_PTR *info)
{
    ULONG length = stack->Parameters.DeviceIoControl.OutputBufferLength;
    char *buffer = irp->AssociatedIrp.SystemBuffer;

    if (!buffer)
        return STATUS_ACCESS_VIOLATION;

    if (length < sizeof(teststr))
        return STATUS_BUFFER_TOO_SMALL;

    memcpy(buffer, teststr, sizeof(teststr));
    *info = sizeof(teststr);

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
    DWORD *context = stack->FileObject->FsContext;

    if (!buffer || !context)
        return STATUS_ACCESS_VIOLATION;

    if (length < sizeof(DWORD))
        return STATUS_BUFFER_TOO_SMALL;

    *(DWORD*)buffer = *context;
    *info = sizeof(DWORD);
    return STATUS_SUCCESS;
}

static NTSTATUS return_status(IRP *irp, IO_STACK_LOCATION *stack, ULONG_PTR *info)
{
    char *buffer = irp->AssociatedIrp.SystemBuffer;
    NTSTATUS ret;

    if (!buffer)
        return STATUS_ACCESS_VIOLATION;

    if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(DWORD)
            || stack->Parameters.DeviceIoControl.OutputBufferLength < 3)
        return STATUS_BUFFER_TOO_SMALL;

    ret = *(DWORD *)irp->AssociatedIrp.SystemBuffer;
    memcpy(buffer, "ghi", 3);
    *info = 3;
    return ret;
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

static NTSTATUS WINAPI driver_Create(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    DWORD *context = ExAllocatePool(PagedPool, sizeof(*context));

    last_created_file = irpsp->FileObject;
    ++create_count;
    if (context)
        *context = create_count;
    irpsp->FileObject->FsContext = context;
    create_caller_thread = KeGetCurrentThread();

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
        case IOCTL_WINETEST_RETURN_STATUS:
            status = return_status(irp, stack, &irp->IoStatus.Information);
            break;
        case IOCTL_WINETEST_DETACH:
            IoDetachDevice(lower_device);
            status = STATUS_SUCCESS;
            break;
        default:
            break;
    }

    if (status != STATUS_PENDING)
    {
        irp->IoStatus.Status = status;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }
    else IoMarkIrpPending(irp);
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

    RtlInitUnicodeString(&linkW, driver_link);
    IoDeleteSymbolicLink(&linkW);

    IoDeleteDevice(upper_device);
    IoDeleteDevice(lower_device);
}

NTSTATUS WINAPI DriverEntry(DRIVER_OBJECT *driver, PUNICODE_STRING registry)
{
    UNICODE_STRING nameW, linkW;
    NTSTATUS status;

    DbgPrint("loading driver\n");

    driver_obj = driver;

    /* Allow unloading of the driver */
    driver->DriverUnload = driver_Unload;

    /* Set driver functions */
    driver->MajorFunction[IRP_MJ_CREATE]            = driver_Create;
    driver->MajorFunction[IRP_MJ_DEVICE_CONTROL]    = driver_IoControl;
    driver->MajorFunction[IRP_MJ_FLUSH_BUFFERS]     = driver_FlushBuffers;
    driver->MajorFunction[IRP_MJ_CLOSE]             = driver_Close;

    RtlInitUnicodeString(&nameW, device_name);
    RtlInitUnicodeString(&linkW, driver_link);

    if (!(status = IoCreateDevice(driver, 0, &nameW, FILE_DEVICE_UNKNOWN,
                                  FILE_DEVICE_SECURE_OPEN, FALSE, &lower_device)))
    {
        status = IoCreateSymbolicLink(&linkW, &nameW);
        lower_device->Flags &= ~DO_DEVICE_INITIALIZING;
    }

    if (!status)
    {
        RtlInitUnicodeString(&nameW, upper_name);

        status = IoCreateDevice(driver, 0, &nameW, FILE_DEVICE_UNKNOWN,
                                FILE_DEVICE_SECURE_OPEN, FALSE, &upper_device);
    }

    if (!status)
    {
        IoAttachDeviceToDeviceStack(upper_device, lower_device);
        upper_device->Flags &= ~DO_DEVICE_INITIALIZING;
    }

    return status;
}
