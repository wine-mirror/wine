/*
 *        Tasks functions
 */
static char Copyright[] = "Copyright  Martin Ayotte, 1994";

/*
#define DEBUG_TASK
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "windows.h"
#include "wine.h"
#include "task.h"

static LPWINETASKENTRY lpTaskList = NULL;
static int nTaskCount = 0;

/**********************************************************************
 *				GetCurrentTask	[KERNEL.36]
 */
HTASK GetCurrentTask()
{
	LPWINETASKENTRY lpTask = lpTaskList;
	int pid = getpid();
#ifdef DEBUG_TASK
	printf("GetCurrentTask() // unix_pid=%08X !\n", pid);
#endif
	if (lpTask == NULL) return 0;
	while (TRUE) {
		if (lpTask->unix_pid == pid) break;
		if (lpTask->lpNextTask == NULL) return 0;
		lpTask = lpTask->lpNextTask;
		}
#ifdef DEBUG_TASK
	printf("GetCurrentTask() returned hTask=%04X !\n", lpTask->te.hTask);
#endif
	return lpTask->te.hTask;
}


/**********************************************************************
 *				GetNumTasks	[KERNEL.152]
 */
WORD GetNumTasks()
{
	printf("GetNumTasks() returned %d !\n", nTaskCount);
	return nTaskCount;
}


/**********************************************************************
 *				GetWindowTask	[USER.224]
 */
HTASK GetWindowTask(HWND hWnd)
{
	HWND 	*wptr;
	int		count;
	LPWINETASKENTRY lpTask = lpTaskList;
	printf("GetWindowTask(%04X) !\n", hWnd);
	while (lpTask != NULL) {
		wptr = lpTask->lpWndList;
		if (wptr != NULL) {
			count = 0;
			while (++count < MAXWIN_PER_TASK) {
				printf("GetWindowTask // searching %04X %04X !\n",
										lpTask->te.hTask, *(wptr));
				if (*(wptr) == hWnd) {
					printf("GetWindowTask(%04X) found hTask=%04X !\n", 
												hWnd, lpTask->te.hTask);
					return lpTask->te.hTask;
					}
				wptr++;
				}
			}
		lpTask = lpTask->lpNextTask;
		}
	return 0;
}


/**********************************************************************
 *				EnumTaskWindows	[USER.225]
 */
BOOL EnumTaskWindows(HANDLE hTask, FARPROC lpEnumFunc, LONG lParam)
{
	HWND 	*wptr, hWnd;
	BOOL	bRet;
	int		count = 0;
	LPWINETASKENTRY lpTask = lpTaskList;
	printf("EnumTaskWindows(%04X, %08X, %08X) !\n", hTask, lpEnumFunc, lParam);
	while (TRUE) {
		if (lpTask->te.hTask == hTask) break;
		if (lpTask == NULL) {
			printf("EnumTaskWindows // hTask=%04X not found !\n", hTask);
			return FALSE;
			}
		lpTask = lpTask->lpNextTask;
		}
	printf("EnumTaskWindows // found hTask=%04X !\n", hTask);
	wptr = lpTask->lpWndList;
	if (wptr == NULL) return FALSE;
	if (lpEnumFunc == NULL)	return FALSE;
	while ((hWnd = *(wptr++)) != 0) {
		if (++count >= MAXWIN_PER_TASK) return FALSE;
		printf("EnumTaskWindows // hWnd=%04X count=%d !\n", hWnd, count);
#ifdef WINELIB
		bRet = (*lpEnumFunc)(hWnd, lParam); 
#else
		bRet = CallBack16(lpEnumFunc, 2, 0, (int)hWnd, 2, (int)lParam);
#endif
		if (bRet == 0) break;
		}
	return TRUE;
}


/**********************************************************************
 *				CreateNewTask		[internal]
 */
