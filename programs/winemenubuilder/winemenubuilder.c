/*
 * Helper program to build unix menu entries
 *
 * Copyright 1997 Marcus Meissner
 * Copyright 1998 Juergen Schmied
 * Copyright 2003 Mike McCormack for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 *  This program will read a Windows shortcut file using the IShellLink
 * interface, then invoke wineshelllink with the appropriate arguments
 * to create a KDE/Gnome menu entry for the shortcut.
 *
 *  winemenubuilder [ -r ] <shortcut.lnk>
 *
 *  If the -r parameter is passed, and the shortcut cannot be created,
 * this program will add RunOnce entry to invoke itself at the next
 * reboot.  This covers the case when a ShortCut is created before the
 * executable containing its icon.
 *
 */

#include "config.h"
#include "wine/port.h"

#include <ctype.h>
#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>
#include <stdarg.h>

#include <windows.h>
#include <shlobj.h>
#include <objidl.h>
#include <shlguid.h>

#include "wine/debug.h"
#include "wine.xpm"

WINE_DEFAULT_DEBUG_CHANNEL(menubuilder);

/* link file formats */

#include "pshpack1.h"

typedef struct
{
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD wPlanes;
    WORD wBitCount;
    DWORD dwBytesInRes;
    WORD nID;
} GRPICONDIRENTRY;

typedef struct
{
    WORD idReserved;
    WORD idType;
    WORD idCount;
    GRPICONDIRENTRY idEntries[1];
} GRPICONDIR;

typedef struct
{
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD wPlanes;
    WORD wBitCount;
    DWORD dwBytesInRes;
    DWORD dwImageOffset;
} ICONDIRENTRY;

typedef struct
{
    WORD idReserved;
    WORD idType;
    WORD idCount;
} ICONDIR;


#include "poppack.h"

typedef struct
{
        HRSRC *pResInfo;
        int   nIndex;
} ENUMRESSTRUCT;


/* Icon extraction routines
 *
 * FIXME: should use PrivateExtractIcons and friends
 * FIXME: should not use stdio
 */

