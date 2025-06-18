/*
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

/* We have to return the HTTP_REQUEST structure to userspace exactly as it will
 * be consumed; httpapi has no opportunity to massage it. Since it contains
 * pointers, this is somewhat nontrivial. */

struct http_unknown_header
{
    USHORT NameLength;
    USHORT RawValueLength;
    POINTER pName; /* char string */
    POINTER pRawValue; /* char string */
};

struct http_data_chunk
{
    HTTP_DATA_CHUNK_TYPE DataChunkType;
    union
    {
        struct
        {
            POINTER pBuffer; /* char string */
            ULONG BufferLength;
        } FromMemory;
        /* for the struct size */
        struct
        {
            ULARGE_INTEGER StartingOffset;
            ULARGE_INTEGER Length;
            POINTER FileHandle;
        } FromFileHandle;
    };
};

struct http_request
{
    ULONG Flags;
    HTTP_CONNECTION_ID ConnectionId;
    HTTP_REQUEST_ID RequestId;
    HTTP_URL_CONTEXT UrlContext;
    HTTP_VERSION Version;
    HTTP_VERB Verb;
    USHORT UnknownVerbLength;
    USHORT RawUrlLength;
    POINTER pUnknownVerb; /* char string */
    POINTER pRawUrl; /* char string */
    struct
    {
        USHORT FullUrlLength;
        USHORT HostLength;
        USHORT AbsPathLength;
        USHORT QueryStringLength;
        POINTER pFullUrl; /* WCHAR string */
        POINTER pHost; /* pointer to above */
        POINTER pAbsPath; /* pointer to above */
        POINTER pQueryString; /* pointer to above */
    } CookedUrl;
    struct
    {
        POINTER pRemoteAddress; /* SOCKADDR */
        POINTER pLocalAddress; /* SOCKADDR */
    } Address;
    struct
    {
        USHORT UnknownHeaderCount;
        POINTER pUnknownHeaders; /* struct http_unknown_header */
        USHORT TrailerCount;
        POINTER pTrailers; /* NULL */
        struct
        {
            USHORT RawValueLength;
            POINTER pRawValue; /* char string */
        } KnownHeaders[HttpHeaderRequestMaximum];
    } Headers;
    ULONGLONG BytesReceived;
    USHORT EntityChunkCount;
    POINTER pEntityChunks; /* struct http_data_chunk */
    HTTP_RAW_CONNECTION_ID RawConnectionId;
    POINTER pSslInfo; /* NULL (FIXME) */
    USHORT RequestInfoCount;
    POINTER pRequestInfo; /* NULL (FIXME) */
};

