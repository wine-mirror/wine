/*
 * Emulator thunks
 *
 * Copyright 1996, 1997 Alexandre Julliard
 */

#include <stdio.h>
#include "windows.h"
#include "callback.h"
#include "resource.h"
#include "task.h"
#include "user.h"
#include "heap.h"
#include "hook.h"
#include "module.h"
#include "stackframe.h"
#include "selectors.h"
#include "task.h"
#include "except.h"
#include "win.h"
#include "stddebug.h"
#include "debug.h"


/* List of the 16-bit callback functions. This list is used  */
/* by the build program to generate the file if1632/callto16.S */

/* ### start build ### */
extern LONG CALLBACK CallTo16_sreg_(const CONTEXT *context, INT32 offset);
extern LONG CALLBACK CallTo16_lreg_(const CONTEXT *context, INT32 offset);
extern WORD CALLBACK CallTo16_word_     (FARPROC16);
extern LONG CALLBACK CallTo16_long_     (FARPROC16);
extern WORD CALLBACK CallTo16_word_w    (FARPROC16,WORD);
extern LONG CALLBACK CallTo16_long_l    (FARPROC16,LONG);
extern WORD CALLBACK CallTo16_word_ww   (FARPROC16,WORD,WORD);
extern WORD CALLBACK CallTo16_word_wl   (FARPROC16,WORD,LONG);
extern WORD CALLBACK CallTo16_word_ll   (FARPROC16,LONG,LONG);
extern LONG CALLBACK CallTo16_long_ll   (FARPROC16,LONG,LONG);
extern WORD CALLBACK CallTo16_word_www  (FARPROC16,WORD,WORD,WORD);
extern WORD CALLBACK CallTo16_word_wwl  (FARPROC16,WORD,WORD,LONG);
extern WORD CALLBACK CallTo16_word_wlw  (FARPROC16,WORD,LONG,WORD);
extern LONG CALLBACK CallTo16_long_wwl  (FARPROC16,WORD,WORD,LONG);
extern LONG CALLBACK CallTo16_long_lll  (FARPROC16,LONG,LONG,LONG);
extern WORD CALLBACK CallTo16_word_llwl (FARPROC16,LONG,LONG,WORD,LONG);
extern WORD CALLBACK CallTo16_word_lwll (FARPROC16,LONG,WORD,LONG,LONG);
extern WORD CALLBACK CallTo16_word_lwww (FARPROC16,LONG,WORD,WORD,WORD);
extern WORD CALLBACK CallTo16_word_wwll (FARPROC16,WORD,WORD,LONG,LONG);
extern WORD CALLBACK CallTo16_word_llll (FARPROC16,LONG,LONG,LONG,LONG);
extern LONG CALLBACK CallTo16_long_llll (FARPROC16,LONG,LONG,LONG,LONG);
extern WORD CALLBACK CallTo16_word_wllwl(FARPROC16,WORD,LONG,LONG,WORD,LONG);
extern LONG CALLBACK CallTo16_long_lwwll(FARPROC16,LONG,WORD,WORD,LONG,LONG);
extern WORD CALLBACK CallTo16_word_wwlll(FARPROC16,WORD,WORD,LONG,LONG,LONG);
extern WORD CALLBACK CallTo16_word_wwwww(FARPROC16,WORD,WORD,WORD,WORD,WORD);
extern WORD CALLBACK CallTo16_word_lwlll(FARPROC16,LONG,WORD,LONG,LONG,LONG);
extern LONG CALLBACK CallTo16_long_lwlll(FARPROC16,LONG,WORD,LONG,LONG,LONG);
extern LONG CALLBACK CallTo16_long_lllll(FARPROC16,LONG,LONG,LONG,LONG,LONG);
extern LONG CALLBACK CallTo16_long_llllll(FARPROC16,LONG,LONG,LONG,LONG,LONG,
                                          LONG);
extern LONG CALLBACK CallTo16_long_lllllll(FARPROC16,LONG,LONG,LONG,LONG,LONG,
                                           LONG,LONG);
extern WORD CALLBACK CallTo16_word_llwwlll(FARPROC16,LONG,LONG,WORD,WORD,LONG,
                                           LONG,LONG);
extern LONG CALLBACK CallTo16_word_lwwlllll(FARPROC16,LONG,WORD,WORD,LONG,LONG,
                                            LONG,LONG,LONG);
