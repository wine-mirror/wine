/*
 * Stub implementation of SNMPAPI.DLL
 *
 * Copyright 2002 Patrik Stridvall
 * Copyright 2007 Hans Leidekker
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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(snmpapi);

/***********************************************************************
 *		DllMain for SNMPAPI
 */
BOOL WINAPI DllMain(
	HINSTANCE hInstDLL,
	DWORD fdwReason,
	LPVOID lpvReserved)
{
    TRACE("(%p,%d,%p)\n", hInstDLL, fdwReason, lpvReserved);

    switch(fdwReason) {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstDLL);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}

/***********************************************************************
 *      SnmpUtilMemAlloc        (SNMPAPI.@)
 */
LPVOID WINAPI SnmpUtilMemAlloc(UINT nbytes)
{
    TRACE("(%d)\n", nbytes);
    return HeapAlloc(GetProcessHeap(), 0, nbytes);
}

/***********************************************************************
 *      SnmpUtilMemReAlloc      (SNMPAPI.@)
 */
LPVOID WINAPI SnmpUtilMemReAlloc(LPVOID mem, UINT nbytes)
{
    TRACE("(%p, %d)\n", mem, nbytes);
    return HeapReAlloc(GetProcessHeap(), 0, mem, nbytes);
}

/***********************************************************************
 *      SnmpUtilMemFree         (SNMPAPI.@)
 */
void WINAPI SnmpUtilMemFree(LPVOID mem)
{
    TRACE("(%p)\n", mem);
    HeapFree(GetProcessHeap(), 0, mem);
}

/***********************************************************************
 *      SnmpUtilOidCpy          (SNMPAPI.@)
 */
INT WINAPI SnmpUtilOidCpy(AsnObjectIdentifier *dst, AsnObjectIdentifier *src)
{
    unsigned int i, size;

    TRACE("(%p, %p)\n", dst, src);

    size = sizeof(AsnObjectIdentifier);
    if ((dst = HeapAlloc(GetProcessHeap(), 0, size)))
    {
        size = src->idLength * sizeof(UINT);
        if (!(dst->ids = HeapAlloc(GetProcessHeap(), 0, size)))
        {
            HeapFree(GetProcessHeap(), 0, dst);
            return SNMPAPI_ERROR;
        }
        dst->idLength = src->idLength;
        for (i = 0; i < dst->idLength; i++) dst->ids[i] = src->ids[i];
        return SNMPAPI_NOERROR;
    }
    return SNMPAPI_ERROR;
}

/***********************************************************************
 *      SnmpUtilOidFree         (SNMPAPI.@)
 */
void WINAPI SnmpUtilOidFree(AsnObjectIdentifier *oid)
{
    TRACE("(%p)\n", oid);

    HeapFree(GetProcessHeap(), 0, oid->ids);
    HeapFree(GetProcessHeap(), 0, oid);
}

/***********************************************************************
 *      SnmpUtilVarBindCpy      (SNMPAPI.@)
 */
