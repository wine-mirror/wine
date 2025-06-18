/*
 * 16 bit ole functions
 *
 * Copyright 1995 Martin von Loewis
 * Copyright 1998 Justin Bradford
 * Copyright 1999 Francis Beaudet
 * Copyright 1999 Sylvain St-Germain
 * Copyright 2002 Marcus Meissner
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


#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"
#include "ole2.h"
#include "rpc.h"
#include "winerror.h"
#include "winreg.h"
#include "wownt32.h"
#include "wtypes.h"
#include "wine/winbase16.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

typedef LPCSTR LPCOLESTR16;

#define CHARS_IN_GUID 39

typedef struct
{
    SEGPTR QueryInterface;
    SEGPTR AddRef;
    SEGPTR Release;
    SEGPTR Alloc;
    SEGPTR Realloc;
    SEGPTR Free;
    SEGPTR GetSize;
    SEGPTR DidAlloc;
    SEGPTR HeapMinimize;
} IMalloc16Vtbl;

typedef struct
{
    SEGPTR lpVtbl;
} IMalloc16;

static ULONG call_IMalloc_AddRef(SEGPTR iface)
{
    IMalloc16 *malloc = MapSL(iface);
    IMalloc16Vtbl *vtbl = MapSL(malloc->lpVtbl);
    DWORD args[1], ret;

    args[0] = iface;
    WOWCallback16Ex(vtbl->AddRef, WCB16_CDECL, sizeof(args), args, &ret);
    return ret;
}

static ULONG call_IMalloc_Release(SEGPTR iface)
{
    IMalloc16 *malloc = MapSL(iface);
    IMalloc16Vtbl *vtbl = MapSL(malloc->lpVtbl);
    DWORD args[1], ret;

    args[0] = iface;
    WOWCallback16Ex(vtbl->Release, WCB16_CDECL, sizeof(args), args, &ret);
    return ret;
}

static SEGPTR call_IMalloc_Alloc(SEGPTR iface, DWORD size)
{
    IMalloc16 *malloc = MapSL(iface);
    IMalloc16Vtbl *vtbl = MapSL(malloc->lpVtbl);
    DWORD args[2], ret;

    args[0] = iface;
    args[1] = size;
    WOWCallback16Ex(vtbl->Alloc, WCB16_CDECL, sizeof(args), args, &ret);
    return ret;
}

static HTASK16 hETask = 0;
static WORD Table_ETask[62];

static SEGPTR compobj_malloc;

/* --- IMalloc16 implementation */


typedef struct
{
    IMalloc16 IMalloc16_iface;
    LONG refcount;
} IMalloc16Impl;

static inline IMalloc16Impl *impl_from_IMalloc16(IMalloc16 *iface)
{
        return CONTAINING_RECORD(iface, IMalloc16Impl, IMalloc16_iface);
}

/******************************************************************************
 *		IMalloc16_QueryInterface	[COMPOBJ.500]
 */
HRESULT CDECL IMalloc16_fnQueryInterface(IMalloc16* iface,REFIID refiid,LPVOID *obj) {
        IMalloc16Impl *This = impl_from_IMalloc16(iface);

	TRACE("(%p)->QueryInterface(%s,%p)\n",This,debugstr_guid(refiid),obj);
	if (	!memcmp(&IID_IUnknown,refiid,sizeof(IID_IUnknown)) ||
		!memcmp(&IID_IMalloc,refiid,sizeof(IID_IMalloc))
	) {
		*obj = This;
		return 0;
	}
	return OLE_E_ENUM_NOMORE;
}

/******************************************************************************
 *		IMalloc16_AddRef	[COMPOBJ.501]
 */
ULONG CDECL IMalloc16_fnAddRef(IMalloc16 *iface)
{
    IMalloc16Impl *malloc = impl_from_IMalloc16(iface);
    ULONG refcount = InterlockedIncrement(&malloc->refcount);
    TRACE("%p increasing refcount to %lu.\n", malloc, refcount);
    return refcount;
}

