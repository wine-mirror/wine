/*
 * Message spying routines
 *
 * Copyright 1994, Bob Amstadt
 *           1995, Alex Korobka  
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "windef.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "wine/winbase16.h"
#include "win.h"
#include "options.h"
#include "debugtools.h"
#include "spy.h"
#include "commctrl.h"

DEFAULT_DEBUG_CHANNEL(message);

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
    "WM_INTERIM",
    "WM_IME_STARTCOMPOSITION",	/* 0x010d */
    "WM_IME_ENDCOMPOSITION",	/* 0x010e */
    "WM_IME_COMPOSITION",	/* 0x010f */

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
    "WM_LBTRACKPOINT",          /* 0x0131 */
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
    "WM_MOUSEWHEEL",            /* 0x020A */
    NULL, NULL, NULL, NULL, NULL,

    "WM_PARENTNOTIFY",		/* 0x0210 */
    "WM_ENTERMENULOOP",         /* 0x0211 */
    "WM_EXITMENULOOP",          /* 0x0212 */
    "WM_NEXTMENU", 		/* 0x0213 */
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

    "WM_DROPOBJECT", 
    "WM_QUERYDROPOBJECT", 
    "WM_BEGINDRAG",
    "WM_DRAGLOOP",
    "WM_DRAGSELECT",
    "WM_DRAGMOVE",
     
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
    NULL, "WM_IME_SETCONTEXT", "WM_IME_NOTIFY", "WM_IME_CONTROL", "WM_IME_COMPOSITIONFULL", "WM_IME_SELECT", "WM_IME_CHAR", NULL,
    "WM_IME_REQUEST", NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "WM_IME_KEYDOWN", "WM_IME_KEYUP", NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x02a0 */
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

    "WM_QUERYAFXWNDPROC",   /*  0x0360 */
    "WM_SIZEPARENT",        /*  0x0361 */
    "WM_SETMESSAGESTRING",  /*  0x0362 */
    "WM_IDLEUPDATECMDUI",   /*  0x0363 */
    "WM_INITIALUPDATE",     /*  0x0364 */
    "WM_COMMANDHELP",       /*  0x0365 */
    "WM_HELPHITTEST",       /*  0x0366 */
    "WM_EXITHELPMODE",      /*  0x0367 */
    "WM_RECALCPARENT",      /*  0x0368 */
    "WM_SIZECHILD",         /*  0x0369 */
    "WM_KICKIDLE",          /*  0x036A */
    "WM_QUERYCENTERWND",    /*  0x036B */
    "WM_DISABLEMODAL",      /*  0x036C */
    "WM_FLOATSTATUS",       /*  0x036D */
    "WM_ACTIVATETOPLEVEL",  /*  0x036E */
    "WM_QUERY3DCONTROLS",   /*  0x036F */
    NULL,NULL,NULL,
    "WM_SOCKET_NOTIFY",     /*  0x0373 */
    "WM_SOCKET_DEAD",       /*  0x0374 */
    "WM_POPMESSAGESTRING",  /*  0x0375 */
    "WM_OCC_LOADFROMSTREAM",     /* 0x0376 */
    "WM_OCC_LOADFROMSTORAGE",    /* 0x0377 */
    "WM_OCC_INITNEW",            /* 0x0378 */
    "WM_QUEUE_SENTINEL",         /* 0x0379 */
    "WM_OCC_LOADFROMSTREAM_EX",  /* 0x037A */
    "WM_OCC_LOADFROMSTORAGE_EX", /* 0x037B */

    NULL,NULL,NULL,NULL,

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


#define SPY_MAX_LVMMSGNUM   140
static const char * const LVMMessageTypeNames[SPY_MAX_LVMMSGNUM + 1] =
{
    "LVM_GETBKCOLOR",		/* 1000 */
    "LVM_SETBKCOLOR",
    "LVM_GETIMAGELIST",
    "LVM_SETIMAGELIST",
    "LVM_GETITEMCOUNT",
    "LVM_GETITEMA",
    "LVM_SETITEMA",
    "LVM_INSERTITEMA",
    "LVM_DELETEITEM",
    "LVM_DELETEALLITEMS",
    "LVM_GETCALLBACKMASK",
    "LVM_SETCALLBACKMASK",
    "LVM_GETNEXTITEM",
    "LVM_FINDITEMA",
    "LVM_GETITEMRECT",
    "LVM_SETITEMPOSITION",
    "LVM_GETITEMPOSITION",
    "LVM_GETSTRINGWIDTHA",
    "LVM_HITTEST",
    "LVM_ENSUREVISIBLE",
    "LVM_SCROLL",
    "LVM_REDRAWITEMS",
    "LVM_ARRANGE",
    "LVM_EDITLABELA",
    "LVM_GETEDITCONTROL",
    "LVM_GETCOLUMNA",
    "LVM_SETCOLUMNA",
    "LVM_INSERTCOLUMNA",
    "LVM_DELETECOLUMN",
    "LVM_GETCOLUMNWIDTH",
    "LVM_SETCOLUMNWIDTH",
    "LVM_GETHEADER",
    NULL,
    "LVM_CREATEDRAGIMAGE",
    "LVM_GETVIEWRECT",
    "LVM_GETTEXTCOLOR",
    "LVM_SETTEXTCOLOR",
    "LVM_GETTEXTBKCOLOR",
    "LVM_SETTEXTBKCOLOR",
    "LVM_GETTOPINDEX",
    "LVM_GETCOUNTPERPAGE",
    "LVM_GETORIGIN",
    "LVM_UPDATE",
    "LVM_SETITEMSTATE",
    "LVM_GETITEMSTATE",
    "LVM_GETITEMTEXTA",
    "LVM_SETITEMTEXTA",
    "LVM_SETITEMCOUNT",
    "LVM_SORTITEMS",
    "LVM_SETITEMPOSITION32",
    "LVM_GETSELECTEDCOUNT",
    "LVM_GETITEMSPACING",
    "LVM_GETISEARCHSTRINGA",
    "LVM_SETICONSPACING",
    "LVM_SETEXTENDEDLISTVIEWSTYLE",
    "LVM_GETEXTENDEDLISTVIEWSTYLE",
    "LVM_GETSUBITEMRECT",
    "LVM_SUBITEMHITTEST",
    "LVM_SETCOLUMNORDERARRAY",
    "LVM_GETCOLUMNORDERARRAY",
    "LVM_SETHOTITEM",
    "LVM_GETHOTITEM",
    "LVM_SETHOTCURSOR",
    "LVM_GETHOTCURSOR",
    "LVM_APPROXIMATEVIEWRECT",
    "LVM_SETWORKAREAS",
    "LVM_GETSELECTIONMARK",
    "LVM_SETSELECTIONMARK",
    "LVM_SETBKIMAGEA",
    "LVM_GETBKIMAGEA",
    "LVM_GETWORKAREAS",
    "LVM_SETHOVERTIME",
    "LVM_GETHOVERTIME",
    "LVM_GETNUMBEROFWORKAREAS",
    "LVM_SETTOOLTIPS",
    "LVM_GETITEMW",
    "LVM_SETITEMW",
    "LVM_INSERTITEMW",
    "LVM_GETTOOLTIPS",
    NULL,
    NULL,
    NULL,
    NULL,
    "LVM_FINDITEMW",
    NULL,
    NULL,
    NULL,
    "LVM_GETSTRINGWIDTHW",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "LVM_GETCOLUMNW",
    "LVM_SETCOLUMNW",
    "LVM_INSERTCOLUMNW",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "LVM_GETITEMTEXTW",
    "LVM_SETITEMTEXTW",
    "LVM_GETISEARCHSTRINGW",
    "LVM_EDITLABELW",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "LVM_SETBKIMAGEW",
    "LVM_GETBKIMAGEW"	/* 0x108B */
};


