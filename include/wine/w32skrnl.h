#ifndef __WINE_WINE_W32SKRNL_H
#define __WINE_WINE_W32SKRNL_H

#include "windef.h"

LPSTR WINAPI GetWin32sDirectory(void);
DWORD WINAPI RtlNtStatusToDosError(DWORD error);

#endif /* __WINE_WINE_W32SKRNL_H */
