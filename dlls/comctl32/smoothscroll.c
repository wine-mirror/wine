/*
 * Undocumented SmoothScrollWindow function from COMCTL32.DLL
 *
 * Copyright 2000 Marcus Meissner <marcus@jet.franken.de>
 *
 * TODO
 *     - actually add smooth scrolling
 */

#include "winbase.h"
#include "winreg.h"
#include "winerror.h"
#include "commctrl.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(commctrl);

static DWORD	smoothscroll = 2;

typedef BOOL (CALLBACK *SCROLLWINDOWEXPROC)(HWND,INT,INT,LPRECT,LPRECT,HRGN,LPRECT,DWORD);
typedef struct tagSMOOTHSCROLLSTRUCT {
	DWORD			dwSize;
	DWORD			x2;
	HWND			hwnd;
	DWORD			dx;

	DWORD			dy;
	LPRECT			lpscrollrect;
	LPRECT			lpcliprect;
	HRGN			hrgnupdate;

	LPRECT			lpupdaterect;
	DWORD			flags;
	DWORD			stepinterval;
	DWORD			dx_step;

	DWORD			dy_step;
	SCROLLWINDOWEXPROC	scrollfun;	/* same parameters as ScrollWindowEx */
} SMOOTHSCROLLSTRUCT;

/**************************************************************************
 * SmoothScrollWindow [COMCTL32.382]
 *
 * Lots of magic for smooth scrolling windows.
 *
 * Currently only scrolls ONCE. The comctl32 implementation uses GetTickCount
 * and what else to do smooth scrolling.
 */

BOOL WINAPI SmoothScrollWindow( SMOOTHSCROLLSTRUCT *smooth ) {
   LPRECT	lpupdaterect = smooth->lpupdaterect;
   HRGN		hrgnupdate = smooth->hrgnupdate;
   RECT		tmprect;
   BOOL		ret = TRUE;
   DWORD	flags = smooth->flags;

   if (smooth->dwSize!=sizeof(SMOOTHSCROLLSTRUCT))
       return FALSE;

   if (!lpupdaterect)
       lpupdaterect = &tmprect;
   SetRectEmpty(lpupdaterect);

   if (!(flags & 0x40000)) { /* no override, use system wide defaults */
       if (smoothscroll == 2) {
	   HKEY	hkey;

	   smoothscroll = 0;
	   if (!RegOpenKeyA(HKEY_CURRENT_USER,"Control Panel\\Desktop",&hkey)) {
	       DWORD	len = 4;

	       RegQueryValueExA(hkey,"SmoothScroll",0,0,(LPBYTE)&smoothscroll,&len);
	       RegCloseKey(hkey);
	   }
       }
       if (!smoothscroll)
	   flags |= 0x20000;
   }

   if (flags & 0x20000) { /* are we doing jump scrolling? */
       if ((smooth->x2 & 1) && smooth->scrollfun)
	   return smooth->scrollfun(
	       smooth->hwnd,smooth->dx,smooth->dy,smooth->lpscrollrect,
	       smooth->lpcliprect,hrgnupdate,lpupdaterect,
	       flags & 0xffff
	   );
       else
	   return ScrollWindowEx(
	       smooth->hwnd,smooth->dx,smooth->dy,smooth->lpscrollrect,
	       smooth->lpcliprect,hrgnupdate,lpupdaterect,
	       flags & 0xffff
	   );
   }

   FIXME("(hwnd=%x,flags=%lx,x2=%lx): should smooth scroll here.\n",
	   smooth->hwnd,flags,smooth->x2
   );
   /* FIXME: do timer based smooth scrolling */
   if ((smooth->x2 & 1) && smooth->scrollfun)
       return smooth->scrollfun(
	   smooth->hwnd,smooth->dx,smooth->dy,smooth->lpscrollrect,
	   smooth->lpcliprect,hrgnupdate,lpupdaterect,
	   flags & 0xffff
       );
   else
       return ScrollWindowEx(
	   smooth->hwnd,smooth->dx,smooth->dy,smooth->lpscrollrect,
	   smooth->lpcliprect,hrgnupdate,lpupdaterect,
	   flags & 0xffff
       );
   return ret;
}
