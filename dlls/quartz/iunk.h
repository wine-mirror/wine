/*
 * An implementation of IUnknown.
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

#ifndef	WINE_DSHOW_IUNK_H
#define	WINE_DSHOW_IUNK_H

/*
	To avoid implementing IUnknown for all interfaces,

  1) To give a method to get rel-offset of IUnknown.
  2) The IUnknown knows all IIDs and offsets of interfaces.

    So each implementation must have following two members
    with the following order:

  typedef struct IDispatchImpl
  {
    ICOM_VFIELD(IDispatch);	<-pointer of the interface.
    size_t	ofsIUnknown;	<-ofs<IDispatchImpl> - ofs<QUARTZ_IUnkImpl>
  };

 */

/* for InterlockedExchangeAdd. */
#include <pshpack4.h>

typedef struct QUARTZ_IFEntry
{
	const IID*	piid;		/* interface ID. */
	size_t		ofsVTPtr;	/* offset from IUnknown. */
} QUARTZ_IFEntry;

typedef struct QUARTZ_IFDelegation
{
	struct QUARTZ_IFDelegation*	pNext;
	HRESULT (*pOnQueryInterface)(
		IUnknown* punk, const IID* piid, void** ppobj );
} QUARTZ_IFDelegation;

typedef struct QUARTZ_IUnkImpl
{
	/* pointer of IUnknown interface. */
	ICOM_VFIELD(IUnknown);

	/* array of supported IIDs and offsets. */
	const QUARTZ_IFEntry*	pEntries;
	DWORD	dwEntries;
	/* list of delegation handlers. */
	QUARTZ_IFDelegation*	pDelegationFirst;
	/* called on final release. */
	void (*pOnFinalRelease)(IUnknown* punk);

	/* IUnknown fields. */
	LONG	ref;
	IUnknown*	punkControl;
} QUARTZ_IUnkImpl;

#include <poppack.h>


void QUARTZ_IUnkInit( QUARTZ_IUnkImpl* pImpl, IUnknown* punkOuter );
void QUARTZ_IUnkAddDelegation(
	QUARTZ_IUnkImpl* pImpl, QUARTZ_IFDelegation* pDelegation );


#endif	/* WINE_DSHOW_IUNK_H */
