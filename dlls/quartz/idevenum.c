/*
 * Implementation of ICreateDevEnum for CLSID_SystemDeviceEnum.
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
#include "devenum.h"

static HRESULT WINAPI
ICreateDevEnum_fnQueryInterface(ICreateDevEnum* iface,REFIID riid,void** ppobj)
{
	CSysDevEnum_THIS(iface,createdevenum);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
ICreateDevEnum_fnAddRef(ICreateDevEnum* iface)
{
	CSysDevEnum_THIS(iface,createdevenum);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
ICreateDevEnum_fnRelease(ICreateDevEnum* iface)
{
	CSysDevEnum_THIS(iface,createdevenum);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}

static HRESULT WINAPI
ICreateDevEnum_fnCreateClassEnumerator(ICreateDevEnum* iface,REFCLSID rclsidDeviceClass,IEnumMoniker** ppobj, DWORD dwFlags)
{
	CSysDevEnum_THIS(iface,createdevenum);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static ICOM_VTABLE(ICreateDevEnum) icreatedevenum =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	ICreateDevEnum_fnQueryInterface,
	ICreateDevEnum_fnAddRef,
	ICreateDevEnum_fnRelease,
	/* ICreateDevEnum fields */
	ICreateDevEnum_fnCreateClassEnumerator,
};

void CSysDevEnum_InitICreateDevEnum( CSysDevEnum* psde )
{
	TRACE("(%p)\n",psde);
	ICOM_VTBL(&psde->createdevenum) = &icreatedevenum;
}

void CSysDevEnum_UninitICreateDevEnum( CSysDevEnum* psde )
{
	TRACE("(%p)\n",psde);
}
