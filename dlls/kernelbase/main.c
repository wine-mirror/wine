/*
 * Copyright 2016 Michael Müller
 * Copyright 2017 Andrey Gusev
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

#define COBJMACROS

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windows.h"
#include "appmodel.h"
#include "shlwapi.h"
#include "perflib.h"

#include "wine/debug.h"
#include "wine/heap.h"
#include "winternl.h"

WINE_DEFAULT_DEBUG_CHANNEL(kernelbase);


/*************************************************************
 *            DllMainCRTStartup
 */
BOOL WINAPI DllMainCRTStartup( HANDLE inst, DWORD reason, LPVOID reserved )
{
    return TRUE;
}


/***********************************************************************
 *          AppPolicyGetProcessTerminationMethod (KERNELBASE.@)
 */
LONG WINAPI AppPolicyGetProcessTerminationMethod(HANDLE token, AppPolicyProcessTerminationMethod *policy)
{
    FIXME("%p, %p\n", token, policy);

    if(policy)
        *policy = AppPolicyProcessTerminationMethod_ExitProcess;

    return ERROR_SUCCESS;
}

/***********************************************************************
 *          AppPolicyGetThreadInitializationType (KERNELBASE.@)
 */
LONG WINAPI AppPolicyGetThreadInitializationType(HANDLE token, AppPolicyThreadInitializationType *policy)
{
    FIXME("%p, %p\n", token, policy);

    if(policy)
        *policy = AppPolicyThreadInitializationType_None;

    return ERROR_SUCCESS;
}

/***********************************************************************
 *          AppPolicyGetShowDeveloperDiagnostic (KERNELBASE.@)
 */
LONG WINAPI AppPolicyGetShowDeveloperDiagnostic(HANDLE token, AppPolicyShowDeveloperDiagnostic *policy)
{
    FIXME("%p, %p\n", token, policy);

    if(policy)
        *policy = AppPolicyShowDeveloperDiagnostic_ShowUI;

    return ERROR_SUCCESS;
}

/***********************************************************************
 *          AppPolicyGetWindowingModel (KERNELBASE.@)
 */
LONG WINAPI AppPolicyGetWindowingModel(HANDLE token, AppPolicyWindowingModel *policy)
{
    FIXME("%p, %p\n", token, policy);

    if(policy)
        *policy = AppPolicyWindowingModel_ClassicDesktop;

    return ERROR_SUCCESS;
}

/***********************************************************************
 *           PerfCreateInstance   (KERNELBASE.@)
 */
PPERF_COUNTERSET_INSTANCE WINAPI PerfCreateInstance(HANDLE handle, LPCGUID guid,
                                                    const WCHAR *name, ULONG id)
{
    FIXME("%p %s %s %u: stub\n", handle, debugstr_guid(guid), debugstr_w(name), id);
    return NULL;
}

/***********************************************************************
 *           PerfDeleteInstance   (KERNELBASE.@)
 */
