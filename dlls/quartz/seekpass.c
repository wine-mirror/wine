/*
 * Implementation of CLSID_SeekingPassThru.
 *
 * hidenori@a2.ctktv.ne.jp
 */

#include "config.h"

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "wine/obj_base.h"
#include "strmif.h"
#include "control.h"
#include "uuids.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "seekpass.h"



static HRESULT WINAPI
ISeekingPassThru_fnQueryInterface(ISeekingPassThru* iface,REFIID riid,void** ppobj)
{
	CSeekingPassThru_THIS(iface,seekpass);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
ISeekingPassThru_fnAddRef(ISeekingPassThru* iface)
{
	CSeekingPassThru_THIS(iface,seekpass);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
ISeekingPassThru_fnRelease(ISeekingPassThru* iface)
{
	CSeekingPassThru_THIS(iface,seekpass);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}

static HRESULT WINAPI
ISeekingPassThru_fnInit(ISeekingPassThru* iface,BOOL bRendering,IPin* pPin)
{
	CSeekingPassThru_THIS(iface,seekpass);

	FIXME("(%p)->(%d,%p) not tested!\n",This,bRendering,pPin);

	if ( pPin == NULL )
		return E_POINTER;

	/* Why 'bRendering' is given as an argument?? */
	EnterCriticalSection( &This->cs );

	if ( This->passthru.pPin != NULL )
		IPin_Release( This->passthru.pPin );
	This->passthru.pPin = pPin; IPin_AddRef( pPin );

	LeaveCriticalSection( &This->cs );

	return NOERROR;
}


static ICOM_VTABLE(ISeekingPassThru) iseekingpassthru =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	ISeekingPassThru_fnQueryInterface,
	ISeekingPassThru_fnAddRef,
	ISeekingPassThru_fnRelease,
	/* ISeekingPassThru fields */
	ISeekingPassThru_fnInit,
};

static
HRESULT CSeekingPassThru_InitISeekingPassThru(CSeekingPassThru* This)
{
	TRACE("(%p)\n",This);
	ICOM_VTBL(&This->seekpass) = &iseekingpassthru;
	This->passthru.punk = This->unk.punkControl;
	This->passthru.pPin = NULL;
	InitializeCriticalSection( &This->cs );

	return NOERROR;
}

static
void CSeekingPassThru_UninitISeekingPassThru(CSeekingPassThru* This)
{
	TRACE("(%p)\n",This);
	if ( This->passthru.pPin != NULL )
	{
		IPin_Release( This->passthru.pPin );
		This->passthru.pPin = NULL;
	}
	DeleteCriticalSection( &This->cs );
}


/* can I use offsetof safely? - FIXME? */
static QUARTZ_IFEntry IFEntries[] =
{
  { &IID_ISeekingPassThru, offsetof(CSeekingPassThru,seekpass)-offsetof(CSeekingPassThru,unk) },
  { &IID_IMediaPosition, offsetof(CSeekingPassThru,passthru.mpos)-offsetof(CSeekingPassThru,unk) },
  { &IID_IMediaSeeking, offsetof(CSeekingPassThru,passthru.mseek)-offsetof(CSeekingPassThru,unk) },
};


static void QUARTZ_DestroySeekingPassThru(IUnknown* punk)
{
	CSeekingPassThru_THIS(punk,unk);

	CSeekingPassThru_UninitISeekingPassThru(This);
}

HRESULT QUARTZ_CreateSeekingPassThru(IUnknown* punkOuter,void** ppobj)
{
	CSeekingPassThru*	This;
	HRESULT	hr;

	TRACE("(%p,%p)\n",punkOuter,ppobj);

	This = (CSeekingPassThru*)QUARTZ_AllocObj( sizeof(CSeekingPassThru) );
	if ( This == NULL )
		return E_OUTOFMEMORY;

	QUARTZ_IUnkInit( &This->unk, punkOuter );
	hr = CSeekingPassThru_InitISeekingPassThru(This);
	if ( FAILED(hr) )
	{
		QUARTZ_FreeObj( This );
		return hr;
	}

	This->unk.pEntries = IFEntries;
	This->unk.dwEntries = sizeof(IFEntries)/sizeof(IFEntries[0]);
	This->unk.pOnFinalRelease = QUARTZ_DestroySeekingPassThru;

	*ppobj = (void*)(&This->unk);

	return S_OK;
}

