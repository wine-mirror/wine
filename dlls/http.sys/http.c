/*
 * HTTP server driver
 *
 * Copyright 2019 Zebediah Figura
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "wine/http.h"
#include "winternl.h"
#include "ddk/wdm.h"
#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/list.h"

static HANDLE directory_obj;
static DEVICE_OBJECT *device_obj;

WINE_DEFAULT_DEBUG_CHANNEL(http);

#define DECLARE_CRITICAL_SECTION(cs) \
    static CRITICAL_SECTION cs; \
    static CRITICAL_SECTION_DEBUG cs##_debug = \
    { 0, 0, &cs, { &cs##_debug.ProcessLocksList, &cs##_debug.ProcessLocksList }, \
      0, 0, { (DWORD_PTR)(__FILE__ ": " # cs) }}; \
    static CRITICAL_SECTION cs = { &cs##_debug, -1, 0, 0, 0, 0 };

DECLARE_CRITICAL_SECTION(http_cs);

static HANDLE request_thread, request_event;
static BOOL thread_stop;

struct connection
{
    struct list entry; /* in "connections" below */

    int socket;

    char *buffer;
    unsigned int len, size;

    BOOL available;

    /* Things we already parsed out of the request header in parse_request().
     * These are valid only if "available" is TRUE. */
    unsigned int req_len;
    HTTP_VERB verb;
    HTTP_VERSION version;
    const char *url, *host;
    ULONG unk_verb_len, url_len, content_len;
};

static struct list connections = LIST_INIT(connections);

struct request_queue
{
    struct list entry;
    HTTP_URL_CONTEXT context;
    char *url;
    int socket;
};

static struct list request_queues = LIST_INIT(request_queues);

static void accept_connection(int socket)
{
    struct connection *conn;
    ULONG true = 1;
    int peer;

    if ((peer = accept(socket, NULL, NULL)) == -1)
        return;

    if (!(conn = heap_alloc_zero(sizeof(*conn))))
    {
        ERR("Failed to allocate memory.\n");
        shutdown(peer, SD_BOTH);
        closesocket(peer);
        return;
    }
    if (!(conn->buffer = heap_alloc(8192)))
    {
        ERR("Failed to allocate buffer memory.\n");
        heap_free(conn);
        shutdown(peer, SD_BOTH);
        closesocket(peer);
        return;
    }
    conn->size = 8192;
    WSAEventSelect(peer, request_event, FD_READ | FD_CLOSE);
    ioctlsocket(peer, FIONBIO, &true);
    conn->socket = peer;
    list_add_head(&connections, &conn->entry);
}

static void close_connection(struct connection *conn)
{
    heap_free(conn->buffer);
    shutdown(conn->socket, SD_BOTH);
    closesocket(conn->socket);
    list_remove(&conn->entry);
    heap_free(conn);
}

static HTTP_VERB parse_verb(const char *verb, int len)
{
    static const char *const verbs[] =
    {
        "OPTIONS",
        "GET",
        "HEAD",
        "POST",
        "PUT",
        "DELETE",
        "TRACE",
        "CONNECT",
        "TRACK",
        "MOVE",
        "COPY",
        "PROPFIND",
        "PROPPATCH",
        "MKCOL",
        "LOCK",
        "UNLOCK",
        "SEARCH",
    };
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(verbs); ++i)
    {
        if (!strncmp(verb, verbs[i], len))
            return HttpVerbOPTIONS + i;
    }
    return HttpVerbUnknown;
}

/* Return the length of a token, as defined in RFC 2616 section 2.2. */
static int parse_token(const char *str, const char *end)
{
    const char *p;
    for (p = str; !end || p < end; ++p)
    {
        if (!isgraph(*p) || strchr("()<>@,;:\\\"/[]?={}", *p))
            break;
    }
    return p - str;
}

/* Return 1 if str matches expect, 0 if str is incomplete, -1 if they don't match. */
static int compare_exact(const char *str, const char *expect, const char *end)
{
    while (*expect)
    {
        if (str >= end) return 0;
        if (*str++ != *expect++) return -1;
    }
    return 1;
}

static int parse_number(const char *str, const char **endptr, const char *end)
{
    int n = 0;
    while (str < end && isdigit(*str))
        n = n * 10 + (*str++ - '0');
    *endptr = str;
    return n;
}

/* Upon receiving a request, parse it to ensure that it is a valid HTTP request,
 * and mark down some information that we will use later. Returns 1 if we parsed
 * a complete request, 0 if incomplete, -1 if invalid. */
