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


/* can I use offsetof safely? - FIXME? */
static QUARTZ_IFEntry IFEntries[] =
{
  { &IID_IReferenceClock, offsetof(CSystemClock,refclk)-offsetof(CSystemClock,unk) },
};


static void QUARTZ_DestroySystemClock(IUnknown* punk)
{
	CSystemClock_THIS(punk,unk);

	CSystemClock_UninitIReferenceClock( This );
}

HRESULT QUARTZ_CreateSystemClock(IUnknown* punkOuter,void** ppobj)
{
	CSystemClock*	psc;
	HRESULT	hr;

	TRACE("(%p,%p)\n",punkOuter,ppobj);

	psc = (CSystemClock*)QUARTZ_AllocObj( sizeof(CSystemClock) );
	if ( psc == NULL )
		return E_OUTOFMEMORY;

	QUARTZ_IUnkInit( &psc->unk, punkOuter );
	hr = CSystemClock_InitIReferenceClock( psc );
	if ( FAILED(hr) )
	{
		QUARTZ_FreeObj( psc );
		return hr;
	}

	psc->unk.pEntries = IFEntries;
	psc->unk.dwEntries = sizeof(IFEntries)/sizeof(IFEntries[0]);
	psc->unk.pOnFinalRelease = QUARTZ_DestroySystemClock;

	*ppobj = (void*)(&psc->unk);

	return S_OK;
}
