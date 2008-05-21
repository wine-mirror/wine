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
#include <assert.h>
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

static void setStringValue(AsnAny *value, BYTE type, DWORD len, BYTE *str)
{
    AsnAny strValue;

    strValue.asnType = type;
    strValue.asnValue.string.stream = str;
    strValue.asnValue.string.length = len;
    strValue.asnValue.string.dynamic = TRUE;
    SnmpUtilAsnAnyCpy(value, &strValue);
}

static void copyLengthPrecededString(AsnAny *value, void *src)
{
    DWORD len = *(DWORD *)src;

    setStringValue(value, ASN_OCTETSTRING, len, (BYTE *)src + sizeof(DWORD));
}

typedef void (*copyValueFunc)(AsnAny *value, void *src);

struct structToAsnValue
{
    size_t        offset;
    copyValueFunc copy;
};

static AsnInteger32 mapStructEntryToValue(struct structToAsnValue *map,
    UINT mapLen, void *record, UINT id, BYTE bPduType, SnmpVarBind *pVarBind)
{
    /* OIDs are 1-based */
    if (!id)
        return SNMP_ERRORSTATUS_NOSUCHNAME;
    --id;
    if (id >= mapLen)
        return SNMP_ERRORSTATUS_NOSUCHNAME;
    if (!map[id].copy)
        return SNMP_ERRORSTATUS_NOSUCHNAME;
    map[id].copy(&pVarBind->value, (BYTE *)record + map[id].offset);
    return SNMP_ERRORSTATUS_NOERROR;
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

static void copyOperStatus(AsnAny *value, void *src)
{
    value->asnType = ASN_INTEGER;
    /* The IPHlpApi definition of operational status differs from the MIB2 one,
     * so map it to the MIB2 value.
     */
    switch (*(DWORD *)src)
    {
    case MIB_IF_OPER_STATUS_OPERATIONAL:
        value->asnValue.number = MIB_IF_ADMIN_STATUS_UP;
        break;
    case MIB_IF_OPER_STATUS_CONNECTING:
    case MIB_IF_OPER_STATUS_CONNECTED:
        value->asnValue.number = MIB_IF_ADMIN_STATUS_TESTING;
        break;
    default:
        value->asnValue.number = MIB_IF_ADMIN_STATUS_DOWN;
    };
}

static struct structToAsnValue mib2IfEntryMap[] = {
    { FIELD_OFFSET(MIB_IFROW, dwIndex), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwDescrLen), copyLengthPrecededString },
    { FIELD_OFFSET(MIB_IFROW, dwType), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwMtu), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwSpeed), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwPhysAddrLen), copyLengthPrecededString },
    { FIELD_OFFSET(MIB_IFROW, dwAdminStatus), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwOperStatus), copyOperStatus },
    { FIELD_OFFSET(MIB_IFROW, dwLastChange), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwInOctets), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwInUcastPkts), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwInNUcastPkts), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwInDiscards), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwInErrors), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwInUnknownProtos), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwOutOctets), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwOutUcastPkts), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwOutNUcastPkts), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwOutDiscards), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwOutErrors), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwOutQLen), copyInt },
};

static UINT mib2IfEntry[] = { 1,3,6,1,2,1,2,2,1 };