INT WINAPI SnmpUtilVarBindCpy(SnmpVarBind *dst, SnmpVarBind *src)
{
    unsigned int i, size;

    TRACE("(%p, %p)\n", dst, src);

    if (!(dst = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SnmpVarBind))))
        return SNMPAPI_ERROR;

    size = src->name.idLength * sizeof(UINT);
    if (!(dst->name.ids = HeapAlloc(GetProcessHeap(), 0, size))) goto error;
    for (i = 0; i < src->name.idLength; i++) dst->name.ids[i] = src->name.ids[i];
    dst->name.idLength = src->name.idLength;

    dst->value.asnType = src->value.asnType;
    switch (dst->value.asnType)
    {
    case ASN_INTEGER32:  dst->value.asnValue.number = src->value.asnValue.number; break;
    case ASN_UNSIGNED32: dst->value.asnValue.unsigned32 = src->value.asnValue.unsigned32; break;
    case ASN_COUNTER64:  dst->value.asnValue.counter64 = src->value.asnValue.counter64; break;
    case ASN_COUNTER32:  dst->value.asnValue.counter = src->value.asnValue.counter; break;
    case ASN_GAUGE32:    dst->value.asnValue.gauge = src->value.asnValue.gauge; break;
    case ASN_TIMETICKS:  dst->value.asnValue.ticks = src->value.asnValue.ticks; break;

    case ASN_OCTETSTRING:
    case ASN_BITS:
    case ASN_SEQUENCE:
    case ASN_IPADDRESS:
    case ASN_OPAQUE:
    {
        BYTE *stream;
        UINT length = src->value.asnValue.string.length;

        if (!(stream = HeapAlloc(GetProcessHeap(), 0, length)))
            goto error;

        dst->value.asnValue.string.stream = stream;
        dst->value.asnValue.string.length = length;
        dst->value.asnValue.string.dynamic = TRUE;
        break;
    }
    case ASN_OBJECTIDENTIFIER:
    {
        UINT *ids, size = src->name.idLength * sizeof(UINT);

        if (!(ids = HeapAlloc(GetProcessHeap(), 0, size)))
            goto error;

        dst->value.asnValue.object.ids = ids;
        dst->value.asnValue.object.idLength = src->value.asnValue.object.idLength;

        for (i = 0; i < dst->value.asnValue.object.idLength; i++)
            dst->value.asnValue.object.ids[i] = src->value.asnValue.object.ids[i];
        break;
    }
    default:
    {
        WARN("unknown ASN type: %d\n", src->value.asnType);
        break;
    }
    }
    return SNMPAPI_NOERROR;

error:
    HeapFree(GetProcessHeap(), 0, dst->name.ids);
    HeapFree(GetProcessHeap(), 0, dst);
    return SNMPAPI_ERROR;
}

/***********************************************************************
 *      SnmpUtilVarBindFree     (SNMPAPI.@)
 */
void WINAPI SnmpUtilVarBindFree(SnmpVarBind *vb)
{
    TRACE("(%p)\n", vb);

    switch (vb->value.asnType)
    {
    case ASN_OCTETSTRING:
    case ASN_BITS:
    case ASN_SEQUENCE:
    case ASN_IPADDRESS:
    case ASN_OPAQUE:
    {
        if (vb->value.asnValue.string.dynamic)
            HeapFree(GetProcessHeap(), 0, vb->value.asnValue.string.stream);
        break;
    }
    case ASN_OBJECTIDENTIFIER:
    {
        HeapFree(GetProcessHeap(), 0, vb->value.asnValue.object.ids);
        break;
    }
    default: break;
    }

    HeapFree(GetProcessHeap(), 0, vb->name.ids);
    HeapFree(GetProcessHeap(), 0, vb);
}

/***********************************************************************
 *      SnmpUtilVarBindListCpy  (SNMPAPI.@)
 */
INT WINAPI SnmpUtilVarBindListCpy(SnmpVarBindList *dst, SnmpVarBindList *src)
{
    unsigned int i, size;
    SnmpVarBind *src_entry, *dst_entry;

    TRACE("(%p, %p)\n", dst, src);

    size = src->len * sizeof(SnmpVarBind *);
    if (!(dst->list = HeapAlloc(GetProcessHeap(), 0, size)))
    {
        HeapFree(GetProcessHeap(), 0, dst);
        return SNMPAPI_ERROR;
    }
    src_entry = src->list;
    dst_entry = dst->list;
    for (i = 0; i < src->len; i++)
    {
        if (SnmpUtilVarBindCpy(dst_entry, src_entry))
        {
            src_entry++;
            dst_entry++;
        }
        else
        {
            for (--i; i > 0; i--) SnmpUtilVarBindFree(--dst_entry);
            HeapFree(GetProcessHeap(), 0, dst->list);
            return SNMPAPI_ERROR;
        }
    }
    dst->len = src->len;
    return SNMPAPI_NOERROR;
}

/***********************************************************************
 *      SnmpUtilVarBindListFree (SNMPAPI.@)
 */
void WINAPI SnmpUtilVarBindListFree(SnmpVarBindList *vb)
{
    unsigned int i;
    SnmpVarBind *entry;

    TRACE("(%p)\n", vb);

    entry = vb->list;
    for (i = 0; i < vb->len; i++) SnmpUtilVarBindFree(entry++);
    HeapFree(GetProcessHeap(), 0, vb->list);
}
