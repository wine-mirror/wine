/*
 * UNIXFS - Shell namespace extension for the unix filesystem
 *
 * Copyright (C) 2005 Michael Jung
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
 */

/*
 * As you know, windows and unix do have a different philosophy with regard to
 * the question of how a filesystem should be laid out. While we unix geeks
 * learned to love the 'one-tree-rooted-at-/' approach, windows has in fact
 * a whole forest of filesystem trees, each of which is typically identified by
 * a drive letter.
 *
 * We would like wine to integrate as smoothly as possible (that is without
 * sacrificing win32 compatibility) into the unix environment. For the
 * filesystem question, this means we really would like those windows
 * applications to work with unix path- and file-names. Unfortunately, this
 * seems to be impossible in general. Therefore we have those symbolic links
 * in wine's 'dosdevices' directory, which are used to simulate drives
 * to keep windows applications happy. And as a consequence, we have those
 * drive letters show up now and then in GUI applications running under wine,
 * which gets the unix hardcore fans all angry, shouting at us @#!&$%* wine
 * hackers that we are seducing the big companies not to port their applications
 * to unix.
 *
 * DOS paths do appear at various places in GUI applications. Sometimes, they
 * show up in the title bar of an application's window. They tend to accumulate
 * in the most-recently-used section of the file-menu. And I've even seen some
 * in a configuration dialog's edit control. In those examples, wine can't do a
 * lot about this, since path-names can't be told appart from ordinary strings
 * here. That's different in the file dialogs, though.
 *
 * With the introduction of the 'shell' in win32, Microsoft established an 
 * abstraction layer on top of the filesystem, called the shell namespace (I was
 * told that Gnome's virtual filesystem is conceptually similar). In the shell
 * namespace, one doesn't use ascii- or unicode-strings to uniquely identify
 * objects. Instead Microsoft introduced item-identifier-lists (The c type is 
 * called ITEMIDLIST) as an abstraction of path-names. As you probably would
 * have guessed, an item-identifier-list is a list of item-identifiers (whose
 * c type's funny name is SHITEMID), which are opaque binary objects. This means
 * that no application (apart from Microsoft Office) should make any assumptions
 * on the internal structure of these SHITEMIDs. 
 *
 * Since the user prefers to be presented the good-old DOS file-names instead of 
 * binary ITEMIDLISTs, a translation method between string-based file-names and
 * ITEMIDLISTs was established. At the core of this are the COM-Interface
 * IShellFolder and especially it's methods ParseDisplayName and 
 * GetDisplayNameOf. Basically, you give a DOS-path (let's say C:\windows) to
 * ParseDisplayName and get a SHITEMID similar to <Desktop|My Computer|C:|windows|>.
 * Since it's opaque, you can't see the 'C', the 'windows' and the other stuff.
 * You can only figure out that the ITEMIDLIST is composed of four SHITEMIDS.
 * The file dialog applies IShellFolder's BindToObject method to bind to each of
 * those four objects (Desktop, My Computer, C: and windows. All of them have to 
 * implement the IShellFolder interface.) and asks them how they would like to be 
 * displayed (basically their icon and the string displayed). If the file dialog 
 * asks <Desktop|My Computer|C:|windows> which sub-objects it contains (via 
 * EnumObjects) it gets a list of opaque SHITEMIDs, which can be concatenated to 
 * <Desktop|...|windows> to build a new ITEMIDLIST and browse, for instance, 
 * into <system32>. This means the file dialog browses the shell namespace by 
 * identifying objects via ITEMIDLISTs. Once the user has selected a location to 
 * save his valuable file, the file dialog calls IShellFolder's GetDisplayNameOf
 * method to translate the ITEMIDLIST back to a DOS filename.
 * 
 * It seems that one intention of the shell namespace concept was to make it 
 * possible to have objects in the namespace, which don't have any counterpart 
 * in the filesystem. The 'My Computer' shell folder object is one instance
 * which comes to mind (Go try to save a file into 'My Computer' on windows.)
 * So, to make matters a little more complex, before the file dialog asks a
 * shell namespace object for it's DOS path, it asks if it actually has one.
 * This is done via the IShellFolder::GetAttributesOf method, which sets the
 * SFGAO_FILESYSTEM if - and only if - it has.
 *
 * The two things, described in the previous two paragraphs, are what unixfs is
 * based on. So basically, if UnixDosFolder's ParseDisplayName method is called 
 * with a 'c:\windows' path-name, it doesn't return an 
 * <Desktop|My Computer|C:|windows|> ITEMIDLIST. Instead, it uses 
 * shell32's wine_get_unix_path_name and the _posix_ (which means not the win32) 
 * fileio api's to figure out that c: is mapped to - let's say - 
 * /home/mjung/.wine/drive_c and then constructs a 
 * <Desktop|/|home|mjung|.wine|drive_c> ITEMIDLIST. Which is what the file 
 * dialog uses to display the folder and file objects, which is why you see a 
 * unix path. When the user has found a nice place for his file and hits the
 * save button, the ITEMIDLIST of the selected folder object is passed to 
 * GetDisplayNameOf, which returns a _DOS_ path name 
 * (like H:\home_of_my_new_file out of <|Desktop|/|home|mjung|home_of_my_new_file|>).
 * Unixfs basically mounts your dos devices together in order to construct
 * a copy of your unix filesystem structure.
 *
 * But what if none of the symbolic links in 'dosdevices' points to '/', you 
 * might ask ("And I don't want wine have access to my complete hard drive, you 
 * *%&1#!"). No problem, as I stated above, unixfs uses the _posix_ apis to 
 * construct the ITEMIDLISTs. Folders, which aren't accessible via a drive letter,
 * don't have the SFGAO_FILESYSTEM flag set. So the file dialogs should'nt allow
 * the user to select such a folder for file storage (And if it does anyhow, it 
 * will not be able to return a valid path, since there is none). Think of those 
 * folders as a hierarchy of 'My Computer'-like folders, which happen to be a 
 * shadow of your unix filesystem tree. And since all of this stuff doesn't 
 * change anything at all in wine's fileio api's, windows applications will have 
 * no more access rights as they had before. 
 *
 * To sum it all up, you can still savely run wine with you root account (Just
 * kidding, don't do it.)
 *
 * If you are now standing in front of your computer, shouting hotly 
 * "I am not convinced, Mr. Rumsfeld^H^H^H^H^H^H^H^H^H^H^H^H", fire up regedit
 * and delete HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\
 * Explorer\Desktop\Namespace\{9D20AAE8-0625-44B0-9CA7-71889C2254D9} and you 
 * will be back in the pre-unixfs days.
 */

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <dirent.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_PWD_H
# include <pwd.h>
#endif
#include <grp.h>
#include <limits.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"
#include "winreg.h"
#include "shlwapi.h"
#include "winternl.h"
#include "wine/debug.h"

#include "shell32_main.h"
#include "shellfolder.h"
#include "shfldr.h"
#include "shresdef.h"
#include "pidl.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#define ADJUST_THIS(c,m,p) ((c*)(((long)p)-(long)&(((c*)0)->lp##m##Vtbl)))
#define STATIC_CAST(i,p) ((i*)&p->lp##i##Vtbl)

#define LEN_SHITEMID_FIXED_PART ((USHORT) \
    ( sizeof(USHORT)      /* SHITEMID's cb field. */ \
    + sizeof(PIDLTYPE)    /* PIDLDATA's type field. */ \
    + sizeof(FileStruct)  /* Well, the FileStruct. */ \
    - sizeof(char)        /* One char too much in FileStruct. */ \
    + sizeof(FileStructW) /* You name it. */ \
    - sizeof(WCHAR)       /* One WCHAR too much in FileStructW. */ \
    + sizeof(WORD) ))     /* Offset of FileStructW field in PIDL. */

#define PATHMODE_UNIX 0
#define PATHMODE_DOS  1

/* UnixFolder object layout and typedef.
 */
typedef struct _UnixFolder {
    const IShellFolder2Vtbl       *lpIShellFolder2Vtbl;
    const IPersistFolder3Vtbl     *lpIPersistFolder3Vtbl;
    const IPersistPropertyBagVtbl *lpIPersistPropertyBagVtbl;
    const IDropTargetVtbl         *lpIDropTargetVtbl;
    const ISFHelperVtbl           *lpISFHelperVtbl;
    LONG         m_cRef;
    CHAR         *m_pszPath;     /* Target path of the shell folder (CP_UNIXCP) */
    LPITEMIDLIST m_pidlLocation; /* Location in the shell namespace */
    DWORD        m_dwPathMode;
    DWORD        m_dwAttributes;
    const CLSID  *m_pCLSID;
    DWORD        m_dwDropEffectsMask;
} UnixFolder;

/* Will hold the registered clipboard format identifier for ITEMIDLISTS. */
static UINT cfShellIDList = 0;

/******************************************************************************
 * UNIXFS_is_rooted_at_desktop [Internal]
 *
 * Checks if the unixfs namespace extension is rooted at desktop level.
 *
 * RETURNS
 *  TRUE, if unixfs is rooted at desktop level
 *  FALSE, if not.
 */
BOOL UNIXFS_is_rooted_at_desktop(void) {
    HKEY hKey;
    WCHAR wszRootedAtDesktop[69 + CHARS_IN_GUID] = { 
        'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
        'W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
        'E','x','p','l','o','r','e','r','\\','D','e','s','k','t','o','p','\\',
        'N','a','m','e','S','p','a','c','e','\\',0 }; 

    if (StringFromGUID2(&CLSID_UnixDosFolder, wszRootedAtDesktop + 69, CHARS_IN_GUID) &&
        RegOpenKeyExW(HKEY_LOCAL_MACHINE, wszRootedAtDesktop, 0, KEY_READ, &hKey) == ERROR_SUCCESS) 
    {
        RegCloseKey(hKey);
        return TRUE;
    }
    return FALSE;
}

/******************************************************************************
 * UNIXFS_filename_from_shitemid [Internal]
 *
 *  Get CP_UNIXCP encoded filename corresponding to the first item of a pidl
 * 
 * PARAMS
 *  pidl           [I] A simple SHITEMID
 *  pszPathElement [O] Filename in CP_UNIXCP encoding will be stored here
 *
 * RETURNS
 *  Success: Number of bytes necessary to store the CP_UNIXCP encoded filename 
 *   _without_ the terminating NUL.
 *  Failure: 0
 *  
 * NOTES
 *  Size of the buffer at pszPathElement has to be FILENAME_MAX. pszPathElement
 *  may be NULL, if you are only interested in the return value. 
 */
static int UNIXFS_filename_from_shitemid(LPCITEMIDLIST pidl, char* pszPathElement) {
    FileStructW *pFileStructW = _ILGetFileStructW(pidl);
    int cLen = 0;

    if (pFileStructW) {
        cLen = WideCharToMultiByte(CP_UNIXCP, 0, pFileStructW->wszName, -1, pszPathElement,
            pszPathElement ? FILENAME_MAX : 0, 0, 0);
    } else {
        /* There might be pidls slipping in from shfldr_fs.c, which don't contain the
         * FileStructW field. In this case, we have to convert from CP_ACP to CP_UNIXCP. */
        char *pszText = _ILGetTextPointer(pidl);
        WCHAR *pwszPathElement = NULL;
        int cWideChars;
    
        cWideChars = MultiByteToWideChar(CP_ACP, 0, pszText, -1, NULL, 0);
        if (!cWideChars) goto cleanup;

        pwszPathElement = SHAlloc(cWideChars * sizeof(WCHAR));
        if (!pwszPathElement) goto cleanup;
    
        cWideChars = MultiByteToWideChar(CP_ACP, 0, pszText, -1, pwszPathElement, cWideChars);
        if (!cWideChars) goto cleanup; 

        cLen = WideCharToMultiByte(CP_UNIXCP, 0, pwszPathElement, -1, pszPathElement, 
            pszPathElement ? FILENAME_MAX : 0, 0, 0);

    cleanup:
        SHFree(pwszPathElement);
    }
        
    if (cLen) cLen--; /* Don't count terminating NUL! */
    return cLen;
}

/******************************************************************************
 * UNIXFS_shitemid_len_from_filename [Internal]
 *
 * Computes the necessary length of a pidl to hold a path element
 *
 * PARAMS
 *  szPathElement    [I] The path element string in CP_UNIXCP encoding.
 *  ppszPathElement  [O] Path element string in CP_ACP encoding.
 *  ppwszPathElement [O] Path element string as WCHAR string.
 *
 * RETURNS
 *  Success: Length in bytes of a SHITEMID representing szPathElement
 *  Failure: 0
 * 
 * NOTES
 *  Provide NULL values if not interested in pp(w)szPathElement. Otherwise
 *  caller is responsible to free ppszPathElement and ppwszPathElement with
 *  SHFree.
 */
