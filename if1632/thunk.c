/*
 * Emulator thunks
 *
 * Copyright 1996, 1997 Alexandre Julliard
 * Copyright 1998       Ulrich Weigand
 */

#include <string.h>
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "hook.h"
#include "callback.h"
#include "task.h"
#include "user.h"
#include "heap.h"
#include "module.h"
#include "process.h"
#include "stackframe.h"
#include "selectors.h"
#include "syslevel.h"
#include "task.h"
#include "except.h"
#include "win.h"
#include "flatthunk.h"
#include "mouse.h"
#include "keyboard.h"
#include "debug.h"


/* List of the 16-bit callback functions. This list is used  */
/* by the build program to generate the file if1632/callto16.S */

/* ### start build ### */
extern LONG CALLBACK CallTo16_sreg_(const CONTEXT *context, INT offset);
extern LONG CALLBACK CallTo16_lreg_(const CONTEXT *context, INT offset);
extern WORD CALLBACK CallTo16_word_     (FARPROC16);
extern LONG CALLBACK CallTo16_long_     (FARPROC16);
extern WORD CALLBACK CallTo16_word_w    (FARPROC16,WORD);
extern WORD CALLBACK CallTo16_word_l    (FARPROC16,LONG);
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
extern WORD CALLBACK CallTo16_word_wlww (FARPROC16,WORD,LONG,WORD,WORD);
extern WORD CALLBACK CallTo16_word_wwll (FARPROC16,WORD,WORD,LONG,LONG);
extern WORD CALLBACK CallTo16_word_wwwl (FARPROC16,WORD,WORD,WORD,LONG);
extern LONG CALLBACK CallTo16_long_wwwl (FARPROC16,WORD,WORD,WORD,LONG);
extern WORD CALLBACK CallTo16_word_llll (FARPROC16,LONG,LONG,LONG,LONG);
extern LONG CALLBACK CallTo16_long_llll (FARPROC16,LONG,LONG,LONG,LONG);
extern WORD CALLBACK CallTo16_word_wllwl(FARPROC16,WORD,LONG,LONG,WORD,LONG);
extern WORD CALLBACK CallTo16_word_lwwww(FARPROC16,LONG,WORD,WORD,WORD,WORD);
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
    FARPROC        proc WINE_PACKED;
    BYTE             pushl_eax;          /* 0x50  pushl %eax */
    BYTE             jmp;                /* 0xe9  jmp   relay (relative jump)*/
    RELAY            relay WINE_PACKED;
    struct tagTHUNK *next WINE_PACKED;
} THUNK;

#pragma pack(4)

#define DECL_THUNK(name,proc,relay) \
    THUNK name = { 0x58, 0x68, (FARPROC)(proc), 0x50, 0xe9, \
                   (RELAY)((char *)(relay) - (char *)(&(name).next)), NULL }


static THUNK *firstThunk = NULL;

static LRESULT WINAPI THUNK_CallWndProc16( WNDPROC16 proc, HWND16 hwnd,
                                           UINT16 msg, WPARAM16 wParam,
                                           LPARAM lParam );
static BOOL WINAPI THUNK_CallTaskReschedule(void);
static BOOL WINAPI THUNK_WOWCallback16Ex( FARPROC16,DWORD,DWORD,
                                            LPVOID,LPDWORD );

/* TASK_Reschedule() 16-bit entry point */
static FARPROC16 TASK_RescheduleProc;

static BOOL THUNK_ThunkletInit( void );

extern void CallFrom16_p_long_wwwll(void);

