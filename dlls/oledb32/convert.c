/* OLE DB Conversion library
 *
 * Copyright 2009 Huw Davies
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "ole2.h"
#include "msdadc.h"
#include "oledberr.h"

#include "oledb_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(oledb);

typedef struct
{
    IDataConvert IDataConvert_iface;
    IDCInfo IDCInfo_iface;

    LONG ref;

    UINT version; /* Set by IDCInfo_SetInfo */
} convert;

static inline convert *impl_from_IDataConvert(IDataConvert *iface)
{
    return CONTAINING_RECORD(iface, convert, IDataConvert_iface);
}

static inline convert *impl_from_IDCInfo(IDCInfo *iface)
{
    return CONTAINING_RECORD(iface, convert, IDCInfo_iface);
}

static HRESULT WINAPI convert_QueryInterface(IDataConvert* iface,
                                             REFIID riid,
                                             void **obj)
{
    convert *This = impl_from_IDataConvert(iface);
    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), obj);

    *obj = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
       IsEqualIID(riid, &IID_IDataConvert))
    {
        *obj = iface;
    }
    else if(IsEqualIID(riid, &IID_IDCInfo))
    {
        *obj = &This->IDCInfo_iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IDataConvert_AddRef(iface);
    return S_OK;
}


static ULONG WINAPI convert_AddRef(IDataConvert* iface)
{
    convert *This = impl_from_IDataConvert(iface);
    TRACE("(%p)\n", This);

    return InterlockedIncrement(&This->ref);
}


static ULONG WINAPI convert_Release(IDataConvert* iface)
{
    convert *This = impl_from_IDataConvert(iface);
    LONG ref;

    TRACE("(%p)\n", This);

    ref = InterlockedDecrement(&This->ref);
    if(ref == 0)
        free(This);

    return ref;
}

static int get_length(DBTYPE type)
{
    switch(type)
    {
    case DBTYPE_I1:
    case DBTYPE_UI1:
        return 1;
    case DBTYPE_I2:
    case DBTYPE_UI2:
        return 2;
    case DBTYPE_BOOL:
        return sizeof(VARIANT_BOOL);
    case DBTYPE_I4:
    case DBTYPE_UI4:
    case DBTYPE_R4:
        return 4;
    case DBTYPE_I8:
    case DBTYPE_UI8:
    case DBTYPE_R8:
    case DBTYPE_DATE:
        return 8;
    case DBTYPE_DBDATE:
        return sizeof(DBDATE);
    case DBTYPE_DBTIMESTAMP:
	return sizeof(DBTIMESTAMP);
    case DBTYPE_CY:
        return sizeof(CY);
    case DBTYPE_BSTR:
        return sizeof(BSTR);
    case DBTYPE_FILETIME:
        return sizeof(FILETIME);
    case DBTYPE_GUID:
        return sizeof(GUID);
    case DBTYPE_NUMERIC:
        return sizeof(DB_NUMERIC);
    case DBTYPE_BYTES:
    case DBTYPE_WSTR:
    case DBTYPE_STR:
    case DBTYPE_BYREF | DBTYPE_WSTR:
        return 0;
    case DBTYPE_VARIANT:
        return sizeof(VARIANT);
    default:
        FIXME("Unhandled type %04x\n", type);
        return 0;
    }
}

