#ifndef _W32SKRNL_H
#define _W32SKRNL_H
#include "wintypes.h"
HTASK16     WINAPI GetCurrentTask(void);
LPSTR WINAPI GetWin32sDirectory(void);
DWORD WINAPI RtlNtStatusToDosError(DWORD error);
#endif /* _W32SKRNL_H */
