#ifndef __WINE_WINDOWS_H
#define __WINE_WINDOWS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "windef.h"
#include "shell.h"
#include "winreg.h"
#include "winnetwk.h"
#include "winver.h"
#include "lzexpand.h"
#include "shellapi.h"
#include "ole2.h"
#include "winnls.h"
#include "objbase.h"

/* FIXME: remove the following includes which are not in SDK */
#include "ntdll.h"
#include "wine/keyboard16.h"
#include "wine/shell16.h"
#include "wine/w32skrnl.h"
#include "wine/winbase16.h"
#include "wine/winesound.h"
#include "wine/winestring.h"
#include "wine/winuser16.h"


#if 0
  Where does this belong? Nobody uses this stuff anyway.
typedef struct {
	BYTE i;  /* much more .... */
} KANJISTRUCT;
typedef KANJISTRUCT *LPKANJISTRUCT;
typedef KANJISTRUCT *NPKANJISTRUCT;
typedef KANJISTRUCT *PKANJISTRUCT;

BOOL16      WINAPI CheckMenuRadioButton16(HMENU16,UINT16,UINT16,UINT16,BOOL16);
BOOL32      WINAPI CheckMenuRadioButton32(HMENU32,UINT32,UINT32,UINT32,BOOL32);
#define     CheckMenuRadioButton WINELIB_NAME(CheckMenuRadioButton)
WORD        WINAPI WOWHandle16(HANDLE32,WOW_HANDLE_TYPE);
BOOL16      WINAPI GetPrivateProfileStruct16(LPCSTR,LPCSTR,LPVOID,UINT16,LPCSTR);
INT16       WINAPI GetPrivateProfileSection16(LPCSTR,LPSTR,UINT16,LPCSTR);

#endif /* 0 */

#ifdef __cplusplus
}
#endif

#endif  /* __WINE_WINDOWS_H */
