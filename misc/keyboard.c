/*
static char RCSId[] = "$Id: keyboard.c,v 1.2 1993/09/13 18:52:02 scott Exp $";
static char Copyright[] = "Copyright  Scott A. Laird, Erik Bos  1993, 1994";
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "windows.h"
#include "keyboard.h"
#include "stddebug.h"
/* #define DEBUG_KEYBOARD */
#include "debug.h"


struct KeyTableEntry {
	int virtualkey;
	int ASCII;
	int scancode;
	const char *name;
};

static const struct KeyTableEntry KeyTable[] =
{
	{ VK_CANCEL,	0x3,	0,	"" },
	{ VK_BACK,	0x8,	0xe,	"Backspace" },
	{ VK_TAB,	0x9,	0xf,	"Tab" },
	{ VK_CLEAR,	0,	0x4c,	"Clear" },
	{ VK_RETURN,	0xd,	0x1c,	"Enter" },
	{ VK_SHIFT,	0,	0x2a,	"Shift" },
	{ VK_CONTROL,	0,	0x1d,	"Ctrl" },
	{ VK_MENU,	0,	0x38,	"Alt" },
	{ VK_CAPITAL,	0,	0x3a,	"Caps Lock" },
	{ VK_ESCAPE,	0x1b,	0x1,	"Esc" },
	{ VK_SPACE,	0x20,	0x39,	"Space" },
	{ VK_PRIOR,	0,	0x49,	"Page Up" },
	{ VK_NEXT,	0,	0x51,	"Page Down" },
	{ VK_END,	0,	0x4f,	"End" },
	{ VK_HOME,	0,	0x47,	"Home" },
	{ VK_LEFT,	0,	0x4b,	"Left Arrow" },
	{ VK_UP,	0,	0x48,	"Up Arrow" },
	{ VK_RIGHT,	0,	0x4d,	"Right Arrow" },
	{ VK_DOWN,	0,	0x50,	"Down Arrow" },
	{ VK_INSERT,	0,	0x52,	"Ins" },
	{ VK_DELETE,	0,	0x53,	"Del" },
	{ VK_0,		0x30,	0xb,	"0" },
	{ VK_1,		0x31,	0x2,	"1" },
	{ VK_2,		0x32,	0x3,	"2" },
	{ VK_3,		0x33,	0x4,	"3" },
	{ VK_4,		0x34,	0x5,	"4" },
	{ VK_5,		0x35,	0x6,	"5" },
	{ VK_6,		0x36,	0x7,	"6" },
	{ VK_7,		0x37,	0x8,	"7" },
	{ VK_8,		0x38,	0x9,	"8" },
	{ VK_9,		0x39,	0xa,	"9" },
	{ VK_A,		0x41,	0x1e,	"A" },
	{ VK_B,		0x42,	0x30,	"B" },
	{ VK_C,		0x43,	0x2e,	"C" },
	{ VK_D,		0x44,	0x20,	"D" },
	{ VK_E,		0x45,	0x12,	"E" },
	{ VK_F,		0x46,	0x21,	"F" },
	{ VK_G,		0x47,	0x22,	"G" },
	{ VK_H,		0x48,	0x23,	"H" },
	{ VK_I,		0x49,	0x17,	"I" },
	{ VK_J,		0x4a,	0x24,	"J" },
	{ VK_K,		0x4b,	0x25,	"K" },
	{ VK_L,		0x4c,	0x26,	"L" },
	{ VK_M,		0x4d,	0x32,	"M" },
	{ VK_N,		0x4e,	0x31,	"N" },
	{ VK_O,		0x4f,	0x18,	"O" },
	{ VK_P,		0x50,	0x19,	"P" },
	{ VK_Q,		0x51,	0x10,	"Q" },
	{ VK_R,		0x52,	0x13,	"R" },
	{ VK_S,		0x53,	0x1f,	"S" },
	{ VK_T,		0x54,	0x14,	"T" },
	{ VK_U,		0x55,	0x16,	"U" },
	{ VK_V,		0x56,	0x2f,	"V" },
	{ VK_W,		0x57,	0x11,	"W" },
	{ VK_X,		0x58,	0x2d,	"X" },
	{ VK_Y,		0x59,	0x15,	"Y" },
	{ VK_Z,		0x5a,	0x2c,	"Z" },
	{ VK_NUMPAD0,	0,	0x52,	"Num 0" },
	{ VK_NUMPAD1,	0,	0x4f,	"Num 1" },
	{ VK_NUMPAD2,	0,	0x50,	"Num 2" },
	{ VK_NUMPAD3,	0,	0x51,	"Num 3" },
	{ VK_NUMPAD4,	0,	0x4b,	"Num 4" },
	{ VK_NUMPAD5,	0,	0x4c,	"Num 5" },
	{ VK_NUMPAD6,	0,	0x4d,	"Num 6" },
	{ VK_NUMPAD7,	0,	0x47,	"Num 7" },
	{ VK_NUMPAD8,	0,	0x48,	"Num 8" },
	{ VK_NUMPAD9,	0,	0x49,	"Num 9" },
	{ VK_MULTIPLY,	0x2a,	0x37,	"Num *" },
	{ VK_ADD,	0x2b,	0x4e,	"Num +" },
	{ VK_SEPARATOR,	0,	0,	"" },
	{ VK_SUBTRACT,	0x2d,	0x4a,	"Num -" },
	{ VK_DECIMAL,	0x2e,	0x53,	"Num Del" },
	{ VK_DIVIDE,	0x2f,	0,	"Num /" },
	{ VK_F1,	0,	0x3b,	"F1" },
	{ VK_F2,	0,	0x3c,	"F2" },
	{ VK_F3,	0,	0x3d,	"F3" },
	{ VK_F4,	0,	0x3e,	"F4" },
	{ VK_F5,	0,	0x3f,	"F5" },
	{ VK_F6,	0,	0x40,	"F6" },
	{ VK_F7,	0,	0x41,	"F7" },
	{ VK_F8,	0,	0x42,	"F8" },
	{ VK_F9,	0,	0x43,	"F9" },
	{ VK_F10,	0,	0x44,	"F10" },
	{ VK_F11,	0,	0x57,	"F11" },
	{ VK_F12,	0,	0x58,	"F12" },
	{ VK_NUMLOCK,	0,	0x45,	"Num Lock" },
	{ VK_SCROLL,	0,	0x46,	"Scroll Lock" },
	/* Allowable ranges for OEM-specific virtual-key codes */
/*	-		    0xBA-0xC0		OEM specific */
	{ 0xba,		0x3b,	0x27,	";" },
	{ 0xbb,		0x3d,	0xd,	"=" },
	{ 0xbc,		0x2c,	0x33,	"," },
	{ 0xbd,		0x2d,	0xc,	"-" },
	{ 0xbe,		0x2e,	0x34,	"." },
	{ 0xbf,		0x2f,	0x35,	"/" },
	{ 0xc0,		0x60,	0x29,	"`" },
/*	-		    0xDB-0xE4		OEM specific */
	{ 0xdb,		0x5b,	0x1a,	"[" },
	{ 0xdc,		0x5c,	0x2b,	"\\" },
	{ 0xdd,		0x5d,	0x1b,	"]" },
	{ 0xde,		0x27,	0x28,	"'" },
	{ 0xe2,		0x5c,	0x56,	"\\" },
/*	-		    0xE6		OEM specific */
/*	-		    0xE9-0xF5		OEM specific */
};

