/*
 * Wininet - Utility functions
 *
 * Copyright 1999 Corel Corporation
 * Copyright 2002 CodeWeavers Inc.
 *
 * Ulrich Czekalla
 * Aric Stewart
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

#include "ws2tcpip.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "windef.h"
#include "winbase.h"
#include "wininet.h"
#include "winnls.h"

#include "wine/debug.h"
#include "internet.h"

WINE_DEFAULT_DEBUG_CHANNEL(wininet);

server_addr_t *GetAddress(const WCHAR *name, INTERNET_PORT port)
{
    struct sockaddr_storage *addr;
    server_addr_t *server_addr;
    ADDRINFOW *res, hints;
    unsigned int len;
    int ret;

    TRACE("%s\n", debugstr_w(name));

    memset( &hints, 0, sizeof(hints) );
    /* Prefer IPv4 to IPv6 addresses, since some servers do not listen on
     * their IPv6 addresses even though they have IPv6 addresses in the DNS.
     */
    hints.ai_family = AF_INET;

    ret = GetAddrInfoW(name, NULL, &hints, &res);
    if (ret != 0)
    {
        TRACE("failed to get IPv4 address of %s, retrying with IPv6\n", debugstr_w(name));
        hints.ai_family = AF_INET6;
        ret = GetAddrInfoW(name, NULL, &hints, &res);
    }
    if (ret != 0)
    {
        TRACE("failed to get address of %s\n", debugstr_w(name));
        return NULL;
    }
    server_addr = malloc(sizeof(*server_addr));
    addr = &server_addr->addr;
    server_addr->addr_len = res->ai_addrlen;
    memcpy( addr, res->ai_addr, res->ai_addrlen );
    /* Copy port */
    switch (res->ai_family)
    {
    case AF_INET:
        ((struct sockaddr_in *)addr)->sin_port = htons(port);
        inet_ntop(res->ai_family, &((struct sockaddr_in *)addr)->sin_addr, server_addr->addr_str, INET6_ADDRSTRLEN);
        break;
    case AF_INET6:
        ((struct sockaddr_in6 *)addr)->sin6_port = htons(port);
        server_addr->addr_str[0] = '[';
        inet_ntop(res->ai_family, &((struct sockaddr_in6 *)addr)->sin6_addr, server_addr->addr_str + 1, INET6_ADDRSTRLEN - 2);
        len = strlen(server_addr->addr_str);
        server_addr->addr_str[len] = ']';
        server_addr->addr_str[len + 1] = 0;
        break;
    }

    FreeAddrInfoW(res);
    return server_addr;
}

/*
 * Helper function for sending async Callbacks
 */

static const char *get_callback_name(DWORD dwInternetStatus) {
    static const wininet_flag_info internet_status[] = {
#define FE(x) { x, #x }
	FE(INTERNET_STATUS_RESOLVING_NAME),
	FE(INTERNET_STATUS_NAME_RESOLVED),
	FE(INTERNET_STATUS_CONNECTING_TO_SERVER),
	FE(INTERNET_STATUS_CONNECTED_TO_SERVER),
	FE(INTERNET_STATUS_SENDING_REQUEST),
	FE(INTERNET_STATUS_REQUEST_SENT),
	FE(INTERNET_STATUS_RECEIVING_RESPONSE),
	FE(INTERNET_STATUS_RESPONSE_RECEIVED),
	FE(INTERNET_STATUS_CTL_RESPONSE_RECEIVED),
	FE(INTERNET_STATUS_PREFETCH),
	FE(INTERNET_STATUS_CLOSING_CONNECTION),
	FE(INTERNET_STATUS_CONNECTION_CLOSED),
	FE(INTERNET_STATUS_HANDLE_CREATED),
	FE(INTERNET_STATUS_HANDLE_CLOSING),
	FE(INTERNET_STATUS_REQUEST_COMPLETE),
	FE(INTERNET_STATUS_REDIRECT),
	FE(INTERNET_STATUS_INTERMEDIATE_RESPONSE),
	FE(INTERNET_STATUS_USER_INPUT_REQUIRED),
	FE(INTERNET_STATUS_STATE_CHANGE),
	FE(INTERNET_STATUS_COOKIE_SENT),
	FE(INTERNET_STATUS_COOKIE_RECEIVED),
	FE(INTERNET_STATUS_PRIVACY_IMPACTED),
	FE(INTERNET_STATUS_P3P_HEADER),
	FE(INTERNET_STATUS_P3P_POLICYREF),
	FE(INTERNET_STATUS_COOKIE_HISTORY)
#undef FE
    };
    DWORD i;

    for (i = 0; i < ARRAY_SIZE(internet_status); i++) {
	if (internet_status[i].val == dwInternetStatus) return internet_status[i].name;
    }
    return "Unknown";
}

static const char *debugstr_status_info(DWORD status, void *info)
{
    switch(status) {
    case INTERNET_STATUS_REQUEST_COMPLETE: {
        INTERNET_ASYNC_RESULT *iar = info;
        return wine_dbg_sprintf("{%s, %ld}", wine_dbgstr_longlong(iar->dwResult), iar->dwError);
    }
    default:
        return wine_dbg_sprintf("%p", info);
    }
}

void INTERNET_SendCallback(object_header_t *hdr, DWORD_PTR context, DWORD status, void *info, DWORD info_len)
{
    void *new_info = info;

    if( !hdr->lpfnStatusCB )
        return;

    /* the IE5 version of wininet does not
       send callbacks if dwContext is zero */
    if(!context)
        return;

    switch(status) {
    case INTERNET_STATUS_NAME_RESOLVED:
    case INTERNET_STATUS_CONNECTING_TO_SERVER:
    case INTERNET_STATUS_CONNECTED_TO_SERVER:
        new_info = malloc(info_len);
        if(new_info)
            memcpy(new_info, info, info_len);
        break;
    case INTERNET_STATUS_RESOLVING_NAME:
    case INTERNET_STATUS_REDIRECT:
        if(hdr->dwInternalFlags & INET_CALLBACKW) {
            new_info = wcsdup(info);
            break;
        }else {
            new_info = strdupWtoA(info);
            info_len = strlen(new_info)+1;
            break;
        }
    }
    
    TRACE(" callback(%p) (%p (%p), %08Ix, %ld (%s), %s, %ld)\n",
	  hdr->lpfnStatusCB, hdr->hInternet, hdr, context, status, get_callback_name(status),
	  debugstr_status_info(status, new_info), info_len);
    
    hdr->lpfnStatusCB(hdr->hInternet, context, status, new_info, info_len);

    TRACE(" end callback().\n");

    if(new_info != info)
        free(new_info);
}
