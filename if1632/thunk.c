/*
 * Emulator thunks
 *
 * Copyright 1996, 1997 Alexandre Julliard
 * Copyright 1998       Ulrich Weigand
 */

#include <string.h>
#include "wine/winbase16.h"
#include "task.h"
#include "hook.h"
#include "callback.h"
#include "builtin16.h"
#include "user.h"
#include "heap.h"
#include "neexe.h"
#include "process.h"
#include "stackframe.h"
#include "win.h"
#include "flatthunk.h"
#include "mouse.h"
#include "keyboard.h"
#include "debugtools.h"

DECLARE_DEBUG_CHANNEL(thunk)


/* List of the 16-bit callback functions. This list is used  */
/* by the build program to generate the file if1632/callto16.S */

/* ### start build ### */
extern WORD CALLBACK THUNK_CallTo16_word_     (FARPROC16);
extern LONG CALLBACK THUNK_CallTo16_long_     (FARPROC16);
extern WORD CALLBACK THUNK_CallTo16_word_w    (FARPROC16,WORD);
extern WORD CALLBACK THUNK_CallTo16_word_l    (FARPROC16,LONG);
extern LONG CALLBACK THUNK_CallTo16_long_l    (FARPROC16,LONG);
extern WORD CALLBACK THUNK_CallTo16_word_ww   (FARPROC16,WORD,WORD);
extern WORD CALLBACK THUNK_CallTo16_word_wl   (FARPROC16,WORD,LONG);
extern WORD CALLBACK THUNK_CallTo16_word_ll   (FARPROC16,LONG,LONG);
extern LONG CALLBACK THUNK_CallTo16_long_ll   (FARPROC16,LONG,LONG);
extern WORD CALLBACK THUNK_CallTo16_word_www  (FARPROC16,WORD,WORD,WORD);
extern WORD CALLBACK THUNK_CallTo16_word_wwl  (FARPROC16,WORD,WORD,LONG);
extern WORD CALLBACK THUNK_CallTo16_word_wlw  (FARPROC16,WORD,LONG,WORD);
extern WORD CALLBACK THUNK_CallTo16_word_llwl (FARPROC16,LONG,LONG,WORD,LONG);
extern WORD CALLBACK THUNK_CallTo16_word_lwww (FARPROC16,LONG,WORD,WORD,WORD);
extern WORD CALLBACK THUNK_CallTo16_word_wlww (FARPROC16,WORD,LONG,WORD,WORD);
extern WORD CALLBACK THUNK_CallTo16_word_wwwl (FARPROC16,WORD,WORD,WORD,LONG);
extern LONG CALLBACK THUNK_CallTo16_long_wwwl (FARPROC16,WORD,WORD,WORD,LONG);
extern WORD CALLBACK THUNK_CallTo16_word_wllwl(FARPROC16,WORD,LONG,LONG,WORD,LONG);
extern WORD CALLBACK THUNK_CallTo16_word_lwwww(FARPROC16,LONG,WORD,WORD,WORD,WORD);
extern LONG CALLBACK THUNK_CallTo16_long_lwwll(FARPROC16,LONG,WORD,WORD,LONG,LONG);
extern WORD CALLBACK THUNK_CallTo16_word_wwlll(FARPROC16,WORD,WORD,LONG,LONG,LONG);
/* ### stop build ### */



#include "pshpack1.h"

typedef struct tagTHUNK
{
    BYTE             popl_eax;           /* 0x58  popl  %eax (return address)*/
    BYTE             pushl_func;         /* 0x68  pushl $proc */
    FARPROC16        proc WINE_PACKED;
    BYTE             pushl_eax;          /* 0x50  pushl %eax */
    BYTE             jmp;                /* 0xe9  jmp   relay (relative jump)*/
    RELAY            relay WINE_PACKED;
    struct tagTHUNK *next WINE_PACKED;
    DWORD            magic;
} THUNK;

#define CALLTO16_THUNK_MAGIC 0x54484e4b   /* "THNK" */

#include "poppack.h"

#define DECL_THUNK(aname,aproc,arelay) \
    THUNK aname; \
    aname.popl_eax = 0x58; \
    aname.pushl_func = 0x68; \
    aname.proc = (FARPROC) (aproc); \
    aname.pushl_eax = 0x50; \
    aname.jmp = 0xe9; \
    aname.relay = (RELAY)((char *)(arelay) - (char *)(&(aname).next)); \
    aname.next = NULL; \
    aname.magic = CALLTO16_THUNK_MAGIC;

static THUNK *firstThunk = NULL;

static BOOL THUNK_ThunkletInit( void );

