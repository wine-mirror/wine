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


/* can I use offsetof safely? - FIXME? */
static QUARTZ_IFEntry IFEntries[] =
{
  { &IID_IMemAllocator, offsetof(CMemoryAllocator,memalloc)-offsetof(CMemoryAllocator,unk) },
};

HRESULT QUARTZ_CreateMemoryAllocator(IUnknown* punkOuter,void** ppobj)
{
	CMemoryAllocator*	pma;

	TRACE("(%p,%p)\n",punkOuter,ppobj);

	pma = (CMemoryAllocator*)QUARTZ_AllocObj( sizeof(CMemoryAllocator) );
	if ( pma == NULL )
		return E_OUTOFMEMORY;

	QUARTZ_IUnkInit( &pma->unk );
	CMemoryAllocator_InitIMemAllocator( pma );

	pma->unk.pEntries = IFEntries;
	pma->unk.dwEntries = sizeof(IFEntries)/sizeof(IFEntries[0]);

	*ppobj = (void*)pma;

	return S_OK;
}
