/*
 * Implementation of miscellaneous interfaces for IE Web Browser control:
 *
 *  - IQuickActivate
 *
 * 2001 John R. Sheets (for CodeWeavers)
 */

#include "debugtools.h"
#include "shdocvw.h"

DEFAULT_DEBUG_CHANNEL(shdocvw);

/**********************************************************************
 * Implement the IQuickActivate interface
 */

static HRESULT WINAPI WBQA_QueryInterface(LPQUICKACTIVATE iface,
                                          REFIID riid, LPVOID *ppobj)
{
    ICOM_THIS(IQuickActivateImpl, iface);

    FIXME("(%p)->(%s,%p),stub!\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI WBQA_AddRef(LPQUICKACTIVATE iface)
{
    ICOM_THIS(IQuickActivateImpl, iface);

    TRACE("\n");
    return ++(This->ref);
}

static ULONG WINAPI WBQA_Release(LPQUICKACTIVATE iface)
{
    ICOM_THIS(IQuickActivateImpl, iface);

    /* static class, won't be freed */
    TRACE("\n");
    return --(This->ref);
}

/* Alternative interface for quicker, easier activation of a control. */
static HRESULT WINAPI WBQA_QuickActivate(LPQUICKACTIVATE iface,
                                         QACONTAINER *pQaContainer,
                                         QACONTROL *pQaControl)
{
    FIXME("stub: QACONTAINER = %p, QACONTROL = %p\n", pQaContainer, pQaControl);
    return S_OK;
}

static HRESULT WINAPI WBQA_SetContentExtent(LPQUICKACTIVATE iface, LPSIZEL pSizel)
{
    FIXME("stub: LPSIZEL = %p\n", pSizel);
    return E_NOINTERFACE;
}

static HRESULT WINAPI WBQA_GetContentExtent(LPQUICKACTIVATE iface, LPSIZEL pSizel)
{
    FIXME("stub: LPSIZEL = %p\n", pSizel);
    return E_NOINTERFACE;
}

/**********************************************************************
 * IQuickActivate virtual function table for IE Web Browser component
 */

static ICOM_VTABLE(IQuickActivate) WBQA_Vtbl = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    WBQA_QueryInterface,
    WBQA_AddRef,
    WBQA_Release,
    WBQA_QuickActivate,
    WBQA_SetContentExtent,
    WBQA_GetContentExtent
};

IQuickActivateImpl SHDOCVW_QuickActivate = { &WBQA_Vtbl, 1 };
