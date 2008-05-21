/*
 * Copyright 2008 Juan Lang
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "snmp.h"
#include "iphlpapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(inetmib1);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

	switch (fdwReason)
	{
		case DLL_WINE_PREATTACH:
			return FALSE;    /* prefer native version */
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls(hinstDLL);
			break;
		case DLL_PROCESS_DETACH:
			break;
		default:
			break;
	}

	return TRUE;
}

static UINT mib2[] = { 1,3,6,1,2,1 };
static UINT mib2System[] = { 1,3,6,1,2,1,1 };

struct mibImplementation
{
    AsnObjectIdentifier name;
    void              (*init)(void);
};

static UINT mib2IfNumber[] = { 1,3,6,1,2,1,2,1 };
static PMIB_IFTABLE ifTable;

static void mib2IfNumberInit(void)
{
    DWORD size = 0, ret = GetIfTable(NULL, &size, FALSE);

    if (ret == ERROR_INSUFFICIENT_BUFFER)
    {
        ifTable = HeapAlloc(GetProcessHeap(), 0, size);
        if (ifTable)
            GetIfTable(ifTable, &size, FALSE);
    }
}

static struct mibImplementation supportedIDs[] = {
    { DEFINE_OID(mib2IfNumber), mib2IfNumberInit },
};

BOOL WINAPI SnmpExtensionInit(DWORD dwUptimeReference,
    HANDLE *phSubagentTrapEvent, AsnObjectIdentifier *pFirstSupportedRegion)
{
    AsnObjectIdentifier myOid = DEFINE_OID(mib2System);
    UINT i;

    TRACE("(%d, %p, %p)\n", dwUptimeReference, phSubagentTrapEvent,
        pFirstSupportedRegion);

    for (i = 0; i < sizeof(supportedIDs) / sizeof(supportedIDs[0]); i++)
        if (supportedIDs[i].init)
            supportedIDs[i].init();
    *phSubagentTrapEvent = NULL;
    SnmpUtilOidCpy(pFirstSupportedRegion, &myOid);
    return TRUE;
}

BOOL WINAPI SnmpExtensionQuery(BYTE bPduType, SnmpVarBindList *pVarBindList,
    AsnInteger32 *pErrorStatus, AsnInteger32 *pErrorIndex)
{
    AsnObjectIdentifier mib2oid = DEFINE_OID(mib2);
    AsnInteger32 error = SNMP_ERRORSTATUS_NOERROR, errorIndex = 0;
    UINT i;

    TRACE("(0x%02x, %p, %p, %p)\n", bPduType, pVarBindList,
        pErrorStatus, pErrorIndex);

    for (i = 0; !error && i < pVarBindList->len; i++)
    {
        /* Ignore any OIDs not in MIB2 */
        if (!SnmpUtilOidNCmp(&pVarBindList->list[i].name, &mib2oid,
            mib2oid.idLength))
        {
            FIXME("%s: stub\n", SnmpUtilOidToA(&pVarBindList->list[i].name));
            error = SNMP_ERRORSTATUS_NOSUCHNAME;
        }
    }
    *pErrorStatus = error;
    *pErrorIndex = errorIndex;
    return TRUE;
}
