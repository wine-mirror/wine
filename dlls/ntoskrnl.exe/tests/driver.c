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
    LARGE_INTEGER offset;
    IO_STATUS_BLOCK io;

    _vsnprintf(buffer, sizeof(buffer), format, ap);
    offset.QuadPart = -1;
    ZwWriteFile(okfile, NULL, NULL, NULL, &io, buffer, strlen(buffer), &offset, NULL);
}

static void WINAPIV kprintf(const char *format, ...)
{
    __ms_va_list valist;

    __ms_va_start(valist, format);
    kvprintf(format, valist);
    __ms_va_end(valist);
}

static void WINAPIV ok_(const char *file, int line, int condition, const char *msg, ...)
{
    const char *current_file;
    __ms_va_list args;

    if (!(current_file = strrchr(file, '/')) &&
        !(current_file = strrchr(file, '\\')))
        current_file = file;
    else
        current_file++;

    __ms_va_start(args, msg);
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

static void test_currentprocess(void)
{
    PEPROCESS current;

    current = IoGetCurrentProcess();
todo_wine
    ok(current != NULL, "Expected current process to be non-NULL\n");
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

static NTSTATUS main_test(IRP *irp, IO_STACK_LOCATION *stack, ULONG_PTR *info)
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
    ZwOpenFile(&okfile, FILE_APPEND_DATA, &attr, &io, 0, 0);

    test_currentprocess();
    test_mdl_map();
    test_init_funcs();

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

static NTSTATUS WINAPI driver_Create(DEVICE_OBJECT *device, IRP *irp)
{
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
            status = main_test(irp, stack, &irp->IoStatus.Information);
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
