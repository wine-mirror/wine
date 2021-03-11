/*
 * RichEdit - functions and interfaces around CreateTextServices
 *
 * Copyright 2005, 2006, Maarten Lankhorst
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

#include "editor.h"
#include "ole2.h"
#include "oleauto.h"
#include "richole.h"
#include "tom.h"
#include "imm.h"
#include "textserv.h"
#include "wine/debug.h"
#include "editstr.h"

WINE_DEFAULT_DEBUG_CHANNEL(richedit);

struct text_services
{
    IUnknown IUnknown_inner;
    ITextServices ITextServices_iface;
    IUnknown *outer_unk;
    LONG ref;
    ITextHost *host;
    ME_TextEditor *editor;
    char spare[256]; /* for bug #12179 */
};

static inline struct text_services *impl_from_IUnknown( IUnknown *iface )
{
    return CONTAINING_RECORD( iface, struct text_services, IUnknown_inner );
}

static HRESULT WINAPI ITextServicesImpl_QueryInterface( IUnknown *iface, REFIID iid, void **obj )
{
    struct text_services *services = impl_from_IUnknown( iface );

    TRACE( "(%p)->(%s, %p)\n", iface, debugstr_guid( iid ), obj );

    if (IsEqualIID( iid, &IID_IUnknown )) *obj = &services->IUnknown_inner;
    else if (IsEqualIID( iid, &IID_ITextServices )) *obj = &services->ITextServices_iface;
    else if (IsEqualIID( iid, &IID_IRichEditOle ) || IsEqualIID( iid, &IID_ITextDocument ) ||
             IsEqualIID( iid, &IID_ITextDocument2Old ))
    {
        if (!services->editor->reOle && !CreateIRichEditOle( services->outer_unk, services->editor, (void **)&services->editor->reOle ))
            return E_OUTOFMEMORY;
        return IUnknown_QueryInterface( services->editor->reOle, iid, obj );
    }
    else
    {
        *obj = NULL;
        FIXME( "Unknown interface: %s\n", debugstr_guid( iid ) );
        return E_NOINTERFACE;
    }

    IUnknown_AddRef( (IUnknown *)*obj );
    return S_OK;
}

static ULONG WINAPI ITextServicesImpl_AddRef(IUnknown *iface)
{
    struct text_services *services = impl_from_IUnknown( iface );
    LONG ref = InterlockedIncrement( &services->ref );

    TRACE( "(%p) ref = %d\n", services, ref );

    return ref;
}

static ULONG WINAPI ITextServicesImpl_Release(IUnknown *iface)
{
    struct text_services *services = impl_from_IUnknown( iface );
    LONG ref = InterlockedDecrement( &services->ref );

    TRACE( "(%p) ref = %d\n", services, ref );

    if (!ref)
    {
        ME_DestroyEditor( services->editor );
        CoTaskMemFree( services );
    }
    return ref;
}

static const IUnknownVtbl textservices_inner_vtbl =
{
   ITextServicesImpl_QueryInterface,
   ITextServicesImpl_AddRef,
   ITextServicesImpl_Release
};

static inline struct text_services *impl_from_ITextServices( ITextServices *iface )
{
    return CONTAINING_RECORD( iface, struct text_services, ITextServices_iface );
}

static HRESULT WINAPI fnTextSrv_QueryInterface( ITextServices *iface, REFIID iid, void **obj )
{
    struct text_services *services = impl_from_ITextServices( iface );
    return IUnknown_QueryInterface( services->outer_unk, iid, obj );
}

static ULONG WINAPI fnTextSrv_AddRef(ITextServices *iface)
{
    struct text_services *services = impl_from_ITextServices( iface );
    return IUnknown_AddRef( services->outer_unk );
}