/* Callbacks function table for the emulator */
static const CALLBACKS_TABLE CALLBACK_EmulatorTable =
{
    (void *)CallTo16RegisterShort,               /* CallRegisterShortProc */
    (void *)CallTo16RegisterLong,                /* CallRegisterLongProc */
    (void *)THUNK_CallTo16_long_lwwll,           /* CallDriverProc */
    (void *)THUNK_CallTo16_word_wwlll,           /* CallDriverCallback */
    (void *)THUNK_CallTo16_word_wwlll,           /* CallTimeFuncProc */
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
 *           THUNK_Init
 */
BOOL THUNK_Init(void)
{
    /* Initialize Thunklets */
    return THUNK_ThunkletInit();
}

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

        TRACE_(thunk)( "(%04x:%04x, %p) -> built-in API %p\n",
                       SELECTOROF( func ), OFFSETOF( func ), relay, proc );
        return proc;
    }

    /* Otherwise, we need to alloc a thunk */
    thunk = HeapAlloc( SystemHeap, 0, sizeof(*thunk) );
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

    TRACE_(thunk)( "(%04x:%04x, %p) -> allocated thunk %p\n",
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

    if (HEAP_IsInsideHeap( SystemHeap, 0, t ))
    {
        THUNK **prev = &firstThunk;
        while (*prev && (*prev != t)) prev = &(*prev)->next;
        if (*prev)
        {
            *prev = t->next;
            HeapFree( SystemHeap, 0, t );
            return;
        }
    }
    ERR_(thunk)("invalid thunk addr %p\n", thunk );
    return;
}


/***********************************************************************
 *           THUNK_EnumObjects16   (GDI.71)
 */
INT16 WINAPI THUNK_EnumObjects16( HDC16 hdc, INT16 nObjType,
                                  GOBJENUMPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, THUNK_CallTo16_word_ll );
    return EnumObjects16( hdc, nObjType, (GOBJENUMPROC16)&thunk, lParam );
}


/*************************************************************************
 *           THUNK_EnumFonts16   (GDI.70)
 */
INT16 WINAPI THUNK_EnumFonts16( HDC16 hdc, LPCSTR lpFaceName,
                                FONTENUMPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, THUNK_CallTo16_word_llwl );
    return EnumFonts16( hdc, lpFaceName, (FONTENUMPROC16)&thunk, lParam );
}

/******************************************************************
 *           THUNK_EnumMetaFile16   (GDI.175)
 */
BOOL16 WINAPI THUNK_EnumMetaFile16( HDC16 hdc, HMETAFILE16 hmf,
                                    MFENUMPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, THUNK_CallTo16_word_wllwl );
    return EnumMetaFile16( hdc, hmf, (MFENUMPROC16)&thunk, lParam );
}


/*************************************************************************
 *           THUNK_EnumFontFamilies16   (GDI.330)
 */
INT16 WINAPI THUNK_EnumFontFamilies16( HDC16 hdc, LPCSTR lpszFamily,
                                       FONTENUMPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, THUNK_CallTo16_word_llwl );
    return EnumFontFamilies16(hdc, lpszFamily, (FONTENUMPROC16)&thunk, lParam);
}


/*************************************************************************
 *           THUNK_EnumFontFamiliesEx16   (GDI.613)
 */
INT16 WINAPI THUNK_EnumFontFamiliesEx16( HDC16 hdc, LPLOGFONT16 lpLF,
                                         FONTENUMPROCEX16 func, LPARAM lParam,
                                         DWORD reserved )
{
    DECL_THUNK( thunk, func, THUNK_CallTo16_word_llwl );
    return EnumFontFamiliesEx16( hdc, lpLF, (FONTENUMPROCEX16)&thunk,
                                 lParam, reserved );
}


/**********************************************************************
 *           THUNK_LineDDA16   (GDI.100)
 */
void WINAPI THUNK_LineDDA16( INT16 nXStart, INT16 nYStart, INT16 nXEnd,
                             INT16 nYEnd, LINEDDAPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, THUNK_CallTo16_word_wwl );
    LineDDA16( nXStart, nYStart, nXEnd, nYEnd, (LINEDDAPROC16)&thunk, lParam );
}


/*******************************************************************
 *           THUNK_EnumWindows16   (USER.54)
 */
BOOL16 WINAPI THUNK_EnumWindows16( WNDENUMPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, THUNK_CallTo16_word_wl );
    return EnumWindows16( (WNDENUMPROC16)&thunk, lParam );
}


/**********************************************************************
 *           THUNK_EnumChildWindows16   (USER.55)
 */
