/*
 * Emulator initialisation code
 *
 */

#include "winbase.h"
#include "wine/winbase16.h"
#include "wingdi.h"
#include "winuser.h"

#include "miscemu.h"
#include "callback.h"
#include "options.h"
#include "dosexe.h"
#include "debugtools.h"

static char main_exe_name[MAX_PATH];
static HANDLE main_exe_file;

static BOOL (WINAPI *pGetMessageA)(LPMSG,HWND,UINT,UINT);
static BOOL (WINAPI *pTranslateMessage)(const MSG*);
static LONG (WINAPI *pDispatchMessageA)(const MSG*);

extern void PROCESS_InitWine( int argc, char *argv[], LPSTR win16_exe_name,
                              HANDLE *win16_exe_file ) WINE_NORETURN;
extern HINSTANCE16 NE_StartMain( LPCSTR name, HANDLE file );

/***********************************************************************
 *           Main loop of initial task
 */
int WINAPI wine_initial_task( HINSTANCE inst, HINSTANCE prev, LPSTR cmdline, INT show )
{
    MSG msg;
    HINSTANCE16 instance;
    HMODULE user32;

    /* some programs assume mmsystem is always present */
    LoadLibrary16( "mmsystem.dll" );

    if ((instance = NE_StartMain( main_exe_name, main_exe_file )) < 32)
    {
        if (instance == 11)  /* try DOS format */
        {
            if (DPMI_LoadDosSystem())
                Dosvm.LoadDosExe( main_exe_name, main_exe_file );
            /* if we get back here it failed */
            instance = GetLastError();
        }

        MESSAGE( "%s: can't exec '%s': ", argv0, GetCommandLineA() );
        switch (instance) 
        {
        case  2: MESSAGE("file not found\n" ); break;
        case 11: MESSAGE("invalid exe file\n" ); break;
        default: MESSAGE("error=%d\n", instance ); break;
        }
        ExitProcess(instance);
    }
    CloseHandle( main_exe_file );  /* avoid file sharing problems */

    /* Start message loop for desktop window */

    if (!(user32 = LoadLibraryA( "user32.dll" )))
    {
        MESSAGE( "Cannot load user32.dll\n" );
        ExitProcess( GetLastError() );
    }
    pGetMessageA      = (void *)GetProcAddress( user32, "GetMessageA" );
    pTranslateMessage = (void *)GetProcAddress( user32, "TranslateMessage" );
    pDispatchMessageA = (void *)GetProcAddress( user32, "DispatchMessageA" );

    while ( GetNumTasks16() > 1  && pGetMessageA( &msg, 0, 0, 0 ) )
    {
        pTranslateMessage( &msg );
        pDispatchMessageA( &msg );
    }

    ExitProcess( 0 );
}


/**********************************************************************
 *           main
 */
int main( int argc, char *argv[] )
{
    PROCESS_InitWine( argc, argv, main_exe_name, &main_exe_file );
    return 1;  /* not reached */
}