static BOOL SaveIconResAsXPM(const BITMAPINFO *pIcon, const char *szXPMFileName, const char *comment)
{
    FILE *fXPMFile;
    int nHeight;
    int nXORWidthBytes;
    int nANDWidthBytes;
    BOOL b8BitColors;
    int nColors;
    BYTE *pXOR;
    BYTE *pAND;
    BOOL aColorUsed[256] = {0};
    int nColorsUsed = 0;
    int i,j;

    if (!((pIcon->bmiHeader.biBitCount == 4) || (pIcon->bmiHeader.biBitCount == 8)))
        return FALSE;

    if (!(fXPMFile = fopen(szXPMFileName, "w")))
        return FALSE;

    nHeight = pIcon->bmiHeader.biHeight / 2;
    nXORWidthBytes = 4 * ((pIcon->bmiHeader.biWidth * pIcon->bmiHeader.biBitCount / 32)
                          + ((pIcon->bmiHeader.biWidth * pIcon->bmiHeader.biBitCount % 32) > 0));
    nANDWidthBytes = 4 * ((pIcon->bmiHeader.biWidth / 32)
                          + ((pIcon->bmiHeader.biWidth % 32) > 0));
    b8BitColors = pIcon->bmiHeader.biBitCount == 8;
    nColors = pIcon->bmiHeader.biClrUsed ? pIcon->bmiHeader.biClrUsed
        : 1 << pIcon->bmiHeader.biBitCount;
    pXOR = (BYTE*) pIcon + sizeof (BITMAPINFOHEADER) + (nColors * sizeof (RGBQUAD));
    pAND = pXOR + nHeight * nXORWidthBytes;

#define MASK(x,y) (pAND[(x) / 8 + (nHeight - (y) - 1) * nANDWidthBytes] & (1 << (7 - (x) % 8)))
#define COLOR(x,y) (b8BitColors ? pXOR[(x) + (nHeight - (y) - 1) * nXORWidthBytes] : (x) % 2 ? pXOR[(x) / 2 + (nHeight - (y) - 1) * nXORWidthBytes] & 0xF : (pXOR[(x) / 2 + (nHeight - (y) - 1) * nXORWidthBytes] & 0xF0) >> 4)

    for (i = 0; i < nHeight; i++) {
        for (j = 0; j < pIcon->bmiHeader.biWidth; j++) {
            if (!aColorUsed[COLOR(j,i)] && !MASK(j,i))
            {
                aColorUsed[COLOR(j,i)] = TRUE;
                nColorsUsed++;
            }
        }
    }

    if (fprintf(fXPMFile, "/* XPM */\n/* %s */\nstatic char *icon[] = {\n", comment) <= 0)
        goto error;
    if (fprintf(fXPMFile, "\"%d %d %d %d\",\n",
                (int) pIcon->bmiHeader.biWidth, nHeight, nColorsUsed + 1, 2) <=0)
        goto error;

    for (i = 0; i < nColors; i++) {
        if (aColorUsed[i])
            if (fprintf(fXPMFile, "\"%.2X c #%.2X%.2X%.2X\",\n", i, pIcon->bmiColors[i].rgbRed,
                        pIcon->bmiColors[i].rgbGreen, pIcon->bmiColors[i].rgbBlue) <= 0)
                goto error;
    }
    if (fprintf(fXPMFile, "\"   c None\"") <= 0)
        goto error;

    for (i = 0; i < nHeight; i++)
    {
        if (fprintf(fXPMFile, ",\n\"") <= 0)
            goto error;
        for (j = 0; j < pIcon->bmiHeader.biWidth; j++)
        {
            if MASK(j,i)
                {
                    if (fprintf(fXPMFile, "  ") <= 0)
                        goto error;
                }
            else
                if (fprintf(fXPMFile, "%.2X", COLOR(j,i)) <= 0)
                    goto error;
        }
        if (fprintf(fXPMFile, "\"") <= 0)
            goto error;
    }
    if (fprintf(fXPMFile, "};\n") <= 0)
        goto error;

#undef MASK
#undef COLOR

    fclose(fXPMFile);
    return TRUE;

 error:
    fclose(fXPMFile);
    unlink( szXPMFileName );
    return FALSE;
}

static BOOL CALLBACK EnumResNameProc(HMODULE hModule, LPCSTR lpszType, LPSTR lpszName, LONG lParam)
{
    ENUMRESSTRUCT *sEnumRes = (ENUMRESSTRUCT *) lParam;

    if (!sEnumRes->nIndex--)
    {
      *sEnumRes->pResInfo = FindResourceA(hModule, lpszName, (LPSTR)RT_GROUP_ICON);
      return FALSE;
    }
    else
      return TRUE;
}