extern LONG CALLBACK CallTo16_long_llllllll(FARPROC16,LONG,LONG,LONG,LONG,LONG,
                                            LONG,LONG,LONG);
extern LONG CALLBACK CallTo16_long_lllllllll(FARPROC16,LONG,LONG,LONG,LONG,
                                             LONG,LONG,LONG,LONG,LONG);
extern LONG CALLBACK CallTo16_long_llllllllll(FARPROC16,LONG,LONG,LONG,LONG,
                                              LONG,LONG,LONG,LONG,LONG,LONG);
extern LONG CALLBACK CallTo16_long_lllllllllll(FARPROC16,LONG,LONG,LONG,LONG,
                                               LONG,LONG,LONG,LONG,LONG,LONG,
                                               LONG);
extern LONG CALLBACK CallTo16_long_llllllllllll(FARPROC16,LONG,LONG,LONG,LONG,
                                                LONG,LONG,LONG,LONG,LONG,LONG,
                                                LONG,LONG);
extern LONG CALLBACK CallTo16_long_lwwllwlllllw(FARPROC16,LONG,WORD,WORD,LONG,
                                                LONG,WORD,LONG,LONG,LONG,LONG,
                                                LONG,WORD);
extern LONG CALLBACK CallTo16_long_lllllllllllll(FARPROC16,LONG,LONG,LONG,LONG,
                                                 LONG,LONG,LONG,LONG,LONG,LONG,
                                                 LONG,LONG,LONG);
extern LONG CALLBACK CallTo16_long_llllllllllllll(FARPROC16,LONG,LONG,LONG,
                                                  LONG,LONG,LONG,LONG,LONG,
                                                  LONG,LONG,LONG,LONG,LONG,
                                                  LONG);
extern LONG CALLBACK CallTo16_word_lwwwwlwwwwllll(FARPROC16,LONG,WORD,WORD,
                                                  WORD,WORD,LONG,WORD,WORD,
                                                  WORD,WORD,LONG,LONG,LONG,
                                                  LONG);
extern LONG CALLBACK CallTo16_long_lllllllllllllll(FARPROC16,LONG,LONG,LONG,
                                                   LONG,LONG,LONG,LONG,LONG,
                                                   LONG,LONG,LONG,LONG,LONG,
                                                   LONG,LONG);
extern LONG CALLBACK CallTo16_long_llllllllllllllll(FARPROC16,LONG,LONG,LONG,
                                                    LONG,LONG,LONG,LONG,LONG,
                                                    LONG,LONG,LONG,LONG,LONG,
                                                    LONG,LONG,LONG);
/* ### stop build ### */


typedef void (*RELAY)();

#pragma pack(1)

typedef struct tagTHUNK
{
    BYTE             popl_eax;           /* 0x58  popl  %eax (return address)*/
    BYTE             pushl_func;         /* 0x68  pushl $proc */
    FARPROC32        proc WINE_PACKED;
    BYTE             pushl_eax;          /* 0x50  pushl %eax */
    BYTE             jmp;                /* 0xe9  jmp   relay (relative jump)*/
    RELAY            relay WINE_PACKED;
    struct tagTHUNK *next WINE_PACKED;
} THUNK;

#pragma pack(4)

#define DECL_THUNK(name,proc,relay) \
    THUNK name = { 0x58, 0x68, (FARPROC32)(proc), 0x50, 0xe9, \
                   (RELAY)((char *)(relay) - (char *)(&(name).next)), NULL }


static THUNK *firstThunk = NULL;

static LRESULT WINAPI THUNK_CallWndProc16( WNDPROC16 proc, HWND16 hwnd,
                                           UINT16 msg, WPARAM16 wParam,
                                           LPARAM lParam );
static void WINAPI THUNK_CallTaskReschedule(void);
static BOOL32 WINAPI THUNK_WOWCallback16Ex( FARPROC16,DWORD,DWORD,
                                            LPVOID,LPDWORD );

/* TASK_Reschedule() 16-bit entry point */
static FARPROC16 TASK_RescheduleProc;

extern void CallFrom16_long_wwwll(void);

