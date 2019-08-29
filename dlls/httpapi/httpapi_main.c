/*
 * HTTPAPI implementation
 *
 * Copyright 2009 Austin English
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

#include "wine/http.h"
#include "winternl.h"
#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(httpapi);

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID lpv )
{
    switch(reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hinst );
        break;
    }
    return TRUE;
}

/***********************************************************************
 *        HttpInitialize       (HTTPAPI.@)
 *
 * Initializes HTTP Server API engine
 *
 * PARAMS
 *   version  [ I] HTTP API version which caller will use
 *   flags    [ I] initialization options which specify parts of API what will be used
 *   reserved [IO] reserved, must be NULL
 *
 * RETURNS
 *   NO_ERROR if function succeeds, or error code if function fails
 *
 */
ULONG WINAPI HttpInitialize( HTTPAPI_VERSION version, ULONG flags, PVOID reserved )
{
    FIXME( "({%d,%d}, 0x%x, %p): stub!\n", version.HttpApiMajorVersion,
           version.HttpApiMinorVersion, flags, reserved );
    return NO_ERROR;
}

/***********************************************************************
 *        HttpTerminate       (HTTPAPI.@)
 *
 * Cleans up HTTP Server API engine resources allocated by HttpInitialize
 *
 * PARAMS
 *   flags    [ I] options which specify parts of API what should be released
 *   reserved [IO] reserved, must be NULL
 *
 * RETURNS
 *   NO_ERROR if function succeeds, or error code if function fails
 *
 */
ULONG WINAPI HttpTerminate( ULONG flags, PVOID reserved )
{
    FIXME( "(0x%x, %p): stub!\n", flags, reserved );
    return NO_ERROR;
}

/***********************************************************************
 *        HttpDeleteServiceConfiguration     (HTTPAPI.@)
 *
 * Remove configuration record from HTTP Server API configuration store
 *
 * PARAMS
 *   handle     [I] reserved, must be 0
 *   type       [I] configuration record type
 *   config     [I] buffer which contains configuration record information
 *   length     [I] length of configuration record buffer
 *   overlapped [I] reserved, must be NULL
 *
 * RETURNS
 *   NO_ERROR if function succeeds, or error code if function fails
 *
 */
ULONG WINAPI HttpDeleteServiceConfiguration( HANDLE handle, HTTP_SERVICE_CONFIG_ID type,
                 PVOID config, ULONG length, LPOVERLAPPED overlapped )
{
    FIXME( "(%p, %d, %p, %d, %p): stub!\n", handle, type, config, length, overlapped );
    return NO_ERROR;
}

/***********************************************************************
 *        HttpQueryServiceConfiguration     (HTTPAPI.@)
 *
 * Retrieves configuration records from HTTP Server API configuration store
 *
 * PARAMS
 *   handle     [ I] reserved, must be 0
 *   type       [ I] configuration records type
 *   query      [ I] buffer which contains query data used to retrieve records
 *   query_len  [ I] length of query buffer
 *   buffer     [IO] buffer to store query results
 *   buffer_len [ I] length of output buffer
 *   data_len   [ O] optional pointer to a buffer which receives query result length
 *   overlapped [ I] reserved, must be NULL
 *
 * RETURNS
 *   NO_ERROR if function succeeds, or error code if function fails
 *
 */
ULONG WINAPI HttpQueryServiceConfiguration( HANDLE handle, HTTP_SERVICE_CONFIG_ID type,
                 PVOID query, ULONG query_len, PVOID buffer, ULONG buffer_len,
                 PULONG data_len, LPOVERLAPPED overlapped )
{
    FIXME( "(%p, %d, %p, %d, %p, %d, %p, %p): stub!\n", handle, type, query, query_len,
            buffer, buffer_len, data_len, overlapped );
    return ERROR_FILE_NOT_FOUND;
}

/***********************************************************************
 *        HttpSetServiceConfiguration     (HTTPAPI.@)
 *
 * Add configuration record to HTTP Server API configuration store
 *
 * PARAMS
 *   handle     [I] reserved, must be 0
 *   type       [I] configuration record type
 *   config     [I] buffer which contains configuration record information
 *   length     [I] length of configuration record buffer
 *   overlapped [I] reserved, must be NULL
 *
 * RETURNS
 *   NO_ERROR if function succeeds, or error code if function fails
 *
 */
