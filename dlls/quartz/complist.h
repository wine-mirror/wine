/*
 * List of components. (for internal use)
 *
 * hidenori@a2.ctktv.ne.jp
 */

#ifndef	QUARTZ_COMPLIST_H
#define	QUARTZ_COMPLIST_H

typedef struct QUARTZ_CompList	QUARTZ_CompList;
typedef struct QUARTZ_CompListItem	QUARTZ_CompListItem;

QUARTZ_CompList* QUARTZ_CompList_Alloc( void );
void QUARTZ_CompList_Free( QUARTZ_CompList* pList );
QUARTZ_CompList* QUARTZ_CompList_Dup( QUARTZ_CompList* pList );
HRESULT QUARTZ_CompList_AddComp( QUARTZ_CompList* pList, IUnknown* punk );
HRESULT QUARTZ_CompList_RemoveComp( QUARTZ_CompList* pList, IUnknown* punk );
QUARTZ_CompListItem* QUARTZ_CompList_SearchComp(
	QUARTZ_CompList* pList, IUnknown* punk );
QUARTZ_CompListItem* QUARTZ_CompList_GetFirst(
	QUARTZ_CompList* pList );
QUARTZ_CompListItem* QUARTZ_CompList_GetNext(
	QUARTZ_CompList* pList, QUARTZ_CompListItem* pPrev );
IUnknown* QUARTZ_CompList_GetItemPtr( QUARTZ_CompListItem* pItem );


#endif	/* QUARTZ_COMPLIST_H */
