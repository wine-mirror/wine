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

#include "config.h"
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <dirent.h>
#include <unistd.h>
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
#include "winternl.h"
#include "wine/debug.h"

#include "shell32_main.h"
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
 * the alternate DOS name, which we don't use here. And then there's
 * the additional StatStruct.
 */
#define SHITEMID_LEN_FROM_NAME_LEN(n) \
    (sizeof(USHORT)+sizeof(PIDLTYPE)+sizeof(FileStruct)+(n)+sizeof(char)+sizeof(StatStruct))
#define NAME_LEN_FROM_LPSHITEMID(s) \
    (((LPSHITEMID)s)->cb-sizeof(USHORT)-sizeof(PIDLTYPE)-sizeof(FileStruct)-sizeof(char)-sizeof(StatStruct))
#define LPSTATSTRUCT_FROM_LPSHITEMID(s) ((StatStruct*)(((LPBYTE)s)+((LPSHITEMID)s)->cb-sizeof(StatStruct)))

#define PATHMODE_UNIX 0
#define PATHMODE_DOS  1

/* This structure is appended to shell32's FileStruct type in IDLs to store unix
 * filesystem specific informationen extracted with the stat system call.
 */
typedef struct tagStatStruct {
    mode_t st_mode;
    uid_t  st_uid;
    gid_t  st_gid;
} StatStruct;

/* UnixFolder object layout and typedef.
 */
typedef struct _UnixFolder {
    const IShellFolder2Vtbl  *lpIShellFolder2Vtbl;
    const IPersistFolder2Vtbl *lpIPersistFolder2Vtbl;
    ULONG m_cRef;
    CHAR *m_pszPath;
    LPITEMIDLIST m_pidlLocation;
    LPITEMIDLIST *m_apidlSubDirs;
    DWORD m_cSubDirs;
    DWORD m_dwPathMode;
} UnixFolder;

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
    LPSTR pszText = _ILGetTextPointer(pIDL);
    if (!pszText) return FALSE;
    if (pszText[0] == '.' && !(fFilter & SHCONTF_INCLUDEHIDDEN)) return FALSE;
    if (_ILIsFolder(pIDL) && (fFilter & SHCONTF_FOLDERS)) return TRUE;
    if (_ILIsValue(pIDL) && (fFilter & SHCONTF_NONFOLDERS)) return TRUE;
    return FALSE;
}

/******************************************************************************
 * UNIXFS_build_shitemid [Internal]
 *
 * Constructs a new SHITEMID for a single path item (up to the next '/' or 
 * '\0') into a buffer. Decorates the SHITEMID with information from a stat 
 * system call.
 *
 * PARAMS
 *  name   [I] The name of the next path item. Terminated by either '\0' or '/'.
 *  pStat  [I] A stat struct variable obtained by a stat system call on the file.
 *  buffer [O] SHITEMID will be constructed here.
 *
 * RETURNS
 *  A pointer to the next '/' or '\0' character in name.
 *  
 * NOTES
 *  Minimum size of buffer is SHITEMID_LEN_FROM_NAME_LEN(strlen(name)).
 *  If what you need is a PIDLLIST with a single SHITEMID, don't forget to append
 *  a 0 USHORT value.
 */
static char* UNIXFS_build_shitemid(char *name, struct stat *pStat, void *buffer) {
    LARGE_INTEGER time;
    FILETIME fileTime;
    LPPIDLDATA pIDLData;
    StatStruct *pStatStruct;
    int cNameLen;
    char *pSlash;

    TRACE("(name=%s, pStat=%p, buffer=%p)\n", debugstr_a(name), pStat, buffer);

    pSlash = strchr(name, '/');
    cNameLen = pSlash ? pSlash - name : strlen(name); 
    
    memset(buffer, 0, SHITEMID_LEN_FROM_NAME_LEN(cNameLen));
    ((LPSHITEMID)buffer)->cb = SHITEMID_LEN_FROM_NAME_LEN(cNameLen) ;
    
    pIDLData = _ILGetDataPointer((LPCITEMIDLIST)buffer);
    pIDLData->type = S_ISDIR(pStat->st_mode) ? PT_FOLDER : PT_VALUE;
    pIDLData->u.file.dwFileSize = (DWORD)pStat->st_size;
    RtlSecondsSince1970ToTime( pStat->st_mtime, &time );
    fileTime.dwLowDateTime = time.u.LowPart;
    fileTime.dwHighDateTime = time.u.HighPart;
    FileTimeToDosDateTime(&fileTime, &pIDLData->u.file.uFileDate, &pIDLData->u.file.uFileTime);
    pIDLData->u.file.uFileAttribs = 
        (S_ISDIR(pStat->st_mode) ? FILE_ATTRIBUTE_DIRECTORY : 0) |
        (name[0] == '.' ? FILE_ATTRIBUTE_HIDDEN : 0);
    memcpy(pIDLData->u.file.szNames, name, cNameLen);

    pStatStruct = LPSTATSTRUCT_FROM_LPSHITEMID(buffer);
    pStatStruct->st_mode = pStat->st_mode;
    pStatStruct->st_uid = pStat->st_uid;
    pStatStruct->st_gid = pStat->st_gid;

    return pSlash ? pSlash+1 : (name + cNameLen);
}

