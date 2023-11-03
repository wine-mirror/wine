/*
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
#include "winsock2.h"
#include "windef.h"
#include "winbase.h"
#include "iphlpapi.h"
#include "dhcpcsdk.h"
#include "wine/test.h"

static IP_ADAPTER_ADDRESSES *get_adapters(void)
{
    ULONG err, size = 1024;
    for (;;)
    {
        IP_ADAPTER_ADDRESSES *ret = malloc( size );
        err = GetAdaptersAddresses( AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
                                               GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME,
                                    NULL, ret, &size );
        if (err == ERROR_SUCCESS) return ret;
        free( ret );
        if (err != ERROR_BUFFER_OVERFLOW) return NULL;
    }
}

static void test_DhcpRequestParams(void)
{
    static WCHAR nosuchW[] = L"nosuchadapter";
    DHCPCAPI_PARAMS params[6];
    DHCPCAPI_PARAMS_ARRAY send_params, recv_params;
    IP_ADAPTER_ADDRESSES *adapters, *ptr;
    BYTE *buf;
    WCHAR name[MAX_ADAPTER_NAME_LENGTH + 1];
    DWORD err, size, i;

    if (!(adapters = get_adapters())) return;

    for (ptr = adapters; ptr; ptr = ptr->Next)
    {
        MultiByteToWideChar( CP_ACP, 0, ptr->AdapterName, -1, name, ARRAY_SIZE(name) );
        trace( "adapter '%s' type %lu dhcpv4 enabled %d\n", wine_dbgstr_w(ptr->Description), ptr->IfType, ptr->Dhcpv4Enabled );

        if (ptr->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;

        memset( &send_params, 0, sizeof(send_params) );
        memset( &recv_params, 0, sizeof(recv_params) );
        err = DhcpRequestParams( DHCPCAPI_REQUEST_SYNCHRONOUS, NULL, NULL, NULL, send_params, recv_params, NULL, NULL, NULL );
        ok( err == ERROR_INVALID_PARAMETER, "got %lu\n", err );

        err = DhcpRequestParams( DHCPCAPI_REQUEST_SYNCHRONOUS, NULL, nosuchW, NULL, send_params, recv_params, NULL, NULL, NULL );
        ok( err == ERROR_INVALID_PARAMETER, "got %lu\n", err );

        err = DhcpRequestParams( DHCPCAPI_REQUEST_SYNCHRONOUS, NULL, name, NULL, send_params, recv_params, NULL, NULL, NULL );
        ok( err == ERROR_INVALID_PARAMETER, "got %lu\n", err );

        size = 0;
        err = DhcpRequestParams( DHCPCAPI_REQUEST_SYNCHRONOUS, NULL, name, NULL, send_params, recv_params, NULL, &size, NULL );
        ok( err == ERROR_INVALID_PARAMETER, "got %lu\n", err );

        memset( params, 0, sizeof(params) );
        params[0].OptionId = OPTION_SUBNET_MASK;
        params[1].OptionId = OPTION_ROUTER_ADDRESS;
        params[2].OptionId = OPTION_HOST_NAME;
        params[3].OptionId = OPTION_DOMAIN_NAME;
        params[4].OptionId = OPTION_BROADCAST_ADDRESS;
        params[5].OptionId = OPTION_MSFT_IE_PROXY;
        recv_params.nParams = 6;
        recv_params.Params  = params;

        size = 0;
        buf = NULL;
        err = DhcpRequestParams( DHCPCAPI_REQUEST_SYNCHRONOUS, NULL, name, NULL, send_params, recv_params,
                                 buf, &size, NULL );
        while (err == ERROR_MORE_DATA)
        {
            buf = realloc( buf, size );
            err = DhcpRequestParams( DHCPCAPI_REQUEST_SYNCHRONOUS, NULL, name, NULL, send_params, recv_params,
                                     buf, &size, NULL );
        }
        if (err == ERROR_SUCCESS)
        {
            for (i = 0; i < ARRAY_SIZE(params); i++)
            {
                switch( params[i].OptionId )
                {
                case OPTION_SUBNET_MASK:
                case OPTION_ROUTER_ADDRESS:
                case OPTION_BROADCAST_ADDRESS:
                    if (params[i].Data)
                    {
                        ok( params[i].nBytesData == sizeof(DWORD), "got %lu\n", params[i].nBytesData );
                        trace( "%lu: Data %p (%08lx) nBytesData %lu OptionId %lu Flags %08lx IsVendor %d\n",
                               i, params[i].Data, *(DWORD *)params[i].Data, params[i].nBytesData, params[i].OptionId,
                               params[i].Flags, params[i].IsVendor );
                    }
                    break;
                case OPTION_HOST_NAME:
                case OPTION_DOMAIN_NAME:
                case OPTION_MSFT_IE_PROXY:
                    if (params[i].Data)
                    {
                        char *str = malloc( params[i].nBytesData + 1 );
                        memcpy( str, params[i].Data, params[i].nBytesData );
                        str[params[i].nBytesData] = 0;
                        trace( "%lu: Data %p (%s) nBytesData %lu OptionId %lu Flags %08lx IsVendor %d\n",
                               i, params[i].Data, str, params[i].nBytesData, params[i].OptionId,
                               params[i].Flags, params[i].IsVendor );
                        free( str );
                    }
                    break;
                default:
                    ok( 0, "unexpected option %lu\n", params[i].OptionId );
                    break;
                }
            }
        }
        free( buf );
    }
    free( adapters );
}

START_TEST(dhcpcsvc)
{
    test_DhcpRequestParams();
}
