/*
 * Message spying routines
 *
 * Copyright 1994, Bob Amstadt
 *           1995, Alex Korobka  
 */

#include <stdlib.h>
#include <string.h>
#include "windows.h"
#include "win.h"
#include "module.h"
#include "options.h"
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
    "WM_CANCELJOURNAL", NULL, NULL, 
    "WM_NOTIFY", NULL,

    /* 0x0050 */
    "WM_INPUTLANGCHANGEREQUEST",
    "WM_INPUTLANGCHANGE", 
    "WM_TCARD", 
    "WM_HELP", 
    "WM_USERCHANGED", 
    "WM_NOTIFYFORMAT", NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0060 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0070 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, 
    "WM_CONTEXTMENU", 
    "WM_STYLECHANGING", 
    "WM_STYLECHANGED", 
    "WM_DISPLAYCHANGE", 
    "WM_GETICON",

    "WM_SETICON",		/* 0x0080 */
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
    "EM_GETSEL32",              /* 0x00b0 */
    "EM_SETSEL32",              /* 0x00b1 */
    "EM_GETRECT32",             /* 0x00b2 */
    "EM_SETRECT32",             /* 0x00b3 */
    "EM_SETRECTNP32",           /* 0x00b4 */
    "EM_SCROLL32",              /* 0x00b5 */
    "EM_LINESCROLL32",          /* 0x00b6 */
    "EM_SCROLLCARET32",         /* 0x00b7 */
    "EM_GETMODIFY32",           /* 0x00b8 */
    "EM_SETMODIFY32",           /* 0x00b9 */
    "EM_GETLINECOUNT32",        /* 0x00ba */
    "EM_LINEINDEX32",           /* 0x00bb */
    "EM_SETHANDLE32",           /* 0x00bc */
    "EM_GETHANDLE32",           /* 0x00bd */
    "EM_GETTHUMB32",            /* 0x00be */
    NULL,                       /* 0x00bf */

    NULL,                       /* 0x00c0 */
    "EM_LINELENGTH32",          /* 0x00c1 */
    "EM_REPLACESEL32",          /* 0x00c2 */
    NULL,                       /* 0x00c3 */
    "EM_GETLINE32",             /* 0x00c4 */
    "EM_LIMITTEXT32",           /* 0x00c5 */
    "EM_CANUNDO32",             /* 0x00c6 */
    "EM_UNDO32",                /* 0x00c7 */
    "EM_FMTLINES32",            /* 0x00c8 */
    "EM_LINEFROMCHAR32",        /* 0x00c9 */
    NULL,                       /* 0x00ca */
    "EM_SETTABSTOPS32",         /* 0x00cb */
    "EM_SETPASSWORDCHAR32",     /* 0x00cc */
    "EM_EMPTYUNDOBUFFER32",     /* 0x00cd */
    "EM_GETFIRSTVISIBLELINE32", /* 0x00ce */
    "EM_SETREADONLY32",         /* 0x00cf */

    "EM_SETWORDBREAKPROC32",    /* 0x00d0 */
    "EM_GETWORDBREAKPROC32",    /* 0x00d1 */
    "EM_GETPASSWORDCHAR32",     /* 0x00d2 */
    "EM_SETMARGINS32",          /* 0x00d3 */
    "EM_GETMARGINS32",          /* 0x00d4 */
    "EM_GETLIMITTEXT32",        /* 0x00d5 */
    "EM_POSFROMCHAR32",         /* 0x00d6 */
    "EM_CHARFROMPOS32",         /* 0x00d7 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x00E0 - Win32 Scrollbars */
    "SBM_SETPOS32",             /* 0x00e0 */
    "SBM_GETPOS32",             /* 0x00e1 */
    "SBM_SETRANGE32",           /* 0x00e2 */
    "SBM_GETRANGE32",           /* 0x00e3 */
    "SBM_ENABLE_ARROWS32",      /* 0x00e4 */
    NULL,
    "SBM_SETRANGEREDRAW32",     /* 0x00e6 */
    NULL, NULL,
    "SBM_SETSCROLLINFO32",      /* 0x00e9 */
    "SBM_GETSCROLLINFO32",      /* 0x00ea */
    NULL, NULL, NULL, NULL, NULL,

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
    "CB_GETEDITSEL32",          /* 0x0140 */
    "CB_LIMITTEXT32",           /* 0x0141 */
    "CB_SETEDITSEL32",          /* 0x0142 */
    "CB_ADDSTRING32",           /* 0x0143 */
    "CB_DELETESTRING32",        /* 0x0144 */
    "CB_DIR32",                 /* 0x0145 */
    "CB_GETCOUNT32",            /* 0x0146 */
    "CB_GETCURSEL32",           /* 0x0147 */
    "CB_GETLBTEXT32",           /* 0x0148 */
    "CB_GETLBTEXTLEN32",        /* 0x0149 */
    "CB_INSERTSTRING32",        /* 0x014a */
    "CB_RESETCONTENT32",        /* 0x014b */
    "CB_FINDSTRING32",          /* 0x014c */
    "CB_SELECTSTRING32",        /* 0x014d */
    "CB_SETCURSEL32",           /* 0x014e */
    "CB_SHOWDROPDOWN32",        /* 0x014f */

    "CB_GETITEMDATA32",         /* 0x0150 */
    "CB_SETITEMDATA32",         /* 0x0151 */
    "CB_GETDROPPEDCONTROLRECT32",/* 0x0152 */
    "CB_SETITEMHEIGHT32",       /* 0x0153 */
    "CB_GETITEMHEIGHT32",       /* 0x0154 */
    "CB_SETEXTENDEDUI32",       /* 0x0155 */
    "CB_GETEXTENDEDUI32",       /* 0x0156 */
    "CB_GETDROPPEDSTATE32",     /* 0x0157 */
    "CB_FINDSTRINGEXACT32",     /* 0x0158 */
    "CB_SETLOCALE32",           /* 0x0159 */
    "CB_GETLOCALE32",           /* 0x015a */
    "CB_GETTOPINDEX32",         /* 0x015b */
    "CB_SETTOPINDEX32",         /* 0x015c */
    "CB_GETHORIZONTALEXTENT32", /* 0x015d */
    "CB_SETHORIZONTALEXTENT32", /* 0x015e */
    "CB_GETDROPPEDWIDTH32",     /* 0x015f */

    "CB_SETDROPPEDWIDTH32",     /* 0x0160 */
    "CB_INITSTORAGE32",         /* 0x0161 */
    NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0170 - Win32 Static controls */
    "STM_SETICON32",		/* 0x0170 */
    "STM_GETICON32",		/* 0x0171 */
    "STM_SETIMAGE32",		/* 0x0172 */
    "STM_GETIMAGE32",		/* 0x0173 */
    NULL, NULL, NULL, NULL, 
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
    "LB_CARETON32",             /* 0x01a3 */
    "LB_CARETOFF32",            /* 0x01a4 */
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
    "WM_SIZING", 
    "WM_CAPTURECHANGED",
    "WM_MOVING", NULL,
    "WM_POWERBROADCAST", 
    "WM_DEVICECHANGE", NULL, NULL, NULL, NULL, NULL, NULL,

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
	  NULL, NULL, NULL, NULL, 
    "WM_PRINT", 
    "WM_PRINTCLIENT", 
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
    
    /* 0x03a0 */
    "MM_JOY1MOVE", 
    "MM_JOY2MOVE", 
    "MM_JOY1ZMOVE", 
    "MM_JOY2ZMOVE", 
                            NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x03b0 */
    NULL, NULL, NULL, NULL, NULL, 
    "MM_JOY1BUTTONDOWN", 
    "MM_JOY2BUTTONDOWN", 
    "MM_JOY1BUTTONUP",
    "MM_JOY2BUTTONUP",
    "MM_MCINOTIFY",
                NULL, 
    "MM_WOM_OPEN",
    "MM_WOM_CLOSE",
    "MM_WOM_DONE",
    "MM_WIM_OPEN",
    "MM_WIM_CLOSE",

    /* 0x03c0 */
    "MM_WIM_DATA",
    "MM_MIM_OPEN",
    "MM_MIM_CLOSE",
    "MM_MIM_DATA",
    "MM_MIM_LONGDATA",
    "MM_MIM_ERROR",
    "MM_MIM_LONGERROR",
    "MM_MOM_OPEN",
    "MM_MOM_CLOSE",
    "MM_MOM_DONE",
                NULL, NULL, NULL, NULL, NULL, NULL,
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