static int parse_request(struct connection *conn)
{
    const char *const req = conn->buffer, *const end = conn->buffer + conn->len;
    const char *p = req, *q;
    int len, ret;

    if (!conn->len) return 0;

    TRACE("%s\n", wine_dbgstr_an(conn->buffer, conn->len));

    len = parse_token(p, end);
    if (p + len >= end) return 0;
    if (!len || p[len] != ' ') return -1;

    /* verb */
    if ((conn->verb = parse_verb(p, len)) == HttpVerbUnknown)
        conn->unk_verb_len = len;
    p += len + 1;

    TRACE("Got verb %u (%s).\n", conn->verb, debugstr_an(req, len));

    /* URL */
    conn->url = p;
    while (p < end && isgraph(*p)) ++p;
    conn->url_len = p - conn->url;
    if (p >= end) return 0;
    if (!conn->url_len) return -1;

    TRACE("Got URI %s.\n", debugstr_an(conn->url, conn->url_len));

    /* version */
    if ((ret = compare_exact(p, " HTTP/", end)) <= 0) return ret;
    p += 6;
    conn->version.MajorVersion = parse_number(p, &q, end);
    if (q >= end) return 0;
    if (q == p || *q != '.') return -1;
    p = q + 1;
    if (p >= end) return 0;
    conn->version.MinorVersion = parse_number(p, &q, end);
    if (q >= end) return 0;
    if (q == p) return -1;
    p = q;
    if ((ret = compare_exact(p, "\r\n", end)) <= 0) return ret;
    p += 2;

    TRACE("Got version %hu.%hu.\n", conn->version.MajorVersion, conn->version.MinorVersion);

    /* headers */
    conn->host = NULL;
    conn->content_len = 0;
    for (;;)
    {
        const char *name = p;

        if (!(ret = compare_exact(p, "\r\n", end))) return 0;
        else if (ret > 0) break;

        len = parse_token(p, end);
        if (p + len >= end) return 0;
        if (!len) return -1;
        p += len;
        while (p < end && (*p == ' ' || *p == '\t')) ++p;
        if (p >= end) return 0;
        if (*p != ':') return -1;
        ++p;
        while (p < end && (*p == ' ' || *p == '\t')) ++p;

        TRACE("Got %s header.\n", debugstr_an(name, len));

        if (!strncmp(name, "Host", len))
            conn->host = p;
        else if (!strncmp(name, "Content-Length", len))
        {
            conn->content_len = parse_number(p, &q, end);
            if (q >= end) return 0;
            if (q == p) return -1;
        }
        else if (!strncmp(name, "Transfer-Encoding", len))
            FIXME("Unhandled Transfer-Encoding header.\n");
        while (p < end && (isprint(*p) || *p == '\t')) ++p;
        if ((ret = compare_exact(p, "\r\n", end)) <= 0) return ret;
        p += 2;
    }
    p += 2;
    if (conn->url[0] == '/' && !conn->host) return -1;

    if (end - p < conn->content_len) return 0;

    conn->req_len = (p - req) + conn->content_len;

    TRACE("Received a full request, length %u bytes.\n", conn->req_len);

    /* Stop selecting on incoming data until a response is queued. */
    WSAEventSelect(conn->socket, request_event, FD_CLOSE);

    conn->available = TRUE;

    return 1;
}

static void receive_data(struct connection *conn)
{
    int len, ret;

    /* We might be waiting for an IRP, but always call recv() anyway, since we
     * might have been woken up by the socket closing. */
    if ((len = recv(conn->socket, conn->buffer + conn->len, conn->size - conn->len, 0)) <= 0)
    {
        if (WSAGetLastError() == WSAEWOULDBLOCK)
            return; /* nothing to receive */
        else if (!len)
            TRACE("Connection was shut down by peer.\n");
        else
            ERR("Got error %u; shutting down connection.\n", WSAGetLastError());
        close_connection(conn);
        return;
    }
    conn->len += len;

    if (conn->available)
        return; /* waiting for an HttpReceiveHttpRequest() call */

    TRACE("Received %u bytes of data.\n", len);

    if (!(ret = parse_request(conn)))
    {
        ULONG available;
        ioctlsocket(conn->socket, FIONREAD, &available);
        if (available)
        {
            TRACE("%u more bytes of data available, trying with larger buffer.\n", available);
            if (!(conn->buffer = heap_realloc(conn->buffer, conn->len + available)))
            {
                ERR("Failed to allocate %u bytes of memory.\n", conn->len + available);
                close_connection(conn);
                return;
            }
            conn->size = conn->len + available;

            if ((len = recv(conn->socket, conn->buffer + conn->len, conn->size - conn->len, 0)) < 0)
            {
                ERR("Got error %u; shutting down connection.\n", WSAGetLastError());
                close_connection(conn);
                return;
            }
            TRACE("Received %u bytes of data.\n", len);
            conn->len += len;
            ret = parse_request(conn);
        }
    }
    if (!ret)
        TRACE("Request is incomplete, waiting for more data.\n");
    else if (ret < 0)
    {
        WARN("Failed to parse request; shutting down connection.\n");
        close_connection(conn);
    }
}