#define SPY_MAX_TVMSGNUM   65
static const char * const TVMessageTypeNames[SPY_MAX_TVMSGNUM + 1] =
{
    "TVM_INSERTITEMA",		/* 1100 */
    "TVM_DELETEITEM",
    "TVM_EXPAND",
    NULL,
    "TVM_GETITEMRECT",
    "TVM_GETCOUNT",
    "TVM_GETINDENT",
    "TVM_SETINDENT",
    "TVM_GETIMAGELIST",
    "TVM_SETIMAGELIST",
    "TVM_GETNEXTITEM",
    "TVM_SELECTITEM",
    "TVM_GETITEMA",
    "TVM_SETITEMA",
    "TVM_EDITLABELA",
    "TVM_GETEDITCONTROL",
    "TVM_GETVISIBLECOUNT",
    "TVM_HITTEST",
    "TVM_CREATEDRAGIMAGE",
    "TVM_SORTCHILDREN",
    "TVM_ENSUREVISIBLE",
    "TVM_SORTCHILDRENCB",
    "TVM_ENDEDITLABELNOW",
    "TVM_GETISEARCHSTRINGA",
    "TVM_SETTOOLTIPS",
    "TVM_GETTOOLTIPS",
    "TVM_SETINSERTMARK",
    "TVM_SETITEMHEIGHT",
    "TVM_GETITEMHEIGHT",
    "TVM_SETBKCOLOR",
    "TVM_SETTEXTCOLOR",
    "TVM_GETBKCOLOR",
    "TVM_GETTEXTCOLOR",
    "TVM_SETSCROLLTIME",
    "TVM_GETSCROLLTIME",
    "TVM_UNKNOWN35",
    "TVM_UNKNOWN36",
    "TVM_SETINSERTMARKCOLOR",
    "TVM_GETINSERTMARKCOLOR",
    "TVM_GETITEMSTATE",
    "TVM_SETLINECOLOR",
    "TVM_GETLINECOLOR",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "TVM_INSERTITEMW",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "TVM_GETITEMW",
    "TVM_SETITEMW",
    "TVM_GETISEARCHSTRINGW",
    "TVM_EDITLABELW"
};


#define SPY_MAX_HDMMSGNUM   19
static const char * const HDMMessageTypeNames[SPY_MAX_HDMMSGNUM + 1] =
{
    "HDM_GETITEMCOUNT",		/* 1200 */
    "HDM_INSERTITEMA",
    "HDM_DELETEITEM",
    "HDM_GETITEMA",
    "HDM_SETITEMA",
    "HDM_LAYOUT",
    "HDM_HITTEST",
    "HDM_GETITEMRECT",
    "HDM_SETIMAGELIST",
    "HDM_GETIMAGELIST",
    "HDM_INSERTITEMW",
    "HDM_GETITEMW",
    "HDM_SETITEMW",
    NULL,
    NULL,
    "HDM_ORDERTOINDEX",
    "HDM_CREATEDRAGIMAGE",
    "GETORDERARRAYINDEX",
    "SETORDERARRAYINDEX",
    "SETHOTDIVIDER"
};


#define SPY_MAX_TCMMSGNUM   62
static const char * const TCMMessageTypeNames[SPY_MAX_TCMMSGNUM + 1] =
{
    NULL,		/* 1300 */
    NULL,
    "TCM_SETIMAGELIST",
    "TCM_GETIMAGELIST",
    "TCM_GETITEMCOUNT",
    "TCM_GETITEMA",
    "TCM_SETITEMA",
    "TCM_INSERTITEMA",
    "TCM_DELETEITEM",
    "TCM_DELETEALLITEMS",
    "TCM_GETITEMRECT",
    "TCM_GETCURSEL",
    "TCM_SETCURSEL",
    "TCM_HITTEST",
    "TCM_SETITEMEXTRA",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "TCM_ADJUSTRECT",
    "TCM_SETITEMSIZE",
    "TCM_REMOVEIMAGE",
    "TCM_SETPADDING",
    "TCM_GETROWCOUNT",
    "TCM_GETTOOLTIPS",
    "TCM_SETTOOLTIPS",
    "TCM_GETCURFOCUS",
    "TCM_SETCURFOCUS",
    "TCM_SETMINTABWIDTH",
    "TCM_DESELECTALL",
    "TCM_HIGHLIGHTITEM",
    "TCM_SETEXTENDEDSTYLE",
    "TCM_GETEXTENDEDSTYLE",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "TCM_GETITEMW",
    "TCM_SETITEMW",
    "TCM_INSERTITEMW"
};

#define SPY_MAX_PGMMSGNUM   13
static const char * const PGMMessageTypeNames[SPY_MAX_PGMMSGNUM + 1] =
{
    NULL,		/* 1400 */
    "PGM_SETCHILD",
    "PGM_RECALCSIZE",
    "PGM_FORWARDMOUSE",
    "PGM_SETBKCOLOR",
    "PGM_GETBKCOLOR",
    "PGM_SETBORDER",
    "PGM_GETBORDER",
    "PGM_SETPOS",
    "PGM_GETPOS",
    "PGM_SETBUTTONSIZE",
    "PGM_GETBUTTONSIZE",
    "PGM_GETBUTTONSTATE",
    "PGM_GETDROPTARGET"
};