/******************************************************************************
 * UNIXFS_canonicalize_path [Internal]
 *
 * Evaluate "/.", "/.." and symbolic links for an absolute unix path.
 *
 * PARAMS
 *  pszUnixPath      [I] An absolute unix path
 *  pszCanonicalPath [O] Buffer of length FILENAME_MAX. Will receive the canonical path.
 *
 * RETURNS
 *  Success, TRUE
 *  Failure, FALSE - Path not existent, too long, insufficient rights, to many symlinks
 */
static BOOL UNIXFS_canonicalize_path(const char *pszUnixPath, char *pszCanonicalPath)
{
    char *pPathTail, *pElement, *pCanonicalTail, szPath[FILENAME_MAX];
    struct stat fileStat;
    
    TRACE("(pszUnixPath=%s, pszCanonicalPath=%p)\n", debugstr_a(pszUnixPath), pszCanonicalPath);
    
    if (!pszUnixPath || *pszUnixPath != '/')
        return FALSE;

    strcpy(szPath, pszUnixPath);
   
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
    } while (pPathTail[0] == '/' && pPathTail[1]); /* Also handles paths terminated by '/' */
   
    TRACE("--> %s\n", debugstr_a(pszCanonicalPath));
    
    return TRUE;
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
    struct stat fileStat;
    int cSubDirs, cPidlLen, res, cPathLen;
    char *pSlash, szCompletePath[FILENAME_MAX], *pNextPathElement;

    TRACE("pUnixFolder=%p, path=%s, ppidl=%p\n", pUnixFolder, debugstr_w(path), ppidl);
   
    if (!ppidl || !path)
        return FALSE;

    cPathLen = lstrlenW(path);
    
    /* Build an absolute path and let pNextPathElement point to the interesting 
     * relative sub-path. We need the absolute path to call 'stat', but the pidl
     * will only contain the relative part.
     */
    if ((pUnixFolder->m_dwPathMode == PATHMODE_DOS) && (path[1] == ':')) 
    {
        /* Absolute dos path. Convert to unix */
        char *pszUnixPath = wine_get_unix_file_name(path);
        if (!UNIXFS_canonicalize_path(pszUnixPath, szCompletePath)) {
            HeapFree(GetProcessHeap(), 0, pszUnixPath);
            return FALSE;
        }
        HeapFree(GetProcessHeap(), 0, pszUnixPath);
        pNextPathElement = szCompletePath + 1;
    } 
    else if ((pUnixFolder->m_dwPathMode == PATHMODE_UNIX) && (path[0] == '/')) 
    {
        /* Absolute unix path. Just convert to ANSI. */
        WideCharToMultiByte(CP_ACP, 0, path, -1, szCompletePath, FILENAME_MAX, NULL, NULL); 
        pNextPathElement = szCompletePath + 1;
    } 
    else 
    {
        /* Relative dos or unix path. Concat with this folder's path */
        int cBasePathLen = strlen(pUnixFolder->m_pszPath);
        memcpy(szCompletePath, pUnixFolder->m_pszPath, cBasePathLen);
        WideCharToMultiByte(CP_ACP, 0, path, -1, szCompletePath + cBasePathLen, 
                            FILENAME_MAX - cBasePathLen, NULL, NULL);
        pNextPathElement = szCompletePath + cBasePathLen;
        
        /* If in dos mode, replace '\' with '/' */
        if (pUnixFolder->m_dwPathMode == PATHMODE_DOS) {
            char *pBackslash = strchr(pNextPathElement, '\\');
            while (pBackslash) {
                *pBackslash = '/';
                pBackslash = strchr(pBackslash, '\\');
            }
        }
    }

    /* At this point, we have an absolute unix path in szCompletePath. */
    TRACE("complete path: %s\n", szCompletePath);
    
    /* Count the number of sub-directories in the path */
    cSubDirs = 1; /* Path may not be terminated with '/', thus start with 1 */
    pSlash = strchr(pNextPathElement, '/');
    while (pSlash && pSlash[1]) {
        cSubDirs++;
        pSlash = strchr(pSlash+1, '/');
    }
  
    /* Allocate enough memory to hold the path. The -cSubDirs is for the '/' 
     * characters, which are not stored in the ITEMIDLIST. */
    cPidlLen = strlen(pNextPathElement) - cSubDirs + 1 + cSubDirs * SHITEMID_LEN_FROM_NAME_LEN(0) + sizeof(USHORT);
    *ppidl = pidl = (LPITEMIDLIST)SHAlloc(cPidlLen);
    if (!pidl) return FALSE;

    /* Concatenate the SHITEMIDs of the sub-directories. */
    while (*pNextPathElement) {
        pSlash = strchr(pNextPathElement, '/');
        if (pSlash) {
            *pSlash = '\0';
            res = stat(szCompletePath, &fileStat);
            *pSlash = '/';
            if (res) {
                SHFree(pidl);
                return FALSE;
            }
        } else {
            if (stat(szCompletePath, &fileStat)) {
                SHFree(pidl);
                return FALSE;
            }
        }
                
        pNextPathElement = UNIXFS_build_shitemid(pNextPathElement, &fileStat, pidl);
        pidl = ILGetNext(pidl);
    }
    pidl->mkid.cb = 0; /* Terminate the ITEMIDLIST */

    if ((int)pidl-(int)*ppidl+sizeof(USHORT) > cPidlLen) /* We've corrupted the heap :( */ 
        ERR("Computed length of pidl to small. Please report.\n");
    
    return TRUE;
}

