/*
 *	shell change notification
 *
 * Copyright 2000 Juergen Schmied
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

#include <string.h>

#include "wine/debug.h"
#include "pidl.h"
#include "shell32_main.h"
#include "undocshell.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

static CRITICAL_SECTION SHELL32_ChangenotifyCS = CRITICAL_SECTION_INIT("SHELL32_ChangenotifyCS");

/* internal list of notification clients (internal) */
typedef struct _NOTIFICATIONLIST
{
	struct _NOTIFICATIONLIST *next;
	struct _NOTIFICATIONLIST *prev;
	HWND hwnd;		/* window to notify */
	DWORD uMsg;		/* message to send */
	LPNOTIFYREGISTER apidl; /* array of entries to watch*/
	UINT cidl;		/* number of pidls in array */
	LONG wEventMask;	/* subscribed events */
	DWORD dwFlags;		/* client flags */
} NOTIFICATIONLIST, *LPNOTIFICATIONLIST;

static NOTIFICATIONLIST head;
static NOTIFICATIONLIST tail;

void InitChangeNotifications()
{
	TRACE("head=%p tail=%p\n", &head, &tail);
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
	  UINT i;
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
	while(ptr != &tail)
	{
	  TRACE("ptr=%p\n", ptr);

	  if (ptr == item)
	  {
	    UINT i;

	    TRACE("item=%p prev=%p next=%p\n", item, item->prev, item->next);

	    /* remove item from list */
	    item->prev->next = item->next;
	    item->next->prev = item->prev;

	    /* free the item */
	    for (i=0; i<item->cidl;i++) SHFree(item->apidl[i].pidlPath);
	    SHFree(item->apidl);
	    SHFree(item);

	    ret = TRUE;
	    break;
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

	TRACE("(%p,0x%08lx,0x%08lx,0x%08lx,0x%08x,%p) item=%p\n",
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
SHChangeNotifyDeregister(HANDLE hNotify)
{
	TRACE("(%p)\n",hNotify);

	return DeleteNode((LPNOTIFICATIONLIST)hNotify);
}

/*************************************************************************
 * SHChangeNotifyUpdateEntryList       		[SHELL32.5]
 */
BOOL WINAPI
SHChangeNotifyUpdateEntryList(DWORD unknown1, DWORD unknown2,
			      DWORD unknown3, DWORD unknown4)
{
	FIXME("(0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx)\n",
	      unknown1, unknown2, unknown3, unknown4);

	return -1;
}

/*************************************************************************
 * SHChangeNotify				[SHELL32.@]
 */
void WINAPI SHChangeNotify(LONG wEventId, UINT uFlags, LPCVOID dwItem1, LPCVOID dwItem2)
{
	LPITEMIDLIST Pidls[2];
	LPNOTIFICATIONLIST ptr;
	DWORD dummy;
	UINT typeFlag = uFlags & SHCNF_TYPE;

	Pidls[0] = (LPITEMIDLIST)dwItem1;
	Pidls[1] = (LPITEMIDLIST)dwItem2;

	TRACE("(0x%08lx,0x%08x,%p,%p):stub.\n", wEventId, uFlags, dwItem1, dwItem2);

	/* convert paths in IDLists*/
	switch (typeFlag)
	{
	  case SHCNF_PATHA:
	    if (dwItem1) SHILCreateFromPathA((LPCSTR)dwItem1, &Pidls[0], &dummy);
	    if (dwItem2) SHILCreateFromPathA((LPCSTR)dwItem2, &Pidls[1], &dummy);
	    break;
	  case SHCNF_PATHW:
	    if (dwItem1) SHILCreateFromPathW((LPCWSTR)dwItem1, &Pidls[0], &dummy);
	    if (dwItem2) SHILCreateFromPathW((LPCWSTR)dwItem2, &Pidls[1], &dummy);
	    break;
	  case SHCNF_PRINTERA:
	  case SHCNF_PRINTERW:
	    FIXME("SHChangeNotify with (uFlags & SHCNF_PRINTER)");
	    break;
	}

	EnterCriticalSection(&SHELL32_ChangenotifyCS);

	/* loop through the list */
	ptr = head.next;
	while (ptr != &tail)
	{
	  TRACE("trying %p\n", ptr);

	  if (wEventId & ptr->wEventMask)
	  {
	    TRACE("notifying\n");
	    SendMessageA(ptr->hwnd, ptr->uMsg, (WPARAM)&Pidls, (LPARAM)wEventId);
	  }
	  ptr = ptr->next;
	}

	LeaveCriticalSection(&SHELL32_ChangenotifyCS);

	/* if we allocated it, free it */
	if ((typeFlag == SHCNF_PATHA) || (typeFlag == SHCNF_PATHW))
	{
	  if (Pidls[0]) SHFree(Pidls[0]);
	  if (Pidls[1]) SHFree(Pidls[1]);
	}
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
	FIXME("(%p,0x%08lx,0x%08lx,0x%08lx,0x%08x,%p):stub.\n",
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