static HRESULT WINAPI convert_DataConvert(IDataConvert* iface,
                                          DBTYPE src_type, DBTYPE dst_type,
                                          DBLENGTH src_len, DBLENGTH *dst_len,
                                          void *src, void *dst,
                                          DBLENGTH dst_max_len,
                                          DBSTATUS src_status, DBSTATUS *dst_status,
                                          BYTE precision, BYTE scale,
                                          DBDATACONVERT flags)
{
    convert *This = impl_from_IDataConvert(iface);
    DBLENGTH dst_len_loc;
    DBSTATUS dst_status_loc;
    VARIANT tmp;
    HRESULT hr;

    TRACE("(%p)->(%d, %d, %Id, %p, %p, %p, %Id, %ld, %p, %d, %d, %lx)\n", This,
          src_type, dst_type, src_len, dst_len, src, dst, dst_max_len,
          src_status, dst_status, precision, scale, flags);

    if (!dst_len) dst_len = &dst_len_loc;
    if (!dst_status) dst_status = &dst_status_loc;

    *dst_status = DBSTATUS_E_BADACCESSOR;

    if(src_status == DBSTATUS_S_ISNULL)
    {
        *dst_status = DBSTATUS_S_ISNULL;
        *dst_len = 0;
        return S_OK;
    }

    if(src_type == DBTYPE_VARIANT && V_VT((VARIANT*)src) == VT_NULL)
    {
        if(dst_type == DBTYPE_VARIANT)
            *dst_len = sizeof(VARIANT);

        *dst_status = DBSTATUS_S_ISNULL;
        return S_OK;
    }

    if(IDataConvert_CanConvert(iface, src_type, dst_type) != S_OK)
    {
        return DB_E_UNSUPPORTEDCONVERSION;
    }

    if(dst == NULL && get_length(dst_type) != 0)
    {
        *dst_len = get_length(src_type);
        *dst_status = DBSTATUS_S_OK;
        return S_OK;
    }

    if(src_type == DBTYPE_STR)
    {
        BSTR b;
        DWORD len;

        if(flags & DBDATACONVERT_LENGTHFROMNTS)
            len = MultiByteToWideChar(CP_ACP, 0, src, -1, NULL, 0) - 1;
        else
            len = MultiByteToWideChar(CP_ACP, 0, src, src_len, NULL, 0);
        b = SysAllocStringLen(NULL, len);
        if(!b) return E_OUTOFMEMORY;
        if(flags & DBDATACONVERT_LENGTHFROMNTS)
            MultiByteToWideChar(CP_ACP, 0, src, -1, b, len + 1);
        else
            MultiByteToWideChar(CP_ACP, 0, src, src_len, b, len);

        hr = IDataConvert_DataConvert(iface, DBTYPE_BSTR, dst_type, 0, dst_len,
                                      &b, dst, dst_max_len, src_status, dst_status,
                                      precision, scale, flags);

        SysFreeString(b);
        return hr;
    }

    if(src_type == DBTYPE_WSTR)
    {
        BSTR b;

        if(flags & DBDATACONVERT_LENGTHFROMNTS)
            b = SysAllocString(src);
        else
            b = SysAllocStringLen(src, src_len / 2);
        if(!b) return E_OUTOFMEMORY;
        hr = IDataConvert_DataConvert(iface, DBTYPE_BSTR, dst_type, 0, dst_len,
                                      &b, dst, dst_max_len, src_status, dst_status,
                                      precision, scale, flags);
        SysFreeString(b);
        return hr;
    }

    switch(dst_type)
    {
    case DBTYPE_I1:
    {
        signed char *d = dst;
        switch(src_type)
        {
        case DBTYPE_EMPTY:       *d = 0; hr = S_OK;                              break;
        case DBTYPE_I1:          *d = *(signed char*)src; hr = S_OK;             break;
        case DBTYPE_I2:          hr = VarI1FromI2(*(signed short*)src, d);       break;
        case DBTYPE_I4:          hr = VarI1FromI4(*(signed int*)src, d);         break;
        case DBTYPE_R4:          hr = VarI1FromR4(*(FLOAT*)src, d);              break;
        case DBTYPE_R8:          hr = VarI1FromR8(*(double*)src, d);             break;
        case DBTYPE_CY:          hr = VarI1FromCy(*(CY*)src, d);                 break;
        case DBTYPE_DATE:        hr = VarI1FromDate(*(DATE*)src, d);             break;
        case DBTYPE_BSTR:        hr = VarI1FromStr(*(WCHAR**)src, LOCALE_USER_DEFAULT, 0, d); break;
        case DBTYPE_BOOL:        hr = VarI1FromBool(*(VARIANT_BOOL*)src, d);     break;
        case DBTYPE_DECIMAL:     hr = VarI1FromDec(src, d);                      break;
        case DBTYPE_UI1:         hr = VarI1FromUI1(*(BYTE*)src, d);              break;
        case DBTYPE_UI2:         hr = VarI1FromUI2(*(WORD*)src, d);              break;
        case DBTYPE_UI4:         hr = VarI1FromUI4(*(DWORD*)src, d);             break;
        case DBTYPE_I8:          hr = VarI1FromI8(*(LONGLONG*)src, d);           break;
        case DBTYPE_UI8:         hr = VarI1FromUI8(*(ULONGLONG*)src, d);         break;
        case DBTYPE_VARIANT:
            VariantInit(&tmp);
            if ((hr = VariantChangeType(&tmp, src, 0, VT_I1)) == S_OK)
                *d = V_I1(&tmp);
            break;
        default: FIXME("Unimplemented conversion %04x -> I1\n", src_type); return E_NOTIMPL;
        }
        break;
    }

    case DBTYPE_I2:
    {
        signed short *d = dst;
        switch(src_type)
        {
        case DBTYPE_EMPTY:       *d = 0; hr = S_OK;                              break;
        case DBTYPE_I2:          *d = *(signed short*)src; hr = S_OK;            break;
        case DBTYPE_I4:          hr = VarI2FromI4(*(signed int*)src, d);         break;
        case DBTYPE_R4:          hr = VarI2FromR4(*(FLOAT*)src, d);              break;
        case DBTYPE_R8:          hr = VarI2FromR8(*(double*)src, d);             break;
        case DBTYPE_CY:          hr = VarI2FromCy(*(CY*)src, d);                 break;
        case DBTYPE_DATE:        hr = VarI2FromDate(*(DATE*)src, d);             break;
        case DBTYPE_BSTR:        hr = VarI2FromStr(*(WCHAR**)src, LOCALE_USER_DEFAULT, 0, d); break;
        case DBTYPE_BOOL:        hr = VarI2FromBool(*(VARIANT_BOOL*)src, d);     break;
        case DBTYPE_DECIMAL:     hr = VarI2FromDec(src, d);                      break;
        case DBTYPE_I1:          hr = VarI2FromI1(*(signed char*)src, d);        break;
        case DBTYPE_UI1:         hr = VarI2FromUI1(*(BYTE*)src, d);              break;
        case DBTYPE_UI2:         hr = VarI2FromUI2(*(WORD*)src, d);              break;
        case DBTYPE_UI4:         hr = VarI2FromUI4(*(DWORD*)src, d);             break;
        case DBTYPE_I8:          hr = VarI2FromI8(*(LONGLONG*)src, d);           break;
        case DBTYPE_UI8:         hr = VarI2FromUI8(*(ULONGLONG*)src, d);         break;
        case DBTYPE_VARIANT:
            VariantInit(&tmp);
            if ((hr = VariantChangeType(&tmp, src, 0, VT_I2)) == S_OK)
                *d = V_I2(&tmp);
            break;
        default: FIXME("Unimplemented conversion %04x -> I2\n", src_type); return E_NOTIMPL;
        }
        break;
    }

    case DBTYPE_I4:
    {
        LONG *d = dst;
        switch(src_type)
        {
        case DBTYPE_EMPTY:       *d = 0; hr = S_OK;                              break;
        case DBTYPE_I2:          hr = VarI4FromI2(*(signed short*)src, d);       break;
        case DBTYPE_I4:          *d = *(signed int*)src; hr = S_OK;              break;
        case DBTYPE_R4:          hr = VarI4FromR4(*(FLOAT*)src, d);              break;
        case DBTYPE_R8:          hr = VarI4FromR8(*(double*)src, d);             break;
        case DBTYPE_CY:          hr = VarI4FromCy(*(CY*)src, d);                 break;
        case DBTYPE_DATE:        hr = VarI4FromDate(*(DATE*)src, d);             break;
        case DBTYPE_BSTR:        hr = VarI4FromStr(*(WCHAR**)src, LOCALE_USER_DEFAULT, 0, d); break;
        case DBTYPE_BOOL:        hr = VarI4FromBool(*(VARIANT_BOOL*)src, d);     break;
        case DBTYPE_DECIMAL:     hr = VarI4FromDec(src, d);                      break;
        case DBTYPE_I1:          hr = VarI4FromI1(*(signed char*)src, d);        break;
        case DBTYPE_UI1:         hr = VarI4FromUI1(*(BYTE*)src, d);              break;
        case DBTYPE_UI2:         hr = VarI4FromUI2(*(WORD*)src, d);              break;
        case DBTYPE_UI4:         hr = VarI4FromUI4(*(DWORD*)src, d);             break;
        case DBTYPE_I8:          hr = VarI4FromI8(*(LONGLONG*)src, d);           break;
        case DBTYPE_UI8:         hr = VarI4FromUI8(*(ULONGLONG*)src, d);         break;
        case DBTYPE_VARIANT:
            VariantInit(&tmp);
            if ((hr = VariantChangeType(&tmp, src, 0, VT_I4)) == S_OK)
                *d = V_I4(&tmp);
            break;
        default: FIXME("Unimplemented conversion %04x -> I4\n", src_type); return E_NOTIMPL;
        }
        break;
    }

    case DBTYPE_I8:
    {
        LONGLONG *d = dst;
        switch(src_type)
        {
        case DBTYPE_BSTR:        hr = VarI8FromStr(*(WCHAR**)src, LOCALE_USER_DEFAULT, 0, d); break;
        case DBTYPE_I8:          *d = *(LONGLONG*)src; hr = S_OK;              break;
        default: FIXME("Unimplemented conversion %04x -> I8\n", src_type); return E_NOTIMPL;
        }
        break;
    }

    case DBTYPE_R4:
    {
        FLOAT *d = dst;
        switch(src_type)
        {
        case DBTYPE_EMPTY:       *d = 0; hr = S_OK;                              break;
        case DBTYPE_I2:          hr = VarR4FromI2(*(signed short*)src, d);       break;
        case DBTYPE_I4:          hr = VarR4FromI4(*(signed int*)src, d);         break;
        case DBTYPE_R4:          *d = *(FLOAT*)src; hr = S_OK;                   break;
        case DBTYPE_R8:          hr = VarR4FromR8(*(double*)src, d);             break;
        case DBTYPE_CY:          hr = VarR4FromCy(*(CY*)src, d);                 break;
        case DBTYPE_DATE:        hr = VarR4FromDate(*(DATE*)src, d);             break;
        case DBTYPE_BSTR:        hr = VarR4FromStr(*(WCHAR**)src, LOCALE_USER_DEFAULT, 0, d); break;
        case DBTYPE_BOOL:        hr = VarR4FromBool(*(VARIANT_BOOL*)src, d);     break;
        case DBTYPE_DECIMAL:     hr = VarR4FromDec(src, d);                      break;
        case DBTYPE_I1:          hr = VarR4FromI1(*(signed char*)src, d);        break;
        case DBTYPE_UI1:         hr = VarR4FromUI1(*(BYTE*)src, d);              break;
        case DBTYPE_UI2:         hr = VarR4FromUI2(*(WORD*)src, d);              break;
        case DBTYPE_UI4:         hr = VarR4FromUI4(*(DWORD*)src, d);             break;
        case DBTYPE_I8:          hr = VarR4FromI8(*(LONGLONG*)src, d);           break;
        case DBTYPE_UI8:         hr = VarR4FromUI8(*(ULONGLONG*)src, d);         break;
        case DBTYPE_VARIANT:
            VariantInit(&tmp);
            if ((hr = VariantChangeType(&tmp, src, 0, VT_R4)) == S_OK)
                *d = V_R4(&tmp);
            break;
        default: FIXME("Unimplemented conversion %04x -> R4\n", src_type); return E_NOTIMPL;
        }
        break;
    }
    case DBTYPE_R8:
    {
        DOUBLE *d=dst;
        switch (src_type)
        {
        case DBTYPE_EMPTY:      *d = 0; hr = S_OK;                               break;
        case DBTYPE_I1:          hr = VarR8FromI1(*(signed char*)src, d);        break;
        case DBTYPE_I2:          hr = VarR8FromI2(*(signed short*)src, d);       break;
        case DBTYPE_I4:          hr = VarR8FromI4(*(signed int*)src, d);         break;
        case DBTYPE_I8:          hr = VarR8FromI8(*(LONGLONG*)src, d);           break;
        case DBTYPE_UI1:         hr = VarR8FromUI1(*(BYTE*)src, d);              break;
        case DBTYPE_UI2:         hr = VarR8FromUI2(*(WORD*)src, d);              break;
        case DBTYPE_UI4:         hr = VarR8FromUI4(*(DWORD*)src, d);             break;
        case DBTYPE_UI8:         hr = VarR8FromUI8(*(ULONGLONG*)src, d);         break;
        case DBTYPE_R4:          hr = VarR8FromR4(*(FLOAT*)src, d);              break;
        case DBTYPE_R8:          *d = *(DOUBLE*)src; hr = S_OK;                  break;
        case DBTYPE_CY:          hr = VarR8FromCy(*(CY*)src, d);                 break;
        case DBTYPE_DATE:        hr = VarR8FromDate(*(DATE*)src, d);             break;
        case DBTYPE_BSTR:        hr = VarR8FromStr(*(WCHAR**)src, LOCALE_USER_DEFAULT, 0, d); break;
        case DBTYPE_BOOL:        hr = VarR8FromBool(*(VARIANT_BOOL*)src, d);     break;
        case DBTYPE_DECIMAL:     hr = VarR8FromDec(src, d);                      break;
        case DBTYPE_VARIANT:
            VariantInit(&tmp);
            if ((hr = VariantChangeType(&tmp, src, 0, VT_R8)) == S_OK)
                *d = V_R8(&tmp);
            break;
        default: FIXME("Unimplemented conversion %04x -> R8\n", src_type); return E_NOTIMPL;
        }
        break;
    }
    case DBTYPE_BOOL:
    {
        VARIANT_BOOL *d=dst;
        switch (src_type)
        {
        case DBTYPE_EMPTY:      *d = 0; hr = S_OK;                               break;
        case DBTYPE_I1:          hr = VarBoolFromI1(*(signed char*)src, d);      break;
        case DBTYPE_I2:          hr = VarBoolFromI2(*(signed short*)src, d);     break;
        case DBTYPE_I4:          hr = VarBoolFromI4(*(signed int*)src, d);       break;
        case DBTYPE_I8:          hr = VarBoolFromI8(*(LONGLONG*)src, d);         break;
        case DBTYPE_UI1:         hr = VarBoolFromUI1(*(BYTE*)src, d);            break;
        case DBTYPE_UI2:         hr = VarBoolFromUI2(*(WORD*)src, d);            break;
        case DBTYPE_UI4:         hr = VarBoolFromUI4(*(DWORD*)src, d);           break;
        case DBTYPE_UI8:         hr = VarBoolFromUI8(*(ULONGLONG*)src, d);       break;
        case DBTYPE_R4:          hr = VarBoolFromR4(*(FLOAT*)src, d);            break;
        case DBTYPE_R8:          hr = VarBoolFromR8(*(DOUBLE*)src, d);           break;
        case DBTYPE_CY:          hr = VarBoolFromCy(*(CY*)src, d);               break;
        case DBTYPE_DATE:        hr = VarBoolFromDate(*(DATE*)src, d);           break;
        case DBTYPE_BSTR:        hr = VarBoolFromStr(*(WCHAR**)src, LOCALE_USER_DEFAULT, 0, d); break;
        case DBTYPE_BOOL:        *d = *(VARIANT_BOOL*)src; hr = S_OK;            break;
        case DBTYPE_DECIMAL:     hr = VarBoolFromDec(src, d);                    break;
        default: FIXME("Unimplemented conversion %04x -> BOOL\n", src_type); return E_NOTIMPL;
        }
        break;
    }
    case DBTYPE_DATE:
    {
        DATE *d=dst;
        switch (src_type)
        {
        case DBTYPE_EMPTY:      *d = 0; hr = S_OK;                           	 break;
        case DBTYPE_I1:          hr = VarDateFromI1(*(signed char*)src, d);      break;
        case DBTYPE_I2:          hr = VarDateFromI2(*(signed short*)src, d);     break;
        case DBTYPE_I4:          hr = VarDateFromI4(*(signed int*)src, d);       break;
        case DBTYPE_I8:          hr = VarDateFromI8(*(LONGLONG*)src, d);         break;
        case DBTYPE_UI1:         hr = VarDateFromUI1(*(BYTE*)src, d);            break;
        case DBTYPE_UI2:         hr = VarDateFromUI2(*(WORD*)src, d);            break;
        case DBTYPE_UI4:         hr = VarDateFromUI4(*(DWORD*)src, d);           break;
        case DBTYPE_UI8:         hr = VarDateFromUI8(*(ULONGLONG*)src, d);       break;
        case DBTYPE_R4:          hr = VarDateFromR4(*(FLOAT*)src, d);            break;
        case DBTYPE_R8:          hr = VarDateFromR8(*(DOUBLE*)src, d);           break;
        case DBTYPE_CY:          hr = VarDateFromCy(*(CY*)src, d);               break;
        case DBTYPE_DATE:       *d = *(DATE*)src;      hr = S_OK;                break;
        case DBTYPE_BSTR:        hr = VarDateFromStr(*(WCHAR**)src, LOCALE_USER_DEFAULT, 0, d); break;
        case DBTYPE_BOOL:        hr = VarDateFromBool(*(VARIANT_BOOL*)src, d);   break;
        case DBTYPE_DECIMAL:     hr = VarDateFromDec(src, d);                    break;
        case DBTYPE_DBTIMESTAMP:
        {
            SYSTEMTIME st;
            DBTIMESTAMP *ts = src;

            st.wYear = ts->year;
            st.wMonth = ts->month;
            st.wDay = ts->day;
            st.wHour = ts->hour;
            st.wMinute = ts->minute;
            st.wSecond = ts->second;
            st.wMilliseconds = ts->fraction/1000000;
            hr = (SystemTimeToVariantTime(&st, d) ? S_OK : E_FAIL);
            break;
        }
        case DBTYPE_VARIANT:
        {
            VARIANT tmp;
            VariantInit(&tmp);
            if ((hr = VariantChangeType(&tmp, src, 0, VT_DATE)) == S_OK)
                *d = V_DATE(&tmp);
            break;
        }
        default: FIXME("Unimplemented conversion %04x -> DATE\n", src_type); return E_NOTIMPL;
        }
        break;
    }
    case DBTYPE_DBDATE:
    {
        DBDATE *d=dst;
        switch (src_type)
        {
        case DBTYPE_DBDATE: memcpy(d, src, sizeof(DBDATE));  hr = S_OK; break;
        case DBTYPE_BSTR:
        {
            VARIANT var;
            BSTR s = *(WCHAR**)src;

            VariantInit(&var);
            V_VT(&var) = VT_BSTR;
            V_BSTR(&var) = SysAllocString(s);

            if ((hr = VariantChangeType(&var, &var, 0, VT_DATE)) == S_OK)
            {
                SYSTEMTIME st;
                hr = (VariantTimeToSystemTime( V_DATE(&var), &st) ? S_OK : E_FAIL);
                d->year = st.wYear;
                d->month = st.wMonth;
                d->day = st.wDay;
            }

            VariantClear(&var);
        }
        break;
        case DBTYPE_VARIANT:
        {
            VariantInit(&tmp);

            if ((hr = VariantChangeType(&tmp, src, 0, VT_DATE)) == S_OK)
            {
                SYSTEMTIME st;
                hr = (VariantTimeToSystemTime( V_DATE(&tmp), &st) ? S_OK : E_FAIL);
                d->year = st.wYear;
                d->month = st.wMonth;
                d->day = st.wDay;
            }
            else
            {
                FIXME("Unimplemented variant type %d -> DBDATE\n", V_VT((VARIANT*)src));
                return E_NOTIMPL;
            }
            break;
        }
        default: FIXME("Unimplemented conversion %04x -> DBDATE\n", src_type); return  E_NOTIMPL;
        }
        break;
    }
    case DBTYPE_DBTIMESTAMP:
    {
        DBTIMESTAMP *d=dst;
        switch (src_type)
        {
        case DBTYPE_EMPTY:       memset(d, 0, sizeof(DBTIMESTAMP));    hr = S_OK; break;
        case DBTYPE_DBTIMESTAMP: memcpy(d, src, sizeof(DBTIMESTAMP));  hr = S_OK; break;
        case DBTYPE_BSTR:
        {
            VARIANT var;
            BSTR s = *(WCHAR**)src;

            VariantInit(&var);
            V_VT(&var) = VT_BSTR;
            V_BSTR(&var) = SysAllocString(s);

            if ((hr = VariantChangeType(&var, &var, 0, VT_DATE)) == S_OK)
            {
                SYSTEMTIME st;

                hr = (VariantTimeToSystemTime( V_DATE(&var), &st) ? S_OK : E_FAIL);
                d->year = st.wYear;
                d->month = st.wMonth;
                d->day = st.wDay;
                d->hour = st.wHour;
                d->minute = st.wMinute;
                d->second = st.wSecond;
                d->fraction = st.wMilliseconds * 1000000;
            }
            else
            {
                if(swscanf( s, L"%d-%d-%d %d:%d:%d.%d", &d->year, &d->month, &d->day, &d->hour, &d->minute,
                             &d->second, &d->fraction) != 7)
                {
                    hr = DISP_E_TYPEMISMATCH;
                    *dst_status = DBSTATUS_E_CANTCONVERTVALUE;
                    *dst_len = get_length(dst_type);
                }
                else
                    hr = S_OK;
            }

            VariantClear(&var);
        }
        break;
        case DBTYPE_DATE:
        {
            SYSTEMTIME st;
            hr = (VariantTimeToSystemTime(*(double*)src, &st) ? S_OK : E_FAIL);
            d->year = st.wYear;
            d->month = st.wMonth;
            d->day = st.wDay;
            d->hour = st.wHour;
            d->minute = st.wMinute;
            d->second = st.wSecond;
            d->fraction = st.wMilliseconds * 1000000;
            break;
        }
        case DBTYPE_VARIANT:
            if( V_VT((VARIANT*)src) == VT_DATE)
            {
                SYSTEMTIME st;
                hr = (VariantTimeToSystemTime( V_DATE((VARIANT*)src), &st) ? S_OK : E_FAIL);
                d->year = st.wYear;
                d->month = st.wMonth;
                d->day = st.wDay;
                d->hour = st.wHour;
                d->minute = st.wMinute;
                d->second = st.wSecond;
                d->fraction = st.wMilliseconds * 1000000;
            }
            else
            {
                FIXME("Unimplemented variant type %d -> DBTIMESTAMP\n", V_VT((VARIANT*)src));
                return E_NOTIMPL;
            }
            break;
        default: FIXME("Unimplemented conversion %04x -> DBTIMESTAMP\n", src_type); return E_NOTIMPL;
        }
        break;
    }

    case DBTYPE_CY:
    {
        CY *d = dst;
        switch(src_type)
        {
        case DBTYPE_EMPTY:       d->int64 = 0; hr = S_OK;                              break;
        case DBTYPE_I2:          hr = VarCyFromI2(*(signed short*)src, d);       break;
        case DBTYPE_I4:          hr = VarCyFromI4(*(signed int*)src, d);         break;
        case DBTYPE_R4:          hr = VarCyFromR4(*(FLOAT*)src, d);              break;
        case DBTYPE_R8:          hr = VarCyFromR8(*(double*)src, d);             break;
        case DBTYPE_CY:          *d = *(CY*)src; hr = S_OK;                      break;
        case DBTYPE_DATE:        hr = VarCyFromDate(*(DATE*)src, d);             break;
        case DBTYPE_BSTR:        hr = VarCyFromStr(*(WCHAR**)src, LOCALE_USER_DEFAULT, 0, d); break;
        case DBTYPE_BOOL:        hr = VarCyFromBool(*(VARIANT_BOOL*)src, d);     break;
        case DBTYPE_DECIMAL:     hr = VarCyFromDec(src, d);                      break;
        case DBTYPE_I1:          hr = VarCyFromI1(*(signed char*)src, d);        break;
        case DBTYPE_UI1:         hr = VarCyFromUI1(*(BYTE*)src, d);              break;
        case DBTYPE_UI2:         hr = VarCyFromUI2(*(WORD*)src, d);              break;
        case DBTYPE_UI4:         hr = VarCyFromUI4(*(DWORD*)src, d);             break;
        case DBTYPE_I8:          hr = VarCyFromI8(*(LONGLONG*)src, d);           break;
        case DBTYPE_UI8:         hr = VarCyFromUI8(*(ULONGLONG*)src, d);         break;
        default: FIXME("Unimplemented conversion %04x -> CY\n", src_type); return E_NOTIMPL;
        }
        break;
    }

    case DBTYPE_BSTR:
    {
        BSTR *d = dst;
        switch(src_type)
        {
        case DBTYPE_EMPTY:       *d = SysAllocStringLen(NULL, 0); hr = *d ? S_OK : E_OUTOFMEMORY;      break;
        case DBTYPE_I2:          hr = VarBstrFromI2(*(signed short*)src, LOCALE_USER_DEFAULT, 0, d);   break;
        case DBTYPE_I4:          hr = VarBstrFromI4(*(signed int*)src, LOCALE_USER_DEFAULT, 0, d);     break;
        case DBTYPE_R4:          hr = VarBstrFromR4(*(FLOAT*)src, LOCALE_USER_DEFAULT, 0, d);          break;
        case DBTYPE_R8:          hr = VarBstrFromR8(*(double*)src, LOCALE_USER_DEFAULT, 0, d);         break;
        case DBTYPE_CY:          hr = VarBstrFromCy(*(CY*)src, LOCALE_USER_DEFAULT, 0, d);             break;
        case DBTYPE_DATE:        hr = VarBstrFromDate(*(DATE*)src, LOCALE_USER_DEFAULT, 0, d);         break;
        case DBTYPE_BSTR:        *d = SysAllocStringLen(*(BSTR*)src, SysStringLen(*(BSTR*)src)); hr = *d ? S_OK : E_OUTOFMEMORY;     break;
        case DBTYPE_BOOL:        hr = VarBstrFromBool(*(VARIANT_BOOL*)src, LOCALE_USER_DEFAULT, 0, d); break;
        case DBTYPE_DECIMAL:     hr = VarBstrFromDec(src, LOCALE_USER_DEFAULT, 0, d);                  break;
        case DBTYPE_I1:          hr = VarBstrFromI1(*(signed char*)src, LOCALE_USER_DEFAULT, 0, d);    break;
        case DBTYPE_UI1:         hr = VarBstrFromUI1(*(BYTE*)src, LOCALE_USER_DEFAULT, 0, d);          break;
        case DBTYPE_UI2:         hr = VarBstrFromUI2(*(WORD*)src, LOCALE_USER_DEFAULT, 0, d);          break;
        case DBTYPE_UI4:         hr = VarBstrFromUI4(*(DWORD*)src, LOCALE_USER_DEFAULT, 0, d);         break;
        case DBTYPE_I8:          hr = VarBstrFromI8(*(LONGLONG*)src, LOCALE_USER_DEFAULT, 0, d);       break;
        case DBTYPE_UI8:         hr = VarBstrFromUI8(*(ULONGLONG*)src, LOCALE_USER_DEFAULT, 0, d);     break;
        case DBTYPE_GUID:
        {
            WCHAR szBuff[39];
            const GUID *id = src;
            wsprintfW(szBuff, L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                id->Data1, id->Data2, id->Data3,
                id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
                id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7] );
            *d = SysAllocString(szBuff);
            hr = *d ? S_OK : E_OUTOFMEMORY;
        }
        break;
        case DBTYPE_BYTES:
        {
            *d = SysAllocStringLen(NULL, 2 * src_len);
            if (*d == NULL)
                hr = E_OUTOFMEMORY;
            else
            {
                static const char hexchars[] = "0123456789ABCDEF";
                WCHAR *s = *d;
                unsigned char *p = src;
                while (src_len > 0)
                {
                    *s++ = hexchars[(*p >> 4) & 0x0F];
                    *s++ = hexchars[(*p)      & 0x0F];
                    src_len--; p++;
                }
                hr = S_OK;
            }
        }
        break;
        case DBTYPE_DBTIMESTAMP:
        {
            WCHAR szBuff[64];
            DBTIMESTAMP *ts = src;

            if(ts->fraction == 0)
                wsprintfW(szBuff, L"%04d-%02d-%02d %02d:%02d:%02d", ts->year, ts->month, ts->day, ts->hour,
                        ts->minute, ts->second);
            else
                wsprintfW(szBuff, L"%04d-%02d-%02d %02d:%02d:%02d.%09d", ts->year, ts->month, ts->day, ts->hour,
                        ts->minute, ts->second, ts->fraction );
            *d = SysAllocString(szBuff);
            hr = *d ? S_OK : E_OUTOFMEMORY;
            break;
        }
        case DBTYPE_VARIANT:
            VariantInit(&tmp);
            if ((hr = VariantChangeType(&tmp, src, 0, VT_BSTR)) == S_OK)
                *d = V_BSTR(&tmp);
            break;
        default: FIXME("Unimplemented conversion %04x -> BSTR\n", src_type); return E_NOTIMPL;
        }
        break;
    }

    case DBTYPE_UI1:
    {
        BYTE *d = dst;
        switch(src_type)
        {
        case DBTYPE_EMPTY:       *d = 0; hr = S_OK;                              break;
        case DBTYPE_I2:          hr = VarUI1FromI2(*(signed short*)src, d);      break;
        case DBTYPE_I4:          hr = VarUI1FromI4(*(signed int*)src, d);        break;
        case DBTYPE_R4:          hr = VarUI1FromR4(*(FLOAT*)src, d);             break;
        case DBTYPE_R8:          hr = VarUI1FromR8(*(double*)src, d);            break;
        case DBTYPE_CY:          hr = VarUI1FromCy(*(CY*)src, d);                break;
        case DBTYPE_DATE:        hr = VarUI1FromDate(*(DATE*)src, d);            break;
        case DBTYPE_BSTR:        hr = VarUI1FromStr(*(WCHAR**)src, LOCALE_USER_DEFAULT, 0, d); break;
        case DBTYPE_BOOL:        hr = VarUI1FromBool(*(VARIANT_BOOL*)src, d);    break;
        case DBTYPE_DECIMAL:     hr = VarUI1FromDec(src, d);                     break;
        case DBTYPE_I1:          hr = VarUI1FromI1(*(signed char*)src, d);       break;
        case DBTYPE_UI1:         *d = *(BYTE*)src; hr = S_OK;                    break;
        case DBTYPE_UI2:         hr = VarUI1FromUI2(*(WORD*)src, d);             break;
        case DBTYPE_UI4:         hr = VarUI1FromUI4(*(DWORD*)src, d);            break;
        case DBTYPE_I8:          hr = VarUI1FromI8(*(LONGLONG*)src, d);          break;
        case DBTYPE_UI8:         hr = VarUI1FromUI8(*(ULONGLONG*)src, d);        break;
        default: FIXME("Unimplemented conversion %04x -> UI1\n", src_type); return E_NOTIMPL;
        }
        break;
    }
    case DBTYPE_UI2:
    {
        WORD *d = dst;
        switch(src_type)
        {
        case DBTYPE_EMPTY:       *d = 0; hr = S_OK;                              break;
        case DBTYPE_I2:          hr = VarUI2FromI2(*(signed short*)src, d);      break;
        case DBTYPE_I4:          hr = VarUI2FromI4(*(signed int*)src, d);        break;
        case DBTYPE_R4:          hr = VarUI2FromR4(*(FLOAT*)src, d);             break;
        case DBTYPE_R8:          hr = VarUI2FromR8(*(double*)src, d);            break;
        case DBTYPE_CY:          hr = VarUI2FromCy(*(CY*)src, d);                break;
        case DBTYPE_DATE:        hr = VarUI2FromDate(*(DATE*)src, d);            break;
        case DBTYPE_BSTR:        hr = VarUI2FromStr(*(WCHAR**)src, LOCALE_USER_DEFAULT, 0, d); break;
        case DBTYPE_BOOL:        hr = VarUI2FromBool(*(VARIANT_BOOL*)src, d);    break;
        case DBTYPE_DECIMAL:     hr = VarUI2FromDec(src, d);                     break;
        case DBTYPE_I1:          hr = VarUI2FromI1(*(signed char*)src, d);       break;
        case DBTYPE_UI1:         hr = VarUI2FromUI1(*(BYTE*)src, d);             break;
        case DBTYPE_UI2:         *d = *(WORD*)src; hr = S_OK;                     break;
        case DBTYPE_UI4:         hr = VarUI2FromUI4(*(DWORD*)src, d);            break;
        case DBTYPE_I8:          hr = VarUI2FromI8(*(LONGLONG*)src, d);          break;
        case DBTYPE_UI8:         hr = VarUI2FromUI8(*(ULONGLONG*)src, d);        break;
        default: FIXME("Unimplemented conversion %04x -> UI2\n", src_type); return E_NOTIMPL;
        }
        break;
    }

    case DBTYPE_UI4:
    {
        DWORD *d = dst;
        switch(src_type)
        {
        case DBTYPE_EMPTY:       *d = 0; hr = S_OK;                              break;
        case DBTYPE_I2:          hr = VarUI4FromI2(*(signed short*)src, d);      break;
        case DBTYPE_I4:          hr = VarUI4FromI4(*(signed int*)src, d);        break;
        case DBTYPE_R4:          hr = VarUI4FromR4(*(FLOAT*)src, d);             break;
        case DBTYPE_R8:          hr = VarUI4FromR8(*(double*)src, d);            break;
        case DBTYPE_CY:          hr = VarUI4FromCy(*(CY*)src, d);                break;
        case DBTYPE_DATE:        hr = VarUI4FromDate(*(DATE*)src, d);            break;
        case DBTYPE_BSTR:        hr = VarUI4FromStr(*(WCHAR**)src, LOCALE_USER_DEFAULT, 0, d); break;
        case DBTYPE_BOOL:        hr = VarUI4FromBool(*(VARIANT_BOOL*)src, d);    break;
        case DBTYPE_DECIMAL:     hr = VarUI4FromDec(src, d);                     break;
        case DBTYPE_I1:          hr = VarUI4FromI1(*(signed char*)src, d);       break;
        case DBTYPE_UI1:         hr = VarUI4FromUI1(*(BYTE*)src, d);             break;
        case DBTYPE_UI2:         hr = VarUI4FromUI2(*(WORD*)src, d);             break;
        case DBTYPE_UI4:         *d = *(DWORD*)src; hr = S_OK;                   break;
        case DBTYPE_I8:          hr = VarUI4FromI8(*(LONGLONG*)src, d);          break;
        case DBTYPE_UI8:         hr = VarUI4FromUI8(*(ULONGLONG*)src, d);        break;
        case DBTYPE_VARIANT:
            VariantInit(&tmp);
            if ((hr = VariantChangeType(&tmp, src, 0, VT_UI4)) == S_OK)
                *d = V_UI4(&tmp);
            break;
        default: FIXME("Unimplemented conversion %04x -> UI4\n", src_type); return E_NOTIMPL;
        }
        break;
    }

    case DBTYPE_UI8:
    {
        ULONGLONG *d = dst;
        switch(src_type)
        {
        case DBTYPE_EMPTY:       *d = 0; hr = S_OK;                              break;
        case DBTYPE_I2:          hr = VarUI8FromI2(*(signed short*)src, d);      break;
        case DBTYPE_I4:          {LONGLONG s = *(signed int*)src; hr = VarUI8FromI8(s, d);        break;}
        case DBTYPE_R4:          hr = VarUI8FromR4(*(FLOAT*)src, d);             break;
        case DBTYPE_R8:          hr = VarUI8FromR8(*(double*)src, d);            break;
        case DBTYPE_CY:          hr = VarUI8FromCy(*(CY*)src, d);                break;
        case DBTYPE_DATE:        hr = VarUI8FromDate(*(DATE*)src, d);            break;
        case DBTYPE_BSTR:        hr = VarUI8FromStr(*(WCHAR**)src, LOCALE_USER_DEFAULT, 0, d); break;
        case DBTYPE_BOOL:        hr = VarUI8FromBool(*(VARIANT_BOOL*)src, d);    break;
        case DBTYPE_DECIMAL:     hr = VarUI8FromDec(src, d);                     break;
        case DBTYPE_I1:          hr = VarUI8FromI1(*(signed char*)src, d);       break;
        case DBTYPE_UI1:         hr = VarUI8FromUI1(*(BYTE*)src, d);             break;
        case DBTYPE_UI2:         hr = VarUI8FromUI2(*(WORD*)src, d);             break;
        case DBTYPE_UI4:         hr = VarUI8FromUI4(*(DWORD*)src, d);            break;
        case DBTYPE_I8:          hr = VarUI8FromI8(*(LONGLONG*)src, d);          break;
        case DBTYPE_UI8:         *d = *(ULONGLONG*)src; hr = S_OK;               break;
        case DBTYPE_VARIANT:
            VariantInit(&tmp);
            if ((hr = VariantChangeType(&tmp, src, 0, VT_UI8)) == S_OK)
                *d = V_UI8(&tmp);
            break;
        default: FIXME("Unimplemented conversion %04x -> UI8\n", src_type); return E_NOTIMPL;
        }
        break;
    }

    case DBTYPE_FILETIME:
    {
        FILETIME *d = dst;
        switch(src_type)
        {
        case DBTYPE_EMPTY:       d->dwLowDateTime = d->dwHighDateTime = 0; hr = S_OK;    break;
        case DBTYPE_FILETIME:    *d = *(FILETIME*)src; hr = S_OK;                        break;
        default: FIXME("Unimplemented conversion %04x -> FILETIME\n", src_type); return E_NOTIMPL;
        }
        break;
    }

    case DBTYPE_GUID:
    {
        GUID *d = dst;
        switch(src_type)
        {
        case DBTYPE_EMPTY:       *d = GUID_NULL; hr = S_OK; break;
        case DBTYPE_GUID:        *d = *(GUID*)src; hr = S_OK; break;
        case DBTYPE_VARIANT:
            if (V_VT((VARIANT *)src) == VT_BSTR)
            {
                hr = CLSIDFromString(V_BSTR((VARIANT *)src), d);
                if (FAILED(hr))
                {
                    *dst_len = sizeof(GUID);
                    *dst_status = DBSTATUS_E_CANTCONVERTVALUE;
                }
                break;
            }
        default: FIXME("Unimplemented conversion %04x -> GUID\n", src_type); return E_NOTIMPL;
        }
        break;
    }

    case DBTYPE_WSTR:
    {
        BSTR b;
        DBLENGTH bstr_len;
        INT bytes_to_copy;
        hr = IDataConvert_DataConvert(iface, src_type, DBTYPE_BSTR, src_len, &bstr_len,
                                      src, &b, sizeof(BSTR), src_status, dst_status,
                                      precision, scale, flags);
        if(hr != S_OK || *dst_status == DBSTATUS_S_ISNULL)
            return hr;
        bstr_len = SysStringLen(b);
        *dst_len = bstr_len * sizeof(WCHAR); /* Doesn't include size for '\0' */
        *dst_status = DBSTATUS_S_OK;
        bytes_to_copy = min(*dst_len + sizeof(WCHAR), dst_max_len);
        if(dst)
        {
            if(bytes_to_copy >= sizeof(WCHAR))
            {
                memcpy(dst, b, bytes_to_copy - sizeof(WCHAR));
                *((WCHAR*)dst + bytes_to_copy / sizeof(WCHAR) - 1) = 0;
                if(bytes_to_copy < *dst_len + sizeof(WCHAR))
                    *dst_status = DBSTATUS_S_TRUNCATED;
            }
            else
            {
                *dst_status = DBSTATUS_E_DATAOVERFLOW;
                hr = DB_E_ERRORSOCCURRED;
            }
        }
        SysFreeString(b);
        return hr;
    }

    case DBTYPE_STR:
    {
        BSTR b;
        DBLENGTH length;
        INT bytes_to_copy;
        hr = IDataConvert_DataConvert(iface, src_type, DBTYPE_BSTR, src_len, &length,
                                      src, &b, sizeof(BSTR), src_status, dst_status,
                                      precision, scale, flags);
        if(hr != S_OK) return hr;
        length = WideCharToMultiByte(CP_ACP, 0, b, SysStringLen(b), NULL, 0, NULL, NULL);
        *dst_len = length; /* Doesn't include size for '\0' */
        *dst_status = DBSTATUS_S_OK;
        bytes_to_copy = min(length + 1, dst_max_len);
        if(dst)
        {
            if(bytes_to_copy >= sizeof(char))
            {
                WideCharToMultiByte(CP_ACP, 0, b, SysStringLen(b), dst, bytes_to_copy - 1, NULL, NULL);
                *((char *)dst + bytes_to_copy - 1) = 0;
                if(bytes_to_copy < length + 1)
                    *dst_status = DBSTATUS_S_TRUNCATED;
            }
            else
            {
                *dst_status = DBSTATUS_E_DATAOVERFLOW;
                hr = DB_E_ERRORSOCCURRED;
            }
        }
        SysFreeString(b);
        return hr;
    }

    case DBTYPE_BYREF | DBTYPE_WSTR:
    {
        BSTR b;
        WCHAR **d = dst;
        DBLENGTH bstr_len;
        hr = IDataConvert_DataConvert(iface, src_type, DBTYPE_BSTR, src_len, &bstr_len,
                                      src, &b, sizeof(BSTR), src_status, dst_status,
                                      precision, scale, flags);
        if(hr != S_OK) return hr;

        bstr_len = SysStringLen(b) * sizeof(WCHAR);
        *dst_len = bstr_len; /* Doesn't include size for '\0' */

        *d = CoTaskMemAlloc(bstr_len + sizeof(WCHAR));
        if(*d) memcpy(*d, b, bstr_len + sizeof(WCHAR));
        else hr = E_OUTOFMEMORY;
        SysFreeString(b);
        return hr;
    }

    case DBTYPE_BYREF | DBTYPE_STR:
    {
        BSTR b;
        char **d = dst;
        DBLENGTH length;
        hr = IDataConvert_DataConvert(iface, src_type, DBTYPE_BSTR, src_len, &length,
                                      src, &b, sizeof(BSTR), src_status, dst_status,
                                      precision, scale, flags);
        if(hr != S_OK) return hr;

        length = WideCharToMultiByte(CP_ACP, 0, b, SysStringLen(b) + 1, NULL, 0, NULL, NULL);
        *dst_len = length - 1; /* Doesn't include size for '\0' */
        *dst_status = DBSTATUS_S_OK;
        *d = CoTaskMemAlloc(length);

        if(*d)
            WideCharToMultiByte(CP_ACP, 0, b, SysStringLen(b) + 1, *d, length, NULL, NULL);
        else
            hr = E_OUTOFMEMORY;
        SysFreeString(b);

        return hr;
    }

    case DBTYPE_VARIANT:
    {
        VARIANT *v = dst;

        switch(src_type)
        {
        case DBTYPE_BOOL:
            V_VT(v) = VT_BOOL;
            V_BOOL(v) = *(VARIANT_BOOL*)src;
            hr = S_OK;
            break;
        case DBTYPE_I2:
            V_VT(v) = VT_I2;
            V_I2(v) = *(signed short*)src;
            hr = S_OK;
            break;
        case DBTYPE_I4:
            V_VT(v) = VT_I4;
            V_I4(v) = *(signed int*)src;
            hr = S_OK;
            break;
        case DBTYPE_I8:
            V_VT(v) = VT_DECIMAL;
            hr = VarDecFromI8( *(LONGLONG*)src,  &V_DECIMAL(v));
            break;
        case DBTYPE_R4:
            V_VT(v) = VT_R4;
            V_R4(v) = *(FLOAT*)src;
            hr = S_OK;
            break;
        case DBTYPE_R8:
            V_VT(v) = VT_R8;
            V_R8(v) = *(double*)src;
            hr = S_OK;
            break;
        case DBTYPE_BSTR:
        {
            BSTR s = *(WCHAR**)src;
            TRACE("%s\n", debugstr_w(s));
            V_VT(v) = VT_BSTR;
            V_BSTR(v) = SysAllocString(s);
            hr = V_BSTR(v) ? S_OK : E_OUTOFMEMORY;
            break;
        }
        case DBTYPE_DATE:
            V_VT(v) = VT_DATE;
            V_DATE(v) = *(DATE*)src;
            hr = S_OK;
            break;
        case DBTYPE_DBDATE:
        {
            SYSTEMTIME st;
            DBDATE *ts = src;

            V_VT(v) = VT_DATE;

            st.wYear = ts->year;
            st.wMonth = ts->month;
            st.wDay = ts->day;
            st.wHour = 0;
            st.wMinute = 0;
            st.wSecond = 0;
            st.wMilliseconds = 0;
            hr = (SystemTimeToVariantTime(&st, &V_DATE(v)) ? S_OK : E_FAIL);
            break;
        }
        case DBTYPE_DBTIMESTAMP:
        {
            SYSTEMTIME st;
            DBTIMESTAMP *ts = src;

            V_VT(v) = VT_DATE;

            st.wYear = ts->year;
            st.wMonth = ts->month;
            st.wDay = ts->day;
            st.wHour = ts->hour;
            st.wMinute = ts->minute;
            st.wSecond = ts->second;
            st.wMilliseconds = ts->fraction/1000000;
            hr = SystemTimeToVariantTime(&st, &V_DATE(v)) ? S_OK : E_FAIL;
            break;
        }
        case DBTYPE_CY:
            V_VT(v) = VT_CY;
            V_CY(v) = *(CY*)src;
            hr = S_OK;
            break;
        case DBTYPE_BYTES:
        {
            LONG i;
            SAFEARRAY *psa = NULL;
            SAFEARRAYBOUND rgsabound[1];
            unsigned char *p = src;

            rgsabound[0].lLbound = 0;
            rgsabound[0].cElements = src_len;

            psa = SafeArrayCreate(VT_UI1,1,rgsabound);
            for(i =0; i < src_len; i++,p++)
            {
                hr = SafeArrayPutElement(psa, &i, p);
                if(FAILED(hr)) {
                   SafeArrayDestroy (psa);
                   return hr;
                }
            }

            V_VT(v) = VT_ARRAY|VT_UI1;
            V_ARRAY(v) = psa;
            hr = S_OK;
            break;
        }
        case DBTYPE_VARIANT:
        {
            VariantInit(v);
            hr = VariantCopy(v, (VARIANT *)src);
            break;
        }
        default: FIXME("Unimplemented conversion %04x -> VARIANT\n", src_type); return E_NOTIMPL;
        }
        break;
    }
    case DBTYPE_BYTES:
    {
        BYTE *d = dst;

        switch(src_type)
        {
        case DBTYPE_BYTES:
            if( src_len > dst_max_len)
                *dst_status = DBSTATUS_S_TRUNCATED;
            else
                *dst_status = DBSTATUS_S_OK;

            *dst_len = src_len;
            memcpy(d, src, min(src_len, dst_max_len));

            return S_OK;
        case DBTYPE_VARIANT:
        {
            switch(V_VT((VARIANT*)src))
            {
            case VT_UI1 | VT_ARRAY:
            {
                LONG l;
                BYTE *data = NULL;

                hr = SafeArrayGetUBound(V_ARRAY((VARIANT*)src), 1, &l);
                if(FAILED(hr))
                    return hr;

                hr = SafeArrayAccessData(V_ARRAY((VARIANT*)src), (VOID**)&data);
                if(FAILED(hr))
                {
                    ERR("SafeArrayAccessData Failed = 0x%08lx\n", hr);
                    return hr;
                }

                *dst_len = l+1;
                *dst_status = DBSTATUS_S_OK;
                memcpy(d, data, *dst_len);

                SafeArrayUnaccessData(V_ARRAY((VARIANT*)src));
                return S_OK;
            }
            break;
            default:
                FIXME("Unimplemented variant type %d -> BYTES\n", V_VT((VARIANT*)src));
                return E_NOTIMPL;
            }
        }
        break;
        default: FIXME("Unimplemented conversion %04x -> DBTYPE_BYTES\n", src_type); return E_NOTIMPL;
        }
        break;
    }
    case DBTYPE_BYTES | DBTYPE_BYREF:
    {
        BYTE **d = dst;

        switch(src_type)
        {
        case DBTYPE_BYTES:
            *d = CoTaskMemAlloc(src_len);
            if(*d) memcpy(*d, src, src_len);
            else hr = E_OUTOFMEMORY;

            *dst_len = src_len;
            *dst_status = DBSTATUS_S_OK;
            return S_OK;
        default: FIXME("Unimplemented conversion %04x -> DBTYPE_BYTES | DBTYPE_BYREF\n", src_type); return E_NOTIMPL;
        }
        break;
    }

    case DBTYPE_NUMERIC:
        FIXME("Unimplemented conversion %04x -> DBTYPE_NUMERIC\n", src_type);
        return E_NOTIMPL;

    default:
        FIXME("Unimplemented conversion %04x -> %04x\n", src_type, dst_type);
        return E_NOTIMPL;
    }

    if(hr == DISP_E_OVERFLOW)
    {
        *dst_status = DBSTATUS_E_DATAOVERFLOW;
        *dst_len = get_length(dst_type);
        hr = DB_E_ERRORSOCCURRED;
    }
    else if(hr == S_OK)
    {
        *dst_status = DBSTATUS_S_OK;
        *dst_len = get_length(dst_type);
    }

    return hr;
}

