/*
 * A pass-through class.
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
#include "ptimpl.h"

static
HRESULT CPassThruImpl_GetConnected( CPassThruImpl* pImpl, IPin** ppPin )
{
	return IPin_ConnectedTo( pImpl->pPin, ppPin );
}

HRESULT CPassThruImpl_QueryPosPass(
	CPassThruImpl* pImpl, IMediaPosition** ppPosition )
{
	IPin*	pPin;
	HRESULT	hr;

	hr = CPassThruImpl_GetConnected( pImpl, &pPin );
	if ( FAILED(hr) )
		return hr;
	hr = IPin_QueryInterface(pPin,&IID_IMediaPosition,(void**)ppPosition);
	IPin_Release(pPin);

	return hr;
}

HRESULT CPassThruImpl_QuerySeekPass(
	CPassThruImpl* pImpl, IMediaSeeking** ppSeeking )
{
	IPin*	pPin;
	HRESULT	hr;

	hr = CPassThruImpl_GetConnected( pImpl, &pPin );
	if ( FAILED(hr) )
		return hr;
	hr = IPin_QueryInterface(pPin,&IID_IMediaSeeking,(void**)ppSeeking);
	IPin_Release(pPin);

	return hr;
}
