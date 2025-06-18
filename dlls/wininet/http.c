/*
 * Wininet - HTTP Implementation
 *
 * Copyright 1999 Corel Corporation
 * Copyright 2002 CodeWeavers Inc.
 * Copyright 2002 TransGaming Technologies Inc.
 * Copyright 2004 Mike McCormack for CodeWeavers
 * Copyright 2005 Aric Stewart for CodeWeavers
 * Copyright 2006 Robert Shearman for CodeWeavers
 * Copyright 2011 Jacek Caban for CodeWeavers
 *
 * Ulrich Czekalla
 * David Hammerton
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

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <zlib.h>

#include "windef.h"
#include "winbase.h"
#include "wininet.h"
#include "winerror.h"
#include "winternl.h"
#include "winsock2.h"
#include "ws2ipdef.h"
#define NO_SHLWAPI_STREAM
#define NO_SHLWAPI_REG
#define NO_SHLWAPI_GDI
#include "shlwapi.h"
#include "sspi.h"
#include "wincrypt.h"
#include "winuser.h"

#include "internet.h"
#include "resource.h"
#include "wine/debug.h"
#include "wine/exception.h"

WINE_DEFAULT_DEBUG_CHANNEL(wininet);

#define HTTP_ADDHDR_FLAG_REQ				0x02000000

#define COLLECT_TIME 60000

struct HttpAuthInfo
{
    LPWSTR scheme;
    CredHandle cred;
    CtxtHandle ctx;
    TimeStamp exp;
    ULONG attr;
    ULONG max_token;
    void *auth_data;
    unsigned int auth_data_len;
    BOOL finished; /* finished authenticating */
};


typedef struct _basicAuthorizationData
{
    struct list entry;

    LPWSTR host;
    LPWSTR realm;
    LPSTR  authorization;
    UINT   authorizationLen;
} basicAuthorizationData;

typedef struct _authorizationData
{
    struct list entry;

    LPWSTR host;
    LPWSTR scheme;
    LPWSTR domain;
    UINT   domain_len;
    LPWSTR user;
    UINT   user_len;
    LPWSTR password;
    UINT   password_len;
} authorizationData;

static struct list basicAuthorizationCache = LIST_INIT(basicAuthorizationCache);
static struct list authorizationCache = LIST_INIT(authorizationCache);

static CRITICAL_SECTION authcache_cs;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &authcache_cs,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": authcache_cs") }
};
static CRITICAL_SECTION authcache_cs = { &critsect_debug, -1, 0, 0, 0, 0 };

static DWORD HTTP_GetResponseHeaders(http_request_t *req, INT *len);
static DWORD HTTP_ProcessHeader(http_request_t *req, LPCWSTR field, LPCWSTR value, DWORD dwModifier);
static LPWSTR * HTTP_InterpretHttpHeader(LPCWSTR buffer);
static DWORD HTTP_InsertCustomHeader(http_request_t *req, LPHTTPHEADERW lpHdr);
static INT HTTP_GetCustomHeaderIndex(http_request_t *req, LPCWSTR lpszField, INT index, BOOL Request);
static BOOL HTTP_DeleteCustomHeader(http_request_t *req, DWORD index);
static LPWSTR HTTP_build_req( LPCWSTR *list, int len );
static DWORD HTTP_HttpQueryInfoW(http_request_t*, DWORD, LPVOID, LPDWORD, LPDWORD);
static UINT HTTP_DecodeBase64(LPCWSTR base64, LPSTR bin);
static DWORD drain_content(http_request_t*,BOOL);

static CRITICAL_SECTION connection_pool_cs;
static CRITICAL_SECTION_DEBUG connection_pool_debug =
{
    0, 0, &connection_pool_cs,
    { &connection_pool_debug.ProcessLocksList, &connection_pool_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": connection_pool_cs") }
};
static CRITICAL_SECTION connection_pool_cs = { &connection_pool_debug, -1, 0, 0, 0, 0 };

static struct list connection_pool = LIST_INIT(connection_pool);
static BOOL collector_running;

void server_addref(server_t *server)
{
    InterlockedIncrement(&server->ref);
}

void server_release(server_t *server)
{
    if(InterlockedDecrement(&server->ref))
        return;

    list_remove(&server->entry);

    if(server->cert_chain)
        CertFreeCertificateChain(server->cert_chain);
    free(server->name);
    free(server->scheme_host_port);
    free(server);
}

static BOOL process_host_port(server_t *server)
{
    BOOL default_port;
    size_t name_len, len;
    WCHAR *buf;

    name_len = lstrlenW(server->name);
    len = name_len + 10 /* strlen("://:<port>") */ + ARRAY_SIZE(L"https");
    buf = malloc(len * sizeof(WCHAR));
    if(!buf)
        return FALSE;

    swprintf(buf, len, L"%s://%s:%u", server->is_https ? L"https" : L"http", server->name, server->port);
    server->scheme_host_port = buf;

    server->host_port = server->scheme_host_port + 7 /* strlen("http://") */;
    if(server->is_https)
        server->host_port++;

    default_port = server->port == (server->is_https ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT);
    server->canon_host_port = default_port ? server->name : server->host_port;
    return TRUE;
}

server_t *get_server(substr_t name, INTERNET_PORT port, BOOL is_https, BOOL do_create)
{
    server_t *iter, *server = NULL;

    EnterCriticalSection(&connection_pool_cs);

    LIST_FOR_EACH_ENTRY(iter, &connection_pool, server_t, entry) {
        if(iter->port == port && name.len == lstrlenW(iter->name) && !wcsnicmp(iter->name, name.str, name.len)
                && iter->is_https == is_https) {
            server = iter;
            server_addref(server);
            break;
        }
    }

    if(!server && do_create) {
        server = calloc(1, sizeof(*server));
        if(server) {
            server->ref = 2; /* list reference and return */
            server->port = port;
            server->is_https = is_https;
            list_init(&server->conn_pool);
            server->name = strndupW(name.str, name.len);
            if(server->name && process_host_port(server)) {
                list_add_head(&connection_pool, &server->entry);
            }else {
                free(server);
                server = NULL;
            }
        }
    }

    LeaveCriticalSection(&connection_pool_cs);

    return server;
}

BOOL collect_connections(collect_type_t collect_type)
{
    netconn_t *netconn, *netconn_safe;
    server_t *server, *server_safe;
    BOOL remaining = FALSE;
    DWORD64 now;

    now = GetTickCount64();

    LIST_FOR_EACH_ENTRY_SAFE(server, server_safe, &connection_pool, server_t, entry) {
        LIST_FOR_EACH_ENTRY_SAFE(netconn, netconn_safe, &server->conn_pool, netconn_t, pool_entry) {
            if(collect_type > COLLECT_TIMEOUT || netconn->keep_until < now) {
                TRACE("freeing %p\n", netconn);
                list_remove(&netconn->pool_entry);
                free_netconn(netconn);
            }else {
                remaining = TRUE;
            }
        }

        if(collect_type == COLLECT_CLEANUP) {
            list_remove(&server->entry);
            list_init(&server->entry);
            server_release(server);
        }
    }

    return remaining;
}

static DWORD WINAPI collect_connections_proc(void *arg)
{
    BOOL remaining_conns;

    SetThreadDescription(GetCurrentThread(), L"wine_wininet_collect_connections");
    do {
        /* FIXME: Use more sophisticated method */
        Sleep(5000);

        EnterCriticalSection(&connection_pool_cs);

        remaining_conns = collect_connections(COLLECT_TIMEOUT);
        if(!remaining_conns)
            collector_running = FALSE;

        LeaveCriticalSection(&connection_pool_cs);
    }while(remaining_conns);

    FreeLibraryAndExitThread(WININET_hModule, 0);
}

/***********************************************************************
 *           HTTP_GetHeader (internal)
 *
 * Headers section must be held
 */
static LPHTTPHEADERW HTTP_GetHeader(http_request_t *req, LPCWSTR head)
{
    int HeaderIndex = 0;
    HeaderIndex = HTTP_GetCustomHeaderIndex(req, head, 0, TRUE);
    if (HeaderIndex == -1)
        return NULL;
    else
        return &req->custHeaders[HeaderIndex];
}

static WCHAR *get_host_header( http_request_t *req )
{
    HTTPHEADERW *header;
    WCHAR *ret = NULL;

    EnterCriticalSection( &req->headers_section );
    if ((header = HTTP_GetHeader( req, L"Host" ))) ret = wcsdup( header->lpszValue );
    else ret = wcsdup( req->server->canon_host_port );
    LeaveCriticalSection( &req->headers_section );
    return ret;
}

struct data_stream_vtbl_t {
    BOOL (*end_of_data)(data_stream_t*,http_request_t*);
    DWORD (*read)(data_stream_t*,http_request_t*,BYTE*,DWORD,DWORD*,BOOL);
    DWORD (*drain_content)(data_stream_t*,http_request_t*,BOOL);
    void (*destroy)(data_stream_t*);
};

typedef struct {
    data_stream_t data_stream;

    BYTE buf[READ_BUFFER_SIZE];
    DWORD buf_size;
    DWORD buf_pos;
    DWORD chunk_size;

    enum {
        CHUNKED_STREAM_STATE_READING_CHUNK_SIZE,
        CHUNKED_STREAM_STATE_DISCARD_EOL_AFTER_SIZE,
        CHUNKED_STREAM_STATE_READING_CHUNK,
        CHUNKED_STREAM_STATE_DISCARD_EOL_AFTER_DATA,
        CHUNKED_STREAM_STATE_DISCARD_EOL_AT_END,
        CHUNKED_STREAM_STATE_END_OF_STREAM,
        CHUNKED_STREAM_STATE_ERROR
    } state;
} chunked_stream_t;

static inline void destroy_data_stream(data_stream_t *stream)
{
    stream->vtbl->destroy(stream);
}

static void reset_data_stream(http_request_t *req)
{
    destroy_data_stream(req->data_stream);
    req->data_stream = &req->netconn_stream.data_stream;
    req->read_pos = req->read_size = req->netconn_stream.content_read = 0;
    req->content_pos = 0;
    req->read_gzip = FALSE;
}

static void remove_header( http_request_t *request, const WCHAR *str, BOOL from_request )
{
    int index;
    EnterCriticalSection( &request->headers_section );
    index = HTTP_GetCustomHeaderIndex( request, str, 0, from_request );
    if (index != -1) HTTP_DeleteCustomHeader( request, index );
    LeaveCriticalSection( &request->headers_section );
}

typedef struct {
    data_stream_t stream;
    data_stream_t *parent_stream;
    z_stream zstream;
    BYTE buf[READ_BUFFER_SIZE];
    DWORD buf_size;
    DWORD buf_pos;
    BOOL end_of_data;
} gzip_stream_t;

static BOOL gzip_end_of_data(data_stream_t *stream, http_request_t *req)
{
    gzip_stream_t *gzip_stream = (gzip_stream_t*)stream;
    return gzip_stream->end_of_data
        || (!gzip_stream->buf_size && gzip_stream->parent_stream->vtbl->end_of_data(gzip_stream->parent_stream, req));
}

static DWORD gzip_read(data_stream_t *stream, http_request_t *req, BYTE *buf, DWORD size,
        DWORD *read, BOOL allow_blocking)
{
    gzip_stream_t *gzip_stream = (gzip_stream_t*)stream;
    z_stream *zstream = &gzip_stream->zstream;
    DWORD current_read, ret_read = 0;
    int zres;
    DWORD res = ERROR_SUCCESS;

    TRACE("(%ld %x)\n", size, allow_blocking);

    while(size && !gzip_stream->end_of_data) {
        if(!gzip_stream->buf_size) {
            if(gzip_stream->buf_pos) {
                if(gzip_stream->buf_size)
                    memmove(gzip_stream->buf, gzip_stream->buf+gzip_stream->buf_pos, gzip_stream->buf_size);
                gzip_stream->buf_pos = 0;
            }
            res = gzip_stream->parent_stream->vtbl->read(gzip_stream->parent_stream, req, gzip_stream->buf+gzip_stream->buf_size,
                    sizeof(gzip_stream->buf)-gzip_stream->buf_size, &current_read, allow_blocking);
            if(res != ERROR_SUCCESS)
                break;

            gzip_stream->buf_size += current_read;
            if(!current_read) {
                WARN("unexpected end of data\n");
                gzip_stream->end_of_data = TRUE;
                break;
            }
        }

        zstream->next_in = gzip_stream->buf+gzip_stream->buf_pos;
        zstream->avail_in = gzip_stream->buf_size;
        zstream->next_out = buf+ret_read;
        zstream->avail_out = size;
        zres = inflate(&gzip_stream->zstream, 0);
        current_read = size - zstream->avail_out;
        size -= current_read;
        ret_read += current_read;
        gzip_stream->buf_size -= zstream->next_in - (gzip_stream->buf+gzip_stream->buf_pos);
        gzip_stream->buf_pos = zstream->next_in-gzip_stream->buf;
        if(zres == Z_STREAM_END) {
            TRACE("end of data\n");
            gzip_stream->end_of_data = TRUE;
            inflateEnd(zstream);
        }else if(zres != Z_OK) {
            WARN("inflate failed %d: %s\n", zres, debugstr_a(zstream->msg));
            if(!ret_read)
                res = ERROR_INTERNET_DECODING_FAILED;
            break;
        }

        if(ret_read)
            allow_blocking = FALSE;
    }

    TRACE("read %lu bytes\n", ret_read);
    if(ret_read)
        res = ERROR_SUCCESS;
    *read = ret_read;
    return res;
}

static DWORD gzip_drain_content(data_stream_t *stream, http_request_t *req, BOOL allow_blocking)
{
    gzip_stream_t *gzip_stream = (gzip_stream_t*)stream;
    return gzip_stream->parent_stream->vtbl->drain_content(gzip_stream->parent_stream, req, allow_blocking);
}

static void gzip_destroy(data_stream_t *stream)
{
    gzip_stream_t *gzip_stream = (gzip_stream_t*)stream;

    destroy_data_stream(gzip_stream->parent_stream);

    if(!gzip_stream->end_of_data)
        inflateEnd(&gzip_stream->zstream);
    free(gzip_stream);
}

static const data_stream_vtbl_t gzip_stream_vtbl = {
    gzip_end_of_data,
    gzip_read,
    gzip_drain_content,
    gzip_destroy
};

static voidpf wininet_zalloc(voidpf opaque, uInt items, uInt size)
{
    return malloc(items * size);
}

static void wininet_zfree(voidpf opaque, voidpf address)
{
    free(address);
}

static DWORD init_gzip_stream(http_request_t *req, BOOL is_gzip)
{
    gzip_stream_t *gzip_stream;
    int zres;

    gzip_stream = calloc(1, sizeof(gzip_stream_t));
    if(!gzip_stream)
        return ERROR_OUTOFMEMORY;

    gzip_stream->stream.vtbl = &gzip_stream_vtbl;
    gzip_stream->zstream.zalloc = wininet_zalloc;
    gzip_stream->zstream.zfree = wininet_zfree;

    zres = inflateInit2(&gzip_stream->zstream, is_gzip ? 0x1f : -15);
    if(zres != Z_OK) {
        ERR("inflateInit failed: %d\n", zres);
        free(gzip_stream);
        return ERROR_OUTOFMEMORY;
    }

    remove_header(req, L"Content-Length", FALSE);

    if(req->read_size) {
        memcpy(gzip_stream->buf, req->read_buf+req->read_pos, req->read_size);
        gzip_stream->buf_size = req->read_size;
        req->read_pos = req->read_size = 0;
    }

    req->read_gzip = TRUE;
    gzip_stream->parent_stream = req->data_stream;
    req->data_stream = &gzip_stream->stream;
    return ERROR_SUCCESS;
}

/***********************************************************************
 *           HTTP_FreeTokens (internal)
 *
 *  Frees table of pointers.
 */
static void HTTP_FreeTokens(LPWSTR * token_array)
{
    int i;
    for (i = 0; token_array[i]; i++) free(token_array[i]);
    free(token_array);
}

static void HTTP_FixURL(http_request_t *request)
{
    /* If we don't have a path we set it to root */
    if (NULL == request->path)
        request->path = wcsdup(L"/");
    else /* remove \r and \n*/
    {
        int nLen = lstrlenW(request->path);
        while ((nLen >0 ) && ((request->path[nLen-1] == '\r')||(request->path[nLen-1] == '\n')))
        {
            nLen--;
            request->path[nLen]='\0';
        }
        /* Replace '\' with '/' */
        while (nLen>0) {
            nLen--;
            if (request->path[nLen] == '\\') request->path[nLen]='/';
        }
    }

    if(CSTR_EQUAL != CompareStringW( LOCALE_INVARIANT, NORM_IGNORECASE,
                       request->path, lstrlenW(request->path), L"http://", lstrlenW(L"http://") )
       && request->path[0] != '/') /* not an absolute path ?? --> fix it !! */
    {
        WCHAR *fixurl = malloc((wcslen(request->path) + 2) * sizeof(WCHAR));
        *fixurl = '/';
        lstrcpyW(fixurl + 1, request->path);
        free(request->path);
        request->path = fixurl;
    }
}

static WCHAR* build_request_header(http_request_t *request, const WCHAR *verb,
        const WCHAR *path, const WCHAR *version, BOOL use_cr)
{
    LPWSTR requestString;
    DWORD len, n;
    LPCWSTR *req;
    UINT i;

    EnterCriticalSection( &request->headers_section );

    /* allocate space for an array of all the string pointers to be added */
    len = request->nCustHeaders * 5 + 10;
    if (!(req = malloc( len * sizeof(const WCHAR *) )))
    {
        LeaveCriticalSection( &request->headers_section );
        return NULL;
    }

    /* add the verb, path and HTTP version string */
    n = 0;
    req[n++] = verb;
    req[n++] = L" ";
    req[n++] = path;
    req[n++] = L" ";
    req[n++] = version;
    if (use_cr)
        req[n++] = L"\r";
    req[n++] = L"\n";

    /* Append custom request headers */
    for (i = 0; i < request->nCustHeaders; i++)
    {
        if (request->custHeaders[i].wFlags & HDR_ISREQUEST)
        {
            req[n++] = request->custHeaders[i].lpszField;
            req[n++] = L": ";
            req[n++] = request->custHeaders[i].lpszValue;
            if (use_cr)
                req[n++] = L"\r";
            req[n++] = L"\n";

            TRACE("Adding custom header %s (%s)\n",
                   debugstr_w(request->custHeaders[i].lpszField),
                   debugstr_w(request->custHeaders[i].lpszValue));
        }
    }
    if (use_cr)
        req[n++] = L"\r";
    req[n++] = L"\n";
    req[n] = NULL;

    requestString = HTTP_build_req( req, 4 );
    free( req );
    LeaveCriticalSection( &request->headers_section );
    return requestString;
}

static WCHAR* build_response_header(http_request_t *request, BOOL use_cr)
{
    const WCHAR **req;
    WCHAR *ret, buf[14];
    DWORD i, n = 0;

    EnterCriticalSection( &request->headers_section );

    if (!(req = malloc((request->nCustHeaders * 5 + 8) * sizeof(WCHAR *))))
    {
        LeaveCriticalSection( &request->headers_section );
        return NULL;
    }

    if (request->status_code)
    {
        req[n++] = request->version;
        swprintf(buf, ARRAY_SIZE(buf), L" %u ", request->status_code);
        req[n++] = buf;
        req[n++] = request->statusText;
        if (use_cr)
            req[n++] = L"\r";
        req[n++] = L"\n";
    }

    for(i = 0; i < request->nCustHeaders; i++)
    {
        if(!(request->custHeaders[i].wFlags & HDR_ISREQUEST)
                && wcscmp(request->custHeaders[i].lpszField, L"Status"))
        {
            req[n++] = request->custHeaders[i].lpszField;
            req[n++] = L": ";
            req[n++] = request->custHeaders[i].lpszValue;
            if(use_cr)
                req[n++] = L"\r";
            req[n++] = L"\n";

            TRACE("Adding custom header %s (%s)\n",
                    debugstr_w(request->custHeaders[i].lpszField),
                    debugstr_w(request->custHeaders[i].lpszValue));
        }
    }
    if(use_cr)
        req[n++] = L"\r";
    req[n++] = L"\n";
    req[n] = NULL;

    ret = HTTP_build_req(req, 0);
    free(req);
    LeaveCriticalSection( &request->headers_section );
    return ret;
}

static void HTTP_ProcessCookies( http_request_t *request )
{
    int HeaderIndex;
    int numCookies = 0;
    LPHTTPHEADERW setCookieHeader;

    if(request->hdr.dwFlags & INTERNET_FLAG_NO_COOKIES)
        return;

    EnterCriticalSection( &request->headers_section );

    while((HeaderIndex = HTTP_GetCustomHeaderIndex(request, L"Set-Cookie", numCookies++, FALSE)) != -1)
    {
        const WCHAR *data;
        substr_t name;

        setCookieHeader = &request->custHeaders[HeaderIndex];

        if (!setCookieHeader->lpszValue)
            continue;

        data = wcschr(setCookieHeader->lpszValue, '=');
        if(!data)
            continue;

        name = substr(setCookieHeader->lpszValue, data - setCookieHeader->lpszValue);
        data++;
        set_cookie(substrz(request->server->name), substrz(request->path), name, substrz(data), INTERNET_COOKIE_HTTPONLY);
    }

    LeaveCriticalSection( &request->headers_section );
}

static void strip_spaces(LPWSTR start)
{
    LPWSTR str = start;
    LPWSTR end;

    while (*str == ' ')
        str++;

    if (str != start)
        memmove(start, str, sizeof(WCHAR) * (lstrlenW(str) + 1));

    end = start + lstrlenW(start) - 1;
    while (end >= start && *end == ' ')
    {
        *end = '\0';
        end--;
    }
}

static inline BOOL is_basic_auth_value( LPCWSTR pszAuthValue, LPWSTR *pszRealm )
{
    static const WCHAR szBasic[] = {'B','a','s','i','c'}; /* Note: not nul-terminated */
    static const WCHAR szRealm[] = {'r','e','a','l','m'}; /* Note: not nul-terminated */
    BOOL is_basic;
    is_basic = !wcsnicmp(pszAuthValue, szBasic, ARRAY_SIZE(szBasic)) &&
        ((pszAuthValue[ARRAY_SIZE(szBasic)] == ' ') || !pszAuthValue[ARRAY_SIZE(szBasic)]);
    if (is_basic && pszRealm)
    {
        LPCWSTR token;
        LPCWSTR ptr = &pszAuthValue[ARRAY_SIZE(szBasic)];
        LPCWSTR realm;
        ptr++;
        *pszRealm=NULL;
        token = wcschr(ptr,'=');
        if (!token)
            return TRUE;
        realm = ptr;
        while (*realm == ' ')
            realm++;
        if(!wcsnicmp(realm, szRealm, ARRAY_SIZE(szRealm)) &&
            (realm[ARRAY_SIZE(szRealm)] == ' ' || realm[ARRAY_SIZE(szRealm)] == '='))
        {
            token++;
            while (*token == ' ')
                token++;
            if (*token == '\0')
                return TRUE;
            *pszRealm = wcsdup(token);
            strip_spaces(*pszRealm);
        }
    }

    return is_basic;
}

static void destroy_authinfo( struct HttpAuthInfo *authinfo )
{
    if (!authinfo) return;

    if (SecIsValidHandle(&authinfo->ctx))
        DeleteSecurityContext(&authinfo->ctx);
    if (SecIsValidHandle(&authinfo->cred))
        FreeCredentialsHandle(&authinfo->cred);

    free(authinfo->auth_data);
    free(authinfo->scheme);
    free(authinfo);
}

static UINT retrieve_cached_basic_authorization(http_request_t *req, const WCHAR *host, const WCHAR *realm, char **auth_data)
{
    basicAuthorizationData *ad;
    UINT rc = 0;

    TRACE("Looking for authorization for %s:%s\n",debugstr_w(host),debugstr_w(realm));

    EnterCriticalSection(&authcache_cs);
    LIST_FOR_EACH_ENTRY(ad, &basicAuthorizationCache, basicAuthorizationData, entry)
    {
        if (!wcsicmp(host, ad->host) && (!realm || !wcscmp(realm, ad->realm)))
        {
            char *colon;
            DWORD length;

            TRACE("Authorization found in cache\n");
            *auth_data = malloc(ad->authorizationLen);
            memcpy(*auth_data,ad->authorization,ad->authorizationLen);
            rc = ad->authorizationLen;

            /* update session username and password to reflect current credentials */
            colon = strchr(ad->authorization, ':');
            length = colon - ad->authorization;

            free(req->session->userName);
            free(req->session->password);

            req->session->userName = strndupAtoW(ad->authorization, length, &length);
            length++;
            req->session->password = strndupAtoW(&ad->authorization[length], ad->authorizationLen - length, &length);
            break;
        }
    }
    LeaveCriticalSection(&authcache_cs);
    return rc;
}

static void cache_basic_authorization(LPWSTR host, LPWSTR realm, LPSTR auth_data, UINT auth_data_len)
{
    struct list *cursor;
    basicAuthorizationData* ad = NULL;

    TRACE("caching authorization for %s:%s = %s\n",debugstr_w(host),debugstr_w(realm),debugstr_an(auth_data,auth_data_len));

    EnterCriticalSection(&authcache_cs);
    LIST_FOR_EACH(cursor, &basicAuthorizationCache)
    {
        basicAuthorizationData *check = LIST_ENTRY(cursor,basicAuthorizationData,entry);
        if (!wcsicmp(host,check->host) && !wcscmp(realm,check->realm))
        {
            ad = check;
            break;
        }
    }

    if (ad)
    {
        TRACE("Found match in cache, replacing\n");
        free(ad->authorization);
        ad->authorization = malloc(auth_data_len);
        memcpy(ad->authorization, auth_data, auth_data_len);
        ad->authorizationLen = auth_data_len;
    }
    else
    {
        ad = malloc(sizeof(basicAuthorizationData));
        ad->host = wcsdup(host);
        ad->realm = wcsdup(realm);
        ad->authorization = malloc(auth_data_len);
        memcpy(ad->authorization, auth_data, auth_data_len);
        ad->authorizationLen = auth_data_len;
        list_add_head(&basicAuthorizationCache,&ad->entry);
        TRACE("authorization cached\n");
    }
    LeaveCriticalSection(&authcache_cs);
}

static BOOL retrieve_cached_authorization(LPWSTR host, LPWSTR scheme,
        SEC_WINNT_AUTH_IDENTITY_W *nt_auth_identity)
{
    authorizationData *ad;

    TRACE("Looking for authorization for %s:%s\n", debugstr_w(host), debugstr_w(scheme));

    EnterCriticalSection(&authcache_cs);
    LIST_FOR_EACH_ENTRY(ad, &authorizationCache, authorizationData, entry) {
        if(!wcsicmp(host, ad->host) && !wcsicmp(scheme, ad->scheme)) {
            TRACE("Authorization found in cache\n");

            nt_auth_identity->User = wcsdup(ad->user);
            nt_auth_identity->Password = wcsdup(ad->password);
            nt_auth_identity->Domain = malloc(sizeof(WCHAR) * ad->domain_len);
            if(!nt_auth_identity->User || !nt_auth_identity->Password ||
                    (!nt_auth_identity->Domain && ad->domain_len)) {
                free(nt_auth_identity->User);
                free(nt_auth_identity->Password);
                free(nt_auth_identity->Domain);
                break;
            }

            nt_auth_identity->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
            nt_auth_identity->UserLength = ad->user_len;
            nt_auth_identity->PasswordLength = ad->password_len;
            memcpy(nt_auth_identity->Domain, ad->domain, sizeof(WCHAR)*ad->domain_len);
            nt_auth_identity->DomainLength = ad->domain_len;
            LeaveCriticalSection(&authcache_cs);
            return TRUE;
        }
    }
    LeaveCriticalSection(&authcache_cs);

    return FALSE;
}

