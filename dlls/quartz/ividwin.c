/*
 * Implementation of IVideoWindow for FilterGraph.
 *
 * FIXME - stub.
 *
 * hidenori@a2.ctktv.ne.jp
 */

#include "config.h"

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winerror.h"
#include "wine/obj_base.h"
#include "wine/obj_oleaut.h"
#include "strmif.h"
#include "control.h"
#include "uuids.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "fgraph.h"



static HRESULT WINAPI
IVideoWindow_fnQueryInterface(IVideoWindow* iface,REFIID riid,void** ppobj)
{
	CFilterGraph_THIS(iface,vidwin);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IVideoWindow_fnAddRef(IVideoWindow* iface)
{
	CFilterGraph_THIS(iface,vidwin);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IVideoWindow_fnRelease(IVideoWindow* iface)
{
	CFilterGraph_THIS(iface,vidwin);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}

static HRESULT WINAPI
IVideoWindow_fnGetTypeInfoCount(IVideoWindow* iface,UINT* pcTypeInfo)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnGetTypeInfo(IVideoWindow* iface,UINT iTypeInfo, LCID lcid, ITypeInfo** ppobj)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnGetIDsOfNames(IVideoWindow* iface,REFIID riid, LPOLESTR* ppwszName, UINT cNames, LCID lcid, DISPID* pDispId)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnInvoke(IVideoWindow* iface,DISPID DispId, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarRes, EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}



static HRESULT WINAPI
IVideoWindow_fnput_Caption(IVideoWindow* iface,BSTR strCaption)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnget_Caption(IVideoWindow* iface,BSTR* pstrCaption)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnput_WindowStyle(IVideoWindow* iface,long lStyle)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnget_WindowStyle(IVideoWindow* iface,long* plStyle)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnput_WindowStyleEx(IVideoWindow* iface,long lExStyle)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnget_WindowStyleEx(IVideoWindow* iface,long* plExStyle)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnput_AutoShow(IVideoWindow* iface,long lAutoShow)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnget_AutoShow(IVideoWindow* iface,long* plAutoShow)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnput_WindowState(IVideoWindow* iface,long lState)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnget_WindowState(IVideoWindow* iface,long* plState)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnput_BackgroundPalette(IVideoWindow* iface,long lBackPal)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnget_BackgroundPalette(IVideoWindow* iface,long* plBackPal)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnput_Visible(IVideoWindow* iface,long lVisible)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnget_Visible(IVideoWindow* iface,long* plVisible)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnput_Left(IVideoWindow* iface,long lLeft)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnget_Left(IVideoWindow* iface,long* plLeft)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnput_Width(IVideoWindow* iface,long lWidth)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnget_Width(IVideoWindow* iface,long* plWidth)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnput_Top(IVideoWindow* iface,long lTop)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnget_Top(IVideoWindow* iface,long* plTop)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnput_Height(IVideoWindow* iface,long lHeight)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnget_Height(IVideoWindow* iface,long* plHeight)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnput_Owner(IVideoWindow* iface,OAHWND hwnd)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnget_Owner(IVideoWindow* iface,OAHWND* phwnd)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnput_MessageDrain(IVideoWindow* iface,OAHWND hwnd)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnget_MessageDrain(IVideoWindow* iface,OAHWND* phwnd)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnget_BorderColor(IVideoWindow* iface,long* plColor)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnput_BorderColor(IVideoWindow* iface,long lColor)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnget_FullScreenMode(IVideoWindow* iface,long* plMode)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnput_FullScreenMode(IVideoWindow* iface,long lMode)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnSetWindowForeground(IVideoWindow* iface,long lFocus)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnNotifyOwnerMessage(IVideoWindow* iface,OAHWND hwnd,long message,LONG_PTR wParam,LONG_PTR lParam)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnSetWindowPosition(IVideoWindow* iface,long lLeft,long lTop,long lWidth,long lHeight)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnGetWindowPosition(IVideoWindow* iface,long* plLeft,long* plTop,long* plWidth,long* plHeight)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnGetMinIdealImageSize(IVideoWindow* iface,long* plWidth,long* plHeight)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnGetMaxIdealImageSize(IVideoWindow* iface,long* plWidth,long* plHeight)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnGetRestorePosition(IVideoWindow* iface,long* plLeft,long* plTop,long* plWidth,long* plHeight)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnHideCursor(IVideoWindow* iface,long lHide)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnIsCursorHidden(IVideoWindow* iface,long* plHide)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}




static ICOM_VTABLE(IVideoWindow) ivideowindow =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IVideoWindow_fnQueryInterface,
	IVideoWindow_fnAddRef,
	IVideoWindow_fnRelease,
	/* IDispatch fields */
	IVideoWindow_fnGetTypeInfoCount,
	IVideoWindow_fnGetTypeInfo,
	IVideoWindow_fnGetIDsOfNames,
	IVideoWindow_fnInvoke,
	/* IVideoWindow fields */
	IVideoWindow_fnput_Caption,
	IVideoWindow_fnget_Caption,
	IVideoWindow_fnput_WindowStyle,
	IVideoWindow_fnget_WindowStyle,
	IVideoWindow_fnput_WindowStyleEx,
	IVideoWindow_fnget_WindowStyleEx,
	IVideoWindow_fnput_AutoShow,
	IVideoWindow_fnget_AutoShow,
	IVideoWindow_fnput_WindowState,
	IVideoWindow_fnget_WindowState,
	IVideoWindow_fnput_BackgroundPalette,
	IVideoWindow_fnget_BackgroundPalette,
	IVideoWindow_fnput_Visible,
	IVideoWindow_fnget_Visible,
	IVideoWindow_fnput_Left,
	IVideoWindow_fnget_Left,
	IVideoWindow_fnput_Width,
	IVideoWindow_fnget_Width,
	IVideoWindow_fnput_Top,
	IVideoWindow_fnget_Top,
	IVideoWindow_fnput_Height,
	IVideoWindow_fnget_Height,
	IVideoWindow_fnput_Owner,
	IVideoWindow_fnget_Owner,
	IVideoWindow_fnput_MessageDrain,
	IVideoWindow_fnget_MessageDrain,
	IVideoWindow_fnget_BorderColor,
	IVideoWindow_fnput_BorderColor,
	IVideoWindow_fnget_FullScreenMode,
	IVideoWindow_fnput_FullScreenMode,
	IVideoWindow_fnSetWindowForeground,
	IVideoWindow_fnNotifyOwnerMessage,
	IVideoWindow_fnSetWindowPosition,
	IVideoWindow_fnGetWindowPosition,
	IVideoWindow_fnGetMinIdealImageSize,
	IVideoWindow_fnGetMaxIdealImageSize,
	IVideoWindow_fnGetRestorePosition,
	IVideoWindow_fnHideCursor,
	IVideoWindow_fnIsCursorHidden,

};


HRESULT CFilterGraph_InitIVideoWindow( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
	ICOM_VTBL(&pfg->vidwin) = &ivideowindow;

	return NOERROR;
}

void CFilterGraph_UninitIVideoWindow( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
}

