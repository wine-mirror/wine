/*
 * 
 *  Character List
 * 
 *  Copyright (c) 2000 by Jean-Claude Batista
 *  
 */

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>
#include <stdlib.h>
#include "charlist.h"
#include "windows.h"

extern HANDLE RICHED32_hHeap;

void CHARLIST_Enqueue( CHARLIST* pCharList, char myChar )
{   
    CHARLISTENTRY* pNewEntry = HeapAlloc(RICHED32_hHeap, 0,sizeof(CHARLISTENTRY));
    pNewEntry->pNext = NULL;
    pNewEntry->myChar = myChar;
    
    if(pCharList->pTail == NULL)
    {
         pCharList->pHead = pCharList->pTail = pNewEntry;
    }
    else
    {
         CHARLISTENTRY* pCurrent = pCharList->pTail;         
         pCharList->pTail = pCurrent->pNext = pNewEntry;
    }

    pCharList->nCount++;
}

void CHARLIST_Push( CHARLIST* pCharList, char myChar)
{   
    CHARLISTENTRY* pNewEntry = malloc(sizeof(CHARLISTENTRY));
    
    pNewEntry->myChar = myChar;
    
    if(pCharList->pHead == NULL)
    {
         pCharList->pHead = pCharList->pTail = pNewEntry;
         pNewEntry->pNext = NULL;

    }
    else
    {
         pNewEntry->pNext = pCharList->pHead;
         pCharList->pHead = pNewEntry;
    }

    pCharList->nCount++;
}

char CHARLIST_Dequeue(CHARLIST* pCharList)
{
    CHARLISTENTRY* pCurrent;
    char myChar;

    if(pCharList->nCount == 0) 
      return 0;
    
    pCharList->nCount--;
    myChar = pCharList->pHead->myChar;
    pCurrent = pCharList->pHead->pNext;
    HeapFree(RICHED32_hHeap, 0,pCharList->pHead);
 
    if(pCharList->nCount == 0)
    {
        pCharList->pHead = pCharList->pTail = NULL;
    }
    else
    {
        pCharList->pHead = pCurrent;
    }

    return myChar;   
}

int CHARLIST_GetNbItems(CHARLIST* pCharList)
{
    return pCharList->nCount;
}

void CHARLIST_FreeList(CHARLIST* pCharList){
    while(pCharList->nCount)
        CHARLIST_Dequeue(pCharList);       
}

/* this function count the number of occurences of a caracter */
int CHARLIST_CountChar(CHARLIST* pCharList, char myChar)
{
    CHARLISTENTRY *pCurrent;
    int nCount = 0;
    
    for(pCurrent =pCharList->pHead ;pCurrent;pCurrent=pCurrent->pNext)
    	if(pCurrent->myChar == myChar)
	    nCount++;
    
    return nCount;
}

int CHARLIST_toBuffer(CHARLIST* pCharList, char* pBuffer, int nBufferSize)
{
   
   /* we add one to store a NULL caracter */
   if(nBufferSize < pCharList->nCount + 1) 
        return pCharList->nCount;
  
   for(;pCharList->nCount;pBuffer++)
       *pBuffer = CHARLIST_Dequeue(pCharList);
   
   *pBuffer = '\0';

   return 0;
}

