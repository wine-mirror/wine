/*
 *        Tasks functions
 */
static char Copyright[] = "Copyright  Martin Ayotte, 1994";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "windows.h"
#include "wine.h"
#include "task.h"

static LPTASKENTRY lpTaskList = NULL;
static int nTaskCount = 0;



/**********************************************************************
 *				GetCurrentTask	[KERNEL.36]
 */
HTASK GetCurrentTask()
{
	LPTASKENTRY lpTask = lpTaskList;
	int pid = getpid();
	printf("GetCurrentTask() // unix_pid=%08X !\n", pid);
	if (lpTask == NULL) return 0;
	while (TRUE) {
		printf("GetCurrentTask() // searching lpTask->unix_pid=%08 !\n", lpTask->unix_pid);
		if (lpTask->unix_pid == pid) break;
		if (lpTask->lpNextTask == NULL) return 0;
		lpTask = lpTask->lpNextTask;
		}
	printf("GetCurrentTask() returned hTask=%04X !\n", lpTask->hTask);
	return lpTask->hTask;
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
	printf("GetWindowTask(%04X) !\n", hWnd);
	return 0;
}


/**********************************************************************
 *				EnumTaskWindows	[USER.225]
 */
BOOL EnumTaskWindows(HANDLE hTask, FARPROC lpEnumFunc, LONG lParam)
{
	printf("EnumTaskWindows(%04X, %08X, %08X) !\n", hTask, lpEnumFunc, lParam);
	return FALSE;
}


/**********************************************************************
 *				CreateNewTask		[internal]
 */
HANDLE CreateNewTask(HINSTANCE hInst)
{
    HANDLE hTask;
	LPTASKENTRY lpTask = lpTaskList;
	LPTASKENTRY lpNewTask;
	if (lpTask != NULL) {
		while (TRUE) {
			if (lpTask->lpNextTask == NULL) break;
			lpTask = lpTask->lpNextTask;
			}
		}
	hTask = GlobalAlloc(GMEM_MOVEABLE, sizeof(TASKENTRY));
	lpNewTask = (LPTASKENTRY) GlobalLock(hTask);
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
		lpNewTask->lpPrevTask = lpTask;
		}
	lpNewTask->lpNextTask = NULL;
	lpNewTask->hIcon = 0;
	lpNewTask->hModule = 0;
	lpNewTask->hTask = hTask;
	lpNewTask->lpWndList = (HWND *) malloc(MAXWIN_PER_TASK * sizeof(HWND));
	if (lpNewTask->lpWndList != NULL) 
		memset((LPSTR)lpNewTask->lpWndList, 0, MAXWIN_PER_TASK * sizeof(HWND));
	lpNewTask->hInst = hInst;
	lpNewTask->unix_pid = getpid();
    printf("CreateNewTask // unix_pid=%08X return hTask=%04X\n", 
									lpNewTask->unix_pid, hTask);
	GlobalUnlock(hTask);	
    return hTask;
}


/**********************************************************************
 *				AddWindowToTask		[internal]
 */
BOOL AddWindowToTask(HTASK hTask, HWND hWnd)
{
	HWND 	*wptr;
	int		count = 0;
	LPTASKENTRY lpTask = lpTaskList;
	printf("AddWindowToTask(%04X, %04X); !\n", hTask, hWnd);
	while (TRUE) {
		if (lpTask->hTask == hTask) break;
		if (lpTask == NULL) return FALSE;
		lpTask = lpTask->lpNextTask;
		}
	wptr = lpTask->lpWndList;
	if (wptr == NULL) return FALSE;
	while (*(wptr++) != 0) {
		if (++count >= MAXWIN_PER_TASK) return FALSE;
		}
	*wptr = hWnd;
	return TRUE;
}


/**********************************************************************
 *				RemoveWindowFromTask		[internal]
 */
BOOL RemoveWindowFromTask(HTASK hTask, HWND hWnd)
{
	HWND 	*wptr;
	int		count = 0;
	LPTASKENTRY lpTask = lpTaskList;
	printf("AddWindowToTask(%04X, %04X); !\n", hTask, hWnd);
	while (TRUE) {
		if (lpTask->hTask == hTask) break;
		if (lpTask == NULL) return FALSE;
		lpTask = lpTask->lpNextTask;
		}
	wptr = lpTask->lpWndList;
	if (wptr == NULL) return FALSE;
	while (*(wptr++) != hWnd) {
		if (++count >= MAXWIN_PER_TASK) return FALSE;
		}
	*wptr = 0;
	return TRUE;
}


