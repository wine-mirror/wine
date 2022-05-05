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

#define TIMEOUT_INFINITE _I64_MAX

static void tcp_socketpair(SOCKET *src, SOCKET *dst)
{
    SOCKET server = INVALID_SOCKET;
    struct sockaddr_in addr;
    int len, ret;

    *src = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    ok(*src != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());

    server = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    ok(server != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    ret = bind(server, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "failed to bind socket, error %u\n", WSAGetLastError());

    len = sizeof(addr);
    ret = getsockname(server, (struct sockaddr *)&addr, &len);
    ok(!ret, "failed to get address, error %u\n", WSAGetLastError());

    ret = listen(server, 1);
    ok(!ret, "failed to listen, error %u\n", WSAGetLastError());

    ret = connect(*src, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "failed to connect, error %u\n", WSAGetLastError());

    len = sizeof(addr);
    *dst = accept(server, (struct sockaddr *)&addr, &len);
    ok(*dst != INVALID_SOCKET, "failed to accept socket, error %u\n", WSAGetLastError());

    closesocket(server);
}

static void set_blocking(SOCKET s, ULONG blocking)
{
    int ret;
    blocking = !blocking;
    ret = ioctlsocket(s, FIONBIO, &blocking);
    ok(!ret, "got error %u\n", WSAGetLastError());
}

/* Set the linger timeout to zero and close the socket. This will trigger an
 * RST on the connection on Windows as well as on Unix systems. */
static void close_with_rst(SOCKET s)
{
    static const struct linger linger = {.l_onoff = 1};
    int ret;

    SetLastError(0xdeadbeef);
    ret = setsockopt(s, SOL_SOCKET, SO_LINGER, (const char *)&linger, sizeof(linger));
    ok(!ret, "got %d\n", ret);
    ok(!GetLastError(), "got error %lu\n", GetLastError());

    closesocket(s);
}

static void test_open_device(void)
{
    OBJECT_BASIC_INFORMATION info;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING string;
    IO_STATUS_BLOCK io;
    HANDLE handle;
    SOCKET s;
    int ret;

    RtlInitUnicodeString(&string, L"\\Device\\Afd");
    InitializeObjectAttributes(&attr, &string, 0, NULL, NULL);
    ret = NtOpenFile(&handle, SYNCHRONIZE, &attr, &io, 0, 0);
    ok(!ret, "got %#x\n", ret);
    CloseHandle(handle);

    RtlInitUnicodeString(&string, L"\\Device\\Afd\\");
    InitializeObjectAttributes(&attr, &string, 0, NULL, NULL);
    ret = NtOpenFile(&handle, SYNCHRONIZE, &attr, &io, 0, 0);
    ok(!ret, "got %#x\n", ret);
    CloseHandle(handle);

    RtlInitUnicodeString(&string, L"\\Device\\Afd\\foobar");
    InitializeObjectAttributes(&attr, &string, 0, NULL, NULL);
    ret = NtOpenFile(&handle, SYNCHRONIZE, &attr, &io, 0, 0);
    ok(!ret, "got %#x\n", ret);
    CloseHandle(handle);

    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ret = NtQueryObject((HANDLE)s, ObjectBasicInformation, &info, sizeof(info), NULL);
    ok(!ret, "got %#x\n", ret);
    todo_wine ok(info.Attributes == OBJ_INHERIT, "got attributes %#lx\n", info.Attributes);
    todo_wine ok(info.GrantedAccess == (FILE_GENERIC_READ | FILE_GENERIC_WRITE | WRITE_DAC), "got access %#lx\n", info.GrantedAccess);

    closesocket(s);
}

#define check_poll(a, b, c) check_poll_(__LINE__, a, b, ~0, c, FALSE)
#define check_poll_mask(a, b, c, d) check_poll_(__LINE__, a, b, c, d, FALSE)
#define check_poll_todo(a, b, c) check_poll_(__LINE__, a, b, ~0, c, TRUE)
static void check_poll_(int line, SOCKET s, HANDLE event, int mask, int expect, BOOL todo)
{
    struct afd_poll_params in_params = {0}, out_params = {0};
    IO_STATUS_BLOCK io;
    NTSTATUS ret;

    in_params.timeout = -1000 * 10000;
    in_params.count = 1;
    in_params.sockets[0].socket = s;
    in_params.sockets[0].flags = mask;
    in_params.sockets[0].status = 0xdeadbeef;

    ret = NtDeviceIoControlFile((HANDLE)s, event, NULL, NULL, &io,
            IOCTL_AFD_POLL, &in_params, sizeof(in_params), &out_params, sizeof(out_params));
    ok_(__FILE__, line)(!ret || ret == STATUS_PENDING, "got %#lx\n", ret);
    if (ret == STATUS_PENDING)
    {
        ret = WaitForSingleObject(event, 1000);
        ok_(__FILE__, line)(!ret, "wait timed out\n");
    }
    ok_(__FILE__, line)(!io.Status, "got %#lx\n", io.Status);
    ok_(__FILE__, line)(io.Information == sizeof(out_params), "got %#Ix\n", io.Information);
    ok_(__FILE__, line)(out_params.timeout == in_params.timeout, "got timeout %I64d\n", out_params.timeout);
    ok_(__FILE__, line)(out_params.count == 1, "got count %u\n", out_params.count);
    ok_(__FILE__, line)(out_params.sockets[0].socket == s, "got socket %#Ix\n", out_params.sockets[0].socket);
    todo_wine_if (todo) ok_(__FILE__, line)(out_params.sockets[0].flags == expect, "got flags %#x\n", out_params.sockets[0].flags);
    todo_wine_if (expect & AFD_POLL_RESET)
        ok_(__FILE__, line)(!out_params.sockets[0].status, "got status %#x\n", out_params.sockets[0].status);
}

static void test_poll(void)
{
    const struct sockaddr_in bind_addr = {.sin_family = AF_INET, .sin_addr.s_addr = htonl(INADDR_LOOPBACK)};
    char in_buffer[offsetof(struct afd_poll_params, sockets[3])];
    char out_buffer[offsetof(struct afd_poll_params, sockets[3])];
    struct afd_poll_params *in_params = (struct afd_poll_params *)in_buffer;
    struct afd_poll_params *out_params = (struct afd_poll_params *)out_buffer;
    int large_buffer_size = 1024 * 1024;
    GUID acceptex_guid = WSAID_ACCEPTEX;
    SOCKET client, server, listener;
    OVERLAPPED overlapped = {0};
    LPFN_ACCEPTEX pAcceptEx;
    struct sockaddr_in addr;
    DWORD size, flags = 0;
    char *large_buffer;
    IO_STATUS_BLOCK io;
    LARGE_INTEGER now;
    ULONG params_size;
    WSABUF wsabuf;
    HANDLE event;
    int ret, len;

    large_buffer = malloc(large_buffer_size);
    memset(in_buffer, 0, sizeof(in_buffer));
    memset(out_buffer, 0, sizeof(out_buffer));
    event = CreateEventW(NULL, TRUE, FALSE, NULL);
    overlapped.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ret = bind(listener, (const struct sockaddr *)&bind_addr, sizeof(bind_addr));
    ok(!ret, "got error %u\n", WSAGetLastError());
    ret = listen(listener, 1);
    ok(!ret, "got error %u\n", WSAGetLastError());
    len = sizeof(addr);
    ret = getsockname(listener, (struct sockaddr *)&addr, &len);
    ok(!ret, "got error %u\n", WSAGetLastError());

    ret = WSAIoctl(listener, SIO_GET_EXTENSION_FUNCTION_POINTER, &acceptex_guid, sizeof(acceptex_guid),
            &pAcceptEx, sizeof(pAcceptEx), &size, NULL, NULL);
    ok(!ret, "failed to get AcceptEx, error %u\n", WSAGetLastError());

    params_size = offsetof(struct afd_poll_params, sockets[1]);
    in_params->count = 1;

    /* out_size must be at least as large as in_size. */

    ret = NtDeviceIoControlFile((HANDLE)listener, event, NULL, NULL, &io,
            IOCTL_AFD_POLL, in_params, params_size, NULL, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#x\n", ret);

    ret = NtDeviceIoControlFile((HANDLE)listener, event, NULL, NULL, &io,
            IOCTL_AFD_POLL, NULL, 0, out_params, params_size);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#x\n", ret);

    ret = NtDeviceIoControlFile((HANDLE)listener, event, NULL, NULL, &io,
            IOCTL_AFD_POLL, in_params, params_size, out_params, params_size + 1);
    ok(ret == STATUS_INVALID_HANDLE, "got %#x\n", ret);

    ret = NtDeviceIoControlFile((HANDLE)listener, event, NULL, NULL, &io,
            IOCTL_AFD_POLL, in_params, params_size + 1, out_params, params_size);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#x\n", ret);

    ret = NtDeviceIoControlFile((HANDLE)listener, event, NULL, NULL, &io,
            IOCTL_AFD_POLL, in_params, params_size - 1, out_params, params_size - 1);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#x\n", ret);

    ret = NtDeviceIoControlFile((HANDLE)listener, event, NULL, NULL, &io,
            IOCTL_AFD_POLL, in_params, params_size + 1, out_params, params_size + 1);
    ok(ret == STATUS_INVALID_HANDLE, "got %#x\n", ret);

    in_params->count = 0;
    ret = NtDeviceIoControlFile((HANDLE)listener, event, NULL, NULL, &io,
            IOCTL_AFD_POLL, in_params, params_size, out_params, params_size);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#x\n", ret);

    /* Basic semantics of the ioctl. */

    in_params->timeout = 0;
    in_params->count = 1;
    in_params->exclusive = FALSE;
    in_params->sockets[0].socket = listener;
    in_params->sockets[0].flags = ~0;
    in_params->sockets[0].status = 0xdeadbeef;

    memset(out_params, 0, params_size);
    ret = NtDeviceIoControlFile((HANDLE)listener, event, NULL, NULL, &io,
            IOCTL_AFD_POLL, in_params, params_size, out_params, params_size);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got %#lx\n", io.Status);
    ok(io.Information == offsetof(struct afd_poll_params, sockets[0]), "got %#Ix\n", io.Information);
    ok(!out_params->timeout, "got timeout %#I64x\n", out_params->timeout);
    ok(!out_params->count, "got count %u\n", out_params->count);
    ok(!out_params->sockets[0].socket, "got socket %#Ix\n", out_params->sockets[0].socket);
    ok(!out_params->sockets[0].flags, "got flags %#x\n", out_params->sockets[0].flags);
    ok(!out_params->sockets[0].status, "got status %#x\n", out_params->sockets[0].status);

    NtQuerySystemTime(&now);
    in_params->timeout = now.QuadPart;

    ret = NtDeviceIoControlFile((HANDLE)listener, event, NULL, NULL, &io,
            IOCTL_AFD_POLL, in_params, params_size, out_params, params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);
    ret = WaitForSingleObject(event, 100);
    ok(!ret, "got %#x\n", ret);
    ok(io.Status == STATUS_TIMEOUT, "got %#lx\n", io.Status);
    ok(io.Information == offsetof(struct afd_poll_params, sockets[0]), "got %#Ix\n", io.Information);
    ok(out_params->timeout == now.QuadPart, "got timeout %#I64x\n", out_params->timeout);
    ok(!out_params->count, "got count %u\n", out_params->count);

    in_params->timeout = -1000 * 10000;

    ret = NtDeviceIoControlFile((HANDLE)listener, event, NULL, NULL, &io,
            IOCTL_AFD_POLL, in_params, params_size, out_params, params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    set_blocking(client, FALSE);
    ret = connect(client, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret || WSAGetLastError() == WSAEWOULDBLOCK, "got error %u\n", WSAGetLastError());

    ret = WaitForSingleObject(event, 100);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got %#lx\n", io.Status);
    ok(io.Information == offsetof(struct afd_poll_params, sockets[1]), "got %#Ix\n", io.Information);
    ok(out_params->timeout == -1000 * 10000, "got timeout %#I64x\n", out_params->timeout);
    ok(out_params->count == 1, "got count %u\n", out_params->count);
    ok(out_params->sockets[0].socket == listener, "got socket %#Ix\n", out_params->sockets[0].socket);
    ok(out_params->sockets[0].flags == AFD_POLL_ACCEPT, "got flags %#x\n", out_params->sockets[0].flags);
    ok(!out_params->sockets[0].status, "got status %#x\n", out_params->sockets[0].status);

    ret = NtDeviceIoControlFile((HANDLE)listener, event, NULL, NULL, &io,
            IOCTL_AFD_POLL, in_params, params_size, out_params, params_size);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got %#lx\n", io.Status);
    ok(io.Information == offsetof(struct afd_poll_params, sockets[1]), "got %#Ix\n", io.Information);
    ok(out_params->timeout == -1000 * 10000, "got timeout %#I64x\n", out_params->timeout);
    ok(out_params->count == 1, "got count %u\n", out_params->count);
    ok(out_params->sockets[0].socket == listener, "got socket %#Ix\n", out_params->sockets[0].socket);
    ok(out_params->sockets[0].flags == AFD_POLL_ACCEPT, "got flags %#x\n", out_params->sockets[0].flags);
    ok(!out_params->sockets[0].status, "got status %#x\n", out_params->sockets[0].status);

    in_params->timeout = now.QuadPart;
    in_params->sockets[0].flags = (~0) & ~AFD_POLL_ACCEPT;

    ret = NtDeviceIoControlFile((HANDLE)listener, event, NULL, NULL, &io,
            IOCTL_AFD_POLL, in_params, params_size, out_params, params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);
    ret = WaitForSingleObject(event, 100);
    ok(!ret, "got %#x\n", ret);
    ok(io.Status == STATUS_TIMEOUT, "got %#lx\n", io.Status);
    ok(io.Information == offsetof(struct afd_poll_params, sockets[0]), "got %#Ix\n", io.Information);
    ok(!out_params->count, "got count %u\n", out_params->count);

    server = accept(listener, NULL, NULL);
    ok(server != -1, "got error %u\n", WSAGetLastError());
    set_blocking(server, FALSE);

    /* Test flags exposed by connected sockets. */

    check_poll(client, event, AFD_POLL_WRITE | AFD_POLL_CONNECT);
    check_poll(server, event, AFD_POLL_WRITE | AFD_POLL_CONNECT);

    /* It is valid to poll on a socket other than the one passed to
     * NtDeviceIoControlFile(). */

    in_params->count = 1;
    in_params->sockets[0].socket = server;
    in_params->sockets[0].flags = ~0;

    ret = NtDeviceIoControlFile((HANDLE)listener, event, NULL, NULL, &io,
            IOCTL_AFD_POLL, in_params, params_size, out_params, params_size);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got %#lx\n", io.Status);
    ok(io.Information == offsetof(struct afd_poll_params, sockets[1]), "got %#Ix\n", io.Information);
    ok(out_params->count == 1, "got count %u\n", out_params->count);
    ok(out_params->sockets[0].socket == server, "got socket %#Ix\n", out_params->sockets[0].socket);
    ok(out_params->sockets[0].flags == (AFD_POLL_WRITE | AFD_POLL_CONNECT),
            "got flags %#x\n", out_params->sockets[0].flags);
    ok(!out_params->sockets[0].status, "got status %#x\n", out_params->sockets[0].status);

    /* Test sending data. */

    ret = send(server, "data", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    check_poll_mask(client, event, AFD_POLL_READ, AFD_POLL_READ);
    check_poll(client, event, AFD_POLL_WRITE | AFD_POLL_CONNECT | AFD_POLL_READ);
    check_poll(server, event, AFD_POLL_WRITE | AFD_POLL_CONNECT);

    while (send(server, large_buffer, large_buffer_size, 0) == large_buffer_size);

    check_poll(client, event, AFD_POLL_WRITE | AFD_POLL_CONNECT | AFD_POLL_READ);
    check_poll(server, event, AFD_POLL_CONNECT);

    /* Test sending data while there is a pending WSARecv(). */

    in_params->timeout = -1000 * 10000;
    in_params->count = 1;
    in_params->sockets[0].socket = server;
    in_params->sockets[0].flags = AFD_POLL_READ;

    ret = NtDeviceIoControlFile((HANDLE)server, event, NULL, NULL, &io,
            IOCTL_AFD_POLL, in_params, params_size, out_params, params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    wsabuf.buf = large_buffer;
    wsabuf.len = 1;
    ret = WSARecv(server, &wsabuf, 1, NULL, &flags, &overlapped, NULL);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == ERROR_IO_PENDING, "got error %u\n", WSAGetLastError());

    ret = send(client, "a", 1, 0);
    ok(ret == 1, "got %d\n", ret);

    ret = WaitForSingleObject(overlapped.hEvent, 200);
    ok(!ret, "got %d\n", ret);
    ret = GetOverlappedResult((HANDLE)server, &overlapped, &size, FALSE);
    ok(ret, "got error %lu\n", GetLastError());
    ok(size == 1, "got size %lu\n", size);

    ret = WaitForSingleObject(event, 0);
    ok(ret == WAIT_TIMEOUT, "got %#x\n", ret);

    ret = send(client, "a", 1, 0);
    ok(ret == 1, "got %d\n", ret);

    ret = WaitForSingleObject(event, 200);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got %#lx\n", io.Status);
    ok(io.Information == offsetof(struct afd_poll_params, sockets[1]), "got %#Ix\n", io.Information);
    ok(out_params->count == 1, "got count %u\n", out_params->count);
    ok(out_params->sockets[0].socket == server, "got socket %#Ix\n", out_params->sockets[0].socket);
    ok(out_params->sockets[0].flags == AFD_POLL_READ, "got flags %#x\n", out_params->sockets[0].flags);
    ok(!out_params->sockets[0].status, "got status %#x\n", out_params->sockets[0].status);

    ret = recv(server, large_buffer, 1, 0);
    ok(ret == 1, "got %d\n", ret);

    /* Test sending out-of-band data. */

    ret = send(client, "a", 1, MSG_OOB);
    ok(ret == 1, "got %d\n", ret);

    check_poll(client, event, AFD_POLL_WRITE | AFD_POLL_CONNECT | AFD_POLL_READ);
    check_poll_mask(server, event, AFD_POLL_OOB, AFD_POLL_OOB);
    check_poll(server, event, AFD_POLL_CONNECT | AFD_POLL_OOB);

    ret = recv(server, large_buffer, 1, MSG_OOB);
    ok(ret == 1, "got %d\n", ret);

    check_poll(client, event, AFD_POLL_WRITE | AFD_POLL_CONNECT | AFD_POLL_READ);
    check_poll(server, event, AFD_POLL_CONNECT);

    ret = 1;
    ret = setsockopt(server, SOL_SOCKET, SO_OOBINLINE, (char *)&ret, sizeof(ret));
    ok(!ret, "got error %u\n", WSAGetLastError());

    ret = send(client, "a", 1, MSG_OOB);
    ok(ret == 1, "got %d\n", ret);

    check_poll(client, event, AFD_POLL_WRITE | AFD_POLL_CONNECT | AFD_POLL_READ);
    check_poll_mask(server, event, AFD_POLL_READ, AFD_POLL_READ);
    check_poll(server, event, AFD_POLL_CONNECT | AFD_POLL_READ);

    closesocket(client);
    closesocket(server);

    /* Test shutdown. */

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ret = connect(client, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "got error %u\n", WSAGetLastError());
    server = accept(listener, NULL, NULL);
    ok(server != -1, "got error %u\n", WSAGetLastError());

    ret = shutdown(client, SD_RECEIVE);
    ok(!ret, "got error %u\n", WSAGetLastError());

    check_poll(client, event, AFD_POLL_WRITE | AFD_POLL_CONNECT);
    check_poll(server, event, AFD_POLL_WRITE | AFD_POLL_CONNECT);

    ret = shutdown(client, SD_SEND);
    ok(!ret, "got error %u\n", WSAGetLastError());

    check_poll(client, event, AFD_POLL_WRITE | AFD_POLL_CONNECT);
    check_poll_mask(server, event, AFD_POLL_HUP, AFD_POLL_HUP);
    check_poll(server, event, AFD_POLL_WRITE | AFD_POLL_CONNECT | AFD_POLL_HUP);

    closesocket(client);
    closesocket(server);

    /* Test shutdown with data in the pipe. */

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ret = connect(client, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "got error %u\n", WSAGetLastError());
    server = accept(listener, NULL, NULL);
    ok(server != -1, "got error %u\n", WSAGetLastError());

    ret = send(client, "data", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    check_poll(client, event, AFD_POLL_WRITE | AFD_POLL_CONNECT);
    check_poll_mask(server, event, AFD_POLL_READ, AFD_POLL_READ);
    check_poll(server, event, AFD_POLL_WRITE | AFD_POLL_CONNECT | AFD_POLL_READ);

    ret = shutdown(client, SD_SEND);
    ok(!ret, "got error %u\n", WSAGetLastError());

    check_poll(client, event, AFD_POLL_WRITE | AFD_POLL_CONNECT);
    check_poll_mask(server, event, AFD_POLL_READ, AFD_POLL_READ);
    check_poll_todo(server, event, AFD_POLL_WRITE | AFD_POLL_CONNECT | AFD_POLL_READ | AFD_POLL_HUP);

    /* Test closing a socket while polling on it. Note that AFD_POLL_CLOSE
     * is always returned, regardless of whether it's polled for. */

    in_params->timeout = -1000 * 10000;
    in_params->count = 1;
    in_params->sockets[0].socket = client;
    in_params->sockets[0].flags = 0;

    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_POLL, in_params, params_size, out_params, params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    closesocket(client);

    ret = WaitForSingleObject(event, 100);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got %#lx\n", io.Status);
    ok(io.Information == offsetof(struct afd_poll_params, sockets[1]), "got %#Ix\n", io.Information);
    ok(out_params->count == 1, "got count %u\n", out_params->count);
    ok(out_params->sockets[0].socket == client, "got socket %#Ix\n", out_params->sockets[0].socket);
    ok(out_params->sockets[0].flags == AFD_POLL_CLOSE,
            "got flags %#x\n", out_params->sockets[0].flags);
    ok(!out_params->sockets[0].status, "got status %#x\n", out_params->sockets[0].status);

    closesocket(server);

    /* Test a failed connection.
     *
     * The following poll works even where the equivalent WSAPoll() call fails.
     * However, it can take over 2 seconds to complete on the testbot. */

    if (winetest_interactive)
    {
        client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        set_blocking(client, FALSE);

        in_params->timeout = -10000 * 10000;
        in_params->count = 1;
        in_params->sockets[0].socket = client;
        in_params->sockets[0].flags = ~0;

        ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
                IOCTL_AFD_POLL, in_params, params_size, out_params, params_size);
        ok(ret == STATUS_PENDING, "got %#x\n", ret);

        addr.sin_port = 255;
        ret = connect(client, (struct sockaddr *)&addr, sizeof(addr));
        ok(ret == -1, "got %d\n", ret);
        ok(WSAGetLastError() == WSAEWOULDBLOCK, "got error %u\n", WSAGetLastError());

        ret = WaitForSingleObject(event, 10000);
        ok(!ret, "got %#x\n", ret);
        ok(!io.Status, "got %#lx\n", io.Status);
        ok(io.Information == offsetof(struct afd_poll_params, sockets[1]), "got %#Ix\n", io.Information);
        ok(out_params->count == 1, "got count %u\n", out_params->count);
        ok(out_params->sockets[0].socket == client, "got socket %#Ix\n", out_params->sockets[0].socket);
        ok(out_params->sockets[0].flags == AFD_POLL_CONNECT_ERR, "got flags %#x\n", out_params->sockets[0].flags);
        ok(out_params->sockets[0].status == STATUS_CONNECTION_REFUSED, "got status %#x\n", out_params->sockets[0].status);

        in_params->timeout = now.QuadPart;
        memset(out_params, 0xcc, sizeof(out_buffer));
        ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
                IOCTL_AFD_POLL, in_params, params_size, out_params, params_size);
        ok(!ret, "got %#x\n", ret);
        ok(!io.Status, "got %#lx\n", io.Status);
        ok(io.Information == offsetof(struct afd_poll_params, sockets[1]), "got %#Ix\n", io.Information);
        ok(out_params->count == 1, "got count %u\n", out_params->count);
        ok(out_params->sockets[0].socket == client, "got socket %#Ix\n", out_params->sockets[0].socket);
        ok(out_params->sockets[0].flags == AFD_POLL_CONNECT_ERR, "got flags %#x\n", out_params->sockets[0].flags);
        ok(out_params->sockets[0].status == STATUS_CONNECTION_REFUSED, "got status %#x\n", out_params->sockets[0].status);

        memset(out_params, 0xcc, sizeof(out_buffer));
        ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
                IOCTL_AFD_POLL, in_params, params_size, out_params, params_size);
        ok(!ret, "got %#x\n", ret);
        ok(!io.Status, "got %#lx\n", io.Status);
        ok(io.Information == offsetof(struct afd_poll_params, sockets[1]), "got %#Ix\n", io.Information);
        ok(out_params->count == 1, "got count %u\n", out_params->count);
        ok(out_params->sockets[0].socket == client, "got socket %#Ix\n", out_params->sockets[0].socket);
        ok(out_params->sockets[0].flags == AFD_POLL_CONNECT_ERR, "got flags %#x\n", out_params->sockets[0].flags);
        ok(out_params->sockets[0].status == STATUS_CONNECTION_REFUSED, "got status %#x\n", out_params->sockets[0].status);

        ret = connect(client, (struct sockaddr *)&addr, sizeof(addr));
        ok(ret == -1, "got %d\n", ret);
        todo_wine ok(WSAGetLastError() == WSAEWOULDBLOCK, "got error %u\n", WSAGetLastError());
        if (WSAGetLastError() == WSAECONNABORTED)
        {
            ret = connect(client, (struct sockaddr *)&addr, sizeof(addr));
            ok(ret == -1, "got %d\n", ret);
            ok(WSAGetLastError() == WSAEWOULDBLOCK, "got error %u\n", WSAGetLastError());
        }

        /* A subsequent poll call returns no events, or times out. However, this
         * can't be reliably tested, as e.g. Linux will fail the connection
         * immediately. */

        closesocket(client);

        client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        ret = connect(client, (struct sockaddr *)&addr, sizeof(addr));
        ok(ret == -1, "expected failure\n");
        ok(WSAGetLastError() == WSAECONNREFUSED, "got error %u\n", WSAGetLastError());

        memset(out_params, 0xcc, sizeof(out_buffer));
        ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
                IOCTL_AFD_POLL, in_params, params_size, out_params, params_size);
        ok(!ret, "got %#x\n", ret);
        ok(!io.Status, "got %#lx\n", io.Status);
        ok(io.Information == offsetof(struct afd_poll_params, sockets[1]), "got %#Ix\n", io.Information);
        ok(out_params->count == 1, "got count %u\n", out_params->count);
        ok(out_params->sockets[0].socket == client, "got socket %#Ix\n", out_params->sockets[0].socket);
        ok(out_params->sockets[0].flags == AFD_POLL_CONNECT_ERR, "got flags %#x\n", out_params->sockets[0].flags);
        ok(out_params->sockets[0].status == STATUS_CONNECTION_REFUSED, "got status %#x\n", out_params->sockets[0].status);

        closesocket(client);
    }

    /* Test supplying multiple handles to the ioctl. */

    len = sizeof(addr);
    ret = getsockname(listener, (struct sockaddr *)&addr, &len);
    ok(!ret, "got error %u\n", WSAGetLastError());

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ret = connect(client, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "got error %u\n", WSAGetLastError());
    server = accept(listener, NULL, NULL);
    ok(server != -1, "got error %u\n", WSAGetLastError());

    in_params->timeout = -1000 * 10000;
    in_params->count = 2;
    in_params->sockets[0].socket = client;
    in_params->sockets[0].flags = AFD_POLL_READ;
    in_params->sockets[1].socket = server;
    in_params->sockets[1].flags = AFD_POLL_READ;

    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_POLL, in_params, params_size, out_params, params_size);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#x\n", ret);

    params_size = offsetof(struct afd_poll_params, sockets[2]);

    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_POLL, in_params, params_size, out_params, params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    ret = send(client, "data", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    ret = WaitForSingleObject(event, 100);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got %#lx\n", io.Status);
    ok(io.Information == offsetof(struct afd_poll_params, sockets[1]), "got %#Ix\n", io.Information);
    ok(out_params->count == 1, "got count %u\n", out_params->count);
    ok(out_params->sockets[0].socket == server, "got socket %#Ix\n", out_params->sockets[0].socket);
    ok(out_params->sockets[0].flags == AFD_POLL_READ, "got flags %#x\n", out_params->sockets[0].flags);
    ok(!out_params->sockets[0].status, "got status %#x\n", out_params->sockets[0].status);

    in_params->count = 2;
    in_params->sockets[0].socket = client;
    in_params->sockets[0].flags = AFD_POLL_READ | AFD_POLL_WRITE;
    in_params->sockets[1].socket = server;
    in_params->sockets[1].flags = AFD_POLL_READ | AFD_POLL_WRITE;

    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_POLL, in_params, params_size, out_params, params_size);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got %#lx\n", io.Status);
    ok(io.Information == offsetof(struct afd_poll_params, sockets[2]), "got %#Ix\n", io.Information);
    ok(out_params->count == 2, "got count %u\n", out_params->count);
    ok(out_params->sockets[0].socket == client, "got socket %#Ix\n", out_params->sockets[0].socket);
    ok(out_params->sockets[0].flags == AFD_POLL_WRITE, "got flags %#x\n", out_params->sockets[0].flags);
    ok(!out_params->sockets[0].status, "got status %#x\n", out_params->sockets[0].status);
    ok(out_params->sockets[1].socket == server, "got socket %#Ix\n", out_params->sockets[1].socket);
    ok(out_params->sockets[1].flags == (AFD_POLL_READ | AFD_POLL_WRITE),
            "got flags %#x\n", out_params->sockets[1].flags);
    ok(!out_params->sockets[1].status, "got status %#x\n", out_params->sockets[1].status);

    in_params->count = 2;
    in_params->sockets[0].socket = client;
    in_params->sockets[0].flags = AFD_POLL_READ | AFD_POLL_WRITE;
    in_params->sockets[1].socket = server;
    in_params->sockets[1].flags = AFD_POLL_READ | AFD_POLL_WRITE;

    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_POLL, in_params, params_size, out_params, params_size);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got %#lx\n", io.Status);
    ok(io.Information == offsetof(struct afd_poll_params, sockets[2]), "got %#Ix\n", io.Information);
    ok(out_params->count == 2, "got count %u\n", out_params->count);
    ok(out_params->sockets[0].socket == client, "got socket %#Ix\n", out_params->sockets[0].socket);
    ok(out_params->sockets[0].flags == AFD_POLL_WRITE, "got flags %#x\n", out_params->sockets[0].flags);
    ok(!out_params->sockets[0].status, "got status %#x\n", out_params->sockets[0].status);
    ok(out_params->sockets[1].socket == server, "got socket %#Ix\n", out_params->sockets[1].socket);
    ok(out_params->sockets[1].flags == (AFD_POLL_READ | AFD_POLL_WRITE),
            "got flags %#x\n", out_params->sockets[1].flags);
    ok(!out_params->sockets[1].status, "got status %#x\n", out_params->sockets[1].status);

    /* Close a socket while polling on another. */

    in_params->timeout = -100 * 10000;
    in_params->count = 1;
    in_params->sockets[0].socket = client;
    in_params->sockets[0].flags = AFD_POLL_READ;
    params_size = offsetof(struct afd_poll_params, sockets[1]);

    ret = NtDeviceIoControlFile((HANDLE)server, event, NULL, NULL, &io,
            IOCTL_AFD_POLL, in_params, params_size, out_params, params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    closesocket(server);

    ret = WaitForSingleObject(event, 1000);
    ok(!ret, "got %#x\n", ret);
    todo_wine ok(io.Status == STATUS_TIMEOUT, "got %#lx\n", io.Status);
    todo_wine ok(io.Information == offsetof(struct afd_poll_params, sockets[0]), "got %#Ix\n", io.Information);
    todo_wine ok(!out_params->count, "got count %u\n", out_params->count);

    closesocket(client);

    /* Test connecting while there is a pending AcceptEx(). */

    in_params->timeout = -1000 * 10000;
    in_params->count = 1;
    in_params->sockets[0].socket = listener;
    in_params->sockets[0].flags = AFD_POLL_ACCEPT;

    ret = NtDeviceIoControlFile((HANDLE)listener, event, NULL, NULL, &io,
            IOCTL_AFD_POLL, in_params, params_size, out_params, params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ret = pAcceptEx(listener, server, large_buffer, 0, 0, sizeof(struct sockaddr_in) + 16, NULL, &overlapped);
    ok(!ret, "got %d\n", ret);
    ok(WSAGetLastError() == ERROR_IO_PENDING, "got error %u\n", WSAGetLastError());

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ret = connect(client, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "got error %u\n", WSAGetLastError());

    ret = WaitForSingleObject(overlapped.hEvent, 200);
    ok(!ret, "got %d\n", ret);
    ret = GetOverlappedResult((HANDLE)listener, &overlapped, &size, FALSE);
    ok(ret, "got error %lu\n", GetLastError());
    ok(!size, "got size %lu\n", size);

    ret = WaitForSingleObject(event, 0);
    ok(ret == WAIT_TIMEOUT, "got %#x\n", ret);

    closesocket(server);
    closesocket(client);

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ret = connect(client, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "got error %u\n", WSAGetLastError());

    ret = WaitForSingleObject(event, 200);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got %#lx\n", io.Status);
    ok(io.Information == offsetof(struct afd_poll_params, sockets[1]), "got %#Ix\n", io.Information);
    ok(out_params->count == 1, "got count %u\n", out_params->count);
    ok(out_params->sockets[0].socket == listener, "got socket %#Ix\n", out_params->sockets[0].socket);
    ok(out_params->sockets[0].flags == AFD_POLL_ACCEPT, "got flags %#x\n", out_params->sockets[0].flags);
    ok(!out_params->sockets[0].status, "got status %#x\n", out_params->sockets[0].status);

    server = accept(listener, NULL, NULL);
    ok(server != -1, "got error %u\n", WSAGetLastError());
    closesocket(server);
    closesocket(client);

    /* Verify that CONNECT and WRITE are signaled simultaneously. */

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    in_params->timeout = -1000 * 10000;
    in_params->count = 1;
    in_params->sockets[0].socket = client;
    in_params->sockets[0].flags = ~0;
    params_size = offsetof(struct afd_poll_params, sockets[1]);

    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_POLL, in_params, params_size, out_params, params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    ret = connect(client, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "got error %u\n", WSAGetLastError());

    ret = WaitForSingleObject(event, 200);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got %#lx\n", io.Status);
    ok(io.Information == offsetof(struct afd_poll_params, sockets[1]), "got %#Ix\n", io.Information);
    ok(out_params->count == 1, "got count %u\n", out_params->count);
    ok(out_params->sockets[0].flags == (AFD_POLL_CONNECT | AFD_POLL_WRITE),
            "got flags %#x\n", out_params->sockets[0].flags);
    ok(!out_params->sockets[0].status, "got status %#x\n", out_params->sockets[0].status);

    server = accept(listener, NULL, NULL);
    ok(server != -1, "got error %u\n", WSAGetLastError());
    closesocket(server);
    closesocket(client);

    closesocket(listener);

    /* Test UDP sockets. */

    client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    check_poll(client, event, AFD_POLL_WRITE);
    check_poll(server, event, AFD_POLL_WRITE);

    ret = bind(client, (const struct sockaddr *)&bind_addr, sizeof(bind_addr));
    ok(!ret, "got error %u\n", WSAGetLastError());
    len = sizeof(addr);
    ret = getsockname(client, (struct sockaddr *)&addr, &len);
    ok(!ret, "got error %u\n", WSAGetLastError());

    check_poll(client, event, AFD_POLL_WRITE);
    check_poll(server, event, AFD_POLL_WRITE);

    in_params->timeout = -1000 * 10000;
    in_params->count = 1;
    in_params->sockets[0].socket = client;
    in_params->sockets[0].flags = (~0) & ~AFD_POLL_WRITE;
    params_size = offsetof(struct afd_poll_params, sockets[1]);

    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_POLL, in_params, params_size, out_params, params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    ret = sendto(server, "data", 5, 0, (struct sockaddr *)&addr, sizeof(addr));
    ok(ret == 5, "got %d\n", ret);

    ret = WaitForSingleObject(event, 100);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got %#lx\n", io.Status);
    ok(io.Information == offsetof(struct afd_poll_params, sockets[1]), "got %#Ix\n", io.Information);
    ok(out_params->count == 1, "got count %u\n", out_params->count);
    ok(out_params->sockets[0].socket == client, "got socket %#Ix\n", out_params->sockets[0].socket);
    ok(out_params->sockets[0].flags == AFD_POLL_READ, "got flags %#x\n", out_params->sockets[0].flags);
    ok(!out_params->sockets[0].status, "got status %#x\n", out_params->sockets[0].status);

    closesocket(client);
    closesocket(server);

    /* Passing any invalid sockets yields STATUS_INVALID_HANDLE.
     *
     * Note however that WSAPoll() happily accepts invalid sockets. It seems
     * user-side cached data is used: closing a handle with CloseHandle() before
     * passing it to WSAPoll() yields ENOTSOCK. */

    tcp_socketpair(&client, &server);

    in_params->count = 2;
    in_params->sockets[0].socket = 0xabacab;
    in_params->sockets[0].flags = AFD_POLL_READ | AFD_POLL_WRITE;
    in_params->sockets[1].socket = client;
    in_params->sockets[1].flags = AFD_POLL_READ | AFD_POLL_WRITE;
    params_size = offsetof(struct afd_poll_params, sockets[2]);

    memset(&io, 0, sizeof(io));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_POLL, in_params, params_size, out_params, params_size);
    ok(ret == STATUS_INVALID_HANDLE, "got %#x\n", ret);
    ok(!io.Status, "got %#lx\n", io.Status);
    ok(!io.Information, "got %#Ix\n", io.Information);

    /* Test passing the same handle twice. */

    in_params->count = 3;
    in_params->sockets[0].socket = client;
    in_params->sockets[0].flags = AFD_POLL_READ | AFD_POLL_WRITE;
    in_params->sockets[1].socket = client;
    in_params->sockets[1].flags = AFD_POLL_READ | AFD_POLL_WRITE;
    in_params->sockets[2].socket = client;
    in_params->sockets[2].flags = AFD_POLL_READ | AFD_POLL_WRITE | AFD_POLL_CONNECT;
    params_size = offsetof(struct afd_poll_params, sockets[3]);

    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_POLL, in_params, params_size, out_params, params_size);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got %#lx\n", io.Status);
    ok(io.Information == offsetof(struct afd_poll_params, sockets[3]), "got %#Ix\n", io.Information);
    ok(out_params->count == 3, "got count %u\n", out_params->count);
    ok(out_params->sockets[0].socket == client, "got socket %#Ix\n", out_params->sockets[0].socket);
    ok(out_params->sockets[0].flags == AFD_POLL_WRITE, "got flags %#x\n", out_params->sockets[0].flags);
    ok(!out_params->sockets[0].status, "got status %#x\n", out_params->sockets[0].status);
    ok(out_params->sockets[1].socket == client, "got socket %#Ix\n", out_params->sockets[1].socket);
    ok(out_params->sockets[1].flags == AFD_POLL_WRITE, "got flags %#x\n", out_params->sockets[1].flags);
    ok(!out_params->sockets[1].status, "got status %#x\n", out_params->sockets[1].status);
    ok(out_params->sockets[2].socket == client, "got socket %#Ix\n", out_params->sockets[2].socket);
    ok(out_params->sockets[2].flags == (AFD_POLL_WRITE | AFD_POLL_CONNECT),
            "got flags %#x\n", out_params->sockets[2].flags);
    ok(!out_params->sockets[2].status, "got status %#x\n", out_params->sockets[2].status);

    in_params->count = 2;
    in_params->sockets[0].socket = client;
    in_params->sockets[0].flags = AFD_POLL_READ;
    in_params->sockets[1].socket = client;
    in_params->sockets[1].flags = AFD_POLL_READ;
    params_size = offsetof(struct afd_poll_params, sockets[2]);

    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_POLL, in_params, params_size, out_params, params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    ret = send(server, "data", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    ret = WaitForSingleObject(event, 100);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got %#lx\n", io.Status);
    ok(io.Information == offsetof(struct afd_poll_params, sockets[1]), "got %#Ix\n", io.Information);
    ok(out_params->count == 1, "got count %u\n", out_params->count);
    ok(out_params->sockets[0].socket == client, "got socket %#Ix\n", out_params->sockets[0].socket);
    ok(out_params->sockets[0].flags == AFD_POLL_READ, "got flags %#x\n", out_params->sockets[0].flags);
    ok(!out_params->sockets[0].status, "got status %#x\n", out_params->sockets[0].status);

    closesocket(client);
    closesocket(server);

    CloseHandle(overlapped.hEvent);
    CloseHandle(event);
    free(large_buffer);
}

