/*
 * Emulator thunks
 *
 * Copyright 1996 Alexandre Julliard
 */

#include "windows.h"
#include "callback.h"
#include "heap.h"
#include "hook.h"
#include "module.h"
#include "stddebug.h"
#include "debug.h"

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

/***********************************************************************
 *           THUNK_Alloc
 */
static THUNK *THUNK_Alloc( FARPROC32 func, RELAY relay )
{
    THUNK *thunk = HeapAlloc( SystemHeap, 0, sizeof(*thunk) );
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
void THUNK_Free( THUNK *thunk )
{
    if (HEAP_IsInsideHeap( SystemHeap, 0, thunk ))
    {
        THUNK **prev = &firstThunk;
        while (*prev && (*prev != thunk)) prev = &(*prev)->next;
        if (*prev)
        {
            *prev = thunk->next;
            HeapFree( SystemHeap, 0, thunk );
            return;
        }
    }
    fprintf( stderr, "THUNK_Free: invalid thunk addr %p\n", thunk );
}


/***********************************************************************
 *           THUNK_EnumObjects16   (GDI.71)
 */
INT16 THUNK_EnumObjects16( HDC16 hdc, INT16 nObjType,
                           GOBJENUMPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo16_word_ll );
    return EnumObjects16( hdc, nObjType, (GOBJENUMPROC16)&thunk, lParam );
}


/***********************************************************************
 *           THUNK_EnumObjects32   (GDI32.89)
 */
INT32 THUNK_EnumObjects32( HDC32 hdc, INT32 nObjType,
                           GOBJENUMPROC32 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo32_2 );
    return EnumObjects32( hdc, nObjType, (GOBJENUMPROC32)&thunk, lParam );
}


/*************************************************************************
 *           THUNK_EnumFonts16   (GDI.70)
 */
INT16 THUNK_EnumFonts16( HDC16 hdc, LPCSTR lpFaceName,
                         FONTENUMPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo16_word_llwl );
    return EnumFonts16( hdc, lpFaceName, (FONTENUMPROC16)&thunk, lParam );
}

/*************************************************************************
 *           THUNK_EnumFonts32A   (GDI32.84)
 */
INT32 THUNK_EnumFonts32A( HDC32 hdc, LPCSTR lpFaceName,
                          FONTENUMPROC32A func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo32_4 );
    return EnumFonts32A( hdc, lpFaceName, (FONTENUMPROC32A)&thunk, lParam );
}

/*************************************************************************
 *           THUNK_EnumFonts32W   (GDI32.85)
 */
INT32 THUNK_EnumFonts32W( HDC32 hdc, LPCWSTR lpFaceName,
                          FONTENUMPROC32W func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo32_4 );
    return EnumFonts32W( hdc, lpFaceName, (FONTENUMPROC32W)&thunk, lParam );
}

/******************************************************************
 *           THUNK_EnumMetaFile16   (GDI.175)
 */
BOOL16 THUNK_EnumMetaFile16( HDC16 hdc, HMETAFILE16 hmf,
                             MFENUMPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo16_word_wllwl );
    return EnumMetaFile( hdc, hmf, (MFENUMPROC16)&thunk, lParam );
}


/*************************************************************************
 *           THUNK_EnumFontFamilies16   (GDI.330)
 */
INT16 THUNK_EnumFontFamilies16( HDC16 hdc, LPCSTR lpszFamily,
                                FONTENUMPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo16_word_llwl );
    return EnumFontFamilies16(hdc, lpszFamily, (FONTENUMPROC16)&thunk, lParam);
}


/*************************************************************************
 *           THUNK_EnumFontFamilies32A   (GDI32.80)
 */
INT32 THUNK_EnumFontFamilies32A( HDC32 hdc, LPCSTR lpszFamily,
                                 FONTENUMPROC32A func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo32_4 );
    return EnumFontFamilies32A(hdc,lpszFamily,(FONTENUMPROC32A)&thunk,lParam);
}