/******************************************************************************
 *		IMalloc16_Release	[COMPOBJ.502]
 */
ULONG CDECL IMalloc16_fnRelease(SEGPTR iface)
{
    IMalloc16Impl *malloc = impl_from_IMalloc16(MapSL(iface));
    ULONG refcount = InterlockedDecrement(&malloc->refcount);
    TRACE("%p decreasing refcount to %lu.\n", malloc, refcount);
    if (!refcount)
    {
        UnMapLS(iface);
        HeapFree(GetProcessHeap(), 0, malloc);
    }
    return refcount;
}

/******************************************************************************
 * IMalloc16_Alloc [COMPOBJ.503]
 */
SEGPTR CDECL IMalloc16_fnAlloc(IMalloc16* iface,DWORD cb) {
        IMalloc16Impl *This = impl_from_IMalloc16(iface);

	TRACE("(%p)->Alloc(%ld)\n",This,cb);
        return MapLS( HeapAlloc( GetProcessHeap(), 0, cb ) );
}

/******************************************************************************
 * IMalloc16_Free [COMPOBJ.505]
 */
VOID CDECL IMalloc16_fnFree(IMalloc16* iface,SEGPTR pv)
{
    void *ptr = MapSL(pv);
    IMalloc16Impl *This = impl_from_IMalloc16(iface);
    TRACE("(%p)->Free(%08lx)\n",This,pv);
    UnMapLS(pv);
    HeapFree( GetProcessHeap(), 0, ptr );
}

/******************************************************************************
 * IMalloc16_Realloc [COMPOBJ.504]
 */
SEGPTR CDECL IMalloc16_fnRealloc(IMalloc16* iface,SEGPTR pv,DWORD cb)
{
    SEGPTR ret;
    IMalloc16Impl *This = impl_from_IMalloc16(iface);

    TRACE("(%p)->Realloc(%08lx,%ld)\n",This,pv,cb);
    if (!pv)
	ret = IMalloc16_fnAlloc(iface, cb);
    else if (cb) {
        ret = MapLS( HeapReAlloc( GetProcessHeap(), 0, MapSL(pv), cb ) );
        UnMapLS(pv);
    } else {
	IMalloc16_fnFree(iface, pv);
	ret = 0;
    }
    return ret;
}

/******************************************************************************
 * IMalloc16_GetSize [COMPOBJ.506]
 */
DWORD CDECL IMalloc16_fnGetSize(IMalloc16* iface,SEGPTR pv)
{
        IMalloc16Impl *This = impl_from_IMalloc16(iface);

        TRACE("(%p)->GetSize(%08lx)\n",This,pv);
        return HeapSize( GetProcessHeap(), 0, MapSL(pv) );
}

/******************************************************************************
 * IMalloc16_DidAlloc [COMPOBJ.507]
 */
INT16 CDECL IMalloc16_fnDidAlloc(IMalloc16* iface,LPVOID pv) {
        IMalloc16Impl *This = impl_from_IMalloc16(iface);

	TRACE("(%p)->DidAlloc(%p)\n",This,pv);
	return (INT16)-1;
}

/******************************************************************************
 * IMalloc16_HeapMinimize [COMPOBJ.508]
 */
LPVOID CDECL IMalloc16_fnHeapMinimize(IMalloc16* iface) {
        IMalloc16Impl *This = impl_from_IMalloc16(iface);

	TRACE("(%p)->HeapMinimize()\n",This);
	return NULL;
}

/******************************************************************************
 * IMalloc16_Constructor [VTABLE]
 */
