#include <stdio.h>
#include <limits.h>
#include "windows.h"
#include "stress.h"
#include "debug.h"


int WINAPI AllocDiskSpace(long lLeft, UINT16 uDrive)
{
	dprintf_fixme(stress, "stress.dll: AllocDiskSpace(%d, %ld) - stub\n", 
		       uDrive, lLeft);

	return 1;
}

int WINAPI AllocFileHandles(int Left)
{
	dprintf_info(stress, "stress.dll: AllocFileHandles(%d) - stub\n", Left);

	if (Left < 0)
		return -1;
	else
		return 1;
}

BOOL16 WINAPI AllocGDIMem(UINT16 uLeft)
{
	dprintf_info(stress, "stress.dll: AllocGDIMem(%d) - stub\n", uLeft);

	return 1;
}

BOOL16 WINAPI AllocMem(DWORD dwLeft)
{
	dprintf_fixme(stress, "stress.dll: AllocMem(%ld) - stub\n", dwLeft);

	return 1;
}

BOOL16 WINAPI AllocUserMem(UINT16 uContig)
{
	dprintf_info(stress, "stress.dll: AllocUserMem %d\n", uContig);

	return 1;
}

void WINAPI FreeAllMem(void)
{
	dprintf_info(stress, "stress.dll: FreeAllMem\n");
}

void WINAPI FreeAllGDIMem(void)
{
	dprintf_info(stress, "stress.dll: FreeAllGDIMem\n");
}

void WINAPI FreeAllUserMem(void)
{
	dprintf_info(stress, "stress.dll: FreeAllUserMem\n");
}

void WINAPI GetFreeAllUserMem(void)
{
       dprintf_info(stress, "stress.dll: GetFreeAllUserMem\n");
}

int WINAPI GetFreeFileHandles(void)
{
	dprintf_info(stress, "stress.dll: GetFreeFileHandles\n");

#ifndef OPEN_MAX
	return _POSIX_OPEN_MAX;
#else
	return OPEN_MAX;
#endif
}

void WINAPI UnAllocDiskSpace(UINT16 drive)
{
	dprintf_info(stress, "stress.dll: UnAllocDiskSpace %d\n", drive);
}

void WINAPI UnAllocFileHandles(void)
{
	dprintf_info(stress, "stress.dll: GetFreeAllUserMem\n");
}
