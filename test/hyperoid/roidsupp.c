//
// ROIDSUPP - hyperoid support functions
//
// Version: 1.1  Copyright (C) 1991, Hutchins Software
//      This software is licenced under the GNU General Public Licence
//      Please read the associated legal documentation
// Author: Edward Hutchins
// Revisions:
// 11/01/91 added GNU General Public License - Ed.
//

#include "hyperoid.h"

//
// defines
//

// you may ask, "why did he embed all these string constants instead of
// using the resource file?". Good question. The answer is: I feel better
// knowing this stuff is part of the executable, and not part of the resource
// file (which can be changed by sneaky people). Or maybe I wuz lazy.
// If you don't like it, then YOU can change it!

#define NL "\x0d\x0a"

#define HYPEROID_HELP \
"The following keys control your ship:" NL NL \
"  Left, Right Arrow .... spin left or right" NL \
"  Down, Up Arrow ..... forward or reverse thrust" NL \
"  Space Bar .............. fire!" NL \
"  Tab ......................... shields" NL \
"  S ............................. smartbomb" NL \
"  Esc ......................... pause/boss key" NL NL \
"Note: You have 3 lives, unlimited fuel and firepower, 3 shields and 3 " \
"smartbombs. Your ship gets darker when you lose a life, but you keep on " \
"playing (unless you hit an asteroid). You get an extra life every 100,000 " \
"points. When you lose the game, you start over immediately and can finish " \
"off the current level (which should now be 0) before starting over at " \
"level 1 (There is no waiting around between games)."

#define HYPEROID_HELP2 \
"The HYPEROID.INI file can be created/modified to change default settings " \
"in Hyperoid. Here are some of the items you can set:" NL \
NL "[Hyperoid]" NL "Max=<0/1>" NL "{X,Y,W,H}=<n>" NL "Mono=<0/1>" NL \
"DrawDelay=<ms> ;microseconds/frame" NL \
NL "[Palette]" NL \
"{Black,DkGrey,Grey,White," NL \
" DkRed,Red,DkGreen,Green,DkBlue,Blue," NL \
" DkYellow,Yellow,DkCyan,Cyan," NL \
" DkMagenta,Magenta}=<r>,<g>,<b>" NL \
NL "[Keys]" NL \
"{Shield,Clockwise,CtrClockwise," NL \
" Thrust,RevThrust,Fire,Bomb}=<virtual keycode>" NL NL \
"Note: Virtual keycodes usually match the key's ASCII value."

#define HYPEROID_HELPSTYLE (MB_OK | MB_ICONASTERISK)

// this is the part I especially want in the executable image

#define HYPEROID_LICENSE \
"This program is free software; you can redistribute it and/or modify " \
"it under the terms of the GNU General Public License as published by " \
"the Free Software Foundation; either version 1, or (at your option) " \
"any later version. " \
NL NL \
"This program is distributed in the hope that it will be useful, " \
"but WITHOUT ANY WARRANTY; without even the implied warranty of " \
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the " \
"GNU General Public License for more details. " \
NL NL \
"You should have received a copy of the GNU General Public License " \
"along with this program; if not, write to the Free Software " \
"Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. "

//
// imports
//

IMPORT CHAR         szAppName[32] FROM( hyperoid.c );
IMPORT HANDLE       hAppInst FROM( hyperoid.c );
IMPORT BOOL         bBW FROM( hyperoid.c );
IMPORT INT          nDrawDelay FROM( hyperoid.c );
IMPORT INT          vkShld FROM( hyperoid.c );
IMPORT INT          vkClkw FROM( hyperoid.c );
IMPORT INT          vkCtrClkw FROM( hyperoid.c );
IMPORT INT          vkThrst FROM( hyperoid.c );
IMPORT INT          vkRvThrst FROM( hyperoid.c );
IMPORT INT          vkFire FROM( hyperoid.c );
IMPORT INT          vkBomb FROM( hyperoid.c );
IMPORT LONG         lHighScore FROM( hyperoid.c );

//
// globals
//

// these parts map to "abcdefghijklm"
GLOBAL POINT        LetterPart[] =
{
	{83, 572}, {64, 512}, {45, 572}, {96, 362}, {32, 362},
	{128, 256}, {0, 0}, {0, 256},
	{160, 362}, {224, 362}, {173, 572}, {192, 512}, {211, 572}
};
// here's the vector font
GLOBAL NPSTR        szNumberDesc[] =
{
	"cakmck",       // 0
	"dbl",          // 1
	"abekm",        // 2
	"abegjlk",      // 3
	"mcfh",         // 4
	"cbfgjlk",      // 5
	"bdiljgi",      // 6
	"acgl",         // 7
	"bdjlieb",      // 8
	"ljebdge"       // 9
};
GLOBAL NPSTR        szLetterDesc[] =
{
	"kdbemhf",      // A
	"kabegjlk",     // B
	"cbflm",        // C
	"kabejlk",      // D
	"cafgfkm",      // E
	"cafgfk",       // F
	"bdiljhg",      // G
	"kafhcm",       // H
	"bl",           // I
	"cjli",         // J
	"akcgm",        // K
	"akm",          // L
	"kagcm",        // M
	"kamc",         // N
	"bdiljeb",      // O
	"kabegf",       // P
	"mlidbejl",     // Q
	"kabegfgm",     // R
	"ebdjli",       // S
	"lbac",         // T
	"ailjc",        // U
	"alc",          // V
	"akgmc",        // W
	"amgkc",        // X
	"aglgc",        // Y
	"ackm"          // Z
};