HANDLE CreateNewTask(HINSTANCE hInst, HTASK hTaskParent)
{
	HANDLE hTask;
	LPWINETASKENTRY lpTask = lpTaskList;
	LPWINETASKENTRY lpNewTask;
	MODULEENTRY module;

	module.dwSize = sizeof(module);
	ModuleFindHandle(&module, hInst);

	if (lpTask != NULL) {
		while (TRUE) {
			if (lpTask->lpNextTask == NULL) break;
			lpTask = lpTask->lpNextTask;
			}
		}
	hTask = GlobalAlloc(GMEM_MOVEABLE, sizeof(WINETASKENTRY));
	lpNewTask = (LPWINETASKENTRY) GlobalLock(hTask);
#ifdef DEBUG_TASK
    printf("CreateNewTask entry allocated %08X\n", lpNewTask);
#endif
	if (lpNewTask == NULL) return 0;
	if (lpTaskList == NULL) {
		lpTaskList = lpNewTask;
		lpNewTask->lpPrevTask = NULL;
		}
	else {
		lpTask->lpNextTask = lpNewTask;
		lpTask->te.hNext = lpNewTask->te.hTask;
		lpNewTask->lpPrevTask = lpTask;
		}
	lpNewTask->lpNextTask = NULL;
	lpNewTask->hIcon = 0;
	lpNewTask->te.dwSize = sizeof(TASKENTRY);
	lpNewTask->te.hModule = 0;
	lpNewTask->te.hInst = hInst;
	lpNewTask->te.hTask = hTask;
	lpNewTask->te.hTaskParent = hTaskParent;
	lpNewTask->te.wSS = 0;
	lpNewTask->te.wSP = 0;
	lpNewTask->te.wStackTop = 0;
	lpNewTask->te.wStackMinimum = 0;
	lpNewTask->te.wStackBottom = 0;
	lpNewTask->te.wcEvents = 0;
	lpNewTask->te.hQueue = 0;
	strcpy(lpNewTask->te.szModule, module.szModule);
	lpNewTask->te.wPSPOffset = 0;
	lpNewTask->unix_pid = getpid();
	lpNewTask->lpWndList = (HWND *) malloc(MAXWIN_PER_TASK * sizeof(HWND));
	if (lpNewTask->lpWndList != NULL) 
		memset((LPSTR)lpNewTask->lpWndList, 0, MAXWIN_PER_TASK * sizeof(HWND));
#ifdef DEBUG_TASK
    printf("CreateNewTask // unix_pid=%08X return hTask=%04X\n", 
									lpNewTask->unix_pid, hTask);
#endif
	GlobalUnlock(hTask);	
	nTaskCount++;
    return hTask;
}


/**********************************************************************
 *				AddWindowToTask		[internal]
 */
BOOL AddWindowToTask(HTASK hTask, HWND hWnd)
{
	HWND 	*wptr;
	int		count = 0;
	LPWINETASKENTRY lpTask = lpTaskList;
#ifdef DEBUG_TASK
	printf("AddWindowToTask(%04X, %04X); !\n", hTask, hWnd);
#endif
	while (TRUE) {
		if (lpTask->te.hTask == hTask) break;
		if (lpTask == NULL) {
			printf("AddWindowToTask // hTask=%04X not found !\n", hTask);
			return FALSE;
			}
		lpTask = lpTask->lpNextTask;
		}
	wptr = lpTask->lpWndList;
	if (wptr == NULL) return FALSE;
	while (*(wptr) != 0) {
		if (++count >= MAXWIN_PER_TASK) return FALSE;
		wptr++;
		}
	*wptr = hWnd;
#ifdef DEBUG_TASK
	printf("AddWindowToTask // window added, count=%d !\n", count);
#endif
	return TRUE;
}


/**********************************************************************
 *				RemoveWindowFromTask		[internal]
 */
BOOL RemoveWindowFromTask(HTASK hTask, HWND hWnd)
{
	HWND 	*wptr;
	int		count = 0;
	LPWINETASKENTRY lpTask = lpTaskList;
#ifdef DEBUG_TASK
	printf("RemoveWindowToTask(%04X, %04X); !\n", hTask, hWnd);
#endif
	while (TRUE) {
		if (lpTask->te.hTask == hTask) break;
		if (lpTask == NULL) {
			printf("RemoveWindowFromTask // hTask=%04X not found !\n", hTask);
			return FALSE;
			}
		lpTask = lpTask->lpNextTask;
		}
	wptr = lpTask->lpWndList;
	if (wptr == NULL) return FALSE;
	while (*(wptr) != hWnd) {
		if (++count >= MAXWIN_PER_TASK) return FALSE;
		wptr++;
		}
	while (*(wptr) != 0) {
		*(wptr) = *(wptr + 1);
		if (++count >= MAXWIN_PER_TASK) return FALSE;
		wptr++;
		}
#ifdef DEBUG_TASK
	printf("RemoveWindowFromTask // window removed, count=%d !\n", --count);
#endif
	return TRUE;
}

BOOL TaskFirst(LPTASKENTRY lpTask)
{
	printf("TaskFirst(%8x)\n", (int) lpTask);

	if (lpTaskList) {
		memcpy(lpTask, &lpTaskList->te, lpTask->dwSize);
		return TRUE;
	} else
		return FALSE;
}

BOOL TaskNext(LPTASKENTRY lpTask)
{
	LPWINETASKENTRY list;
	
	printf("TaskNext(%8x)\n", (int) lpTask);

	list = lpTaskList;
	while (list) {
		if (list->te.hTask == lpTask->hTask) {
			list = list->lpNextTask;
			if (list) {
				memcpy(lpTask, &list->te, lpTask->dwSize);
				return TRUE;
			} else
				return FALSE;
		}
		list = list->lpNextTask;
	}
	return FALSE;
}

BOOL TaskFindHandle(LPTASKENTRY lpTask, HTASK hTask)
{
	static LPWINETASKENTRY list;
	
	printf("TaskFindHandle(%8x,%4x)\n", (int) lpTask, hTask);

	list = lpTaskList;
	while (list) {
		if (list->te.hTask == hTask) {
			list = list->lpNextTask;
			if (list) {
				memcpy(lpTask, &list->te, lpTask->dwSize);
				return TRUE;
			} else
				return FALSE;
		}
		list = list->lpNextTask;
	}
	return FALSE;
}
