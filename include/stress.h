#ifndef __STRESS_H
#define __STRESS_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#define	EDS_WIN		1
#define EDS_CUR		2
#define EDS_TEMP	3

BOOL	AllocGDIMem(UINT);
BOOL	AllocMem(DWORD);
BOOL	AllocUserMem(UINT);
int	AllocDiskSpace(long, UINT);
int	AllocFileHandles(int);
int	GetFreeFileHandles(void);
void	FreeAllGDIMem(void);
void	FreeAllMem(void);
void	FreeAllUserMem(void);
void	UnAllocDiskSpace(UINT);
void	UnAllocFileHandles(void);

#ifdef __cplusplus
}
#endif

#endif /* __STRESS_H */
