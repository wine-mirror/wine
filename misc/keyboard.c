static char RCSId[] = "$Id: keyboard.c,v 1.2 1993/09/13 18:52:02 scott Exp $";
static char Copyright[] = "Copyright  Scott A. Laird, 1993";

#include <stdlib.h>
#include <stdio.h>
#include "prototypes.h"
#include "windows.h"

int ToAscii(WORD wVirtKey, WORD wScanCode, LPSTR lpKeyState, 
	    LPVOID lpChar, WORD wFlags) 
{
  printf("ToAscii (%d,%d)\n",wVirtKey, wScanCode);
  return -1;
}

#ifdef  BOGUS_ANSI_OEM

int AnsiToOem(LPSTR lpAnsiStr, LPSTR lpOemStr)
{
  printf("AnsiToOem (%s)\n",lpAnsiStr);
  strcpy(lpOemStr,lpAnsiStr);  /* Probably not the right thing to do, but... */
  return -1;
}

BOOL OemToAnsi(LPSTR lpOemStr, LPSTR lpAnsiStr)
{
  printf("OemToAnsi (%s)\n",lpOemStr);
  strcpy(lpAnsiStr,lpOemStr);  /* Probably not the right thing to do, but... */
  return -1;
}

#endif

DWORD OemKeyScan(WORD wOemChar)
{
  printf("*OemKeyScan (%d)\n",wOemChar);
  return 0;
}

/* VkKeyScan translates an ANSI character to a virtual-key and shift code
 * for the current keyboard.  For now we return -1, which is fail. */

WORD VkKeyScan(WORD cChar)
{
  printf("VkKeyScan (%d)\n",cChar);
  return -1;
}

int GetKeyboardType(int nTypeFlag)
{
  printf("GetKeyboardType(%d)\n",nTypeFlag);
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
      printf("  Unknown type on GetKeyboardType\n");
      return 0;    /* The book says 0 here, so 0 */
    }
}

/* MapVirtualKey translates keycodes from one format to another.  This
 *  is a total punt.  */

WORD MapVirtualKey(WORD wCode, WORD wMapType)
{
  printf("*MapVirtualKey(%d,%d)\n",wCode,wMapType);
  return 0;
}

int GetKbCodePage(void)
{
  printf("GetKbCodePage()\n");
  return 437; /* US -- probably should be 850 from time to time */
}

/* This should distinguish key names.  Maybe later */

int GetKeyNameText(LONG lParam, LPSTR lpBuffer, int nSize)
{
  printf("GetKeyNameText(%d,<ptr>, %d)\n",lParam,nSize);
  lpBuffer[0]=0;  /* This key has no name */
  return 0;
}

#ifdef  BOGUS_ANSI_OEM

void AnsiToOemBuff(LPSTR lpAnsiStr, LPSTR lpOemStr, int nLength)
{
  printf("AnsiToOemBuff(%s,<ptr>,%d)\n",lpAnsiStr,nLength);
  strncpy(lpOemStr,lpAnsiStr,nLength);  /* should translate... */
}

void OemToAnsiBuff(LPSTR lpOemStr, LPSTR lpAnsiStr, int nLength)
{
  printf("OemToAnsiBuff(%s,<ptr>,%d)\n",lpOemStr,nLength);
  strncpy(lpAnsiStr,lpOemStr,nLength);  /* should translate... */
}

#endif











