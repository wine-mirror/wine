#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "windows.h"
#include "xmalloc.h"

extern int MAIN_Init(void);
extern BOOL WIDGETS_Init(void);
extern BOOL WIN_CreateDesktopWindow(void);
extern int WinMain(HINSTANCE,HINSTANCE,LPSTR,int);
extern void TASK_Reschedule(void);
extern int USER_InitApp(HINSTANCE);


int _WinMain (int argc, char *argv [])
{
  HINSTANCE hInstance;
  LPSTR lpszCmdParam;
  int i, len = 0;

  /* Alloc szCmdParam */
  for (i = 1; i < argc; i++) len += strlen(argv[i]) + 1;
  lpszCmdParam = (LPSTR) xmalloc(len + 1);
  /* Concatenate arguments */
  if (argc > 1) strcpy(lpszCmdParam, argv[1]);
  else lpszCmdParam[0] = '\0';
  for (i = 2; i < argc; i++) strcat(strcat(lpszCmdParam, " "), argv[i]);

  if(!MAIN_Init()) return 0; /* JBP: Needed for DosDrives[] structure, etc. */
  hInstance = WinExec( *argv, SW_SHOWNORMAL );
  TASK_Reschedule();
  USER_InitApp( hInstance );

#ifdef WINELIBDLL
  return (int)hInstance;
#else
  return WinMain (hInstance,    /* hInstance */
		  0,	        /* hPrevInstance */
		  lpszCmdParam, /* lpszCmdParam */
		  SW_NORMAL);   /* nCmdShow */
#endif
}
