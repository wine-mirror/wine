/*
 * Emulator thunks
 *
 * Copyright 1996, 1997 Alexandre Julliard
 * Copyright 1998       Ulrich Weigand
 */

#include <string.h>
#include "wine/winbase16.h"
#include "callback.h"
#include "builtin16.h"
#include "heap.h"
#include "module.h"
#include "neexe.h"
#include "stackframe.h"
#include "selectors.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(thunk);


/* List of the 16-bit callback functions. This list is used  */
/* by the build program to generate the file if1632/callto16.S */

/* ### start build ### */
extern WORD CALLBACK THUNK_CallTo16_word_     (FARPROC16);
extern WORD CALLBACK THUNK_CallTo16_word_w    (FARPROC16,WORD);
extern WORD CALLBACK THUNK_CallTo16_word_l    (FARPROC16,LONG);
extern LONG CALLBACK THUNK_CallTo16_long_l    (FARPROC16,LONG);
extern WORD CALLBACK THUNK_CallTo16_word_ww   (FARPROC16,WORD,WORD);
extern LONG CALLBACK THUNK_CallTo16_long_ll   (FARPROC16,LONG,LONG);
extern WORD CALLBACK THUNK_CallTo16_word_www  (FARPROC16,WORD,WORD,WORD);
extern WORD CALLBACK THUNK_CallTo16_word_lllw (FARPROC16,LONG,LONG,LONG,WORD);
extern WORD CALLBACK THUNK_CallTo16_word_lwww (FARPROC16,LONG,WORD,WORD,WORD);
extern WORD CALLBACK THUNK_CallTo16_word_wlww (FARPROC16,WORD,LONG,WORD,WORD);
extern WORD CALLBACK THUNK_CallTo16_word_wwwl (FARPROC16,WORD,WORD,WORD,LONG);
extern LONG CALLBACK THUNK_CallTo16_long_wwwl (FARPROC16,WORD,WORD,WORD,LONG);
extern WORD CALLBACK THUNK_CallTo16_word_lwwww(FARPROC16,LONG,WORD,WORD,WORD,WORD);
/* ### stop build ### */

static THUNK *firstThunk = NULL;

/* Callbacks function table for the emulator */
static const CALLBACKS_TABLE CALLBACK_EmulatorTable =
{
    (void *)CallTo16RegisterShort,               /* CallRegisterShortProc */
    (void *)CallTo16RegisterLong,                /* CallRegisterLongProc */
    (void *)THUNK_CallTo16_word_w,               /* CallWindowsExitProc */
    (void *)THUNK_CallTo16_word_lwww,            /* CallWordBreakProc */
    (void *)THUNK_CallTo16_word_ww,              /* CallBootAppProc */
    (void *)THUNK_CallTo16_word_www,             /* CallLoadAppSegProc */
    (void *)THUNK_CallTo16_word_www,             /* CallLocalNotifyFunc */
    (void *)THUNK_CallTo16_word_www,             /* CallResourceHandlerProc */
    (void *)THUNK_CallTo16_long_ll,              /* CallUTProc */
    (void *)THUNK_CallTo16_long_l                /* CallASPIPostProc */
};

const CALLBACKS_TABLE *Callbacks = &CALLBACK_EmulatorTable;

CALLOUT_TABLE Callout = { 0 };


/***********************************************************************
 *           THUNK_Alloc
 */
FARPROC THUNK_Alloc( FARPROC16 func, RELAY relay )
{
    HANDLE16 hSeg;
    NE_MODULE *pModule;
    THUNK *thunk;

    /* NULL maps to NULL */
    if ( !func ) return NULL;

    /* 
     * If we got an 16-bit built-in API entry point, retrieve the Wine
     * 32-bit handler for that API routine.
     *
     * NOTE: For efficiency reasons, we only check whether the selector
     *       of 'func' points to the code segment of a built-in module.
     *       It might be theoretically possible that the offset is such
     *       that 'func' does not point, in fact, to an API entry point.
     *       In this case, however, the pointer is corrupt anyway.
     */
    hSeg = GlobalHandle16( SELECTOROF( func ) );
    pModule = NE_GetPtr( FarGetOwner16( hSeg ) );

    if ( pModule && (pModule->flags & NE_FFLAGS_BUILTIN) 
                 && NE_SEG_TABLE(pModule)[0].hSeg == hSeg )
    {
        FARPROC proc = (FARPROC)((ENTRYPOINT16 *)PTR_SEG_TO_LIN( func ))->target;

        TRACE( "(%04x:%04x, %p) -> built-in API %p\n",
               SELECTOROF( func ), OFFSETOF( func ), relay, proc );
        return proc;
    }

    /* Otherwise, we need to alloc a thunk */
    thunk = HeapAlloc( GetProcessHeap(), 0, sizeof(*thunk) );
    if (thunk)
    {
        thunk->popl_eax   = 0x58;
        thunk->pushl_func = 0x68;
        thunk->proc       = func;
        thunk->pushl_eax  = 0x50;
        thunk->jmp        = 0xe9;
        thunk->relay      = (RELAY)((char *)relay - (char *)(&thunk->next));
        thunk->magic      = CALLTO16_THUNK_MAGIC;
        thunk->next       = firstThunk;
        firstThunk = thunk;
    }

    TRACE( "(%04x:%04x, %p) -> allocated thunk %p\n",
           SELECTOROF( func ), OFFSETOF( func ), relay, thunk );
    return (FARPROC)thunk;
}