BOOL16 WINAPI THUNK_EnumChildWindows16( HWND16 parent, WNDENUMPROC16 func,
                                        LPARAM lParam )
{
    DECL_THUNK( thunk, func, THUNK_CallTo16_word_wl );
    return EnumChildWindows16( parent, (WNDENUMPROC16)&thunk, lParam );
}


/**********************************************************************
 *           THUNK_EnumTaskWindows16   (USER.225)
 */
BOOL16 WINAPI THUNK_EnumTaskWindows16( HTASK16 hTask, WNDENUMPROC16 func,
                                       LPARAM lParam )
{
    DECL_THUNK( thunk, func, THUNK_CallTo16_word_wl );
    return EnumTaskWindows16( hTask, (WNDENUMPROC16)&thunk, lParam );
}


/***********************************************************************
 *           THUNK_EnumProps16   (USER.27)
 */
INT16 WINAPI THUNK_EnumProps16( HWND16 hwnd, PROPENUMPROC16 func )
{
    DECL_THUNK( thunk, func, THUNK_CallTo16_word_wlw );
    return EnumProps16( hwnd, (PROPENUMPROC16)&thunk );
}


/***********************************************************************
 *           THUNK_GrayString16   (USER.185)
 */
BOOL16 WINAPI THUNK_GrayString16( HDC16 hdc, HBRUSH16 hbr,
                                  GRAYSTRINGPROC16 func, LPARAM lParam,
                                  INT16 cch, INT16 x, INT16 y,
                                  INT16 cx, INT16 cy )
{
    DECL_THUNK( thunk, func, THUNK_CallTo16_word_wlw );
    if (!func)
        return GrayString16( hdc, hbr, NULL, lParam, cch, x, y, cx, cy );
    else
        return GrayString16( hdc, hbr, (GRAYSTRINGPROC16)&thunk, lParam, cch,
                             x, y, cx, cy );
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

    hModule = GetModuleHandleA( "USER32" );
    if ( hModule )
    {
#define GETADDR( var, name )  \
        *(FARPROC *)&Callout.##var = GetProcAddress( hModule, name )

        GETADDR( PeekMessageA, "PeekMessageA" );
        GETADDR( PeekMessageW, "PeekMessageW" );
        GETADDR( GetMessageA, "GetMessageA" );
        GETADDR( GetMessageW, "GetMessageW" );
        GETADDR( SendMessageA, "SendMessageA" );
        GETADDR( SendMessageW, "SendMessageW" );
        GETADDR( PostMessageA, "PostMessageA" );
        GETADDR( PostMessageW, "PostMessageW" );
        GETADDR( PostThreadMessageA, "PostThreadMessageA" );
        GETADDR( PostThreadMessageW, "PostThreadMessageW" );
        GETADDR( TranslateMessage, "TranslateMessage" );
        GETADDR( DispatchMessageW, "DispatchMessageW" );
        GETADDR( DispatchMessageA, "DispatchMessageA" );
        GETADDR( RedrawWindow, "RedrawWindow" );
        GETADDR( UserSignalProc, "UserSignalProc" );

#undef GETADDR
    }

    pModule = NE_GetPtr( GetModuleHandle16( "USER" ) );
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

#undef GETADDR
    }
}

/***********************************************************************
 * 16->32 Flat Thunk routines:
 */

/***********************************************************************
 *              ThunkConnect16          (KERNEL.651)
 * Connects a 32bit and a 16bit thunkbuffer.
 */
