/*
 * Helper program to build unix menu entries
 *
 * Copyright 1997 Marcus Meissner
 * Copyright 1998 Juergen Schmied
 * Copyright 2003 Mike McCormack for CodeWeavers
 * Copyright 2004 Dmitry Timoshkov
 * Copyright 2005 Bill Medland
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 *
 *  This program is used to replicate the Windows desktop and start menu
 * into the native desktop's copies.  Desktop entries are merged directly
 * into the native desktop.  The Windows Start Menu corresponds to a Wine
 * entry within the native "start" menu and replicates the whole tree
 * structure of the Windows Start Menu.  Currently it does not differentiate
 * between the user's desktop/start menu and the "All Users" copies.
 *
 *  This program will read a Windows shortcut file using the IShellLink
 * interface, then invoke wineshelllink with the appropriate arguments
 * to create a KDE/Gnome menu entry for the shortcut.
 *
 *  winemenubuilder [ -w ] <shortcut.lnk>
 *
 *  If the -w parameter is passed, and the shortcut cannot be created,
 * this program will wait for the parent process to finish and then try
 * again. This covers the case when a ShortCut is created before the
 * executable containing its icon.
 *
 * TODO
 *  Handle data lnk files. There is no icon in the file; the icon is in 
 * the handler for the file type (or pointed to by the lnk file).  Also it 
 * might be better to use a native handler (e.g. a native acroread for pdf
 * files).  
 *  Differentiate between the user's entries and the "All Users" entries.
 * If it is possible to add the desktop files to the native system's
 * shared location for an "All Users" entry then do so.  As a suggestion the
 * shared menu Wine base could be writable to the wine group, or a wineadm 
 * group.
 * 
 */

#include "config.h"
#include "wine/port.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>
#include <stdarg.h>

#define COBJMACROS

#include <windows.h>
#include <shlobj.h>
#include <objidl.h>
#include <shlguid.h>
#include <appmgmt.h>
#include <tlhelp32.h>

#include "wine/unicode.h"
#include "wine/debug.h"
#include "wine.xpm"

WINE_DEFAULT_DEBUG_CHANNEL(menubuilder);

#define in_desktop_dir(csidl) ((csidl)==CSIDL_DESKTOPDIRECTORY || \
                               (csidl)==CSIDL_COMMON_DESKTOPDIRECTORY)
#define in_startmenu(csidl)   ((csidl)==CSIDL_STARTMENU || \
                               (csidl)==CSIDL_COMMON_STARTMENU)
        
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

static BOOL SaveIconResAsXPM(const BITMAPINFO *pIcon, const char *szXPMFileName, LPCWSTR commentW)
{
    FILE *fXPMFile;
    int nHeight;
    int nXORWidthBytes;
    int nANDWidthBytes;
    BOOL b8BitColors;
    int nColors;
    const BYTE *pXOR;
    const BYTE *pAND;
    BOOL aColorUsed[256] = {0};
    int nColorsUsed = 0;
    int i,j;
    char *comment;

    if (!((pIcon->bmiHeader.biBitCount == 4) || (pIcon->bmiHeader.biBitCount == 8)))
    {
        WINE_FIXME("Unsupported color depth %d-bit\n", pIcon->bmiHeader.biBitCount);
        return FALSE;
    }

    if (!(fXPMFile = fopen(szXPMFileName, "w")))
    {
        WINE_TRACE("unable to open '%s' for writing: %s\n", szXPMFileName, strerror(errno));
        return FALSE;
    }

    i = WideCharToMultiByte(CP_UNIXCP, 0, commentW, -1, NULL, 0, NULL, NULL);
    comment = HeapAlloc(GetProcessHeap(), 0, i);
    WideCharToMultiByte(CP_UNIXCP, 0, commentW, -1, comment, i, NULL, NULL);

    nHeight = pIcon->bmiHeader.biHeight / 2;
    nXORWidthBytes = 4 * ((pIcon->bmiHeader.biWidth * pIcon->bmiHeader.biBitCount / 32)
                          + ((pIcon->bmiHeader.biWidth * pIcon->bmiHeader.biBitCount % 32) > 0));
    nANDWidthBytes = 4 * ((pIcon->bmiHeader.biWidth / 32)
                          + ((pIcon->bmiHeader.biWidth % 32) > 0));
    b8BitColors = pIcon->bmiHeader.biBitCount == 8;
    nColors = pIcon->bmiHeader.biClrUsed ? pIcon->bmiHeader.biClrUsed
        : 1 << pIcon->bmiHeader.biBitCount;
    pXOR = (const BYTE*) pIcon + sizeof (BITMAPINFOHEADER) + (nColors * sizeof (RGBQUAD));
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

    HeapFree(GetProcessHeap(), 0, comment);
    fclose(fXPMFile);
    return TRUE;

 error:
    HeapFree(GetProcessHeap(), 0, comment);
    fclose(fXPMFile);
    unlink( szXPMFileName );
    return FALSE;
}