static void cache_authorization(LPWSTR host, LPWSTR scheme,
        SEC_WINNT_AUTH_IDENTITY_W *nt_auth_identity)
{
    authorizationData *ad;
    BOOL found = FALSE;

    TRACE("Caching authorization for %s:%s\n", debugstr_w(host), debugstr_w(scheme));

    EnterCriticalSection(&authcache_cs);
    LIST_FOR_EACH_ENTRY(ad, &authorizationCache, authorizationData, entry)
        if(!wcsicmp(host, ad->host) && !wcsicmp(scheme, ad->scheme)) {
            found = TRUE;
            break;
        }

    if(found) {
        free(ad->user);
        free(ad->password);
        free(ad->domain);
    } else {
        ad = malloc(sizeof(authorizationData));
        if(!ad) {
            LeaveCriticalSection(&authcache_cs);
            return;
        }

        ad->host = wcsdup(host);
        ad->scheme = wcsdup(scheme);
        list_add_head(&authorizationCache, &ad->entry);
    }

    ad->user = strndupW(nt_auth_identity->User, nt_auth_identity->UserLength);
    ad->password = strndupW(nt_auth_identity->Password, nt_auth_identity->PasswordLength);
    ad->domain = strndupW(nt_auth_identity->Domain, nt_auth_identity->DomainLength);
    ad->user_len = nt_auth_identity->UserLength;
    ad->password_len = nt_auth_identity->PasswordLength;
    ad->domain_len = nt_auth_identity->DomainLength;

    if(!ad->host || !ad->scheme || !ad->user || !ad->password
            || (nt_auth_identity->Domain && !ad->domain)) {
        free(ad->host);
        free(ad->scheme);
        free(ad->user);
        free(ad->password);
        free(ad->domain);
        list_remove(&ad->entry);
        free(ad);
    }

    LeaveCriticalSection(&authcache_cs);
}

void free_authorization_cache(void)
{
    authorizationData *ad, *sa_safe;
    basicAuthorizationData *basic, *basic_safe;

    EnterCriticalSection(&authcache_cs);

    LIST_FOR_EACH_ENTRY_SAFE(basic, basic_safe, &basicAuthorizationCache, basicAuthorizationData, entry)
    {
        free(basic->host);
        free(basic->realm);
        free(basic->authorization);

        list_remove(&basic->entry);
        free(basic);
    }

    LIST_FOR_EACH_ENTRY_SAFE(ad, sa_safe, &authorizationCache, authorizationData, entry)
    {
        free(ad->host);
        free(ad->scheme);
        free(ad->user);
        free(ad->password);
        free(ad->domain);
        list_remove(&ad->entry);
        free(ad);
    }

    LeaveCriticalSection(&authcache_cs);
}

static BOOL HTTP_DoAuthorization( http_request_t *request, LPCWSTR pszAuthValue,
                                  struct HttpAuthInfo **ppAuthInfo,
                                  LPWSTR domain_and_username, LPWSTR password,
                                  LPWSTR host )
{
    SECURITY_STATUS sec_status;
    struct HttpAuthInfo *pAuthInfo = *ppAuthInfo;
    BOOL first = FALSE;
    LPWSTR szRealm = NULL;

    TRACE("%s\n", debugstr_w(pszAuthValue));

    if (!pAuthInfo)
    {
        TimeStamp exp;

        first = TRUE;
        pAuthInfo = malloc(sizeof(*pAuthInfo));
        if (!pAuthInfo)
            return FALSE;

        SecInvalidateHandle(&pAuthInfo->cred);
        SecInvalidateHandle(&pAuthInfo->ctx);
        memset(&pAuthInfo->exp, 0, sizeof(pAuthInfo->exp));
        pAuthInfo->attr = 0;
        pAuthInfo->auth_data = NULL;
        pAuthInfo->auth_data_len = 0;
        pAuthInfo->finished = FALSE;

        if (is_basic_auth_value(pszAuthValue,NULL))
        {
            pAuthInfo->scheme = wcsdup(L"Basic");
            if (!pAuthInfo->scheme)
            {
                free(pAuthInfo);
                return FALSE;
            }
        }
        else
        {
            PVOID pAuthData;
            SEC_WINNT_AUTH_IDENTITY_W nt_auth_identity;

            pAuthInfo->scheme = wcsdup(pszAuthValue);
            if (!pAuthInfo->scheme)
            {
                free(pAuthInfo);
                return FALSE;
            }

            if (domain_and_username)
            {
                WCHAR *user = wcschr(domain_and_username, '\\');
                WCHAR *domain = domain_and_username;

                /* FIXME: make sure scheme accepts SEC_WINNT_AUTH_IDENTITY before calling AcquireCredentialsHandle */

                pAuthData = &nt_auth_identity;

                if (user) user++;
                else
                {
                    user = domain_and_username;
                    domain = NULL;
                }

                nt_auth_identity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
                nt_auth_identity.User = user;
                nt_auth_identity.UserLength = lstrlenW(nt_auth_identity.User);
                nt_auth_identity.Domain = domain;
                nt_auth_identity.DomainLength = domain ? user - domain - 1 : 0;
                nt_auth_identity.Password = password;
                nt_auth_identity.PasswordLength = lstrlenW(nt_auth_identity.Password);

                cache_authorization(host, pAuthInfo->scheme, &nt_auth_identity);
            }
            else if(retrieve_cached_authorization(host, pAuthInfo->scheme, &nt_auth_identity))
                pAuthData = &nt_auth_identity;
            else
                /* use default credentials */
                pAuthData = NULL;

            sec_status = AcquireCredentialsHandleW(NULL, pAuthInfo->scheme,
                                                   SECPKG_CRED_OUTBOUND, NULL,
                                                   pAuthData, NULL,
                                                   NULL, &pAuthInfo->cred,
                                                   &exp);

            if(pAuthData && !domain_and_username) {
                free(nt_auth_identity.User);
                free(nt_auth_identity.Domain);
                free(nt_auth_identity.Password);
            }

            if (sec_status == SEC_E_OK)
            {
                PSecPkgInfoW sec_pkg_info;
                sec_status = QuerySecurityPackageInfoW(pAuthInfo->scheme, &sec_pkg_info);
                if (sec_status == SEC_E_OK)
                {
                    pAuthInfo->max_token = sec_pkg_info->cbMaxToken;
                    FreeContextBuffer(sec_pkg_info);
                }
            }
            if (sec_status != SEC_E_OK)
            {
                WARN("AcquireCredentialsHandleW for scheme %s failed with error 0x%08lx\n",
                     debugstr_w(pAuthInfo->scheme), sec_status);
                free(pAuthInfo->scheme);
                free(pAuthInfo);
                return FALSE;
            }
        }
        *ppAuthInfo = pAuthInfo;
    }
    else if (pAuthInfo->finished)
        return FALSE;

    if ((lstrlenW(pszAuthValue) < lstrlenW(pAuthInfo->scheme)) ||
        wcsnicmp(pszAuthValue, pAuthInfo->scheme, lstrlenW(pAuthInfo->scheme)))
    {
        ERR("authentication scheme changed from %s to %s\n",
            debugstr_w(pAuthInfo->scheme), debugstr_w(pszAuthValue));
        return FALSE;
    }

    if (is_basic_auth_value(pszAuthValue,&szRealm))
    {
        int userlen;
        int passlen;
        char *auth_data = NULL;
        UINT auth_data_len = 0;

        TRACE("basic authentication realm %s\n",debugstr_w(szRealm));

        if (!domain_and_username)
        {
            if (host && szRealm)
                auth_data_len = retrieve_cached_basic_authorization(request, host, szRealm,&auth_data);
            if (auth_data_len == 0)
            {
                free(szRealm);
                return FALSE;
            }
        }
        else
        {
            userlen = WideCharToMultiByte(CP_UTF8, 0, domain_and_username, lstrlenW(domain_and_username), NULL, 0, NULL, NULL);
            passlen = WideCharToMultiByte(CP_UTF8, 0, password, lstrlenW(password), NULL, 0, NULL, NULL);

            /* length includes a nul terminator, which will be re-used for the ':' */
            auth_data = malloc(userlen + 1 + passlen);
            if (!auth_data)
            {
                free(szRealm);
                return FALSE;
            }

            WideCharToMultiByte(CP_UTF8, 0, domain_and_username, -1, auth_data, userlen, NULL, NULL);
            auth_data[userlen] = ':';
            WideCharToMultiByte(CP_UTF8, 0, password, -1, &auth_data[userlen+1], passlen, NULL, NULL);
            auth_data_len = userlen + 1 + passlen;
            if (host && szRealm)
                cache_basic_authorization(host, szRealm, auth_data, auth_data_len);
        }

        pAuthInfo->auth_data = auth_data;
        pAuthInfo->auth_data_len = auth_data_len;
        pAuthInfo->finished = TRUE;
        free(szRealm);
        return TRUE;
    }
    else
    {
        LPCWSTR pszAuthData;
        SecBufferDesc out_desc, in_desc;
        SecBuffer out, in;
        unsigned char *buffer;
        ULONG context_req = ISC_REQ_CONNECTION | ISC_REQ_USE_DCE_STYLE |
            ISC_REQ_MUTUAL_AUTH | ISC_REQ_DELEGATE;

        in.BufferType = SECBUFFER_TOKEN;
        in.cbBuffer = 0;
        in.pvBuffer = NULL;

        in_desc.ulVersion = 0;
        in_desc.cBuffers = 1;
        in_desc.pBuffers = &in;

        pszAuthData = pszAuthValue + lstrlenW(pAuthInfo->scheme);
        if (*pszAuthData == ' ')
        {
            pszAuthData++;
            in.cbBuffer = HTTP_DecodeBase64(pszAuthData, NULL);
            in.pvBuffer = malloc(in.cbBuffer);
            HTTP_DecodeBase64(pszAuthData, in.pvBuffer);
        }

        buffer = malloc(pAuthInfo->max_token);

        out.BufferType = SECBUFFER_TOKEN;
        out.cbBuffer = pAuthInfo->max_token;
        out.pvBuffer = buffer;

        out_desc.ulVersion = 0;
        out_desc.cBuffers = 1;
        out_desc.pBuffers = &out;

        sec_status = InitializeSecurityContextW(first ? &pAuthInfo->cred : NULL,
                                                first ? NULL : &pAuthInfo->ctx,
                                                first ? request->server->name : NULL,
                                                context_req, 0, SECURITY_NETWORK_DREP,
                                                in.pvBuffer ? &in_desc : NULL,
                                                0, &pAuthInfo->ctx, &out_desc,
                                                &pAuthInfo->attr, &pAuthInfo->exp);
        if (sec_status == SEC_E_OK)
        {
            pAuthInfo->finished = TRUE;
            pAuthInfo->auth_data = out.pvBuffer;
            pAuthInfo->auth_data_len = out.cbBuffer;
            TRACE("sending last auth packet\n");
        }
        else if (sec_status == SEC_I_CONTINUE_NEEDED)
        {
            pAuthInfo->auth_data = out.pvBuffer;
            pAuthInfo->auth_data_len = out.cbBuffer;
            TRACE("sending next auth packet\n");
        }
        else
        {
            ERR("InitializeSecurityContextW returned error 0x%08lx\n", sec_status);
            free(out.pvBuffer);
            destroy_authinfo(pAuthInfo);
            *ppAuthInfo = NULL;
            return FALSE;
        }
    }

    return TRUE;
}

/***********************************************************************
 *           HTTP_HttpAddRequestHeadersW (internal)
 */
static DWORD HTTP_HttpAddRequestHeadersW(http_request_t *request,
	LPCWSTR lpszHeader, DWORD dwHeaderLength, DWORD dwModifier)
{
    LPWSTR lpszStart;
    LPWSTR lpszEnd;
    LPWSTR buffer;
    DWORD len, res = ERROR_HTTP_INVALID_HEADER;

    TRACE("copying header: %s\n", debugstr_wn(lpszHeader, dwHeaderLength));

    if( dwHeaderLength == ~0U )
        len = lstrlenW(lpszHeader);
    else
        len = dwHeaderLength;
    buffer = malloc(sizeof(WCHAR) * (len + 1));
    lstrcpynW( buffer, lpszHeader, len + 1);

    lpszStart = buffer;

    do
    {
        LPWSTR * pFieldAndValue;

        lpszEnd = lpszStart;

        while (*lpszEnd != '\0')
        {
            if (*lpszEnd == '\r' || *lpszEnd == '\n')
                 break;
            lpszEnd++;
        }

        if (*lpszStart == '\0')
	    break;

        if (*lpszEnd == '\r' || *lpszEnd == '\n')
        {
            *lpszEnd = '\0';
            lpszEnd++; /* Jump over newline */
        }
        TRACE("interpreting header %s\n", debugstr_w(lpszStart));
        if (*lpszStart == '\0')
        {
            /* Skip 0-length headers */
            lpszStart = lpszEnd;
            res = ERROR_SUCCESS;
            continue;
        }
        pFieldAndValue = HTTP_InterpretHttpHeader(lpszStart);
        if (pFieldAndValue)
        {
            res = HTTP_ProcessHeader(request, pFieldAndValue[0],
                pFieldAndValue[1], dwModifier | HTTP_ADDHDR_FLAG_REQ);
            HTTP_FreeTokens(pFieldAndValue);
        }

        lpszStart = lpszEnd;
    } while (res == ERROR_SUCCESS);

    free(buffer);
    return res;
}

/***********************************************************************
 *           HttpAddRequestHeadersW (WININET.@)
 *
 * Adds one or more HTTP header to the request handler
 *
 * NOTE
 * On Windows if dwHeaderLength includes the trailing '\0', then
 * HttpAddRequestHeadersW() adds it too. However this results in an
 * invalid HTTP header which is rejected by some servers so we probably
 * don't need to match Windows on that point.
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HttpAddRequestHeadersW(HINTERNET hHttpRequest,
	LPCWSTR lpszHeader, DWORD dwHeaderLength, DWORD dwModifier)
{
    http_request_t *request;
    DWORD res = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;

    TRACE("%p, %s, %lu, %08lx\n", hHttpRequest, debugstr_wn(lpszHeader, dwHeaderLength), dwHeaderLength, dwModifier);

    if (!lpszHeader)
      return TRUE;

    request = (http_request_t*) get_handle_object( hHttpRequest );
    if (request && request->hdr.htype == WH_HHTTPREQ)
        res = HTTP_HttpAddRequestHeadersW( request, lpszHeader, dwHeaderLength, dwModifier );
    if( request )
        WININET_Release( &request->hdr );

    if(res != ERROR_SUCCESS)
        SetLastError(res);
    return res == ERROR_SUCCESS;
}

/***********************************************************************
 *           HttpAddRequestHeadersA (WININET.@)
 *
 * Adds one or more HTTP header to the request handler
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HttpAddRequestHeadersA(HINTERNET hHttpRequest,
	LPCSTR lpszHeader, DWORD dwHeaderLength, DWORD dwModifier)
{
    WCHAR *headers = NULL;
    BOOL r;

    TRACE("%p, %s, %lu, %08lx\n", hHttpRequest, debugstr_an(lpszHeader, dwHeaderLength), dwHeaderLength, dwModifier);

    if(lpszHeader)
        headers = strndupAtoW(lpszHeader, dwHeaderLength, &dwHeaderLength);

    r = HttpAddRequestHeadersW(hHttpRequest, headers, dwHeaderLength, dwModifier);

    free(headers);
    return r;
}

static void free_accept_types( WCHAR **accept_types )
{
    WCHAR *ptr, **types = accept_types;

    if (!types) return;
    while ((ptr = *types))
    {
        free(ptr);
        types++;
    }
    free(accept_types);
}

static WCHAR **convert_accept_types( const char **accept_types )
{
    unsigned int count;
    const char **types = accept_types;
    WCHAR **typesW;
    BOOL invalid_pointer = FALSE;

    if (!types) return NULL;
    count = 0;
    while (*types)
    {
        __TRY
        {
            /* find out how many there are */
            if (*types && **types)
            {
                TRACE("accept type: %s\n", debugstr_a(*types));
                count++;
            }
        }
        __EXCEPT_PAGE_FAULT
        {
            WARN("invalid accept type pointer\n");
            invalid_pointer = TRUE;
        }
        __ENDTRY;
        types++;
    }
    if (invalid_pointer) return NULL;
    if (!(typesW = malloc(sizeof(WCHAR *) * (count + 1)))) return NULL;
    count = 0;
    types = accept_types;
    while (*types)
    {
        if (*types && **types) typesW[count++] = strdupAtoW(*types);
        types++;
    }
    typesW[count] = NULL;
    return typesW;
}

/***********************************************************************
 *           HttpOpenRequestA (WININET.@)
 *
 * Open a HTTP request handle
 *
 * RETURNS
 *    HINTERNET  a HTTP request handle on success
 *    NULL 	 on failure
 *
 */
HINTERNET WINAPI HttpOpenRequestA(HINTERNET hHttpSession,
	LPCSTR lpszVerb, LPCSTR lpszObjectName, LPCSTR lpszVersion,
	LPCSTR lpszReferrer , LPCSTR *lpszAcceptTypes,
	DWORD dwFlags, DWORD_PTR dwContext)
{
    LPWSTR szVerb = NULL, szObjectName = NULL;
    LPWSTR szVersion = NULL, szReferrer = NULL, *szAcceptTypes = NULL;
    HINTERNET rc = NULL;

    TRACE("(%p, %s, %s, %s, %s, %p, %08lx, %08Ix)\n", hHttpSession,
          debugstr_a(lpszVerb), debugstr_a(lpszObjectName),
          debugstr_a(lpszVersion), debugstr_a(lpszReferrer), lpszAcceptTypes,
          dwFlags, dwContext);

    if (lpszVerb)
    {
        szVerb = strdupAtoW(lpszVerb);
        if ( !szVerb )
            goto end;
    }

    if (lpszObjectName)
    {
        szObjectName = strdupAtoW(lpszObjectName);
        if ( !szObjectName )
            goto end;
    }

    if (lpszVersion)
    {
        szVersion = strdupAtoW(lpszVersion);
        if ( !szVersion )
            goto end;
    }

    if (lpszReferrer)
    {
        szReferrer = strdupAtoW(lpszReferrer);
        if ( !szReferrer )
            goto end;
    }

    szAcceptTypes = convert_accept_types( lpszAcceptTypes );
    rc = HttpOpenRequestW(hHttpSession, szVerb, szObjectName, szVersion, szReferrer,
                          (const WCHAR **)szAcceptTypes, dwFlags, dwContext);

end:
    free_accept_types(szAcceptTypes);
    free(szReferrer);
    free(szVersion);
    free(szObjectName);
    free(szVerb);
    return rc;
}

/***********************************************************************
 *  HTTP_EncodeBase64
 */
static UINT HTTP_EncodeBase64( LPCSTR bin, unsigned int len, LPWSTR base64 )
{
    UINT n = 0, x;
    static const CHAR HTTP_Base64Enc[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    while( len > 0 )
    {
        /* first 6 bits, all from bin[0] */
        base64[n++] = HTTP_Base64Enc[(bin[0] & 0xfc) >> 2];
        x = (bin[0] & 3) << 4;

        /* next 6 bits, 2 from bin[0] and 4 from bin[1] */
        if( len == 1 )
        {
            base64[n++] = HTTP_Base64Enc[x];
            base64[n++] = '=';
            base64[n++] = '=';
            break;
        }
        base64[n++] = HTTP_Base64Enc[ x | ( (bin[1]&0xf0) >> 4 ) ];
        x = ( bin[1] & 0x0f ) << 2;

        /* next 6 bits 4 from bin[1] and 2 from bin[2] */
        if( len == 2 )
        {
            base64[n++] = HTTP_Base64Enc[x];
            base64[n++] = '=';
            break;
        }
        base64[n++] = HTTP_Base64Enc[ x | ( (bin[2]&0xc0 ) >> 6 ) ];

        /* last 6 bits, all from bin [2] */
        base64[n++] = HTTP_Base64Enc[ bin[2] & 0x3f ];
        bin += 3;
        len -= 3;
    }
    base64[n] = 0;
    return n;
}

static const signed char HTTP_Base64Dec[] =
{
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 0x00 */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 0x10 */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, /* 0x20 */
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1, /* 0x30 */
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, /* 0x40 */
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, /* 0x50 */
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, /* 0x60 */
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1  /* 0x70 */
};

/***********************************************************************
 *  HTTP_DecodeBase64
 */
static UINT HTTP_DecodeBase64( LPCWSTR base64, LPSTR bin )
{
    unsigned int n = 0;

    while(*base64)
    {
        signed char in[4];

        if (base64[0] >= ARRAY_SIZE(HTTP_Base64Dec) ||
            ((in[0] = HTTP_Base64Dec[base64[0]]) == -1) ||
            base64[1] >= ARRAY_SIZE(HTTP_Base64Dec) ||
            ((in[1] = HTTP_Base64Dec[base64[1]]) == -1))
        {
            WARN("invalid base64: %s\n", debugstr_w(base64));
            return 0;
        }
        if (bin)
            bin[n] = (unsigned char) (in[0] << 2 | in[1] >> 4);
        n++;

        if ((base64[2] == '=') && (base64[3] == '='))
            break;
        if (base64[2] > ARRAY_SIZE(HTTP_Base64Dec) ||
            ((in[2] = HTTP_Base64Dec[base64[2]]) == -1))
        {
            WARN("invalid base64: %s\n", debugstr_w(&base64[2]));
            return 0;
        }
        if (bin)
            bin[n] = (unsigned char) (in[1] << 4 | in[2] >> 2);
        n++;

        if (base64[3] == '=')
            break;
        if (base64[3] > ARRAY_SIZE(HTTP_Base64Dec) ||
            ((in[3] = HTTP_Base64Dec[base64[3]]) == -1))
        {
            WARN("invalid base64: %s\n", debugstr_w(&base64[3]));
            return 0;
        }
        if (bin)
            bin[n] = (unsigned char) (((in[2] << 6) & 0xc0) | in[3]);
        n++;

        base64 += 4;
    }

    return n;
}

static WCHAR *encode_auth_data( const WCHAR *scheme, const char *data, UINT data_len )
{
    WCHAR *ret;
    UINT len, scheme_len = lstrlenW( scheme );

    /* scheme + space + base64 encoded data (3/2/1 bytes data -> 4 bytes of characters) */
    len = scheme_len + 1 + ((data_len + 2) * 4) / 3;
    if (!(ret = malloc( (len + 1) * sizeof(WCHAR) ))) return NULL;
    memcpy( ret, scheme, scheme_len * sizeof(WCHAR) );
    ret[scheme_len] = ' ';
    HTTP_EncodeBase64( data, data_len, ret + scheme_len + 1 );
    return ret;
}


/***********************************************************************
 *  HTTP_InsertAuthorization
 *
 *   Insert or delete the authorization field in the request header.
 */
static BOOL HTTP_InsertAuthorization( http_request_t *request, struct HttpAuthInfo *pAuthInfo, LPCWSTR header )
{
    WCHAR *host, *authorization = NULL;

    if (pAuthInfo)
    {
        if (pAuthInfo->auth_data_len)
        {
            if (!(authorization = encode_auth_data(pAuthInfo->scheme, pAuthInfo->auth_data, pAuthInfo->auth_data_len)))
                return FALSE;

            /* clear the data as it isn't valid now that it has been sent to the
             * server, unless it's Basic authentication which doesn't do
             * connection tracking */
            if (wcsicmp(pAuthInfo->scheme, L"Basic"))
            {
                free(pAuthInfo->auth_data);
                pAuthInfo->auth_data = NULL;
                pAuthInfo->auth_data_len = 0;
            }
        }

        TRACE("Inserting authorization: %s\n", debugstr_w(authorization));

        HTTP_ProcessHeader(request, header, authorization,
                           HTTP_ADDHDR_FLAG_REQ | HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD);
        free(authorization);
    }
    else
    {
        UINT data_len;
        char *data;

        /* Don't use cached credentials when a username or Authorization was specified */
        if ((request->session->userName && request->session->userName[0]) || wcscmp(header, L"Authorization"))
            return TRUE;

        if (!(host = get_host_header(request)))
            return TRUE;

        if ((data_len = retrieve_cached_basic_authorization(request, host, NULL, &data)))
        {
            TRACE("Found cached basic authorization for %s\n", debugstr_w(host));

            if (!(authorization = encode_auth_data(L"Basic", data, data_len)))
            {
                free(data);
                free(host);
                return FALSE;
            }

            TRACE("Inserting authorization: %s\n", debugstr_w(authorization));

            HTTP_ProcessHeader(request, header, authorization,
                               HTTP_ADDHDR_FLAG_REQ | HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD);
            free(data);
            free(authorization);
        }
        free(host);
    }
    return TRUE;
}

static WCHAR *build_proxy_path_url(http_request_t *req)
{
    DWORD size, len;
    WCHAR *url;

    len = lstrlenW(req->server->scheme_host_port);
    size = len + lstrlenW(req->path) + 1;
    if(*req->path != '/')
        size++;
    url = malloc(size * sizeof(WCHAR));
    if(!url)
        return NULL;

    memcpy(url, req->server->scheme_host_port, len*sizeof(WCHAR));
    if(*req->path != '/')
        url[len++] = '/';

    lstrcpyW(url+len, req->path);

    TRACE("url=%s\n", debugstr_w(url));
    return url;
}

static BOOL HTTP_DomainMatches(LPCWSTR server, substr_t domain)
{
    const WCHAR *dot, *ptr;
    int len;

    if(domain.len == ARRAY_SIZE(L"<local>")-1 && !wcsnicmp(domain.str, L"<local>", domain.len) && !wcschr(server, '.' ))
        return TRUE;

    if(domain.len && *domain.str != '*')
        return domain.len == lstrlenW(server) && !wcsnicmp(server, domain.str, domain.len);

    if(domain.len < 2 || domain.str[1] != '.')
        return FALSE;

    /* For a hostname to match a wildcard, the last domain must match
     * the wildcard exactly.  E.g. if the wildcard is *.a.b, and the
     * hostname is www.foo.a.b, it matches, but a.b does not.
     */
    dot = wcschr(server, '.');
    if(!dot)
        return FALSE;

    len = lstrlenW(dot + 1);
    if(len < domain.len - 2)
        return FALSE;

    /* The server's domain is longer than the wildcard, so it
     * could be a subdomain.  Compare the last portion of the
     * server's domain.
     */
    ptr = dot + 1 + len - domain.len + 2;
    if(!wcsnicmp(ptr, domain.str+2, domain.len-2))
        /* This is only a match if the preceding character is
         * a '.', i.e. that it is a matching domain.  E.g.
         * if domain is '*.b.c' and server is 'www.ab.c' they
         * do not match.
         */
        return *(ptr - 1) == '.';

    return len == domain.len-2 && !wcsnicmp(dot + 1, domain.str + 2, len);
}

static BOOL HTTP_ShouldBypassProxy(appinfo_t *lpwai, LPCWSTR server)
{
    LPCWSTR ptr;
    BOOL ret = FALSE;

    if (!lpwai->proxyBypass) return FALSE;
    ptr = lpwai->proxyBypass;
    while(1) {
        LPCWSTR tmp = ptr;

        ptr = wcschr( ptr, ';' );
        if (!ptr)
            ptr = wcschr( tmp, ' ' );
        if (!ptr)
            ptr = tmp + lstrlenW(tmp);
        ret = HTTP_DomainMatches( server, substr(tmp, ptr-tmp) );
        if (ret || !*ptr)
            break;
        ptr++;
    }
    return ret;
}

/***********************************************************************
 *           HTTP_DealWithProxy
 */
static BOOL HTTP_DealWithProxy(appinfo_t *hIC, http_session_t *session, http_request_t *request)
{
    static WCHAR szNul[] = L"";
    URL_COMPONENTSW UrlComponents = { sizeof(UrlComponents) };
    server_t *new_server = NULL;
    WCHAR *proxy;

    proxy = INTERNET_FindProxyForProtocol(hIC->proxy, L"http");
    if(!proxy)
        return FALSE;
    if(CSTR_EQUAL != CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                                    proxy, lstrlenW(L"http://"), L"http://", lstrlenW(L"http://"))) {
        WCHAR *proxy_url = malloc(wcslen(proxy) * sizeof(WCHAR) + sizeof(L"http://"));
        if(!proxy_url) {
            free(proxy);
            return FALSE;
        }
        lstrcpyW(proxy_url, L"http://");
        lstrcatW(proxy_url, proxy);
        free(proxy);
        proxy = proxy_url;
    }

    UrlComponents.dwHostNameLength = 1;
    if(InternetCrackUrlW(proxy, 0, 0, &UrlComponents) && UrlComponents.dwHostNameLength) {
        if( !request->path )
            request->path = szNul;

        new_server = get_server(substr(UrlComponents.lpszHostName, UrlComponents.dwHostNameLength),
                                UrlComponents.nPort, UrlComponents.nScheme == INTERNET_SCHEME_HTTPS, TRUE);
    }
    free(proxy);
    if(!new_server)
        return FALSE;

    request->proxy = new_server;

    TRACE("proxy server=%s port=%d\n", debugstr_w(new_server->name), new_server->port);
    return TRUE;
}