UINT WINAPI ThunkConnect16(
        LPSTR module16,              /* [in] name of win16 dll */
        LPSTR module32,              /* [in] name of win32 dll */
        HINSTANCE16 hInst16,         /* [in] hInst of win16 dll */
        DWORD dwReason,              /* [in] initialisation argument */
        struct ThunkDataCommon *TD,  /* [in/out] thunkbuffer */
        LPSTR thunkfun32,            /* [in] win32 thunkfunction */
        WORD cs                      /* [in] CS of win16 dll */
) {
    BOOL directionSL;

    if (!strncmp(TD->magic, "SL01", 4))
    {
        directionSL = TRUE;

        TRACE_(thunk)("SL01 thunk %s (%lx) -> %s (%s), Reason: %ld\n",
                     module16, (DWORD)TD, module32, thunkfun32, dwReason);
    }
    else if (!strncmp(TD->magic, "LS01", 4))
    {
        directionSL = FALSE;

        TRACE_(thunk)("LS01 thunk %s (%lx) <- %s (%s), Reason: %ld\n",
                     module16, (DWORD)TD, module32, thunkfun32, dwReason);
    }
    else
    {
        ERR_(thunk)("Invalid magic %c%c%c%c\n",
                   TD->magic[0], TD->magic[1], TD->magic[2], TD->magic[3]);
        return 0;
    }

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            if (directionSL)
            {
                struct ThunkDataSL16 *SL16 = (struct ThunkDataSL16 *)TD;
                struct ThunkDataSL   *SL   = SL16->fpData;

                if (SL == NULL)
                {
                    SL = HeapAlloc(GetProcessHeap(), 0, sizeof(*SL));

                    SL->common   = SL16->common;
                    SL->flags1   = SL16->flags1;
                    SL->flags2   = SL16->flags2;

                    SL->apiDB    = PTR_SEG_TO_LIN(SL16->apiDatabase);
                    SL->targetDB = NULL;

                    lstrcpynA(SL->pszDll16, module16, 255);
                    lstrcpynA(SL->pszDll32, module32, 255);

                    /* We should create a SEGPTR to the ThunkDataSL,
                       but since the contents are not in the original format,
                       any access to this by 16-bit code would crash anyway. */
                    SL16->spData = 0;
                    SL16->fpData = SL;
                }


                if (SL->flags2 & 0x80000000)
                {
                    TRACE_(thunk)("Preloading 32-bit library\n");
                    LoadLibraryA(module32);
                }
            }
            else
            {
                /* nothing to do */
            }
            break;

        case DLL_PROCESS_DETACH:
            /* FIXME: cleanup */
            break;
    }

    return 1;
}


/***********************************************************************
 *           C16ThkSL                           (KERNEL.630)
 */

void WINAPI C16ThkSL(CONTEXT86 *context)
{
    LPBYTE stub = PTR_SEG_TO_LIN(EAX_reg(context)), x = stub;
    WORD cs, ds;
    GET_CS(cs);
    GET_DS(ds);

    /* We produce the following code:
     *
     *   mov ax, __FLATDS
     *   mov es, ax
     *   movzx ecx, cx
     *   mov edx, es:[ecx + $EDX]
     *   push bp
     *   push edx
     *   push dx
     *   push edx
     *   call __FLATCS:CallFrom16Thunk
     */

    *x++ = 0xB8; *((WORD *)x)++ = ds;
    *x++ = 0x8E; *x++ = 0xC0;
    *x++ = 0x66; *x++ = 0x0F; *x++ = 0xB7; *x++ = 0xC9;
    *x++ = 0x67; *x++ = 0x66; *x++ = 0x26; *x++ = 0x8B;
                 *x++ = 0x91; *((DWORD *)x)++ = EDX_reg(context);

    *x++ = 0x55;
    *x++ = 0x66; *x++ = 0x52;
    *x++ = 0x52;
    *x++ = 0x66; *x++ = 0x52;
    *x++ = 0x66; *x++ = 0x9A; *((DWORD *)x)++ = (DWORD)CallFrom16Thunk;
                              *((WORD *)x)++ = cs;

    /* Jump to the stub code just created */
    EIP_reg(context) = LOWORD(EAX_reg(context));
    CS_reg(context)  = HIWORD(EAX_reg(context));

    /* Since C16ThkSL got called by a jmp, we need to leave the
       original return address on the stack */
    ESP_reg(context) -= 4;
}

/***********************************************************************
 *           C16ThkSL01                         (KERNEL.631)
 */

