/*
 * Implementation of IEnumUnknown (for internal use).
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
#include "wine/obj_oleaut.h"
#include "strmif.h"
#include "control.h"
#include "uuids.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "enumunk.h"
#include "iunk.h"



typedef struct IEnumUnknownImpl
{
	ICOM_VFIELD(IEnumUnknown);
} IEnumUnknownImpl;

typedef struct
{
	QUARTZ_IUnkImpl	unk;
	IEnumUnknownImpl	enumunk;
	struct QUARTZ_IFEntry	IFEntries[1];
	QUARTZ_CompList*	pCompList;
	QUARTZ_CompListItem*	pItemCur;
} CEnumUnknown;

#define	CEnumUnknown_THIS(iface,member)		CEnumUnknown*	This = ((CEnumUnknown*)(((char*)iface)-offsetof(CEnumUnknown,member)))




static HRESULT WINAPI
IEnumUnknown_fnQueryInterface(IEnumUnknown* iface,REFIID riid,void** ppobj)
{
	CEnumUnknown_THIS(iface,enumunk);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IEnumUnknown_fnAddRef(IEnumUnknown* iface)
{
	CEnumUnknown_THIS(iface,enumunk);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IEnumUnknown_fnRelease(IEnumUnknown* iface)
{
	CEnumUnknown_THIS(iface,enumunk);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}

static HRESULT WINAPI
IEnumUnknown_fnNext(IEnumUnknown* iface,ULONG cReq,IUnknown** ppunk,ULONG* pcFetched)
{
	CEnumUnknown_THIS(iface,enumunk);
	HRESULT	hr;
	ULONG	cFetched;

	TRACE("(%p)->(%lu,%p,%p)\n",This,cReq,ppunk,pcFetched);

	if ( pcFetched == NULL && cReq > 1 )
		return E_INVALIDARG;
	if ( ppunk == NULL )
		return E_POINTER;

	QUARTZ_CompList_Lock( This->pCompList );

	hr = NOERROR;
	cFetched = 0;
	while ( cReq > 0 )
	{
		if ( This->pItemCur == NULL )
		{
			hr = S_FALSE;
			break;
		}
		ppunk[ cFetched ++ ] = QUARTZ_CompList_GetItemPtr( This->pItemCur );
		IUnknown_AddRef( *ppunk );

		This->pItemCur =
			QUARTZ_CompList_GetNext( This->pCompList, This->pItemCur );
		cReq --;
	}

	QUARTZ_CompList_Unlock( This->pCompList );

	if ( pcFetched != NULL )
		*pcFetched = cFetched;

	return hr;
}

static HRESULT WINAPI
IEnumUnknown_fnSkip(IEnumUnknown* iface,ULONG cSkip)
{
	CEnumUnknown_THIS(iface,enumunk);
	HRESULT	hr;

	TRACE("(%p)->()\n",This);

	QUARTZ_CompList_Lock( This->pCompList );

	hr = NOERROR;
	while ( cSkip > 0 )
	{
		if ( This->pItemCur == NULL )
		{
			hr = S_FALSE;
			break;
		}
		This->pItemCur =
			QUARTZ_CompList_GetNext( This->pCompList, This->pItemCur );
		cSkip --;
	}

	QUARTZ_CompList_Unlock( This->pCompList );

	return hr;
}

static HRESULT WINAPI
IEnumUnknown_fnReset(IEnumUnknown* iface)
{
	CEnumUnknown_THIS(iface,enumunk);

	TRACE("(%p)->()\n",This);

	QUARTZ_CompList_Lock( This->pCompList );

	This->pItemCur = QUARTZ_CompList_GetFirst( This->pCompList );

	QUARTZ_CompList_Unlock( This->pCompList );

	return NOERROR;
}

static HRESULT WINAPI
IEnumUnknown_fnClone(IEnumUnknown* iface,IEnumUnknown** ppunk)
{
	CEnumUnknown_THIS(iface,enumunk);
	HRESULT	hr;

	TRACE("(%p)->()\n",This);

	if ( ppunk == NULL )
		return E_POINTER;

	QUARTZ_CompList_Lock( This->pCompList );

	hr = QUARTZ_CreateEnumUnknown(
		This->IFEntries[0].piid,
		(void**)ppunk,
		This->pCompList );
	FIXME( "current pointer must be seeked correctly\n" );

	QUARTZ_CompList_Unlock( This->pCompList );

	return hr;
}


static ICOM_VTABLE(IEnumUnknown) ienumunk =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IEnumUnknown_fnQueryInterface,
	IEnumUnknown_fnAddRef,
	IEnumUnknown_fnRelease,
	/* IEnumUnknown fields */
	IEnumUnknown_fnNext,
	IEnumUnknown_fnSkip,
	IEnumUnknown_fnReset,
	IEnumUnknown_fnClone,
};

void QUARTZ_DestroyEnumUnknown(IUnknown* punk)
{
	CEnumUnknown_THIS(punk,unk);

	if ( This->pCompList != NULL )
		QUARTZ_CompList_Free( This->pCompList );
}

HRESULT QUARTZ_CreateEnumUnknown(
	REFIID riidEnum, void** ppobj, const QUARTZ_CompList* pCompList )
{
	CEnumUnknown*	penum;
	QUARTZ_CompList*	pCompListDup;

	TRACE("(%s,%p,%p)\n",debugstr_guid(riidEnum),ppobj,pCompList);

	pCompListDup = QUARTZ_CompList_Dup( pCompList, FALSE );
	if ( pCompListDup == NULL )
		return E_OUTOFMEMORY;

	penum = (CEnumUnknown*)QUARTZ_AllocObj( sizeof(CEnumUnknown) );
	if ( penum == NULL )
	{
		QUARTZ_CompList_Free( pCompListDup );
		return E_OUTOFMEMORY;
	}

	QUARTZ_IUnkInit( &penum->unk, NULL );
	ICOM_VTBL(&penum->enumunk) = &ienumunk;

	penum->IFEntries[0].piid = riidEnum;
	penum->IFEntries[0].ofsVTPtr =
		offsetof(CEnumUnknown,enumunk)-offsetof(CEnumUnknown,unk);
	penum->pCompList = pCompListDup;
	penum->pItemCur = QUARTZ_CompList_GetFirst( pCompListDup );

	penum->unk.pEntries = penum->IFEntries;
	penum->unk.dwEntries = 1;
	penum->unk.pOnFinalRelease = QUARTZ_DestroyEnumUnknown;

	*ppobj = (void*)(&penum->enumunk);

	return S_OK;
}