static DWORD HTTP_ResolveName(http_request_t *request)
{
    server_t *server = request->proxy ? request->proxy : request->server;
    int addr_len;

    if(server->addr_len)
        return ERROR_SUCCESS;

    INTERNET_SendCallback(&request->hdr, request->hdr.dwContext,
                          INTERNET_STATUS_RESOLVING_NAME,
                          server->name,
                          (lstrlenW(server->name)+1) * sizeof(WCHAR));

    addr_len = sizeof(server->addr);
    if (!GetAddress(server->name, server->port, (SOCKADDR*)&server->addr, &addr_len, server->addr_str))
        return ERROR_INTERNET_NAME_NOT_RESOLVED;

    server->addr_len = addr_len;
    INTERNET_SendCallback(&request->hdr, request->hdr.dwContext,
                          INTERNET_STATUS_NAME_RESOLVED,
                          server->addr_str, strlen(server->addr_str)+1);

    TRACE("resolved %s to %s\n", debugstr_w(server->name), server->addr_str);
    return ERROR_SUCCESS;
}

static WCHAR *compose_request_url(http_request_t *req)
{
    const WCHAR *host, *scheme;
    WCHAR *buf, *ptr;
    size_t len;

    host = req->server->canon_host_port;

    if (req->server->is_https)
        scheme = L"https://";
    else
        scheme = L"http://";

    len = lstrlenW(scheme) + lstrlenW(host) + (req->path[0] != '/' ? 1 : 0) + lstrlenW(req->path);
    ptr = buf = malloc((len + 1) * sizeof(WCHAR));
    if(buf) {
        lstrcpyW(ptr, scheme);
        ptr += lstrlenW(ptr);

        lstrcpyW(ptr, host);
        ptr += lstrlenW(ptr);

        if(req->path[0] != '/')
            *ptr++ = '/';

        lstrcpyW(ptr, req->path);
        ptr += lstrlenW(ptr);
        *ptr = 0;
    }

    return buf;
}


/***********************************************************************
 *           HTTPREQ_Destroy (internal)
 *
 * Deallocate request handle
 *
 */
static void HTTPREQ_Destroy(object_header_t *hdr)
{
    http_request_t *request = (http_request_t*) hdr;
    DWORD i;

    TRACE("\n");

    if(request->hCacheFile)
        CloseHandle(request->hCacheFile);
    if(request->req_file)
        req_file_release(request->req_file);

    request->headers_section.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection( &request->headers_section );
    request->read_section.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection( &request->read_section );
    WININET_Release(&request->session->hdr);

    destroy_authinfo(request->authInfo);
    destroy_authinfo(request->proxyAuthInfo);

    if(request->server)
        server_release(request->server);
    if(request->proxy)
        server_release(request->proxy);

    free(request->path);
    free(request->verb);
    free(request->version);
    free(request->statusText);

    for (i = 0; i < request->nCustHeaders; i++)
    {
        free(request->custHeaders[i].lpszField);
        free(request->custHeaders[i].lpszValue);
    }
    destroy_data_stream(request->data_stream);
    free(request->custHeaders);
}

static void http_release_netconn(http_request_t *req, BOOL reuse)
{
    TRACE("%p %p %x\n",req, req->netconn, reuse);

    if(!is_valid_netconn(req->netconn))
        return;

    if(reuse && req->netconn->keep_alive) {
        BOOL run_collector;

        EnterCriticalSection(&connection_pool_cs);

        list_add_head(&req->netconn->server->conn_pool, &req->netconn->pool_entry);
        req->netconn->keep_until = GetTickCount64() + COLLECT_TIME;
        req->netconn = NULL;

        run_collector = !collector_running;
        collector_running = TRUE;

        LeaveCriticalSection(&connection_pool_cs);

        if(run_collector) {
            HANDLE thread = NULL;
            HMODULE module;

            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (const WCHAR*)WININET_hModule, &module);
            if(module)
                thread = CreateThread(NULL, 0, collect_connections_proc, NULL, 0, NULL);
            if(!thread) {
                EnterCriticalSection(&connection_pool_cs);
                collector_running = FALSE;
                LeaveCriticalSection(&connection_pool_cs);

                if(module)
                    FreeLibrary(module);
            }
            else
                CloseHandle(thread);
        }
        return;
    }

    INTERNET_SendCallback(&req->hdr, req->hdr.dwContext,
                          INTERNET_STATUS_CLOSING_CONNECTION, 0, 0);

    close_netconn(req->netconn);

    INTERNET_SendCallback(&req->hdr, req->hdr.dwContext,
                          INTERNET_STATUS_CONNECTION_CLOSED, 0, 0);
}

static BOOL has_token(const WCHAR *str, const WCHAR *token)
{
    UINT len = wcslen(token);
    const WCHAR *next;

    for (; *str; str = next + wcsspn(next, L" ,"))
    {
        next = str + wcscspn(str, L" ,");
        if (next - str != len) continue;
        if (wcsnicmp(str, token, len)) continue;
        return TRUE;
    }

    return FALSE;
}

static BOOL has_keep_alive(const WCHAR *version, const WCHAR *connection)
{
    return !wcsicmp(version, L"HTTP/1.1") ? !has_token(connection, L"Close")
                                          : has_token(connection, L"Keep-Alive");
}

static BOOL HTTP_KeepAlive(http_request_t *request)
{
    WCHAR szVersion[10];
    WCHAR szConnectionResponse[20];
    DWORD dwBufferSize = sizeof(szVersion);

    /* as per RFC 2068, S8.1.2.1, if the client is HTTP/1.1 then assume that
     * the connection is keep-alive by default */
    if (HTTP_HttpQueryInfoW(request, HTTP_QUERY_VERSION, szVersion, &dwBufferSize, NULL) != ERROR_SUCCESS)
        *szVersion = 0;

    dwBufferSize = sizeof(szConnectionResponse);
    if (HTTP_HttpQueryInfoW(request, HTTP_QUERY_PROXY_CONNECTION, szConnectionResponse, &dwBufferSize, NULL) != ERROR_SUCCESS &&
        HTTP_HttpQueryInfoW(request, HTTP_QUERY_CONNECTION, szConnectionResponse, &dwBufferSize, NULL) != ERROR_SUCCESS)
        *szConnectionResponse = 0;

    return has_keep_alive(szVersion, szConnectionResponse);
}

static void HTTPREQ_CloseConnection(object_header_t *hdr)
{
    http_request_t *req = (http_request_t*)hdr;

    http_release_netconn(req, drain_content(req, FALSE) == ERROR_SUCCESS);
}

static DWORD str_to_buffer(const WCHAR *str, void *buffer, DWORD *size, BOOL unicode)
{
    int len;
    if (unicode)
    {
        WCHAR *buf = buffer;

        if (str) len = lstrlenW(str);
        else len = 0;
        if (*size < (len + 1) * sizeof(WCHAR))
        {
            *size = (len + 1) * sizeof(WCHAR);
            return ERROR_INSUFFICIENT_BUFFER;
        }
        if (str) lstrcpyW(buf, str);
        else buf[0] = 0;

        *size = len;
        return ERROR_SUCCESS;
    }
    else
    {
        char *buf = buffer;

        if (str) len = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
        else len = 1;
        if (*size < len)
        {
            *size = len;
            return ERROR_INSUFFICIENT_BUFFER;
        }
        if (str) WideCharToMultiByte(CP_ACP, 0, str, -1, buf, *size, NULL, NULL);
        else buf[0] = 0;

        *size = len - 1;
        return ERROR_SUCCESS;
    }
}

static DWORD get_security_cert_struct(http_request_t *req, INTERNET_CERTIFICATE_INFOA *info)
{
    PCCERT_CONTEXT context;
    DWORD len;

    context = (PCCERT_CONTEXT)NETCON_GetCert(req->netconn);
    if(!context)
        return ERROR_NOT_SUPPORTED;

    memset(info, 0, sizeof(*info));
    info->ftExpiry = context->pCertInfo->NotAfter;
    info->ftStart = context->pCertInfo->NotBefore;
    len = CertNameToStrA(context->dwCertEncodingType,
             &context->pCertInfo->Subject, CERT_SIMPLE_NAME_STR|CERT_NAME_STR_CRLF_FLAG, NULL, 0);
    info->lpszSubjectInfo = LocalAlloc(0, len);
    if(info->lpszSubjectInfo)
        CertNameToStrA(context->dwCertEncodingType,
                 &context->pCertInfo->Subject, CERT_SIMPLE_NAME_STR|CERT_NAME_STR_CRLF_FLAG,
                 info->lpszSubjectInfo, len);
    len = CertNameToStrA(context->dwCertEncodingType,
             &context->pCertInfo->Issuer, CERT_SIMPLE_NAME_STR|CERT_NAME_STR_CRLF_FLAG, NULL, 0);
    info->lpszIssuerInfo = LocalAlloc(0, len);
    if(info->lpszIssuerInfo)
        CertNameToStrA(context->dwCertEncodingType,
                 &context->pCertInfo->Issuer, CERT_SIMPLE_NAME_STR|CERT_NAME_STR_CRLF_FLAG,
                 info->lpszIssuerInfo, len);
    info->dwKeySize = NETCON_GetCipherStrength(req->netconn);

    CertFreeCertificateContext(context);
    return ERROR_SUCCESS;
}

static DWORD HTTPREQ_QueryOption(object_header_t *hdr, DWORD option, void *buffer, DWORD *size, BOOL unicode)
{
    http_request_t *req = (http_request_t*)hdr;

    switch(option) {
    case INTERNET_OPTION_DIAGNOSTIC_SOCKET_INFO:
    {
        INTERNET_DIAGNOSTIC_SOCKET_INFO *info = buffer;

        FIXME("INTERNET_DIAGNOSTIC_SOCKET_INFO stub\n");

        if (*size < sizeof(INTERNET_DIAGNOSTIC_SOCKET_INFO))
            return ERROR_INSUFFICIENT_BUFFER;
        *size = sizeof(INTERNET_DIAGNOSTIC_SOCKET_INFO);
        /* FIXME: can't get a SOCKET from our connection since we don't use
         * winsock
         */
        info->Socket = 0;
        /* FIXME: get source port from req->netConnection */
        info->SourcePort = 0;
        info->DestPort = req->server->port;
        info->Flags = 0;
        if (HTTP_KeepAlive(req))
            info->Flags |= IDSI_FLAG_KEEP_ALIVE;
        if (req->proxy)
            info->Flags |= IDSI_FLAG_PROXY;
        if (is_valid_netconn(req->netconn) && req->netconn->secure)
            info->Flags |= IDSI_FLAG_SECURE;

        return ERROR_SUCCESS;
    }

    case 98:
        TRACE("Queried undocumented option 98, forwarding to INTERNET_OPTION_SECURITY_FLAGS\n");
        /* fall through */
    case INTERNET_OPTION_SECURITY_FLAGS:
    {
        DWORD flags;

        if (*size < sizeof(ULONG))
            return ERROR_INSUFFICIENT_BUFFER;

        *size = sizeof(DWORD);
        flags = is_valid_netconn(req->netconn) ? req->netconn->security_flags : req->security_flags | req->server->security_flags;
        *(DWORD *)buffer = flags;

        TRACE("INTERNET_OPTION_SECURITY_FLAGS %lx\n", flags);
        return ERROR_SUCCESS;
    }

    case INTERNET_OPTION_HANDLE_TYPE:
        TRACE("INTERNET_OPTION_HANDLE_TYPE\n");

        if (*size < sizeof(ULONG))
            return ERROR_INSUFFICIENT_BUFFER;

        *size = sizeof(DWORD);
        *(DWORD*)buffer = INTERNET_HANDLE_TYPE_HTTP_REQUEST;
        return ERROR_SUCCESS;

    case INTERNET_OPTION_URL: {
        WCHAR *url;
        DWORD res;

        TRACE("INTERNET_OPTION_URL\n");

        url = compose_request_url(req);
        if(!url)
            return ERROR_OUTOFMEMORY;

        res = str_to_buffer(url, buffer, size, unicode);
        free(url);
        return res;
    }
    case INTERNET_OPTION_USER_AGENT:
        return str_to_buffer(req->session->appInfo->agent, buffer, size, unicode);
    case INTERNET_OPTION_USERNAME:
        return str_to_buffer(req->session->userName, buffer, size, unicode);
    case INTERNET_OPTION_PASSWORD:
        return str_to_buffer(req->session->password, buffer, size, unicode);
    case INTERNET_OPTION_PROXY_USERNAME:
        return str_to_buffer(req->session->appInfo->proxyUsername, buffer, size, unicode);
    case INTERNET_OPTION_PROXY_PASSWORD:
        return str_to_buffer(req->session->appInfo->proxyPassword, buffer, size, unicode);

    case INTERNET_OPTION_CACHE_TIMESTAMPS: {
        INTERNET_CACHE_ENTRY_INFOW *info;
        INTERNET_CACHE_TIMESTAMPS *ts = buffer;
        DWORD nbytes, error;
        BOOL ret;

        TRACE("INTERNET_OPTION_CACHE_TIMESTAMPS\n");

        if(!req->req_file)
            return ERROR_FILE_NOT_FOUND;

        if (*size < sizeof(*ts))
        {
            *size = sizeof(*ts);
            return ERROR_INSUFFICIENT_BUFFER;
        }

        nbytes = 0;
        ret = GetUrlCacheEntryInfoW(req->req_file->url, NULL, &nbytes);
        error = GetLastError();
        if (!ret && error == ERROR_INSUFFICIENT_BUFFER)
        {
            if (!(info = malloc(nbytes)))
                return ERROR_OUTOFMEMORY;

            GetUrlCacheEntryInfoW(req->req_file->url, info, &nbytes);

            ts->ftExpires = info->ExpireTime;
            ts->ftLastModified = info->LastModifiedTime;

            free(info);
            *size = sizeof(*ts);
            return ERROR_SUCCESS;
        }
        return error;
    }

    case INTERNET_OPTION_DATAFILE_NAME: {
        DWORD req_size;

        TRACE("INTERNET_OPTION_DATAFILE_NAME\n");

        if(!req->req_file) {
            *size = 0;
            return ERROR_INTERNET_ITEM_NOT_FOUND;
        }

        if(unicode) {
            req_size = (lstrlenW(req->req_file->file_name)+1) * sizeof(WCHAR);
            if(*size < req_size)
                return ERROR_INSUFFICIENT_BUFFER;

            *size = req_size;
            memcpy(buffer, req->req_file->file_name, *size);
            return ERROR_SUCCESS;
        }else {
            req_size = WideCharToMultiByte(CP_ACP, 0, req->req_file->file_name, -1, NULL, 0, NULL, NULL);
            if (req_size > *size)
                return ERROR_INSUFFICIENT_BUFFER;

            *size = WideCharToMultiByte(CP_ACP, 0, req->req_file->file_name,
                    -1, buffer, *size, NULL, NULL);
            return ERROR_SUCCESS;
        }
    }

    case INTERNET_OPTION_SECURITY_CERTIFICATE_STRUCT: {
        if(!req->netconn)
            return ERROR_INTERNET_INVALID_OPERATION;

        if(*size < sizeof(INTERNET_CERTIFICATE_INFOA)) {
            *size = sizeof(INTERNET_CERTIFICATE_INFOA);
            return ERROR_INSUFFICIENT_BUFFER;
        }

        return get_security_cert_struct(req, (INTERNET_CERTIFICATE_INFOA*)buffer);
    }
    case INTERNET_OPTION_SECURITY_CERTIFICATE: {
        DWORD err;
        int needed;
        char subject[64];
        char issuer[64];
        char effective[64];
        char expiration[64];
        char protocol[64];
        char signature[64];
        char encryption[64];
        char privacy[64];
        char bits[16];
        char strength[16];
        char start_date[32];
        char start_time[32];
        char expiry_date[32];
        char expiry_time[32];
        SYSTEMTIME start, expiry;
        INTERNET_CERTIFICATE_INFOA info;

        if(!size)
            return ERROR_INVALID_PARAMETER;

        if(!req->netconn) {
            *size = 0;
            return ERROR_INTERNET_INVALID_OPERATION;
        }

        if(!buffer) {
            *size = 1;
            return ERROR_INSUFFICIENT_BUFFER;
        }

        if((err = get_security_cert_struct(req, &info)))
            return err;

        LoadStringA(WININET_hModule, IDS_CERT_SUBJECT, subject, sizeof(subject));
        LoadStringA(WININET_hModule, IDS_CERT_ISSUER, issuer, sizeof(issuer));
        LoadStringA(WININET_hModule, IDS_CERT_EFFECTIVE, effective, sizeof(effective));
        LoadStringA(WININET_hModule, IDS_CERT_EXPIRATION, expiration, sizeof(expiration));
        LoadStringA(WININET_hModule, IDS_CERT_PROTOCOL, protocol, sizeof(protocol));
        LoadStringA(WININET_hModule, IDS_CERT_SIGNATURE, signature, sizeof(signature));
        LoadStringA(WININET_hModule, IDS_CERT_ENCRYPTION, encryption, sizeof(encryption));
        LoadStringA(WININET_hModule, IDS_CERT_PRIVACY, privacy, sizeof(privacy));
        LoadStringA(WININET_hModule, info.dwKeySize >= 128 ? IDS_CERT_HIGH : IDS_CERT_LOW,
                    strength, sizeof(strength));
        LoadStringA(WININET_hModule, IDS_CERT_BITS, bits, sizeof(bits));

        FileTimeToSystemTime(&info.ftStart, &start);
        FileTimeToSystemTime(&info.ftExpiry, &expiry);
        GetDateFormatA(LOCALE_USER_DEFAULT, 0, &start, NULL, start_date, sizeof(start_date));
        GetTimeFormatA(LOCALE_USER_DEFAULT, 0, &start, NULL, start_time, sizeof(start_time));
        GetDateFormatA(LOCALE_USER_DEFAULT, 0, &expiry, NULL, expiry_date, sizeof(expiry_date));
        GetTimeFormatA(LOCALE_USER_DEFAULT, 0, &expiry, NULL, expiry_time, sizeof(expiry_time));

        needed = _scprintf("%s:\r\n%s\r\n"
                           "%s:\r\n%s\r\n"
                           "%s:\t%s %s\r\n"
                           "%s:\t%s %s\r\n"
                           "%s:\t(null)\r\n"
                           "%s:\t(null)\r\n"
                           "%s:\t(null)\r\n"
                           "%s:\t%s (%lu %s)",
                           subject, info.lpszSubjectInfo,
                           issuer, info.lpszIssuerInfo,
                           effective, start_date, start_time,
                           expiration, expiry_date, expiry_time,
                           protocol, signature, encryption,
                           privacy, strength, info.dwKeySize, bits);

        if(needed < *size) {
            err = ERROR_SUCCESS;
            *size = snprintf(buffer, *size,
                           "%s:\r\n%s\r\n"
                           "%s:\r\n%s\r\n"
                           "%s:\t%s %s\r\n"
                           "%s:\t%s %s\r\n"
                           "%s:\t(null)\r\n"
                           "%s:\t(null)\r\n"
                           "%s:\t(null)\r\n"
                           "%s:\t%s (%lu %s)",
                           subject, info.lpszSubjectInfo,
                           issuer, info.lpszIssuerInfo,
                           effective, start_date, start_time,
                           expiration, expiry_date, expiry_time,
                           protocol, signature, encryption,
                           privacy, strength, info.dwKeySize, bits);
        }else {
            err = ERROR_INSUFFICIENT_BUFFER;
            *size = 1;
        }

        LocalFree(info.lpszSubjectInfo);
        LocalFree(info.lpszIssuerInfo);
        LocalFree(info.lpszProtocolName);
        LocalFree(info.lpszSignatureAlgName);
        LocalFree(info.lpszEncryptionAlgName);
        return err;
    }
    case INTERNET_OPTION_CONNECT_TIMEOUT:
        if (*size < sizeof(ULONG))
            return ERROR_INSUFFICIENT_BUFFER;

        *size = sizeof(ULONG);
        *(ULONG *)buffer = hdr->connect_timeout;
        return ERROR_SUCCESS;
    case INTERNET_OPTION_REQUEST_FLAGS: {
        DWORD flags = 0;

        if (*size < sizeof(DWORD))
            return ERROR_INSUFFICIENT_BUFFER;

        /* FIXME: Add support for:
         * INTERNET_REQFLAG_FROM_CACHE
         * INTERNET_REQFLAG_CACHE_WRITE_DISABLED
         */

        if(req->proxy)
            flags |= INTERNET_REQFLAG_VIA_PROXY;
        if(!req->status_code)
            flags |= INTERNET_REQFLAG_NO_HEADERS;

        TRACE("INTERNET_OPTION_REQUEST_FLAGS returning %lx\n", flags);

        *size = sizeof(DWORD);
        *(DWORD*)buffer = flags;
        return ERROR_SUCCESS;
    }
    case INTERNET_OPTION_ERROR_MASK:
        TRACE("INTERNET_OPTION_ERROR_MASK\n");

        if (*size < sizeof(ULONG))
            return ERROR_INSUFFICIENT_BUFFER;

        *(ULONG*)buffer = hdr->ErrorMask;
        *size = sizeof(ULONG);
        return ERROR_SUCCESS;
    case INTERNET_OPTION_SERVER_CERT_CHAIN_CONTEXT:
        TRACE("INTERNET_OPTION_SERVER_CERT_CHAIN_CONTEXT\n");

        if (*size < sizeof(PCCERT_CHAIN_CONTEXT))
            return ERROR_INSUFFICIENT_BUFFER;

        if (!req->server->cert_chain)
            return ERROR_INTERNET_INCORRECT_HANDLE_STATE;

        *(PCCERT_CHAIN_CONTEXT *)buffer = CertDuplicateCertificateChain(req->server->cert_chain);
        *size = sizeof(PCCERT_CHAIN_CONTEXT);

        return ERROR_SUCCESS;
    }

    return INET_QueryOption(hdr, option, buffer, size, unicode);
}

static DWORD HTTPREQ_SetOption(object_header_t *hdr, DWORD option, void *buffer, DWORD size)
{
    http_request_t *req = (http_request_t*)hdr;

    switch(option) {
    case 99: /* Undocumented, seems to be INTERNET_OPTION_SECURITY_FLAGS with argument validation */
        TRACE("Undocumented option 99\n");

        if (!buffer || size != sizeof(DWORD))
            return ERROR_INVALID_PARAMETER;
        if(*(DWORD*)buffer & ~SECURITY_SET_MASK)
            return ERROR_INTERNET_OPTION_NOT_SETTABLE;

        /* fall through */
    case INTERNET_OPTION_SECURITY_FLAGS:
    {
        DWORD flags;

        if (!buffer || size != sizeof(DWORD))
            return ERROR_INVALID_PARAMETER;
        flags = *(DWORD *)buffer;
        TRACE("INTERNET_OPTION_SECURITY_FLAGS %08lx\n", flags);
        flags &= SECURITY_SET_MASK;
        req->security_flags |= flags;
        if(is_valid_netconn(req->netconn))
            req->netconn->security_flags |= flags;
        return ERROR_SUCCESS;
    }

    case INTERNET_OPTION_USERNAME:
        free(req->session->userName);
        if (!(req->session->userName = wcsdup(buffer))) return ERROR_OUTOFMEMORY;
        return ERROR_SUCCESS;

    case INTERNET_OPTION_PASSWORD:
        free(req->session->password);
        if (!(req->session->password = wcsdup(buffer))) return ERROR_OUTOFMEMORY;
        return ERROR_SUCCESS;

    case INTERNET_OPTION_PROXY_USERNAME:
        free(req->session->appInfo->proxyUsername);
        if (!(req->session->appInfo->proxyUsername = wcsdup(buffer))) return ERROR_OUTOFMEMORY;
        return ERROR_SUCCESS;

    case INTERNET_OPTION_PROXY_PASSWORD:
        free(req->session->appInfo->proxyPassword);
        if (!(req->session->appInfo->proxyPassword = wcsdup(buffer))) return ERROR_OUTOFMEMORY;
        return ERROR_SUCCESS;

    }

    return INET_SetOption(hdr, option, buffer, size);
}

static void commit_cache_entry(http_request_t *req)
{
    WCHAR *header;
    DWORD header_len;
    BOOL res;

    TRACE("%p\n", req);

    CloseHandle(req->hCacheFile);
    req->hCacheFile = NULL;

    header = build_response_header(req, TRUE);
    header_len = (header ? lstrlenW(header) : 0);
    res = CommitUrlCacheEntryW(req->req_file->url, req->req_file->file_name, req->expires,
             req->last_modified, NORMAL_CACHE_ENTRY,
            header, header_len, NULL, 0);
    if(res)
        req->req_file->is_committed = TRUE;
    else
        WARN("CommitUrlCacheEntry failed: %lu\n", GetLastError());
    free(header);
}

