/*
 * Implementation of CLSID_MemoryAllocator.
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
#include "memalloc.h"


static HRESULT WINAPI
IMemAllocator_fnQueryInterface(IMemAllocator* iface,REFIID riid,void** ppobj)
{
	CMemoryAllocator_THIS(iface,memalloc);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IMemAllocator_fnAddRef(IMemAllocator* iface)
{
	CMemoryAllocator_THIS(iface,memalloc);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IMemAllocator_fnRelease(IMemAllocator* iface)
{
	CMemoryAllocator_THIS(iface,memalloc);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}

static HRESULT WINAPI
IMemAllocator_fnSetProperties(IMemAllocator* iface,ALLOCATOR_PROPERTIES* pPropReq,ALLOCATOR_PROPERTIES* pPropActual)
{
	CMemoryAllocator_THIS(iface,memalloc);

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IMemAllocator_fnGetProperties(IMemAllocator* iface,ALLOCATOR_PROPERTIES* pProp)
{
	CMemoryAllocator_THIS(iface,memalloc);

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IMemAllocator_fnCommit(IMemAllocator* iface)
{
	CMemoryAllocator_THIS(iface,memalloc);

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IMemAllocator_fnDecommit(IMemAllocator* iface)
{
	CMemoryAllocator_THIS(iface,memalloc);

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IMemAllocator_fnGetBuffer(IMemAllocator* iface,IMediaSample** ppSample,REFERENCE_TIME* prtStart,REFERENCE_TIME* prtEnd,DWORD dwFlags)
{
	CMemoryAllocator_THIS(iface,memalloc);

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IMemAllocator_fnReleaseBuffer(IMemAllocator* iface,IMediaSample* pSample)
{
	CMemoryAllocator_THIS(iface,memalloc);

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}



static ICOM_VTABLE(IMemAllocator) imemalloc =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IMemAllocator_fnQueryInterface,
	IMemAllocator_fnAddRef,
	IMemAllocator_fnRelease,
	/* IMemAllocator fields */
	IMemAllocator_fnSetProperties,
	IMemAllocator_fnGetProperties,
	IMemAllocator_fnCommit,
	IMemAllocator_fnDecommit,
	IMemAllocator_fnGetBuffer,
	IMemAllocator_fnReleaseBuffer,
};


void CMemoryAllocator_InitIMemAllocator( CMemoryAllocator* pma )
{
	TRACE("(%p)\n",pma);
	ICOM_VTBL(&pma->memalloc) = &imemalloc;
}
