#include <limits.h>
#include "windef.h"
#include "wine/windef16.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(stress);

/***********************************************************************
 *		AllocDiskSpace (STRESS.10)
 */
INT16 WINAPI AllocDiskSpace(LONG lLeft, UINT16 uDrive)
{
	FIXME("(%d, %ld) - stub\n", 
		       uDrive, lLeft);

	return 1;
}

/***********************************************************************
 *		AllocFileHandles (STRESS.6)
 */
INT16 WINAPI AllocFileHandles(INT16 Left)
{
	TRACE("(%d) - stub\n", Left);

	if (Left < 0)
		return -1;
	else
		return 1;
}

/***********************************************************************
 *		AllocGDIMem (STRESS.14)
 */
BOOL16 WINAPI AllocGDIMem(UINT16 uLeft)
{
	TRACE("(%d) - stub\n", uLeft);

	return 1;
}

/***********************************************************************
 *		AllocMem (STRESS.2)
 */
BOOL16 WINAPI AllocMem(DWORD dwLeft)
{
	FIXME("(%ld) - stub\n", dwLeft);

	return 1;
}

/***********************************************************************
 *		AllocUserMem (STRESS.12)
 */
BOOL16 WINAPI AllocUserMem(UINT16 uContig)
{
	TRACE("AllocUserMem %d\n", uContig);

	return 1;
}

/***********************************************************************
 *		FreeAllMem (STRESS.3)
 */
void WINAPI FreeAllMem(void)
{
	TRACE("FreeAllMem\n");
}

/***********************************************************************
 *		FreeAllGDIMem (STRESS.15)
 */
void WINAPI FreeAllGDIMem(void)
{
	TRACE("FreeAllGDIMem\n");
}

/***********************************************************************
 *		FreeAllUserMem (STRESS.13)
 */
void WINAPI FreeAllUserMem(void)
{
	TRACE("FreeAllUserMem\n");
}

/***********************************************************************
 *		GetFreeFileHandles (STRESS.8)
 */
INT16 WINAPI GetFreeFileHandles(void)
{
	TRACE("GetFreeFileHandles\n");

#ifndef OPEN_MAX
	return _POSIX_OPEN_MAX;
#else
	return OPEN_MAX;
#endif
}

/***********************************************************************
 *		UnAllocDiskSpace (STRESS.11)
 */
void WINAPI UnAllocDiskSpace(UINT16 drive)
{
	TRACE("UnAllocDiskSpace %d\n", drive);
}

/***********************************************************************
 *		UnAllocFileHandles (STRESS.7)
 */
void WINAPI UnAllocFileHandles(void)
{
	TRACE("GetFreeAllUserMem\n");
}