static USHORT UNIXFS_shitemid_len_from_filename(
    const char *szPathElement, char **ppszPathElement, WCHAR **ppwszPathElement) 
{
    USHORT cbPidlLen = 0;
    WCHAR *pwszPathElement = NULL;
    char *pszPathElement = NULL;
    int cWideChars, cChars;

    /* There and Back Again: A Hobbit's Holiday. CP_UNIXCP might be some ANSI
     * codepage or it might be a real multi-byte encoding like utf-8. There is no
     * other way to figure out the length of the corresponding WCHAR and CP_ACP 
     * strings without actually doing the full CP_UNIXCP -> WCHAR -> CP_ACP cycle. */
    
    cWideChars = MultiByteToWideChar(CP_UNIXCP, 0, szPathElement, -1, NULL, 0);
    if (!cWideChars) goto cleanup;

    pwszPathElement = SHAlloc(cWideChars * sizeof(WCHAR));
    if (!pwszPathElement) goto cleanup;

    cWideChars = MultiByteToWideChar(CP_UNIXCP, 0, szPathElement, -1, pwszPathElement, cWideChars);
    if (!cWideChars) goto cleanup; 

    cChars = WideCharToMultiByte(CP_ACP, 0, pwszPathElement, -1, NULL, 0, 0, 0);
    if (!cChars) goto cleanup;

    pszPathElement = SHAlloc(cChars);
    if (!pszPathElement) goto cleanup;

    cChars = WideCharToMultiByte(CP_ACP, 0, pwszPathElement, -1, pszPathElement, cChars, 0, 0);
    if (!cChars) goto cleanup;

    /* (cChars & 0x1) is for the potential alignment byte */
    cbPidlLen = LEN_SHITEMID_FIXED_PART + cChars + (cChars & 0x1) + cWideChars * sizeof(WCHAR);
    
cleanup:
    if (cbPidlLen && ppszPathElement) 
        *ppszPathElement = pszPathElement;
    else 
        SHFree(pszPathElement);

    if (cbPidlLen && ppwszPathElement)
        *ppwszPathElement = pwszPathElement;
    else
        SHFree(pwszPathElement);

    return cbPidlLen;
}

/******************************************************************************
 * UNIXFS_is_pidl_of_type [Internal]
 *
 * Checks for the first SHITEMID of an ITEMIDLIST if it passes a filter.
 *
 * PARAMS
 *  pIDL    [I] The ITEMIDLIST to be checked.
 *  fFilter [I] Shell condition flags, which specify the filter.
 *
 * RETURNS
 *  TRUE, if pIDL is accepted by fFilter
 *  FALSE, otherwise
 */
static inline BOOL UNIXFS_is_pidl_of_type(LPITEMIDLIST pIDL, SHCONTF fFilter) {
    LPPIDLDATA pIDLData = _ILGetDataPointer(pIDL);
    if (!(fFilter & SHCONTF_INCLUDEHIDDEN) && pIDLData && 
        (pIDLData->u.file.uFileAttribs & FILE_ATTRIBUTE_HIDDEN)) 
    {
        return FALSE;
    }
    if (_ILIsFolder(pIDL) && (fFilter & SHCONTF_FOLDERS)) return TRUE;
    if (_ILIsValue(pIDL) && (fFilter & SHCONTF_NONFOLDERS)) return TRUE;
    return FALSE;
}

/******************************************************************************
 * UNIXFS_get_unix_path [Internal]
 *
 * Convert an absolute dos path to an absolute unix path.
 * Evaluate "/.", "/.." and the symbolic links in $WINEPREFIX/dosdevices.
 *
 * PARAMS
 *  pszDosPath       [I] An absolute dos path
 *  pszCanonicalPath [O] Buffer of length FILENAME_MAX. Will receive the canonical path.
 *
 * RETURNS
 *  Success, TRUE
 *  Failure, FALSE - Path not existent, too long, insufficient rights, to many symlinks
 */
static BOOL UNIXFS_get_unix_path(LPCWSTR pszDosPath, char *pszCanonicalPath)
{
    char *pPathTail, *pElement, *pCanonicalTail, szPath[FILENAME_MAX], *pszUnixPath;
    WCHAR wszDrive[] = { '?', ':', '\\', 0 };
    int cDriveSymlinkLen;
    
    TRACE("(pszDosPath=%s, pszCanonicalPath=%p)\n", debugstr_w(pszDosPath), pszCanonicalPath);

    if (!pszDosPath || pszDosPath[1] != ':')
        return FALSE;

    /* Get the canonicalized unix path corresponding to the drive letter. */
    wszDrive[0] = pszDosPath[0];
    pszUnixPath = wine_get_unix_file_name(wszDrive);
    if (!pszUnixPath) return FALSE;
    cDriveSymlinkLen = strlen(pszUnixPath);
    pElement = realpath(pszUnixPath, szPath);
    HeapFree(GetProcessHeap(), 0, pszUnixPath);
    if (!pElement) return FALSE;
    if (szPath[strlen(szPath)-1] != '/') strcat(szPath, "/");

    /* Append the part relative to the drive symbolic link target. */
    pszUnixPath = wine_get_unix_file_name(pszDosPath);
    if (!pszUnixPath) return FALSE;
    strcat(szPath, pszUnixPath + cDriveSymlinkLen);
    HeapFree(GetProcessHeap(), 0, pszUnixPath);
    
    /* pCanonicalTail always points to the end of the canonical path constructed
     * thus far. pPathTail points to the still to be processed part of the input
     * path. pElement points to the path element currently investigated.
     */
    *pszCanonicalPath = '\0';
    pCanonicalTail = pszCanonicalPath;
    pPathTail = szPath;

    do {
        char cTemp;
            
        pElement = pPathTail;
        pPathTail = strchr(pPathTail+1, '/');
        if (!pPathTail) /* Last path element may not be terminated by '/'. */ 
            pPathTail = pElement + strlen(pElement);
        /* Temporarily terminate the current path element. Will be restored later. */
        cTemp = *pPathTail;
        *pPathTail = '\0';

        /* Skip "/." path elements */
        if (!strcmp("/.", pElement)) {
            *pPathTail = cTemp;
        } else if (!strcmp("/..", pElement)) {
            /* Remove last element in canonical path for "/.." elements, then skip. */
            char *pTemp = strrchr(pszCanonicalPath, '/');
            if (pTemp)
                pCanonicalTail = pTemp;
            *pCanonicalTail = '\0';
            *pPathTail = cTemp;
        } else {
            /* Directory or file. Copy to canonical path */
            if (pCanonicalTail - pszCanonicalPath + pPathTail - pElement + 1 > FILENAME_MAX)
                return FALSE;
                
            memcpy(pCanonicalTail, pElement, pPathTail - pElement + 1);
            pCanonicalTail += pPathTail - pElement;
            *pPathTail = cTemp;
        }
    } while (pPathTail[0] == '/');
   
    TRACE("--> %s\n", debugstr_a(pszCanonicalPath));
    
    return TRUE;
}

/******************************************************************************
 * UNIXFS_seconds_since_1970_to_dos_date_time [Internal]
 *
 * Convert unix time to FAT time
 *
 * PARAMS 
 *  ss1970 [I] Unix time (seconds since 1970)
 *  pDate  [O] Corresponding FAT date
 *  pTime  [O] Corresponding FAT time
 */
static inline void UNIXFS_seconds_since_1970_to_dos_date_time(
    time_t ss1970, LPWORD pDate, LPWORD pTime)
{
    LARGE_INTEGER time;
    FILETIME fileTime;

    RtlSecondsSince1970ToTime( ss1970, &time );
    fileTime.dwLowDateTime = time.u.LowPart;
    fileTime.dwHighDateTime = time.u.HighPart;
    FileTimeToDosDateTime(&fileTime, pDate, pTime);
}

/******************************************************************************
 * UNIXFS_build_shitemid [Internal]
 *
 * Constructs a new SHITEMID for the last component of path 'pszUnixPath' into 
 * buffer 'pIDL'.
 *
 * PARAMS
 *  pszUnixPath [I] An absolute path. The SHITEMID will be build for the last component.
 *  pIDL        [O] SHITEMID will be constructed here.
 *
 * RETURNS
 *  Success: A pointer to the terminating '\0' character of path.
 *  Failure: NULL
 *
 * NOTES
 *  Minimum size of pIDL is SHITEMID_LEN_FROM_NAME_LEN(strlen(last_component_of_path)).
 *  If what you need is a PIDLLIST with a single SHITEMID, don't forget to append
 *  a 0 USHORT value.
 */
static char* UNIXFS_build_shitemid(char *pszUnixPath, void *pIDL) {
    LPPIDLDATA pIDLData;
    struct stat fileStat;
    char *pszComponentU, *pszComponentA;
    WCHAR *pwszComponentW;
    int cComponentULen, cComponentALen;
    USHORT cbLen;
    FileStructW *pFileStructW;
    WORD uOffsetW, *pOffsetW;

    TRACE("(pszUnixPath=%s, pIDL=%p)\n", debugstr_a(pszUnixPath), pIDL);

    /* We are only interested in regular files and directories. */
    if (stat(pszUnixPath, &fileStat)) return NULL;
    if (!S_ISDIR(fileStat.st_mode) && !S_ISREG(fileStat.st_mode)) return NULL;
    
    /* Compute the SHITEMID's length and wipe it. */
    pszComponentU = strrchr(pszUnixPath, '/') + 1;
    cComponentULen = strlen(pszComponentU);
    cbLen = UNIXFS_shitemid_len_from_filename(pszComponentU, &pszComponentA, &pwszComponentW);
    if (!cbLen) return NULL;
    memset(pIDL, 0, cbLen);
    ((LPSHITEMID)pIDL)->cb = cbLen;
    
    /* Set shell32's standard SHITEMID data fields. */
    pIDLData = _ILGetDataPointer((LPCITEMIDLIST)pIDL);
    pIDLData->type = S_ISDIR(fileStat.st_mode) ? PT_FOLDER : PT_VALUE;
    pIDLData->u.file.dwFileSize = (DWORD)fileStat.st_size;
    UNIXFS_seconds_since_1970_to_dos_date_time(fileStat.st_mtime, &pIDLData->u.file.uFileDate, 
        &pIDLData->u.file.uFileTime);
    pIDLData->u.file.uFileAttribs = 0;
    if (S_ISDIR(fileStat.st_mode)) pIDLData->u.file.uFileAttribs |= FILE_ATTRIBUTE_DIRECTORY;
    if (pszComponentU[0] == '.') pIDLData->u.file.uFileAttribs |=  FILE_ATTRIBUTE_HIDDEN;
    cComponentALen = lstrlenA(pszComponentA) + 1;
    memcpy(pIDLData->u.file.szNames, pszComponentA, cComponentALen);
    
    pFileStructW = (FileStructW*)(pIDLData->u.file.szNames + cComponentALen + (cComponentALen & 0x1));
    uOffsetW = (WORD)(((LPBYTE)pFileStructW) - ((LPBYTE)pIDL));
    pFileStructW->cbLen = cbLen - uOffsetW;
    UNIXFS_seconds_since_1970_to_dos_date_time(fileStat.st_mtime, &pFileStructW->uCreationDate, 
        &pFileStructW->uCreationTime);
    UNIXFS_seconds_since_1970_to_dos_date_time(fileStat.st_atime, &pFileStructW->uLastAccessDate,
        &pFileStructW->uLastAccessTime);
    lstrcpyW(pFileStructW->wszName, pwszComponentW);

    pOffsetW = (WORD*)(((LPBYTE)pIDL) + cbLen - sizeof(WORD));
    *pOffsetW = uOffsetW;
    
    SHFree(pszComponentA);
    SHFree(pwszComponentW);
    
    return pszComponentU + cComponentULen;
}

/******************************************************************************
 * UNIXFS_path_to_pidl [Internal]
 *
 * PARAMS
 *  pUnixFolder [I] If path is relative, pUnixFolder represents the base path
 *  path        [I] An absolute unix or dos path or a path relativ to pUnixFolder
 *  ppidl       [O] The corresponding ITEMIDLIST. Release with SHFree/ILFree
 *  
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE, invalid params or out of memory
 *
 * NOTES
 *  pUnixFolder also carries the information if the path is expected to be unix or dos.
 */