ULONG WINAPI PerfDeleteInstance(HANDLE provider, PPERF_COUNTERSET_INSTANCE block)
{
    FIXME("%p %p: stub\n", provider, block);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *           PerfSetCounterSetInfo   (KERNELBASE.@)
 */
ULONG WINAPI PerfSetCounterSetInfo(HANDLE handle, PPERF_COUNTERSET_INFO template, ULONG size)
{
    FIXME("%p %p %u: stub\n", handle, template, size);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *           PerfSetCounterRefValue   (KERNELBASE.@)
 */
ULONG WINAPI PerfSetCounterRefValue(HANDLE provider, PPERF_COUNTERSET_INSTANCE instance,
                                    ULONG counterid, void *address)
{
    FIXME("%p %p %u %p: stub\n", provider, instance, counterid, address);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *           PerfStartProvider   (KERNELBASE.@)
 */
ULONG WINAPI PerfStartProvider(GUID *guid, PERFLIBREQUEST callback, HANDLE *provider)
{
    FIXME("%s %p %p: stub\n", debugstr_guid(guid), callback, provider);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *           PerfStartProviderEx   (KERNELBASE.@)
 */
ULONG WINAPI PerfStartProviderEx(GUID *guid, PPERF_PROVIDER_CONTEXT context, HANDLE *provider)
{
    FIXME("%s %p %p: stub\n", debugstr_guid(guid), context, provider);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *           PerfStopProvider   (KERNELBASE.@)
 */
ULONG WINAPI PerfStopProvider(HANDLE handle)
{
    FIXME("%p: stub\n", handle);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *           QuirkIsEnabled   (KERNELBASE.@)
 */
BOOL WINAPI QuirkIsEnabled(void *arg)
{
    FIXME("(%p): stub\n", arg);
    return FALSE;
}

/***********************************************************************
 *          QuirkIsEnabled3 (KERNELBASE.@)
 */
BOOL WINAPI QuirkIsEnabled3(void *unk1, void *unk2)
{
    static int once;

    if (!once++)
        FIXME("(%p, %p) stub!\n", unk1, unk2);

    return FALSE;
}

/***********************************************************************
 *           WaitOnAddress   (KERNELBASE.@)
 */
BOOL WINAPI WaitOnAddress(volatile void *addr, void *cmp, SIZE_T size, DWORD timeout)
{
    LARGE_INTEGER to;
    NTSTATUS status;

    if (timeout != INFINITE)
    {
        to.QuadPart = -(LONGLONG)timeout * 10000;
        status = RtlWaitOnAddress((const void *)addr, cmp, size, &to);
    }
    else
        status = RtlWaitOnAddress((const void *)addr, cmp, size, NULL);

    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }

    return TRUE;
}

HRESULT WINAPI QISearch(void *base, const QITAB *table, REFIID riid, void **obj)
{
    const QITAB *ptr;
    IUnknown *unk;

    TRACE("%p, %p, %s, %p\n", base, table, debugstr_guid(riid), obj);

    if (!obj)
        return E_POINTER;

    for (ptr = table; ptr->piid; ++ptr)
    {
        TRACE("trying (offset %d) %s\n", ptr->dwOffset, debugstr_guid(ptr->piid));
        if (IsEqualIID(riid, ptr->piid))
        {
            unk = (IUnknown *)((BYTE *)base + ptr->dwOffset);
            TRACE("matched, returning (%p)\n", unk);
            *obj = unk;
            IUnknown_AddRef(unk);
            return S_OK;
        }
    }

    if (IsEqualIID(riid, &IID_IUnknown))
    {
        unk = (IUnknown *)((BYTE *)base + table->dwOffset);
        TRACE("returning first for IUnknown (%p)\n", unk);
        *obj = unk;
        IUnknown_AddRef(unk);
        return S_OK;
    }

    WARN("Not found %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

HRESULT WINAPI GetAcceptLanguagesA(LPSTR langbuf, DWORD *buflen)
{
    DWORD buflenW, convlen;
    WCHAR *langbufW;
    HRESULT hr;

    TRACE("%p, %p, *%p: %d\n", langbuf, buflen, buflen, buflen ? *buflen : -1);

    if (!langbuf || !buflen || !*buflen)
        return E_FAIL;

    buflenW = *buflen;
    langbufW = heap_alloc(sizeof(WCHAR) * buflenW);
    hr = GetAcceptLanguagesW(langbufW, &buflenW);

    if (hr == S_OK)
    {
        convlen = WideCharToMultiByte(CP_ACP, 0, langbufW, -1, langbuf, *buflen, NULL, NULL);
        convlen--;  /* do not count the terminating 0 */
    }
    else  /* copy partial string anyway */
    {
        convlen = WideCharToMultiByte(CP_ACP, 0, langbufW, *buflen, langbuf, *buflen, NULL, NULL);
        if (convlen < *buflen)
        {
            langbuf[convlen] = 0;
            convlen--;  /* do not count the terminating 0 */
        }
        else
        {
            convlen = *buflen;
        }
    }
    *buflen = buflenW ? convlen : 0;

    heap_free(langbufW);
    return hr;
}

static HRESULT lcid_to_rfc1766(LCID lcid, WCHAR *rfc1766, INT len)
{
    WCHAR buffer[6 /* MAX_RFC1766_NAME */];
    INT n = GetLocaleInfoW(lcid, LOCALE_SISO639LANGNAME, buffer, ARRAY_SIZE(buffer));
    INT i;

    if (n)
    {
        i = PRIMARYLANGID(lcid);
        if ((((i == LANG_ENGLISH) || (i == LANG_CHINESE) || (i == LANG_ARABIC)) &&
            (SUBLANGID(lcid) == SUBLANG_DEFAULT)) ||
            (SUBLANGID(lcid) > SUBLANG_DEFAULT)) {

            buffer[n - 1] = '-';
            i = GetLocaleInfoW(lcid, LOCALE_SISO3166CTRYNAME, buffer + n, ARRAY_SIZE(buffer) - n);
            if (!i)
                buffer[n - 1] = '\0';
        }
        else
            i = 0;

        LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE, buffer, n + i, rfc1766, len);
        return ((n + i) > len) ? E_INVALIDARG : S_OK;
    }
    return E_FAIL;
}

HRESULT WINAPI GetAcceptLanguagesW(WCHAR *langbuf, DWORD *buflen)
{
    static const WCHAR keyW[] = {
        'S','o','f','t','w','a','r','e','\\',
        'M','i','c','r','o','s','o','f','t','\\',
        'I','n','t','e','r','n','e','t',' ','E','x','p','l','o','r','e','r','\\',
        'I','n','t','e','r','n','a','t','i','o','n','a','l',0};
    static const WCHAR valueW[] = {'A','c','c','e','p','t','L','a','n','g','u','a','g','e',0};
    DWORD mystrlen, mytype;
    WCHAR *mystr;
    LCID mylcid;
    HKEY mykey;
    LONG lres;
    DWORD len;

    TRACE("%p, %p, *%p: %d\n", langbuf, buflen, buflen, buflen ? *buflen : -1);

    if (!langbuf || !buflen || !*buflen)
        return E_FAIL;

    mystrlen = (*buflen > 20) ? *buflen : 20 ;
    len = mystrlen * sizeof(WCHAR);
    mystr = heap_alloc(len);
    mystr[0] = 0;
    RegOpenKeyExW(HKEY_CURRENT_USER, keyW, 0, KEY_QUERY_VALUE, &mykey);
    lres = RegQueryValueExW(mykey, valueW, 0, &mytype, (PBYTE)mystr, &len);
    RegCloseKey(mykey);
    len = lstrlenW(mystr);

    if (!lres && (*buflen > len))
    {
        lstrcpyW(langbuf, mystr);
        *buflen = len;
        heap_free(mystr);
        return S_OK;
    }

    /* Did not find a value in the registry or the user buffer is too small */
    mylcid = GetUserDefaultLCID();
    lcid_to_rfc1766(mylcid, mystr, mystrlen);
    len = lstrlenW(mystr);

    memcpy(langbuf, mystr, min(*buflen, len + 1)*sizeof(WCHAR));
    heap_free(mystr);

    if (*buflen > len)
    {
        *buflen = len;
        return S_OK;
    }

    *buflen = 0;
    return E_NOT_SUFFICIENT_BUFFER;
}
