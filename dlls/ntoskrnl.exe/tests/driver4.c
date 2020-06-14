/*
 * ntoskrnl.exe testing framework
 *
 * Copyright 2020 Paul Gofman for Codeweavers
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
#include "ddk/wsk.h"

#include "driver.h"

#include "utils.h"

static DRIVER_OBJECT *driver_obj;
static DEVICE_OBJECT *lower_device, *upper_device;

static unsigned int create_count, close_count;

static const WCHAR driver_link[] = L"\\DosDevices\\WineTestDriver4";
static const WCHAR device_name[] = L"\\Device\\WineTestDriver4";
static const WCHAR upper_name[] = L"\\Device\\WineTestUpper4";

static FILE_OBJECT *last_created_file;
static void *create_caller_thread;
static PETHREAD create_irp_thread;

static POBJECT_TYPE *pIoDriverObjectType;

static WSK_CLIENT_NPI client_npi;
static WSK_REGISTRATION registration;
static WSK_PROVIDER_NPI provider_npi;
static KEVENT irp_complete_event;
static IRP *wsk_irp;

static NTSTATUS WINAPI irp_completion_routine(DEVICE_OBJECT *reserved, IRP *irp, void *context)
{
    KEVENT *event = context;

    KeSetEvent(event, 1, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

static void netio_init(void)
{
    static const WSK_CLIENT_DISPATCH client_dispatch =
    {
        MAKE_WSK_VERSION(1, 0), 0, NULL
    };
    NTSTATUS status;

    client_npi.Dispatch = &client_dispatch;
    status = WskRegister(&client_npi, &registration);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#x.\n", status);

    status = WskCaptureProviderNPI(&registration, WSK_INFINITE_WAIT, &provider_npi);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#x.\n", status);

    ok(provider_npi.Dispatch->Version >= MAKE_WSK_VERSION(1, 0), "Got unexpected version %#x.\n",
            provider_npi.Dispatch->Version);
    ok(!!provider_npi.Client, "Got null WSK_CLIENT.\n");

    KeInitializeEvent(&irp_complete_event, SynchronizationEvent, FALSE);
    wsk_irp = IoAllocateIrp(1, FALSE);
}

static void netio_uninit(void)
{
    IoFreeIrp(wsk_irp);
    WskReleaseProviderNPI(&registration);
    WskDeregister(&registration);
}

static void test_wsk_get_address_info(void)
{
    UNICODE_STRING node_name, service_name;
    ADDRINFOEXW *result, *addr_info;
    unsigned int count;
    NTSTATUS status;

    RtlInitUnicodeString(&service_name, L"12345");

    wsk_irp->IoStatus.Status = 0xdeadbeef;
    wsk_irp->IoStatus.Information = 0xdeadbeef;
    status = provider_npi.Dispatch->WskGetAddressInfo(provider_npi.Client, &node_name, &service_name,
            NS_ALL, NULL, NULL, &result, NULL, NULL, NULL);
    ok(status == STATUS_INVALID_PARAMETER, "Got unexpected status %#x.\n", status);
    ok(wsk_irp->IoStatus.Status == 0xdeadbeef, "Got unexpected status %#x.\n", wsk_irp->IoStatus.Status);
    ok(wsk_irp->IoStatus.Information == 0xdeadbeef, "Got unexpected Information %#lx.\n",
            wsk_irp->IoStatus.Information);

    RtlInitUnicodeString(&node_name, L"dead.beef");

    IoReuseIrp(wsk_irp, STATUS_UNSUCCESSFUL);
    IoSetCompletionRoutine(wsk_irp, irp_completion_routine, &irp_complete_event, TRUE, TRUE, TRUE);
    wsk_irp->IoStatus.Status = 0xdeadbeef;
    wsk_irp->IoStatus.Information = 0xdeadbeef;
    status = provider_npi.Dispatch->WskGetAddressInfo(provider_npi.Client, &node_name, &service_name,
            NS_ALL, NULL, NULL,  &result, NULL, NULL, wsk_irp);
    ok(status == STATUS_PENDING, "Got unexpected status %#x.\n", status);
    status = KeWaitForSingleObject(&irp_complete_event, Executive, KernelMode, FALSE, NULL);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#x.\n", status);
    ok(wsk_irp->IoStatus.Status == STATUS_NOT_FOUND
            || broken(wsk_irp->IoStatus.Status == STATUS_NO_MATCH) /* Win7 */,
            "Got unexpected status %#x.\n", wsk_irp->IoStatus.Status);
    ok(wsk_irp->IoStatus.Information == 0, "Got unexpected Information %#lx.\n",
            wsk_irp->IoStatus.Information);

    RtlInitUnicodeString(&node_name, L"127.0.0.1");
    IoReuseIrp(wsk_irp, STATUS_UNSUCCESSFUL);
    IoSetCompletionRoutine(wsk_irp, irp_completion_routine, &irp_complete_event, TRUE, TRUE, TRUE);
    wsk_irp->IoStatus.Status = 0xdeadbeef;
    wsk_irp->IoStatus.Information = 0xdeadbeef;
    status = provider_npi.Dispatch->WskGetAddressInfo(provider_npi.Client, &node_name, &service_name,
            NS_ALL, NULL, NULL, &result, NULL, NULL, wsk_irp);
    ok(status == STATUS_PENDING, "Got unexpected status %#x.\n", status);
    status = KeWaitForSingleObject(&irp_complete_event, Executive, KernelMode, FALSE, NULL);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#x.\n", status);
    ok(wsk_irp->IoStatus.Status == STATUS_SUCCESS, "Got unexpected status %#x.\n", wsk_irp->IoStatus.Status);
    ok(wsk_irp->IoStatus.Information == 0, "Got unexpected Information %#lx.\n",
            wsk_irp->IoStatus.Information);

    count = 0;
    addr_info = result;
    while (addr_info)
    {
        struct sockaddr_in *addr = (struct sockaddr_in *)addr_info->ai_addr;

        ok(addr_info->ai_addrlen == sizeof(*addr), "Got unexpected ai_addrlen %u.\n", addr_info->ai_addrlen);
        ok(addr->sin_family == AF_INET, "Got unexpected sin_family %u.\n", addr->sin_family);
        ok(ntohs(addr->sin_port) == 12345, "Got unexpected sin_port %u.\n", ntohs(addr->sin_port));
        ok(ntohl(addr->sin_addr.s_addr) == 0x7f000001, "Got unexpected sin_addr %#x.\n",
                ntohl(addr->sin_addr.s_addr));

        ++count;
        addr_info = addr_info->ai_next;
    }
    ok(count, "Got zero addr_info count.\n");
    provider_npi.Dispatch->WskFreeAddressInfo(provider_npi.Client, result);
}

