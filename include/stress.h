#ifndef __WINE_STRESS_H
#define __WINE_STRESS_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#define	EDS_WIN		1
#define EDS_CUR		2
#define EDS_TEMP	3

BOOL16	AllocGDIMem(UINT16);
BOOL16	AllocMem(DWORD);
BOOL16	AllocUserMem(UINT16);
int	AllocDiskSpace(long, UINT16);
int	AllocFileHandles(int);
int	GetFreeFileHandles(void);
void	FreeAllGDIMem(void);
void	FreeAllMem(void);
void	FreeAllUserMem(void);
void	UnAllocDiskSpace(UINT16);
void	UnAllocFileHandles(void);

#ifdef __cplusplus
}
#endif

#endif /* __WINE_STRESS_H */
