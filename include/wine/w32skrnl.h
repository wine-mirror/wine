#ifndef _W32SKRNL_H
#define _W32SKRNL_H
#include "windef.h"
LPSTR WINAPI GetWin32sDirectory(void);
DWORD WINAPI RtlNtStatusToDosError(DWORD error);
#endif /* _W32SKRNL_H */