static BOOL mib2IfEntryQuery(BYTE bPduType, SnmpVarBind *pVarBind,
    AsnInteger32 *pErrorStatus)
{
    AsnObjectIdentifier entryOid = DEFINE_OID(mib2IfEntry);

    TRACE("(0x%02x, %s, %p)\n", bPduType, SnmpUtilOidToA(&pVarBind->name),
        pErrorStatus);

    switch (bPduType)
    {
    case SNMP_PDU_GET:
    case SNMP_PDU_GETNEXT:
        if (!ifTable)
        {
            /* There is no interface present, so let the caller deal
             * with finding the successor.
             */
            *pErrorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
        }
        else if (!SnmpUtilOidNCmp(&pVarBind->name, &entryOid, entryOid.idLength))
        {
            UINT tableIndex = 0, item = 0;

            *pErrorStatus = 0;
            if (pVarBind->name.idLength == entryOid.idLength ||
                pVarBind->name.idLength == entryOid.idLength + 1)
            {
                /* Either the table or an element within the table is specified,
                 * but the instance is not.
                 */
                if (bPduType == SNMP_PDU_GET)
                {
                    /* Can't get an interface entry without specifying the
                     * instance.
                     */
                    *pErrorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
                }
                else
                {
                    /* Get the first interface */
                    tableIndex = 1;
                    if (pVarBind->name.idLength == entryOid.idLength + 1)
                        item = pVarBind->name.ids[entryOid.idLength];
                    else
                        item = 1;
                }
            }
            else
            {
                tableIndex = pVarBind->name.ids[entryOid.idLength + 1];
                item = pVarBind->name.ids[entryOid.idLength];
                if (bPduType == SNMP_PDU_GETNEXT)
                {
                    tableIndex++;
                    item = 1;
                }
            }
            if (!*pErrorStatus)
            {
                assert(tableIndex);
                assert(item);
                if (tableIndex > ifTable->dwNumEntries)
                    *pErrorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
                else
                {
                    *pErrorStatus = mapStructEntryToValue(mib2IfEntryMap,
                        DEFINE_SIZEOF(mib2IfEntryMap),
                        &ifTable->table[tableIndex - 1], item, bPduType,
                        pVarBind);
                    if (bPduType == SNMP_PDU_GETNEXT)
                    {
                        AsnObjectIdentifier oid;

                        SnmpUtilOidCpy(&pVarBind->name, &entryOid);
                        oid.idLength = 1;
                        oid.ids = &item;
                        SnmpUtilOidAppend(&pVarBind->name, &oid);
                        oid.idLength = 1;
                        oid.ids = &ifTable->table[tableIndex - 1].dwIndex;
                        SnmpUtilOidAppend(&pVarBind->name, &oid);
                    }
                }
            }
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

static UINT mib2Ip[] = { 1,3,6,1,2,1,4 };
static MIB_IPSTATS ipStats;

static void mib2IpStatsInit(void)
{
    GetIpStatistics(&ipStats);
}

static struct structToAsnValue mib2IpMap[] = {
    { FIELD_OFFSET(MIB_IPSTATS, dwForwarding), copyInt }, /* 1 */
    { FIELD_OFFSET(MIB_IPSTATS, dwDefaultTTL), copyInt }, /* 2 */
    { FIELD_OFFSET(MIB_IPSTATS, dwInReceives), copyInt }, /* 3 */
    { FIELD_OFFSET(MIB_IPSTATS, dwInHdrErrors), copyInt }, /* 4 */
    { FIELD_OFFSET(MIB_IPSTATS, dwInAddrErrors), copyInt }, /* 5 */
    { FIELD_OFFSET(MIB_IPSTATS, dwForwDatagrams), copyInt }, /* 6 */
    { FIELD_OFFSET(MIB_IPSTATS, dwInUnknownProtos), copyInt }, /* 7 */
    { FIELD_OFFSET(MIB_IPSTATS, dwInDiscards), copyInt }, /* 8 */
    { FIELD_OFFSET(MIB_IPSTATS, dwInDelivers), copyInt }, /* 9 */
    { FIELD_OFFSET(MIB_IPSTATS, dwOutRequests), copyInt }, /* 10 */
    { FIELD_OFFSET(MIB_IPSTATS, dwOutDiscards), copyInt }, /* 11 */
    { FIELD_OFFSET(MIB_IPSTATS, dwOutNoRoutes), copyInt }, /* 12 */
    { FIELD_OFFSET(MIB_IPSTATS, dwReasmTimeout), copyInt }, /* 13 */
    { FIELD_OFFSET(MIB_IPSTATS, dwReasmReqds), copyInt }, /* 14 */
    { FIELD_OFFSET(MIB_IPSTATS, dwReasmOks), copyInt }, /* 15 */
    { FIELD_OFFSET(MIB_IPSTATS, dwReasmFails), copyInt }, /* 16 */
    { FIELD_OFFSET(MIB_IPSTATS, dwFragOks), copyInt }, /* 17 */
    { FIELD_OFFSET(MIB_IPSTATS, dwFragFails), copyInt }, /* 18 */
    { FIELD_OFFSET(MIB_IPSTATS, dwFragCreates), copyInt }, /* 19 */
    { 0, NULL }, /* 20: not used, IP addr table */
    { 0, NULL }, /* 21: not used, route table */
    { 0, NULL }, /* 22: not used, net to media (ARP) table */
    { FIELD_OFFSET(MIB_IPSTATS, dwRoutingDiscards), copyInt }, /* 23 */
};

static BOOL mib2IpStatsQuery(BYTE bPduType, SnmpVarBind *pVarBind,
    AsnInteger32 *pErrorStatus)
{
    AsnObjectIdentifier myOid = DEFINE_OID(mib2Ip);
    UINT item = 0;

    TRACE("(0x%02x, %s, %p)\n", bPduType, SnmpUtilOidToA(&pVarBind->name),
        pErrorStatus);

    switch (bPduType)
    {
    case SNMP_PDU_GET:
        if (!SnmpUtilOidNCmp(&pVarBind->name, &myOid, myOid.idLength) &&
            pVarBind->name.idLength == myOid.idLength + 1)
        {
            item = pVarBind->name.ids[pVarBind->name.idLength - 1];
            *pErrorStatus = mapStructEntryToValue(mib2IpMap,
                DEFINE_SIZEOF(mib2IpMap), &ipStats, item, bPduType, pVarBind);
        }
        else
            *pErrorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
        break;
    case SNMP_PDU_GETNEXT:
        if (!SnmpUtilOidCmp(&pVarBind->name, &myOid) ||
            SnmpUtilOidNCmp(&pVarBind->name, &myOid, myOid.idLength) < 0)
            item = 1;
        else if (!SnmpUtilOidNCmp(&pVarBind->name, &myOid, myOid.idLength) &&
            pVarBind->name.idLength == myOid.idLength + 1)
            item = pVarBind->name.ids[pVarBind->name.idLength - 1] + 1;
        else
            *pErrorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
        if (item)
        {
            *pErrorStatus = mapStructEntryToValue(mib2IpMap,
                DEFINE_SIZEOF(mib2IpMap), &ipStats, item, bPduType, pVarBind);
            if (!*pErrorStatus)
            {
                AsnObjectIdentifier oid;

                SnmpUtilOidCpy(&pVarBind->name, &myOid);
                oid.idLength = 1;
                oid.ids = &item;
                SnmpUtilOidAppend(&pVarBind->name, &oid);
            }
        }
        else
            *pErrorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
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
    { DEFINE_OID(mib2IfEntry), NULL, mib2IfEntryQuery },
    { DEFINE_OID(mib2Ip), mib2IpStatsInit, mib2IpStatsQuery },
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
