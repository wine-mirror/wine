/*
 * Windows hook functions
 *
 * Copyright 1994, 1995 Alexandre Julliard
 *                 1996 Andrew Lewycky
 *
 * Based on investigations by Alex Korobka
 */

/*
 * Warning!
 * A HHOOK is a 32-bit handle for compatibility with Windows 3.0 where it was
 * a pointer to the next function. Now it is in fact composed of a USER heap
 * handle in the low 16 bits and of a HOOK_MAGIC value in the high 16 bits.
 */

#include "windows.h"
#include "hook.h"
#include "queue.h"
#include "user.h"
#include "heap.h"
#include "struct32.h"
#include "winproc.h"
#include "stddebug.h"
#include "debug.h"

#pragma pack(1)

  /* Hook data (pointed to by a HHOOK) */
typedef struct
{
    HANDLE16   next;               /* 00 Next hook in chain */
    HOOKPROC32 proc WINE_PACKED;   /* 02 Hook procedure */
    INT16      id;                 /* 06 Hook id (WH_xxx) */
    HQUEUE16   ownerQueue;         /* 08 Owner queue (0 for system hook) */
    HMODULE16  ownerModule;        /* 0a Owner module */
    WORD       flags;              /* 0c flags */
} HOOKDATA;

#pragma pack(4)

#define HOOK_MAGIC  ((int)'H' | (int)'K' << 8)  /* 'HK' */

  /* This should probably reside in USER heap */
static HANDLE16 HOOK_systemHooks[WH_NB_HOOKS] = { 0, };

typedef VOID (*HOOK_MapFunc)(INT32, INT32, WPARAM32 *, LPARAM *);
typedef VOID (*HOOK_UnMapFunc)(INT32, INT32, WPARAM32, LPARAM, WPARAM32,
			       LPARAM);

/***********************************************************************
 *           HOOK_Map16To32Common
 */
static void HOOK_Map16To32Common(INT32 id, INT32 code, WPARAM32 *pwParam,
				 LPARAM *plParam, BOOL32 bA )
{

   switch( id )
   {
	case WH_MSGFILTER:
	case WH_SYSMSGFILTER: 
	case WH_GETMESSAGE: 
	case WH_JOURNALRECORD:
        {
            LPMSG16 lpmsg16 = PTR_SEG_TO_LIN(*plParam);
            LPMSG32 lpmsg32 = HeapAlloc( SystemHeap, 0, sizeof(*lpmsg32) );
	
            STRUCT32_MSG16to32( lpmsg16, lpmsg32 );
            *plParam = (LPARAM)lpmsg32;
	    break;
        } 

	case WH_JOURNALPLAYBACK:
        {
            LPEVENTMSG16 lpem16 = PTR_SEG_TO_LIN(*plParam);
            LPEVENTMSG32 lpem32 = HeapAlloc( SystemHeap, 0, sizeof(*lpem32) );

            lpem32->message = lpem16->message;
            lpem32->paramL = lpem16->paramL;
            lpem32->paramH = lpem16->paramH;
            lpem32->time = lpem16->time;
            lpem32->hwnd = 0;	/* FIXME */

            *plParam = (LPARAM)lpem32;
	    break;
        } 

	case WH_CALLWNDPROC:
	{
	    LPCWPSTRUCT16   lpcwp16 = PTR_SEG_TO_LIN(*plParam);
	    LPCWPSTRUCT32   lpcwp32 = HeapAlloc( SystemHeap, 0, sizeof(*lpcwp32) );
	    
	    lpcwp32->hwnd = lpcwp16->hwnd;
	    lpcwp32->lParam = lpcwp16->lParam;
	    
            if (bA) WINPROC_MapMsg16To32A( lpcwp16->message, lpcwp16->wParam, 
                                           &lpcwp32->message, &lpcwp32->wParam,
                                           &lpcwp32->lParam );
            else WINPROC_MapMsg16To32W( lpcwp16->message, lpcwp16->wParam, 
                                        &lpcwp32->message, &lpcwp32->wParam,
                                        &lpcwp32->lParam );
	    *plParam = (LPARAM)lpcwp32;
	    break;
	}

	case WH_CBT:
	  switch (code)
	  {
	    case HCBT_CREATEWND:
	    {
		LPCBT_CREATEWND16  lpcbtcw16 = PTR_SEG_TO_LIN(*plParam);
		LPCREATESTRUCT16   lpcs16 = PTR_SEG_TO_LIN(lpcbtcw16->lpcs);
		LPCBT_CREATEWND32A lpcbtcw32 = HeapAlloc( SystemHeap, 0,
							  sizeof(*lpcbtcw32) );
		lpcbtcw32->lpcs = HeapAlloc( SystemHeap, 0,
					     sizeof(*lpcbtcw32->lpcs) );

		STRUCT32_CREATESTRUCT16to32A( lpcs16,
					     (LPCREATESTRUCT32A)lpcbtcw32->lpcs );

		if (HIWORD(lpcs16->lpszName))
		    lpcbtcw32->lpcs->lpszName = 
			(bA) ? PTR_SEG_TO_LIN(lpcs16->lpszName)
			     : HEAP_strdupAtoW( SystemHeap, 0,
                                                PTR_SEG_TO_LIN(lpcs16->lpszName) );
		else
		    lpcbtcw32->lpcs->lpszName = (LPCSTR)lpcs16->lpszName;

		if (HIWORD(lpcs16->lpszClass))
		    lpcbtcw32->lpcs->lpszClass =
			(bA) ? PTR_SEG_TO_LIN(lpcs16->lpszClass)
			     : HEAP_strdupAtoW( SystemHeap, 0,
                                                PTR_SEG_TO_LIN(lpcs16->lpszClass) );
		else
		    lpcbtcw32->lpcs->lpszClass = (LPCSTR)lpcs16->lpszClass;

		lpcbtcw32->hwndInsertAfter = lpcbtcw16->hwndInsertAfter;

		*plParam = (LPARAM)lpcbtcw32;
		break;
	    } 
	    case HCBT_ACTIVATE:
            {
                LPCBTACTIVATESTRUCT16 lpcas16 = PTR_SEG_TO_LIN(*plParam);
                LPCBTACTIVATESTRUCT32 lpcas32 = HeapAlloc( SystemHeap, 0,
                                                           sizeof(*lpcas32) );
                lpcas32->fMouse = lpcas16->fMouse;
                lpcas32->hWndActive = lpcas16->hWndActive;
                *plParam = (LPARAM)lpcas32;
                break;
            }
            case HCBT_CLICKSKIPPED:
            {
                LPMOUSEHOOKSTRUCT16 lpms16 = PTR_SEG_TO_LIN(*plParam);
                LPMOUSEHOOKSTRUCT32 lpms32 = HeapAlloc( SystemHeap, 0,
                                                        sizeof(*lpms32) );

                CONV_POINT16TO32( &lpms16->pt, &lpms32->pt );

                /* wHitTestCode may be negative, so convince compiler to do
                   correct sign extension. Yay. :| */
                lpms32->wHitTestCode = (INT32)((INT16)lpms16->wHitTestCode);

                lpms32->dwExtraInfo = lpms16->dwExtraInfo;
                lpms32->hwnd = lpms16->hwnd;
                *plParam = (LPARAM)lpms32;
                break;
            }
            case HCBT_MOVESIZE:
            {
                LPRECT16 lprect16 = PTR_SEG_TO_LIN(*plParam);
                LPRECT32 lprect32 = HeapAlloc( SystemHeap, 0,
                                               sizeof(*lprect32) );

                CONV_RECT16TO32( lprect16, lprect32 );
                *plParam = (LPARAM)lprect32;
                break;
            }
	  } 
	  break;

	case WH_MOUSE:
        {
            LPMOUSEHOOKSTRUCT16 lpms16 = PTR_SEG_TO_LIN(*plParam);
            LPMOUSEHOOKSTRUCT32 lpms32 = HeapAlloc( SystemHeap, 0,
                                                    sizeof(*lpms32) );

            CONV_POINT16TO32( &lpms16->pt, &lpms32->pt );

            /* wHitTestCode may be negative, so convince compiler to do
               correct sign extension. Yay. :| */
            lpms32->wHitTestCode = (INT32)((INT16)lpms16->wHitTestCode);
            lpms32->dwExtraInfo = lpms16->dwExtraInfo;
            lpms32->hwnd = lpms16->hwnd;
            *plParam = (LPARAM)lpms32;
	    break;
        } 

	case WH_DEBUG:
        {
            LPDEBUGHOOKINFO16 lpdh16 = PTR_SEG_TO_LIN(*plParam);
            LPDEBUGHOOKINFO32 lpdh32 = HeapAlloc( SystemHeap, 0,
                                                  sizeof(*lpdh32) );

            lpdh32->idThread = 0;               /* FIXME */
            lpdh32->idThreadInstaller = 0;	/* FIXME */
            lpdh32->lParam = lpdh16->lParam;	/* FIXME Check for sign ext */
            lpdh32->wParam = lpdh16->wParam;
            lpdh32->code = lpdh16->code;
	  
            /* do sign extension if it was WH_MSGFILTER */
            if (*pwParam == 0xffff) *pwParam = WH_MSGFILTER;

            *plParam = (LPARAM)lpdh32;
	    break;
        }

	case WH_SHELL:
	case WH_KEYBOARD:
	    break;

	case WH_HARDWARE: 
	case WH_FOREGROUNDIDLE: 
	case WH_CALLWNDPROCRET:
	    fprintf(stderr, "\t[%i] 16to32 translation unimplemented\n", id);
    }
}


