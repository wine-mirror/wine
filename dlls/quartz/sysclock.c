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

HRESULT QUARTZ_CreateSystemClock(IUnknown* punkOuter,void** ppobj)
{
	CSystemClock*	psc;

	TRACE("(%p,%p)\n",punkOuter,ppobj);

	psc = (CSystemClock*)QUARTZ_AllocObj( sizeof(CSystemClock) );
	if ( psc == NULL )
		return E_OUTOFMEMORY;

	QUARTZ_IUnkInit( &psc->unk );
	CSystemClock_InitIReferenceClock( psc );

	psc->unk.pEntries = IFEntries;
	psc->unk.dwEntries = sizeof(IFEntries)/sizeof(IFEntries[0]);

	*ppobj = (void*)psc;

	return S_OK;
}
