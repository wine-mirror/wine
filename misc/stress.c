#include <stdio.h>
#include <limits.h>
#include "windows.h"
#include "stress.h"

#define STRESS_DEBUG

int AllocDiskSpace(long lLeft, UINT uDrive)
{
#ifdef STRESS_DEBUG
	fprintf(stderr, "stress.dll: AllocDiskSpace %d, %ld\n", uDrive, lLeft);
#endif

	return 1;
}

int AllocFileHandles(int Left)
{
#ifdef STRESS_DEBUG
	fprintf(stderr, "stress.dll: AllocFileHandles %d\n", Left);
#endif

	if (Left < 0)
		return -1;
	else
		return 1;
}

BOOL AllocGDIMem(UINT uLeft)
{
#ifdef STRESS_DEBUG
	fprintf(stderr, "stress.dll: AllocGDIMem %d\n", uLeft);
#endif

	return 1;
}

BOOL AllocMem(DWORD dwLeft)
{
#ifdef STRESS_DEBUG
	fprintf(stderr, "stress.dll: AllocMem %ld\n", dwLeft);
#endif

	return 1;
}

BOOL AllocUserMem(UINT uContig)
{
#ifdef STRESS_DEBUG
	fprintf(stderr, "stress.dll: AllocUserMem %d\n", uContig);
#endif

	return 1;
}

void FreeAllMem(void)
{
#ifdef STRESS_DEBUG
	fprintf(stderr, "stress.dll: FreeAllMem\n");
#endif
}

void FreeAllGDIMem(void)
{
#ifdef STRESS_DEBUG
	fprintf(stderr, "stress.dll: FreeAllGDIMem\n");
#endif
}

void FreeAllUserMem(void)
{
#ifdef STRESS_DEBUG
	fprintf(stderr, "stress.dll: FreeAllUserMem\n");
#endif
}

void GetFreeAllUserMem(void)
{
#ifdef STRESS_DEBUG
	fprintf(stderr, "stress.dll: GetFreeAllUserMem\n");
#endif
}

int GetFreeFileHandles(void)
{
#ifdef STRESS_DEBUG
	fprintf(stderr, "stress.dll: GetFreeFileHandles\n");
#endif

#ifndef OPEN_MAX
	return _POSIX_OPEN_MAX;
#else
	return OPEN_MAX;
#endif
}

void UnAllocDiskSpace(UINT drive)
{
#ifdef STRESS_DEBUG
	fprintf(stderr, "stress.dll: UnAllocDiskSpace %d\n", drive);
#endif
}

void UnAllocFileHandles(void)
{
#ifdef STRESS_DEBUG
	fprintf(stderr, "stress.dll: GetFreeAllUserMem\n");
#endif
}