#define KeyTableSize	sizeof(KeyTable) / sizeof(struct KeyTableEntry)

int ToAscii(WORD wVirtKey, WORD wScanCode, LPSTR lpKeyState, 
	LPVOID lpChar, WORD wFlags) 
{
	char shift = lpKeyState[VK_SHIFT] < 0;
	int i;

    	dprintf_keyboard(stddeb, "ToAscii (%x,%x) -> ", wVirtKey, wScanCode);

	/* FIXME: codepage is broken */

	*(BYTE*)lpChar = 0;
	switch (wVirtKey)
	  {
#define vkcase2(k1,k2,val) case val : *(BYTE*)lpChar = shift ? k2 : k1; break;
#define vkcase(k, val) vkcase2(val, k, val)
	  WINE_VKEY_MAPPINGS
#undef vkcase
#undef vkcase2
	  default :
	    for (i = 0 ; ; i++) 
	      {
	      if (i == KeyTableSize)
		{
		  dprintf_keyboard(stddeb, "0\n");
		  return 0;
		}
	      if (KeyTable[i].virtualkey == wVirtKey)  
	        {
		  if (!isprint(KeyTable[i].ASCII) && !isspace(KeyTable[i].ASCII))
		    {
		      dprintf_keyboard(stddeb, "0\n");
		      return 0;
		    }
		  dprintf_keyboard(stddeb,"\"%s\" ", KeyTable[i].name);

		  *(BYTE*)lpChar = KeyTable[i].ASCII;
		  *(((BYTE*)lpChar) + 1) = 0;

		  if (isalpha(*(BYTE*)lpChar))
		    if( (lpKeyState[VK_CAPITAL]<0 && !lpKeyState[VK_SHIFT]) ||
			(!lpKeyState[VK_CAPITAL] && lpKeyState[VK_SHIFT]<0) )
			*(BYTE*)lpChar = toupper( *(BYTE*)lpChar );
		    else
			*(BYTE*)lpChar = tolower( *(BYTE*)lpChar );
		  break;
	        }
	     }
	  }
	if (lpKeyState[VK_CONTROL] < 0)
	  *(BYTE*)lpChar = *(BYTE*)lpChar & 0x1f;
	dprintf_keyboard(stddeb, "1 (%x)\n", *(BYTE*)lpChar);
	return 1;
}