//
// locals
//

LOCAL CHAR          szIni[] = "HYPEROID.INI";
LOCAL CHAR          szLicense[] = "LicenseRead";
LOCAL CHAR          szDrawDelay[] = "DrawDelay";
LOCAL CHAR          szMax[] = "Max";
LOCAL CHAR          szX[] = "X";
LOCAL CHAR          szY[] = "Y";
LOCAL CHAR          szW[] = "W";
LOCAL CHAR          szH[] = "H";
LOCAL CHAR          szBW[] = "Mono";
LOCAL CHAR          szPalette[] = "Palette";
LOCAL CHAR          szKeys[] = "Keys";
LOCAL CHAR          szShield[] = "Shield";
LOCAL CHAR          szClockwise[] = "Clockwise";
LOCAL CHAR          szCtrClockwise[] = "CtrClockwise";
LOCAL CHAR          szThrust[] = "Thrust";
LOCAL CHAR          szRevThrust[] = "RevThrust";
LOCAL CHAR          szFire[] = "Fire";
LOCAL CHAR          szBomb[] = "Bomb";
LOCAL CHAR          szHi[] = "Hi";
LOCAL CHAR          *szColorName[] =
{
	"Black", "DkGrey", "Grey", "White",
	"DkRed", "Red",  "DkGreen", "Green", "DkBlue", "Blue",
	"DkYellow", "Yellow", "DkCyan", "Cyan", "DkMagenta", "Magenta"
};
LOCAL DWORD         dwColors[] =
{
	RGB(0,0,0), RGB(128,128,128),
	RGB(192,192,192), RGB(255,255,255),
	RGB(128,0,0), RGB(255,0,0),
	RGB(0,128,0), RGB(0,255,0),
	RGB(0,0,128), RGB(0,0,255),
	RGB(128,128,0), RGB(255,255,0),
	RGB(0,128,128), RGB(0,255,255),
	RGB(128,0,128), RGB(255,0,255),
};

//
// PrintLetters - create letter objects from a string
//

VOID FAR PASCAL PrintLetters( NPSTR npszText, POINT Pos, POINT Vel,
							  BYTE byColor, INT nSize )
{
	INT             nLen = strlen( npszText );
	INT             nCnt = nLen;
	INT             nSpace = nSize + nSize / 2;
	INT             nBase = (nLen - 1) * nSpace;
	INT             nBaseStart = Pos.x + nBase / 2;

	while (nCnt--)
	{
		NPOBJ npLtr = CreateLetter( npszText[nCnt], nSize / 2 );
		if (npLtr)
		{
			npLtr->Pos.x = nBaseStart;
			npLtr->Pos.y = Pos.y;
			npLtr->Vel = Vel;
			npLtr->byColor = byColor;
		}
		nBaseStart -= nSpace;
	}
}

//
// SpinLetters - spin letter objects away from center for effect
//

VOID FAR PASCAL SpinLetters( NPSTR npszText, POINT Pos, POINT Vel,
							 BYTE byColor, INT nSize )
{
	INT             nLen = strlen( npszText );
	INT             nCnt = nLen;
	INT             nSpace = nSize + nSize / 2;
	INT             nBase = (nLen - 1) * nSpace;
	INT             nBaseStart = Pos.x + nBase / 2;

	while (nCnt--)
	{
		NPOBJ npLtr = CreateLetter( npszText[nCnt], nSize / 2 );
		if (npLtr)
		{
			INT nSpin = (nCnt - nLen / 2) * 2;
			npLtr->Pos.x = nBaseStart;
			npLtr->Pos.y = Pos.y;
			npLtr->Vel = Vel;
			npLtr->Vel.x += nSpin * 16;
			npLtr->nSpin = -nSpin;
			npLtr->byColor = byColor;
		}
		nBaseStart -= nSpace;
	}
}

//
// CreateHyperoidPalette - create a logical palette
//