/***********************************************************************
 *           HOOK_Map16To32A
 */
static void HOOK_Map16To32A(INT32 id, INT32 code, WPARAM32 *pwParam,
			    LPARAM *plParam)
{
    HOOK_Map16To32Common( id, code, pwParam, plParam, TRUE );
}


/***********************************************************************
 *           HOOK_Map16To32W
 */
static void HOOK_Map16To32W(INT32 id, INT32 code, WPARAM32 *pwParam,
			    LPARAM *plParam)
{
    HOOK_Map16To32Common( id, code, pwParam, plParam, FALSE );
}


/***********************************************************************
 *           HOOK_UnMap16To32Common
 */
static void HOOK_UnMap16To32Common(INT32 id, INT32 code, WPARAM32 wParamOrig,
				   LPARAM lParamOrig, WPARAM32 wParam,
				   LPARAM lParam, BOOL32 bA)
{
    switch (id)
    {
	case WH_MSGFILTER:
	case WH_SYSMSGFILTER:
	case WH_JOURNALRECORD:
	case WH_JOURNALPLAYBACK:
      
	    HeapFree( SystemHeap, 0, (LPVOID)lParam );
	    break;

	case WH_CALLWNDPROC:
	{
            LPCWPSTRUCT32   lpcwp32 = (LPCWPSTRUCT32)lParam;
            if (bA) WINPROC_UnmapMsg16To32A( lpcwp32->message, lpcwp32->wParam,
                                             lpcwp32->lParam, 0 );
            else WINPROC_UnmapMsg16To32W( lpcwp32->message, lpcwp32->wParam,
                                          lpcwp32->lParam, 0 );
	    HeapFree( SystemHeap, 0, lpcwp32 );
            break;
	}

	case WH_GETMESSAGE:
        {
	    LPMSG16 lpmsg16 = PTR_SEG_TO_LIN(lParamOrig);
	    STRUCT32_MSG32to16( (LPMSG32)lParam, lpmsg16 );
	    HeapFree( SystemHeap, 0, (LPVOID)lParam );
	    break;
        }

        case WH_MOUSE:
        case WH_DEBUG:

	    HeapFree( SystemHeap, 0, (LPVOID)lParam );
	    break;

        case WH_CBT:
	    switch (code)
  	    {
	      case HCBT_CREATEWND:
	      {
		LPCBT_CREATEWND32A lpcbtcw32 = (LPCBT_CREATEWND32A)lParam;
		LPCBT_CREATEWND16  lpcbtcw16 = PTR_SEG_TO_LIN(lParamOrig);

		if( !bA )
		{
		   if (HIWORD(lpcbtcw32->lpcs->lpszName))
                       HeapFree( SystemHeap, 0, (LPWSTR)lpcbtcw32->lpcs->lpszName );
		   if (HIWORD(lpcbtcw32->lpcs->lpszClass))
                       HeapFree( SystemHeap, 0, (LPWSTR)lpcbtcw32->lpcs->lpszClass );
		}

		lpcbtcw16->hwndInsertAfter = lpcbtcw32->hwndInsertAfter;

		HeapFree( SystemHeap, 0, lpcbtcw32->lpcs );
	      } /* fall through */

	      case HCBT_ACTIVATE:
	      case HCBT_CLICKSKIPPED:
	      case HCBT_MOVESIZE:

	        HeapFree( SystemHeap, 0, (LPVOID)lParam);
	        break;
	    }
  	    break;

        case WH_SHELL:
        case WH_KEYBOARD:
	    break;

        case WH_HARDWARE:
	case WH_FOREGROUNDIDLE:
	case WH_CALLWNDPROCRET:
	    fprintf(stderr, "\t[%i] skipping unmap\n", id);
  	    break;
    }
}


/***********************************************************************
 *           HOOK_UnMap16To32A
 */
static void HOOK_UnMap16To32A(INT32 id, INT32 code, WPARAM32 wParamOrig,
			      LPARAM lParamOrig, WPARAM32 wParam,
			      LPARAM lParam)
{
    HOOK_UnMap16To32Common( id, code, wParamOrig, lParamOrig, wParam,
			    lParam, TRUE );
}


/***********************************************************************
 *           HOOK_UnMap16To32W
 */
static void HOOK_UnMap16To32W(INT32 id, INT32 code, WPARAM32 wParamOrig,
			      LPARAM lParamOrig, WPARAM32 wParam,
			      LPARAM lParam)
{
    HOOK_UnMap16To32Common( id, code, wParamOrig, lParamOrig, wParam, 
			    lParam, FALSE );
}


