/*
 * 				Shell Library definitions
 */
#include "wintypes.h"

#ifndef __WINE_SHELL_H
#define __WINE_SHELL_H

#include "windows.h"
#include "winreg.h"

extern INT ShellAbout(HWND hWnd, LPCSTR szApp, LPCSTR szOtherStuff, HICON hIcon);
extern void SHELL_LoadRegistry();
extern void SHELL_SaveRegistry();
extern void SHELL_Init();

#define SHELL_ERROR_SUCCESS           0L
#define SHELL_ERROR_BADDB             1L
#define SHELL_ERROR_BADKEY            2L
#define SHELL_ERROR_CANTOPEN          3L
#define SHELL_ERROR_CANTREAD          4L
#define SHELL_ERROR_CANTWRITE         5L
#define SHELL_ERROR_OUTOFMEMORY       6L
#define SHELL_ERROR_INVALID_PARAMETER 7L
#define SHELL_ERROR_ACCESS_DENIED     8L

typedef struct { 	   /* structure for dropped files */ 
	WORD		wSize;
	POINT		ptMousePos;   
	BOOL		fInNonClientArea;
	/* memory block with filenames follows */     
} DROPFILESTRUCT, *LPDROPFILESTRUCT; 

#define SE_ERR_SHARE            26
#define SE_ERR_ASSOCINCOMPLETE  27
#define SE_ERR_DDETIMEOUT       28
#define SE_ERR_DDEFAIL          29
#define SE_ERR_DDEBUSY          30
#define SE_ERR_NOASSOC          31

LRESULT AboutDlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam);

#endif  /* __WINE_SHELL_H */
