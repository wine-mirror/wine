/*
static char RCSId[] = "$Id: keyboard.c,v 1.2 1993/09/13 18:52:02 scott Exp $";
static char Copyright[] = "Copyright  Scott A. Laird, Erik Bos  1993, 1994";
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "windows.h"
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
	{ 0x3, 0x3, 0x0, "" },
	{ 0x8, 0x8, 0xe, "Backspace" },
	{ 0x9, 0x9, 0xf, "Tab" },
	{ 0xc, 0x0, 0x4c, "Num 5" },
	{ 0xd, 0xd, 0x1c, "Enter" },
	{ 0x10, 0x0, 0x2a, "Shift" },
	{ 0x11, 0x0, 0x1d, "Ctrl" },
	{ 0x12, 0x0, 0x38, "Alt" },
	{ 0x14, 0x0, 0x3a, "Caps Lock" },
	{ 0x1b, 0x1b, 0x1, "Esc" },
	{ 0x20, 0x20, 0x39, "Space" },
	{ 0x21, 0x0, 0x49, "Num 9" },
	{ 0x22, 0x0, 0x51, "Num 3" },
	{ 0x23, 0x0, 0x4f, "Num 1" },
	{ 0x24, 0x0, 0x47, "Num 7" },
	{ 0x25, 0x0, 0x4b, "Num 4" },
	{ 0x26, 0x0, 0x48, "Num 8" },
	{ 0x27, 0x0, 0x4d, "Num 6" },
	{ 0x28, 0x0, 0x50, "Num 2" },
	{ 0x2d, 0x0, 0x52, "Num 0" },
	{ 0x2e, 0x0, 0x53, "Num Del" },
	{ 0x30, 0x30, 0xb, "0" },
	{ 0x31, 0x31, 0x2, "1" },
	{ 0x32, 0x32, 0x3, "2" },
	{ 0x33, 0x33, 0x4, "3" },
	{ 0x34, 0x34, 0x5, "4" },
	{ 0x35, 0x35, 0x6, "5" },
	{ 0x36, 0x36, 0x7, "6" },
	{ 0x37, 0x37, 0x8, "7" },
	{ 0x38, 0x38, 0x9, "8" },
	{ 0x39, 0x39, 0xa, "9" },
	{ 0x41, 0x41, 0x1e, "A" },
	{ 0x42, 0x42, 0x30, "B" },
	{ 0x43, 0x43, 0x2e, "C" },
	{ 0x44, 0x44, 0x20, "D" },
	{ 0x45, 0x45, 0x12, "E" },
	{ 0x46, 0x46, 0x21, "F" },
	{ 0x47, 0x47, 0x22, "G" },
	{ 0x48, 0x48, 0x23, "H" },
	{ 0x49, 0x49, 0x17, "I" },
	{ 0x4a, 0x4a, 0x24, "J" },
	{ 0x4b, 0x4b, 0x25, "K" },
	{ 0x4c, 0x4c, 0x26, "L" },
	{ 0x4d, 0x4d, 0x32, "M" },
	{ 0x4e, 0x4e, 0x31, "N" },
	{ 0x4f, 0x4f, 0x18, "O" },
	{ 0x50, 0x50, 0x19, "P" },
	{ 0x51, 0x51, 0x10, "Q" },
	{ 0x52, 0x52, 0x13, "R" },
	{ 0x53, 0x53, 0x1f, "S" },
	{ 0x54, 0x54, 0x14, "T" },
	{ 0x55, 0x55, 0x16, "U" },
	{ 0x56, 0x56, 0x2f, "V" },
	{ 0x57, 0x57, 0x11, "W" },
	{ 0x58, 0x58, 0x2d, "X" },
	{ 0x59, 0x59, 0x15, "Y" },
	{ 0x5a, 0x5a, 0x2c, "Z" },
	{ 0x60, 0x0, 0x52, "Num 0" },
	{ 0x61, 0x0, 0x4f, "Num 1" },
	{ 0x62, 0x0, 0x50, "Num 2" },
	{ 0x63, 0x0, 0x51, "Num 3" },
	{ 0x64, 0x0, 0x4b, "Num 4" },
	{ 0x65, 0x0, 0x4c, "Num 5" },
	{ 0x66, 0x0, 0x4d, "Num 6" },
	{ 0x67, 0x0, 0x47, "Num 7" },
	{ 0x68, 0x0, 0x48, "Num 8" },
	{ 0x69, 0x0, 0x49, "Num 9" },
	{ 0x6a, 0x2a, 0x37, "Num *" },
	{ 0x6b, 0x2b, 0x4e, "Num +" },
	{ 0x6c, 0x0, 0x0, "" },
	{ 0x6d, 0x2d, 0x4a, "Num -" },
	{ 0x6e, 0x2e, 0x53, "Num Del" },
	{ 0x6f, 0x2f, 0x0, "" },
	{ 0x70, 0x0, 0x3b, "F1" },
	{ 0x71, 0x0, 0x3c, "F2" },
	{ 0x72, 0x0, 0x3d, "F3" },
	{ 0x73, 0x0, 0x3e, "F4" },
	{ 0x74, 0x0, 0x3f, "F5" },
	{ 0x75, 0x0, 0x40, "F6" },
	{ 0x76, 0x0, 0x41, "F7" },
	{ 0x77, 0x0, 0x42, "F8" },
	{ 0x78, 0x0, 0x43, "F9" },
	{ 0x79, 0x0, 0x44, "F10" },
	{ 0x7a, 0x0, 0x57, "F11" },
	{ 0x7b, 0x0, 0x58, "F12" },
	{ 0x90, 0x0, 0x45, "Pause" },
	{ 0x91, 0x0, 0x46, "Scroll Lock" },
	{ 0xba, 0x3b, 0x27, ";" },
	{ 0xbb, 0x3d, 0xd, "=" },
	{ 0xbc, 0x2c, 0x33, "," },
	{ 0xbd, 0x2d, 0xc, "-" },
	{ 0xbe, 0x2e, 0x34, "." },
	{ 0xbf, 0x2f, 0x35, "/" },
	{ 0xc0, 0x60, 0x29, "`" },
	{ 0xdb, 0x5b, 0x1a, "[" },
	{ 0xdc, 0x5c, 0x2b, "\\" },
	{ 0xdd, 0x5d, 0x1b, "]" },
	{ 0xde, 0x27, 0x28, "\'" },
	{ 0xe2, 0x5c, 0x56, "\\" },
};

#define KeyTableSize	sizeof(KeyTable) / sizeof(struct KeyTableEntry)

int ToAscii(WORD wVirtKey, WORD wScanCode, LPSTR lpKeyState, 
	LPVOID lpChar, WORD wFlags) 
{
	int i;

    	dprintf_keyboard(stddeb,"ToAscii (%d,%d)\n",wVirtKey, wScanCode);

	/* FIXME: codepage is broken */

	for (i = 0 ; i != KeyTableSize ; i++) 
		if (KeyTable[i].virtualkey == wVirtKey)  
		 {
		   dprintf_keyboard(stddeb,"\t\tchar = %s\n", KeyTable[i].name);
		   if( isprint(KeyTable[i].ASCII) || isspace(KeyTable[i].ASCII) )
		     {
			*(BYTE*)lpChar = KeyTable[i].ASCII;
			*(((BYTE*)lpChar) + 1) = 0;

			if( isalpha( *(BYTE*)lpChar ) )
			  if( (lpKeyState[VK_CAPITAL]<0 && !lpKeyState[VK_SHIFT]) ||
			      (!lpKeyState[VK_CAPITAL] && lpKeyState[VK_SHIFT]<0) )
			      *(BYTE*)lpChar = toupper( *(BYTE*)lpChar );
			  else
			      *(BYTE*)lpChar = tolower( *(BYTE*)lpChar );

			return 1;
		     }
		 }

	*(BYTE*)lpChar = 0;
	return 0;
}

DWORD OemKeyScan(WORD wOemChar)
{
    dprintf_keyboard(stddeb,"*OemKeyScan (%d)\n",wOemChar);

	return wOemChar;
}

/* VkKeyScan translates an ANSI character to a virtual-key and shift code
 * for the current keyboard. */

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
		case 0:
			for (i = 0 ; i != KeyTableSize ; i++) 
				if (KeyTable[i].virtualkey == wCode) 
					return KeyTable[i].scancode;
			return 0;

		case 1:
			for (i = 0 ; i != KeyTableSize ; i++) 
				if (KeyTable[i].scancode == wCode) 
					return KeyTable[i].virtualkey;
			return 0;

		case 2:
			for (i = 0 ; i != KeyTableSize ; i++) 
				if (KeyTable[i].virtualkey == wCode) 
					return KeyTable[i].ASCII;
			return 0;

		default: 
			fprintf(stderr, "MapVirtualKey: unknown wMapType!\n");
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
