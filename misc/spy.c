/* SPY.C
 *
 * Copyright 1994, Bob Amstadt
 *           1995, Alex Korobka  
 */

#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <string.h>
#include "windows.h"
#include "wine.h"
#include "options.h"
#include "stddebug.h"
#include "debug.h"
#include "spy.h"

#define SPY_MAX_MSGNUM		WM_USER
#define SPY_MAX_INDENTLEVEL     64

const char *MessageTypeNames[SPY_MAX_MSGNUM + 1] =
{
    "WM_NULL",			/* 0x00 */
    "WM_CREATE",	
    "WM_DESTROY",    
    "WM_MOVE",
    "WM_SIZEWAIT",
    "WM_SIZE",
    "WM_ACTIVATE",
    "WM_SETFOCUS",
    "WM_KILLFOCUS",
    "WM_SETVISIBLE",
    "WM_ENABLE",
    "WM_SETREDRAW",
    "WM_SETTEXT",
    "WM_GETTEXT",
    "WM_GETTEXTLENGTH",
    "WM_PAINT",
    "WM_CLOSE",			/* 0x10 */
    "WM_QUERYENDSESSION",
    "WM_QUIT",
    "WM_QUERYOPEN",
    "WM_ERASEBKGND",
    "WM_SYSCOLORCHANGE",
    "WM_ENDSESSION",
    "WM_SYSTEMERROR",
    "WM_SHOWWINDOW",
    "WM_CTLCOLOR",
    "WM_WININICHANGE",
    "WM_DEVMODECHANGE",
    "WM_ACTIVATEAPP",
    "WM_FONTCHANGE",
    "WM_TIMECHANGE",
    "WM_CANCELMODE",
    "WM_SETCURSOR",		/* 0x20 */
    "WM_MOUSEACTIVATE",
    "WM_CHILDACTIVATE",
    "WM_QUEUESYNC",
    "WM_GETMINMAXINFO",
    "WM_UNUSED3",
    "WM_PAINTICON",
    "WM_ICONERASEBKGND",
    "WM_NEXTDLGCTL",
    "WM_ALTTABACTIVE",
    "WM_SPOOLERSTATUS",
    "WM_DRAWITEM",
    "WM_MEASUREITEM",
    "WM_DELETEITEM",
    "WM_VKEYTOITEM",
    "WM_CHARTOITEM",
    "WM_SETFONT",		/* 0x30 */
    "WM_GETFONT",
    "WM_SETHOTKEY", 
    "WM_GETHOTKEY", 
    "WM_FILESYSCHANGE", 
    "WM_ISACTIVEICON",
    "WM_QUERYPARKICON",
    "WM_QUERYDRAGICON",
    "WM_QUERYSAVESTATE",
    "WM_COMPAREITEM", 
    "WM_TESTING", NULL, 
    "WM_OTHERWINDOWCREATED", 
    "WM_OTHERWINDOWDESTROYED", 
    "WM_ACTIVATESHELLWINDOW", NULL,

    NULL, 		        /* 0x40 */
    "WM_COMPACTING", NULL, NULL, 
    "WM_COMMNOTIFY", NULL, 
    "WM_WINDOWPOSCHANGING",	/* 0x0046 */
    "WM_WINDOWPOSCHANGED",	/* 0x0047 */
    "WM_POWER", NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    NULL, 		        /* 0x0050 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0060 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0070 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    NULL,			/* 0x0080 */
    "WM_NCCREATE",		/* 0x0081 */
    "WM_NCDESTROY",		/* 0x0082 */
    "WM_NCCALCSIZE",		/* 0x0083 */
    "WM_NCHITTEST",        	/* 0x0084 */
    "WM_NCPAINT",          	/* 0x0085 */
    "WM_NCACTIVATE",       	/* 0x0086 */
    "WM_GETDLGCODE",		/* 0x0087 */
    "WM_SYNCPAINT", 
    "WM_SYNCTASK", NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0090 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x00A0 */
    "WM_NCMOUSEMOVE",		/* 0x00A0 */
    "WM_NCLBUTTONDOWN",		/* 0x00A1 */
    "WM_NCLBUTTONUP",		/* 0x00A2 */
    "WM_NCLBUTTONDBLCLK",	/* 0x00A3 */
    "WM_NCRBUTTONDOWN",		/* 0x00A4 */
    "WM_NCRBUTTONUP",		/* 0x00A5 */
    "WM_NCRBUTTONDBLCLK",	/* 0x00A6 */
    "WM_NCMBUTTONDOWN",		/* 0x00A7 */
    "WM_NCMBUTTONUP",		/* 0x00A8 */
    "WM_NCMBUTTONDBLCLK",	/* 0x00A9 */
    NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x00B0 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x00C0 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x00D0 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x00E0 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x00F0 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    "WM_KEYDOWN",		/* 0x0100 */
    "WM_KEYUP",			/* 0x0101 */
    "WM_CHAR",			/* 0x0102 */
    "WM_DEADCHAR",		/* 0x0103 */
    "WM_SYSKEYDOWN",		/* 0x0104 */
    "WM_SYSKEYUP",		/* 0x0105 */
    "WM_SYSCHAR",		/* 0x0106 */
    "WM_SYSDEADCHAR",		/* 0x0107 */
    "WM_KEYLAST",		/* 0x0108 */
    NULL, 
    "WM_CONVERTREQUEST",
    "WM_CONVERTRESULT", 
    "WM_INTERIM", NULL, NULL, NULL,

    "WM_INITDIALOG",		/* 0x0110 */
    "WM_COMMAND",		/* 0x0111 */
    "WM_SYSCOMMAND",       	/* 0x0112 */
    "WM_TIMER",			/* 0x0113 */
    "WM_HSCROLL",		/* 0x0114 */
    "WM_VSCROLL",		/* 0x0115 */
    "WM_INITMENU",              /* 0x0116 */
    "WM_INITMENUPOPUP",         /* 0x0117 */
    "WM_SYSTIMER",		/* 0x0118 */
    NULL, NULL, NULL, NULL, NULL, NULL,
    "WM_MENUSELECT",            /* 0x011f */

    "WM_MENUCHAR",              /* 0x0120 */
    "WM_ENTERIDLE",             /* 0x0121 */
    NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0130 */
    NULL, "wm_lbtrackpoint", 
    NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0140 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0150 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0160 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0170 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0180 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0190 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x01A0 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x01B0 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x01C0 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x01D0 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x01E0 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x01F0 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    "WM_MOUSEMOVE",		/* 0x0200 */
    "WM_LBUTTONDOWN",		/* 0x0201 */
    "WM_LBUTTONUP",		/* 0x0202 */
    "WM_LBUTTONDBLCLK",		/* 0x0203 */
    "WM_RBUTTONDOWN",		/* 0x0204 */
    "WM_RBUTTONUP",		/* 0x0205 */
    "WM_RBUTTONDBLCLK",		/* 0x0206 */
    "WM_MBUTTONDOWN",		/* 0x0207 */
    "WM_MBUTTONUP",		/* 0x0208 */
    "WM_MBUTTONDBLCLK",		/* 0x0209 */
    NULL, NULL, NULL, NULL, NULL, NULL,

    "WM_PARENTNOTIFY",		/* 0x0210 */
    "WM_ENTERMENULOOP",         /* 0x0211 */
    "WM_EXITMENULOOP",          /* 0x0212 */
    "WM_NEXTMENU", 		/* 0x0213 */
                            NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    "WM_MDICREATE",             /* 0x0220 */
    "WM_MDIDESTROY",            /* 0x0221 */
    "WM_MDIACTIVATE",           /* 0x0222 */
    "WM_MDIRESTORE",            /* 0x0223 */
    "WM_MDINEXT",               /* 0x0224 */
    "WM_MDIMAXIMIZE",           /* 0x0225 */
    "WM_MDITILE",               /* 0x0226 */
    "WM_MDICASCADE",            /* 0x0227 */
    "WM_MDIICONARRANGE",        /* 0x0228 */
    "WM_MDIGETACTIVE",          /* 0x0229 */

    "wm_dropobject", 
    "wm_querydropobject", 
    "wm_begindrag",
    "wm_dragloop",
    "wn_dragselect",
    "wm_dragmove",
     
    /* 0x0230*/
    "WM_MDISETMENU",            /* 0x0230 */
    "WM_ENTERSIZEMOVE",		/* 0x0231 */
    "WM_EXITSIZEMOVE",		/* 0x0232 */
    "WM_DROPFILES", 		/* 0x0233 */
    NULL, NULL, NULL, NULL, 
    /* 0x0238*/
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    
    /* 0x0240 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0250 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    
    /* 0x0260 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0280 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x02c0 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    "WM_CUT", 			/* 0x0300 */
    "WM_COPY", 
    "WM_PASTE", 
    "WM_CLEAR", 
    "WM_UNDO", 
    "WM_RENDERFORMAT", 
    "WM_RENDERALLFORMATS", 
    "WM_DESTROYCLIPBOARD",
    "WM_DRAWCLIPBOARD", 
    "WM_PAINTCLIPBOARD", 
    "WM_VSCROLLCLIPBOARD", 
    "WM_SIZECLIPBOARD", 
    "WM_ASKCBFORMATNAME", 
    "WM_CHANGECBCHAIN",
    "WM_HSCROLLCLIPBOARD",
    "WM_QUERYNEWPALETTE",	/* 0x030f*/

    "WM_PALETTEISCHANGING",
    "WM_PALETTECHANGED", 	/* 0x0311 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, 

    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0340 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0380 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    "WM_COALESCE_FIRST", 
          NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    "WM_COALESCE_LAST", 

    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x03c0 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x03e0 */
    "WM_DDE_INITIATE",  /* 0x3E0 */
    "WM_DDE_TERMINATE",	/* 0x3E1 */
    "WM_DDE_ADVISE",	/* 0x3E2 */
    "WM_DDE_UNADVISE",	/* 0x3E3 */
    "WM_DDE_ACK",	/* 0x3E4 */
    "WM_DDE_DATA",	/* 0x3E5 */
    "WM_DDE_REQUEST",	/* 0x3E6 */
    "WM_DDE_POKE",	/* 0x3E7 */
    "WM_DDE_EXECUTE",	/* 0x3E8 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    
    /* 0x03f0 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    "WM_USER"
};

static BOOL  	SpyFilters [SPY_MAX_MSGNUM+1];
static BOOL 	SpyIncludes[SPY_MAX_MSGNUM+1];

static int      iSpyMessageIndentLevel  = 0;
static char     lpstrSpyMessageIndent[SPY_MAX_INDENTLEVEL];
static char    *lpstrSpyMessageFromWine = "Wine";
static char     lpstrSpyMessageFromTask[10]; 
static char    *lpstrSpyMessageFromSelf = "self";
static char    *lpstrSpyMessageFrom     =  NULL;


/**********************************************************************
 *			      EnterSpyMessage
 */
void EnterSpyMessage(int iFlag, HWND hWnd, WORD msg, WORD wParam, LONG lParam)
{
  HTASK	hTask     = GetWindowTask(hWnd);
  WORD 	wCheckMsg = (msg > WM_USER)? WM_USER: msg;

  if( !SpyIncludes[wCheckMsg] || SpyFilters[wCheckMsg]) return;

  /* each SPY_SENDMESSAGE must be complemented by call to ExitSpyMessage */
  switch(iFlag)
   {
	case SPY_DISPATCHMESSAGE:
		    if(msg <= WM_USER)
		      {
		       if(MessageTypeNames[msg])
		          dprintf_message(stddeb,"("NPFMT") message [%04x] %s dispatched  wp=%04x lp=%08lx\n",
		                          hWnd, msg, MessageTypeNames[msg], wParam, lParam);
		       else
		          dprintf_message(stddeb,"("NPFMT") message [%04x] dispatched  wp=%04x lp=%08lx\n",
		                          hWnd, msg, wParam, lParam);
		      }
		    else
		          dprintf_message(stddeb,"("NPFMT") message [%04x] WM_USER+%04d dispatched  wp=%04x lp=%08lx\n",
		                          hWnd, msg, msg-WM_USER ,wParam ,lParam);
		    break;
	case SPY_SENDMESSAGE:
		    if(hTask == GetCurrentTask())
 		      lpstrSpyMessageFrom = lpstrSpyMessageFromSelf;
		    else if(hTask == NULL)
  		      	   lpstrSpyMessageFrom = lpstrSpyMessageFromWine;
  		    	 else
  				{
  				   sprintf(lpstrSpyMessageFromTask, "task "NPFMT, hTask);	
  				   lpstrSpyMessageFrom = lpstrSpyMessageFromTask;
  				}
		     
	            if(msg <= WM_USER)
	                {
	                  if(MessageTypeNames[msg])
			      dprintf_message(stddeb,"%s("NPFMT") message [%04x] %s sent from %s wp=%04x lp=%08lx\n",
                                   	      lpstrSpyMessageIndent,
                                   	      hWnd, msg, MessageTypeNames[msg],
                                   	      lpstrSpyMessageFrom,
                                   	      wParam, lParam);
	                  else
	                      dprintf_message(stddeb,"%s("NPFMT") message [%04x] sent from %s wp=%04x lp=%08lx\n",
                                   	      lpstrSpyMessageIndent,
                                   	      hWnd, msg,
                                   	      lpstrSpyMessageFrom,
                                   	      wParam, lParam);
          		}
        	    else
             		  dprintf_message(stddeb,"%s("NPFMT") message [%04x] WM_USER+%04x sent from %s wp=%04x lp=%08lx\n",
                               		  lpstrSpyMessageIndent,
                               		  hWnd, msg, msg-WM_USER,
                               		  lpstrSpyMessageFrom,
                             		  wParam, lParam);

	            if(SPY_MAX_INDENTLEVEL > iSpyMessageIndentLevel ) 
		    	{
			  iSpyMessageIndentLevel++;
			  lpstrSpyMessageIndent[iSpyMessageIndentLevel]='\0';
			  lpstrSpyMessageIndent[iSpyMessageIndentLevel-1]  ='\t';
		      	}
		    break;   
	case SPY_DEFWNDPROC:
		    if(msg <= WM_USER)
		        if(MessageTypeNames[msg])
		           dprintf_message(stddeb, "%s("NPFMT") DefWindowProc: %s [%04x]  wp=%04x lp=%08lx\n",
		                           lpstrSpyMessageIndent,
		                           hWnd, MessageTypeNames[msg], msg, wParam, lParam );
		        else
		           dprintf_message(stddeb, "%s("NPFMT") DefWindowProc: [%04x]  wp=%04x lp=%08lx\n",
		                           lpstrSpyMessageIndent,
		                           hWnd, msg, wParam, lParam );
		    else
		        dprintf_message(stddeb, "%s("NPFMT") DefWindowProc: WM_USER+%d [%04x] wp=%04x lp=%08lx\n",
		                        lpstrSpyMessageIndent,
		                        hWnd, msg - WM_USER, msg, wParam, lParam );
		    break;
	default:		    
   }  

}

/**********************************************************************
 *			      ExitSpyMessage
 */
void ExitSpyMessage(int iFlag, HWND hWnd, WORD msg, LONG lReturn)
{
  WORD wCheckMsg = (msg > WM_USER)? WM_USER: msg;

  if( !SpyIncludes[wCheckMsg] || SpyFilters[wCheckMsg]) return;

  iSpyMessageIndentLevel--;
  lpstrSpyMessageIndent[iSpyMessageIndentLevel]='\0';

  switch(iFlag)
    {
	case SPY_RESULT_INVALIDHWND: 
		dprintf_message(stddeb,"%s("NPFMT") message [%04x] HAS INVALID HWND\n",
                                lpstrSpyMessageIndent, hWnd, msg);
	        break;
	case SPY_RESULT_OK:
		dprintf_message(stddeb,"%s("NPFMT") message [%04x] returned %08lx\n",
	                        lpstrSpyMessageIndent, hWnd, msg, lReturn);
		break;
	default:
    }
}

/**********************************************************************
 *					SpyInit
 */
void SpyInit(void)
{
    int      i;
    char     lpstrBuffer[512];

    for(i=0; i <= SPY_MAX_MSGNUM; i++) SpyFilters[i] = SpyIncludes[i] = FALSE;

    GetPrivateProfileString("spy", "Exclude", "",lpstrBuffer ,511 , WINE_INI);
    dprintf_message(stddeb,"SpyInit: Exclude=%s\n",lpstrBuffer);
    if( *lpstrBuffer != 0 )
      if(strstr(lpstrBuffer,"EXCLUDEALL"))
	for(i=0; i <= SPY_MAX_MSGNUM; i++) SpyFilters[i] = TRUE;
      else
        for(i=0; i <= SPY_MAX_MSGNUM; i++)
	    if(MessageTypeNames[i])
	       if(strstr(lpstrBuffer,MessageTypeNames[i])) SpyFilters[i] = TRUE; 

    GetPrivateProfileString("spy", "Include", "INCLUDEALL",lpstrBuffer ,511 , WINE_INI);
    dprintf_message(stddeb,"SpyInit: Include=%s\n",lpstrBuffer);
    if( *lpstrBuffer != 0 )
      if(strstr(lpstrBuffer,"INCLUDEALL"))
        for(i=0; i <= SPY_MAX_MSGNUM; i++) SpyIncludes[i] = TRUE;
      else 
        for(i=0; i <= SPY_MAX_MSGNUM; i++)
            if(MessageTypeNames[i])
               if(strstr(lpstrBuffer,MessageTypeNames[i])) SpyIncludes[i] = TRUE;

}

