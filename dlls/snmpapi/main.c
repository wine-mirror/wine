/*
 * Implementation of SNMPAPI.DLL
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

static INT asn_any_copy(AsnAny *dst, AsnAny *src)
{
    memset(dst, 0, sizeof(AsnAny));
    switch (src->asnType)
    {
    case ASN_INTEGER32:  dst->asnValue.number = src->asnValue.number; break;
    case ASN_UNSIGNED32: dst->asnValue.unsigned32 = src->asnValue.unsigned32; break;
    case ASN_COUNTER64:  dst->asnValue.counter64 = src->asnValue.counter64; break;
    case ASN_COUNTER32:  dst->asnValue.counter = src->asnValue.counter; break;
    case ASN_GAUGE32:    dst->asnValue.gauge = src->asnValue.gauge; break;
    case ASN_TIMETICKS:  dst->asnValue.ticks = src->asnValue.ticks; break;

    case ASN_OCTETSTRING:
    case ASN_BITS:
    case ASN_SEQUENCE:
    case ASN_IPADDRESS:
    case ASN_OPAQUE:
    {
        BYTE *stream;
        UINT length = src->asnValue.string.length;

        if (!(stream = HeapAlloc(GetProcessHeap(), 0, length))) return SNMPAPI_ERROR;

        dst->asnValue.string.stream = stream;
        dst->asnValue.string.length = length;
        dst->asnValue.string.dynamic = TRUE;
        break;
    }
    case ASN_OBJECTIDENTIFIER:
    {
        UINT *ids, i, size = src->asnValue.object.idLength * sizeof(UINT);

        if (!(ids = HeapAlloc(GetProcessHeap(), 0, size))) return SNMPAPI_ERROR;

        dst->asnValue.object.ids = ids;
        dst->asnValue.object.idLength = src->asnValue.object.idLength;

        for (i = 0; i < dst->asnValue.object.idLength; i++)
            dst->asnValue.object.ids[i] = src->asnValue.object.ids[i];
        break;
    }
    default:
    {
        WARN("unknown ASN type: %d\n", src->asnType);
        return SNMPAPI_ERROR;
    }
    }
    dst->asnType = src->asnType;
    return SNMPAPI_NOERROR;
}

static void asn_any_free(AsnAny *any)
{
    switch (any->asnType)
    {
    case ASN_OCTETSTRING:
    case ASN_BITS:
    case ASN_SEQUENCE:
    case ASN_IPADDRESS:
    case ASN_OPAQUE:
    {
        if (any->asnValue.string.dynamic)
        {
            HeapFree(GetProcessHeap(), 0, any->asnValue.string.stream);
            any->asnValue.string.stream = NULL;
        }
        break;
    }
    case ASN_OBJECTIDENTIFIER:
    {
        HeapFree(GetProcessHeap(), 0, any->asnValue.object.ids);
        any->asnValue.object.ids = NULL;
        break;
    }
    default: break;
    }
    any->asnType = ASN_NULL;
}

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
 *      SnmpUtilAsnAnyCpy       (SNMPAPI.@)
 */
INT WINAPI SnmpUtilAsnAnyCpy(AsnAny *dst, AsnAny *src)
{
    TRACE("(%p, %p)\n", dst, src);
    return asn_any_copy(dst, src);
}

/***********************************************************************
 *      SnmpUtilAsnAnyFree      (SNMPAPI.@)
 */
void WINAPI SnmpUtilAsnAnyFree(AsnAny *any)
{
    TRACE("(%p)\n", any);
    asn_any_free(any);
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

    if (!dst) return SNMPAPI_ERROR;
    if (!src)
    {
        dst->value.asnType = ASN_NULL;
        return SNMPAPI_NOERROR;
    }

    size = src->name.idLength * sizeof(UINT);
    if (!(dst->name.ids = HeapAlloc(GetProcessHeap(), 0, size))) return SNMPAPI_ERROR;

    for (i = 0; i < src->name.idLength; i++) dst->name.ids[i] = src->name.ids[i];
    dst->name.idLength = src->name.idLength;

    if (!asn_any_copy(&dst->value, &src->value))
    {
        HeapFree(GetProcessHeap(), 0, dst->name.ids);
        return SNMPAPI_ERROR;
    }
    return SNMPAPI_NOERROR;
}

/***********************************************************************
 *      SnmpUtilVarBindFree     (SNMPAPI.@)
 */
void WINAPI SnmpUtilVarBindFree(SnmpVarBind *vb)
{
    TRACE("(%p)\n", vb);

    if (!vb) return;

    asn_any_free(&vb->value);
    HeapFree(GetProcessHeap(), 0, vb->name.ids);
    vb->name.idLength = 0;
    vb->name.ids = NULL;
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
