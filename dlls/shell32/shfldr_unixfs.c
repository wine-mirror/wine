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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

const GUID CLSID_UnixFolder = {0xcc702eb2, 0x7dc5, 0x11d9, {0xc6, 0x87, 0x00, 0x04, 0x23, 0x8a, 0x01, 0xcd}};
const GUID CLSID_UnixDosFolder = {0x9d20aae8, 0x0625, 0x44b0, {0x9c, 0xa7, 0x71, 0x88, 0x9c, 0x22, 0x54, 0xd9}};

#define ADJUST_THIS(c,m,p) ((c*)(((long)p)-(long)&(((c*)0)->lp##m##Vtbl)))
#define STATIC_CAST(i,p) ((i*)&p->lp##i##Vtbl)

/* FileStruct reserves one byte for szNames, thus we don't have to 
 * alloc a byte for the terminating '\0' of 'name'. Two of the
 * additional bytes are for SHITEMID's cb field. One is for IDLDATA's
 * type field. One is for FileStruct's szNames field, to terminate
 * the alternate DOS name, which we don't use here.
 */
#define SHITEMID_LEN_FROM_NAME_LEN(n) \
    (sizeof(USHORT)+sizeof(PIDLTYPE)+sizeof(FileStruct)+(n)+sizeof(char))
#define NAME_LEN_FROM_LPSHITEMID(s) \
    (((LPSHITEMID)s)->cb-sizeof(USHORT)-sizeof(PIDLTYPE)-sizeof(FileStruct)-sizeof(char))

#define PATHMODE_UNIX 0
#define PATHMODE_DOS  1

/* UnixFolder object layout and typedef.
 */
typedef struct _UnixFolder {
    const IShellFolder2Vtbl  *lpIShellFolder2Vtbl;
    const IPersistFolder3Vtbl *lpIPersistFolder3Vtbl;
    const IPersistPropertyBagVtbl *lpIPersistPropertyBagVtbl;
    const ISFHelperVtbl *lpISFHelperVtbl;
    LONG m_cRef;
    CHAR *m_pszPath;             /* Target path of the shell folder */
    LPITEMIDLIST m_pidlLocation; /* Location in the shell namespace */
    DWORD m_dwPathMode;
    DWORD m_dwAttributes;
    const CLSID *m_pCLSID;
} UnixFolder;

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
 * UNIXFS_is_pidl_of_type [INTERNAL]
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
 * UNIXFS_is_dos_device [Internal]
 *
 * Determines if a unix directory corresponds to any dos device.
 *
 * PARAMS
 *  statPath [I] The stat struct of the directory, as returned by stat(2).
 *
 * RETURNS
 *  TRUE, if statPath corresponds to any dos drive letter
 *  FALSE, otherwise
 */
static BOOL UNIXFS_is_dos_device(const struct stat *statPath) {
    struct stat statDrive;
    char *pszDrivePath;
    DWORD dwDriveMap;
    WCHAR wszDosDevice[4] = { 'A', ':', '\\', 0 };

    for (dwDriveMap = GetLogicalDrives(); dwDriveMap; dwDriveMap >>= 1, wszDosDevice[0]++) {
        if (!(dwDriveMap & 0x1)) continue;
        pszDrivePath = wine_get_unix_file_name(wszDosDevice);
        if (pszDrivePath && !stat(pszDrivePath, &statDrive)) {
            HeapFree(GetProcessHeap(), 0, pszDrivePath);
            if ((statPath->st_dev == statDrive.st_dev) && (statPath->st_ino == statDrive.st_ino))
                return TRUE;
        }
    }
    return FALSE;
}

