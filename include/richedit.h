#ifndef __WINE_RICHEDIT_H
#define __WINE_RICHEDIT_H

#include "windef.h"
#include "pshpack4.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _RICHEDIT_VER
#define _RICHEDIT_VER   0x0210
#endif /* _RICHEDIT_VER */

#define cchTextLimitDefault 0x7fff

#define RICHEDIT_CLASS20A	"RichEdit20A"
static const WCHAR RICHEDIT_CLASS20W[] = { 'R','i','c','h','E','d','i','t','2','0','W', '\0' };
#define RICHEDIT_CLASS10A	"RICHEDIT"

#if (_RICHEDIT_VER >= 0x0200 )
#define RICHEDIT_CLASS		WINELIB_NAME_AW(RICHEDIT_CLASS20)
#else
#define RICHEDIT_CLASS		RICHEDIT_CLASS10A
#endif

#define EM_CANPASTE		(WM_USER + 50)
#define EM_DISPLAYBAND		(WM_USER + 51)
#define EM_EXGETSEL		(WM_USER + 52)
#define EM_EXLIMITTEXT		(WM_USER + 53)
#define EM_EXLINEFROMCHAR	(WM_USER + 54)
#define EM_EXSETSEL		(WM_USER + 55)
#define EM_FINDTEXT		(WM_USER + 56)
#define EM_FORMATRANGE		(WM_USER + 57)
#define EM_GETCHARFORMAT	(WM_USER + 58)
#define EM_GETEVENTMASK		(WM_USER + 59)
#define EM_GETOLEINTERFACE	(WM_USER + 60)
#define EM_GETPARAFORMAT	(WM_USER + 61)
#define EM_GETSELTEXT		(WM_USER + 62)
#define EM_HIDESELECTION	(WM_USER + 63)
#define EM_PASTESPECIAL		(WM_USER + 64)
#define EM_REQUESTRESIZE	(WM_USER + 65)
#define EM_SELECTIONTYPE	(WM_USER + 66)
#define EM_SETBKGNDCOLOR	(WM_USER + 67)
#define EM_SETCHARFORMAT	(WM_USER + 68)
#define EM_SETEVENTMASK		(WM_USER + 69)
#define EM_SETOLECALLBACK	(WM_USER + 70)
#define EM_SETPARAFORMAT	(WM_USER + 71)
#define EM_SETTARGETDEVICE	(WM_USER + 72)
#define EM_STREAMIN		(WM_USER + 73)
#define EM_STREAMOUT		(WM_USER + 74)
#define EM_GETTEXTRANGE		(WM_USER + 75)
#define EM_FINDWORDBREAK	(WM_USER + 76)
#define EM_SETOPTIONS		(WM_USER + 77)
#define EM_GETOPTIONS		(WM_USER + 78)
#define EM_FINDTEXTEX		(WM_USER + 79)
#define EM_GETWORDBREAKPROCEX	(WM_USER + 80)
#define EM_SETWORDBREAKPROCEX	(WM_USER + 81)

#define EM_SETUNDOLIMIT		(WM_USER + 82)
#define EM_REDO			(WM_USER + 84)
#define EM_CANREDO		(WM_USER + 85)
#define EM_GETUNDONAME		(WM_USER + 86)
#define EM_GETREDONAME		(WM_USER + 87)
#define EM_STOPGROUPTYPING	(WM_USER + 88)

#define EM_SETTEXTMODE		(WM_USER + 89)
#define EM_GETTEXTMODE		(WM_USER + 90)
#define EM_AUTOURLDETECT	(WM_USER + 91)
#define EM_GETAUTOURLDETECT	(WM_USER + 92)
#define EM_SETPALETTE		(WM_USER + 93)
#define EM_GETTEXTEX		(WM_USER + 94)
#define EM_GETTEXTLENGTHEX	(WM_USER + 95)
#define EM_SHOWSCROLLBAR	(WM_USER + 96)
#define EM_SETTEXTEX		(WM_USER + 97)

#define EM_SETPUNCTUATION	(WM_USER + 100)
#define EM_GETPUNCTUATION	(WM_USER + 101)
#define EM_SETWORDWRAPMODE	(WM_USER + 102)
#define EM_GETWORDWRAPMODE	(WM_USER + 103)
#define EM_SETIMECOLOR		(WM_USER + 104)
#define EM_GETIMECOLOR		(WM_USER + 105)
#define EM_SETIMEOPTIONS	(WM_USER + 106)
#define EM_GETIMEOPTIONS	(WM_USER + 107)
#define EM_CONVPOSITION		(WM_USER + 108)

#define EM_SETLANGOPTIONS	(WM_USER + 120)
#define EM_GETLANGOPTIONS	(WM_USER + 121)
#define EM_GETIMECOMPMODE	(WM_USER + 122)

#define EM_SETLANGOPTIONS	(WM_USER + 120)
#define EM_GETLANGOPTIONS	(WM_USER + 121)
#define EM_GETIMECOMPMODE	(WM_USER + 122)

#define EM_FINDTEXTW		(WM_USER + 123)
#define EM_FINDTEXTEXW		(WM_USER + 124)

#define EM_RECONVERSION		(WM_USER + 125)
#define EM_SETIMEMODEBIAS	(WM_USER + 126)
#define EM_GETIMEMODEBIAS	(WM_USER + 127)

#define EM_SETBIDIOPTIONS	(WM_USER + 200)
#define EM_GETBIDIOPTIONS	(WM_USER + 201)

