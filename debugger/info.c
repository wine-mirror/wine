/*
 * Wine debugger utility routines
 *
 * Copyright 1993 Eric Youngdale
 * Copyright 1995 Alexandre Julliard
 */

#include <stdio.h>
#include <stdlib.h>
#include "global.h"
#include "user.h"
#include "class.h"
#include "win.h"
#include "message.h"
#include "spy.h"
#include "debugger.h"

int	  	iWndIndent = 0;

extern 	char	lpstrSpyMessageIndent[]; /* from misc/spy.c */

/***********************************************************************
 *	    DEBUG_InitWalk
 */
void DEBUG_InitWalk(void)
{
  fprintf(stderr,"%-24.24s %-6.6s %-17.17s %-8.8s %s\n",
                 "HWND / WNDPTR","hQueue","Class Name", "Style", "WndProc");
  lpstrSpyMessageIndent[0]='\0';
}

/***********************************************************************
 *           DEBUG_WndWalk
 *
 */
void DEBUG_WndWalk(HWND hStart)
{
  WND*  	wndPtr;
  CLASS*	classPtr;
  char	 	className[0x10];
  int		i;

  if( !hStart )
       hStart = GetDesktopHwnd();

  wndPtr = WIN_FindWndPtr(hStart);

  if( !wndPtr )
    { fprintf(stderr, "Invalid window handle: %04x\n", hStart);
      return; }

  i = strlen(lpstrSpyMessageIndent);
 
  /* 0x10 bytes are always reserved at the end of lpstrSpyMessageIndent */
  sprintf(lpstrSpyMessageIndent + i,"%04x %08x",hStart, (unsigned) wndPtr);

  classPtr = CLASS_FindClassPtr(wndPtr->hClass);
  if(classPtr)
     GlobalGetAtomName(classPtr->atomName ,className, 0x10);
  else
     strcpy(className,"<BAD>");

  fprintf(stderr,"%-24.24s %-6.4x %-17.17s %08x %04x:%04x\n",
		 lpstrSpyMessageIndent,
		 wndPtr->hmemTaskQ, 
		 className,
		 (unsigned) wndPtr->dwStyle,
		 HIWORD(wndPtr->lpfnWndProc),
		 LOWORD(wndPtr->lpfnWndProc));

  lpstrSpyMessageIndent[i] = '\0';

  if( wndPtr->hwndChild )  
    {
      /* walk children */

      hStart = wndPtr->hwndChild;
      wndPtr = WIN_FindWndPtr(hStart);

      iWndIndent ++;
      if( iWndIndent < SPY_MAX_INDENTLEVEL - 0x10 )
        { 
          lpstrSpyMessageIndent[iWndIndent - 1] = ' ';
	  lpstrSpyMessageIndent[iWndIndent] = '\0';
        }
 
      while( wndPtr )
  	  { 
             DEBUG_WndWalk(hStart);
	     hStart = wndPtr->hwndNext; 
	     wndPtr = WIN_FindWndPtr(hStart);
  	  }

      if( hStart )
          fprintf(stderr, "%s%s"NPFMT"\n", lpstrSpyMessageIndent,
					  "<BAD>", hStart);

      if( iWndIndent )
        {
          iWndIndent--;
          if( iWndIndent < SPY_MAX_INDENTLEVEL - 0x10 )
              lpstrSpyMessageIndent[iWndIndent] = '\0';
        }

    }
}

/***********************************************************************
 *	     DEBUG_WndDump
 *
 */ 
void DEBUG_WndDump(HWND hWnd)
{
  WND*		wnd;
  char*		lpWndText = NULL;

  wnd = WIN_FindWndPtr(hWnd);

  if( !wnd )
    { fprintf(stderr, "Invalid window handle: %04x\n", hWnd);
      return; }
 
  if( wnd->hText ) 
      lpWndText = (LPSTR) USER_HEAP_LIN_ADDR( wnd->hText );

  fprintf( stderr, "next: %12.4x\n"
                   "child:  %10.4x\n"
                   "parent: %10.4x\n"
                   "owner:  %10.4x\n"
                   "hClass: %10.4x\n"
                   "hInst:  %10.4x\n"
                   "clientRect: %i,%i - %i,%i\n"
                   "windowRect: %i,%i - %i,%i\n"
                   "hRgnUpdate: %6.4x\n"
                   "hLastPopup: %6.4x\n"
                   "Style:  %10.8x\n"
                   "StyleEx: %9.8x\n"
                   "hDCE:   %10.4x\n"
                   "hVscroll: %8.4x\n"
                   "hHscroll: %8.4x\n"
                   "menuID: %10.4x\n"
                   "hText:  %10.4x (\"%s\")\n"
                   "flags:  %10.4x\n",
                   wnd->hwndNext, wnd->hwndChild,wnd->hwndParent,
                   wnd->hwndOwner,wnd->hClass,wnd->hInstance,
                   wnd->rectClient.left, wnd->rectClient.top, 
                   wnd->rectClient.right, wnd->rectClient.bottom,
                   wnd->rectWindow.left, wnd->rectWindow.top,
                   wnd->rectWindow.right, wnd->rectWindow.bottom,
                   wnd->hrgnUpdate, wnd->hwndLastActive,
                   (unsigned) wnd->dwStyle, (unsigned) wnd->dwExStyle,
                   wnd->hdce, wnd->hVScroll, wnd->hHScroll,
                   wnd->wIDmenu, wnd->hText, (lpWndText)?lpWndText:"NULL",
                   wnd->flags);
}

/***********************************************************************
 *	     DEBUG_QueueDump
 *
 */
