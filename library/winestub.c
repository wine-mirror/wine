/* Sample winestub.c file for compiling programs with libwine.so. */

#include <string.h>
#include "windows.h"
#include "xmalloc.h"

extern int WinMain(HINSTANCE32,HINSTANCE32,LPSTR,int);
extern int MAIN_Init(void);
extern BOOL32 MAIN_WineInit( int *argc, char *argv[] );
extern void TASK_Reschedule(void);

int main( int argc, char *argv [] )
{
  HINSTANCE16 hInstance;
  LPSTR lpszCmdParam;
  int i, len = 0;

  MAIN_WineInit( &argc, argv );

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
  InitApp( hInstance );

  return WinMain (hInstance,    /* hInstance */
		  0,	        /* hPrevInstance */
		  lpszCmdParam, /* lpszCmdParam */
		  SW_NORMAL);   /* nCmdShow */
}

#if 0

extern int WinMain(HINSTANCE32,HINSTANCE32,LPSTR,int);
/* This is the renamed main() subroutine in misc/main.c. */
/* Note that the libdll `init()' won't work: */
extern HINSTANCE32 _wine_main(int, char *[]);

int main (int argc, char *argv [])
{
  HINSTANCE32 hInstance;
  char szCmdParam[256] = {0};
  int index, buffer_pos;
  char *arg_holder;
  /* "Wired-in" command-line options for Wine: */
  /*char *wine_argv[] = {argv[0], "-managed", "-dll", "+commdlg",
  			"-dll", "+shell", ""};*/
  char *wine_argv[] = {argv[0], ""};

  /* Initialize the library dll: */
  hInstance = (HINSTANCE32)_wine_main((sizeof(wine_argv)/sizeof(char *))-1, wine_argv);

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
#endif
