#ifndef WINE_DSHOW_PTIMPL_H
#define WINE_DSHOW_PTIMPL_H

/*
 * A pass-through class.
 *
 * hidenori@a2.ctktv.ne.jp
 */

typedef struct CPassThruImpl
{
	struct { ICOM_VFIELD(IMediaPosition); } mpos;
	struct { ICOM_VFIELD(IMediaSeeking); } mseek;

	IUnknown* punk;
	IPin* pPin;
} CPassThruImpl;

#define	CPassThruImpl_THIS(iface,member)		CPassThruImpl*	This = ((CPassThruImpl*)(((char*)iface)-offsetof(CPassThruImpl,member)))

HRESULT CPassThruImpl_InitIMediaPosition( CPassThruImpl* pImpl );
void CPassThruImpl_UninitIMediaPosition( CPassThruImpl* pImpl );
HRESULT CPassThruImpl_InitIMediaSeeking( CPassThruImpl* pImpl );
void CPassThruImpl_UninitIMediaSeeking( CPassThruImpl* pImpl );

HRESULT CPassThruImpl_QueryPosPass(
	CPassThruImpl* pImpl, IMediaPosition** ppPosition );
HRESULT CPassThruImpl_QuerySeekPass(
	CPassThruImpl* pImpl, IMediaSeeking** ppSeeking );


#endif  /* WINE_DSHOW_PTIMPL_H */