/***********************************************************************
 *           HOOK_Map32To16Common
 */
static void HOOK_Map32To16Common(INT32 id, INT32 code, WPARAM32 *pwParam,
				 LPARAM *plParam, BOOL32 bA)
{
    switch (id)
    {
      case WH_MSGFILTER:
      case WH_SYSMSGFILTER:
      case WH_GETMESSAGE:
      case WH_JOURNALRECORD:
      {
	  LPMSG32 lpmsg32 = (LPMSG32)*plParam;
	  LPMSG16 lpmsg16 = SEGPTR_NEW( MSG16 );

	  STRUCT32_MSG32to16( lpmsg32, lpmsg16 );

	  *plParam = (LPARAM)SEGPTR_GET( lpmsg16 );
	  break;
      }

      case WH_JOURNALPLAYBACK:
      {
	  LPEVENTMSG32 lpem32 = (LPEVENTMSG32)*plParam;
	  LPEVENTMSG16 lpem16 = SEGPTR_NEW( EVENTMSG16 );

	  lpem16->message = lpem32->message;
	  lpem16->paramL  = lpem32->paramL;
	  lpem16->paramH  = lpem32->paramH;
	  lpem16->time    = lpem32->time;

	  *plParam = (LPARAM)SEGPTR_GET( lpem16 );
	  break;
      }

      case WH_CALLWNDPROC:
      {
          LPCWPSTRUCT32   lpcwp32 = (LPCWPSTRUCT32)*plParam;
	  LPCWPSTRUCT16   lpcwp16 = SEGPTR_NEW( CWPSTRUCT16 );

          lpcwp16->hwnd = lpcwp32->hwnd;
          lpcwp16->lParam = lpcwp32->lParam;

          if (bA) WINPROC_MapMsg32ATo16( lpcwp32->hwnd, lpcwp32->message,
                                         lpcwp32->wParam, &lpcwp16->message,
                                         &lpcwp16->wParam, &lpcwp16->lParam );
          else WINPROC_MapMsg32WTo16( lpcwp32->hwnd, lpcwp32->message,
                                      lpcwp32->wParam, &lpcwp16->message,
                                      &lpcwp16->wParam, &lpcwp16->lParam );
	  *plParam = (LPARAM)SEGPTR_GET( lpcwp16 );
          break;
      }

      case WH_CBT:
	switch (code)
	{
	  case HCBT_ACTIVATE:
	  {
	      LPCBTACTIVATESTRUCT32 lpcas32 = (LPCBTACTIVATESTRUCT32)*plParam;
	      LPCBTACTIVATESTRUCT16 lpcas16 =SEGPTR_NEW( CBTACTIVATESTRUCT16 );

	      lpcas16->fMouse     = lpcas32->fMouse;
	      lpcas16->hWndActive = lpcas32->hWndActive;

	      *plParam = (LPARAM)SEGPTR_GET( lpcas16 );
	      break;
	  }
	      
	  case HCBT_CLICKSKIPPED:
	  {
	      LPMOUSEHOOKSTRUCT32 lpms32 = (LPMOUSEHOOKSTRUCT32)*plParam;
	      LPMOUSEHOOKSTRUCT16 lpms16 = SEGPTR_NEW( MOUSEHOOKSTRUCT16 );

	      CONV_POINT32TO16( &lpms32->pt, &lpms16->pt );

	      lpms16->hwnd         = lpms32->hwnd;
	      lpms16->wHitTestCode = lpms32->wHitTestCode;
	      lpms16->dwExtraInfo  = lpms32->dwExtraInfo;

	      *plParam = (LPARAM)SEGPTR_GET( lpms16 );
	      break;
	  }

	  case HCBT_MOVESIZE:
	  {
	      LPRECT32 lprect32 = (LPRECT32)*plParam;
	      LPRECT16 lprect16 = SEGPTR_NEW( RECT16 );

	      CONV_RECT32TO16( lprect32, lprect16 );

	      *plParam = (LPARAM)SEGPTR_GET( lprect16 );
	      break;
	  }
	}
	break;

      case WH_MOUSE:
      {
	  LPMOUSEHOOKSTRUCT32 lpms32 = (LPMOUSEHOOKSTRUCT32)*plParam;
	  LPMOUSEHOOKSTRUCT16 lpms16 = SEGPTR_NEW( MOUSEHOOKSTRUCT16 );

	  CONV_POINT32TO16( &lpms32->pt, &lpms16->pt );

	  lpms16->hwnd = lpms32->hwnd;
	  lpms16->wHitTestCode = lpms32->wHitTestCode;
	  lpms16->dwExtraInfo = lpms32->dwExtraInfo;

	  *plParam = (LPARAM)SEGPTR_GET( lpms16 );
	  break;
      }

      case WH_DEBUG:
      {
	  LPDEBUGHOOKINFO32 lpdh32 = (LPDEBUGHOOKINFO32)*plParam;
	  LPDEBUGHOOKINFO16 lpdh16 = SEGPTR_NEW( DEBUGHOOKINFO16 );

	  lpdh16->hModuleHook = 0;	/* FIXME */
	  lpdh16->reserved    = 0;
	  lpdh16->lParam      = lpdh32->lParam;
	  lpdh16->wParam      = lpdh32->wParam;
	  lpdh16->code        = lpdh32->code;
	  
	  *plParam = (LPARAM)SEGPTR_GET( lpdh16 );
	  break;
      }

      case WH_SHELL:
      case WH_KEYBOARD:
	break;

      case WH_HARDWARE:
      case WH_FOREGROUNDIDLE:
      case WH_CALLWNDPROCRET:
	fprintf(stderr,"\t[%i] 32to16 translation unimplemented\n", id);
    }
}


/***********************************************************************
 *           HOOK_Map32ATo16
 */
static void HOOK_Map32ATo16(INT32 id, INT32 code, WPARAM32 *pwParam,
			    LPARAM *plParam)
{
    if (id == WH_CBT && code == HCBT_CREATEWND)
    {
	LPCBT_CREATEWND32A lpcbtcw32 = (LPCBT_CREATEWND32A)*plParam;
	LPCBT_CREATEWND16 lpcbtcw16 = SEGPTR_NEW( CBT_CREATEWND16 );
	LPCREATESTRUCT16 lpcs16 = SEGPTR_NEW( CREATESTRUCT16 );

	lpcbtcw16->lpcs = (LPCREATESTRUCT16)SEGPTR_GET( lpcs16 );
	STRUCT32_CREATESTRUCT32Ato16( lpcbtcw32->lpcs, lpcs16 );

	if (HIWORD(lpcbtcw32->lpcs->lpszName))
	  lpcs16->lpszName =
	    SEGPTR_GET( SEGPTR_STRDUP( lpcbtcw32->lpcs->lpszName ) );
	else
	  lpcs16->lpszName = (SEGPTR)lpcbtcw32->lpcs->lpszName;

	if (HIWORD(lpcbtcw32->lpcs->lpszClass))
	  lpcs16->lpszClass =
	    SEGPTR_GET( SEGPTR_STRDUP( lpcbtcw32->lpcs->lpszClass ) );
	else
	  lpcs16->lpszClass = (SEGPTR)lpcbtcw32->lpcs->lpszClass;

	lpcbtcw16->hwndInsertAfter = lpcbtcw32->hwndInsertAfter;

	*plParam = (LPARAM)SEGPTR_GET( lpcbtcw16 );
    }
    else HOOK_Map32To16Common(id, code, pwParam, plParam, TRUE);
}