static BOOL16 SPY_Exclude[SPY_MAX_MSGNUM+1];
static BOOL16 SPY_ExcludeDWP = 0;
static int SPY_IndentLevel  = 0;

#define SPY_EXCLUDE(msg) \
    (SPY_Exclude[(msg) > SPY_MAX_MSGNUM ? SPY_MAX_MSGNUM : (msg)])

/***********************************************************************
 *           SPY_GetMsgName
 */
const char *SPY_GetMsgName( UINT32 msg )
{
    static char msg_buffer[20];

    if (msg <= SPY_MAX_MSGNUM)
    {
        if (!MessageTypeNames[msg]) return "???";
        return MessageTypeNames[msg];
    }
    sprintf( msg_buffer, "WM_USER+%04x", msg - WM_USER );
    return msg_buffer;
}

/***********************************************************************
 *           SPY_GetWndName
 */
const char *SPY_GetWndName( HWND32 hwnd )
{
    static char wnd_buffer[16];

    WND* pWnd = WIN_FindWndPtr( hwnd );
    if( pWnd )
    {
	INT32 n = sizeof(wnd_buffer) - 6;
	LPSTR p = wnd_buffer;
	LPSTR src;
	
        char  postfix;
	
	if( pWnd->text && pWnd->text[0] != '\0' )
	{
	    src = pWnd->text;
	    *(p++) = postfix = '\"';
	    while ((n-- > 1) && *src) *p++ = *src++;
	}
	else /* get class name */
	{
	    INT32 len;

	    *(p++)='{';
	    GlobalGetAtomName32A( pWnd->class->atomName, p, n + 1);
	    src = p += (len = lstrlen32A(p));
	    if( len >= n ) src = wnd_buffer;	/* something nonzero */
	    postfix = '}';
	}
	if( *src ) for( n = 0; n < 3; n++ ) *(p++)='.';
	*(p++) = postfix;
	*(p++) = '\0';
    }
    else lstrcpy32A( wnd_buffer, "\"NULL\"" );
    return wnd_buffer;
}