#define SPY_MAX_CCMMSGNUM   6
static const char * const CCMMessageTypeNames[SPY_MAX_CCMMSGNUM + 1] =
{
    NULL,		/* 0x2000 */
    "CCM_SETBKCOLOR",
    "CCM_SETCOLORSCHEME",
    "CCM_GETCOLORSCHEME",
    "CCM_GETDROPTARGET",
    "CCM_SETUNICODEFORMAT",
    "CCM_GETUNICODEFORMAT"
};

/* Virtual key names */
#define SPY_MAX_VKKEYSNUM 255
static const char * const VK_KeyNames[SPY_MAX_VKKEYSNUM + 1] =
{
    NULL,		/* 0x00 */
    "VK_LBUTTON",	/* 0x01 */
    "VK_RBUTTON",	/* 0x02 */
    "VK_CANCEL",	/* 0x03 */
    "VK_MBUTTON",	/* 0x04 */
    NULL,		/* 0x05 */
    NULL,		/* 0x06 */
    NULL,		/* 0x07 */
    "VK_BACK",		/* 0x08 */
    "VK_TAB",		/* 0x09 */
    NULL,		/* 0x0A */
    NULL,		/* 0x0B */
    "VK_CLEAR",		/* 0x0C */
    "VK_RETURN",	/* 0x0D */
    NULL,		/* 0x0E */
    NULL,		/* 0x0F */
    "VK_SHIFT",		/* 0x10 */
    "VK_CONTROL",	/* 0x11 */
    "VK_MENU",		/* 0x12 */
    "VK_PAUSE",		/* 0x13 */
    "VK_CAPITAL",	/* 0x14 */
    NULL,		/* 0x15 */
    NULL,		/* 0x16 */
    NULL,		/* 0x17 */
    NULL,		/* 0x18 */
    NULL,		/* 0x19 */
    NULL,		/* 0x1A */
    "VK_ESCAPE",	/* 0x1B */
    NULL,		/* 0x1C */
    NULL,		/* 0x1D */
    NULL,		/* 0x1E */
    NULL,		/* 0x1F */
    "VK_SPACE",		/* 0x20 */
    "VK_PRIOR",		/* 0x21 */
    "VK_NEXT",		/* 0x22 */
    "VK_END",		/* 0x23 */
    "VK_HOME",		/* 0x24 */
    "VK_LEFT",		/* 0x25 */
    "VK_UP",		/* 0x26 */
    "VK_RIGHT",		/* 0x27 */
    "VK_DOWN",		/* 0x28 */
    "VK_SELECT",	/* 0x29 */
    "VK_PRINT",		/* 0x2A */
    "VK_EXECUTE",	/* 0x2B */
    "VK_SNAPSHOT",	/* 0x2C */
    "VK_INSERT",	/* 0x2D */
    "VK_DELETE",	/* 0x2E */
    "VK_HELP",		/* 0x2F */
    "VK_0",		/* 0x30 */
    "VK_1",		/* 0x31 */
    "VK_2",		/* 0x32 */
    "VK_3",		/* 0x33 */
    "VK_4",		/* 0x34 */
    "VK_5",		/* 0x35 */
    "VK_6",		/* 0x36 */
    "VK_7",		/* 0x37 */
    "VK_8",		/* 0x38 */
    "VK_9",		/* 0x39 */
    NULL,		/* 0x3A */
    NULL,		/* 0x3B */
    NULL,		/* 0x3C */
    NULL,		/* 0x3D */
    NULL,		/* 0x3E */
    NULL,		/* 0x3F */
    NULL,		/* 0x40 */
    "VK_A",		/* 0x41 */
    "VK_B",		/* 0x42 */
    "VK_C",		/* 0x43 */
    "VK_D",		/* 0x44 */
    "VK_E",		/* 0x45 */
    "VK_F",		/* 0x46 */
    "VK_G",		/* 0x47 */
    "VK_H",		/* 0x48 */
    "VK_I",		/* 0x49 */
    "VK_J",		/* 0x4A */
    "VK_K",		/* 0x4B */
    "VK_L",		/* 0x4C */
    "VK_M",		/* 0x4D */
    "VK_N",		/* 0x4E */
    "VK_O",		/* 0x4F */
    "VK_P",		/* 0x50 */
    "VK_Q",		/* 0x51 */
    "VK_R",		/* 0x52 */
    "VK_S",		/* 0x53 */
    "VK_T",		/* 0x54 */
    "VK_U",		/* 0x55 */
    "VK_V",		/* 0x56 */
    "VK_W",		/* 0x57 */
    "VK_X",		/* 0x58 */
    "VK_Y",		/* 0x59 */
    "VK_Z",		/* 0x5A */
    "VK_LWIN",		/* 0x5B */
    "VK_RWIN",		/* 0x5C */
    "VK_APPS",		/* 0x5D */
    NULL,		/* 0x5E */
    NULL,		/* 0x5F */
    "VK_NUMPAD0",	/* 0x60 */
    "VK_NUMPAD1",	/* 0x61 */
    "VK_NUMPAD2",	/* 0x62 */
    "VK_NUMPAD3",	/* 0x63 */
    "VK_NUMPAD4",	/* 0x64 */
    "VK_NUMPAD5",	/* 0x65 */
    "VK_NUMPAD6",	/* 0x66 */
    "VK_NUMPAD7",	/* 0x67 */
    "VK_NUMPAD8",	/* 0x68 */
    "VK_NUMPAD9",	/* 0x69 */
    "VK_MULTIPLY",	/* 0x6A */
    "VK_ADD",		/* 0x6B */
    "VK_SEPARATOR",	/* 0x6C */
    "VK_SUBTRACT",	/* 0x6D */
    "VK_DECIMAL",	/* 0x6E */
    "VK_DIVIDE",	/* 0x6F */
    "VK_F1",		/* 0x70 */
    "VK_F2",		/* 0x71 */
    "VK_F3",		/* 0x72 */
    "VK_F4",		/* 0x73 */
    "VK_F5",		/* 0x74 */
    "VK_F6",		/* 0x75 */
    "VK_F7",		/* 0x76 */
    "VK_F8",		/* 0x77 */
    "VK_F9",		/* 0x78 */
    "VK_F10",		/* 0x79 */
    "VK_F11",		/* 0x7A */
    "VK_F12",		/* 0x7B */
    "VK_F13",		/* 0x7C */
    "VK_F14",		/* 0x7D */
    "VK_F15",		/* 0x7E */
    "VK_F16",		/* 0x7F */
    "VK_F17",		/* 0x80 */
    "VK_F18",		/* 0x81 */
    "VK_F19",		/* 0x82 */
    "VK_F20",		/* 0x83 */
    "VK_F21",		/* 0x84 */
    "VK_F22",		/* 0x85 */
    "VK_F23",		/* 0x86 */
    "VK_F24",		/* 0x87 */
    NULL,		/* 0x88 */
    NULL,		/* 0x89 */
    NULL,		/* 0x8A */
    NULL,		/* 0x8B */
    NULL,		/* 0x8C */
    NULL,		/* 0x8D */
    NULL,		/* 0x8E */
    NULL,		/* 0x8F */
    "VK_NUMLOCK",	/* 0x90 */
    "VK_SCROLL",	/* 0x91 */
    NULL,		/* 0x92 */
    NULL,		/* 0x93 */
    NULL,		/* 0x94 */
    NULL,		/* 0x95 */
    NULL,		/* 0x96 */
    NULL,		/* 0x97 */
    NULL,		/* 0x98 */
    NULL,		/* 0x99 */
    NULL,		/* 0x9A */
    NULL,		/* 0x9B */
    NULL,		/* 0x9C */
    NULL,		/* 0x9D */
    NULL,		/* 0x9E */
    NULL,		/* 0x9F */
    "VK_LSHIFT",	/* 0xA0 */
    "VK_RSHIFT",	/* 0xA1 */
    "VK_LCONTROL",	/* 0xA2 */
    "VK_RCONTROL",	/* 0xA3 */
    "VK_LMENU",		/* 0xA4 */
    "VK_RMENU",		/* 0xA5 */
    NULL,		/* 0xA6 */
    NULL,		/* 0xA7 */
    NULL,		/* 0xA8 */
    NULL,		/* 0xA9 */
    NULL,		/* 0xAA */
    NULL,		/* 0xAB */
    NULL,		/* 0xAC */
    NULL,		/* 0xAD */
    NULL,		/* 0xAE */
    NULL,		/* 0xAF */
    NULL,		/* 0xB0 */
    NULL,		/* 0xB1 */
    NULL,		/* 0xB2 */
    NULL,		/* 0xB3 */
    NULL,		/* 0xB4 */
    NULL,		/* 0xB5 */
    NULL,		/* 0xB6 */
    NULL,		/* 0xB7 */
    NULL,		/* 0xB8 */
    NULL,		/* 0xB9 */
    "VK_OEM_1",		/* 0xBA */
    "VK_OEM_PLUS",	/* 0xBB */
    "VK_OEM_COMMA",	/* 0xBC */
    "VK_OEM_MINUS",	/* 0xBD */
    "VK_OEM_PERIOD",	/* 0xBE */
    "VK_OEM_2",		/* 0xBF */
    "VK_OEM_3",		/* 0xC0 */
    NULL,		/* 0xC1 */
    NULL,		/* 0xC2 */
    NULL,		/* 0xC3 */
    NULL,		/* 0xC4 */
    NULL,		/* 0xC5 */
    NULL,		/* 0xC6 */
    NULL,		/* 0xC7 */
    NULL,		/* 0xC8 */
    NULL,		/* 0xC9 */
    NULL,		/* 0xCA */
    NULL,		/* 0xCB */
    NULL,		/* 0xCC */
    NULL,		/* 0xCD */
    NULL,		/* 0xCE */
    NULL,		/* 0xCF */
    NULL,		/* 0xD0 */
    NULL,		/* 0xD1 */
    NULL,		/* 0xD2 */
    NULL,		/* 0xD3 */
    NULL,		/* 0xD4 */
    NULL,		/* 0xD5 */
    NULL,		/* 0xD6 */
    NULL,		/* 0xD7 */
    NULL,		/* 0xD8 */
    NULL,		/* 0xD9 */
    NULL,		/* 0xDA */
    "VK_OEM_4",		/* 0xDB */
    "VK_OEM_5",		/* 0xDC */
    "VK_OEM_6",		/* 0xDD */
    "VK_OEM_7",		/* 0xDE */
    "VK_OEM_8",		/* 0xDF */
    NULL,		/* 0xE0 */
    "VK_OEM_AX",	/* 0xE1 */
    "VK_OEM_102",	/* 0xE2 */
    "VK_ICO_HELP",	/* 0xE3 */
    "VK_ICO_00",	/* 0xE4 */
    "VK_PROCESSKEY",	/* 0xE5 */
    NULL,		/* 0xE6 */
    NULL,		/* 0xE7 */
    NULL,		/* 0xE8 */
    NULL,		/* 0xE9 */
    NULL,		/* 0xEA */
    NULL,		/* 0xEB */
    NULL,		/* 0xEC */
    NULL,		/* 0xED */
    NULL,		/* 0xEE */
    NULL,		/* 0xEF */
    NULL,		/* 0xF0 */
    NULL,		/* 0xF1 */
    NULL,		/* 0xF2 */
    NULL,		/* 0xF3 */
    NULL,		/* 0xF4 */
    NULL,		/* 0xF5 */
    "VK_ATTN",		/* 0xF6 */
    "VK_CRSEL",		/* 0xF7 */
    "VK_EXSEL",		/* 0xF8 */
    "VK_EREOF",		/* 0xF9 */
    "VK_PLAY",		/* 0xFA */
    "VK_ZOOM",		/* 0xFB */
    "VK_NONAME",	/* 0xFC */
    "VK_PA1",		/* 0xFD */
    "VK_OEM_CLEAR",	/* 0xFE */
    NULL		/* 0xFF */
};


