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
#include "windows.h"
#include "wineopts.h"
#include "wine.h"
#include "task.h"
#include "options.h"
#include "pe_image.h"
#include "stddebug.h"
#include "debug.h"


/***********************************************************************
 *           Main initialisation routine
 */
int MAIN_Init(void)
{
    extern BOOL RELAY_Init(void);

    int queueSize;

    SpyInit();

      /* Initialize relay code */
    if (!RELAY_Init()) return 0;

      /* Initialize Win32 relay code */
    if (!RELAY32_Init()) return 0;

      /* Create built-in modules */
    if (!MODULE_Init()) return 0;

      /* Initialize tasks */
    if (!TASK_Init()) return 0;

      /* Initialize the DOS file system */
    DOS_InitFS();

      /* Create DOS environment */
    CreateSelectors();

      /* Initialize signal handling */
    init_wine_signals();

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
    
      /* Initialize dialog manager */
    if (!DIALOG_Init()) return 0;

      /* Initialize menus */
    if (!MENU_Init()) return 0;

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
    int i;

    if (!MAIN_Init()) return 0;

    for (i = 1; i < argc; i++)
    {
        if (WinExec( argv[i], SW_SHOWNORMAL ) < 32)
        {
            fprintf(stderr, "wine: can't exec '%s'.\n", argv[i]);
            exit(1);
        }
    }

    if (Options.debug) wine_debug(0, NULL);

    Yield();  /* Start the first task */
    fprintf( stderr, "WinMain: Should never happen: returned from Yield()\n" );
    return 0;
}

#endif /* #ifndef WINELIB */