static BOOL UNIXFS_path_to_pidl(UnixFolder *pUnixFolder, const WCHAR *path, LPITEMIDLIST *ppidl) {
    LPITEMIDLIST pidl;
    int cPidlLen, cPathLen;
    char *pSlash, *pNextSlash, szCompletePath[FILENAME_MAX], *pNextPathElement, *pszAPath;
    WCHAR *pwszPath;

    TRACE("pUnixFolder=%p, path=%s, ppidl=%p\n", pUnixFolder, debugstr_w(path), ppidl);
   
    if (!ppidl || !path)
        return FALSE;

    /* Build an absolute path and let pNextPathElement point to the interesting 
     * relative sub-path. We need the absolute path to call 'stat', but the pidl
     * will only contain the relative part.
     */
    if ((pUnixFolder->m_dwPathMode == PATHMODE_DOS) && (path[1] == ':')) 
    {
        /* Absolute dos path. Convert to unix */
        if (!UNIXFS_get_unix_path(path, szCompletePath))
            return FALSE;
        pNextPathElement = szCompletePath;
    } 
    else if ((pUnixFolder->m_dwPathMode == PATHMODE_UNIX) && (path[0] == '/')) 
    {
        /* Absolute unix path. Just convert to ANSI. */
        WideCharToMultiByte(CP_UNIXCP, 0, path, -1, szCompletePath, FILENAME_MAX, NULL, NULL); 
        pNextPathElement = szCompletePath;
    } 
    else 
    {
        /* Relative dos or unix path. Concat with this folder's path */
        int cBasePathLen = strlen(pUnixFolder->m_pszPath);
        memcpy(szCompletePath, pUnixFolder->m_pszPath, cBasePathLen);
        WideCharToMultiByte(CP_UNIXCP, 0, path, -1, szCompletePath + cBasePathLen, 
                            FILENAME_MAX - cBasePathLen, NULL, NULL);
        pNextPathElement = szCompletePath + cBasePathLen - 1;
        
        /* If in dos mode, replace '\' with '/' */
        if (pUnixFolder->m_dwPathMode == PATHMODE_DOS) {
            char *pBackslash = strchr(pNextPathElement, '\\');
            while (pBackslash) {
                *pBackslash = '/';
                pBackslash = strchr(pBackslash, '\\');
            }
        }
    }

    /* Special case for the root folder. */
    if (!strcmp(szCompletePath, "/")) {
        *ppidl = pidl = (LPITEMIDLIST)SHAlloc(sizeof(USHORT));
        if (!pidl) return FALSE;
        pidl->mkid.cb = 0; /* Terminate the ITEMIDLIST */
        return TRUE;
    }
    
    /* Remove trailing slash, if present */
    cPathLen = strlen(szCompletePath);
    if (szCompletePath[cPathLen-1] == '/') 
        szCompletePath[cPathLen-1] = '\0';

    if ((szCompletePath[0] != '/') || (pNextPathElement[0] != '/')) {
        ERR("szCompletePath: %s, pNextPathElment: %s\n", szCompletePath, pNextPathElement);
        return FALSE;
    }
    
    /* At this point, we have an absolute unix path in szCompletePath 
     * and the relative portion of it in pNextPathElement. Both starting with '/'
     * and _not_ terminated by a '/'. */
    TRACE("complete path: %s, relative path: %s\n", szCompletePath, pNextPathElement);
    
    /* Convert to CP_ACP and WCHAR */
    if (!UNIXFS_shitemid_len_from_filename(pNextPathElement, &pszAPath, &pwszPath))
        return 0;

    /* Compute the length of the complete ITEMIDLIST */
    cPidlLen = 0;
    pSlash = pszAPath;
    while (pSlash) {
        pNextSlash = strchr(pSlash+1, '/');
        cPidlLen += LEN_SHITEMID_FIXED_PART + /* Fixed part length plus potential alignment byte. */
            (pNextSlash ? (pNextSlash - pSlash) & 0x1 : lstrlenA(pSlash) & 0x1); 
        pSlash = pNextSlash;
    }

    /* The USHORT is for the ITEMIDLIST terminator. The NUL terminators for the sub-path-strings
     * are accounted for by the '/' separators, which are not stored in the SHITEMIDs. Above we
     * have ensured that the number of '/'s exactly matches the number of sub-path-strings. */
    cPidlLen += lstrlenA(pszAPath) + lstrlenW(pwszPath) * sizeof(WCHAR) + sizeof(USHORT);

    SHFree(pszAPath);
    SHFree(pwszPath);

    *ppidl = pidl = (LPITEMIDLIST)SHAlloc(cPidlLen);
    if (!pidl) return FALSE;

    /* Concatenate the SHITEMIDs of the sub-directories. */
    while (*pNextPathElement) {
        pSlash = strchr(pNextPathElement+1, '/');
        if (pSlash) *pSlash = '\0';
        pNextPathElement = UNIXFS_build_shitemid(szCompletePath, pidl);
        if (pSlash) *pSlash = '/';
            
        if (!pNextPathElement) {
            SHFree(*ppidl);
            *ppidl = NULL;
            return FALSE;
        }
        pidl = ILGetNext(pidl);
    }
    pidl->mkid.cb = 0; /* Terminate the ITEMIDLIST */

    if ((char *)pidl-(char *)*ppidl+sizeof(USHORT) != cPidlLen) /* We've corrupted the heap :( */ 
        ERR("Computed length of pidl incorrect. Please report.\n");
    
    return TRUE;
}

/******************************************************************************
 * UNIXFS_initialize_target_folder [Internal]
 *
 *  Initialize the m_pszPath member of an UnixFolder, given an absolute unix
 *  base path and a relative ITEMIDLIST. Leave the m_pidlLocation member, which
 *  specifies the location in the shell namespace alone. 
 * 
 * PARAMS
 *  This          [IO] The UnixFolder, whose target path is to be initialized
 *  szBasePath    [I]  The absolute base path
 *  pidlSubFolder [I]  Relative part of the path, given as an ITEMIDLIST
 *  dwAttributes  [I]  Attributes to add to the Folders m_dwAttributes member 
 *                     (Used to pass the SFGAO_FILESYSTEM flag down the path)
 * RETURNS
 *  Success: S_OK,
 *  Failure: E_FAIL
 */
static HRESULT UNIXFS_initialize_target_folder(UnixFolder *This, const char *szBasePath,
    LPCITEMIDLIST pidlSubFolder, DWORD dwAttributes)
{
    LPCITEMIDLIST current = pidlSubFolder;
    DWORD dwPathLen = strlen(szBasePath)+1;
    char *pNextDir;
    WCHAR *dos_name;

    /* Determine the path's length bytes */
    while (current && current->mkid.cb) {
        dwPathLen += UNIXFS_filename_from_shitemid(current, NULL) + 1; /* For the '/' */
        current = ILGetNext(current);
    };

    /* Build the path and compute the attributes*/
    This->m_dwAttributes = 
            dwAttributes|SFGAO_FOLDER|SFGAO_HASSUBFOLDER|SFGAO_FILESYSANCESTOR|SFGAO_CANRENAME;
    This->m_pszPath = pNextDir = SHAlloc(dwPathLen);
    if (!This->m_pszPath) {
        WARN("SHAlloc failed!\n");
        return E_FAIL;
    }
    current = pidlSubFolder;
    strcpy(pNextDir, szBasePath);
    pNextDir += strlen(szBasePath);
    if (This->m_dwPathMode == PATHMODE_UNIX || IsEqualCLSID(&CLSID_MyDocuments, This->m_pCLSID))
        This->m_dwAttributes |= SFGAO_FILESYSTEM;
    while (current && current->mkid.cb) {
        pNextDir += UNIXFS_filename_from_shitemid(current, pNextDir);
        *pNextDir++ = '/';
        current = ILGetNext(current);
    }
    *pNextDir='\0';

    if (!(This->m_dwAttributes & SFGAO_FILESYSTEM) &&
        ((dos_name = wine_get_dos_file_name(This->m_pszPath))))
    {
        This->m_dwAttributes |= SFGAO_FILESYSTEM;
        HeapFree( GetProcessHeap(), 0, dos_name );
    }

    return S_OK;
}

/******************************************************************************
 * UnixFolder
 *
 * Class whose heap based instances represent unix filesystem directories.
 */

static void UnixFolder_Destroy(UnixFolder *pUnixFolder) {
    TRACE("(pUnixFolder=%p)\n", pUnixFolder);
    
    SHFree(pUnixFolder->m_pszPath);
    ILFree(pUnixFolder->m_pidlLocation);
    SHFree(pUnixFolder);
}

static HRESULT WINAPI UnixFolder_IShellFolder2_QueryInterface(IShellFolder2 *iface, REFIID riid, 
    void **ppv) 
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, IShellFolder2, iface);
        
    TRACE("(iface=%p, riid=%p, ppv=%p)\n", iface, riid, ppv);
    
    if (!ppv) return E_INVALIDARG;
    
    if (IsEqualIID(&IID_IUnknown, riid) || IsEqualIID(&IID_IShellFolder, riid) || 
        IsEqualIID(&IID_IShellFolder2, riid)) 
    {
        *ppv = STATIC_CAST(IShellFolder2, This);
    } else if (IsEqualIID(&IID_IPersistFolder3, riid) || IsEqualIID(&IID_IPersistFolder2, riid) || 
               IsEqualIID(&IID_IPersistFolder, riid) || IsEqualIID(&IID_IPersist, riid)) 
    {
        *ppv = STATIC_CAST(IPersistFolder3, This);
    } else if (IsEqualIID(&IID_IPersistPropertyBag, riid)) {
        *ppv = STATIC_CAST(IPersistPropertyBag, This);
    } else if (IsEqualIID(&IID_ISFHelper, riid)) {
        *ppv = STATIC_CAST(ISFHelper, This);
    } else if (IsEqualIID(&IID_IDropTarget, riid)) {
        *ppv = STATIC_CAST(IDropTarget, This);
        if (!cfShellIDList) 
            cfShellIDList = RegisterClipboardFormatA(CFSTR_SHELLIDLIST);
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI UnixFolder_IShellFolder2_AddRef(IShellFolder2 *iface) {
    UnixFolder *This = ADJUST_THIS(UnixFolder, IShellFolder2, iface);

    TRACE("(iface=%p)\n", iface);

    return InterlockedIncrement(&This->m_cRef);
}

static ULONG WINAPI UnixFolder_IShellFolder2_Release(IShellFolder2 *iface) {
    UnixFolder *This = ADJUST_THIS(UnixFolder, IShellFolder2, iface);
    ULONG cRef;
    
    TRACE("(iface=%p)\n", iface);

    cRef = InterlockedDecrement(&This->m_cRef);
    
    if (!cRef) 
        UnixFolder_Destroy(This);

    return cRef;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_ParseDisplayName(IShellFolder2* iface, HWND hwndOwner, 
    LPBC pbcReserved, LPOLESTR lpszDisplayName, ULONG* pchEaten, LPITEMIDLIST* ppidl, 
    ULONG* pdwAttributes)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, IShellFolder2, iface);
    BOOL result;

    TRACE("(iface=%p, hwndOwner=%p, pbcReserved=%p, lpszDisplayName=%s, pchEaten=%p, ppidl=%p, "
          "pdwAttributes=%p) stub\n", iface, hwndOwner, pbcReserved, debugstr_w(lpszDisplayName), 
          pchEaten, ppidl, pdwAttributes);

    result = UNIXFS_path_to_pidl(This, lpszDisplayName, ppidl);
    if (result && pdwAttributes && *pdwAttributes)
    {
        IShellFolder *pParentSF;
        LPCITEMIDLIST pidlLast;
        LPITEMIDLIST pidlComplete = ILCombine(This->m_pidlLocation, *ppidl);
        HRESULT hr;
        
        hr = SHBindToParent(pidlComplete, &IID_IShellFolder, (LPVOID*)&pParentSF, &pidlLast);
        if (FAILED(hr)) {
            FIXME("SHBindToParent failed! hr = %08x\n", hr);
            ILFree(pidlComplete);
            return E_FAIL;
        }
        IShellFolder_GetAttributesOf(pParentSF, 1, &pidlLast, pdwAttributes);
        IShellFolder_Release(pParentSF);
        ILFree(pidlComplete);
    }

    if (!result) TRACE("FAILED!\n");
    return result ? S_OK : E_FAIL;
}

static IUnknown *UnixSubFolderIterator_Constructor(UnixFolder *pUnixFolder, SHCONTF fFilter);

static HRESULT WINAPI UnixFolder_IShellFolder2_EnumObjects(IShellFolder2* iface, HWND hwndOwner, 
    SHCONTF grfFlags, IEnumIDList** ppEnumIDList)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, IShellFolder2, iface);
    IUnknown *newIterator;
    HRESULT hr;
    
    TRACE("(iface=%p, hwndOwner=%p, grfFlags=%08x, ppEnumIDList=%p)\n", 
            iface, hwndOwner, grfFlags, ppEnumIDList);

    if (!This->m_pszPath) {
        WARN("EnumObjects called on uninitialized UnixFolder-object!\n");
        return E_UNEXPECTED;
    }

    newIterator = UnixSubFolderIterator_Constructor(This, grfFlags);
    hr = IUnknown_QueryInterface(newIterator, &IID_IEnumIDList, (void**)ppEnumIDList);
    IUnknown_Release(newIterator);
    
    return hr;
}

static HRESULT CreateUnixFolder(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv, const CLSID *pCLSID);