static void create_cache_entry(http_request_t *req)
{
    WCHAR file_name[MAX_PATH+1];
    WCHAR *url;
    BOOL b = TRUE;

    /* FIXME: We should free previous cache file earlier */
    if(req->req_file) {
        req_file_release(req->req_file);
        req->req_file = NULL;
    }
    if(req->hCacheFile) {
        CloseHandle(req->hCacheFile);
        req->hCacheFile = NULL;
    }

    if(req->hdr.dwFlags & INTERNET_FLAG_NO_CACHE_WRITE)
        b = FALSE;

    if(b) {
        int header_idx;

        EnterCriticalSection( &req->headers_section );

        header_idx = HTTP_GetCustomHeaderIndex(req, L"Cache-Control", 0, FALSE);
        if(header_idx != -1) {
            WCHAR *ptr;

            for(ptr=req->custHeaders[header_idx].lpszValue; *ptr; ) {
                WCHAR *end;

                while(*ptr==' ' || *ptr=='\t')
                    ptr++;

                end = wcschr(ptr, ',');
                if(!end)
                    end = ptr + lstrlenW(ptr);

                if(!wcsnicmp(ptr, L"no-cache", ARRAY_SIZE(L"no-cache")-1)
                        || !wcsnicmp(ptr, L"no-store", ARRAY_SIZE(L"no-store")-1)) {
                    b = FALSE;
                    break;
                }

                ptr = end;
                if(*ptr == ',')
                    ptr++;
            }
        }

        LeaveCriticalSection( &req->headers_section );
    }

    if(!b) {
        if(!(req->hdr.dwFlags & INTERNET_FLAG_NEED_FILE))
            return;

        FIXME("INTERNET_FLAG_NEED_FILE is not supported correctly\n");
    }

    url = compose_request_url(req);
    if(!url) {
        WARN("Could not get URL\n");
        return;
    }

    b = CreateUrlCacheEntryW(url, req->contentLength == ~0 ? 0 : req->contentLength, NULL, file_name, 0);
    if(!b) {
        WARN("Could not create cache entry: %08lx\n", GetLastError());
        return;
    }

    create_req_file(file_name, &req->req_file);
    req->req_file->url = url;
    req->content_pos = 0;
    req->cache_size = 0;

    req->hCacheFile = CreateFileW(file_name, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
              NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if(req->hCacheFile == INVALID_HANDLE_VALUE) {
        WARN("Could not create file: %lu\n", GetLastError());
        req->hCacheFile = NULL;
        return;
    }

    if(req->read_size) {
        DWORD written;

        b = WriteFile(req->hCacheFile, req->read_buf+req->read_pos, req->read_size, &written, NULL);
        if(b)
            req->cache_size += written;
        else
            FIXME("WriteFile failed: %lu\n", GetLastError());

        if(req->data_stream->vtbl->end_of_data(req->data_stream, req))
            commit_cache_entry(req);
    }
}

/* read some more data into the read buffer (the read section must be held) */
static DWORD read_more_data( http_request_t *req, int maxlen )
{
    DWORD res;
    int len;

    if (req->read_pos)
    {
        /* move existing data to the start of the buffer */
        if(req->read_size)
            memmove( req->read_buf, req->read_buf + req->read_pos, req->read_size );
        req->read_pos = 0;
    }

    if (maxlen == -1) maxlen = sizeof(req->read_buf);

    res = NETCON_recv( req->netconn, req->read_buf + req->read_size,
                       maxlen - req->read_size, TRUE, &len );
    if(res == ERROR_SUCCESS)
        req->read_size += len;

    return res;
}

/* remove some amount of data from the read buffer (the read section must be held) */
static void remove_data( http_request_t *req, int count )
{
    if (!(req->read_size -= count)) req->read_pos = 0;
    else req->read_pos += count;
}

static DWORD read_line( http_request_t *req, LPSTR buffer, DWORD *len )
{
    int count, bytes_read, pos = 0;
    DWORD res;

    EnterCriticalSection( &req->read_section );
    for (;;)
    {
        BYTE *eol = memchr( req->read_buf + req->read_pos, '\n', req->read_size );

        if (eol)
        {
            count = eol - (req->read_buf + req->read_pos);
            bytes_read = count + 1;
        }
        else count = bytes_read = req->read_size;

        count = min( count, *len - pos );
        memcpy( buffer + pos, req->read_buf + req->read_pos, count );
        pos += count;
        remove_data( req, bytes_read );
        if (eol) break;

        if ((res = read_more_data( req, -1 )))
        {
            WARN( "read failed %lu\n", res );
            LeaveCriticalSection( &req->read_section );
            return res;
        }
        if (!req->read_size)
        {
            *len = 0;
            TRACE( "returning empty string\n" );
            LeaveCriticalSection( &req->read_section );
            return ERROR_SUCCESS;
        }
    }
    LeaveCriticalSection( &req->read_section );

    if (pos < *len)
    {
        if (pos && buffer[pos - 1] == '\r') pos--;
        *len = pos + 1;
    }
    buffer[*len - 1] = 0;
    TRACE( "returning %s\n", debugstr_a(buffer));
    return ERROR_SUCCESS;
}

/* check if we have reached the end of the data to read (the read section must be held) */
static BOOL end_of_read_data( http_request_t *req )
{
    return !req->read_size && req->data_stream->vtbl->end_of_data(req->data_stream, req);
}

static DWORD read_http_stream(http_request_t *req, BYTE *buf, DWORD size, DWORD *read, BOOL allow_blocking)
{
    DWORD res;

    res = req->data_stream->vtbl->read(req->data_stream, req, buf, size, read, allow_blocking);
    if(res != ERROR_SUCCESS)
        *read = 0;
    assert(*read <= size);

    if(req->hCacheFile) {
        if(*read) {
            BOOL bres;
            DWORD written;

            bres = WriteFile(req->hCacheFile, buf, *read, &written, NULL);
            if(bres)
                req->cache_size += written;
            else
                FIXME("WriteFile failed: %lu\n", GetLastError());
        }

        if((res == ERROR_SUCCESS && !*read) || req->data_stream->vtbl->end_of_data(req->data_stream, req))
            commit_cache_entry(req);
    }

    return res;
}

/* fetch some more data into the read buffer (the read section must be held) */
static DWORD refill_read_buffer(http_request_t *req, BOOL allow_blocking, DWORD *read_bytes)
{
    DWORD res, read=0;

    if(req->read_size == sizeof(req->read_buf))
        return ERROR_SUCCESS;

    if(req->read_pos) {
        if(req->read_size)
            memmove(req->read_buf, req->read_buf+req->read_pos, req->read_size);
        req->read_pos = 0;
    }

    res = read_http_stream(req, req->read_buf+req->read_size, sizeof(req->read_buf) - req->read_size,
            &read, allow_blocking);
    if(res != ERROR_SUCCESS)
        return res;

    req->read_size += read;

    TRACE("read %lu bytes, read_size %lu\n", read, req->read_size);
    if(read_bytes)
        *read_bytes = read;
    return res;
}

static BOOL netconn_end_of_data(data_stream_t *stream, http_request_t *req)
{
    netconn_stream_t *netconn_stream = (netconn_stream_t*)stream;
    return netconn_stream->content_read == netconn_stream->content_length || !is_valid_netconn(req->netconn);
}

static DWORD netconn_read(data_stream_t *stream, http_request_t *req, BYTE *buf, DWORD size,
        DWORD *read, BOOL allow_blocking)
{
    netconn_stream_t *netconn_stream = (netconn_stream_t*)stream;
    DWORD res = ERROR_SUCCESS;
    int ret = 0;

    size = min(size, netconn_stream->content_length-netconn_stream->content_read);

    if(size && is_valid_netconn(req->netconn)) {
        res = NETCON_recv(req->netconn, buf, size, allow_blocking, &ret);
        if(res == ERROR_SUCCESS) {
            if(!ret)
                netconn_stream->content_length = netconn_stream->content_read;
            netconn_stream->content_read += ret;
        }
    }

    TRACE("res %lu read %u bytes\n", res, ret);
    *read = ret;
    return res;
}

static DWORD netconn_drain_content(data_stream_t *stream, http_request_t *req, BOOL allow_blocking)
{
    netconn_stream_t *netconn_stream = (netconn_stream_t*)stream;
    BYTE buf[1024];
    int len, res;
    size_t size;

    if(netconn_stream->content_length == ~0)
        return WSAEISCONN;

    while(netconn_stream->content_read < netconn_stream->content_length) {
        size = min(sizeof(buf), netconn_stream->content_length-netconn_stream->content_read);
        res = NETCON_recv(req->netconn, buf, size, allow_blocking, &len);
        if(res)
            return res;
        if(!len)
            return WSAECONNABORTED;

        netconn_stream->content_read += len;
    }

    return ERROR_SUCCESS;
}

static void netconn_destroy(data_stream_t *stream)
{
}

static const data_stream_vtbl_t netconn_stream_vtbl = {
    netconn_end_of_data,
    netconn_read,
    netconn_drain_content,
    netconn_destroy
};

static char next_chunked_data_char(chunked_stream_t *stream)
{
    assert(stream->buf_size);

    stream->buf_size--;
    return stream->buf[stream->buf_pos++];
}

static BOOL chunked_end_of_data(data_stream_t *stream, http_request_t *req)
{
    chunked_stream_t *chunked_stream = (chunked_stream_t*)stream;
    switch(chunked_stream->state) {
    case CHUNKED_STREAM_STATE_DISCARD_EOL_AT_END:
    case CHUNKED_STREAM_STATE_END_OF_STREAM:
    case CHUNKED_STREAM_STATE_ERROR:
        return TRUE;
    default:
        return FALSE;
    }
}

static DWORD chunked_read(data_stream_t *stream, http_request_t *req, BYTE *buf, DWORD size,
        DWORD *read, BOOL allow_blocking)
{
    chunked_stream_t *chunked_stream = (chunked_stream_t*)stream;
    DWORD ret_read = 0, res = ERROR_SUCCESS;
    BOOL continue_read = TRUE;
    int read_bytes;
    char ch;

    do {
        TRACE("state %d\n", chunked_stream->state);

        /* Ensure that we have data in the buffer for states that need it. */
        if(!chunked_stream->buf_size) {
            BOOL blocking_read = allow_blocking;

            switch(chunked_stream->state) {
            case CHUNKED_STREAM_STATE_DISCARD_EOL_AT_END:
            case CHUNKED_STREAM_STATE_DISCARD_EOL_AFTER_SIZE:
                /* never allow blocking after 0 chunk size */
                if(!chunked_stream->chunk_size)
                    blocking_read = FALSE;
                /* fall through */
            case CHUNKED_STREAM_STATE_READING_CHUNK_SIZE:
            case CHUNKED_STREAM_STATE_DISCARD_EOL_AFTER_DATA:
                chunked_stream->buf_pos = 0;
                res = NETCON_recv(req->netconn, chunked_stream->buf, sizeof(chunked_stream->buf), blocking_read, &read_bytes);
                if(res == ERROR_SUCCESS && read_bytes) {
                    chunked_stream->buf_size += read_bytes;
                }else if(res == WSAEWOULDBLOCK) {
                    if(ret_read || allow_blocking)
                        res = ERROR_SUCCESS;
                    continue_read = FALSE;
                    continue;
                }else {
                    chunked_stream->state = CHUNKED_STREAM_STATE_ERROR;
                }
                break;
            default:
                break;
            }
        }

        switch(chunked_stream->state) {
        case CHUNKED_STREAM_STATE_READING_CHUNK_SIZE:
            ch = next_chunked_data_char(chunked_stream);

            if(ch >= '0' && ch <= '9') {
                chunked_stream->chunk_size = chunked_stream->chunk_size * 16 + ch - '0';
            }else if(ch >= 'a' && ch <= 'f') {
                chunked_stream->chunk_size = chunked_stream->chunk_size * 16 + ch - 'a' + 10;
            }else if (ch >= 'A' && ch <= 'F') {
                chunked_stream->chunk_size = chunked_stream->chunk_size * 16 + ch - 'A' + 10;
            }else if (ch == ';' || ch == '\r' || ch == '\n') {
                TRACE("reading %lu byte chunk\n", chunked_stream->chunk_size);
                chunked_stream->buf_size++;
                chunked_stream->buf_pos--;
                if(req->contentLength == ~0) req->contentLength = chunked_stream->chunk_size;
                else req->contentLength += chunked_stream->chunk_size;
                chunked_stream->state = CHUNKED_STREAM_STATE_DISCARD_EOL_AFTER_SIZE;
            }
            break;

        case CHUNKED_STREAM_STATE_DISCARD_EOL_AFTER_SIZE:
            ch = next_chunked_data_char(chunked_stream);
            if(ch == '\n')
                chunked_stream->state = chunked_stream->chunk_size
                    ? CHUNKED_STREAM_STATE_READING_CHUNK
                    : CHUNKED_STREAM_STATE_DISCARD_EOL_AT_END;
            else if(ch != '\r')
                WARN("unexpected char '%c'\n", ch);
            break;

        case CHUNKED_STREAM_STATE_READING_CHUNK:
            assert(chunked_stream->chunk_size);
            if(!size) {
                continue_read = FALSE;
                break;
            }
            read_bytes = min(size, chunked_stream->chunk_size);

            if(chunked_stream->buf_size) {
                if(read_bytes > chunked_stream->buf_size)
                    read_bytes = chunked_stream->buf_size;

                memcpy(buf+ret_read, chunked_stream->buf+chunked_stream->buf_pos, read_bytes);
                chunked_stream->buf_pos += read_bytes;
                chunked_stream->buf_size -= read_bytes;
            }else {
                res = NETCON_recv(req->netconn, (char*)buf+ret_read, read_bytes,
                                  allow_blocking, (int*)&read_bytes);
                if(res != ERROR_SUCCESS) {
                    continue_read = FALSE;
                    break;
                }

                if(!read_bytes) {
                    chunked_stream->state = CHUNKED_STREAM_STATE_ERROR;
                    continue;
                }
            }

            chunked_stream->chunk_size -= read_bytes;
            size -= read_bytes;
            ret_read += read_bytes;
            if(!chunked_stream->chunk_size)
                chunked_stream->state = CHUNKED_STREAM_STATE_DISCARD_EOL_AFTER_DATA;
            allow_blocking = FALSE;
            break;

        case CHUNKED_STREAM_STATE_DISCARD_EOL_AFTER_DATA:
            ch = next_chunked_data_char(chunked_stream);
            if(ch == '\n')
                chunked_stream->state = CHUNKED_STREAM_STATE_READING_CHUNK_SIZE;
            else if(ch != '\r')
                WARN("unexpected char '%c'\n", ch);
            break;

        case CHUNKED_STREAM_STATE_DISCARD_EOL_AT_END:
            ch = next_chunked_data_char(chunked_stream);
            if(ch == '\n')
                chunked_stream->state = CHUNKED_STREAM_STATE_END_OF_STREAM;
            else if(ch != '\r')
                WARN("unexpected char '%c'\n", ch);
            break;

        case CHUNKED_STREAM_STATE_END_OF_STREAM:
        case CHUNKED_STREAM_STATE_ERROR:
            continue_read = FALSE;
            break;
        }
    } while(continue_read);

    if(ret_read)
        res = ERROR_SUCCESS;
    if(res != ERROR_SUCCESS)
        return res;

    TRACE("read %ld bytes\n", ret_read);
    *read = ret_read;
    return ERROR_SUCCESS;
}

static DWORD chunked_drain_content(data_stream_t *stream, http_request_t *req, BOOL allow_blocking)
{
    chunked_stream_t *chunked_stream = (chunked_stream_t*)stream;
    BYTE buf[1024];
    DWORD size, res;

    while(chunked_stream->state != CHUNKED_STREAM_STATE_END_OF_STREAM
          && chunked_stream->state != CHUNKED_STREAM_STATE_ERROR) {
        res = chunked_read(stream, req, buf, sizeof(buf), &size, allow_blocking);
        if(res != ERROR_SUCCESS)
            return res;
    }

    if(chunked_stream->state != CHUNKED_STREAM_STATE_END_OF_STREAM)
        return ERROR_NO_DATA;
    return ERROR_SUCCESS;
}

static void chunked_destroy(data_stream_t *stream)
{
    chunked_stream_t *chunked_stream = (chunked_stream_t*)stream;
    free(chunked_stream);
}

static const data_stream_vtbl_t chunked_stream_vtbl = {
    chunked_end_of_data,
    chunked_read,
    chunked_drain_content,
    chunked_destroy
};

/* set the request content length based on the headers */
static DWORD set_content_length(http_request_t *request)
{
    WCHAR contentLength[32];
    WCHAR encoding[20];
    DWORD size;

    if(request->status_code == HTTP_STATUS_NO_CONTENT || request->status_code == HTTP_STATUS_NOT_MODIFIED ||
       !wcscmp(request->verb, L"HEAD"))
    {
        if (HTTP_GetCustomHeaderIndex(request, L"Content-Length", 0, FALSE) == -1 ||
            !wcscmp(request->verb, L"HEAD"))
            request->read_size = 0;
        request->contentLength = request->netconn_stream.content_length = 0;
        return ERROR_SUCCESS;
    }

    size = sizeof(contentLength);
    if (HTTP_HttpQueryInfoW(request, HTTP_QUERY_CONTENT_LENGTH,
                            contentLength, &size, NULL) != ERROR_SUCCESS ||
        !StrToInt64ExW(contentLength, STIF_DEFAULT, (LONGLONG*)&request->contentLength)) {
        request->contentLength = ~0;
    }

    request->netconn_stream.content_length = request->contentLength;
    request->netconn_stream.content_read = request->read_size;

    size = sizeof(encoding);
    if (HTTP_HttpQueryInfoW(request, HTTP_QUERY_TRANSFER_ENCODING, encoding, &size, NULL) == ERROR_SUCCESS &&
        !wcsicmp(encoding, L"chunked"))
    {
        chunked_stream_t *chunked_stream;

        chunked_stream = malloc(sizeof(*chunked_stream));
        if(!chunked_stream)
            return ERROR_OUTOFMEMORY;

        chunked_stream->data_stream.vtbl = &chunked_stream_vtbl;
        chunked_stream->buf_size = chunked_stream->buf_pos = 0;
        chunked_stream->chunk_size = 0;
        chunked_stream->state = CHUNKED_STREAM_STATE_READING_CHUNK_SIZE;

        if(request->read_size) {
            memcpy(chunked_stream->buf, request->read_buf+request->read_pos, request->read_size);
            chunked_stream->buf_size = request->read_size;
            request->read_size = request->read_pos = 0;
        }

        request->data_stream = &chunked_stream->data_stream;
        request->contentLength = ~0;
    }

    if(request->hdr.decoding) {
        int encoding_idx;

        EnterCriticalSection( &request->headers_section );

        encoding_idx = HTTP_GetCustomHeaderIndex(request, L"Content-Encoding", 0, FALSE);
        if(encoding_idx != -1) {
            if(!wcsicmp(request->custHeaders[encoding_idx].lpszValue, L"gzip")) {
                HTTP_DeleteCustomHeader(request, encoding_idx);
                LeaveCriticalSection( &request->headers_section );
                return init_gzip_stream(request, TRUE);
            }
            if(!wcsicmp(request->custHeaders[encoding_idx].lpszValue, L"deflate")) {
                HTTP_DeleteCustomHeader(request, encoding_idx);
                LeaveCriticalSection( &request->headers_section );
                return init_gzip_stream(request, FALSE);
            }
        }

        LeaveCriticalSection( &request->headers_section );
    }

    return ERROR_SUCCESS;
}

static void send_request_complete(http_request_t *req, DWORD_PTR result, DWORD error)
{
    INTERNET_ASYNC_RESULT iar;

    iar.dwResult = result;
    iar.dwError = error;

    INTERNET_SendCallback(&req->hdr, req->hdr.dwContext, INTERNET_STATUS_REQUEST_COMPLETE, &iar,
            sizeof(INTERNET_ASYNC_RESULT));
}

static void HTTP_ReceiveRequestData(http_request_t *req)
{
    DWORD res, read = 0;

    TRACE("%p\n", req);

    EnterCriticalSection( &req->read_section );

    res = refill_read_buffer(req, FALSE, &read);
    if(res == ERROR_SUCCESS)
        read += req->read_size;

    LeaveCriticalSection( &req->read_section );

    if(res != WSAEWOULDBLOCK && (res != ERROR_SUCCESS || !read)) {
        WARN("res %lu read %lu, closing connection\n", res, read);
        http_release_netconn(req, FALSE);
    }

    if(res != ERROR_SUCCESS && res != WSAEWOULDBLOCK) {
        send_request_complete(req, 0, res);
        return;
    }

    send_request_complete(req, req->session->hdr.dwInternalFlags & INET_OPENURL ? (DWORD_PTR)req->hdr.hInternet : 1, 0);
}

static DWORD read_req_file(http_request_t *req, BYTE *buffer, DWORD size, DWORD *read, BOOL allow_blocking)
{
    DWORD ret_read = 0, res;
    LARGE_INTEGER off;
    BYTE buf[1024];

    while (req->content_pos > req->cache_size) {
        res = read_http_stream(req, (BYTE*)buf, min(sizeof(buf), req->content_pos - req->cache_size),
                               &ret_read, allow_blocking);
        if (res != ERROR_SUCCESS)
            return res;
        if (!ret_read) {
            *read = 0;
            return ERROR_SUCCESS;
        }
    }

    if (req->content_pos < req->cache_size) {
        off.QuadPart = req->content_pos;
        if (!SetFilePointerEx(req->req_file->file_handle, off, NULL, FILE_BEGIN))
            return GetLastError();
        if (!ReadFile(req->req_file->file_handle, buffer, size, &ret_read, NULL))
            return GetLastError();
    }

    *read = ret_read;
    return ERROR_SUCCESS;
}

/* read data from the http connection (the read section must be held) */
static DWORD HTTPREQ_Read(http_request_t *req, void *buffer, DWORD size, DWORD *read, BOOL allow_blocking)
{
    DWORD current_read = 0, ret_read = 0;
    DWORD res = ERROR_SUCCESS;

    EnterCriticalSection( &req->read_section );

    if(req->read_size) {
        ret_read = min(size, req->read_size);
        memcpy(buffer, req->read_buf+req->read_pos, ret_read);
        req->read_size -= ret_read;
        req->read_pos += ret_read;
        req->content_pos += ret_read;
        allow_blocking = FALSE;
    }

    if(ret_read < size) {
        res = read_http_stream(req, (BYTE*)buffer+ret_read, size-ret_read, &current_read, allow_blocking);
        if(res == ERROR_SUCCESS) {
            ret_read += current_read;
            req->content_pos += current_read;
        }
        else if(res == WSAEWOULDBLOCK && ret_read)
            res = ERROR_SUCCESS;
    }

    LeaveCriticalSection( &req->read_section );

    *read = ret_read;
    TRACE( "retrieved %lu bytes (res %lu)\n", ret_read, res );

    if(res != WSAEWOULDBLOCK) {
        if(res != ERROR_SUCCESS)
            http_release_netconn(req, FALSE);
        else if(!ret_read && drain_content(req, FALSE) == ERROR_SUCCESS)
            http_release_netconn(req, TRUE);
    }

    return res;
}

static DWORD drain_content(http_request_t *req, BOOL blocking)
{
    DWORD res;

    TRACE("%p\n", req->netconn);

    if(!is_valid_netconn(req->netconn))
        return ERROR_NO_DATA;

    if(!wcscmp(req->verb, L"HEAD"))
        return ERROR_SUCCESS;

    EnterCriticalSection( &req->read_section );
    res = req->data_stream->vtbl->drain_content(req->data_stream, req, blocking);
    LeaveCriticalSection( &req->read_section );
    return res;
}

typedef struct {
    task_header_t hdr;
    void *buf;
    DWORD size;
    DWORD read_pos;
    DWORD *ret_read;
} read_file_task_t;

static void async_read_file_proc(task_header_t *hdr)
{
    read_file_task_t *task = (read_file_task_t*)hdr;
    http_request_t *req = (http_request_t*)task->hdr.hdr;
    DWORD res = ERROR_SUCCESS, read = task->read_pos, complete_arg = 0;

    TRACE("req %p buf %p size %lu read_pos %lu ret_read %p\n", req, task->buf, task->size, task->read_pos, task->ret_read);

    if(req->req_file && req->req_file->file_handle) {
        DWORD ret, ret_read;
        BYTE buf[1024];
        while (req->content_pos > req->cache_size) {
            ret = read_http_stream(req, (BYTE*)buf, min(sizeof(buf), req->content_pos - req->cache_size),
                                   &ret_read, TRUE);
            if(ret != ERROR_SUCCESS || !ret_read)
                break;
        }
    }

    if(task->buf) {
        DWORD read_bytes;
        while (read < task->size) {
            res = HTTPREQ_Read(req, (char*)task->buf + read, task->size - read, &read_bytes, TRUE);
            if (res != ERROR_SUCCESS || !read_bytes)
                break;
            read += read_bytes;
        }
    }else {
        EnterCriticalSection(&req->read_section);
        res = refill_read_buffer(req, TRUE, &read);
        LeaveCriticalSection(&req->read_section);

        if(task->ret_read)
            complete_arg = read; /* QueryDataAvailable reports read bytes in request complete notification */
        if(res != ERROR_SUCCESS || !read)
            http_release_netconn(req, drain_content(req, FALSE) == ERROR_SUCCESS);
    }

    TRACE("res %lu read %lu\n", res, read);

    if(task->ret_read)
        *task->ret_read = read;

    /* FIXME: We should report bytes transferred before decoding content. */
    INTERNET_SendCallback(&req->hdr, req->hdr.dwContext, INTERNET_STATUS_RESPONSE_RECEIVED, &read, sizeof(read));

    if(res != ERROR_SUCCESS)
        complete_arg = res;
    send_request_complete(req, res == ERROR_SUCCESS, complete_arg);
}

static DWORD async_read(http_request_t *req, void *buf, DWORD size, DWORD read_pos, DWORD *ret_read)
{
    read_file_task_t *task;

    task = alloc_async_task(&req->hdr, async_read_file_proc, sizeof(*task));
    if(!task)
        return ERROR_OUTOFMEMORY;

    task->buf = buf;
    task->size = size;
    task->read_pos = read_pos;
    task->ret_read = ret_read;

    INTERNET_AsyncCall(&task->hdr);
    return ERROR_IO_PENDING;
}

static DWORD HTTPREQ_SetFilePointer(object_header_t *hdr, LONG lDistanceToMove, DWORD dwMoveContext)
{
    http_request_t *req = (http_request_t*)hdr;
    DWORD res = INVALID_SET_FILE_POINTER, err = ERROR_SUCCESS;

    if(req->hdr.dwFlags & (INTERNET_FLAG_DONT_CACHE|INTERNET_FLAG_NO_CACHE_WRITE)) {
        SetLastError(ERROR_INTERNET_INVALID_OPERATION);
        return INVALID_SET_FILE_POINTER;
    }

    EnterCriticalSection(&req->read_section);

    switch (dwMoveContext) {
        case FILE_BEGIN:
            res = lDistanceToMove;
            break;
        case FILE_CURRENT:
            if(req->content_pos && lDistanceToMove < 0) {
                err = ERROR_NEGATIVE_SEEK;
                break;
            }
            res = req->content_pos + lDistanceToMove;
            break;
        case FILE_END:
            FIXME("dwMoveContext FILE_END not implemented\n");
            /* fallthrough */
        default:
            err = ERROR_INTERNET_INVALID_OPERATION;
            break;
    }

    if(err == ERROR_SUCCESS) {
        req->content_pos = res;
        req->read_pos = req->read_size = 0;
    }

    LeaveCriticalSection(&req->read_section);
    SetLastError(err);
    return res;
}

static DWORD HTTPREQ_ReadFile(object_header_t *hdr, void *buf, DWORD size, DWORD *ret_read,
        DWORD flags, DWORD_PTR context)
{
    http_request_t *req = (http_request_t*)hdr;
    DWORD res = ERROR_SUCCESS, read = 0, cread, error = ERROR_SUCCESS;
    BOOL allow_blocking, notify_received = FALSE;

    TRACE("(%p %p %lu %lx)\n", req, buf, size, flags);

    if (flags & ~(IRF_ASYNC|IRF_NO_WAIT))
        FIXME("these dwFlags aren't implemented: 0x%lx\n", flags & ~(IRF_ASYNC|IRF_NO_WAIT));

    allow_blocking = !(req->session->appInfo->hdr.dwFlags & INTERNET_FLAG_ASYNC);

    if(allow_blocking || TryEnterCriticalSection(&req->read_section)) {
        if(allow_blocking)
            EnterCriticalSection(&req->read_section);
        if(hdr->dwError == ERROR_SUCCESS)
            hdr->dwError = INTERNET_HANDLE_IN_USE;
        else if(hdr->dwError == INTERNET_HANDLE_IN_USE)
            hdr->dwError = ERROR_INTERNET_INTERNAL_ERROR;

        if(req->read_size) {
            read = min(size, req->read_size);
            memcpy(buf, req->read_buf + req->read_pos, read);
            req->read_size -= read;
            req->read_pos += read;
            req->content_pos += read;
        }

        if(read < size && req->req_file && req->req_file->file_handle) {
            res = read_req_file(req, (BYTE*)buf + read, size - read, &cread, allow_blocking);
            if(res == ERROR_SUCCESS) {
                read += cread;
                req->content_pos += cread;
            }
        }

        if(res == ERROR_SUCCESS && read < size && (!read || !(flags & IRF_NO_WAIT)) && !end_of_read_data(req)) {
            LeaveCriticalSection(&req->read_section);
            INTERNET_SendCallback(&req->hdr, req->hdr.dwContext, INTERNET_STATUS_RECEIVING_RESPONSE, NULL, 0);
            EnterCriticalSection( &req->read_section );
            notify_received = TRUE;

            while(read < size) {
                res = HTTPREQ_Read(req, (char*)buf+read, size-read, &cread, allow_blocking);
                read += cread;
                if (res != ERROR_SUCCESS || !cread)
                    break;
            }
        }

        if(hdr->dwError == INTERNET_HANDLE_IN_USE)
            hdr->dwError = ERROR_SUCCESS;
        else
            error = hdr->dwError;

        LeaveCriticalSection( &req->read_section );
    }else {
        res = WSAEWOULDBLOCK;
    }

    if(res == WSAEWOULDBLOCK) {
        if(!(flags & IRF_NO_WAIT))
            return async_read(req, buf, size, read, ret_read);
        if(!read)
            return async_read(req, NULL, 0, 0, NULL);
        res = ERROR_SUCCESS;
    }

    *ret_read = read;
    if (res != ERROR_SUCCESS)
        return res;

    if(notify_received)
        INTERNET_SendCallback(&req->hdr, req->hdr.dwContext, INTERNET_STATUS_RESPONSE_RECEIVED,
                &read, sizeof(read));
    return error;
}

static DWORD HTTPREQ_WriteFile(object_header_t *hdr, const void *buffer, DWORD size, DWORD *written)
{
    DWORD res;
    http_request_t *request = (http_request_t*)hdr;

    INTERNET_SendCallback(&request->hdr, request->hdr.dwContext, INTERNET_STATUS_SENDING_REQUEST, NULL, 0);

    *written = 0;
    res = NETCON_send(request->netconn, buffer, size, 0, (LPINT)written);
    if (res == ERROR_SUCCESS)
        request->bytesWritten += *written;

    INTERNET_SendCallback(&request->hdr, request->hdr.dwContext, INTERNET_STATUS_REQUEST_SENT, written, sizeof(DWORD));
    return res;
}

static DWORD HTTPREQ_QueryDataAvailable(object_header_t *hdr, DWORD *available, DWORD flags, DWORD_PTR ctx)
{
    http_request_t *req = (http_request_t*)hdr;
    DWORD res = ERROR_SUCCESS, avail = 0, error = ERROR_SUCCESS;
    BOOL allow_blocking, notify_received = FALSE;

    TRACE("(%p %p %lx %Ix)\n", req, available, flags, ctx);

    if (flags & ~(IRF_ASYNC|IRF_NO_WAIT))
        FIXME("these dwFlags aren't implemented: 0x%lx\n", flags & ~(IRF_ASYNC|IRF_NO_WAIT));

    *available = 0;
    allow_blocking = !(req->session->appInfo->hdr.dwFlags & INTERNET_FLAG_ASYNC);

    if(allow_blocking || TryEnterCriticalSection(&req->read_section)) {
        if(allow_blocking)
            EnterCriticalSection(&req->read_section);
        if(hdr->dwError == ERROR_SUCCESS)
            hdr->dwError = INTERNET_HANDLE_IN_USE;
        else if(hdr->dwError == INTERNET_HANDLE_IN_USE)
            hdr->dwError = ERROR_INTERNET_INTERNAL_ERROR;

        avail = req->read_size;
        if(req->cache_size > req->content_pos)
            avail = max(avail, req->cache_size - req->content_pos);

        if(!avail && !end_of_read_data(req)) {
            LeaveCriticalSection(&req->read_section);
            INTERNET_SendCallback(&req->hdr, req->hdr.dwContext, INTERNET_STATUS_RECEIVING_RESPONSE, NULL, 0);
            EnterCriticalSection( &req->read_section );
            notify_received = TRUE;

            res = refill_read_buffer(req, allow_blocking, &avail);
        }

        if(hdr->dwError == INTERNET_HANDLE_IN_USE)
            hdr->dwError = ERROR_SUCCESS;
        else
            error = hdr->dwError;

        LeaveCriticalSection( &req->read_section );
    }else {
        res = WSAEWOULDBLOCK;
    }

    if(res == WSAEWOULDBLOCK)
        return async_read(req, NULL, 0, 0, available);

    if (res != ERROR_SUCCESS)
        return res;

    *available = avail;
    if(notify_received)
        INTERNET_SendCallback(&req->hdr, req->hdr.dwContext, INTERNET_STATUS_RESPONSE_RECEIVED,
                &avail, sizeof(avail));
    return error;
}

static DWORD HTTPREQ_LockRequestFile(object_header_t *hdr, req_file_t **ret)
{
    http_request_t *req = (http_request_t*)hdr;

    TRACE("(%p)\n", req);

    if(!req->req_file) {
        WARN("No cache file name available\n");
        return ERROR_FILE_NOT_FOUND;
    }

    *ret = req_file_addref(req->req_file);
    return ERROR_SUCCESS;
}

static const object_vtbl_t HTTPREQVtbl = {
    HTTPREQ_Destroy,
    HTTPREQ_CloseConnection,
    HTTPREQ_QueryOption,
    HTTPREQ_SetOption,
    HTTPREQ_SetFilePointer,
    HTTPREQ_ReadFile,
    HTTPREQ_WriteFile,
    HTTPREQ_QueryDataAvailable,
    NULL,
    HTTPREQ_LockRequestFile
};

/***********************************************************************
 *           HTTP_HttpOpenRequestW (internal)
 *
 * Open a HTTP request handle
 *
 * RETURNS
 *    HINTERNET  a HTTP request handle on success
 *    NULL 	 on failure
 *
 */
static DWORD HTTP_HttpOpenRequestW(http_session_t *session,
        LPCWSTR lpszVerb, LPCWSTR lpszObjectName, LPCWSTR lpszVersion,
        LPCWSTR lpszReferrer , LPCWSTR *lpszAcceptTypes,
        DWORD dwFlags, DWORD_PTR dwContext, HINTERNET *ret)
{
    appinfo_t *hIC = session->appInfo;
    http_request_t *request;
    DWORD port, len;

    TRACE("-->\n");

    request = alloc_object(&session->hdr, &HTTPREQVtbl, sizeof(http_request_t));
    if(!request)
        return ERROR_OUTOFMEMORY;

    request->hdr.htype = WH_HHTTPREQ;
    request->hdr.dwFlags = dwFlags;
    request->hdr.dwContext = dwContext;
    request->hdr.decoding = session->hdr.decoding;
    request->contentLength = ~0;

    request->netconn_stream.data_stream.vtbl = &netconn_stream_vtbl;
    request->data_stream = &request->netconn_stream.data_stream;
    request->hdr.connect_timeout = session->hdr.connect_timeout;
    request->hdr.send_timeout = session->hdr.send_timeout;
    request->hdr.receive_timeout = session->hdr.receive_timeout;

    InitializeCriticalSectionEx( &request->headers_section, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO );
    request->headers_section.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": http_request_t.headers_section");

    InitializeCriticalSectionEx( &request->read_section, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO );
    request->read_section.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": http_request_t.read_section");

    WININET_AddRef( &session->hdr );
    request->session = session;
    list_add_head( &session->hdr.children, &request->hdr.entry );

    port = session->hostPort;
    if (port == INTERNET_INVALID_PORT_NUMBER)
        port = (session->hdr.dwFlags & INTERNET_FLAG_SECURE) ?
                INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;

    request->server = get_server(substrz(session->hostName), port, (dwFlags & INTERNET_FLAG_SECURE) != 0, TRUE);
    if(!request->server) {
        WININET_Release(&request->hdr);
        return ERROR_OUTOFMEMORY;
    }

    if (dwFlags & INTERNET_FLAG_IGNORE_CERT_CN_INVALID)
        request->security_flags |= SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
    if (dwFlags & INTERNET_FLAG_IGNORE_CERT_DATE_INVALID)
        request->security_flags |= SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;

    if (lpszObjectName && *lpszObjectName) {
        HRESULT rc;
        WCHAR dummy;

        len = 1;
        rc = UrlCanonicalizeW(lpszObjectName, &dummy, &len, URL_ESCAPE_SPACES_ONLY);
        if (rc != E_POINTER)
            len = lstrlenW(lpszObjectName)+1;
        request->path = malloc(len * sizeof(WCHAR));
        rc = UrlCanonicalizeW(lpszObjectName, request->path, &len,
                   URL_ESCAPE_SPACES_ONLY);
        if (rc != S_OK)
        {
            ERR("Unable to escape string!(%s) (%ld)\n",debugstr_w(lpszObjectName),rc);
            lstrcpyW(request->path,lpszObjectName);
        }
    }else {
        request->path = wcsdup(L"/");
    }

    if (lpszReferrer && *lpszReferrer)
        HTTP_ProcessHeader(request, L"Referer", lpszReferrer, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDHDR_FLAG_REQ);

    if (lpszAcceptTypes)
    {
        int i;
        for (i = 0; lpszAcceptTypes[i]; i++)
        {
            if (!*lpszAcceptTypes[i]) continue;
            HTTP_ProcessHeader(request, L"Accept", lpszAcceptTypes[i],
                               HTTP_ADDREQ_FLAG_COALESCE_WITH_COMMA |
                               HTTP_ADDHDR_FLAG_REQ |
                               (i == 0 ? (HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD) : 0));
        }
    }

    request->verb = wcsdup(lpszVerb && *lpszVerb ? lpszVerb : L"GET");
    request->version = wcsdup(lpszVersion && *lpszVersion ? lpszVersion : L"HTTP/1.1");

    if (hIC->proxy && hIC->proxy[0] && !HTTP_ShouldBypassProxy(hIC, session->hostName))
        HTTP_DealWithProxy( hIC, session, request );

    INTERNET_SendCallback(&session->hdr, dwContext,
                          INTERNET_STATUS_HANDLE_CREATED, &request->hdr.hInternet,
                          sizeof(HINTERNET));

    TRACE("<-- (%p)\n", request);

    *ret = request->hdr.hInternet;
    return ERROR_SUCCESS;
}

