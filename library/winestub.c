/* Sample winestub.c file for compiling programs with libwine.so. */

#include <string.h>
#include <stdio.h>
#include "windows.h"
#include "xmalloc.h"

/* Stub needed for linking with Winelib */
/* FIXME: this should not be necessary */
HMODULE32 BUILTIN_LoadModule( LPCSTR name, BOOL32 force ) { 
	fprintf(stderr,"BUILTIN_LoadModule(%s,%d) called in a library!\n",name,force);
	return 0;
}

extern int PASCAL WinMain(HINSTANCE32,HINSTANCE32,LPSTR,int);
extern BOOL32 MAIN_WinelibInit( int *argc, char *argv[] );
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

  if (!MAIN_WinelibInit( &argc, argv )) return 0;

  /* Alloc szCmdParam */
  for (i = 1; i < argc; i++) len += strlen(argv[i]) + 1;
  lpszCmdParam = (LPSTR) xmalloc(len + 1);
  /* Concatenate arguments */
  if (argc > 1) strcpy(lpszCmdParam, argv[1]);
  else lpszCmdParam[0] = '\0';
  for (i = 2; i < argc; i++) strcat(strcat(lpszCmdParam, " "), argv[i]);

  hInstance = WinExec32( *argv, SW_SHOWNORMAL );
  TASK_Reschedule();
  InitApp( hInstance );

  return WinMain (hInstance,    /* hInstance */
		  0,	        /* hPrevInstance */
		  lpszCmdParam, /* lpszCmdParam */
		  SW_NORMAL);   /* nCmdShow */
}
