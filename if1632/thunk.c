/*
 * Emulator and Win95 thunks
 *
 * Copyright 1996, 1997 Alexandre Julliard
 * Copyright 1997 Marcus Meissner
 */

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
extern LONG CALLBACK CallTo16_regs_short(const CONTEXT *context, INT32 offset);
extern LONG CALLBACK CallTo16_regs_long (const CONTEXT *context, INT32 offset);
extern WORD CALLBACK CallTo16_word_     (FARPROC16);
extern WORD CALLBACK CallTo16_word_w    (FARPROC16,WORD);
extern LONG CALLBACK CallTo16_long_l    (FARPROC16,LONG);
extern WORD CALLBACK CallTo16_word_ww   (FARPROC16,WORD,WORD);
extern WORD CALLBACK CallTo16_word_wl   (FARPROC16,WORD,LONG);
extern WORD CALLBACK CallTo16_word_ll   (FARPROC16,LONG,LONG);
extern WORD CALLBACK CallTo16_word_www  (FARPROC16,WORD,WORD,WORD);
extern WORD CALLBACK CallTo16_word_wwl  (FARPROC16,WORD,WORD,LONG);
extern WORD CALLBACK CallTo16_word_wlw  (FARPROC16,WORD,LONG,WORD);
extern LONG CALLBACK CallTo16_long_wwl  (FARPROC16,WORD,WORD,LONG);
extern WORD CALLBACK CallTo16_word_llwl (FARPROC16,LONG,LONG,WORD,LONG);
extern WORD CALLBACK CallTo16_word_lwll (FARPROC16,LONG,WORD,LONG,LONG);
extern WORD CALLBACK CallTo16_word_lwww (FARPROC16,LONG,WORD,WORD,WORD);
extern WORD CALLBACK CallTo16_word_wwll (FARPROC16,WORD,WORD,LONG,LONG);
extern WORD CALLBACK CallTo16_word_wllwl(FARPROC16,WORD,LONG,LONG,WORD,LONG);
extern LONG CALLBACK CallTo16_long_lwwll(FARPROC16,LONG,WORD,WORD,LONG,LONG);
extern WORD CALLBACK CallTo16_word_wwlll(FARPROC16,WORD,WORD,LONG,LONG,LONG);
extern WORD CALLBACK CallTo16_word_wwwww(FARPROC16,WORD,WORD,WORD,WORD,WORD);
extern WORD CALLBACK CallTo16_word_lwlll(FARPROC16,LONG,WORD,LONG,LONG,LONG);
extern WORD CALLBACK CallTo16_word_llll (FARPROC16,LONG,LONG,LONG,LONG);
extern LONG CALLBACK CallTo16_long_lwlll(FARPROC16,LONG,WORD,LONG,LONG,LONG);
extern LONG CALLBACK CallTo16_word_lwwlllll(FARPROC16,LONG,WORD,WORD,LONG,LONG,
                                            LONG,LONG,WORD);
extern LONG CALLBACK CallTo16_long_lwwllwlllllw(FARPROC16,LONG,WORD,WORD,LONG,
                                                LONG,WORD,LONG,LONG,LONG,LONG,
                                                LONG,WORD);
extern LONG CALLBACK CallTo16_word_lwwwwlwwwwllll(FARPROC16,LONG,WORD,WORD,
                                                  WORD,WORD,LONG,WORD,WORD,
                                                  WORD,WORD,LONG,LONG,LONG,
                                                  LONG);
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

/* TASK_Reschedule() 16-bit entry point */
static FARPROC16 TASK_RescheduleProc;

extern void CallFrom16_long_wwwll(void);