static DWORD WINAPI request_thread_proc(void *arg)
{
    struct connection *conn, *cursor;
    struct request_queue *queue;

    TRACE("Starting request thread.\n");

    while (!WaitForSingleObject(request_event, INFINITE))
    {
        EnterCriticalSection(&http_cs);

        LIST_FOR_EACH_ENTRY(queue, &request_queues, struct request_queue, entry)
        {
            if (queue->socket != -1)
                accept_connection(queue->socket);
        }

        LIST_FOR_EACH_ENTRY_SAFE(conn, cursor, &connections, struct connection, entry)
        {
            receive_data(conn);
        }

        LeaveCriticalSection(&http_cs);
    }

    TRACE("Stopping request thread.\n");

    return 0;
}

static NTSTATUS http_add_url(struct request_queue *queue, IRP *irp)
{
    const struct http_add_url_params *params = irp->AssociatedIrp.SystemBuffer;
    struct sockaddr_in addr;
    char *url, *endptr;
    int s, count = 0;
    ULONG true = 1;
    const char *p;

    TRACE("host %s, context %s.\n", debugstr_a(params->url), wine_dbgstr_longlong(params->context));

    if (!strncmp(params->url, "https://", 8))
    {
        FIXME("HTTPS is not implemented.\n");
        return STATUS_NOT_IMPLEMENTED;
    }
    else if (strncmp(params->url, "http://", 7) || !strchr(params->url + 7, ':')
            || params->url[strlen(params->url) - 1] != '/')
        return STATUS_INVALID_PARAMETER;
    if (!(addr.sin_port = htons(strtol(strchr(params->url + 7, ':') + 1, &endptr, 10))) || *endptr != '/')
        return STATUS_INVALID_PARAMETER;

    if (!(url = heap_alloc(strlen(params->url))))
        return STATUS_NO_MEMORY;
    strcpy(url, params->url);

    for (p = url; *p; ++p)
        if (*p == '/') ++count;
    if (count > 3)
        FIXME("Binding to relative URIs is not implemented; binding to all URIs instead.\n");

    EnterCriticalSection(&http_cs);

    if (queue->url && !strcmp(queue->url, url))
    {
        LeaveCriticalSection(&http_cs);
        heap_free(url);
        return STATUS_OBJECT_NAME_COLLISION;
    }
    else if (queue->url)
    {
        FIXME("Binding to multiple URLs is not implemented.\n");
        LeaveCriticalSection(&http_cs);
        heap_free(url);
        return STATUS_NOT_IMPLEMENTED;
    }

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        ERR("Failed to create socket, error %u.\n", WSAGetLastError());
        LeaveCriticalSection(&http_cs);
        heap_free(url);
        return STATUS_UNSUCCESSFUL;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.S_un.S_addr = INADDR_ANY;
    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        ERR("Failed to bind socket, error %u.\n", WSAGetLastError());
        LeaveCriticalSection(&http_cs);
        closesocket(s);
        heap_free(url);
        return STATUS_UNSUCCESSFUL;
    }

    if (listen(s, SOMAXCONN) == -1)
    {
        ERR("Failed to listen to port %u, error %u.\n", addr.sin_port, WSAGetLastError());
        LeaveCriticalSection(&http_cs);
        closesocket(s);
        heap_free(url);
        return STATUS_OBJECT_NAME_COLLISION;
    }

    ioctlsocket(s, FIONBIO, &true);
    WSAEventSelect(s, request_event, FD_ACCEPT);
    queue->socket = s;
    queue->url = url;
    queue->context = params->context;

    LeaveCriticalSection(&http_cs);

    return STATUS_SUCCESS;
}

static NTSTATUS http_remove_url(struct request_queue *queue, IRP *irp)
{
    const char *url = irp->AssociatedIrp.SystemBuffer;

    TRACE("host %s.\n", debugstr_a(url));

    EnterCriticalSection(&http_cs);

    if (!queue->url || strcmp(url, queue->url))
    {
        LeaveCriticalSection(&http_cs);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }
    heap_free(queue->url);
    queue->url = NULL;

    LeaveCriticalSection(&http_cs);
    return STATUS_SUCCESS;
}