ULONG WINAPI HttpSetServiceConfiguration( HANDLE handle, HTTP_SERVICE_CONFIG_ID type,
                 PVOID config, ULONG length, LPOVERLAPPED overlapped )
{
    FIXME( "(%p, %d, %p, %d, %p): stub!\n", handle, type, config, length, overlapped );
    return NO_ERROR;
}

/***********************************************************************
 *        HttpCreateHttpHandle     (HTTPAPI.@)
 *
 * Creates a handle to the HTTP request queue
 *
 * PARAMS
 *   handle     [O] handle to request queue
 *   reserved   [I] reserved, must be NULL
 *
 * RETURNS
 *   NO_ERROR if function succeeds, or error code if function fails
 *
 */
ULONG WINAPI HttpCreateHttpHandle(HANDLE *handle, ULONG reserved)
{
    static const WCHAR device_nameW[] = {'\\','D','e','v','i','c','e','\\','H','t','t','p','\\','R','e','q','Q','u','e','u','e',0};
    OBJECT_ATTRIBUTES attr = {sizeof(attr)};
    UNICODE_STRING string;
    IO_STATUS_BLOCK iosb;

    TRACE("handle %p, reserved %#x.\n", handle, reserved);

    if (!handle)
        return ERROR_INVALID_PARAMETER;

    RtlInitUnicodeString(&string, device_nameW);
    attr.ObjectName = &string;
    return RtlNtStatusToDosError(NtCreateFile(handle, 0, &attr, &iosb, NULL,
            FILE_ATTRIBUTE_NORMAL, 0, FILE_OPEN, FILE_NON_DIRECTORY_FILE, NULL, 0));
}

static ULONG add_url(HANDLE queue, const WCHAR *urlW, HTTP_URL_CONTEXT context)
{
    struct http_add_url_params *params;
    ULONG ret = ERROR_SUCCESS;
    OVERLAPPED ovl;
    int len;

    len = WideCharToMultiByte(CP_ACP, 0, urlW, -1, NULL, 0, NULL, NULL);
    if (!(params = heap_alloc(offsetof(struct http_add_url_params, url[len]))))
        return ERROR_OUTOFMEMORY;
    WideCharToMultiByte(CP_ACP, 0, urlW, -1, params->url, len, NULL, NULL);
    params->context = context;

    ovl.hEvent = (HANDLE)((ULONG_PTR)CreateEventW(NULL, TRUE, FALSE, NULL) | 1);

    if (!DeviceIoControl(queue, IOCTL_HTTP_ADD_URL, params,
            offsetof(struct http_add_url_params, url[len]), NULL, 0, NULL, &ovl))
        ret = GetLastError();
    CloseHandle(ovl.hEvent);
    return ret;
}

/***********************************************************************
 *        HttpAddUrl     (HTTPAPI.@)
 */
ULONG WINAPI HttpAddUrl(HANDLE queue, const WCHAR *url, void *reserved)
{
    TRACE("queue %p, url %s, reserved %p.\n", queue, debugstr_w(url), reserved);

    return add_url(queue, url, 0);
}

/***********************************************************************
 *        HttpRemoveUrl     (HTTPAPI.@)
 */
ULONG WINAPI HttpRemoveUrl(HANDLE queue, const WCHAR *urlW)
{
    ULONG ret = ERROR_SUCCESS;
    OVERLAPPED ovl = {};
    char *url;
    int len;

    TRACE("queue %p, url %s.\n", queue, debugstr_w(urlW));

    if (!queue)
        return ERROR_INVALID_PARAMETER;

    len = WideCharToMultiByte(CP_ACP, 0, urlW, -1, NULL, 0, NULL, NULL);
    if (!(url = heap_alloc(len)))
        return ERROR_OUTOFMEMORY;
    WideCharToMultiByte(CP_ACP, 0, urlW, -1, url, len, NULL, NULL);

    ovl.hEvent = (HANDLE)((ULONG_PTR)CreateEventW(NULL, TRUE, FALSE, NULL) | 1);

    if (!DeviceIoControl(queue, IOCTL_HTTP_REMOVE_URL, url, len, NULL, 0, NULL, &ovl))
        ret = GetLastError();
    CloseHandle(ovl.hEvent);
    heap_free(url);
    return ret;
}

/***********************************************************************
 *        HttpReceiveHttpRequest     (HTTPAPI.@)
 */
