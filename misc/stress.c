#include <stdio.h>
#include <limits.h>
#include "windows.h"
#include "stress.h"
#include "debug.h"


int WINAPI AllocDiskSpace(long lLeft, UINT16 uDrive)
{
	FIXME(stress, "(%d, %ld) - stub\n", 
		       uDrive, lLeft);

	return 1;
}

int WINAPI AllocFileHandles(int Left)
{
	TRACE(stress, "(%d) - stub\n", Left);

	if (Left < 0)
		return -1;
	else
		return 1;
}

BOOL16 WINAPI AllocGDIMem(UINT16 uLeft)
{
	TRACE(stress, "(%d) - stub\n", uLeft);

	return 1;
}

BOOL16 WINAPI AllocMem(DWORD dwLeft)
{
	FIXME(stress, "(%ld) - stub\n", dwLeft);

	return 1;
}

BOOL16 WINAPI AllocUserMem(UINT16 uContig)
{
	TRACE(stress, "AllocUserMem %d\n", uContig);

	return 1;
}

void WINAPI FreeAllMem(void)
{
	TRACE(stress, "FreeAllMem\n");
}

void WINAPI FreeAllGDIMem(void)
{
	TRACE(stress, "FreeAllGDIMem\n");
}

void WINAPI FreeAllUserMem(void)
{
	TRACE(stress, "FreeAllUserMem\n");
}

void WINAPI GetFreeAllUserMem(void)
{
       TRACE(stress, "GetFreeAllUserMem\n");
}

int WINAPI GetFreeFileHandles(void)
{
	TRACE(stress, "GetFreeFileHandles\n");

#ifndef OPEN_MAX
	return _POSIX_OPEN_MAX;
#else
	return OPEN_MAX;
#endif
}

void WINAPI UnAllocDiskSpace(UINT16 drive)
{
	TRACE(stress, "UnAllocDiskSpace %d\n", drive);
}

void WINAPI UnAllocFileHandles(void)
{
	TRACE(stress, "GetFreeAllUserMem\n");
}
