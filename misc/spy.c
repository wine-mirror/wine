/*
 * Message spying routines
 *
 * Copyright 1994, Bob Amstadt
 *           1995, Alex Korobka  
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "windows.h"
#include "module.h"
#include "options.h"
#include "stddebug.h"
#include "debug.h"
#include "spy.h"

#define SPY_MAX_MSGNUM   WM_USER
#define SPY_INDENT_UNIT  4  /* 4 spaces */

static const char * const MessageTypeNames[SPY_MAX_MSGNUM + 1] =
{
    "wm_null",			/* 0x00 */
    "WM_CREATE",	
    "WM_DESTROY",    
    "WM_MOVE",
    "wm_sizewait",
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
    "wm_systemerror",
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
    "wm_unused3",
    "wm_painticon",
    "WM_ICONERASEBKGND",
    "WM_NEXTDLGCTL",
    "wm_alttabactive",
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
    "wm_filesyschange", 
    "wm_isactiveicon",
    "wm_queryparkicon",
    "WM_QUERYDRAGICON",
    "wm_querysavestate",
    "WM_COMPAREITEM", 
    "wm_testing",
    NULL, 
    "wm_otherwindowcreated", 
    "wm_otherwindowdestroyed", 
    "wm_activateshellwindow",
    NULL,

    NULL, 		        /* 0x40 */
    "wm_compacting", NULL, NULL, 
    "WM_COMMNOTIFY", NULL, 
    "WM_WINDOWPOSCHANGING",	/* 0x0046 */
    "WM_WINDOWPOSCHANGED",	/* 0x0047 */
    "WM_POWER", NULL, 
    "WM_COPYDATA", 
    "WM_CANCELJOURNAL", NULL, NULL, NULL, NULL,

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
    "wm_syncpaint", 
    "wm_synctask", NULL, NULL, NULL, NULL, NULL, NULL,

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

    /* 0x00B0 - Win32 Edit controls */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x00C0 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x00D0 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x00E0 - Win32 Scrollbars */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x00F0 - Win32 Buttons */
    "BM_GETCHECK32",            /* 0x00f0 */
    "BM_SETCHECK32",            /* 0x00f1 */
    "BM_GETSTATE32",            /* 0x00f2 */
    "BM_SETSTATE32",            /* 0x00f3 */
    "BM_SETSTYLE32",            /* 0x00f4 */
    "BM_CLICK32",               /* 0x00f5 */
    "BM_GETIMAGE32",            /* 0x00f6 */
    "BM_SETIMAGE32",            /* 0x00f7 */
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
    NULL,
    "wm_lbtrackpoint",          /* 0x0131 */
    "WM_CTLCOLORMSGBOX",        /* 0x0132 */
    "WM_CTLCOLOREDIT",          /* 0x0133 */
    "WM_CTLCOLORLISTBOX",       /* 0x0134 */
    "WM_CTLCOLORBTN",           /* 0x0135 */
    "WM_CTLCOLORDLG",           /* 0x0136 */
    "WM_CTLCOLORSCROLLBAR",     /* 0x0137 */
    "WM_CTLCOLORSTATIC",        /* 0x0138 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0140 - Win32 Comboboxes */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0150 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0160 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0170 - Win32 Static controls */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0180 - Win32 Listboxes */
    "LB_ADDSTRING32",           /* 0x0180 */
    "LB_INSERTSTRING32",        /* 0x0181 */
    "LB_DELETESTRING32",        /* 0x0182 */
    "LB_SELITEMRANGEEX32",      /* 0x0183 */
    "LB_RESETCONTENT32",        /* 0x0184 */
    "LB_SETSEL32",              /* 0x0185 */
    "LB_SETCURSEL32",           /* 0x0186 */
    "LB_GETSEL32",              /* 0x0187 */
    "LB_GETCURSEL32",           /* 0x0188 */
    "LB_GETTEXT32",             /* 0x0189 */
    "LB_GETTEXTLEN32",          /* 0x018a */
    "LB_GETCOUNT32",            /* 0x018b */
    "LB_SELECTSTRING32",        /* 0x018c */
    "LB_DIR32",                 /* 0x018d */
    "LB_GETTOPINDEX32",         /* 0x018e */
    "LB_FINDSTRING32",          /* 0x018f */

    "LB_GETSELCOUNT32",         /* 0x0190 */
    "LB_GETSELITEMS32",         /* 0x0191 */
    "LB_SETTABSTOPS32",         /* 0x0192 */
    "LB_GETHORIZONTALEXTENT32", /* 0x0193 */
    "LB_SETHORIZONTALEXTENT32", /* 0x0194 */
    "LB_SETCOLUMNWIDTH32",      /* 0x0195 */
    "LB_ADDFILE32",             /* 0x0196 */
    "LB_SETTOPINDEX32",         /* 0x0197 */
    "LB_GETITEMRECT32",         /* 0x0198 */
    "LB_GETITEMDATA32",         /* 0x0199 */
    "LB_SETITEMDATA32",         /* 0x019a */
    "LB_SELITEMRANGE32",        /* 0x019b */
    "LB_SETANCHORINDEX32",      /* 0x019c */
    "LB_GETANCHORINDEX32",      /* 0x019d */
    "LB_SETCARETINDEX32",       /* 0x019e */
    "LB_GETCARETINDEX32",       /* 0x019f */

    "LB_SETITEMHEIGHT32",       /* 0x01a0 */
    "LB_GETITEMHEIGHT32",       /* 0x01a1 */
    "LB_FINDSTRINGEXACT32",     /* 0x01a2 */
    NULL, NULL,
    "LB_SETLOCALE32",           /* 0x01a5 */
    "LB_GETLOCALE32",           /* 0x01a6 */
    "LB_SETCOUNT32",            /* 0x01a7 */
    "LB_INITSTORAGE32",         /* 0x01a8 */
    "LB_ITEMFROMPOINT32",       /* 0x01a9 */
    NULL, NULL, NULL, NULL, NULL, NULL,

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
    "wm_nextmenu", 		/* 0x0213 */
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
    "WM_MDIREFRESHMENU", NULL, NULL, NULL, 
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
    "WM_PALETTECHANGED",
    "WM_HOTKEY", 		/* 0x0312 */
	  NULL, NULL, NULL, NULL, NULL, NULL, 
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
    "WM_PENWINFIRST", 
    "WM_RCRESULT", 
    "WM_HOOKRCRESULT", 
    "WM_GLOBALRCCHANGE", 
    "WM_SKB", 
    "WM_HEDITCTL", 
					NULL, NULL,
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


static BOOL SPY_Exclude[SPY_MAX_MSGNUM+1];
static int SPY_IndentLevel  = 0;

#define SPY_EXCLUDE(msg) \
    (SPY_Exclude[(msg) > SPY_MAX_MSGNUM ? SPY_MAX_MSGNUM : (msg)])

/***********************************************************************
 *           SPY_GetMsgName
 */
const char *SPY_GetMsgName( UINT msg )
{
    static char buffer[20];

    if (msg <= SPY_MAX_MSGNUM)
    {
        if (!MessageTypeNames[msg]) return "???";
        return MessageTypeNames[msg];
    }
    sprintf( buffer, "WM_USER+%04x", msg - WM_USER );
    return buffer;
}


/***********************************************************************
 *           SPY_EnterMessage
 */
void SPY_EnterMessage( INT32 iFlag, HWND32 hWnd, UINT32 msg,
                       WPARAM32 wParam, LPARAM lParam )
{
    if (!debugging_message || SPY_EXCLUDE(msg)) return;

    /* each SPY_SENDMESSAGE must be complemented by call to SPY_ExitMessage */
    switch(iFlag)
    {
    case SPY_DISPATCHMESSAGE16:
        dprintf_message(stddeb,"%*s(%04x) message [%04x] %s dispatched  wp=%04x lp=%08lx\n",
                        SPY_IndentLevel, "", hWnd, msg, SPY_GetMsgName( msg ),
                        wParam, lParam);
        break;

    case SPY_DISPATCHMESSAGE32:
        dprintf_message(stddeb,"%*s(%08x) message [%04x] %s dispatched  wp=%08x lp=%08lx\n",
                        SPY_IndentLevel, "", hWnd, msg, SPY_GetMsgName( msg ),
                        wParam, lParam);
        break;

    case SPY_SENDMESSAGE16:
    case SPY_SENDMESSAGE32:
        {
            char taskName[30];
            HTASK hTask = GetWindowTask16(hWnd);
            if (hTask == GetCurrentTask()) strcpy( taskName, "self" );
            else if (!hTask) strcpy( taskName, "Wine" );
            else sprintf( taskName, "task %04x %s",
                          hTask, MODULE_GetModuleName( GetExePtr(hTask) ) );

            if (iFlag == SPY_SENDMESSAGE16)
                dprintf_message(stddeb,"%*s(%04x) message [%04x] %s sent from %s wp=%04x lp=%08lx\n",
                                SPY_IndentLevel, "", hWnd, msg,
                                SPY_GetMsgName( msg ), taskName, wParam,
                                lParam );
            else
                dprintf_message(stddeb,"%*s(%08x) message [%04x] %s sent from %s wp=%08x lp=%08lx\n",
                                SPY_IndentLevel, "", hWnd, msg,
                                SPY_GetMsgName( msg ), taskName, wParam,
                                lParam );
        }
        break;   

    case SPY_DEFWNDPROC16:
        dprintf_message(stddeb, "%*s(%04x) DefWindowProc: %s [%04x]  wp=%04x lp=%08lx\n",
                        SPY_IndentLevel, "", hWnd, SPY_GetMsgName( msg ),
                        msg, wParam, lParam );
        break;

    case SPY_DEFWNDPROC32:
        dprintf_message(stddeb, "%*s(%08x) DefWindowProc: %s [%04x]  wp=%08x lp=%08lx\n",
                        SPY_IndentLevel, "", hWnd, SPY_GetMsgName( msg ),
                        msg, wParam, lParam );
        break;
    }  
    SPY_IndentLevel += SPY_INDENT_UNIT;
}


/***********************************************************************
 *           SPY_ExitMessage
 */
void SPY_ExitMessage( INT32 iFlag, HWND32 hWnd, UINT32 msg, LRESULT lReturn )
{
    if (!debugging_message || SPY_EXCLUDE(msg)) return;
    if (SPY_IndentLevel) SPY_IndentLevel -= SPY_INDENT_UNIT;

    switch(iFlag)
    {
    case SPY_RESULT_OK16:
        dprintf_message(stddeb,"%*s(%04x) message [%04x] %s returned %08lx\n",
                        SPY_IndentLevel, "", hWnd, msg,
                        SPY_GetMsgName( msg ), lReturn );
        break;
    case SPY_RESULT_OK32:
        dprintf_message(stddeb,"%*s(%08x) message [%04x] %s returned %08lx\n",
                        SPY_IndentLevel, "", hWnd, msg,
                        SPY_GetMsgName( msg ), lReturn );
        break; 
    case SPY_RESULT_INVALIDHWND16:
        dprintf_message(stddeb,"%*s(%04x) message [%04x] %s HAS INVALID HWND\n",
                        SPY_IndentLevel, "", hWnd, msg,
                        SPY_GetMsgName( msg ) );
        break;
    case SPY_RESULT_INVALIDHWND32:
        dprintf_message(stddeb,"%*s(%08x) message [%04x] %s HAS INVALID HWND\n",
                        SPY_IndentLevel, "", hWnd, msg,
                        SPY_GetMsgName( msg ) );
        break;
   }
}


/***********************************************************************
 *           SPY_Init
 */
int SPY_Init(void)
{
    int i;
    char buffer[512];

    PROFILE_GetWineIniString( "Spy", "Include", "", buffer, sizeof(buffer) );
    if (buffer[0] && strcmp( buffer, "INCLUDEALL" ))
    {
        dprintf_message( stddeb, "SpyInit: Include=%s\n", buffer );
        for (i = 0; i <= SPY_MAX_MSGNUM; i++)
            SPY_Exclude[i] = (MessageTypeNames[i] && !strstr(buffer,MessageTypeNames[i]));
    }

    PROFILE_GetWineIniString( "Spy", "Exclude", "", buffer, sizeof(buffer) );
    if (buffer[0])
    {
        dprintf_message( stddeb, "SpyInit: Exclude=%s\n", buffer );
        if (!strcmp( buffer, "EXCLUDEALL" ))
            for (i = 0; i <= SPY_MAX_MSGNUM; i++) SPY_Exclude[i] = TRUE;
        else
            for (i = 0; i <= SPY_MAX_MSGNUM; i++)
                SPY_Exclude[i] = (MessageTypeNames[i] && strstr(buffer,MessageTypeNames[i]));
    }
    return 1;
}
