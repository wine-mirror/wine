#include <limits.h>
#include "stress.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(stress);

/***********************************************************************
 *		AllocDiskSpace
 */
INT16 WINAPI AllocDiskSpace(LONG lLeft, UINT16 uDrive)
{
	FIXME("(%d, %ld) - stub\n", 
		       uDrive, lLeft);

	return 1;
}

/***********************************************************************
 *		AllocFileHandles
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
 *		AllocGDIMem
 */
BOOL16 WINAPI AllocGDIMem(UINT16 uLeft)
{
	TRACE("(%d) - stub\n", uLeft);

	return 1;
}

/***********************************************************************
 *		AllocMem
 */
BOOL16 WINAPI AllocMem(DWORD dwLeft)
{
	FIXME("(%ld) - stub\n", dwLeft);

	return 1;
}

/***********************************************************************
 *		AllocUserMem
 */
BOOL16 WINAPI AllocUserMem(UINT16 uContig)
{
	TRACE("AllocUserMem %d\n", uContig);

	return 1;
}

/***********************************************************************
 *		FreeAllMem
 */
void WINAPI FreeAllMem(void)
{
	TRACE("FreeAllMem\n");
}

/***********************************************************************
 *		FreeAllGDIMem
 */
void WINAPI FreeAllGDIMem(void)
{
	TRACE("FreeAllGDIMem\n");
}

/***********************************************************************
 *		FreeAllUserMem
 */
void WINAPI FreeAllUserMem(void)
{
	TRACE("FreeAllUserMem\n");
}

/***********************************************************************
 *
 */
void WINAPI GetFreeAllUserMem(void)
{
       TRACE("GetFreeAllUserMem\n");
}

/***********************************************************************
 *		GetFreeFileHandles
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
 *		UnAllocDiskSpace
 */
void WINAPI UnAllocDiskSpace(UINT16 drive)
{
	TRACE("UnAllocDiskSpace %d\n", drive);
}

/***********************************************************************
 *		UnAllocFileHandles
 */
void WINAPI UnAllocFileHandles(void)
{
	TRACE("GetFreeAllUserMem\n");
}
