/* Sample winestub.c file for compiling programs with libwine.so. */

#include <string.h>
#include "windows.h"
#include "xmalloc.h"

extern int PASCAL WinMain(HINSTANCE32,HINSTANCE32,LPSTR,int);

/* external declaration here because we don't want to depend on Wine headers */
extern HINSTANCE32 MAIN_WinelibInit( int *argc, char *argv[] );

/* Most Windows C/C++ compilers use something like this to */
/* access argc and argv globally: */
int _ARGC;
char **_ARGV;

int main( int argc, char *argv [] )
{
  HINSTANCE32 hInstance;
  LPSTR lpszCmdParam;
  int i, len = 0;
  _ARGC = argc;
  _ARGV = (char **)argv;

  if (!(hInstance = MAIN_WinelibInit( &argc, argv ))) return 0;

  /* Alloc szCmdParam */
  for (i = 1; i < argc; i++) len += strlen(argv[i]) + 1;
  lpszCmdParam = (LPSTR) xmalloc(len + 1);
  /* Concatenate arguments */
  if (argc > 1) strcpy(lpszCmdParam, argv[1]);
  else lpszCmdParam[0] = '\0';
  for (i = 2; i < argc; i++) strcat(strcat(lpszCmdParam, " "), argv[i]);

  return WinMain (hInstance,    /* hInstance */
		  0,	        /* hPrevInstance */
		  lpszCmdParam, /* lpszCmdParam */
		  SW_NORMAL);   /* nCmdShow */
}