/* Callbacks function table for the emulator */
static const CALLBACKS_TABLE CALLBACK_EmulatorTable =
{
    (void *)CallTo16_regs_short,           /* CallRegisterProc */
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
    (void *)CallTo16_long_l,               /* CallASPIPostProc */
    (void *)CallTo16_word_lwll,            /* CallDrvControlProc */
    (void *)CallTo16_word_lwlll,           /* CallDrvEnableProc */
    (void *)CallTo16_word_llll,            /* CallDrvEnumDFontsProc */
    (void *)CallTo16_word_lwll,            /* CallDrvEnumObjProc */
    (void *)CallTo16_word_lwwlllll,        /* CallDrvOutputProc */
    (void *)CallTo16_long_lwlll,           /* CallDrvRealizeProc */
    (void *)CallTo16_word_lwwwwlwwwwllll,  /* CallDrvStretchBltProc */
    (void *)CallTo16_long_lwwllwlllllw     /* CallDrvExtTextOutProc */
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
    fprintf( stderr, "THUNK_Free: invalid thunk addr %p\n", thunk );
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

    /* Window procedures want ax = hInstance, ds = es = ss */

    DS_reg(&context)  = SELECTOROF(IF1632_Saved16_ss_sp);
    ES_reg(&context)  = DS_reg(&context);
    EAX_reg(&context) = wndPtr ? wndPtr->hInstance : DS_reg(&context);
    CS_reg(&context)  = SELECTOROF(proc);
    EIP_reg(&context) = OFFSETOF(proc);
    EBP_reg(&context) = OFFSETOF(IF1632_Saved16_ss_sp)
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
	    lParam = STACK16_PUSH( offset );
	    memcpy( PTR_SEG_TO_LIN(lParam), s, offset );
	}
    }

    args = (WORD *)CURRENT_STACK16 - 7;
    args[0] = LOWORD(lParam);
    args[1] = HIWORD(lParam);
    args[2] = wParam;
    args[3] = msg;
    args[4] = hwnd;
    /* args[5] and args[6] are used by relay code to store the stack pointer */

    ret = CallTo16_regs_short( &context, -(5 * sizeof(WORD)) );
    if (offset) STACK16_POP(offset);
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
 *                                                                     *
 *                 Win95 internal thunks                               *
 *                                                                     *
 ***********************************************************************/

/***********************************************************************
 * Generates a FT_Prolog call.
 *	
 *  0FB6D1                  movzbl edx,cl
 *  8B1495xxxxxxxx	    mov edx,[4*edx + xxxxxxxx]
 *  68xxxxxxxx		    push FT_Prolog
 *  C3			    lret
 */
static void _write_ftprolog(LPBYTE thunk,DWORD thunkstart) {
	LPBYTE	x;

	x	= thunk;
	*x++	= 0x0f;*x++=0xb6;*x++=0xd1; /* movzbl edx,cl */
	*x++	= 0x8B;*x++=0x14;*x++=0x95;*(DWORD*)x= thunkstart;
	x+=4;	/* mov edx, [4*edx + thunkstart] */
	*x++	= 0x68; *(DWORD*)x = (DWORD)GetProcAddress32(GetModuleHandle32A("KERNEL32"),"FT_Prolog");
	x+=4; 	/* push FT_Prolog */
	*x++	= 0xC3;		/* lret */
	/* fill rest with 0xCC / int 3 */
}

/***********************************************************************
 *			FT_PrologPrime			(KERNEL32.89)
 */
void WINAPI FT_PrologPrime(DWORD startind,LPBYTE thunk) {
	_write_ftprolog(thunk,*(DWORD*)(startind+thunk));
}

/***********************************************************************
 * Generates a QT_Thunk style call.
 *	
 *  33C9                    xor ecx, ecx
 *  8A4DFC                  mov cl , [ebp-04]
 *  8B148Dxxxxxxxx          mov edx, [4*ecx + (EAX+EDX)]
 *  B8yyyyyyyy              mov eax, QT_Thunk
 *  FFE0                    jmp eax
 */
static void _write_qtthunk(LPBYTE start,DWORD thunkstart) {
	LPBYTE	x;

	x	= start;
	*x++	= 0x33;*x++=0xC9; /* xor ecx,ecx */
	*x++	= 0x8A;*x++=0x4D;*x++=0xFC; /* movb cl,[ebp-04] */
	*x++	= 0x8B;*x++=0x14;*x++=0x8D;*(DWORD*)x= thunkstart;
	x+=4;	/* mov edx, [4*ecx + (EAX+EDX) */
	*x++	= 0xB8; *(DWORD*)x = (DWORD)GetProcAddress32(GetModuleHandle32A("KERNEL32"),"QT_Thunk");
	x+=4; 	/* mov eax , QT_Thunk */
	*x++	= 0xFF; *x++ = 0xE0;	/* jmp eax */
	/* should fill the rest of the 32 bytes with 0xCC */
}