static BOOL ExtractFromEXEDLL(const char *szFileName, int nIndex, const char *szXPMFileName)
{
    HMODULE hModule;
    HRSRC hResInfo;
    char *lpName = NULL;
    HGLOBAL hResData;
    GRPICONDIR *pIconDir;
    BITMAPINFO *pIcon;
    ENUMRESSTRUCT sEnumRes;
    int nMax = 0;
    int nMaxBits = 0;
    int i;

    if (!(hModule = LoadLibraryExA(szFileName, 0, LOAD_LIBRARY_AS_DATAFILE)))
    {
        WINE_ERR("LoadLibraryExA (%s) failed, error %ld\n", szFileName, GetLastError());
        goto error1;
    }

    if (nIndex < 0)
    {
        hResInfo = FindResourceA(hModule, MAKEINTRESOURCEA(-nIndex), (LPSTR)RT_GROUP_ICON);
        WINE_ERR("FindResourceA (%s) called, return %p, error %ld\n", szFileName, hResInfo, GetLastError());
    }
    else
    {
        hResInfo=NULL;
        sEnumRes.pResInfo = &hResInfo;
        sEnumRes.nIndex = nIndex;
        EnumResourceNamesA(hModule, (LPSTR)RT_GROUP_ICON, &EnumResNameProc, (LONG) &sEnumRes);
    }

    if (!hResInfo)
    {
        WINE_ERR("ExtractFromEXEDLL failed, error %ld\n", GetLastError());
        goto error2;
    }

    if (!(hResData = LoadResource(hModule, hResInfo)))
    {
        WINE_ERR("LoadResource failed, error %ld\n", GetLastError());
        goto error2;
    }
    if (!(pIconDir = LockResource(hResData)))
    {
        WINE_ERR("LockResource failed, error %ld\n", GetLastError());
        goto error3;
    }

    for (i = 0; i < pIconDir->idCount; i++)
        if ((pIconDir->idEntries[i].wBitCount >= nMaxBits) && (pIconDir->idEntries[i].wBitCount <= 8))
        {
          if (pIconDir->idEntries[i].wBitCount > nMaxBits)
          {
              nMaxBits = pIconDir->idEntries[i].wBitCount;
              nMax = 0;
          }
          if ((pIconDir->idEntries[i].bHeight * pIconDir->idEntries[i].bWidth) > nMax)
          {
              lpName = MAKEINTRESOURCEA(pIconDir->idEntries[i].nID);
              nMax = pIconDir->idEntries[i].bHeight * pIconDir->idEntries[i].bWidth;
          }
        }

    FreeResource(hResData);

    if (!(hResInfo = FindResourceA(hModule, lpName, (LPSTR)RT_ICON)))
    {
        WINE_ERR("Second FindResourceA failed, error %ld\n", GetLastError());
        goto error2;
    }
    if (!(hResData = LoadResource(hModule, hResInfo)))
    {
        WINE_ERR("Second LoadResource failed, error %ld\n", GetLastError());
        goto error2;
    }
    if (!(pIcon = LockResource(hResData)))
    {
        WINE_ERR("Second LockResource failed, error %ld\n", GetLastError());
        goto error3;
    }

    if(!SaveIconResAsXPM(pIcon, szXPMFileName, szFileName))
    {
        WINE_ERR("Failed saving icon as XPM, error %ld\n", GetLastError());
        goto error3;
    }

    FreeResource(hResData);
    FreeLibrary(hModule);

    return TRUE;

 error3:
    FreeResource(hResData);
 error2:
    FreeLibrary(hModule);
 error1:
    return FALSE;
}

/* get the Unix file name for a given path, allocating the string */
inline static char *get_unix_file_name( const char *dos )
{
    char buffer[MAX_PATH], *ret;

    if (!wine_get_unix_file_name( dos, buffer, sizeof(buffer) )) return NULL;
    ret = HeapAlloc( GetProcessHeap(), 0, lstrlenA( buffer ) + 1 );
    lstrcpyA( ret, buffer );
    return ret;
}