/* WM_NOTIFY function codes display */

typedef struct
{
    const char *name;     /* name of notify message        */
    UINT        value;     /* notify code value             */
    UINT        len;       /* length of extra space to dump */
} SPY_NOTIFY;

#define SPNFY(a,b) { #a ,a,sizeof(b)-sizeof(NMHDR)}

/* Array MUST be in descending order by the 'value' field  */
/* (since value is UNSIGNED, 0xffffffff is largest and     */
/*  0xfffffffe is smaller). A binary search is used to     */
/* locate the correct 'value'.                             */
static const SPY_NOTIFY spnfy_array[] = {
    /*  common        0U       to  0U-99U  */
    SPNFY(NM_OUTOFMEMORY,        NMHDR), 
    SPNFY(NM_CLICK,              NMHDR),    
    SPNFY(NM_DBLCLK,             NMHDR),
    SPNFY(NM_RETURN,             NMHDR),  
    SPNFY(NM_RCLICK,             NMHDR),
    SPNFY(NM_RDBLCLK,            NMHDR),       
    SPNFY(NM_SETFOCUS,           NMHDR),
    SPNFY(NM_KILLFOCUS,          NMHDR),
    SPNFY(NM_CUSTOMDRAW,         NMCUSTOMDRAW),
    SPNFY(NM_HOVER,              NMHDR),
    SPNFY(NM_NCHITTEST,          NMHDR),
    SPNFY(NM_KEYDOWN,            NMKEY),
    SPNFY(NM_RELEASEDCAPTURE,    NMHDR),
    SPNFY(NM_SETCURSOR,          NMMOUSE),
    SPNFY(NM_CHAR,               NMCHAR),
    SPNFY(NM_TOOLTIPSCREATED,    NMTOOLTIPSCREATED),  
    /* Listview       0U-100U  to  0U-199U  */
    SPNFY(LVN_ITEMCHANGING,      NMHDR),
    SPNFY(LVN_ITEMCHANGED,       NMHDR),
    SPNFY(LVN_INSERTITEM,        NMHDR),
    SPNFY(LVN_DELETEITEM,        NMHDR),
    SPNFY(LVN_DELETEALLITEMS,    NMHDR),
    SPNFY(LVN_BEGINLABELEDITA,   NMHDR),
    SPNFY(LVN_ENDLABELEDITA,     NMHDR),
    SPNFY(LVN_COLUMNCLICK,       NMHDR),
    SPNFY(LVN_BEGINDRAG,         NMHDR),
    SPNFY(LVN_BEGINRDRAG,        NMHDR),
    SPNFY(LVN_ODCACHEHINT,       NMHDR),
    SPNFY(LVN_ITEMACTIVATE,      NMHDR),
    SPNFY(LVN_ODSTATECHANGED,    NMHDR),
    SPNFY(LVN_HOTTRACK,          NMHDR),
    SPNFY(LVN_GETDISPINFOA,      NMHDR),
    SPNFY(LVN_SETDISPINFOA,      NMHDR),
    SPNFY(LVN_KEYDOWN,           NMHDR),
    SPNFY(LVN_MARQUEEBEGIN,      NMHDR),
    SPNFY(LVN_GETINFOTIPA,       NMHDR),
    SPNFY(LVN_GETINFOTIPW,      NMHDR),
    SPNFY(LVN_BEGINLABELEDITW,   NMHDR),
    SPNFY(LVN_ENDLABELEDITW,     NMHDR),
    SPNFY(LVN_GETDISPINFOW,      NMHDR),
    SPNFY(LVN_SETDISPINFOW,      NMHDR),
    SPNFY(LVN_ODFINDITEMW,       NMHDR),
    /* Header         0U-300U  to  0U-399U  */
    SPNFY(HDN_ITEMCHANGINGA,     NMHDR),
    SPNFY(HDN_ITEMCHANGEDA,      NMHDR),
    SPNFY(HDN_ITEMCLICKA,        NMHDR),
    SPNFY(HDN_ITEMDBLCLICKA,     NMHDR),
    SPNFY(HDN_DIVIDERDBLCLICKA,  NMHDR),
    SPNFY(HDN_BEGINTRACKA,       NMHDR),
    SPNFY(HDN_ENDTRACKA,         NMHDR),
    SPNFY(HDN_GETDISPINFOA,      NMHDR),
    SPNFY(HDN_BEGINDRAG,         NMHDR),
    SPNFY(HDN_ENDDRAG,           NMHDR),
    SPNFY(HDN_ITEMCHANGINGW,     NMHDR),
    SPNFY(HDN_ITEMCHANGEDW,      NMHDR),
    SPNFY(HDN_ITEMCLICKW,        NMHDR),
    SPNFY(HDN_ITEMDBLCLICKW,     NMHDR),
    SPNFY(HDN_DIVIDERDBLCLICKW,  NMHDR),
    SPNFY(HDN_BEGINTRACKW,       NMHDR),
    SPNFY(HDN_ENDTRACKW,         NMHDR),
    SPNFY(HDN_GETDISPINFOW,      NMHDR),
    /* Treeview       0U-400U  to  0U-499U  */
    SPNFY(TVN_SELCHANGINGA,      NMHDR),
    SPNFY(TVN_SELCHANGEDA,       NMHDR),
    SPNFY(TVN_GETDISPINFOA,      NMHDR),
    SPNFY(TVN_SETDISPINFOA,      NMHDR),
    SPNFY(TVN_ITEMEXPANDINGA,    NMHDR),
    SPNFY(TVN_ITEMEXPANDEDA,     NMHDR),
    SPNFY(TVN_BEGINDRAGA,        NMHDR),
    SPNFY(TVN_BEGINRDRAGA,       NMHDR),
    SPNFY(TVN_DELETEITEMA,       NMHDR),
    SPNFY(TVN_BEGINLABELEDITA,   NMHDR),
    SPNFY(TVN_ENDLABELEDITA,     NMHDR),
    SPNFY(TVN_KEYDOWN,           NMHDR),
    SPNFY(TVN_SELCHANGINGW,      NMHDR),
    SPNFY(TVN_SELCHANGEDW,       NMHDR),
    SPNFY(TVN_GETDISPINFOW,      NMHDR),
    SPNFY(TVN_SETDISPINFOW,      NMHDR),
    SPNFY(TVN_ITEMEXPANDINGW,    NMHDR),
    SPNFY(TVN_ITEMEXPANDEDW,     NMHDR),
    SPNFY(TVN_BEGINDRAGW,        NMHDR),
    SPNFY(TVN_BEGINRDRAGW,       NMHDR),
    SPNFY(TVN_DELETEITEMW,       NMHDR),
    SPNFY(TVN_BEGINLABELEDITW,   NMHDR),
    SPNFY(TVN_ENDLABELEDITW,     NMHDR),
    /* Tooltips       0U-520U  to  0U-549U  */
    /* Tab            0U-550U  to  0U-580U  */
    SPNFY(TCN_KEYDOWN,           NMHDR),
    SPNFY(TCN_SELCHANGE,         NMHDR),
    SPNFY(TCN_SELCHANGING,       NMHDR),
    SPNFY(TCN_GETOBJECT,         NMHDR),
    /* Common Dialog  0U-601U  to  0U-699U  */
    /* Toolbar        0U-700U  to  0U-720U  */
    SPNFY(TBN_GETBUTTONINFOA,    NMTOOLBARA),
    SPNFY(TBN_BEGINDRAG,         NMTOOLBARA),
    SPNFY(TBN_ENDDRAG,           NMTOOLBARA),
    SPNFY(TBN_BEGINADJUST,       NMHDR),
    SPNFY(TBN_ENDADJUST,         NMHDR),
    SPNFY(TBN_RESET,             NMHDR),
    SPNFY(TBN_QUERYINSERT,       NMTOOLBARA),
    SPNFY(TBN_QUERYDELETE,       NMTOOLBARA),
    SPNFY(TBN_TOOLBARCHANGE,     NMHDR),
    SPNFY(TBN_CUSTHELP,          NMHDR),
    SPNFY(TBN_DROPDOWN,          NMTOOLBARA),
    SPNFY(TBN_GETOBJECT,         NMOBJECTNOTIFY),
    SPNFY(TBN_HOTITEMCHANGE,     NMHDR),    /* NMTBHOTITEM), */
    SPNFY(TBN_DRAGOUT,           NMTOOLBARA),
    SPNFY(TBN_DELETINGBUTTON,    NMTOOLBARA),
    SPNFY(TBN_GETDISPINFOA,      NMHDR),    /* NMTBDISPINFO), */
    SPNFY(TBN_GETDISPINFOW,      NMHDR),    /* NMTBDISPINFO), */
    SPNFY(TBN_GETINFOTIPA,       NMTBGETINFOTIPA),
    SPNFY(TBN_GETINFOTIPW,       NMTBGETINFOTIPW),
    SPNFY(TBN_GETBUTTONINFOW,    NMTOOLBARW),
    /* Up/Down        0U-721U  to  0U-740U  */
    /* Month Calendar 0U-750U  to  0U-759U  */
    /* Date/Time      0U-760U  to  0U-799U  */
    /* ComboBoxEx     0U-800U  to  0U-830U  */
    SPNFY(CBEN_GETDISPINFOA,     NMHDR),    /* NMCOMBOBOXEX), */
    SPNFY(CBEN_INSERTITEM,       NMHDR),    /* NMCOMBOBOXEX), */
    SPNFY(CBEN_DELETEITEM,       NMHDR),    /* NMCOMBOBOXEX), */
    SPNFY(CBEN_BEGINEDIT,        NMHDR),
    SPNFY(CBEN_ENDEDITA,         NMCBEENDEDITA),
    SPNFY(CBEN_ENDEDITW,         NMCBEENDEDITW),
    SPNFY(CBEN_GETDISPINFOW,     NMHDR),    /* NMCOMBOBOXEXW), */
    SPNFY(CBEN_DRAGBEGINA,       NMHDR),    /* NMCBEDRAGBEGINA), */
    SPNFY(CBEN_DRAGBEGINW,       NMHDR),    /* NMCBEDRAGBEGINW), */
    /* Rebar          0U-831U  to  0U-859U  */
    SPNFY(RBN_HEIGHTCHANGE,      NMHDR),
    SPNFY(RBN_GETOBJECT,         NMOBJECTNOTIFY),
    SPNFY(RBN_LAYOUTCHANGED,     NMHDR),
    SPNFY(RBN_AUTOSIZE,          NMRBAUTOSIZE),
    SPNFY(RBN_BEGINDRAG,         NMREBAR),
    SPNFY(RBN_ENDDRAG,           NMREBAR),
    SPNFY(RBN_DELETINGBAND,      NMREBAR),
    SPNFY(RBN_DELETEDBAND,       NMREBAR),
    SPNFY(RBN_CHILDSIZE,         NMREBARCHILDSIZE),
    /* IP Adderss     0U-860U  to  0U-879U  */
    /* Status bar     0U-880U  to  0U-899U  */
    /* Pager          0U-900U  to  0U-950U  */
    {0,0,0}};
