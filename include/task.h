/*
 * Task definitions
 */

#ifndef TASK_H
#define TASK_H

#include "toolhelp.h"

typedef struct {
	TASKENTRY	te;
	int		unix_pid;
	HICON		hIcon;
	HWND		*lpWndList;
	void		*lpPrevTask;
	void		*lpNextTask;
} WINETASKENTRY;
typedef WINETASKENTRY *LPWINETASKENTRY;

#define MAXWIN_PER_TASK  256

HANDLE CreateNewTask(HINSTANCE hInst, HTASK hTaskParent);
BOOL RemoveWindowFromTask(HTASK hTask, HWND hWnd);
BOOL AddWindowToTask(HTASK hTask, HWND hWnd);

#endif /* TASK_H */