static HRESULT WINAPI UnixFolder_IShellFolder2_BindToObject(IShellFolder2* iface, LPCITEMIDLIST pidl,
    LPBC pbcReserved, REFIID riid, void** ppvOut)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, IShellFolder2, iface);
    IPersistFolder3 *persistFolder;
    HRESULT hr;
    const CLSID *clsidChild;
        
    TRACE("(iface=%p, pidl=%p, pbcReserver=%p, riid=%p, ppvOut=%p)\n", 
            iface, pidl, pbcReserved, riid, ppvOut);

    if (!pidl || !pidl->mkid.cb)
        return E_INVALIDARG;
   
    if (IsEqualCLSID(This->m_pCLSID, &CLSID_FolderShortcut)) {
        /* Children of FolderShortcuts are ShellFSFolders on Windows. 
         * Unixfs' counterpart is UnixDosFolder. */
        clsidChild = &CLSID_UnixDosFolder;    
    } else {
        clsidChild = This->m_pCLSID;
    }
    
    hr = CreateUnixFolder(NULL, &IID_IPersistFolder3, (void**)&persistFolder, clsidChild);
    if (!SUCCEEDED(hr)) return hr;
    hr = IPersistFolder_QueryInterface(persistFolder, riid, (void**)ppvOut);
   
    if (SUCCEEDED(hr)) {
        UnixFolder *subfolder = ADJUST_THIS(UnixFolder, IPersistFolder3, persistFolder);
        subfolder->m_pidlLocation = ILCombine(This->m_pidlLocation, pidl);
        hr = UNIXFS_initialize_target_folder(subfolder, This->m_pszPath, pidl,
                                             This->m_dwAttributes & SFGAO_FILESYSTEM);
    } 

    IPersistFolder3_Release(persistFolder);
    
    return hr;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_BindToStorage(IShellFolder2* This, LPCITEMIDLIST pidl, 
    LPBC pbcReserved, REFIID riid, void** ppvObj)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_CompareIDs(IShellFolder2* iface, LPARAM lParam, 
    LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    BOOL isEmpty1, isEmpty2;
    HRESULT hr = E_FAIL;
    LPITEMIDLIST firstpidl;
    IShellFolder2 *psf;
    int compare;

    TRACE("(iface=%p, lParam=%ld, pidl1=%p, pidl2=%p)\n", iface, lParam, pidl1, pidl2);
    
    isEmpty1 = !pidl1 || !pidl1->mkid.cb;
    isEmpty2 = !pidl2 || !pidl2->mkid.cb;

    if (isEmpty1 && isEmpty2) 
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);
    else if (isEmpty1)
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (WORD)-1);
    else if (isEmpty2)
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (WORD)1);

    if (_ILIsFolder(pidl1) && !_ILIsFolder(pidl2)) 
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (WORD)-1);
    if (!_ILIsFolder(pidl1) && _ILIsFolder(pidl2))
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (WORD)1);

    compare = CompareStringA(LOCALE_USER_DEFAULT, NORM_IGNORECASE, 
                             _ILGetTextPointer(pidl1), -1,
                             _ILGetTextPointer(pidl2), -1);
    
    if ((compare == CSTR_LESS_THAN) || (compare == CSTR_GREATER_THAN)) 
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (WORD)((compare == CSTR_LESS_THAN)?-1:1));

    if (pidl1->mkid.cb < pidl2->mkid.cb)
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (WORD)-1);
    else if (pidl1->mkid.cb > pidl2->mkid.cb)
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (WORD)1);

    firstpidl = ILCloneFirst(pidl1);
    pidl1 = ILGetNext(pidl1);
    pidl2 = ILGetNext(pidl2);
    
    hr = IShellFolder2_BindToObject(iface, firstpidl, NULL, &IID_IShellFolder, (LPVOID*)&psf);
    if (SUCCEEDED(hr)) {
        hr = IShellFolder_CompareIDs(psf, lParam, pidl1, pidl2);
        IShellFolder2_Release(psf);
    }

    ILFree(firstpidl);
    return hr;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_CreateViewObject(IShellFolder2* iface, HWND hwndOwner,
    REFIID riid, void** ppv)
{
    HRESULT hr = E_INVALIDARG;
        
    TRACE("(iface=%p, hwndOwner=%p, riid=%p, ppv=%p) stub\n", iface, hwndOwner, riid, ppv);
    
    if (!ppv) return E_INVALIDARG;
    *ppv = NULL;
    
    if (IsEqualIID(&IID_IShellView, riid)) {
        LPSHELLVIEW pShellView;
        
        pShellView = IShellView_Constructor((IShellFolder*)iface);
        if (pShellView) {
            hr = IShellView_QueryInterface(pShellView, riid, ppv);
            IShellView_Release(pShellView);
        }
    } else if (IsEqualIID(&IID_IDropTarget, riid)) {
        hr = IShellFolder2_QueryInterface(iface, &IID_IDropTarget, ppv);
    }
    
    return hr;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_GetAttributesOf(IShellFolder2* iface, UINT cidl, 
    LPCITEMIDLIST* apidl, SFGAOF* rgfInOut)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, IShellFolder2, iface);
    HRESULT hr = S_OK;
        
    TRACE("(iface=%p, cidl=%u, apidl=%p, rgfInOut=%p)\n", iface, cidl, apidl, rgfInOut);
 
    if (!rgfInOut || (cidl && !apidl)) 
        return E_INVALIDARG;
    
    if (cidl == 0) {
        *rgfInOut &= This->m_dwAttributes;
    } else {
        char szAbsolutePath[FILENAME_MAX], *pszRelativePath;
        UINT i;

        *rgfInOut = SFGAO_CANCOPY|SFGAO_CANMOVE|SFGAO_CANLINK|SFGAO_CANRENAME|SFGAO_CANDELETE|
                    SFGAO_HASPROPSHEET|SFGAO_DROPTARGET|SFGAO_FILESYSTEM;
        lstrcpyA(szAbsolutePath, This->m_pszPath);
        pszRelativePath = szAbsolutePath + lstrlenA(szAbsolutePath);
        for (i=0; i<cidl; i++) {
            if (!(This->m_dwAttributes & SFGAO_FILESYSTEM)) {
                WCHAR *dos_name;
                if (!UNIXFS_filename_from_shitemid(apidl[i], pszRelativePath)) 
                    return E_INVALIDARG;
                if (!(dos_name = wine_get_dos_file_name( szAbsolutePath )))
                    *rgfInOut &= ~SFGAO_FILESYSTEM;
                else
                    HeapFree( GetProcessHeap(), 0, dos_name );
            }
            if (_ILIsFolder(apidl[i])) 
                *rgfInOut |= SFGAO_FOLDER|SFGAO_HASSUBFOLDER|SFGAO_FILESYSANCESTOR;
        }
    }
    
    return hr;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_GetUIObjectOf(IShellFolder2* iface, HWND hwndOwner, 
    UINT cidl, LPCITEMIDLIST* apidl, REFIID riid, UINT* prgfInOut, void** ppvOut)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, IShellFolder2, iface);
    UINT i;
    
    TRACE("(iface=%p, hwndOwner=%p, cidl=%d, apidl=%p, riid=%s, prgfInOut=%p, ppv=%p)\n",
        iface, hwndOwner, cidl, apidl, debugstr_guid(riid), prgfInOut, ppvOut);

    if (!cidl || !apidl || !riid || !ppvOut) 
        return E_INVALIDARG;

    for (i=0; i<cidl; i++) 
        if (!apidl[i]) 
            return E_INVALIDARG;
    
    if (IsEqualIID(&IID_IContextMenu, riid)) {
        *ppvOut = ISvItemCm_Constructor((IShellFolder*)iface, This->m_pidlLocation, apidl, cidl);
        return S_OK;
    } else if (IsEqualIID(&IID_IDataObject, riid)) {
        *ppvOut = IDataObject_Constructor(hwndOwner, This->m_pidlLocation, apidl, cidl);
        return S_OK;
    } else if (IsEqualIID(&IID_IExtractIconA, riid)) {
        LPITEMIDLIST pidl;
        if (cidl != 1) return E_INVALIDARG;
        pidl = ILCombine(This->m_pidlLocation, apidl[0]);
        *ppvOut = (LPVOID)IExtractIconA_Constructor(pidl);
        SHFree(pidl);
        return S_OK;
    } else if (IsEqualIID(&IID_IExtractIconW, riid)) {
        LPITEMIDLIST pidl;
        if (cidl != 1) return E_INVALIDARG;
        pidl = ILCombine(This->m_pidlLocation, apidl[0]);
        *ppvOut = (LPVOID)IExtractIconW_Constructor(pidl);
        SHFree(pidl);
        return S_OK;
    } else if (IsEqualIID(&IID_IDropTarget, riid)) {
        if (cidl != 1) return E_INVALIDARG;
        return IShellFolder2_BindToObject(iface, apidl[0], NULL, &IID_IDropTarget, ppvOut);
    } else if (IsEqualIID(&IID_IShellLinkW, riid)) {
        FIXME("IShellLinkW\n");
        return E_FAIL;
    } else if (IsEqualIID(&IID_IShellLinkA, riid)) {
        FIXME("IShellLinkA\n");
        return E_FAIL;
    } else {
        FIXME("Unknown interface %s in GetUIObjectOf\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
}

static HRESULT WINAPI UnixFolder_IShellFolder2_GetDisplayNameOf(IShellFolder2* iface, 
    LPCITEMIDLIST pidl, SHGDNF uFlags, STRRET* lpName)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, IShellFolder2, iface);
    HRESULT hr = S_OK;    

    TRACE("(iface=%p, pidl=%p, uFlags=%x, lpName=%p)\n", iface, pidl, uFlags, lpName);
    
    if ((GET_SHGDN_FOR(uFlags) & SHGDN_FORPARSING) &&
        (GET_SHGDN_RELATION(uFlags) != SHGDN_INFOLDER))
    {
        if (!pidl || !pidl->mkid.cb) {
            lpName->uType = STRRET_WSTR;
            if (This->m_dwPathMode == PATHMODE_UNIX) {
                UINT len = MultiByteToWideChar(CP_UNIXCP, 0, This->m_pszPath, -1, NULL, 0);
                lpName->u.pOleStr = SHAlloc(len * sizeof(WCHAR));
                if (!lpName->u.pOleStr) return HRESULT_FROM_WIN32(GetLastError());
                MultiByteToWideChar(CP_UNIXCP, 0, This->m_pszPath, -1, lpName->u.pOleStr, len);
            } else {
                LPWSTR pwszDosFileName = wine_get_dos_file_name(This->m_pszPath);
                if (!pwszDosFileName) return HRESULT_FROM_WIN32(GetLastError());
                lpName->u.pOleStr = SHAlloc((lstrlenW(pwszDosFileName) + 1) * sizeof(WCHAR));
                if (!lpName->u.pOleStr) return HRESULT_FROM_WIN32(GetLastError());
                lstrcpyW(lpName->u.pOleStr, pwszDosFileName);
                PathRemoveBackslashW(lpName->u.pOleStr);
                HeapFree(GetProcessHeap(), 0, pwszDosFileName);
            }
        } else {
            IShellFolder *pSubFolder;
            SHITEMID emptyIDL = { 0, { 0 } };

            hr = IShellFolder_BindToObject(iface, pidl, NULL, &IID_IShellFolder, (void**)&pSubFolder);
            if (!SUCCEEDED(hr)) return hr;
       
            hr = IShellFolder_GetDisplayNameOf(pSubFolder, (LPITEMIDLIST)&emptyIDL, uFlags, lpName);
            IShellFolder_Release(pSubFolder);
        }
    } else {
        WCHAR wszFileName[MAX_PATH];
        if (!_ILSimpleGetTextW(pidl, wszFileName, MAX_PATH)) return E_INVALIDARG;
        lpName->uType = STRRET_WSTR;
        lpName->u.pOleStr = SHAlloc((lstrlenW(wszFileName)+1)*sizeof(WCHAR));
        if (!lpName->u.pOleStr) return HRESULT_FROM_WIN32(GetLastError());
        lstrcpyW(lpName->u.pOleStr, wszFileName);
        if (!(GET_SHGDN_FOR(uFlags) & SHGDN_FORPARSING) && This->m_dwPathMode == PATHMODE_DOS && 
            !_ILIsFolder(pidl) && wszFileName[0] != '.' && SHELL_FS_HideExtension(wszFileName))
        {
            PathRemoveExtensionW(lpName->u.pOleStr);
        }
    }

    TRACE("--> %s\n", debugstr_w(lpName->u.pOleStr));
    
    return hr;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_SetNameOf(IShellFolder2* iface, HWND hwnd, 
    LPCITEMIDLIST pidl, LPCOLESTR lpcwszName, SHGDNF uFlags, LPITEMIDLIST* ppidlOut)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, IShellFolder2, iface);

    static const WCHAR awcInvalidChars[] = { '\\', '/', ':', '*', '?', '"', '<', '>', '|' };
    char szSrc[FILENAME_MAX], szDest[FILENAME_MAX];
    WCHAR wszSrcRelative[MAX_PATH];
    int cBasePathLen = lstrlenA(This->m_pszPath), i;
    struct stat statDest;
    LPITEMIDLIST pidlSrc, pidlDest, pidlRelativeDest;
    LPOLESTR lpwszName;
    HRESULT hr;
   
    TRACE("(iface=%p, hwnd=%p, pidl=%p, lpcwszName=%s, uFlags=0x%08x, ppidlOut=%p)\n",
          iface, hwnd, pidl, debugstr_w(lpcwszName), uFlags, ppidlOut); 

    /* prepare to fail */
    if (ppidlOut)
        *ppidlOut = NULL;
    
    /* pidl has to contain a single non-empty SHITEMID */
    if (_ILIsDesktop(pidl) || !_ILIsPidlSimple(pidl) || !_ILGetTextPointer(pidl))
        return E_INVALIDARG;
 
    /* check for invalid characters in lpcwszName. */
    for (i=0; i < sizeof(awcInvalidChars)/sizeof(*awcInvalidChars); i++)
        if (StrChrW(lpcwszName, awcInvalidChars[i]))
            return HRESULT_FROM_WIN32(ERROR_CANCELLED);

    /* build source path */
    memcpy(szSrc, This->m_pszPath, cBasePathLen);
    UNIXFS_filename_from_shitemid(pidl, szSrc + cBasePathLen);

    /* build destination path */
    memcpy(szDest, This->m_pszPath, cBasePathLen);
    WideCharToMultiByte(CP_UNIXCP, 0, lpcwszName, -1, szDest+cBasePathLen, 
                        FILENAME_MAX-cBasePathLen, NULL, NULL);

    /* If the filename's extension is hidden to the user, we have to append it. */
    if (!(uFlags & SHGDN_FORPARSING) && 
        _ILSimpleGetTextW(pidl, wszSrcRelative, MAX_PATH) && 
        SHELL_FS_HideExtension(wszSrcRelative))
    {
        WCHAR *pwszExt = PathFindExtensionW(wszSrcRelative);
        int cLenDest = strlen(szDest);
        WideCharToMultiByte(CP_UNIXCP, 0, pwszExt, -1, szDest + cLenDest, 
            FILENAME_MAX - cLenDest, NULL, NULL);
    }

    TRACE("src=%s dest=%s\n", szSrc, szDest);

    /* Fail, if destination does already exist */
    if (!stat(szDest, &statDest)) 
        return E_FAIL;

    /* Rename the file */
    if (rename(szSrc, szDest)) 
        return E_FAIL;
    
    /* Build a pidl for the path of the renamed file */
    lpwszName = SHAlloc((lstrlenW(lpcwszName)+1)*sizeof(WCHAR)); /* due to const correctness. */
    lstrcpyW(lpwszName, lpcwszName);
    hr = IShellFolder2_ParseDisplayName(iface, NULL, NULL, lpwszName, NULL, &pidlRelativeDest, NULL);
    SHFree(lpwszName);
    if (FAILED(hr)) {
        rename(szDest, szSrc); /* Undo the renaming */
        return E_FAIL;
    }
    pidlDest = ILCombine(This->m_pidlLocation, pidlRelativeDest);
    ILFree(pidlRelativeDest);
    pidlSrc = ILCombine(This->m_pidlLocation, pidl);
    
    /* Inform the shell */
    if (_ILIsFolder(ILFindLastID(pidlDest))) 
        SHChangeNotify(SHCNE_RENAMEFOLDER, SHCNF_IDLIST, pidlSrc, pidlDest);
    else 
        SHChangeNotify(SHCNE_RENAMEITEM, SHCNF_IDLIST, pidlSrc, pidlDest);
    
    if (ppidlOut) 
        *ppidlOut = ILClone(ILFindLastID(pidlDest));
        
    ILFree(pidlSrc);
    ILFree(pidlDest);
    
    return S_OK;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_EnumSearches(IShellFolder2* iface, 
    IEnumExtraSearch **ppEnum) 
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_GetDefaultColumn(IShellFolder2* iface, 
    DWORD dwReserved, ULONG *pSort, ULONG *pDisplay) 
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_GetDefaultColumnState(IShellFolder2* iface, 
    UINT iColumn, SHCOLSTATEF *pcsFlags)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_GetDefaultSearchGUID(IShellFolder2* iface, 
    GUID *pguid)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_GetDetailsEx(IShellFolder2* iface, 
    LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

#define SHELLVIEWCOLUMNS 7 

static HRESULT WINAPI UnixFolder_IShellFolder2_GetDetailsOf(IShellFolder2* iface, 
    LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *psd)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, IShellFolder2, iface);
    HRESULT hr = E_FAIL;
    struct passwd *pPasswd;
    struct group *pGroup;
    static const shvheader SFHeader[SHELLVIEWCOLUMNS] = {
        {IDS_SHV_COLUMN1,  SHCOLSTATE_TYPE_STR  | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15},
        {IDS_SHV_COLUMN2,  SHCOLSTATE_TYPE_STR  | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
        {IDS_SHV_COLUMN3,  SHCOLSTATE_TYPE_STR  | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
        {IDS_SHV_COLUMN4,  SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 12},
        {IDS_SHV_COLUMN5,  SHCOLSTATE_TYPE_STR  | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 9},
        {IDS_SHV_COLUMN10, SHCOLSTATE_TYPE_STR  | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 7},
        {IDS_SHV_COLUMN11, SHCOLSTATE_TYPE_STR  | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 7}
    };

    TRACE("(iface=%p, pidl=%p, iColumn=%d, psd=%p) stub\n", iface, pidl, iColumn, psd);
    
    if (!psd || iColumn >= SHELLVIEWCOLUMNS)
        return E_INVALIDARG;

    if (!pidl) {
        psd->fmt = SFHeader[iColumn].fmt;
        psd->cxChar = SFHeader[iColumn].cxChar;
        psd->str.uType = STRRET_CSTR;
        LoadStringA(shell32_hInstance, SFHeader[iColumn].colnameid, psd->str.u.cStr, MAX_PATH);
        return S_OK;
    } else {
        struct stat statItem;
        if (iColumn == 4 || iColumn == 5 || iColumn == 6) {
            char szPath[FILENAME_MAX];
            strcpy(szPath, This->m_pszPath);
            if (!UNIXFS_filename_from_shitemid(pidl, szPath + strlen(szPath)))
                return E_INVALIDARG;
            if (stat(szPath, &statItem)) 
                return E_INVALIDARG;
        }
        psd->str.u.cStr[0] = '\0';
        psd->str.uType = STRRET_CSTR;
        switch (iColumn) {
            case 0:
                hr = IShellFolder2_GetDisplayNameOf(iface, pidl, SHGDN_NORMAL|SHGDN_INFOLDER, &psd->str);
                break;
            case 1:
                _ILGetFileSize(pidl, psd->str.u.cStr, MAX_PATH);
                break;
            case 2:
                _ILGetFileType (pidl, psd->str.u.cStr, MAX_PATH);
                break;
            case 3:
                _ILGetFileDate(pidl, psd->str.u.cStr, MAX_PATH);
                break;
            case 4:
                psd->str.u.cStr[0] = S_ISDIR(statItem.st_mode) ? 'd' : '-';
                psd->str.u.cStr[1] = (statItem.st_mode & S_IRUSR) ? 'r' : '-';
                psd->str.u.cStr[2] = (statItem.st_mode & S_IWUSR) ? 'w' : '-';
                psd->str.u.cStr[3] = (statItem.st_mode & S_IXUSR) ? 'x' : '-';
                psd->str.u.cStr[4] = (statItem.st_mode & S_IRGRP) ? 'r' : '-';
                psd->str.u.cStr[5] = (statItem.st_mode & S_IWGRP) ? 'w' : '-';
                psd->str.u.cStr[6] = (statItem.st_mode & S_IXGRP) ? 'x' : '-';
                psd->str.u.cStr[7] = (statItem.st_mode & S_IROTH) ? 'r' : '-';
                psd->str.u.cStr[8] = (statItem.st_mode & S_IWOTH) ? 'w' : '-';
                psd->str.u.cStr[9] = (statItem.st_mode & S_IXOTH) ? 'x' : '-';
                psd->str.u.cStr[10] = '\0';
                break;
            case 5:
                pPasswd = getpwuid(statItem.st_uid);
                if (pPasswd) strcpy(psd->str.u.cStr, pPasswd->pw_name);
                break;
            case 6:
                pGroup = getgrgid(statItem.st_gid);
                if (pGroup) strcpy(psd->str.u.cStr, pGroup->gr_name);
                break;
        }
    }
    
    return hr;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_MapColumnToSCID(IShellFolder2* iface, UINT iColumn,
    SHCOLUMNID *pscid) 
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

/* VTable for UnixFolder's IShellFolder2 interface.
 */
static const IShellFolder2Vtbl UnixFolder_IShellFolder2_Vtbl = {
    UnixFolder_IShellFolder2_QueryInterface,
    UnixFolder_IShellFolder2_AddRef,
    UnixFolder_IShellFolder2_Release,
    UnixFolder_IShellFolder2_ParseDisplayName,
    UnixFolder_IShellFolder2_EnumObjects,
    UnixFolder_IShellFolder2_BindToObject,
    UnixFolder_IShellFolder2_BindToStorage,
    UnixFolder_IShellFolder2_CompareIDs,
    UnixFolder_IShellFolder2_CreateViewObject,
    UnixFolder_IShellFolder2_GetAttributesOf,
    UnixFolder_IShellFolder2_GetUIObjectOf,
    UnixFolder_IShellFolder2_GetDisplayNameOf,
    UnixFolder_IShellFolder2_SetNameOf,
    UnixFolder_IShellFolder2_GetDefaultSearchGUID,
    UnixFolder_IShellFolder2_EnumSearches,
    UnixFolder_IShellFolder2_GetDefaultColumn,
    UnixFolder_IShellFolder2_GetDefaultColumnState,
    UnixFolder_IShellFolder2_GetDetailsEx,
    UnixFolder_IShellFolder2_GetDetailsOf,
    UnixFolder_IShellFolder2_MapColumnToSCID
};

static HRESULT WINAPI UnixFolder_IPersistFolder3_QueryInterface(IPersistFolder3* iface, REFIID riid, 
    void** ppvObject)
{
    return UnixFolder_IShellFolder2_QueryInterface(
        STATIC_CAST(IShellFolder2, ADJUST_THIS(UnixFolder, IPersistFolder3, iface)), riid, ppvObject);
}

static ULONG WINAPI UnixFolder_IPersistFolder3_AddRef(IPersistFolder3* iface)
{
    return UnixFolder_IShellFolder2_AddRef(
        STATIC_CAST(IShellFolder2, ADJUST_THIS(UnixFolder, IPersistFolder3, iface)));
}

static ULONG WINAPI UnixFolder_IPersistFolder3_Release(IPersistFolder3* iface)
{
    return UnixFolder_IShellFolder2_Release(
        STATIC_CAST(IShellFolder2, ADJUST_THIS(UnixFolder, IPersistFolder3, iface)));
}

static HRESULT WINAPI UnixFolder_IPersistFolder3_GetClassID(IPersistFolder3* iface, CLSID* pClassID)
{    
    UnixFolder *This = ADJUST_THIS(UnixFolder, IPersistFolder3, iface);
    
    TRACE("(iface=%p, pClassId=%p)\n", iface, pClassID);
    
    if (!pClassID)
        return E_INVALIDARG;

    memcpy(pClassID, This->m_pCLSID, sizeof(CLSID));
    return S_OK;
}

static HRESULT WINAPI UnixFolder_IPersistFolder3_Initialize(IPersistFolder3* iface, LPCITEMIDLIST pidl)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, IPersistFolder3, iface);
    LPCITEMIDLIST current = pidl;
    char szBasePath[FILENAME_MAX] = "/";
    
    TRACE("(iface=%p, pidl=%p)\n", iface, pidl);

    /* Find the UnixFolderClass root */
    while (current->mkid.cb) {
        if ((_ILIsDrive(current) && IsEqualCLSID(This->m_pCLSID, &CLSID_ShellFSFolder)) ||
            (_ILIsSpecialFolder(current) && IsEqualCLSID(This->m_pCLSID, _ILGetGUIDPointer(current))))
        {
            break;
        }
        current = ILGetNext(current);
    }

    if (current && current->mkid.cb) {
        if (_ILIsDrive(current)) {
            WCHAR wszDrive[4] = { '?', ':', '\\', 0 };
            wszDrive[0] = (WCHAR)*_ILGetTextPointer(current);
            if (!UNIXFS_get_unix_path(wszDrive, szBasePath))
                return E_FAIL;
        } else if (IsEqualIID(&CLSID_MyDocuments, _ILGetGUIDPointer(current))) {
            WCHAR wszMyDocumentsPath[MAX_PATH];
            if (!SHGetSpecialFolderPathW(0, wszMyDocumentsPath, CSIDL_PERSONAL, FALSE))
                return E_FAIL;
            PathAddBackslashW(wszMyDocumentsPath);
            if (!UNIXFS_get_unix_path(wszMyDocumentsPath, szBasePath))
                return E_FAIL;
        } 
        current = ILGetNext(current);
    } else if (_ILIsDesktop(pidl) || _ILIsValue(pidl) || _ILIsFolder(pidl)) {
        /* Path rooted at Desktop */
        WCHAR wszDesktopPath[MAX_PATH];
        if (!SHGetSpecialFolderPathW(0, wszDesktopPath, CSIDL_DESKTOPDIRECTORY, FALSE)) 
            return E_FAIL;
        PathAddBackslashW(wszDesktopPath);
        if (!UNIXFS_get_unix_path(wszDesktopPath, szBasePath))
            return E_FAIL;
        current = pidl;
    } else if (IsEqualCLSID(This->m_pCLSID, &CLSID_FolderShortcut)) {
        /* FolderShortcuts' Initialize method only sets the ITEMIDLIST, which
         * specifies the location in the shell namespace, but leaves the
         * target folder (m_pszPath) alone. See unit tests in tests/shlfolder.c */
        This->m_pidlLocation = ILClone(pidl);
        return S_OK;
    } else {
        ERR("Unknown pidl type!\n");
        pdump(pidl);
        return E_INVALIDARG;
    }
  
    This->m_pidlLocation = ILClone(pidl);
    return UNIXFS_initialize_target_folder(This, szBasePath, current, 0); 
}