/******************************************************************************
 * UNIXFS_get_unix_path [Internal]
 *
 * Convert an absolute dos path to an absolute canonicalized unix path.
 * Evaluate "/.", "/.." and symbolic links.
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
    struct stat fileStat;
    
    TRACE("(pszDosPath=%s, pszCanonicalPath=%p)\n", debugstr_w(pszDosPath), pszCanonicalPath);
    
    if (!pszDosPath || pszDosPath[1] != ':')
        return FALSE;

    pszUnixPath = wine_get_unix_file_name(pszDosPath);
    if (!pszUnixPath) return FALSE;   
    strcpy(szPath, pszUnixPath);
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
        int cLinks = 0;
            
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
            continue;
        }

        /* Remove last element in canonical path for "/.." elements, then skip. */
        if (!strcmp("/..", pElement)) {
            char *pTemp = strrchr(pszCanonicalPath, '/');
            if (pTemp)
                pCanonicalTail = pTemp;
            *pCanonicalTail = '\0';
            *pPathTail = cTemp;
            continue;
        }
       
        /* lstat returns zero on success. */
        if (lstat(szPath, &fileStat)) 
            return FALSE;

        if (S_ISLNK(fileStat.st_mode)) {
            char szSymlink[FILENAME_MAX];
            int cLinkLen, cTailLen;
          
            /* Avoid infinite loop for recursive links. */
            if (++cLinks > 64) 
                return FALSE;
            
            cLinkLen = readlink(szPath, szSymlink, FILENAME_MAX);
            if (cLinkLen < 0) 
                return FALSE;

            *pPathTail = cTemp;
            cTailLen = strlen(pPathTail);
           
            if (szSymlink[0] == '/') {
                /* Absolute link. Copy to szPath, concat remaining path and start all over. */
                if (cLinkLen + cTailLen + 1 > FILENAME_MAX)
                    return FALSE;
                    
                /* Avoid double slashes. */
                if (szSymlink[cLinkLen-1] == '/' && pPathTail[0] == '/') {
                    szSymlink[cLinkLen-1] = '\0';
                    cLinkLen--;
                }
                
                memcpy(szSymlink + cLinkLen, pPathTail, cTailLen + 1);
                memcpy(szPath, szSymlink, cLinkLen + cTailLen + 1);
                *pszCanonicalPath = '\0';
                pCanonicalTail = pszCanonicalPath;
                pPathTail = szPath;
            } else {
                /* Relative link. Expand into szPath and continue. */
                char szTemp[FILENAME_MAX];
                int cTailLen = strlen(pPathTail);

                if (pElement - szPath + 1 + cLinkLen + cTailLen + 1 > FILENAME_MAX)
                    return FALSE;

                memcpy(szTemp, pPathTail, cTailLen + 1);
                memcpy(pElement + 1, szSymlink, cLinkLen);
                memcpy(pElement + 1 + cLinkLen, szTemp, cTailLen + 1);
                pPathTail = pElement;
            }
        } else {
            /* Regular directory or file. Copy to canonical path */
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
    LARGE_INTEGER time;
    FILETIME fileTime;
    LPPIDLDATA pIDLData;
    struct stat fileStat;
    char *pszComponent;
    int cComponentLen;

    TRACE("(pszUnixPath=%s, pIDL=%p)\n", debugstr_a(pszUnixPath), pIDL);

    /* Compute the SHITEMID's length and wipe it. */
    pszComponent = strrchr(pszUnixPath, '/') + 1;
    cComponentLen = strlen(pszComponent);
    memset(pIDL, 0, SHITEMID_LEN_FROM_NAME_LEN(cComponentLen));
    ((LPSHITEMID)pIDL)->cb = SHITEMID_LEN_FROM_NAME_LEN(cComponentLen) ;
   
    /* We are only interested in regular files and directories. */
    if (stat(pszUnixPath, &fileStat)) return NULL;
    if (!S_ISDIR(fileStat.st_mode) && !S_ISREG(fileStat.st_mode)) return NULL;
    
    /* Set shell32's standard SHITEMID data fields. */
    pIDLData = _ILGetDataPointer((LPCITEMIDLIST)pIDL);
    pIDLData->type = S_ISDIR(fileStat.st_mode) ? PT_FOLDER : PT_VALUE;
    pIDLData->u.file.dwFileSize = (DWORD)fileStat.st_size;
    RtlSecondsSince1970ToTime( fileStat.st_mtime, &time );
    fileTime.dwLowDateTime = time.u.LowPart;
    fileTime.dwHighDateTime = time.u.HighPart;
    FileTimeToDosDateTime(&fileTime, &pIDLData->u.file.uFileDate, &pIDLData->u.file.uFileTime);
    pIDLData->u.file.uFileAttribs = 0;
    if (S_ISDIR(fileStat.st_mode)) pIDLData->u.file.uFileAttribs |= FILE_ATTRIBUTE_DIRECTORY;
    if (pszComponent[0] == '.') pIDLData->u.file.uFileAttribs |=  FILE_ATTRIBUTE_HIDDEN;
    memcpy(pIDLData->u.file.szNames, pszComponent, cComponentLen);

    return pszComponent + cComponentLen;
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
    int cSubDirs, cPidlLen, cPathLen;
    char *pSlash, szCompletePath[FILENAME_MAX], *pNextPathElement;

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
    
    /* Count the number of sub-directories in the path */
    cSubDirs = 0;
    pSlash = pNextPathElement;
    while (pSlash) {
        cSubDirs++;
        pSlash = strchr(pSlash+1, '/');
    }
  
    /* Allocate enough memory to hold the path. The -cSubDirs is for the '/' 
     * characters, which are not stored in the ITEMIDLIST. */
    cPidlLen = strlen(pNextPathElement) - cSubDirs + cSubDirs * SHITEMID_LEN_FROM_NAME_LEN(0) + sizeof(USHORT);
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
    struct stat statPrefix;
    char *pNextDir;
        
    /* Determine the path's length bytes */
    while (current && current->mkid.cb) {
        dwPathLen += NAME_LEN_FROM_LPSHITEMID(current) + 1; /* For the '/' */
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
    if (!(This->m_dwAttributes & SFGAO_FILESYSTEM)) {
        *pNextDir = '\0';
        if (!stat(This->m_pszPath, &statPrefix) && UNIXFS_is_dos_device(&statPrefix))
            This->m_dwAttributes |= SFGAO_FILESYSTEM;
    }
    while (current && current->mkid.cb) {
        memcpy(pNextDir, _ILGetTextPointer(current), NAME_LEN_FROM_LPSHITEMID(current));
        pNextDir += NAME_LEN_FROM_LPSHITEMID(current);
        if (!(This->m_dwAttributes & SFGAO_FILESYSTEM)) {
            *pNextDir = '\0';
            if (!stat(This->m_pszPath, &statPrefix) && UNIXFS_is_dos_device(&statPrefix))
                This->m_dwAttributes |= SFGAO_FILESYSTEM;
        }
        *pNextDir++ = '/';
        current = ILGetNext(current);
    }
    *pNextDir='\0';
 
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
        *ppv = &This->lpIShellFolder2Vtbl;
    } else if (IsEqualIID(&IID_IPersistFolder3, riid) || IsEqualIID(&IID_IPersistFolder2, riid) || 
               IsEqualIID(&IID_IPersistFolder, riid) || IsEqualIID(&IID_IPersist, riid)) 
    {
        *ppv = &This->lpIPersistFolder3Vtbl;
    } else if (IsEqualIID(&IID_IPersistPropertyBag, riid)) {
        *ppv = &This->lpIPersistPropertyBagVtbl;
    } else if (IsEqualIID(&IID_ISFHelper, riid)) {
        *ppv = &This->lpISFHelperVtbl;
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
        HRESULT hr;
        
        hr = SHBindToParent(*ppidl, &IID_IShellFolder, (LPVOID*)&pParentSF, &pidlLast);
        if (FAILED(hr)) return E_FAIL;
        IShellFolder_GetAttributesOf(pParentSF, 1, &pidlLast, pdwAttributes);
        IShellFolder_Release(pParentSF);
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
    
    TRACE("(iface=%p, hwndOwner=%p, grfFlags=%08lx, ppEnumIDList=%p)\n", 
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
                             _ILGetTextPointer(pidl1), NAME_LEN_FROM_LPSHITEMID(pidl1),
                             _ILGetTextPointer(pidl2), NAME_LEN_FROM_LPSHITEMID(pidl2));
    
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
                struct stat fileStat;
                char *pszName = _ILGetTextPointer(apidl[i]);
                if (!pszName) return E_INVALIDARG;
                lstrcpyA(pszRelativePath, pszName);
                if (stat(szAbsolutePath, &fileStat) || !UNIXFS_is_dos_device(&fileStat))
                    *rgfInOut &= ~SFGAO_FILESYSTEM;
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
    
    TRACE("(iface=%p, hwndOwner=%p, cidl=%d, apidl=%p, riid=%s, prgfInOut=%p, ppv=%p)\n",
        iface, hwndOwner, cidl, apidl, debugstr_guid(riid), prgfInOut, ppvOut);

    if (IsEqualIID(&IID_IContextMenu, riid)) {
        *ppvOut = ISvItemCm_Constructor((IShellFolder*)iface, This->m_pidlLocation, apidl, cidl);
        return S_OK;
    } else if (IsEqualIID(&IID_IDataObject, riid)) {
        *ppvOut = IDataObject_Constructor(hwndOwner, This->m_pidlLocation, apidl, cidl);
        return S_OK;
    } else if (IsEqualIID(&IID_IExtractIconA, riid)) {
        LPITEMIDLIST pidl;
        if (cidl != 1) return E_FAIL;
        pidl = ILCombine(This->m_pidlLocation, apidl[0]);
        *ppvOut = (LPVOID)IExtractIconA_Constructor(pidl);
        SHFree(pidl);
        return S_OK;
    } else if (IsEqualIID(&IID_IExtractIconW, riid)) {
        LPITEMIDLIST pidl;
        if (cidl != 1) return E_FAIL;
        pidl = ILCombine(This->m_pidlLocation, apidl[0]);
        *ppvOut = (LPVOID)IExtractIconW_Constructor(pidl);
        SHFree(pidl);
        return S_OK;
    } else if (IsEqualIID(&IID_IDropTarget, riid)) {
        FIXME("IDropTarget\n");
        return E_FAIL;
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

/******************************************************************************
 * Translate file name from unix to ANSI encoding.
 */
static void strcpyn_U2A(char *win_fn, UINT win_fn_len, const char *unix_fn)
{
    UINT len;
    WCHAR *unicode_fn;

    len = MultiByteToWideChar(CP_UNIXCP, 0, unix_fn, -1, NULL, 0);
    unicode_fn = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    MultiByteToWideChar(CP_UNIXCP, 0, unix_fn, -1, unicode_fn, len);

    WideCharToMultiByte(CP_ACP, 0, unicode_fn, len, win_fn, win_fn_len, NULL, NULL);
    HeapFree(GetProcessHeap(), 0, unicode_fn);
}

static HRESULT WINAPI UnixFolder_IShellFolder2_GetDisplayNameOf(IShellFolder2* iface, 
    LPCITEMIDLIST pidl, SHGDNF uFlags, STRRET* lpName)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, IShellFolder2, iface);
    HRESULT hr = S_OK;    

    TRACE("(iface=%p, pidl=%p, uFlags=%lx, lpName=%p)\n", iface, pidl, uFlags, lpName);
    
    if ((GET_SHGDN_FOR(uFlags) & SHGDN_FORPARSING) &&
        (GET_SHGDN_RELATION(uFlags) != SHGDN_INFOLDER))
    {
        if (!pidl || !pidl->mkid.cb) {
            lpName->uType = STRRET_CSTR;
            if (This->m_dwPathMode == PATHMODE_UNIX) {
                strcpyn_U2A(lpName->u.cStr, MAX_PATH, This->m_pszPath);
            } else {
                WCHAR *pwszDosPath = wine_get_dos_file_name(This->m_pszPath);
                if (!pwszDosPath)
                    return HRESULT_FROM_WIN32(GetLastError());
                PathRemoveBackslashW(pwszDosPath);
                WideCharToMultiByte(CP_ACP, 0, pwszDosPath, -1, lpName->u.cStr, MAX_PATH, NULL, NULL);
                HeapFree(GetProcessHeap(), 0, pwszDosPath);        
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
        char *pszFileName = _ILGetTextPointer(pidl);
        lpName->uType = STRRET_CSTR;
        strcpyn_U2A(lpName->u.cStr, MAX_PATH, pszFileName ? pszFileName : "");
    }

    /* If in dos mode, do some post-processing on the path.
     * (e.g. remove filename extension, if uFlags & SHGDN_FOREDITING) 
     */
    if (SUCCEEDED(hr) && This->m_dwPathMode == PATHMODE_DOS && !_ILIsFolder(pidl))
        SHELL_FS_ProcessDisplayFilename(lpName->u.cStr, uFlags);
    
    TRACE("--> %s\n", lpName->u.cStr);
    
    return hr;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_SetNameOf(IShellFolder2* iface, HWND hwnd, 
    LPCITEMIDLIST pidl, LPCOLESTR lpszName, SHGDNF uFlags, LPITEMIDLIST* ppidlOut)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, IShellFolder2, iface);

    char szSrc[FILENAME_MAX], szDest[FILENAME_MAX];
    WCHAR *pwszDosDest;
    int cBasePathLen = lstrlenA(This->m_pszPath);
    struct stat statDest;
    LPITEMIDLIST pidlSrc, pidlDest;
   
    TRACE("(iface=%p, hwnd=%p, pidl=%p, lpszName=%s, uFlags=0x%08lx, ppidlOut=%p)\n",
          iface, hwnd, pidl, debugstr_w(lpszName), uFlags, ppidlOut); 

    /* pidl has to contain a single non-empty SHITEMID */
    if (_ILIsDesktop(pidl) || !_ILIsPidlSimple(pidl) || !_ILGetTextPointer(pidl))
        return E_INVALIDARG;
   
    if (ppidlOut)
        *ppidlOut = NULL;
    
    /* build source path */
    memcpy(szSrc, This->m_pszPath, cBasePathLen);
    lstrcpyA(szSrc+cBasePathLen, _ILGetTextPointer(pidl));

    /* build destination path */
    if (uFlags & SHGDN_FORPARSING) { /* absolute path in lpszName */
        WideCharToMultiByte(CP_UNIXCP, 0, lpszName, -1, szDest, FILENAME_MAX, NULL, NULL);
    } else {
        WCHAR wszSrcRelative[MAX_PATH];
        memcpy(szDest, This->m_pszPath, cBasePathLen);
        WideCharToMultiByte(CP_UNIXCP, 0, lpszName, -1, szDest+cBasePathLen, 
                            FILENAME_MAX-cBasePathLen, NULL, NULL);

        /* uFlags is SHGDN_FOREDITING of SHGDN_FORADDRESSBAR. If the filename's 
         * extension is hidden to the user, we have to append it. */
        if (_ILSimpleGetTextW(pidl, wszSrcRelative, MAX_PATH) && 
            SHELL_FS_HideExtension(wszSrcRelative))
        {
            char *pszExt = PathFindExtensionA(_ILGetTextPointer(pidl));
            lstrcatA(szDest, pszExt);
        }
    } 

    TRACE("src=%s dest=%s\n", szSrc, szDest);

    /* Fail, if destination does already exist */
    if (!stat(szDest, &statDest)) 
        return E_FAIL;

    /* Rename the file */
    if (rename(szSrc, szDest)) 
        return E_FAIL;
    
    /* Build a pidl for the path of the renamed file */
    pwszDosDest = wine_get_dos_file_name(szDest);
    if (!pwszDosDest || !UNIXFS_path_to_pidl(This, pwszDosDest, &pidlDest)) {
        HeapFree(GetProcessHeap(), 0, pwszDosDest);
        rename(szDest, szSrc); /* Undo the renaming */
        return E_FAIL;
    }
    
    /* Inform the shell */
    pidlSrc = ILCombine(This->m_pidlLocation, pidl);
    if (_ILIsFolder(ILFindLastID(pidlDest))) 
        SHChangeNotify(SHCNE_RENAMEFOLDER, SHCNF_IDLIST, pidlSrc, pidlDest);
    else 
        SHChangeNotify(SHCNE_RENAMEITEM, SHCNF_IDLIST, pidlSrc, pidlDest);
    ILFree(pidlSrc);
    ILFree(pidlDest);
    
    if (ppidlOut) 
        _ILCreateFromPathW(pwszDosDest, ppidlOut);
    
    HeapFree(GetProcessHeap(), 0, pwszDosDest);
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
            char szPath[FILENAME_MAX], *pszFile = _ILGetTextPointer(pidl);
            if (!pszFile) 
                return E_INVALIDARG;
            lstrcpyA(szPath, This->m_pszPath);
            lstrcatA(szPath, pszFile);
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

static HRESULT WINAPI UnixFolder_IPersistFolder3_QueryInterface(IPersistFolder3* This, REFIID riid, 
    void** ppvObject)
{
    return UnixFolder_IShellFolder2_QueryInterface(
                (IShellFolder2*)ADJUST_THIS(UnixFolder, IPersistFolder3, This), riid, ppvObject);
}

static ULONG WINAPI UnixFolder_IPersistFolder3_AddRef(IPersistFolder3* This)
{
    return UnixFolder_IShellFolder2_AddRef(
                (IShellFolder2*)ADJUST_THIS(UnixFolder, IPersistFolder3, This));
}

static ULONG WINAPI UnixFolder_IPersistFolder3_Release(IPersistFolder3* This)
{
    return UnixFolder_IShellFolder2_Release(
                (IShellFolder2*)ADJUST_THIS(UnixFolder, IPersistFolder3, This));
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
        if (_ILIsSpecialFolder(current) && IsEqualIID(This->m_pCLSID, _ILGetGUIDPointer(current)))
            break;
        current = ILGetNext(current);
    }

    if (current && current->mkid.cb) {
        if (IsEqualIID(&CLSID_MyDocuments, _ILGetGUIDPointer(current))) {
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

static HRESULT WINAPI UnixFolder_IPersistPropertyBag_QueryInterface(IPersistPropertyBag* This,
    REFIID riid, void** ppvObject)
{
    return UnixFolder_IShellFolder2_QueryInterface(
                (IShellFolder2*)ADJUST_THIS(UnixFolder, IPersistPropertyBag, This), riid, ppvObject);
}

static ULONG WINAPI UnixFolder_IPersistPropertyBag_AddRef(IPersistPropertyBag* This)
{
    return UnixFolder_IShellFolder2_AddRef(
                (IShellFolder2*)ADJUST_THIS(UnixFolder, IPersistPropertyBag, This));
}

static ULONG WINAPI UnixFolder_IPersistPropertyBag_Release(IPersistPropertyBag* This)
{
    return UnixFolder_IShellFolder2_Release(
                (IShellFolder2*)ADJUST_THIS(UnixFolder, IPersistPropertyBag, This));
}

static HRESULT WINAPI UnixFolder_IPersistPropertyBag_GetClassID(IPersistPropertyBag* iface, 
    CLSID* pClassID)
{
    return UnixFolder_IPersistFolder3_GetClassID(
        (IPersistFolder3*)&ADJUST_THIS(UnixFolder, IPersistPropertyBag, iface)->lpIPersistFolder3Vtbl,
        pClassID);
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
                (IShellFolder2*)ADJUST_THIS(UnixFolder, ISFHelper, iface), riid, ppvObject);
}

static ULONG WINAPI UnixFolder_ISFHelper_AddRef(ISFHelper* iface)
{
    return UnixFolder_IShellFolder2_AddRef(
                (IShellFolder2*)ADJUST_THIS(UnixFolder, ISFHelper, iface));
}

static ULONG WINAPI UnixFolder_ISFHelper_Release(ISFHelper* iface)
{
    return UnixFolder_IShellFolder2_Release(
                (IShellFolder2*)ADJUST_THIS(UnixFolder, ISFHelper, iface));
}

static HRESULT WINAPI UnixFolder_ISFHelper_GetUniqueName(ISFHelper* iface, LPSTR lpName, UINT uLen)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, ISFHelper, iface);
    IEnumIDList *pEnum;
    HRESULT hr;
    LPITEMIDLIST pidlElem;
    DWORD dwFetched;
    int i;
    static const char szNewFolder[] = "New Folder";

    TRACE("(iface=%p, lpName=%p, uLen=%u)\n", iface, lpName, uLen);
    
    if (uLen < sizeof(szNewFolder)+3)
        return E_INVALIDARG;

    hr = IShellFolder2_EnumObjects(STATIC_CAST(IShellFolder2, This), 0,
                                   SHCONTF_FOLDERS|SHCONTF_NONFOLDERS|SHCONTF_INCLUDEHIDDEN, &pEnum);
    if (SUCCEEDED(hr)) {
        lstrcpyA(lpName, szNewFolder);
        IEnumIDList_Reset(pEnum);
        i = 2;
        while ((IEnumIDList_Next(pEnum, 1, &pidlElem, &dwFetched) == S_OK) && (dwFetched == 1)) {
            if (!strcasecmp(_ILGetTextPointer(pidlElem), lpName)) {
                IEnumIDList_Reset(pEnum);
                sprintf(lpName, "%s %d", szNewFolder, i++);
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

static HRESULT WINAPI UnixFolder_ISFHelper_AddFolder(ISFHelper* iface, HWND hwnd, LPCSTR pszName, 
    LPITEMIDLIST* ppidlOut)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, ISFHelper, iface);
    char szNewDir[FILENAME_MAX];

    TRACE("(iface=%p, hwnd=%p, pszName=%s, ppidlOut=%p)\n", iface, hwnd, pszName, ppidlOut);

    if (ppidlOut)
        *ppidlOut = NULL;
    
    lstrcpyA(szNewDir, This->m_pszPath);
    lstrcatA(szNewDir, pszName);

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
        WCHAR wszName[MAX_PATH];

        /* Inform the shell */
        MultiByteToWideChar(CP_UNIXCP, 0, pszName, -1, wszName, MAX_PATH);
        if (UNIXFS_path_to_pidl(This, wszName, &pidlRelative)) {
            LPITEMIDLIST pidlAbsolute = ILCombine(This->m_pidlLocation, pidlRelative);
            if (ppidlOut)
                *ppidlOut = pidlRelative;
            else
                ILFree(pidlRelative);
            SHChangeNotify(SHCNE_MKDIR, SHCNF_IDLIST, pidlAbsolute, NULL);
            ILFree(pidlAbsolute);
        }
        return S_OK;
    }
}

static HRESULT WINAPI UnixFolder_ISFHelper_DeleteItems(ISFHelper* iface, UINT cidl, 
    LPCITEMIDLIST* apidl)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, ISFHelper, iface);
    char szAbsolute[FILENAME_MAX], *pszRelative;
    LPITEMIDLIST pidlAbsolute;
    HRESULT hr = S_OK;
    UINT i;
    
    TRACE("(iface=%p, cidl=%d, apidl=%p)\n", iface, cidl, apidl);

    lstrcpyA(szAbsolute, This->m_pszPath);
    pszRelative = szAbsolute + lstrlenA(szAbsolute);
    
    for (i=0; i<cidl && SUCCEEDED(hr); i++) {
        lstrcpyA(pszRelative, _ILGetTextPointer(apidl[i]));
        pidlAbsolute = ILCombine(This->m_pidlLocation, apidl[i]);
        if (_ILIsFolder(apidl[i])) {
            if (rmdir(szAbsolute)) {
                hr = E_FAIL;
            } else {
                SHChangeNotify(SHCNE_RMDIR, SHCNF_IDLIST, pidlAbsolute, NULL);
            }
        } else if (_ILIsValue(apidl[i])) {
            if (unlink(szAbsolute)) {
                hr = E_FAIL;
            } else {
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
        lstrcpyA(pszRelativeDst, _ILGetTextPointer(apidl[i]));

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
        pUnixFolder->m_cRef = 0;
        pUnixFolder->m_pszPath = NULL;
        pUnixFolder->m_pidlLocation = NULL;
        pUnixFolder->m_dwPathMode = IsEqualCLSID(&CLSID_UnixFolder, pCLSID) ? PATHMODE_UNIX : PATHMODE_DOS;
        pUnixFolder->m_dwAttributes = 0;
        pUnixFolder->m_pCLSID = pCLSID;

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
            rgelt[i] = (LPITEMIDLIST)SHAlloc(SHITEMID_LEN_FROM_NAME_LEN(lstrlenA(pszRelativePath))+sizeof(USHORT));
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
    
    TRACE("(iface=%p, celt=%ld)\n", iface, celt);

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
