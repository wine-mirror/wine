#ifndef __WINE_STRESS_H
#define __WINE_STRESS_H

#include "windef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define	EDS_WIN		1
#define EDS_CUR		2
#define EDS_TEMP	3

BOOL16	WINAPI AllocGDIMem(UINT16);
BOOL16	WINAPI AllocMem(DWORD);
BOOL16	WINAPI AllocUserMem(UINT16);
INT16	WINAPI AllocDiskSpace(LONG, UINT16);
INT16	WINAPI AllocFileHandles(INT16);
INT16	WINAPI GetFreeFileHandles(void);
void	WINAPI FreeAllGDIMem(void);
void	WINAPI FreeAllMem(void);
void	WINAPI FreeAllUserMem(void);
void	WINAPI UnAllocDiskSpace(UINT16);
void	WINAPI UnAllocFileHandles(void);

#ifdef __cplusplus
}
#endif

#endif /* __WINE_STRESS_H */
