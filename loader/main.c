/*
static char RCSId[] = "$Id: wine.c,v 1.2 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "neexe.h"
#include "dos_fs.h"
#include "dlls.h"
#include "library.h"
#include "windows.h"
#include "wineopts.h"
#include "wine.h"
#include "task.h"
#include "prototypes.h"
#include "options.h"
#include "if1632.h"
#include "ne_image.h"
#include "pe_image.h"
#include "stddebug.h"
#include "debug.h"

char **Argv;
int Argc;
HINSTANCE hSysRes, hInstMain;
unsigned short WIN_StackSize;

/**********************************************************************
 *					myerror
 */
void
myerror(const char *s)
{
    if (s == NULL)
	perror("wine");
    else
	fprintf(stderr, "wine: %s\n", s);

    exit(1);
}


/***********************************************************************
 *           Main initialisation routine
 */
int MAIN_Init(void)
{
    int queueSize;

    SpyInit();

      /* Initialize relay code */
    if (!RELAY_Init()) return 0;

      /* Create built-in modules */
    if (!MODULE_Init()) return 0;

      /* Initialize tasks */
    if (!TASK_Init()) return 0;

      /* Initialize the DOS file system */
    DOS_InitFS();

      /* Initialize communications */
    COMM_Init();

#ifndef WINELIB    
      /* Initialize the DOS memory */
    INT21_Init();

      /* Create USER heap */
    if (!USER_HeapInit()) return 0;
#endif
    
      /* Global atom table initialisation */
    if (!ATOM_Init()) return 0;
    
      /* GDI initialisation */
    if (!GDI_Init()) return 0;

      /* Initialize system colors and metrics*/
    SYSMETRICS_Init();
    SYSCOLOR_Init();

      /* Create the DCEs */
    DCE_Init();
    
      /* Initialize built-in window classes */
    if (!WIDGETS_Init()) return 0;

      /* Initialize dialog manager */
    if (!DIALOG_Init()) return 0;

      /* Initialize menus */
    if (!MENU_Init()) return 0;

      /* Create desktop window */
    if (!WIN_CreateDesktopWindow()) return 0;
    if (!DESKTOP_Init()) return 0;

      /* Create system message queue */
    queueSize = GetProfileInt( "windows", "TypeAhead", 120 );
    if (!MSG_CreateSysMsgQueue( queueSize )) return 0;

    return 1;
}


#ifndef WINELIB
/**********************************************************************
 *					main
 */
int _WinMain(int argc, char **argv)
{
    char *p, filename[256];
    int i;

    struct w_files *wpnt;
#ifdef WINESTAT
    char * cp;
#endif

    if (!MAIN_Init()) return 0;

	Argc = argc - 1;
	Argv = argv + 1;

	if (strchr(Argv[0], '\\') || strchr(Argv[0],'/')) {
            for (p = Argv[0] + strlen(Argv[0]); *p != '\\' && *p !='/'; p--)
		/* NOTHING */;
		
	    strncpy(filename, Argv[0], p - Argv[0]);
	    filename[p - Argv[0]] = '\0';
	    strcat(WindowsPath, ";");
	    if (strchr(filename, '/'))
		    strcat(WindowsPath, DOS_GetDosFileName(filename));
	    else
	    	    strcat(WindowsPath, filename);
	}

	for (i = 0; i < Argc; i++)
        {
            if ((hInstMain = LoadImage(Argv[i], EXE, 1)) < 32) {
		fprintf(stderr, "wine: can't load %s!.\n", Argv[i]);
		exit(1);
            }
        }

	GetPrivateProfileString("wine", "SystemResources", "sysres.dll", 
				filename, sizeof(filename), WINE_INI);

	hSysRes = LoadImage(filename, DLL, 0);
	if (hSysRes < 32) {
		fprintf(stderr, "wine: can't load %s!.\n", filename);
		exit(1);
	} else
 	    dprintf_dll(stddeb,"System Resources Loaded // hSysRes='%04X'\n",
			hSysRes);
	

#ifdef WINESTAT
    cp = strrchr(argv[0], '/');
    if(!cp) cp = argv[0];
	else cp++;
    if(strcmp(cp,"winestat") == 0) {
	    winestat();
	    exit(0);
    };
#endif

    /*
     * Initialize signal handling.
     */
    init_wine_signals();
        
    wpnt = GetFileInfo(hInstMain);
    if (Options.debug)
	wine_debug(0, NULL);

    Yield();  /* Start the first task */
    fprintf( stderr, "WinMain: Should never happen: returned from Yield()\n" );
    return 0;
}

#endif /* #ifndef WINELIB */
