/* 
 * Driver Environment functions
 *
 * Note: This has NOTHING to do with the task/process environment!
 *
 * Copyright 1997 Marcus Meissner
 */
#include <stdio.h>
#include "windows.h"
#include "gdi.h"
#include "debug.h"

/***********************************************************************
 *           GetEnvironment   (GDI.134)
 */
INT16 WINAPI GetEnvironment(LPCSTR lpPortName, LPDEVMODE16 lpdev, UINT16 nMaxSiz)
{
    fprintf(stddeb, "GetEnvironment('%s','%p',%d),\n",
		lpPortName, lpdev, nMaxSiz);
    return 0;
}

/***********************************************************************
 *          SetEnvironment   (GDI.132)
 */
INT16 WINAPI SetEnvironment(LPCSTR lpPortName, LPDEVMODE16 lpdev, UINT16 nCount)
{
    fprintf(stddeb, "SetEnvironment('%s', '%p', %d) !\n", 
		lpPortName, lpdev, nCount);
    fprintf(stderr,
    	"\tdevmode:\n"
    	"\tname = %s\n"
	"\tdmSpecVersion = %d\n"
	"\tdmDriverVersion = %d\n"
	"\tdmSize = %d\n"
	"\tdmDriverExtra = %d\n"
	"\tdmFields = %ld\n"
	"\tdmOrientation = %d\n"
	"\tdmPaperSize = %d\n"
	"\tdmPaperLength = %d\n"
	"\tdmPaperWidth = %d\n"
	"\tdmScale = %d\n"
	"\tdmCopies = %d\n"
	"\tdmDefaultSource = %d\n"
	"\tdmPrintQuality = %d\n"
	"\tdmColor = %d\n"
	"\tdmDuplex = %d\n"
	"\tdmYResolution = %d\n"
	"\tdmTTOption = %d\n"
	"\tdmCollate = %d\n"
	"\tdmFBitsPerPel = %d\n"
	"\tdmPelsWidth = %ld\n"
	"\tdmPelsHeight = %ld\n"
	"\tdmDisplayFlags = %ld\n"
	"\tdmDisplayFrequency = %ld\n",
	
    	lpdev->dmDeviceName,
    	lpdev->dmSpecVersion,
    	lpdev->dmDriverVersion,
    	lpdev->dmSize,
    	lpdev->dmDriverExtra,
    	lpdev->dmFields,
    	lpdev->dmOrientation,
    	lpdev->dmPaperSize,
    	lpdev->dmPaperLength,
    	lpdev->dmPaperWidth,
    	lpdev->dmScale,
    	lpdev->dmCopies,
    	lpdev->dmDefaultSource,
    	lpdev->dmPrintQuality,
    	lpdev->dmColor,
    	lpdev->dmDuplex,
    	lpdev->dmYResolution,
    	lpdev->dmTTOption,
    	lpdev->dmCollate,
    	lpdev->dmBitsPerPel,
    	lpdev->dmPelsWidth,
    	lpdev->dmPelsHeight,
    	lpdev->dmDisplayFlags,
    	lpdev->dmDisplayFrequency
   );
    return 0;
}
