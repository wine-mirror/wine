/*
 * Implementation of CLSID_FilterMapper2.
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
#include "fmap2.h"

/* can I use offsetof safely? - FIXME? */
static QUARTZ_IFEntry IFEntries[] =
{
  { &IID_IFilterMapper2, offsetof(CFilterMapper2,fmap3)-offsetof(CFilterMapper2,unk) },
  { &IID_IFilterMapper3, offsetof(CFilterMapper2,fmap3)-offsetof(CFilterMapper2,unk) },
};


static void QUARTZ_DestroyFilterMapper2(IUnknown* punk)
{
	CFilterMapper2_THIS(punk,unk);

	CFilterMapper2_UninitIFilterMapper3( This );
}

HRESULT QUARTZ_CreateFilterMapper2(IUnknown* punkOuter,void** ppobj)
{
	CFilterMapper2*	pfm;
	HRESULT	hr;

	TRACE("(%p,%p)\n",punkOuter,ppobj);

	pfm = (CFilterMapper2*)QUARTZ_AllocObj( sizeof(CFilterMapper2) );
	if ( pfm == NULL )
		return E_OUTOFMEMORY;

	QUARTZ_IUnkInit( &pfm->unk, punkOuter );
	hr = CFilterMapper2_InitIFilterMapper3( pfm );
	if ( FAILED(hr) )
	{
		QUARTZ_FreeObj( pfm );
		return hr;
	}

	pfm->unk.pEntries = IFEntries;
	pfm->unk.dwEntries = sizeof(IFEntries)/sizeof(IFEntries[0]);
	pfm->unk.pOnFinalRelease = QUARTZ_DestroyFilterMapper2;

	*ppobj = (void*)(&pfm->unk);

	return S_OK;
}