/***********************************************************************
 *           HttpOpenRequestW (WININET.@)
 *
 * Open a HTTP request handle
 *
 * RETURNS
 *    HINTERNET  a HTTP request handle on success
 *    NULL 	 on failure
 *
 */
HINTERNET WINAPI HttpOpenRequestW(HINTERNET hHttpSession,
	LPCWSTR lpszVerb, LPCWSTR lpszObjectName, LPCWSTR lpszVersion,
	LPCWSTR lpszReferrer , LPCWSTR *lpszAcceptTypes,
	DWORD dwFlags, DWORD_PTR dwContext)
{
    http_session_t *session;
    HINTERNET handle = NULL;
    DWORD res;

    TRACE("(%p, %s, %s, %s, %s, %p, %08lx, %08Ix)\n", hHttpSession,
          debugstr_w(lpszVerb), debugstr_w(lpszObjectName),
          debugstr_w(lpszVersion), debugstr_w(lpszReferrer), lpszAcceptTypes,
          dwFlags, dwContext);
    if(lpszAcceptTypes!=NULL)
    {
        int i;
        for(i=0;lpszAcceptTypes[i]!=NULL;i++)
            TRACE("\taccept type: %s\n",debugstr_w(lpszAcceptTypes[i]));
    }

    session = (http_session_t*) get_handle_object( hHttpSession );
    if (NULL == session ||  session->hdr.htype != WH_HHTTPSESSION)
    {
        res = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        goto lend;
    }

    /*
     * My tests seem to show that the windows version does not
     * become asynchronous until after this point. And anyhow
     * if this call was asynchronous then how would you get the
     * necessary HINTERNET pointer returned by this function.
     *
     */
    res = HTTP_HttpOpenRequestW(session, lpszVerb, lpszObjectName,
                                lpszVersion, lpszReferrer, lpszAcceptTypes,
                                dwFlags, dwContext, &handle);
lend:
    if( session )
        WININET_Release( &session->hdr );
    TRACE("returning %p\n", handle);
    if(res != ERROR_SUCCESS)
        SetLastError(res);
    return handle;
}

static const LPCWSTR header_lookup[] = {
    L"Mime-Version",              /* HTTP_QUERY_MIME_VERSION = 0 */
    L"Content-Type",              /* HTTP_QUERY_CONTENT_TYPE = 1 */
    L"Content-Transfer-Encoding", /* HTTP_QUERY_CONTENT_TRANSFER_ENCODING = 2 */
    L"Content-ID",                /* HTTP_QUERY_CONTENT_ID = 3 */
    NULL,                         /* HTTP_QUERY_CONTENT_DESCRIPTION = 4 */
    L"Content-Length",            /* HTTP_QUERY_CONTENT_LENGTH =  5 */
    L"Content-Language",          /* HTTP_QUERY_CONTENT_LANGUAGE =  6 */
    L"Allow",                     /* HTTP_QUERY_ALLOW = 7 */
    L"Public",                    /* HTTP_QUERY_PUBLIC = 8 */
    L"Date",                      /* HTTP_QUERY_DATE = 9 */
    L"Expires",                   /* HTTP_QUERY_EXPIRES = 10 */
    L"Last-Modified",             /* HTTP_QUERY_LAST_MODIFIED = 11 */
    NULL,                         /* HTTP_QUERY_MESSAGE_ID = 12 */
    L"URI",                       /* HTTP_QUERY_URI = 13 */
    L"From",                      /* HTTP_QUERY_DERIVED_FROM = 14 */
    NULL,                         /* HTTP_QUERY_COST = 15 */
    NULL,                         /* HTTP_QUERY_LINK = 16 */
    L"Pragma",                    /* HTTP_QUERY_PRAGMA = 17 */
    NULL,                         /* HTTP_QUERY_VERSION = 18 */
    L"Status",                    /* HTTP_QUERY_STATUS_CODE = 19 */
    NULL,                         /* HTTP_QUERY_STATUS_TEXT = 20 */
    NULL,                         /* HTTP_QUERY_RAW_HEADERS = 21 */
    NULL,                         /* HTTP_QUERY_RAW_HEADERS_CRLF = 22 */
    L"Connection",                /* HTTP_QUERY_CONNECTION = 23 */
    L"Accept",                    /* HTTP_QUERY_ACCEPT = 24 */
    L"Accept-Charset",            /* HTTP_QUERY_ACCEPT_CHARSET = 25 */
    L"Accept-Encoding",           /* HTTP_QUERY_ACCEPT_ENCODING = 26 */
    L"Accept-Language",           /* HTTP_QUERY_ACCEPT_LANGUAGE = 27 */
    L"Authorization",             /* HTTP_QUERY_AUTHORIZATION = 28 */
    L"Content-Encoding",          /* HTTP_QUERY_CONTENT_ENCODING = 29 */
    NULL,                         /* HTTP_QUERY_FORWARDED = 30 */
    NULL,                         /* HTTP_QUERY_FROM = 31 */
    L"If-Modified-Since",         /* HTTP_QUERY_IF_MODIFIED_SINCE = 32 */
    L"Location",                  /* HTTP_QUERY_LOCATION = 33 */
    NULL,                         /* HTTP_QUERY_ORIG_URI = 34 */
    L"Referer",                   /* HTTP_QUERY_REFERER = 35 */
    L"Retry-After",               /* HTTP_QUERY_RETRY_AFTER = 36 */
    L"Server",                    /* HTTP_QUERY_SERVER = 37 */
    NULL,                         /* HTTP_TITLE = 38 */
    L"User-Agent",                /* HTTP_QUERY_USER_AGENT = 39 */
    L"WWW-Authenticate",          /* HTTP_QUERY_WWW_AUTHENTICATE = 40 */
    L"Proxy-Authenticate",        /* HTTP_QUERY_PROXY_AUTHENTICATE = 41 */
    L"Accept-Ranges",             /* HTTP_QUERY_ACCEPT_RANGES = 42 */
    L"Set-Cookie",                /* HTTP_QUERY_SET_COOKIE = 43 */
    L"Cookie",                    /* HTTP_QUERY_COOKIE = 44 */
    NULL,                         /* HTTP_QUERY_REQUEST_METHOD = 45 */
    NULL,                         /* HTTP_QUERY_REFRESH = 46 */
    L"Content-Disposition",       /* HTTP_QUERY_CONTENT_DISPOSITION = 47 */
    L"Age",                       /* HTTP_QUERY_AGE = 48 */
    L"Cache-Control",             /* HTTP_QUERY_CACHE_CONTROL = 49 */
    L"Content-Base",              /* HTTP_QUERY_CONTENT_BASE = 50 */
    L"Content-Location",          /* HTTP_QUERY_CONTENT_LOCATION = 51 */
    L"Content-MD5",               /* HTTP_QUERY_CONTENT_MD5 = 52 */
    L"Content-Range",             /* HTTP_QUERY_CONTENT_RANGE = 53 */
    L"ETag",                      /* HTTP_QUERY_ETAG = 54 */
    L"Host",                      /* HTTP_QUERY_HOST = 55 */
    L"If-Match",                  /* HTTP_QUERY_IF_MATCH = 56 */
    L"If-None-Match",             /* HTTP_QUERY_IF_NONE_MATCH = 57 */
    L"If-Range",                  /* HTTP_QUERY_IF_RANGE = 58 */
    L"If-Unmodified-Since",       /* HTTP_QUERY_IF_UNMODIFIED_SINCE = 59 */
    L"Max-Forwards",              /* HTTP_QUERY_MAX_FORWARDS = 60 */
    L"Proxy-Authorization",       /* HTTP_QUERY_PROXY_AUTHORIZATION = 61 */
    L"Range",                     /* HTTP_QUERY_RANGE = 62 */
    L"Transfer-Encoding",         /* HTTP_QUERY_TRANSFER_ENCODING = 63 */
    L"Upgrade",                   /* HTTP_QUERY_UPGRADE = 64 */
    L"Vary",                      /* HTTP_QUERY_VARY = 65 */
    L"Via",                       /* HTTP_QUERY_VIA = 66 */
    L"Warning",                   /* HTTP_QUERY_WARNING = 67 */
    L"Expect",                    /* HTTP_QUERY_EXPECT = 68 */
    L"Proxy-Connection",          /* HTTP_QUERY_PROXY_CONNECTION = 69 */
    L"Unless-Modified-Since",     /* HTTP_QUERY_UNLESS_MODIFIED_SINCE = 70 */
};

/***********************************************************************
 *           HTTP_HttpQueryInfoW (internal)
 */