/***********************************************************************
 *		ThunkConnect32		(KERNEL32)
 * Connects a 32bit and a 16bit thunkbuffer.
 */
struct thunkstruct
{
	char	magic[4];
	DWORD	length;
	DWORD	ptr;
	DWORD	x0C;

	DWORD	x10;
	DWORD	x14;
	DWORD	x18;
	DWORD	x1C;
	DWORD	x20;
};

UINT32 WINAPI ThunkConnect32( struct thunkstruct *ths, LPSTR thunkfun16,
                              LPSTR module16, LPSTR module32, HMODULE32 hmod32,
                              DWORD dllinitarg1 )
{
	HINSTANCE16	hmm;
	SEGPTR		thkbuf;
	struct	thunkstruct	*ths16;

	fprintf(stdnimp,"ThunkConnect32(<struct>,%s,%s,%s,%x,%lx)\n",
		thunkfun16,module32,module16,hmod32,dllinitarg1
	);
	fprintf(stdnimp,"	magic = %c%c%c%c\n",
		ths->magic[0],
		ths->magic[1],
		ths->magic[2],
		ths->magic[3]
	);
	fprintf(stdnimp,"	length = %lx\n",ths->length);
	if (lstrncmp32A(ths->magic,"SL01",4)&&lstrncmp32A(ths->magic,"LS01",4))
		return 0;
	hmm=LoadModule16(module16,NULL);
	if (hmm<=32)
		return 0;
	thkbuf=(SEGPTR)WIN32_GetProcAddress16(hmm,thunkfun16);
	if (!thkbuf)
		return 0;
	ths16=(struct thunkstruct*)PTR_SEG_TO_LIN(thkbuf);
	if (lstrncmp32A(ths16->magic,ths->magic,4))
		return 0;

	if (!lstrncmp32A(ths->magic,"SL01",4))  {
		if (ths16->length != ths->length)
			return 0;
		ths->x0C = (DWORD)ths16;

		fprintf(stderr,"	ths16 magic is 0x%08lx\n",*(DWORD*)ths16->magic);
		if (*((DWORD*)ths16->magic) != 0x0000304C)
			return 0;
		if (!*(WORD*)(((LPBYTE)ths16)+0x12))
			return 0;
		
	}
	if (!lstrncmp32A(ths->magic,"LS01",4))  {
		if (ths16->length != ths->length)
			return 0;
		ths->ptr = (DWORD)PTR_SEG_TO_LIN(ths16->ptr);
		/* code offset for QT_Thunk is at 0x1C...  */
		_write_qtthunk (((LPBYTE)ths) + ths->x1C,ths->ptr);
		/* code offset for FT_Prolog is at 0x20...  */
		_write_ftprolog(((LPBYTE)ths) + ths->x20,ths->ptr);
		return 1;
	}
	return TRUE;
}


/**********************************************************************
 * 		QT_Thunk			(KERNEL32)
 *
 * The target address is in EDX.
 * The 16 bit arguments start at ESP+4.
 * The number of 16bit argumentbytes is EBP-ESP-0x44 (68 Byte thunksetup).
 * [ok]
 */
VOID WINAPI QT_Thunk(CONTEXT *context)
{
	CONTEXT	context16;
	DWORD	argsize;

	memcpy(&context16,context,sizeof(context16));

	CS_reg(&context16)	 = HIWORD(EDX_reg(context));
	IP_reg(&context16)	 = LOWORD(EDX_reg(context));

	argsize = EBP_reg(context)-ESP_reg(context)-0x44;

	/* additional 4 bytes used by the relaycode for storing the stackptr */
	memcpy(	((LPBYTE)CURRENT_STACK16)-argsize-4,
		(LPBYTE)ESP_reg(context)+4,
		argsize
	);
	EAX_reg(context) = CallTo16_regs_short(&context16,-argsize);
}


/**********************************************************************
 *           WOWCallback16 (KERNEL32.62)
 */
DWORD WINAPI WOWCallback16(FARPROC16 fproc,DWORD arg)
{
	DWORD	ret;
	fprintf(stderr,"WOWCallback16(%p,0x%08lx) ",fproc,arg);
	ret =  CallTo16_long_l(fproc,arg);
	fprintf(stderr,"... returns %ld\n",ret);
	return ret;
}