static SEGPTR IMalloc16_Constructor(void)
{
    static IMalloc16Vtbl vt16;
    static SEGPTR msegvt16;
    IMalloc16Impl* This;
    HMODULE16 hcomp = GetModuleHandle16("COMPOBJ");

    This = HeapAlloc( GetProcessHeap(), 0, sizeof(IMalloc16Impl) );
    if (!msegvt16)
    {
#define VTENT(x) vt16.x = (SEGPTR)GetProcAddress16(hcomp,"IMalloc16_"#x);assert(vt16.x)
        VTENT(QueryInterface);
        VTENT(AddRef);
        VTENT(Release);
        VTENT(Alloc);
        VTENT(Realloc);
        VTENT(Free);
        VTENT(GetSize);
        VTENT(DidAlloc);
        VTENT(HeapMinimize);
#undef VTENT
        msegvt16 = MapLS( &vt16 );
    }
    This->IMalloc16_iface.lpVtbl = msegvt16;
    This->refcount = 1;
    return MapLS(This);
}

/******************************************************************************
 *           CoBuildVersion [COMPOBJ.1]
 */
DWORD WINAPI CoBuildVersion16(void)
{
    return CoBuildVersion();
}

/***********************************************************************
 *           CoGetMalloc   [COMPOBJ.4]
 */
HRESULT WINAPI CoGetMalloc16(MEMCTX context, SEGPTR *malloc)
{
    call_IMalloc_AddRef(*malloc = compobj_malloc);
    return S_OK;
}

/***********************************************************************
 *           CoCreateStandardMalloc [COMPOBJ.71]
 */
HRESULT WINAPI CoCreateStandardMalloc16(MEMCTX context, SEGPTR *malloc)
{
    /* FIXME: docu says we shouldn't return the same allocator as in
     * CoGetMalloc16 */
    *malloc = IMalloc16_Constructor();
    return S_OK;
}

/***********************************************************************
 *           CoMemAlloc [COMPOBJ.151]
 */
SEGPTR WINAPI CoMemAlloc(DWORD size, MEMCTX context, DWORD unknown)
{
    TRACE("size %lu, context %d, unknown %#lx.\n", size, context, unknown);
    if (context != MEMCTX_TASK)
        FIXME("Ignoring context %d.\n", context);
    if (unknown)
        FIXME("Ignoring unknown parameter %#lx.\n", unknown);

    return call_IMalloc_Alloc(compobj_malloc, size);
}

/***********************************************************************
 *           CoInitialize   [COMPOBJ.2]
 */
HRESULT WINAPI CoInitialize16(SEGPTR malloc)
{
    if (!malloc)
        CoCreateStandardMalloc16(MEMCTX_TASK, &compobj_malloc);
    else
        call_IMalloc_AddRef(compobj_malloc = malloc);
    return S_OK;
}

/***********************************************************************
 *           CoUninitialize   [COMPOBJ.3]
 * Don't know what it does.
 * 3-Nov-98 -- this was originally misspelled, I changed it to what I
 *   believe is the correct spelling
 */
void WINAPI CoUninitialize16(void)
{
    TRACE("\n");

    CoFreeAllLibraries();
    call_IMalloc_Release(compobj_malloc);
    compobj_malloc = 0;
}

/***********************************************************************
 *           CoFreeUnusedLibraries [COMPOBJ.17]
 */
void WINAPI CoFreeUnusedLibraries16(void)
{
    CoFreeUnusedLibraries();
}

/***********************************************************************
 *           IsEqualGUID [COMPOBJ.18]
 *
 * Compares two Unique Identifiers.
 *
 * RETURNS
 *	TRUE if equal
 */
BOOL16 WINAPI IsEqualGUID16(
	GUID* g1,	/* [in] unique id 1 */
	GUID* g2)	/* [in] unique id 2 */
{
    return !memcmp( g1, g2, sizeof(GUID) );
}

/******************************************************************************
 *		CLSIDFromString	[COMPOBJ.20]
 * Converts a unique identifier from its string representation into
 * the GUID struct.
 *
 * Class id: DWORD-WORD-WORD-BYTES[2]-BYTES[6]
 *
 * RETURNS
 *	the converted GUID
 */