static BOOL CALLBACK EnumResNameProc(HMODULE hModule, LPCWSTR lpszType, LPWSTR lpszName, LONG_PTR lParam)
{
    ENUMRESSTRUCT *sEnumRes = (ENUMRESSTRUCT *) lParam;

    if (!sEnumRes->nIndex--)
    {
        *sEnumRes->pResInfo = FindResourceW(hModule, lpszName, (LPCWSTR)RT_GROUP_ICON);
        return FALSE;
    }
    else
        return TRUE;
}

static BOOL extract_icon32(LPCWSTR szFileName, int nIndex, const char *szXPMFileName)
{
    HMODULE hModule;
    HRSRC hResInfo;
    LPCWSTR lpName = NULL;
    HGLOBAL hResData;
    GRPICONDIR *pIconDir;
    BITMAPINFO *pIcon;
    ENUMRESSTRUCT sEnumRes;
    int nMax = 0;
    int nMaxBits = 0;
    int i;
    BOOL ret = FALSE;

    hModule = LoadLibraryExW(szFileName, 0, LOAD_LIBRARY_AS_DATAFILE);
    if (!hModule)
    {
        WINE_WARN("LoadLibraryExW (%s) failed, error %d\n",
                 wine_dbgstr_w(szFileName), GetLastError());
        return FALSE;
    }

    if (nIndex < 0)
    {
        hResInfo = FindResourceW(hModule, MAKEINTRESOURCEW(-nIndex), (LPCWSTR)RT_GROUP_ICON);
        WINE_TRACE("FindResourceW (%s) called, return %p, error %d\n",
                   wine_dbgstr_w(szFileName), hResInfo, GetLastError());
    }
    else
    {
        hResInfo=NULL;
        sEnumRes.pResInfo = &hResInfo;
        sEnumRes.nIndex = nIndex;
        if (!EnumResourceNamesW(hModule, (LPCWSTR)RT_GROUP_ICON,
                                EnumResNameProc, (LONG_PTR)&sEnumRes))
        {
            WINE_TRACE("EnumResourceNamesW failed, error %d\n", GetLastError());
        }
    }

    if (hResInfo)
    {
        if ((hResData = LoadResource(hModule, hResInfo)))
        {
            if ((pIconDir = LockResource(hResData)))
            {
                for (i = 0; i < pIconDir->idCount; i++)
                {
		    if ((pIconDir->idEntries[i].wBitCount >= nMaxBits) && (pIconDir->idEntries[i].wBitCount <= 8))
		    {
			nMaxBits = pIconDir->idEntries[i].wBitCount;

			if ((pIconDir->idEntries[i].bHeight * pIconDir->idEntries[i].bWidth) >= nMax)
			{
			    lpName = MAKEINTRESOURCEW(pIconDir->idEntries[i].nID);
			    nMax = pIconDir->idEntries[i].bHeight * pIconDir->idEntries[i].bWidth;
			}
		    }		    
                }
            }

            FreeResource(hResData);
        }
    }
    else
    {
        WINE_WARN("found no icon\n");
        FreeLibrary(hModule);
        return FALSE;
    }
 
    if ((hResInfo = FindResourceW(hModule, lpName, (LPCWSTR)RT_ICON)))
    {
        if ((hResData = LoadResource(hModule, hResInfo)))
        {
            if ((pIcon = LockResource(hResData)))
            {
                if(SaveIconResAsXPM(pIcon, szXPMFileName, szFileName))
                    ret = TRUE;
            }

            FreeResource(hResData);
        }
    }

    FreeLibrary(hModule);
    return ret;
}

