/*
 * Implementation of VER.DLL
 *
 * Copyright 1999 Ulrich Weigand
 */

#include "winbase.h"
#include "winver.h"
#include "ldt.h"
#include "ver.h"
#include "debug.h"


/*************************************************************************
 * GetFileResourceSize16                     [VER.2]
 */
DWORD WINAPI GetFileResourceSize16( LPCSTR lpszFileName, 
                                    SEGPTR spszResType, SEGPTR spszResId,
                                    LPDWORD lpdwFileOffset )
{
    LPCSTR lpszResType = HIWORD( spszResType )? PTR_SEG_TO_LIN( spszResType )
                                              : (LPCSTR)spszResType;
    LPCSTR lpszResId = HIWORD( spszResId )? PTR_SEG_TO_LIN( spszResId )
                                          : (LPCSTR)spszResId;

    return GetFileResourceSize32( lpszFileName, 
                                  lpszResType, lpszResId, lpdwFileOffset );
}

/*************************************************************************
 * GetFileResource16                         [VER.3]
 */
DWORD WINAPI GetFileResource16( LPCSTR lpszFileName, 
                                SEGPTR spszResType, SEGPTR spszResId,
                                DWORD dwFileOffset, 
                                DWORD dwResLen, LPVOID lpvData )
{
    LPCSTR lpszResType = HIWORD( spszResType )? PTR_SEG_TO_LIN( spszResType )
                                              : (LPCSTR)spszResType;
    LPCSTR lpszResId = HIWORD( spszResId )? PTR_SEG_TO_LIN( spszResId )
                                          : (LPCSTR)spszResId;

    return GetFileResource32( lpszFileName, lpszResType, lpszResId,
                              dwFileOffset, dwResLen, lpvData );
}

/*************************************************************************
 * GetFileVersionInfoSize16                  [VER.6]
 */
DWORD WINAPI GetFileVersionInfoSize16( LPCSTR lpszFileName, LPDWORD lpdwHandle )
{
    TRACE( ver, "(%s, %p)\n", debugstr_a(lpszFileName), lpdwHandle );
    return GetFileVersionInfoSize32A( lpszFileName, lpdwHandle );
}

/*************************************************************************
 * GetFileVersionInfo16                      [VER.7]
 */
DWORD WINAPI GetFileVersionInfo16( LPCSTR lpszFileName, DWORD handle,
                                   DWORD cbBuf, LPVOID lpvData )
{
    TRACE( ver, "(%s, %08lx, %ld, %p)\n", 
                debugstr_a(lpszFileName), handle, cbBuf, lpvData );

    return GetFileVersionInfo32A( lpszFileName, handle, cbBuf, lpvData );
}

/*************************************************************************
 * VerFindFile16                             [VER.8]
 */
DWORD WINAPI VerFindFile16( UINT16 flags, LPCSTR lpszFilename, 
                            LPCSTR lpszWinDir, LPCSTR lpszAppDir,
                            LPSTR lpszCurDir, UINT16 *lpuCurDirLen,
                            LPSTR lpszDestDir, UINT16 *lpuDestDirLen )
{
    UINT32 curDirLen, destDirLen;
    DWORD retv = VerFindFile32A( flags, lpszFilename, lpszWinDir, lpszAppDir,
                                 lpszCurDir, &curDirLen, lpszDestDir, &destDirLen );

    *lpuCurDirLen = (UINT16)curDirLen;
    *lpuDestDirLen = (UINT16)destDirLen;
    return retv;
}

/*************************************************************************
 * VerInstallFile16                          [VER.9]
 */
DWORD WINAPI VerInstallFile16( UINT16 flags, 
                               LPCSTR lpszSrcFilename, LPCSTR lpszDestFilename, 
                               LPCSTR lpszSrcDir, LPCSTR lpszDestDir, LPCSTR lpszCurDir,
                               LPSTR lpszTmpFile, UINT16 *lpwTmpFileLen )
{
    UINT32 filelen;
    DWORD retv = VerInstallFile32A( flags, lpszSrcFilename, lpszDestFilename, 
                                    lpszSrcDir, lpszDestDir, lpszCurDir, 
                                    lpszTmpFile, &filelen);

    *lpwTmpFileLen = (UINT16)filelen;
    return retv;
}

/*************************************************************************
 * VerLanguageName16                        [VER.10]
 */
DWORD WINAPI VerLanguageName16( UINT16 uLang, LPSTR lpszLang, UINT16 cbLang )
{
    return VerLanguageName32A( uLang, lpszLang, cbLang );
}

/*************************************************************************
 * VerQueryValue16                          [VER.11]
 */
DWORD WINAPI VerQueryValue16( SEGPTR spvBlock, LPCSTR lpszSubBlock, 
                              SEGPTR *lpspBuffer, UINT16 *lpcb )
{
    LPVOID lpvBlock = PTR_SEG_TO_LIN( spvBlock );
    LPVOID buffer = lpvBlock;
    UINT32 buflen;
    DWORD retv;

    TRACE( ver, "(%p, %s, %p, %p)\n", 
                lpvBlock, debugstr_a(lpszSubBlock), lpspBuffer, lpcb );

    retv = VerQueryValue32A( lpvBlock, lpszSubBlock, &buffer, &buflen );
    if ( !retv ) return FALSE;

    if ( OFFSETOF( spvBlock ) + (buffer - lpvBlock) >= 0x10000 )
    {
        FIXME( ver, "offset %08X too large relative to %04X:%04X\n",
               buffer - lpvBlock, SELECTOROF( spvBlock ), OFFSETOF( spvBlock ) );
        return FALSE;
    }

    *lpcb = buflen;
    *lpspBuffer = spvBlock + (buffer - lpvBlock);

    return retv;
}