/***********************************************************************
 *           HOOK_Map32WTo16
 */
static void HOOK_Map32WTo16(INT32 id, INT32 code, WPARAM32 *pwParam,
			    LPARAM *plParam)
{
    if (id == WH_CBT && code == HCBT_CREATEWND)
    {
        LPSTR name, cls;
	LPCBT_CREATEWND32W lpcbtcw32 = (LPCBT_CREATEWND32W)*plParam;
	LPCBT_CREATEWND16 lpcbtcw16 = SEGPTR_NEW( CBT_CREATEWND16 );
	LPCREATESTRUCT16 lpcs16 = SEGPTR_NEW( CREATESTRUCT16 );

	lpcbtcw16->lpcs = (LPCREATESTRUCT16)SEGPTR_GET( lpcs16 );
	STRUCT32_CREATESTRUCT32Ato16( (LPCREATESTRUCT32A)lpcbtcw32->lpcs,
				      lpcs16 );

        name = SEGPTR_STRDUP_WtoA( lpcbtcw32->lpcs->lpszName );
        cls  = SEGPTR_STRDUP_WtoA( lpcbtcw32->lpcs->lpszClass );
        lpcs16->lpszName  = SEGPTR_GET( name );
        lpcs16->lpszClass = SEGPTR_GET( cls );
	lpcbtcw16->hwndInsertAfter = lpcbtcw32->hwndInsertAfter;

	*plParam = (LPARAM)SEGPTR_GET( lpcbtcw16 );
    }
    else HOOK_Map32To16Common(id, code, pwParam, plParam, FALSE);
}


/***********************************************************************
 *           HOOK_UnMap32To16Common
 */
static void HOOK_UnMap32To16Common(INT32 id, INT32 code, WPARAM32 wParamOrig,
				   LPARAM lParamOrig, WPARAM32 wParam,
				   LPARAM lParam, BOOL32 bA)
{
    switch (id)
    {
      case WH_MSGFILTER:
      case WH_SYSMSGFILTER:
      case WH_JOURNALRECORD:
      case WH_JOURNALPLAYBACK:
      case WH_MOUSE:
      case WH_DEBUG:
	SEGPTR_FREE( PTR_SEG_TO_LIN(lParam) );
	break;

      case WH_CALLWNDPROC:
      {
          LPCWPSTRUCT16   lpcwp16 = (LPCWPSTRUCT16)PTR_SEG_TO_LIN(lParam);
	  LPCWPSTRUCT32   lpcwp32 = (LPCWPSTRUCT32)lParamOrig;
	  MSGPARAM16	  mp16 = { lpcwp16->wParam, lpcwp16->lParam, 0 };

          if (bA) WINPROC_UnmapMsg32ATo16( lpcwp32->message, lpcwp32->wParam,
                                           lpcwp32->lParam, &mp16 );
          else WINPROC_UnmapMsg32WTo16( lpcwp32->message, lpcwp32->wParam,
                                        lpcwp32->lParam, &mp16 );
	  SEGPTR_FREE( PTR_SEG_TO_LIN(lParam) );
          break;
      }

      case WH_GETMESSAGE:
      {
	  LPMSG32 lpmsg32 = (LPMSG32)lParamOrig;

	  STRUCT32_MSG16to32( (LPMSG16)PTR_SEG_TO_LIN(lParam), lpmsg32 );
	  SEGPTR_FREE( PTR_SEG_TO_LIN(lParam) );
	  break;
      }

      case WH_CBT:
	switch (code)
	{
	  case HCBT_CREATEWND:
	  {
	       LPCBT_CREATEWND32A lpcbtcw32 = (LPCBT_CREATEWND32A)(lParamOrig);
               LPCBT_CREATEWND16 lpcbtcw16 = PTR_SEG_TO_LIN(lParam);
               LPCREATESTRUCT16  lpcs16 = PTR_SEG_TO_LIN(lpcbtcw16->lpcs);

               if (HIWORD(lpcs16->lpszName))
                   SEGPTR_FREE( PTR_SEG_TO_LIN(lpcs16->lpszName) );

               if (HIWORD(lpcs16->lpszClass))
                   SEGPTR_FREE( PTR_SEG_TO_LIN(lpcs16->lpszClass) );

	       lpcbtcw32->hwndInsertAfter = lpcbtcw16->hwndInsertAfter;

               SEGPTR_FREE( lpcs16 );
	  } /* fall through */

	  case HCBT_ACTIVATE:
	  case HCBT_CLICKSKIPPED:
	  case HCBT_MOVESIZE:

	       SEGPTR_FREE( PTR_SEG_TO_LIN(lParam) );
	       break;
	}
	break;

      case WH_SHELL:
      case WH_KEYBOARD:
	break;

      case WH_HARDWARE:
      case WH_FOREGROUNDIDLE:
      case WH_CALLWNDPROCRET:
	fprintf(stderr, "\t[%i] skipping unmap\n", id);
    }
}


/***********************************************************************
 *           HOOK_UnMap32ATo16
 */
static void HOOK_UnMap32ATo16(INT32 id, INT32 code, WPARAM32 wParamOrig,
			      LPARAM lParamOrig, WPARAM32 wParam,
			      LPARAM lParam)
{
    HOOK_UnMap32To16Common( id, code, wParamOrig, lParamOrig, wParam,
			    lParam, TRUE );
}


/***********************************************************************
 *           HOOK_UnMap32WTo16
 */
static void HOOK_UnMap32WTo16(INT32 id, INT32 code, WPARAM32 wParamOrig,
			      LPARAM lParamOrig, WPARAM32 wParam,
			      LPARAM lParam)
{
    HOOK_UnMap32To16Common( id, code, wParamOrig, lParamOrig, wParam,
                            lParam, FALSE );
}


/***********************************************************************
 *           HOOK_Map32ATo32W
 */