static BOOL ExtractFromEXEDLL(LPCWSTR szFileName, int nIndex, const char *szXPMFileName)
{
    if (!extract_icon32(szFileName, nIndex, szXPMFileName) /*&&
        !extract_icon16(szFileName, szXPMFileName)*/)
        return FALSE;
    return TRUE;
}

static int ExtractFromICO(LPCWSTR szFileName, const char *szXPMFileName)
{
    FILE *fICOFile;
    ICONDIR iconDir;
    ICONDIRENTRY *pIconDirEntry;
    int nMax = 0, nMaxBits = 0;
    int nIndex = 0;
    void *pIcon;
    int i;
    char *filename;

    filename = wine_get_unix_file_name(szFileName);
    if (!(fICOFile = fopen(filename, "r")))
    {
        WINE_TRACE("unable to open '%s' for reading: %s\n", filename, strerror(errno));
        goto error1;
    }

    if (fread(&iconDir, sizeof (ICONDIR), 1, fICOFile) != 1 ||
        (iconDir.idReserved != 0) || (iconDir.idType != 1))
    {
        WINE_WARN("Invalid ico file format\n");
        goto error2;
    }

    if ((pIconDirEntry = HeapAlloc(GetProcessHeap(), 0, iconDir.idCount * sizeof (ICONDIRENTRY))) == NULL)
        goto error2;
    if (fread(pIconDirEntry, sizeof (ICONDIRENTRY), iconDir.idCount, fICOFile) != iconDir.idCount)
        goto error3;

    for (i = 0; i < iconDir.idCount; i++)
        if (pIconDirEntry[i].wBitCount <= 8 && pIconDirEntry[i].wBitCount >= nMaxBits &&
            (pIconDirEntry[i].bHeight * pIconDirEntry[i].bWidth) >= nMax)
        {
            nIndex = i;
            nMax = pIconDirEntry[i].bHeight * pIconDirEntry[i].bWidth;
            nMaxBits = pIconDirEntry[i].wBitCount;
        }
    if ((pIcon = HeapAlloc(GetProcessHeap(), 0, pIconDirEntry[nIndex].dwBytesInRes)) == NULL)
        goto error3;
    if (fseek(fICOFile, pIconDirEntry[nIndex].dwImageOffset, SEEK_SET))
        goto error4;
    if (fread(pIcon, pIconDirEntry[nIndex].dwBytesInRes, 1, fICOFile) != 1)
        goto error4;

    if(!SaveIconResAsXPM(pIcon, szXPMFileName, szFileName))
        goto error4;

    HeapFree(GetProcessHeap(), 0, pIcon);
    HeapFree(GetProcessHeap(), 0, pIconDirEntry);
    fclose(fICOFile);
    HeapFree(GetProcessHeap(), 0, filename);
    return 1;

 error4:
    HeapFree(GetProcessHeap(), 0, pIcon);
 error3:
    HeapFree(GetProcessHeap(), 0, pIconDirEntry);
 error2:
    fclose(fICOFile);
 error1:
    HeapFree(GetProcessHeap(), 0, filename);
    return 0;
}

