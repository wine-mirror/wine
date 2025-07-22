/*
 * Unit tests for netio.sys (kernel sockets)
 *
 * Copyright 2020 Paul Gofman for CodeWeavers
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
#include "ddk/wsk.h"

#include "driver.h"

#include "utils.h"

#define htonl(x) RtlUlongByteSwap(x)
#define htons(x) RtlUshortByteSwap(x)
#define ntohl(x) RtlUlongByteSwap(x)
#define ntohs(x) RtlUshortByteSwap(x)

static DRIVER_OBJECT *driver_obj;
static DEVICE_OBJECT *device_obj;

static const WCHAR driver_link[] = L"\\DosDevices\\winetest_netio";
static const WCHAR device_name[] = L"\\Device\\winetest_netio";

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
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);

    status = WskCaptureProviderNPI(&registration, WSK_INFINITE_WAIT, &provider_npi);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);

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
    ok(status == STATUS_INVALID_PARAMETER, "Got unexpected status %#lx.\n", status);
    ok(wsk_irp->IoStatus.Status == 0xdeadbeef, "Got unexpected status %#lx.\n", wsk_irp->IoStatus.Status);
    ok(wsk_irp->IoStatus.Information == 0xdeadbeef, "Got unexpected Information %#Ix.\n",
            wsk_irp->IoStatus.Information);

    RtlInitUnicodeString(&node_name, L"dead.beef");

    IoReuseIrp(wsk_irp, STATUS_UNSUCCESSFUL);
    IoSetCompletionRoutine(wsk_irp, irp_completion_routine, &irp_complete_event, TRUE, TRUE, TRUE);
    wsk_irp->IoStatus.Status = 0xdeadbeef;
    wsk_irp->IoStatus.Information = 0xdeadbeef;
    status = provider_npi.Dispatch->WskGetAddressInfo(provider_npi.Client, &node_name, &service_name,
            NS_ALL, NULL, NULL,  &result, NULL, NULL, wsk_irp);
    ok(status == STATUS_PENDING, "Got unexpected status %#lx.\n", status);
    status = KeWaitForSingleObject(&irp_complete_event, Executive, KernelMode, FALSE, NULL);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
    ok(wsk_irp->IoStatus.Status == STATUS_NOT_FOUND
            || broken(wsk_irp->IoStatus.Status == STATUS_NO_MATCH) /* Win7 */,
            "Got unexpected status %#lx.\n", wsk_irp->IoStatus.Status);
    ok(wsk_irp->IoStatus.Information == 0, "Got unexpected Information %#Ix.\n",
            wsk_irp->IoStatus.Information);

    RtlInitUnicodeString(&node_name, L"127.0.0.1");
    IoReuseIrp(wsk_irp, STATUS_UNSUCCESSFUL);
    IoSetCompletionRoutine(wsk_irp, irp_completion_routine, &irp_complete_event, TRUE, TRUE, TRUE);
    wsk_irp->IoStatus.Status = 0xdeadbeef;
    wsk_irp->IoStatus.Information = 0xdeadbeef;
    status = provider_npi.Dispatch->WskGetAddressInfo(provider_npi.Client, &node_name, &service_name,
            NS_ALL, NULL, NULL, &result, NULL, NULL, wsk_irp);
    ok(status == STATUS_PENDING, "Got unexpected status %#lx.\n", status);
    status = KeWaitForSingleObject(&irp_complete_event, Executive, KernelMode, FALSE, NULL);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
    ok(wsk_irp->IoStatus.Status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", wsk_irp->IoStatus.Status);
    ok(wsk_irp->IoStatus.Information == 0, "Got unexpected Information %#Ix.\n",
            wsk_irp->IoStatus.Information);

    count = 0;
    addr_info = result;
    while (addr_info)
    {
        struct sockaddr_in *addr = (struct sockaddr_in *)addr_info->ai_addr;

        ok(addr_info->ai_addrlen == sizeof(*addr), "Got unexpected ai_addrlen %I64u.\n", (UINT64)addr_info->ai_addrlen);
        ok(addr->sin_family == AF_INET, "Got unexpected sin_family %u.\n", addr->sin_family);
        ok(ntohs(addr->sin_port) == 12345, "Got unexpected sin_port %u.\n", ntohs(addr->sin_port));
        ok(ntohl(addr->sin_addr.s_addr) == INADDR_LOOPBACK, "Got unexpected sin_addr %#lx.\n",
                ntohl(addr->sin_addr.s_addr));

        ++count;
        addr_info = addr_info->ai_next;
    }
    ok(count, "Got zero addr_info count.\n");
    provider_npi.Dispatch->WskFreeAddressInfo(provider_npi.Client, result);
}

