/*
 * Implementation of CLSID_SystemDeviceEnum.
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

/* can I use offsetof safely? - FIXME? */
static QUARTZ_IFEntry IFEntries[] =
{
  { &IID_ICreateDevEnum, offsetof(CSysDevEnum,createdevenum)-offsetof(CSysDevEnum,unk) },
};


static void QUARTZ_DestroySystemDeviceEnum(IUnknown* punk)
{
	CSysDevEnum_THIS(punk,unk);

	CSysDevEnum_UninitICreateDevEnum( This );
}

HRESULT QUARTZ_CreateSystemDeviceEnum(IUnknown* punkOuter,void** ppobj)
{
	CSysDevEnum*	psde;
	HRESULT	hr;

	TRACE("(%p,%p)\n",punkOuter,ppobj);

	psde = (CSysDevEnum*)QUARTZ_AllocObj( sizeof(CSysDevEnum) );
	if ( psde == NULL )
		return E_OUTOFMEMORY;

	QUARTZ_IUnkInit( &psde->unk, punkOuter );

	hr = CSysDevEnum_InitICreateDevEnum( psde );
	if ( FAILED(hr) )
	{
		QUARTZ_FreeObj( psde );
		return hr;
	}

	psde->unk.pEntries = IFEntries;
	psde->unk.dwEntries = sizeof(IFEntries)/sizeof(IFEntries[0]);
	psde->unk.pOnFinalRelease = QUARTZ_DestroySystemDeviceEnum;

	*ppobj = (void*)(&psde->unk);

	return S_OK;
}