/*************************************************************************
 *           THUNK_EnumFontFamilies32W   (GDI32.83)
 */
INT32 THUNK_EnumFontFamilies32W( HDC32 hdc, LPCWSTR lpszFamily,
                                 FONTENUMPROC32W func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo32_4 );
    return EnumFontFamilies32W(hdc,lpszFamily,(FONTENUMPROC32W)&thunk,lParam);
}

/*************************************************************************
 *           THUNK_EnumFontFamiliesEx16   (GDI.613)
 */
INT16 THUNK_EnumFontFamiliesEx16( HDC16 hdc, LPLOGFONT16 lpLF,
                                  FONTENUMPROCEX16 func, LPARAM lParam,
                                  DWORD reserved )
{
    DECL_THUNK( thunk, func, CallTo16_word_llwl );
    return EnumFontFamiliesEx16( hdc, lpLF, (FONTENUMPROCEX16)&thunk,
                                 lParam, reserved );
}


/*************************************************************************
 *           THUNK_EnumFontFamiliesEx32A   (GDI32.81)
 */
INT32 THUNK_EnumFontFamiliesEx32A( HDC32 hdc, LPLOGFONT32A lpLF,
                                   FONTENUMPROCEX32A func, LPARAM lParam,
                                   DWORD reserved)
{
    DECL_THUNK( thunk, func, CallTo32_4 );
    return EnumFontFamiliesEx32A( hdc, lpLF, (FONTENUMPROCEX32A)&thunk,
                                  lParam, reserved );
}


/*************************************************************************
 *           THUNK_EnumFontFamiliesEx32W   (GDI32.82)
 */
INT32 THUNK_EnumFontFamiliesEx32W( HDC32 hdc, LPLOGFONT32W lpLF,
                                   FONTENUMPROCEX32W func, LPARAM lParam,
                                   DWORD reserved )
{
    DECL_THUNK( thunk, func, CallTo32_4 );
    return EnumFontFamiliesEx32W( hdc, lpLF, (FONTENUMPROCEX32W)&thunk,
                                  lParam, reserved );
}


/**********************************************************************
 *           THUNK_LineDDA16   (GDI.100)
 */
void THUNK_LineDDA16( INT16 nXStart, INT16 nYStart, INT16 nXEnd, INT16 nYEnd,
                      LINEDDAPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo16_word_wwl );
    LineDDA16( nXStart, nYStart, nXEnd, nYEnd, (LINEDDAPROC16)&thunk, lParam );
}


/**********************************************************************
 *           THUNK_LineDDA32   (GDI32.248)
 */
BOOL32 THUNK_LineDDA32( INT32 nXStart, INT32 nYStart, INT32 nXEnd, INT32 nYEnd,
                        LINEDDAPROC32 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo32_3 );
    return LineDDA32( nXStart, nYStart, nXEnd, nYEnd,
                      (LINEDDAPROC32)&thunk, lParam );
}


/*******************************************************************
 *           THUNK_EnumWindows16   (USER.54)
 */
BOOL16 THUNK_EnumWindows16( WNDENUMPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo16_word_wl );
    return EnumWindows16( (WNDENUMPROC16)&thunk, lParam );
}


/*******************************************************************
 *           THUNK_EnumWindows32   (USER32.192)
 */
BOOL32 THUNK_EnumWindows32( WNDENUMPROC32 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo32_2 );
    return EnumWindows32( (WNDENUMPROC32)&thunk, lParam );
}


/**********************************************************************
 *           THUNK_EnumChildWindows16   (USER.55)
 */
BOOL16 THUNK_EnumChildWindows16( HWND16 parent, WNDENUMPROC16 func,
                                 LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo16_word_wl );
    return EnumChildWindows16( parent, (WNDENUMPROC16)&thunk, lParam );
}


/**********************************************************************
 *           THUNK_EnumChildWindows32   (USER32.177)
 */