HRESULT WINAPI CLSIDFromString16(
	LPCOLESTR16 idstr,	/* [in] string representation of guid */
	CLSID *id)		/* [out] GUID converted from string */
{
  const BYTE *s;
  int	i;
  BYTE table[256];

  if (!idstr) {
    memset( id, 0, sizeof (CLSID) );
    return S_OK;
  }

  /* validate the CLSID string */
  if (strlen(idstr) != 38)
    return CO_E_CLASSSTRING;

  s = (const BYTE *) idstr;
  if ((s[0]!='{') || (s[9]!='-') || (s[14]!='-') || (s[19]!='-') || (s[24]!='-') || (s[37]!='}'))
    return CO_E_CLASSSTRING;

  for (i=1; i<37; i++) {
    if ((i == 9)||(i == 14)||(i == 19)||(i == 24)) continue;
    if (!(((s[i] >= '0') && (s[i] <= '9'))  ||
          ((s[i] >= 'a') && (s[i] <= 'f'))  ||
          ((s[i] >= 'A') && (s[i] <= 'F'))))
       return CO_E_CLASSSTRING;
  }

  TRACE("%s -> %p\n", s, id);

  /* quick lookup table */
  memset(table, 0, 256);

  for (i = 0; i < 10; i++) {
    table['0' + i] = i;
  }
  for (i = 0; i < 6; i++) {
    table['A' + i] = i+10;
    table['a' + i] = i+10;
  }

  /* in form {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} */

  id->Data1 = (table[s[1]] << 28 | table[s[2]] << 24 | table[s[3]] << 20 | table[s[4]] << 16 |
               table[s[5]] << 12 | table[s[6]] << 8  | table[s[7]] << 4  | table[s[8]]);
  id->Data2 = table[s[10]] << 12 | table[s[11]] << 8 | table[s[12]] << 4 | table[s[13]];
  id->Data3 = table[s[15]] << 12 | table[s[16]] << 8 | table[s[17]] << 4 | table[s[18]];

  /* these are just sequential bytes */
  id->Data4[0] = table[s[20]] << 4 | table[s[21]];
  id->Data4[1] = table[s[22]] << 4 | table[s[23]];
  id->Data4[2] = table[s[25]] << 4 | table[s[26]];
  id->Data4[3] = table[s[27]] << 4 | table[s[28]];
  id->Data4[4] = table[s[29]] << 4 | table[s[30]];
  id->Data4[5] = table[s[31]] << 4 | table[s[32]];
  id->Data4[6] = table[s[33]] << 4 | table[s[34]];
  id->Data4[7] = table[s[35]] << 4 | table[s[36]];

  return S_OK;
}

/***********************************************************************
 *           StringFromCLSID   [COMPOBJ.151]
 */
HRESULT WINAPI StringFromCLSID16(REFCLSID id, SEGPTR *str)
{
    WCHAR buffer[40];
    if (!(*str = CoMemAlloc(40, MEMCTX_TASK, 0)))
        return E_OUTOFMEMORY;
    StringFromGUID2( id, buffer, 40 );
    WideCharToMultiByte( CP_ACP, 0, buffer, -1, MapSL(*str), 40, NULL, NULL );
    return S_OK;
}

/***********************************************************************
 *           ProgIDFromCLSID   [COMPOBJ.151]
 */
HRESULT WINAPI ProgIDFromCLSID16(REFCLSID clsid, SEGPTR *str)
{
    LPOLESTR progid;
    HRESULT ret;

    ret = ProgIDFromCLSID( clsid, &progid );
    if (ret == S_OK)
    {
        INT len = WideCharToMultiByte( CP_ACP, 0, progid, -1, NULL, 0, NULL, NULL );
        if ((*str = CoMemAlloc(len, MEMCTX_TASK, 0)))
            WideCharToMultiByte( CP_ACP, 0, progid, -1, MapSL(*str), len, NULL, NULL );
        CoTaskMemFree( progid );
    }
    return ret;
}

