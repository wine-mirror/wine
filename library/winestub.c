/* Sample winestub.c file for compiling programs with libwine.so. */

#include "windows.h"
#ifdef WIN_DEBUG
#include <stdio.h>
#endif

extern int PASCAL WinMain(HINSTANCE,HINSTANCE,LPSTR,int);
/* This is the renamed main() subroutine in misc/main.c. */
/* Note that the libdll `init()' won't work: */
extern HINSTANCE _wine_main(int, char *[]);

int main (int argc, char *argv [])
{
  HINSTANCE hInstance;
  char szCmdParam[256] = {0};
  int index, buffer_pos;
  char *arg_holder;
  /* "Wired-in" command-line options for Wine: */
  /*char *wine_argv[] = {argv[0], "-managed", "-dll", "+commdlg",
  			"-dll", "+shell", ""};*/
  char *wine_argv[] = {argv[0], ""};

  /* Initialize the library dll: */
  hInstance = (HINSTANCE)_wine_main((sizeof(wine_argv)/sizeof(char *))-1, wine_argv);

#ifdef WIN_DEBUG
  fprintf(stderr,"In winestub, reporting hInstance = %d\n", hInstance);
#endif

  /* Move things from argv to the szCmdParam that WinMain expects: */
    for (index = 1, buffer_pos = 0;
  	(index < argc) && (buffer_pos < 255);
  	index++, buffer_pos++)
  	{
	for (arg_holder = argv[index]; ; buffer_pos++, arg_holder++)
	 if ((szCmdParam[buffer_pos] = *arg_holder) == '\0') break;
	szCmdParam[buffer_pos] = ' ';
	};
  szCmdParam[buffer_pos] = '\0';

#ifdef WIN_DEBUG
  fprintf(stderr,"In winestub, Program Name: %s, Parameters: %s\n",
  	progname, szCmdParam);
#endif

  return WinMain (hInstance,		/* hInstance */
		  0,			/* hPrevInstance */
		  (LPSTR)szCmdParam,	/* lpszCmdParam */
		  SW_NORMAL);		/* nCmdShow */
}
