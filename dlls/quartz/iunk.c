/*
 * An implementation of IUnknown.
 *
 * hidenori@a2.ctktv.ne.jp
 */

#include "config.h"

#include "windef.h"
#include "winerror.h"
#include "wine/obj_base.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "iunk.h"


static HRESULT WINAPI
IUnknown_fnQueryInterface(IUnknown* iface,REFIID riid,LPVOID *ppobj)
{
	ICOM_THIS(QUARTZ_IUnkImpl,iface);
	size_t	ofs;
	DWORD	dwIndex;

	TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);

	if ( ppobj == NULL )
		return E_POINTER;
	*ppobj = NULL;

	ofs = 0;

	if ( IsEqualGUID( &IID_IUnknown, riid ) )
	{
		TRACE("IID_IUnknown - returns inner object.\n");
	}
	else
	{
		for ( dwIndex = 0; dwIndex < This->dwEntries; dwIndex++ )
		{
			if ( IsEqualGUID( This->pEntries[dwIndex].piid, riid ) )
			{
				ofs = This->pEntries[dwIndex].ofsVTPtr;
				break;
			}
		}
		if ( dwIndex == This->dwEntries )
		{
			FIXME("unknown interface: %s\n",debugstr_guid(riid));
			return E_NOINTERFACE;
		}
	}

	*ppobj = (LPVOID)(((char*)This) + ofs);
	IUnknown_AddRef((IUnknown*)(*ppobj));

	return S_OK;
}

static ULONG WINAPI
IUnknown_fnAddRef(IUnknown* iface)
{
	ICOM_THIS(QUARTZ_IUnkImpl,iface);

	TRACE("(%p)->()\n",This);

	return ++(This->ref);
}

static ULONG WINAPI
IUnknown_fnRelease(IUnknown* iface)
{
	ICOM_THIS(QUARTZ_IUnkImpl,iface);

	TRACE("(%p)->()\n",This);
	if ( (--(This->ref)) > 0 )
		return This->ref;

	QUARTZ_FreeObj(This);

	return 0;
}

static ICOM_VTABLE(IUnknown) iunknown =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IUnknown_fnQueryInterface,
	IUnknown_fnAddRef,
	IUnknown_fnRelease,
};


void QUARTZ_IUnkInit( QUARTZ_IUnkImpl* pImpl, IUnknown* punkOuter )
{
	TRACE("(%p)\n",pImpl);

	ICOM_VTBL(pImpl) = &iunknown;
	pImpl->pEntries = NULL;
	pImpl->dwEntries = 0;
	pImpl->ref = 1;
	pImpl->punkControl = (IUnknown*)pImpl;

	/* for delegation. */
	if ( punkOuter != NULL )
		pImpl->punkControl = punkOuter;
}

