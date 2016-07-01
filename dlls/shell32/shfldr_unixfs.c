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
 * lot about this, since path-names can't be told apart from ordinary strings
 * here. That's different in the file dialogs, though.
 *
 * With the introduction of the 'shell' in win32, Microsoft established an 
 * abstraction layer on top of the filesystem, called the shell namespace (I was
 * told that GNOME's virtual filesystem is conceptually similar). In the shell
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
 * IShellFolder and especially its methods ParseDisplayName and
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
 * shell namespace object for its DOS path, it asks if it actually has one.
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
 * don't have the SFGAO_FILESYSTEM flag set. So the file dialogs shouldn't allow
 * the user to select such a folder for file storage (And if it does anyhow, it 
 * will not be able to return a valid path, since there is none). Think of those 
 * folders as a hierarchy of 'My Computer'-like folders, which happen to be a 
 * shadow of your unix filesystem tree. And since all of this stuff doesn't 
 * change anything at all in wine's fileio api's, windows applications will have 
 * no more access rights as they had before. 
 *
 * To sum it all up, you can still safely run wine with you root account (Just
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
#include <errno.h>
#ifdef HAVE_DIRENT_H
# include <dirent.h>
#endif
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
#ifdef HAVE_GRP_H
# include <grp.h>
#endif

#define COBJMACROS
#define NONAMELESSUNION

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
#include "debughlp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#if !defined(__MINGW32__) && !defined(_MSC_VER)

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

static const WCHAR wFileSystemBindData[] = {
    'F','i','l','e',' ','S','y','s','t','e','m',' ','B','i','n','d',' ','D','a','t','a',0};

typedef struct {
    IShellFolder2       IShellFolder2_iface;
    IPersistFolder3     IPersistFolder3_iface;
    IPersistPropertyBag IPersistPropertyBag_iface;
    IDropTarget         IDropTarget_iface;
    ISFHelper           ISFHelper_iface;

    LONG         ref;
    CHAR         *m_pszPath;     /* Target path of the shell folder (CP_UNIXCP) */
    LPITEMIDLIST m_pidlLocation; /* Location in the shell namespace */
    DWORD        m_dwPathMode;
    DWORD        m_dwAttributes;
    const CLSID  *m_pCLSID;
    DWORD        m_dwDropEffectsMask;
} UnixFolder;

static inline UnixFolder *impl_from_IShellFolder2(IShellFolder2 *iface)
{
    return CONTAINING_RECORD(iface, UnixFolder, IShellFolder2_iface);
}

static inline UnixFolder *impl_from_IPersistFolder3(IPersistFolder3 *iface)
{
    return CONTAINING_RECORD(iface, UnixFolder, IPersistFolder3_iface);
}

static inline UnixFolder *impl_from_IPersistPropertyBag(IPersistPropertyBag *iface)
{
    return CONTAINING_RECORD(iface, UnixFolder, IPersistPropertyBag_iface);
}

static inline UnixFolder *impl_from_ISFHelper(ISFHelper *iface)
{
    return CONTAINING_RECORD(iface, UnixFolder, ISFHelper_iface);
}

static inline UnixFolder *impl_from_IDropTarget(IDropTarget *iface)
{
    return CONTAINING_RECORD(iface, UnixFolder, IDropTarget_iface);
}

/* Will hold the registered clipboard format identifier for ITEMIDLISTS. */
static UINT cfShellIDList = 0;

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
static inline BOOL UNIXFS_is_pidl_of_type(LPCITEMIDLIST pIDL, SHCONTF fFilter) {
    const PIDLDATA *pIDLData = _ILGetDataPointer(pIDL);
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
 *  Failure, FALSE - Nonexistent path, too long, insufficient rights, too many symlinks
 */
static BOOL UNIXFS_get_unix_path(LPCWSTR pszDosPath, char *pszCanonicalPath)
{
    char *pPathTail, *pElement, *pCanonicalTail, szPath[FILENAME_MAX], *pszUnixPath, mb_path[FILENAME_MAX];
    BOOL has_failed = FALSE;
    WCHAR wszDrive[] = { '?', ':', '\\', 0 }, dospath[MAX_PATH], *dospath_end;
    int cDriveSymlinkLen;
    void *redir;

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
    lstrcpyW(dospath, pszDosPath);
    dospath_end = dospath + lstrlenW(dospath);
    /* search for the most valid UNIX path possible, then append missing
     * path parts */
    Wow64DisableWow64FsRedirection(&redir);
    while(!(pszUnixPath = wine_get_unix_file_name(dospath))){
        if(has_failed){
            *dospath_end = '/';
            --dospath_end;
        }else
            has_failed = TRUE;
        while(*dospath_end != '\\' && *dospath_end != '/'){
            --dospath_end;
            if(dospath_end < dospath)
                break;
        }
        *dospath_end = '\0';
    }
    Wow64RevertWow64FsRedirection(redir);
    if(dospath_end < dospath)
        return FALSE;
    strcat(szPath, pszUnixPath + cDriveSymlinkLen);
    HeapFree(GetProcessHeap(), 0, pszUnixPath);

    if(has_failed && WideCharToMultiByte(CP_UNIXCP, 0, dospath_end + 1, -1,
                mb_path, FILENAME_MAX, NULL, NULL) > 0){
        strcat(szPath, "/");
        strcat(szPath, mb_path);
    }

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
 * UNIXFS_build_shitemid [Internal]
 *
 * Constructs a new SHITEMID for the last component of path 'pszUnixPath' into 
 * buffer 'pIDL'.
 *
 * PARAMS
 *  pszUnixPath [I] An absolute path. The SHITEMID will be built for the last component.
 *  pbc         [I] Bind context for this action, used to determine if the file must exist
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
static char* UNIXFS_build_shitemid(char *pszUnixPath, BOOL bMustExist, WIN32_FIND_DATAW *pFindData, void *pIDL) {
    LPPIDLDATA pIDLData;
    struct stat fileStat;
    WIN32_FIND_DATAW findData;
    char *pszComponentU, *pszComponentA;
    WCHAR *pwszComponentW;
    int cComponentULen, cComponentALen;
    USHORT cbLen;
    FileStructW *pFileStructW;
    WORD uOffsetW, *pOffsetW;

    TRACE("(pszUnixPath=%s, bMustExist=%s, pFindData=%p, pIDL=%p)\n",
            debugstr_a(pszUnixPath), bMustExist ? "T" : "F", pFindData, pIDL);

    if (pFindData)
        memcpy(&findData, pFindData, sizeof(WIN32_FIND_DATAW));
    else {
        memset(&findData, 0, sizeof(WIN32_FIND_DATAW));
        findData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
    }

    /* We are only interested in regular files and directories. */
    if (stat(pszUnixPath, &fileStat)){
        if (bMustExist || errno != ENOENT)
            return NULL;
    } else {
        LARGE_INTEGER time;

        if (S_ISDIR(fileStat.st_mode))
            findData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
        else if (S_ISREG(fileStat.st_mode))
            findData.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
        else
            return NULL;

        findData.nFileSizeLow = (DWORD)fileStat.st_size;
        findData.nFileSizeHigh = fileStat.st_size >> 32;

        RtlSecondsSince1970ToTime(fileStat.st_mtime, &time);
        findData.ftLastWriteTime.dwLowDateTime = time.u.LowPart;
        findData.ftLastWriteTime.dwHighDateTime = time.u.HighPart;
        RtlSecondsSince1970ToTime(fileStat.st_atime, &time);
        findData.ftLastAccessTime.dwLowDateTime = time.u.LowPart;
        findData.ftLastAccessTime.dwHighDateTime = time.u.HighPart;
    }
    
    /* Compute the SHITEMID's length and wipe it. */
    pszComponentU = strrchr(pszUnixPath, '/') + 1;
    cComponentULen = strlen(pszComponentU);
    cbLen = UNIXFS_shitemid_len_from_filename(pszComponentU, &pszComponentA, &pwszComponentW);
    if (!cbLen) return NULL;
    memset(pIDL, 0, cbLen);
    ((LPSHITEMID)pIDL)->cb = cbLen;
    
    /* Set shell32's standard SHITEMID data fields. */
    pIDLData = _ILGetDataPointer(pIDL);
    pIDLData->type = (findData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) ? PT_FOLDER : PT_VALUE;
    pIDLData->u.file.dwFileSize = findData.nFileSizeLow;
    FileTimeToDosDateTime(&findData.ftLastWriteTime, &pIDLData->u.file.uFileDate,
            &pIDLData->u.file.uFileTime);
    pIDLData->u.file.uFileAttribs = 0;
    pIDLData->u.file.uFileAttribs |= findData.dwFileAttributes;
    if (pszComponentU[0] == '.') pIDLData->u.file.uFileAttribs |=  FILE_ATTRIBUTE_HIDDEN;
    cComponentALen = lstrlenA(pszComponentA) + 1;
    memcpy(pIDLData->u.file.szNames, pszComponentA, cComponentALen);
    
    pFileStructW = (FileStructW*)(pIDLData->u.file.szNames + cComponentALen + (cComponentALen & 0x1));
    uOffsetW = (WORD)(((LPBYTE)pFileStructW) - ((LPBYTE)pIDL));
    pFileStructW->cbLen = cbLen - uOffsetW;
    FileTimeToDosDateTime(&findData.ftLastWriteTime, &pFileStructW->uCreationDate,
        &pFileStructW->uCreationTime);
    FileTimeToDosDateTime(&findData.ftLastAccessTime, &pFileStructW->uLastAccessDate,
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
 *  path        [I] An absolute unix or dos path or a path relative to pUnixFolder
 *  ppidl       [O] The corresponding ITEMIDLIST. Release with SHFree/ILFree
 *  
 * RETURNS
 *  Success: S_OK
 *  Failure: Error code, invalid params or out of memory
 *
 * NOTES
 *  pUnixFolder also carries the information if the path is expected to be unix or dos.
 */
static HRESULT UNIXFS_path_to_pidl(UnixFolder *pUnixFolder, LPBC pbc, const WCHAR *path,
        LPITEMIDLIST *ppidl) {
    LPITEMIDLIST pidl;
    int cPidlLen, cPathLen;
    char *pSlash, *pNextSlash, szCompletePath[FILENAME_MAX], *pNextPathElement, *pszAPath;
    WCHAR *pwszPath;
    WIN32_FIND_DATAW find_data;
    BOOL must_exist = TRUE;

    TRACE("pUnixFolder=%p, pbc=%p, path=%s, ppidl=%p\n", pUnixFolder, pbc, debugstr_w(path), ppidl);
   
    if (!ppidl || !path)
        return E_INVALIDARG;

    /* Build an absolute path and let pNextPathElement point to the interesting 
     * relative sub-path. We need the absolute path to call 'stat', but the pidl
     * will only contain the relative part.
     */
    if ((pUnixFolder->m_dwPathMode == PATHMODE_DOS) && (path[1] == ':')) 
    {
        /* Absolute dos path. Convert to unix */
        if (!UNIXFS_get_unix_path(path, szCompletePath))
            return E_FAIL;
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
        *ppidl = pidl = SHAlloc(sizeof(USHORT));
        if (!pidl) return E_FAIL;
        pidl->mkid.cb = 0; /* Terminate the ITEMIDLIST */
        return S_OK;
    }
    
    /* Remove trailing slash, if present */
    cPathLen = strlen(szCompletePath);
    if (szCompletePath[cPathLen-1] == '/') 
        szCompletePath[cPathLen-1] = '\0';

    if ((szCompletePath[0] != '/') || (pNextPathElement[0] != '/')) {
        ERR("szCompletePath: %s, pNextPathElement: %s\n", szCompletePath, pNextPathElement);
        return E_FAIL;
    }
    
    /* At this point, we have an absolute unix path in szCompletePath 
     * and the relative portion of it in pNextPathElement. Both starting with '/'
     * and _not_ terminated by a '/'. */
    TRACE("complete path: %s, relative path: %s\n", szCompletePath, pNextPathElement);
    
    /* Convert to CP_ACP and WCHAR */
    if (!UNIXFS_shitemid_len_from_filename(pNextPathElement, &pszAPath, &pwszPath))
        return E_FAIL;

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

    *ppidl = pidl = SHAlloc(cPidlLen);
    if (!pidl) return E_FAIL;

    if (pbc) {
        IUnknown *unk;
        IFileSystemBindData *fsb;
        HRESULT hr;

        hr = IBindCtx_GetObjectParam(pbc, (LPOLESTR)wFileSystemBindData, &unk);
        if (SUCCEEDED(hr)) {
            hr = IUnknown_QueryInterface(unk, &IID_IFileSystemBindData, (LPVOID*)&fsb);
            if (SUCCEEDED(hr)) {
                hr = IFileSystemBindData_GetFindData(fsb, &find_data);
                if (FAILED(hr))
                    memset(&find_data, 0, sizeof(WIN32_FIND_DATAW));

                must_exist = FALSE;
                IFileSystemBindData_Release(fsb);
            }
            IUnknown_Release(unk);
        }
    }

    /* Concatenate the SHITEMIDs of the sub-directories. */
    while (*pNextPathElement) {
        pSlash = strchr(pNextPathElement+1, '/');
        if (pSlash) *pSlash = '\0';
        pNextPathElement = UNIXFS_build_shitemid(szCompletePath, must_exist,
                must_exist&&!pSlash ? &find_data : NULL, pidl);
        if (pSlash) *pSlash = '/';
            
        if (!pNextPathElement) {
            SHFree(*ppidl);
            *ppidl = NULL;
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        }
        pidl = ILGetNext(pidl);
    }
    pidl->mkid.cb = 0; /* Terminate the ITEMIDLIST */

    if ((char *)pidl-(char *)*ppidl+sizeof(USHORT) != cPidlLen) /* We've corrupted the heap :( */ 
        ERR("Computed length of pidl incorrect. Please report.\n");
    
    return S_OK;
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
    while (!_ILIsEmpty(current)) {
        dwPathLen += UNIXFS_filename_from_shitemid(current, NULL) + 1; /* For the '/' */
        current = ILGetNext(current);
    };

    /* Build the path and compute the attributes */
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
    while (!_ILIsEmpty(current)) {
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
 * UNIXFS_copy [Internal]
 *
 *  Copy pwszDosSrc to pwszDosDst.
 *
 * PARAMS
 *  pwszDosSrc [I]  absolute path of the source
 *  pwszDosDst [I]  absolute path of the destination
 *
 * RETURNS
 *  Success: S_OK,
 *  Failure: E_FAIL
 */
static HRESULT UNIXFS_copy(LPCWSTR pwszDosSrc, LPCWSTR pwszDosDst)
{
    SHFILEOPSTRUCTW op;
    LPWSTR pwszSrc, pwszDst;
    HRESULT res = E_OUTOFMEMORY;
    UINT iSrcLen, iDstLen;

    if (!pwszDosSrc || !pwszDosDst)
        return E_FAIL;

    iSrcLen = lstrlenW(pwszDosSrc);
    iDstLen = lstrlenW(pwszDosDst);
    pwszSrc = HeapAlloc(GetProcessHeap(), 0, (iSrcLen + 2) * sizeof(WCHAR));
    pwszDst = HeapAlloc(GetProcessHeap(), 0, (iDstLen + 2) * sizeof(WCHAR));

    if (pwszSrc && pwszDst) {
        lstrcpyW(pwszSrc, pwszDosSrc);
        lstrcpyW(pwszDst, pwszDosDst);
        /* double null termination */
        pwszSrc[iSrcLen + 1] = 0;
        pwszDst[iDstLen + 1] = 0;

        ZeroMemory(&op, sizeof(op));
        op.hwnd = GetActiveWindow();
        op.wFunc = FO_COPY;
        op.pFrom = pwszSrc;
        op.pTo = pwszDst;
        op.fFlags = FOF_ALLOWUNDO;
        if (SHFileOperationW(&op))
        {
            WARN("SHFileOperationW failed\n");
            res = E_FAIL;
        }
        else
            res = S_OK;
    }

    HeapFree(GetProcessHeap(), 0, pwszSrc);
    HeapFree(GetProcessHeap(), 0, pwszDst);
    return res;
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

static HRESULT WINAPI ShellFolder2_QueryInterface(IShellFolder2 *iface, REFIID riid,
    void **ppv) 
{
    UnixFolder *This = impl_from_IShellFolder2(iface);
        
    TRACE("(%p)->(%s %p)\n", This, shdebugstr_guid(riid), ppv);
    
    if (!ppv) return E_INVALIDARG;
    
    if (IsEqualIID(&IID_IUnknown, riid)     ||
        IsEqualIID(&IID_IShellFolder, riid) ||
        IsEqualIID(&IID_IShellFolder2, riid)) 
    {
        *ppv = &This->IShellFolder2_iface;
    } else if (IsEqualIID(&IID_IPersistFolder3, riid) ||
               IsEqualIID(&IID_IPersistFolder2, riid) ||
               IsEqualIID(&IID_IPersistFolder, riid)  ||
               IsEqualIID(&IID_IPersist, riid))
    {
        *ppv = &This->IPersistFolder3_iface;
    } else if (IsEqualIID(&IID_IPersistPropertyBag, riid)) {
        *ppv = &This->IPersistPropertyBag_iface;
    } else if (IsEqualIID(&IID_ISFHelper, riid)) {
        *ppv = &This->ISFHelper_iface;
    } else if (IsEqualIID(&IID_IDropTarget, riid)) {
        *ppv = &This->IDropTarget_iface;
        if (!cfShellIDList) 
            cfShellIDList = RegisterClipboardFormatW(CFSTR_SHELLIDLISTW);
    } else {
        *ppv = NULL;
        TRACE("Unimplemented interface %s\n", shdebugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ShellFolder2_AddRef(IShellFolder2 *iface)
{
    UnixFolder *This = impl_from_IShellFolder2(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%u)\n", This, ref);
    return ref;
}

static ULONG WINAPI ShellFolder2_Release(IShellFolder2 *iface)
{
    UnixFolder *This = impl_from_IShellFolder2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%u)\n", This, ref);

    if (!ref)
        UnixFolder_Destroy(This);

    return ref;
}

static HRESULT WINAPI ShellFolder2_ParseDisplayName(IShellFolder2* iface, HWND hwndOwner,
    LPBC pbc, LPOLESTR display_name, ULONG* pchEaten, LPITEMIDLIST* ppidl,
    ULONG* attrs)
{
    UnixFolder *This = impl_from_IShellFolder2(iface);
    HRESULT result;

    TRACE("(%p)->(%p %p %s %p %p %p)\n", This, hwndOwner, pbc, debugstr_w(display_name),
          pchEaten, ppidl, attrs);

    result = UNIXFS_path_to_pidl(This, pbc, display_name, ppidl);
    if (SUCCEEDED(result) && attrs && *attrs)
    {
        IShellFolder *parent;
        LPCITEMIDLIST pidlLast;
        LPITEMIDLIST pidlComplete = ILCombine(This->m_pidlLocation, *ppidl);
        HRESULT hr;
        
        hr = SHBindToParent(pidlComplete, &IID_IShellFolder, (void**)&parent, &pidlLast);
        if (FAILED(hr)) {
            FIXME("SHBindToParent failed! hr = 0x%08x\n", hr);
            ILFree(pidlComplete);
            return E_FAIL;
        }
        IShellFolder_GetAttributesOf(parent, 1, &pidlLast, attrs);
        IShellFolder_Release(parent);
        ILFree(pidlComplete);
    }

    if (FAILED(result)) TRACE("FAILED!\n");
    return result;
}

static IEnumIDList *UnixSubFolderIterator_Constructor(UnixFolder *pUnixFolder, SHCONTF fFilter);

static HRESULT WINAPI ShellFolder2_EnumObjects(IShellFolder2* iface, HWND hwndOwner,
    SHCONTF grfFlags, IEnumIDList** ppEnumIDList)
{
    UnixFolder *This = impl_from_IShellFolder2(iface);

    TRACE("(%p)->(%p 0x%08x %p)\n", This, hwndOwner, grfFlags, ppEnumIDList);

    if (!This->m_pszPath) {
        WARN("EnumObjects called on uninitialized UnixFolder-object!\n");
        return E_UNEXPECTED;
    }

    *ppEnumIDList = UnixSubFolderIterator_Constructor(This, grfFlags);
    return S_OK;
}

static HRESULT CreateUnixFolder(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv, const CLSID *pCLSID);

static HRESULT WINAPI ShellFolder2_BindToObject(IShellFolder2* iface, LPCITEMIDLIST pidl,
    LPBC pbcReserved, REFIID riid, void** ppvOut)
{
    UnixFolder *This = impl_from_IShellFolder2(iface);
    IPersistFolder3 *persistFolder;
    const CLSID *clsidChild;
    HRESULT hr;
        
    TRACE("(%p)->(%p %p %s %p)\n", This, pidl, pbcReserved, debugstr_guid(riid), ppvOut);

    if (_ILIsEmpty(pidl))
        return E_INVALIDARG;
   
    /* Don't bind to files */
    if (_ILIsValue(ILFindLastID(pidl)))
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    if (IsEqualCLSID(This->m_pCLSID, &CLSID_FolderShortcut)) {
        /* Children of FolderShortcuts are ShellFSFolders on Windows. 
         * Unixfs' counterpart is UnixDosFolder. */
        clsidChild = &CLSID_UnixDosFolder;    
    } else {
        clsidChild = This->m_pCLSID;
    }

    hr = CreateUnixFolder(NULL, &IID_IPersistFolder3, (void**)&persistFolder, clsidChild);
    if (FAILED(hr)) return hr;
    hr = IPersistFolder3_QueryInterface(persistFolder, riid, ppvOut);

    if (SUCCEEDED(hr)) {
        UnixFolder *subfolder = impl_from_IPersistFolder3(persistFolder);
        subfolder->m_pidlLocation = ILCombine(This->m_pidlLocation, pidl);
        hr = UNIXFS_initialize_target_folder(subfolder, This->m_pszPath, pidl,
                                             This->m_dwAttributes & SFGAO_FILESYSTEM);
    } 

    IPersistFolder3_Release(persistFolder);
    
    return hr;
}

static HRESULT WINAPI ShellFolder2_BindToStorage(IShellFolder2* iface, LPCITEMIDLIST pidl,
    LPBC pbcReserved, REFIID riid, void** ppvObj)
{
    UnixFolder *This = impl_from_IShellFolder2(iface);
    FIXME("(%p)->(%p %p %s %p): stub\n", This, pidl, pbcReserved, debugstr_guid(riid), ppvObj);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellFolder2_CompareIDs(IShellFolder2* iface, LPARAM lParam,
    LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    UnixFolder *This = impl_from_IShellFolder2(iface);
    BOOL isEmpty1, isEmpty2;
    HRESULT hr = E_FAIL;
    LPCITEMIDLIST firstpidl;
    IShellFolder2 *psf;
    int compare;

    TRACE("(%p)->(%ld %p %p)\n", This, lParam, pidl1, pidl2);
    
    isEmpty1 = _ILIsEmpty(pidl1);
    isEmpty2 = _ILIsEmpty(pidl2);

    if (isEmpty1 && isEmpty2) 
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);
    else if (isEmpty1)
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (WORD)-1);
    else if (isEmpty2)
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (WORD)1);

    compare = CompareStringA(LOCALE_USER_DEFAULT, NORM_IGNORECASE, 
                             _ILGetTextPointer(pidl1), -1,
                             _ILGetTextPointer(pidl2), -1);

    if ((compare != CSTR_EQUAL) && _ILIsFolder(pidl1) && !_ILIsFolder(pidl2))
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (WORD)-1);
    if ((compare != CSTR_EQUAL) && !_ILIsFolder(pidl1) && _ILIsFolder(pidl2))
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (WORD)1);

    if ((compare == CSTR_LESS_THAN) || (compare == CSTR_GREATER_THAN)) 
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (WORD)((compare == CSTR_LESS_THAN)?-1:1));

    if (pidl1->mkid.cb < pidl2->mkid.cb)
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (WORD)-1);
    else if (pidl1->mkid.cb > pidl2->mkid.cb)
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (WORD)1);

    firstpidl = pidl1;
    pidl1 = ILGetNext(pidl1);
    pidl2 = ILGetNext(pidl2);

    isEmpty1 = _ILIsEmpty(pidl1);
    isEmpty2 = _ILIsEmpty(pidl2);

    if (isEmpty1 && isEmpty2)
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);
    else if (isEmpty1)
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (WORD)-1);
    else if (isEmpty2)
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (WORD)1);
    else if (SUCCEEDED(IShellFolder2_BindToObject(iface, firstpidl, NULL, &IID_IShellFolder, (void**)&psf))) {
        hr = IShellFolder2_CompareIDs(psf, lParam, pidl1, pidl2);
        IShellFolder2_Release(psf);
    }

    return hr;
}

static HRESULT WINAPI ShellFolder2_CreateViewObject(IShellFolder2* iface, HWND hwndOwner,
    REFIID riid, void** ppv)
{
    UnixFolder *This = impl_from_IShellFolder2(iface);
    HRESULT hr = E_INVALIDARG;

    TRACE("(%p)->(%p %s %p)\n", This, hwndOwner, debugstr_guid(riid), ppv);

    if (!ppv) return E_INVALIDARG;
    *ppv = NULL;

    if (IsEqualIID(&IID_IShellView, riid)) {
        IShellView *view;
        
        view = IShellView_Constructor((IShellFolder*)iface);
        if (view) {
            hr = IShellView_QueryInterface(view, riid, ppv);
            IShellView_Release(view);
        }
    } else if (IsEqualIID(&IID_IDropTarget, riid)) {
        hr = IShellFolder2_QueryInterface(iface, &IID_IDropTarget, ppv);
    }

    return hr;
}

static HRESULT WINAPI ShellFolder2_GetAttributesOf(IShellFolder2* iface, UINT cidl,
    LPCITEMIDLIST* apidl, SFGAOF* attrs)
{
    UnixFolder *This = impl_from_IShellFolder2(iface);
    HRESULT hr = S_OK;
        
    TRACE("(%p)->(%u %p %p)\n", This, cidl, apidl, attrs);
 
    if (!attrs || (cidl && !apidl))
        return E_INVALIDARG;
    
    if (cidl == 0) {
        *attrs &= This->m_dwAttributes;
    } else {
        char szAbsolutePath[FILENAME_MAX], *pszRelativePath;
        UINT i;

        *attrs = SFGAO_CANCOPY | SFGAO_CANMOVE | SFGAO_CANLINK | SFGAO_CANRENAME | SFGAO_CANDELETE |
            SFGAO_HASPROPSHEET | SFGAO_DROPTARGET | SFGAO_FILESYSTEM;
        lstrcpyA(szAbsolutePath, This->m_pszPath);
        pszRelativePath = szAbsolutePath + lstrlenA(szAbsolutePath);
        for (i=0; i<cidl; i++) {
            if (!(This->m_dwAttributes & SFGAO_FILESYSTEM)) {
                WCHAR *dos_name;
                if (!UNIXFS_filename_from_shitemid(apidl[i], pszRelativePath)) 
                    return E_INVALIDARG;
                if (!(dos_name = wine_get_dos_file_name( szAbsolutePath )))
                    *attrs &= ~SFGAO_FILESYSTEM;
                else
                    HeapFree( GetProcessHeap(), 0, dos_name );
            }
            if (_ILIsFolder(apidl[i])) 
                *attrs |= SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_FILESYSANCESTOR |
                    SFGAO_STORAGEANCESTOR | SFGAO_STORAGE;
            else
                *attrs |= SFGAO_STREAM;
        }
    }

    return hr;
}

static HRESULT WINAPI ShellFolder2_GetUIObjectOf(IShellFolder2* iface, HWND hwndOwner,
    UINT cidl, LPCITEMIDLIST* apidl, REFIID riid, UINT* prgfInOut, void** ppvOut)
{
    UnixFolder *This = impl_from_IShellFolder2(iface);
    HRESULT hr;
    UINT i;
    
    TRACE("(%p)->(%p %d %p riid=%s %p %p)\n",
        This, hwndOwner, cidl, apidl, debugstr_guid(riid), prgfInOut, ppvOut);

    if (!cidl || !apidl || !riid || !ppvOut) 
        return E_INVALIDARG;

    for (i=0; i<cidl; i++) 
        if (!apidl[i]) 
            return E_INVALIDARG;

    if(cidl == 1) {
        hr = SHELL32_CreateExtensionUIObject(iface, *apidl, riid, ppvOut);
        if(hr != S_FALSE)
            return hr;
    }

    if (IsEqualIID(&IID_IContextMenu, riid)) {
        return ItemMenu_Constructor((IShellFolder*)iface, This->m_pidlLocation, apidl, cidl, riid, ppvOut);
    } else if (IsEqualIID(&IID_IDataObject, riid)) {
        *ppvOut = IDataObject_Constructor(hwndOwner, This->m_pidlLocation, apidl, cidl);
        return S_OK;
    } else if (IsEqualIID(&IID_IExtractIconA, riid)) {
        LPITEMIDLIST pidl;
        if (cidl != 1) return E_INVALIDARG;
        pidl = ILCombine(This->m_pidlLocation, apidl[0]);
        *ppvOut = IExtractIconA_Constructor(pidl);
        SHFree(pidl);
        return S_OK;
    } else if (IsEqualIID(&IID_IExtractIconW, riid)) {
        LPITEMIDLIST pidl;
        if (cidl != 1) return E_INVALIDARG;
        pidl = ILCombine(This->m_pidlLocation, apidl[0]);
        *ppvOut = IExtractIconW_Constructor(pidl);
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

static HRESULT WINAPI ShellFolder2_GetDisplayNameOf(IShellFolder2* iface,
    LPCITEMIDLIST pidl, SHGDNF uFlags, STRRET* lpName)
{
    UnixFolder *This = impl_from_IShellFolder2(iface);
    SHITEMID emptyIDL = { 0, { 0 } };
    HRESULT hr = S_OK;    

    TRACE("(%p)->(%p 0x%x %p)\n", This, pidl, uFlags, lpName);
    
    if ((GET_SHGDN_FOR(uFlags) & SHGDN_FORPARSING) &&
        (GET_SHGDN_RELATION(uFlags) != SHGDN_INFOLDER))
    {
        if (_ILIsEmpty(pidl)) {
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
        } else if (_ILIsValue(pidl)) {
            STRRET str;
            PWSTR path, file;

            /* We are looking for the complete path to a file */

            /* Get the complete path for the current folder object */
            hr = IShellFolder2_GetDisplayNameOf(iface, (LPITEMIDLIST)&emptyIDL, uFlags, &str);
            if (SUCCEEDED(hr)) {
                hr = StrRetToStrW(&str, NULL, &path);
                if (SUCCEEDED(hr)) {

                    /* Get the child filename */
                    hr = IShellFolder2_GetDisplayNameOf(iface, pidl, SHGDN_FORPARSING | SHGDN_INFOLDER, &str);
                    if (SUCCEEDED(hr)) {
                        hr = StrRetToStrW(&str, NULL, &file);
                        if (SUCCEEDED(hr)) {
                            static const WCHAR slashW = '/';
                            UINT len_path = strlenW(path), len_file = strlenW(file);

                            /* Now, combine them */
                            lpName->uType = STRRET_WSTR;
                            lpName->u.pOleStr = SHAlloc( (len_path + len_file + 2)*sizeof(WCHAR) );
                            lstrcpyW(lpName->u.pOleStr, path);
                            if (This->m_dwPathMode == PATHMODE_UNIX &&
                               lpName->u.pOleStr[len_path-1] != slashW) {
                                lpName->u.pOleStr[len_path] = slashW;
                                lpName->u.pOleStr[len_path+1] = '\0';
                            } else
                                PathAddBackslashW(lpName->u.pOleStr);
                            lstrcatW(lpName->u.pOleStr, file);

                            CoTaskMemFree(file);
                        } else
                            WARN("Failed to convert strret (file)\n");
                    }
                    CoTaskMemFree(path);
                } else
                    WARN("Failed to convert strret (path)\n");
            }
        } else {
            IShellFolder *pSubFolder;

            hr = IShellFolder2_BindToObject(iface, pidl, NULL, &IID_IShellFolder, (void**)&pSubFolder);
            if (SUCCEEDED(hr)) {
                hr = IShellFolder_GetDisplayNameOf(pSubFolder, (LPITEMIDLIST)&emptyIDL, uFlags, lpName);
                IShellFolder_Release(pSubFolder);
            } else if (FAILED(hr) && !_ILIsPidlSimple(pidl)) {
                LPITEMIDLIST pidl_parent = ILClone(pidl);
                LPITEMIDLIST pidl_child = ILFindLastID(pidl);

                /* Might be a file, try binding to its parent */
                ILRemoveLastID(pidl_parent);
                hr = IShellFolder2_BindToObject(iface, pidl_parent, NULL, &IID_IShellFolder, (void**)&pSubFolder);
                if (SUCCEEDED(hr)) {
                    hr = IShellFolder_GetDisplayNameOf(pSubFolder, pidl_child, uFlags, lpName);
                    IShellFolder_Release(pSubFolder);
                }
                ILFree(pidl_parent);
            }
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

static HRESULT WINAPI ShellFolder2_SetNameOf(IShellFolder2* iface, HWND hwnd,
    LPCITEMIDLIST pidl, LPCOLESTR lpcwszName, SHGDNF uFlags, LPITEMIDLIST* ppidlOut)
{
    UnixFolder *This = impl_from_IShellFolder2(iface);

    static const WCHAR awcInvalidChars[] = { '\\', '/', ':', '*', '?', '"', '<', '>', '|' };
    char szSrc[FILENAME_MAX], szDest[FILENAME_MAX];
    WCHAR wszSrcRelative[MAX_PATH], *pwszExt = NULL;
    unsigned int i;
    int cBasePathLen = lstrlenA(This->m_pszPath), cNameLen;
    struct stat statDest;
    LPITEMIDLIST pidlSrc, pidlDest, pidlRelativeDest;
    LPOLESTR lpwszName;
    HRESULT hr;
   
    TRACE("(%p)->(%p %p %s 0x%08x %p)\n", This, hwnd, pidl, debugstr_w(lpcwszName), uFlags, ppidlOut);

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
        int cLenDest = strlen(szDest);
        pwszExt = PathFindExtensionW(wszSrcRelative);
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
    cNameLen = lstrlenW(lpcwszName) + 1;
    if(pwszExt)
        cNameLen += lstrlenW(pwszExt);
    lpwszName = SHAlloc(cNameLen*sizeof(WCHAR)); /* due to const correctness. */
    lstrcpyW(lpwszName, lpcwszName);
    if(pwszExt)
        lstrcatW(lpwszName, pwszExt);

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

static HRESULT WINAPI ShellFolder2_EnumSearches(IShellFolder2* iface, IEnumExtraSearch **ppEnum)
{
    UnixFolder *This = impl_from_IShellFolder2(iface);
    FIXME("(%p)->(%p): stub\n", This, ppEnum);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellFolder2_GetDefaultColumn(IShellFolder2* iface,
    DWORD dwReserved, ULONG *pSort, ULONG *pDisplay)
{
    UnixFolder *This = impl_from_IShellFolder2(iface);

    TRACE("(%p)->(0x%x %p %p)\n", This, dwReserved, pSort, pDisplay);

    if (pSort)
        *pSort = 0;
    if (pDisplay)
        *pDisplay = 0;

    return S_OK;
}

static HRESULT WINAPI ShellFolder2_GetDefaultColumnState(IShellFolder2* iface,
    UINT column, SHCOLSTATEF *flags)
{
    UnixFolder *This = impl_from_IShellFolder2(iface);
    FIXME("(%p)->(%u %p): stub\n", This, column, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellFolder2_GetDefaultSearchGUID(IShellFolder2* iface,
    GUID *guid)
{
    UnixFolder *This = impl_from_IShellFolder2(iface);
    FIXME("(%p)->(%p): stub\n", This, guid);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellFolder2_GetDetailsEx(IShellFolder2* iface,
    LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    UnixFolder *This = impl_from_IShellFolder2(iface);
    FIXME("(%p)->(%p %p %p): stub\n", This, pidl, pscid, pv);
    return E_NOTIMPL;
}

#define SHELLVIEWCOLUMNS 7 

static HRESULT WINAPI ShellFolder2_GetDetailsOf(IShellFolder2* iface,
    LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *psd)
{
    UnixFolder *This = impl_from_IShellFolder2(iface);
    HRESULT hr = E_FAIL;
    struct passwd *pPasswd;
    struct group *pGroup;
    struct stat statItem;

    static const shvheader unixfs_header[SHELLVIEWCOLUMNS] = {
        {IDS_SHV_COLUMN1,  SHCOLSTATE_TYPE_STR  | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15},
        {IDS_SHV_COLUMN2,  SHCOLSTATE_TYPE_STR  | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
        {IDS_SHV_COLUMN3,  SHCOLSTATE_TYPE_STR  | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
        {IDS_SHV_COLUMN4,  SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 12},
        {IDS_SHV_COLUMN5,  SHCOLSTATE_TYPE_STR  | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 9},
        {IDS_SHV_COLUMN10, SHCOLSTATE_TYPE_STR  | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 7},
        {IDS_SHV_COLUMN11, SHCOLSTATE_TYPE_STR  | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 7}
    };

    TRACE("(%p)->(%p %d %p)\n", This, pidl, iColumn, psd);
    
    if (!psd || iColumn >= SHELLVIEWCOLUMNS)
        return E_INVALIDARG;

    if (!pidl)
        return SHELL32_GetColumnDetails(unixfs_header, iColumn, psd);

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
    
    return hr;
}

static HRESULT WINAPI ShellFolder2_MapColumnToSCID(IShellFolder2* iface, UINT column,
    SHCOLUMNID *pscid)
{
    UnixFolder *This = impl_from_IShellFolder2(iface);
    FIXME("(%p)->(%u %p): stub\n", This, column, pscid);
    return E_NOTIMPL;
}

static const IShellFolder2Vtbl ShellFolder2Vtbl = {
    ShellFolder2_QueryInterface,
    ShellFolder2_AddRef,
    ShellFolder2_Release,
    ShellFolder2_ParseDisplayName,
    ShellFolder2_EnumObjects,
    ShellFolder2_BindToObject,
    ShellFolder2_BindToStorage,
    ShellFolder2_CompareIDs,
    ShellFolder2_CreateViewObject,
    ShellFolder2_GetAttributesOf,
    ShellFolder2_GetUIObjectOf,
    ShellFolder2_GetDisplayNameOf,
    ShellFolder2_SetNameOf,
    ShellFolder2_GetDefaultSearchGUID,
    ShellFolder2_EnumSearches,
    ShellFolder2_GetDefaultColumn,
    ShellFolder2_GetDefaultColumnState,
    ShellFolder2_GetDetailsEx,
    ShellFolder2_GetDetailsOf,
    ShellFolder2_MapColumnToSCID
};

static HRESULT WINAPI PersistFolder3_QueryInterface(IPersistFolder3* iface, REFIID riid,
    void** ppvObject)
{
    UnixFolder *This = impl_from_IPersistFolder3(iface);
    return IShellFolder2_QueryInterface(&This->IShellFolder2_iface, riid, ppvObject);
}

static ULONG WINAPI PersistFolder3_AddRef(IPersistFolder3* iface)
{
    UnixFolder *This = impl_from_IPersistFolder3(iface);
    return IShellFolder2_AddRef(&This->IShellFolder2_iface);
}

static ULONG WINAPI PersistFolder3_Release(IPersistFolder3* iface)
{
    UnixFolder *This = impl_from_IPersistFolder3(iface);
    return IShellFolder2_Release(&This->IShellFolder2_iface);
}

static HRESULT WINAPI PersistFolder3_GetClassID(IPersistFolder3* iface, CLSID* pClassID)
{
    UnixFolder *This = impl_from_IPersistFolder3(iface);
    
    TRACE("(%p)->(%p)\n", This, pClassID);
    
    if (!pClassID)
        return E_INVALIDARG;

    *pClassID = *This->m_pCLSID;
    return S_OK;
}

static HRESULT WINAPI PersistFolder3_Initialize(IPersistFolder3* iface, LPCITEMIDLIST pidl)
{
    UnixFolder *This = impl_from_IPersistFolder3(iface);
    LPCITEMIDLIST current = pidl;
    char szBasePath[FILENAME_MAX] = "/";
    
    TRACE("(%p)->(%p)\n", This, pidl);

    /* Find the UnixFolderClass root */
    while (current->mkid.cb) {
        if ((_ILIsDrive(current) && IsEqualCLSID(This->m_pCLSID, &CLSID_ShellFSFolder)) ||
            (_ILIsSpecialFolder(current) && IsEqualCLSID(This->m_pCLSID, _ILGetGUIDPointer(current))))
        {
            break;
        }
        current = ILGetNext(current);
    }

    if (current->mkid.cb) {
        if (_ILIsDrive(current)) {
            WCHAR wszDrive[] = { '?', ':', '\\', 0 };
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

static HRESULT WINAPI PersistFolder3_GetCurFolder(IPersistFolder3* iface, LPITEMIDLIST* ppidl)
{
    UnixFolder *This = impl_from_IPersistFolder3(iface);
    
    TRACE ("(iface=%p, ppidl=%p)\n", iface, ppidl);

    if (!ppidl)
        return E_POINTER;
    *ppidl = ILClone (This->m_pidlLocation);
    return S_OK;
}

static HRESULT WINAPI PersistFolder3_InitializeEx(IPersistFolder3 *iface, IBindCtx *pbc,
    LPCITEMIDLIST pidlRoot, const PERSIST_FOLDER_TARGET_INFO *ppfti)
{
    UnixFolder *This = impl_from_IPersistFolder3(iface);
    WCHAR wszTargetDosPath[MAX_PATH];
    char szTargetPath[FILENAME_MAX] = "";
    
    TRACE("(%p)->(%p %p %p)\n", This, pbc, pidlRoot, ppfti);

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

static HRESULT WINAPI PersistFolder3_GetFolderTargetInfo(IPersistFolder3 *iface,
    PERSIST_FOLDER_TARGET_INFO *ppfti)
{
    UnixFolder *This = impl_from_IPersistFolder3(iface);
    FIXME("(%p)->(%p): stub\n", This, ppfti);
    return E_NOTIMPL;
}

static const IPersistFolder3Vtbl PersistFolder3Vtbl = {
    PersistFolder3_QueryInterface,
    PersistFolder3_AddRef,
    PersistFolder3_Release,
    PersistFolder3_GetClassID,
    PersistFolder3_Initialize,
    PersistFolder3_GetCurFolder,
    PersistFolder3_InitializeEx,
    PersistFolder3_GetFolderTargetInfo
};

static HRESULT WINAPI PersistPropertyBag_QueryInterface(IPersistPropertyBag* iface,
    REFIID riid, void** ppv)
{
    UnixFolder *This = impl_from_IPersistPropertyBag(iface);
    return IShellFolder2_QueryInterface(&This->IShellFolder2_iface, riid, ppv);
}

static ULONG WINAPI PersistPropertyBag_AddRef(IPersistPropertyBag* iface)
{
    UnixFolder *This = impl_from_IPersistPropertyBag(iface);
    return IShellFolder2_AddRef(&This->IShellFolder2_iface);
}

static ULONG WINAPI PersistPropertyBag_Release(IPersistPropertyBag* iface)
{
    UnixFolder *This = impl_from_IPersistPropertyBag(iface);
    return IShellFolder2_Release(&This->IShellFolder2_iface);
}

static HRESULT WINAPI PersistPropertyBag_GetClassID(IPersistPropertyBag* iface, CLSID* pClassID)
{
    UnixFolder *This = impl_from_IPersistPropertyBag(iface);
    return IPersistFolder3_GetClassID(&This->IPersistFolder3_iface, pClassID);
}

static HRESULT WINAPI PersistPropertyBag_InitNew(IPersistPropertyBag* iface)
{
    UnixFolder *This = impl_from_IPersistPropertyBag(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistPropertyBag_Load(IPersistPropertyBag *iface,
    IPropertyBag *pPropertyBag, IErrorLog *pErrorLog)
{
    UnixFolder *This = impl_from_IPersistPropertyBag(iface);

     static const WCHAR wszTarget[] = { 'T','a','r','g','e','t', 0 }, wszNull[] = { 0 };
     PERSIST_FOLDER_TARGET_INFO pftiTarget;
     VARIANT var;
     HRESULT hr;
     
     TRACE("(%p)->(%p %p)\n", This, pPropertyBag, pErrorLog);
 
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
 
     return IPersistFolder3_InitializeEx(&This->IPersistFolder3_iface, NULL, NULL, &pftiTarget);
}

static HRESULT WINAPI PersistPropertyBag_Save(IPersistPropertyBag *iface,
    IPropertyBag *pPropertyBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    UnixFolder *This = impl_from_IPersistPropertyBag(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static const IPersistPropertyBagVtbl PersistPropertyBagVtbl = {
    PersistPropertyBag_QueryInterface,
    PersistPropertyBag_AddRef,
    PersistPropertyBag_Release,
    PersistPropertyBag_GetClassID,
    PersistPropertyBag_InitNew,
    PersistPropertyBag_Load,
    PersistPropertyBag_Save
};

static HRESULT WINAPI SFHelper_QueryInterface(ISFHelper* iface, REFIID riid, void** ppvObject)
{
    UnixFolder *This = impl_from_ISFHelper(iface);
    return IShellFolder2_QueryInterface(&This->IShellFolder2_iface, riid, ppvObject);
}

static ULONG WINAPI SFHelper_AddRef(ISFHelper* iface)
{
    UnixFolder *This = impl_from_ISFHelper(iface);
    return IShellFolder2_AddRef(&This->IShellFolder2_iface);
}

static ULONG WINAPI SFHelper_Release(ISFHelper* iface)
{
    UnixFolder *This = impl_from_ISFHelper(iface);
    return IShellFolder2_Release(&This->IShellFolder2_iface);
}

static HRESULT WINAPI SFHelper_GetUniqueName(ISFHelper* iface, LPWSTR pwszName, UINT uLen)
{
    UnixFolder *This = impl_from_ISFHelper(iface);
    IEnumIDList *pEnum;
    HRESULT hr;
    LPITEMIDLIST pidlElem;
    DWORD dwFetched;
    int i;
    WCHAR wszNewFolder[25];
    static const WCHAR wszFormat[] = { '%','s',' ','%','d',0 };

    TRACE("(%p)->(%p %u)\n", This, pwszName, uLen);

    LoadStringW(shell32_hInstance, IDS_NEWFOLDER, wszNewFolder, sizeof(wszNewFolder)/sizeof(WCHAR));

    if (uLen < sizeof(wszNewFolder)/sizeof(WCHAR)+3)
        return E_INVALIDARG;

    hr = IShellFolder2_EnumObjects(&This->IShellFolder2_iface, 0,
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

static HRESULT WINAPI SFHelper_AddFolder(ISFHelper* iface, HWND hwnd, LPCWSTR pwszName,
    LPITEMIDLIST* ppidlOut)
{
    UnixFolder *This = impl_from_ISFHelper(iface);
    char szNewDir[FILENAME_MAX];
    int cBaseLen;

    TRACE("(%p)->(%p %s %p)\n", This, hwnd, debugstr_w(pwszName), ppidlOut);

    if (ppidlOut)
        *ppidlOut = NULL;

    if (!This->m_pszPath || !(This->m_dwAttributes & SFGAO_FILESYSTEM))
        return E_FAIL;
    
    lstrcpynA(szNewDir, This->m_pszPath, FILENAME_MAX);
    cBaseLen = lstrlenA(szNewDir);
    WideCharToMultiByte(CP_UNIXCP, 0, pwszName, -1, szNewDir+cBaseLen, FILENAME_MAX-cBaseLen, 0, 0);
   
    if (mkdir(szNewDir, 0777)) {
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
        if (SUCCEEDED(UNIXFS_path_to_pidl(This, NULL, pwszName, &pidlRelative))) {
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
static HRESULT UNIXFS_delete_with_shfileop(UnixFolder *This, UINT cidl, const LPCITEMIDLIST *apidl)
{
    char szAbsolute[FILENAME_MAX], *pszRelative;
    LPWSTR wszPathsList, wszListPos;
    SHFILEOPSTRUCTW op;
    HRESULT ret;
    UINT i;
    
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
    if (SHFileOperationW(&op))
    {
        WARN("SHFileOperationW failed\n");
        ret = E_FAIL;
    }
    else
        ret = S_OK;

    HeapFree(GetProcessHeap(), 0, wszPathsList);
    return ret;
}

static HRESULT UNIXFS_delete_with_syscalls(UnixFolder *This, UINT cidl, const LPCITEMIDLIST *apidl)
{
    char szAbsolute[FILENAME_MAX], *pszRelative;
    static const WCHAR empty[] = {0};
    UINT i;
    
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

static HRESULT WINAPI SFHelper_DeleteItems(ISFHelper* iface, UINT cidl, LPCITEMIDLIST* apidl)
{
    UnixFolder *This = impl_from_ISFHelper(iface);
    char szAbsolute[FILENAME_MAX], *pszRelative;
    LPITEMIDLIST pidlAbsolute;
    HRESULT hr = S_OK;
    UINT i;
    struct stat st;
    
    TRACE("(%p)->(%d %p)\n", This, cidl, apidl);

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

static HRESULT WINAPI SFHelper_CopyItems(ISFHelper* iface, IShellFolder *psfFrom,
    UINT cidl, LPCITEMIDLIST *apidl)
{
    UnixFolder *This = impl_from_ISFHelper(iface);
    DWORD dwAttributes;
    UINT i;
    HRESULT hr;
    char szAbsoluteDst[FILENAME_MAX], *pszRelativeDst;
    
    TRACE("(%p)->(%p %d %p)\n", This, psfFrom, cidl, apidl);

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
        HRESULT res;
        WCHAR *pwszDosSrc, *pwszDosDst;

        /* Build the unix path of the current source item. */
        if (FAILED(IShellFolder_GetDisplayNameOf(psfFrom, apidl[i], SHGDN_FORPARSING, &strret)))
            return E_FAIL;
        if (FAILED(StrRetToBufW(&strret, apidl[i], wszSrc, MAX_PATH)))
            return E_FAIL;
        if (!UNIXFS_get_unix_path(wszSrc, szSrc)) 
            return E_FAIL;

        /* Build the unix path of the current destination item */
        UNIXFS_filename_from_shitemid(apidl[i], pszRelativeDst);

        pwszDosSrc = wine_get_dos_file_name(szSrc);
        pwszDosDst = wine_get_dos_file_name(szAbsoluteDst);

        if (pwszDosSrc && pwszDosDst)
            res = UNIXFS_copy(pwszDosSrc, pwszDosDst);
        else
            res = E_OUTOFMEMORY;

        HeapFree(GetProcessHeap(), 0, pwszDosSrc);
        HeapFree(GetProcessHeap(), 0, pwszDosDst);

        if (res != S_OK)
            return res;
    }
    return S_OK;
}

static const ISFHelperVtbl SFHelperVtbl = {
    SFHelper_QueryInterface,
    SFHelper_AddRef,
    SFHelper_Release,
    SFHelper_GetUniqueName,
    SFHelper_AddFolder,
    SFHelper_DeleteItems,
    SFHelper_CopyItems
};

static HRESULT WINAPI DropTarget_QueryInterface(IDropTarget* iface, REFIID riid, void** ppvObject)
{
    UnixFolder *This = impl_from_IDropTarget(iface);
    return IShellFolder2_QueryInterface(&This->IShellFolder2_iface, riid, ppvObject);
}

static ULONG WINAPI DropTarget_AddRef(IDropTarget* iface)
{
    UnixFolder *This = impl_from_IDropTarget(iface);
    return IShellFolder2_AddRef(&This->IShellFolder2_iface);
}

static ULONG WINAPI DropTarget_Release(IDropTarget* iface)
{
    UnixFolder *This = impl_from_IDropTarget(iface);
    return IShellFolder2_Release(&This->IShellFolder2_iface);
}

#define HIDA_GetPIDLFolder(pida) (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[0])
#define HIDA_GetPIDLItem(pida, i) (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[i+1])

static HRESULT WINAPI DropTarget_DragEnter(IDropTarget *iface, IDataObject *pDataObject,
    DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
{
    UnixFolder *This = impl_from_IDropTarget(iface);
    FORMATETC format;
    STGMEDIUM medium; 
        
    TRACE("(%p)->(%p 0x%08x {.x=%d, .y=%d} %p)\n", This, pDataObject, dwKeyState, pt.x, pt.y, pdwEffect);

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

static HRESULT WINAPI DropTarget_DragOver(IDropTarget *iface, DWORD dwKeyState,
    POINTL pt, DWORD *pdwEffect)
{
    UnixFolder *This = impl_from_IDropTarget(iface);

    TRACE("(%p)->(0x%08x {.x=%d, .y=%d} %p)\n", This, dwKeyState, pt.x, pt.y, pdwEffect);

    if (!pdwEffect)
        return E_INVALIDARG;

    *pdwEffect = KeyStateToDropEffect(dwKeyState) & This->m_dwDropEffectsMask;
    
    return S_OK;
}

static HRESULT WINAPI DropTarget_DragLeave(IDropTarget *iface)
{
    UnixFolder *This = impl_from_IDropTarget(iface);

    TRACE("(%p)\n", This);

    This->m_dwDropEffectsMask = DROPEFFECT_NONE;
    return S_OK;
}

static HRESULT WINAPI DropTarget_Drop(IDropTarget *iface, IDataObject *pDataObject,
    DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
{
    UnixFolder *This = impl_from_IDropTarget(iface);
    FORMATETC format;
    STGMEDIUM medium; 
    HRESULT hr;

    TRACE("(%p)->(%p %d {.x=%d, .y=%d} %p) semi-stub\n",
        This, pDataObject, dwKeyState, pt.x, pt.y, pdwEffect);

    InitFormatEtc(format, cfShellIDList, TYMED_HGLOBAL);
    hr = IDataObject_GetData(pDataObject, &format, &medium);
    if (FAILED(hr))
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

static const IDropTargetVtbl DropTargetVtbl = {
    DropTarget_QueryInterface,
    DropTarget_AddRef,
    DropTarget_Release,
    DropTarget_DragEnter,
    DropTarget_DragOver,
    DropTarget_DragLeave,
    DropTarget_Drop
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
static HRESULT CreateUnixFolder(IUnknown *outer, REFIID riid, void **ppv, const CLSID *clsid)
{
    UnixFolder *This;
    HRESULT hr;
   
    if (outer) {
        FIXME("Aggregation not yet implemented!\n");
        return CLASS_E_NOAGGREGATION;
    }
    
    This = SHAlloc((ULONG)sizeof(UnixFolder));
    if (!This) return E_OUTOFMEMORY;

    This->IShellFolder2_iface.lpVtbl = &ShellFolder2Vtbl;
    This->IPersistFolder3_iface.lpVtbl = &PersistFolder3Vtbl;
    This->IPersistPropertyBag_iface.lpVtbl = &PersistPropertyBagVtbl;
    This->ISFHelper_iface.lpVtbl = &SFHelperVtbl;
    This->IDropTarget_iface.lpVtbl = &DropTargetVtbl;
    This->ref = 1;
    This->m_pszPath = NULL;
    This->m_pidlLocation = NULL;
    This->m_dwPathMode = IsEqualCLSID(&CLSID_UnixFolder, clsid) ? PATHMODE_UNIX : PATHMODE_DOS;
    This->m_dwAttributes = 0;
    This->m_pCLSID = clsid;
    This->m_dwDropEffectsMask = DROPEFFECT_NONE;

    hr = IShellFolder2_QueryInterface(&This->IShellFolder2_iface, riid, ppv);
    IShellFolder2_Release(&This->IShellFolder2_iface);

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
    IEnumIDList IEnumIDList_iface;
    LONG ref;
    SHCONTF m_fFilter;
    DIR *m_dirFolder;
    char m_szFolder[FILENAME_MAX];
} UnixSubFolderIterator;

static inline UnixSubFolderIterator *impl_from_IEnumIDList(IEnumIDList *iface)
{
    return CONTAINING_RECORD(iface, UnixSubFolderIterator, IEnumIDList_iface);
}

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
    UnixSubFolderIterator *This = impl_from_IEnumIDList(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI UnixSubFolderIterator_IEnumIDList_Release(IEnumIDList* iface)
{
    UnixSubFolderIterator *This = impl_from_IEnumIDList(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if (!ref)
        UnixSubFolderIterator_Destroy(This);

    return ref;
}

static HRESULT WINAPI UnixSubFolderIterator_IEnumIDList_Next(IEnumIDList* iface, ULONG celt, 
    LPITEMIDLIST* rgelt, ULONG* pceltFetched)
{
    UnixSubFolderIterator *This = impl_from_IEnumIDList(iface);
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
            rgelt[i] = SHAlloc(
                UNIXFS_shitemid_len_from_filename(pszRelativePath, NULL, NULL)+sizeof(USHORT));
            if (!UNIXFS_build_shitemid(This->m_szFolder, TRUE, NULL, rgelt[i]) ||
                !UNIXFS_is_pidl_of_type(rgelt[i], This->m_fFilter)) 
            {
                SHFree(rgelt[i]);
                rgelt[i] = NULL;
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
    apidl = SHAlloc(celt * sizeof(LPITEMIDLIST));
    hr = IEnumIDList_Next(iface, celt, apidl, &cFetched);
    if (SUCCEEDED(hr))
        while (cFetched--) 
            SHFree(apidl[cFetched]);
    SHFree(apidl);

    return hr;
}

static HRESULT WINAPI UnixSubFolderIterator_IEnumIDList_Reset(IEnumIDList* iface)
{
    UnixSubFolderIterator *This = impl_from_IEnumIDList(iface);

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

static IEnumIDList *UnixSubFolderIterator_Constructor(UnixFolder *pUnixFolder, SHCONTF fFilter)
{
    UnixSubFolderIterator *iterator;

    TRACE("(pUnixFolder=%p)\n", pUnixFolder);

    iterator = SHAlloc(sizeof(*iterator));
    iterator->IEnumIDList_iface.lpVtbl = &UnixSubFolderIterator_IEnumIDList_Vtbl;
    iterator->ref = 1;
    iterator->m_fFilter = fFilter;
    iterator->m_dirFolder = opendir(pUnixFolder->m_pszPath);
    lstrcpyA(iterator->m_szFolder, pUnixFolder->m_pszPath);

    return &iterator->IEnumIDList_iface;
}

#else /* __MINGW32__ || _MSC_VER */

HRESULT WINAPI UnixFolder_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv)
{
    return E_NOTIMPL;
}

HRESULT WINAPI UnixDosFolder_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv)
{
    return E_NOTIMPL;
}

HRESULT WINAPI FolderShortcut_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv)
{
    return E_NOTIMPL;
}

HRESULT WINAPI MyDocuments_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv)
{
    return E_NOTIMPL;
}

#endif /* __MINGW32__ || _MSC_VER */

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