void DEBUG_QueueDump(HQUEUE hQ)
{
 MESSAGEQUEUE*	pq; 

 if( !hQ || IsBadReadPtr((SEGPTR)MAKELONG( 0, GlobalHandleToSel(hQ)),
		    				sizeof(MESSAGEQUEUE)) )
   {
     fprintf(stderr, "Invalid queue handle: "NPFMT"\n", hQ);
     return;
   }

 pq = (MESSAGEQUEUE*) GlobalLock( hQ );

 fprintf(stderr,"next: %12.4x  Intertask SendMessage:\n"
                "hTask: %11.4x  ----------------------\n"
                "msgSize: %9.4x  hWnd: %10.4x\n"
                "msgCount: %8.4x  msg: %11.4x\n"
                "msgNext: %9.4x  wParam: %8.4x\n"
                "msgFree: %9.4x  lParam: %8.8x\n"
                "qSize: %11.4x  lRet: %10.8x\n"
                "wWinVer: %9.4x  ISMH: %10.4x\n"
                "paints: %10.4x  hSendTask: %5.4x\n"
                "timers: %10.4x  hPrevSend: %5.4x\n"
                "wakeBits: %8.4x\n"
                "wakeMask: %8.4x\n"
                "hCurHook: %8.4x\n",
                pq->next, pq->hTask, pq->msgSize, pq->hWnd, 
		pq->msgCount, pq->msg, pq->nextMessage, pq->wParam,
		pq->nextFreeMessage, (unsigned)pq->lParam, pq->queueSize,
		(unsigned)pq->SendMessageReturn, pq->wWinVersion, pq->InSendMessageHandle,
		pq->wPaintCount, pq->hSendingTask, pq->wTimerCount,
		pq->hPrevSendingTask, pq->status, pq->wakeMask, pq->hCurHook);
}

/***********************************************************************
 *           DEBUG_Print
 *
 * Implementation of the 'print' command.
 */
void DEBUG_Print( const DBG_ADDR *addr, int count, char format )
{
    if (count != 1)
    {
        fprintf( stderr, "Count other than 1 is meaningless in 'print' command\n" );
        return;
    }

    if (addr->seg && (addr->seg != 0xffffffff))
    {
        switch(format)
        {
	case 'x':
            fprintf( stderr, "0x%04lx:", addr->seg );
            break;

	case 'd':
            fprintf( stderr, "%ld:", addr->seg );
            break;

	case 'c':
            break;  /* No segment to print */

	case 'i':
	case 's':
	case 'w':
	case 'b':
            break;  /* Meaningless format */
        }
    }

    switch(format)
    {
    case 'x':
        if (addr->seg) fprintf( stderr, "0x%04lx\n", addr->off );
        else fprintf( stderr, "0x%08lx\n", addr->off );
        break;

    case 'd':
        fprintf( stderr, "%ld\n", addr->off );
        break;

    case 'c':
        fprintf( stderr, "%d = '%c'\n",
                 (char)(addr->off & 0xff), (char)(addr->off & 0xff) );
        break;

    case 'i':
    case 's':
    case 'w':
    case 'b':
        fprintf( stderr, "Format specifier '%c' is meaningless in 'print' command\n", format );
        break;
    }
}


/***********************************************************************
 *           DEBUG_PrintAddress
 *
 * Print an 16- or 32-bit address, with the nearest symbol if any.
 */
void DEBUG_PrintAddress( const DBG_ADDR *addr, int addrlen )
{
    const char *name = DEBUG_FindNearestSymbol( addr );

    if (addr->seg) fprintf( stderr, "0x%04lx:", addr->seg );
    if (addrlen == 16) fprintf( stderr, "0x%04lx", addr->off );
    else fprintf( stderr, "0x%08lx", addr->off );
    if (name) fprintf( stderr, " (%s)", name );
}


/***********************************************************************
 *           DEBUG_Help
 *
 * Implementation of the 'help' command.
 */
void DEBUG_Help(void)
{
    int i = 0;
    static const char * helptext[] =
{
"The commands accepted by the Wine debugger are a small subset",
"of the commands that gdb would accept.  The commands currently",
"are:\n",
"  break [*<addr>]                      delete break bpnum",
"  disable bpnum                        enable bpnum",
"  help                                 quit",
"  x <addr>                             cont",
"  step                                 next",
"  mode [16,32]                         print <expr>",
"  set <reg> = <expr>                   set *<addr> = <expr>",
"  walk [wnd] <expr>			 dump [wnd, queue] <expr>",
"  info [reg,stack,break,segments]      bt",
"  symbolfile <filename>                define <identifier> <addr>",
"  list <addr> ",
"",
"The 'x' command accepts repeat counts and formats (including 'i') in the",
"same way that gdb does.",
"",
" The following are examples of legal expressions:",
" $eax     $eax+0x3   0x1000   ($eip + 256)  *$eax   *($esp + 3)",
" Also, a nm format symbol table can be read from a file using the",
" symbolfile command.  Symbols can also be defined individually with",
" the define command.",
"",
NULL
};

    while(helptext[i]) fprintf(stderr,"%s\n", helptext[i++]);
}



/***********************************************************************
 *           DEBUG_List
 *
 * Implementation of the 'list' command.
 */
void DEBUG_List( DBG_ADDR *addr, int count )
{
    static DBG_ADDR lasttime = { 0xffffffff, 0 };

    if (addr == NULL) addr = &lasttime;
    DBG_FIX_ADDR_SEG( addr, CS_reg(DEBUG_context) );
    while (count-- > 0)
    {
        DEBUG_PrintAddress( addr, dbg_mode );
        fprintf( stderr, ":  " );
        if (!DBG_CHECK_READ_PTR( addr, 1 )) return;
        DEBUG_Disasm( addr );
        fprintf (stderr, "\n");
    }
    lasttime = *addr;
}