static int ExtractFromICO(const char *szFileName, const char *szXPMFileName)
{
    FILE *fICOFile;
    ICONDIR iconDir;
    ICONDIRENTRY *pIconDirEntry;
    int nMax = 0;
    int nIndex = 0;
    void *pIcon;
    int i;
    char *filename;

    filename = get_unix_file_name(szFileName);
    if (!(fICOFile = fopen(filename, "r")))
        goto error1;

    if (fread(&iconDir, sizeof (ICONDIR), 1, fICOFile) != 1)
        goto error2;
    if ((iconDir.idReserved != 0) || (iconDir.idType != 1))
        goto error2;

    if ((pIconDirEntry = malloc(iconDir.idCount * sizeof (ICONDIRENTRY))) == NULL)
        goto error2;
    if (fread(pIconDirEntry, sizeof (ICONDIRENTRY), iconDir.idCount, fICOFile) != iconDir.idCount)
        goto error3;

    for (i = 0; i < iconDir.idCount; i++)
        if ((pIconDirEntry[i].bHeight * pIconDirEntry[i].bWidth) > nMax)
        {
            nIndex = i;
            nMax = pIconDirEntry[i].bHeight * pIconDirEntry[i].bWidth;
        }
    if ((pIcon = malloc(pIconDirEntry[nIndex].dwBytesInRes)) == NULL)
        goto error3;
    if (fseek(fICOFile, pIconDirEntry[nIndex].dwImageOffset, SEEK_SET))
        goto error4;
    if (fread(pIcon, pIconDirEntry[nIndex].dwBytesInRes, 1, fICOFile) != 1)
        goto error4;

    if(!SaveIconResAsXPM(pIcon, szXPMFileName, szFileName))
        goto error4;

    free(pIcon);
    free(pIconDirEntry);
    fclose(fICOFile);

    return 1;

 error4:
    free(pIcon);
 error3:
    free(pIconDirEntry);
 error2:
    fclose(fICOFile);
 error1:
    HeapFree(GetProcessHeap(), 0, filename);
    return 0;
}

static BOOL create_default_icon( const char *filename, const char* comment )
{
    FILE *fXPM;
    int i;

    if (!(fXPM = fopen(filename, "w"))) return FALSE;
    if (fprintf(fXPM, "/* XPM */\n/* %s */\nstatic char * icon[] = {", comment) <= 0)
        goto error;
    for (i = 0; i < sizeof(wine_xpm)/sizeof(wine_xpm[0]); i++) {
        if (fprintf( fXPM, "\n\"%s\",", wine_xpm[i]) <= 0)
            goto error;
    }
    if (fprintf( fXPM, "};\n" ) <=0)
        goto error;
    fclose( fXPM );
    return TRUE;
 error:
    fclose( fXPM );
    unlink( filename );
    return FALSE;

}

static unsigned short crc16(const char* string)
{
    unsigned short crc = 0;
    int i, j, xor_poly;

    for (i = 0; string[i] != 0; i++)
    {
        char c = string[i];
        for (j = 0; j < 8; c >>= 1, j++)
        {
            xor_poly = (c ^ crc) & 1;
            crc >>= 1;
            if (xor_poly)
                crc ^= 0xa001;
        }
    }
    return crc;
}

/* extract an icon from an exe or icon file; helper for IPersistFile_fnSave */
static char *extract_icon( const char *path, int index)
{
    int nodefault = 1;
    unsigned short crc;
    char *iconsdir, *ico_path, *ico_name, *xpm_path;
    char* s;
    HKEY hkey;

    /* Where should we save the icon? */
    WINE_TRACE("path=[%s] index=%d\n",path,index);
    iconsdir=NULL;  /* Default is no icon */
    if (!RegOpenKeyA( HKEY_LOCAL_MACHINE, "Software\\Wine\\Wine\\Config\\Wine", &hkey ))
    {
        DWORD size = 0;
        if (RegQueryValueExA(hkey, "IconsDir", 0, NULL, NULL, &size)==0) {
            iconsdir = HeapAlloc(GetProcessHeap(), 0, size);
            RegQueryValueExA(hkey, "IconsDir", 0, NULL, iconsdir, &size);

            s=get_unix_file_name(iconsdir);
            if (s) {
                HeapFree(GetProcessHeap(), 0, iconsdir);
                iconsdir=s;
            }
        }
        RegCloseKey( hkey );
    }
    if (iconsdir==NULL || *iconsdir=='\0')
    {
        if (iconsdir)
            HeapFree(GetProcessHeap(), 0, iconsdir);
        return NULL;  /* No icon created */
    }

    /* If icon path begins with a '*' then this is a deferred call */
    if (path[0] == '*')
    {
        path++;
        nodefault = 0;
    }

    /* Determine the icon base name */
    ico_path=HeapAlloc(GetProcessHeap(), 0, lstrlenA(path)+1);
    strcpy(ico_path, path);
    s=ico_name=ico_path;
    while (*s!='\0') {
        if (*s=='/' || *s=='\\') {
            *s='\\';
            ico_name=s;
        } else {
            *s=tolower(*s);
        }
        s++;
    }
    if (*ico_name=='\\') *ico_name++='\0';
    s=strrchr(ico_name,'.');
    if (s) *s='\0';

    /* Compute the source-path hash */
    crc=crc16(ico_path);

    /* Try to treat the source file as an exe */
    xpm_path=HeapAlloc(GetProcessHeap(), 0, strlen(iconsdir)+1+4+1+strlen(ico_name)+1+12+1+3);
    sprintf(xpm_path,"%s/%04x_%s.%d.xpm",iconsdir,crc,ico_name,index);
    if (ExtractFromEXEDLL( path, index, xpm_path ))
        goto end;

    /* Must be something else, ignore the index in that case */
    sprintf(xpm_path,"%s/%04x_%s.xpm",iconsdir,crc,ico_name);
    if (ExtractFromICO( path, xpm_path))
        goto end;
    if (!nodefault)
        if (create_default_icon( xpm_path, path ))
            goto end;

    HeapFree( GetProcessHeap(), 0, xpm_path );
    xpm_path=NULL;

 end:
    HeapFree( GetProcessHeap(), 0, ico_path );
    return xpm_path;
}

