/* Copyright (C) 2007 C John Klehm
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

#include "inkobj_internal.h"

WINE_DEFAULT_DEBUG_CHANNEL(inkobj);


typedef struct tagInkCollector
{
    IInkCollectorVtbl* lpVtbl;

    ULONG refCount;
} InkCollector;

static HRESULT WINAPI InkCollector_QueryInterface(
    IInkCollector* This, REFIID riid, void** ppvObject)
{
    FIXME("stub!\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI InkCollector_AddRef(
    IInkCollector* This)
{
    FIXME("stub!\n");
    return 0;
}

static ULONG WINAPI InkCollector_Release(
    IInkCollector* This)
{
    FIXME("stub!\n");
    return 0;
}

static HRESULT WINAPI InkCollector_GetTypeInfoCount(
    IInkCollector* This, UINT* pctinfo)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_GetTypeInfo(
    IInkCollector* This, UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_GetIDsOfNames(
    IInkCollector* This, REFIID riid, LPOLESTR* rgszNames, UINT cNames,
    LCID lcid, DISPID* rgDispId)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_Invoke(
    IInkCollector* This, DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_get_hWnd(
    IInkCollector* This, long* CurrentWindow)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_put_hWnd(
    IInkCollector* This, long CurrentWindow)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_get_Enabled(
    IInkCollector* This, VARIANT_BOOL* Collecting)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_put_Enabled(
    IInkCollector* This, VARIANT_BOOL Collecting)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_get_DefaultDrawingAttributes(
    IInkCollector* This, IInkDrawingAttributes** CurrentAttributes)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_putref_DefaultDrawingAttributes(
    IInkCollector* This, IInkDrawingAttributes* CurrentAttributes)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_get_Renderer(
    IInkCollector* This, IInkRenderer** CurrentInkRenderer)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_putref_Renderer(
    IInkCollector* This, IInkRenderer* CurrentInkRenderer)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_get_Ink(
    IInkCollector* This, IInkDisp** Ink)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_putref_Ink(
    IInkCollector* This, IInkDisp* Ink)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_get_AutoRedraw(
    IInkCollector* This, VARIANT_BOOL* AutoRedraw)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_put_AutoRedraw(
    IInkCollector* This, VARIANT_BOOL AutoRedraw)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_get_CollectingInk(
    IInkCollector* This, VARIANT_BOOL* Collecting)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_get_CollectionMode(
    IInkCollector* This, InkCollectionMode* Mode)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_put_CollectionMode(
    IInkCollector* This, InkCollectionMode Mode)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_get_DynamicRendering(
    IInkCollector* This, VARIANT_BOOL* Enabled)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_put_DynamicRendering(
    IInkCollector* This, VARIANT_BOOL Enabled)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_get_DesiredPacketDescription(
    IInkCollector* This, VARIANT* PacketGuids)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_put_DesiredPacketDescription(
    IInkCollector* This, VARIANT PacketGuids)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_get_MouseIcon(
    IInkCollector* This, IPictureDisp** MouseIcon)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_put_MouseIcon(
    IInkCollector* This, IPictureDisp* MouseIcon)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_putref_MouseIcon(
    IInkCollector* This, IPictureDisp* MouseIcon)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_get_MousePointer(
    IInkCollector* This, InkMousePointer* MousePointer)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_put_MousePointer(
    IInkCollector* This, InkMousePointer MousePointer)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_get_Cursors(
    IInkCollector* This, IInkCursors** Cursors)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_get_MarginX(
    IInkCollector* This, long* MarginX)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_put_MarginX(
    IInkCollector* This, long MarginX)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_get_MarginY(
    IInkCollector* This, long* MarginY)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_put_MarginY(
    IInkCollector* This, long MarginY)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_get_Tablet(
    IInkCollector* This, IInkTablet** SingleTablet)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_get_SupportHighContrastInk(
    IInkCollector* This, VARIANT_BOOL* Support)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_put_SupportHighContrastInk(
    IInkCollector* This, VARIANT_BOOL Support)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_SetGestureStatus(
    IInkCollector* This, InkApplicationGesture Gesture, VARIANT_BOOL Listen)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_GetGestureStatus(
    IInkCollector* This, InkApplicationGesture Gesture, VARIANT_BOOL* Listen)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_GetWindowInputRectangle(
    IInkCollector* This, IInkRectangle** WindowInputRectangle)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_SetWindowInputRectangle(
    IInkCollector* This, IInkRectangle* WindowInputRectangle)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_SetAllTabletsMode(
    IInkCollector* This, VARIANT_BOOL UseMouseForInput)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_SetSingleTabletIntegratedMode(
    IInkCollector* This, IInkTablet* Tablet)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_GetEventInterest(
    IInkCollector* This, InkCollectorEventInterest EventId,
    VARIANT_BOOL* Listen)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InkCollector_SetEventInterest(
    IInkCollector* This, InkCollectorEventInterest EventId,
    VARIANT_BOOL Listen)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static const IInkCollectorVtbl InkCollectorVtbl =
{
    InkCollector_QueryInterface,
    InkCollector_AddRef,
    InkCollector_Release,
    InkCollector_GetTypeInfoCount,
    InkCollector_GetTypeInfo,
    InkCollector_GetIDsOfNames,
    InkCollector_Invoke,
    InkCollector_get_hWnd,
    InkCollector_put_hWnd,
    InkCollector_get_Enabled,
    InkCollector_put_Enabled,
    InkCollector_get_DefaultDrawingAttributes,
    InkCollector_putref_DefaultDrawingAttributes,
    InkCollector_get_Renderer,
    InkCollector_putref_Renderer,
    InkCollector_get_Ink,
    InkCollector_putref_Ink,
    InkCollector_get_AutoRedraw,
    InkCollector_put_AutoRedraw,
    InkCollector_get_CollectingInk,
    InkCollector_get_CollectionMode,
    InkCollector_put_CollectionMode,
    InkCollector_get_DynamicRendering,
    InkCollector_put_DynamicRendering,
    InkCollector_get_DesiredPacketDescription,
    InkCollector_put_DesiredPacketDescription,
    InkCollector_get_MouseIcon,
    InkCollector_put_MouseIcon,
    InkCollector_putref_MouseIcon,
    InkCollector_get_MousePointer,
    InkCollector_put_MousePointer,
    InkCollector_get_Cursors,
    InkCollector_get_MarginX,
    InkCollector_put_MarginX,
    InkCollector_get_MarginY,
    InkCollector_put_MarginY,
    InkCollector_get_Tablet,
    InkCollector_get_SupportHighContrastInk,
    InkCollector_put_SupportHighContrastInk,
    InkCollector_SetGestureStatus,
    InkCollector_GetGestureStatus,
    InkCollector_GetWindowInputRectangle,
    InkCollector_SetWindowInputRectangle,
    InkCollector_SetAllTabletsMode,
    InkCollector_SetSingleTabletIntegratedMode,
    InkCollector_GetEventInterest,
    InkCollector_SetEventInterest
};
