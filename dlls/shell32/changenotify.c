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

typedef struct
{
	LPCITEMIDLIST pidlPath;
	BOOL bWatchSubtree;
} NOTIFYREGISTER, *LPNOTIFYREGISTER;

typedef const LPNOTIFYREGISTER LPCNOTIFYREGISTER;

typedef struct
{
	USHORT	cb;
	DWORD	dwItem1;
	DWORD	dwItem2;
} DWORDITEMID;


#define SHCNF_ACCEPT_INTERRUPTS		0x0001
#define SHCNF_ACCEPT_NON_INTERRUPTS	0x0002
#define SHCNF_NO_PROXY			0x8001

/* internal list of notification clients */
typedef struct _NOTIFICATIONLIST
{
	struct _NOTIFICATIONLIST *next;
	struct _NOTIFICATIONLIST *prev; 
	HWND hwnd;		/* window to notify */
	DWORD uMsg;		/* message to send */
	LPNOTIFYREGISTER * apidl; /* array of entrys to watch*/
	UINT cidl;		/* number of pidls in array */
	LONG wEventMask;	/* subscribed events */
	DWORD dwFlags;		/* client flags */
} NOTIFICATIONLIST, *LPNOTIFICATIONLIST;

NOTIFICATIONLIST head;
NOTIFICATIONLIST tail;

void InitChangeNotifications()
{
	ZeroMemory(&head, sizeof(NOTIFICATIONLIST));
	ZeroMemory(&tail, sizeof(NOTIFICATIONLIST));
	head.next = &tail;
	tail.prev = &head;
}

void FreeChangeNotifications()
{
	LPNOTIFICATIONLIST ptr, item;

	ptr = head.next;

	while(ptr != &tail)
	{ 
	  int i;
	  item = ptr;
	  ptr = ptr->next;

	  /* free the item */
	  for (i=0; i<item->cidl;i++) SHFree(item->apidl[i]);
	  SHFree(item->apidl);
	  SHFree(item);
	}
	head.next = NULL;
	tail.prev = NULL;
}

static BOOL AddNode(LPNOTIFICATIONLIST item)
{
	LPNOTIFICATIONLIST last;
	
	/* lock list */

	/* get last entry */
	last = tail.prev;

	/* link items */
	last->next = item;
	item->prev = last;
	item->next = &tail;
	tail.prev = item;
	return TRUE;

	/* unlock list */
}

static BOOL DeleteNode(LPNOTIFICATIONLIST item)
{
	LPNOTIFICATIONLIST ptr;
	int ret = FALSE;

	/* lock list */

	ptr = head.next;
	while(ptr != &tail)
	{ 
	  if (ptr == item)
	  {
	    int i;
	    
	    /* remove item from list */
	    item->prev->next = item->next;
	    item->next->prev = item->prev;

	    /* free the item */
	    for (i=0; i<item->cidl;i++) SHFree(item->apidl[i]);
	    SHFree(item->apidl);
	    SHFree(item);
	    ret = TRUE;
	  }
	}
	
	/* unlock list */
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
    LPCNOTIFYREGISTER *lpItems)
{
	LPNOTIFICATIONLIST item;
	int i;

	TRACE("(0x%04x,0x%08lx,0x%08lx,0x%08lx,0x%08x,%p)\n",
		hwnd,wEventMask,wEventMask,uMsg,cItems,lpItems);
	
	item = SHAlloc(sizeof(NOTIFICATIONLIST));
	item->next = NULL;
	item->prev = NULL;
	item->cidl = cItems;
	item->apidl = SHAlloc(sizeof(NOTIFYREGISTER) * cItems);
	for(i=0;i<cItems;i++)
	{
	  item->apidl[i]->pidlPath = ILClone(lpItems[i]->pidlPath);
	  item->apidl[i]->bWatchSubtree = lpItems[i]->bWatchSubtree;
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
	FIXME("(0x%08lx,0x%08x,%p,%p):stub.\n", wEventId,uFlags,dwItem1,dwItem2);
}

void WINAPI SHChangeNotifyA (LONG wEventId, UINT  uFlags, LPCVOID dwItem1, LPCVOID dwItem2)
{
	FIXME("(0x%08lx,0x%08x,%p,%p):stub.\n", wEventId,uFlags,dwItem1,dwItem2);
}

void WINAPI SHChangeNotifyAW (LONG wEventId, UINT  uFlags, LPCVOID dwItem1, LPCVOID dwItem2)
{
	if(VERSION_OsIsUnicode())
	  return SHChangeNotifyW (wEventId, uFlags, dwItem1, dwItem2);
	return SHChangeNotifyA (wEventId, uFlags, dwItem1, dwItem2);
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

