/*
 * Help Viewer
 *
 * Copyright    1996 Ulrich Schmid
 * Copyright    2002 Sylvain Petreolle <spetreolle@yahoo.fr>
 *              2002 Eric Pouech
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define MAX_LANGUAGE_NUMBER     255
#define MAX_PATHNAME_LEN        1024
#define MAX_STRING_LEN          255

#define INTERNAL_BORDER_WIDTH   5
#define POPUP_YDISTANCE         20
#define SHADOW_DX               10
#define SHADOW_DY               10
#define BUTTON_CX               6
#define BUTTON_CY               6

#ifndef RC_INVOKED

#include "hlpfile.h"
#include "macro.h"
#include "winhelp_res.h"

typedef struct tagHelpLinePart
{
    RECT      rect;
    enum {hlp_line_part_text, hlp_line_part_bitmap, hlp_line_part_metafile} cookie;
    union
    {
        struct
        {
            LPCSTR      lpsText;
            HFONT       hFont;
            COLORREF    color;
            WORD        wTextLen;
            WORD        wUnderline; /* 0 None, 1 simple, 2 double, 3 dotted */
        } text;
        struct
        {
            HBITMAP     hBitmap;
        } bitmap;
        struct
        {
            HMETAFILE   hMetaFile;
        } metafile;
    } u;
    HLPFILE_LINK*       link;

    struct tagHelpLinePart *next;
} WINHELP_LINE_PART;

typedef struct tagHelpLine
{
    RECT                rect;
    WINHELP_LINE_PART   first_part;
    struct tagHelpLine* next;
} WINHELP_LINE;

typedef struct tagHelpButton
{
    HWND                hWnd;

    LPCSTR              lpszID;
    LPCSTR              lpszName;
    LPCSTR              lpszMacro;

    WPARAM              wParam;

    RECT                rect;

    struct tagHelpButton*next;
} WINHELP_BUTTON;

typedef struct tagWinHelp
{
    LPCSTR              lpszName;

    WINHELP_BUTTON*     first_button;
    HLPFILE_PAGE*       page;
    WINHELP_LINE*       first_line;

    HWND                hMainWnd;
    HWND                hButtonBoxWnd;
    HWND                hTextWnd;
    HWND                hShadowWnd;
    HWND                hHistoryWnd;

    HFONT*              fonts;
    UINT                fonts_len;

    HCURSOR             hArrowCur;
    HCURSOR             hHandCur;

    HLPFILE_WINDOWINFO* info;

    /* FIXME: for now it's a fixed size */
    HLPFILE_PAGE*       history[40];
    unsigned            histIndex;
    HLPFILE_PAGE*       back[40];
    unsigned            backIndex;

    struct tagWinHelp*  next;
} WINHELP_WINDOW;

typedef struct
{
    UINT                wVersion;
    HANDLE              hInstance;
    HWND                hPopupWnd;
    UINT                wStringTableOffset;
    BOOL                isBook;
    WINHELP_WINDOW*     active_win;
    WINHELP_WINDOW*     win_list;
} WINHELP_GLOBALS;

extern WINHELP_GLOBALS Globals;

BOOL WINHELP_CreateHelpWindowByHash(HLPFILE*, LONG, HLPFILE_WINDOWINFO*, int);
BOOL WINHELP_CreateHelpWindow(HLPFILE_PAGE*, HLPFILE_WINDOWINFO*, int);
INT  WINHELP_MessageBoxIDS(UINT, UINT, WORD);
INT  WINHELP_MessageBoxIDS_s(UINT, LPCSTR, UINT, WORD);
HLPFILE* WINHELP_LookupHelpFile(LPCSTR lpszFile);
HLPFILE_WINDOWINFO* WINHELP_GetWindowInfo(HLPFILE* hlpfile, LPCSTR name);

extern char MAIN_WIN_CLASS_NAME[];
extern char BUTTON_BOX_WIN_CLASS_NAME[];
extern char TEXT_WIN_CLASS_NAME[];
extern char SHADOW_WIN_CLASS_NAME[];
extern char HISTORY_WIN_CLASS_NAME[];
extern char STRING_BUTTON[];
extern char STRING_MENU_Xx[];
extern char STRING_DIALOG_TEST[];
#endif

/* Buttons */
#define WH_FIRST_BUTTON     500