static ULONG WINAPI fnTextSrv_Release(ITextServices *iface)
{
    struct text_services *services = impl_from_ITextServices( iface );
    return IUnknown_Release( services->outer_unk );
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_TxSendMessage,20)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxSendMessage(ITextServices *iface, UINT msg, WPARAM wparam,
                                                           LPARAM lparam, LRESULT *plresult)
{
   struct text_services *services = impl_from_ITextServices( iface );
   HRESULT hresult;
   LRESULT lresult;

   lresult = ME_HandleMessage( services->editor, msg, wparam, lparam, TRUE, &hresult );
   if (plresult) *plresult = lresult;
   return hresult;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_TxDraw,52)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxDraw(ITextServices *iface, DWORD dwDrawAspect, LONG lindex,
                                                    void *pvAspect, DVTARGETDEVICE *ptd, HDC hdcDraw, HDC hdcTargetDev,
                                                    LPCRECTL lprcBounds, LPCRECTL lprcWBounds, LPRECT lprcUpdate,
                                                    BOOL (CALLBACK * pfnContinue)(DWORD), DWORD dwContinue,
                                                    LONG lViewId)
{
    struct text_services *services = impl_from_ITextServices( iface );

    FIXME( "%p: STUB\n", services );
    return E_NOTIMPL;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetHScroll,24)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxGetHScroll( ITextServices *iface, LONG *min_pos, LONG *max_pos, LONG *pos,
                                                           LONG *page, BOOL *enabled )
{
    struct text_services *services = impl_from_ITextServices( iface );

    if (min_pos) *min_pos = services->editor->horz_si.nMin;
    if (max_pos) *max_pos = services->editor->horz_si.nMax;
    if (pos) *pos = services->editor->horz_si.nPos;
    if (page) *page = services->editor->horz_si.nPage;
    if (enabled) *enabled = (services->editor->styleFlags & WS_HSCROLL) != 0;
    return S_OK;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetVScroll,24)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxGetVScroll( ITextServices *iface, LONG *min_pos, LONG *max_pos, LONG *pos,
                                                           LONG *page, BOOL *enabled )
{
    struct text_services *services = impl_from_ITextServices( iface );

    if (min_pos) *min_pos = services->editor->vert_si.nMin;
    if (max_pos) *max_pos = services->editor->vert_si.nMax;
    if (pos) *pos = services->editor->vert_si.nPos;
    if (page) *page = services->editor->vert_si.nPage;
    if (enabled) *enabled = (services->editor->styleFlags & WS_VSCROLL) != 0;
    return S_OK;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_OnTxSetCursor,40)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_OnTxSetCursor(ITextServices *iface, DWORD dwDrawAspect, LONG lindex,
                                                           void *pvAspect, DVTARGETDEVICE *ptd, HDC hdcDraw,
                                                           HDC hicTargetDev, LPCRECT lprcClient, INT x, INT y)
{
    struct text_services *services = impl_from_ITextServices( iface );

    FIXME( "%p: STUB\n", services );
    return E_NOTIMPL;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_TxQueryHitPoint,44)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxQueryHitPoint(ITextServices *iface, DWORD dwDrawAspect, LONG lindex,
                                                             void *pvAspect, DVTARGETDEVICE *ptd, HDC hdcDraw,
                                                             HDC hicTargetDev, LPCRECT lprcClient, INT x, INT y,
                                                             DWORD *pHitResult)
{
    struct text_services *services = impl_from_ITextServices( iface );

    FIXME( "%p: STUB\n", services );
    return E_NOTIMPL;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_OnTxInplaceActivate,8)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_OnTxInplaceActivate(ITextServices *iface, LPCRECT prcClient)
{
    struct text_services *services = impl_from_ITextServices( iface );

    FIXME( "%p: STUB\n", services );
    return E_NOTIMPL;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_OnTxInplaceDeactivate,4)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_OnTxInplaceDeactivate(ITextServices *iface)
{
    struct text_services *services = impl_from_ITextServices( iface );

    FIXME( "%p: STUB\n", services );
    return E_NOTIMPL;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_OnTxUIActivate,4)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_OnTxUIActivate(ITextServices *iface)
{
    struct text_services *services = impl_from_ITextServices( iface );

    FIXME( "%p: STUB\n", services );
    return E_NOTIMPL;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_OnTxUIDeactivate,4)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_OnTxUIDeactivate(ITextServices *iface)
{
    struct text_services *services = impl_from_ITextServices( iface );

    FIXME( "%p: STUB\n", services );
    return E_NOTIMPL;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetText,8)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxGetText( ITextServices *iface, BSTR *text )
{
    struct text_services *services = impl_from_ITextServices( iface );
    int length;

    length = ME_GetTextLength( services->editor );
    if (length)
    {
        ME_Cursor start;
        BSTR bstr;
        bstr = SysAllocStringByteLen( NULL, length * sizeof(WCHAR) );
        if (bstr == NULL) return E_OUTOFMEMORY;

        cursor_from_char_ofs( services->editor, 0, &start );
        ME_GetTextW( services->editor, bstr, length, &start, INT_MAX, FALSE, FALSE );
        *text = bstr;
    }
    else *text = NULL;

    return S_OK;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_TxSetText,8)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxSetText( ITextServices *iface, const WCHAR *text )
{
    struct text_services *services = impl_from_ITextServices( iface );
    ME_Cursor cursor;

    ME_SetCursorToStart( services->editor, &cursor );
    ME_InternalDeleteText( services->editor, &cursor, ME_GetTextLength( services->editor ), FALSE );
    if (text) ME_InsertTextFromCursor( services->editor, 0, text, -1, services->editor->pBuffer->pDefaultStyle );
    set_selection_cursors( services->editor, 0, 0);
    services->editor->nModifyStep = 0;
    OleFlushClipboard();
    ME_EmptyUndoStack( services->editor );
    ME_UpdateRepaint( services->editor, FALSE );

    return S_OK;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetCurTargetX,8)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxGetCurTargetX(ITextServices *iface, LONG *x)
{
    struct text_services *services = impl_from_ITextServices( iface );

    FIXME( "%p: STUB\n", services );
    return E_NOTIMPL;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetBaseLinePos,8)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxGetBaseLinePos(ITextServices *iface, LONG *x)
{
    struct text_services *services = impl_from_ITextServices( iface );

    FIXME( "%p: STUB\n", services );
    return E_NOTIMPL;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetNaturalSize,36)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxGetNaturalSize(ITextServices *iface, DWORD dwAspect, HDC hdcDraw,
                                                              HDC hicTargetDev, DVTARGETDEVICE *ptd, DWORD dwMode,
                                                              const SIZEL *psizelExtent, LONG *pwidth, LONG *pheight)
{
    struct text_services *services = impl_from_ITextServices( iface );

    FIXME( "%p: STUB\n", services );
    return E_NOTIMPL;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetDropTarget,8)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxGetDropTarget(ITextServices *iface, IDropTarget **ppDropTarget)
{
    struct text_services *services = impl_from_ITextServices( iface );

    FIXME( "%p: STUB\n", services );
    return E_NOTIMPL;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_OnTxPropertyBitsChange,12)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_OnTxPropertyBitsChange(ITextServices *iface, DWORD dwMask, DWORD dwBits)
{
    struct text_services *services = impl_from_ITextServices( iface );

    FIXME( "%p: STUB\n", services );
    return E_NOTIMPL;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetCachedSize,12)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxGetCachedSize(ITextServices *iface, DWORD *pdwWidth, DWORD *pdwHeight)
{
    struct text_services *services = impl_from_ITextServices( iface );

    FIXME( "%p: STUB\n", services );
    return E_NOTIMPL;
}