static void HOOK_Map32ATo32W(INT32 id, INT32 code, WPARAM32 *pwParam,
			     LPARAM *plParam)
{
    if (id == WH_CBT && code == HCBT_CREATEWND)
    {
	LPCBT_CREATEWND32A lpcbtcwA = (LPCBT_CREATEWND32A)*plParam;
	LPCBT_CREATEWND32W lpcbtcwW = HeapAlloc( SystemHeap, 0,
						 sizeof(*lpcbtcwW) );
	lpcbtcwW->lpcs = HeapAlloc( SystemHeap, 0, sizeof(*lpcbtcwW->lpcs) );

	lpcbtcwW->hwndInsertAfter = lpcbtcwA->hwndInsertAfter;
	*lpcbtcwW->lpcs = *(LPCREATESTRUCT32W)lpcbtcwA->lpcs;

	if (HIWORD(lpcbtcwA->lpcs->lpszName))
	{
	    lpcbtcwW->lpcs->lpszName = HEAP_strdupAtoW( SystemHeap, 0,
                                                    lpcbtcwA->lpcs->lpszName );
	}
	else
	  lpcbtcwW->lpcs->lpszName = (LPWSTR)lpcbtcwA->lpcs->lpszName;

	if (HIWORD(lpcbtcwA->lpcs->lpszClass))
	{
	    lpcbtcwW->lpcs->lpszClass = HEAP_strdupAtoW( SystemHeap, 0,
                                                   lpcbtcwA->lpcs->lpszClass );
	}
	else
	  lpcbtcwW->lpcs->lpszClass = (LPCWSTR)lpcbtcwA->lpcs->lpszClass;
	*plParam = (LPARAM)lpcbtcwW;
    }
    return;
}


/***********************************************************************
 *           HOOK_UnMap32ATo32W
 */
static void HOOK_UnMap32ATo32W(INT32 id, INT32 code, WPARAM32 wParamOrig,
			       LPARAM lParamOrig, WPARAM32 wParam,
			       LPARAM lParam)
{
    if (id == WH_CBT && code == HCBT_CREATEWND)
    {
	LPCBT_CREATEWND32W lpcbtcwW = (LPCBT_CREATEWND32W)lParam;
	if (HIWORD(lpcbtcwW->lpcs->lpszName))
            HeapFree( SystemHeap, 0, (LPWSTR)lpcbtcwW->lpcs->lpszName );
	if (HIWORD(lpcbtcwW->lpcs->lpszClass))
            HeapFree( SystemHeap, 0, (LPWSTR)lpcbtcwW->lpcs->lpszClass );
	HeapFree( SystemHeap, 0, lpcbtcwW->lpcs );
	HeapFree( SystemHeap, 0, lpcbtcwW );
    }
    return;
}


/***********************************************************************
 *           HOOK_Map32WTo32A
 */
static void HOOK_Map32WTo32A(INT32 id, INT32 code, WPARAM32 *pwParam,
			     LPARAM *plParam)
{
    if (id == WH_CBT && code == HCBT_CREATEWND)
    {
	LPCBT_CREATEWND32W lpcbtcwW = (LPCBT_CREATEWND32W)*plParam;
	LPCBT_CREATEWND32A lpcbtcwA = HeapAlloc( SystemHeap, 0,
						 sizeof(*lpcbtcwA) );
	lpcbtcwA->lpcs = HeapAlloc( SystemHeap, 0, sizeof(*lpcbtcwA->lpcs) );

	lpcbtcwA->hwndInsertAfter = lpcbtcwW->hwndInsertAfter;
	*lpcbtcwA->lpcs = *(LPCREATESTRUCT32A)lpcbtcwW->lpcs;

	if (HIWORD(lpcbtcwW->lpcs->lpszName))
	  lpcbtcwA->lpcs->lpszName = HEAP_strdupWtoA( SystemHeap, 0,
                                                    lpcbtcwW->lpcs->lpszName );
	else
	  lpcbtcwA->lpcs->lpszName = (LPSTR)lpcbtcwW->lpcs->lpszName;

	if (HIWORD(lpcbtcwW->lpcs->lpszClass))
	  lpcbtcwA->lpcs->lpszClass = HEAP_strdupWtoA( SystemHeap, 0,
                                                   lpcbtcwW->lpcs->lpszClass );
	else
	  lpcbtcwA->lpcs->lpszClass = (LPSTR)lpcbtcwW->lpcs->lpszClass;
	*plParam = (LPARAM)lpcbtcwA;
    }
    return;
}


/***********************************************************************
 *           HOOK_UnMap32WTo32A
 */
static void HOOK_UnMap32WTo32A(INT32 id, INT32 code, WPARAM32 wParamOrig,
			       LPARAM lParamOrig, WPARAM32 wParam,
			       LPARAM lParam)
{
    if (id == WH_CBT && code == HCBT_CREATEWND)
    {
	LPCBT_CREATEWND32A lpcbtcwA = (LPCBT_CREATEWND32A)lParam;
	if (HIWORD(lpcbtcwA->lpcs->lpszName))
            HeapFree( SystemHeap, 0, (LPSTR)lpcbtcwA->lpcs->lpszName );
	if (HIWORD(lpcbtcwA->lpcs->lpszClass))
            HeapFree( SystemHeap, 0, (LPSTR)lpcbtcwA->lpcs->lpszClass );
	HeapFree( SystemHeap, 0, lpcbtcwA->lpcs );
	HeapFree( SystemHeap, 0, lpcbtcwA );
    }
    return;
}


/***********************************************************************
 *           Map Function Tables
 */
static const HOOK_MapFunc HOOK_MapFuncs[3][3] = 
{
    { NULL,            HOOK_Map16To32A,  HOOK_Map16To32W },
    { HOOK_Map32ATo16, NULL,             HOOK_Map32ATo32W },
    { HOOK_Map32WTo16, HOOK_Map32WTo32A, NULL }
};

static const HOOK_UnMapFunc HOOK_UnMapFuncs[3][3] = 
{
    { NULL,              HOOK_UnMap16To32A,  HOOK_UnMap16To32W },
    { HOOK_UnMap32ATo16, NULL,               HOOK_UnMap32ATo32W },
    { HOOK_UnMap32WTo16, HOOK_UnMap32WTo32A, NULL }
};


/***********************************************************************
 *           Internal Functions
 */

/***********************************************************************
 *           HOOK_GetNextHook
 *
 * Get the next hook of a given hook.
 */
static HANDLE16 HOOK_GetNextHook( HANDLE16 hook )
{
    HOOKDATA *data = (HOOKDATA *)USER_HEAP_LIN_ADDR( hook );

    if (!data || !hook) return 0;
    if (data->next) return data->next;
    if (!data->ownerQueue) return 0;  /* Already system hook */

    /* Now start enumerating the system hooks */
    return HOOK_systemHooks[data->id - WH_MINHOOK];
}


/***********************************************************************
 *           HOOK_GetHook
 *
 * Get the first hook for a given type.
 */
static HANDLE16 HOOK_GetHook( INT16 id, HQUEUE16 hQueue )
{
    MESSAGEQUEUE *queue;
    HANDLE16 hook = 0;

    if ((queue = (MESSAGEQUEUE *)GlobalLock16( hQueue )) != NULL)
        hook = queue->hooks[id - WH_MINHOOK];
    if (!hook) hook = HOOK_systemHooks[id - WH_MINHOOK];
    return hook;
}


/***********************************************************************
 *           HOOK_SetHook
 *
 * Install a given hook.
 */