static const SPY_NOTIFY *end_spnfy_array;     /* ptr to last good entry in array */
#undef SPNFY


static BOOL16 SPY_Exclude[SPY_MAX_MSGNUM+1];
static BOOL16 SPY_ExcludeDWP = 0;
static int SPY_IndentLevel  = 0;

#define SPY_EXCLUDE(msg) \
    (SPY_Exclude[(msg) > SPY_MAX_MSGNUM ? SPY_MAX_MSGNUM : (msg)])

/***********************************************************************
 *           SPY_GetMsgName
 */
const char *SPY_GetMsgName( UINT msg )
{
    static char msg_buffer[20];

    if (msg <= SPY_MAX_MSGNUM)
    {
        if (!MessageTypeNames[msg]) return "???";
        return MessageTypeNames[msg];
    }

    if (msg >= LVM_FIRST && msg <= LVM_FIRST + SPY_MAX_LVMMSGNUM)
    {
        if (!LVMMessageTypeNames[msg-LVM_FIRST]) return "LVM_?";
        return LVMMessageTypeNames[msg-LVM_FIRST];
    }

    if (msg >= TV_FIRST && msg <= TV_FIRST + SPY_MAX_TVMSGNUM)
    {
        if (!TVMessageTypeNames[msg-TV_FIRST]) return "TV_?";
        return TVMessageTypeNames[msg-TV_FIRST];
    }

    if (msg >= HDM_FIRST && msg <= HDM_FIRST + SPY_MAX_HDMMSGNUM)
    {
        if (!HDMMessageTypeNames[msg-HDM_FIRST]) return "HDM_?";
        return HDMMessageTypeNames[msg-HDM_FIRST];
    }

    if (msg >= TCM_FIRST && msg <= TCM_FIRST + SPY_MAX_TCMMSGNUM)
    {
        if (!TCMMessageTypeNames[msg-TCM_FIRST]) return "TCM_?";
        return TCMMessageTypeNames[msg-TCM_FIRST];
    }

    if (msg >= PGM_FIRST && msg <= PGM_FIRST + SPY_MAX_PGMMSGNUM)
    {
        if (!PGMMessageTypeNames[msg-PGM_FIRST]) return "PGM_?";
        return PGMMessageTypeNames[msg-PGM_FIRST];
    }

    if (msg >= CCM_FIRST && msg <= CCM_FIRST + SPY_MAX_CCMMSGNUM)
    {
        if (!CCMMessageTypeNames[msg-CCM_FIRST]) return "???";
        return CCMMessageTypeNames[msg-CCM_FIRST];
    }

    sprintf( msg_buffer, "WM_USER+%04x", msg - WM_USER );
    return msg_buffer;
}