#ifdef __ASM_USE_THISCALL_WRAPPER

#define STDCALL(func) (void *) __stdcall_ ## func
#ifdef _MSC_VER
#define DEFINE_STDCALL_WRAPPER(num,func) \
    __declspec(naked) HRESULT __stdcall_##func(void) \
    { \
        __asm pop eax \
        __asm pop ecx \
        __asm push eax \
        __asm mov eax, [ecx] \
        __asm jmp dword ptr [eax + 4*num] \
    }
#else /* _MSC_VER */
#define DEFINE_STDCALL_WRAPPER(num,func) \
   extern HRESULT __stdcall_ ## func(void); \
   __ASM_GLOBAL_FUNC(__stdcall_ ## func, \
                   "popl %eax\n\t" \
                   "popl %ecx\n\t" \
                   "pushl %eax\n\t" \
                   "movl (%ecx), %eax\n\t" \
                   "jmp *(4*(" #num "))(%eax)" )
#endif /* _MSC_VER */

DEFINE_STDCALL_WRAPPER(3, ITextServices_TxSendMessage)
DEFINE_STDCALL_WRAPPER(4, ITextServices_TxDraw)
DEFINE_STDCALL_WRAPPER(5, ITextServices_TxGetHScroll)
DEFINE_STDCALL_WRAPPER(6, ITextServices_TxGetVScroll)
DEFINE_STDCALL_WRAPPER(7, ITextServices_OnTxSetCursor)
DEFINE_STDCALL_WRAPPER(8, ITextServices_TxQueryHitPoint)
DEFINE_STDCALL_WRAPPER(9, ITextServices_OnTxInplaceActivate)
DEFINE_STDCALL_WRAPPER(10, ITextServices_OnTxInplaceDeactivate)
DEFINE_STDCALL_WRAPPER(11, ITextServices_OnTxUIActivate)
DEFINE_STDCALL_WRAPPER(12, ITextServices_OnTxUIDeactivate)
DEFINE_STDCALL_WRAPPER(13, ITextServices_TxGetText)
DEFINE_STDCALL_WRAPPER(14, ITextServices_TxSetText)
DEFINE_STDCALL_WRAPPER(15, ITextServices_TxGetCurTargetX)
DEFINE_STDCALL_WRAPPER(16, ITextServices_TxGetBaseLinePos)
DEFINE_STDCALL_WRAPPER(17, ITextServices_TxGetNaturalSize)
DEFINE_STDCALL_WRAPPER(18, ITextServices_TxGetDropTarget)
DEFINE_STDCALL_WRAPPER(19, ITextServices_OnTxPropertyBitsChange)
DEFINE_STDCALL_WRAPPER(20, ITextServices_TxGetCachedSize)

const ITextServicesVtbl text_services_stdcall_vtbl =
{
    NULL,
    NULL,
    NULL,
    STDCALL(ITextServices_TxSendMessage),
    STDCALL(ITextServices_TxDraw),
    STDCALL(ITextServices_TxGetHScroll),
    STDCALL(ITextServices_TxGetVScroll),
    STDCALL(ITextServices_OnTxSetCursor),
    STDCALL(ITextServices_TxQueryHitPoint),
    STDCALL(ITextServices_OnTxInplaceActivate),
    STDCALL(ITextServices_OnTxInplaceDeactivate),
    STDCALL(ITextServices_OnTxUIActivate),
    STDCALL(ITextServices_OnTxUIDeactivate),
    STDCALL(ITextServices_TxGetText),
    STDCALL(ITextServices_TxSetText),
    STDCALL(ITextServices_TxGetCurTargetX),
    STDCALL(ITextServices_TxGetBaseLinePos),
    STDCALL(ITextServices_TxGetNaturalSize),
    STDCALL(ITextServices_TxGetDropTarget),
    STDCALL(ITextServices_OnTxPropertyBitsChange),
    STDCALL(ITextServices_TxGetCachedSize),
};

#endif /* __ASM_USE_THISCALL_WRAPPER */

static const ITextServicesVtbl textservices_vtbl =
{
    fnTextSrv_QueryInterface,
    fnTextSrv_AddRef,
    fnTextSrv_Release,
    THISCALL(fnTextSrv_TxSendMessage),
    THISCALL(fnTextSrv_TxDraw),
    THISCALL(fnTextSrv_TxGetHScroll),
    THISCALL(fnTextSrv_TxGetVScroll),
    THISCALL(fnTextSrv_OnTxSetCursor),
    THISCALL(fnTextSrv_TxQueryHitPoint),
    THISCALL(fnTextSrv_OnTxInplaceActivate),
    THISCALL(fnTextSrv_OnTxInplaceDeactivate),
    THISCALL(fnTextSrv_OnTxUIActivate),
    THISCALL(fnTextSrv_OnTxUIDeactivate),
    THISCALL(fnTextSrv_TxGetText),
    THISCALL(fnTextSrv_TxSetText),
    THISCALL(fnTextSrv_TxGetCurTargetX),
    THISCALL(fnTextSrv_TxGetBaseLinePos),
    THISCALL(fnTextSrv_TxGetNaturalSize),
    THISCALL(fnTextSrv_TxGetDropTarget),
    THISCALL(fnTextSrv_OnTxPropertyBitsChange),
    THISCALL(fnTextSrv_TxGetCachedSize)
};

HRESULT create_text_services( IUnknown *outer, ITextHost *text_host, IUnknown **unk, BOOL emulate_10,
                              ME_TextEditor **editor )
{
    struct text_services *services;

    TRACE( "%p %p --> %p\n", outer, text_host, unk );
    if (text_host == NULL) return E_POINTER;

    services = CoTaskMemAlloc( sizeof(*services) );
    if (services == NULL) return E_OUTOFMEMORY;
    services->ref = 1;
    services->host = text_host; /* Don't take a ref of the host - this would lead to a mutual dependency */
    services->IUnknown_inner.lpVtbl = &textservices_inner_vtbl;
    services->ITextServices_iface.lpVtbl = &textservices_vtbl;
    services->editor = ME_MakeEditor( text_host, emulate_10 );
    if (editor) *editor = services->editor; /* To be removed */

    if (outer) services->outer_unk = outer;
    else services->outer_unk = &services->IUnknown_inner;

    *unk = &services->IUnknown_inner;
    return S_OK;
}

/******************************************************************
 *        CreateTextServices (RICHED20.4)
 */
HRESULT WINAPI CreateTextServices( IUnknown *outer, ITextHost *text_host, IUnknown **unk )
{
    return create_text_services( outer, text_host, unk, FALSE, NULL );
}