static inline WORD get_dbtype_class(DBTYPE type)
{
    switch(type)
    {
    case DBTYPE_I2:
    case DBTYPE_R4:
    case DBTYPE_R8:
    case DBTYPE_I1:
    case DBTYPE_UI1:
    case DBTYPE_UI2:
        return DBTYPE_I2;

    case DBTYPE_I4:
    case DBTYPE_UI4:
        return DBTYPE_I4;

    case DBTYPE_I8:
    case DBTYPE_UI8:
        return DBTYPE_I8;

    case DBTYPE_BSTR:
    case DBTYPE_STR:
    case DBTYPE_WSTR:
        return DBTYPE_BSTR;

    case DBTYPE_DBDATE:
    case DBTYPE_DBTIME:
    case DBTYPE_DBTIMESTAMP:
        return DBTYPE_DBDATE;
    }
    return type;
}

/* Many src types will convert to this group of dst types */
static inline BOOL common_class(WORD dst_class)
{
    switch(dst_class)
    {
    case DBTYPE_EMPTY:
    case DBTYPE_NULL:
    case DBTYPE_I2:
    case DBTYPE_I4:
    case DBTYPE_BSTR:
    case DBTYPE_BOOL:
    case DBTYPE_VARIANT:
    case DBTYPE_I8:
    case DBTYPE_CY:
    case DBTYPE_DECIMAL:
    case DBTYPE_NUMERIC:
        return TRUE;
    }
    return FALSE;
}

