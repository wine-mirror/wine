/*
 * Implementation of CLSID_SystemClock.
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
#include "strmif.h"
#include "uuids.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "sysclock.h"


static HRESULT WINAPI
IReferenceClock_fnQueryInterface(IReferenceClock* iface,REFIID riid,void** ppobj)
{
	CSystemClock_THIS(iface,refclk);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IReferenceClock_fnAddRef(IReferenceClock* iface)
{
	CSystemClock_THIS(iface,refclk);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IReferenceClock_fnRelease(IReferenceClock* iface)
{
	CSystemClock_THIS(iface,refclk);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}

static HRESULT WINAPI
IReferenceClock_fnGetTime(IReferenceClock* iface,REFERENCE_TIME* prtTime)
{
	CSystemClock_THIS(iface,refclk);

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IReferenceClock_fnAdviseTime(IReferenceClock* iface,REFERENCE_TIME rtBase,REFERENCE_TIME rtStream,HEVENT hEvent,DWORD_PTR* pdwAdvCookie)
{
	CSystemClock_THIS(iface,refclk);

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IReferenceClock_fnAdvisePeriodic(IReferenceClock* iface,REFERENCE_TIME rtStart,REFERENCE_TIME rtPeriod,HSEMAPHORE hSemaphore,DWORD_PTR* pdwAdvCookie)
{
	CSystemClock_THIS(iface,refclk);

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IReferenceClock_fnUnadvise(IReferenceClock* iface,DWORD_PTR dwAdvCookie)
{
	CSystemClock_THIS(iface,refclk);

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}

static ICOM_VTABLE(IReferenceClock) irefclk =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IReferenceClock_fnQueryInterface,
	IReferenceClock_fnAddRef,
	IReferenceClock_fnRelease,
	/* IReferenceClock fields */
	IReferenceClock_fnGetTime,
	IReferenceClock_fnAdviseTime,
	IReferenceClock_fnAdvisePeriodic,
	IReferenceClock_fnUnadvise,
};


void CSystemClock_InitIReferenceClock( CSystemClock* psc )
{
	TRACE("(%p)\n",psc);
	ICOM_VTBL(&psc->refclk) = &irefclk;
}