HPALETTE FAR PASCAL CreateHyperoidPalette( VOID )
{
	HPALETTE        hPalette;
	HDC             hIC = CreateIC( "DISPLAY", NULL, NULL, NULL );
	INT             t;
	PALETTEENTRY    Pal[PALETTE_SIZE + 2];
	NPLOGPALETTE    npLogPalette = (NPLOGPALETTE)Pal;

	// are we forced into using b&w?
	bBW = FALSE;
	if (GetDeviceCaps( hIC, NUMCOLORS ) < 8) bBW = TRUE;
	DeleteDC( hIC );
	if (GetPrivateProfileInt( szAppName, szBW, FALSE, szIni )) bBW = TRUE;

	npLogPalette->palVersion = 0x0300;
	npLogPalette->palNumEntries = PALETTE_SIZE;

	for (t = 0; t < PALETTE_SIZE; ++t)
	{
		DWORD           dwColor = dwColors[t];
		CHAR            szBuff[32];

		GetPrivateProfileString( szPalette, szColorName[t], "",
								 szBuff, sizeof(szBuff), szIni );
		if (szBuff[0])
		{
			INT r, g, b;
			NPSTR npBuff = szBuff;
			r = g = b = 255;
			while (*npBuff == ' ') ++npBuff;
			r = atoi( npBuff );
			while (*npBuff && *npBuff != ',') ++npBuff;
			if (*npBuff == ',') g = atoi( ++npBuff );
			while (*npBuff && *npBuff != ',') ++npBuff;
			if (*npBuff == ',') b = atoi( ++npBuff );
			dwColor = RGB( r, g, b );
		}
		if (bBW) dwColor = ((dwColor == RGB(0,0,0)) ? RGB(0,0,0) : RGB(255,255,255));
		npLogPalette->palPalEntry[t].peRed = GetRValue( dwColor );
		npLogPalette->palPalEntry[t].peGreen = GetGValue( dwColor );
		npLogPalette->palPalEntry[t].peBlue = GetBValue( dwColor );
		npLogPalette->palPalEntry[t].peFlags = 0;
	}

	hPalette = CreatePalette( npLogPalette );
	return( hPalette );
}

//
// CreateHyperoidClass - create the class of Hyperoid's window
//

BOOL FAR PASCAL CreateHyperoidClass( VOID )
{
	WNDCLASS        Class;

	// load the name from the resource file
	LoadString( hAppInst, IDS_NAME, szAppName, sizeof(szAppName) );

	Class.style = CS_HREDRAW | CS_VREDRAW;
	Class.lpfnWndProc = HyperoidWndProc;
	Class.cbClsExtra = 0;
	Class.cbWndExtra = 0;
	Class.hInstance = hAppInst;
	Class.hIcon = NULL;
	Class.hCursor = LoadCursor( NULL, IDC_CROSS );
	Class.hbrBackground = HNULL;
	Class.lpszMenuName = szAppName;
	Class.lpszClassName = szAppName;

	return( RegisterClass( &Class ) );
}

//
// SetHyperoidMenu - add Hyperoid's menu items to the system menu
//

VOID NEAR PASCAL SetHyperoidMenu( HWND hWnd, INT nFirstID, INT nLastID )
{
	CHAR            szMenuName[40];
	HMENU           hMenu;

	hMenu = GetSystemMenu( hWnd, TRUE );
	if (hMenu == HNULL) hMenu = GetSystemMenu( hWnd, FALSE );
	if (hMenu == HNULL) return;

	while (nFirstID <= nLastID)
	{
		LoadString( hAppInst, nFirstID, szMenuName, sizeof(szMenuName) );
		ChangeMenu( hMenu, 0, szMenuName, nFirstID, MF_APPEND );
		++nFirstID;
	}
}

//
// CreateHyperoidWindow - open the Hyperoid window
//

HWND FAR PASCAL CreateHyperoidWindow( LPSTR lpszCmd, INT nCmdShow )
{
	HWND            hWnd;
	INT             x, y, w, h;
	CHAR            szBuff[32];

	// get the highscore profile here for lack of a better place...
	GetPrivateProfileString( szAppName, szHi, "0", szBuff, sizeof(szBuff), szIni );
	lHighScore = atol( szBuff );

	x = GetPrivateProfileInt( szAppName, szX, CW_USEDEFAULT, szIni );
	y = GetPrivateProfileInt( szAppName, szY, CW_USEDEFAULT, szIni );
	w = GetPrivateProfileInt( szAppName, szW, CW_USEDEFAULT, szIni );
	h = GetPrivateProfileInt( szAppName, szH, CW_USEDEFAULT, szIni );
	if (GetPrivateProfileInt( szAppName, szMax, FALSE, szIni ) &&
		nCmdShow == SW_NORMAL) nCmdShow = SW_SHOWMAXIMIZED;

	hWnd = CreateWindow( szAppName, szAppName, WS_OVERLAPPEDWINDOW,
						 x, y, w, h, HNULL, HNULL, hAppInst, NULL );
	if (hWnd == HNULL) return( HNULL );

	ShowWindow( hWnd, nCmdShow );
	UpdateWindow( hWnd );
	SetHyperoidMenu( hWnd, IDM_NEW, IDM_ABOUT );

	// show the license...
	if (!GetPrivateProfileInt( szAppName, szLicense, FALSE, szIni ))
	{
		MessageBeep( HYPEROID_HELPSTYLE );
		MessageBox( hWnd, HYPEROID_LICENSE, "Hyperoid License", HYPEROID_HELPSTYLE );
		// ...and never show it again (unless they want to see it)
		WritePrivateProfileString( szAppName, szLicense, "1", szIni );
	}

	return( hWnd );
}