/***********************************************************************
 *           LookupETask (COMPOBJ.94)
 */
HRESULT WINAPI LookupETask16(HTASK16 *hTask,LPVOID p) {
	FIXME("(%p,%p),stub!\n",hTask,p);
	if ((*hTask = GetCurrentTask()) == hETask) {
		memcpy(p, Table_ETask, sizeof(Table_ETask));
	}
	return 0;
}

/***********************************************************************
 *           SetETask (COMPOBJ.95)
 */
HRESULT WINAPI SetETask16(HTASK16 hTask, LPVOID p) {
        FIXME("(%04x,%p),stub!\n",hTask,p);
	hETask = hTask;
	return 0;
}

/***********************************************************************
 *           CALLOBJECTINWOW (COMPOBJ.201)
 */
HRESULT WINAPI CallObjectInWOW(LPVOID p1,LPVOID p2) {
	FIXME("(%p,%p),stub!\n",p1,p2);
	return 0;
}

/******************************************************************************
 *		CoRegisterClassObject	[COMPOBJ.5]
 *
 * Don't know where it registers it ...
 */
HRESULT WINAPI CoRegisterClassObject16(
	REFCLSID rclsid,
	LPUNKNOWN pUnk,
	DWORD dwClsContext, /* [in] CLSCTX flags indicating the context in which to run the executable */
	DWORD flags,        /* [in] REGCLS flags indicating how connections are made */
	LPDWORD lpdwRegister
) {
	FIXME("(%s,%p,0x%08lx,0x%08lx,%p),stub\n",
		debugstr_guid(rclsid),pUnk,dwClsContext,flags,lpdwRegister
	);
	return 0;
}

/******************************************************************************
 *      CoRevokeClassObject [COMPOBJ.6]
 *
 */
HRESULT WINAPI CoRevokeClassObject16(DWORD dwRegister) /* [in] token on class obj */
{
    FIXME("(0x%08lx),stub!\n", dwRegister);
    return 0;
}

/******************************************************************************
 *              IsValidInterface        [COMPOBJ.23]
 *
 * Determines whether a pointer is a valid interface.
 *
 * PARAMS
 *  punk [I] Interface to be tested.
 *
 * RETURNS
 *  TRUE, if the passed pointer is a valid interface, or FALSE otherwise.
 */
BOOL WINAPI IsValidInterface16(SEGPTR punk)
{
	DWORD **ptr;

	if (IsBadReadPtr16(punk,4))
		return FALSE;
	ptr = MapSL(punk);
	if (IsBadReadPtr16((SEGPTR)ptr[0],4))	/* check vtable ptr */
		return FALSE;
	ptr = MapSL((SEGPTR)ptr[0]);		/* ptr to first method */
	if (IsBadReadPtr16((SEGPTR)ptr[0],2))
		return FALSE;
	return TRUE;
}

/******************************************************************************
 *      CoFileTimeToDosDateTime [COMPOBJ.30]
 */
BOOL16 WINAPI CoFileTimeToDosDateTime16(const FILETIME *ft, LPWORD lpDosDate, LPWORD lpDosTime)
{
    return FileTimeToDosDateTime(ft, lpDosDate, lpDosTime);
}

/******************************************************************************
 *      CoDosDateTimeToFileTime [COMPOBJ.31]
 */
BOOL16 WINAPI CoDosDateTimeToFileTime16(WORD wDosDate, WORD wDosTime, FILETIME *ft)
{
    return DosDateTimeToFileTime(wDosDate, wDosTime, ft);
}

/******************************************************************************
 *		CoGetCurrentProcess	[COMPOBJ.34]
 */
DWORD WINAPI CoGetCurrentProcess16(void)
{
    return CoGetCurrentProcess();
}

