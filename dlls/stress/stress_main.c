#include <limits.h>
#include "stress.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(stress)


int WINAPI AllocDiskSpace(long lLeft, UINT16 uDrive)
{
	FIXME("(%d, %ld) - stub\n", 
		       uDrive, lLeft);

	return 1;
}

int WINAPI AllocFileHandles(int Left)
{
	TRACE("(%d) - stub\n", Left);

	if (Left < 0)
		return -1;
	else
		return 1;
}

BOOL16 WINAPI AllocGDIMem(UINT16 uLeft)
{
	TRACE("(%d) - stub\n", uLeft);

	return 1;
}

BOOL16 WINAPI AllocMem(DWORD dwLeft)
{
	FIXME("(%ld) - stub\n", dwLeft);

	return 1;
}

BOOL16 WINAPI AllocUserMem(UINT16 uContig)
{
	TRACE("AllocUserMem %d\n", uContig);

	return 1;
}

void WINAPI FreeAllMem(void)
{
	TRACE("FreeAllMem\n");
}

void WINAPI FreeAllGDIMem(void)
{
	TRACE("FreeAllGDIMem\n");
}

void WINAPI FreeAllUserMem(void)
{
	TRACE("FreeAllUserMem\n");
}

void WINAPI GetFreeAllUserMem(void)
{
       TRACE("GetFreeAllUserMem\n");
}

int WINAPI GetFreeFileHandles(void)
{
	TRACE("GetFreeFileHandles\n");

#ifndef OPEN_MAX
	return _POSIX_OPEN_MAX;
#else
	return OPEN_MAX;
#endif
}

void WINAPI UnAllocDiskSpace(UINT16 drive)
{
	TRACE("UnAllocDiskSpace %d\n", drive);
}

void WINAPI UnAllocFileHandles(void)
{
	TRACE("GetFreeAllUserMem\n");
}
