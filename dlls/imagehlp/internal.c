/*
 *	IMAGEHLP library
 *
 *	Copyright 1998	Patrik Stridvall
 */

#include "winbase.h"
#include "winerror.h"
#include "wintypes.h"
#include "debug.h"
#include "imagehlp.h"

/***********************************************************************
 *           InitializeListHead32
 */
VOID InitializeListHead32(PLIST_ENTRY32 pListHead)
{
  pListHead->Flink = pListHead;
  pListHead->Blink = pListHead;
}

/***********************************************************************
 *           InsertHeadList32
 */
VOID InsertHeadList32(PLIST_ENTRY32 pListHead, PLIST_ENTRY32 pEntry)
{
  pEntry->Blink = pListHead;
  pEntry->Flink = pListHead->Flink;
  pListHead->Flink = pEntry;
}

/***********************************************************************
 *           InsertTailList32
 */
VOID InsertTailList32(PLIST_ENTRY32 pListHead, PLIST_ENTRY32 pEntry)
{
  pEntry->Flink = pListHead;
  pEntry->Blink = pListHead->Blink;
  pListHead->Blink = pEntry;
}

/***********************************************************************
 *           IsListEmpty32
 */
BOOLEAN IsListEmpty32(PLIST_ENTRY32 pListHead)
{
  return !pListHead;
}

/***********************************************************************
 *           PopEntryList32
 */
PSINGLE_LIST_ENTRY32 PopEntryList32(PSINGLE_LIST_ENTRY32 pListHead)
{
  pListHead->Next = NULL;
  return pListHead;
}

/***********************************************************************
 *           PushEntryList32
 */
VOID PushEntryList32(
  PSINGLE_LIST_ENTRY32 pListHead, PSINGLE_LIST_ENTRY32 pEntry)
{
  pEntry->Next=pListHead;
}

/***********************************************************************
 *           RemoveEntryList32
 */
VOID RemoveEntryList32(PLIST_ENTRY32 pEntry)
{
  pEntry->Flink->Blink = pEntry->Blink;
  pEntry->Blink->Flink = pEntry->Flink;
  pEntry->Flink = NULL;
  pEntry->Blink = NULL;
}

/***********************************************************************
 *           RemoveHeadList32
 */
PLIST_ENTRY32 RemoveHeadList32(PLIST_ENTRY32 pListHead)
{
  PLIST_ENTRY32 p = pListHead->Flink;
  
  if(p != pListHead)
    {
      RemoveEntryList32(pListHead); 
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
 *           RemoveTailList32
 */
PLIST_ENTRY32 RemoveTailList32(PLIST_ENTRY32 pListHead)
{
  RemoveHeadList32(pListHead->Blink);
  if(pListHead != pListHead->Blink)
    return pListHead;
  else
    return NULL;
}




