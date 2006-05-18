/*
 * Implementation of Active Template Library (atl.dll)
 *
 * Copyright 2004 Aric Stewart for CodeWeavers
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
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winuser.h"
#include "wine/debug.h"
#include "objbase.h"
#include "objidl.h"
#include "ole2.h"
#include "atlbase.h"
#include "atliface.h"

WINE_DEFAULT_DEBUG_CHANNEL(atl);

HINSTANCE hInst;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %ld, %p)\n",hinstDLL,fdwReason,lpvReserved);

    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
        hInst = hinstDLL;
    }
    return TRUE;
}

#define ATLVer1Size 100

HRESULT WINAPI AtlModuleInit(_ATL_MODULEA* pM, _ATL_OBJMAP_ENTRYA* p, HINSTANCE h)
{
    INT i;
    UINT size;

    FIXME("SEMI-STUB (%p %p %p)\n",pM,p,h);

    size = pM->cbSize;
    if  (size != sizeof(_ATL_MODULEA) && size != ATLVer1Size)
    {
        FIXME("Unknown structure version (size %i)\n",size);
        return E_INVALIDARG;
    }
    
    memset(pM,0,pM->cbSize);
    pM->cbSize = size;
    pM->m_hInst = h;
    pM->m_hInstResource = h;
    pM->m_hInstTypeLib = h;
    pM->m_pObjMap = p;
    pM->m_hHeap = GetProcessHeap();

    InitializeCriticalSection(&pM->u.m_csTypeInfoHolder);
    InitializeCriticalSection(&pM->m_csWindowCreate);
    InitializeCriticalSection(&pM->m_csObjMap);

    /* call mains */
    i = 0;
    if (pM->m_pObjMap != NULL  && size > ATLVer1Size)
    {
        while (pM->m_pObjMap[i].pclsid != NULL)
        {
            TRACE("Initializing object %i %p\n",i,p[i].pfnObjectMain);
            if (p[i].pfnObjectMain)
                p[i].pfnObjectMain(TRUE);
            i++;
        }
    }

    return S_OK;
}

HRESULT WINAPI AtlModuleTerm(_ATL_MODULEA* pM)
{
    _ATL_TERMFUNC_ELEM *iter = pM->m_pTermFuncs, *tmp;

    TRACE("(%p)\n", pM);

    while(iter) {
        iter->pFunc(iter->dw);
        tmp = iter;
        iter = iter->pNext;
        HeapFree(GetProcessHeap(), 0, tmp);
    }

    HeapFree(GetProcessHeap(), 0, pM);

    return S_OK;
}

HRESULT WINAPI AtlModuleAddTermFunc(_ATL_MODULEW *pM, _ATL_TERMFUNC *pFunc, DWORD_PTR dw)
{
    _ATL_TERMFUNC_ELEM *termfunc_elem;

    TRACE("(%p %p %ld)\n", pM, pFunc, dw);

    termfunc_elem = HeapAlloc(GetProcessHeap(), 0, sizeof(_ATL_TERMFUNC_ELEM));
    termfunc_elem->pFunc = pFunc;
    termfunc_elem->dw = dw;
    termfunc_elem->pNext = pM->m_pTermFuncs;

    pM->m_pTermFuncs = termfunc_elem;

    return S_OK;
}

HRESULT WINAPI AtlModuleRegisterClassObjects(_ATL_MODULEA *pM, DWORD dwClsContext,
                                             DWORD dwFlags)
{
    HRESULT hRes = S_OK;
    int i=0;

    TRACE("(%p %li %li)\n",pM, dwClsContext, dwFlags);

    if (pM == NULL)
        return E_INVALIDARG;

    while(pM->m_pObjMap[i].pclsid != NULL)
    {
        IUnknown* pUnknown;
        _ATL_OBJMAP_ENTRYA *obj = &(pM->m_pObjMap[i]);
        HRESULT rc;

        TRACE("Registering object %i\n",i);
        if (obj->pfnGetClassObject)
        {
            rc = obj->pfnGetClassObject(obj->pfnCreateInstance, &IID_IUnknown,
                                   (LPVOID*)&pUnknown);
            if (SUCCEEDED (rc) )
            {
                CoRegisterClassObject(obj->pclsid, pUnknown, dwClsContext,
                                      dwFlags, &obj->dwRegister);
                if (pUnknown)
                    IUnknown_Release(pUnknown);
            }
        }
        i++;
    }

   return hRes;
}

HRESULT WINAPI AtlModuleUnregisterServerEx(_ATL_MODULEA* pM, BOOL bUnRegTypeLib, const CLSID* pCLSID)
{
    FIXME("(%p, %i, %p) stub\n", pM, bUnRegTypeLib, pCLSID);
    return S_OK;
}

/***********************************************************************
 *           AtlAxWinInit          [ATL.@]
 * Initializes the control-hosting code: registering the AtlAxWin7 and AtlAxWinLic7 window
 * classes and some messages.
 *  
 * RETURNS
 *  TRUE or FALSE
 */

BOOL WINAPI AtlAxWinInit(void)
{
    FIXME("Try use native atl.dll if possible\n");
    return FALSE;
}