static inline BOOL array_type(DBTYPE type)
{
    return (type >= DBTYPE_I2 && type <= DBTYPE_UI4);
}

static HRESULT WINAPI convert_CanConvert(IDataConvert* iface,
                                         DBTYPE src_type, DBTYPE dst_type)
{
    convert *This = impl_from_IDataConvert(iface);
    DBTYPE src_base_type = src_type & 0x1ff;
    DBTYPE dst_base_type = dst_type & 0x1ff;
    WORD dst_class = get_dbtype_class(dst_base_type);

    TRACE("(%p)->(%d, %d)\n", This, src_type, dst_type);

    if(src_type & DBTYPE_VECTOR || dst_type & DBTYPE_VECTOR) return S_FALSE;

    if(src_type & DBTYPE_ARRAY)
    {
        if(!array_type(src_base_type)) return S_FALSE;
        if(dst_type & DBTYPE_ARRAY)
        {
            if(src_type == dst_type) return S_OK;
            return S_FALSE;
        }
        if(dst_type == DBTYPE_VARIANT) return S_OK;
        return S_FALSE;
    }

    if(dst_type & DBTYPE_ARRAY)
    {
        if(!array_type(dst_base_type)) return S_FALSE;
        if(src_type == DBTYPE_IDISPATCH || src_type == DBTYPE_VARIANT) return S_OK;
        return S_FALSE;
    }

    if(dst_type & DBTYPE_BYREF)
        if(dst_base_type != DBTYPE_BYTES && dst_base_type != DBTYPE_STR && dst_base_type != DBTYPE_WSTR)
            return S_FALSE;

    switch(get_dbtype_class(src_base_type))
    {
    case DBTYPE_EMPTY:
        if(common_class(dst_class)) return S_OK;
        switch(dst_class)
        {
        case DBTYPE_DATE:
        case DBTYPE_GUID:
        case DBTYPE_FILETIME:
            return S_OK;
        default:
            if(dst_base_type == DBTYPE_DBTIMESTAMP) return S_OK;
            return S_FALSE;
        }

    case DBTYPE_NULL:
        switch(dst_base_type)
        {
        case DBTYPE_NULL:
        case DBTYPE_VARIANT:
        case DBTYPE_FILETIME: return S_OK;
        default: return S_FALSE;
        }

    case DBTYPE_I4:
        if(dst_base_type == DBTYPE_BYTES) return S_OK;
        /* fall through */
    case DBTYPE_I2:
        if(dst_base_type == DBTYPE_DATE) return S_OK;
        /* fall through */
    case DBTYPE_DECIMAL:
        if(common_class(dst_class)) return S_OK;
        if(dst_class == DBTYPE_DBDATE) return S_OK;
        return S_FALSE;

    case DBTYPE_BOOL:
        if(dst_base_type == DBTYPE_DATE) return S_OK;
        /* fall through */
    case DBTYPE_NUMERIC:
    case DBTYPE_CY:
        if(common_class(dst_class)) return S_OK;
        return S_FALSE;

    case DBTYPE_I8:
        if(common_class(dst_class)) return S_OK;
        switch(dst_base_type)
        {
        case DBTYPE_BYTES:
        case DBTYPE_FILETIME: return S_OK;
        default: return S_FALSE;
        }

    case DBTYPE_DATE:
        switch(dst_class)
        {
        case DBTYPE_EMPTY:
        case DBTYPE_NULL:
        case DBTYPE_I2:
        case DBTYPE_I4:
        case DBTYPE_BSTR:
        case DBTYPE_BOOL:
        case DBTYPE_VARIANT:
        case DBTYPE_I8:
        case DBTYPE_DATE:
        case DBTYPE_DBDATE:
        case DBTYPE_FILETIME:
            return S_OK;
        default: return S_FALSE;
        }

    case DBTYPE_IDISPATCH:
    case DBTYPE_VARIANT:
        switch(dst_base_type)
        {
        case DBTYPE_IDISPATCH:
        case DBTYPE_ERROR:
        case DBTYPE_IUNKNOWN:
            return S_OK;
        }
        /* fall through */
    case DBTYPE_BSTR:
        if(common_class(dst_class)) return S_OK;
        switch(dst_class)
        {
        case DBTYPE_DATE:
        case DBTYPE_GUID:
        case DBTYPE_BYTES:
        case DBTYPE_DBDATE:
        case DBTYPE_FILETIME:
            return S_OK;
        default: return S_FALSE;
        }

    case DBTYPE_ERROR:
        switch(dst_base_type)
        {
        case DBTYPE_BSTR:
        case DBTYPE_ERROR:
        case DBTYPE_VARIANT:
        case DBTYPE_WSTR:
            return S_OK;
        default: return S_FALSE;
        }

    case DBTYPE_IUNKNOWN:
        switch(dst_base_type)
        {
        case DBTYPE_EMPTY:
        case DBTYPE_NULL:
        case DBTYPE_IDISPATCH:
        case DBTYPE_VARIANT:
        case DBTYPE_IUNKNOWN:
            return S_OK;
        default: return S_FALSE;
        }

    case DBTYPE_BYTES:
        if(dst_class == DBTYPE_I4 || dst_class == DBTYPE_I8) return S_OK;
        /* fall through */
    case DBTYPE_GUID:
        switch(dst_class)
        {
        case DBTYPE_EMPTY:
        case DBTYPE_NULL:
        case DBTYPE_BSTR:
        case DBTYPE_VARIANT:
        case DBTYPE_GUID:
        case DBTYPE_BYTES:
            return S_OK;
        default: return S_FALSE;
        }

    case DBTYPE_FILETIME:
        if(dst_class == DBTYPE_I8) return S_OK;
        /* fall through */
    case DBTYPE_DBDATE:
        switch(dst_class)
        {
        case DBTYPE_EMPTY:
        case DBTYPE_NULL:
        case DBTYPE_DATE:
        case DBTYPE_BSTR:
        case DBTYPE_VARIANT:
        case DBTYPE_DBDATE:
        case DBTYPE_FILETIME:
            return S_OK;
        default: return S_FALSE;
        }

    }
    return S_FALSE;
}