//
// SaveHyperoidWindowPos - write out the .ini information
//

VOID FAR PASCAL SaveHyperoidWindowPos( HWND hWnd )
{
	RECT            rect;
	CHAR            szBuff[32];

	// save the highscore profile here for lack of a better place...
	if (lHighScore)
	{
		wsprintf( szBuff, "%lu", lHighScore );
		WritePrivateProfileString( szAppName, szHi, szBuff, szIni );
	}

	if (IsIconic( hWnd )) return;
	if (IsZoomed( hWnd ))
	{
		WritePrivateProfileString( szAppName, szMax, "1", szIni );
		return;
	}
	else WritePrivateProfileString( szAppName, szMax, NULL, szIni );

	GetWindowRect( hWnd, &rect );
	wsprintf( szBuff, "%d", rect.left );
	WritePrivateProfileString( szAppName, szX, szBuff, szIni );
	wsprintf( szBuff, "%d", rect.top );
	WritePrivateProfileString( szAppName, szY, szBuff, szIni );
	wsprintf( szBuff, "%d", rect.right - rect.left );
	WritePrivateProfileString( szAppName, szW, szBuff, szIni );
	wsprintf( szBuff, "%d", rect.bottom - rect.top );
	WritePrivateProfileString( szAppName, szH, szBuff, szIni );
}

//
// GetHyperoidIni - load the ini file information
//

VOID FAR PASCAL GetHyperoidIni( VOID )
{
	nDrawDelay = GetPrivateProfileInt( szAppName, szDrawDelay, DRAW_DELAY, szIni );
	vkShld = GetPrivateProfileInt( szKeys, szShield, VK_TAB, szIni );
	vkClkw = GetPrivateProfileInt( szKeys, szClockwise, VK_LEFT, szIni );
	vkCtrClkw = GetPrivateProfileInt( szKeys, szCtrClockwise, VK_RIGHT, szIni );
	vkThrst = GetPrivateProfileInt( szKeys, szThrust, VK_DOWN, szIni );
	vkRvThrst = GetPrivateProfileInt( szKeys, szRevThrust, VK_UP, szIni );
	vkFire = GetPrivateProfileInt( szKeys, szFire, VK_SPACE, szIni );
	vkBomb = GetPrivateProfileInt( szKeys, szBomb, 'S', szIni );
}

//
// HyperoidHelp - show help
//

VOID FAR PASCAL HyperoidHelp( HWND hWnd )
{
	MessageBox( hWnd, HYPEROID_HELP, "Hyperoid help", HYPEROID_HELPSTYLE );
	MessageBox( hWnd, HYPEROID_HELP2, "Hyperoid.ini help", HYPEROID_HELPSTYLE );
}

//
// HyperoidAboutDlg - the about box proc
//

BOOL FAR PASCAL EXPORT HyperoidAboutDlg( HWND hDlg, WORD mess,
										 WORD wParam, LONG lParam )
{
	switch (mess)
	{
	case WM_INITDIALOG:
		if (lHighScore)
		{
			CHAR szBuff[40];
			wsprintf( szBuff, "High Score: %7.7lu", lHighScore );
			SetDlgItemText( hDlg, IDD_A_HISCORE, szBuff );
		}
		break;

	case WM_COMMAND:
		switch (wParam)
		{
		case IDD_A_HELP:
			HyperoidHelp( hDlg );
			// fall through...
		case IDOK:
			EndDialog( hDlg, 0 );
			break;

		default:
			return( FALSE );
		}
		break;

	case WM_CLOSE:
		EndDialog( hDlg, FALSE );
		break;

	default:
		return( FALSE );
	}
	return( TRUE );
}

//
// AboutHyperoid - show the about box
//

VOID FAR PASCAL AboutHyperoid( HWND hWnd )
{
	FARPROC lpprocAbout = MakeProcInstance( HyperoidAboutDlg, hAppInst );
	DialogBox( hAppInst, INTRES( IDD_ABOUT ), hWnd, lpprocAbout );
	FreeProcInstance( lpprocAbout );
}
