/*
 * Unit tests for AFD device ioctls
 *
 * Copyright 2021 Zebediah Figura for CodeWeavers
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

#include <limits.h>
#include <stdarg.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "winioctl.h"
#include "winsock2.h"
#include "ws2tcpip.h"
#include "mswsock.h"
#include "wine/afd.h"
#include "wine/test.h"

static void set_blocking(SOCKET s, ULONG blocking)
{
    int ret;
    blocking = !blocking;
    ret = ioctlsocket(s, FIONBIO, &blocking);
    ok(!ret, "got error %u\n", WSAGetLastError());
}

static void test_recv(void)
{
    const struct sockaddr_in bind_addr = {.sin_family = AF_INET, .sin_addr.s_addr = htonl(INADDR_LOOPBACK)};
    struct afd_recv_params params = {0};
    SOCKET client, server, listener;
    struct sockaddr addr;
    IO_STATUS_BLOCK io;
    WSABUF wsabufs[2];
    char buffer[8];
    HANDLE event;
    int ret, len;

    event = CreateEventW(NULL, TRUE, FALSE, NULL);

    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ret = bind(listener, (const struct sockaddr *)&bind_addr, sizeof(bind_addr));
    ok(!ret, "got error %u\n", WSAGetLastError());
    ret = listen(listener, 1);
    ok(!ret, "got error %u\n", WSAGetLastError());
    len = sizeof(addr);
    ret = getsockname(listener, (struct sockaddr *)&addr, &len);
    ok(!ret, "got error %u\n", WSAGetLastError());

    memset(&io, 0, sizeof(io));
    ret = NtDeviceIoControlFile((HANDLE)listener, event, NULL, NULL, &io, IOCTL_AFD_RECV, NULL, 0, NULL, 0);
    todo_wine ok(ret == STATUS_INVALID_CONNECTION, "got %#x\n", ret);
    todo_wine ok(!io.Status, "got status %#x\n", io.Status);
    ok(!io.Information, "got information %#Ix\n", io.Information);

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ret = connect(client, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "got error %u\n", WSAGetLastError());
    server = accept(listener, NULL, NULL);
    ok(server != -1, "got error %u\n", WSAGetLastError());

    memset(&io, 0, sizeof(io));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io, IOCTL_AFD_RECV, NULL, 0, NULL, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#x\n", ret);
    todo_wine ok(!io.Status, "got status %#x\n", io.Status);
    ok(!io.Information, "got information %#Ix\n", io.Information);

    wsabufs[0].len = sizeof(buffer);
    wsabufs[0].buf = buffer;
    params.buffers = wsabufs;
    params.count = 1;
    params.msg_flags = AFD_MSG_NOT_OOB;

    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params) - 1, NULL, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#x\n", ret);

    memset(&io, 0, sizeof(io));
    memset(buffer, 0xcc, sizeof(buffer));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);
    ok(!io.Status, "got status %#x\n", io.Status);
    ok(!io.Information, "got information %#Ix\n", io.Information);

    /* These structures need not remain valid. */
    memset(&params, 0xcc, sizeof(params));
    memset(wsabufs, 0xcc, sizeof(wsabufs));

    ret = send(server, "data", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    ret = WaitForSingleObject(event, 200);
    ok(!ret, "wait timed out\n");
    ok(!io.Status, "got %#x\n", io.Status);
    ok(io.Information == 5, "got %#Ix\n", io.Information);
    ok(!strcmp(buffer, "data"), "got %s\n", debugstr_an(buffer, io.Information));

    /* Test with multiple buffers. */

    wsabufs[0].len = 2;
    wsabufs[0].buf = buffer;
    wsabufs[1].len = 4;
    wsabufs[1].buf = buffer + 3;
    memset(&params, 0, sizeof(params));
    params.buffers = wsabufs;
    params.count = 2;
    params.msg_flags = AFD_MSG_NOT_OOB;

    memset(&io, 0, sizeof(io));
    memset(buffer, 0xcc, sizeof(buffer));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);
    ok(!io.Status, "got status %#x\n", io.Status);
    ok(!io.Information, "got information %#Ix\n", io.Information);

    ret = send(server, "data", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    ret = WaitForSingleObject(event, 200);
    ok(!ret, "wait timed out\n");
    ok(!io.Status, "got %#x\n", io.Status);
    ok(io.Information == 5, "got %#Ix\n", io.Information);
    ok(!strcmp(buffer, "da\xccta"), "got %s\n", debugstr_an(buffer, io.Information));

    /* Test synchronous return. */

    ret = send(server, "data", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    memset(&io, 0xcc, sizeof(io));
    memset(buffer, 0xcc, sizeof(buffer));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got %#x\n", io.Status);
    ok(io.Information == 5, "got %#Ix\n", io.Information);
    ok(!strcmp(buffer, "da\xccta"), "got %s\n", debugstr_an(buffer, io.Information));

    /* Test nonblocking mode. */

    set_blocking(client, FALSE);

    memset(&io, 0, sizeof(io));
    memset(buffer, 0xcc, sizeof(buffer));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    ok(ret == STATUS_DEVICE_NOT_READY, "got %#x\n", ret);
    todo_wine ok(!io.Status, "got status %#x\n", io.Status);
    ok(!io.Information, "got information %#Ix\n", io.Information);

    ret = send(server, "data", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    memset(&io, 0xcc, sizeof(io));
    memset(buffer, 0xcc, sizeof(buffer));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got %#x\n", io.Status);
    ok(io.Information == 5, "got %#Ix\n", io.Information);
    ok(!strcmp(buffer, "da\xccta"), "got %s\n", debugstr_an(buffer, io.Information));

    params.recv_flags = AFD_RECV_FORCE_ASYNC;

    memset(&io, 0, sizeof(io));
    memset(buffer, 0xcc, sizeof(buffer));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);
    todo_wine ok(!io.Status, "got status %#x\n", io.Status);
    todo_wine ok(!io.Information, "got information %#Ix\n", io.Information);

    ret = send(server, "data", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    ret = WaitForSingleObject(event, 200);
    ok(!ret, "wait timed out\n");
    ok(!io.Status, "got %#x\n", io.Status);
    ok(io.Information == 5, "got %#Ix\n", io.Information);
    ok(!strcmp(buffer, "da\xccta"), "got %s\n", debugstr_an(buffer, io.Information));

    params.recv_flags = 0;

    set_blocking(client, TRUE);

    /* Test flags. */

    ret = send(server, "a", 1, MSG_OOB);
    ok(ret == 1, "got %d\n", ret);

    ret = send(server, "data", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    memset(&io, 0xcc, sizeof(io));
    memset(buffer, 0xcc, sizeof(buffer));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got %#x\n", io.Status);
    ok(io.Information == 5, "got %#Ix\n", io.Information);
    ok(!strcmp(buffer, "da\xccta"), "got %s\n", debugstr_an(buffer, io.Information));

    params.msg_flags = 0;

    io.Status = 0xdeadbeef;
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#x\n", ret);
    todo_wine ok(io.Status == 0xdeadbeef, "got %#x\n", io.Status);

    params.msg_flags = AFD_MSG_OOB | AFD_MSG_NOT_OOB;

    io.Status = 0xdeadbeef;
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#x\n", ret);
    todo_wine ok(io.Status == 0xdeadbeef, "got %#x\n", io.Status);

    params.msg_flags = AFD_MSG_OOB;

    memset(buffer, 0xcc, sizeof(buffer));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    todo_wine ok(!ret, "got %#x\n", ret);
    todo_wine ok(!io.Status, "got %#x\n", io.Status);
    todo_wine ok(io.Information == 1, "got %#Ix\n", io.Information);
    todo_wine ok(buffer[0] == 'a', "got %s\n", debugstr_an(buffer, io.Information));

    params.msg_flags = AFD_MSG_NOT_OOB | AFD_MSG_PEEK;

    ret = send(server, "data", 4, 0);
    ok(ret == 4, "got %d\n", ret);

    memset(buffer, 0xcc, sizeof(buffer));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got %#x\n", io.Status);
    ok(io.Information == 4, "got %#Ix\n", io.Information);
    ok(!memcmp(buffer, "da\xccta", 5), "got %s\n", debugstr_an(buffer, io.Information));

    memset(buffer, 0xcc, sizeof(buffer));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got %#x\n", io.Status);
    ok(io.Information == 4, "got %#Ix\n", io.Information);
    ok(!memcmp(buffer, "da\xccta", 5), "got %s\n", debugstr_an(buffer, io.Information));

    params.msg_flags = AFD_MSG_NOT_OOB | AFD_MSG_WAITALL;

    memset(buffer, 0xcc, sizeof(buffer));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    todo_wine ok(ret == STATUS_PENDING, "got %#x\n", ret);

    if (ret == STATUS_PENDING)
    {
        ret = send(server, "s", 2, 0);
        ok(ret == 2, "got %d\n", ret);

        ret = WaitForSingleObject(event, 200);
        ok(!ret, "wait timed out\n");
        ok(!io.Status, "got %#x\n", io.Status);
        ok(io.Information == 6, "got %#Ix\n", io.Information);
        ok(!strcmp(buffer, "da\xcctas"), "got %s\n", debugstr_an(buffer, io.Information));
    }

    params.msg_flags = AFD_MSG_NOT_OOB;

    /* Test shutdown. */

    memset(buffer, 0xcc, sizeof(buffer));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    closesocket(server);

    ret = WaitForSingleObject(event, 200);
    ok(!ret, "wait timed out\n");
    ok(!io.Status, "got %#x\n", io.Status);
    ok(!io.Information, "got %#Ix\n", io.Information);

    ret = shutdown(client, SD_RECEIVE);
    ok(!ret, "got error %u\n", WSAGetLastError());

    memset(&io, 0, sizeof(io));
    memset(buffer, 0xcc, sizeof(buffer));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    todo_wine ok(ret == STATUS_PIPE_DISCONNECTED, "got %#x\n", ret);
    ok(!io.Status, "got status %#x\n", io.Status);
    ok(!io.Information, "got information %#Ix\n", io.Information);

    closesocket(client);
    closesocket(listener);

    /* Test UDP datagrams. */

    client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    memset(buffer, 0xcc, sizeof(buffer));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    todo_wine ok(ret == STATUS_INVALID_PARAMETER, "got %#x\n", ret);
    if (ret == STATUS_PENDING)
        CancelIo((HANDLE)client);

    ret = bind(client, (const struct sockaddr *)&bind_addr, sizeof(bind_addr));
    ok(!ret, "got error %u\n", WSAGetLastError());
    len = sizeof(addr);
    ret = getsockname(listener, (struct sockaddr *)&addr, &len);
    ok(!ret, "got error %u\n", WSAGetLastError());

    memset(buffer, 0xcc, sizeof(buffer));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    ret = sendto(server, "data", 5, 0, (struct sockaddr *)&addr, sizeof(addr));
    ok(ret == 5, "got %d\n", ret);

    ret = WaitForSingleObject(event, 200);
    ok(!ret, "wait timed out\n");
    ok(!io.Status, "got %#x\n", io.Status);
    ok(io.Information == 5, "got %#Ix\n", io.Information);
    ok(!strcmp(buffer, "da\xccta"), "got %s\n", debugstr_an(buffer, io.Information));

    /* Test a short read of a UDP datagram. */

    memset(buffer, 0xcc, sizeof(buffer));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    ret = sendto(server, "moredata", 9, 0, (struct sockaddr *)&addr, sizeof(addr));
    ok(ret == 9, "got %d\n", ret);

    ret = WaitForSingleObject(event, 200);
    ok(!ret, "wait timed out\n");
    ok(io.Status == STATUS_BUFFER_OVERFLOW, "got %#x\n", io.Status);
    ok(io.Information == 6, "got %#Ix\n", io.Information);
    ok(!memcmp(buffer, "mo\xccreda\xcc", 7), "got %s\n", debugstr_an(buffer, io.Information));

    ret = sendto(server, "moredata", 9, 0, (struct sockaddr *)&addr, sizeof(addr));
    ok(ret == 9, "got %d\n", ret);

    memset(&io, 0, sizeof(io));
    memset(buffer, 0xcc, sizeof(buffer));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    ok(ret == STATUS_BUFFER_OVERFLOW, "got %#x\n", ret);
    ok(io.Status == STATUS_BUFFER_OVERFLOW, "got %#x\n", io.Status);
    ok(io.Information == 6, "got %#Ix\n", io.Information);
    ok(!memcmp(buffer, "mo\xccreda\xcc", 7), "got %s\n", debugstr_an(buffer, io.Information));

    /* Test closing a socket during an async. */

    memset(buffer, 0xcc, sizeof(buffer));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    closesocket(client);

    ret = WaitForSingleObject(event, 200);
    ok(!ret, "wait timed out\n");
    todo_wine ok(io.Status == STATUS_CANCELLED, "got %#x\n", io.Status);
    ok(!io.Information, "got %#Ix\n", io.Information);

    closesocket(server);
    CloseHandle(event);
}

START_TEST(afd)
{
    WSADATA data;

    WSAStartup(MAKEWORD(2, 2), &data);

    test_recv();

    WSACleanup();
}
