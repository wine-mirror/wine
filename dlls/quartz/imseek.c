/*
 * Implementation of IMediaSeeking for FilterGraph.
 *
 * FIXME - stub.
 * FIXME - this interface should be allocated as a plug-in(?)
 *
 * Copyright (C) Hidenori TAKESHIMA <hidenori@a2.ctktv.ne.jp>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "strmif.h"
#include "control.h"
#include "uuids.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "fgraph.h"



static HRESULT WINAPI
IMediaSeeking_fnQueryInterface(IMediaSeeking* iface,REFIID riid,void** ppobj)
{
	CFilterGraph_THIS(iface,mediaseeking);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IMediaSeeking_fnAddRef(IMediaSeeking* iface)
{
	CFilterGraph_THIS(iface,mediaseeking);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IMediaSeeking_fnRelease(IMediaSeeking* iface)
{
	CFilterGraph_THIS(iface,mediaseeking);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}


static HRESULT WINAPI
IMediaSeeking_fnGetCapabilities(IMediaSeeking* iface,DWORD* pdwCaps)
{
	CFilterGraph_THIS(iface,mediaseeking);
	HRESULT	hr = E_NOTIMPL;
	HRESULT	hrFilter;
	DWORD	n;

	TRACE("(%p)->()\n",This);

	EnterCriticalSection( &This->m_csFilters );

	for ( n = 0; n < This->m_cActiveFilters; n++ )
	{
		if ( This->m_pActiveFilters[n].pSeeking != NULL )
		{
			hrFilter = IMediaSeeking_GetCapabilities( This->m_pActiveFilters[n].pSeeking, pdwCaps );
			if ( hr == E_NOTIMPL )
			{
				hr = hrFilter;
			}
			else
			if ( hrFilter != E_NOTIMPL )
			{
				if ( SUCCEEDED(hr) )
					hr = hrFilter;
			}
		}
	}

	LeaveCriticalSection( &This->m_csFilters );

	return hr;
}

static HRESULT WINAPI
IMediaSeeking_fnCheckCapabilities(IMediaSeeking* iface,DWORD* pdwCaps)
{
	CFilterGraph_THIS(iface,mediaseeking);
	HRESULT	hr = E_NOTIMPL;
	HRESULT	hrFilter;
	DWORD	n;

	TRACE("(%p)->()\n",This);

	EnterCriticalSection( &This->m_csFilters );

	for ( n = 0; n < This->m_cActiveFilters; n++ )
	{
		if ( This->m_pActiveFilters[n].pSeeking != NULL )
		{
			hrFilter = IMediaSeeking_CheckCapabilities( This->m_pActiveFilters[n].pSeeking, pdwCaps );
			if ( hr == E_NOTIMPL )
			{
				hr = hrFilter;
			}
			else
			if ( hrFilter != E_NOTIMPL )
			{
				if ( SUCCEEDED(hr) )
					hr = hrFilter;
			}
		}
	}

	LeaveCriticalSection( &This->m_csFilters );

	return hr;
}

static HRESULT WINAPI
IMediaSeeking_fnIsFormatSupported(IMediaSeeking* iface,const GUID* pidFormat)
{
	CFilterGraph_THIS(iface,mediaseeking);
	HRESULT	hr = E_NOTIMPL;
	HRESULT	hrFilter;
	DWORD	n;

	TRACE("(%p)->()\n",This);

	EnterCriticalSection( &This->m_csFilters );

	for ( n = 0; n < This->m_cActiveFilters; n++ )
	{
		if ( This->m_pActiveFilters[n].pSeeking != NULL )
		{
			hrFilter = IMediaSeeking_IsFormatSupported( This->m_pActiveFilters[n].pSeeking, pidFormat );
			if ( hr == E_NOTIMPL )
			{
				hr = hrFilter;
			}
			else
			if ( hrFilter != E_NOTIMPL )
			{
				if ( SUCCEEDED(hr) )
					hr = hrFilter;
			}
		}
	}

	LeaveCriticalSection( &This->m_csFilters );

	return hr;
}

static HRESULT WINAPI
IMediaSeeking_fnQueryPreferredFormat(IMediaSeeking* iface,GUID* pidFormat)
{
	CFilterGraph_THIS(iface,mediaseeking);
	HRESULT	hr = E_NOTIMPL;
	HRESULT	hrFilter;
	DWORD	n;

	TRACE("(%p)->()\n",This);

	EnterCriticalSection( &This->m_csFilters );

	for ( n = 0; n < This->m_cActiveFilters; n++ )
	{
		if ( This->m_pActiveFilters[n].pSeeking != NULL )
		{
			hrFilter = IMediaSeeking_QueryPreferredFormat( This->m_pActiveFilters[n].pSeeking, pidFormat );
			if ( hr == E_NOTIMPL )
			{
				hr = hrFilter;
			}
			else
			if ( hrFilter != E_NOTIMPL )
			{
				if ( SUCCEEDED(hr) )
					hr = hrFilter;
			}
		}
	}

	LeaveCriticalSection( &This->m_csFilters );

	return hr;
}

static HRESULT WINAPI
IMediaSeeking_fnGetTimeFormat(IMediaSeeking* iface,GUID* pidFormat)
{
	CFilterGraph_THIS(iface,mediaseeking);
	HRESULT	hr = E_NOTIMPL;
	HRESULT	hrFilter;
	DWORD	n;

	TRACE("(%p)->()\n",This);

	EnterCriticalSection( &This->m_csFilters );

	for ( n = 0; n < This->m_cActiveFilters; n++ )
	{
		if ( This->m_pActiveFilters[n].pSeeking != NULL )
		{
			hrFilter = IMediaSeeking_GetTimeFormat( This->m_pActiveFilters[n].pSeeking, pidFormat );
			if ( hr == E_NOTIMPL )
			{
				hr = hrFilter;
			}
			else
			if ( hrFilter != E_NOTIMPL )
			{
				if ( SUCCEEDED(hr) )
					hr = hrFilter;
			}
		}
	}

	LeaveCriticalSection( &This->m_csFilters );

	return hr;
}

static HRESULT WINAPI
IMediaSeeking_fnIsUsingTimeFormat(IMediaSeeking* iface,const GUID* pidFormat)
{
	CFilterGraph_THIS(iface,mediaseeking);
	HRESULT	hr = E_NOTIMPL;
	HRESULT	hrFilter;
	DWORD	n;

	TRACE("(%p)->()\n",This);

	EnterCriticalSection( &This->m_csFilters );

	for ( n = 0; n < This->m_cActiveFilters; n++ )
	{
		if ( This->m_pActiveFilters[n].pSeeking != NULL )
		{
			hrFilter = IMediaSeeking_IsUsingTimeFormat( This->m_pActiveFilters[n].pSeeking, pidFormat );
			if ( hr == E_NOTIMPL )
			{
				hr = hrFilter;
			}
			else
			if ( hrFilter != E_NOTIMPL )
			{
				if ( SUCCEEDED(hr) )
					hr = hrFilter;
			}
		}
	}

	LeaveCriticalSection( &This->m_csFilters );

	return hr;
}

static HRESULT WINAPI
IMediaSeeking_fnSetTimeFormat(IMediaSeeking* iface,const GUID* pidFormat)
{
	CFilterGraph_THIS(iface,mediaseeking);
	HRESULT	hr = E_NOTIMPL;
	HRESULT	hrFilter;
	DWORD	n;

	TRACE("(%p)->()\n",This);

	EnterCriticalSection( &This->m_csFilters );

	for ( n = 0; n < This->m_cActiveFilters; n++ )
	{
		if ( This->m_pActiveFilters[n].pSeeking != NULL )
		{
			hrFilter = IMediaSeeking_SetTimeFormat( This->m_pActiveFilters[n].pSeeking, pidFormat );
			if ( hr == E_NOTIMPL )
			{
				hr = hrFilter;
			}
			else
			if ( hrFilter != E_NOTIMPL )
			{
				if ( SUCCEEDED(hr) )
					hr = hrFilter;
			}
		}
	}

	LeaveCriticalSection( &This->m_csFilters );

	return hr;
}

static HRESULT WINAPI
IMediaSeeking_fnGetDuration(IMediaSeeking* iface,LONGLONG* pllDuration)
{
	CFilterGraph_THIS(iface,mediaseeking);
	HRESULT	hr = E_NOTIMPL;
	HRESULT	hrFilter;
	DWORD	n;

	TRACE("(%p)->()\n",This);

	EnterCriticalSection( &This->m_csFilters );

	for ( n = 0; n < This->m_cActiveFilters; n++ )
	{
		if ( This->m_pActiveFilters[n].pSeeking != NULL )
		{
			hrFilter = IMediaSeeking_GetDuration( This->m_pActiveFilters[n].pSeeking, pllDuration );
			if ( hr == E_NOTIMPL )
			{
				hr = hrFilter;
			}
			else
			if ( hrFilter != E_NOTIMPL )
			{
				if ( SUCCEEDED(hr) )
					hr = hrFilter;
			}
		}
	}

	LeaveCriticalSection( &This->m_csFilters );

	return hr;
}

static HRESULT WINAPI
IMediaSeeking_fnGetStopPosition(IMediaSeeking* iface,LONGLONG* pllPos)
{
	CFilterGraph_THIS(iface,mediaseeking);
	HRESULT	hr = E_NOTIMPL;
	HRESULT	hrFilter;
	DWORD	n;

	TRACE("(%p)->()\n",This);

	EnterCriticalSection( &This->m_csFilters );

	for ( n = 0; n < This->m_cActiveFilters; n++ )
	{
		if ( This->m_pActiveFilters[n].pSeeking != NULL )
		{
			hrFilter = IMediaSeeking_GetStopPosition( This->m_pActiveFilters[n].pSeeking, pllPos );
			if ( hr == E_NOTIMPL )
			{
				hr = hrFilter;
			}
			else
			if ( hrFilter != E_NOTIMPL )
			{
				if ( SUCCEEDED(hr) )
					hr = hrFilter;
			}
		}
	}

	LeaveCriticalSection( &This->m_csFilters );

	return hr;
}

static HRESULT WINAPI
IMediaSeeking_fnGetCurrentPosition(IMediaSeeking* iface,LONGLONG* pllPos)
{
	CFilterGraph_THIS(iface,mediaseeking);
	HRESULT	hr = E_NOTIMPL;
	HRESULT	hrFilter;
	DWORD	n;

	TRACE("(%p)->()\n",This);

	EnterCriticalSection( &This->m_csFilters );

	for ( n = 0; n < This->m_cActiveFilters; n++ )
	{
		if ( This->m_pActiveFilters[n].pSeeking != NULL )
		{
			hrFilter = IMediaSeeking_GetCurrentPosition( This->m_pActiveFilters[n].pSeeking, pllPos );
			if ( hr == E_NOTIMPL )
			{
				hr = hrFilter;
			}
			else
			if ( hrFilter != E_NOTIMPL )
			{
				if ( SUCCEEDED(hr) )
					hr = hrFilter;
			}
		}
	}

	LeaveCriticalSection( &This->m_csFilters );

	return hr;
}

static HRESULT WINAPI
IMediaSeeking_fnConvertTimeFormat(IMediaSeeking* iface,LONGLONG* pllOut,const GUID* pidFmtOut,LONGLONG llIn,const GUID* pidFmtIn)
{
	CFilterGraph_THIS(iface,mediaseeking);
	HRESULT	hr = E_NOTIMPL;
	HRESULT	hrFilter;
	DWORD	n;

	TRACE("(%p)->()\n",This);

	EnterCriticalSection( &This->m_csFilters );

	for ( n = 0; n < This->m_cActiveFilters; n++ )
	{
		if ( This->m_pActiveFilters[n].pSeeking != NULL )
		{
			hrFilter = IMediaSeeking_ConvertTimeFormat( This->m_pActiveFilters[n].pSeeking, pllOut, pidFmtOut, llIn, pidFmtIn );
			if ( hr == E_NOTIMPL )
			{
				hr = hrFilter;
			}
			else
			if ( hrFilter != E_NOTIMPL )
			{
				if ( SUCCEEDED(hr) )
					hr = hrFilter;
			}
		}
	}

	LeaveCriticalSection( &This->m_csFilters );

	return hr;
}

static HRESULT WINAPI
IMediaSeeking_fnSetPositions(IMediaSeeking* iface,LONGLONG* pllCur,DWORD dwCurFlags,LONGLONG* pllStop,DWORD dwStopFlags)
{
	CFilterGraph_THIS(iface,mediaseeking);
	HRESULT	hr = E_NOTIMPL;
	HRESULT	hrFilter;
	DWORD	n;

	TRACE("(%p)->()\n",This);

	EnterCriticalSection( &This->m_csFilters );

	for ( n = 0; n < This->m_cActiveFilters; n++ )
	{
		if ( This->m_pActiveFilters[n].pSeeking != NULL )
		{
			hrFilter = IMediaSeeking_SetPositions( This->m_pActiveFilters[n].pSeeking, pllCur, dwCurFlags, pllStop, dwStopFlags );
			if ( hr == E_NOTIMPL )
			{
				hr = hrFilter;
			}
			else
			if ( hrFilter != E_NOTIMPL )
			{
				if ( SUCCEEDED(hr) )
					hr = hrFilter;
			}
		}
	}

	LeaveCriticalSection( &This->m_csFilters );

	return hr;
}

static HRESULT WINAPI
IMediaSeeking_fnGetPositions(IMediaSeeking* iface,LONGLONG* pllCur,LONGLONG* pllStop)
{
	CFilterGraph_THIS(iface,mediaseeking);
	HRESULT	hr = E_NOTIMPL;
	HRESULT	hrFilter;
	DWORD	n;

	TRACE("(%p)->()\n",This);

	EnterCriticalSection( &This->m_csFilters );

	for ( n = 0; n < This->m_cActiveFilters; n++ )
	{
		if ( This->m_pActiveFilters[n].pSeeking != NULL )
		{
			hrFilter = IMediaSeeking_GetPositions( This->m_pActiveFilters[n].pSeeking, pllCur, pllStop );
			if ( hr == E_NOTIMPL )
			{
				hr = hrFilter;
			}
			else
			if ( hrFilter != E_NOTIMPL )
			{
				if ( SUCCEEDED(hr) )
					hr = hrFilter;
			}
		}
	}

	LeaveCriticalSection( &This->m_csFilters );

	return hr;
}

static HRESULT WINAPI
IMediaSeeking_fnGetAvailable(IMediaSeeking* iface,LONGLONG* pllFirst,LONGLONG* pllLast)
{
	CFilterGraph_THIS(iface,mediaseeking);
	HRESULT	hr = E_NOTIMPL;
	HRESULT	hrFilter;
	DWORD	n;

	TRACE("(%p)->()\n",This);

	EnterCriticalSection( &This->m_csFilters );

	for ( n = 0; n < This->m_cActiveFilters; n++ )
	{
		if ( This->m_pActiveFilters[n].pSeeking != NULL )
		{
			hrFilter = IMediaSeeking_GetAvailable( This->m_pActiveFilters[n].pSeeking, pllFirst, pllLast );
			if ( hr == E_NOTIMPL )
			{
				hr = hrFilter;
			}
			else
			if ( hrFilter != E_NOTIMPL )
			{
				if ( SUCCEEDED(hr) )
					hr = hrFilter;
			}
		}
	}

	LeaveCriticalSection( &This->m_csFilters );

	return hr;
}

static HRESULT WINAPI
IMediaSeeking_fnSetRate(IMediaSeeking* iface,double dblRate)
{
	CFilterGraph_THIS(iface,mediaseeking);
	HRESULT	hr = E_NOTIMPL;
	HRESULT	hrFilter;
	DWORD	n;

	TRACE("(%p)->()\n",This);

	EnterCriticalSection( &This->m_csFilters );

	for ( n = 0; n < This->m_cActiveFilters; n++ )
	{
		if ( This->m_pActiveFilters[n].pSeeking != NULL )
		{
			hrFilter = IMediaSeeking_SetRate( This->m_pActiveFilters[n].pSeeking, dblRate );
			if ( hr == E_NOTIMPL )
			{
				hr = hrFilter;
			}
			else
			if ( hrFilter != E_NOTIMPL )
			{
				if ( SUCCEEDED(hr) )
					hr = hrFilter;
			}
		}
	}

	LeaveCriticalSection( &This->m_csFilters );

	return hr;
}

static HRESULT WINAPI
IMediaSeeking_fnGetRate(IMediaSeeking* iface,double* pdblRate)
{
	CFilterGraph_THIS(iface,mediaseeking);
	HRESULT	hr = E_NOTIMPL;
	HRESULT	hrFilter;
	DWORD	n;

	TRACE("(%p)->()\n",This);

	EnterCriticalSection( &This->m_csFilters );

	for ( n = 0; n < This->m_cActiveFilters; n++ )
	{
		if ( This->m_pActiveFilters[n].pSeeking != NULL )
		{
			hrFilter = IMediaSeeking_GetRate( This->m_pActiveFilters[n].pSeeking, pdblRate );
			if ( hr == E_NOTIMPL )
			{
				hr = hrFilter;
			}
			else
			if ( hrFilter != E_NOTIMPL )
			{
				if ( SUCCEEDED(hr) )
					hr = hrFilter;
			}
		}
	}

	LeaveCriticalSection( &This->m_csFilters );

	return hr;
}

static HRESULT WINAPI
IMediaSeeking_fnGetPreroll(IMediaSeeking* iface,LONGLONG* pllPreroll)
{
	CFilterGraph_THIS(iface,mediaseeking);
	HRESULT	hr = E_NOTIMPL;
	HRESULT	hrFilter;
	DWORD	n;

	TRACE("(%p)->()\n",This);

	EnterCriticalSection( &This->m_csFilters );

	for ( n = 0; n < This->m_cActiveFilters; n++ )
	{
		if ( This->m_pActiveFilters[n].pSeeking != NULL )
		{
			hrFilter = IMediaSeeking_GetPreroll( This->m_pActiveFilters[n].pSeeking, pllPreroll );
			if ( hr == E_NOTIMPL )
			{
				hr = hrFilter;
			}
			else
			if ( hrFilter != E_NOTIMPL )
			{
				if ( SUCCEEDED(hr) )
					hr = hrFilter;
			}
		}
	}

	LeaveCriticalSection( &This->m_csFilters );

	return hr;
}




static ICOM_VTABLE(IMediaSeeking) imediaseeking =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IMediaSeeking_fnQueryInterface,
	IMediaSeeking_fnAddRef,
	IMediaSeeking_fnRelease,
	/* IMediaSeeking fields */
	IMediaSeeking_fnGetCapabilities,
	IMediaSeeking_fnCheckCapabilities,
	IMediaSeeking_fnIsFormatSupported,
	IMediaSeeking_fnQueryPreferredFormat,
	IMediaSeeking_fnGetTimeFormat,
	IMediaSeeking_fnIsUsingTimeFormat,
	IMediaSeeking_fnSetTimeFormat,
	IMediaSeeking_fnGetDuration,
	IMediaSeeking_fnGetStopPosition,
	IMediaSeeking_fnGetCurrentPosition,
	IMediaSeeking_fnConvertTimeFormat,
	IMediaSeeking_fnSetPositions,
	IMediaSeeking_fnGetPositions,
	IMediaSeeking_fnGetAvailable,
	IMediaSeeking_fnSetRate,
	IMediaSeeking_fnGetRate,
	IMediaSeeking_fnGetPreroll,
};

HRESULT CFilterGraph_InitIMediaSeeking( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
	ICOM_VTBL(&pfg->mediaseeking) = &imediaseeking;

	return NOERROR;
}

void CFilterGraph_UninitIMediaSeeking( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
}