/* Callbacks function table for the emulator */
static const CALLBACKS_TABLE CALLBACK_EmulatorTable =
{
    (void *)CallTo16_sreg_,                /* CallRegisterShortProc */
    (void *)CallTo16_lreg_,                /* CallRegisterLongProc */
    THUNK_CallTaskReschedule,              /* CallTaskRescheduleProc */
    (void*)CallFrom16_p_long_wwwll,        /* CallFrom16WndProc */
    THUNK_CallWndProc16,                   /* CallWndProc */
    (void *)CallTo16_long_lwwll,           /* CallDriverProc */
    (void *)CallTo16_word_wwlll,           /* CallDriverCallback */
    (void *)CallTo16_word_wwlll,           /* CallTimeFuncProc */
    (void *)CallTo16_word_w,               /* CallWindowsExitProc */
    (void *)CallTo16_word_lwww,            /* CallWordBreakProc */
    (void *)CallTo16_word_ww,              /* CallBootAppProc */
    (void *)CallTo16_word_www,             /* CallLoadAppSegProc */
    (void *)CallTo16_word_www,             /* CallLocalNotifyFunc */
    (void *)CallTo16_word_www,             /* CallResourceHandlerProc */
    (void *)CallTo16_long_l,               /* CallWOWCallbackProc */
    THUNK_WOWCallback16Ex,                 /* CallWOWCallback16Ex */
    (void *)CallTo16_long_ll,              /* CallUTProc */
    (void *)CallTo16_long_l,               /* CallASPIPostProc */
    (void *)CallTo16_word_lwll,            /* CallDrvControlProc */
    (void *)CallTo16_word_lwlll,           /* CallDrvEnableProc */
    (void *)CallTo16_word_llll,            /* CallDrvEnumDFontsProc */
    (void *)CallTo16_word_lwll,            /* CallDrvEnumObjProc */
    (void *)CallTo16_word_lwwlllll,        /* CallDrvOutputProc */
    (void *)CallTo16_long_lwlll,           /* CallDrvRealizeProc */
    (void *)CallTo16_word_lwwwwlwwwwllll,  /* CallDrvStretchBltProc */
    (void *)CallTo16_long_lwwllwlllllw,    /* CallDrvExtTextOutProc */
    (void *)CallTo16_word_llwwlll,         /* CallDrvGetCharWidth */ 
    (void *)CallTo16_word_ww               /* CallDrvAbortProc */
};


/***********************************************************************
 *           THUNK_Init
 */
BOOL THUNK_Init(void)
{
    /* Set the window proc calling functions */
    Callbacks = &CALLBACK_EmulatorTable;
    /* Get the 16-bit reschedule function pointer */
    TASK_RescheduleProc = MODULE_GetWndProcEntry16( "TASK_Reschedule" );
    /* Initialize Thunklets */
    return THUNK_ThunkletInit();
}

/***********************************************************************
 *           THUNK_Alloc
 */