/***********************************************************************
 *           _KERNEL32_52    (KERNEL32.52)
 * Returns a pointer to ThkBuf in the 16bit library SYSTHUNK.DLL.
 * [ok probably]
 */
LPVOID WINAPI _KERNEL32_52()
{
	HMODULE32	hmod = LoadLibrary16("systhunk.dll");

	if (hmod<=32)
		return 0;
	return PTR_SEG_TO_LIN(WIN32_GetProcAddress16(hmod,"ThkBuf"));
}

/***********************************************************************
 * 		_KERNEL32_43 	(KERNEL32.42)
 * A thunkbuffer link routine 
 * The thunkbuf looks like:
 *
 *	00: DWORD	length		? don't know exactly
 *	04: SEGPTR	ptr		? where does it point to?
 * The pointer ptr is written into the first DWORD of 'thunk'.
 * (probably correct implemented)
 * [ok probably]
 */
DWORD WINAPI _KERNEL32_43(LPDWORD thunk,LPCSTR thkbuf,DWORD len,
                           LPCSTR dll16,LPCSTR dll32)
{
	HINSTANCE16	hmod;
	LPDWORD		addr;
	SEGPTR		segaddr;

	hmod = LoadLibrary16(dll16);
	if (hmod<32) {
		fprintf(stderr,"KERNEL32_43->failed to load 16bit DLL %s, error %d\n",dll16,hmod);
		return 0;
	}
	segaddr = (DWORD)WIN32_GetProcAddress16(hmod,(LPSTR)thkbuf);
	if (!segaddr) {
		fprintf(stderr,"KERNEL32_43->no %s exported from %s!\n",thkbuf,dll16);
		return 0;
	}
	addr = (LPDWORD)PTR_SEG_TO_LIN(segaddr);
	if (addr[0] != len) {
		fprintf(stderr,"KERNEL32_43->thkbuf length mismatch? %ld vs %ld\n",len,addr[0]);
		return 0;
	}
	if (!addr[1])
		return 0;
	*(DWORD*)thunk = addr[1];
	return addr[1];
}

/***********************************************************************
 * 		_KERNEL32_45 	(KERNEL32.44)
 * Another 32->16 thunk, the difference to QT_Thunk is, that the called routine
 * uses 0x66 lret, and that we have to pass CX in DI.
 * (there seems to be some kind of BL/BX return magic too...)
 *
 * [doesn't crash anymore]
 */
VOID WINAPI _KERNEL32_45(CONTEXT *context)
{
	CONTEXT	context16;
	LPBYTE	curstack;
	DWORD	ret,stacksize;

	fprintf(stderr,"KERNEL32_45(%%eax=0x%08lx(%%cx=0x%04lx,%%edx=0x%08lx))\n",
		(DWORD)EAX_reg(context),(DWORD)CX_reg(context),(DWORD)EDX_reg(context)
	);
	stacksize = EBP_reg(context)-ESP_reg(context);
	fprintf(stderr,"	stacksize = %ld\n",stacksize);

	memcpy(&context16,context,sizeof(context16));

	DI_reg(&context16)	 = CX_reg(context);
	CS_reg(&context16)	 = HIWORD(EAX_reg(context));
	IP_reg(&context16)	 = LOWORD(EAX_reg(context));

	curstack = PTR_SEG_TO_LIN(STACK16_PUSH(stacksize));
	memcpy(curstack-stacksize,(LPBYTE)ESP_reg(context),stacksize);
	ret = CallTo16_regs_long(&context16,0);
	STACK16_POP(stacksize);

	fprintf(stderr,". returned %08lx\n",ret);
	EAX_reg(context) 	 = ret;
}

/***********************************************************************
 * 		_KERNEL32_40 	(KERNEL32.40)
 * YET Another 32->16 thunk, the difference to the others is still mysterious
 * target address is EDX
 *
 * [crashes]
 */