DWORD OemKeyScan(WORD wOemChar)
{
    dprintf_keyboard(stddeb,"*OemKeyScan (%d)\n",wOemChar);

	return wOemChar;
}

/* VkKeyScan translates an ANSI character to a virtual-key and shift code
 * for the current keyboard.
 * FIXME high-order byte should yield :
 *	0	Unshifted
 *	1	Shift
 *	2	Ctrl
 *	3-5	Shift-key combinations that are not used for characters
 *	6	Ctrl-Alt
 *	7	Ctrl-Alt-Shift
 *	I.e. :	Shift = 1, Ctrl = 2, Alt = 4.
 */

WORD VkKeyScan(WORD cChar)
{
	int i;
	
    	dprintf_keyboard(stddeb,"VkKeyScan (%d)\n",cChar);
	
	for (i = 0 ; i != KeyTableSize ; i++) 
		if (KeyTable[i].ASCII == cChar)
			return KeyTable[i].virtualkey;

	return -1;
}

int GetKeyboardType(int nTypeFlag)
{
  dprintf_keyboard(stddeb,"GetKeyboardType(%d)\n",nTypeFlag);
  switch(nTypeFlag)
    {
    case 0:      /* Keyboard type */
      return 4;    /* AT-101 */
      break;
    case 1:      /* Keyboard Subtype */
      return 0;    /* There are no defined subtypes */
      break;
    case 2:      /* Number of F-keys */
      return 12;   /* We're doing an 101 for now, so return 12 F-keys */
      break;
    default:     
      fprintf(stderr, "Unknown type on GetKeyboardType\n");
      return 0;    /* The book says 0 here, so 0 */
    }
}

/* MapVirtualKey translates keycodes from one format to another. */

WORD MapVirtualKey(WORD wCode, WORD wMapType)
{
	int i;
	
	switch(wMapType) {
		case 0:	/* vkey-code to scan-code */
			for (i = 0 ; i != KeyTableSize ; i++) 
				if (KeyTable[i].virtualkey == wCode) 
					return KeyTable[i].scancode;
			return 0;

		case 1: /* scan-code to vkey-code */
			for (i = 0 ; i != KeyTableSize ; i++) 
				if (KeyTable[i].scancode == wCode) 
					return KeyTable[i].virtualkey;
			return 0;

		case 2: /* vkey-code to unshifted ANSI code */
			/* FIXME : what does unshifted mean ? 'a' or 'A' ? */
			for (i = 0 ; i != KeyTableSize ; i++) 
				if (KeyTable[i].virtualkey == wCode) 
					return KeyTable[i].ASCII;
			return 0;

		default: /* reserved */
			fprintf(stderr, "MapVirtualKey: unknown wMapType %d !\n",
				wMapType);
			return 0;	
	}
	return 0;
}

int GetKbCodePage(void)
{
    	dprintf_keyboard(stddeb,"GetKbCodePage()\n");
	return 850;
}

int GetKeyNameText(LONG lParam, LPSTR lpBuffer, int nSize)
{
	int i;
	
    	dprintf_keyboard(stddeb,"GetKeyNameText(%ld,<ptr>,%d)\n",lParam,nSize);

	lParam >>= 16;
	lParam &= 0xff;

	for (i = 0 ; i != KeyTableSize ; i++) 
		if (KeyTable[i].scancode == lParam)  {
			lstrcpyn32A( lpBuffer, KeyTable[i].name, nSize );
			return strlen(lpBuffer);
		}

	*lpBuffer = 0;
	return 0;
}