/* Callbacks function table for the emulator */
static const CALLBACKS_TABLE CALLBACK_EmulatorTable =
{
    (void *)CallTo16_sreg_,                /* CallRegisterShortProc */
    (void *)CallTo16_lreg_,                /* CallRegisterLongProc */
    THUNK_CallTaskReschedule,              /* CallTaskRescheduleProc */
    CallFrom16_long_wwwll,                 /* CallFrom16WndProc */
    THUNK_CallWndProc16,                   /* CallWndProc */
    (void *)CallTo16_long_lwwll,           /* CallDriverProc */
    (void *)CallTo16_word_wwlll,           /* CallDriverCallback */
    (void *)CallTo16_word_wwlll,           /* CallTimeFuncProc */
    (void *)CallTo16_word_w,               /* CallWindowsExitProc */
    (void *)CallTo16_word_lwww,            /* CallWordBreakProc */
    (void *)CallTo16_word_ww,              /* CallBootAppProc */
    (void *)CallTo16_word_www,             /* CallLoadAppSegProc */
    (void *)CallTo16_word_,                /* CallSystemTimerProc */
    (void *)CallTo16_long_l,               /* CallWOWCallbackProc */
    THUNK_WOWCallback16Ex,                 /* CallWOWCallback16Ex */
    (void *)CallTo16_long_l,               /* CallASPIPostProc */
    (void *)CallTo16_word_lwll,            /* CallDrvControlProc */
    (void *)CallTo16_word_lwlll,           /* CallDrvEnableProc */
    (void *)CallTo16_word_llll,            /* CallDrvEnumDFontsProc */
    (void *)CallTo16_word_lwll,            /* CallDrvEnumObjProc */
    (void *)CallTo16_word_lwwlllll,        /* CallDrvOutputProc */
    (void *)CallTo16_long_lwlll,           /* CallDrvRealizeProc */
    (void *)CallTo16_word_lwwwwlwwwwllll,  /* CallDrvStretchBltProc */
    (void *)CallTo16_long_lwwllwlllllw,    /* CallDrvExtTextOutProc */
    (void *)CallTo16_word_llwwlll          /* CallDrvGetCharWidth */ 
};


/***********************************************************************
 *           THUNK_Init
 */
BOOL32 THUNK_Init(void)
{
    /* Set the window proc calling functions */
    Callbacks = &CALLBACK_EmulatorTable;
    /* Get the 16-bit reschedule function pointer */
    TASK_RescheduleProc = MODULE_GetWndProcEntry16( "TASK_Reschedule" );
    return TRUE;
}


/***********************************************************************
 *           THUNK_Alloc
 */
static THUNK *THUNK_Alloc( FARPROC32 func, RELAY relay )
{
    THUNK *thunk = HeapAlloc( GetProcessHeap(), 0, sizeof(*thunk) );
    if (thunk)
    {
        thunk->popl_eax   = 0x58;
        thunk->pushl_func = 0x68;
        thunk->proc       = func;
        thunk->pushl_eax  = 0x50;
        thunk->jmp        = 0xe9;
        thunk->relay      = (RELAY)((char *)relay - (char *)(&thunk->next));
        thunk->next       = firstThunk;
        firstThunk = thunk;
    }
    return thunk;
}


/***********************************************************************
 *           THUNK_Find
 */
static THUNK *THUNK_Find( FARPROC32 func )
{
    THUNK *thunk = firstThunk;
    while (thunk && (thunk->proc != func)) thunk = thunk->next;
    return thunk;
}


/***********************************************************************
 *           THUNK_Free
 */
static void THUNK_Free( THUNK *thunk )
{
    if (HEAP_IsInsideHeap( GetProcessHeap(), 0, thunk ))
    {
        THUNK **prev = &firstThunk;
        while (*prev && (*prev != thunk)) prev = &(*prev)->next;
        if (*prev)
        {
            *prev = thunk->next;
            HeapFree( GetProcessHeap(), 0, thunk );
            return;
        }
    }
    dprintf_thunk( stddeb, "THUNK_Free: invalid thunk addr %p\n", thunk );
}


/***********************************************************************
 *           THUNK_CallWndProc16
 *
 * Call a 16-bit window procedure
 */