BOOL32 THUNK_EnumChildWindows32( HWND32 parent, WNDENUMPROC32 func,
                                 LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo32_2 );
    return EnumChildWindows32( parent, (WNDENUMPROC32)&thunk, lParam );
}


/**********************************************************************
 *           THUNK_EnumTaskWindows16   (USER.225)
 */
BOOL16 THUNK_EnumTaskWindows16( HTASK16 hTask, WNDENUMPROC16 func,
                                LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo16_word_wl );
    return EnumTaskWindows16( hTask, (WNDENUMPROC16)&thunk, lParam );
}


/**********************************************************************
 *           THUNK_EnumThreadWindows   (USER32.189)
 */
BOOL32 THUNK_EnumThreadWindows( DWORD id, WNDENUMPROC32 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo32_2 );
    return EnumThreadWindows( id, (WNDENUMPROC32)&thunk, lParam );
}


/***********************************************************************
 *           THUNK_EnumProps16   (USER.27)
 */
INT16 THUNK_EnumProps16( HWND16 hwnd, PROPENUMPROC16 func )
{
    DECL_THUNK( thunk, func, CallTo16_word_wlw );
    return EnumProps16( hwnd, (PROPENUMPROC16)&thunk );
}


/***********************************************************************
 *           THUNK_EnumProps32A   (USER32.185)
 */
INT32 THUNK_EnumProps32A( HWND32 hwnd, PROPENUMPROC32A func )
{
    DECL_THUNK( thunk, func, CallTo32_3 );
    return EnumProps32A( hwnd, (PROPENUMPROC32A)&thunk );
}


/***********************************************************************
 *           THUNK_EnumProps32W   (USER32.188)
 */
INT32 THUNK_EnumProps32W( HWND32 hwnd, PROPENUMPROC32W func )
{
    DECL_THUNK( thunk, func, CallTo32_3 );
    return EnumProps32W( hwnd, (PROPENUMPROC32W)&thunk );
}


/***********************************************************************
 *           THUNK_EnumPropsEx32A   (USER32.186)
 */
INT32 THUNK_EnumPropsEx32A( HWND32 hwnd, PROPENUMPROCEX32A func, LPARAM lParam)
{
    DECL_THUNK( thunk, func, CallTo32_4 );
    return EnumPropsEx32A( hwnd, (PROPENUMPROCEX32A)&thunk, lParam );
}


/***********************************************************************
 *           THUNK_EnumPropsEx32W   (USER32.187)
 */
INT32 THUNK_EnumPropsEx32W( HWND32 hwnd, PROPENUMPROCEX32W func, LPARAM lParam)
{
    DECL_THUNK( thunk, func, CallTo32_4 );
    return EnumPropsEx32W( hwnd, (PROPENUMPROCEX32W)&thunk, lParam );
}


/***********************************************************************
 *           THUNK_EnumSystemCodePages32A	(KERNEL32.92)
 */
BOOL32 THUNK_EnumSystemCodePages32A( CODEPAGE_ENUMPROC32A func, DWORD flags )
{
    DECL_THUNK( thunk, func, CallTo32_1 );
    return EnumSystemCodePages32A( (CODEPAGE_ENUMPROC32A)&thunk, flags );
}


/***********************************************************************
 *           THUNK_EnumSystemCodePages32W	(KERNEL32.93)
 */
BOOL32 THUNK_EnumSystemCodePages32W( CODEPAGE_ENUMPROC32W func, DWORD flags )
{
    DECL_THUNK( thunk, func, CallTo32_1 );
    return EnumSystemCodePages32W( (CODEPAGE_ENUMPROC32W)&thunk, flags );
}

/***********************************************************************
 *           THUNK_EnumSystemLocales32A   (KERNEL32.92)
 */
BOOL32 THUNK_EnumSystemLocales32A( LOCALE_ENUMPROC32A func, DWORD flags )
{
    DECL_THUNK( thunk, func, CallTo32_1 );
    return EnumSystemLocales32A( (LOCALE_ENUMPROC32A)&thunk, flags );
}