static HRESULT WINAPI convert_GetConversionSize(IDataConvert* iface,
                                                DBTYPE src_type, DBTYPE dst_type,
                                                DBLENGTH *src_len, DBLENGTH *dst_len,
                                                void *src)
{
    convert *This = impl_from_IDataConvert(iface);
    HRESULT hr;

    TRACE("(%p)->(%d, %d, %p, %p, %p)\n", This, src_type, dst_type, src_len, dst_len, src);

    hr = IDataConvert_CanConvert(iface, src_type, dst_type);
    if (hr != S_OK)
        return DB_E_UNSUPPORTEDCONVERSION;

    if (!dst_len)
        return E_INVALIDARG;

    /* for some types we don't need to look into source data */
    if ((*dst_len = get_length(dst_type)))
        return S_OK;

    *dst_len = 110;

    if(src_type == DBTYPE_VARIANT && V_VT((VARIANT*)src) == VT_NULL)
        return S_OK;

    switch (dst_type)
    {
    case DBTYPE_STR:
        switch (src_type)
        {
        case DBTYPE_VARIANT:
        {
            VARIANT v;

            VariantInit(&v);
            if ((hr = VariantChangeType(&v, src, 0, VT_BSTR)) == S_OK)
            {
                *dst_len = WideCharToMultiByte(CP_ACP, 0, V_BSTR(&v), -1, NULL, 0, NULL, NULL);
                VariantClear(&v);
            }
            else
                return hr;
        }
        break;
        case DBTYPE_DATE:
        case DBTYPE_DECIMAL:
        case DBTYPE_EMPTY:
        case DBTYPE_I1:
        case DBTYPE_I2:
        case DBTYPE_UI2:
        case DBTYPE_I4:
        case DBTYPE_UI4:
        case DBTYPE_I8:
        case DBTYPE_UI8:
        case DBTYPE_R4:
        case DBTYPE_DBDATE:
        case DBTYPE_DBTIMESTAMP:
        case DBTYPE_DBTIME:
            break;
        default:
            FIXME("unimplemented for %04x -> DBTYPE_STR\n", src_type);
            return E_NOTIMPL;
        }
        break;
    case DBTYPE_WSTR:
        switch (src_type)
        {
        case DBTYPE_VARIANT:
        {
            VARIANT v;

            VariantInit(&v);
            if ((hr = VariantChangeType(&v, src, 0, VT_BSTR)) == S_OK)
            {
                *dst_len = (SysStringLen(V_BSTR(&v))+1) * sizeof(WCHAR);
                VariantClear(&v);
            }
        }
            break;
        case DBTYPE_STR:
            if(src_len)
                *dst_len = (*src_len + 1) * sizeof(WCHAR);
            else
                *dst_len = (strlen(src) + 1) * sizeof(WCHAR);
            break;
        case DBTYPE_WSTR:
            if(src_len)
                *dst_len = (*src_len) + sizeof(WCHAR);
            else
                *dst_len = (lstrlenW(src) + 1) * sizeof(WCHAR);
            break;
        case DBTYPE_DATE:
        case DBTYPE_DECIMAL:
        case DBTYPE_EMPTY:
        case DBTYPE_I1:
        case DBTYPE_I2:
        case DBTYPE_UI2:
        case DBTYPE_I4:
        case DBTYPE_UI4:
        case DBTYPE_I8:
        case DBTYPE_UI8:
        case DBTYPE_R4:
        case DBTYPE_DBDATE:
        case DBTYPE_DBTIMESTAMP:
        case DBTYPE_DBTIME:
            break;
        default:
            FIXME("unimplemented for %04x -> DBTYPE_WSTR\n", src_type);
            return E_NOTIMPL;
        }
        break;
    case DBTYPE_BYTES:
        switch (src_type)
        {
        case DBTYPE_VARIANT:
            if(V_VT((VARIANT*)src) == VT_BSTR)
                *dst_len = (SysStringLen(V_BSTR((VARIANT*)src))) / sizeof(WCHAR);
            else
            {
                switch(V_VT((VARIANT*)src))
                {
                case VT_UI1 | VT_ARRAY:
                {
                    LONG l;

                    hr = SafeArrayGetUBound(V_ARRAY((VARIANT*)src), 1, &l);
                    if(FAILED(hr))
                        return hr;
                    *dst_len = l+1;

                    break;
                }
                default:
                    WARN("DBTYPE_VARIANT(%d)->DBTYPE_BYTES unimplemented\n", V_VT((VARIANT*)src));
                }
            }
            break;
        default:
            FIXME("unimplemented for %04x -> DBTYPE_BYTES\n", src_type);
            return E_NOTIMPL;
        }
        break;
    default:
        FIXME("unimplemented for conversion %d->%d\n", src_type, dst_type);
        return E_NOTIMPL;
    }

    return S_OK;
}