static LRESULT WINAPI THUNK_CallWndProc16( WNDPROC16 proc, HWND16 hwnd,
                                           UINT16 msg, WPARAM16 wParam,
                                           LPARAM lParam )
{
    CONTEXT context;
    LRESULT ret;
    WORD *args;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    DWORD offset = 0;
    THDB *thdb = THREAD_Current();

    /* Window procedures want ax = hInstance, ds = es = ss */

    DS_reg(&context)  = SELECTOROF(thdb->cur_stack);
    ES_reg(&context)  = DS_reg(&context);
    EAX_reg(&context) = wndPtr ? wndPtr->hInstance : DS_reg(&context);
    CS_reg(&context)  = SELECTOROF(proc);
    EIP_reg(&context) = OFFSETOF(proc);
    EBP_reg(&context) = OFFSETOF(thdb->cur_stack)
                        + (WORD)&((STACK16FRAME*)0)->bp;

    if (lParam)
    {
	/* Some programs (eg. the "Undocumented Windows" examples, JWP) only
           work if structures passed in lParam are placed in the stack/data
           segment. Programmers easily make the mistake of converting lParam
           to a near rather than a far pointer, since Windows apparently
           allows this. We copy the structures to the 16 bit stack; this is
           ugly but makes these programs work. */
	switch (msg)
	{
	  case WM_CREATE:
	  case WM_NCCREATE:
	    offset = sizeof(CREATESTRUCT16); break;
	  case WM_DRAWITEM:
	    offset = sizeof(DRAWITEMSTRUCT16); break;
	  case WM_COMPAREITEM:
	    offset = sizeof(COMPAREITEMSTRUCT16); break;
	}
	if (offset)
	{
	    void *s = PTR_SEG_TO_LIN(lParam);
	    lParam = STACK16_PUSH( thdb, offset );
	    memcpy( PTR_SEG_TO_LIN(lParam), s, offset );
	}
    }

    args = (WORD *)THREAD_STACK16(thdb) - 5;
    args[0] = LOWORD(lParam);
    args[1] = HIWORD(lParam);
    args[2] = wParam;
    args[3] = msg;
    args[4] = hwnd;

    ret = CallTo16_sreg_( &context, 5 * sizeof(WORD) );
    if (offset) STACK16_POP( thdb, offset );
    return ret;
}


/***********************************************************************
 *           THUNK_CallTaskReschedule
 */
static void WINAPI THUNK_CallTaskReschedule(void)
{
    CallTo16_word_(TASK_RescheduleProc);
}


/***********************************************************************
 *           THUNK_EnumObjects16   (GDI.71)
 */
INT16 WINAPI THUNK_EnumObjects16( HDC16 hdc, INT16 nObjType,
                                  GOBJENUMPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo16_word_ll );
    return EnumObjects16( hdc, nObjType, (GOBJENUMPROC16)&thunk, lParam );
}


/*************************************************************************
 *           THUNK_EnumFonts16   (GDI.70)
 */
INT16 WINAPI THUNK_EnumFonts16( HDC16 hdc, LPCSTR lpFaceName,
                                FONTENUMPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo16_word_llwl );
    return EnumFonts16( hdc, lpFaceName, (FONTENUMPROC16)&thunk, lParam );
}

/******************************************************************
 *           THUNK_EnumMetaFile16   (GDI.175)
 */
BOOL16 WINAPI THUNK_EnumMetaFile16( HDC16 hdc, HMETAFILE16 hmf,
                                    MFENUMPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo16_word_wllwl );
    return EnumMetaFile16( hdc, hmf, (MFENUMPROC16)&thunk, lParam );
}


/*************************************************************************
 *           THUNK_EnumFontFamilies16   (GDI.330)
 */
INT16 WINAPI THUNK_EnumFontFamilies16( HDC16 hdc, LPCSTR lpszFamily,
                                       FONTENUMPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo16_word_llwl );
    return EnumFontFamilies16(hdc, lpszFamily, (FONTENUMPROC16)&thunk, lParam);
}


/*************************************************************************
 *           THUNK_EnumFontFamiliesEx16   (GDI.613)
 */
INT16 WINAPI THUNK_EnumFontFamiliesEx16( HDC16 hdc, LPLOGFONT16 lpLF,
                                         FONTENUMPROCEX16 func, LPARAM lParam,
                                         DWORD reserved )
{
    DECL_THUNK( thunk, func, CallTo16_word_llwl );
    return EnumFontFamiliesEx16( hdc, lpLF, (FONTENUMPROCEX16)&thunk,
                                 lParam, reserved );
}


/**********************************************************************
 *           THUNK_LineDDA16   (GDI.100)
 */
void WINAPI THUNK_LineDDA16( INT16 nXStart, INT16 nYStart, INT16 nXEnd,
                             INT16 nYEnd, LINEDDAPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo16_word_wwl );
    LineDDA16( nXStart, nYStart, nXEnd, nYEnd, (LINEDDAPROC16)&thunk, lParam );
}