static BOOL DeferToRunOnce(LPWSTR link)
{
    HKEY hkey;
    LONG r, len;
    const WCHAR szRunOnce[] = {
        'S','o','f','t','w','a','r','e','\\',
        'M','i','c','r','o','s','o','f','t','\\',
        'W','i','n','d','o','w','s','\\',
        'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
        'R','u','n','O','n','c','e',0
    };
    const WCHAR szFormat[] = { '%','s',' ','"','%','s','"',0 };
    LPWSTR buffer;
    WCHAR szExecutable[MAX_PATH];

    WINE_TRACE( "Deferring icon creation to reboot.\n");

    if( !GetModuleFileNameW( 0, szExecutable, MAX_PATH ) )
        return FALSE;

    len = ( lstrlenW( link ) + lstrlenW( szExecutable ) + 4)*sizeof(WCHAR);
    buffer = HeapAlloc( GetProcessHeap(), 0, len );
    if( !buffer )
        return FALSE;

    wsprintfW( buffer, szFormat, szExecutable, link );

    r = RegCreateKeyExW(HKEY_LOCAL_MACHINE, szRunOnce, 0,
              NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, NULL);
    if ( r == ERROR_SUCCESS )
    {
        r = RegSetValueExW(hkey, link, 0, REG_SZ,
                   (LPBYTE) buffer, (lstrlenW(buffer) + 1)*sizeof(WCHAR));
        RegCloseKey(hkey);
    }
    HeapFree(GetProcessHeap(), 0, buffer);

    return ! r;
}

/* This escapes \ in filenames */
static LPSTR
escape(LPCSTR arg) {
    LPSTR      narg, x;

    narg = HeapAlloc(GetProcessHeap(),0,2*strlen(arg)+2);
    x = narg;
    while (*arg) {
        *x++ = *arg;
        if (*arg == '\\')
            *x++='\\'; /* escape \ */
        arg++;
    }
    *x = 0;
    return narg;
}