/***********************************************************************
 *           THUNK_Free
 */
void THUNK_Free( FARPROC thunk )
{
    THUNK *t = (THUNK*)thunk;
    if ( !t || IsBadReadPtr( t, sizeof(*t) ) 
            || t->magic != CALLTO16_THUNK_MAGIC )
         return;

    if (HEAP_IsInsideHeap( GetProcessHeap(), 0, t ))
    {
        THUNK **prev = &firstThunk;
        while (*prev && (*prev != t)) prev = &(*prev)->next;
        if (*prev)
        {
            *prev = t->next;
            HeapFree( GetProcessHeap(), 0, t );
            return;
        }
    }
    ERR("invalid thunk addr %p\n", thunk );
    return;
}


/***********************************************************************
 *           THUNK_GetCalloutThunk
 *
 * Retrieve API entry point with given name from given module.
 * If module is builtin, return the 32-bit entry point, otherwise
 * create a 32->16 thunk to the 16-bit entry point, using the 
 * given relay code.
 *
 */
static FARPROC THUNK_GetCalloutThunk( NE_MODULE *pModule, LPSTR name, RELAY relay )
{
    FARPROC16 proc = WIN32_GetProcAddress16( pModule->self, name );
    if ( !proc ) return 0;

    if ( pModule->flags & NE_FFLAGS_BUILTIN )
        return (FARPROC)((ENTRYPOINT16 *)PTR_SEG_TO_LIN( proc ))->target;
    else
        return (FARPROC)THUNK_Alloc( proc, relay );
}

/***********************************************************************
 *           THUNK_InitCallout
 */
void THUNK_InitCallout(void)
{
    HMODULE hModule;
    NE_MODULE *pModule;

    hModule = LoadLibraryA( "user32.dll" );
    if ( hModule )
    {
#define GETADDR( name )  \
        *(FARPROC *)&Callout.##name = GetProcAddress( hModule, #name )

        GETADDR( PeekMessageA );
        GETADDR( PeekMessageW );
        GETADDR( GetMessageA );
        GETADDR( GetMessageW );
        GETADDR( SendMessageA );
        GETADDR( SendMessageW );
        GETADDR( PostMessageA );
        GETADDR( PostMessageW );
        GETADDR( PostThreadMessageA );
        GETADDR( PostThreadMessageW );
        GETADDR( TranslateMessage );
        GETADDR( DispatchMessageW );
        GETADDR( DispatchMessageA );
        GETADDR( RedrawWindow );
        GETADDR( WaitForInputIdle );
        GETADDR( MsgWaitForMultipleObjects );
        GETADDR( WindowFromDC );
        GETADDR( GetForegroundWindow );
        GETADDR( IsChild );
        GETADDR( MessageBoxA );
        GETADDR( MessageBoxW );
#undef GETADDR
    }

    pModule = NE_GetPtr( LoadLibrary16( "USER.EXE" ) );
    if ( pModule )
    {
#define GETADDR( var, name, thk )  \
        *(FARPROC *)&Callout.##var = THUNK_GetCalloutThunk( pModule, name, \
                                                 (RELAY)THUNK_CallTo16_##thk )

        GETADDR( PeekMessage16, "PeekMessage", word_lwwww );
        GETADDR( GetMessage16, "GetMessage", word_lwww );
        GETADDR( SendMessage16, "SendMessage", long_wwwl );
        GETADDR( PostMessage16, "PostMessage", word_wwwl );
        GETADDR( PostAppMessage16, "PostAppMessage", word_wwwl );
        GETADDR( TranslateMessage16, "TranslateMessage", word_l );
        GETADDR( DispatchMessage16, "DispatchMessage", long_l );
        GETADDR( RedrawWindow16, "RedrawWindow", word_wlww );
        GETADDR( FinalUserInit16, "FinalUserInit", word_ );
        GETADDR( InitApp16, "InitApp", word_w );
        GETADDR( InitThreadInput16, "InitThreadInput", word_ww );
        GETADDR( UserYield16, "UserYield", word_ );
        GETADDR( DestroyIcon32, "DestroyIcon32", word_ww );
        GETADDR( UserSignalProc, "SignalProc32", word_lllw );

#undef GETADDR
    }
}