static HRESULT WINAPI UnixFolder_IPersistFolder3_GetCurFolder(IPersistFolder3* iface, LPITEMIDLIST* ppidl)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, IPersistFolder3, iface);
    
    TRACE ("(iface=%p, ppidl=%p)\n", iface, ppidl);

    if (!ppidl)
        return E_POINTER;
    *ppidl = ILClone (This->m_pidlLocation);
    return S_OK;
}

static HRESULT WINAPI UnixFolder_IPersistFolder3_InitializeEx(IPersistFolder3 *iface, IBindCtx *pbc, 
    LPCITEMIDLIST pidlRoot, const PERSIST_FOLDER_TARGET_INFO *ppfti)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, IPersistFolder3, iface);
    WCHAR wszTargetDosPath[MAX_PATH];
    char szTargetPath[FILENAME_MAX] = "";
    
    TRACE("(iface=%p, pbc=%p, pidlRoot=%p, ppfti=%p)\n", iface, pbc, pidlRoot, ppfti);

    /* If no PERSIST_FOLDER_TARGET_INFO is given InitializeEx is equivalent to Initialize. */
    if (!ppfti) 
        return IPersistFolder3_Initialize(iface, pidlRoot);

    if (ppfti->csidl != -1) {
        if (FAILED(SHGetFolderPathW(0, ppfti->csidl, NULL, 0, wszTargetDosPath)) ||
            !UNIXFS_get_unix_path(wszTargetDosPath, szTargetPath))
        {
            return E_FAIL;
        }
    } else if (*ppfti->szTargetParsingName) {
        lstrcpyW(wszTargetDosPath, ppfti->szTargetParsingName);
        PathAddBackslashW(wszTargetDosPath);
        if (!UNIXFS_get_unix_path(wszTargetDosPath, szTargetPath)) {
            return E_FAIL;
        }
    } else if (ppfti->pidlTargetFolder) {
        if (!SHGetPathFromIDListW(ppfti->pidlTargetFolder, wszTargetDosPath) ||
            !UNIXFS_get_unix_path(wszTargetDosPath, szTargetPath))
        {
            return E_FAIL;
        }
    } else {
        return E_FAIL;
    }

    This->m_pszPath = SHAlloc(lstrlenA(szTargetPath)+1);
    if (!This->m_pszPath) 
        return E_FAIL;
    lstrcpyA(This->m_pszPath, szTargetPath);
    This->m_pidlLocation = ILClone(pidlRoot);
    This->m_dwAttributes = (ppfti->dwAttributes != -1) ? ppfti->dwAttributes :
        (SFGAO_FOLDER|SFGAO_HASSUBFOLDER|SFGAO_FILESYSANCESTOR|SFGAO_CANRENAME|SFGAO_FILESYSTEM);

    return S_OK;
}