static int fork_and_wait( char *linker, char *link_name, char *path,
                          int desktop, char *args, char *icon_name,
                          char *workdir, char *description )
{
    int pos = 0;
    const char *argv[20];

    WINE_TRACE( "linker app='%s' link='%s' mode=%s "
        "path='%s' args='%s' icon='%s' workdir='%s' descr='%s'\n",
        linker, link_name, desktop ? "desktop" : "menu",
        path, args, icon_name, workdir, description  );

    argv[pos++] = linker ;
    argv[pos++] = "--link";
    argv[pos++] = link_name;
    argv[pos++] = "--path";
    argv[pos++] = path;
    argv[pos++] = desktop ? "--desktop" : "--menu";
    if (args && strlen(args))
    {
        argv[pos++] = "--args";
        argv[pos++] = args;
    }
    if (icon_name)
    {
        argv[pos++] = "--icon";
        argv[pos++] = icon_name;
    }
    if (workdir && strlen(workdir))
    {
        argv[pos++] = "--workdir";
        argv[pos++] = workdir;
    }
    if (description && strlen(description))
    {
        argv[pos++] = "--descr";
        argv[pos++] = description;
    }
    argv[pos] = NULL;

    return spawnvp( _P_WAIT, linker, argv );
}

/* write the name of the ShellLinker into the buffer provided */
static BOOL GetLinkerName( LPSTR szLinker, DWORD max )
{
    LONG r;
    DWORD type = 0;
    HKEY hkey;

    szLinker[0] = 0;
    r = RegOpenKeyExA( HKEY_LOCAL_MACHINE,
                        "Software\\Wine\\Wine\\Config\\Wine",
                        0, KEY_ALL_ACCESS, &hkey );
    if( r )
        return FALSE;
    r = RegQueryValueExA( hkey, "ShellLinker", 0, &type, szLinker, &max );
    RegCloseKey( hkey );
    if( r || ( type != REG_SZ ) )
        return FALSE;

    return TRUE ;
}

static char *cleanup_link( LPCWSTR link )
{
    char  *p, *link_name;
    int len;

    /* make link name a Unix name -
       strip leading slashes & remove extension */
    while ( (*link == '\\') || (*link == '/' ) )
        link++;
    len = WideCharToMultiByte( CP_ACP, 0, link, -1, NULL, 0, NULL, NULL);
    link_name = HeapAlloc( GetProcessHeap(), 0, len*sizeof (WCHAR) );
    if( ! link_name )
        return link_name;
    len = WideCharToMultiByte( CP_ACP, 0, link, -1, link_name, len, NULL, NULL);
    for (p = link_name; *p; p++)
        if (*p == '\\')
             *p = '/';
    p = strrchr( link_name, '.' );
    if (p)
        *p = 0;
    return link_name;
}

/***********************************************************************
 *
 *           GetLinkLocation
 *
 * returns TRUE if successful
 * *loc will contain CS_DESKTOPDIRECTORY, CS_STARTMENU, CS_STARTUP
 */
static BOOL GetLinkLocation( LPCWSTR linkfile, DWORD *ofs, DWORD *loc )
{
    WCHAR ch, filename[MAX_PATH], buffer[MAX_PATH];
    DWORD len, i, r;
    const DWORD locations[] = {
        CSIDL_STARTUP, CSIDL_DESKTOPDIRECTORY, CSIDL_STARTMENU };

    if( !GetFullPathNameW( linkfile, MAX_PATH, filename, NULL ))
        return FALSE;

    for( i=0; i<sizeof(locations)/sizeof(locations[0]); i++ )
    {
        if (!SHGetSpecialFolderPathW( 0, buffer, locations[i], FALSE ))
            continue;

        len = lstrlenW(buffer);
        if( len >= MAX_PATH )
            continue;

        /* do a lstrcmpinW */
        ch = filename[len];
        filename[len] = 0;
        r = lstrcmpiW( filename, buffer );
        filename[len] = ch;

        if ( r )
            continue;

        /* return the remainder of the string and link type */
        *ofs = len;
        *loc = locations[i];
        return TRUE;
    }

    return FALSE;
}

