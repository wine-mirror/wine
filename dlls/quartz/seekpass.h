#ifndef WINE_DSHOW_SEEKPASS_H
#define WINE_DSHOW_SEEKPASS_H

/*
		implements CLSID_SeekingPassThru.

	- At least, the following interfaces should be implemented:

	IUnknown
		+ ISeekingPassThru

*/

#include "iunk.h"

typedef struct QUARTZ_ISeekingPassThruImpl
{
	ICOM_VFIELD(ISeekingPassThru);
} QUARTZ_ISeekingPassThruImpl;


typedef struct CSeekingPassThru
{
	QUARTZ_IUnkImpl	unk;
	QUARTZ_ISeekingPassThruImpl	seekpass;

	/* ISeekingPassThru fields. */
} CSeekingPassThru;

#define	CSeekingPassThru_THIS(iface,member)		CSeekingPassThru*	This = ((CSeekingPassThru*)(((char*)iface)-offsetof(CSeekingPassThru,member)))

HRESULT QUARTZ_CreateSeekingPassThru(IUnknown* punkOuter,void** ppobj);


#endif  /* WINE_DSHOW_SEEKPASS_H */