static HRESULT WINAPI UnixFolder_IPersistFolder3_GetFolderTargetInfo(IPersistFolder3 *iface, 
    PERSIST_FOLDER_TARGET_INFO *ppfti)
{
    FIXME("(iface=%p, ppfti=%p) stub\n", iface, ppfti);
    return E_NOTIMPL;
}

/* VTable for UnixFolder's IPersistFolder interface.
 */
static const IPersistFolder3Vtbl UnixFolder_IPersistFolder3_Vtbl = {
    UnixFolder_IPersistFolder3_QueryInterface,
    UnixFolder_IPersistFolder3_AddRef,
    UnixFolder_IPersistFolder3_Release,
    UnixFolder_IPersistFolder3_GetClassID,
    UnixFolder_IPersistFolder3_Initialize,
    UnixFolder_IPersistFolder3_GetCurFolder,
    UnixFolder_IPersistFolder3_InitializeEx,
    UnixFolder_IPersistFolder3_GetFolderTargetInfo
};

static HRESULT WINAPI UnixFolder_IPersistPropertyBag_QueryInterface(IPersistPropertyBag* iface,
    REFIID riid, void** ppv)
{
    return UnixFolder_IShellFolder2_QueryInterface(
        STATIC_CAST(IShellFolder2, ADJUST_THIS(UnixFolder, IPersistPropertyBag, iface)), riid, ppv);
}

static ULONG WINAPI UnixFolder_IPersistPropertyBag_AddRef(IPersistPropertyBag* iface)
{
    return UnixFolder_IShellFolder2_AddRef(
        STATIC_CAST(IShellFolder2, ADJUST_THIS(UnixFolder, IPersistPropertyBag, iface)));
}

static ULONG WINAPI UnixFolder_IPersistPropertyBag_Release(IPersistPropertyBag* iface)
{
    return UnixFolder_IShellFolder2_Release(
        STATIC_CAST(IShellFolder2, ADJUST_THIS(UnixFolder, IPersistPropertyBag, iface)));
}

static HRESULT WINAPI UnixFolder_IPersistPropertyBag_GetClassID(IPersistPropertyBag* iface, 
    CLSID* pClassID)
{
    return UnixFolder_IPersistFolder3_GetClassID(
        STATIC_CAST(IPersistFolder3, ADJUST_THIS(UnixFolder, IPersistPropertyBag, iface)), pClassID);
}

static HRESULT WINAPI UnixFolder_IPersistPropertyBag_InitNew(IPersistPropertyBag* iface)
{
    FIXME("() stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI UnixFolder_IPersistPropertyBag_Load(IPersistPropertyBag *iface, 
    IPropertyBag *pPropertyBag, IErrorLog *pErrorLog)
{
     UnixFolder *This = ADJUST_THIS(UnixFolder, IPersistPropertyBag, iface);
     static const WCHAR wszTarget[] = { 'T','a','r','g','e','t', 0 }, wszNull[] = { 0 };
     PERSIST_FOLDER_TARGET_INFO pftiTarget;
     VARIANT var;
     HRESULT hr;
     
     TRACE("(iface=%p, pPropertyBag=%p, pErrorLog=%p)\n", iface, pPropertyBag, pErrorLog);
 
     if (!pPropertyBag)
         return E_POINTER;
 
     /* Get 'Target' property from the property bag. */
     V_VT(&var) = VT_BSTR;
     hr = IPropertyBag_Read(pPropertyBag, wszTarget, &var, NULL);
     if (FAILED(hr)) 
         return E_FAIL;
     lstrcpyW(pftiTarget.szTargetParsingName, V_BSTR(&var));
     SysFreeString(V_BSTR(&var));
 
     pftiTarget.pidlTargetFolder = NULL;
     lstrcpyW(pftiTarget.szNetworkProvider, wszNull);
     pftiTarget.dwAttributes = -1;
     pftiTarget.csidl = -1;
 
     return UnixFolder_IPersistFolder3_InitializeEx(
                 STATIC_CAST(IPersistFolder3, This), NULL, NULL, &pftiTarget);
}

static HRESULT WINAPI UnixFolder_IPersistPropertyBag_Save(IPersistPropertyBag *iface,
    IPropertyBag *pPropertyBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    FIXME("() stub\n");
    return E_NOTIMPL;
}

/* VTable for UnixFolder's IPersistPropertyBag interface.
 */
static const IPersistPropertyBagVtbl UnixFolder_IPersistPropertyBag_Vtbl = {
    UnixFolder_IPersistPropertyBag_QueryInterface,
    UnixFolder_IPersistPropertyBag_AddRef,
    UnixFolder_IPersistPropertyBag_Release,
    UnixFolder_IPersistPropertyBag_GetClassID,
    UnixFolder_IPersistPropertyBag_InitNew,
    UnixFolder_IPersistPropertyBag_Load,
    UnixFolder_IPersistPropertyBag_Save
};

static HRESULT WINAPI UnixFolder_ISFHelper_QueryInterface(ISFHelper* iface, REFIID riid, 
    void** ppvObject)
{
    return UnixFolder_IShellFolder2_QueryInterface(
        STATIC_CAST(IShellFolder2, ADJUST_THIS(UnixFolder, ISFHelper, iface)), riid, ppvObject);
}

static ULONG WINAPI UnixFolder_ISFHelper_AddRef(ISFHelper* iface)
{
    return UnixFolder_IShellFolder2_AddRef(
        STATIC_CAST(IShellFolder2, ADJUST_THIS(UnixFolder, ISFHelper, iface)));
}

static ULONG WINAPI UnixFolder_ISFHelper_Release(ISFHelper* iface)
{
    return UnixFolder_IShellFolder2_Release(
        STATIC_CAST(IShellFolder2, ADJUST_THIS(UnixFolder, ISFHelper, iface)));
}

static HRESULT WINAPI UnixFolder_ISFHelper_GetUniqueName(ISFHelper* iface, LPWSTR pwszName, UINT uLen)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, ISFHelper, iface);
    IEnumIDList *pEnum;
    HRESULT hr;
    LPITEMIDLIST pidlElem;
    DWORD dwFetched;
    int i;
    static const WCHAR wszNewFolder[] = { 'N','e','w',' ','F','o','l','d','e','r', 0 };
    static const WCHAR wszFormat[] = { '%','s',' ','%','d',0 };

    TRACE("(iface=%p, pwszName=%p, uLen=%u)\n", iface, pwszName, uLen);
    
    if (uLen < sizeof(wszNewFolder)/sizeof(WCHAR)+3)
        return E_INVALIDARG;

    hr = IShellFolder2_EnumObjects(STATIC_CAST(IShellFolder2, This), 0,
                                   SHCONTF_FOLDERS|SHCONTF_NONFOLDERS|SHCONTF_INCLUDEHIDDEN, &pEnum);
    if (SUCCEEDED(hr)) {
        lstrcpynW(pwszName, wszNewFolder, uLen);
        IEnumIDList_Reset(pEnum);
        i = 2;
        while ((IEnumIDList_Next(pEnum, 1, &pidlElem, &dwFetched) == S_OK) && (dwFetched == 1)) {
            WCHAR wszTemp[MAX_PATH];
            _ILSimpleGetTextW(pidlElem, wszTemp, MAX_PATH);
            if (!lstrcmpiW(wszTemp, pwszName)) {
                IEnumIDList_Reset(pEnum);
                snprintfW(pwszName, uLen, wszFormat, wszNewFolder, i++);
                if (i > 99) {
                    hr = E_FAIL;
                    break;
                }
            }
        }
        IEnumIDList_Release(pEnum);
    }
    return hr;
}

static HRESULT WINAPI UnixFolder_ISFHelper_AddFolder(ISFHelper* iface, HWND hwnd, LPCWSTR pwszName, 
    LPITEMIDLIST* ppidlOut)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, ISFHelper, iface);
    char szNewDir[FILENAME_MAX];
    int cBaseLen;

    TRACE("(iface=%p, hwnd=%p, pwszName=%s, ppidlOut=%p)\n", 
            iface, hwnd, debugstr_w(pwszName), ppidlOut);

    if (ppidlOut)
        *ppidlOut = NULL;

    if (!This->m_pszPath || !(This->m_dwAttributes & SFGAO_FILESYSTEM))
        return E_FAIL;
    
    lstrcpynA(szNewDir, This->m_pszPath, FILENAME_MAX);
    cBaseLen = lstrlenA(szNewDir);
    WideCharToMultiByte(CP_UNIXCP, 0, pwszName, -1, szNewDir+cBaseLen, FILENAME_MAX-cBaseLen, 0, 0);
   
    if (mkdir(szNewDir, 0755)) {
        char szMessage[256 + FILENAME_MAX];
        char szCaption[256];

        LoadStringA(shell32_hInstance, IDS_CREATEFOLDER_DENIED, szCaption, sizeof(szCaption));
        sprintf(szMessage, szCaption, szNewDir);
        LoadStringA(shell32_hInstance, IDS_CREATEFOLDER_CAPTION, szCaption, sizeof(szCaption));
        MessageBoxA(hwnd, szMessage, szCaption, MB_OK | MB_ICONEXCLAMATION);

        return E_FAIL;
    } else {
        LPITEMIDLIST pidlRelative;

        /* Inform the shell */
        if (UNIXFS_path_to_pidl(This, pwszName, &pidlRelative)) {
            LPITEMIDLIST pidlAbsolute = ILCombine(This->m_pidlLocation, pidlRelative);
            if (ppidlOut)
                *ppidlOut = pidlRelative;
            else
                ILFree(pidlRelative);
            SHChangeNotify(SHCNE_MKDIR, SHCNF_IDLIST, pidlAbsolute, NULL);
            ILFree(pidlAbsolute);
        } else return E_FAIL;
        return S_OK;
    }
}