/*******************************************************************
 *           THUNK_EnumWindows16   (USER.54)
 */
BOOL16 WINAPI THUNK_EnumWindows16( WNDENUMPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo16_word_wl );
    return EnumWindows16( (WNDENUMPROC16)&thunk, lParam );
}


/**********************************************************************
 *           THUNK_EnumChildWindows16   (USER.55)
 */
BOOL16 WINAPI THUNK_EnumChildWindows16( HWND16 parent, WNDENUMPROC16 func,
                                        LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo16_word_wl );
    return EnumChildWindows16( parent, (WNDENUMPROC16)&thunk, lParam );
}


/**********************************************************************
 *           THUNK_EnumTaskWindows16   (USER.225)
 */
BOOL16 WINAPI THUNK_EnumTaskWindows16( HTASK16 hTask, WNDENUMPROC16 func,
                                       LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo16_word_wl );
    return EnumTaskWindows16( hTask, (WNDENUMPROC16)&thunk, lParam );
}


/***********************************************************************
 *           THUNK_EnumProps16   (USER.27)
 */
INT16 WINAPI THUNK_EnumProps16( HWND16 hwnd, PROPENUMPROC16 func )
{
    DECL_THUNK( thunk, func, CallTo16_word_wlw );
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
    DECL_THUNK( thunk, func, CallTo16_word_wlw );
    if (!func)
        return GrayString16( hdc, hbr, NULL, lParam, cch, x, y, cx, cy );
    else
        return GrayString16( hdc, hbr, (GRAYSTRINGPROC16)&thunk, lParam, cch,
                             x, y, cx, cy );
}


/***********************************************************************
 *           THUNK_SetWindowsHook16   (USER.121)
 */
FARPROC16 WINAPI THUNK_SetWindowsHook16( INT16 id, HOOKPROC16 proc )
{
    HINSTANCE16 hInst = FarGetOwner( HIWORD(proc) );
    HTASK16 hTask = (id == WH_MSGFILTER) ? GetCurrentTask() : 0;
    THUNK *thunk = THUNK_Alloc( (FARPROC16)proc, (RELAY)CallTo16_long_wwl );
    if (!thunk) return 0;
    return (FARPROC16)SetWindowsHookEx16( id, (HOOKPROC16)thunk, hInst, hTask);
}


/***********************************************************************
 *           THUNK_UnhookWindowsHook16   (USER.234)
 */
BOOL16 WINAPI THUNK_UnhookWindowsHook16( INT16 id, HOOKPROC16 proc )
{
    BOOL16 ret;
    THUNK *thunk = THUNK_Find( (FARPROC16)proc );
    if (!thunk) return FALSE;
    ret = UnhookWindowsHook16( id, (HOOKPROC16)thunk );
    THUNK_Free( thunk );
    return ret;
}


/***********************************************************************
 *           THUNK_SetWindowsHookEx16   (USER.291)
 */
HHOOK WINAPI THUNK_SetWindowsHookEx16( INT16 id, HOOKPROC16 proc,
                                       HINSTANCE16 hInst, HTASK16 hTask )
{
    THUNK *thunk = THUNK_Alloc( (FARPROC16)proc, (RELAY)CallTo16_long_wwl );
    if (!thunk) return 0;
    return SetWindowsHookEx16( id, (HOOKPROC16)thunk, hInst, hTask );
}


/***********************************************************************
 *           THUNK_UnhookWindowHookEx16   (USER.292)
 */
BOOL16 WINAPI THUNK_UnhookWindowsHookEx16( HHOOK hhook )
{
    THUNK *thunk = (THUNK *)HOOK_GetProc16( hhook );
    BOOL16 ret = UnhookWindowsHookEx16( hhook );
    if (thunk) THUNK_Free( thunk );
    return ret;
}



static FARPROC16 defDCHookProc = NULL;

/***********************************************************************
 *           THUNK_SetDCHook   (GDI.190)
 */