static BOOL create_default_icon( const char *filename, const char* comment )
{
    FILE *fXPM;
    unsigned int i;

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
static char *extract_icon( LPCWSTR path, int index, BOOL bWait )
{
    unsigned short crc;
    char *iconsdir, *ico_path, *ico_name, *xpm_path;
    char* s;
    HKEY hkey;
    int n;

    /* Where should we save the icon? */
    WINE_TRACE("path=[%s] index=%d\n", wine_dbgstr_w(path), index);
    iconsdir=NULL;  /* Default is no icon */
    /* @@ Wine registry key: HKCU\Software\Wine\WineMenuBuilder */
    if (!RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\WineMenuBuilder", &hkey ))
    {
        static const WCHAR IconsDirW[] = {'I','c','o','n','s','D','i','r',0};
        LPWSTR iconsdirW;
        DWORD size = 0;

        if (!RegQueryValueExW(hkey, IconsDirW, 0, NULL, NULL, &size))
        {
            iconsdirW = HeapAlloc(GetProcessHeap(), 0, size);
            RegQueryValueExW(hkey, IconsDirW, 0, NULL, (LPBYTE)iconsdirW, &size);

            if (!(iconsdir = wine_get_unix_file_name(iconsdirW)))
            {
                int n = WideCharToMultiByte(CP_UNIXCP, 0, iconsdirW, -1, NULL, 0, NULL, NULL);
                iconsdir = HeapAlloc(GetProcessHeap(), 0, n);
                WideCharToMultiByte(CP_UNIXCP, 0, iconsdirW, -1, iconsdir, n, NULL, NULL);
            }
            HeapFree(GetProcessHeap(), 0, iconsdirW);
        }
        RegCloseKey( hkey );
    }

    if (!iconsdir)
    {
        WCHAR path[MAX_PATH];
        if (GetTempPathW(MAX_PATH, path))
            iconsdir = wine_get_unix_file_name(path);
        if (!iconsdir)
        {
            WINE_TRACE("no IconsDir\n");
            return NULL;  /* No icon created */
        }
    }
    
    if (!*iconsdir)
    {
        WINE_TRACE("icon generation disabled\n");
        HeapFree(GetProcessHeap(), 0, iconsdir);
        return NULL;  /* No icon created */
    }

    /* Determine the icon base name */
    n = WideCharToMultiByte(CP_UNIXCP, 0, path, -1, NULL, 0, NULL, NULL);
    ico_path = HeapAlloc(GetProcessHeap(), 0, n);
    WideCharToMultiByte(CP_UNIXCP, 0, path, -1, ico_path, n, NULL, NULL);
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
    if (!bWait)
        if (create_default_icon( xpm_path, ico_path ))
            goto end;

    HeapFree( GetProcessHeap(), 0, xpm_path );
    xpm_path=NULL;

 end:
    HeapFree(GetProcessHeap(), 0, iconsdir);
    HeapFree(GetProcessHeap(), 0, ico_path);
    return xpm_path;
}

/* This escapes \ in filenames */
static LPSTR escape(LPCWSTR arg)
{
    LPSTR narg, x;
    LPCWSTR esc;
    int len = 0, n;

    esc = arg;
    while((esc = strchrW(esc, '\\')))
    {
        esc++;
        len++;
    }

    len += WideCharToMultiByte(CP_UNIXCP, 0, arg, -1, NULL, 0, NULL, NULL);
    narg = HeapAlloc(GetProcessHeap(), 0, len);

    x = narg;
    while (*arg)
    {
        n = WideCharToMultiByte(CP_UNIXCP, 0, arg, 1, x, len, NULL, NULL);
        x += n;
        len -= n;
        if (*arg == '\\')
            *x++='\\'; /* escape \ */
        arg++;
    }
    *x = 0;
    return narg;
}

static int fork_and_wait( const char *linker, const char *link_name, const char *path,
                          int desktop, const char *args, const char *icon_name,
                          const char *workdir, const char *description )
{
    int pos = 0;
    const char *argv[20];
    int retcode;

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

    retcode=spawnvp( _P_WAIT, linker, argv );
    if (retcode!=0)
        WINE_ERR("%s returned %d\n",linker,retcode);
    return retcode;
}

/* Return a heap-allocated copy of the unix format difference between the two
 * Windows-format paths.
 * locn is the owning location
 * link is within locn
 */
static char *relative_path( LPCWSTR link, LPCWSTR locn )
{
    char *unix_locn, *unix_link;
    char *relative = NULL;

    unix_locn = wine_get_unix_file_name(locn);
    unix_link = wine_get_unix_file_name(link);
    if (unix_locn && unix_link)
    {
        size_t len_unix_locn, len_unix_link;
        len_unix_locn = strlen (unix_locn);
        len_unix_link = strlen (unix_link);
        if (len_unix_locn < len_unix_link && memcmp (unix_locn, unix_link, len_unix_locn) == 0 && unix_link[len_unix_locn] == '/')
        {
            size_t len_rel;
            char *p = strrchr (unix_link + len_unix_locn, '/');
            p = strrchr (p, '.');
            if (p)
            {
                *p = '\0';
                len_unix_link = p - unix_link;
            }
            len_rel = len_unix_link - len_unix_locn;
            relative = HeapAlloc(GetProcessHeap(), 0, len_rel);
            if (relative)
            {
                memcpy (relative, unix_link + len_unix_locn + 1, len_rel);
            }
        }
    }
    if (!relative)
        WINE_WARN("Could not separate the relative link path of %s in %s\n", wine_dbgstr_w(link), wine_dbgstr_w(locn));
    HeapFree(GetProcessHeap(), 0, unix_locn);
    HeapFree(GetProcessHeap(), 0, unix_link);
    return relative;
}

/***********************************************************************
 *
 *           GetLinkLocation
 *
 * returns TRUE if successful
 * *loc will contain CS_DESKTOPDIRECTORY, CS_STARTMENU, CS_STARTUP etc.
 * *relative will contain the address of a heap-allocated copy of the portion
 * of the filename that is within the specified location, in unix form
 */
static BOOL GetLinkLocation( LPCWSTR linkfile, DWORD *loc, char **relative )
{
    WCHAR filename[MAX_PATH], buffer[MAX_PATH];
    DWORD len, i, r, filelen;
    const DWORD locations[] = {
        CSIDL_STARTUP, CSIDL_DESKTOPDIRECTORY, CSIDL_STARTMENU,
        CSIDL_COMMON_STARTUP, CSIDL_COMMON_DESKTOPDIRECTORY,
        CSIDL_COMMON_STARTMENU };

    WINE_TRACE("%s\n", wine_dbgstr_w(linkfile));
    filelen=GetFullPathNameW( linkfile, MAX_PATH, filename, NULL );
    if (filelen==0 || filelen>MAX_PATH)
        return FALSE;

    WINE_TRACE("%s\n", wine_dbgstr_w(filename));

    for( i=0; i<sizeof(locations)/sizeof(locations[0]); i++ )
    {
        if (!SHGetSpecialFolderPathW( 0, buffer, locations[i], FALSE ))
            continue;

        len = lstrlenW(buffer);
        if (len >= MAX_PATH)
            continue; /* We've just trashed memory! Hopefully we are OK */

        if (len > filelen || filename[len]!='\\')
            continue;
        /* do a lstrcmpinW */
        filename[len] = 0;
        r = lstrcmpiW( filename, buffer );
        filename[len] = '\\';
        if ( r )
            continue;

        /* return the remainder of the string and link type */
        *loc = locations[i];
        *relative = relative_path (filename, buffer);
        return (*relative != NULL);
    }

    return FALSE;
}

/* gets the target path directly or through MSI */
static HRESULT get_cmdline( IShellLinkW *sl, LPWSTR szPath, DWORD pathSize,
                            LPWSTR szArgs, DWORD argsSize)
{
    IShellLinkDataList *dl = NULL;
    EXP_DARWIN_LINK *dar = NULL;
    HRESULT hr;

    szPath[0] = 0;
    szArgs[0] = 0;

    hr = IShellLinkW_GetPath( sl, szPath, pathSize, NULL, SLGP_RAWPATH );
    if (hr == S_OK && szPath[0])
    {
        IShellLinkW_GetArguments( sl, szArgs, argsSize );
        return hr;
    }

    hr = IShellLinkW_QueryInterface( sl, &IID_IShellLinkDataList, (LPVOID*) &dl );
    if (FAILED(hr))
        return hr;

    hr = IShellLinkDataList_CopyDataBlock( dl, EXP_DARWIN_ID_SIG, (LPVOID*) &dar );
    if (SUCCEEDED(hr))
    {
        WCHAR* szCmdline;
        DWORD cmdSize;

        cmdSize=0;
        hr = CommandLineFromMsiDescriptor( dar->szwDarwinID, NULL, &cmdSize );
        if (hr == ERROR_SUCCESS)
        {
            cmdSize++;
            szCmdline = HeapAlloc( GetProcessHeap(), 0, cmdSize*sizeof(WCHAR) );
            hr = CommandLineFromMsiDescriptor( dar->szwDarwinID, szCmdline, &cmdSize );
            WINE_TRACE("      command    : %s\n", wine_dbgstr_w(szCmdline));
            if (hr == ERROR_SUCCESS)
            {
                WCHAR *s, *d;
                int bcount, in_quotes;

                /* Extract the application path */
                bcount=0;
                in_quotes=0;
                s=szCmdline;
                d=szPath;
                while (*s)
                {
                    if ((*s==0x0009 || *s==0x0020) && !in_quotes)
                    {
                        /* skip the remaining spaces */
                        do {
                            s++;
                        } while (*s==0x0009 || *s==0x0020);
                        break;
                    }
                    else if (*s==0x005c)
                    {
                        /* '\\' */
                        *d++=*s++;
                        bcount++;
                    }
                    else if (*s==0x0022)
                    {
                        /* '"' */
                        if ((bcount & 1)==0)
                        {
                            /* Preceded by an even number of '\', this is
                             * half that number of '\', plus a quote which
                             * we erase.
                             */
                            d-=bcount/2;
                            in_quotes=!in_quotes;
                            s++;
                        }
                        else
                        {
                            /* Preceded by an odd number of '\', this is
                             * half that number of '\' followed by a '"'
                             */
                            d=d-bcount/2-1;
                            *d++='"';
                            s++;
                        }
                        bcount=0;
                    }
                    else
                    {
                        /* a regular character */
                        *d++=*s++;
                        bcount=0;
                    }
                    if ((d-szPath) == pathSize)
                    {
                        /* Keep processing the path till we get to the
                         * arguments, but 'stand still'
                         */
                        d--;
                    }
                }
                /* Close the application path */
                *d=0;

                lstrcpynW(szArgs, s, argsSize);
            }
            HeapFree( GetProcessHeap(), 0, szCmdline );
        }
        LocalFree( dar );
    }

    IShellLinkDataList_Release( dl );
    return hr;
}

static BOOL InvokeShellLinker( IShellLinkW *sl, LPCWSTR link, BOOL bWait )
{
    static const WCHAR startW[] = {'\\','c','o','m','m','a','n','d',
                                   '\\','s','t','a','r','t','.','e','x','e',0};
    char *link_name = NULL, *icon_name = NULL, *work_dir = NULL;
    char *escaped_path = NULL, *escaped_args = NULL, *escaped_description = NULL;
    WCHAR szDescription[INFOTIPSIZE], szPath[MAX_PATH], szWorkDir[MAX_PATH];
    WCHAR szArgs[INFOTIPSIZE], szIconPath[MAX_PATH];
    int iIconId = 0, r = -1;
    DWORD csidl = -1;
    HANDLE hsem = NULL;

    if ( !link )
    {
        WINE_ERR("Link name is null\n");
        return FALSE;
    }

    if( !GetLinkLocation( link, &csidl, &link_name ) )
    {
        WINE_WARN("Unknown link location %s. Ignoring.\n",wine_dbgstr_w(link));
        return TRUE;
    }
    if (!in_desktop_dir(csidl) && !in_startmenu(csidl))
    {
        WINE_WARN("Not under desktop or start menu. Ignoring.\n");
        return TRUE;
    }
    WINE_TRACE("Link       : %s\n", wine_dbgstr_a(link_name));

    szWorkDir[0] = 0;
    IShellLinkW_GetWorkingDirectory( sl, szWorkDir, MAX_PATH );
    WINE_TRACE("workdir    : %s\n", wine_dbgstr_w(szWorkDir));

    szDescription[0] = 0;
    IShellLinkW_GetDescription( sl, szDescription, INFOTIPSIZE );
    WINE_TRACE("description: %s\n", wine_dbgstr_w(szDescription));

    get_cmdline( sl, szPath, MAX_PATH, szArgs, INFOTIPSIZE);
    WINE_TRACE("path       : %s\n", wine_dbgstr_w(szPath));
    WINE_TRACE("args       : %s\n", wine_dbgstr_w(szArgs));

    szIconPath[0] = 0;
    IShellLinkW_GetIconLocation( sl, szIconPath, MAX_PATH, &iIconId );
    WINE_TRACE("icon file  : %s\n", wine_dbgstr_w(szIconPath) );

    if( !szPath[0] )
    {
        LPITEMIDLIST pidl = NULL;
        IShellLinkW_GetIDList( sl, &pidl );
        if( pidl && SHGetPathFromIDListW( pidl, szPath ) )
            WINE_TRACE("pidl path  : %s\n", wine_dbgstr_w(szPath));
    }

    /* extract the icon */
    if( szIconPath[0] )
        icon_name = extract_icon( szIconPath , iIconId, bWait );
    else
        icon_name = extract_icon( szPath, iIconId, bWait );

    /* fail - try once again after parent process exit */
    if( !icon_name )
    {
        if (bWait)
        {
            WINE_WARN("Unable to extract icon, deferring.\n");
            goto cleanup;
        }
        WINE_ERR("failed to extract icon from %s\n",
                 wine_dbgstr_w( szIconPath[0] ? szIconPath : szPath ));
    }

    /* check the path */
    if( szPath[0] )
    {
        static const WCHAR exeW[] = {'.','e','x','e',0};
        WCHAR *p;

        /* check for .exe extension */
        if (!(p = strrchrW( szPath, '.' )) ||
            strchrW( p, '\\' ) || strchrW( p, '/' ) ||
            lstrcmpiW( p, exeW ))
        {
            /* Not .exe - use 'start.exe' to launch this file */
            p = szArgs + lstrlenW(szPath) + 2;
            if (szArgs[0])
            {
                p[0] = ' ';
                memmove( p+1, szArgs, min( (lstrlenW(szArgs) + 1) * sizeof(szArgs[0]),
                                           sizeof(szArgs) - (p + 1 - szArgs) * sizeof(szArgs[0]) ) );
            }
            else
                p[0] = 0;

            szArgs[0] = '"';
            lstrcpyW(szArgs + 1, szPath);
            p[-1] = '"';

            GetWindowsDirectoryW(szPath, MAX_PATH);
            lstrcatW(szPath, startW);
        }

        /* convert app working dir */
        if (szWorkDir[0])
            work_dir = wine_get_unix_file_name( szWorkDir );
    }
    else
    {
        /* if there's no path... try run the link itself */
        lstrcpynW(szArgs, link, MAX_PATH);
        GetWindowsDirectoryW(szPath, MAX_PATH);
        lstrcatW(szPath, startW);
    }

    /* escape the path and parameters */
    escaped_path = escape(szPath);
    escaped_args = escape(szArgs);
    escaped_description = escape(szDescription);

    /* running multiple instances of wineshelllink
       at the same time may be dangerous */
    hsem = CreateSemaphoreA( NULL, 1, 1, "winemenubuilder_semaphore");
    if( WAIT_OBJECT_0 != WaitForSingleObject( hsem, INFINITE ) )
    {
        WINE_ERR("failed wait for semaphore\n");
        goto cleanup;
    }

    r = fork_and_wait("wineshelllink", link_name, escaped_path,
                      in_desktop_dir(csidl), escaped_args, icon_name,
                      work_dir ? work_dir : "", escaped_description);

    ReleaseSemaphore( hsem, 1, NULL );

cleanup:
    if (hsem) CloseHandle( hsem );
    HeapFree( GetProcessHeap(), 0, icon_name );
    HeapFree( GetProcessHeap(), 0, work_dir );
    HeapFree( GetProcessHeap(), 0, link_name );
    HeapFree( GetProcessHeap(), 0, escaped_args );
    HeapFree( GetProcessHeap(), 0, escaped_path );
    HeapFree( GetProcessHeap(), 0, escaped_description );

    if (r && !bWait)
        WINE_ERR("failed to fork and exec wineshelllink\n" );

    return ( r == 0 );
}

static BOOL WaitForParentProcess( void )
{
    PROCESSENTRY32 procentry;
    HANDLE hsnapshot = NULL, hprocess = NULL;
    DWORD ourpid = GetCurrentProcessId();
    BOOL ret = FALSE, rc;

    WINE_TRACE("Waiting for parent process\n");
    if ((hsnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 )) ==
        INVALID_HANDLE_VALUE)
    {
        WINE_ERR("CreateToolhelp32Snapshot failed, error %d\n", GetLastError());
        goto done;
    }

    procentry.dwSize = sizeof(PROCESSENTRY32);
    rc = Process32First( hsnapshot, &procentry );
    while (rc)
    {
        if (procentry.th32ProcessID == ourpid) break;
        rc = Process32Next( hsnapshot, &procentry );
    }
    if (!rc)
    {
        WINE_WARN("Unable to find current process id %d when listing processes\n", ourpid);
        goto done;
    }

    if ((hprocess = OpenProcess( SYNCHRONIZE, FALSE, procentry.th32ParentProcessID )) ==
        NULL)
    {
        WINE_WARN("OpenProcess failed pid=%d, error %d\n", procentry.th32ParentProcessID,
                 GetLastError());
        goto done;
    }

    if (WaitForSingleObject( hprocess, INFINITE ) == WAIT_OBJECT_0)
        ret = TRUE;
    else
        WINE_ERR("Unable to wait for parent process, error %d\n", GetLastError());

done:
    if (hprocess) CloseHandle( hprocess );
    if (hsnapshot) CloseHandle( hsnapshot );
    return ret;
}

static BOOL Process_Link( LPCWSTR linkname, BOOL bWait )
{
    IShellLinkW *sl;
    IPersistFile *pf;
    HRESULT r;
    WCHAR fullname[MAX_PATH];
    DWORD len;

    WINE_TRACE("%s, wait %d\n", wine_dbgstr_w(linkname), bWait);

    if( !linkname[0] )
    {
        WINE_ERR("link name missing\n");
        return 1;
    }

    len=GetFullPathNameW( linkname, MAX_PATH, fullname, NULL );
    if (len==0 || len>MAX_PATH)
    {
        WINE_ERR("couldn't get full path of link file\n");
        return 1;
    }

    r = CoInitialize( NULL );
    if( FAILED( r ) )
    {
        WINE_ERR("CoInitialize failed\n");
        return 1;
    }

    r = CoCreateInstance( &CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IShellLinkW, (LPVOID *) &sl );
    if( FAILED( r ) )
    {
        WINE_ERR("No IID_IShellLink\n");
        return 1;
    }

    r = IShellLinkW_QueryInterface( sl, &IID_IPersistFile, (LPVOID*) &pf );
    if( FAILED( r ) )
    {
        WINE_ERR("No IID_IPersistFile\n");
        return 1;
    }

    r = IPersistFile_Load( pf, fullname, STGM_READ );
    if( SUCCEEDED( r ) )
    {
        /* If something fails (eg. Couldn't extract icon)
         * wait for parent process and try again
         */
        if( ! InvokeShellLinker( sl, fullname, bWait ) && bWait )
        {
            WaitForParentProcess();
            InvokeShellLinker( sl, fullname, FALSE );
        }
    }

    IPersistFile_Release( pf );
    IShellLinkW_Release( sl );

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
    BOOL bWait = FALSE;
    int ret = 0;

    for( p = cmdline; p && *p; )
    {
        token = next_token( &p );
	if( !token )
	    break;
        if( !lstrcmpA( token, "-w" ) )
            bWait = TRUE;
	else if( token[0] == '-' )
	{
	    WINE_ERR( "unknown option %s\n",token);
	}
        else
        {
            WCHAR link[MAX_PATH];

            MultiByteToWideChar( CP_ACP, 0, token, -1, link, sizeof(link)/sizeof(WCHAR) );
            if( !Process_Link( link, bWait ) )
            {
	        WINE_ERR( "failed to build menu item for %s\n",token);
	        ret = 1;
            }
        }
    }

    return ret;
}