static HANDLE16 HOOK_SetHook( INT16 id, LPVOID proc, INT32 type,
			      HINSTANCE16 hInst, HTASK16 hTask )
{
    HOOKDATA *data;
    HANDLE16 handle;
    HQUEUE16 hQueue = 0;

    if ((id < WH_MINHOOK) || (id > WH_MAXHOOK)) return 0;

    dprintf_hook( stddeb, "Setting hook %d: %08x %04x %04x\n",
                  id, (UINT32)proc, hInst, hTask );

    if (!hInst && (type!=HOOK_WIN16))
    	hInst = GetModuleHandle32A(NULL);/*FIXME: correct? probably not */

    if (id == WH_JOURNALPLAYBACK) EnableHardwareInput(FALSE);

    if (hTask)  /* Task-specific hook */
    {
	if ((id == WH_JOURNALRECORD) || (id == WH_JOURNALPLAYBACK) ||
	    (id == WH_SYSMSGFILTER)) return 0;  /* System-only hooks */
        if (!(hQueue = GetTaskQueue( hTask ))) return 0;
    }

    /* Create the hook structure */

    if (!(handle = USER_HEAP_ALLOC( sizeof(HOOKDATA) ))) return 0;
    data = (HOOKDATA *) USER_HEAP_LIN_ADDR( handle );
    data->proc        = proc;
    data->id          = id;
    data->ownerQueue  = hQueue;
    data->ownerModule = hInst;
    data->flags       = type;

    /* Insert it in the correct linked list */

    if (hQueue)
    {
        MESSAGEQUEUE *queue = (MESSAGEQUEUE *)GlobalLock16( hQueue );
        data->next = queue->hooks[id - WH_MINHOOK];
        queue->hooks[id - WH_MINHOOK] = handle;
    }
    else
    {
        data->next = HOOK_systemHooks[id - WH_MINHOOK];
        HOOK_systemHooks[id - WH_MINHOOK] = handle;
    }
    dprintf_hook( stddeb, "Setting hook %d: ret=%04x [next=%04x]\n", 
			   id, handle, data->next );
    return handle;
}


/***********************************************************************
 *           HOOK_RemoveHook
 *
 * Remove a hook from the list.
 */
static BOOL32 HOOK_RemoveHook( HANDLE16 hook )
{
    HOOKDATA *data;
    HANDLE16 *prevHook;

    dprintf_hook( stddeb, "Removing hook %04x\n", hook );

    if (!(data = (HOOKDATA *)USER_HEAP_LIN_ADDR(hook))) return FALSE;
    if (data->flags & HOOK_INUSE)
    {
        /* Mark it for deletion later on */
        dprintf_hook( stddeb, "Hook still running, deletion delayed\n" );
        data->proc = (HOOKPROC32)0;
        return TRUE;
    }

    if (data->id == WH_JOURNALPLAYBACK) EnableHardwareInput(TRUE);
     
    /* Remove it from the linked list */

    if (data->ownerQueue)
    {
        MESSAGEQUEUE *queue = (MESSAGEQUEUE *)GlobalLock16( data->ownerQueue );
        if (!queue) return FALSE;
        prevHook = &queue->hooks[data->id - WH_MINHOOK];
    }
    else prevHook = &HOOK_systemHooks[data->id - WH_MINHOOK];

    while (*prevHook && *prevHook != hook)
        prevHook = &((HOOKDATA *)USER_HEAP_LIN_ADDR(*prevHook))->next;

    if (!*prevHook) return FALSE;
    *prevHook = data->next;
    USER_HEAP_FREE( hook );
    return TRUE;
}


/***********************************************************************
 *           HOOK_FindValidHook
 */
static HANDLE16 HOOK_FindValidHook( HANDLE16 hook )
{
    HOOKDATA *data;

    for (;;)
    {
	if (!(data = (HOOKDATA *)USER_HEAP_LIN_ADDR(hook))) return 0;
	if (data->proc) return hook;
	hook = data->next;
    }
}


/***********************************************************************
 *           HOOK_CallHook
 *
 * Call a hook procedure.
 */
static LRESULT HOOK_CallHook( HANDLE16 hook, INT32 fromtype, INT32 code,
                              WPARAM32 wParam, LPARAM lParam )
{
    MESSAGEQUEUE *queue;
    HANDLE16 prevHook;
    HOOKDATA *data = (HOOKDATA *)USER_HEAP_LIN_ADDR(hook);
    LRESULT ret;

    WPARAM32 wParamOrig = wParam;
    LPARAM lParamOrig = lParam;
    HOOK_MapFunc MapFunc;
    HOOK_UnMapFunc UnMapFunc;

    MapFunc = HOOK_MapFuncs[fromtype][data->flags & HOOK_MAPTYPE];
    UnMapFunc = HOOK_UnMapFuncs[fromtype][data->flags & HOOK_MAPTYPE];

    if (MapFunc)
      MapFunc( data->id, code, &wParam, &lParam );

    /* Now call it */

    if (!(queue = (MESSAGEQUEUE *)GlobalLock16( GetTaskQueue(0) ))) return 0;
    prevHook = queue->hCurHook;
    queue->hCurHook = hook;
    data->flags |= HOOK_INUSE;

    dprintf_hook( stddeb, "Calling hook %04x: %d %08x %08lx\n",
                  hook, code, wParam, lParam );

    ret = data->proc(code, wParam, lParam);

    dprintf_hook( stddeb, "Ret hook %04x = %08lx\n", hook, ret );

    data->flags &= ~HOOK_INUSE;
    queue->hCurHook = prevHook;

    if (UnMapFunc)
      UnMapFunc( data->id, code, wParamOrig, lParamOrig, wParam, lParam );

    if (!data->proc) HOOK_RemoveHook( hook );

    return ret;
}

/***********************************************************************
 *           Exported Functions & APIs
 */

/***********************************************************************
 *           HOOK_GetProc16
 *
 * Don't call this unless you are the if1632/thunk.c.
 */
HOOKPROC16 HOOK_GetProc16( HHOOK hhook )
{
    HOOKDATA *data;
    if (HIWORD(hhook) != HOOK_MAGIC) return NULL;
    if (!(data = (HOOKDATA *)USER_HEAP_LIN_ADDR( LOWORD(hhook) ))) return NULL;
    if ((data->flags & HOOK_MAPTYPE) != HOOK_WIN16) return NULL;
    return (HOOKPROC16)data->proc;
}


/***********************************************************************
 *           HOOK_IsHooked
 *
 * Replacement for calling HOOK_GetHook from other modules.
 */
BOOL32 HOOK_IsHooked( INT16 id )
{
    return HOOK_GetHook( id, GetTaskQueue(0) ) != 0;
}


/***********************************************************************
 *           HOOK_CallHooks16
 *
 * Call a hook chain.
 */
