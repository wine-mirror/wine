/*
 *	IMAGEHLP library
 *
 *	Copyright 1998	Patrik Stridvall
 */

#include "winbase.h"
#include "winerror.h"
#include "windef.h"
#include "debugtools.h"
#include "imagehlp.h"

/***********************************************************************
 *		InitializeListHead
 */
VOID InitializeListHead(PLIST_ENTRY pListHead)
{
  pListHead->Flink = pListHead;
  pListHead->Blink = pListHead;
}

/***********************************************************************
 *		InsertHeadList
 */
VOID InsertHeadList(PLIST_ENTRY pListHead, PLIST_ENTRY pEntry)
{
  pEntry->Blink = pListHead;
  pEntry->Flink = pListHead->Flink;
  pListHead->Flink = pEntry;
}

/***********************************************************************
 *		InsertTailList
 */
VOID InsertTailList(PLIST_ENTRY pListHead, PLIST_ENTRY pEntry)
{
  pEntry->Flink = pListHead;
  pEntry->Blink = pListHead->Blink;
  pListHead->Blink = pEntry;
}

/***********************************************************************
 *		IsListEmpty
 */
BOOLEAN IsListEmpty(PLIST_ENTRY pListHead)
{
  return !pListHead;
}

/***********************************************************************
 *		PopEntryList
 */
PSINGLE_LIST_ENTRY PopEntryList(PSINGLE_LIST_ENTRY pListHead)
{
  pListHead->Next = NULL;
  return pListHead;
}

/***********************************************************************
 *		PushEntryList
 */
VOID PushEntryList(
  PSINGLE_LIST_ENTRY pListHead, PSINGLE_LIST_ENTRY pEntry)
{
  pEntry->Next=pListHead;
}

/***********************************************************************
 *		RemoveEntryList
 */
VOID RemoveEntryList(PLIST_ENTRY pEntry)
{
  pEntry->Flink->Blink = pEntry->Blink;
  pEntry->Blink->Flink = pEntry->Flink;
  pEntry->Flink = NULL;
  pEntry->Blink = NULL;
}

/***********************************************************************
 *		RemoveHeadList
 */
PLIST_ENTRY RemoveHeadList(PLIST_ENTRY pListHead)
{
  PLIST_ENTRY p = pListHead->Flink;
  
  if(p != pListHead)
    {
      RemoveEntryList(pListHead); 
      return p;
    }
  else
    {
      pListHead->Flink = NULL;
      pListHead->Blink = NULL;
      return NULL;
    }
}

/***********************************************************************
 *		RemoveTailList
 */
PLIST_ENTRY RemoveTailList(PLIST_ENTRY pListHead)
{
  RemoveHeadList(pListHead->Blink);
  if(pListHead != pListHead->Blink)
    return pListHead;
  else
    return NULL;
}