/***********************************************************************
 *           SPY_EnterMessage
 */
void SPY_EnterMessage( INT32 iFlag, HWND32 hWnd, UINT32 msg,
                       WPARAM32 wParam, LPARAM lParam )
{
    LPCSTR pname;

    if (!TRACE_ON(message) || SPY_EXCLUDE(msg)) return;

    /* each SPY_SENDMESSAGE must be complemented by call to SPY_ExitMessage */
    switch(iFlag)
    {
    case SPY_DISPATCHMESSAGE16:
	pname = SPY_GetWndName(hWnd);
        TRACE(message,"%*s(%04x) %-16s message [%04x] %s dispatched  wp=%04x lp=%08lx\n",
                        SPY_IndentLevel, "", hWnd, pname, msg, SPY_GetMsgName( msg ),
                        wParam, lParam);
        break;

    case SPY_DISPATCHMESSAGE32:
	pname = SPY_GetWndName(hWnd);
        TRACE(message,"%*s(%08x) %-16s message [%04x] %s dispatched  wp=%08x lp=%08lx\n",
                        SPY_IndentLevel, "", hWnd, pname, msg, SPY_GetMsgName( msg ),
                        wParam, lParam);
        break;

    case SPY_SENDMESSAGE16:
    case SPY_SENDMESSAGE32:
        {
            char taskName[30];
            HTASK16 hTask = GetWindowTask16(hWnd);

            if (hTask == GetCurrentTask()) strcpy( taskName, "self" );
            else if (!hTask) strcpy( taskName, "Wine" );
            else
            {
                sprintf( taskName, "task %04x ???", hTask );
                GetModuleName( hTask, taskName + 10, sizeof(taskName) - 10 );
            }
	    pname = SPY_GetWndName(hWnd);

            if (iFlag == SPY_SENDMESSAGE16)
                TRACE(message, "%*s(%04x) %-16s message [%04x] %s sent from %s wp=%04x lp=%08lx\n",
			     SPY_IndentLevel, "", hWnd, pname, msg, SPY_GetMsgName( msg ), 
			     taskName, wParam, lParam );
            else
                TRACE(message, "%*s(%08x) %-16s message [%04x] %s sent from %s wp=%08x lp=%08lx\n",
			     SPY_IndentLevel, "", hWnd, pname, msg, SPY_GetMsgName( msg ), 
			     taskName, wParam, lParam );
        }
        break;   

    case SPY_DEFWNDPROC16:
	if( SPY_ExcludeDWP ) return;
        TRACE(message, "%*s(%04x)  DefWindowProc16: %s [%04x]  wp=%04x lp=%08lx\n",
                        SPY_IndentLevel, "", hWnd, SPY_GetMsgName( msg ),
                        msg, wParam, lParam );
        break;

    case SPY_DEFWNDPROC32:
	if( SPY_ExcludeDWP ) return;
        TRACE(message, "%*s(%08x)  DefWindowProc32: %s [%04x]  wp=%08x lp=%08lx\n",
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
    LPCSTR pname;

    if (!TRACE_ON(message) || SPY_EXCLUDE(msg) ||
	(SPY_ExcludeDWP && (iFlag == SPY_RESULT_DEFWND16 || iFlag == SPY_RESULT_DEFWND32)) )
	return;

    if (SPY_IndentLevel) SPY_IndentLevel -= SPY_INDENT_UNIT;

    switch(iFlag)
    {
    case SPY_RESULT_DEFWND16:
	TRACE(message," %*s(%04x)  DefWindowProc16: %s [%04x] returned %08lx\n",
			SPY_IndentLevel, "", hWnd, SPY_GetMsgName( msg ), msg, lReturn );
	break;

    case SPY_RESULT_DEFWND32:
	TRACE(message," %*s(%08x)  DefWindowProc32: %s [%04x] returned %08lx\n",
			SPY_IndentLevel, "", hWnd, SPY_GetMsgName( msg ), msg, lReturn );
	break;

    case SPY_RESULT_OK16:
	pname = SPY_GetWndName(hWnd);
        TRACE(message," %*s(%04x) %-16s message [%04x] %s returned %08lx\n",
                        SPY_IndentLevel, "", hWnd, pname, msg,
                        SPY_GetMsgName( msg ), lReturn );
        break;

    case SPY_RESULT_OK32:
	pname = SPY_GetWndName(hWnd);
        TRACE(message," %*s(%08x) %-16s message [%04x] %s returned %08lx\n",
                        SPY_IndentLevel, "", hWnd, pname, msg,
                        SPY_GetMsgName( msg ), lReturn );
        break; 

    case SPY_RESULT_INVALIDHWND16:
	pname = SPY_GetWndName(hWnd);
        WARN(message, " %*s(%04x) %-16s message [%04x] %s HAS INVALID HWND\n",
                        SPY_IndentLevel, "", hWnd, pname, msg,
                        SPY_GetMsgName( msg ) );
        break;

    case SPY_RESULT_INVALIDHWND32:
	pname = SPY_GetWndName(hWnd);
        WARN(message, " %*s(%08x) %-16s message [%04x] %s HAS INVALID HWND\n",
                        SPY_IndentLevel, "", hWnd, pname, msg,
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
    char buffer[1024];

    if (!TRACE_ON(message)) return TRUE;

    PROFILE_GetWineIniString( "Spy", "Include", "", buffer, sizeof(buffer) );
    if (buffer[0] && strcmp( buffer, "INCLUDEALL" ))
    {
        TRACE(message, "Include=%s\n", buffer );
        for (i = 0; i <= SPY_MAX_MSGNUM; i++)
            SPY_Exclude[i] = (MessageTypeNames[i] && !strstr(buffer,MessageTypeNames[i]));
    }

    PROFILE_GetWineIniString( "Spy", "Exclude", "", buffer, sizeof(buffer) );
    if (buffer[0])
    {
        TRACE(message, "Exclude=%s\n", buffer );
        if (!strcmp( buffer, "EXCLUDEALL" ))
            for (i = 0; i <= SPY_MAX_MSGNUM; i++) SPY_Exclude[i] = TRUE;
        else
            for (i = 0; i <= SPY_MAX_MSGNUM; i++)
                SPY_Exclude[i] = (MessageTypeNames[i] && strstr(buffer,MessageTypeNames[i]));
    }

    SPY_ExcludeDWP = PROFILE_GetWineIniInt( "Spy", "ExcludeDWP", 0 );

    return 1;
}