/******************************************************************************
 * UNIXFS_pidl_to_path [Internal]
 *
 * Construct the unix path that corresponds to a fully qualified ITEMIDLIST
 *
 * PARAMS
 *  pidl [I] ITEMIDLIST that specifies the absolute location of the folder
 *  path [O] The corresponding unix path as a zero terminated ascii string
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE, pidl doesn't specify a unix path or out of memory
 */
static BOOL UNIXFS_pidl_to_path(LPCITEMIDLIST pidl, UnixFolder *pUnixFolder) {
    LPCITEMIDLIST current = pidl, root;
    DWORD dwPathLen;
    char *pNextDir;

    TRACE("(pidl=%p, pUnixFolder=%p)\n", pidl, pUnixFolder);
    
    pUnixFolder->m_pszPath = NULL;

    /* Find the UnixFolderClass root */
    while (current->mkid.cb) {
        LPPIDLDATA pData = _ILGetDataPointer(current);
        if (!pData) return FALSE;
        if (pData->type == PT_GUID && 
            (IsEqualIID(&CLSID_UnixFolder, &pData->u.guid.guid) ||
             IsEqualIID(&CLSID_UnixDosFolder, &pData->u.guid.guid)))
        {
            break;
        }
        current = ILGetNext(current);
    }
    root = current = ILGetNext(current);
    
    /* Determine the path's length bytes */
    dwPathLen = 2; /* For the '/' prefix and the terminating '\0' */
    while (current->mkid.cb) {
        dwPathLen += NAME_LEN_FROM_LPSHITEMID(current) + 1; /* For the '/' */
        current = ILGetNext(current);
    };

    /* Build the path */
    pUnixFolder->m_pszPath = pNextDir = SHAlloc(dwPathLen);
    if (!pUnixFolder->m_pszPath) {
        WARN("SHAlloc failed!\n");
        return FALSE;
    }
    current = root;
    *pNextDir++ = '/';
    while (current->mkid.cb) {
        memcpy(pNextDir, _ILGetTextPointer(current), NAME_LEN_FROM_LPSHITEMID(current));
        pNextDir += NAME_LEN_FROM_LPSHITEMID(current);
        *pNextDir++ = '/';
        current = ILGetNext(current);
    }
    *pNextDir='\0';
    
    TRACE("--> %s\n", pUnixFolder->m_pszPath);
    return TRUE;
}

/******************************************************************************
 * UNIXFS_build_subfolder_pidls [Internal]
 *
 * Builds an array of subfolder PIDLs relative to a unix directory
 *
 * PARAMS
 *  path   [I] Name of a unix directory as a zero terminated ascii string
 *  apidl  [O] The array of PIDLs
 *  pCount [O] Size of apidl
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE, path is not a valid unix directory or out of memory
 *
 * NOTES
 *  The array of PIDLs and each PIDL are allocated with SHAlloc. You'll have
 *  to release each PIDL as well as the array itself with SHFree.
 */
