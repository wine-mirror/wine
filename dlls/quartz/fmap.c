/*
 * Implementation of CLSID_FilterMapper.
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
#include "fmap.h"

/* can I use offsetof safely? - FIXME? */
static QUARTZ_IFEntry IFEntries[] =
{
  { &IID_IFilterMapper, offsetof(CFilterMapper,fmap)-offsetof(CFilterMapper,unk) },
};


static void QUARTZ_DestroyFilterMapper(IUnknown* punk)
{
	CFilterMapper_THIS(punk,unk);

	CFilterMapper_UninitIFilterMapper( This );
}

HRESULT QUARTZ_CreateFilterMapper(IUnknown* punkOuter,void** ppobj)
{
	CFilterMapper*	pfm;
	HRESULT	hr;

	TRACE("(%p,%p)\n",punkOuter,ppobj);

	pfm = (CFilterMapper*)QUARTZ_AllocObj( sizeof(CFilterMapper) );
	if ( pfm == NULL )
		return E_OUTOFMEMORY;

	QUARTZ_IUnkInit( &pfm->unk, punkOuter );
	hr = CFilterMapper_InitIFilterMapper( pfm );
	if ( FAILED(hr) )
	{
		QUARTZ_FreeObj( pfm );
		return hr;
	}

	pfm->unk.pEntries = IFEntries;
	pfm->unk.dwEntries = sizeof(IFEntries)/sizeof(IFEntries[0]);
	pfm->unk.pOnFinalRelease = QUARTZ_DestroyFilterMapper;

	*ppobj = (void*)(&pfm->unk);

	return S_OK;
}
