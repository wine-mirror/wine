/*
static char RCSId[] = "$Id: keyboard.c,v 1.2 1993/09/13 18:52:02 scott Exp $";
static char Copyright[] = "Copyright  Scott A. Laird, Erik Bos  1993, 1994";
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "prototypes.h"
#include "windows.h"
#include "keyboard.h"
#include "stddebug.h"
/* #define DEBUG_KEYBOARD */
#include "debug.h"

int ToAscii(WORD wVirtKey, WORD wScanCode, LPSTR lpKeyState, 
	LPVOID lpChar, WORD wFlags) 
{
	int i;

    	dprintf_keyboard(stddeb,"ToAscii (%d,%d)\n",wVirtKey, wScanCode);

	/* FIXME: this is not sufficient but better than returing -1 */

	for (i = 0 ; i != KeyTableSize ; i++) 
		if (KeyTable[i].virtualkey == wVirtKey)  {
			*(BYTE*)lpChar++ = *KeyTable[i].name;
			*(BYTE*)lpChar = 0;
			return 1;
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
			strncpy(lpBuffer, KeyTable[i].name, nSize);
			return strlen(lpBuffer);
		}

	*lpBuffer = 0;
	return 0;
}