static NTSTATUS main_test(DEVICE_OBJECT *device, IRP *irp, IO_STACK_LOCATION *stack)
{
    ULONG length = stack->Parameters.DeviceIoControl.OutputBufferLength;
    void *buffer = irp->AssociatedIrp.SystemBuffer;
    struct test_input *test_input = buffer;
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

    netio_init();
    test_wsk_get_address_info();

    if (winetest_debug)
    {
        kprintf("%04x:ntoskrnl: %d tests executed (%d marked as todo, %d %s), %d skipped.\n",
            PsGetCurrentProcessId(), successes + failures + todo_successes + todo_failures,
            todo_successes, failures + todo_failures,
            (failures + todo_failures != 1) ? "failures" : "failure", skipped );
    }
    ZwClose(okfile);

    *((LONG *)buffer) = failures;
    irp->IoStatus.Information = sizeof(failures);
    return STATUS_SUCCESS;
}

static NTSTATUS WINAPI driver_iocontrol(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    NTSTATUS status = STATUS_NOT_SUPPORTED;

    switch (stack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_WINETEST_MAIN_TEST:
            status = main_test(device, irp, stack);
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
    return status;
}

static NTSTATUS WINAPI driver_create(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    DWORD *context = ExAllocatePool(PagedPool, sizeof(*context));

    last_created_file = irpsp->FileObject;
    ++create_count;
    if (context)
        *context = create_count;
    irpsp->FileObject->FsContext = context;
    create_caller_thread = KeGetCurrentThread();
    create_irp_thread = irp->Tail.Overlay.Thread;

    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static NTSTATUS WINAPI driver_close(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);

    netio_uninit();

    ++close_count;
    if (stack->FileObject->FsContext)
        ExFreePool(stack->FileObject->FsContext);
    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static VOID WINAPI driver_unload(DRIVER_OBJECT *driver)
{
    UNICODE_STRING linkW;

    DbgPrint("Unloading driver.\n");

    RtlInitUnicodeString(&linkW, driver_link);
    IoDeleteSymbolicLink(&linkW);

    IoDeleteDevice(upper_device);
    IoDeleteDevice(lower_device);
}

NTSTATUS WINAPI DriverEntry(DRIVER_OBJECT *driver, PUNICODE_STRING registry)
{
    static const WCHAR IoDriverObjectTypeW[] = L"IoDriverObjectType";
    static const WCHAR driver_nameW[] = L"\\Driver\\WineTestDriver4";
    UNICODE_STRING nameW, linkW;
    NTSTATUS status;
    void *obj;

    DbgPrint("Loading driver.\n");

    driver_obj = driver;

    driver->DriverUnload = driver_unload;
    driver->MajorFunction[IRP_MJ_CREATE]            = driver_create;
    driver->MajorFunction[IRP_MJ_DEVICE_CONTROL]    = driver_iocontrol;
    driver->MajorFunction[IRP_MJ_CLOSE]             = driver_close;

    RtlInitUnicodeString(&nameW, IoDriverObjectTypeW);
    pIoDriverObjectType = MmGetSystemRoutineAddress(&nameW);

    RtlInitUnicodeString(&nameW, driver_nameW);
    if ((status = ObReferenceObjectByName(&nameW, 0, NULL, 0, *pIoDriverObjectType, KernelMode, NULL, &obj)))
        return status;
    if (obj != driver)
    {
        ObDereferenceObject(obj);
        return STATUS_UNSUCCESSFUL;
    }
    ObDereferenceObject(obj);

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