VOID WINAPI _KERNEL32_40(CONTEXT *context)
{
	CONTEXT	context16;
	LPBYTE	curstack;
	DWORD	ret,stacksize;

	fprintf(stderr,"_KERNEL32_40(EDX=0x%08lx)\n",
		EDX_reg(context)
	);
	stacksize = EBP_reg(context)-ESP_reg(context);
	fprintf(stderr,"	stacksize = %ld\n",stacksize);
	fprintf(stderr,"on top of stack: 0x%04x\n",*(WORD*)ESP_reg(context));

	memcpy(&context16,context,sizeof(context16));

	CS_reg(&context16)	 = HIWORD(EDX_reg(context));
	IP_reg(&context16)	 = LOWORD(EDX_reg(context));

	curstack = PTR_SEG_TO_LIN(STACK16_PUSH(stacksize));
	memcpy(curstack-stacksize,(LPBYTE)ESP_reg(context),stacksize);
	ret = CallTo16_regs_short(&context16,0);
	STACK16_POP(stacksize);

	fprintf(stderr,". returned %08lx\n",ret);
	EAX_reg(context) 	 = ret;
}

/***********************************************************************
 *		(KERNEL32.41)
 * A thunk setup routine.
 * Expects a pointer to a preinitialized thunkbuffer in the first argument
 * looking like:
 *	00..03:		unknown	(pointer, check _41, _43, _46)
 *	04: EB1E		jmp +0x20
 *
 *	06..23:		unknown (space for replacement code, check .90)
 *
 *	24:>E800000000		call offset 29
 *	29:>58			pop eax		   ( target of call )
 *	2A: 2D25000000		sub eax,0x00000025 ( now points to offset 4 )
 *	2F: BAxxxxxxxx		mov edx,xxxxxxxx
 *	34: 68yyyyyyyy		push KERNEL32.90
 *	39: C3			ret
 *
 *	3A: EB1E		jmp +0x20
 *	3E ... 59:	unknown (space for replacement code?)
 *	5A: E8xxxxxxxx		call <32bitoffset xxxxxxxx>
 *	5F: 5A			pop edx
 *	60: 81EA25xxxxxx	sub edx, 0x25xxxxxx
 *	66: 52			push edx
 *	67: 68xxxxxxxx		push xxxxxxxx
 *	6C: 68yyyyyyyy		push KERNEL32.89
 *	71: C3			ret
 *	72: end?
 * This function checks if the code is there, and replaces the yyyyyyyy entries
 * by the functionpointers.
 * The thunkbuf looks like:
 *
 *	00: DWORD	length		? don't know exactly
 *	04: SEGPTR	ptr		? where does it point to?
 * The segpointer ptr is written into the first DWORD of 'thunk'.
 * [ok probably]
 */

LPVOID WINAPI _KERNEL32_41(LPBYTE thunk,LPCSTR thkbuf,DWORD len,LPCSTR dll16,
                           LPCSTR dll32)
{
	HMODULE32	hkrnl32 = GetModuleHandle32A("KERNEL32");
	HMODULE16	hmod;
	LPDWORD		addr,addr2;
	DWORD		segaddr;

	/* FIXME: add checks for valid code ... */
	/* write pointers to kernel32.89 and kernel32.90 (+ordinal base of 1) */
	*(DWORD*)(thunk+0x35) = (DWORD)GetProcAddress32(hkrnl32,(LPSTR)90);
	*(DWORD*)(thunk+0x6D) = (DWORD)GetProcAddress32(hkrnl32,(LPSTR)89);

	
	hmod = LoadLibrary16(dll16);
	if (hmod<32) {
		fprintf(stderr,"KERNEL32_41->failed to load 16bit DLL %s, error %d\n",dll16,hmod);
		return NULL;
	}
	segaddr = (DWORD)WIN32_GetProcAddress16(hmod,(LPSTR)thkbuf);
	if (!segaddr) {
		fprintf(stderr,"KERNEL32_41->no %s exported from %s!\n",thkbuf,dll16);
		return NULL;
	}
	addr = (LPDWORD)PTR_SEG_TO_LIN(segaddr);
	if (addr[0] != len) {
		fprintf(stderr,"KERNEL32_41->thkbuf length mismatch? %ld vs %ld\n",len,addr[0]);
		return NULL;
	}
	addr2 = PTR_SEG_TO_LIN(addr[1]);
	if (HIWORD(addr2))
		*(DWORD*)thunk = (DWORD)addr2;
	return addr2;
}

/***********************************************************************
 *							(KERNEL32.90)
 * QT Thunk priming function
 * Rewrites the first part of the thunk to use the QT_Thunk interface
 * and jumps to the start of that code.
 * [ok]
 */