static NTSTATUS complete_irp(struct connection *conn, IRP *irp)
{
    const struct http_receive_request_params params
            = *(struct http_receive_request_params *)irp->AssociatedIrp.SystemBuffer;
    ULONG cooked_len, host_len, abs_path_len, query_len, chunk_len = 0, offset, processed;
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    const DWORD output_len = stack->Parameters.DeviceIoControl.OutputBufferLength;
    struct http_request *req = irp->AssociatedIrp.SystemBuffer;
    const char *p, *name, *value, *host, *abs_path, *query;
    struct http_unknown_header *unk_headers = NULL;
    char *buffer = irp->AssociatedIrp.SystemBuffer;
    DWORD irp_size = sizeof(struct http_request);
    USHORT unk_headers_count = 0, unk_header_idx;
    struct http_data_chunk *chunk = NULL;
    int name_len, value_len, len;
    struct sockaddr_in addr;

    /* First calculate the total buffer size needed for this IRP. */

    if (conn->unk_verb_len)
        irp_size += conn->unk_verb_len + 1;
    irp_size += conn->url_len + 1;

    /* cooked URL */
    if (conn->url[0] == '/')
    {
        p = host = conn->host;
        while (isgraph(*p)) ++p;
        host_len = p - conn->host;
        abs_path = conn->url;
        abs_path_len = conn->url_len;
    }
    else
    {
        host = conn->url + 7;
        abs_path = strchr(host, '/');
        host_len = abs_path - host;
        abs_path_len = (conn->url + conn->url_len) - abs_path;
    }
    if ((query = memchr(abs_path, '?', abs_path_len)))
    {
        query_len = (abs_path + abs_path_len) - query;
        abs_path_len = query - abs_path;
    }
    else
        query_len = 0;
    cooked_len = (7 /* scheme */ + host_len + abs_path_len + query_len) * sizeof(WCHAR);
    irp_size += cooked_len + sizeof(WCHAR);

    /* addresses */
    irp_size += 2 * sizeof(addr);

    /* headers */
    p = strstr(conn->buffer, "\r\n") + 2;
    while (memcmp(p, "\r\n", 2))
    {
        name = p;
        parse_header(name, &name_len, &value, &value_len);
        if (parse_header_name(name, name_len) == HttpHeaderRequestMaximum)
        {
            irp_size += name_len + 1;
            ++unk_headers_count;
        }
        irp_size += value_len + 1;
        p = strstr(p, "\r\n") + 2;
    }
    p += 2;

    irp_size += unk_headers_count * sizeof(struct http_unknown_header);

    TRACE("Need %lu bytes, have %lu.\n", irp_size, output_len);
    irp->IoStatus.Information = irp_size;

    memset(irp->AssociatedIrp.SystemBuffer, 0, output_len);

    if (output_len < irp_size)
    {
        req->ConnectionId = (ULONG_PTR)conn;
        req->RequestId = conn->req_id;
        return STATUS_BUFFER_OVERFLOW;
    }

    offset = sizeof(*req);

    req->UrlContext = conn->context;
    req->ConnectionId = (ULONG_PTR)conn;
    req->RequestId = conn->req_id;
    req->Version = conn->version;
    req->Verb = conn->verb;
    req->UnknownVerbLength = conn->unk_verb_len;
    req->RawUrlLength = conn->url_len;

    if (conn->unk_verb_len)
    {
        req->pUnknownVerb = params.addr + offset;
        memcpy(buffer + offset, conn->buffer, conn->unk_verb_len);
        offset += conn->unk_verb_len;
        buffer[offset++] = 0;
    }

    req->pRawUrl = params.addr + offset;
    memcpy(buffer + offset, conn->url, conn->url_len);
    offset += conn->url_len;
    buffer[offset++] = 0;

    req->CookedUrl.FullUrlLength = cooked_len;
    req->CookedUrl.HostLength = host_len * sizeof(WCHAR);
    req->CookedUrl.AbsPathLength = abs_path_len * sizeof(WCHAR);
    req->CookedUrl.QueryStringLength = query_len * sizeof(WCHAR);
    req->CookedUrl.pFullUrl = params.addr + offset;
    req->CookedUrl.pHost = req->CookedUrl.pFullUrl + 7 * sizeof(WCHAR);
    req->CookedUrl.pAbsPath = req->CookedUrl.pHost + host_len * sizeof(WCHAR);
    if (query)
        req->CookedUrl.pQueryString = req->CookedUrl.pAbsPath + abs_path_len * sizeof(WCHAR);

    memcpy(buffer + offset, L"http://", sizeof(L"http://"));
    offset += 7 * sizeof(WCHAR);
    MultiByteToWideChar(CP_ACP, 0, host, host_len, (WCHAR *)(buffer + offset), host_len * sizeof(WCHAR));
    offset += host_len * sizeof(WCHAR);
    MultiByteToWideChar(CP_ACP, 0, abs_path, abs_path_len + query_len,
            (WCHAR *)(buffer + offset), (abs_path_len + query_len) * sizeof(WCHAR));
    offset += (abs_path_len + query_len) * sizeof(WCHAR);
    buffer[offset++] = 0;
    buffer[offset++] = 0;

    req->Address.pRemoteAddress = params.addr + offset;
    len = sizeof(addr);
    getpeername(conn->socket, (struct sockaddr *)&addr, &len);
    memcpy(buffer + offset, &addr, sizeof(addr));
    offset += sizeof(addr);

    req->Address.pLocalAddress = params.addr + offset;
    len = sizeof(addr);
    getsockname(conn->socket, (struct sockaddr *)&addr, &len);
    memcpy(buffer + offset, &addr, sizeof(addr));
    offset += sizeof(addr);

    req->Headers.UnknownHeaderCount = unk_headers_count;
    if (unk_headers_count)
    {
        req->Headers.pUnknownHeaders = params.addr + offset;
        unk_headers = (struct http_unknown_header *)(buffer + offset);
        offset += unk_headers_count * sizeof(*unk_headers);
    }

    unk_header_idx = 0;
    p = strstr(conn->buffer, "\r\n") + 2;
    while (memcmp(p, "\r\n", 2))
    {
        HTTP_HEADER_ID id;

        name = p;
        parse_header(name, &name_len, &value, &value_len);
        if ((id = parse_header_name(name, name_len)) == HttpHeaderRequestMaximum)
        {
            unk_headers[unk_header_idx].NameLength = name_len;
            unk_headers[unk_header_idx].RawValueLength = value_len;
            unk_headers[unk_header_idx].pName = params.addr + offset;
            memcpy(buffer + offset, name, name_len);
            offset += name_len;
            buffer[offset++] = 0;
            unk_headers[unk_header_idx].pRawValue = params.addr + offset;
            memcpy(buffer + offset, value, value_len);
            offset += value_len;
            buffer[offset++] = 0;
            ++unk_header_idx;
        }
        else
        {
            req->Headers.KnownHeaders[id].RawValueLength = value_len;
            req->Headers.KnownHeaders[id].pRawValue = params.addr + offset;
            memcpy(buffer + offset, value, value_len);
            offset += value_len;
            buffer[offset++] = 0;
        }
        p = strstr(p, "\r\n") + 2;
    }
    p += 2;

    if (irp_size + sizeof(*chunk) < output_len && (params.flags & HTTP_RECEIVE_REQUEST_FLAG_COPY_BODY))
        chunk_len = min(conn->content_len, output_len - (irp_size + sizeof(*chunk)));
    if (chunk_len)
    {
        req->EntityChunkCount = 1;
        req->pEntityChunks = params.addr + offset;
        chunk = (struct http_data_chunk *)(buffer + offset);
        offset += sizeof(*chunk);
        chunk->DataChunkType = HttpDataChunkFromMemory;
        chunk->FromMemory.BufferLength = chunk_len;
        chunk->FromMemory.pBuffer = params.addr + offset;
        memcpy(buffer + offset, p, chunk_len);
        offset += chunk_len;

        irp->IoStatus.Information = irp_size + sizeof(*chunk) + chunk_len;
    }

    if (chunk_len < conn->content_len)
        req->Flags |= HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS;

    req->BytesReceived = conn->req_len;

    assert(offset == irp->IoStatus.Information);

    conn->available = FALSE;
    processed = conn->req_len - (conn->content_len - chunk_len);
    memmove(conn->buffer, conn->buffer + processed, conn->len - processed);
    conn->content_len -= chunk_len;
    conn->len -= processed;

    return STATUS_SUCCESS;
}