ULONG WINAPI HttpReceiveHttpRequest(HANDLE queue, HTTP_REQUEST_ID id, ULONG flags,
        HTTP_REQUEST *request, ULONG size, ULONG *ret_size, OVERLAPPED *ovl)
{
    struct http_receive_request_params params =
    {
        .addr = (ULONG_PTR)request,
        .id = id,
        .flags = flags,
        .bits = sizeof(void *) * 8,
    };
    ULONG ret = ERROR_SUCCESS;
    OVERLAPPED sync_ovl;

    TRACE("queue %p, id %s, flags %#x, request %p, size %#x, ret_size %p, ovl %p.\n",
            queue, wine_dbgstr_longlong(id), flags, request, size, ret_size, ovl);

    if (flags & ~HTTP_RECEIVE_REQUEST_FLAG_COPY_BODY)
        FIXME("Ignoring flags %#x.\n", flags & ~HTTP_RECEIVE_REQUEST_FLAG_COPY_BODY);

    if (size < sizeof(HTTP_REQUEST_V1))
        return ERROR_INSUFFICIENT_BUFFER;

    if (!ovl)
    {
        sync_ovl.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
        ovl = &sync_ovl;
    }

    if (!DeviceIoControl(queue, IOCTL_HTTP_RECEIVE_REQUEST, &params, sizeof(params), request, size, NULL, ovl))
        ret = GetLastError();

    if (ovl == &sync_ovl)
    {
        ret = ERROR_SUCCESS;
        if (!GetOverlappedResult(queue, ovl, ret_size, TRUE))
            ret = GetLastError();
        CloseHandle(sync_ovl.hEvent);
    }

    return ret;
}

