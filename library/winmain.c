#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "windows.h"
#include "wine.h"

extern int MAIN_Init(void);
extern BOOL WIDGETS_Init(void);
extern BOOL WIN_CreateDesktopWindow(void);
extern int PASCAL WinMain(HINSTANCE,HINSTANCE,LPSTR,int);
extern void TASK_Reschedule(void);
extern int USER_InitApp(HINSTANCE);

char* progname=NULL;

int _WinMain (int argc, char *argv [])
{
  HINSTANCE hInstance;

  progname=*argv;
  if(!MAIN_Init()) return 0; /* JBP: Needed for DosDrives[] structure, etc. */
  hInstance = WinExec( *argv, SW_SHOWNORMAL );
  TASK_Reschedule();
  USER_InitApp( hInstance );
  /* Perform global initialisations that need a task context */
  if (!WIDGETS_Init()) return -1;
  if (!WIN_CreateDesktopWindow()) return -1;

  return WinMain (hInstance,    /* hInstance */
		  0,	        /* hPrevInstance */
		  "",	        /* lpszCmdParam */
		  SW_NORMAL);   /* nCmdShow */
}
