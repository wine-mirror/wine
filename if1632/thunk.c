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
#include "module.h"
#include "stackframe.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(thunk);


/* List of the 16-bit callback functions. This list is used  */
/* by the build program to generate the file if1632/callto16.S */

/* ### start build ### */
extern WORD CALLBACK THUNK_CallTo16_word_lllw (FARPROC16,LONG,LONG,LONG,WORD);
extern WORD CALLBACK THUNK_CallTo16_word_ww   (FARPROC16,WORD,WORD);
/* ### stop build ### */

static THUNK *firstThunk = NULL;

CALLOUT_TABLE Callout = {
    /* UserSignalProc */ NULL,
    /* DestroyIcon32 */ NULL
};

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
        FARPROC proc = (FARPROC)((ENTRYPOINT16 *)MapSL( (SEGPTR)func ))->target;

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

    if (HeapValidate( GetProcessHeap(), 0, t ))
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
    FARPROC16 proc = GetProcAddress16( pModule->self, name );
    if ( !proc ) return 0;

    if ( pModule->flags & NE_FFLAGS_BUILTIN )
        return (FARPROC)((ENTRYPOINT16 *)MapSL( (SEGPTR)proc ))->target;
    else
        return (FARPROC)THUNK_Alloc( proc, relay );
}

/***********************************************************************
 *           THUNK_InitCallout
 */
void THUNK_InitCallout(void)
{
    NE_MODULE *pModule = NE_GetPtr( GetModuleHandle16( "USER.EXE" ) );
    if ( pModule )
    {
#define GETADDR( var, name, thk )  \
        *(FARPROC *)&Callout.var = THUNK_GetCalloutThunk( pModule, name, \
                                               (RELAY)THUNK_CallTo16_##thk )

        GETADDR( DestroyIcon32, "DestroyIcon32", word_ww );
        GETADDR( UserSignalProc, "SignalProc32", word_lllw );
#undef GETADDR
    }
    else WARN("no 16-bit USER\n");
}