static BOOL UNIXFS_build_subfolder_pidls(UnixFolder *pUnixFolder)
{
    struct dirent *pDirEntry;
    struct stat fileStat;
    DIR *dir;
    DWORD cDirEntries, i;
    USHORT sLen;
    char *pszFQPath;

    TRACE("(pUnixFolder=%p)\n", pUnixFolder);
    
    pUnixFolder->m_apidlSubDirs = NULL;
    pUnixFolder->m_cSubDirs = 0;
  
    dir = opendir(pUnixFolder->m_pszPath);
    if (!dir) {
        WARN("Failed to open directory '%s'.\n", pUnixFolder->m_pszPath);
        return FALSE;
    }

    /* Allocate space for fully qualified paths */
    pszFQPath = SHAlloc(strlen(pUnixFolder->m_pszPath) + PATH_MAX);
    if (!pszFQPath) {
        WARN("SHAlloc failed!\n");
        return FALSE;
    }
 
    /* Count number of directory entries. */
    for (cDirEntries = 0, pDirEntry = readdir(dir); pDirEntry; pDirEntry = readdir(dir)) { 
        if (!strcmp(pDirEntry->d_name, ".") || !strcmp(pDirEntry->d_name, "..")) continue;
        sprintf(pszFQPath, "%s%s", pUnixFolder->m_pszPath, pDirEntry->d_name);
        if (!stat(pszFQPath, &fileStat) && (S_ISDIR(fileStat.st_mode) || S_ISREG(fileStat.st_mode))) cDirEntries++;
    }

    /* If there are no entries, we are done. */
    if (cDirEntries == 0) {
        closedir(dir);
        SHFree(pszFQPath);
        return TRUE;
    }

    /* Allocate the array of PIDLs */
    pUnixFolder->m_apidlSubDirs = SHAlloc(cDirEntries * sizeof(LPITEMIDLIST));
    if (!pUnixFolder->m_apidlSubDirs) {
        WARN("SHAlloc failed!\n");
        return FALSE;
    }
  
    /* Allocate and initialize one SHITEMID per sub-directory. */
    for (rewinddir(dir), pDirEntry = readdir(dir), i = 0; pDirEntry; pDirEntry = readdir(dir)) {
        LPSHITEMID pid;
            
        if (!strcmp(pDirEntry->d_name, ".") || !strcmp(pDirEntry->d_name, "..")) continue;

        sprintf(pszFQPath, "%s%s", pUnixFolder->m_pszPath, pDirEntry->d_name);
        if (stat(pszFQPath, &fileStat)) continue;
        if (!S_ISDIR(fileStat.st_mode) && !S_ISREG(fileStat.st_mode)) continue;
   
        sLen = strlen(pDirEntry->d_name);
        pid = (LPSHITEMID)SHAlloc(SHITEMID_LEN_FROM_NAME_LEN(sLen)+sizeof(USHORT));
        if (!pid) {
            WARN("SHAlloc failed!\n");
            return FALSE;
        }
        UNIXFS_build_shitemid(pDirEntry->d_name, &fileStat, pid);
        memset(((PBYTE)pid)+pid->cb, 0, sizeof(USHORT));

        pUnixFolder->m_apidlSubDirs[i++] = (LPITEMIDLIST)pid;
    }
    
    pUnixFolder->m_cSubDirs = i;    
    closedir(dir);
    SHFree(pszFQPath);

    return TRUE;
}

/******************************************************************************
 * UnixFolderIcon 
 *
 * Singleton class, which is used by the shell to extract icons to represent
 * folders in tree- and listviews. Currently, all this singleton does is to
 * provide the shell with the absolute path to "shell32.dll" and with the 
 * indices of the closed and opened folder icons in the resources of this dll.
 */

/* UnixFolderIcon object layout and typedef.
 */
typedef struct _UnixFolderIcon {
    const IExtractIconWVtbl *lpIExtractIconWVtbl;
    BOOL bFolder;
} UnixFolderIcon;

