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
#include <limits.h>
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

/**
 * Utility functions
 */
static void copyInt(AsnAny *value, void *src)
{
    value->asnType = ASN_INTEGER;
    value->asnValue.number = *(DWORD *)src;
}

static UINT mib2[] = { 1,3,6,1,2,1 };
static UINT mib2System[] = { 1,3,6,1,2,1,1 };

typedef BOOL (*varqueryfunc)(BYTE bPduType, SnmpVarBind *pVarBind,
    AsnInteger32 *pErrorStatus);

struct mibImplementation
{
    AsnObjectIdentifier name;
    void              (*init)(void);
    varqueryfunc        query;
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

static BOOL mib2IfNumberQuery(BYTE bPduType, SnmpVarBind *pVarBind,
    AsnInteger32 *pErrorStatus)
{
    AsnObjectIdentifier numberOid = DEFINE_OID(mib2IfNumber);

    TRACE("(0x%02x, %s, %p)\n", bPduType, SnmpUtilOidToA(&pVarBind->name),
        pErrorStatus);

    switch (bPduType)
    {
    case SNMP_PDU_GET:
    case SNMP_PDU_GETNEXT:
        if ((bPduType == SNMP_PDU_GET &&
            !SnmpUtilOidNCmp(&pVarBind->name, &numberOid, numberOid.idLength))
            || SnmpUtilOidNCmp(&pVarBind->name, &numberOid, numberOid.idLength)
            < 0)
        {
            DWORD numIfs = ifTable ? ifTable->dwNumEntries : 0;

            copyInt(&pVarBind->value, &numIfs);
            if (bPduType == SNMP_PDU_GETNEXT)
                SnmpUtilOidCpy(&pVarBind->name, &numberOid);
            *pErrorStatus = SNMP_ERRORSTATUS_NOERROR;
        }
        else
        {
            *pErrorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
            /* Caller deals with OID if bPduType == SNMP_PDU_GETNEXT, so don't
             * need to set it here.
             */
        }
        break;
    case SNMP_PDU_SET:
        *pErrorStatus = SNMP_ERRORSTATUS_READONLY;
        break;
    default:
        FIXME("0x%02x: unsupported PDU type\n", bPduType);
        *pErrorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
    }
    return TRUE;
}

/* This list MUST BE lexicographically sorted */
static struct mibImplementation supportedIDs[] = {
    { DEFINE_OID(mib2IfNumber), mib2IfNumberInit, mib2IfNumberQuery },
};
static UINT minSupportedIDLength;

BOOL WINAPI SnmpExtensionInit(DWORD dwUptimeReference,
    HANDLE *phSubagentTrapEvent, AsnObjectIdentifier *pFirstSupportedRegion)
{
    AsnObjectIdentifier myOid = DEFINE_OID(mib2System);
    UINT i;

    TRACE("(%d, %p, %p)\n", dwUptimeReference, phSubagentTrapEvent,
        pFirstSupportedRegion);

    minSupportedIDLength = UINT_MAX;
    for (i = 0; i < sizeof(supportedIDs) / sizeof(supportedIDs[0]); i++)
    {
        if (supportedIDs[i].init)
            supportedIDs[i].init();
        if (supportedIDs[i].name.idLength < minSupportedIDLength)
            minSupportedIDLength = supportedIDs[i].name.idLength;
    }
    *phSubagentTrapEvent = NULL;
    SnmpUtilOidCpy(pFirstSupportedRegion, &myOid);
    return TRUE;
}

static struct mibImplementation *findSupportedQuery(UINT *ids, UINT idLength,
    UINT *matchingIndex)
{
    int indexHigh = DEFINE_SIZEOF(supportedIDs) - 1, indexLow = 0, i;
    struct mibImplementation *impl = NULL;
    AsnObjectIdentifier oid1 = { idLength, ids};

    if (!idLength)
        return NULL;
    for (i = (indexLow + indexHigh) / 2; !impl && indexLow <= indexHigh;
         i = (indexLow + indexHigh) / 2)
    {
        INT cmp;

        cmp = SnmpUtilOidNCmp(&oid1, &supportedIDs[i].name, idLength);
        if (!cmp)
        {
            impl = &supportedIDs[i];
            *matchingIndex = i;
        }
        else if (cmp > 0)
            indexLow = i + 1;
        else
            indexHigh = i - 1;
    }
    return impl;
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
            struct mibImplementation *impl = NULL;
            UINT len, matchingIndex = 0;

            TRACE("%s\n", SnmpUtilOidToA(&pVarBindList->list[i].name));
            /* Search for an implementation matching as many octets as possible
             */
            for (len = pVarBindList->list[i].name.idLength;
                len >= minSupportedIDLength && !impl; len--)
                impl = findSupportedQuery(pVarBindList->list[i].name.ids, len,
                    &matchingIndex);
            if (impl && impl->query)
                impl->query(bPduType, &pVarBindList->list[i], &error);
            else
                error = SNMP_ERRORSTATUS_NOSUCHNAME;
            if (error == SNMP_ERRORSTATUS_NOSUCHNAME &&
                bPduType == SNMP_PDU_GETNEXT)
            {
                /* GetNext is special: it finds the successor to the given OID,
                 * so we have to continue until an implementation handles the
                 * query or we exhaust the table of supported OIDs.
                 */
                for (; error == SNMP_ERRORSTATUS_NOSUCHNAME &&
                    matchingIndex < DEFINE_SIZEOF(supportedIDs);
                    matchingIndex++)
                {
                    error = SNMP_ERRORSTATUS_NOERROR;
                    impl = &supportedIDs[matchingIndex];
                    if (impl->query)
                        impl->query(bPduType, &pVarBindList->list[i], &error);
                    else
                        error = SNMP_ERRORSTATUS_NOSUCHNAME;
                }
                /* If the query still isn't resolved, set the OID to the
                 * successor to the last entry in the table.
                 */
                if (error == SNMP_ERRORSTATUS_NOSUCHNAME)
                {
                    SnmpUtilOidFree(&pVarBindList->list[i].name);
                    SnmpUtilOidCpy(&pVarBindList->list[i].name,
                        &supportedIDs[matchingIndex - 1].name);
                    pVarBindList->list[i].name.ids[
                        pVarBindList->list[i].name.idLength - 1] += 1;
                }
            }
            if (error)
                errorIndex = i + 1;
        }
    }
    *pErrorStatus = error;
    *pErrorIndex = errorIndex;
    return TRUE;
}
