/*
 * SHFileOperation
 */
#include <string.h>
#include "debugtools.h"
#include "shellapi.h"
#include "shell32_main.h"
#include "winversion.h"

DEFAULT_DEBUG_CHANNEL(shell);

/*************************************************************************
 * SHFileOperationA				[SHELL32.243]
 *
 * NOTES
 *     exported by name
 */
DWORD WINAPI SHFileOperationA (LPSHFILEOPSTRUCTA lpFileOp)   
{
	FIXME("(%p):stub.\n", lpFileOp);
	return 1;
}

/*************************************************************************
 * SHFileOperationW				[SHELL32.244]
 *
 * NOTES
 *     exported by name
 */
DWORD WINAPI SHFileOperationW (LPSHFILEOPSTRUCTW lpFileOp)   
{
	FIXME("(%p):stub.\n", lpFileOp);
	return 1;
}

/*************************************************************************
 * SHFileOperation				[SHELL32.242]
 *
 */
DWORD WINAPI SHFileOperationAW(LPVOID lpFileOp)
{
	if (VERSION_OsIsUnicode())
	  return SHFileOperationW(lpFileOp);
	return SHFileOperationA(lpFileOp);
}