LRESULT HOOK_CallHooks16( INT16 id, INT16 code, WPARAM16 wParam,
                          LPARAM lParam )
{
    HANDLE16 hook; 

    if (!(hook = HOOK_GetHook( id , GetTaskQueue(0) ))) return 0;
    if (!(hook = HOOK_FindValidHook(hook))) return 0;
    return HOOK_CallHook( hook, HOOK_WIN16, code, wParam, lParam );
}

/***********************************************************************
 *           HOOK_CallHooks32A
 *
 * Call a hook chain.
 */
LRESULT HOOK_CallHooks32A( INT32 id, INT32 code, WPARAM32 wParam,
                           LPARAM lParam )
{
    HANDLE16 hook; 

    if (!(hook = HOOK_GetHook( id , GetTaskQueue(0) ))) return 0;
    if (!(hook = HOOK_FindValidHook(hook))) return 0;
    return HOOK_CallHook( hook, HOOK_WIN32A, code, wParam, lParam );
}

/***********************************************************************
 *           HOOK_CallHooks32W
 *
 * Call a hook chain.
 */
LRESULT HOOK_CallHooks32W( INT32 id, INT32 code, WPARAM32 wParam,
                           LPARAM lParam )
{
    HANDLE16 hook; 

    if (!(hook = HOOK_GetHook( id , GetTaskQueue(0) ))) return 0;
    if (!(hook = HOOK_FindValidHook(hook))) return 0;
    return HOOK_CallHook( hook, HOOK_WIN32W, code, wParam,
			  lParam );
}


/***********************************************************************
 *           HOOK_ResetQueueHooks
 */
void HOOK_ResetQueueHooks( HQUEUE16 hQueue )
{
    MESSAGEQUEUE *queue;

    if ((queue = (MESSAGEQUEUE *)GlobalLock16( hQueue )) != NULL)
    {
	HOOKDATA*	data;
	HHOOK		hook;
	int		id;
	for( id = WH_MINHOOK; id <= WH_MAXHOOK; id++ )
	{
	    hook = queue->hooks[id - WH_MINHOOK];
	    while( hook )
	    {
	        if( (data = (HOOKDATA *)USER_HEAP_LIN_ADDR(hook)) )
	        {
		  data->ownerQueue = hQueue;
		  hook = data->next;
		} else break;
	    }
	}
    }
}

/***********************************************************************
 *	     HOOK_FreeModuleHooks
 */
void HOOK_FreeModuleHooks( HMODULE16 hModule )
{
 /* remove all system hooks registered by this module */

  HOOKDATA*     hptr;
  HHOOK         hook, next;
  int           id;

  for( id = WH_MINHOOK; id <= WH_MAXHOOK; id++ )
    {
       hook = HOOK_systemHooks[id - WH_MINHOOK];
       while( hook )
          if( (hptr = (HOOKDATA *)USER_HEAP_LIN_ADDR(hook)) )
	    {
	      next = hptr->next;
	      if( hptr->ownerModule == hModule )
                {
                  hptr->flags &= HOOK_MAPTYPE;
                  HOOK_RemoveHook(hook);
                }
	      hook = next;
	    }
	  else hook = 0;
    }
}

/***********************************************************************
 *	     HOOK_FreeQueueHooks
 */
void HOOK_FreeQueueHooks( HQUEUE16 hQueue )
{
  /* remove all hooks registered by this queue */

  HOOKDATA*	hptr = NULL;
  HHOOK 	hook, next;
  int 		id;

  for( id = WH_MINHOOK; id <= WH_MAXHOOK; id++ )
    {
       hook = HOOK_GetHook( id, hQueue );
       while( hook )
	{
	  next = HOOK_GetNextHook(hook);

	  hptr = (HOOKDATA *)USER_HEAP_LIN_ADDR(hook);
	  if( hptr && hptr->ownerQueue == hQueue )
	    {
	      hptr->flags &= HOOK_MAPTYPE;
	      HOOK_RemoveHook(hook);
	    }
	  hook = next;
	}
    }
}


/***********************************************************************
 *           SetWindowsHook16   (USER.121)
 */
FARPROC16 WINAPI SetWindowsHook16( INT16 id, HOOKPROC16 proc )
{
    HANDLE16 handle;
    HINSTANCE16 hInst = __winelib ? 0 : FarGetOwner( HIWORD(proc) );

    /* WH_MSGFILTER is the only task-specific hook for SetWindowsHook() */
    HTASK16 hTask = (id == WH_MSGFILTER) ? GetCurrentTask() : 0;

    if (id == WH_DEBUG)
    {
	fprintf( stdnimp, "WH_DEBUG is broken in 16-bit Windows.\n");
	return 0;
    }

    handle = HOOK_SetHook( id, proc, HOOK_WIN16, GetExePtr(hInst), hTask );
    return (handle) ? (FARPROC16)MAKELONG( handle, HOOK_MAGIC ) : NULL;
}


/***********************************************************************
 *           SetWindowsHook32A   (USER32.524)
 *
 * FIXME: I don't know if this is correct
 */
HHOOK WINAPI SetWindowsHook32A( INT32 id, HOOKPROC32 proc )
{
    HINSTANCE16 hInst = __winelib ? 0 : FarGetOwner( HIWORD(proc) );

    /* WH_MSGFILTER is the only task-specific hook for SetWindowsHook() */
    HTASK16 hTask = (id == WH_MSGFILTER) ? GetCurrentTask() : 0;

    HANDLE16 handle = HOOK_SetHook( id, proc, HOOK_WIN32A, hInst, hTask );
    return (handle) ? (HHOOK)MAKELONG( handle, HOOK_MAGIC ) : 0;
}


/***********************************************************************
 *           SetWindowsHook32W   (USER32.527)
 *
 * FIXME: I don't know if this is correct
 */
HHOOK WINAPI SetWindowsHook32W( INT32 id, HOOKPROC32 proc )
{
    HINSTANCE16 hInst = __winelib ? 0 : FarGetOwner( HIWORD(proc) );

    /* WH_MSGFILTER is the only task-specific hook for SetWindowsHook() */
    HTASK16 hTask = (id == WH_MSGFILTER) ? GetCurrentTask() : 0;

    HANDLE16 handle = HOOK_SetHook( id, proc, HOOK_WIN32W, hInst, hTask );
    return (handle) ? (HHOOK)MAKELONG( handle, HOOK_MAGIC ) : 0;
}


/***********************************************************************
 *           SetWindowsHookEx16   (USER.291)
 */
HHOOK WINAPI SetWindowsHookEx16( INT16 id, HOOKPROC16 proc, HINSTANCE16 hInst,
                                 HTASK16 hTask )
{
    HANDLE16 handle = HOOK_SetHook( id, proc, HOOK_WIN16, GetExePtr(hInst), hTask );
    return (handle) ? (HHOOK)MAKELONG( handle, HOOK_MAGIC ) : (HHOOK)NULL;
}


/***********************************************************************
 *           SetWindowsHookEx32A   (USER32.525)
 */