static DWORD HTTP_HttpQueryInfoW(http_request_t *request, DWORD dwInfoLevel,
        LPVOID lpBuffer, LPDWORD lpdwBufferLength, LPDWORD lpdwIndex)
{
    LPHTTPHEADERW lphttpHdr = NULL;
    BOOL request_only = !!(dwInfoLevel & HTTP_QUERY_FLAG_REQUEST_HEADERS);
    INT requested_index = lpdwIndex ? *lpdwIndex : 0;
    DWORD level = (dwInfoLevel & ~HTTP_QUERY_MODIFIER_FLAGS_MASK);
    INT index = -1;

    EnterCriticalSection( &request->headers_section );

    /* Find requested header structure */
    switch (level)
    {
    case HTTP_QUERY_CUSTOM:
        if (!lpBuffer)
        {
            LeaveCriticalSection( &request->headers_section );
            return ERROR_INVALID_PARAMETER;
        }
        index = HTTP_GetCustomHeaderIndex(request, lpBuffer, requested_index, request_only);
        break;
    case HTTP_QUERY_RAW_HEADERS_CRLF:
        {
            LPWSTR headers;
            DWORD len = 0;
            DWORD res = ERROR_INVALID_PARAMETER;

            if (request_only)
                headers = build_request_header(request, request->verb, request->path, request->version, TRUE);
            else
                headers = build_response_header(request, TRUE);
            if (!headers)
            {
                LeaveCriticalSection( &request->headers_section );
                return ERROR_OUTOFMEMORY;
            }

            len = lstrlenW(headers) * sizeof(WCHAR);
            if (len + sizeof(WCHAR) > *lpdwBufferLength)
            {
                len += sizeof(WCHAR);
                res = ERROR_INSUFFICIENT_BUFFER;
            }
            else if (lpBuffer)
            {
                memcpy(lpBuffer, headers, len + sizeof(WCHAR));
                TRACE("returning data: %s\n", debugstr_wn(lpBuffer, len / sizeof(WCHAR)));
                res = ERROR_SUCCESS;
            }
            *lpdwBufferLength = len;

            free(headers);
            LeaveCriticalSection( &request->headers_section );
            return res;
        }
    case HTTP_QUERY_RAW_HEADERS:
        {
            LPWSTR headers;
            DWORD len;

            if (request_only)
                headers = build_request_header(request, request->verb, request->path, request->version, FALSE);
            else
                headers = build_response_header(request, FALSE);

            if (!headers)
            {
                LeaveCriticalSection( &request->headers_section );
                return ERROR_OUTOFMEMORY;
            }

            len = lstrlenW(headers) * sizeof(WCHAR);
            if (len > *lpdwBufferLength)
            {
                *lpdwBufferLength = len;
                free(headers);
                LeaveCriticalSection( &request->headers_section );
                return ERROR_INSUFFICIENT_BUFFER;
            }

            if (lpBuffer)
            {
                DWORD i;

                TRACE("returning data: %s\n", debugstr_wn(headers, len / sizeof(WCHAR)));

                for (i = 0; i < len / sizeof(WCHAR); i++)
                {
                    if (headers[i] == '\n')
                        headers[i] = 0;
                }
                memcpy(lpBuffer, headers, len);
            }
            *lpdwBufferLength = len - sizeof(WCHAR);

            free(headers);
            LeaveCriticalSection( &request->headers_section );
            return ERROR_SUCCESS;
        }
    case HTTP_QUERY_STATUS_TEXT:
        if (request->statusText)
        {
            DWORD len = lstrlenW(request->statusText);
            if (len + 1 > *lpdwBufferLength/sizeof(WCHAR))
            {
                *lpdwBufferLength = (len + 1) * sizeof(WCHAR);
                LeaveCriticalSection( &request->headers_section );
                return ERROR_INSUFFICIENT_BUFFER;
            }
            if (lpBuffer)
            {
                memcpy(lpBuffer, request->statusText, (len + 1) * sizeof(WCHAR));
                TRACE("returning data: %s\n", debugstr_wn(lpBuffer, len));
            }
            *lpdwBufferLength = len * sizeof(WCHAR);
            LeaveCriticalSection( &request->headers_section );
            return ERROR_SUCCESS;
        }
        break;
    case HTTP_QUERY_VERSION:
        if (request->version)
        {
            DWORD len = lstrlenW(request->version);
            if (len + 1 > *lpdwBufferLength/sizeof(WCHAR))
            {
                *lpdwBufferLength = (len + 1) * sizeof(WCHAR);
                LeaveCriticalSection( &request->headers_section );
                return ERROR_INSUFFICIENT_BUFFER;
            }
            if (lpBuffer)
            {
                memcpy(lpBuffer, request->version, (len + 1) * sizeof(WCHAR));
                TRACE("returning data: %s\n", debugstr_wn(lpBuffer, len));
            }
            *lpdwBufferLength = len * sizeof(WCHAR);
            LeaveCriticalSection( &request->headers_section );
            return ERROR_SUCCESS;
        }
        break;
    case HTTP_QUERY_CONTENT_ENCODING:
        index = HTTP_GetCustomHeaderIndex(request, header_lookup[request->read_gzip ? HTTP_QUERY_CONTENT_TYPE : level],
                requested_index,request_only);
        break;
    case HTTP_QUERY_STATUS_CODE: {
        DWORD res = ERROR_SUCCESS;

        if(request_only)
        {
            LeaveCriticalSection( &request->headers_section );
            return ERROR_HTTP_INVALID_QUERY_REQUEST;
        }

        if(requested_index)
            break;

        if(dwInfoLevel & HTTP_QUERY_FLAG_NUMBER) {
            if(*lpdwBufferLength >= sizeof(DWORD))
                *(DWORD*)lpBuffer = request->status_code;
            else
                res = ERROR_INSUFFICIENT_BUFFER;
            *lpdwBufferLength = sizeof(DWORD);
        }else {
            WCHAR buf[12];
            DWORD size;

            size = swprintf(buf, ARRAY_SIZE(buf), L"%u", request->status_code) * sizeof(WCHAR);

            if(size <= *lpdwBufferLength) {
                memcpy(lpBuffer, buf, size+sizeof(WCHAR));
            }else {
                size += sizeof(WCHAR);
                res = ERROR_INSUFFICIENT_BUFFER;
            }

            *lpdwBufferLength = size;
        }
        LeaveCriticalSection( &request->headers_section );
        return res;
    }
    default:
        assert (ARRAY_SIZE(header_lookup) == (HTTP_QUERY_UNLESS_MODIFIED_SINCE + 1));

        if (level < ARRAY_SIZE(header_lookup) && header_lookup[level])
            index = HTTP_GetCustomHeaderIndex(request, header_lookup[level],
                                              requested_index,request_only);
    }

    if (index >= 0)
        lphttpHdr = &request->custHeaders[index];

    /* Ensure header satisfies requested attributes */
    if (!lphttpHdr ||
        ((dwInfoLevel & HTTP_QUERY_FLAG_REQUEST_HEADERS) &&
         (~lphttpHdr->wFlags & HDR_ISREQUEST)))
    {
        LeaveCriticalSection( &request->headers_section );
        return ERROR_HTTP_HEADER_NOT_FOUND;
    }

    /* coalesce value to requested type */
    if (dwInfoLevel & HTTP_QUERY_FLAG_NUMBER && lpBuffer)
    {
        unsigned long value;

        if (*lpdwBufferLength != sizeof(DWORD))
        {
            LeaveCriticalSection( &request->headers_section );
            return ERROR_HTTP_INVALID_HEADER;
        }

        errno = 0;
        value = wcstoul( lphttpHdr->lpszValue, NULL, 10 );
        if (value > UINT_MAX || (value == ULONG_MAX && errno == ERANGE))
        {
            LeaveCriticalSection( &request->headers_section );
            return ERROR_HTTP_INVALID_HEADER;
        }

        *(DWORD *)lpBuffer = value;
        TRACE(" returning number: %lu\n", *(DWORD *)lpBuffer);
    }
    else if (dwInfoLevel & HTTP_QUERY_FLAG_SYSTEMTIME && lpBuffer)
    {
        SYSTEMTIME st;

        if (!InternetTimeToSystemTimeW(lphttpHdr->lpszValue, &st, 0))
        {
            LeaveCriticalSection( &request->headers_section );
            return ERROR_HTTP_INVALID_HEADER;
        }
        if (*lpdwBufferLength < sizeof(st))
        {
            *lpdwBufferLength = sizeof(st);
            LeaveCriticalSection( &request->headers_section );
            return ERROR_INSUFFICIENT_BUFFER;
        }
        TRACE(" returning time: %04u/%02u/%02u - %u - %02u:%02u:%02u.%02u\n",
              st.wYear, st.wMonth, st.wDay, st.wDayOfWeek, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
        memcpy(lpBuffer, &st, sizeof(st));
        *lpdwBufferLength = sizeof(st);
    }
    else if (lphttpHdr->lpszValue)
    {
        DWORD len = (lstrlenW(lphttpHdr->lpszValue) + 1) * sizeof(WCHAR);

        if (len > *lpdwBufferLength)
        {
            *lpdwBufferLength = len;
            LeaveCriticalSection( &request->headers_section );
            return ERROR_INSUFFICIENT_BUFFER;
        }
        if (lpBuffer)
        {
            memcpy(lpBuffer, lphttpHdr->lpszValue, len);
            TRACE("! returning string: %s\n", debugstr_w(lpBuffer));
        }
        *lpdwBufferLength = len - sizeof(WCHAR);
    }
    if (lpdwIndex) (*lpdwIndex)++;

    LeaveCriticalSection( &request->headers_section );
    return ERROR_SUCCESS;
}

/***********************************************************************
 *           HttpQueryInfoW (WININET.@)
 *
 * Queries for information about an HTTP request
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HttpQueryInfoW(HINTERNET hHttpRequest, DWORD dwInfoLevel,
        LPVOID lpBuffer, LPDWORD lpdwBufferLength, LPDWORD lpdwIndex)
{
    http_request_t *request;
    DWORD res;

    if (TRACE_ON(wininet)) {
#define FE(x) { x, #x }
	static const wininet_flag_info query_flags[] = {
	    FE(HTTP_QUERY_MIME_VERSION),
	    FE(HTTP_QUERY_CONTENT_TYPE),
	    FE(HTTP_QUERY_CONTENT_TRANSFER_ENCODING),
	    FE(HTTP_QUERY_CONTENT_ID),
	    FE(HTTP_QUERY_CONTENT_DESCRIPTION),
	    FE(HTTP_QUERY_CONTENT_LENGTH),
	    FE(HTTP_QUERY_CONTENT_LANGUAGE),
	    FE(HTTP_QUERY_ALLOW),
	    FE(HTTP_QUERY_PUBLIC),
	    FE(HTTP_QUERY_DATE),
	    FE(HTTP_QUERY_EXPIRES),
	    FE(HTTP_QUERY_LAST_MODIFIED),
	    FE(HTTP_QUERY_MESSAGE_ID),
	    FE(HTTP_QUERY_URI),
	    FE(HTTP_QUERY_DERIVED_FROM),
	    FE(HTTP_QUERY_COST),
	    FE(HTTP_QUERY_LINK),
	    FE(HTTP_QUERY_PRAGMA),
	    FE(HTTP_QUERY_VERSION),
	    FE(HTTP_QUERY_STATUS_CODE),
	    FE(HTTP_QUERY_STATUS_TEXT),
	    FE(HTTP_QUERY_RAW_HEADERS),
	    FE(HTTP_QUERY_RAW_HEADERS_CRLF),
	    FE(HTTP_QUERY_CONNECTION),
	    FE(HTTP_QUERY_ACCEPT),
	    FE(HTTP_QUERY_ACCEPT_CHARSET),
	    FE(HTTP_QUERY_ACCEPT_ENCODING),
	    FE(HTTP_QUERY_ACCEPT_LANGUAGE),
	    FE(HTTP_QUERY_AUTHORIZATION),
	    FE(HTTP_QUERY_CONTENT_ENCODING),
	    FE(HTTP_QUERY_FORWARDED),
	    FE(HTTP_QUERY_FROM),
	    FE(HTTP_QUERY_IF_MODIFIED_SINCE),
	    FE(HTTP_QUERY_LOCATION),
	    FE(HTTP_QUERY_ORIG_URI),
	    FE(HTTP_QUERY_REFERER),
	    FE(HTTP_QUERY_RETRY_AFTER),
	    FE(HTTP_QUERY_SERVER),
	    FE(HTTP_QUERY_TITLE),
	    FE(HTTP_QUERY_USER_AGENT),
	    FE(HTTP_QUERY_WWW_AUTHENTICATE),
	    FE(HTTP_QUERY_PROXY_AUTHENTICATE),
	    FE(HTTP_QUERY_ACCEPT_RANGES),
        FE(HTTP_QUERY_SET_COOKIE),
        FE(HTTP_QUERY_COOKIE),
	    FE(HTTP_QUERY_REQUEST_METHOD),
	    FE(HTTP_QUERY_REFRESH),
	    FE(HTTP_QUERY_CONTENT_DISPOSITION),
	    FE(HTTP_QUERY_AGE),
	    FE(HTTP_QUERY_CACHE_CONTROL),
	    FE(HTTP_QUERY_CONTENT_BASE),
	    FE(HTTP_QUERY_CONTENT_LOCATION),
	    FE(HTTP_QUERY_CONTENT_MD5),
	    FE(HTTP_QUERY_CONTENT_RANGE),
	    FE(HTTP_QUERY_ETAG),
	    FE(HTTP_QUERY_HOST),
	    FE(HTTP_QUERY_IF_MATCH),
	    FE(HTTP_QUERY_IF_NONE_MATCH),
	    FE(HTTP_QUERY_IF_RANGE),
	    FE(HTTP_QUERY_IF_UNMODIFIED_SINCE),
	    FE(HTTP_QUERY_MAX_FORWARDS),
	    FE(HTTP_QUERY_PROXY_AUTHORIZATION),
	    FE(HTTP_QUERY_RANGE),
	    FE(HTTP_QUERY_TRANSFER_ENCODING),
	    FE(HTTP_QUERY_UPGRADE),
	    FE(HTTP_QUERY_VARY),
	    FE(HTTP_QUERY_VIA),
	    FE(HTTP_QUERY_WARNING),
	    FE(HTTP_QUERY_CUSTOM)
	};
	static const wininet_flag_info modifier_flags[] = {
	    FE(HTTP_QUERY_FLAG_REQUEST_HEADERS),
	    FE(HTTP_QUERY_FLAG_SYSTEMTIME),
	    FE(HTTP_QUERY_FLAG_NUMBER),
	    FE(HTTP_QUERY_FLAG_COALESCE)
	};
#undef FE
	DWORD info_mod = dwInfoLevel & HTTP_QUERY_MODIFIER_FLAGS_MASK;
	DWORD info = dwInfoLevel & HTTP_QUERY_HEADER_MASK;
	DWORD i;

	TRACE("(%p, 0x%08lx)--> %ld\n", hHttpRequest, dwInfoLevel, info);
	TRACE("  Attribute:");
        for (i = 0; i < ARRAY_SIZE(query_flags); i++) {
	    if (query_flags[i].val == info) {
		TRACE(" %s", query_flags[i].name);
		break;
	    }
	}
        if (i == ARRAY_SIZE(query_flags)) {
	    TRACE(" Unknown (%08lx)", info);
	}

	TRACE(" Modifier:");
        for (i = 0; i < ARRAY_SIZE(modifier_flags); i++) {
	    if (modifier_flags[i].val & info_mod) {
		TRACE(" %s", modifier_flags[i].name);
		info_mod &= ~ modifier_flags[i].val;
	    }
	}

	if (info_mod) {
	    TRACE(" Unknown (%08lx)", info_mod);
	}
	TRACE("\n");
    }

    request = (http_request_t*) get_handle_object( hHttpRequest );
    if (NULL == request ||  request->hdr.htype != WH_HHTTPREQ)
    {
        res = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        goto lend;
    }

    if (lpBuffer == NULL)
        *lpdwBufferLength = 0;
    res = HTTP_HttpQueryInfoW( request, dwInfoLevel,
                               lpBuffer, lpdwBufferLength, lpdwIndex);

lend:
    if( request )
         WININET_Release( &request->hdr );

    TRACE("%lu <--\n", res);

    SetLastError(res);
    return res == ERROR_SUCCESS;
}

/***********************************************************************
 *           HttpQueryInfoA (WININET.@)
 *
 * Queries for information about an HTTP request
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HttpQueryInfoA(HINTERNET hHttpRequest, DWORD dwInfoLevel,
	LPVOID lpBuffer, LPDWORD lpdwBufferLength, LPDWORD lpdwIndex)
{
    BOOL result;
    DWORD len;
    WCHAR* bufferW;

    TRACE("%p %lx\n", hHttpRequest, dwInfoLevel);

    if((dwInfoLevel & HTTP_QUERY_FLAG_NUMBER) ||
       (dwInfoLevel & HTTP_QUERY_FLAG_SYSTEMTIME))
    {
        return HttpQueryInfoW( hHttpRequest, dwInfoLevel, lpBuffer,
                               lpdwBufferLength, lpdwIndex );
    }

    if (lpBuffer)
    {
        DWORD alloclen;
        len = (*lpdwBufferLength)*sizeof(WCHAR);
        if ((dwInfoLevel & HTTP_QUERY_HEADER_MASK) == HTTP_QUERY_CUSTOM)
        {
            alloclen = MultiByteToWideChar( CP_ACP, 0, lpBuffer, -1, NULL, 0 ) * sizeof(WCHAR);
            if (alloclen < len)
                alloclen = len;
        }
        else
            alloclen = len;
        bufferW = malloc( alloclen );
        /* buffer is in/out because of HTTP_QUERY_CUSTOM */
        if ((dwInfoLevel & HTTP_QUERY_HEADER_MASK) == HTTP_QUERY_CUSTOM)
            MultiByteToWideChar( CP_ACP, 0, lpBuffer, -1, bufferW, alloclen / sizeof(WCHAR) );
    } else
    {
        bufferW = NULL;
        len = 0;
    }

    result = HttpQueryInfoW( hHttpRequest, dwInfoLevel, bufferW,
                           &len, lpdwIndex );
    if( result )
    {
        len = WideCharToMultiByte( CP_ACP,0, bufferW, len / sizeof(WCHAR) + 1,
                                     lpBuffer, *lpdwBufferLength, NULL, NULL );
        *lpdwBufferLength = len - 1;

        TRACE("lpBuffer: %s\n", debugstr_a(lpBuffer));
    }
    else
        /* since the strings being returned from HttpQueryInfoW should be
         * only ASCII characters, it is reasonable to assume that all of
         * the Unicode characters can be reduced to a single byte */
        *lpdwBufferLength = len / sizeof(WCHAR);

    free( bufferW );
    return result;
}

static WCHAR *get_redirect_url(http_request_t *request)
{
    static WCHAR szHttp[] = L"http";
    static WCHAR szHttps[] = L"https";
    http_session_t *session = request->session;
    URL_COMPONENTSW urlComponents = { sizeof(urlComponents) };
    WCHAR *orig_url = NULL, *redirect_url = NULL, *combined_url = NULL;
    DWORD url_length = 0, res;
    BOOL b;

    url_length = 0;
    res = HTTP_HttpQueryInfoW(request, HTTP_QUERY_LOCATION, redirect_url, &url_length, NULL);
    if(res == ERROR_INSUFFICIENT_BUFFER) {
        redirect_url = malloc(url_length);
        res = HTTP_HttpQueryInfoW(request, HTTP_QUERY_LOCATION, redirect_url, &url_length, NULL);
    }
    if(res != ERROR_SUCCESS) {
        free(redirect_url);
        return NULL;
    }

    urlComponents.dwSchemeLength = 1;
    b = InternetCrackUrlW(redirect_url, url_length / sizeof(WCHAR), 0, &urlComponents);
    if(b && urlComponents.dwSchemeLength &&
       urlComponents.nScheme != INTERNET_SCHEME_HTTP && urlComponents.nScheme != INTERNET_SCHEME_HTTPS) {
        TRACE("redirect to non-http URL\n");
        return NULL;
    }

    urlComponents.lpszScheme = (request->hdr.dwFlags & INTERNET_FLAG_SECURE) ? szHttps : szHttp;
    urlComponents.dwSchemeLength = 0;
    urlComponents.lpszHostName = request->server->name;
    urlComponents.nPort = request->server->port;
    urlComponents.lpszUserName = session->userName;
    urlComponents.lpszUrlPath = request->path;

    b = InternetCreateUrlW(&urlComponents, 0, NULL, &url_length);
    if(!b && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        orig_url = malloc(url_length);

        /* convert from bytes to characters */
        url_length = url_length / sizeof(WCHAR) - 1;
        b = InternetCreateUrlW(&urlComponents, 0, orig_url, &url_length);
    }

    if(b) {
        url_length = 0;
        b = InternetCombineUrlW(orig_url, redirect_url, NULL, &url_length, ICU_ENCODE_SPACES_ONLY);
        if(!b && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            combined_url = malloc(url_length * sizeof(WCHAR));
            b = InternetCombineUrlW(orig_url, redirect_url, combined_url, &url_length, ICU_ENCODE_SPACES_ONLY);
            if(!b) {
                free(combined_url);
                combined_url = NULL;
            }
        }
    }

    free(orig_url);
    free(redirect_url);
    return combined_url;
}


/***********************************************************************
 *           HTTP_HandleRedirect (internal)
 */
static DWORD HTTP_HandleRedirect(http_request_t *request, WCHAR *url)
{
    URL_COMPONENTSW urlComponents = { sizeof(urlComponents) };
    http_session_t *session = request->session;
    size_t url_len = lstrlenW(url);

    if(url[0] == '/')
    {
        /* if it's an absolute path, keep the same session info */
        urlComponents.lpszUrlPath = url;
        urlComponents.dwUrlPathLength = url_len;
    }
    else
    {
        urlComponents.dwHostNameLength = 1;
        urlComponents.dwUserNameLength = 1;
        urlComponents.dwUrlPathLength = 1;
        if(!InternetCrackUrlW(url, url_len, 0, &urlComponents))
            return INTERNET_GetLastError();

        if(!urlComponents.dwHostNameLength)
            return ERROR_INTERNET_INVALID_URL;
    }

    INTERNET_SendCallback(&request->hdr, request->hdr.dwContext, INTERNET_STATUS_REDIRECT,
                          url, (url_len + 1) * sizeof(WCHAR));

    if(urlComponents.dwHostNameLength) {
        BOOL custom_port = FALSE;
        substr_t host;

        if(urlComponents.nScheme == INTERNET_SCHEME_HTTP) {
            if(request->hdr.dwFlags & INTERNET_FLAG_SECURE) {
                TRACE("redirect from secure page to non-secure page\n");
                /* FIXME: warn about from secure redirect to non-secure page */
                request->hdr.dwFlags &= ~INTERNET_FLAG_SECURE;
            }

            custom_port = urlComponents.nPort != INTERNET_DEFAULT_HTTP_PORT;
        }else if(urlComponents.nScheme == INTERNET_SCHEME_HTTPS) {
            if(!(request->hdr.dwFlags & INTERNET_FLAG_SECURE)) {
                TRACE("redirect from non-secure page to secure page\n");
                /* FIXME: notify about redirect to secure page */
                request->hdr.dwFlags |= INTERNET_FLAG_SECURE;
            }

            custom_port = urlComponents.nPort != INTERNET_DEFAULT_HTTPS_PORT;
        }

        free(session->hostName);

        session->hostName = strndupW(urlComponents.lpszHostName, urlComponents.dwHostNameLength);
        session->hostPort = urlComponents.nPort;

        free(session->userName);
        session->userName = NULL;
        if (urlComponents.dwUserNameLength)
            session->userName = strndupW(urlComponents.lpszUserName, urlComponents.dwUserNameLength);

        reset_data_stream(request);

        host = substr(urlComponents.lpszHostName, urlComponents.dwHostNameLength);

        if(host.len != lstrlenW(request->server->name) || wcsnicmp(request->server->name, host.str, host.len)
           || request->server->port != urlComponents.nPort) {
            server_t *new_server;

            new_server = get_server(host, urlComponents.nPort, urlComponents.nScheme == INTERNET_SCHEME_HTTPS, TRUE);
            server_release(request->server);
            request->server = new_server;
        }

        if (custom_port)
            HTTP_ProcessHeader(request, L"Host", request->server->host_port,
                               HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDHDR_FLAG_REQ);
        else
            HTTP_ProcessHeader(request, L"Host", request->server->name,
                               HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDHDR_FLAG_REQ);
    }

    free(request->path);
    request->path = NULL;
    if(urlComponents.dwUrlPathLength)
    {
        DWORD needed = 1;
        HRESULT rc;
        WCHAR dummy[] = L"";
        WCHAR *path;

        path = strndupW(urlComponents.lpszUrlPath, urlComponents.dwUrlPathLength);
        rc = UrlEscapeW(path, dummy, &needed, URL_ESCAPE_SPACES_ONLY);
        if (rc != E_POINTER)
            ERR("Unable to escape string!(%s) (%ld)\n",debugstr_w(path),rc);
        request->path = malloc(needed * sizeof(WCHAR));
        rc = UrlEscapeW(path, request->path, &needed,
                        URL_ESCAPE_SPACES_ONLY);
        if (rc != S_OK)
        {
            ERR("Unable to escape string!(%s) (%ld)\n",debugstr_w(path),rc);
            lstrcpyW(request->path, path);
        }
        free(path);
    }

    /* Remove custom content-type/length headers on redirects.  */
    remove_header(request, L"Content-Type", TRUE);
    remove_header(request, L"Content-Length", TRUE);

    return ERROR_SUCCESS;
}

/***********************************************************************
 *           HTTP_build_req (internal)
 *
 *  concatenate all the strings in the request together
 */
static LPWSTR HTTP_build_req( LPCWSTR *list, int len )
{
    LPCWSTR *t;
    LPWSTR str;

    for( t = list; *t ; t++  )
        len += lstrlenW( *t );
    len++;

    str = malloc(len * sizeof(WCHAR));
    *str = 0;

    for( t = list; *t ; t++ )
        lstrcatW( str, *t );

    return str;
}

static void HTTP_InsertCookies(http_request_t *request)
{
    WCHAR *cookies;
    DWORD res;

    res = get_cookie_header(request->server->name, request->path, &cookies);
    if(res != ERROR_SUCCESS || !cookies)
        return;

    HTTP_HttpAddRequestHeadersW(request, cookies, lstrlenW(cookies),
                                HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD);
    free(cookies);
}

static WORD HTTP_ParseWkday(LPCWSTR day)
{
    static const WCHAR days[7][4] = {L"sun",
                                     L"mon",
                                     L"tue",
                                     L"wed",
                                     L"thu",
                                     L"fri",
                                     L"sat"};
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(days); i++)
        if (!wcsicmp(day, days[i]))
            return i;

    /* Invalid */
    return 7;
}

static WORD HTTP_ParseMonth(LPCWSTR month)
{
    if (!wcsicmp(month, L"jan")) return 1;
    if (!wcsicmp(month, L"feb")) return 2;
    if (!wcsicmp(month, L"mar")) return 3;
    if (!wcsicmp(month, L"apr")) return 4;
    if (!wcsicmp(month, L"may")) return 5;
    if (!wcsicmp(month, L"jun")) return 6;
    if (!wcsicmp(month, L"jul")) return 7;
    if (!wcsicmp(month, L"aug")) return 8;
    if (!wcsicmp(month, L"sep")) return 9;
    if (!wcsicmp(month, L"oct")) return 10;
    if (!wcsicmp(month, L"nov")) return 11;
    if (!wcsicmp(month, L"dec")) return 12;
    /* Invalid */
    return 0;
}

/* Parses the string pointed to by *str, assumed to be a 24-hour time HH:MM:SS,
 * optionally preceded by whitespace.
 * Upon success, returns TRUE, sets the wHour, wMinute, and wSecond fields of
 * st, and sets *str to the first character after the time format.
 */
static BOOL HTTP_ParseTime(SYSTEMTIME *st, LPCWSTR *str)
{
    LPCWSTR ptr = *str;
    WCHAR *nextPtr;
    unsigned long num;

    while (iswspace(*ptr))
        ptr++;

    num = wcstoul(ptr, &nextPtr, 10);
    if (!nextPtr || nextPtr <= ptr || *nextPtr != ':')
    {
        ERR("unexpected time format %s\n", debugstr_w(ptr));
        return FALSE;
    }
    if (num > 23)
    {
        ERR("unexpected hour in time format %s\n", debugstr_w(ptr));
        return FALSE;
    }
    ptr = nextPtr + 1;
    st->wHour = (WORD)num;
    num = wcstoul(ptr, &nextPtr, 10);
    if (!nextPtr || nextPtr <= ptr || *nextPtr != ':')
    {
        ERR("unexpected time format %s\n", debugstr_w(ptr));
        return FALSE;
    }
    if (num > 59)
    {
        ERR("unexpected minute in time format %s\n", debugstr_w(ptr));
        return FALSE;
    }
    ptr = nextPtr + 1;
    st->wMinute = (WORD)num;
    num = wcstoul(ptr, &nextPtr, 10);
    if (!nextPtr || nextPtr <= ptr)
    {
        ERR("unexpected time format %s\n", debugstr_w(ptr));
        return FALSE;
    }
    if (num > 59)
    {
        ERR("unexpected second in time format %s\n", debugstr_w(ptr));
        return FALSE;
    }
    *str = nextPtr;
    st->wSecond = (WORD)num;
    return TRUE;
}

static BOOL HTTP_ParseDateAsAsctime(LPCWSTR value, FILETIME *ft)
{
    WCHAR day[4], *dayPtr, month[4], *monthPtr, *nextPtr;
    LPCWSTR ptr;
    SYSTEMTIME st = { 0 };
    unsigned long num;

    for (ptr = value, dayPtr = day; *ptr && !iswspace(*ptr) &&
         dayPtr - day < ARRAY_SIZE(day) - 1; ptr++, dayPtr++)
        *dayPtr = *ptr;
    *dayPtr = 0;
    st.wDayOfWeek = HTTP_ParseWkday(day);
    if (st.wDayOfWeek >= 7)
    {
        ERR("unexpected weekday %s\n", debugstr_w(day));
        return FALSE;
    }

    while (iswspace(*ptr))
        ptr++;

    for (monthPtr = month; !iswspace(*ptr) && monthPtr - month < ARRAY_SIZE(month) - 1;
         monthPtr++, ptr++)
        *monthPtr = *ptr;
    *monthPtr = 0;
    st.wMonth = HTTP_ParseMonth(month);
    if (!st.wMonth || st.wMonth > 12)
    {
        ERR("unexpected month %s\n", debugstr_w(month));
        return FALSE;
    }

    while (iswspace(*ptr))
        ptr++;

    num = wcstoul(ptr, &nextPtr, 10);
    if (!nextPtr || nextPtr <= ptr || !num || num > 31)
    {
        ERR("unexpected day %s\n", debugstr_w(ptr));
        return FALSE;
    }
    ptr = nextPtr;
    st.wDay = (WORD)num;

    while (iswspace(*ptr))
        ptr++;

    if (!HTTP_ParseTime(&st, &ptr))
        return FALSE;

    while (iswspace(*ptr))
        ptr++;

    num = wcstoul(ptr, &nextPtr, 10);
    if (!nextPtr || nextPtr <= ptr || num < 1601 || num > 30827)
    {
        ERR("unexpected year %s\n", debugstr_w(ptr));
        return FALSE;
    }
    ptr = nextPtr;
    st.wYear = (WORD)num;

    while (iswspace(*ptr))
        ptr++;

    /* asctime() doesn't report a timezone, but some web servers do, so accept
     * with or without GMT.
     */
    if (*ptr && (wcscmp(ptr, L"GMT") && wcscmp(ptr, L"UTC")))
    {
        ERR("unexpected timezone %s\n", debugstr_w(ptr));
        return FALSE;
    }
    return SystemTimeToFileTime(&st, ft);
}

static BOOL HTTP_ParseRfc1123Date(LPCWSTR value, FILETIME *ft)
{
    WCHAR *nextPtr, day[4], month[4], *monthPtr;
    LPCWSTR ptr;
    unsigned long num;
    SYSTEMTIME st = { 0 };

    ptr = wcschr(value, ',');
    if (!ptr)
        return FALSE;
    if (ptr - value != 3)
    {
        WARN("unexpected weekday %s\n", debugstr_wn(value, ptr - value));
        return FALSE;
    }
    memcpy(day, value, (ptr - value) * sizeof(WCHAR));
    day[3] = 0;
    st.wDayOfWeek = HTTP_ParseWkday(day);
    if (st.wDayOfWeek > 6)
    {
        WARN("unexpected weekday %s\n", debugstr_wn(value, ptr - value));
        return FALSE;
    }
    ptr++;

    while (iswspace(*ptr))
        ptr++;

    num = wcstoul(ptr, &nextPtr, 10);
    if (!nextPtr || nextPtr <= ptr || !num || num > 31)
    {
        WARN("unexpected day %s\n", debugstr_w(value));
        return FALSE;
    }
    ptr = nextPtr;
    st.wDay = (WORD)num;

    while (iswspace(*ptr))
        ptr++;

    for (monthPtr = month; !iswspace(*ptr) && monthPtr - month < ARRAY_SIZE(month) - 1;
         monthPtr++, ptr++)
        *monthPtr = *ptr;
    *monthPtr = 0;
    st.wMonth = HTTP_ParseMonth(month);
    if (!st.wMonth || st.wMonth > 12)
    {
        WARN("unexpected month %s\n", debugstr_w(month));
        return FALSE;
    }

    while (iswspace(*ptr))
        ptr++;

    num = wcstoul(ptr, &nextPtr, 10);
    if (!nextPtr || nextPtr <= ptr || num < 1601 || num > 30827)
    {
        ERR("unexpected year %s\n", debugstr_w(value));
        return FALSE;
    }
    ptr = nextPtr;
    st.wYear = (WORD)num;

    if (!HTTP_ParseTime(&st, &ptr))
        return FALSE;

    while (iswspace(*ptr))
        ptr++;

    if (wcscmp(ptr, L"GMT") && wcscmp(ptr, L"UTC"))
    {
        ERR("unexpected time zone %s\n", debugstr_w(ptr));
        return FALSE;
    }
    return SystemTimeToFileTime(&st, ft);
}

static WORD HTTP_ParseWeekday(LPCWSTR day)
{
    static const WCHAR days[7][10] = {L"sunday",
                                     L"monday",
                                     L"tuesday",
                                     L"wednesday",
                                     L"thursday",
                                     L"friday",
                                     L"saturday"};
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(days); i++)
        if (!wcsicmp(day, days[i]))
            return i;

    /* Invalid */
    return 7;
}

static BOOL HTTP_ParseRfc850Date(LPCWSTR value, FILETIME *ft)
{
    WCHAR *nextPtr, day[10], month[4], *monthPtr;
    LPCWSTR ptr;
    unsigned long num;
    SYSTEMTIME st = { 0 };

    ptr = wcschr(value, ',');
    if (!ptr)
        return FALSE;
    if (ptr - value == 3)
    {
        memcpy(day, value, (ptr - value) * sizeof(WCHAR));
        day[3] = 0;
        st.wDayOfWeek = HTTP_ParseWkday(day);
        if (st.wDayOfWeek > 6)
        {
            ERR("unexpected weekday %s\n", debugstr_wn(value, ptr - value));
            return FALSE;
        }
    }
    else if (ptr - value < ARRAY_SIZE(day))
    {
        memcpy(day, value, (ptr - value) * sizeof(WCHAR));
        day[ptr - value + 1] = 0;
        st.wDayOfWeek = HTTP_ParseWeekday(day);
        if (st.wDayOfWeek > 6)
        {
            ERR("unexpected weekday %s\n", debugstr_wn(value, ptr - value));
            return FALSE;
        }
    }
    else
    {
        ERR("unexpected weekday %s\n", debugstr_wn(value, ptr - value));
        return FALSE;
    }
    ptr++;

    while (iswspace(*ptr))
        ptr++;

    num = wcstoul(ptr, &nextPtr, 10);
    if (!nextPtr || nextPtr <= ptr || !num || num > 31)
    {
        ERR("unexpected day %s\n", debugstr_w(value));
        return FALSE;
    }
    ptr = nextPtr;
    st.wDay = (WORD)num;

    if (*ptr != '-')
    {
        ERR("unexpected month format %s\n", debugstr_w(ptr));
        return FALSE;
    }
    ptr++;

    for (monthPtr = month; *ptr != '-' && monthPtr - month < ARRAY_SIZE(month) - 1;
         monthPtr++, ptr++)
        *monthPtr = *ptr;
    *monthPtr = 0;
    st.wMonth = HTTP_ParseMonth(month);
    if (!st.wMonth || st.wMonth > 12)
    {
        ERR("unexpected month %s\n", debugstr_w(month));
        return FALSE;
    }

    if (*ptr != '-')
    {
        ERR("unexpected year format %s\n", debugstr_w(ptr));
        return FALSE;
    }
    ptr++;

    num = wcstoul(ptr, &nextPtr, 10);
    if (!nextPtr || nextPtr <= ptr || num < 1601 || num > 30827)
    {
        ERR("unexpected year %s\n", debugstr_w(value));
        return FALSE;
    }
    ptr = nextPtr;
    st.wYear = (WORD)num;

    if (!HTTP_ParseTime(&st, &ptr))
        return FALSE;

    while (iswspace(*ptr))
        ptr++;

    if (wcscmp(ptr, L"GMT") && wcscmp(ptr, L"UTC"))
    {
        ERR("unexpected time zone %s\n", debugstr_w(ptr));
        return FALSE;
    }
    return SystemTimeToFileTime(&st, ft);
}

