/* Sample winestub.c file for compiling programs with libwine.so. */

#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "debugtools.h"

extern int PASCAL WinMain(HINSTANCE,HINSTANCE,LPSTR,int);

/* external declaration here because we don't want to depend on Wine headers */
#ifdef __cplusplus
extern "C" HINSTANCE MAIN_WinelibInit( int *argc, char *argv[] );
#else
extern HINSTANCE MAIN_WinelibInit( int *argc, char *argv[] );
#endif

/* Most Windows C/C++ compilers use something like this to */
/* access argc and argv globally: */
int _ARGC;
char **_ARGV;

int main( int argc, char *argv [] )
{
  HINSTANCE hInstance;
  LPSTR lpszCmdParam;
  int i, len = 0, retv;
  _ARGC = argc;
  _ARGV = (char **)argv;

  if (!(hInstance = MAIN_WinelibInit( &argc, argv ))) return 0;

  /* Alloc szCmdParam */
  for (i = 1; i < argc; i++) len += strlen(argv[i]) + 1;
  lpszCmdParam = (LPSTR) malloc(len + 1);
  if(lpszCmdParam == NULL) {
    MESSAGE("Not enough memory to store command parameters!");
    return 1;
  }
  /* Concatenate arguments */
  if (argc > 1) strcpy(lpszCmdParam, argv[1]);
  else lpszCmdParam[0] = '\0';
  for (i = 2; i < argc; i++) strcat(strcat(lpszCmdParam, " "), argv[i]);

  retv = WinMain (hInstance,    /* hInstance */
		  0,	        /* hPrevInstance */
		  lpszCmdParam, /* lpszCmdParam */
		  SW_NORMAL);   /* nCmdShow */

  ExitProcess( retv );
  return retv;
}