BOOL16 WINAPI THUNK_SetDCHook( HDC16 hdc, FARPROC16 proc, DWORD dwHookData )
{
    THUNK *thunk, *oldThunk;

    if (!defDCHookProc)  /* Get DCHook Win16 entry point */
        defDCHookProc = MODULE_GetEntryPoint( GetModuleHandle16("USER"), 362 );

    if (proc != defDCHookProc)
    {
        thunk = THUNK_Alloc( proc, (RELAY)CallTo16_word_wwll );
        if (!thunk) return FALSE;
    }
    else thunk = (THUNK *)DCHook;

    /* Free the previous thunk */
    GetDCHook( hdc, (FARPROC16 *)&oldThunk );
    if (oldThunk && (oldThunk != (THUNK *)DCHook)) THUNK_Free( oldThunk );

    return SetDCHook( hdc, (FARPROC16)thunk, dwHookData );
}


/***********************************************************************
 *           THUNK_GetDCHook   (GDI.191)
 */
DWORD WINAPI THUNK_GetDCHook( HDC16 hdc, FARPROC16 *phookProc )
{
    THUNK *thunk = NULL;
    DWORD ret = GetDCHook( hdc, (FARPROC16 *)&thunk );
    if (thunk)
    {
        if (thunk == (THUNK *)DCHook)
        {
            if (!defDCHookProc)  /* Get DCHook Win16 entry point */
                defDCHookProc = MODULE_GetEntryPoint(GetModuleHandle16("USER"),
                                                     362 );
            *phookProc = defDCHookProc;
        }
        else *phookProc = thunk->proc;
    }
    return ret;
}


/***********************************************************************
 *           THUNK_SetTaskSignalProc (KERNEL.38)
 */
FARPROC16 WINAPI THUNK_SetTaskSignalProc( HTASK16 hTask, FARPROC16 proc )
{
    static FARPROC16 defSignalProc16 = NULL;

    THUNK *thunk = NULL;

    if( !defSignalProc16 )
	defSignalProc16 = MODULE_GetEntryPoint(GetModuleHandle16("USER"), 314 );

    if( proc == defSignalProc16 )
	thunk = (THUNK*)SetTaskSignalProc( hTask, (FARPROC16)&USER_SignalProc );
    else 
    {
	thunk = THUNK_Alloc( proc, (RELAY)CallTo16_word_wwwww );
	if( !thunk ) return FALSE;
	thunk = (THUNK*)SetTaskSignalProc( hTask, (FARPROC16)thunk );
    }

    if( thunk != (THUNK*)USER_SignalProc )
    {
	if( !thunk ) return NULL;

	proc = thunk->proc;
	THUNK_Free( thunk );
	return proc;
    }
    return defSignalProc16;
}


/***********************************************************************
 *           THUNK_SetResourceHandler	(KERNEL.67)
 */
FARPROC16 WINAPI THUNK_SetResourceHandler( HMODULE16 hModule, SEGPTR typeId, FARPROC16 proc )
{
    /* loader/ne_resource.c */
    extern HGLOBAL16 WINAPI NE_DefResourceHandler(HGLOBAL16,HMODULE16,HRSRC16);

    static FARPROC16 defDIBIconLoader16 = NULL;
    static FARPROC16 defDIBCursorLoader16 = NULL;
    static FARPROC16 defResourceLoader16 = NULL;

    THUNK *thunk = NULL;

    if( !defResourceLoader16 )
    {
	HMODULE16 hUser = GetModuleHandle16("USER");
	defDIBIconLoader16 = MODULE_GetEntryPoint( hUser, 357 );
	defDIBCursorLoader16 = MODULE_GetEntryPoint( hUser, 356 );
	defResourceLoader16 = MODULE_GetWndProcEntry16( "DefResourceHandler" );
    }

    if( proc == defResourceLoader16 )
	thunk = (THUNK*)&NE_DefResourceHandler;
    else if( proc == defDIBIconLoader16 )
	thunk = (THUNK*)&LoadDIBIconHandler;
    else if( proc == defDIBCursorLoader16 )
	thunk = (THUNK*)&LoadDIBCursorHandler;
    else
    {
	thunk = THUNK_Alloc( proc, (RELAY)CallTo16_word_www );
	if( !thunk ) return FALSE;
    }

    thunk = (THUNK*)SetResourceHandler( hModule, typeId, (FARPROC16)thunk );

    if( thunk == (THUNK*)&NE_DefResourceHandler )
	return defResourceLoader16;
    if( thunk == (THUNK*)&LoadDIBIconHandler )
	return defDIBIconLoader16;
    if( thunk == (THUNK*)&LoadDIBCursorHandler )
	return defDIBCursorLoader16;

    if( thunk )
    {
	proc = thunk->proc;
	THUNK_Free( thunk );
	return proc;
    }
    return NULL;
}