static const struct IDataConvertVtbl convert_vtbl =
{
    convert_QueryInterface,
    convert_AddRef,
    convert_Release,
    convert_DataConvert,
    convert_CanConvert,
    convert_GetConversionSize
};

static HRESULT WINAPI dcinfo_QueryInterface(IDCInfo* iface, REFIID riid, void **obj)
{
    convert *This = impl_from_IDCInfo(iface);

    return IDataConvert_QueryInterface(&This->IDataConvert_iface, riid, obj);
}

static ULONG WINAPI dcinfo_AddRef(IDCInfo* iface)
{
    convert *This = impl_from_IDCInfo(iface);

    return IDataConvert_AddRef(&This->IDataConvert_iface);
}

static ULONG WINAPI dcinfo_Release(IDCInfo* iface)
{
    convert *This = impl_from_IDCInfo(iface);

    return IDataConvert_Release(&This->IDataConvert_iface);
}

static HRESULT WINAPI dcinfo_GetInfo(IDCInfo *iface, ULONG num, DCINFOTYPE types[], DCINFO **info_ptr)
{
    convert *This = impl_from_IDCInfo(iface);
    ULONG i;
    DCINFO *infos;

    TRACE("(%p)->(%ld, %p, %p)\n", This, num, types, info_ptr);

    *info_ptr = infos = CoTaskMemAlloc(num * sizeof(*infos));
    if(!infos) return E_OUTOFMEMORY;

    for(i = 0; i < num; i++)
    {
        infos[i].eInfoType = types[i];
        VariantInit(&infos[i].vData);

        switch(types[i])
        {
        case DCINFOTYPE_VERSION:
            V_VT(&infos[i].vData) = VT_UI4;
            V_UI4(&infos[i].vData) = This->version;
            break;
        }
    }

    return S_OK;
}

