/*
 * DNS support
 *
 * Copyright (C) 2006 Hans Leidekker
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

#include "config.h"
#include "wine/port.h"
#include "wine/debug.h"

#include <stdarg.h>
#include <string.h>
#include <sys/types.h>

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_NAMESER_H
# include <arpa/nameser.h>
#endif
#ifdef HAVE_RESOLV_H
# include <resolv.h>
#endif
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"
#include "windns.h"

#include "dnsapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(dnsapi);

#ifdef HAVE_RESOLV

static CRITICAL_SECTION resolver_cs;
static CRITICAL_SECTION_DEBUG resolver_cs_debug =
{
    0, 0, &resolver_cs,
    { &resolver_cs_debug.ProcessLocksList,
      &resolver_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": resolver_cs") }
};
static CRITICAL_SECTION resolver_cs = { &resolver_cs_debug, -1, 0, 0, 0, 0 };

#define LOCK_RESOLVER()     do { EnterCriticalSection( &resolver_cs ); } while (0)
#define UNLOCK_RESOLVER()   do { LeaveCriticalSection( &resolver_cs ); } while (0)


/*  the resolver lock must be held and res_init() must have been
 *  called before calling this function.
 */
static DNS_STATUS dns_get_serverlist( PIP4_ARRAY addrs, PDWORD len )
{
    unsigned int i, size;

    size = sizeof(IP4_ARRAY) + sizeof(IP4_ADDRESS) * (_res.nscount - 1);
    if (!addrs || *len < size)
    {
        *len = size;
        return ERROR_INSUFFICIENT_BUFFER;
    }

    addrs->AddrCount = _res.nscount;

    for (i = 0; i < _res.nscount; i++)
        addrs->AddrArray[i] = _res.nsaddr_list[i].sin_addr.s_addr;

    return ERROR_SUCCESS;
}

#endif /* HAVE_RESOLV */

static DNS_STATUS dns_get_hostname_a( COMPUTER_NAME_FORMAT format,
                                      LPSTR buffer, PDWORD len )
{
    char name[256];
    DWORD size = sizeof(name);

    if (!GetComputerNameExA( format, name, &size ))
        return DNS_ERROR_NAME_DOES_NOT_EXIST;

    if (!buffer || (size = lstrlenA( name ) + 1) > *len)
    {
        *len = size;
        return ERROR_INSUFFICIENT_BUFFER;
    }

    lstrcpyA( buffer, name );
    return ERROR_SUCCESS;
}

static DNS_STATUS dns_get_hostname_w( COMPUTER_NAME_FORMAT format,
                                      LPWSTR buffer, PDWORD len )
{
    WCHAR name[256];
    DWORD size = sizeof(name);

    if (!GetComputerNameExW( format, name, &size ))
        return DNS_ERROR_NAME_DOES_NOT_EXIST;

    if (!buffer || (size = lstrlenW( name ) + 1) > *len)
    {
        *len = size;
        return ERROR_INSUFFICIENT_BUFFER;
    }

    lstrcpyW( buffer, name );
    return ERROR_SUCCESS;
}

/******************************************************************************
 * DnsQueryConfig          [DNSAPI.@]
 *
 */
DNS_STATUS WINAPI DnsQueryConfig( DNS_CONFIG_TYPE config, DWORD flag, PWSTR adapter,
                                  PVOID reserved, PVOID buffer, PDWORD len )
{
    DNS_STATUS ret = ERROR_INVALID_PARAMETER;

    TRACE( "(%d,0x%08lx,%s,%p,%p,%p)\n", config, flag, debugstr_w(adapter),
           reserved, buffer, len );

    if (!len) return ERROR_INVALID_PARAMETER;

    switch (config)
    {
    case DnsConfigDnsServerList:
    {
#ifdef HAVE_RESOLV
        LOCK_RESOLVER();

        res_init();
        ret = dns_get_serverlist( (IP4_ARRAY *)buffer, len );

        UNLOCK_RESOLVER();
        break;
#else
        WARN( "compiled without resolver support\n" );
        break;
#endif
    }
    case DnsConfigHostName_A:
    case DnsConfigHostName_UTF8:
        return dns_get_hostname_a( ComputerNameDnsHostname, buffer, len );

    case DnsConfigFullHostName_A:
    case DnsConfigFullHostName_UTF8:
        return dns_get_hostname_a( ComputerNameDnsFullyQualified, buffer, len );

    case DnsConfigPrimaryDomainName_A:
    case DnsConfigPrimaryDomainName_UTF8:
        return dns_get_hostname_a( ComputerNameDnsDomain, buffer, len );

    case DnsConfigHostName_W:
        return dns_get_hostname_w( ComputerNameDnsHostname, buffer, len );

    case DnsConfigFullHostName_W:
        return dns_get_hostname_w( ComputerNameDnsFullyQualified, buffer, len );

    case DnsConfigPrimaryDomainName_W:
        return dns_get_hostname_w( ComputerNameDnsDomain, buffer, len );

    case DnsConfigAdapterDomainName_A:
    case DnsConfigAdapterDomainName_W:
    case DnsConfigAdapterDomainName_UTF8:
    case DnsConfigSearchList:
    case DnsConfigAdapterInfo:
    case DnsConfigPrimaryHostNameRegistrationEnabled:
    case DnsConfigAdapterHostNameRegistrationEnabled:
    case DnsConfigAddressRegistrationMaxCount:
        FIXME( "unimplemented config type %d\n", config );
        break;

    default:
        WARN( "unknown config type: %d\n", config );
        break;
    }
    return ret;
}