/***********************************************************************
 *           THUNK_EnumSystemLocales32W   (KERNEL32.93)
 */
BOOL32 THUNK_EnumSystemLocales32W( LOCALE_ENUMPROC32W func, DWORD flags )
{
    DECL_THUNK( thunk, func, CallTo32_1 );
    return EnumSystemLocales32W( (LOCALE_ENUMPROC32W)&thunk, flags );
}


/***********************************************************************
 *           THUNK_GrayString16   (USER.185)
 */
BOOL16 THUNK_GrayString16( HDC16 hdc, HBRUSH16 hbr, GRAYSTRINGPROC16 func,
                           LPARAM lParam, INT16 cch, INT16 x, INT16 y,
                           INT16 cx, INT16 cy )
{
    DECL_THUNK( thunk, func, CallTo16_word_wlw );
    if (!func)
        return GrayString( hdc, hbr, NULL, lParam, cch, x, y, cx, cy );
    else
        return GrayString( hdc, hbr, (GRAYSTRINGPROC16)&thunk, lParam, cch,
                           x, y, cx, cy );
}


/***********************************************************************
 *           THUNK_SetWindowsHook16   (USER.121)
 */
FARPROC16 THUNK_SetWindowsHook16( INT16 id, HOOKPROC16 proc )
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
BOOL16 THUNK_UnhookWindowsHook16( INT16 id, HOOKPROC16 proc )
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
HHOOK THUNK_SetWindowsHookEx16( INT16 id, HOOKPROC16 proc, HINSTANCE16 hInst,
                                HTASK16 hTask )
{
    THUNK *thunk = THUNK_Alloc( (FARPROC16)proc, (RELAY)CallTo16_long_wwl );
    if (!thunk) return 0;
    return SetWindowsHookEx16( id, (HOOKPROC16)thunk, hInst, hTask );
}


/***********************************************************************
 *           THUNK_UnhookWindowHookEx16   (USER.292)
 */
BOOL16 THUNK_UnhookWindowsHookEx16( HHOOK hhook )
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
BOOL16 THUNK_SetDCHook( HDC16 hdc, FARPROC16 proc, DWORD dwHookData )
{
    THUNK *thunk, *oldThunk;

    if (!defDCHookProc)  /* Get DCHook Win16 entry point */
        defDCHookProc = MODULE_GetEntryPoint( GetModuleHandle("USER"), 362 );

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
DWORD THUNK_GetDCHook( HDC16 hdc, FARPROC16 *phookProc )
{
    THUNK *thunk = NULL;
    DWORD ret = GetDCHook( hdc, (FARPROC16 *)&thunk );
    if (thunk)
    {
        if (thunk == (THUNK *)DCHook)
        {
            if (!defDCHookProc)  /* Get DCHook Win16 entry point */
                defDCHookProc = MODULE_GetEntryPoint( GetModuleHandle("USER"),
                                                      362 );
            *phookProc = defDCHookProc;
        }
        else *phookProc = thunk->proc;
    }
    return ret;
}


struct thunkstruct
{
	char	magic[4];
	DWORD	x1;
	DWORD	x2;
};

UINT32 ThunkConnect32( struct thunkstruct *ths, LPSTR thunkfun16,
                       LPSTR module16, LPSTR module32, HMODULE32 hmod32,
                       DWORD dllinitarg1 )
{
	HINSTANCE16	hmm;

	fprintf(stdnimp,"ThunkConnect32(<struct>,%s,%s,%s,%x,%lx)\n",
		thunkfun16,module32,module16,hmod32,dllinitarg1
	);
	fprintf(stdnimp,"	magic = %c%c%c%c\n",
		ths->magic[0],
		ths->magic[1],
		ths->magic[2],
		ths->magic[3]
	);
	fprintf(stdnimp,"	x1 = %lx\n",ths->x1);
	fprintf(stdnimp,"	x2 = %lx\n",ths->x2);
	hmm=LoadModule(module16,NULL);
	return TRUE;
}
