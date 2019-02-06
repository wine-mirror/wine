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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"
#include "ddk/ntddk.h"
#include "ddk/wdm.h"

#include "driver.h"

static const WCHAR driver_device[] = {'\\','D','e','v','i','c','e',
                                      '\\','W','i','n','e','T','e','s','t','D','r','i','v','e','r',0};
static const WCHAR driver_link[] = {'\\','D','o','s','D','e','v','i','c','e','s',
                                    '\\','W','i','n','e','T','e','s','t','D','r','i','v','e','r',0};

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

extern int CDECL _vsnprintf(char *str, size_t len, const char *format, __ms_va_list argptr);

static void kvprintf(const char *format, __ms_va_list ap)
{
    static char buffer[512];
    IO_STATUS_BLOCK io;

    _vsnprintf(buffer, sizeof(buffer), format, ap);
    ZwWriteFile(okfile, NULL, NULL, NULL, &io, buffer, strlen(buffer), NULL, NULL);
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

void vskip_(const char *file, int line, const char *msg, __ms_va_list args)
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

void WINAPIV win_skip_(const char *file, int line, const char *msg, ...)
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

static void test_currentprocess(void)
{
    PEPROCESS current;

    current = IoGetCurrentProcess();
todo_wine
    ok(current != NULL, "Expected current process to be non-NULL\n");
}

static FILE_OBJECT *last_created_file;

static void test_irp_struct(IRP *irp, DEVICE_OBJECT *device)
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );

    ok(last_created_file != NULL, "last_created_file = NULL\n");
    ok(irpsp->FileObject == last_created_file, "FileObject != last_created_file\n");
    ok(irpsp->DeviceObject == device, "unexpected DeviceObject\n");
    ok(irpsp->FileObject->DeviceObject == device, "unexpected FileObject->DeviceObject\n");
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

    /* MmUnmapLockedPages(addr, mdl); */

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

static void run_thread(PKSTART_ROUTINE proc, void *arg)
{
    OBJECT_ATTRIBUTES attr = {0};
    HANDLE thread;
    NTSTATUS ret;

    attr.Length = sizeof(attr);
    attr.Attributes = OBJ_KERNEL_HANDLE;
    ret = PsCreateSystemThread(&thread, THREAD_ALL_ACCESS, &attr, NULL, NULL, proc, arg);
    ok(!ret, "got %#x\n", ret);

    ret = ZwWaitForSingleObject(thread, FALSE, NULL);
    ok(!ret, "got %#x\n", ret);
    ret = ZwClose(thread);
    ok(!ret, "got %#x\n", ret);
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
    KEVENT manual_event, auto_event;
    KTIMER timer;
    LARGE_INTEGER timeout;
    void *objs[2];
    NTSTATUS ret;
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

    timeout.QuadPart = -100;
    KeSetTimerEx(&timer, timeout, 0, NULL);

    ret = wait_single(&timer, 0);
    ok(ret == WAIT_TIMEOUT, "got %#x\n", ret);

    ret = wait_single(&timer, -200);
    ok(ret == 0, "got %#x\n", ret);

    ret = wait_single(&timer, 0);
    ok(ret == 0, "got %#x\n", ret);

    KeCancelTimer(&timer);
    KeInitializeTimerEx(&timer, SynchronizationTimer);

    KeSetTimerEx(&timer, timeout, 0, NULL);

    ret = wait_single(&timer, 0);
    ok(ret == WAIT_TIMEOUT, "got %#x\n", ret);

    ret = wait_single(&timer, -200);
    ok(ret == 0, "got %#x\n", ret);

    ret = wait_single(&timer, 0);
    ok(ret == WAIT_TIMEOUT, "got %#x\n", ret);

    KeCancelTimer(&timer);
    KeSetTimerEx(&timer, timeout, 10, NULL);

    ret = wait_single(&timer, 0);
    ok(ret == WAIT_TIMEOUT, "got %#x\n", ret);

    ret = wait_single(&timer, -200);
    ok(ret == 0, "got %#x\n", ret);

    ret = wait_single(&timer, 0);
    ok(ret == WAIT_TIMEOUT, "got %#x\n", ret);

    ret = wait_single(&timer, -20 * 10000);
    ok(ret == 0, "got %#x\n", ret);

    ret = wait_single(&timer, -20 * 10000);
    ok(ret == 0, "got %#x\n", ret);

    KeCancelTimer(&timer);
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

static NTSTATUS main_test(DEVICE_OBJECT *device, IRP *irp, IO_STACK_LOCATION *stack, ULONG_PTR *info)
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

    test_irp_struct(irp, device);
    test_currentprocess();
    test_mdl_map();
    test_init_funcs();
    test_load_driver();
    test_sync();
    test_version();
    test_stack_callout();
    test_lookaside_list();

    /* print process report */
    if (test_input->winetest_debug)
    {
        kprintf("%04x:ntoskrnl: %d tests executed (%d marked as todo, %d %s), %d skipped.\n",
            PsGetCurrentProcessId(), successes + failures + todo_successes + todo_failures,
            todo_successes, failures + todo_failures,
            (failures + todo_failures != 1) ? "failures" : "failure", skipped );
    }
    ZwClose(okfile);
    *((LONG *)buffer) = failures;
    *info = sizeof(failures);
    return STATUS_SUCCESS;
}

static NTSTATUS test_basic_ioctl(IRP *irp, IO_STACK_LOCATION *stack, ULONG_PTR *info)
{
    ULONG length = stack->Parameters.DeviceIoControl.OutputBufferLength;
    char *buffer = irp->AssociatedIrp.SystemBuffer;

    if (!buffer)
        return STATUS_ACCESS_VIOLATION;

    if (length < sizeof(teststr))
        return STATUS_BUFFER_TOO_SMALL;

    strcpy(buffer, teststr);
    *info = sizeof(teststr);

    return STATUS_SUCCESS;
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

    last_created_file = irpsp->FileObject;

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
            status = main_test(device, irp, stack, &irp->IoStatus.Information);
            break;
        case IOCTL_WINETEST_LOAD_DRIVER:
            status = test_load_driver_ioctl(irp, stack, &irp->IoStatus.Information);
            break;
        default:
            break;
    }

    irp->IoStatus.Status = status;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return status;
}

static NTSTATUS WINAPI driver_Close(DEVICE_OBJECT *device, IRP *irp)
{
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

    IoDeleteDevice(driver->DeviceObject);
}

NTSTATUS WINAPI DriverEntry(DRIVER_OBJECT *driver, PUNICODE_STRING registry)
{
    UNICODE_STRING nameW, linkW;
    DEVICE_OBJECT *device;
    NTSTATUS status;

    DbgPrint("loading driver\n");

    /* Allow unloading of the driver */
    driver->DriverUnload = driver_Unload;

    /* Set driver functions */
    driver->MajorFunction[IRP_MJ_CREATE]            = driver_Create;
    driver->MajorFunction[IRP_MJ_DEVICE_CONTROL]    = driver_IoControl;
    driver->MajorFunction[IRP_MJ_CLOSE]             = driver_Close;

    RtlInitUnicodeString(&nameW, driver_device);
    RtlInitUnicodeString(&linkW, driver_link);

    if (!(status = IoCreateDevice(driver, 0, &nameW, FILE_DEVICE_UNKNOWN,
                                  FILE_DEVICE_SECURE_OPEN, FALSE, &device)))
        status = IoCreateSymbolicLink(&linkW, &nameW);

    return status;
}