/***********************************************************************
 *           SPY_GetWndName
 */
const char *SPY_GetWndName( HWND hwnd )
{
    static char wnd_buffer[16];

    WND* pWnd = WIN_FindWndPtr( hwnd );
    if( pWnd )
    {
	INT n = sizeof(wnd_buffer) - 6;
	LPSTR p = wnd_buffer;
        char  postfix;
	
	if( pWnd->text && pWnd->text[0] != '\0' )
	{
	    LPWSTR src = pWnd->text;
	    *(p++) = postfix = '\"';
	    while ((n-- > 1) && *src) *p++ = *src++;
            if( *src ) for( n = 0; n < 3; n++ ) *(p++)='.';
	}
	else /* get class name */
	{
	    *(p++)='{';
	    GlobalGetAtomNameA((ATOM) GetClassWord(pWnd->hwndSelf, GCW_ATOM), p, n + 1);
	    p += strlen(p);
	    postfix = '}';
	}
	*(p++) = postfix;
	*(p++) = '\0';
        WIN_ReleaseWndPtr(pWnd);

    }
    else strcpy( wnd_buffer, "\"NULL\"" );
    return wnd_buffer;
}

/***********************************************************************
 *           SPY_GetVKeyName
 */
const char *SPY_GetVKeyName(WPARAM wParam)
{
    const char *vk_key_name;

    if(wParam <= SPY_MAX_VKKEYSNUM && VK_KeyNames[wParam])
	vk_key_name = VK_KeyNames[wParam];
    else
	vk_key_name = "VK_???";

    return vk_key_name;
}