static NTSTATUS WINAPI dispatch_ioctl(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    struct request_queue *queue = stack->FileObject->FsContext;
    NTSTATUS ret;

    switch (stack->Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_HTTP_ADD_URL:
        ret = http_add_url(queue, irp);
        break;
    case IOCTL_HTTP_REMOVE_URL:
        ret = http_remove_url(queue, irp);
        break;
    default:
        FIXME("Unhandled ioctl %#x.\n", stack->Parameters.DeviceIoControl.IoControlCode);
        ret = STATUS_NOT_IMPLEMENTED;
    }

    if (ret != STATUS_PENDING)
    {
        irp->IoStatus.Status = ret;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }
    else
        IoMarkIrpPending(irp);
    return ret;
}

static NTSTATUS WINAPI dispatch_create(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    struct request_queue *queue;

    if (!(queue = heap_alloc_zero(sizeof(*queue))))
        return STATUS_NO_MEMORY;
    stack->FileObject->FsContext = queue;

    EnterCriticalSection(&http_cs);
    list_add_head(&request_queues, &queue->entry);
    LeaveCriticalSection(&http_cs);

    TRACE("Created queue %p.\n", queue);

    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static void close_queue(struct request_queue *queue)
{
    EnterCriticalSection(&http_cs);
    list_remove(&queue->entry);
    if (queue->socket != -1)
    {
        shutdown(queue->socket, SD_BOTH);
        closesocket(queue->socket);
    }
    LeaveCriticalSection(&http_cs);

    heap_free(queue->url);
    heap_free(queue);
}

static NTSTATUS WINAPI dispatch_close(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    struct request_queue *queue = stack->FileObject->FsContext;

    TRACE("Closing queue %p.\n", queue);
    close_queue(queue);

    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static void WINAPI unload(DRIVER_OBJECT *driver)
{
    struct request_queue *queue, *queue_next;
    struct connection *conn, *conn_next;

    thread_stop = TRUE;
    SetEvent(request_event);
    WaitForSingleObject(request_thread, INFINITE);
    CloseHandle(request_thread);
    CloseHandle(request_event);

    LIST_FOR_EACH_ENTRY_SAFE(conn, conn_next, &connections, struct connection, entry)
    {
        close_connection(conn);
    }

    LIST_FOR_EACH_ENTRY_SAFE(queue, queue_next, &request_queues, struct request_queue, entry)
    {
        close_queue(queue);
    }

    WSACleanup();

    IoDeleteDevice(device_obj);
    NtClose(directory_obj);
}

NTSTATUS WINAPI DriverEntry(DRIVER_OBJECT *driver, UNICODE_STRING *path)
{
    static const WCHAR device_nameW[] = {'\\','D','e','v','i','c','e','\\','H','t','t','p','\\','R','e','q','Q','u','e','u','e',0};
    static const WCHAR directory_nameW[] = {'\\','D','e','v','i','c','e','\\','H','t','t','p',0};
    OBJECT_ATTRIBUTES attr = {sizeof(attr)};
    UNICODE_STRING string;
    WSADATA wsadata;
    NTSTATUS ret;

    TRACE("driver %p, path %s.\n", driver, debugstr_w(path->Buffer));

    RtlInitUnicodeString(&string, directory_nameW);
    attr.ObjectName = &string;
    if ((ret = NtCreateDirectoryObject(&directory_obj, 0, &attr)) && ret != STATUS_OBJECT_NAME_COLLISION)
        ERR("Failed to create \\Device\\Http directory, status %#x.\n", ret);

    RtlInitUnicodeString(&string, device_nameW);
    if ((ret = IoCreateDevice(driver, 0, &string, FILE_DEVICE_UNKNOWN, 0, FALSE, &device_obj)))
    {
        ERR("Failed to create request queue device, status %#x.\n", ret);
        NtClose(directory_obj);
        return ret;
    }

    driver->MajorFunction[IRP_MJ_CREATE] = dispatch_create;
    driver->MajorFunction[IRP_MJ_CLOSE] = dispatch_close;
    driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = dispatch_ioctl;
    driver->DriverUnload = unload;

    WSAStartup(MAKEWORD(1,1), &wsadata);

    request_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    request_thread = CreateThread(NULL, 0, request_thread_proc, NULL, 0, NULL);

    return STATUS_SUCCESS;
}