void WINAPI C16ThkSL01(CONTEXT86 *context)
{
    LPBYTE stub = PTR_SEG_TO_LIN(EAX_reg(context)), x = stub;

    if (stub)
    {
        struct ThunkDataSL16 *SL16 = PTR_SEG_TO_LIN(EDX_reg(context));
        struct ThunkDataSL *td = SL16->fpData;

        DWORD procAddress = (DWORD)GetProcAddress16(GetModuleHandle16("KERNEL"), 631);
        WORD cs;
        GET_CS(cs);

        if (!td)
        {
            ERR_(thunk)("ThunkConnect16 was not called!\n");
            return;
        }

        TRACE_(thunk)("Creating stub for ThunkDataSL %08lx\n", (DWORD)td);


        /* We produce the following code:
         *
         *   xor eax, eax
         *   mov edx, $td
         *   call C16ThkSL01
         *   push bp
         *   push edx
         *   push dx
         *   push edx
         *   call __FLATCS:CallFrom16Thunk
         */

        *x++ = 0x66; *x++ = 0x33; *x++ = 0xC0;
        *x++ = 0x66; *x++ = 0xBA; *((DWORD *)x)++ = (DWORD)td;
        *x++ = 0x9A; *((DWORD *)x)++ = procAddress;

        *x++ = 0x55;
        *x++ = 0x66; *x++ = 0x52;
        *x++ = 0x52;
        *x++ = 0x66; *x++ = 0x52;
        *x++ = 0x66; *x++ = 0x9A; *((DWORD *)x)++ = (DWORD)CallFrom16Thunk;
                                  *((WORD *)x)++ = cs;

        /* Jump to the stub code just created */
        EIP_reg(context) = LOWORD(EAX_reg(context));
        CS_reg(context)  = HIWORD(EAX_reg(context));

        /* Since C16ThkSL01 got called by a jmp, we need to leave the
           orginal return address on the stack */
        ESP_reg(context) -= 4;
    }
    else
    {
        struct ThunkDataSL *td = (struct ThunkDataSL *)EDX_reg(context);
        DWORD targetNr = CX_reg(context) / 4;
        struct SLTargetDB *tdb;

        TRACE_(thunk)("Process %08lx calling target %ld of ThunkDataSL %08lx\n",
                     (DWORD)PROCESS_Current(), targetNr, (DWORD)td);

        for (tdb = td->targetDB; tdb; tdb = tdb->next)
            if (tdb->process == PROCESS_Current())
                break;

        if (!tdb)
        {
            TRACE_(thunk)("Loading 32-bit library %s\n", td->pszDll32);
            LoadLibraryA(td->pszDll32);

            for (tdb = td->targetDB; tdb; tdb = tdb->next)
                if (tdb->process == PROCESS_Current())
                    break;
        }

        if (tdb)
        {
            EDX_reg(context) = tdb->targetTable[targetNr];

            TRACE_(thunk)("Call target is %08lx\n", EDX_reg(context));
        }
        else
        {
            WORD *stack = PTR_SEG_OFF_TO_LIN(SS_reg(context), LOWORD(ESP_reg(context)));
            DX_reg(context) = HIWORD(td->apiDB[targetNr].errorReturnValue);
            AX_reg(context) = LOWORD(td->apiDB[targetNr].errorReturnValue);
            EIP_reg(context) = stack[2];
            CS_reg(context)  = stack[3];
            ESP_reg(context) += td->apiDB[targetNr].nrArgBytes + 4;

            ERR_(thunk)("Process %08lx did not ThunkConnect32 %s to %s\n",
                       (DWORD)PROCESS_Current(), td->pszDll32, td->pszDll16);
        }
    }
}



/***********************************************************************
 * 16<->32 Thunklet/Callback API:
 */

#include "pshpack1.h"
typedef struct _THUNKLET
{
    BYTE        prefix_target;
    BYTE        pushl_target;
    DWORD       target;

    BYTE        prefix_relay;
    BYTE        pushl_relay;
    DWORD       relay;

    BYTE        jmp_glue;
    DWORD       glue;

    BYTE        type;
    HINSTANCE16 owner;
    struct _THUNKLET *next;
} THUNKLET;
#include "poppack.h"

#define THUNKLET_TYPE_LS  1
#define THUNKLET_TYPE_SL  2

static HANDLE  ThunkletHeap = 0;
static THUNKLET *ThunkletAnchor = NULL;

static FARPROC ThunkletSysthunkGlueLS = 0;
static SEGPTR    ThunkletSysthunkGlueSL = 0;

static FARPROC ThunkletCallbackGlueLS = 0;
static SEGPTR    ThunkletCallbackGlueSL = 0;

/***********************************************************************
 *     THUNK_ThunkletInit
 */
static BOOL THUNK_ThunkletInit( void )
{
    LPBYTE thunk;

    ThunkletHeap = HeapCreate(HEAP_WINE_SEGPTR | HEAP_WINE_CODE16SEG, 0, 0);
    if (!ThunkletHeap) return FALSE;

    thunk = HeapAlloc( ThunkletHeap, 0, 5 );
    if (!thunk) return FALSE;
    
    ThunkletSysthunkGlueLS = (FARPROC)thunk;
    *thunk++ = 0x58;                             /* popl eax */
    *thunk++ = 0xC3;                             /* ret      */

    ThunkletSysthunkGlueSL = HEAP_GetSegptr( ThunkletHeap, 0, thunk );
    *thunk++ = 0x66; *thunk++ = 0x58;            /* popl eax */
    *thunk++ = 0xCB;                             /* lret     */

    return TRUE;
}

/***********************************************************************
 *     SetThunkletCallbackGlue             (KERNEL.560)
 */
void WINAPI SetThunkletCallbackGlue16( FARPROC glueLS, SEGPTR glueSL )
{
    ThunkletCallbackGlueLS = glueLS;
    ThunkletCallbackGlueSL = glueSL;
}


/***********************************************************************
 *     THUNK_FindThunklet
 */