/***********************************************************************
 *           SPY_Bsearch_Notify
 */
const SPY_NOTIFY *SPY_Bsearch_Notify( const SPY_NOTIFY *first, const SPY_NOTIFY *last, UINT code)
{
    INT count;
    const SPY_NOTIFY *test;

    while (last >= first) {
	count = 1 + last - first;
	if (count < 3) {
	    /* TRACE("code=%d, f-value=%d, f-name=%s, l-value=%d, l-name=%s, l-len=%d,\n",
	       code, first->value, first->name, last->value, last->name, last->len); */
	    if (first->value == code) return first;
	    if (last->value == code) return last;
	    return NULL;
	}
	count = count / 2;
	test = first + count;
	/* TRACE("first=%p, last=%p, test=%p, t-value=%d, code=%d, count=%d\n",
	   first, last, test, test->value, code, count); */
	if (test->value == code) return test;
	if (test->value < code)
	    last = test - 1;
	else
	    first = test + 1;
    }
    return NULL;
}

/***********************************************************************
 *           SPY_DumpStructure
 */
void SPY_DumpStructure (UINT msg, BOOL enter, LPARAM structure)
{
    switch (msg)
	{
	case WM_DRAWITEM:
	    if (!enter) break;
	    {   
		DRAWITEMSTRUCT *lpdis = (DRAWITEMSTRUCT*) structure;
		TRACE("DRAWITEMSTRUCT: CtlType=0x%08x CtlID=0x%08x\n", 
		      lpdis->CtlType, lpdis->CtlID);
		TRACE("itemID=0x%08x itemAction=0x%08x itemState=0x%08x\n", 
		      lpdis->itemID, lpdis->itemAction, lpdis->itemState);
		TRACE("hWnd=0x%04x hDC=0x%04x (%d,%d)-(%d,%d) itemData=0x%08lx\n",
		      lpdis->hwndItem, lpdis->hDC, lpdis->rcItem.left, 
		      lpdis->rcItem.top, lpdis->rcItem.right,
		      lpdis->rcItem.bottom, lpdis->itemData);
	    }
	    break;
	case WM_MEASUREITEM:
	    {   
		MEASUREITEMSTRUCT *lpmis = (MEASUREITEMSTRUCT*) structure;
		TRACE("MEASUREITEMSTRUCT: CtlType=0x%08x CtlID=0x%08x\n", 
		      lpmis->CtlType, lpmis->CtlID);
		TRACE("itemID=0x%08x itemWidth=0x%08x itemHeight=0x%08x\n", 
		      lpmis->itemID, lpmis->itemWidth, lpmis->itemHeight);
		TRACE("itemData=0x%08lx\n", lpmis->itemData);
	    }
	    break;
	case WM_WINDOWPOSCHANGED:
	    if (!enter) break;
	case WM_WINDOWPOSCHANGING:
	    {   
		WINDOWPOS *lpwp = (WINDOWPOS *)structure;
		TRACE("WINDOWPOS hwnd=0x%04x, after=0x%04x, at (%d,%d) h=%d w=%d, flags=0x%08x\n",
		      lpwp->hwnd, lpwp->hwndInsertAfter, lpwp->x, lpwp->y,
		      lpwp->cx, lpwp->cy, lpwp->flags);
	    }
	    break;
	case WM_STYLECHANGED:
	    if (!enter) break;
	case WM_STYLECHANGING:
	    {   
		LPSTYLESTRUCT ss = (LPSTYLESTRUCT) structure;
		TRACE("STYLESTRUCT: StyleOld=0x%08lx, StyleNew=0x%08lx\n",
		      ss->styleOld, ss->styleNew); 
	    }
	    break;
	case WM_NOTIFY:
	    if (!enter) break;
	    {   
		NMHDR * pnmh = (NMHDR*) structure;
		UINT *q;
		const SPY_NOTIFY *p;

		p = SPY_Bsearch_Notify (&spnfy_array[0], end_spnfy_array, 
					pnmh->code);
		if (p) {
		    TRACE("NMHDR hwndFrom=0x%08x idFrom=0x%08x code=%s<0x%08x>, extra=0x%x\n",
			  pnmh->hwndFrom, pnmh->idFrom, p->name, pnmh->code, p->len);
		    if (p->len > 0) {
			int i;
			q = (UINT *)(pnmh + 1);
			for(i=0; i<((INT)p->len)-12; i+=16) {
			    TRACE("NM extra [%04x] %08x %08x %08x %08x\n",
				  i, *q, *(q+1), *(q+2), *(q+3));
			    q += 4;
			}
			switch (p->len - i) {
			case 12:
			    TRACE("NM extra [%04x] %08x %08x %08x\n",
				  i, *q, *(q+1), *(q+2));
			    break;
			case 8:
			    TRACE("NM extra [%04x] %08x %08x\n",
				  i, *q, *(q+1));
			    break;
			case 4:
			    TRACE("NM extra [%04x] %08x\n",
				  i, *q);
			    break;
			default:
			    break;
			}
		    }
		}
		else
		    TRACE("NMHDR hwndFrom=0x%08x idFrom=0x%08x code=0x%08x\n",
			  pnmh->hwndFrom, pnmh->idFrom, pnmh->code);
	    }
	default:
	    break;
	}
	
}
/***********************************************************************
 *           SPY_EnterMessage
 */