/*
 * Delete specified files by converting the path to DOS paths and calling
 * SHFileOperationW. If an error occurs it returns an error code. If the paths can't
 * be converted, S_FALSE is returned. In such situation DeleteItems will try to delete
 * the files using syscalls
 */
static HRESULT UNIXFS_delete_with_shfileop(UnixFolder *This, UINT cidl, LPCITEMIDLIST *apidl)
{
    char szAbsolute[FILENAME_MAX], *pszRelative;
    LPWSTR wszPathsList, wszListPos;
    SHFILEOPSTRUCTW op;
    HRESULT ret;
    int i;
    
    lstrcpyA(szAbsolute, This->m_pszPath);
    pszRelative = szAbsolute + lstrlenA(szAbsolute);
    
    wszListPos = wszPathsList = HeapAlloc(GetProcessHeap(), 0, cidl*MAX_PATH*sizeof(WCHAR)+1);
    if (wszPathsList == NULL)
        return E_OUTOFMEMORY;
    for (i=0; i<cidl; i++) {
        LPWSTR wszDosPath;
        
        if (!_ILIsFolder(apidl[i]) && !_ILIsValue(apidl[i]))
            continue;
        if (!UNIXFS_filename_from_shitemid(apidl[i], pszRelative))
        {
            HeapFree(GetProcessHeap(), 0, wszPathsList);
            return E_INVALIDARG;
        }
        wszDosPath = wine_get_dos_file_name(szAbsolute);
        if (wszDosPath == NULL || lstrlenW(wszDosPath) >= MAX_PATH)
        {
            HeapFree(GetProcessHeap(), 0, wszPathsList);
            HeapFree(GetProcessHeap(), 0, wszDosPath);
            return S_FALSE;
        }
        lstrcpyW(wszListPos, wszDosPath);
        wszListPos += lstrlenW(wszListPos)+1;
        HeapFree(GetProcessHeap(), 0, wszDosPath);
    }
    *wszListPos = 0;
    
    ZeroMemory(&op, sizeof(op));
    op.hwnd = GetActiveWindow();
    op.wFunc = FO_DELETE;
    op.pFrom = wszPathsList;
    op.fFlags = FOF_ALLOWUNDO;
    if (!SHFileOperationW(&op))
    {
        WARN("SHFileOperationW failed\n");
        ret = E_FAIL;
    }
    else
        ret = S_OK;

    HeapFree(GetProcessHeap(), 0, wszPathsList);
    return ret;
}

static HRESULT UNIXFS_delete_with_syscalls(UnixFolder *This, UINT cidl, LPCITEMIDLIST *apidl)
{
    char szAbsolute[FILENAME_MAX], *pszRelative;
    static const WCHAR empty[] = {0};
    int i;
    
    if (!SHELL_ConfirmYesNoW(GetActiveWindow(), ASK_DELETE_SELECTED, empty))
        return S_OK;
    
    lstrcpyA(szAbsolute, This->m_pszPath);
    pszRelative = szAbsolute + lstrlenA(szAbsolute);
    
    for (i=0; i<cidl; i++) {
        if (!UNIXFS_filename_from_shitemid(apidl[i], pszRelative))
            return E_INVALIDARG;
        if (_ILIsFolder(apidl[i])) {
            if (rmdir(szAbsolute))
                return E_FAIL;
        } else if (_ILIsValue(apidl[i])) {
            if (unlink(szAbsolute))
                return E_FAIL;
        }
    }
    return S_OK;
}

static HRESULT WINAPI UnixFolder_ISFHelper_DeleteItems(ISFHelper* iface, UINT cidl, 
    LPCITEMIDLIST* apidl)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, ISFHelper, iface);
    char szAbsolute[FILENAME_MAX], *pszRelative;
    LPITEMIDLIST pidlAbsolute;
    HRESULT hr = S_OK;
    UINT i;
    struct stat st;
    
    TRACE("(iface=%p, cidl=%d, apidl=%p)\n", iface, cidl, apidl);

    hr = UNIXFS_delete_with_shfileop(This, cidl, apidl);
    if (hr == S_FALSE)
        hr = UNIXFS_delete_with_syscalls(This, cidl, apidl);

    lstrcpyA(szAbsolute, This->m_pszPath);
    pszRelative = szAbsolute + lstrlenA(szAbsolute);
    
    /* we need to manually send the notifies if the files doesn't exist */
    for (i=0; i<cidl; i++) {
        if (!UNIXFS_filename_from_shitemid(apidl[i], pszRelative))
            continue;
        pidlAbsolute = ILCombine(This->m_pidlLocation, apidl[i]);
        if (stat(szAbsolute, &st))
        {
            if (_ILIsFolder(apidl[i])) {
                SHChangeNotify(SHCNE_RMDIR, SHCNF_IDLIST, pidlAbsolute, NULL);
            } else if (_ILIsValue(apidl[i])) {
                SHChangeNotify(SHCNE_DELETE, SHCNF_IDLIST, pidlAbsolute, NULL);
            }
        }
        ILFree(pidlAbsolute);
    }
        
    return hr;
}

static HRESULT WINAPI UnixFolder_ISFHelper_CopyItems(ISFHelper* iface, IShellFolder *psfFrom, 
    UINT cidl, LPCITEMIDLIST *apidl)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, ISFHelper, iface);
    DWORD dwAttributes;
    UINT i;
    HRESULT hr;
    char szAbsoluteDst[FILENAME_MAX], *pszRelativeDst;
    
    TRACE("(iface=%p, psfFrom=%p, cidl=%d, apidl=%p): semi-stub\n", iface, psfFrom, cidl, apidl);

    if (!psfFrom || !cidl || !apidl)
        return E_INVALIDARG;

    /* All source items have to be filesystem items. */
    dwAttributes = SFGAO_FILESYSTEM;
    hr = IShellFolder_GetAttributesOf(psfFrom, cidl, apidl, &dwAttributes);
    if (FAILED(hr) || !(dwAttributes & SFGAO_FILESYSTEM)) 
        return E_INVALIDARG;

    lstrcpyA(szAbsoluteDst, This->m_pszPath);
    pszRelativeDst = szAbsoluteDst + strlen(szAbsoluteDst);
    
    for (i=0; i<cidl; i++) {
        WCHAR wszSrc[MAX_PATH];
        char szSrc[FILENAME_MAX];
        STRRET strret;

        /* Build the unix path of the current source item. */
        if (FAILED(IShellFolder_GetDisplayNameOf(psfFrom, apidl[i], SHGDN_FORPARSING, &strret)))
            return E_FAIL;
        if (FAILED(StrRetToBufW(&strret, apidl[i], wszSrc, MAX_PATH)))
            return E_FAIL;
        if (!UNIXFS_get_unix_path(wszSrc, szSrc)) 
            return E_FAIL;

        /* Build the unix path of the current destination item */
        UNIXFS_filename_from_shitemid(apidl[i], pszRelativeDst);

        FIXME("Would copy %s to %s. Not yet implemented.\n", szSrc, szAbsoluteDst);
    }
    return S_OK;
}

/* VTable for UnixFolder's ISFHelper interface
 */
static const ISFHelperVtbl UnixFolder_ISFHelper_Vtbl = {
    UnixFolder_ISFHelper_QueryInterface,
    UnixFolder_ISFHelper_AddRef,
    UnixFolder_ISFHelper_Release,
    UnixFolder_ISFHelper_GetUniqueName,
    UnixFolder_ISFHelper_AddFolder,
    UnixFolder_ISFHelper_DeleteItems,
    UnixFolder_ISFHelper_CopyItems
};

static HRESULT WINAPI UnixFolder_IDropTarget_QueryInterface(IDropTarget* iface, REFIID riid, 
    void** ppvObject)
{
    return UnixFolder_IShellFolder2_QueryInterface(
        STATIC_CAST(IShellFolder2, ADJUST_THIS(UnixFolder, IDropTarget, iface)), riid, ppvObject);
}

static ULONG WINAPI UnixFolder_IDropTarget_AddRef(IDropTarget* iface)
{
    return UnixFolder_IShellFolder2_AddRef(
        STATIC_CAST(IShellFolder2, ADJUST_THIS(UnixFolder, IDropTarget, iface)));
}

static ULONG WINAPI UnixFolder_IDropTarget_Release(IDropTarget* iface)
{
    return UnixFolder_IShellFolder2_Release(
        STATIC_CAST(IShellFolder2, ADJUST_THIS(UnixFolder, IDropTarget, iface)));
}

#define HIDA_GetPIDLFolder(pida) (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[0])
#define HIDA_GetPIDLItem(pida, i) (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[i+1])

static HRESULT WINAPI UnixFolder_IDropTarget_DragEnter(IDropTarget *iface, IDataObject *pDataObject,
    DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, IDropTarget, iface);
    FORMATETC format;
    STGMEDIUM medium; 
        
    TRACE("(iface=%p, pDataObject=%p, dwKeyState=%08x, pt={.x=%d, .y=%d}, pdwEffect=%p)\n",
        iface, pDataObject, dwKeyState, pt.x, pt.y, pdwEffect);

    if (!pdwEffect || !pDataObject)
        return E_INVALIDARG;
  
    /* Compute a mask of supported drop-effects for this shellfolder object and the given data 
     * object. Dropping is only supported on folders, which represent filesystem locations. One
     * can't drop on file objects. And the 'move' drop effect is only supported, if the source
     * folder is not identical to the target folder. */
    This->m_dwDropEffectsMask = DROPEFFECT_NONE;
    InitFormatEtc(format, cfShellIDList, TYMED_HGLOBAL);
    if ((This->m_dwAttributes & SFGAO_FILESYSTEM) && /* Only drop to filesystem folders */
        _ILIsFolder(ILFindLastID(This->m_pidlLocation)) && /* Only drop to folders, not to files */
        SUCCEEDED(IDataObject_GetData(pDataObject, &format, &medium))) /* Only ShellIDList format */
    {
        LPIDA pidaShellIDList = GlobalLock(medium.u.hGlobal);
        This->m_dwDropEffectsMask |= DROPEFFECT_COPY|DROPEFFECT_LINK;

        if (pidaShellIDList) { /* Files can only be moved between two different folders */
            if (!ILIsEqual(HIDA_GetPIDLFolder(pidaShellIDList), This->m_pidlLocation))
                This->m_dwDropEffectsMask |= DROPEFFECT_MOVE;
            GlobalUnlock(medium.u.hGlobal);
        }
    }

    *pdwEffect = KeyStateToDropEffect(dwKeyState) & This->m_dwDropEffectsMask;
    
    return S_OK;
}

static HRESULT WINAPI UnixFolder_IDropTarget_DragOver(IDropTarget *iface, DWORD dwKeyState, 
    POINTL pt, DWORD *pdwEffect)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, IDropTarget, iface);
    
    TRACE("(iface=%p, dwKeyState=%08x, pt={.x=%d, .y=%d}, pdwEffect=%p)\n", iface, dwKeyState, 
        pt.x, pt.y, pdwEffect);

    if (!pdwEffect)
        return E_INVALIDARG;

    *pdwEffect = KeyStateToDropEffect(dwKeyState) & This->m_dwDropEffectsMask;
    
    return S_OK;
}

static HRESULT WINAPI UnixFolder_IDropTarget_DragLeave(IDropTarget *iface) {
    UnixFolder *This = ADJUST_THIS(UnixFolder, IDropTarget, iface);
    
    TRACE("(iface=%p)\n", iface);
 
    This->m_dwDropEffectsMask = DROPEFFECT_NONE;
    
    return S_OK;
}