#define EM_SETTYPOGRAPHYOPTIONS (WM_USER + 202)
#define EM_GETTYPOGRAPHYOPTIONS (WM_USER + 203)

#define EM_SETEDITSTYLE		(WM_USER + 204)
#define EM_GETEDITSTYLE		(WM_USER + 205)

#define EM_OUTLINE              (WM_USER + 220)

#define EM_GETSCROLLPOS         (WM_USER + 221)
#define EM_SETSCROLLPOS         (WM_USER + 222)

#define EM_SETFONTSIZE          (WM_USER + 223)  
#define EM_GETZOOM		(WM_USER + 224)
#define EM_SETZOOM		(WM_USER + 225)

/* New notifications */
#define EN_MSGFILTER                    0x0700   
#define EN_REQUESTRESIZE                0x0701
#define EN_SELCHANGE                    0x0702
#define EN_DROPFILES                    0x0703
#define EN_PROTECTED                    0x0704
#define EN_CORRECTTEXT                  0x0705
#define EN_STOPNOUNDO                   0x0706
#define EN_IMECHANGE                    0x0707
#define EN_SAVECLIPBOARD                0x0708
#define EN_OLEOPFAILED                  0x0709
#define EN_OBJECTPOSITIONS              0x070a
#define EN_LINK				0x070b
#define EN_DRAGDROPDONE                 0x070c
#define EN_PARAGRAPHEXPANDED		0x070d
#define EN_ALIGNLTR			0x0710
#define EN_ALIGNRTL			0x0711


typedef DWORD (CALLBACK * EDITSTREAMCALLBACK)( DWORD, LPBYTE, LONG, LONG * );

/* Rich edit control styles */
#define ES_SAVESEL            0x00008000
#define ES_SUNKEN             0x00004000
#define ES_DISABLENOSCROLL    0x00002000
#define ES_SELECTIONBAR       0x01000000
#define ES_VERTICAL           0x00400000
#define ES_NOIME              0x00080000
#define ES_SELFIME            0x00040000

/* CHARFORMAT structure */
typedef struct _charformat
{
    UINT       cbSize;
    DWORD      dwMask;
    DWORD      dwEffects;
    LONG       yHeight;
    LONG       yOffset;
    COLORREF   crTextColor;
    BYTE       bCharSet;
    BYTE       bPitchAndFamily;
    char       szFaceName[LF_FACESIZE];
} CHARFORMATA;

typedef struct _charformatw
{
    UINT       cbSize;
    DWORD      dwMask;
    DWORD      dwEffects;
    LONG       yHeight;
    LONG       yOffset;
    COLORREF   crTextColor;
    BYTE       bCharSet;
    BYTE       bPitchAndFamily;
    WCHAR      szFaceName[LF_FACESIZE];
} CHARFORMATW;

DECL_WINELIB_TYPE_AW(CHARFORMAT)

/* CHARFORMAT masks */
#define CFM_BOLD              0x00000001
#define CFM_ITALIC            0x00000002
#define CFM_UNDERLINE         0x00000004
#define CFM_STRIKEOUT         0x00000008
#define CFM_PROTECTED         0x00000010
#define CFM_SIZE              0x80000000
#define CFM_COLOR             0x40000000
#define CFM_FACE              0x20000000
#define CFM_OFFSET            0x10000000
#define CFM_CHARSET           0x08000000

/* CHARFORMAT effects */
#define CFE_BOLD              0x00000001
#define CFE_ITALIC            0x00000002
#define CFE_UNDERLINE         0x00000004
#define CFE_STRIKEOUT         0x00000008
#define CFE_PROTECTED         0x00000010
#define CFE_AUTOCOLOR         0x40000000

/* ECO operations */
#define ECOOP_SET             0x0001
#define ECOOP_OR              0x0002
#define ECOOP_AND             0x0003
#define ECOOP_XOR             0x0004

/* edit control options */
#define ECO_AUTOWORDSELECTION 0x00000001
#define ECO_AUTOVSCROLL       0x00000040
#define ECO_AUTOHSCROLL       0x00000080
#define ECO_NOHIDESEL         0x00000100
#define ECO_READONLY          0x00000800
#define ECO_WANTRETURN        0x00001000
#define ECO_SAVESEL           0x00008000
#define ECO_SELECTIONBAR      0x01000000
#define ECO_VERTICAL          0x00400000

/* Event notification masks */
#define ENM_NONE              0x00000000
#define ENM_CHANGE            0x00000001
#define ENM_UPDATE            0x00000002
#define ENM_SCROLL            0x00000004
#define ENM_KEYEVENTS         0x00010000
#define ENM_MOUSEEVENTS       0x00020000
#define ENM_REQUESTRESIZE     0x00040000
#define ENM_SELCHANGE         0x00080000
#define ENM_DROPFILES         0x00100000
#define ENM_PROTECTED         0x00200000
#define ENM_CORRECTTEXT       0x00400000
#define ENM_IMECHANGE         0x00800000

typedef struct _charrange
{
    LONG    cpMin;
    LONG    cpMax;
} CHARRANGE;

typedef struct
{
    DWORD		dwCookie;
    DWORD		dwError;
    EDITSTREAMCALLBACK	pfnCallback;
} EDITSTREAM;

#define SF_TEXT		0x0001
#define SF_RTF		0x0002
#define SF_RTFNOOBJS	0x0003
#define SF_TEXTIZED	0x0004



#ifdef __cplusplus
}
#endif

#include "poppack.h"

#endif /* __WINE_RICHEDIT_H */