/******************************************************************************
 *		CoRegisterMessageFilter	[COMPOBJ.27]
 */
HRESULT WINAPI CoRegisterMessageFilter16(
	LPMESSAGEFILTER lpMessageFilter,
	LPMESSAGEFILTER *lplpMessageFilter
) {
	FIXME("(%p,%p),stub!\n",lpMessageFilter,lplpMessageFilter);
	return 0;
}

/******************************************************************************
 *		CoLockObjectExternal	[COMPOBJ.63]
 */
HRESULT WINAPI CoLockObjectExternal16(
    LPUNKNOWN pUnk,		/* [in] object to be locked */
    BOOL16 fLock,		/* [in] do lock */
    BOOL16 fLastUnlockReleases	/* [in] ? */
) {
    FIXME("(%p,%d,%d),stub!\n",pUnk,fLock,fLastUnlockReleases);
    return S_OK;
}

/***********************************************************************
 *           CoGetState [COMPOBJ.115]
 */
HRESULT WINAPI CoGetState16(LPDWORD state)
{
    FIXME("(%p),stub!\n", state);

    *state = 0;
    return S_OK;
}

/***********************************************************************
 *      DllEntryPoint                   [COMPOBJ.116]
 *
 *    Initialization code for the COMPOBJ DLL
 *
 * RETURNS:
 */
BOOL WINAPI COMPOBJ_DllEntryPoint(DWORD Reason, HINSTANCE16 hInst, WORD ds, WORD HeapSize, DWORD res1, WORD res2)
{
        TRACE("(%08lx, %04x, %04x, %04x, %08lx, %04x)\n", Reason, hInst, ds, HeapSize, res1, res2);
        return TRUE;
}

/******************************************************************************
 *		CLSIDFromProgID [COMPOBJ.61]
 *
 * Converts a program ID into the respective GUID.
 *
 * PARAMS
 *  progid       [I] program id as found in registry
 *  riid         [O] associated CLSID
 *
 * RETURNS
 *	Success: S_OK
 *  Failure: CO_E_CLASSSTRING - the given ProgID cannot be found.
 */
HRESULT WINAPI CLSIDFromProgID16(LPCOLESTR16 progid, LPCLSID riid)
{
	char	*buf,buf2[80];
	LONG	buf2len;
	HKEY	xhkey;

	buf = HeapAlloc(GetProcessHeap(),0,strlen(progid)+8);
	sprintf(buf,"%s\\CLSID",progid);
	if (RegOpenKeyA(HKEY_CLASSES_ROOT,buf,&xhkey)) {
		HeapFree(GetProcessHeap(),0,buf);
                return CO_E_CLASSSTRING;
	}
	HeapFree(GetProcessHeap(),0,buf);
	buf2len = sizeof(buf2);
	if (RegQueryValueA(xhkey,NULL,buf2,&buf2len)) {
		RegCloseKey(xhkey);
                return CO_E_CLASSSTRING;
	}
	RegCloseKey(xhkey);
	return CLSIDFromString16(buf2,riid);
}

/******************************************************************************
 *		StringFromGUID2	[COMPOBJ.76]
 */
INT16 WINAPI StringFromGUID216(REFGUID id, char *str, INT16 cmax)
{
    static const char format[] = "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}";
    if (!id || cmax < CHARS_IN_GUID) return 0;
    sprintf( str, format, id->Data1, id->Data2, id->Data3,
             id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
             id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7] );
    return CHARS_IN_GUID;
}


/***********************************************************************
 *           CoFileTimeNow [COMPOBJ.82]
 */
HRESULT WINAPI CoFileTimeNow16( FILETIME *lpFileTime )
{
    return CoFileTimeNow( lpFileTime );
}

/***********************************************************************
 *           CoGetClassObject [COMPOBJ.7]
 *
 */