VOID WINAPI _KERNEL32_90(CONTEXT *context)
{
	_write_qtthunk((LPBYTE)EAX_reg(context),*(DWORD*)(EAX_reg(context)+EDX_reg(context)));
	/* we just call the real QT_Thunk right now 
	 * we can bypass the relaycode, for we already have the registercontext
	 */
	EDX_reg(context) = *(DWORD*)((*(DWORD*)(EAX_reg(context)+EDX_reg(context)))+4*(((BYTE*)EBP_reg(context))[-4]));
	return QT_Thunk(context);
}

/***********************************************************************
 *							(KERNEL32.45)
 * Another thunkbuf link routine.
 * The start of the thunkbuf looks like this:
 * 	00: DWORD	length
 *	04: SEGPTR	address for thunkbuffer pointer
 * [ok probably]
 */
VOID WINAPI _KERNEL32_46(LPBYTE thunk,LPSTR thkbuf,DWORD len,LPSTR dll16,
                         LPSTR dll32)
{
	LPDWORD		addr;
	HMODULE16	hmod;
	SEGPTR		segaddr;

	hmod = LoadLibrary16(dll16);
	if (hmod < 32) {
		fprintf(stderr,"KERNEL32_46->couldn't load %s, error %d\n",dll16,hmod);
		return;
	}
	segaddr = (SEGPTR)WIN32_GetProcAddress16(hmod,thkbuf);
	if (!segaddr) {
		fprintf(stderr,"KERNEL32_46-> haven't found %s in %s!\n",thkbuf,dll16);
		return;
	}
	addr = (LPDWORD)PTR_SEG_TO_LIN(segaddr);
	if (addr[0] != len) {
		fprintf(stderr,"KERNEL32_46-> length of thkbuf differs from expected length! (%ld vs %ld)\n",addr[0],len);
		return;
	}
	*(DWORD*)PTR_SEG_TO_LIN(addr[1]) = (DWORD)thunk;
}

/**********************************************************************
 *           _KERNEL32_87
 * Check if thunking is initialized (ss selector set up etc.)
 * We do that differently, so just return TRUE.
 * [ok]
 */
BOOL32 WINAPI _KERNEL32_87()
{
    return TRUE;
}

/**********************************************************************
 *           _KERNEL32_88
 * One of the real thunking functions. This one seems to be for 32<->32
 * thunks. It should probably be capable of crossing processboundaries.
 *
 * And YES, I've seen nr=48 (somewhere in the Win95 32<->16 OLE coupling)
 * [ok]
 */
DWORD WINAPIV _KERNEL32_88( DWORD nr, DWORD flags, FARPROC32 fun, ... )
{
    DWORD i,ret;
    DWORD *args = ((DWORD *)&fun) + 1;

    fprintf(stderr,"KERNEL32_88(%ld,0x%08lx,%p,[ ",nr,flags,fun);
    for (i=0;i<nr/4;i++) fprintf(stderr,"0x%08lx,",args[i]);
    fprintf(stderr,"])");
    switch (nr) {
    case 0:	ret = fun();
		break;
    case 4:	ret = fun(args[0]);
		break;
    case 8:	ret = fun(args[0],args[1]);
		break;
    case 12:	ret = fun(args[0],args[1],args[2]);
		break;
    case 16:	ret = fun(args[0],args[1],args[2],args[3]);
		break;
    case 20:	ret = fun(args[0],args[1],args[2],args[3],args[4]);
		break;
    case 24:	ret = fun(args[0],args[1],args[2],args[3],args[4],args[5]);
		break;
    case 28:	ret = fun(args[0],args[1],args[2],args[3],args[4],args[5],args[6]);
		break;
    case 32:	ret = fun(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7]);
		break;
    case 36:	ret = fun(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8]);
		break;
    case 40:	ret = fun(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9]);
		break;
    case 44:	ret = fun(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10]);
		break;
    case 48:	ret = fun(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11]);
		break;
    default:
	fprintf(stderr,"    unsupported nr of arguments, %ld\n",nr);
	ret = 0;
	break;

    }
    fprintf(stderr," returning %ld ...\n",ret);
    return ret;
}

/**********************************************************************
 * 		KERNEL_619		(KERNEL)
 * Seems to store y and z depending on x in some internal lists...
 */
