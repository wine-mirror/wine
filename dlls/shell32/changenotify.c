/*
 *	shell change notification
 *
 *	Juergen Schmied <juergen.schmied@debitel.de>
 *
 */

#include <string.h>

#include "debugtools.h"
#include "pidl.h"
#include "shell32_main.h"
#include "winversion.h"
#include "wine/undocshell.h"

DEFAULT_DEBUG_CHANNEL(shell)

static CRITICAL_SECTION SHELL32_ChangenotifyCS;

/* internal list of notification clients (internal) */
typedef struct _NOTIFICATIONLIST
{
	struct _NOTIFICATIONLIST *next;
	struct _NOTIFICATIONLIST *prev; 
	HWND hwnd;		/* window to notify */
	DWORD uMsg;		/* message to send */
	LPNOTIFYREGISTER apidl; /* array of entrys to watch*/
	UINT cidl;		/* number of pidls in array */
	LONG wEventMask;	/* subscribed events */
	DWORD dwFlags;		/* client flags */
} NOTIFICATIONLIST, *LPNOTIFICATIONLIST;

NOTIFICATIONLIST head;
NOTIFICATIONLIST tail;

void InitChangeNotifications()
{
	TRACE("head=%p tail=%p\n", &head, &tail);

	InitializeCriticalSection(&SHELL32_ChangenotifyCS);
	MakeCriticalSectionGlobal(&SHELL32_ChangenotifyCS);
	ZeroMemory(&head, sizeof(NOTIFICATIONLIST));
	ZeroMemory(&tail, sizeof(NOTIFICATIONLIST));
	head.next = &tail;
	tail.prev = &head;
}

void FreeChangeNotifications()
{
	LPNOTIFICATIONLIST ptr, item;

	TRACE("\n");

	EnterCriticalSection(&SHELL32_ChangenotifyCS);
	ptr = head.next;

	while(ptr != &tail)
	{ 
	  int i;
	  item = ptr;
	  ptr = ptr->next;

	  TRACE("item=%p\n", item);
	  
	  /* free the item */
	  for (i=0; i<item->cidl;i++) SHFree(item->apidl[i].pidlPath);
	  SHFree(item->apidl);
	  SHFree(item);
	}
	head.next = NULL;
	tail.prev = NULL;

	LeaveCriticalSection(&SHELL32_ChangenotifyCS);

	DeleteCriticalSection(&SHELL32_ChangenotifyCS);
}

static BOOL AddNode(LPNOTIFICATIONLIST item)
{
	LPNOTIFICATIONLIST last;
	
	EnterCriticalSection(&SHELL32_ChangenotifyCS);

	/* get last entry */
	last = tail.prev;

	/* link items */
	last->next = item;
	item->prev = last;
	item->next = &tail;
	tail.prev = item;
	TRACE("item=%p prev=%p next=%p\n", item, item->prev, item->next);

	LeaveCriticalSection(&SHELL32_ChangenotifyCS);

	return TRUE;
}

static BOOL DeleteNode(LPNOTIFICATIONLIST item)
{
	LPNOTIFICATIONLIST ptr;
	int ret = FALSE;

	TRACE("item=%p\n", item);

	EnterCriticalSection(&SHELL32_ChangenotifyCS);

	ptr = head.next;
	while((ptr != &tail) && (ret == FALSE))
	{ 
	  TRACE("ptr=%p\n", ptr);
	  
	  if (ptr == item)
	  {
	    int i;
	    
	    TRACE("item=%p prev=%p next=%p\n", item, item->prev, item->next);

	    /* remove item from list */
	    item->prev->next = item->next;
	    item->next->prev = item->prev;

	    /* free the item */
	    for (i=0; i<item->cidl;i++) SHFree(item->apidl[i].pidlPath);
	    SHFree(item->apidl);
	    SHFree(item);
	    ret = TRUE;
	  }
	  ptr = ptr->next;
	}
	
	LeaveCriticalSection(&SHELL32_ChangenotifyCS);
	return ret;
	
}

/*************************************************************************
 * SHChangeNotifyRegister			[SHELL32.2]
 *
 */
HANDLE WINAPI
SHChangeNotifyRegister(
    HWND hwnd,
    LONG dwFlags,
    LONG wEventMask,
    DWORD uMsg,
    int cItems,
    LPCNOTIFYREGISTER lpItems)
{
	LPNOTIFICATIONLIST item;
	int i;

	item = SHAlloc(sizeof(NOTIFICATIONLIST));

	TRACE("(0x%04x,0x%08lx,0x%08lx,0x%08lx,0x%08x,%p) item=%p\n",
		hwnd,dwFlags,wEventMask,uMsg,cItems,lpItems,item);
	
	item->next = NULL;
	item->prev = NULL;
	item->cidl = cItems;
	item->apidl = SHAlloc(sizeof(NOTIFYREGISTER) * cItems);
	for(i=0;i<cItems;i++)
	{
	  item->apidl[i].pidlPath = ILClone(lpItems[i].pidlPath);
	  item->apidl[i].bWatchSubtree = lpItems[i].bWatchSubtree;
	}
	item->hwnd = hwnd;
	item->uMsg = uMsg;
	item->wEventMask = wEventMask;
	item->dwFlags = dwFlags;
	AddNode(item);
	return (HANDLE)item;
}

/*************************************************************************
 * SHChangeNotifyDeregister			[SHELL32.4]
 */
BOOL WINAPI
SHChangeNotifyDeregister(
	HANDLE hNotify)
{
	TRACE("(0x%08x)\n",hNotify);

	return DeleteNode((LPNOTIFICATIONLIST)hNotify);;
}