HRESULT WINAPI CoGetClassObject16(
    SEGPTR rclsid, DWORD dwClsContext, COSERVERINFO *pServerInfo,
    SEGPTR riid, SEGPTR ppv)
{
    LPVOID *ppvl = MapSL(ppv);

    TRACE("CLSID: %s, IID: %s\n", debugstr_guid(MapSL(rclsid)), debugstr_guid(MapSL(riid)));

    *ppvl = NULL;

    if (pServerInfo) {
        FIXME("pServerInfo->name=%s pAuthInfo=%p\n",
              debugstr_w(pServerInfo->pwszName), pServerInfo->pAuthInfo);
    }

    if (CLSCTX_INPROC_SERVER & dwClsContext)
    {
        char idstr[CHARS_IN_GUID];
        char buf_key[CHARS_IN_GUID+19], dllpath[MAX_PATH+1];
        LONG dllpath_len = sizeof(dllpath);

        HMODULE16 dll;
        FARPROC16 DllGetClassObject;

        WORD args[6];
        DWORD dwRet;

        StringFromGUID216(MapSL(rclsid), idstr, CHARS_IN_GUID);
        sprintf(buf_key, "CLSID\\%s\\InprocServer", idstr);
        if (RegQueryValueA(HKEY_CLASSES_ROOT, buf_key, dllpath, &dllpath_len))
        {
            ERR("class %s not registered\n", debugstr_guid(MapSL(rclsid)));
            return REGDB_E_CLASSNOTREG;
        }

        dll = LoadLibrary16(dllpath);
        if (!dll)
        {
            ERR("couldn't load in-process dll %s\n", debugstr_a(dllpath));
            return E_ACCESSDENIED; /* FIXME: or should this be CO_E_DLLNOTFOUND? */
        }

        DllGetClassObject = GetProcAddress16(dll, "DllGetClassObject");
        if (!DllGetClassObject)
        {
            ERR("couldn't find function DllGetClassObject in %s\n", debugstr_a(dllpath));
            FreeLibrary16(dll);
            return CO_E_DLLNOTFOUND;
        }

        TRACE("calling DllGetClassObject %p\n", DllGetClassObject);
        args[5] = SELECTOROF(rclsid);
        args[4] = OFFSETOF(rclsid);
        args[3] = SELECTOROF(riid);
        args[2] = OFFSETOF(riid);
        args[1] = SELECTOROF(ppv);
        args[0] = OFFSETOF(ppv);
        WOWCallback16Ex((DWORD) DllGetClassObject, WCB16_PASCAL, sizeof(args), args, &dwRet);
        if (dwRet != S_OK)
        {
            ERR("DllGetClassObject returned error 0x%08lx\n", dwRet);
            FreeLibrary16(dll);
            return dwRet;
        }

        return S_OK;
    }

    FIXME("semi-stub\n");
    return E_NOTIMPL;
}

/******************************************************************************
 *		CoCreateGuid [COMPOBJ.73]
 */
HRESULT WINAPI CoCreateGuid16(GUID *pguid)
{
    return CoCreateGuid( pguid );
}

/***********************************************************************
 *           CoCreateInstance [COMPOBJ.13]
 */
HRESULT WINAPI CoCreateInstance16(
	REFCLSID rclsid,
	LPUNKNOWN pUnkOuter,
	DWORD dwClsContext,
	REFIID iid,
	LPVOID *ppv)
{
  FIXME("(%s, %p, %lx, %s, %p), stub!\n",
	debugstr_guid(rclsid), pUnkOuter, dwClsContext, debugstr_guid(iid),
	ppv
  );
  return E_NOTIMPL;
}

/***********************************************************************
 *           CoDisconnectObject [COMPOBJ.15]
 */
HRESULT WINAPI CoDisconnectObject16( LPUNKNOWN lpUnk, DWORD reserved )
{
  FIXME("(%p, 0x%08lx): stub!\n", lpUnk, reserved);
  return E_NOTIMPL;
}
