/*
 * List of components. (for internal use)
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

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "complist.h"



struct QUARTZ_CompList
{
	QUARTZ_CompListItem*	pFirst;
	QUARTZ_CompListItem*	pLast;
};

struct QUARTZ_CompListItem
{
	IUnknown*	punk;
	QUARTZ_CompListItem*	pNext;
	QUARTZ_CompListItem*	pPrev;
};


QUARTZ_CompList* QUARTZ_CompList_Alloc( void )
{
	QUARTZ_CompList*	pList;

	pList = (QUARTZ_CompList*)QUARTZ_AllocMem( sizeof(QUARTZ_CompList) );
	if ( pList != NULL )
	{
		/* construct. */
		pList->pFirst = NULL;
		pList->pLast = NULL;
	}

	return pList;
}

void QUARTZ_CompList_Free( QUARTZ_CompList* pList )
{
	QUARTZ_CompListItem*	pCur;
	QUARTZ_CompListItem*	pNext;

	if ( pList != NULL )
	{
		pCur = pList->pFirst;
		while ( pCur != NULL )
		{
			pNext = pCur->pNext;
			if ( pCur->punk != NULL )
				IUnknown_Release( pCur->punk );
			QUARTZ_FreeMem( pCur );
			pCur = pNext;
		}
		QUARTZ_FreeMem( pList );
	}
}

QUARTZ_CompList* QUARTZ_CompList_Dup( QUARTZ_CompList* pList )
{
	QUARTZ_CompList*	pNewList;
	QUARTZ_CompListItem*	pCur;
	HRESULT	hr;

	pNewList = QUARTZ_CompList_Alloc();
	if ( pNewList == NULL )
		return NULL;

	pCur = pList->pFirst;
	while ( pCur != NULL )
	{
		if ( pCur->punk != NULL )
		{
			hr = QUARTZ_CompList_AddComp( pNewList, pCur->punk );
			if ( FAILED(hr) )
			{
				QUARTZ_CompList_Free( pNewList );
				return NULL;
			}
		}
		pCur = pCur->pNext;
	}

	return pNewList;
}

HRESULT QUARTZ_CompList_AddComp( QUARTZ_CompList* pList, IUnknown* punk )
{
	QUARTZ_CompListItem*	pItem;

	pItem = (QUARTZ_CompListItem*)QUARTZ_AllocMem( sizeof(QUARTZ_CompListItem) );
	if ( pItem == NULL )
		return E_OUTOFMEMORY; /* out of memory. */
	pItem->punk = punk; IUnknown_AddRef(punk);

	if ( pList->pFirst != NULL )
		pList->pFirst->pPrev = pItem;
	else
		pList->pLast = pItem;
	pList->pFirst = pItem;
	pItem->pNext = pList->pFirst;
	pItem->pPrev = NULL;

	return S_OK;
}

HRESULT QUARTZ_CompList_RemoveComp( QUARTZ_CompList* pList, IUnknown* punk )
{
	QUARTZ_CompListItem*	pCur;

	pCur = QUARTZ_CompList_SearchComp( pList, punk );
	if ( pCur == NULL )
		return S_FALSE; /* already removed. */

	/* remove from list. */
	if ( pCur->pNext != NULL )
		pCur->pNext->pPrev = pCur->pPrev;
	else
		pList->pLast = pCur->pPrev;
	if ( pCur->pPrev != NULL )
		pCur->pPrev->pNext = pCur->pNext;
	else
		pList->pFirst = pCur->pNext;

	/* release this item. */
	if ( pCur->punk != NULL )
		IUnknown_Release( pCur->punk );
	QUARTZ_FreeMem( pCur );

	return S_OK;
}

QUARTZ_CompListItem* QUARTZ_CompList_SearchComp(
	QUARTZ_CompList* pList, IUnknown* punk )
{
	QUARTZ_CompListItem*	pCur;

	pCur = pList->pFirst;
	while ( pCur != NULL )
	{
		if ( pCur->punk == punk )
			return pCur;
		pCur = pCur->pNext;
	}

	return NULL;
}

QUARTZ_CompListItem* QUARTZ_CompList_GetFirst(
	QUARTZ_CompList* pList )
{
	return pList->pFirst;
}

QUARTZ_CompListItem* QUARTZ_CompList_GetNext(
	QUARTZ_CompList* pList, QUARTZ_CompListItem* pPrev )
{
	return pPrev->pNext;
}

IUnknown* QUARTZ_CompList_GetItemPtr( QUARTZ_CompListItem* pItem )
{
	return pItem->punk;
}