static BOOL HTTP_ParseDate(LPCWSTR value, FILETIME *ft)
{
    BOOL ret;

    if (!wcscmp(value, L"0"))
    {
        ft->dwLowDateTime = ft->dwHighDateTime = 0;
        ret = TRUE;
    }
    else if (wcschr(value, ','))
    {
        ret = HTTP_ParseRfc1123Date(value, ft);
        if (!ret)
        {
            ret = HTTP_ParseRfc850Date(value, ft);
            if (!ret)
                ERR("unexpected date format %s\n", debugstr_w(value));
        }
    }
    else
    {
        ret = HTTP_ParseDateAsAsctime(value, ft);
        if (!ret)
            ERR("unexpected date format %s\n", debugstr_w(value));
    }
    return ret;
}

static void HTTP_ProcessExpires(http_request_t *request)
{
    BOOL expirationFound = FALSE;
    int headerIndex;

    EnterCriticalSection( &request->headers_section );

    /* Look for a Cache-Control header with a max-age directive, as it takes
     * precedence over the Expires header.
     */
    headerIndex = HTTP_GetCustomHeaderIndex(request, L"Cache-Control", 0, FALSE);
    if (headerIndex != -1)
    {
        LPHTTPHEADERW ccHeader = &request->custHeaders[headerIndex];
        LPWSTR ptr;

        for (ptr = ccHeader->lpszValue; ptr && *ptr; )
        {
            LPWSTR comma = wcschr(ptr, ','), end, equal;

            if (comma)
                end = comma;
            else
                end = ptr + lstrlenW(ptr);
            for (equal = end - 1; equal > ptr && *equal != '='; equal--)
                ;
            if (*equal == '=')
            {
                if (!wcsnicmp(ptr, L"max-age", equal - ptr - 1))
                {
                    LPWSTR nextPtr;
                    unsigned long age;

                    age = wcstoul(equal + 1, &nextPtr, 10);
                    if (nextPtr > equal + 1)
                    {
                        LARGE_INTEGER ft;

                        NtQuerySystemTime( &ft );
                        /* Age is in seconds, FILETIME resolution is in
                         * 100 nanosecond intervals.
                         */
                        ft.QuadPart += age * (ULONGLONG)1000000;
                        request->expires.dwLowDateTime = ft.u.LowPart;
                        request->expires.dwHighDateTime = ft.u.HighPart;
                        expirationFound = TRUE;
                    }
                }
            }
            if (comma)
            {
                ptr = comma + 1;
                while (iswspace(*ptr))
                    ptr++;
            }
            else
                ptr = NULL;
        }
    }
    if (!expirationFound)
    {
        headerIndex = HTTP_GetCustomHeaderIndex(request, L"Expires", 0, FALSE);
        if (headerIndex != -1)
        {
            LPHTTPHEADERW expiresHeader = &request->custHeaders[headerIndex];
            FILETIME ft;

            if (HTTP_ParseDate(expiresHeader->lpszValue, &ft))
            {
                expirationFound = TRUE;
                request->expires = ft;
            }
        }
    }
    if (!expirationFound)
    {
        LARGE_INTEGER t;

        /* With no known age, default to 10 minutes until expiration. */
        NtQuerySystemTime( &t );
        t.QuadPart += 10 * 60 * (ULONGLONG)10000000;
        request->expires.dwLowDateTime = t.u.LowPart;
        request->expires.dwHighDateTime = t.u.HighPart;
    }

    LeaveCriticalSection( &request->headers_section );
}

static void HTTP_ProcessLastModified(http_request_t *request)
{
    int headerIndex;

    EnterCriticalSection( &request->headers_section );

    headerIndex = HTTP_GetCustomHeaderIndex(request, L"Last-Modified", 0, FALSE);
    if (headerIndex != -1)
    {
        LPHTTPHEADERW expiresHeader = &request->custHeaders[headerIndex];
        FILETIME ft;

        if (HTTP_ParseDate(expiresHeader->lpszValue, &ft))
            request->last_modified = ft;
    }

    LeaveCriticalSection( &request->headers_section );
}

static void http_process_keep_alive(http_request_t *req)
{
    const WCHAR *connection = L"";
    int index;

    EnterCriticalSection( &req->headers_section );

    if ((index = HTTP_GetCustomHeaderIndex(req, L"Connection", 0, FALSE)) != -1 ||
        (index = HTTP_GetCustomHeaderIndex(req, L"Proxy-Connection", 0, FALSE)) != -1)
        connection = req->custHeaders[index].lpszValue;
    req->netconn->keep_alive = has_keep_alive(req->version, connection);

    LeaveCriticalSection( &req->headers_section );
}

static DWORD open_http_connection(http_request_t *request, BOOL *reusing)
{
    netconn_t *netconn = NULL;
    DWORD res;

    if (request->netconn)
    {
        if (NETCON_is_alive(request->netconn) && drain_content(request, TRUE) == ERROR_SUCCESS)
        {
            reset_data_stream(request);
            *reusing = TRUE;
            return ERROR_SUCCESS;
        }

        TRACE("freeing netconn\n");
        free_netconn(request->netconn);
        request->netconn = NULL;
    }

    reset_data_stream(request);

    res = HTTP_ResolveName(request);
    if(res != ERROR_SUCCESS)
        return res;

    EnterCriticalSection(&connection_pool_cs);

    while(!list_empty(&request->server->conn_pool)) {
        netconn = LIST_ENTRY(list_head(&request->server->conn_pool), netconn_t, pool_entry);
        list_remove(&netconn->pool_entry);

        if(is_valid_netconn(netconn) && NETCON_is_alive(netconn))
            break;

        TRACE("connection %p closed during idle\n", netconn);
        free_netconn(netconn);
        netconn = NULL;
    }

    LeaveCriticalSection(&connection_pool_cs);

    if(netconn) {
        TRACE("<-- reusing %p netconn\n", netconn);
        request->netconn = netconn;
        *reusing = TRUE;
        return ERROR_SUCCESS;
    }

    TRACE("connecting to %s, proxy %s\n", debugstr_w(request->server->name),
          request->proxy ? debugstr_w(request->proxy->name) : "(null)");

    INTERNET_SendCallback(&request->hdr, request->hdr.dwContext,
                          INTERNET_STATUS_CONNECTING_TO_SERVER,
                          request->server->addr_str,
                          strlen(request->server->addr_str)+1);

    res = create_netconn(request->proxy ? request->proxy : request->server, request->security_flags,
                         (request->hdr.ErrorMask & INTERNET_ERROR_MASK_COMBINED_SEC_CERT) != 0,
                         request->hdr.connect_timeout, &netconn);
    if(res != ERROR_SUCCESS) {
        ERR("create_netconn failed: %lu\n", res);
        return res;
    }

    request->netconn = netconn;

    INTERNET_SendCallback(&request->hdr, request->hdr.dwContext,
            INTERNET_STATUS_CONNECTED_TO_SERVER,
            request->server->addr_str, strlen(request->server->addr_str)+1);

    *reusing = FALSE;
    TRACE("Created connection to %s: %p\n", debugstr_w(request->server->name), netconn);
    return ERROR_SUCCESS;
}

static char *build_ascii_request( const WCHAR *str, void *data, DWORD data_len, DWORD *out_len )
{
    int len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL );
    char *ret;

    if (!(ret = malloc( len + data_len ))) return NULL;
    WideCharToMultiByte( CP_ACP, 0, str, -1, ret, len, NULL, NULL );
    if (data_len) memcpy( ret + len - 1, data, data_len );
    *out_len = len + data_len - 1;
    ret[*out_len] = 0;
    return ret;
}

static void set_content_length_header( http_request_t *request, DWORD len, DWORD flags )
{
    WCHAR buf[ARRAY_SIZE(L"Content-Length: %u\r\n") + 10];

    swprintf( buf, ARRAY_SIZE(buf), L"Content-Length: %u\r\n", len );
    HTTP_HttpAddRequestHeadersW( request, buf, ~0u, flags );
}

/***********************************************************************
 *           HTTP_HttpSendRequestW (internal)
 *
 * Sends the specified request to the HTTP server
 *
 * RETURNS
 *    ERROR_SUCCESS on success
 *    win32 error code on failure
 *
 */
static DWORD HTTP_HttpSendRequestW(http_request_t *request, LPCWSTR lpszHeaders,
	DWORD dwHeaderLength, LPVOID lpOptional, DWORD dwOptionalLength,
	DWORD dwContentLength, BOOL bEndRequest)
{
    BOOL redirected = FALSE, secure_proxy_connect = FALSE, loop_next;
    WCHAR *request_header = NULL;
    INT responseLen, cnt;
    DWORD res;
    const WCHAR *appinfo_agent;

    TRACE("--> %p\n", request);

    assert(request->hdr.htype == WH_HHTTPREQ);

    /* if the verb is NULL default to GET */
    if (!request->verb)
        request->verb = wcsdup(L"GET");

    HTTP_ProcessHeader(request, L"Host", request->server->canon_host_port,
                       HTTP_ADDREQ_FLAG_ADD_IF_NEW | HTTP_ADDHDR_FLAG_REQ);

    if (dwContentLength || wcscmp(request->verb, L"GET"))
    {
        set_content_length_header(request, dwContentLength, HTTP_ADDREQ_FLAG_ADD_IF_NEW);
        request->bytesToWrite = dwContentLength;
    }

    appinfo_agent = request->session->appInfo->agent;

    if (appinfo_agent && *appinfo_agent)
    {
        WCHAR *agent_header;
        int len;

        len = lstrlenW(appinfo_agent) + lstrlenW(L"User-Agent: %s\r\n");
        agent_header = malloc(len * sizeof(WCHAR));
        swprintf(agent_header, len, L"User-Agent: %s\r\n", appinfo_agent);

        HTTP_HttpAddRequestHeadersW(request, agent_header, lstrlenW(agent_header), HTTP_ADDREQ_FLAG_ADD_IF_NEW);
        free(agent_header);
    }
    if (request->hdr.dwFlags & INTERNET_FLAG_PRAGMA_NOCACHE)
    {
        HTTP_HttpAddRequestHeadersW(request, L"Pragma: no-cache\r\n",
                                    lstrlenW(L"Pragma: no-cache\r\n"), HTTP_ADDREQ_FLAG_ADD_IF_NEW);
    }
    if ((request->hdr.dwFlags & INTERNET_FLAG_NO_CACHE_WRITE) && wcscmp(request->verb, L"GET"))
    {
        HTTP_HttpAddRequestHeadersW(request, L"Cache-Control: no-cache\r\n",
                                    lstrlenW(L"Cache-Control: no-cache\r\n"), HTTP_ADDREQ_FLAG_ADD_IF_NEW);
    }

    /* add the headers the caller supplied */
    if (lpszHeaders)
    {
        if (dwHeaderLength == 0)
            dwHeaderLength = lstrlenW(lpszHeaders);

        HTTP_HttpAddRequestHeadersW(request, lpszHeaders, dwHeaderLength, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE);
    }

    do
    {
        DWORD len, data_len = dwOptionalLength;
        BOOL reusing_connection;
        char *ascii_req;

        loop_next = FALSE;

        if(redirected) {
            request->contentLength = ~0;
            request->bytesToWrite = 0;
        }

        if (TRACE_ON(wininet))
        {
            HTTPHEADERW *host;

            EnterCriticalSection( &request->headers_section );
            host = HTTP_GetHeader(request, L"Host");
            TRACE("Going to url %s %s\n", debugstr_w(host->lpszValue), debugstr_w(request->path));
            LeaveCriticalSection( &request->headers_section );
        }

        HTTP_FixURL(request);
        if (request->hdr.dwFlags & INTERNET_FLAG_KEEP_CONNECTION)
        {
            HTTP_ProcessHeader(request, L"Connection", L"Keep-Alive",
                               HTTP_ADDHDR_FLAG_REQ | HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD);
        }
        HTTP_InsertAuthorization(request, request->authInfo, L"Authorization");
        HTTP_InsertAuthorization(request, request->proxyAuthInfo, L"Proxy-Authorization");

        if (!(request->hdr.dwFlags & INTERNET_FLAG_NO_COOKIES))
            HTTP_InsertCookies(request);

        res = open_http_connection(request, &reusing_connection);
        if (res != ERROR_SUCCESS)
            break;

        if (!reusing_connection && (request->hdr.dwFlags & INTERNET_FLAG_SECURE))
        {
            if (request->proxy) secure_proxy_connect = TRUE;
            else
            {
                res = NETCON_secure_connect(request->netconn, request->server);
                if (res != ERROR_SUCCESS)
                {
                    WARN("failed to upgrade to secure connection\n");
                    http_release_netconn(request, FALSE);
                    break;
                }
            }
        }
        if (secure_proxy_connect)
        {
            const WCHAR *target = request->server->host_port;

            if (HTTP_GetCustomHeaderIndex(request, L"Content-Length", 0, TRUE) >= 0)
                set_content_length_header(request, 0, HTTP_ADDREQ_FLAG_REPLACE);

            request_header = build_request_header(request, L"CONNECT", target, L"HTTP/1.1", TRUE);
        }
        else if (request->proxy && !(request->hdr.dwFlags & INTERNET_FLAG_SECURE))
        {
            WCHAR *url = build_proxy_path_url(request);
            request_header = build_request_header(request, request->verb, url, request->version, TRUE);
            free(url);
        }
        else
        {
            if (request->proxy && HTTP_GetCustomHeaderIndex(request, L"Content-Length", 0, TRUE) >= 0)
                set_content_length_header(request, dwContentLength, HTTP_ADDREQ_FLAG_REPLACE);

            request_header = build_request_header(request, request->verb, request->path, request->version, TRUE);
        }

        TRACE("Request header -> %s\n", debugstr_w(request_header) );

        /* send the request as ASCII, tack on the optional data */
        if (!lpOptional || redirected || secure_proxy_connect)
            data_len = 0;

        ascii_req = build_ascii_request(request_header, lpOptional, data_len, &len);
        free(request_header);
        TRACE("full request -> %s\n", debugstr_a(ascii_req) );

        INTERNET_SendCallback(&request->hdr, request->hdr.dwContext,
                              INTERNET_STATUS_SENDING_REQUEST, NULL, 0);

        NETCON_set_timeout( request->netconn, TRUE, request->hdr.send_timeout );
        res = NETCON_send(request->netconn, ascii_req, len, 0, &cnt);
        free(ascii_req);
        if(res != ERROR_SUCCESS) {
            TRACE("send failed: %lu\n", res);
            if(!reusing_connection)
                break;
            http_release_netconn(request, FALSE);
            loop_next = TRUE;
            continue;
        }

        request->bytesWritten = data_len;

        INTERNET_SendCallback(&request->hdr, request->hdr.dwContext,
                              INTERNET_STATUS_REQUEST_SENT,
                              &len, sizeof(DWORD));

        if (bEndRequest)
        {
            DWORD dwBufferSize;

            INTERNET_SendCallback(&request->hdr, request->hdr.dwContext,
                                INTERNET_STATUS_RECEIVING_RESPONSE, NULL, 0);

            if (HTTP_GetResponseHeaders(request, &responseLen))
            {
                http_release_netconn(request, FALSE);
                res = ERROR_INTERNET_CONNECTION_ABORTED;
                goto lend;
            }
            /* FIXME: We should know that connection is closed before sending
             * headers. Otherwise wrong callbacks are executed */
            if(!responseLen && reusing_connection) {
                TRACE("Connection closed by server, reconnecting\n");
                http_release_netconn(request, FALSE);
                loop_next = TRUE;
                continue;
            }

            INTERNET_SendCallback(&request->hdr, request->hdr.dwContext,
                                INTERNET_STATUS_RESPONSE_RECEIVED, &responseLen,
                                sizeof(DWORD));

            http_process_keep_alive(request);
            HTTP_ProcessCookies(request);
            HTTP_ProcessExpires(request);
            HTTP_ProcessLastModified(request);

            res = set_content_length(request);
            if(res != ERROR_SUCCESS)
                goto lend;
            if(!request->contentLength && !secure_proxy_connect)
                http_release_netconn(request, TRUE);

            if (!(request->hdr.dwFlags & INTERNET_FLAG_NO_AUTO_REDIRECT) && responseLen)
            {
                WCHAR *new_url;

                switch(request->status_code) {
                case HTTP_STATUS_REDIRECT:
                case HTTP_STATUS_MOVED:
                case HTTP_STATUS_REDIRECT_KEEP_VERB:
                case HTTP_STATUS_REDIRECT_METHOD:
                case HTTP_STATUS_PERMANENT_REDIRECT:
                    new_url = get_redirect_url(request);
                    if(!new_url)
                        break;

                    if (wcscmp(request->verb, L"GET") && wcscmp(request->verb, L"HEAD") &&
                        request->status_code != HTTP_STATUS_REDIRECT_KEEP_VERB &&
                        request->status_code != HTTP_STATUS_PERMANENT_REDIRECT)
                    {
                        free(request->verb);
                        request->verb = wcsdup(L"GET");
                    }
                    http_release_netconn(request, drain_content(request, FALSE) == ERROR_SUCCESS);
                    res = HTTP_HandleRedirect(request, new_url);
                    free(new_url);
                    if (res == ERROR_SUCCESS)
                        loop_next = TRUE;
                    redirected = TRUE;
                }
            }
            if (!(request->hdr.dwFlags & INTERNET_FLAG_NO_AUTH) && res == ERROR_SUCCESS)
            {
                WCHAR szAuthValue[2048];
                dwBufferSize=2048;
                if (request->status_code == HTTP_STATUS_DENIED)
                {
                    WCHAR *host = wcsdup(request->server->canon_host_port);
                    DWORD dwIndex = 0;
                    while (HTTP_HttpQueryInfoW(request,HTTP_QUERY_WWW_AUTHENTICATE,szAuthValue,&dwBufferSize,&dwIndex) == ERROR_SUCCESS)
                    {
                        if (HTTP_DoAuthorization(request, szAuthValue,
                                                 &request->authInfo,
                                                 request->session->userName,
                                                 request->session->password, host))
                        {
                            if (drain_content(request, TRUE) != ERROR_SUCCESS)
                            {
                                FIXME("Could not drain content\n");
                                http_release_netconn(request, FALSE);
                            }
                            loop_next = TRUE;
                            break;
                        }

                        dwBufferSize = 2048;
                    }
                    free(host);

                    if(!loop_next) {
                        TRACE("Cleaning wrong authorization data\n");
                        destroy_authinfo(request->authInfo);
                        request->authInfo = NULL;
                    }
                }
                if (request->status_code == HTTP_STATUS_PROXY_AUTH_REQ)
                {
                    DWORD dwIndex = 0;
                    while (HTTP_HttpQueryInfoW(request,HTTP_QUERY_PROXY_AUTHENTICATE,szAuthValue,&dwBufferSize,&dwIndex) == ERROR_SUCCESS)
                    {
                        if (HTTP_DoAuthorization(request, szAuthValue,
                                                 &request->proxyAuthInfo,
                                                 request->session->appInfo->proxyUsername,
                                                 request->session->appInfo->proxyPassword,
                                                 NULL))
                        {
                            if (drain_content(request, TRUE) != ERROR_SUCCESS)
                            {
                                FIXME("Could not drain content\n");
                                http_release_netconn(request, FALSE);
                            }
                            loop_next = TRUE;
                            break;
                        }

                        dwBufferSize = 2048;
                    }

                    if(!loop_next) {
                        TRACE("Cleaning wrong proxy authorization data\n");
                        destroy_authinfo(request->proxyAuthInfo);
                        request->proxyAuthInfo = NULL;
                    }
                }
            }
            if (secure_proxy_connect && request->status_code == HTTP_STATUS_OK)
            {
                res = NETCON_secure_connect(request->netconn, request->server);
                if (res != ERROR_SUCCESS)
                {
                    WARN("failed to upgrade to secure proxy connection\n");
                    http_release_netconn( request, FALSE );
                    break;
                }
                remove_header(request, L"Proxy-Authorization", TRUE);
                destroy_authinfo(request->proxyAuthInfo);
                request->proxyAuthInfo = NULL;
                request->contentLength = 0;
                request->netconn_stream.content_length = 0;

                secure_proxy_connect = FALSE;
                loop_next = TRUE;
            }
        }
        else
            res = ERROR_SUCCESS;
    }
    while (loop_next);

lend:
    /* TODO: send notification for P3P header */

    if(res == ERROR_SUCCESS)
        create_cache_entry(request);

    if (request->session->appInfo->hdr.dwFlags & INTERNET_FLAG_ASYNC)
    {
        if (res == ERROR_SUCCESS) {
            if(bEndRequest && request->contentLength && request->bytesWritten == request->bytesToWrite)
                HTTP_ReceiveRequestData(request);
            else
                send_request_complete(request,
                        request->session->hdr.dwInternalFlags & INET_OPENURL ? (DWORD_PTR)request->hdr.hInternet : 1, 0);
        }else {
                send_request_complete(request, 0, res);
        }
    }

    TRACE("<--\n");
    return res;
}

typedef struct {
    task_header_t hdr;
    WCHAR *headers;
    DWORD  headers_len;
    void  *optional;
    DWORD  optional_len;
    DWORD  content_len;
    BOOL   end_request;
} send_request_task_t;

/***********************************************************************
 *
 * Helper functions for the HttpSendRequest(Ex) functions
 *
 */
static void AsyncHttpSendRequestProc(task_header_t *hdr)
{
    send_request_task_t *task = (send_request_task_t*)hdr;
    http_request_t *request = (http_request_t*)task->hdr.hdr;

    TRACE("%p\n", request);

    HTTP_HttpSendRequestW(request, task->headers, task->headers_len, task->optional,
            task->optional_len, task->content_len, task->end_request);

    free(task->headers);
}


static DWORD HTTP_HttpEndRequestW(http_request_t *request, DWORD dwFlags, DWORD_PTR dwContext)
{
    INT responseLen;
    DWORD res = ERROR_SUCCESS;

    if(!is_valid_netconn(request->netconn)) {
        WARN("Not connected\n");
        send_request_complete(request, 0, ERROR_INTERNET_OPERATION_CANCELLED);
        return ERROR_INTERNET_OPERATION_CANCELLED;
    }

    INTERNET_SendCallback(&request->hdr, request->hdr.dwContext,
                  INTERNET_STATUS_RECEIVING_RESPONSE, NULL, 0);

    if (HTTP_GetResponseHeaders(request, &responseLen) || !responseLen)
        res = ERROR_HTTP_HEADER_NOT_FOUND;

    INTERNET_SendCallback(&request->hdr, request->hdr.dwContext,
                  INTERNET_STATUS_RESPONSE_RECEIVED, &responseLen, sizeof(DWORD));

    /* process cookies here. Is this right? */
    http_process_keep_alive(request);
    HTTP_ProcessCookies(request);
    HTTP_ProcessExpires(request);
    HTTP_ProcessLastModified(request);

    if ((res = set_content_length(request)) == ERROR_SUCCESS) {
        if(!request->contentLength)
            http_release_netconn(request, TRUE);
    }

    if (res == ERROR_SUCCESS && !(request->hdr.dwFlags & INTERNET_FLAG_NO_AUTO_REDIRECT))
    {
        switch(request->status_code) {
        case HTTP_STATUS_REDIRECT:
        case HTTP_STATUS_MOVED:
        case HTTP_STATUS_REDIRECT_METHOD:
        case HTTP_STATUS_REDIRECT_KEEP_VERB:
        case HTTP_STATUS_PERMANENT_REDIRECT: {
            WCHAR *new_url;

            new_url = get_redirect_url(request);
            if(!new_url)
                break;

            if (wcscmp(request->verb, L"GET") && wcscmp(request->verb, L"HEAD") &&
                request->status_code != HTTP_STATUS_REDIRECT_KEEP_VERB &&
                request->status_code != HTTP_STATUS_PERMANENT_REDIRECT)
            {
                free(request->verb);
                request->verb = wcsdup(L"GET");
            }
            http_release_netconn(request, drain_content(request, FALSE) == ERROR_SUCCESS);
            res = HTTP_HandleRedirect(request, new_url);
            free(new_url);
            if (res == ERROR_SUCCESS)
                res = HTTP_HttpSendRequestW(request, NULL, 0, NULL, 0, 0, TRUE);
        }
        }
    }

    if(res == ERROR_SUCCESS)
        create_cache_entry(request);

    if (res == ERROR_SUCCESS && request->contentLength)
        HTTP_ReceiveRequestData(request);
    else
        send_request_complete(request, res == ERROR_SUCCESS, res);

    return res;
}

/***********************************************************************
 *           HttpEndRequestA (WININET.@)
 *
 * Ends an HTTP request that was started by HttpSendRequestEx
 *
 * RETURNS
 *    TRUE	if successful
 *    FALSE	on failure
 *
 */
BOOL WINAPI HttpEndRequestA(HINTERNET hRequest,
        LPINTERNET_BUFFERSA lpBuffersOut, DWORD dwFlags, DWORD_PTR dwContext)
{
    TRACE("(%p, %p, %08lx, %08Ix)\n", hRequest, lpBuffersOut, dwFlags, dwContext);

    if (lpBuffersOut)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return HttpEndRequestW(hRequest, NULL, dwFlags, dwContext);
}

typedef struct {
    task_header_t hdr;
    DWORD flags;
    DWORD context;
} end_request_task_t;

static void AsyncHttpEndRequestProc(task_header_t *hdr)
{
    end_request_task_t *task = (end_request_task_t*)hdr;
    http_request_t *req = (http_request_t*)task->hdr.hdr;

    TRACE("%p\n", req);

    HTTP_HttpEndRequestW(req, task->flags, task->context);
}

/***********************************************************************
 *           HttpEndRequestW (WININET.@)
 *
 * Ends an HTTP request that was started by HttpSendRequestEx
 *
 * RETURNS
 *    TRUE	if successful
 *    FALSE	on failure
 *
 */
BOOL WINAPI HttpEndRequestW(HINTERNET hRequest,
        LPINTERNET_BUFFERSW lpBuffersOut, DWORD dwFlags, DWORD_PTR dwContext)
{
    http_request_t *request;
    DWORD res;

    TRACE("%p %p %lx %Ix -->\n", hRequest, lpBuffersOut, dwFlags, dwContext);

    if (lpBuffersOut)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    request = (http_request_t*) get_handle_object( hRequest );

    if (NULL == request || request->hdr.htype != WH_HHTTPREQ)
    {
        SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
        if (request)
            WININET_Release( &request->hdr );
        return FALSE;
    }
    request->hdr.dwFlags |= dwFlags;

    if (request->session->appInfo->hdr.dwFlags & INTERNET_FLAG_ASYNC)
    {
        end_request_task_t *task;

        task = alloc_async_task(&request->hdr, AsyncHttpEndRequestProc, sizeof(*task));
        task->flags = dwFlags;
        task->context = dwContext;

        INTERNET_AsyncCall(&task->hdr);
        res = ERROR_IO_PENDING;
    }
    else
        res = HTTP_HttpEndRequestW(request, dwFlags, dwContext);

    WININET_Release( &request->hdr );
    TRACE("%lu <--\n", res);
    if(res != ERROR_SUCCESS)
        SetLastError(res);
    return res == ERROR_SUCCESS;
}