IUnknown* WINAPI AtlComPtrAssign(IUnknown** pp, IUnknown *p)
{
    TRACE("(%p %p)\n", pp, p);

    if (p) IUnknown_AddRef(p);
    if (*pp) IUnknown_Release(*pp);
    *pp = p;
    return p;
}


HRESULT WINAPI AtlInternalQueryInterface(LPVOID this, const _ATL_INTMAP_ENTRY* pEntries,  REFIID iid, LPVOID* ppvObject)
{
    int i = 0;
    HRESULT rc = E_NOINTERFACE;
    TRACE("(%p, %p, %p, %p)\n",this, pEntries, iid, ppvObject);

    if (IsEqualGUID(iid,&IID_IUnknown))
    {
        TRACE("Returning IUnknown\n");
        *ppvObject = this;
        IUnknown_AddRef((IUnknown*)this);
        return S_OK;
    }

    while (pEntries[i].pFunc != 0)
    {
        TRACE("Trying entry %i (%p %li %p)\n",i,pEntries[i].piid,
              pEntries[i].dw, pEntries[i].pFunc);

        if (pEntries[i].piid && IsEqualGUID(iid,pEntries[i].piid))
        {
            TRACE("MATCH\n");
            if (pEntries[i].pFunc == (_ATL_CREATORARGFUNC*)1)
            {
                TRACE("Offset\n");
                *ppvObject = ((LPSTR)this+pEntries[i].dw);
                IUnknown_AddRef((IUnknown*)this);
                rc = S_OK;
            }
            else
            {
                TRACE("Function\n");
                rc = pEntries[i].pFunc(this, iid, ppvObject,0);
            }
            break;
        }
        i++;
    }
    TRACE("Done returning (0x%lx)\n",rc);
    return rc;
}

/***********************************************************************
 *           AtlModuleRegisterServer         [ATL.@]
 *
 */
HRESULT WINAPI AtlModuleRegisterServer(_ATL_MODULEW* pM, BOOL bRegTypeLib, const CLSID* clsid) 
{
    FIXME("%p %d %s\n", pM, bRegTypeLib, debugstr_guid(clsid));
    return S_OK;
}

/***********************************************************************
 *           AtlAdvise         [ATL.@]
 */
HRESULT WINAPI AtlAdvise(IUnknown *pUnkCP, IUnknown *pUnk, const IID *iid, LPDWORD pdw)
{
    FIXME("%p %p %p %p\n", pUnkCP, pUnk, iid, pdw);
    return E_FAIL;
}

/***********************************************************************
 *           AtlUnadvise         [ATL.@]
 */
HRESULT WINAPI AtlUnadvise(IUnknown *pUnkCP, const IID *iid, DWORD dw)
{
    FIXME("%p %p %ld\n", pUnkCP, iid, dw);
    return S_OK;
}

/***********************************************************************
 *           AtlFreeMarshalStream         [ATL.@]
 */
HRESULT WINAPI AtlFreeMarshalStream(IStream *stm)
{
    FIXME("%p\n", stm);
    return S_OK;
}

/***********************************************************************
 *           AtlMarshalPtrInProc         [ATL.@]
 */
HRESULT WINAPI AtlMarshalPtrInProc(IUnknown *pUnk, const IID *iid, IStream **pstm)
{
    FIXME("%p %p %p\n", pUnk, iid, pstm);
    return E_FAIL;
}

/***********************************************************************
 *           AtlUnmarshalPtr              [ATL.@]
 */
HRESULT WINAPI AtlUnmarshalPtr(IStream *stm, const IID *iid, IUnknown **ppUnk)
{
    FIXME("%p %p %p\n", stm, iid, ppUnk);
    return E_FAIL;
}

/***********************************************************************
 *           AtlModuleGetClassObject              [ATL.@]
 */
HRESULT WINAPI AtlModuleGetClassObject(_ATL_MODULEW *pm, REFCLSID rclsid,
                                       REFIID riid, LPVOID *ppv)
{
    FIXME("%p %p %p %p\n", pm, rclsid, riid, ppv);
    return E_FAIL;
}

/***********************************************************************
 *           AtlModuleGetClassObject              [ATL.@]
 */
HRESULT WINAPI AtlModuleRegisterTypeLib(_ATL_MODULEW *pm, LPCOLESTR lpszIndex)
{
    FIXME("%p %s\n", pm, debugstr_w(lpszIndex));
    return E_FAIL;
}

/***********************************************************************
 *           AtlModuleRevokeClassObjects          [ATL.@]
 */
HRESULT WINAPI AtlModuleRevokeClassObjects(_ATL_MODULEW *pm)
{
    FIXME("%p\n", pm);
    return E_FAIL;
}

/***********************************************************************
 *           AtlModuleUnregisterServer           [ATL.@]
 */
HRESULT WINAPI AtlModuleUnregisterServer(_ATL_MODULEW *pm, const CLSID *clsid)
{
    FIXME("%p %s\n", pm, debugstr_guid(clsid));
    return E_FAIL;
}

/***********************************************************************
 *           AtlAxCreateControl           [ATL.@]
 */
HRESULT WINAPI AtlAxCreateControl(LPCOLESTR lpszName, HWND hWnd,
        IStream *pStream, IUnknown **ppUnkContainer)
{
    FIXME("%s %p %p %p)\n", debugstr_w(lpszName), hWnd, pStream, ppUnkContainer);
    return E_NOTIMPL;
}