THUNKLET *THUNK_FindThunklet( DWORD target, DWORD relay, 
                              DWORD glue, BYTE type ) 
{
    THUNKLET *thunk; 

    for (thunk = ThunkletAnchor; thunk; thunk = thunk->next)
        if (    thunk->type   == type
             && thunk->target == target
             && thunk->relay  == relay 
             && ( type == THUNKLET_TYPE_LS ?
                    ( thunk->glue == glue - (DWORD)&thunk->type )
                  : ( thunk->glue == glue ) ) )
            return thunk;

     return NULL;
}

/***********************************************************************
 *     THUNK_AllocLSThunklet
 */
FARPROC THUNK_AllocLSThunklet( SEGPTR target, DWORD relay, 
                                 FARPROC glue, HTASK16 owner ) 
{
    THUNKLET *thunk = THUNK_FindThunklet( (DWORD)target, relay, (DWORD)glue,
                                          THUNKLET_TYPE_LS );
    if (!thunk)
    {
        TDB *pTask = (TDB*)GlobalLock16( owner );

        if ( !(thunk = HeapAlloc( ThunkletHeap, 0, sizeof(THUNKLET) )) )
            return 0;

        thunk->prefix_target = thunk->prefix_relay = 0x90;
        thunk->pushl_target  = thunk->pushl_relay  = 0x68;
        thunk->jmp_glue = 0xE9;

        thunk->target  = (DWORD)target;
        thunk->relay   = (DWORD)relay;
        thunk->glue    = (DWORD)glue - (DWORD)&thunk->type;

        thunk->type    = THUNKLET_TYPE_LS;
        thunk->owner   = pTask? pTask->hInstance : 0;

        thunk->next    = ThunkletAnchor;
        ThunkletAnchor = thunk;
    }

    return (FARPROC)thunk;
}

/***********************************************************************
 *     THUNK_AllocSLThunklet
 */
SEGPTR THUNK_AllocSLThunklet( FARPROC target, DWORD relay,
                              SEGPTR glue, HTASK16 owner )
{
    THUNKLET *thunk = THUNK_FindThunklet( (DWORD)target, relay, (DWORD)glue,
                                          THUNKLET_TYPE_SL );
    if (!thunk)
    {
        TDB *pTask = (TDB*)GlobalLock16( owner );

        if ( !(thunk = HeapAlloc( ThunkletHeap, 0, sizeof(THUNKLET) )) )
            return 0;

        thunk->prefix_target = thunk->prefix_relay = 0x66;
        thunk->pushl_target  = thunk->pushl_relay  = 0x68;
        thunk->jmp_glue = 0xEA;

        thunk->target  = (DWORD)target;
        thunk->relay   = (DWORD)relay;
        thunk->glue    = (DWORD)glue;

        thunk->type    = THUNKLET_TYPE_SL;
        thunk->owner   = pTask? pTask->hInstance : 0;

        thunk->next    = ThunkletAnchor;
        ThunkletAnchor = thunk;
    }

    return HEAP_GetSegptr( ThunkletHeap, 0, thunk );
}

/**********************************************************************
 *     IsLSThunklet
 */
BOOL16 WINAPI IsLSThunklet( THUNKLET *thunk )
{
    return    thunk->prefix_target == 0x90 && thunk->pushl_target == 0x68
           && thunk->prefix_relay  == 0x90 && thunk->pushl_relay  == 0x68
           && thunk->jmp_glue == 0xE9 && thunk->type == THUNKLET_TYPE_LS;
}

/**********************************************************************
 *     IsSLThunklet                        (KERNEL.612)
 */
BOOL16 WINAPI IsSLThunklet16( THUNKLET *thunk )
{
    return    thunk->prefix_target == 0x66 && thunk->pushl_target == 0x68
           && thunk->prefix_relay  == 0x66 && thunk->pushl_relay  == 0x68
           && thunk->jmp_glue == 0xEA && thunk->type == THUNKLET_TYPE_SL;
}



/***********************************************************************
 *     AllocLSThunkletSysthunk             (KERNEL.607)
 */
FARPROC WINAPI AllocLSThunkletSysthunk16( SEGPTR target, 
                                          FARPROC relay, DWORD dummy )
{
    return THUNK_AllocLSThunklet( (SEGPTR)relay, (DWORD)target, 
                                  ThunkletSysthunkGlueLS, GetCurrentTask() );
}

/***********************************************************************
 *     AllocSLThunkletSysthunk             (KERNEL.608)
 */
