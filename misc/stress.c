#include <stdio.h>
#include <limits.h>
#include "windows.h"
#include "stress.h"
#include "stddebug.h"
/* #define DEBUG_STRESS */
/* #undef  DEBUG_STRESS */
#include "debug.h"


int WINAPI AllocDiskSpace(long lLeft, UINT16 uDrive)
{
	dprintf_stress(stdnimp, "stress.dll: AllocDiskSpace %d, %ld\n", uDrive, lLeft);

	return 1;
}

int WINAPI AllocFileHandles(int Left)
{
	dprintf_stress(stddeb, "stress.dll: AllocFileHandles %d\n", Left);

	if (Left < 0)
		return -1;
	else
		return 1;
}

BOOL16 WINAPI AllocGDIMem(UINT16 uLeft)
{
	dprintf_stress(stddeb, "stress.dll: AllocGDIMem %d\n", uLeft);

	return 1;
}

BOOL16 WINAPI AllocMem(DWORD dwLeft)
{
	dprintf_stress(stdnimp, "stress.dll: AllocMem %ld\n", dwLeft);

	return 1;
}

BOOL16 WINAPI AllocUserMem(UINT16 uContig)
{
	dprintf_stress(stddeb, "stress.dll: AllocUserMem %d\n", uContig);

	return 1;
}

void WINAPI FreeAllMem(void)
{
	dprintf_stress(stddeb, "stress.dll: FreeAllMem\n");
}

void WINAPI FreeAllGDIMem(void)
{
	dprintf_stress(stddeb, "stress.dll: FreeAllGDIMem\n");
}

void WINAPI FreeAllUserMem(void)
{
	dprintf_stress(stddeb, "stress.dll: FreeAllUserMem\n");
}

void WINAPI GetFreeAllUserMem(void)
{
       dprintf_stress(stddeb, "stress.dll: GetFreeAllUserMem\n");
}

int WINAPI GetFreeFileHandles(void)
{
	dprintf_stress(stddeb, "stress.dll: GetFreeFileHandles\n");

#ifndef OPEN_MAX
	return _POSIX_OPEN_MAX;
#else
	return OPEN_MAX;
#endif
}

void WINAPI UnAllocDiskSpace(UINT16 drive)
{
	dprintf_stress(stddeb, "stress.dll: UnAllocDiskSpace %d\n", drive);
}

void WINAPI UnAllocFileHandles(void)
{
	dprintf_stress(stddeb, "stress.dll: GetFreeAllUserMem\n");
}