static BOOL InvokeShellLinker( IShellLinkA *sl, LPCWSTR link )
{
    char *link_name, *p, *icon_name = NULL, *work_dir = NULL;
    char *escaped_path = NULL, *escaped_args = NULL;
    CHAR szDescription[MAX_PATH], szPath[MAX_PATH], szWorkDir[MAX_PATH];
    CHAR szArgs[MAX_PATH], szIconPath[MAX_PATH], szLinker[MAX_PATH];
    int iIconId = 0, r;
    DWORD ofs=0, csidl= -1;

    if ( !link )
    {
        WINE_ERR("Link name is null\n");
        return FALSE;
    }

    if( !GetLinkerName( szLinker, MAX_PATH ) )
    {
        WINE_ERR("Can't find the name of the linker script\n");
        return FALSE;
    }

    if( !GetLinkLocation( link, &ofs, &csidl ) )
    {
        WINE_WARN("Unknown link location (%08lx). Ignoring\n", csidl);
        return TRUE;
    }
    if( (csidl != CSIDL_DESKTOPDIRECTORY) && (csidl != CSIDL_STARTMENU) )
    {
        WINE_WARN("Not under desktop or start menu. Ignoring.\n");
        return TRUE;
    }

    szWorkDir[0]=0;
    IShellLinkA_GetWorkingDirectory( sl, szWorkDir, sizeof(szWorkDir));
    WINE_TRACE("workdir    : %s\n", szWorkDir);

    szDescription[0] = 0;
    IShellLinkA_GetDescription( sl, szDescription, sizeof(szDescription));
    WINE_TRACE("description: %s\n", szDescription);

    szPath[0] = 0;
    IShellLinkA_GetPath( sl, szPath, sizeof(szPath), NULL, SLGP_RAWPATH );
    WINE_TRACE("path       : %s\n", szPath);

    szArgs[0] = 0;
    IShellLinkA_GetArguments( sl, szArgs, sizeof(szArgs) );
    WINE_TRACE("args       : %s\n", szArgs);

    szIconPath[0] = 0;
    IShellLinkA_GetIconLocation( sl, szIconPath,
                        sizeof(szIconPath), &iIconId );
    WINE_TRACE("icon file  : %s\n", szIconPath );

    if( !szPath[0] )
    {
        LPITEMIDLIST pidl = NULL;
        IShellLinkA_GetIDList( sl, &pidl );
        if( pidl && SHGetPathFromIDListA( pidl, szPath ) );
            WINE_TRACE("pidl path  : %s\n", szPath );
    }

    /* extract the icon */
    if( szIconPath[0] )
        icon_name = extract_icon( szIconPath , iIconId );
    else
        icon_name = extract_icon( szPath, iIconId );

    /* fail - try once again at reboot time */
    if( !icon_name )
    {
        WINE_ERR("failed to extract icon.\n");
        return FALSE;
    }

    /* check the path */
    if( szPath[0] )
    {
        /* check for .exe extension */
        if (!(p = strrchr( szPath, '.' ))) return FALSE;
        if (strchr( p, '\\' ) || strchr( p, '/' )) return FALSE;
        if (strcasecmp( p, ".exe" )) return FALSE;

        /* convert app working dir */
        if (szWorkDir[0])
            work_dir = get_unix_file_name( szWorkDir );
    }
    else
    {
        /* if there's no path... try run the link itself */
        WideCharToMultiByte( CP_ACP, 0, link, -1, szArgs, MAX_PATH, NULL, NULL );
        strcpy(szPath, "C:\\Windows\\System\\start.exe");
    }

    link_name = cleanup_link( &link[ofs] );
    if( !link_name )
    {
        WINE_ERR("Couldn't clean up link name\n");
        return FALSE;
    }

    /* escape the path and parameters */
    escaped_path = escape(szPath);
    if (szArgs)
        escaped_args = escape(szArgs);

    r = fork_and_wait(szLinker, link_name, escaped_path,
                  (csidl == CSIDL_DESKTOPDIRECTORY), escaped_args, icon_name,
                   work_dir ? work_dir : "", szDescription );

    HeapFree( GetProcessHeap(), 0, icon_name );
    HeapFree( GetProcessHeap(), 0, work_dir );
    HeapFree( GetProcessHeap(), 0, link_name );
    if (escaped_args)
        HeapFree( GetProcessHeap(), 0, escaped_args );
    if (escaped_path)
        HeapFree( GetProcessHeap(), 0, escaped_path );

    if (r)
    {
        WINE_ERR("failed to fork and exec %s\n", szLinker );
        return FALSE;
    }

    return TRUE;
}