WORD WINAPI _KERNEL_619(WORD x,DWORD y,DWORD z)
{
    fprintf(stderr,"KERNEL_619(0x%04x,0x%08lx,0x%08lx)\n",x,y,z);
    return x;
}

/**********************************************************************
 *			AllocSLCallback		(KERNEL32)
 *
 * Win95 uses some structchains for callbacks. It allocates them
 * in blocks of 100 entries, size 32 bytes each, layout:
 * blockstart:
 * 	0:	PTR	nextblockstart
 *	4:	entry	*first;
 *	8:	WORD	sel ( start points to blockstart)
 *	A:	WORD	unknown
 * 100xentry:
 *	00..17:		Code
 *	18:	PDB	*owning_process;
 *	1C:	PTR	blockstart
 *
 * We ignore this for now. (Just a note for further developers)
 * FIXME: use this method, so we don't waste selectors...
 *
 * Following code is then generated by AllocSLCallback. The code is 16 bit, so
 * the 0x66 prefix switches from word->long registers.
 *
 *	665A		pop	edx 
 *	6668x arg2 x 	pushl	<arg2>
 *	6652		push	edx
 *	EAx arg1 x	jmpf	<arg1>
 *
 * returns the startaddress of this thunk.
 *
 * Note, that they look very similair to the ones allocates by THUNK_Alloc.
 */
DWORD WINAPI
AllocSLCallback(DWORD finalizer,DWORD callback) {
	LPBYTE	x,thunk = HeapAlloc( GetProcessHeap(), 0, 32 );
	WORD	sel;

	x=thunk;
	*x++=0x66;*x++=0x5a;				/* popl edx */
	*x++=0x66;*x++=0x68;*(DWORD*)x=finalizer;x+=4;	/* pushl finalizer */
	*x++=0x66;*x++=0x52;				/* pushl edx */
	*x++=0xea;*(DWORD*)x=callback;x+=4;		/* jmpf callback */

	*(DWORD*)(thunk+18) = GetCurrentProcessId();

	sel = SELECTOR_AllocBlock( thunk , 32, SEGMENT_CODE, FALSE, FALSE );
	return (sel<<16)|0;
}

void WINAPI
FreeSLCallback(DWORD x) {
	fprintf(stderr,"FreeSLCallback(0x%08lx)\n",x);
}

/**********************************************************************
 * 		KERNEL_358		(KERNEL)
 * Allocates a code segment which starts at the address passed in x. limit
 * 0xfffff, and returns the pointer to the start.
 */
DWORD WINAPI
_KERNEL_358(DWORD x) {
	WORD	sel;

	fprintf(stderr,"_KERNEL_358(0x%08lx),stub\n",x);
	if (!HIWORD(x))
		return x;

	sel = SELECTOR_AllocBlock( PTR_SEG_TO_LIN(x) , 0xffff, SEGMENT_CODE, FALSE, FALSE );
	return (sel<<16)|(0x0000);
}

/**********************************************************************
 * 		KERNEL_359		(KERNEL)
 * Frees the code segment of the passed linear pointer (This has usually
 * been allocated by _KERNEL_358).
 */
VOID WINAPI
_KERNEL_359(DWORD x) {
	DWORD	savedsssp;

	fprintf(stderr,"_KERNEL_359(0x%08lx),stub\n",x);
	if ((HIWORD(x) & 7)!=7)
		return;
	savedsssp = IF1632_Saved16_ss_sp;IF1632_Saved16_ss_sp = 0;
	SELECTOR_FreeBlock(x>>16,1);
	IF1632_Saved16_ss_sp = savedsssp;
	return;
}

/**********************************************************************
 * 		KERNEL_472		(KERNEL)
 * something like GetCurrenthInstance.
 */
VOID WINAPI
_KERNEL_472(CONTEXT *context) {
	fprintf(stderr,"_KERNEL_472(0x%08lx),stub\n",EAX_reg(context));
	if (!EAX_reg(context)) {
		TDB *pTask = (TDB*)GlobalLock16(GetCurrentTask());
		AX_reg(context)=pTask->hInstance;
		return;
	}
	if (!HIWORD(EAX_reg(context)))
		return; /* returns the passed value */
	/* hmm ... fixme */
}
