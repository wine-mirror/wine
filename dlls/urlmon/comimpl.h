/*
 * A basic implementation for COM DLL implementor.
 *
 * Copyright (C) 2002 Hidenori TAKESHIMA <hidenori@a2.ctktv.ne.jp>
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

#ifndef WINE_COMIMPL_H
#define WINE_COMIMPL_H

/*
	This code provides a basic thread-safe COM implementation
	including aggregation(and an IClassFactory implementation).
	This code is based on my quartz code.

    The usage of this library is:

	1) copy comimpl.h and comimpl.c.
	2) implement the global class entries 'COMIMPL_ClassList'.
	3) export COMIMPL_DllMain, COMIMPL_DllGetClassObject and
	   COMIMPL_DllCanUnloadNow.
	4) add 'comimpl' to your debug channels.
	5) implement your IUnknown as a thunk to call
	   COMIMPL_IUnkImpl.punk methods.
	6) provide pointers of vtables in constructor.
	7) optionally, you can edit comimpl.h and/or comimpl.c.

	A sample allocator of class COne that supports
	two interface IOne and ITwo is:

	const COMIMPL_CLASSENTRY COMIMPL_ClassList[] =
	{
	  { &CLSID_COne, &COne_AllocObj },
	  { NULL, NULL } << the last entry must be NULL >>
	};

	typedef struct COne
	{
	  COMIMPL_IUnkImpl vfunk; << must be the first member of this struct >>
	  struct { ICOM_VFIELD(IOne); } vfone;
	  struct { ICOM_VFIELD(ITwo); } vftwo;
	  << ... >>
	} COne;
	#define COne_THIS(iface,member) COne* This = ((COne*)(((char*)iface)-offsetof(COne,member)))

	static COMIMPL_IFEntry IFEntries[] =
	{
	    << all supported interfaces >>
	  { &IID_IOne, offsetof(COne,vfone)-offsetof(COne,vfunk) },
	  { &IID_ITwo, offsetof(COne,vftwo)-offsetof(COne,vfunk) },
	};

	static void COne_Destructor(IUnknown* iface)
	{
	    COne_THIS(iface,vfunk);

	    << ... implement destructor ... >>
	}

	HRESULT COne_AllocObj(IUnknown* punkOuter,void** ppobj)
	{
	    COne* This;

	    This = (COne*)COMIMPL_AllocObj( sizeof(COne) );
	    if ( This == NULL ) return E_OUTOFMEMORY;
	    COMIMPL_IUnkInit( &This->vfunk, punkOuter );
	    This->vfunk.pEntries = IFEntries;
	    This->vfunk.dwEntries = sizeof(IFEntries)/sizeof(IFEntries[0]);
	    This->vfunk.pOnFinalRelease = COne_Destructor;

	    ICOM_VTBL(&This->vfone) = &ione;
	    ICOM_VTBL(&This->vftwo) = &itwo;

	    << ... implement constructor ... >>
	    << if error occurs in constructing, you can call simply
	       punk::Release() and return HRESULT. >>

	    *ppobj = (void*)(&This->vfunk);

	    return S_OK; << return S_OK if succeeded >>
	}
 */

/* if per-thread initialization is needed, uncomment the following line */
/*#define COMIMPL_PERTHREAD_INIT*/
/* If an own heap is needed, customize the following line */
#define COMIMPL_hHeap	GetProcessHeap()


typedef HRESULT (*COMIMPL_pCreateIUnknown)(IUnknown* punkOuter,void** ppobj);
typedef void (*COMIMPL_pOnFinalRelease)(IUnknown* punkInner);

typedef struct COMIMPL_IFEntry COMIMPL_IFEntry;
typedef struct COMIMPL_IFDelegation COMIMPL_IFDelegation;
typedef struct COMIMPL_IUnkImpl COMIMPL_IUnkImpl;

struct COMIMPL_IFEntry
{
	const IID*	piid;		/* interface ID. */
	size_t		ofsVTPtr;	/* offset from IUnknown. */
};

struct COMIMPL_IFDelegation
{
	struct COMIMPL_IFDelegation*	pNext;
	HRESULT (*pOnQueryInterface)(
		IUnknown* punk, const IID* piid, void** ppobj );
};

/* COMIMPL_IUnkimpl must be aligned for InterlockedExchangeAdd. */
#include <pshpack4.h>

struct COMIMPL_IUnkImpl
{
	/* pointer of IUnknown interface. */
	ICOM_VFIELD(IUnknown);

	/* array of supported IIDs and offsets. */
	const COMIMPL_IFEntry*	pEntries;
	DWORD	dwEntries;
	/* list of delegation handlers. */
	COMIMPL_IFDelegation*	pDelegationFirst;
	/* called on final release. */
	COMIMPL_pOnFinalRelease	pOnFinalRelease;

	/* IUnknown fields. */
	LONG	ref;
	IUnknown*	punkControl;
};

#include <poppack.h>

typedef struct COMIMPL_CLASSENTRY
{
	const CLSID*	pclsid;
	COMIMPL_pCreateIUnknown	pCreateIUnk;
} COMIMPL_CLASSENTRY;


extern const COMIMPL_CLASSENTRY COMIMPL_ClassList[]; /* must be provided  */

void COMIMPL_IUnkInit( COMIMPL_IUnkImpl* pImpl, IUnknown* punkOuter );
void COMIMPL_IUnkAddDelegationHandler(
	COMIMPL_IUnkImpl* pImpl, COMIMPL_IFDelegation* pDelegation );

BOOL WINAPI COMIMPL_DllMain(
	HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpvReserved );
HRESULT WINAPI COMIMPL_DllGetClassObject(
		const CLSID* pclsid,const IID* piid,void** ppv);
HRESULT WINAPI COMIMPL_DllCanUnloadNow(void);

void* COMIMPL_AllocObj( DWORD dwSize );
void COMIMPL_FreeObj( void* pobj ); /* for internal use only. */




#endif	/* WINE_COMIMPL_H */
