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

struct request_queue
{
    struct list entry;
    HTTP_URL_CONTEXT context;
    char *url;
};

static struct list request_queues = LIST_INIT(request_queues);


static NTSTATUS http_add_url(struct request_queue *queue, IRP *irp)
{
    const struct http_add_url_params *params = irp->AssociatedIrp.SystemBuffer;
    unsigned short port;
    char *url, *endptr;
    int count = 0;
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
    if (!(port = strtol(strchr(params->url + 7, ':') + 1, &endptr, 10)) || *endptr != '/')
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

    LIST_FOR_EACH_ENTRY_SAFE(queue, queue_next, &request_queues, struct request_queue, entry)
    {
        close_queue(queue);
    }

    IoDeleteDevice(device_obj);
    NtClose(directory_obj);
}

NTSTATUS WINAPI DriverEntry(DRIVER_OBJECT *driver, UNICODE_STRING *path)
{
    static const WCHAR device_nameW[] = {'\\','D','e','v','i','c','e','\\','H','t','t','p','\\','R','e','q','Q','u','e','u','e',0};
    static const WCHAR directory_nameW[] = {'\\','D','e','v','i','c','e','\\','H','t','t','p',0};
    OBJECT_ATTRIBUTES attr = {sizeof(attr)};
    UNICODE_STRING string;
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

    return STATUS_SUCCESS;
}