HHOOK WINAPI SetWindowsHookEx32A( INT32 id, HOOKPROC32 proc, HINSTANCE32 hInst,
                                  DWORD dwThreadID )
{
    HANDLE16 handle;
    HTASK16 hTask;

    if (dwThreadID == GetCurrentThreadId())
      hTask = GetCurrentTask();
    else
      hTask = LOWORD(dwThreadID);

    handle = HOOK_SetHook( id, proc, HOOK_WIN32A, hInst, hTask );
    return (handle) ? (HHOOK)MAKELONG( handle, HOOK_MAGIC ) : (HHOOK)NULL;
}


/***********************************************************************
 *           SetWindowsHookEx32W   (USER32.526)
 */
HHOOK WINAPI SetWindowsHookEx32W( INT32 id, HOOKPROC32 proc, HINSTANCE32 hInst,
                                  DWORD dwThreadID )
{
    HANDLE16 handle;
    HTASK16 hTask;

    if (dwThreadID == GetCurrentThreadId())
      hTask = GetCurrentTask();
    else
      hTask = LOWORD(dwThreadID);

    handle = HOOK_SetHook( id, proc, HOOK_WIN32W, hInst, hTask );
    return (handle) ? (HHOOK)MAKELONG( handle, HOOK_MAGIC ) : (HHOOK)NULL;
}


/***********************************************************************
 *           UnhookWindowsHook16   (USER.234)
 */
BOOL16 WINAPI UnhookWindowsHook16( INT16 id, HOOKPROC16 proc )
{
    HANDLE16 hook = HOOK_GetHook( id , GetTaskQueue(0) );

    dprintf_hook( stddeb, "UnhookWindowsHook: %d %08lx\n", id, (DWORD)proc );

    while (hook)
    {
        HOOKDATA *data = (HOOKDATA *)USER_HEAP_LIN_ADDR(hook);
        if (data->proc == (HOOKPROC32)proc) break;
        hook = HOOK_GetNextHook( hook );
    }
    if (!hook) return FALSE;
    return HOOK_RemoveHook( hook );
}


/***********************************************************************
 *           UnhookWindowsHook32   (USER32.556)
 */
BOOL32 WINAPI UnhookWindowsHook32( INT32 id, HOOKPROC32 proc )
{
    HANDLE16 hook = HOOK_GetHook( id , GetTaskQueue(0) );

    dprintf_hook( stddeb, "UnhookWindowsHook: %d %08lx\n", id, (DWORD)proc );

    while (hook)
    {
        HOOKDATA *data = (HOOKDATA *)USER_HEAP_LIN_ADDR(hook);
        if (data->proc == proc) break;
        hook = HOOK_GetNextHook( hook );
    }
    if (!hook) return FALSE;
    return HOOK_RemoveHook( hook );
}


/***********************************************************************
 *           UnhookWindowHookEx16   (USER.292)
 */
BOOL16 WINAPI UnhookWindowsHookEx16( HHOOK hhook )
{
    if (HIWORD(hhook) != HOOK_MAGIC) return FALSE;  /* Not a new format hook */
    return HOOK_RemoveHook( LOWORD(hhook) );
}


/***********************************************************************
 *           UnhookWindowHookEx32   (USER32.557)
 */
BOOL32 WINAPI UnhookWindowsHookEx32( HHOOK hhook )
{
    return UnhookWindowsHookEx16( hhook );
}


/***********************************************************************
 *           CallNextHookEx16     (USER.293)
 *
 * I wouldn't have separated this into 16 and 32 bit versions, but I
 * need a way to figure out if I need to do a mapping or not.
 */
LRESULT WINAPI CallNextHookEx16( HHOOK hhook, INT16 code, WPARAM16 wParam,
                                 LPARAM lParam )
{
    HANDLE16 next;

    if (HIWORD(hhook) != HOOK_MAGIC) return 0;  /* Not a new format hook */
    if (!(next = HOOK_GetNextHook( LOWORD(hhook) ))) return 0;

    return HOOK_CallHook( next, HOOK_WIN16, code, wParam, lParam );
}


/***********************************************************************
 *           CallNextHookEx32    (USER32.16)
 *
 * There aren't ANSI and UNICODE versions of this.
 */
LRESULT WINAPI CallNextHookEx32( HHOOK hhook, INT32 code, WPARAM32 wParam,
                                 LPARAM lParam )
{
    HANDLE16 next;
    INT32 fromtype;	/* figure out Ansi/Unicode */
    HOOKDATA *oldhook;

    if (HIWORD(hhook) != HOOK_MAGIC) return 0;  /* Not a new format hook */
    if (!(next = HOOK_GetNextHook( LOWORD(hhook) ))) return 0;

    oldhook = (HOOKDATA *)USER_HEAP_LIN_ADDR( LOWORD(hhook) );
    fromtype = oldhook->flags & HOOK_MAPTYPE;

    if (fromtype == HOOK_WIN16)
      fprintf(stderr, "CallNextHookEx32: called from 16bit hook!\n");

    return HOOK_CallHook( next, fromtype, code, wParam, lParam );
}


/***********************************************************************
 *           DefHookProc16   (USER.235)
 */
LRESULT WINAPI DefHookProc16( INT16 code, WPARAM16 wParam, LPARAM lParam,
                              HHOOK *hhook )
{
    /* Note: the *hhook parameter is never used, since we rely on the
     * current hook value from the task queue to find the next hook. */
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock16( GetTaskQueue(0) ))) return 0;
    return CallNextHookEx16( queue->hCurHook, code, wParam, lParam );
}


/***********************************************************************
 *           CallMsgFilter16   (USER.123)
 */
BOOL16 WINAPI CallMsgFilter16( SEGPTR msg, INT16 code )
{
    if (GetSysModalWindow16()) return FALSE;
    if (HOOK_CallHooks16( WH_SYSMSGFILTER, code, 0, (LPARAM)msg )) return TRUE;
    return HOOK_CallHooks16( WH_MSGFILTER, code, 0, (LPARAM)msg );
}


/***********************************************************************
 *           CallMsgFilter32A   (USER32.14)
 */
/*
 * FIXME: There are ANSI and UNICODE versions of this, plus an unspecified
 * version, plus USER (the 16bit one) has a CallMsgFilter32 function.
 */
BOOL32 WINAPI CallMsgFilter32A( LPMSG32 msg, INT32 code )
{
    if (GetSysModalWindow16()) return FALSE;	/* ??? */
    if (HOOK_CallHooks32A( WH_SYSMSGFILTER, code, 0, (LPARAM)msg ))
      return TRUE;
    return HOOK_CallHooks32A( WH_MSGFILTER, code, 0, (LPARAM)msg );
}


/***********************************************************************
 *           CallMsgFilter32W   (USER32.15)
 */
BOOL32 WINAPI CallMsgFilter32W( LPMSG32 msg, INT32 code )
{
    if (GetSysModalWindow16()) return FALSE;	/* ??? */
    if (HOOK_CallHooks32W( WH_SYSMSGFILTER, code, 0, (LPARAM)msg ))
      return TRUE;
    return HOOK_CallHooks32W( WH_MSGFILTER, code, 0, (LPARAM)msg );
}