void SPY_EnterMessage( INT iFlag, HWND hWnd, UINT msg,
                       WPARAM wParam, LPARAM lParam )
{
    LPCSTR pname;

    if (!TRACE_ON(message) || SPY_EXCLUDE(msg)) return;

    /* each SPY_SENDMESSAGE must be complemented by call to SPY_ExitMessage */
    switch(iFlag)
    {
    case SPY_DISPATCHMESSAGE16:
	pname = SPY_GetWndName(hWnd);
        TRACE("%*s(%04x) %-16s message [%04x] %s dispatched  wp=%04x lp=%08lx\n",
                        SPY_IndentLevel, "", hWnd, pname, msg, SPY_GetMsgName( msg ),
                        wParam, lParam);
        break;

    case SPY_DISPATCHMESSAGE:
	pname = SPY_GetWndName(hWnd);
        TRACE("%*s(%08x) %-16s message [%04x] %s dispatched  wp=%08x lp=%08lx\n",
                        SPY_IndentLevel, "", hWnd, pname, msg, SPY_GetMsgName( msg ),
                        wParam, lParam);
        break;

    case SPY_SENDMESSAGE16:
    case SPY_SENDMESSAGE:
        {
            char taskName[30];
            HTASK16 hTask = GetWindowTask16(hWnd);

            if (hTask == GetCurrentTask()) strcpy( taskName, "self" );
            else if (!hTask) strcpy( taskName, "Wine" );
            else
            {
                sprintf( taskName, "task %04x ???", hTask );
                GetModuleName16( hTask, taskName + 10, sizeof(taskName) - 10 );
            }
	    pname = SPY_GetWndName(hWnd);

            if (iFlag == SPY_SENDMESSAGE16)
                TRACE("%*s(%04x) %-16s message [%04x] %s sent from %s wp=%04x lp=%08lx\n",
			     SPY_IndentLevel, "", hWnd, pname, msg, SPY_GetMsgName( msg ), 
			     taskName, wParam, lParam );
            else
            {   TRACE("%*s(%08x) %-16s message [%04x] %s sent from %s wp=%08x lp=%08lx\n",
			     SPY_IndentLevel, "", hWnd, pname, msg, SPY_GetMsgName( msg ), 
			     taskName, wParam, lParam );
		SPY_DumpStructure(msg, TRUE, lParam);
	    }
        }
        break;   

    case SPY_DEFWNDPROC16:
	if( SPY_ExcludeDWP ) return;
        TRACE("%*s(%04x)  DefWindowProc16: %s [%04x]  wp=%04x lp=%08lx\n",
                        SPY_IndentLevel, "", hWnd, SPY_GetMsgName( msg ),
                        msg, wParam, lParam );
        break;

    case SPY_DEFWNDPROC:
	if( SPY_ExcludeDWP ) return;
        TRACE("%*s(%08x)  DefWindowProc32: %s [%04x]  wp=%08x lp=%08lx\n",
                        SPY_IndentLevel, "", hWnd, SPY_GetMsgName( msg ),
                        msg, wParam, lParam );
        break;
    }  
    SPY_IndentLevel += SPY_INDENT_UNIT;
}


/***********************************************************************
 *           SPY_ExitMessage
 */
void SPY_ExitMessage( INT iFlag, HWND hWnd, UINT msg, LRESULT lReturn,
                       WPARAM wParam, LPARAM lParam )
{
    LPCSTR pname;

    if (!TRACE_ON(message) || SPY_EXCLUDE(msg) ||
	(SPY_ExcludeDWP && (iFlag == SPY_RESULT_DEFWND16 || iFlag == SPY_RESULT_DEFWND)) )
	return;

    if (SPY_IndentLevel) SPY_IndentLevel -= SPY_INDENT_UNIT;

    switch(iFlag)
    {
    case SPY_RESULT_DEFWND16:
	TRACE(" %*s(%04x)  DefWindowProc16: %s [%04x] returned %08lx\n",
			SPY_IndentLevel, "", hWnd, SPY_GetMsgName( msg ), msg, lReturn );
	break;

    case SPY_RESULT_DEFWND:
	TRACE(" %*s(%08x)  DefWindowProc32: %s [%04x] returned %08lx\n",
			SPY_IndentLevel, "", hWnd, SPY_GetMsgName( msg ), msg, lReturn );
	break;

    case SPY_RESULT_OK16:
	pname = SPY_GetWndName(hWnd);
        TRACE(" %*s(%04x) %-16s message [%04x] %s returned %08lx\n",
                        SPY_IndentLevel, "", hWnd, pname, msg,
                        SPY_GetMsgName( msg ), lReturn );
        break;

    case SPY_RESULT_OK:
	pname = SPY_GetWndName(hWnd);
        TRACE(" %*s(%08x) %-16s message [%04x] %s returned %08lx\n",
                        SPY_IndentLevel, "", hWnd, pname, msg,
                        SPY_GetMsgName( msg ), lReturn );
	SPY_DumpStructure(msg, FALSE, lParam);
        break; 

    case SPY_RESULT_INVALIDHWND16:
	pname = SPY_GetWndName(hWnd);
        WARN(" %*s(%04x) %-16s message [%04x] %s HAS INVALID HWND\n",
                        SPY_IndentLevel, "", hWnd, pname, msg,
                        SPY_GetMsgName( msg ) );
        break;

    case SPY_RESULT_INVALIDHWND:
	pname = SPY_GetWndName(hWnd);
        WARN(" %*s(%08x) %-16s message [%04x] %s HAS INVALID HWND\n",
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
    const SPY_NOTIFY *p;

    if (!TRACE_ON(message)) return TRUE;

    PROFILE_GetWineIniString( "Spy", "Include", "", buffer, sizeof(buffer) );
    if (buffer[0] && strcmp( buffer, "INCLUDEALL" ))
    {
        TRACE("Include=%s\n", buffer );
        for (i = 0; i <= SPY_MAX_MSGNUM; i++)
            SPY_Exclude[i] = (MessageTypeNames[i] && !strstr(buffer,MessageTypeNames[i]));
    }

    PROFILE_GetWineIniString( "Spy", "Exclude", "", buffer, sizeof(buffer) );
    if (buffer[0])
    {
        TRACE("Exclude=%s\n", buffer );
        if (!strcmp( buffer, "EXCLUDEALL" ))
            for (i = 0; i <= SPY_MAX_MSGNUM; i++) SPY_Exclude[i] = TRUE;
        else
            for (i = 0; i <= SPY_MAX_MSGNUM; i++)
                SPY_Exclude[i] = (MessageTypeNames[i] && strstr(buffer,MessageTypeNames[i]));
    }

    SPY_ExcludeDWP = PROFILE_GetWineIniInt( "Spy", "ExcludeDWP", 0 );

    /* find last good entry in spy notify array and save addr for b-search */
    p = &spnfy_array[0];
    while (p->name) p++;
    p--;
    end_spnfy_array = p;

    return 1;
}
