/*
 * Copyright 2011 Stefan Leichter
 * Copyright 2019 Hans Leidekker for CodeWeavers
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
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "dhcpcsdk.h"
#include "winioctl.h"
#include "winternl.h"
#include "ws2def.h"
#include "ws2ipdef.h"
#include "iphlpapi.h"
#include "netioapi.h"
#define WINE_MOUNTMGR_EXTENSIONS
#include "ddk/mountmgr.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dhcpcsvc);

void WINAPI DhcpCApiCleanup(void)
{
    FIXME(": stub\n");
}

DWORD WINAPI DhcpCApiInitialize(LPDWORD version)
{
    *version = 2; /* 98, XP, and 8 */
    FIXME(": stub\n");
    return ERROR_SUCCESS;
}

static DWORD get_adapter_luid( const WCHAR *adapter, NET_LUID *luid )
{
    UNICODE_STRING ustr;
    NTSTATUS status;
    GUID guid;

    if (adapter[0] == '{')
    {
        RtlInitUnicodeString( &ustr, adapter );
        status = RtlGUIDFromString( &ustr, &guid );
        if (!status) return ConvertInterfaceGuidToLuid( &guid, luid );
    }
    return ConvertInterfaceNameToLuidW( adapter, luid );
}

#define IF_NAMESIZE 16
static DWORD get_adapter_name( const WCHAR *adapter, char *unix_name, DWORD len )
{
    WCHAR unix_nameW[IF_NAMESIZE];
    NET_LUID luid;
    DWORD ret;

    if ((ret = get_adapter_luid( adapter, &luid ))) return ret;
    if ((ret = ConvertInterfaceLuidToAlias( &luid, unix_nameW, ARRAY_SIZE(unix_nameW) ))) return ret;
    if (!WideCharToMultiByte( CP_UNIXCP, 0, unix_nameW, -1, unix_name, len, NULL, NULL ))
        return ERROR_INVALID_PARAMETER;
    return ERROR_SUCCESS;
}

DWORD WINAPI DhcpRequestParams( DWORD flags, void *reserved, WCHAR *adapter, DHCPCAPI_CLASSID *class_id,
                                DHCPCAPI_PARAMS_ARRAY send_params, DHCPCAPI_PARAMS_ARRAY recv_params, BYTE *buf,
                                DWORD *buflen, WCHAR *request_id )
{
    struct mountmgr_dhcp_request_params *query;
    DWORD i, size, err;
    BYTE *src, *dst;
    HANDLE mgr;
    char unix_name[IF_NAMESIZE];

    TRACE( "(%08lx, %p, %s, %p, %lu, %lu, %p, %p, %s)\n", flags, reserved, debugstr_w(adapter), class_id,
           send_params.nParams, recv_params.nParams, buf, buflen, debugstr_w(request_id) );

    if (!adapter || !buflen) return ERROR_INVALID_PARAMETER;
    if (flags != DHCPCAPI_REQUEST_SYNCHRONOUS) FIXME( "unsupported flags %08lx\n", flags );
    if ((err = get_adapter_name( adapter, unix_name, sizeof(unix_name) ))) return err;

    for (i = 0; i < send_params.nParams; i++)
        FIXME( "send option %lu not supported\n", send_params.Params->OptionId );

    mgr = CreateFileW( MOUNTMGR_DOS_DEVICE_NAME, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                       OPEN_EXISTING, 0, 0 );
    if (mgr == INVALID_HANDLE_VALUE) return GetLastError();

    size = FIELD_OFFSET(struct mountmgr_dhcp_request_params, params[recv_params.nParams]) + *buflen;
    if (!(query = calloc( 1, size )))
    {
        err = ERROR_OUTOFMEMORY;
        goto done;
    }
    for (i = 0; i < recv_params.nParams; i++) query->params[i].id = recv_params.Params[i].OptionId;
    query->count = recv_params.nParams;
    strcpy( query->unix_name, unix_name );

    if (!DeviceIoControl( mgr, IOCTL_MOUNTMGR_QUERY_DHCP_REQUEST_PARAMS, query, size, query, size, NULL, NULL ))
    {
        err = GetLastError();
        if (err == ERROR_MORE_DATA) *buflen = query->size - (size - *buflen);
        goto done;
    }

    dst = buf;
    for (i = 0; i < query->count; i++)
    {
        if (buf)
        {
            recv_params.Params[i].OptionId = query->params[i].id;
            recv_params.Params[i].IsVendor = FALSE; /* FIXME */
            if (query->params[i].size)
            {
                src = (BYTE *)query + query->params[i].offset;
                memcpy( dst, src, query->params[i].size );

                recv_params.Params[i].Data = dst;
                recv_params.Params[i].nBytesData = query->params[i].size;
            }
            else
            {
                recv_params.Params[i].Data = NULL;
                recv_params.Params[i].nBytesData = 0;
            }
        }
        dst += query->params[i].size;
    }

    *buflen = dst - buf;
    err = ERROR_SUCCESS;

done:
    free( query );
    CloseHandle( mgr );
    return err;
}