static HRESULT WINAPI UnixFolder_IDropTarget_Drop(IDropTarget *iface, IDataObject *pDataObject,
    DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, IDropTarget, iface);
    FORMATETC format;
    STGMEDIUM medium; 
    HRESULT hr;

    TRACE("(iface=%p, pDataObject=%p, dwKeyState=%d, pt={.x=%d, .y=%d}, pdwEffect=%p) semi-stub\n",
        iface, pDataObject, dwKeyState, pt.x, pt.y, pdwEffect);

    InitFormatEtc(format, cfShellIDList, TYMED_HGLOBAL);
    hr = IDataObject_GetData(pDataObject, &format, &medium);
    if (!SUCCEEDED(hr)) 
        return hr;

    if (medium.tymed == TYMED_HGLOBAL) {
        IShellFolder *psfSourceFolder, *psfDesktopFolder;
        LPIDA pidaShellIDList = GlobalLock(medium.u.hGlobal);
        STRRET strret;
        UINT i;
    
        if (!pidaShellIDList) 
            return HRESULT_FROM_WIN32(GetLastError());
        
        hr = SHGetDesktopFolder(&psfDesktopFolder);
        if (FAILED(hr)) {
            GlobalUnlock(medium.u.hGlobal);
            return hr;
        }

        hr = IShellFolder_BindToObject(psfDesktopFolder, HIDA_GetPIDLFolder(pidaShellIDList), NULL, 
                                       &IID_IShellFolder, (LPVOID*)&psfSourceFolder);
        IShellFolder_Release(psfDesktopFolder);
        if (FAILED(hr)) {
            GlobalUnlock(medium.u.hGlobal);
            return hr;
        }

        for (i = 0; i < pidaShellIDList->cidl; i++) {
            WCHAR wszSourcePath[MAX_PATH];

            hr = IShellFolder_GetDisplayNameOf(psfSourceFolder, HIDA_GetPIDLItem(pidaShellIDList, i),
                                               SHGDN_FORPARSING, &strret);
            if (FAILED(hr)) 
                break;

            hr = StrRetToBufW(&strret, NULL, wszSourcePath, MAX_PATH);
            if (FAILED(hr)) 
                break;

            switch (*pdwEffect) {
                case DROPEFFECT_MOVE:
                    FIXME("Move %s to %s!\n", debugstr_w(wszSourcePath), This->m_pszPath);
                    break;
                case DROPEFFECT_COPY:
                    FIXME("Copy %s to %s!\n", debugstr_w(wszSourcePath), This->m_pszPath);
                    break;
                case DROPEFFECT_LINK:
                    FIXME("Link %s from %s!\n", debugstr_w(wszSourcePath), This->m_pszPath);
                    break;
            }
        }
    
        IShellFolder_Release(psfSourceFolder);
        GlobalUnlock(medium.u.hGlobal);
        return hr;
    }
 
    return E_NOTIMPL;
}

/* VTable for UnixFolder's IDropTarget interface
 */
static const IDropTargetVtbl UnixFolder_IDropTarget_Vtbl = {
    UnixFolder_IDropTarget_QueryInterface,
    UnixFolder_IDropTarget_AddRef,
    UnixFolder_IDropTarget_Release,
    UnixFolder_IDropTarget_DragEnter,
    UnixFolder_IDropTarget_DragOver,
    UnixFolder_IDropTarget_DragLeave,
    UnixFolder_IDropTarget_Drop
};

/******************************************************************************
 * Unix[Dos]Folder_Constructor [Internal]
 *
 * PARAMS
 *  pUnkOuter [I] Outer class for aggregation. Currently ignored.
 *  riid      [I] Interface asked for by the client.
 *  ppv       [O] Pointer to an riid interface to the UnixFolder object.
 *
 * NOTES
 *  Those are the only functions exported from shfldr_unixfs.c. They are called from
 *  shellole.c's default class factory and thus have to exhibit a LPFNCREATEINSTANCE
 *  compatible signature.
 *
 *  The UnixDosFolder_Constructor sets the dwPathMode member to PATHMODE_DOS. This
 *  means that paths are converted from dos to unix and back at the interfaces.
 */
static HRESULT CreateUnixFolder(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv, const CLSID *pCLSID) 
{
    HRESULT hr = E_FAIL;
    UnixFolder *pUnixFolder = SHAlloc((ULONG)sizeof(UnixFolder));
   
    if (pUnkOuter) {
        FIXME("Aggregation not yet implemented!\n");
        return CLASS_E_NOAGGREGATION;
    }
    
    if(pUnixFolder) {
        pUnixFolder->lpIShellFolder2Vtbl = &UnixFolder_IShellFolder2_Vtbl;
        pUnixFolder->lpIPersistFolder3Vtbl = &UnixFolder_IPersistFolder3_Vtbl;
        pUnixFolder->lpIPersistPropertyBagVtbl = &UnixFolder_IPersistPropertyBag_Vtbl;
        pUnixFolder->lpISFHelperVtbl = &UnixFolder_ISFHelper_Vtbl;
        pUnixFolder->lpIDropTargetVtbl = &UnixFolder_IDropTarget_Vtbl;
        pUnixFolder->m_cRef = 0;
        pUnixFolder->m_pszPath = NULL;
        pUnixFolder->m_pidlLocation = NULL;
        pUnixFolder->m_dwPathMode = IsEqualCLSID(&CLSID_UnixFolder, pCLSID) ? PATHMODE_UNIX : PATHMODE_DOS;
        pUnixFolder->m_dwAttributes = 0;
        pUnixFolder->m_pCLSID = pCLSID;
        pUnixFolder->m_dwDropEffectsMask = DROPEFFECT_NONE;

        UnixFolder_IShellFolder2_AddRef(STATIC_CAST(IShellFolder2, pUnixFolder));
        hr = UnixFolder_IShellFolder2_QueryInterface(STATIC_CAST(IShellFolder2, pUnixFolder), riid, ppv);
        UnixFolder_IShellFolder2_Release(STATIC_CAST(IShellFolder2, pUnixFolder));
    }
    return hr;
}

HRESULT WINAPI UnixFolder_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv) {
    TRACE("(pUnkOuter=%p, riid=%p, ppv=%p)\n", pUnkOuter, riid, ppv);
    return CreateUnixFolder(pUnkOuter, riid, ppv, &CLSID_UnixFolder);
}

HRESULT WINAPI UnixDosFolder_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv) {
    TRACE("(pUnkOuter=%p, riid=%p, ppv=%p)\n", pUnkOuter, riid, ppv);
    return CreateUnixFolder(pUnkOuter, riid, ppv, &CLSID_UnixDosFolder);
}

HRESULT WINAPI FolderShortcut_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv) {
    TRACE("(pUnkOuter=%p, riid=%p, ppv=%p)\n", pUnkOuter, riid, ppv);
    return CreateUnixFolder(pUnkOuter, riid, ppv, &CLSID_FolderShortcut);
}

HRESULT WINAPI MyDocuments_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv) {
    TRACE("(pUnkOuter=%p, riid=%p, ppv=%p)\n", pUnkOuter, riid, ppv);
    return CreateUnixFolder(pUnkOuter, riid, ppv, &CLSID_MyDocuments);
}

HRESULT WINAPI ShellFSFolder_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv) {
    TRACE("(pUnkOuter=%p, riid=%p, ppv=%p)\n", pUnkOuter, riid, ppv);
    return CreateUnixFolder(pUnkOuter, riid, ppv, &CLSID_ShellFSFolder);
}

/******************************************************************************
 * UnixSubFolderIterator
 *
 * Class whose heap based objects represent iterators over the sub-directories
 * of a given UnixFolder object. 
 */

/* UnixSubFolderIterator object layout and typedef.
 */
typedef struct _UnixSubFolderIterator {
    const IEnumIDListVtbl *lpIEnumIDListVtbl;
    LONG m_cRef;
    SHCONTF m_fFilter;
    DIR *m_dirFolder;
    char m_szFolder[FILENAME_MAX];
} UnixSubFolderIterator;

static void UnixSubFolderIterator_Destroy(UnixSubFolderIterator *iterator) {
    TRACE("(iterator=%p)\n", iterator);

    if (iterator->m_dirFolder)
        closedir(iterator->m_dirFolder);
    SHFree(iterator);
}

static HRESULT WINAPI UnixSubFolderIterator_IEnumIDList_QueryInterface(IEnumIDList* iface, 
    REFIID riid, void** ppv)
{
    TRACE("(iface=%p, riid=%p, ppv=%p)\n", iface, riid, ppv);
    
    if (!ppv) return E_INVALIDARG;
    
    if (IsEqualIID(&IID_IUnknown, riid) || IsEqualIID(&IID_IEnumIDList, riid)) {
        *ppv = iface;
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IEnumIDList_AddRef(iface);
    return S_OK;
}
                            
static ULONG WINAPI UnixSubFolderIterator_IEnumIDList_AddRef(IEnumIDList* iface)
{
    UnixSubFolderIterator *This = ADJUST_THIS(UnixSubFolderIterator, IEnumIDList, iface);

    TRACE("(iface=%p)\n", iface);
   
    return InterlockedIncrement(&This->m_cRef);
}

static ULONG WINAPI UnixSubFolderIterator_IEnumIDList_Release(IEnumIDList* iface)
{
    UnixSubFolderIterator *This = ADJUST_THIS(UnixSubFolderIterator, IEnumIDList, iface);
    ULONG cRef;
    
    TRACE("(iface=%p)\n", iface);

    cRef = InterlockedDecrement(&This->m_cRef);
    
    if (!cRef) 
        UnixSubFolderIterator_Destroy(This);

    return cRef;
}

static HRESULT WINAPI UnixSubFolderIterator_IEnumIDList_Next(IEnumIDList* iface, ULONG celt, 
    LPITEMIDLIST* rgelt, ULONG* pceltFetched)
{
    UnixSubFolderIterator *This = ADJUST_THIS(UnixSubFolderIterator, IEnumIDList, iface);
    ULONG i = 0;

    /* This->m_dirFolder will be NULL if the user doesn't have access rights for the dir. */
    if (This->m_dirFolder) {
        char *pszRelativePath = This->m_szFolder + lstrlenA(This->m_szFolder);
        struct dirent *pDirEntry;

        while (i < celt) {
            pDirEntry = readdir(This->m_dirFolder);
            if (!pDirEntry) break; /* No more entries */
            if (!strcmp(pDirEntry->d_name, ".") || !strcmp(pDirEntry->d_name, "..")) continue;

            /* Temporarily build absolute path in This->m_szFolder. Then construct a pidl
             * and see if it passes the filter. 
             */
            lstrcpyA(pszRelativePath, pDirEntry->d_name);
            rgelt[i] = (LPITEMIDLIST)SHAlloc(
                UNIXFS_shitemid_len_from_filename(pszRelativePath, NULL, NULL)+sizeof(USHORT));
            if (!UNIXFS_build_shitemid(This->m_szFolder, rgelt[i]) ||
                !UNIXFS_is_pidl_of_type(rgelt[i], This->m_fFilter)) 
            {
                SHFree(rgelt[i]);
                continue;
            }
            memset(((PBYTE)rgelt[i])+rgelt[i]->mkid.cb, 0, sizeof(USHORT));
            i++;
        }
        *pszRelativePath = '\0'; /* Restore the original path in This->m_szFolder. */
    }
    
    if (pceltFetched)
        *pceltFetched = i;

    return (i == 0) ? S_FALSE : S_OK;
}
    
static HRESULT WINAPI UnixSubFolderIterator_IEnumIDList_Skip(IEnumIDList* iface, ULONG celt)
{
    LPITEMIDLIST *apidl;
    ULONG cFetched;
    HRESULT hr;
    
    TRACE("(iface=%p, celt=%d)\n", iface, celt);

    /* Call IEnumIDList::Next and delete the resulting pidls. */
    apidl = (LPITEMIDLIST*)SHAlloc(celt * sizeof(LPITEMIDLIST));
    hr = IEnumIDList_Next(iface, celt, apidl, &cFetched);
    if (SUCCEEDED(hr))
        while (cFetched--) 
            SHFree(apidl[cFetched]);
    SHFree(apidl);

    return hr;
}

static HRESULT WINAPI UnixSubFolderIterator_IEnumIDList_Reset(IEnumIDList* iface)
{
    UnixSubFolderIterator *This = ADJUST_THIS(UnixSubFolderIterator, IEnumIDList, iface);
        
    TRACE("(iface=%p)\n", iface);

    if (This->m_dirFolder)
        rewinddir(This->m_dirFolder);
    
    return S_OK;
}

static HRESULT WINAPI UnixSubFolderIterator_IEnumIDList_Clone(IEnumIDList* This, 
    IEnumIDList** ppenum)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

/* VTable for UnixSubFolderIterator's IEnumIDList interface.
 */
static const IEnumIDListVtbl UnixSubFolderIterator_IEnumIDList_Vtbl = {
    UnixSubFolderIterator_IEnumIDList_QueryInterface,
    UnixSubFolderIterator_IEnumIDList_AddRef,
    UnixSubFolderIterator_IEnumIDList_Release,
    UnixSubFolderIterator_IEnumIDList_Next,
    UnixSubFolderIterator_IEnumIDList_Skip,
    UnixSubFolderIterator_IEnumIDList_Reset,
    UnixSubFolderIterator_IEnumIDList_Clone
};

static IUnknown *UnixSubFolderIterator_Constructor(UnixFolder *pUnixFolder, SHCONTF fFilter) {
    UnixSubFolderIterator *iterator;

    TRACE("(pUnixFolder=%p)\n", pUnixFolder);
    
    iterator = SHAlloc((ULONG)sizeof(UnixSubFolderIterator));
    iterator->lpIEnumIDListVtbl = &UnixSubFolderIterator_IEnumIDList_Vtbl;
    iterator->m_cRef = 0;
    iterator->m_fFilter = fFilter;
    iterator->m_dirFolder = opendir(pUnixFolder->m_pszPath);
    lstrcpyA(iterator->m_szFolder, pUnixFolder->m_pszPath);

    UnixSubFolderIterator_IEnumIDList_AddRef((IEnumIDList*)iterator);
    
    return (IUnknown*)iterator;
}
