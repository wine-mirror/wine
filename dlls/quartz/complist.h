/*
 * List of components. (for internal use)
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

#ifndef	QUARTZ_COMPLIST_H
#define	QUARTZ_COMPLIST_H

typedef struct QUARTZ_CompList	QUARTZ_CompList;
typedef struct QUARTZ_CompListItem	QUARTZ_CompListItem;

QUARTZ_CompList* QUARTZ_CompList_Alloc( void );
void QUARTZ_CompList_Free( QUARTZ_CompList* pList );
void QUARTZ_CompList_Lock( QUARTZ_CompList* pList );
void QUARTZ_CompList_Unlock( QUARTZ_CompList* pList );

QUARTZ_CompList* QUARTZ_CompList_Dup(
	const QUARTZ_CompList* pList, BOOL fDupData );
HRESULT QUARTZ_CompList_AddComp(
	QUARTZ_CompList* pList, IUnknown* punk,
	const void* pvData, DWORD dwDataLen );
HRESULT QUARTZ_CompList_AddTailComp(
	QUARTZ_CompList* pList, IUnknown* punk,
	const void* pvData, DWORD dwDataLen );
HRESULT QUARTZ_CompList_RemoveComp( QUARTZ_CompList* pList, IUnknown* punk );
QUARTZ_CompListItem* QUARTZ_CompList_SearchComp(
	QUARTZ_CompList* pList, IUnknown* punk );
QUARTZ_CompListItem* QUARTZ_CompList_SearchData(
	QUARTZ_CompList* pList, const void* pvData, DWORD dwDataLen );
QUARTZ_CompListItem* QUARTZ_CompList_GetFirst(
	QUARTZ_CompList* pList );
QUARTZ_CompListItem* QUARTZ_CompList_GetLast(
	QUARTZ_CompList* pList );
QUARTZ_CompListItem* QUARTZ_CompList_GetNext(
	QUARTZ_CompList* pList, QUARTZ_CompListItem* pPrev );
QUARTZ_CompListItem* QUARTZ_CompList_GetPrev(
	QUARTZ_CompList* pList, QUARTZ_CompListItem* pNext );
IUnknown* QUARTZ_CompList_GetItemPtr( QUARTZ_CompListItem* pItem );
const void* QUARTZ_CompList_GetDataPtr( QUARTZ_CompListItem* pItem );
DWORD QUARTZ_CompList_GetDataLength( QUARTZ_CompListItem* pItem );


#endif	/* QUARTZ_COMPLIST_H */
