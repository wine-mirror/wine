#ifndef _CHARLIST
#define _CHARLIST

typedef struct _tagCHARLISTENTRY
{
    struct _tagCHARLISTENTRY *pNext;
    char   myChar;
} CHARLISTENTRY;

typedef struct _tagCHARLIST
{
    unsigned int nCount; // Entries Count;
    CHARLISTENTRY *pHead;
    CHARLISTENTRY *pTail;
} CHARLIST;


void CHARLIST_Enqueue( CHARLIST* pCharList, char myChar);
void CHARLIST_Push( CHARLIST* pCharList, char myChar);
char CHARLIST_Dequeue(CHARLIST* pCharList);
int CHARLIST_GetNbItems(CHARLIST* pCharList);
void CHARLIST_FreeList(CHARLIST* pCharList);
int CHARLIST_CountChar(CHARLIST* pCharList, char myChar);
int CHARLIST_toBuffer(CHARLIST* pCharList, char* pBuffer, int nBufferSize);

#endif