/*************************************************************************
 * SHChangeNotify				[SHELL32.239]
 */
void WINAPI SHChangeNotifyW (LONG wEventId, UINT  uFlags, LPCVOID dwItem1, LPCVOID dwItem2)
{
	LPITEMIDLIST pidl1=(LPITEMIDLIST)dwItem1, pidl2=(LPITEMIDLIST)dwItem2;
	LPNOTIFICATIONLIST ptr;
	
	TRACE("(0x%08lx,0x%08x,%p,%p):stub.\n", wEventId,uFlags,dwItem1,dwItem2);

	/* convert paths in IDLists*/
	if(uFlags & SHCNF_PATHA)
	{
	  DWORD dummy;
	  if (dwItem1) SHILCreateFromPathA((LPCSTR)dwItem1, &pidl1, &dummy);
	  if (dwItem2) SHILCreateFromPathA((LPCSTR)dwItem2, &pidl2, &dummy);
	}
	
	EnterCriticalSection(&SHELL32_ChangenotifyCS);
	
	/* loop through the list */
	ptr = head.next;
	while(ptr != &tail)
	{
	  TRACE("trying %p\n", ptr);
	  
	  if(wEventId & ptr->wEventMask)
	  {
	    TRACE("notifying\n");
	    SendMessageA(ptr->hwnd, ptr->uMsg, (WPARAM)pidl1, (LPARAM)pidl2);
	  }
	  ptr = ptr->next;
	}
	
	LeaveCriticalSection(&SHELL32_ChangenotifyCS);

	if(uFlags & SHCNF_PATHA)
	{
            if (pidl1) SHFree(pidl1);
            if (pidl2) SHFree(pidl2);
	}
}

/*************************************************************************
 * SHChangeNotify				[SHELL32.239]
 */
void WINAPI SHChangeNotifyA (LONG wEventId, UINT  uFlags, LPCVOID dwItem1, LPCVOID dwItem2)
{
	LPITEMIDLIST Pidls[2];
	LPNOTIFICATIONLIST ptr;
	
	Pidls[0] = (LPITEMIDLIST)dwItem1;
	Pidls[1] = (LPITEMIDLIST)dwItem2;

	TRACE("(0x%08lx,0x%08x,%p,%p):stub.\n", wEventId,uFlags,dwItem1,dwItem2);

	/* convert paths in IDLists*/
	if(uFlags & SHCNF_PATHA)
	{
	  DWORD dummy;
	  if (Pidls[0]) SHILCreateFromPathA((LPCSTR)dwItem1, &Pidls[0], &dummy);
	  if (Pidls[1]) SHILCreateFromPathA((LPCSTR)dwItem2, &Pidls[1], &dummy);
	}
	
	EnterCriticalSection(&SHELL32_ChangenotifyCS);
	
	/* loop through the list */
	ptr = head.next;
	while(ptr != &tail)
	{
	  TRACE("trying %p\n", ptr);
	  
	  if(wEventId & ptr->wEventMask)
	  {
	    TRACE("notifying\n");
	    SendMessageA(ptr->hwnd, ptr->uMsg, (WPARAM)&Pidls, (LPARAM)wEventId);
	  }
	  ptr = ptr->next;
	}
	
	LeaveCriticalSection(&SHELL32_ChangenotifyCS);

	/* if we allocated it, free it */
	if(uFlags & SHCNF_PATHA)
	{
            if (Pidls[0]) SHFree(Pidls[0]);
            if (Pidls[1]) SHFree(Pidls[1]);
	}
}

/*************************************************************************
 * SHChangeNotifyAW				[SHELL32.239]
 */
void WINAPI SHChangeNotifyAW (LONG wEventId, UINT  uFlags, LPCVOID dwItem1, LPCVOID dwItem2)
{
	if(VERSION_OsIsUnicode())
	  SHChangeNotifyW (wEventId, uFlags, dwItem1, dwItem2);
	else
	  SHChangeNotifyA (wEventId, uFlags, dwItem1, dwItem2);
}

/*************************************************************************
 * NTSHChangeNotifyRegister			[SHELL32.640]
 * NOTES
 *   Idlist is an array of structures and Count specifies how many items in the array
 *   (usually just one I think).
 */
DWORD WINAPI NTSHChangeNotifyRegister(
    HWND hwnd,
    LONG events1,
    LONG events2,
    DWORD msg,
    int count,
    LPNOTIFYREGISTER idlist)
{
	FIXME("(0x%04x,0x%08lx,0x%08lx,0x%08lx,0x%08x,%p):stub.\n",
		hwnd,events1,events2,msg,count,idlist);
	return 0;
}

/*************************************************************************
 * SHChangeNotification_Lock			[SHELL32.644]
 */
HANDLE WINAPI SHChangeNotification_Lock(
	HANDLE hMemoryMap,
	DWORD dwProcessId,
	LPCITEMIDLIST **lppidls,
	LPLONG lpwEventId)
{
	FIXME("\n");
	return 0;
}

/*************************************************************************
 * SHChangeNotification_Unlock			[SHELL32.645]
 */
BOOL WINAPI SHChangeNotification_Unlock (
	HANDLE hLock)
{
	FIXME("\n");
	return 0;
}

/*************************************************************************
 * NTSHChangeNotifyDeregister			[SHELL32.641]
 */
DWORD WINAPI NTSHChangeNotifyDeregister(LONG x1)
{
	FIXME("(0x%08lx):stub.\n",x1);
	return 0;
}

