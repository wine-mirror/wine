/* Sample winestub.c file for compiling programs with libwine.so. */

#include <string.h>
#include "windows.h"
#include "xmalloc.h"

extern int PASCAL WinMain(HINSTANCE32,HINSTANCE32,LPSTR,int);
extern int MAIN_Init(void);
extern BOOL32 MAIN_WineInit( int *argc, char *argv[] );
extern void TASK_Reschedule(void);

/* Most Windows C/C++ compilers use something like this to */
/* access argc and argv globally: */
int _ARGC;
char **_ARGV;

int main( int argc, char *argv [] )
{
  HINSTANCE16 hInstance;
  LPSTR lpszCmdParam;
  int i, len = 0;
  _ARGC = argc;
  _ARGV = (char **)argv;

  MAIN_WineInit( &argc, argv );

  /* Alloc szCmdParam */
  for (i = 1; i < argc; i++) len += strlen(argv[i]) + 1;
  lpszCmdParam = (LPSTR) xmalloc(len + 1);
  /* Concatenate arguments */
  if (argc > 1) strcpy(lpszCmdParam, argv[1]);
  else lpszCmdParam[0] = '\0';
  for (i = 2; i < argc; i++) strcat(strcat(lpszCmdParam, " "), argv[i]);

  if(!MAIN_Init()) return 0; /* JBP: Needed for DosDrives[] structure, etc. */
  hInstance = WinExec32( *argv, SW_SHOWNORMAL );
  TASK_Reschedule();
  InitApp( hInstance );

  return WinMain (hInstance,    /* hInstance */
		  0,	        /* hPrevInstance */
		  lpszCmdParam, /* lpszCmdParam */
		  SW_NORMAL);   /* nCmdShow */
}