static THUNK *THUNK_Alloc( FARPROC func, RELAY relay )
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
static THUNK *THUNK_Find( FARPROC func )
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
    ERR(thunk, "invalid thunk addr %p\n", thunk );
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
    
    memset(&context, '\0', sizeof(context));
    DS_reg(&context)  = SELECTOROF(thdb->cur_stack);
    ES_reg(&context)  = DS_reg(&context);
    EAX_reg(&context) = wndPtr ? wndPtr->hInstance : DS_reg(&context);
    CS_reg(&context)  = SELECTOROF(proc);
    EIP_reg(&context) = OFFSETOF(proc);
    EBP_reg(&context) = OFFSETOF(thdb->cur_stack)
                        + (WORD)&((STACK16FRAME*)0)->bp;

    WIN_ReleaseWndPtr(wndPtr);
    
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
static BOOL WINAPI THUNK_CallTaskReschedule(void)
{
    return CallTo16_word_(TASK_RescheduleProc);
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
    HINSTANCE16 hInst = FarGetOwner16( HIWORD(proc) );
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
        defDCHookProc = NE_GetEntryPoint( GetModuleHandle16("USER"), 362 );

    if (proc != defDCHookProc)
    {
        thunk = THUNK_Alloc( proc, (RELAY)CallTo16_word_wwll );
        if (!thunk) return FALSE;
    }
    else thunk = (THUNK *)DCHook16;

    /* Free the previous thunk */
    GetDCHook( hdc, (FARPROC16 *)&oldThunk );
    if (oldThunk && (oldThunk != (THUNK *)DCHook16)) THUNK_Free( oldThunk );

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
        if (thunk == (THUNK *)DCHook16)
        {
            if (!defDCHookProc)  /* Get DCHook Win16 entry point */
                defDCHookProc = NE_GetEntryPoint(GetModuleHandle16("USER"),362);
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
    THUNK *thunk = THUNK_Alloc( proc, (RELAY)CallTo16_word_wwwww );
    if ( !thunk ) return NULL;

    thunk = (THUNK*)SetTaskSignalProc( hTask, (FARPROC16)thunk );
    if ( !thunk ) return NULL;

    proc = thunk->proc;
    THUNK_Free( thunk );
    return proc;
}

/***********************************************************************
 *           THUNK_CreateThread16   (KERNEL.441)
 */
static DWORD CALLBACK THUNK_StartThread16( LPVOID threadArgs )
{
    FARPROC16 start = ((FARPROC16 *)threadArgs)[0];
    DWORD     param = ((DWORD *)threadArgs)[1];
    HeapFree( GetProcessHeap(), 0, threadArgs );

    return CallTo16_long_l( start, param );
}
HANDLE WINAPI THUNK_CreateThread16( SECURITY_ATTRIBUTES *sa, DWORD stack,
                                      FARPROC16 start, SEGPTR param,
                                      DWORD flags, LPDWORD id )
{
    DWORD *threadArgs = HeapAlloc( GetProcessHeap(), 0, 2*sizeof(DWORD) );
    if (!threadArgs) return INVALID_HANDLE_VALUE;
    threadArgs[0] = (DWORD)start;
    threadArgs[1] = (DWORD)param;

    return CreateThread( sa, stack, THUNK_StartThread16, threadArgs, flags, id );
}

/***********************************************************************
 *           THUNK_WOWCallback16Ex	(WOW32.3)(KERNEL32.55)
 * Generic thunking routine to call 16 bit functions from 32bit code.
 * 
 * RETURNS
 * 	TRUE if the call was done
 */
static BOOL WINAPI THUNK_WOWCallback16Ex(
	FARPROC16 proc,		/* [in] 16bit function to call */
	DWORD dwFlags,		/* [in] flags (WCB_*) */
	DWORD cbArgs,		/* [in] number of arguments */
	LPVOID xargs,		/* [in/out] arguments */
	LPDWORD pdwret		/* [out] return value of the 16bit call */
) {
    LPDWORD     args = (LPDWORD)xargs;
    DWORD       ret,i;

    TRACE(relay,"(%p,0x%08lx,%ld,%p,%p)\n",
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
	    ERR(thunk,"%ld arguments not supported.\n",cbArgs);
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

/***********************************************************************
 *           THUNK_MOUSE_Enable   (MOUSE.2)
 */
static VOID WINAPI THUNK_CallMouseEventProc( FARPROC16 proc, 
                                             DWORD dwFlags, DWORD dx, DWORD dy,
                                             DWORD cButtons, DWORD dwExtraInfo )
{
    CONTEXT context;

    memset( &context, 0, sizeof(context) );
    CS_reg(&context)  = SELECTOROF( proc );
    EIP_reg(&context) = OFFSETOF( proc );
    AX_reg(&context)  = (WORD)dwFlags;
    BX_reg(&context)  = (WORD)dx;
    CX_reg(&context)  = (WORD)dy;
    DX_reg(&context)  = (WORD)cButtons;
    SI_reg(&context)  = LOWORD( dwExtraInfo );
    DI_reg(&context)  = HIWORD( dwExtraInfo );

    CallTo16_sreg_( &context, 0 );
}
VOID WINAPI THUNK_MOUSE_Enable( FARPROC16 proc )
{
    static THUNK *lastThunk = NULL;
    static FARPROC16 lastProc = NULL;

    if ( lastProc != proc )
    {
        if ( lastThunk ) 
            THUNK_Free( lastThunk );

        if ( !proc )
            lastThunk = NULL;
        else
            lastThunk = THUNK_Alloc( proc, (RELAY)THUNK_CallMouseEventProc );

        lastProc = proc;
    }

    return MOUSE_Enable( (LPMOUSE_EVENT_PROC)lastThunk );
}

/***********************************************************************
 *           GetMouseEventProc   (USER.337)
 */
FARPROC16 WINAPI GetMouseEventProc16(void)
{
    HMODULE16 hmodule = GetModuleHandle16("USER");
    return NE_GetEntryPoint( hmodule, NE_GetOrdinal( hmodule, "mouse_event" ));
}


/***********************************************************************
 *           WIN16_mouse_event   (USER.299)
 */
void WINAPI WIN16_mouse_event( CONTEXT *context )
{
    mouse_event( AX_reg(context), BX_reg(context), CX_reg(context),
                 DX_reg(context), MAKELONG(SI_reg(context), DI_reg(context)) );
}


/***********************************************************************
 *           THUNK_KEYBD_Enable   (KEYBOARD.2)
 */
static VOID WINAPI THUNK_CallKeybdEventProc( FARPROC16 proc, 
                                             BYTE bVk, BYTE bScan,
                                             DWORD dwFlags, DWORD dwExtraInfo )
{
    CONTEXT context;

    memset( &context, 0, sizeof(context) );
    CS_reg(&context)  = SELECTOROF( proc );
    EIP_reg(&context) = OFFSETOF( proc );
    AH_reg(&context)  = (dwFlags & KEYEVENTF_KEYUP)? 0x80 : 0;
    AL_reg(&context)  = bVk;
    BH_reg(&context)  = (dwFlags & KEYEVENTF_EXTENDEDKEY)? 1 : 0;
    BL_reg(&context)  = bScan;
    SI_reg(&context)  = LOWORD( dwExtraInfo );
    DI_reg(&context)  = HIWORD( dwExtraInfo );

    CallTo16_sreg_( &context, 0 );
}
VOID WINAPI THUNK_KEYBOARD_Enable( FARPROC16 proc, LPBYTE lpKeyState )
{
    static THUNK *lastThunk = NULL;
    static FARPROC16 lastProc = NULL;

    if ( lastProc != proc )
    {
        if ( lastThunk ) 
            THUNK_Free( lastThunk );

        if ( !proc )
            lastThunk = NULL;
        else
            lastThunk = THUNK_Alloc( proc, (RELAY)THUNK_CallKeybdEventProc );

        lastProc = proc;
    }

    return KEYBOARD_Enable( (LPKEYBD_EVENT_PROC)lastThunk, lpKeyState );
}

/***********************************************************************
 *           WIN16_keybd_event   (USER.289)
 */
void WINAPI WIN16_keybd_event( CONTEXT *context )
{
    DWORD dwFlags = 0;
    
    if (AH_reg(context) & 0x80) dwFlags |= KEYEVENTF_KEYUP;
    if (BH_reg(context) & 1   ) dwFlags |= KEYEVENTF_EXTENDEDKEY;

    keybd_event( AL_reg(context), BL_reg(context), 
                 dwFlags, MAKELONG(SI_reg(context), DI_reg(context)) );
}


/***********************************************************************
 *           WIN16_CreateSystemTimer   (SYSTEM.2)
 */
static void THUNK_CallSystemTimerProc( FARPROC16 proc, WORD timer )
{
    CONTEXT context;
    memset( &context, '\0', sizeof(context) );

    CS_reg( &context ) = SELECTOROF( proc );
    IP_reg( &context ) = OFFSETOF( proc );
    BP_reg( &context ) = OFFSETOF( THREAD_Current()->cur_stack )
                         + (WORD)&((STACK16FRAME*)0)->bp;

    AX_reg( &context ) = timer;

    if ( _ConfirmWin16Lock() )
    {
        FIXME( system, "Skipping timer %d callback because timer signal "
                       "arrived while we own the Win16Lock!\n", timer );
        return;
    }

    CallTo16_sreg_( &context, 0 ); 

    /* FIXME: This does not work if the signal occurs while this thread
              is currently in 16-bit code. With the current structure
              of the Wine thunking code, this seems to be hard to fix ... */
}
WORD WINAPI WIN16_CreateSystemTimer( WORD rate, FARPROC16 proc )
{
    THUNK *thunk = THUNK_Alloc( proc, (RELAY)THUNK_CallSystemTimerProc );
    WORD timer = CreateSystemTimer( rate, (SYSTEMTIMERPROC)thunk );
    if (!timer) THUNK_Free( thunk );
    return timer;
}

/***********************************************************************
 *           THUNK_InitCallout
 */
void THUNK_InitCallout(void)
{
    HMODULE hModule = GetModuleHandleA( "USER32" );
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

    hModule = GetModuleHandle16( "USER" );
    if ( hModule )
    {
#define GETADDR( var, name, thk )  \
        *(FARPROC *)&Callout.##var = (FARPROC) \
              THUNK_Alloc( WIN32_GetProcAddress16( hModule, name ), \
                           (RELAY)CallTo16_##thk )

        GETADDR( PeekMessage16, "PeekMessage", word_lwwww );
        GETADDR( GetMessage16, "GetMessage", word_lwww );
        GETADDR( SendMessage16, "SendMessage", long_wwwl );
        GETADDR( PostMessage16, "PostMessage", word_wwwl );
        GETADDR( PostAppMessage16, "PostAppMessage", word_wwwl );
        GETADDR( TranslateMessage16, "TranslateMessage", word_l );
        GETADDR( DispatchMessage16, "DispatchMessage", long_l );
        GETADDR( RedrawWindow16, "RedrawWindow", word_wlww );
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

    if (!lstrncmpA(TD->magic, "SL01", 4))
    {
        directionSL = TRUE;

        TRACE(thunk, "SL01 thunk %s (%lx) -> %s (%s), Reason: %ld\n",
                     module16, (DWORD)TD, module32, thunkfun32, dwReason);
    }
    else if (!lstrncmpA(TD->magic, "LS01", 4))
    {
        directionSL = FALSE;

        TRACE(thunk, "LS01 thunk %s (%lx) <- %s (%s), Reason: %ld\n",
                     module16, (DWORD)TD, module32, thunkfun32, dwReason);
    }
    else
    {
        ERR(thunk, "Invalid magic %c%c%c%c\n",
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
                    TRACE(thunk, "Preloading 32-bit library\n");
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

void WINAPI C16ThkSL(CONTEXT *context)
{
    extern void CallFrom16_t_long_(void);
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
     *   call __FLATCS:CallFrom16_t_long_
     */

    *x++ = 0xB8; *((WORD *)x)++ = ds;
    *x++ = 0x8E; *x++ = 0xC0;
    *x++ = 0x66; *x++ = 0x0F; *x++ = 0xB7; *x++ = 0xC9;
    *x++ = 0x67; *x++ = 0x66; *x++ = 0x26; *x++ = 0x8B;
                 *x++ = 0x91; *((DWORD *)x)++ = EDX_reg(context);

    *x++ = 0x55;
    *x++ = 0x66; *x++ = 0x52;
    *x++ = 0x66; *x++ = 0x9A; *((DWORD *)x)++ = (DWORD)CallFrom16_t_long_;
                              *((WORD *)x)++ = cs;

    /* Jump to the stub code just created */
    IP_reg(context) = LOWORD(EAX_reg(context));
    CS_reg(context) = HIWORD(EAX_reg(context));

    /* Since C16ThkSL got called by a jmp, we need to leave the
       orginal return address on the stack */
    SP_reg(context) -= 4;
}

/***********************************************************************
 *           C16ThkSL01                         (KERNEL.631)
 */

void WINAPI C16ThkSL01(CONTEXT *context)
{
    LPBYTE stub = PTR_SEG_TO_LIN(EAX_reg(context)), x = stub;

    if (stub)
    {
        struct ThunkDataSL16 *SL16 = PTR_SEG_TO_LIN(EDX_reg(context));
        struct ThunkDataSL *td = SL16->fpData;

        extern void CallFrom16_t_long_(void);
        DWORD procAddress = (DWORD)GetProcAddress16(GetModuleHandle16("KERNEL"), 631);
        WORD cs;
        GET_CS(cs);

        if (!td)
        {
            ERR(thunk, "ThunkConnect16 was not called!\n");
            return;
        }

        TRACE(thunk, "Creating stub for ThunkDataSL %08lx\n", (DWORD)td);


        /* We produce the following code:
         *
         *   xor eax, eax
         *   mov edx, $td
         *   call C16ThkSL01
         *   push bp
         *   push edx
         *   call __FLATCS:CallFrom16_t_long_
         */

        *x++ = 0x66; *x++ = 0x33; *x++ = 0xC0;
        *x++ = 0x66; *x++ = 0xBA; *((DWORD *)x)++ = (DWORD)td;
        *x++ = 0x9A; *((DWORD *)x)++ = procAddress;

        *x++ = 0x55;
        *x++ = 0x66; *x++ = 0x52;
        *x++ = 0x66; *x++ = 0x9A; *((DWORD *)x)++ = (DWORD)CallFrom16_t_long_;
                                  *((WORD *)x)++ = cs;

        /* Jump to the stub code just created */
        IP_reg(context) = LOWORD(EAX_reg(context));
        CS_reg(context) = HIWORD(EAX_reg(context));

        /* Since C16ThkSL01 got called by a jmp, we need to leave the
           orginal return address on the stack */
        SP_reg(context) -= 4;
    }
    else
    {
        struct ThunkDataSL *td = (struct ThunkDataSL *)EDX_reg(context);
        DWORD targetNr = CX_reg(context) / 4;
        struct SLTargetDB *tdb;

        TRACE(thunk, "Process %08lx calling target %ld of ThunkDataSL %08lx\n",
                     (DWORD)PROCESS_Current(), targetNr, (DWORD)td);

        for (tdb = td->targetDB; tdb; tdb = tdb->next)
            if (tdb->process == PROCESS_Current())
                break;

        if (!tdb)
        {
            TRACE(thunk, "Loading 32-bit library %s\n", td->pszDll32);
            LoadLibraryA(td->pszDll32);

            for (tdb = td->targetDB; tdb; tdb = tdb->next)
                if (tdb->process == PROCESS_Current())
                    break;
        }

        if (tdb)
        {
            EDX_reg(context) = tdb->targetTable[targetNr];

            TRACE(thunk, "Call target is %08lx\n", EDX_reg(context));
        }
        else
        {
            WORD *stack = PTR_SEG_OFF_TO_LIN(SS_reg(context), SP_reg(context));
            DX_reg(context) = HIWORD(td->apiDB[targetNr].errorReturnValue);
            AX_reg(context) = LOWORD(td->apiDB[targetNr].errorReturnValue);
            IP_reg(context) = stack[2];
            CS_reg(context) = stack[3];
            SP_reg(context) += td->apiDB[targetNr].nrArgBytes + 4;

            ERR(thunk, "Process %08lx did not ThunkConnect32 %s to %s\n",
                       (DWORD)PROCESS_Current(), td->pszDll32, td->pszDll16);
        }
    }
}

DWORD WINAPI 
WOW16Call(WORD x,WORD y,WORD z) {
	int	i;
	DWORD	calladdr;
	FIXME(thunk,"(0x%04x,0x%04x,%d),calling (",x,y,z);

	for (i=0;i<x/2;i++) {
		WORD	a = STACK16_POP(THREAD_Current(),2);
		DPRINTF("%04x ",a);
	}
	calladdr = STACK16_POP(THREAD_Current(),4);
	DPRINTF(") calling address was 0x%08lx\n",calladdr);
	return 0;
}


/***********************************************************************
 * 16<->32 Thunklet/Callback API:
 */

#pragma pack(1)
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
#pragma pack(4)

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
             && thunk->glue   == glue )
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
    if (   IsLSThunklet( thunk ) && thunk->relay == relay 
        && thunk->glue == (DWORD)ThunkletCallbackGlueLS )
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
        && thunk->glue == (DWORD)ThunkletCallbackGlueLS )
        return (SEGPTR)thunk->target;

    thunk = THUNK_FindThunklet( (DWORD)target, relay, 
                                (DWORD)ThunkletCallbackGlueSL, 
                                THUNKLET_TYPE_SL );
    return HEAP_GetSegptr( ThunkletHeap, 0, thunk );
}


/***********************************************************************
 * Callback Client API
 */

#define N_CBC_FIXED    20
#define N_CBC_VARIABLE 10
#define N_CBC_TOTAL    (N_CBC_FIXED + N_CBC_VARIABLE)

static SEGPTR    *CBClientRelay16[ N_CBC_TOTAL ];
static FARPROC *CBClientRelay32[ N_CBC_TOTAL ];

/***********************************************************************
 *     RegisterCBClient                    (KERNEL.619)
 */
INT16 WINAPI RegisterCBClient16( INT16 wCBCId, 
                               SEGPTR *relay16, FARPROC *relay32 )
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
                                 SEGPTR *relay16, FARPROC *relay32 )
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
void WINAPI CBClientGlueSL( CONTEXT *context )
{
    /* Create stack frame */
    SEGPTR stackSeg = STACK16_PUSH( THREAD_Current(), 12 );
    LPWORD stackLin = PTR_SEG_TO_LIN( stackSeg );
    SEGPTR glue;
    
    stackLin[3] = BP_reg( context );
    stackLin[2] = SI_reg( context );
    stackLin[1] = DI_reg( context );
    stackLin[0] = DS_reg( context );

    BP_reg( context ) = OFFSETOF( stackSeg ) + 6;
    SP_reg( context ) = OFFSETOF( stackSeg ) - 4;
    GS_reg( context ) = 0;

    /* Jump to 16-bit relay code */
    glue = CBClientRelay16[ stackLin[5] ][ stackLin[4] ];
    CS_reg ( context ) = SELECTOROF( glue );
    EIP_reg( context ) = OFFSETOF  ( glue );
}

/***********************************************************************
 *     CBClientThunkSL                      (KERNEL.620)
 */
void WINAPI CBClientThunkSL( CONTEXT *context )
{
    /* Call 32-bit relay code */
    extern DWORD WINAPI CALL32_CBClient( FARPROC proc, LPWORD args );

    LPWORD args = PTR_SEG_OFF_TO_LIN( SS_reg( context ), BP_reg( context ) );
    FARPROC proc = CBClientRelay32[ args[2] ][ args[1] ];

    EAX_reg(context) = CALL32_CBClient( proc, args );
}

/***********************************************************************
 *     CBClientThunkSLEx                    (KERNEL.621)
 */
void WINAPI CBClientThunkSLEx( CONTEXT *context )
{
    /* Call 32-bit relay code */
    extern DWORD WINAPI CALL32_CBClientEx( FARPROC proc, 
                                           LPWORD args, INT *nArgs );

    LPWORD args = PTR_SEG_OFF_TO_LIN( SS_reg( context ), BP_reg( context ) );
    FARPROC proc = CBClientRelay32[ args[2] ][ args[1] ];
    INT nArgs;
    LPWORD stackLin;

    EAX_reg(context) = CALL32_CBClientEx( proc, args, &nArgs );

    /* Restore registers saved by CBClientGlueSL */
    stackLin = (LPWORD)((LPBYTE)CURRENT_STACK16 + sizeof(STACK16FRAME) - 4);
    BP_reg( context ) = stackLin[3];
    SI_reg( context ) = stackLin[2];
    DI_reg( context ) = stackLin[1];
    DS_reg( context ) = stackLin[0];
    SP_reg( context ) += 16+nArgs;

    /* Return to caller of CBClient thunklet */
    CS_reg ( context ) = stackLin[9];
    EIP_reg( context ) = stackLin[8];
}