/***********************************************************************
 *           THUNK_WOWCallback16Ex	(WOW32.3)(KERNEL32.55)
 */
static BOOL32 WINAPI THUNK_WOWCallback16Ex(
	FARPROC16 proc,DWORD dwFlags,DWORD cbArgs,LPVOID xargs,LPDWORD pdwret
) {
    LPDWORD     args = (LPDWORD)xargs;
    DWORD       ret,i;

    dprintf_relay(stddeb,"WOWCallback16Ex(%p,0x%08lx,%ld,%p,%p)\n",
    	proc,dwFlags,cbArgs,xargs,pdwret
    );
    if (dwFlags == WCB16_CDECL) {
	/* swap the arguments */
	args = HeapAlloc(GetProcessHeap(),0,cbArgs*sizeof(DWORD));
	for (i=0;i<cbArgs;i++)
	    args[i] = ((DWORD*)xargs)[cbArgs-i-1];
    }
    switch (cbArgs) {
    case 0: ret = CallTo16_long_(proc);break;
    case 1: ret = CallTo16_long_l(proc,args[0]);break;
    case 2: ret = CallTo16_long_ll(proc,args[0],args[1]);break;
    case 3: ret = CallTo16_long_lll(proc,args[0],args[1],args[2]);break;
    case 4: ret = CallTo16_long_llll(proc,args[0],args[1],args[2],args[3]);
	    break;
    case 5: ret = CallTo16_long_lllll(proc,args[0],args[1],args[2],args[3],
		args[4]
	    );
	    break;
    case 6: ret = CallTo16_long_llllll(proc,args[0],args[1],args[2],args[3],
		args[4],args[5]
	    );
	    break;
    case 7: ret = CallTo16_long_lllllll(proc,args[0],args[1],args[2],args[3],
		args[4],args[5],args[6]
	    );
	    break;
    case 8: ret = CallTo16_long_llllllll(proc,args[0],args[1],args[2],args[3],
  		args[4],args[5],args[6],args[7]
	    );
	    break;
    case 9: ret = CallTo16_long_lllllllll(proc,args[0],args[1],args[2],args[3],
    		args[4],args[5],args[6],args[7],args[8]
	    );
	    break;
    case 10:ret = CallTo16_long_llllllllll(proc,args[0],args[1],args[2],args[3],
    		args[4],args[5],args[6],args[7],args[8],args[9]
	    );
	    break;
    case 11:ret = CallTo16_long_lllllllllll(proc,args[0],args[1],args[2],
    		args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10]
	    );
	    break;
    case 12:ret = CallTo16_long_llllllllllll(proc,args[0],args[1],args[2],
    		args[3],args[4],args[5],args[6],args[7],args[8],args[9],
		args[10],args[11]
	    );
	    break;
    case 13:ret = CallTo16_long_lllllllllllll(proc,args[0],args[1],args[2],
    		args[3],args[4],args[5],args[6],args[7],args[8],args[9],
		args[10],args[11],args[12]
	    );
	    break;
    case 14:ret = CallTo16_long_llllllllllllll(proc,args[0],args[1],args[2],
    		args[3],args[4],args[5],args[6],args[7],args[8],args[9],
		args[10],args[11],args[12],args[13]
	    );
	    break;
    case 15:ret = CallTo16_long_lllllllllllllll(proc,args[0],args[1],args[2],
    		args[3],args[4],args[5],args[6],args[7],args[8],args[9],
		args[10],args[11],args[12],args[13],args[14]
	    );
	    break;
    case 16:ret = CallTo16_long_llllllllllllllll(proc,args[0],args[1],args[2],
    		args[3],args[4],args[5],args[6],args[7],args[8],args[9],
		args[10],args[11],args[12],args[13],args[14],args[15]
	    );
	    break;
    default:
	    fprintf(stderr,"CALLBACK_CallWOWCallback16Ex(), %ld arguments not supported.!n",cbArgs);
	    if (dwFlags == WCB16_CDECL)
		HeapFree(GetProcessHeap(),0,args);
	    return FALSE;
    }
    if (dwFlags == WCB16_CDECL)
    	HeapFree(GetProcessHeap(),0,args);
    if (pdwret) 
    	*pdwret = ret;
    return TRUE;
}