static BOOL Process_Link( LPWSTR linkname, BOOL bAgain )
{
    IShellLinkA *sl;
    IPersistFile *pf;
    HRESULT r;
    WCHAR fullname[MAX_PATH];
 
    if( !linkname[0] )
    {
        WINE_ERR("link name missing\n");
        return 1;
    }

    if( !GetFullPathNameW( linkname, MAX_PATH, fullname, NULL ))
    {
        WINE_ERR("couldn't get full path of link file\n");
        return 1;
    }

    r = CoInitialize( NULL );
    if( FAILED( r ) )
        return 1;

    r = CoCreateInstance( &CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                      &IID_IShellLink, (LPVOID *) &sl );
    if( FAILED( r ) )
    {
        WINE_ERR("No IID_IShellLink\n");
        return 1;
    }

    r = IShellLinkA_QueryInterface( sl, &IID_IPersistFile, (LPVOID*) &pf );
    if( FAILED( r ) )
    {
        WINE_ERR("No IID_IPersistFile\n");
        return 1;
    }

    r = IPersistFile_Load( pf, fullname, STGM_READ );
    if( SUCCEEDED( r ) )
    {
        /* If we something fails (eg. Couldn't extract icon)
         * defer this menu entry to reboot via runonce
         */
        if( ! InvokeShellLinker( sl, fullname ) && bAgain )
            DeferToRunOnce( fullname );
        else
            WINE_TRACE("Success.\n");
    }

    IPersistFile_Release( pf );
    IShellLinkA_Release( sl );

    CoUninitialize();

    return !r;
}


static CHAR *next_token( LPSTR *p )
{
    LPSTR token = NULL, t = *p;

    if( !t )
        return NULL;

    while( t && !token )
    {
        switch( *t )
        {
        case ' ':
            t++;
            continue;
        case '"':
            /* unquote the token */
            token = ++t;
            t = strchr( token, '"' );
            if( t )
                 *t++ = 0;
            break;
        case 0:
            t = NULL;
            break;
        default:
            token = t;
            t = strchr( token, ' ' );
            if( t )
                 *t++ = 0;
            break;
        }
    }
    *p = t;
    return token;
}

/***********************************************************************
 *
 *           WinMain
 */
int PASCAL WinMain (HINSTANCE hInstance, HINSTANCE prev, LPSTR cmdline, int show)
{
    LPSTR token = NULL, p;
    BOOL bAgain = FALSE;
    HANDLE hsem = CreateSemaphoreA( NULL, 1, 1, "winemenubuilder_semaphore");
    int ret = 0;

    /* running multiple instances of wineshelllink
       at the same time may be dangerous */
    if( WAIT_OBJECT_0 != WaitForSingleObject( hsem, INFINITE ) )
        return FALSE;

    for( p = cmdline; p && *p; )
    {
        token = next_token( &p );
	if( !token )
	    break;
        if( !lstrcmpA( token, "-r" ) )
            bAgain = TRUE;
	else if( token[0] == '-' )
	{
	    WINE_ERR( "unknown option %s\n",token);
	}
        else
        {
            WCHAR link[MAX_PATH];

            MultiByteToWideChar( CP_ACP, 0, token, -1, link, sizeof(link) );
            if( !Process_Link( link, bAgain ) )
            {
	        WINE_ERR( "failed to build menu item for %s\n",token);
	        ret = 1;
                break;
            }
        }
    }

    ReleaseSemaphore( hsem, 1, NULL );
    CloseHandle( hsem );

    return ret;
}