SEGPTR WINAPI AllocSLThunkletSysthunk16( FARPROC target, 
                                       SEGPTR relay, DWORD dummy )
{
    return THUNK_AllocSLThunklet( (FARPROC)relay, (DWORD)target, 
                                  ThunkletSysthunkGlueSL, GetCurrentTask() );
}


/***********************************************************************
 *     AllocLSThunkletCallbackEx           (KERNEL.567)
 */
FARPROC WINAPI AllocLSThunkletCallbackEx16( SEGPTR target, 
                                            DWORD relay, HTASK16 task )
{
    THUNKLET *thunk = (THUNKLET *)PTR_SEG_TO_LIN( target );
    if ( !thunk ) return NULL;

    if (   IsSLThunklet16( thunk ) && thunk->relay == relay 
        && thunk->glue == (DWORD)ThunkletCallbackGlueSL )
        return (FARPROC)thunk->target;

    return THUNK_AllocLSThunklet( target, relay, 
                                  ThunkletCallbackGlueLS, task );
}

/***********************************************************************
 *     AllocSLThunkletCallbackEx           (KERNEL.568)
 */
SEGPTR WINAPI AllocSLThunkletCallbackEx16( FARPROC target, 
                                         DWORD relay, HTASK16 task )
{
    THUNKLET *thunk = (THUNKLET *)target;
    if ( !thunk ) return 0;

    if (   IsLSThunklet( thunk ) && thunk->relay == relay 
        && thunk->glue == (DWORD)ThunkletCallbackGlueLS - (DWORD)&thunk->type )
        return (SEGPTR)thunk->target;

    return THUNK_AllocSLThunklet( target, relay, 
                                  ThunkletCallbackGlueSL, task );
}

/***********************************************************************
 *     AllocLSThunkletCallback             (KERNEL.561) (KERNEL.606)
 */
FARPROC WINAPI AllocLSThunkletCallback16( SEGPTR target, DWORD relay )
{
    return AllocLSThunkletCallbackEx16( target, relay, GetCurrentTask() );
}

/***********************************************************************
 *     AllocSLThunkletCallback             (KERNEL.562) (KERNEL.605)
 */
SEGPTR WINAPI AllocSLThunkletCallback16( FARPROC target, DWORD relay )
{
    return AllocSLThunkletCallbackEx16( target, relay, GetCurrentTask() );
}

/***********************************************************************
 *     FindLSThunkletCallback              (KERNEL.563) (KERNEL.609)
 */
FARPROC WINAPI FindLSThunkletCallback( SEGPTR target, DWORD relay )
{
    THUNKLET *thunk = (THUNKLET *)PTR_SEG_TO_LIN( target );
    if (   thunk && IsSLThunklet16( thunk ) && thunk->relay == relay 
        && thunk->glue == (DWORD)ThunkletCallbackGlueSL )
        return (FARPROC)thunk->target;

    thunk = THUNK_FindThunklet( (DWORD)target, relay, 
                                (DWORD)ThunkletCallbackGlueLS, 
                                THUNKLET_TYPE_LS );
    return (FARPROC)thunk;
}

/***********************************************************************
 *     FindSLThunkletCallback              (KERNEL.564) (KERNEL.610)
 */
SEGPTR WINAPI FindSLThunkletCallback( FARPROC target, DWORD relay )
{
    THUNKLET *thunk = (THUNKLET *)target;
    if (   thunk && IsLSThunklet( thunk ) && thunk->relay == relay 
        && thunk->glue == (DWORD)ThunkletCallbackGlueLS - (DWORD)&thunk->type )
        return (SEGPTR)thunk->target;

    thunk = THUNK_FindThunklet( (DWORD)target, relay, 
                                (DWORD)ThunkletCallbackGlueSL, 
                                THUNKLET_TYPE_SL );
    return HEAP_GetSegptr( ThunkletHeap, 0, thunk );
}


/***********************************************************************
 *     FreeThunklet16            (KERNEL.611)
 */
BOOL16 WINAPI FreeThunklet16( DWORD unused1, DWORD unused2 )
{
    return FALSE;
}

/***********************************************************************
 * Callback Client API
 */

#define N_CBC_FIXED    20
#define N_CBC_VARIABLE 10
#define N_CBC_TOTAL    (N_CBC_FIXED + N_CBC_VARIABLE)

static SEGPTR CBClientRelay16[ N_CBC_TOTAL ];
static FARPROC *CBClientRelay32[ N_CBC_TOTAL ];

/***********************************************************************
 *     RegisterCBClient                    (KERNEL.619)
 */