static HRESULT WINAPI dcinfo_SetInfo(IDCInfo* iface, ULONG num, DCINFO info[])
{
    convert *This = impl_from_IDCInfo(iface);
    ULONG i;
    HRESULT hr = S_OK;

    TRACE("(%p)->(%ld, %p)\n", This, num, info);

    for(i = 0; i < num; i++)
    {
        switch(info[i].eInfoType)
        {
        case DCINFOTYPE_VERSION:
            if(V_VT(&info[i].vData) != VT_UI4)
            {
                FIXME("VERSION with vt %x\n", V_VT(&info[i].vData));
                hr = DB_S_ERRORSOCCURRED;
                break;
            }
            This->version = V_UI4(&info[i].vData);
            break;

        default:
            FIXME("Unhandled info type %ld (vt %x)\n", info[i].eInfoType, V_VT(&info[i].vData));
        }
    }
    return hr;
}

static const struct IDCInfoVtbl dcinfo_vtbl =
{
    dcinfo_QueryInterface,
    dcinfo_AddRef,
    dcinfo_Release,
    dcinfo_GetInfo,
    dcinfo_SetInfo
};

HRESULT create_oledb_convert(IUnknown *outer, void **obj)
{
    convert *This;

    TRACE("(%p, %p)\n", outer, obj);

    *obj = NULL;

    if(outer) return CLASS_E_NOAGGREGATION;

    This = malloc(sizeof(*This));
    if(!This) return E_OUTOFMEMORY;

    This->IDataConvert_iface.lpVtbl = &convert_vtbl;
    This->IDCInfo_iface.lpVtbl = &dcinfo_vtbl;
    This->ref = 1;
    This->version = 0x110;

    *obj = &This->IDataConvert_iface;

    return S_OK;
}