/***********************************************************************
 *           HttpSendRequestExA (WININET.@)
 *
 * Sends the specified request to the HTTP server and allows chunked
 * transfers.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE, call GetLastError() for more information.
 */
BOOL WINAPI HttpSendRequestExA(HINTERNET hRequest,
			       LPINTERNET_BUFFERSA lpBuffersIn,
			       LPINTERNET_BUFFERSA lpBuffersOut,
			       DWORD dwFlags, DWORD_PTR dwContext)
{
    INTERNET_BUFFERSW BuffersInW;
    BOOL rc = FALSE;
    DWORD headerlen;
    LPWSTR header = NULL;

    TRACE("(%p, %p, %p, %08lx, %08Ix)\n", hRequest, lpBuffersIn,
	    lpBuffersOut, dwFlags, dwContext);

    if (lpBuffersIn)
    {
        BuffersInW.dwStructSize = sizeof(LPINTERNET_BUFFERSW);
        if (lpBuffersIn->lpcszHeader)
        {
            if (lpBuffersIn->dwHeadersLength == 0 && *lpBuffersIn->lpcszHeader != '\0')
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }

            headerlen = MultiByteToWideChar(CP_ACP,0,lpBuffersIn->lpcszHeader,
                    lpBuffersIn->dwHeadersLength,0,0);
            header = malloc(headerlen * sizeof(WCHAR));
            if (!(BuffersInW.lpcszHeader = header))
            {
                SetLastError(ERROR_OUTOFMEMORY);
                return FALSE;
            }
            BuffersInW.dwHeadersLength = MultiByteToWideChar(CP_ACP, 0,
                    lpBuffersIn->lpcszHeader, lpBuffersIn->dwHeadersLength,
                    header, headerlen);
        }
        else
            BuffersInW.lpcszHeader = NULL;
        BuffersInW.dwHeadersTotal = lpBuffersIn->dwHeadersTotal;
        BuffersInW.lpvBuffer = lpBuffersIn->lpvBuffer;
        BuffersInW.dwBufferLength = lpBuffersIn->dwBufferLength;
        BuffersInW.dwBufferTotal = lpBuffersIn->dwBufferTotal;
        BuffersInW.Next = NULL;
    }

    rc = HttpSendRequestExW(hRequest, lpBuffersIn ? &BuffersInW : NULL, NULL, dwFlags, dwContext);

    free(header);
    return rc;
}

/***********************************************************************
 *           HttpSendRequestExW (WININET.@)
 *
 * Sends the specified request to the HTTP server and allows chunked
 * transfers
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE, call GetLastError() for more information.
 */
BOOL WINAPI HttpSendRequestExW(HINTERNET hRequest,
                   LPINTERNET_BUFFERSW lpBuffersIn,
                   LPINTERNET_BUFFERSW lpBuffersOut,
                   DWORD dwFlags, DWORD_PTR dwContext)
{
    http_request_t *request;
    http_session_t *session;
    appinfo_t *hIC;
    DWORD res;

    TRACE("(%p, %p, %p, %08lx, %08Ix)\n", hRequest, lpBuffersIn,
            lpBuffersOut, dwFlags, dwContext);

    request = (http_request_t*) get_handle_object( hRequest );

    if (NULL == request || request->hdr.htype != WH_HHTTPREQ)
    {
        res = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        goto lend;
    }

    session = request->session;
    assert(session->hdr.htype == WH_HHTTPSESSION);
    hIC = session->appInfo;
    assert(hIC->hdr.htype == WH_HINIT);

    if (hIC->hdr.dwFlags & INTERNET_FLAG_ASYNC)
    {
        send_request_task_t *task;

        task = alloc_async_task(&request->hdr, AsyncHttpSendRequestProc, sizeof(*task));
        if (lpBuffersIn)
        {
            DWORD size = 0;

            if (lpBuffersIn->lpcszHeader)
            {
                if (lpBuffersIn->dwHeadersLength == ~0u)
                    size = (lstrlenW( lpBuffersIn->lpcszHeader ) + 1) * sizeof(WCHAR);
                else
                    size = lpBuffersIn->dwHeadersLength * sizeof(WCHAR);

                task->headers = malloc(size);
                memcpy(task->headers, lpBuffersIn->lpcszHeader, size);
            }
            else task->headers = NULL;

            task->headers_len = size / sizeof(WCHAR);
            task->optional = lpBuffersIn->lpvBuffer;
            task->optional_len = lpBuffersIn->dwBufferLength;
            task->content_len = lpBuffersIn->dwBufferTotal;
        }
        else
        {
            task->headers = NULL;
            task->headers_len = 0;
            task->optional = NULL;
            task->optional_len = 0;
            task->content_len = 0;
        }

        task->end_request = FALSE;

        INTERNET_AsyncCall(&task->hdr);
        res = ERROR_IO_PENDING;
    }
    else
    {
        if (lpBuffersIn)
            res = HTTP_HttpSendRequestW(request, lpBuffersIn->lpcszHeader, lpBuffersIn->dwHeadersLength,
                                        lpBuffersIn->lpvBuffer, lpBuffersIn->dwBufferLength,
                                        lpBuffersIn->dwBufferTotal, FALSE);
        else
            res = HTTP_HttpSendRequestW(request, NULL, 0, NULL, 0, 0, FALSE);
    }

lend:
    if ( request )
        WININET_Release( &request->hdr );

    TRACE("<---\n");
    SetLastError(res);
    return res == ERROR_SUCCESS;
}

/***********************************************************************
 *           HttpSendRequestW (WININET.@)
 *
 * Sends the specified request to the HTTP server
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HttpSendRequestW(HINTERNET hHttpRequest, LPCWSTR lpszHeaders,
	DWORD dwHeaderLength, LPVOID lpOptional ,DWORD dwOptionalLength)
{
    http_request_t *request;
    http_session_t *session = NULL;
    appinfo_t *hIC = NULL;
    DWORD res = ERROR_SUCCESS;

    TRACE("%p, %s, %li, %p, %li)\n", hHttpRequest,
            debugstr_wn(lpszHeaders, dwHeaderLength), dwHeaderLength, lpOptional, dwOptionalLength);

    request = (http_request_t*) get_handle_object( hHttpRequest );
    if (NULL == request || request->hdr.htype != WH_HHTTPREQ)
    {
        res = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        goto lend;
    }

    session = request->session;
    if (NULL == session ||  session->hdr.htype != WH_HHTTPSESSION)
    {
        res = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        goto lend;
    }

    hIC = session->appInfo;
    if (NULL == hIC ||  hIC->hdr.htype != WH_HINIT)
    {
        res = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        goto lend;
    }

    if (hIC->hdr.dwFlags & INTERNET_FLAG_ASYNC)
    {
        send_request_task_t *task;

        task = alloc_async_task(&request->hdr, AsyncHttpSendRequestProc, sizeof(*task));
        if (lpszHeaders)
        {
            DWORD size;

            if (dwHeaderLength == ~0u) size = (lstrlenW(lpszHeaders) + 1) * sizeof(WCHAR);
            else size = dwHeaderLength * sizeof(WCHAR);

            task->headers = malloc(size);
            memcpy(task->headers, lpszHeaders, size);
        }
        else
            task->headers = NULL;
        task->headers_len = dwHeaderLength;
        task->optional = lpOptional;
        task->optional_len = dwOptionalLength;
        task->content_len = dwOptionalLength;
        task->end_request = TRUE;

        INTERNET_AsyncCall(&task->hdr);
        res = ERROR_IO_PENDING;
    }
    else
    {
	res = HTTP_HttpSendRequestW(request, lpszHeaders,
		dwHeaderLength, lpOptional, dwOptionalLength,
		dwOptionalLength, TRUE);
    }
lend:
    if( request )
        WININET_Release( &request->hdr );

    SetLastError(res);
    return res == ERROR_SUCCESS;
}

/***********************************************************************
 *           HttpSendRequestA (WININET.@)
 *
 * Sends the specified request to the HTTP server
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HttpSendRequestA(HINTERNET hHttpRequest, LPCSTR lpszHeaders,
	DWORD dwHeaderLength, LPVOID lpOptional ,DWORD dwOptionalLength)
{
    BOOL result;
    LPWSTR szHeaders=NULL;
    DWORD nLen=dwHeaderLength;
    if(lpszHeaders!=NULL)
    {
        if (dwHeaderLength == 0 && *lpszHeaders != '\0')
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        nLen=MultiByteToWideChar(CP_ACP,0,lpszHeaders,dwHeaderLength,NULL,0);
        szHeaders = malloc(nLen * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP,0,lpszHeaders,dwHeaderLength,szHeaders,nLen);
    }
    result = HttpSendRequestW(hHttpRequest, szHeaders, nLen, lpOptional, dwOptionalLength);
    free(szHeaders);
    return result;
}

/***********************************************************************
 *           HTTPSESSION_Destroy (internal)
 *
 * Deallocate session handle
 *
 */
static void HTTPSESSION_Destroy(object_header_t *hdr)
{
    http_session_t *session = (http_session_t*) hdr;

    TRACE("%p\n", session);

    WININET_Release(&session->appInfo->hdr);

    free(session->hostName);
    free(session->password);
    free(session->userName);
}

static DWORD HTTPSESSION_QueryOption(object_header_t *hdr, DWORD option, void *buffer, DWORD *size, BOOL unicode)
{

    switch(option) {
    case INTERNET_OPTION_HANDLE_TYPE:
        TRACE("INTERNET_OPTION_HANDLE_TYPE\n");

        if (*size < sizeof(ULONG))
            return ERROR_INSUFFICIENT_BUFFER;

        *size = sizeof(DWORD);
        *(DWORD*)buffer = INTERNET_HANDLE_TYPE_CONNECT_HTTP;
        return ERROR_SUCCESS;
    }

    return INET_QueryOption(hdr, option, buffer, size, unicode);
}

static DWORD HTTPSESSION_SetOption(object_header_t *hdr, DWORD option, void *buffer, DWORD size)
{
    http_session_t *ses = (http_session_t*)hdr;

    switch(option) {
    case INTERNET_OPTION_USERNAME:
    {
        free(ses->userName);
        if (!(ses->userName = wcsdup(buffer))) return ERROR_OUTOFMEMORY;
        return ERROR_SUCCESS;
    }
    case INTERNET_OPTION_PASSWORD:
    {
        free(ses->password);
        if (!(ses->password = wcsdup(buffer))) return ERROR_OUTOFMEMORY;
        return ERROR_SUCCESS;
    }
    case INTERNET_OPTION_PROXY_USERNAME:
    {
        free(ses->appInfo->proxyUsername);
        if (!(ses->appInfo->proxyUsername = wcsdup(buffer))) return ERROR_OUTOFMEMORY;
        return ERROR_SUCCESS;
    }
    case INTERNET_OPTION_PROXY_PASSWORD:
    {
        free(ses->appInfo->proxyPassword);
        if (!(ses->appInfo->proxyPassword = wcsdup(buffer))) return ERROR_OUTOFMEMORY;
        return ERROR_SUCCESS;
    }
    default: break;
    }

    return INET_SetOption(hdr, option, buffer, size);
}

static const object_vtbl_t HTTPSESSIONVtbl = {
    HTTPSESSION_Destroy,
    NULL,
    HTTPSESSION_QueryOption,
    HTTPSESSION_SetOption,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};


/***********************************************************************
 *           HTTP_Connect  (internal)
 *
 * Create http session handle
 *
 * RETURNS
 *   HINTERNET a session handle on success
 *   NULL on failure
 *
 */
DWORD HTTP_Connect(appinfo_t *hIC, LPCWSTR lpszServerName,
        INTERNET_PORT serverPort, LPCWSTR lpszUserName,
        LPCWSTR lpszPassword, DWORD dwFlags, DWORD_PTR dwContext,
        DWORD dwInternalFlags, HINTERNET *ret)
{
    http_session_t *session = NULL;

    TRACE("-->\n");

    if (!lpszServerName || !lpszServerName[0])
        return ERROR_INVALID_PARAMETER;

    assert( hIC->hdr.htype == WH_HINIT );

    session = alloc_object(&hIC->hdr, &HTTPSESSIONVtbl, sizeof(http_session_t));
    if (!session)
        return ERROR_OUTOFMEMORY;

   /*
    * According to my tests. The name is not resolved until a request is sent
    */

    session->hdr.htype = WH_HHTTPSESSION;
    session->hdr.dwFlags = dwFlags;
    session->hdr.dwContext = dwContext;
    session->hdr.dwInternalFlags |= dwInternalFlags;
    session->hdr.decoding = hIC->hdr.decoding;

    WININET_AddRef( &hIC->hdr );
    session->appInfo = hIC;
    list_add_head( &hIC->hdr.children, &session->hdr.entry );

    session->hostName = wcsdup(lpszServerName);
    if (lpszUserName && lpszUserName[0])
        session->userName = wcsdup(lpszUserName);
    session->password = wcsdup(lpszPassword);
    session->hostPort = serverPort;
    session->hdr.connect_timeout = hIC->hdr.connect_timeout;
    session->hdr.send_timeout = hIC->hdr.send_timeout;
    session->hdr.receive_timeout = hIC->hdr.receive_timeout;

    /* Don't send a handle created callback if this handle was created with InternetOpenUrl */
    if (!(session->hdr.dwInternalFlags & INET_OPENURL))
    {
        INTERNET_SendCallback(&hIC->hdr, dwContext,
                              INTERNET_STATUS_HANDLE_CREATED, &session->hdr.hInternet,
                              sizeof(HINTERNET));
    }

/*
 * an INTERNET_STATUS_REQUEST_COMPLETE is NOT sent here as per my tests on
 * windows
 */

    TRACE("%p --> %p\n", hIC, session);

    *ret = session->hdr.hInternet;
    return ERROR_SUCCESS;
}

/***********************************************************************
 *           HTTP_clear_response_headers (internal)
 *
 * clear out any old response headers
 */
static void HTTP_clear_response_headers( http_request_t *request )
{
    DWORD i;

    EnterCriticalSection( &request->headers_section );

    for( i=0; i<request->nCustHeaders; i++)
    {
        if( !request->custHeaders[i].lpszField )
            continue;
        if( !request->custHeaders[i].lpszValue )
            continue;
        if ( request->custHeaders[i].wFlags & HDR_ISREQUEST )
            continue;
        HTTP_DeleteCustomHeader( request, i );
        i--;
    }

    LeaveCriticalSection( &request->headers_section );
}

/***********************************************************************
 *           HTTP_GetResponseHeaders (internal)
 *
 * Read server response
 *
 * RETURNS
 *
 *   TRUE  on success
 *   FALSE on error
 */
static DWORD HTTP_GetResponseHeaders(http_request_t *request, INT *len)
{
    INT cbreaks = 0;
    WCHAR buffer[MAX_REPLY_LEN];
    DWORD buflen = MAX_REPLY_LEN;
    INT  rc = 0;
    char bufferA[MAX_REPLY_LEN];
    LPWSTR status_code = NULL, status_text = NULL;
    DWORD res = ERROR_HTTP_INVALID_SERVER_RESPONSE;
    BOOL codeHundred = FALSE;

    TRACE("-->\n");

    if(!is_valid_netconn(request->netconn))
        goto lend;

    /* clear old response headers (eg. from a redirect response) */
    HTTP_clear_response_headers( request );

    NETCON_set_timeout( request->netconn, FALSE, request->hdr.receive_timeout );
    do {
        /*
         * We should first receive 'HTTP/1.x nnn OK' where nnn is the status code.
         */
        buflen = MAX_REPLY_LEN;
        if ((res = read_line(request, bufferA, &buflen)))
            goto lend;

        if (!buflen) goto lend;

        rc += buflen;
        MultiByteToWideChar( CP_ACP, 0, bufferA, buflen, buffer, MAX_REPLY_LEN );
        /* check is this a status code line? */
        if (!wcsncmp(buffer, L"HTTP/1.0", 4))
        {
            /* split the version from the status code */
            status_code = wcschr( buffer, ' ' );
            if( !status_code )
                goto lend;
            *status_code++=0;

            /* split the status code from the status text */
            status_text = wcschr( status_code, ' ' );
            if( status_text )
                *status_text++=0;

            request->status_code = wcstol(status_code, NULL, 10);

            TRACE("version [%s] status code [%s] status text [%s]\n",
               debugstr_w(buffer), debugstr_w(status_code), debugstr_w(status_text) );

            codeHundred = request->status_code == HTTP_STATUS_CONTINUE;
        }
        else if (!codeHundred)
        {
            WARN("No status line at head of response (%s)\n", debugstr_w(buffer));

            free(request->version);
            free(request->statusText);

            request->status_code = HTTP_STATUS_OK;
            request->version = wcsdup(L"HTTP/1.0");
            request->statusText = wcsdup(L"OK");

            goto lend;
        }
    } while (codeHundred);

    /* Add status code */
    HTTP_ProcessHeader(request, L"Status", status_code,
                       HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD);

    free(request->version);
    free(request->statusText);

    request->version = wcsdup(buffer);
    request->statusText = wcsdup(status_text ? status_text : L"");

    /* Restore the spaces */
    *(status_code-1) = ' ';
    if (status_text)
        *(status_text-1) = ' ';

    /* Parse each response line */
    do
    {
        buflen = MAX_REPLY_LEN;
        if (!read_line(request, bufferA, &buflen) && buflen)
        {
            LPWSTR * pFieldAndValue;

            TRACE("got line %s, now interpreting\n", debugstr_a(bufferA));

            if (!bufferA[0]) break;
            MultiByteToWideChar( CP_ACP, 0, bufferA, buflen, buffer, MAX_REPLY_LEN );

            pFieldAndValue = HTTP_InterpretHttpHeader(buffer);
            if (pFieldAndValue)
            {
                HTTP_ProcessHeader(request, pFieldAndValue[0], pFieldAndValue[1],
                                   HTTP_ADDREQ_FLAG_ADD );
                HTTP_FreeTokens(pFieldAndValue);
            }
        }
        else
        {
            cbreaks++;
            if (cbreaks >= 2)
                break;
        }
    }while(1);

    res = ERROR_SUCCESS;

lend:

    *len = rc;
    TRACE("<--\n");
    return res;
}

/***********************************************************************
 *           HTTP_InterpretHttpHeader (internal)
 *
 * Parse server response
 *
 * RETURNS
 *
 *   Pointer to array of field, value, NULL on success.
 *   NULL on error.
 */
static LPWSTR * HTTP_InterpretHttpHeader(LPCWSTR buffer)
{
    LPWSTR * pTokenPair;
    LPWSTR pszColon;
    INT len;

    pTokenPair = calloc(3, sizeof(*pTokenPair));

    pszColon = wcschr(buffer, ':');
    /* must have two tokens */
    if (!pszColon)
    {
        HTTP_FreeTokens(pTokenPair);
        if (buffer[0])
            TRACE("No ':' in line: %s\n", debugstr_w(buffer));
        return NULL;
    }

    pTokenPair[0] = malloc((pszColon - buffer + 1) * sizeof(WCHAR));
    if (!pTokenPair[0])
    {
        HTTP_FreeTokens(pTokenPair);
        return NULL;
    }
    memcpy(pTokenPair[0], buffer, (pszColon - buffer) * sizeof(WCHAR));
    pTokenPair[0][pszColon - buffer] = '\0';

    /* skip colon */
    pszColon++;
    len = lstrlenW(pszColon);
    pTokenPair[1] = malloc((len + 1) * sizeof(WCHAR));
    if (!pTokenPair[1])
    {
        HTTP_FreeTokens(pTokenPair);
        return NULL;
    }
    memcpy(pTokenPair[1], pszColon, (len + 1) * sizeof(WCHAR));

    strip_spaces(pTokenPair[0]);
    strip_spaces(pTokenPair[1]);

    TRACE("field(%s) Value(%s)\n", debugstr_w(pTokenPair[0]), debugstr_w(pTokenPair[1]));
    return pTokenPair;
}

/***********************************************************************
 *           HTTP_ProcessHeader (internal)
 *
 * Stuff header into header tables according to <dwModifier>
 *
 */

#define COALESCEFLAGS (HTTP_ADDREQ_FLAG_COALESCE_WITH_COMMA|HTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON)

static DWORD HTTP_ProcessHeader(http_request_t *request, LPCWSTR field, LPCWSTR value, DWORD dwModifier)
{
    LPHTTPHEADERW lphttpHdr = NULL;
    INT index;
    BOOL request_only = !!(dwModifier & HTTP_ADDHDR_FLAG_REQ);
    DWORD res = ERROR_HTTP_INVALID_HEADER;

    TRACE("--> %s: %s - 0x%08lx\n", debugstr_w(field), debugstr_w(value), dwModifier);

    EnterCriticalSection( &request->headers_section );

    /* REPLACE wins out over ADD */
    if (dwModifier & HTTP_ADDREQ_FLAG_REPLACE)
        dwModifier &= ~HTTP_ADDREQ_FLAG_ADD;

    if (dwModifier & HTTP_ADDREQ_FLAG_ADD)
        index = -1;
    else
        index = HTTP_GetCustomHeaderIndex(request, field, 0, request_only);

    if (index >= 0)
    {
        if (dwModifier & HTTP_ADDREQ_FLAG_ADD_IF_NEW)
        {
            LeaveCriticalSection( &request->headers_section );
            return ERROR_HTTP_INVALID_HEADER;
        }
        lphttpHdr = &request->custHeaders[index];
    }
    else if (value)
    {
        HTTPHEADERW hdr;

        hdr.lpszField = (LPWSTR)field;
        hdr.lpszValue = (LPWSTR)value;
        hdr.wFlags = hdr.wCount = 0;

        if (dwModifier & HTTP_ADDHDR_FLAG_REQ)
            hdr.wFlags |= HDR_ISREQUEST;

        res = HTTP_InsertCustomHeader(request, &hdr);
        LeaveCriticalSection( &request->headers_section );
        return res;
    }
    /* no value to delete */
    else
    {
        LeaveCriticalSection( &request->headers_section );
        return ERROR_SUCCESS;
    }

    if (dwModifier & HTTP_ADDHDR_FLAG_REQ)
	    lphttpHdr->wFlags |= HDR_ISREQUEST;
    else
        lphttpHdr->wFlags &= ~HDR_ISREQUEST;

    if (dwModifier & HTTP_ADDREQ_FLAG_REPLACE)
    {
        HTTP_DeleteCustomHeader( request, index );

        if (value && value[0])
        {
            HTTPHEADERW hdr;

            hdr.lpszField = (LPWSTR)field;
            hdr.lpszValue = (LPWSTR)value;
            hdr.wFlags = hdr.wCount = 0;

            if (dwModifier & HTTP_ADDHDR_FLAG_REQ)
                hdr.wFlags |= HDR_ISREQUEST;

            res = HTTP_InsertCustomHeader(request, &hdr);
            LeaveCriticalSection( &request->headers_section );
            return res;
        }

        LeaveCriticalSection( &request->headers_section );
        return ERROR_SUCCESS;
    }
    else if (dwModifier & COALESCEFLAGS)
    {
        LPWSTR lpsztmp;
        WCHAR ch = 0;
        INT len = 0;
        INT origlen = lstrlenW(lphttpHdr->lpszValue);
        INT valuelen = lstrlenW(value);

        if (dwModifier & HTTP_ADDREQ_FLAG_COALESCE_WITH_COMMA)
        {
            ch = ',';
            lphttpHdr->wFlags |= HDR_COMMADELIMITED;
        }
        else if (dwModifier & HTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON)
        {
            ch = ';';
            lphttpHdr->wFlags |= HDR_COMMADELIMITED;
        }

        len = origlen + valuelen + ((ch > 0) ? 2 : 0);

        lpsztmp = realloc(lphttpHdr->lpszValue, (len + 1) * sizeof(WCHAR));
        if (lpsztmp)
        {
            lphttpHdr->lpszValue = lpsztmp;
    /* FIXME: Increment lphttpHdr->wCount. Perhaps lpszValue should be an array */
            if (ch > 0)
            {
                lphttpHdr->lpszValue[origlen] = ch;
                origlen++;
                lphttpHdr->lpszValue[origlen] = ' ';
                origlen++;
            }

            memcpy(&lphttpHdr->lpszValue[origlen], value, valuelen*sizeof(WCHAR));
            lphttpHdr->lpszValue[len] = '\0';
            res = ERROR_SUCCESS;
        }
        else
        {
            WARN("realloc (%d bytes) failed\n",len+1);
            res = ERROR_OUTOFMEMORY;
        }
    }
    TRACE("<-- %ld\n", res);
    LeaveCriticalSection( &request->headers_section );
    return res;
}

/***********************************************************************
 *           HTTP_GetCustomHeaderIndex (internal)
 *
 * Return index of custom header from header array
 * Headers section must be held
 */
static INT HTTP_GetCustomHeaderIndex(http_request_t *request, LPCWSTR lpszField,
                                     int requested_index, BOOL request_only)
{
    DWORD index;

    TRACE("%s, %d, %d\n", debugstr_w(lpszField), requested_index, request_only);

    for (index = 0; index < request->nCustHeaders; index++)
    {
        if (wcsicmp(request->custHeaders[index].lpszField, lpszField))
            continue;

        if (request_only && !(request->custHeaders[index].wFlags & HDR_ISREQUEST))
            continue;

        if (!request_only && (request->custHeaders[index].wFlags & HDR_ISREQUEST))
            continue;

        if (requested_index == 0)
            break;
        requested_index --;
    }

    if (index >= request->nCustHeaders)
	index = -1;

    TRACE("Return: %ld\n", index);
    return index;
}


/***********************************************************************
 *           HTTP_InsertCustomHeader (internal)
 *
 * Insert header into array
 * Headers section must be held
 */
static DWORD HTTP_InsertCustomHeader(http_request_t *request, LPHTTPHEADERW lpHdr)
{
    INT count;
    LPHTTPHEADERW lph = NULL;

    TRACE("--> %s: %s\n", debugstr_w(lpHdr->lpszField), debugstr_w(lpHdr->lpszValue));
    count = request->nCustHeaders + 1;
    lph = realloc(request->custHeaders, sizeof(HTTPHEADERW) * count);

    if (!lph)
        return ERROR_OUTOFMEMORY;

    request->custHeaders = lph;
    request->custHeaders[count-1].lpszField = wcsdup(lpHdr->lpszField);
    request->custHeaders[count-1].lpszValue = wcsdup(lpHdr->lpszValue);
    request->custHeaders[count-1].wFlags = lpHdr->wFlags;
    request->custHeaders[count-1].wCount= lpHdr->wCount;
    request->nCustHeaders++;

    return ERROR_SUCCESS;
}


/***********************************************************************
 *           HTTP_DeleteCustomHeader (internal)
 *
 * Delete header from array
 * If this function is called, the index may change.
 * Headers section must be held
 */
static BOOL HTTP_DeleteCustomHeader(http_request_t *request, DWORD index)
{
    if( request->nCustHeaders <= 0 )
        return FALSE;
    if( index >= request->nCustHeaders )
        return FALSE;
    request->nCustHeaders--;

    free(request->custHeaders[index].lpszField);
    free(request->custHeaders[index].lpszValue);

    memmove( &request->custHeaders[index], &request->custHeaders[index+1],
             (request->nCustHeaders - index)* sizeof(HTTPHEADERW) );
    memset( &request->custHeaders[request->nCustHeaders], 0, sizeof(HTTPHEADERW) );

    return TRUE;
}


/***********************************************************************
 *          IsHostInProxyBypassList (WININET.@)
 */
BOOL WINAPI IsHostInProxyBypassList(INTERNET_SCHEME scheme, LPCSTR szHost, DWORD length)
{
    FIXME("STUB: scheme=%d host=%s length=%ld\n", scheme, szHost, length);
    return FALSE;
}