struct poll_exclusive_thread_cb_ctx
{
    SOCKET ctl_sock;
    HANDLE event;
    IO_STATUS_BLOCK *io;
    ULONG params_size;
    struct afd_poll_params *in_params;
    struct afd_poll_params *out_params;
};

static DWORD WINAPI poll_exclusive_thread_cb(void *param)
{
    struct poll_exclusive_thread_cb_ctx *ctx = param;
    int ret;

    ret = NtDeviceIoControlFile((HANDLE)ctx->ctl_sock, ctx->event, NULL, NULL, ctx->io,
            IOCTL_AFD_POLL, ctx->in_params, ctx->params_size, ctx->out_params, ctx->params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    ret = WaitForSingleObject(ctx->event, 100);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    return 0;
}

#define POLL_SOCK_CNT 2
#define POLL_CNT 4

static void test_poll_exclusive(void)
{
    char in_buffer[offsetof(struct afd_poll_params, sockets[POLL_SOCK_CNT + 1])];
    char out_buffers[POLL_CNT][offsetof(struct afd_poll_params, sockets[POLL_SOCK_CNT + 1])];
    struct afd_poll_params *in_params = (struct afd_poll_params *)in_buffer;
    struct afd_poll_params *out_params[POLL_CNT] =
    {
        (struct afd_poll_params *)out_buffers[0],
        (struct afd_poll_params *)out_buffers[1],
        (struct afd_poll_params *)out_buffers[2],
        (struct afd_poll_params *)out_buffers[3]
    };
    SOCKET ctl_sock;
    SOCKET socks[POLL_CNT];
    IO_STATUS_BLOCK io[POLL_CNT];
    HANDLE events[POLL_CNT];
    ULONG params_size;
    struct poll_exclusive_thread_cb_ctx cb_ctx;
    HANDLE thrd;
    size_t i;
    int ret;

    ctl_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    for (i = 0; i < POLL_CNT; i++)
    {
        socks[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        events[i] = CreateEventW(NULL, TRUE, FALSE, NULL);
    }

    params_size = offsetof(struct afd_poll_params, sockets[1]);

    in_params->timeout = TIMEOUT_INFINITE;
    in_params->count = 1;
    in_params->exclusive = FALSE;
    in_params->sockets[0].socket = socks[0];
    in_params->sockets[0].flags = ~0;
    in_params->sockets[0].status = 0;

    /***** Exclusive explicitly terminated *****/

    in_params->exclusive = TRUE;

    ret = NtDeviceIoControlFile((HANDLE)ctl_sock, events[0], NULL, NULL, &io[0],
            IOCTL_AFD_POLL, in_params, params_size, out_params[0], params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    ret = WaitForSingleObject(events[0], 100);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    CancelIo((HANDLE)ctl_sock);

    ret = WaitForSingleObject(events[0], 100);
    ok(ret == STATUS_SUCCESS, "got %#x\n", ret);

    /***** Same socket tests *****/

    /* Basic non-exclusive behavior as reference. */

    in_params->exclusive = FALSE;

    ret = NtDeviceIoControlFile((HANDLE)ctl_sock, events[0], NULL, NULL, &io[0],
            IOCTL_AFD_POLL, in_params, params_size, out_params[0], params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    ret = NtDeviceIoControlFile((HANDLE)ctl_sock, events[1], NULL, NULL, &io[1],
            IOCTL_AFD_POLL, in_params, params_size, out_params[1], params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    ret = WaitForSingleObject(events[0], 100);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    ret = WaitForSingleObject(events[1], 100);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    CancelIo((HANDLE)ctl_sock);

    /* If the main poll is exclusive it is terminated by the following exclusive. */

    in_params->exclusive = TRUE;

    ret = NtDeviceIoControlFile((HANDLE)ctl_sock, events[0], NULL, NULL, &io[0],
            IOCTL_AFD_POLL, in_params, params_size, out_params[0], params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    ret = NtDeviceIoControlFile((HANDLE)ctl_sock, events[1], NULL, NULL, &io[1],
            IOCTL_AFD_POLL, in_params, params_size, out_params[1], params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    ret = WaitForSingleObject(events[0], 100);
    ok(ret == STATUS_SUCCESS, "got %#x\n", ret);

    ret = WaitForSingleObject(events[1], 100);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    CancelIo((HANDLE)ctl_sock);

    /* If the main poll is non-exclusive neither itself nor the following exclusives are terminated. */

    in_params->exclusive = FALSE;

    ret = NtDeviceIoControlFile((HANDLE)ctl_sock, events[0], NULL, NULL, &io[0],
            IOCTL_AFD_POLL, in_params, params_size, out_params[0], params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    in_params->exclusive = TRUE;

    ret = NtDeviceIoControlFile((HANDLE)ctl_sock, events[1], NULL, NULL, &io[1],
            IOCTL_AFD_POLL, in_params, params_size, out_params[1], params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    ret = NtDeviceIoControlFile((HANDLE)ctl_sock, events[2], NULL, NULL, &io[2],
            IOCTL_AFD_POLL, in_params, params_size, out_params[2], params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    ret = WaitForSingleObject(events[0], 100);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    ret = WaitForSingleObject(events[1], 100);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    ret = WaitForSingleObject(events[2], 100);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    CancelIo((HANDLE)ctl_sock);

    /* A new poll is considered the main poll if no others are queued at the time. */

    in_params->exclusive = FALSE;

    ret = NtDeviceIoControlFile((HANDLE)socks[0], events[0], NULL, NULL, &io[0],
            IOCTL_AFD_POLL, in_params, params_size, out_params[0], params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    CancelIo((HANDLE)socks[0]);

    ret = WaitForSingleObject(events[0], 100);
    ok(ret == STATUS_SUCCESS, "got %#x\n", ret);

    in_params->exclusive = TRUE;

    ret = NtDeviceIoControlFile((HANDLE)ctl_sock, events[1], NULL, NULL, &io[1],
            IOCTL_AFD_POLL, in_params, params_size, out_params[1], params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    ret = NtDeviceIoControlFile((HANDLE)ctl_sock, events[2], NULL, NULL, &io[2],
            IOCTL_AFD_POLL, in_params, params_size, out_params[2], params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    ret = WaitForSingleObject(events[1], 100);
    ok(ret == STATUS_SUCCESS, "got %#x\n", ret);

    ret = WaitForSingleObject(events[2], 100);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    CancelIo((HANDLE)ctl_sock);

    /* The exclusive check does not happen again after the call to NtDeviceIoControlFile(). */

    in_params->exclusive = FALSE;

    ret = NtDeviceIoControlFile((HANDLE)socks[0], events[0], NULL, NULL, &io[0],
            IOCTL_AFD_POLL, in_params, params_size, out_params[0], params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    in_params->exclusive = TRUE;

    ret = NtDeviceIoControlFile((HANDLE)ctl_sock, events[1], NULL, NULL, &io[1],
            IOCTL_AFD_POLL, in_params, params_size, out_params[1], params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    CancelIo((HANDLE)socks[0]);

    ret = WaitForSingleObject(events[0], 100);
    ok(ret == STATUS_SUCCESS, "got %#x\n", ret);

    ret = NtDeviceIoControlFile((HANDLE)ctl_sock, events[2], NULL, NULL, &io[2],
            IOCTL_AFD_POLL, in_params, params_size, out_params[2], params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    ret = WaitForSingleObject(events[1], 100);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    ret = WaitForSingleObject(events[2], 100);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    CancelIo((HANDLE)ctl_sock);

    /* After the main poll is terminated, any subsequent poll becomes the main
     * and can be terminated if exclusive. */

    in_params->exclusive = FALSE;

    ret = NtDeviceIoControlFile((HANDLE)socks[0], events[0], NULL, NULL, &io[0],
            IOCTL_AFD_POLL, in_params, params_size, out_params[0], params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    in_params->exclusive = TRUE;

    ret = NtDeviceIoControlFile((HANDLE)ctl_sock, events[1], NULL, NULL, &io[1],
            IOCTL_AFD_POLL, in_params, params_size, out_params[1], params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    CancelIo((HANDLE)socks[0]);

    ret = WaitForSingleObject(events[0], 100);
    ok(ret == STATUS_SUCCESS, "got %#x\n", ret);

    ret = NtDeviceIoControlFile((HANDLE)ctl_sock, events[2], NULL, NULL, &io[2],
            IOCTL_AFD_POLL, in_params, params_size, out_params[2], params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    ret = NtDeviceIoControlFile((HANDLE)ctl_sock, events[3], NULL, NULL, &io[3],
            IOCTL_AFD_POLL, in_params, params_size, out_params[3], params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    ret = WaitForSingleObject(events[1], 100);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    ret = WaitForSingleObject(events[2], 100);
    ok(ret == STATUS_SUCCESS, "got %#x\n", ret);

    ret = WaitForSingleObject(events[3], 100);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    CancelIo((HANDLE)ctl_sock);

    /***** Exclusive poll on different sockets *****/

    in_params->exclusive = TRUE;
    in_params->sockets[0].socket = socks[0];

    ret = NtDeviceIoControlFile((HANDLE)ctl_sock, events[0], NULL, NULL, &io[0],
            IOCTL_AFD_POLL, in_params, params_size, out_params[0], params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    in_params->sockets[0].socket = socks[1];

    ret = NtDeviceIoControlFile((HANDLE)ctl_sock, events[1], NULL, NULL, &io[1],
            IOCTL_AFD_POLL, in_params, params_size, out_params[1], params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    ret = WaitForSingleObject(events[0], 100);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    ret = WaitForSingleObject(events[1], 100);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    CancelIo((HANDLE)ctl_sock);

    /***** Exclusive poll from other thread *****/

    in_params->exclusive = TRUE;
    in_params->sockets[0].socket = ctl_sock;

    ret = NtDeviceIoControlFile((HANDLE)ctl_sock, events[0], NULL, NULL, &io[0],
            IOCTL_AFD_POLL, in_params, params_size, out_params[0], params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    cb_ctx.ctl_sock = ctl_sock;
    cb_ctx.event = events[1];
    cb_ctx.io = &io[1];
    cb_ctx.params_size = params_size;
    cb_ctx.in_params = in_params;
    cb_ctx.out_params = out_params[1];

    thrd = CreateThread(NULL, 0, poll_exclusive_thread_cb, &cb_ctx, 0, NULL);
    WaitForSingleObject(thrd, INFINITE);
    CloseHandle(thrd);

    ret = WaitForSingleObject(events[0], 100);
    ok(ret == STATUS_SUCCESS, "got %#x\n", ret);

    CancelIo((HANDLE)ctl_sock);

    /***** Exclusive poll on overlapping socket sets *****/

    params_size = offsetof(struct afd_poll_params, sockets[2]);

    in_params->exclusive = TRUE;
    in_params->count = 2;
    in_params->sockets[0].socket = ctl_sock;
    in_params->sockets[1].socket = socks[0];

    ret = NtDeviceIoControlFile((HANDLE)ctl_sock, events[0], NULL, NULL, &io[0],
            IOCTL_AFD_POLL, in_params, params_size, out_params[0], params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    in_params->sockets[0].socket = ctl_sock;
    in_params->sockets[1].socket = socks[1];

    ret = NtDeviceIoControlFile((HANDLE)ctl_sock, events[1], NULL, NULL, &io[1],
            IOCTL_AFD_POLL, in_params, params_size, out_params[1], params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    ret = WaitForSingleObject(events[0], 100);
    ok(ret == STATUS_SUCCESS, "got %#x\n", ret);

    ret = WaitForSingleObject(events[1], 100);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    CancelIo((HANDLE)ctl_sock);

    /* Cleanup. */

    closesocket(ctl_sock);

    for (i = 0; i < POLL_CNT; i++)
    {
        closesocket(socks[i]);
        CloseHandle(events[i]);
    }
}

#undef POLL_SOCK_CNT
#undef POLL_CNT

static void test_poll_completion_port(void)
{
    struct afd_poll_params params = {0};
    LARGE_INTEGER zero = {{0}};
    SOCKET client, server;
    ULONG_PTR key, value;
    IO_STATUS_BLOCK io;
    HANDLE event, port;
    int ret;

    event = CreateEventW(NULL, TRUE, FALSE, NULL);
    tcp_socketpair(&client, &server);
    port = CreateIoCompletionPort((HANDLE)client, NULL, 0, 0);

    params.timeout = -100 * 10000;
    params.count = 1;
    params.sockets[0].socket = client;
    params.sockets[0].flags = AFD_POLL_WRITE;
    params.sockets[0].status = 0xdeadbeef;

    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_POLL, &params, sizeof(params), &params, sizeof(params));
    ok(!ret, "got %#x\n", ret);

    ret = NtRemoveIoCompletion(port, &key, &value, &io, &zero);
    ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);

    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, (void *)0xdeadbeef, &io,
            IOCTL_AFD_POLL, &params, sizeof(params), &params, sizeof(params));
    ok(!ret, "got %#x\n", ret);

    ret = NtRemoveIoCompletion(port, &key, &value, &io, &zero);
    ok(!ret, "got %#x\n", ret);
    ok(!key, "got key %#Ix\n", key);
    ok(value == 0xdeadbeef, "got value %#Ix\n", value);

    params.timeout = 0;
    params.count = 1;
    params.sockets[0].socket = client;
    params.sockets[0].flags = AFD_POLL_READ;
    params.sockets[0].status = 0xdeadbeef;

    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, (void *)0xdeadbeef, &io,
            IOCTL_AFD_POLL, &params, sizeof(params), &params, sizeof(params));
    ok(!ret, "got %#x\n", ret);

    ret = NtRemoveIoCompletion(port, &key, &value, &io, &zero);
    ok(!ret, "got %#x\n", ret);
    ok(!key, "got key %#Ix\n", key);
    ok(value == 0xdeadbeef, "got value %#Ix\n", value);

    /* Close a socket while polling on another. */

    params.timeout = -100 * 10000;
    params.count = 1;
    params.sockets[0].socket = server;
    params.sockets[0].flags = AFD_POLL_READ;
    params.sockets[0].status = 0xdeadbeef;

    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, (void *)0xdeadbeef, &io,
            IOCTL_AFD_POLL, &params, sizeof(params), &params, sizeof(params));
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    closesocket(client);

    ret = WaitForSingleObject(event, 1000);
    ok(!ret, "got %#x\n", ret);
    todo_wine ok(io.Status == STATUS_TIMEOUT, "got %#lx\n", io.Status);
    todo_wine ok(io.Information == offsetof(struct afd_poll_params, sockets[0]), "got %#Ix\n", io.Information);
    todo_wine ok(!params.count, "got count %u\n", params.count);

    ret = NtRemoveIoCompletion(port, &key, &value, &io, &zero);
    ok(!ret, "got %#x\n", ret);
    ok(!key, "got key %#Ix\n", key);
    ok(value == 0xdeadbeef, "got value %#Ix\n", value);

    CloseHandle(port);
    closesocket(server);
    CloseHandle(event);
}

static void test_poll_reset(void)
{
    char in_buffer[offsetof(struct afd_poll_params, sockets[3])];
    char out_buffer[offsetof(struct afd_poll_params, sockets[3])];
    struct afd_poll_params *in_params = (struct afd_poll_params *)in_buffer;
    struct afd_poll_params *out_params = (struct afd_poll_params *)out_buffer;
    SOCKET client, server;
    IO_STATUS_BLOCK io;
    ULONG params_size;
    HANDLE event;
    int ret;

    memset(in_buffer, 0, sizeof(in_buffer));
    memset(out_buffer, 0, sizeof(out_buffer));
    event = CreateEventW(NULL, TRUE, FALSE, NULL);
    tcp_socketpair(&client, &server);

    in_params->timeout = -1000 * 10000;
    in_params->count = 1;
    in_params->sockets[0].socket = client;
    in_params->sockets[0].flags = ~(AFD_POLL_WRITE | AFD_POLL_CONNECT);
    params_size = offsetof(struct afd_poll_params, sockets[1]);

    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_POLL, in_params, params_size, out_params, params_size);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    close_with_rst(server);

    ret = WaitForSingleObject(event, 100);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got %#lx\n", io.Status);
    ok(io.Information == offsetof(struct afd_poll_params, sockets[1]), "got %#Ix\n", io.Information);
    ok(out_params->count == 1, "got count %u\n", out_params->count);
    ok(out_params->sockets[0].socket == client, "got socket %#Ix\n", out_params->sockets[0].socket);
    todo_wine ok(out_params->sockets[0].flags == AFD_POLL_RESET, "got flags %#x\n", out_params->sockets[0].flags);
    ok(!out_params->sockets[0].status, "got status %#x\n", out_params->sockets[0].status);

    check_poll_todo(client, event, AFD_POLL_WRITE | AFD_POLL_CONNECT | AFD_POLL_RESET);

    closesocket(client);
    CloseHandle(event);
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
    ok(!io.Status, "got status %#lx\n", io.Status);
    ok(!io.Information, "got information %#Ix\n", io.Information);

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ret = connect(client, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "got error %u\n", WSAGetLastError());
    server = accept(listener, NULL, NULL);
    ok(server != -1, "got error %u\n", WSAGetLastError());

    memset(&io, 0, sizeof(io));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io, IOCTL_AFD_RECV, NULL, 0, NULL, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#x\n", ret);
    ok(!io.Status, "got status %#lx\n", io.Status);
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
    ok(!io.Status, "got status %#lx\n", io.Status);
    ok(!io.Information, "got information %#Ix\n", io.Information);

    /* These structures need not remain valid. */
    memset(&params, 0xcc, sizeof(params));
    memset(wsabufs, 0xcc, sizeof(wsabufs));

    ret = send(server, "data", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    ret = WaitForSingleObject(event, 200);
    ok(!ret, "wait timed out\n");
    ok(!io.Status, "got %#lx\n", io.Status);
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
    ok(!io.Status, "got status %#lx\n", io.Status);
    ok(!io.Information, "got information %#Ix\n", io.Information);

    ret = send(server, "data", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    ret = WaitForSingleObject(event, 200);
    ok(!ret, "wait timed out\n");
    ok(!io.Status, "got %#lx\n", io.Status);
    ok(io.Information == 5, "got %#Ix\n", io.Information);
    ok(!strcmp(buffer, "da\xccta"), "got %s\n", debugstr_an(buffer, io.Information));

    /* Test synchronous return. */

    ret = send(server, "data", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    /* wait for the data to be available */
    check_poll_mask(client, event, AFD_POLL_READ, AFD_POLL_READ);

    memset(&io, 0xcc, sizeof(io));
    memset(buffer, 0xcc, sizeof(buffer));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got %#lx\n", io.Status);
    ok(io.Information == 5, "got %#Ix\n", io.Information);
    ok(!strcmp(buffer, "da\xccta"), "got %s\n", debugstr_an(buffer, io.Information));

    /* Test nonblocking mode. */

    set_blocking(client, FALSE);

    memset(&io, 0, sizeof(io));
    memset(buffer, 0xcc, sizeof(buffer));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    ok(ret == STATUS_DEVICE_NOT_READY, "got %#x\n", ret);
    ok(!io.Status, "got status %#lx\n", io.Status);
    ok(!io.Information, "got information %#Ix\n", io.Information);

    ret = send(server, "data", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    /* wait for the data to be available */
    check_poll_mask(client, event, AFD_POLL_READ, AFD_POLL_READ);

    memset(&io, 0xcc, sizeof(io));
    memset(buffer, 0xcc, sizeof(buffer));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got %#lx\n", io.Status);
    ok(io.Information == 5, "got %#Ix\n", io.Information);
    ok(!strcmp(buffer, "da\xccta"), "got %s\n", debugstr_an(buffer, io.Information));

    params.recv_flags = AFD_RECV_FORCE_ASYNC;

    memset(&io, 0, sizeof(io));
    memset(buffer, 0xcc, sizeof(buffer));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);
    ok(!io.Status, "got status %#lx\n", io.Status);
    ok(!io.Information, "got information %#Ix\n", io.Information);

    ret = send(server, "data", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    ret = WaitForSingleObject(event, 200);
    ok(!ret, "wait timed out\n");
    ok(!io.Status, "got %#lx\n", io.Status);
    ok(io.Information == 5, "got %#Ix\n", io.Information);
    ok(!strcmp(buffer, "da\xccta"), "got %s\n", debugstr_an(buffer, io.Information));

    params.recv_flags = 0;

    set_blocking(client, TRUE);

    /* Test flags. */

    ret = send(server, "a", 1, MSG_OOB);
    ok(ret == 1, "got %d\n", ret);

    ret = send(server, "data", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    /* wait for the data to be available */
    check_poll_mask(client, event, AFD_POLL_READ, AFD_POLL_READ);

    memset(&io, 0xcc, sizeof(io));
    memset(buffer, 0xcc, sizeof(buffer));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got %#lx\n", io.Status);
    ok(io.Information == 5, "got %#Ix\n", io.Information);
    ok(!strcmp(buffer, "da\xccta"), "got %s\n", debugstr_an(buffer, io.Information));

    params.msg_flags = 0;

    io.Status = 0xdeadbeef;
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#x\n", ret);
    ok(io.Status == 0xdeadbeef, "got %#lx\n", io.Status);

    params.msg_flags = AFD_MSG_OOB | AFD_MSG_NOT_OOB;

    io.Status = 0xdeadbeef;
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#x\n", ret);
    ok(io.Status == 0xdeadbeef, "got %#lx\n", io.Status);

    params.msg_flags = AFD_MSG_OOB;

    memset(buffer, 0xcc, sizeof(buffer));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    todo_wine ok(!ret, "got %#x\n", ret);
    todo_wine ok(!io.Status, "got %#lx\n", io.Status);
    todo_wine ok(io.Information == 1, "got %#Ix\n", io.Information);
    todo_wine ok(buffer[0] == 'a', "got %s\n", debugstr_an(buffer, io.Information));
    if (ret == STATUS_PENDING)
    {
        CancelIo((HANDLE)client);
        ret = WaitForSingleObject(event, 100);
        ok(!ret, "wait timed out\n");
    }

    params.msg_flags = AFD_MSG_NOT_OOB | AFD_MSG_PEEK;

    ret = send(server, "data", 4, 0);
    ok(ret == 4, "got %d\n", ret);

    /* wait for the data to be available */
    check_poll_mask(client, event, AFD_POLL_READ, AFD_POLL_READ);

    memset(buffer, 0xcc, sizeof(buffer));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got %#lx\n", io.Status);
    ok(io.Information == 4, "got %#Ix\n", io.Information);
    ok(!memcmp(buffer, "da\xccta", 5), "got %s\n", debugstr_an(buffer, io.Information));

    memset(buffer, 0xcc, sizeof(buffer));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got %#lx\n", io.Status);
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
        ok(!io.Status, "got %#lx\n", io.Status);
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
    ok(!io.Status, "got %#lx\n", io.Status);
    ok(!io.Information, "got %#Ix\n", io.Information);

    ret = shutdown(client, SD_RECEIVE);
    ok(!ret, "got error %u\n", WSAGetLastError());

    memset(&io, 0, sizeof(io));
    memset(buffer, 0xcc, sizeof(buffer));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    ok(ret == STATUS_PIPE_DISCONNECTED, "got %#x\n", ret);
    ok(!io.Status, "got status %#lx\n", io.Status);
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
    ret = getsockname(client, (struct sockaddr *)&addr, &len);
    ok(!ret, "got error %u\n", WSAGetLastError());

    memset(buffer, 0xcc, sizeof(buffer));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    ok(ret == STATUS_PENDING, "got %#x\n", ret);

    ret = sendto(server, "data", 5, 0, (struct sockaddr *)&addr, sizeof(addr));
    ok(ret == 5, "got %d\n", ret);

    ret = WaitForSingleObject(event, 200);
    ok(!ret, "wait timed out\n");
    ok(!io.Status, "got %#lx\n", io.Status);
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
    ok(io.Status == STATUS_BUFFER_OVERFLOW, "got %#lx\n", io.Status);
    ok(io.Information == 6, "got %#Ix\n", io.Information);
    ok(!memcmp(buffer, "mo\xccreda\xcc", 7), "got %s\n", debugstr_an(buffer, io.Information));

    ret = sendto(server, "moredata", 9, 0, (struct sockaddr *)&addr, sizeof(addr));
    ok(ret == 9, "got %d\n", ret);

    /* wait for the data to be available */
    check_poll_mask(client, event, AFD_POLL_READ, AFD_POLL_READ);

    memset(&io, 0, sizeof(io));
    memset(buffer, 0xcc, sizeof(buffer));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_RECV, &params, sizeof(params), NULL, 0);
    ok(ret == STATUS_BUFFER_OVERFLOW, "got %#x\n", ret);
    ok(io.Status == STATUS_BUFFER_OVERFLOW, "got %#lx\n", io.Status);
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
    todo_wine ok(io.Status == STATUS_CANCELLED, "got %#lx\n", io.Status);
    ok(!io.Information, "got %#Ix\n", io.Information);

    closesocket(server);
    CloseHandle(event);
}

static void test_event_select(void)
{
    struct afd_event_select_params params;
    WSANETWORKEVENTS events;
    SOCKET client, server;
    IO_STATUS_BLOCK io;
    HANDLE event;
    int ret;

    event = CreateEventW(NULL, TRUE, FALSE, NULL);

    tcp_socketpair(&client, &server);

    memset(&io, 0xcc, sizeof(io));
    ret = NtDeviceIoControlFile((HANDLE)client, NULL, NULL, NULL, &io,
            IOCTL_AFD_EVENT_SELECT, NULL, 0, NULL, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#x\n", ret);
    ok(io.Status == STATUS_INVALID_PARAMETER, "got status %#lx\n", io.Status);
    ok(!io.Information, "got information %#Ix\n", io.Information);

    params.event = 0;
    params.mask = ~0;
    memset(&io, 0xcc, sizeof(io));
    ret = NtDeviceIoControlFile((HANDLE)client, NULL, NULL, NULL, &io,
            IOCTL_AFD_EVENT_SELECT, &params, sizeof(params), NULL, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#x\n", ret);
    ok(io.Status == STATUS_INVALID_PARAMETER, "got status %#lx\n", io.Status);
    ok(!io.Information, "got information %#Ix\n", io.Information);

    params.event = event;
    params.mask = ~0;
    memset(&io, 0xcc, sizeof(io));
    ret = NtDeviceIoControlFile((HANDLE)client, NULL, NULL, NULL, &io,
            IOCTL_AFD_EVENT_SELECT, &params, sizeof(params), NULL, 0);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got status %#lx\n", io.Status);
    ok(!io.Information, "got information %#Ix\n", io.Information);

    ret = WSAEnumNetworkEvents(client, event, &events);
    ok(!ret, "got error %u\n", WSAGetLastError());
    ok(events.lNetworkEvents == (FD_CONNECT | FD_WRITE), "got events %#lx\n", events.lNetworkEvents);

    closesocket(client);
    closesocket(server);

    tcp_socketpair(&client, &server);

    params.event = event;
    params.mask = AFD_POLL_CONNECT;
    memset(&io, 0xcc, sizeof(io));
    ret = NtDeviceIoControlFile((HANDLE)client, NULL, NULL, NULL, &io,
            IOCTL_AFD_EVENT_SELECT, &params, sizeof(params), NULL, 0);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got status %#lx\n", io.Status);
    ok(!io.Information, "got information %#Ix\n", io.Information);

    ret = WSAEnumNetworkEvents(client, event, &events);
    ok(!ret, "got error %u\n", WSAGetLastError());
    ok(events.lNetworkEvents == FD_CONNECT, "got events %#lx\n", events.lNetworkEvents);

    closesocket(client);
    closesocket(server);

    tcp_socketpair(&client, &server);

    params.event = event;
    params.mask = ~0;
    memset(&io, 0xcc, sizeof(io));
    ret = NtDeviceIoControlFile((HANDLE)client, NULL, NULL, NULL, &io,
            IOCTL_AFD_EVENT_SELECT, &params, sizeof(params), NULL, 0);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got status %#lx\n", io.Status);
    ok(!io.Information, "got information %#Ix\n", io.Information);

    params.event = 0;
    params.mask = 0;
    memset(&io, 0xcc, sizeof(io));
    ret = NtDeviceIoControlFile((HANDLE)client, NULL, NULL, NULL, &io,
            IOCTL_AFD_EVENT_SELECT, &params, sizeof(params), NULL, 0);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got status %#lx\n", io.Status);
    ok(!io.Information, "got information %#Ix\n", io.Information);

    ret = WSAEnumNetworkEvents(client, event, &events);
    ok(!ret, "got error %u\n", WSAGetLastError());
    ok(!events.lNetworkEvents, "got events %#lx\n", events.lNetworkEvents);

    closesocket(client);
    closesocket(server);

    CloseHandle(event);
}

static void test_get_events(void)
{
    const struct sockaddr_in invalid_addr =
    {
        .sin_family = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_LOOPBACK),
        .sin_port = 255,
    };
    struct afd_get_events_params params;
    WSANETWORKEVENTS events;
    SOCKET client, server;
    IO_STATUS_BLOCK io;
    unsigned int i;
    HANDLE event;
    int ret;

    event = CreateEventW(NULL, TRUE, FALSE, NULL);

    tcp_socketpair(&client, &server);

    ret = WSAEventSelect(client, event, FD_ACCEPT | FD_CLOSE | FD_CONNECT | FD_OOB | FD_READ | FD_WRITE);
    ok(!ret, "got error %lu\n", GetLastError());

    memset(&params, 0xcc, sizeof(params));
    memset(&io, 0xcc, sizeof(io));
    ret = NtDeviceIoControlFile((HANDLE)client, NULL, NULL, NULL, &io,
            IOCTL_AFD_GET_EVENTS, NULL, 0, NULL, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#x\n", ret);

    memset(&params, 0xcc, sizeof(params));
    memset(&io, 0xcc, sizeof(io));
    ret = NtDeviceIoControlFile((HANDLE)client, NULL, NULL, NULL, &io,
            IOCTL_AFD_GET_EVENTS, NULL, 0, &params, sizeof(params));
    ok(!ret, "got %#x\n", ret);
    ok(params.flags == (AFD_POLL_WRITE | AFD_POLL_CONNECT), "got flags %#x\n", params.flags);
    for (i = 0; i < ARRAY_SIZE(params.status); ++i)
        ok(!params.status[i], "got status[%u] %#x\n", i, params.status[i]);

    ret = WSAEnumNetworkEvents(client, event, &events);
    ok(!ret, "got error %lu\n", GetLastError());
    ok(!events.lNetworkEvents, "got events %#lx\n", events.lNetworkEvents);

    ret = WSAEventSelect(server, event, FD_ACCEPT | FD_CLOSE | FD_CONNECT | FD_OOB | FD_READ | FD_WRITE);
    ok(!ret, "got error %lu\n", GetLastError());

    memset(&params, 0xcc, sizeof(params));
    memset(&io, 0xcc, sizeof(io));
    ret = NtDeviceIoControlFile((HANDLE)server, NULL, NULL, NULL, &io,
            IOCTL_AFD_GET_EVENTS, NULL, 0, &params, sizeof(params));
    ok(!ret, "got %#x\n", ret);
    ok(params.flags == AFD_POLL_WRITE, "got flags %#x\n", params.flags);
    for (i = 0; i < ARRAY_SIZE(params.status); ++i)
        ok(!params.status[i], "got status[%u] %#x\n", i, params.status[i]);

    closesocket(client);
    closesocket(server);

    tcp_socketpair(&client, &server);

    ret = WSAEventSelect(client, event, FD_ACCEPT | FD_CLOSE | FD_CONNECT | FD_OOB | FD_READ | FD_WRITE);
    ok(!ret, "got error %lu\n", GetLastError());

    ret = WSAEnumNetworkEvents(client, event, &events);
    ok(!ret, "got error %lu\n", GetLastError());
    ok(events.lNetworkEvents == (FD_WRITE | FD_CONNECT), "got events %#lx\n", events.lNetworkEvents);

    memset(&params, 0xcc, sizeof(params));
    memset(&io, 0xcc, sizeof(io));
    ret = NtDeviceIoControlFile((HANDLE)client, NULL, NULL, NULL, &io,
            IOCTL_AFD_GET_EVENTS, NULL, 0, &params, sizeof(params));
    ok(!ret, "got %#x\n", ret);
    ok(!params.flags, "got flags %#x\n", params.flags);
    for (i = 0; i < ARRAY_SIZE(params.status); ++i)
        ok(!params.status[i], "got status[%u] %#x\n", i, params.status[i]);

    closesocket(client);
    closesocket(server);

    /* Test a failed connection. The following call can take over 2 seconds to
     * complete, so make the test interactive-only. */

    if (winetest_interactive)
    {
        client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        ResetEvent(event);
        ret = WSAEventSelect(client, event, FD_CONNECT);
        ok(!ret, "got error %lu\n", GetLastError());

        ret = connect(client, (struct sockaddr *)&invalid_addr, sizeof(invalid_addr));
        ok(ret == -1, "expected failure\n");
        ok(WSAGetLastError() == WSAEWOULDBLOCK, "got error %u\n", WSAGetLastError());

        ret = WaitForSingleObject(event, 10000);
        ok(!ret, "got %#x\n", ret);

        memset(&params, 0xcc, sizeof(params));
        memset(&io, 0xcc, sizeof(io));
        ret = NtDeviceIoControlFile((HANDLE)client, NULL, NULL, NULL, &io,
                IOCTL_AFD_GET_EVENTS, NULL, 0, &params, sizeof(params));
        ok(!ret, "got %#x\n", ret);
        ok(params.flags == AFD_POLL_CONNECT_ERR, "got flags %#x\n", params.flags);
        for (i = 0; i < ARRAY_SIZE(params.status); ++i)
        {
            if (i == AFD_POLL_BIT_CONNECT_ERR)
                ok(params.status[i] == STATUS_CONNECTION_REFUSED, "got status[%u] %#x\n", i, params.status[i]);
            else
                ok(!params.status[i], "got status[%u] %#x\n", i, params.status[i]);
        }

        ResetEvent(event);
        ret = WSAEventSelect(client, event, FD_CONNECT);
        ok(!ret, "got error %lu\n", GetLastError());

        ret = WaitForSingleObject(event, 0);
        ok(ret == WAIT_TIMEOUT, "got %#x\n", ret);

        memset(&params, 0xcc, sizeof(params));
        memset(&io, 0xcc, sizeof(io));
        ret = NtDeviceIoControlFile((HANDLE)client, NULL, NULL, NULL, &io,
                IOCTL_AFD_GET_EVENTS, NULL, 0, &params, sizeof(params));
        ok(!ret, "got %#x\n", ret);
        ok(!params.flags, "got flags %#x\n", params.flags);
        for (i = 0; i < ARRAY_SIZE(params.status); ++i)
        {
            if (i == AFD_POLL_BIT_CONNECT_ERR)
                ok(params.status[i] == STATUS_CONNECTION_REFUSED, "got status[%u] %#x\n", i, params.status[i]);
            else
                ok(!params.status[i], "got status[%u] %#x\n", i, params.status[i]);
        }

        closesocket(client);
    }

    CloseHandle(event);
}

static void test_get_events_reset(void)
{
    struct afd_get_events_params params;
    SOCKET client, server;
    IO_STATUS_BLOCK io;
    unsigned int i;
    HANDLE event;
    int ret;

    event = CreateEventW(NULL, TRUE, FALSE, NULL);

    tcp_socketpair(&client, &server);

    ret = WSAEventSelect(client, event, FD_ACCEPT | FD_CONNECT | FD_CLOSE | FD_OOB | FD_READ | FD_WRITE);
    ok(!ret, "got error %lu\n", GetLastError());

    close_with_rst(server);

    memset(&params, 0xcc, sizeof(params));
    memset(&io, 0xcc, sizeof(io));
    ret = NtDeviceIoControlFile((HANDLE)client, NULL, NULL, NULL, &io,
            IOCTL_AFD_GET_EVENTS, NULL, 0, &params, sizeof(params));
    ok(!ret, "got %#x\n", ret);
    todo_wine ok(params.flags == (AFD_POLL_RESET | AFD_POLL_CONNECT | AFD_POLL_WRITE), "got flags %#x\n", params.flags);
    for (i = 0; i < ARRAY_SIZE(params.status); ++i)
        ok(!params.status[i], "got status[%u] %#x\n", i, params.status[i]);

    closesocket(client);

    tcp_socketpair(&client, &server);

    ret = WSAEventSelect(server, event, FD_ACCEPT | FD_CONNECT | FD_CLOSE | FD_OOB | FD_READ | FD_WRITE);
    ok(!ret, "got error %lu\n", GetLastError());

    close_with_rst(client);

    memset(&params, 0xcc, sizeof(params));
    memset(&io, 0xcc, sizeof(io));
    ret = NtDeviceIoControlFile((HANDLE)server, NULL, NULL, NULL, &io,
            IOCTL_AFD_GET_EVENTS, NULL, 0, &params, sizeof(params));
    ok(!ret, "got %#x\n", ret);
    todo_wine ok(params.flags == (AFD_POLL_RESET | AFD_POLL_WRITE), "got flags %#x\n", params.flags);
    for (i = 0; i < ARRAY_SIZE(params.status); ++i)
        ok(!params.status[i], "got status[%u] %#x\n", i, params.status[i]);

    closesocket(server);

    CloseHandle(event);
}

static void test_bind(void)
{
    const struct sockaddr_in6 bind_addr6 = {.sin6_family = AF_INET6, .sin6_addr.s6_words = {0, 0, 0, 0, 0, 0, 0, htons(1)}};
    const struct sockaddr_in6 invalid_addr6 = {.sin6_family = AF_INET6, .sin6_addr.s6_words = {htons(0x0100)}};
    const struct sockaddr_in invalid_addr = {.sin_family = AF_INET, .sin_addr.s_addr = inet_addr("192.0.2.0")};
    const struct sockaddr_in bind_addr = {.sin_family = AF_INET, .sin_addr.s_addr = htonl(INADDR_LOOPBACK)};
    static const size_t params6_size = offsetof(struct afd_bind_params, addr) + sizeof(struct sockaddr_in6);
    static const size_t params4_size = offsetof(struct afd_bind_params, addr) + sizeof(struct sockaddr_in);
    struct afd_bind_params *params = malloc(params6_size);
    struct sockaddr_in6 addr6, addr6_2;
    struct sockaddr_in addr, addr2;
    struct hostent *host;
    IO_STATUS_BLOCK io;
    unsigned int i;
    HANDLE event;
    SOCKET s, s2;
    int ret;

    event = CreateEventW(NULL, TRUE, FALSE, NULL);
    memset(&addr, 0xcc, sizeof(addr));
    memset(params, 0, params6_size);

    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    params->addr.sa_family = 0xdead;
    ret = NtDeviceIoControlFile((HANDLE)s, event, NULL, NULL, &io, IOCTL_AFD_BIND,
            params, params4_size, &addr, sizeof(addr));
    ok(ret == STATUS_INVALID_ADDRESS, "got %#x\n", ret);

    memcpy(&params->addr, &bind_addr, sizeof(bind_addr));
    ret = NtDeviceIoControlFile((HANDLE)s, event, NULL, NULL, &io, IOCTL_AFD_BIND,
            params, params4_size - 1, &addr, sizeof(addr));
    ok(ret == STATUS_INVALID_ADDRESS, "got %#x\n", ret);

    memcpy(&params->addr, &bind_addr, sizeof(bind_addr));
    ret = NtDeviceIoControlFile((HANDLE)s, event, NULL, NULL, &io, IOCTL_AFD_BIND,
            params, offsetof(struct afd_bind_params, addr.sa_data), &addr, sizeof(addr));
    ok(ret == STATUS_INVALID_ADDRESS, "got %#x\n", ret);

    memcpy(&params->addr, &bind_addr, sizeof(bind_addr));
    ret = NtDeviceIoControlFile((HANDLE)s, event, NULL, NULL, &io, IOCTL_AFD_BIND,
            params, offsetof(struct afd_bind_params, addr.sa_data) - 1, &addr, sizeof(addr));
    ok(ret == STATUS_INVALID_PARAMETER, "got %#x\n", ret);

    memcpy(&params->addr, &invalid_addr, sizeof(invalid_addr));
    ret = NtDeviceIoControlFile((HANDLE)s, event, NULL, NULL, &io, IOCTL_AFD_BIND,
            params, params4_size, &addr, sizeof(addr));
    todo_wine ok(ret == STATUS_PENDING, "got %#x\n", ret);
    ret = WaitForSingleObject(event, 0);
    ok(!ret, "got %#x\n", ret);
    ok(io.Status == STATUS_INVALID_ADDRESS_COMPONENT, "got %#lx\n", io.Status);

    memcpy(&params->addr, &bind_addr, sizeof(bind_addr));
    ret = NtDeviceIoControlFile((HANDLE)s, event, NULL, NULL, &io, IOCTL_AFD_BIND,
            params, params4_size, &addr, sizeof(addr) - 1);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#x\n", ret);

    memcpy(&params->addr, &bind_addr, sizeof(bind_addr));
    memset(&io, 0xcc, sizeof(io));
    memset(&addr, 0xcc, sizeof(addr));
    ret = NtDeviceIoControlFile((HANDLE)s, event, NULL, NULL, &io, IOCTL_AFD_BIND,
            params, params4_size, &addr, sizeof(addr));
    todo_wine ok(ret == STATUS_PENDING, "got %#x\n", ret);
    ret = WaitForSingleObject(event, 0);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got %#lx\n", io.Status);
    ok(io.Information == sizeof(addr), "got %#Ix\n", io.Information);
    ok(addr.sin_family == AF_INET, "got family %u\n", addr.sin_family);
    ok(addr.sin_addr.s_addr == htonl(INADDR_LOOPBACK), "got address %#08lx\n", addr.sin_addr.s_addr);
    ok(addr.sin_port, "expected nonzero port\n");

    /* getsockname() returns EINVAL here. Possibly the socket name is cached (in shared memory?) */
    memset(&addr2, 0xcc, sizeof(addr2));
    ret = NtDeviceIoControlFile((HANDLE)s, event, NULL, NULL, &io,
            IOCTL_AFD_GETSOCKNAME, NULL, 0, &addr2, sizeof(addr2));
    ok(!ret, "got %#x\n", ret);
    ok(!memcmp(&addr, &addr2, sizeof(addr)), "addresses didn't match\n");

    ret = NtDeviceIoControlFile((HANDLE)s, event, NULL, NULL, &io, IOCTL_AFD_BIND,
            params, params4_size, &addr, sizeof(addr));
    ok(ret == STATUS_ADDRESS_ALREADY_ASSOCIATED, "got %#x\n", ret);

    s2 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    memcpy(&params->addr, &addr2, sizeof(addr2));
    memset(&io, 0xcc, sizeof(io));
    memset(&addr, 0xcc, sizeof(addr));
    ret = NtDeviceIoControlFile((HANDLE)s2, event, NULL, NULL, &io, IOCTL_AFD_BIND,
            params, params4_size, &addr, sizeof(addr));
    todo_wine ok(ret == STATUS_PENDING, "got %#x\n", ret);
    ret = WaitForSingleObject(event, 0);
    ok(!ret, "got %#x\n", ret);
    ok(io.Status == STATUS_SHARING_VIOLATION, "got %#lx\n", io.Status);
    ok(!io.Information, "got %#Ix\n", io.Information);

    closesocket(s2);
    closesocket(s);

    /* test UDP */

    s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    memcpy(&params->addr, &bind_addr, sizeof(bind_addr));
    memset(&io, 0xcc, sizeof(io));
    memset(&addr, 0xcc, sizeof(addr));
    ret = NtDeviceIoControlFile((HANDLE)s, event, NULL, NULL, &io, IOCTL_AFD_BIND,
            params, params4_size, &addr, sizeof(addr));
    todo_wine ok(ret == STATUS_PENDING, "got %#x\n", ret);
    ret = WaitForSingleObject(event, 0);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got %#lx\n", io.Status);
    ok(io.Information == sizeof(addr), "got %#Ix\n", io.Information);
    ok(addr.sin_family == AF_INET, "got family %u\n", addr.sin_family);
    ok(addr.sin_addr.s_addr == htonl(INADDR_LOOPBACK), "got address %#08lx\n", addr.sin_addr.s_addr);
    ok(addr.sin_port, "expected nonzero port\n");

    memset(&addr2, 0xcc, sizeof(addr2));
    ret = NtDeviceIoControlFile((HANDLE)s, event, NULL, NULL, &io,
            IOCTL_AFD_GETSOCKNAME, NULL, 0, &addr2, sizeof(addr2));
    ok(!ret, "got %#x\n", ret);
    ok(!memcmp(&addr, &addr2, sizeof(addr)), "addresses didn't match\n");

    closesocket(s);

    host = gethostbyname("");
    if (host && host->h_length == 4)
    {
        for (i = 0; host->h_addr_list[i]; ++i)
        {
            ULONG in_addr = *(ULONG *)host->h_addr_list[i];

            s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

            ((struct sockaddr_in *)&params->addr)->sin_addr.s_addr = in_addr;
            memset(&io, 0xcc, sizeof(io));
            memset(&addr, 0xcc, sizeof(addr));
            ret = NtDeviceIoControlFile((HANDLE)s, event, NULL, NULL, &io, IOCTL_AFD_BIND,
                    params, params4_size, &addr, sizeof(addr));
            todo_wine ok(ret == STATUS_PENDING, "got %#x\n", ret);
            ret = WaitForSingleObject(event, 0);
            ok(!ret, "got %#x\n", ret);
            ok(!io.Status, "got %#lx\n", io.Status);
            ok(io.Information == sizeof(addr), "got %#Ix\n", io.Information);
            ok(addr.sin_family == AF_INET, "got family %u\n", addr.sin_family);
            ok(addr.sin_addr.s_addr == in_addr, "expected address %#08lx, got %#08lx\n", in_addr, addr.sin_addr.s_addr);
            ok(addr.sin_port, "expected nonzero port\n");

            memset(&addr2, 0xcc, sizeof(addr2));
            ret = NtDeviceIoControlFile((HANDLE)s, event, NULL, NULL, &io,
                    IOCTL_AFD_GETSOCKNAME, NULL, 0, &addr2, sizeof(addr2));
            ok(!ret, "got %#x\n", ret);
            ok(!memcmp(&addr, &addr2, sizeof(addr)), "addresses didn't match\n");

            closesocket(s);
        }
    }

    /* test IPv6 */

    s = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    ok(s != -1, "failed to create IPv6 socket\n");

    params->addr.sa_family = 0xdead;
    ret = NtDeviceIoControlFile((HANDLE)s, event, NULL, NULL, &io, IOCTL_AFD_BIND,
            params, params6_size, &addr6, sizeof(addr6));
    ok(ret == STATUS_INVALID_ADDRESS, "got %#x\n", ret);

    memcpy(&params->addr, &bind_addr6, sizeof(bind_addr6));
    ret = NtDeviceIoControlFile((HANDLE)s, event, NULL, NULL, &io, IOCTL_AFD_BIND,
            params, params6_size - 1, &addr6, sizeof(addr6));
    ok(ret == STATUS_INVALID_ADDRESS, "got %#x\n", ret);

    memcpy(&params->addr, &bind_addr6, sizeof(bind_addr6));
    ret = NtDeviceIoControlFile((HANDLE)s, event, NULL, NULL, &io, IOCTL_AFD_BIND,
            params, offsetof(struct afd_bind_params, addr) + sizeof(struct sockaddr_in6_old), &addr6, sizeof(addr6));
    ok(ret == STATUS_INVALID_ADDRESS, "got %#x\n", ret);

    memcpy(&params->addr, &bind_addr6, sizeof(bind_addr6));
    ret = NtDeviceIoControlFile((HANDLE)s, event, NULL, NULL, &io, IOCTL_AFD_BIND,
            params, offsetof(struct afd_bind_params, addr.sa_data), &addr6, sizeof(addr6));
    ok(ret == STATUS_INVALID_ADDRESS, "got %#x\n", ret);

    memcpy(&params->addr, &bind_addr6, sizeof(bind_addr6));
    ret = NtDeviceIoControlFile((HANDLE)s, event, NULL, NULL, &io, IOCTL_AFD_BIND,
            params, offsetof(struct afd_bind_params, addr.sa_data) - 1, &addr6, sizeof(addr6));
    ok(ret == STATUS_INVALID_PARAMETER, "got %#x\n", ret);

    memcpy(&params->addr, &invalid_addr6, sizeof(invalid_addr6));
    ret = NtDeviceIoControlFile((HANDLE)s, event, NULL, NULL, &io, IOCTL_AFD_BIND,
            params, params6_size, &addr6, sizeof(addr6));
    todo_wine ok(ret == STATUS_PENDING, "got %#x\n", ret);
    ret = WaitForSingleObject(event, 0);
    ok(!ret, "got %#x\n", ret);
    ok(io.Status == STATUS_INVALID_ADDRESS_COMPONENT, "got %#lx\n", io.Status);

    memcpy(&params->addr, &bind_addr6, sizeof(bind_addr6));
    ret = NtDeviceIoControlFile((HANDLE)s, event, NULL, NULL, &io, IOCTL_AFD_BIND,
            params, params6_size, &addr6, sizeof(addr6) - 1);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#x\n", ret);

    memcpy(&params->addr, &bind_addr6, sizeof(bind_addr6));
    ret = NtDeviceIoControlFile((HANDLE)s, event, NULL, NULL, &io, IOCTL_AFD_BIND,
            params, params6_size - 1, &addr6, sizeof(addr6) - 1);
    ok(ret == STATUS_INVALID_ADDRESS, "got %#x\n", ret);

    memcpy(&params->addr, &bind_addr6, sizeof(bind_addr6));
    ret = NtDeviceIoControlFile((HANDLE)s, event, NULL, NULL, &io, IOCTL_AFD_BIND,
            params, params6_size, &addr6, sizeof(struct sockaddr_in6_old));
    ok(ret == STATUS_INVALID_PARAMETER, "got %#x\n", ret);

    memcpy(&params->addr, &bind_addr6, sizeof(bind_addr6));
    memset(&io, 0xcc, sizeof(io));
    memset(&addr6, 0xcc, sizeof(addr6));
    ret = NtDeviceIoControlFile((HANDLE)s, event, NULL, NULL, &io, IOCTL_AFD_BIND,
            params, params6_size, &addr6, sizeof(addr6));
    todo_wine ok(ret == STATUS_PENDING, "got %#x\n", ret);
    ret = WaitForSingleObject(event, 0);
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got %#lx\n", io.Status);
    ok(io.Information == sizeof(addr6), "got %#Ix\n", io.Information);
    ok(addr6.sin6_family == AF_INET6, "got family %u\n", addr6.sin6_family);
    ok(!memcmp(&addr6.sin6_addr, &bind_addr6.sin6_addr, sizeof(addr6.sin6_addr)), "address didn't match\n");
    ok(!addr6.sin6_flowinfo, "got flow info %#lx\n", addr6.sin6_flowinfo);
    ok(addr6.sin6_port, "expected nonzero port\n");

    /* getsockname() returns EINVAL here. Possibly the socket name is cached (in shared memory?) */
    memset(&addr6_2, 0xcc, sizeof(addr6_2));
    ret = NtDeviceIoControlFile((HANDLE)s, event, NULL, NULL, &io,
            IOCTL_AFD_GETSOCKNAME, NULL, 0, &addr6_2, sizeof(addr6_2));
    ok(!ret, "got %#x\n", ret);
    ok(!memcmp(&addr6, &addr6_2, sizeof(addr6)), "addresses didn't match\n");

    ret = NtDeviceIoControlFile((HANDLE)s, event, NULL, NULL, &io, IOCTL_AFD_BIND,
            params, params6_size, &addr6, sizeof(addr6));
    ok(ret == STATUS_ADDRESS_ALREADY_ASSOCIATED, "got %#x\n", ret);

    s2 = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);

    memcpy(&params->addr, &addr6_2, sizeof(addr6_2));
    memset(&io, 0xcc, sizeof(io));
    memset(&addr6, 0xcc, sizeof(addr6));
    ret = NtDeviceIoControlFile((HANDLE)s2, event, NULL, NULL, &io, IOCTL_AFD_BIND,
            params, params6_size, &addr6, sizeof(addr6));
    todo_wine ok(ret == STATUS_PENDING, "got %#x\n", ret);
    ret = WaitForSingleObject(event, 0);
    ok(!ret, "got %#x\n", ret);
    ok(io.Status == STATUS_SHARING_VIOLATION, "got %#lx\n", io.Status);
    ok(!io.Information, "got %#Ix\n", io.Information);

    closesocket(s2);
    closesocket(s);

    CloseHandle(event);
    free(params);
}

static void test_getsockname(void)
{
    const struct sockaddr_in bind_addr = {.sin_family = AF_INET, .sin_addr.s_addr = htonl(INADDR_LOOPBACK)};
    struct sockaddr addr, addr2;
    SOCKET server, client;
    IO_STATUS_BLOCK io;
    HANDLE event;
    int ret, len;

    event = CreateEventW(NULL, TRUE, FALSE, NULL);

    tcp_socketpair(&client, &server);

    memset(&addr, 0xcc, sizeof(addr));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_GETSOCKNAME, NULL, 0, &addr, sizeof(addr) - 1);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %#x\n", ret);

    memset(&io, 0xcc, sizeof(io));
    memset(&addr, 0xcc, sizeof(addr));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_GETSOCKNAME, NULL, 0, &addr, sizeof(addr));
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got %#lx\n", io.Status);
    ok(io.Information == sizeof(addr), "got %#Ix\n", io.Information);
    len = sizeof(addr2);
    ret = getsockname(client, (struct sockaddr *)&addr2, &len);
    ok(!ret, "got error %u\n", WSAGetLastError());
    ok(!memcmp(&addr, &addr2, sizeof(struct sockaddr)), "addresses didn't match\n");

    memset(&io, 0xcc, sizeof(io));
    memset(&addr, 0xcc, sizeof(addr));
    ret = NtDeviceIoControlFile((HANDLE)server, event, NULL, NULL, &io,
            IOCTL_AFD_GETSOCKNAME, NULL, 0, &addr, sizeof(addr));
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got %#lx\n", io.Status);
    ok(io.Information == sizeof(addr), "got %#Ix\n", io.Information);
    len = sizeof(addr2);
    ret = getsockname(server, (struct sockaddr *)&addr2, &len);
    ok(!ret, "got error %u\n", WSAGetLastError());
    ok(!memcmp(&addr, &addr2, sizeof(struct sockaddr)), "addresses didn't match\n");

    closesocket(server);
    closesocket(client);

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_GETSOCKNAME, NULL, 0, &addr, sizeof(addr));
    ok(ret == STATUS_INVALID_PARAMETER, "got %#x\n", ret);

    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_GETSOCKNAME, NULL, 0, &addr, sizeof(addr) - 1);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#x\n", ret);

    ret = bind(client, (const struct sockaddr *)&bind_addr, sizeof(bind_addr));
    ok(!ret, "got error %u\n", WSAGetLastError());

    memset(&io, 0xcc, sizeof(io));
    memset(&addr, 0xcc, sizeof(addr));
    ret = NtDeviceIoControlFile((HANDLE)client, event, NULL, NULL, &io,
            IOCTL_AFD_GETSOCKNAME, NULL, 0, &addr, sizeof(addr));
    ok(!ret, "got %#x\n", ret);
    ok(!io.Status, "got %#lx\n", io.Status);
    ok(io.Information == sizeof(addr), "got %#Ix\n", io.Information);
    len = sizeof(addr2);
    ret = getsockname(client, (struct sockaddr *)&addr2, &len);
    ok(!ret, "got error %u\n", WSAGetLastError());
    ok(!memcmp(&addr, &addr2, sizeof(struct sockaddr)), "addresses didn't match\n");

    closesocket(client);

    CloseHandle(event);
}

START_TEST(afd)
{
    WSADATA data;

    WSAStartup(MAKEWORD(2, 2), &data);

    test_open_device();
    test_poll();
    test_poll_exclusive();
    test_poll_completion_port();
    test_poll_reset();
    test_recv();
    test_event_select();
    test_get_events();
    test_get_events_reset();
    test_bind();
    test_getsockname();

    WSACleanup();
}