INT16 WINAPI RegisterCBClient16( INT16 wCBCId, 
                                 SEGPTR relay16, FARPROC *relay32 )
{
    /* Search for free Callback ID */
    if ( wCBCId == -1 )
        for ( wCBCId = N_CBC_FIXED; wCBCId < N_CBC_TOTAL; wCBCId++ )
            if ( !CBClientRelay16[ wCBCId ] )
                break;

    /* Register Callback ID */
    if ( wCBCId > 0 && wCBCId < N_CBC_TOTAL )
    {
        CBClientRelay16[ wCBCId ] = relay16;
        CBClientRelay32[ wCBCId ] = relay32;
    }
    else
        wCBCId = 0;

    return wCBCId;
}

/***********************************************************************
 *     UnRegisterCBClient                  (KERNEL.622)
 */
INT16 WINAPI UnRegisterCBClient16( INT16 wCBCId, 
                                   SEGPTR relay16, FARPROC *relay32 )
{
    if (    wCBCId >= N_CBC_FIXED && wCBCId < N_CBC_TOTAL 
         && CBClientRelay16[ wCBCId ] == relay16 
         && CBClientRelay32[ wCBCId ] == relay32 )
    {
        CBClientRelay16[ wCBCId ] = 0;
        CBClientRelay32[ wCBCId ] = 0;
    }
    else
        wCBCId = 0;

    return wCBCId;
}


/***********************************************************************
 *     InitCBClient                        (KERNEL.623)
 */
void WINAPI InitCBClient16( FARPROC glueLS )
{
    HMODULE16 kernel = GetModuleHandle16( "KERNEL" );
    SEGPTR glueSL = (SEGPTR)WIN32_GetProcAddress16( kernel, (LPCSTR)604 );

    SetThunkletCallbackGlue16( glueLS, glueSL );
}

/***********************************************************************
 *     CBClientGlueSL                      (KERNEL.604)
 */
void WINAPI CBClientGlueSL( CONTEXT86 *context )
{
    /* Create stack frame */
    SEGPTR stackSeg = stack16_push( 12 );
    LPWORD stackLin = PTR_SEG_TO_LIN( stackSeg );
    SEGPTR glue, *glueTab;
    
    stackLin[3] = BP_reg( context );
    stackLin[2] = SI_reg( context );
    stackLin[1] = DI_reg( context );
    stackLin[0] = DS_reg( context );

    EBP_reg( context ) = OFFSETOF( stackSeg ) + 6;
    ESP_reg( context ) = OFFSETOF( stackSeg ) - 4;
    GS_reg( context ) = 0;

    /* Jump to 16-bit relay code */
    glueTab = PTR_SEG_TO_LIN( CBClientRelay16[ stackLin[5] ] );
    glue = glueTab[ stackLin[4] ];
    CS_reg ( context ) = SELECTOROF( glue );
    EIP_reg( context ) = OFFSETOF  ( glue );
}

/***********************************************************************
 *     CBClientThunkSL                      (KERNEL.620)
 */
extern DWORD CALL32_CBClient( FARPROC proc, LPWORD args, DWORD *esi );
void WINAPI CBClientThunkSL( CONTEXT86 *context )
{
    /* Call 32-bit relay code */

    LPWORD args = PTR_SEG_OFF_TO_LIN( SS_reg( context ), BP_reg( context ) );
    FARPROC proc = CBClientRelay32[ args[2] ][ args[1] ];

    EAX_reg(context) = CALL32_CBClient( proc, args, &ESI_reg( context ) );
}

/***********************************************************************
 *     CBClientThunkSLEx                    (KERNEL.621)
 */
extern DWORD CALL32_CBClientEx( FARPROC proc, LPWORD args, DWORD *esi, INT *nArgs );
void WINAPI CBClientThunkSLEx( CONTEXT86 *context )
{
    /* Call 32-bit relay code */

    LPWORD args = PTR_SEG_OFF_TO_LIN( SS_reg( context ), BP_reg( context ) );
    FARPROC proc = CBClientRelay32[ args[2] ][ args[1] ];
    INT nArgs;
    LPWORD stackLin;

    EAX_reg(context) = CALL32_CBClientEx( proc, args, &ESI_reg( context ), &nArgs );

    /* Restore registers saved by CBClientGlueSL */
    stackLin = (LPWORD)((LPBYTE)CURRENT_STACK16 + sizeof(STACK16FRAME) - 4);
    BP_reg( context ) = stackLin[3];
    SI_reg( context ) = stackLin[2];
    DI_reg( context ) = stackLin[1];
    DS_reg( context ) = stackLin[0];
    ESP_reg( context ) += 16+nArgs;

    /* Return to caller of CBClient thunklet */
    CS_reg ( context ) = stackLin[9];
    EIP_reg( context ) = stackLin[8];
}