struct socket_context
{
};

#define TEST_BUFFER_LENGTH 256

static void test_wsk_listen_socket(void)
{
    static const char test_receive_string[] = "Client test string 1.";
    const WSK_PROVIDER_LISTEN_DISPATCH *tcp_dispatch, *udp_dispatch;
    static const char test_send_string[] = "Server test string 1.";
    static const WSK_CLIENT_LISTEN_DISPATCH client_listen_dispatch;
    const WSK_PROVIDER_CONNECTION_DISPATCH *accept_dispatch;
    WSK_SOCKET *tcp_socket, *udp_socket, *accept_socket;
    struct sockaddr_in addr, local_addr, remote_addr;
    struct socket_context context;
    WSK_BUF wsk_buf1, wsk_buf2;
    void *buffer1, *buffer2;
    LARGE_INTEGER timeout;
    MDL *mdl1, *mdl2;
    NTSTATUS status;
    KEVENT event;
    IRP *irp;

    irp = IoAllocateIrp(1, FALSE);
    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    buffer1 = ExAllocatePool(NonPagedPool, TEST_BUFFER_LENGTH);
    mdl1 = IoAllocateMdl(buffer1, TEST_BUFFER_LENGTH, FALSE, FALSE, NULL);
    MmBuildMdlForNonPagedPool(mdl1);
    buffer2 = ExAllocatePool(NonPagedPool, TEST_BUFFER_LENGTH);
    mdl2 = IoAllocateMdl(buffer2, TEST_BUFFER_LENGTH, FALSE, FALSE, NULL);
    MmBuildMdlForNonPagedPool(mdl2);

    wsk_buf1.Mdl = mdl1;
    wsk_buf1.Offset = 0;
    wsk_buf1.Length = TEST_BUFFER_LENGTH;
    wsk_buf2 = wsk_buf1;
    wsk_buf2.Mdl = mdl2;

    status = provider_npi.Dispatch->WskSocket(NULL, AF_INET, SOCK_STREAM, IPPROTO_TCP,
            WSK_FLAG_LISTEN_SOCKET, &context, &client_listen_dispatch, NULL, NULL, NULL, NULL);
    ok(status == STATUS_INVALID_PARAMETER, "Got unexpected status %#lx.\n", status);

    IoReuseIrp(wsk_irp, STATUS_UNSUCCESSFUL);
    IoSetCompletionRoutine(wsk_irp, irp_completion_routine, &irp_complete_event, TRUE, TRUE, TRUE);
    status = provider_npi.Dispatch->WskSocket(NULL, AF_INET, SOCK_STREAM, IPPROTO_TCP,
            WSK_FLAG_LISTEN_SOCKET, &context, &client_listen_dispatch, NULL, NULL, NULL, wsk_irp);
    ok(status == STATUS_INVALID_HANDLE, "Got unexpected status %#lx.\n", status);

    IoReuseIrp(wsk_irp, STATUS_UNSUCCESSFUL);
    IoSetCompletionRoutine(wsk_irp, irp_completion_routine, &irp_complete_event, TRUE, TRUE, TRUE);
    wsk_irp->IoStatus.Status = 0xdeadbeef;
    wsk_irp->IoStatus.Information = 0xdeadbeef;
    status = provider_npi.Dispatch->WskSocket(provider_npi.Client, AF_INET, SOCK_STREAM, IPPROTO_TCP,
            WSK_FLAG_LISTEN_SOCKET, &context, &client_listen_dispatch, NULL, NULL, NULL, wsk_irp);
    ok(status == STATUS_PENDING, "Got unexpected status %#lx.\n", status);
    status = KeWaitForSingleObject(&irp_complete_event, Executive, KernelMode, FALSE, NULL);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
    ok(wsk_irp->IoStatus.Status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", wsk_irp->IoStatus.Status);
    ok(wsk_irp->IoStatus.Information, "Got zero Information.\n");

    tcp_socket = (WSK_SOCKET *)wsk_irp->IoStatus.Information;
    tcp_dispatch = tcp_socket->Dispatch;

    IoReuseIrp(wsk_irp, STATUS_UNSUCCESSFUL);
    IoSetCompletionRoutine(wsk_irp, irp_completion_routine, &irp_complete_event, TRUE, TRUE, TRUE);
    status = provider_npi.Dispatch->WskSocket(provider_npi.Client, AF_INET, SOCK_DGRAM, IPPROTO_UDP,
            WSK_FLAG_LISTEN_SOCKET, &context, &client_listen_dispatch, NULL, NULL, NULL, wsk_irp);
    ok(status == STATUS_PENDING, "Got unexpected status %#lx.\n", status);
    status = KeWaitForSingleObject(&irp_complete_event, Executive, KernelMode, FALSE, NULL);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
    ok(wsk_irp->IoStatus.Status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", wsk_irp->IoStatus.Status);
    ok(wsk_irp->IoStatus.Information, "Got zero Information.\n");

    udp_socket = (WSK_SOCKET *)wsk_irp->IoStatus.Information;
    udp_dispatch = udp_socket->Dispatch;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_LISTEN_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    IoReuseIrp(wsk_irp, STATUS_UNSUCCESSFUL);
    IoSetCompletionRoutine(wsk_irp, irp_completion_routine, &irp_complete_event, TRUE, TRUE, TRUE);
    wsk_irp->IoStatus.Status = 0xdeadbeef;
    wsk_irp->IoStatus.Information = 0xdeadbeef;
    status = udp_dispatch->WskBind(udp_socket, (SOCKADDR *)&addr, 0, wsk_irp);
    ok(status == STATUS_PENDING, "Got unexpected status %#lx.\n", status);
    status = KeWaitForSingleObject(&irp_complete_event, Executive, KernelMode, FALSE, NULL);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
    ok(wsk_irp->IoStatus.Status == STATUS_NOT_IMPLEMENTED, "Got unexpected status %#lx.\n", wsk_irp->IoStatus.Status);
    ok(!wsk_irp->IoStatus.Information, "Got unexpected Information %#Ix.\n",
            wsk_irp->IoStatus.Information);

    IoReuseIrp(wsk_irp, STATUS_UNSUCCESSFUL);
    IoSetCompletionRoutine(wsk_irp, irp_completion_routine, &irp_complete_event, TRUE, TRUE, TRUE);
    status = udp_dispatch->Basic.WskCloseSocket(udp_socket, wsk_irp);
    ok(status == STATUS_PENDING, "Got unexpected status %#lx.\n", status);
    status = KeWaitForSingleObject(&irp_complete_event, Executive, KernelMode, FALSE, NULL);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
    ok(wsk_irp->IoStatus.Status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", wsk_irp->IoStatus.Status);

    do
    {
        IoReuseIrp(wsk_irp, STATUS_UNSUCCESSFUL);
        IoSetCompletionRoutine(wsk_irp, irp_completion_routine, &irp_complete_event, TRUE, TRUE, TRUE);
        wsk_irp->IoStatus.Status = 0xdeadbeef;
        wsk_irp->IoStatus.Information = 0xdeadbeef;
        status = tcp_dispatch->WskBind(tcp_socket, (SOCKADDR *)&addr, 0, wsk_irp);
        ok(status == STATUS_PENDING, "Got unexpected status %#lx.\n", status);
        status = KeWaitForSingleObject(&irp_complete_event, Executive, KernelMode, FALSE, NULL);
        ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
        ok(wsk_irp->IoStatus.Status == STATUS_SUCCESS
                || wsk_irp->IoStatus.Status == STATUS_ADDRESS_ALREADY_ASSOCIATED,
                "Got unexpected status %#lx.\n", wsk_irp->IoStatus.Status);
        ok(!wsk_irp->IoStatus.Information, "Got unexpected Information %#Ix.\n",
                wsk_irp->IoStatus.Information);
    }
    while (wsk_irp->IoStatus.Status == STATUS_ADDRESS_ALREADY_ASSOCIATED);

    timeout.QuadPart = -2000 * 10000;

    IoReuseIrp(wsk_irp, STATUS_UNSUCCESSFUL);
    IoSetCompletionRoutine(wsk_irp, irp_completion_routine, &irp_complete_event, TRUE, TRUE, TRUE);
    memset(&local_addr, 0, sizeof(local_addr));
    memset(&remote_addr, 0, sizeof(remote_addr));
    status = tcp_dispatch->WskAccept(tcp_socket, 0, NULL, NULL,
            (SOCKADDR *)&local_addr, (SOCKADDR *)&remote_addr, wsk_irp);
    ok(status == STATUS_PENDING, "Got unexpected status %#lx.\n", status);

    if (0)
    {
        /* Queuing another WskAccept in parallel with different irp results in Windows hang. */
        IoReuseIrp(irp, STATUS_UNSUCCESSFUL);
        IoSetCompletionRoutine(irp, irp_completion_routine, &event, TRUE, TRUE, TRUE);
        status = tcp_dispatch->WskAccept(tcp_socket, 0, NULL, NULL, NULL, NULL, irp);
        ok(status == STATUS_PENDING, "Got unexpected status %#lx.\n", status);
    }

    status = KeWaitForSingleObject(&irp_complete_event, Executive, KernelMode, FALSE, &timeout);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
    ok(wsk_irp->IoStatus.Status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", wsk_irp->IoStatus.Status);
    ok(wsk_irp->IoStatus.Information, "Got zero Information.\n");

    if (status == STATUS_SUCCESS && wsk_irp->IoStatus.Status == STATUS_SUCCESS)
    {
        ok(local_addr.sin_family == AF_INET, "Got unexpected sin_family %u.\n", local_addr.sin_family);
        ok(local_addr.sin_port == htons(SERVER_LISTEN_PORT), "Got unexpected sin_port %u.\n",
                ntohs(local_addr.sin_port));
        ok(local_addr.sin_addr.s_addr == htonl(INADDR_LOOPBACK), "Got unexpected sin_addr %#lx.\n",
                ntohl(local_addr.sin_addr.s_addr));

        ok(remote_addr.sin_family == AF_INET, "Got unexpected sin_family %u.\n", remote_addr.sin_family);
        ok(remote_addr.sin_port, "Got zero sin_port.\n");
        ok(remote_addr.sin_addr.s_addr == htonl(INADDR_LOOPBACK), "Got unexpected sin_addr %#lx.\n",
                ntohl(remote_addr.sin_addr.s_addr));

        accept_socket = (WSK_SOCKET *)wsk_irp->IoStatus.Information;
        accept_dispatch = accept_socket->Dispatch;

        IoReuseIrp(irp, STATUS_UNSUCCESSFUL);
        IoSetCompletionRoutine(irp, irp_completion_routine, &event, TRUE, TRUE, TRUE);
        status = accept_dispatch->WskReceive(accept_socket, &wsk_buf2, 0, irp);
        ok(status == STATUS_PENDING, "Got unexpected status %#lx.\n", status);

        IoReuseIrp(wsk_irp, STATUS_UNSUCCESSFUL);
        IoSetCompletionRoutine(wsk_irp, irp_completion_routine, &irp_complete_event, TRUE, TRUE, TRUE);
        strcpy(buffer1, test_send_string);
        /* Setting Length in WSK_BUF greater than MDL allocation size BSODs Windows.
         * wsk_buf1.Length = TEST_BUFFER_LENGTH * 2; */
        status = accept_dispatch->WskSend(accept_socket, &wsk_buf1, 0, wsk_irp);
        ok(status == STATUS_PENDING, "Got unexpected status %#lx.\n", status);

        status = KeWaitForSingleObject(&irp_complete_event, Executive, KernelMode, FALSE, &timeout);
        ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
        ok(wsk_irp->IoStatus.Status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", wsk_irp->IoStatus.Status);
        ok(wsk_irp->IoStatus.Information == TEST_BUFFER_LENGTH, "Got unexpected status %#lx.\n",
                wsk_irp->IoStatus.Status);

        status = KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, &timeout);
        ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
        ok(irp->IoStatus.Status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", irp->IoStatus.Status);
        ok(irp->IoStatus.Information == sizeof(test_receive_string), "Got unexpected Information %#Ix.\n",
                irp->IoStatus.Information);
        ok(!strcmp(buffer2, test_receive_string), "Received unexpected data.\n");

        IoReuseIrp(wsk_irp, STATUS_UNSUCCESSFUL);
        IoSetCompletionRoutine(wsk_irp, irp_completion_routine, &irp_complete_event, TRUE, TRUE, TRUE);
        status = accept_dispatch->Basic.WskCloseSocket(accept_socket, wsk_irp);
        ok(status == STATUS_PENDING, "Got unexpected status %#lx.\n", status);
        status = KeWaitForSingleObject(&irp_complete_event, Executive, KernelMode, FALSE, NULL);
        ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
        ok(wsk_irp->IoStatus.Status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", wsk_irp->IoStatus.Status);
        ok(!wsk_irp->IoStatus.Information, "Got unexpected Information %#Ix.\n",
                wsk_irp->IoStatus.Information);
    }

    /* WskAccept to be aborted by WskCloseSocket(). */
    IoReuseIrp(irp, STATUS_UNSUCCESSFUL);
    IoSetCompletionRoutine(irp, irp_completion_routine, &event, TRUE, TRUE, TRUE);
    status = tcp_dispatch->WskAccept(tcp_socket, 0, NULL, NULL, NULL, NULL, irp);
    ok(status == STATUS_PENDING, "Got unexpected status %#lx.\n", status);

    IoReuseIrp(wsk_irp, STATUS_UNSUCCESSFUL);
    IoSetCompletionRoutine(wsk_irp, irp_completion_routine, &irp_complete_event, TRUE, TRUE, TRUE);
    wsk_irp->IoStatus.Status = 0xdeadbeef;
    wsk_irp->IoStatus.Information = 0xdeadbeef;
    status = tcp_dispatch->Basic.WskCloseSocket(tcp_socket, wsk_irp);
    ok(status == STATUS_PENDING, "Got unexpected status %#lx.\n", status);
    status = KeWaitForSingleObject(&irp_complete_event, Executive, KernelMode, FALSE, NULL);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
    ok(wsk_irp->IoStatus.Status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", wsk_irp->IoStatus.Status);
    ok(!wsk_irp->IoStatus.Information, "Got unexpected Information %#Ix.\n",
            wsk_irp->IoStatus.Information);

    status = KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, &timeout);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
    ok(irp->IoStatus.Status == STATUS_CANCELLED, "Got unexpected status %#lx.\n", irp->IoStatus.Status);
    ok(!irp->IoStatus.Information, "Got unexpected Information %#Ix.\n", irp->IoStatus.Information);
    IoFreeIrp(irp);

    IoFreeMdl(mdl1);
    IoFreeMdl(mdl2);
    ExFreePool(buffer1);
    ExFreePool(buffer2);
}

static void test_wsk_connect_socket(void)
{
    const WSK_PROVIDER_CONNECTION_DISPATCH *connect_dispatch;
    static const WSK_PROVIDER_CONNECTION_DISPATCH client_dispatch;
    struct socket_context context;
    struct sockaddr_in addr;
    LARGE_INTEGER timeout;
    WSK_SOCKET *socket;
    NTSTATUS status;

    timeout.QuadPart = -1000 * 10000;

    IoReuseIrp(wsk_irp, STATUS_UNSUCCESSFUL);
    IoSetCompletionRoutine(wsk_irp, irp_completion_routine, &irp_complete_event, TRUE, TRUE, TRUE);
    wsk_irp->IoStatus.Status = 0xdeadbeef;
    wsk_irp->IoStatus.Information = 0xdeadbeef;
    status = provider_npi.Dispatch->WskSocket(provider_npi.Client, AF_INET, SOCK_STREAM, IPPROTO_TCP,
            WSK_FLAG_CONNECTION_SOCKET, &context, &client_dispatch, NULL, NULL, NULL, wsk_irp);
    ok(status == STATUS_PENDING, "Got unexpected status %#lx.\n", status);
    status = KeWaitForSingleObject(&irp_complete_event, Executive, KernelMode, FALSE, NULL);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
    ok(wsk_irp->IoStatus.Status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", wsk_irp->IoStatus.Status);
    ok(wsk_irp->IoStatus.Information, "Got zero Information.\n");

    socket = (WSK_SOCKET *)wsk_irp->IoStatus.Information;
    connect_dispatch = socket->Dispatch;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(CLIENT_LISTEN_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    IoReuseIrp(wsk_irp, STATUS_UNSUCCESSFUL);
    IoSetCompletionRoutine(wsk_irp, irp_completion_routine, &irp_complete_event, TRUE, TRUE, TRUE);
    status = connect_dispatch->WskConnect(socket, (SOCKADDR *)&addr, 0, wsk_irp);
    ok(status == STATUS_INVALID_DEVICE_STATE, "Got unexpected status %#lx.\n", status);
    status = KeWaitForSingleObject(&irp_complete_event, Executive, KernelMode, FALSE, &timeout);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
    ok(wsk_irp->IoStatus.Status == STATUS_INVALID_DEVICE_STATE, "Got unexpected status %#lx.\n", wsk_irp->IoStatus.Status);
    ok(!wsk_irp->IoStatus.Information, "Got unexpected Information %#Ix.\n",
            wsk_irp->IoStatus.Information);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;

    IoReuseIrp(wsk_irp, STATUS_UNSUCCESSFUL);
    IoSetCompletionRoutine(wsk_irp, irp_completion_routine, &irp_complete_event, TRUE, TRUE, TRUE);
    wsk_irp->IoStatus.Status = 0xdeadbeef;
    wsk_irp->IoStatus.Information = 0xdeadbeef;
    status = connect_dispatch->WskBind(socket, (SOCKADDR *)&addr, 0, wsk_irp);
    ok(status == STATUS_PENDING, "Got unexpected status %#lx.\n", status);
    status = KeWaitForSingleObject(&irp_complete_event, Executive, KernelMode, FALSE, NULL);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
    ok(wsk_irp->IoStatus.Status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", wsk_irp->IoStatus.Status);
    ok(!wsk_irp->IoStatus.Information, "Got unexpected Information %#Ix.\n",
            wsk_irp->IoStatus.Information);

    addr.sin_port = htons(CLIENT_LISTEN_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    IoReuseIrp(wsk_irp, STATUS_UNSUCCESSFUL);
    IoSetCompletionRoutine(wsk_irp, irp_completion_routine, &irp_complete_event, TRUE, TRUE, TRUE);
    status = connect_dispatch->WskConnect(socket, (SOCKADDR *)&addr, 0, wsk_irp);
    ok(status == STATUS_PENDING, "Got unexpected status %#lx.\n", status);
    status = KeWaitForSingleObject(&irp_complete_event, Executive, KernelMode, FALSE, &timeout);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
    ok(wsk_irp->IoStatus.Status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", wsk_irp->IoStatus.Status);
    ok(!wsk_irp->IoStatus.Information, "Got unexpected Information %#Ix.\n",
            wsk_irp->IoStatus.Information);

    IoReuseIrp(wsk_irp, STATUS_UNSUCCESSFUL);
    IoSetCompletionRoutine(wsk_irp, irp_completion_routine, &irp_complete_event, TRUE, TRUE, TRUE);
    wsk_irp->IoStatus.Status = 0xdeadbeef;
    wsk_irp->IoStatus.Information = 0xdeadbeef;
    status = connect_dispatch->Basic.WskCloseSocket(socket, wsk_irp);
    ok(status == STATUS_PENDING, "Got unexpected status %#lx.\n", status);
    status = KeWaitForSingleObject(&irp_complete_event, Executive, KernelMode, FALSE, NULL);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
    ok(wsk_irp->IoStatus.Status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", wsk_irp->IoStatus.Status);
    ok(!wsk_irp->IoStatus.Information, "Got unexpected Information %#Ix.\n",
            wsk_irp->IoStatus.Information);
}

static NTSTATUS main_test(DEVICE_OBJECT *device, IRP *irp, IO_STACK_LOCATION *stack)
{
    void *buffer = irp->AssociatedIrp.SystemBuffer;

    if (!buffer)
        return STATUS_ACCESS_VIOLATION;

    netio_init();
    test_wsk_get_address_info();
    test_wsk_listen_socket();
    test_wsk_connect_socket();

    irp->IoStatus.Information = 0;
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
    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static NTSTATUS WINAPI driver_close(DEVICE_OBJECT *device, IRP *irp)
{
    netio_uninit();

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

    IoDeleteDevice(device_obj);

    winetest_cleanup();
}

NTSTATUS WINAPI DriverEntry(DRIVER_OBJECT *driver, PUNICODE_STRING registry)
{
    UNICODE_STRING nameW, linkW;
    NTSTATUS status;

    if ((status = winetest_init()))
        return status;

    DbgPrint("Loading driver.\n");

    driver_obj = driver;

    driver->DriverUnload = driver_unload;
    driver->MajorFunction[IRP_MJ_CREATE]            = driver_create;
    driver->MajorFunction[IRP_MJ_DEVICE_CONTROL]    = driver_iocontrol;
    driver->MajorFunction[IRP_MJ_CLOSE]             = driver_close;

    RtlInitUnicodeString(&nameW, device_name);
    RtlInitUnicodeString(&linkW, driver_link);

    status = IoCreateDevice(driver, 0, &nameW, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &device_obj);
    ok(!status, "failed to create device, status %#lx\n", status);
    status = IoCreateSymbolicLink(&linkW, &nameW);
    ok(!status, "failed to create link, status %#lx\n", status);
    device_obj->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}