static void format_date(char *buffer)
{
    static const char day_names[7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static const char month_names[12][4] =
            {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    SYSTEMTIME date;
    GetSystemTime(&date);
    sprintf(buffer + strlen(buffer), "Date: %s, %02u %s %u %02u:%02u:%02u GMT\r\n",
            day_names[date.wDayOfWeek], date.wDay, month_names[date.wMonth - 1],
            date.wYear, date.wHour, date.wMinute, date.wSecond);
}

/***********************************************************************
 *        HttpSendHttpResponse     (HTTPAPI.@)
 */
ULONG WINAPI HttpSendHttpResponse(HANDLE queue, HTTP_REQUEST_ID id, ULONG flags,
        HTTP_RESPONSE *response, HTTP_CACHE_POLICY *cache_policy, ULONG *ret_size,
        void *reserved1, ULONG reserved2, OVERLAPPED *ovl, HTTP_LOG_DATA *log_data)
{
    static const char *const header_names[] =
    {
        "Cache-Control",
        "Connection",
        "Date",
        "Keep-Alive",
        "Pragma",
        "Trailer",
        "Transfer-Encoding",
        "Upgrade",
        "Via",
        "Warning",
        "Allow",
        "Content-Length",
        "Content-Type",
        "Content-Encoding",
        "Content-Language",
        "Content-Location",
        "Content-MD5",
        "Content-Range",
        "Expires",
        "Last-Modified",
        "Accept-Ranges",
        "Age",
        "ETag",
        "Location",
        "Proxy-Authenticate",
        "Retry-After",
        "Server",
        "Set-Cookie",
        "Vary",
        "WWW-Authenticate",
    };

    struct http_response *buffer;
    OVERLAPPED dummy_ovl = {};
    ULONG ret = ERROR_SUCCESS;
    int len, body_len = 0;
    char *p, dummy[12];
    USHORT i;

    TRACE("queue %p, id %s, flags %#x, response %p, cache_policy %p, "
            "ret_size %p, reserved1 %p, reserved2 %#x, ovl %p, log_data %p.\n",
            queue, wine_dbgstr_longlong(id), flags, response, cache_policy,
            ret_size, reserved1, reserved2, ovl, log_data);

    if (flags)
        FIXME("Unhandled flags %#x.\n", flags);
    if (response->s.Flags)
        FIXME("Unhandled response flags %#x.\n", response->s.Flags);
    if (cache_policy)
        WARN("Ignoring cache_policy.\n");
    if (log_data)
        WARN("Ignoring log_data.\n");

    len = 12 + sprintf(dummy, "%hu", response->s.StatusCode) + response->s.ReasonLength;
    for (i = 0; i < response->s.EntityChunkCount; ++i)
    {
        if (response->s.pEntityChunks[i].DataChunkType != HttpDataChunkFromMemory)
        {
            FIXME("Unhandled data chunk type %u.\n", response->s.pEntityChunks[i].DataChunkType);
            return ERROR_CALL_NOT_IMPLEMENTED;
        }
        body_len += response->s.pEntityChunks[i].FromMemory.BufferLength;
    }
    len += body_len;
    for (i = 0; i < HttpHeaderResponseMaximum; ++i)
    {
        if (i == HttpHeaderDate)
            len += 37;
        else if (response->s.Headers.KnownHeaders[i].RawValueLength)
            len += strlen(header_names[i]) + 2 + response->s.Headers.KnownHeaders[i].RawValueLength + 2;
        else if (i == HttpHeaderContentLength)
        {
            char dummy[12];
            len += strlen(header_names[i]) + 2 + sprintf(dummy, "%d", body_len) + 2;
        }
    }
    for (i = 0; i < response->s.Headers.UnknownHeaderCount; ++i)
    {
        len += response->s.Headers.pUnknownHeaders[i].NameLength + 2;
        len += response->s.Headers.pUnknownHeaders[i].RawValueLength + 2;
    }
    len += 2;

    if (!(buffer = heap_alloc(offsetof(struct http_response, buffer[len]))))
        return ERROR_OUTOFMEMORY;
    buffer->id = id;
    buffer->len = len;
    sprintf(buffer->buffer, "HTTP/1.1 %u %.*s\r\n", response->s.StatusCode,
            response->s.ReasonLength, response->s.pReason);

    for (i = 0; i < HttpHeaderResponseMaximum; ++i)
    {
        const HTTP_KNOWN_HEADER *header = &response->s.Headers.KnownHeaders[i];
        if (i == HttpHeaderDate)
            format_date(buffer->buffer);
        else if (header->RawValueLength)
            sprintf(buffer->buffer + strlen(buffer->buffer), "%s: %.*s\r\n",
                    header_names[i], header->RawValueLength, header->pRawValue);
        else if (i == HttpHeaderContentLength)
            sprintf(buffer->buffer + strlen(buffer->buffer), "Content-Length: %d\r\n", body_len);
    }
    for (i = 0; i < response->s.Headers.UnknownHeaderCount; ++i)
    {
        const HTTP_UNKNOWN_HEADER *header = &response->s.Headers.pUnknownHeaders[i];
        sprintf(buffer->buffer + strlen(buffer->buffer), "%.*s: %.*s\r\n", header->NameLength,
                header->pName, header->RawValueLength, header->pRawValue);
    }
    p = buffer->buffer + strlen(buffer->buffer);
    /* Don't use strcat, because this might be the end of the buffer. */
    memcpy(p, "\r\n", 2);
    p += 2;
    for (i = 0; i < response->s.EntityChunkCount; ++i)
    {
        const HTTP_DATA_CHUNK *chunk = &response->s.pEntityChunks[i];
        memcpy(p, chunk->FromMemory.pBuffer, chunk->FromMemory.BufferLength);
        p += chunk->FromMemory.BufferLength;
    }

    if (!ovl)
        ovl = &dummy_ovl;

    if (!DeviceIoControl(queue, IOCTL_HTTP_SEND_RESPONSE, buffer,
            offsetof(struct http_response, buffer[len]), NULL, 0, NULL, ovl))
        ret = GetLastError();

    heap_free(buffer);
    return ret;
}

/***********************************************************************
 *        HttpCreateServerSession     (HTTPAPI.@)
 */
ULONG WINAPI HttpCreateServerSession( HTTPAPI_VERSION version, HTTP_SERVER_SESSION_ID *id, ULONG reserved )
{
    FIXME( "({%d,%d}, %p, %d): stub!\n", version.HttpApiMajorVersion, version.HttpApiMinorVersion, id, reserved );
    return ERROR_ACCESS_DENIED;
}

/***********************************************************************
 *        HttpCloseServerSession     (HTTPAPI.@)
 */
ULONG WINAPI HttpCloseServerSession( HTTP_SERVER_SESSION_ID id )
{
    FIXME( "(%s): stub!\n", wine_dbgstr_longlong(id));
    return ERROR_INVALID_PARAMETER;
}
