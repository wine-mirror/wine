#ifndef WINE_DSHOW_SEEKPASS_H
#define WINE_DSHOW_SEEKPASS_H

/*
		implements CLSID_SeekingPassThru.

	- At least, the following interfaces should be implemented:

	IUnknown
		+ ISeekingPassThru

*/

#include "iunk.h"


/****************************************************************************/

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

/****************************************************************************/

typedef struct QUARTZ_ISeekingPassThruImpl
{
	ICOM_VFIELD(ISeekingPassThru);
} QUARTZ_ISeekingPassThruImpl;

typedef struct CSeekingPassThru
{
	QUARTZ_IUnkImpl	unk;
	QUARTZ_ISeekingPassThruImpl	seekpass;

	/* ISeekingPassThru fields. */
	CRITICAL_SECTION	cs;
	CPassThruImpl	passthru;
} CSeekingPassThru;

#define	CSeekingPassThru_THIS(iface,member)		CSeekingPassThru*	This = ((CSeekingPassThru*)(((char*)iface)-offsetof(CSeekingPassThru,member)))

HRESULT QUARTZ_CreateSeekingPassThru(IUnknown* punkOuter,void** ppobj);


#endif  /* WINE_DSHOW_SEEKPASS_H */
