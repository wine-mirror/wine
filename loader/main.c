/*
 * Main initialization code
 */

#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "windef.h"
#include "wine/winbase16.h"
#include "main.h"
#include "drive.h"
#include "file.h"
#include "options.h"
#include "process.h"
#include "shell.h"
#include "debugtools.h"
#include "server.h"
#include "loadorder.h"

DEFAULT_DEBUG_CHANNEL(server);

/***********************************************************************
 *           Main initialisation routine
 */
BOOL MAIN_MainInit(void)
{
    MAIN_WineInit();

    /* Load the configuration file */
    if (!PROFILE_LoadWineIni()) return FALSE;

    /* Initialise DOS drives */
    if (!DRIVE_Init()) return FALSE;

    /* Initialise DOS directories */
    if (!DIR_Init()) return FALSE;

    /* Registry initialisation */
    SHELL_LoadRegistry();
    
    /* Global boot finished, the rest is process-local */
    CLIENT_BootDone( TRACE_ON(server) );

    /* Initialize module loadorder */
    if (!MODULE_InitLoadOrder()) return FALSE;

    /* Initialize relay code */
    if (!RELAY_Init()) return FALSE;

    return TRUE;
}


/***********************************************************************
 *           ExitKernel16 (KERNEL.2)
 *
 * Clean-up everything and exit the Wine process.
 *
 */
void WINAPI ExitKernel16( void )
{
    /* Do the clean-up stuff */

    WriteOutProfiles16();
    TerminateProcess( GetCurrentProcess(), 0 );
}

