/*
 * Task definitions
 */

#ifndef TASK_H
#define TASK_H

typedef struct {
	HANDLE		hTask;
	HANDLE		hModule;
	HINSTANCE	hInst;
	int			unix_pid;
	HICON		hIcon;
	HWND		*lpWndList;
	void		*lpPrevTask;
	void		*lpNextTask;
} TASKENTRY;
typedef TASKENTRY *LPTASKENTRY;

#define MAXWIN_PER_TASK  256

HANDLE CreateNewTask(HINSTANCE hInst);
BOOL RemoveWindowFromTask(HTASK hTask, HWND hWnd);
BOOL AddWindowToTask(HTASK hTask, HWND hWnd);

#endif /* TASK_H */

