/* SPY.C
 *
 * Copyright 1994, Bob Amstadt
 */

#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <string.h>
#include "wineopts.h"
#include "windows.h"
#include "wine.h"
#include "options.h"

#ifndef NOSPY

#define SPY_MAX_MSGNUM		0x0232

const char *MessageTypeNames[SPY_MAX_MSGNUM + 1] =
{
    "WM_NULL",			/* 0x00 */
    "WM_CREATE",	
    "WM_DESTROY",    
    "WM_MOVE",
    "WM_UNUSED0",
    "WM_SIZE",
    "WM_ACTIVATE",
    "WM_SETFOCUS",
    "WM_KILLFOCUS",
    "WM_UNUSED1",
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
    "WM_UNUSED2",
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
    "WM_UNUSED4",
    "WM_SPOOLERSTATUS",
    "WM_DRAWITEM",
    "WM_MEASUREITEM",
    "WM_DELETEITEM",
    "WM_VKEYTOITEM",
    "WM_CHARTOITEM",
    "WM_SETFONT",		/* 0x30 */
    "WM_GETFONT", NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x40 */
    NULL, NULL, NULL, NULL, NULL, NULL,
    "WM_WINDOWPOSCHANGING",	/* 0x0046 */
    "WM_WINDOWPOSCHANGED",	/* 0x0047 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0050 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
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
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0090 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x00A0 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

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
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    "WM_INITDIALOG",		/* 0x0110 */
    "WM_COMMAND",		/* 0x0111 */
    "WM_SYSCOMMAND",       	/* 0x0112 */
    "WM_TIMER",			/* 0x0113 */
    "WM_HSCROLL",		/* 0x0114 */
    "WM_VSCROLL",		/* 0x0115 */
    NULL, NULL,
    "WM_SYSTIMER",		/* 0x0118 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0120 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0130 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
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

    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0220 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    
    NULL,			/* 0x0230 */
    "WM_ENTERSIZEMOVE",		/* 0x0231 */
    "WM_EXITSIZEMOVE"		/* 0x0232 */
};

char SpyFilters[256];
char SpyIncludes[256];

#endif /* NOSPY */

/**********************************************************************
 *					SpyMessage
 */
void SpyMessage(HWND hwnd, WORD msg, WORD wParam, LONG lParam)
{
#ifndef NOSPY
    char msg_name[80];
    
    if (SpyFp == NULL)
	return;
    
    if (msg > SPY_MAX_MSGNUM || MessageTypeNames[msg] == NULL)
	msg_name[0] = '\0';
    else
	strcpy(msg_name, MessageTypeNames[msg]);
    
    strcat(msg_name, ";");
    
    if ((strlen(SpyIncludes) == 0 || strstr(SpyIncludes, msg_name) != NULL) &&
	strstr(SpyFilters, msg_name) == NULL)
    {
	msg_name[strlen(msg_name) - 1] = '\0';
	fprintf(SpyFp, "%04.4x  %20.20s  %04.4x  %04.4x  %08.8x\n",
		hwnd, msg_name, msg, wParam, lParam);
    }
#endif
}

/**********************************************************************
 *					SpyInit
 */
void SpyInit(void)
{
    char filename[100];

    if (SpyFp != NULL)
	return;

    if (Options.spyFilename == NULL)
    {
	GetPrivateProfileString("spy", "file", "", filename, sizeof(filename),
				WINE_INI);
    }
    else
	strncpy(filename, Options.spyFilename, 100);

    if (strcasecmp(filename, "CON") == 0)
	SpyFp = stdout;
    else if (strlen(filename))
	SpyFp = fopen(filename, "a");
    else
    {
	SpyFp = NULL;
	return;
    }
    
    GetPrivateProfileString("spy", "exclude", "", SpyFilters, 
			    sizeof(SpyFilters), WINE_INI);
    GetPrivateProfileString("spy", "include", "", SpyIncludes, 
			    sizeof(SpyIncludes), WINE_INI);
}