static HRESULT WINAPI UnixFolderIcon_IExtractIconW_QueryInterface(IExtractIconW *iface, REFIID riid, 
    void **ppv) 
{
    TRACE("(iface=%p, riid=%p, ppv=%p)\n", iface, riid, ppv);
    
    if (!ppv) return E_INVALIDARG;
    
    if (IsEqualIID(&IID_IUnknown, riid) ||
        IsEqualIID(&IID_IExtractIconW, riid))
    {
        *ppv = iface;
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IExtractIconW_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI UnixFolderIcon_IExtractIconW_AddRef(IExtractIconW *iface) {
    TRACE("(iface=%p)\n", iface);
    return 2;
}

static ULONG WINAPI UnixFolderIcon_IExtractIconW_Release(IExtractIconW *iface) {
    TRACE("(iface=%p)\n", iface);
    return 1;
}

static HRESULT WINAPI UnixFolderIcon_IExtractIconW_GetIconLocation(IExtractIconW *iface, 
    UINT uFlags, LPWSTR szIconFile, UINT cchMax, INT* piIndex, UINT* pwFlags)
{
    UnixFolderIcon *This = ADJUST_THIS(UnixFolderIcon, IExtractIconW, iface);
        
    TRACE("(iface=%p, uFlags=%u, szIconFile=%s, cchMax=%u, piIndex=%p, pwFlags=%p)\n",
            iface, uFlags, debugstr_w(szIconFile), cchMax, piIndex, pwFlags);
    
    lstrcpynW(szIconFile, swShell32Name, cchMax);
    if (This->bFolder) {
        *piIndex = (uFlags & GIL_OPENICON) ? -IDI_SHELL_FOLDER_OPEN : -IDI_SHELL_FOLDER;
    } else {
        *piIndex = -IDI_SHELL_DOCUMENT;
    }
    *pwFlags = 0;

    return S_OK;
}

static HRESULT WINAPI UnixFolderIcon_IExtractIconW_Extract(
    IExtractIconW *iface, LPCWSTR pszFile, UINT nIconIndex, HICON* phiconLarge, HICON* phiconSmall, 
    UINT nIconSize)
{
    TRACE("(iface=%p, pszFile=%s, nIconIndex=%u, phiconLarge=%p, phiconSmall=%p, nIconSize=%u)"
          "stub\n", iface, debugstr_w(pszFile), nIconIndex, phiconLarge, phiconSmall, nIconSize);

    return E_NOTIMPL;
}

/* VTable for the IExtractIconW interface of the UnixFolderIcon class. 
 */
static const IExtractIconWVtbl UnixFolderIcon_IExtractIconW_Vtbl = {
    UnixFolderIcon_IExtractIconW_QueryInterface,
    UnixFolderIcon_IExtractIconW_AddRef,
    UnixFolderIcon_IExtractIconW_Release,
    UnixFolderIcon_IExtractIconW_GetIconLocation,
    UnixFolderIcon_IExtractIconW_Extract
};

/* The singleton instance
 */
UnixFolderIcon UnixFolderIconSingleton = { &UnixFolderIcon_IExtractIconW_Vtbl, TRUE };
UnixFolderIcon UnixDocumentIconSingleton = { &UnixFolderIcon_IExtractIconW_Vtbl, FALSE };

/******************************************************************************
 * UnixFolder
 *
 * Class whose heap based instances represent unix filesystem directories.
 */

static void UnixFolder_Destroy(UnixFolder *pUnixFolder) {
    DWORD i;

    TRACE("(pUnixFolder=%p)\n", pUnixFolder);
    
    if (pUnixFolder->m_apidlSubDirs) 
        for (i=0; i < pUnixFolder->m_cSubDirs; i++) 
            SHFree(pUnixFolder->m_apidlSubDirs[i]);
    SHFree(pUnixFolder->m_apidlSubDirs);
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
    } else if (IsEqualIID(&IID_IPersistFolder2, riid) || IsEqualIID(&IID_IPersistFolder, riid) || 
               IsEqualIID(&IID_IPersist, riid)) 
    {
        *ppv = &This->lpIPersistFolder2Vtbl;
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
        /* need to traverse to the last element for the attribute */
        LPCITEMIDLIST pidl, last_pidl;
        pidl = last_pidl = *ppidl;
        while(pidl && pidl->mkid.cb)
        {
            last_pidl = pidl;
            pidl = ILGetNext(pidl);
        }
        SHELL32_GetItemAttributes((IShellFolder*)iface, last_pidl, pdwAttributes);
    }

    if (!result) TRACE("FAILED!\n");
    return result ? S_OK : E_FAIL;
}

static IUnknown *UnixSubFolderIterator_Construct(UnixFolder *pUnixFolder, SHCONTF fFilter);

static HRESULT WINAPI UnixFolder_IShellFolder2_EnumObjects(IShellFolder2* iface, HWND hwndOwner, 
    SHCONTF grfFlags, IEnumIDList** ppEnumIDList)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, IShellFolder2, iface);
    IUnknown *newIterator;
    HRESULT hr;
    
    TRACE("(iface=%p, hwndOwner=%p, grfFlags=%08lx, ppEnumIDList=%p)\n", 
            iface, hwndOwner, grfFlags, ppEnumIDList);

    newIterator = UnixSubFolderIterator_Construct(This, grfFlags);
    hr = IUnknown_QueryInterface(newIterator, &IID_IEnumIDList, (void**)ppEnumIDList);
    IUnknown_Release(newIterator);
    
    return hr;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_BindToObject(IShellFolder2* iface, LPCITEMIDLIST pidl,
    LPBC pbcReserved, REFIID riid, void** ppvOut)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, IShellFolder2, iface);
    IPersistFolder2 *persistFolder;
    LPITEMIDLIST pidlSubFolder;
    HRESULT hr;
        
    TRACE("(iface=%p, pidl=%p, pbcReserver=%p, riid=%p, ppvOut=%p)\n", 
            iface, pidl, pbcReserved, riid, ppvOut);

    if (This->m_dwPathMode == PATHMODE_DOS)
        hr = UnixDosFolder_Constructor(NULL, &IID_IPersistFolder2, (void**)&persistFolder);
    else
        hr = UnixFolder_Constructor(NULL, &IID_IPersistFolder2, (void**)&persistFolder);
        
    if (!SUCCEEDED(hr)) return hr;
    hr = IPersistFolder_QueryInterface(persistFolder, riid, (void**)ppvOut);
    
    pidlSubFolder = ILCombine(This->m_pidlLocation, pidl);
    IPersistFolder2_Initialize(persistFolder, pidlSubFolder);
    IPersistFolder2_Release(persistFolder);
    ILFree(pidlSubFolder);

    return hr;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_BindToStorage(IShellFolder2* This, LPCITEMIDLIST pidl, 
    LPBC pbcReserved, REFIID riid, void** ppvObj)
{
    TRACE("stub\n");
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
    UINT i;
    SFGAOF flags= ~(SFGAOF)0;
        
    TRACE("(iface=%p, cidl=%u, apidl=%p, rgfInOut=%p) semi-stub\n", iface, cidl, apidl, rgfInOut);
   
    for (i=0; i<cidl; i++) {
        LPPIDLDATA pData = _ILGetDataPointer(apidl[i]);
        if (!pData) continue;
        if (pData->type == PT_FOLDER) flags &= (SFGAO_FILESYSTEM|SFGAO_FILESYSANCESTOR|SFGAO_FOLDER|SFGAO_HASSUBFOLDER);
        if (pData->type == PT_VALUE) flags &= SFGAO_FILESYSTEM;
    }
    
    *rgfInOut = *rgfInOut & flags;
            
    return S_OK;
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
        FIXME("IExtractIconA\n");
        return E_FAIL;
    } else if (IsEqualIID(&IID_IExtractIconW, riid)) {
        if (cidl != 1) return E_FAIL;
        if (_ILIsFolder(apidl[0])) 
            *ppvOut = &UnixFolderIconSingleton;
        else
            *ppvOut = &UnixDocumentIconSingleton;
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

static HRESULT WINAPI UnixFolder_IShellFolder2_GetDisplayNameOf(IShellFolder2* iface, 
    LPCITEMIDLIST pidl, SHGDNF uFlags, STRRET* lpName)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, IShellFolder2, iface);
    HRESULT hr = S_OK;    

    TRACE("(iface=%p, pidl=%p, uFlags=%lx, lpName=%p)\n", iface, pidl, uFlags, lpName);
    
    if ((GET_SHGDN_FOR(uFlags) & SHGDN_FORPARSING) &&
        (GET_SHGDN_RELATION(uFlags) != SHGDN_INFOLDER))
    {
        if (!pidl->mkid.cb) {
            lpName->uType = STRRET_CSTR;
            strcpy(lpName->u.cStr, This->m_pszPath);
            if (This->m_dwPathMode == PATHMODE_DOS) {
                char path[MAX_PATH];
                GetFullPathNameA(lpName->u.cStr, MAX_PATH, path, NULL);
                strcpy(lpName->u.cStr, path);
            }
        } else {
            IShellFolder *pSubFolder;
            USHORT emptyIDL = 0;

            hr = IShellFolder_BindToObject(iface, pidl, NULL, &IID_IShellFolder, (void**)&pSubFolder);
            if (!SUCCEEDED(hr)) return hr;
       
            hr = IShellFolder_GetDisplayNameOf(pSubFolder, (LPITEMIDLIST)&emptyIDL, uFlags, lpName);
            IShellFolder_Release(pSubFolder);
        }
    } else {
        char *pszFileName = _ILGetTextPointer(pidl);
        lpName->uType = STRRET_CSTR;
        strcpy(lpName->u.cStr, pszFileName ? pszFileName : "");
    }

    TRACE("--> %s\n", lpName->u.cStr);
    
    return hr;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_SetNameOf(IShellFolder2* This, HWND hwnd, 
    LPCITEMIDLIST pidl, LPCOLESTR lpszName, SHGDNF uFlags, LPITEMIDLIST* ppidlOut)
{
    TRACE("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_EnumSearches(IShellFolder2* iface, 
    IEnumExtraSearch **ppEnum) 
{
    TRACE("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_GetDefaultColumn(IShellFolder2* iface, 
    DWORD dwReserved, ULONG *pSort, ULONG *pDisplay) 
{
    TRACE("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_GetDefaultColumnState(IShellFolder2* iface, 
    UINT iColumn, SHCOLSTATEF *pcsFlags)
{
    TRACE("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_GetDefaultSearchGUID(IShellFolder2* iface, 
    GUID *pguid)
{
    TRACE("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_GetDetailsEx(IShellFolder2* iface, 
    LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    TRACE("stub\n");
    return E_NOTIMPL;
}

#define SHELLVIEWCOLUMNS 6 

static HRESULT WINAPI UnixFolder_IShellFolder2_GetDetailsOf(IShellFolder2* iface, 
    LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *psd)
{
    HRESULT hr = E_FAIL;
    struct passwd *pPasswd;
    struct group *pGroup;
    static const shvheader SFHeader[SHELLVIEWCOLUMNS] = {
        {IDS_SHV_COLUMN1,  SHCOLSTATE_TYPE_STR  | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15},
        {IDS_SHV_COLUMN5,  SHCOLSTATE_TYPE_STR  | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
        {IDS_SHV_COLUMN10, SHCOLSTATE_TYPE_STR  | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 7},
        {IDS_SHV_COLUMN11, SHCOLSTATE_TYPE_STR  | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 7},
        {IDS_SHV_COLUMN2,  SHCOLSTATE_TYPE_STR  | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 8},
        {IDS_SHV_COLUMN4,  SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10}
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
        StatStruct *pStatStruct = LPSTATSTRUCT_FROM_LPSHITEMID(pidl);
        psd->str.u.cStr[0] = '\0';
        psd->str.uType = STRRET_CSTR;
        switch (iColumn) {
            case 0:
                hr = IShellFolder2_GetDisplayNameOf(iface, pidl, SHGDN_NORMAL|SHGDN_INFOLDER, &psd->str);
                break;
            case 1:
                psd->str.u.cStr[0] = S_ISDIR(pStatStruct->st_mode) ? 'd' : '-';
                psd->str.u.cStr[1] = (pStatStruct->st_mode & S_IRUSR) ? 'r' : '-';
                psd->str.u.cStr[2] = (pStatStruct->st_mode & S_IWUSR) ? 'w' : '-';
                psd->str.u.cStr[3] = (pStatStruct->st_mode & S_IXUSR) ? 'x' : '-';
                psd->str.u.cStr[4] = (pStatStruct->st_mode & S_IRGRP) ? 'r' : '-';
                psd->str.u.cStr[5] = (pStatStruct->st_mode & S_IWGRP) ? 'w' : '-';
                psd->str.u.cStr[6] = (pStatStruct->st_mode & S_IXGRP) ? 'x' : '-';
                psd->str.u.cStr[7] = (pStatStruct->st_mode & S_IROTH) ? 'r' : '-';
                psd->str.u.cStr[8] = (pStatStruct->st_mode & S_IWOTH) ? 'w' : '-';
                psd->str.u.cStr[9] = (pStatStruct->st_mode & S_IXOTH) ? 'x' : '-';
                psd->str.u.cStr[10] = '\0';
                break;
            case 2:
                pPasswd = getpwuid(pStatStruct->st_uid);
                if (pPasswd) strcpy(psd->str.u.cStr, pPasswd->pw_name);
                break;
            case 3:
                pGroup = getgrgid(pStatStruct->st_gid);
                if (pGroup) strcpy(psd->str.u.cStr, pGroup->gr_name);
                break;
            case 4:
                _ILGetFileSize(pidl, psd->str.u.cStr, MAX_PATH);
                break;
            case 5:
                _ILGetFileDate(pidl, psd->str.u.cStr, MAX_PATH);
                break;
        }
    }
    
    return hr;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_MapColumnToSCID(IShellFolder2* iface, UINT iColumn,
    SHCOLUMNID *pscid) 
{
    TRACE("stub\n");
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

static HRESULT WINAPI UnixFolder_IPersistFolder2_QueryInterface(IPersistFolder2* This, REFIID riid, 
    void** ppvObject)
{
    return UnixFolder_IShellFolder2_QueryInterface(
                (IShellFolder2*)ADJUST_THIS(UnixFolder, IPersistFolder2, This), riid, ppvObject);
}

static ULONG WINAPI UnixFolder_IPersistFolder2_AddRef(IPersistFolder2* This)
{
    return UnixFolder_IShellFolder2_AddRef(
                (IShellFolder2*)ADJUST_THIS(UnixFolder, IPersistFolder2, This));
}

static ULONG WINAPI UnixFolder_IPersistFolder2_Release(IPersistFolder2* This)
{
    return UnixFolder_IShellFolder2_Release(
                (IShellFolder2*)ADJUST_THIS(UnixFolder, IPersistFolder2, This));
}

static HRESULT WINAPI UnixFolder_IPersistFolder2_GetClassID(IPersistFolder2* This, CLSID* pClassID)
{
    TRACE("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI UnixFolder_IPersistFolder2_Initialize(IPersistFolder2* iface, LPCITEMIDLIST pidl)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, IPersistFolder2, iface);
    
    TRACE("(iface=%p, pidl=%p)\n", iface, pidl);

    This->m_pidlLocation = ILClone(pidl);
    
    pdump(pidl);
    
    UNIXFS_pidl_to_path(pidl, This);
    UNIXFS_build_subfolder_pidls(This);
   
    return S_OK;
}

static HRESULT WINAPI UnixFolder_IPersistFolder2_GetCurFolder(IPersistFolder2* iface, LPITEMIDLIST* ppidl)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, IPersistFolder2, iface);
    
    TRACE ("(iface=%p, ppidl=%p)\n", iface, ppidl);

    if (!ppidl)
        return E_POINTER;
    *ppidl = ILClone (This->m_pidlLocation);
    return S_OK;
}
    
/* VTable for UnixFolder's IPersistFolder interface.
 */
static const IPersistFolder2Vtbl UnixFolder_IPersistFolder2_Vtbl = {
    UnixFolder_IPersistFolder2_QueryInterface,
    UnixFolder_IPersistFolder2_AddRef,
    UnixFolder_IPersistFolder2_Release,
    UnixFolder_IPersistFolder2_GetClassID,
    UnixFolder_IPersistFolder2_Initialize,
    UnixFolder_IPersistFolder2_GetCurFolder
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
static HRESULT CreateUnixFolder(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv, DWORD dwPathMode) {
    HRESULT hr = E_FAIL;
    UnixFolder *pUnixFolder = SHAlloc((ULONG)sizeof(UnixFolder));
    
    if(pUnixFolder) {
        pUnixFolder->lpIShellFolder2Vtbl = &UnixFolder_IShellFolder2_Vtbl;
        pUnixFolder->lpIPersistFolder2Vtbl = &UnixFolder_IPersistFolder2_Vtbl;
        pUnixFolder->m_cRef = 0;
        pUnixFolder->m_pszPath = NULL;
        pUnixFolder->m_apidlSubDirs = NULL;
        pUnixFolder->m_cSubDirs = 0;
        pUnixFolder->m_dwPathMode = dwPathMode;

        UnixFolder_IShellFolder2_AddRef(STATIC_CAST(IShellFolder2, pUnixFolder));
        hr = UnixFolder_IShellFolder2_QueryInterface(STATIC_CAST(IShellFolder2, pUnixFolder), riid, ppv);
        UnixFolder_IShellFolder2_Release(STATIC_CAST(IShellFolder2, pUnixFolder));
    }
    return hr;
}

HRESULT WINAPI UnixFolder_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv) {
    TRACE("(pUnkOuter=%p, riid=%p, ppv=%p)\n", pUnkOuter, riid, ppv);
    return CreateUnixFolder(pUnkOuter, riid, ppv, PATHMODE_UNIX);
}

HRESULT WINAPI UnixDosFolder_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv) {
    TRACE("(pUnkOuter=%p, riid=%p, ppv=%p)\n", pUnkOuter, riid, ppv);
    return CreateUnixFolder(pUnkOuter, riid, ppv, PATHMODE_DOS);
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
    ULONG m_cRef;
    UnixFolder *m_pUnixFolder;
    ULONG m_cIdx;
    SHCONTF m_fFilter;
} UnixSubFolderIterator;

static void UnixSubFolderIterator_Destroy(UnixSubFolderIterator *iterator) {
    TRACE("(iterator=%p)\n", iterator);
        
    UnixFolder_IShellFolder2_Release((IShellFolder2*)iterator->m_pUnixFolder);
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
    ULONG i;

    TRACE("(iface=%p, celt=%ld, rgelt=%p, pceltFetched=%p)\n", iface, celt, rgelt, pceltFetched);
   
    for (i=0; (i < celt) && (This->m_cIdx < This->m_pUnixFolder->m_cSubDirs); This->m_cIdx++) {
        LPITEMIDLIST pCurrent = This->m_pUnixFolder->m_apidlSubDirs[This->m_cIdx];
        if (UNIXFS_is_pidl_of_type(pCurrent, This->m_fFilter)) {
            rgelt[i] = ILClone(pCurrent);
            i++;
        }
    }

    if (pceltFetched)
        *pceltFetched = i;

    return i == celt ? S_OK : S_FALSE;
}

static HRESULT WINAPI UnixSubFolderIterator_IEnumIDList_Skip(IEnumIDList* iface, ULONG celt)
{
    UnixSubFolderIterator *This = ADJUST_THIS(UnixSubFolderIterator, IEnumIDList, iface);
    ULONG i;
    
    TRACE("(iface=%p, celt=%ld)\n", iface, celt);

    for (i=0; i < celt; i++) {
        while (This->m_cIdx < This->m_pUnixFolder->m_cSubDirs &&
               !UNIXFS_is_pidl_of_type(This->m_pUnixFolder->m_apidlSubDirs[This->m_cIdx], This->m_fFilter)) 
        {
            This->m_cIdx++;
        }
        This->m_cIdx++;
    }
    
    if (This->m_cIdx > This->m_pUnixFolder->m_cSubDirs) {
        This->m_cIdx = This->m_pUnixFolder->m_cSubDirs;
        return S_FALSE;
    } else {
        return S_OK;
    }
}

static HRESULT WINAPI UnixSubFolderIterator_IEnumIDList_Reset(IEnumIDList* iface)
{
    UnixSubFolderIterator *This = ADJUST_THIS(UnixSubFolderIterator, IEnumIDList, iface);
        
    TRACE("(iface=%p)\n", iface);

    This->m_cIdx = 0;
    
    return S_OK;
}

static HRESULT WINAPI UnixSubFolderIterator_IEnumIDList_Clone(IEnumIDList* This, 
    IEnumIDList** ppenum)
{
    TRACE("stub\n");
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

static IUnknown *UnixSubFolderIterator_Construct(UnixFolder *pUnixFolder, SHCONTF fFilter) {
    UnixSubFolderIterator *iterator;

    TRACE("(pUnixFolder=%p)\n", pUnixFolder);
    
    iterator = SHAlloc((ULONG)sizeof(UnixSubFolderIterator));
    iterator->lpIEnumIDListVtbl = &UnixSubFolderIterator_IEnumIDList_Vtbl;
    iterator->m_cRef = 0;
    iterator->m_cIdx = 0;
    iterator->m_pUnixFolder = pUnixFolder;
    iterator->m_fFilter = fFilter;

    UnixSubFolderIterator_IEnumIDList_AddRef((IEnumIDList*)iterator);
    UnixFolder_IShellFolder2_AddRef((IShellFolder2*)pUnixFolder);
    
    return (IUnknown*)iterator;
}
