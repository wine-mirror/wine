/*
 * The freedesktop.org Trash, implemented using the 0.7 spec version
 * (see http://www.ramendik.ru/docs/trashspec.html)
 *
 * Copyright (C) 2006 Mikolaj Zalewski
 * Copyright 2011 Jay Yang
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

#include "config.h"

#ifdef HAVE_CORESERVICES_CORESERVICES_H
#define GetCurrentThread MacGetCurrentThread
#define LoadResource MacLoadResource
#include <CoreServices/CoreServices.h>
#undef GetCurrentThread
#undef LoadResource
#undef DPRINTF
#endif

#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#include <sys/types.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_DIRENT_H
# include <dirent.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "shlwapi.h"
#include "winternl.h"
#include "wine/debug.h"
#include "shell32_main.h"
#include "xdg.h"

WINE_DEFAULT_DEBUG_CHANNEL(trash);

#ifdef HAVE_CORESERVICES_CORESERVICES_H

BOOL TRASH_CanTrashFile(LPCWSTR wszPath)
{
    char *unix_path;
    OSStatus status;
    FSRef ref;
    FSCatalogInfo catalogInfo;

    TRACE("(%s)\n", debugstr_w(wszPath));
    if (!(unix_path = wine_get_unix_file_name(wszPath)))
        return FALSE;

    status = FSPathMakeRef((UInt8*)unix_path, &ref, NULL);
    HeapFree(GetProcessHeap(), 0, unix_path);
    if (status == noErr)
        status = FSGetCatalogInfo(&ref, kFSCatInfoVolume, &catalogInfo, NULL,
                                  NULL, NULL);
    if (status == noErr)
        status = FSFindFolder(catalogInfo.volume, kTrashFolderType,
                              kCreateFolder, &ref);

    return (status == noErr);
}

BOOL TRASH_TrashFile(LPCWSTR wszPath)
{
    char *unix_path;
    OSStatus status;

    TRACE("(%s)\n", debugstr_w(wszPath));
    if (!(unix_path = wine_get_unix_file_name(wszPath)))
        return FALSE;

    status = FSPathMoveObjectToTrashSync(unix_path, NULL, kFSFileOperationSkipPreflight);

    HeapFree(GetProcessHeap(), 0, unix_path);
    return (status == noErr);
}

HRESULT TRASH_EnumItems(LPITEMIDLIST **pidls, int *count)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

HRESULT TRASH_UnpackItemID(LPCSHITEMID id, WIN32_FIND_DATAW *data)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

HRESULT TRASH_RestoreItem(LPCITEMIDLIST pidl)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

HRESULT TRASH_EraseItem(LPCITEMIDLIST pidl)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

#else /* HAVE_CORESERVICES_CORESERVICES_H */

static CRITICAL_SECTION TRASH_Creating;
static CRITICAL_SECTION_DEBUG TRASH_Creating_Debug =
{
    0, 0, &TRASH_Creating,
    { &TRASH_Creating_Debug.ProcessLocksList,
      &TRASH_Creating_Debug.ProcessLocksList},
    0, 0, { (DWORD_PTR)__FILE__ ": TRASH_Creating"}
};
static CRITICAL_SECTION TRASH_Creating = { &TRASH_Creating_Debug, -1, 0, 0, 0, 0 };

static const char trashinfo_suffix[] = ".trashinfo";
static const char trashinfo_header[] = "[Trash Info]\n";
static const char trashinfo_group[] = "Trash Info";

typedef struct
{
    char *info_dir;
    char *files_dir;
    dev_t device;
} TRASH_BUCKET;

static TRASH_BUCKET *home_trash=NULL;

static char *init_home_dir(const char *subpath)
{
    char *path = XDG_BuildPath(XDG_DATA_HOME, subpath);
    if (path == NULL) return NULL;
    if (!XDG_MakeDirs(path))
    {
        ERR("Couldn't create directory %s (errno=%d). Trash won't be available\n", debugstr_a(path), errno);
        SHFree(path);
        path=NULL;
    }
    return path;
}

static TRASH_BUCKET *TRASH_CreateHomeBucket(void)
{
    TRASH_BUCKET *bucket;
    struct stat trash_stat;
    char *trash_path = NULL;
    
    bucket = SHAlloc(sizeof(TRASH_BUCKET));
    if (bucket == NULL)
    {
        errno = ENOMEM;
        goto error;
    }
    memset(bucket, 0, sizeof(*bucket));
    bucket->info_dir = init_home_dir("Trash/info/");
    if (bucket->info_dir == NULL) goto error;
    bucket->files_dir = init_home_dir("Trash/files/");
    if (bucket->files_dir == NULL) goto error;
    
    trash_path = XDG_BuildPath(XDG_DATA_HOME, "Trash/");
    if (stat(trash_path, &trash_stat) == -1)
        goto error;
    bucket->device = trash_stat.st_dev;
    SHFree(trash_path);
    return bucket;
error:
    SHFree(trash_path);
    if (bucket)
    {
        SHFree(bucket->info_dir);
        SHFree(bucket->files_dir);
    }
    SHFree(bucket);
    return NULL;
}

static BOOL TRASH_EnsureInitialized(void)
{
    if (home_trash == NULL)
    {
        EnterCriticalSection(&TRASH_Creating);
        if (home_trash == NULL)
            home_trash = TRASH_CreateHomeBucket();
        LeaveCriticalSection(&TRASH_Creating);
    }

    if (home_trash == NULL)
    {
        ERR("Couldn't initialize home trash (errno=%d)\n", errno);
        return FALSE;
    }
    return TRUE;
}

static BOOL file_good_for_bucket(const TRASH_BUCKET *pBucket, const struct stat *file_stat)
{
    if (pBucket->device != file_stat->st_dev)
        return FALSE;
    return TRUE;
}

BOOL TRASH_CanTrashFile(LPCWSTR wszPath)
{
    struct stat file_stat;
    char *unix_path;
    
    TRACE("(%s)\n", debugstr_w(wszPath));
    if (!TRASH_EnsureInitialized()) return FALSE;
    if (!(unix_path = wine_get_unix_file_name(wszPath)))
        return FALSE;
    if (lstat(unix_path, &file_stat)==-1)
    {
        HeapFree(GetProcessHeap(), 0, unix_path);
        return FALSE;
    }
    HeapFree(GetProcessHeap(), 0, unix_path);
    return file_good_for_bucket(home_trash, &file_stat);
}

/*
 * Try to create a single .trashinfo file. Return TRUE if successful, else FALSE
 */
static BOOL try_create_trashinfo_file(const char *info_dir, const char *file_name,
    const char *original_file_name)
{
    SYSTEMTIME curr_time;
    char datebuf[200];
    char *path = SHAlloc(strlen(info_dir)+strlen(file_name)+strlen(trashinfo_suffix)+1);
    int writer = -1;
    
    if (path==NULL) return FALSE;
    wsprintfA(path, "%s%s%s", info_dir, file_name, trashinfo_suffix);
    TRACE("Trying to create '%s'\n", path);
    writer = open(path, O_CREAT|O_WRONLY|O_TRUNC|O_EXCL, 0600);
    if (writer==-1) goto error;
    
    write(writer, trashinfo_header, strlen(trashinfo_header));
    if (!XDG_WriteDesktopStringEntry(writer, "Path", XDG_URLENCODE, original_file_name))
        goto error;

    GetLocalTime( &curr_time );
    wnsprintfA(datebuf, 200, "%04d-%02d-%02dT%02d:%02d:%02d",
               curr_time.wYear, curr_time.wMonth, curr_time.wDay,
               curr_time.wHour, curr_time.wMinute, curr_time.wSecond);
    if (!XDG_WriteDesktopStringEntry(writer, "DeletionDate", 0, datebuf))
        goto error;
    close(writer);
    SHFree(path);
    return TRUE;

error:
    if (writer != -1)
    {
        close(writer);
        unlink(path);
    }
    SHFree(path);
    return FALSE;
}

/*
 * Try to create a .trashinfo file. This function will make several attempts with
 * different filenames. It will return the filename that succeeded or NULL if a file
 * couldn't be created.
 */
static char *create_trashinfo(const char *info_dir, const char *file_path)
{
    const char *base_name;
    char *filename_buffer;
    ULONG seed = GetTickCount();
    int i;

    errno = ENOMEM;       /* out-of-memory is the only case when errno isn't set */
    base_name = strrchr(file_path, '/');
    if (base_name == NULL)
        base_name = file_path;
    else
        base_name++;

    filename_buffer = SHAlloc(strlen(base_name)+9+1);
    if (filename_buffer == NULL)
        return NULL;
    lstrcpyA(filename_buffer, base_name);
    if (try_create_trashinfo_file(info_dir, filename_buffer, file_path))
        return filename_buffer;
    for (i=0; i<30; i++)
    {
        sprintf(filename_buffer, "%s-%d", base_name, i+1);
        if (try_create_trashinfo_file(info_dir, filename_buffer, file_path))
            return filename_buffer;
    }
    
    for (i=0; i<1000; i++)
    {
        sprintf(filename_buffer, "%s-%08x", base_name, RtlRandom(&seed));
        if (try_create_trashinfo_file(info_dir, filename_buffer, file_path))
            return filename_buffer;
    }
    
    WARN("Couldn't create trashinfo after 1031 tries (errno=%d)\n", errno);
    SHFree(filename_buffer);
    return NULL;
}

static void remove_trashinfo_file(const char *info_dir, const char *base_name)
{
    char *filename_buffer;
    
    filename_buffer = SHAlloc(lstrlenA(info_dir)+lstrlenA(base_name)+lstrlenA(trashinfo_suffix)+1);
    if (filename_buffer == NULL) return;
    sprintf(filename_buffer, "%s%s%s", info_dir, base_name, trashinfo_suffix);
    unlink(filename_buffer);
    SHFree(filename_buffer);
}

static BOOL TRASH_MoveFileToBucket(TRASH_BUCKET *pBucket, const char *unix_path)
{
    struct stat file_stat;
    char *trash_file_name = NULL;
    char *trash_path = NULL;
    BOOL ret = TRUE;

    if (lstat(unix_path, &file_stat)==-1)
        return FALSE;
    if (!file_good_for_bucket(pBucket, &file_stat))
        return FALSE;
        
    trash_file_name = create_trashinfo(pBucket->info_dir, unix_path);
    if (trash_file_name == NULL)
        return FALSE;
        
    trash_path = SHAlloc(strlen(pBucket->files_dir)+strlen(trash_file_name)+1);
    if (trash_path == NULL) goto error;
    lstrcpyA(trash_path, pBucket->files_dir);
    lstrcatA(trash_path, trash_file_name);
    
    if (rename(unix_path, trash_path)==0)
    {
        TRACE("rename succeeded\n");
        goto cleanup;
    }
    
    /* TODO: try to manually move the file */
    ERR("Couldn't move file\n");
error:
    ret = FALSE;
    remove_trashinfo_file(pBucket->info_dir, trash_file_name);
cleanup:
    SHFree(trash_file_name);
    SHFree(trash_path);
    return ret;
}

BOOL TRASH_TrashFile(LPCWSTR wszPath)
{
    char *unix_path;
    BOOL result;
    
    TRACE("(%s)\n", debugstr_w(wszPath));
    if (!TRASH_EnsureInitialized()) return FALSE;
    if (!(unix_path = wine_get_unix_file_name(wszPath)))
        return FALSE;
    result = TRASH_MoveFileToBucket(home_trash, unix_path);
    HeapFree(GetProcessHeap(), 0, unix_path);
    return result;
}

/*
 * The item ID of a trashed element is built as follows:
 *  NUL byte                    - in most PIDLs the first byte is the type so we keep it constant
 *  WIN32_FIND_DATAW structure  - with data about original file attributes
 *  bucket name                 - currently only an empty string meaning the home bucket is supported
 *  trash file name             - a NUL-terminated string
 */
static HRESULT TRASH_CreateSimplePIDL(LPCSTR filename, const WIN32_FIND_DATAW *data, LPITEMIDLIST *pidlOut)
{
    LPITEMIDLIST pidl = SHAlloc(2+1+sizeof(WIN32_FIND_DATAW)+1+lstrlenA(filename)+1+2);
    *pidlOut = NULL;
    if (pidl == NULL)
        return E_OUTOFMEMORY;
    pidl->mkid.cb = (USHORT)(2+1+sizeof(WIN32_FIND_DATAW)+1+lstrlenA(filename)+1);
    pidl->mkid.abID[0] = 0;
    memcpy(pidl->mkid.abID+1, data, sizeof(WIN32_FIND_DATAW));
    pidl->mkid.abID[1+sizeof(WIN32_FIND_DATAW)] = 0;
    lstrcpyA((LPSTR)(pidl->mkid.abID+1+sizeof(WIN32_FIND_DATAW)+1), filename);
    *(USHORT *)(pidl->mkid.abID+1+sizeof(WIN32_FIND_DATAW)+1+lstrlenA(filename)+1) = 0;
    *pidlOut = pidl;
    return S_OK;
}

/***********************************************************************
 *      TRASH_UnpackItemID [Internal]
 *
 * DESCRIPTION:
 * Extract the information stored in an Item ID. The WIN32_FIND_DATA contains
 * the information about the original file. The data->ftLastAccessTime contains
 * the deletion time
 *
 * PARAMETER(S):
 * [I] id : the ID of the item
 * [O] data : the WIN32_FIND_DATA of the original file. Can be NULL is not needed
 */                 
HRESULT TRASH_UnpackItemID(LPCSHITEMID id, WIN32_FIND_DATAW *data)
{
    if (id->cb < 2+1+sizeof(WIN32_FIND_DATAW)+2)
        return E_INVALIDARG;
    if (id->abID[0] != 0 || id->abID[1+sizeof(WIN32_FIND_DATAW)] != 0)
        return E_INVALIDARG;
    if (memchr(id->abID+1+sizeof(WIN32_FIND_DATAW)+1, 0, id->cb-(2+1+sizeof(WIN32_FIND_DATAW)+1)) == NULL)
        return E_INVALIDARG;

    if (data != NULL)
        *data = *(const WIN32_FIND_DATAW *)(id->abID+1);
    return S_OK;
}

static HRESULT TRASH_GetDetails(const TRASH_BUCKET *bucket, LPCSTR filename, WIN32_FIND_DATAW *data)
{
    LPSTR path = NULL;
    XDG_PARSED_FILE *parsed = NULL;
    char *original_file_name = NULL;
    char *deletion_date = NULL;
    int fd = -1;
    struct stat stats;
    HRESULT ret = S_FALSE;
    LPWSTR original_dos_name;
    int suffix_length = lstrlenA(trashinfo_suffix);
    int filename_length = lstrlenA(filename);
    int files_length = lstrlenA(bucket->files_dir);
    int path_length = max(lstrlenA(bucket->info_dir), files_length);
    
    path = SHAlloc(path_length + filename_length + 1);
    if (path == NULL) return E_OUTOFMEMORY;
    wsprintfA(path, "%s%s", bucket->files_dir, filename);
    path[path_length + filename_length - suffix_length] = 0;  /* remove the '.trashinfo' */    
    if (lstat(path, &stats) == -1)
    {
        ERR("Error accessing data file for trashinfo %s (errno=%d)\n", filename, errno);
        goto failed;
    }
    
    wsprintfA(path, "%s%s", bucket->info_dir, filename);
    fd = open(path, O_RDONLY);
    if (fd == -1)
    {
        ERR("Couldn't open trashinfo file %s (errno=%d)\n", path, errno);
        goto failed;
    }
    
    parsed = XDG_ParseDesktopFile(fd);
    if (parsed == NULL)
    {
        ERR("Parse error in trashinfo file %s\n", path);
        goto failed;
    }
    
    original_file_name = XDG_GetStringValue(parsed, trashinfo_group, "Path", XDG_URLENCODE);
    if (original_file_name == NULL)
    {
        ERR("No 'Path' entry in trashinfo file\n");
        goto failed;
    }
    
    ZeroMemory(data, sizeof(*data));
    data->nFileSizeHigh = (DWORD)((LONGLONG)stats.st_size>>32);
    data->nFileSizeLow = stats.st_size & 0xffffffff;
    RtlSecondsSince1970ToTime(stats.st_mtime, (LARGE_INTEGER *)&data->ftLastWriteTime);
    
    original_dos_name = wine_get_dos_file_name(original_file_name);
    if (original_dos_name != NULL)
    {
        lstrcpynW(data->cFileName, original_dos_name, MAX_PATH);
        SHFree(original_dos_name);
    }
    else
    {
        /* show only the file name */
        char *file = strrchr(original_file_name, '/');
        if (file == NULL)
            file = original_file_name;
        MultiByteToWideChar(CP_UNIXCP, 0, file, -1, data->cFileName, MAX_PATH);
    }
    
    deletion_date = XDG_GetStringValue(parsed, trashinfo_group, "DeletionDate", 0);
    if (deletion_date)
    {
        struct tm del_time;
        time_t del_secs;
        
        sscanf(deletion_date, "%d-%d-%dT%d:%d:%d",
            &del_time.tm_year, &del_time.tm_mon, &del_time.tm_mday,
            &del_time.tm_hour, &del_time.tm_min, &del_time.tm_sec);
        del_time.tm_year -= 1900;
        del_time.tm_mon--;
        del_secs = mktime(&del_time);
        
        RtlSecondsSince1970ToTime(del_secs, (LARGE_INTEGER *)&data->ftLastAccessTime);
    }
    
    ret = S_OK;
failed:
    SHFree(path);
    SHFree(original_file_name);
    SHFree(deletion_date);
    if (fd != -1)
        close(fd);
    XDG_FreeParsedFile(parsed);
    return ret;
}

static INT CALLBACK free_item_callback(void *item, void *lParam)
{
    SHFree(item);
    return TRUE;
}

static HDPA enum_bucket_trashinfos(const TRASH_BUCKET *bucket, int *count)
{
    HDPA ret = DPA_Create(32);
    struct dirent *entry;
    DIR *dir = NULL;
    
    errno = ENOMEM;
    *count = 0;
    if (ret == NULL) goto failed;
    dir = opendir(bucket->info_dir);
    if (dir == NULL) goto failed;
    while ((entry = readdir(dir)) != NULL)
    {
        LPSTR filename;
        int namelen = lstrlenA(entry->d_name);
        int suffixlen = lstrlenA(trashinfo_suffix);
        if (namelen <= suffixlen ||
                lstrcmpA(entry->d_name+namelen-suffixlen, trashinfo_suffix) != 0)
            continue;

        filename = StrDupA(entry->d_name);
        if (filename == NULL)
            goto failed;
        if (DPA_InsertPtr(ret, DPA_APPEND, filename) == -1)
        {
            LocalFree(filename);
            goto failed;
        }
        (*count)++;
    }
    closedir(dir);
    return ret;
failed:
    if (dir) closedir(dir);
    if (ret)
        DPA_DestroyCallback(ret, free_item_callback, NULL);
    return NULL;
}

HRESULT TRASH_EnumItems(LPITEMIDLIST **pidls, int *count)
{
    int ti_count;
    int pos=0, i;
    HRESULT err = E_OUTOFMEMORY;
    HDPA tinfs;
    
    if (!TRASH_EnsureInitialized()) return E_FAIL;
    tinfs = enum_bucket_trashinfos(home_trash, &ti_count);
    if (tinfs == NULL) return E_FAIL;
    *pidls = SHAlloc(sizeof(LPITEMIDLIST)*ti_count);
    if (!*pidls) goto failed;
    for (i=0; i<ti_count; i++)
    {
        WIN32_FIND_DATAW data;
        LPCSTR filename;
        
        filename = DPA_GetPtr(tinfs, i);
        if (FAILED(err = TRASH_GetDetails(home_trash, filename, &data)))
            goto failed;
        if (err == S_FALSE)
            continue;
        if (FAILED(err = TRASH_CreateSimplePIDL(filename, &data, &(*pidls)[pos])))
            goto failed;
        pos++;
    }
    *count = pos;
    DPA_DestroyCallback(tinfs, free_item_callback, NULL);
    return S_OK;
failed:
    if (*pidls != NULL)
    {
        int j;
        for (j=0; j<pos; j++)
            SHFree((*pidls)[j]);
        SHFree(*pidls);
    }
    DPA_DestroyCallback(tinfs, free_item_callback, NULL);
    
    return err;
}

HRESULT TRASH_RestoreItem(LPCITEMIDLIST pidl){
    int suffix_length = strlen(trashinfo_suffix);
    LPCSHITEMID id = &(pidl->mkid);
    const char *bucket_name = (const char*)(id->abID+1+sizeof(WIN32_FIND_DATAW));
    const char *filename = (const char*)(id->abID+1+sizeof(WIN32_FIND_DATAW)+strlen(bucket_name)+1);
    char *restore_path;
    WIN32_FIND_DATAW data;
    char *file_path;

    TRACE("(%p)\n",pidl);
    if(strcmp(filename+strlen(filename)-suffix_length,trashinfo_suffix))
    {
        ERR("pidl at %p is not a valid recycle bin entry\n",pidl);
        return E_INVALIDARG;
    }
    TRASH_UnpackItemID(id,&data);
    restore_path = wine_get_unix_file_name(data.cFileName);
    file_path = SHAlloc(max(strlen(home_trash->files_dir),strlen(home_trash->info_dir))+strlen(filename)+1);
    sprintf(file_path,"%s%s",home_trash->files_dir,filename);
    file_path[strlen(home_trash->files_dir)+strlen(filename)-suffix_length] = '\0';
    if(!rename(file_path,restore_path))
    {
            sprintf(file_path,"%s%s",home_trash->info_dir,filename);
            if(unlink(file_path))
                WARN("failed to delete the trashinfo file %s\n",filename);
    }
    else
        WARN("could not erase %s from the trash (errno=%i)\n",filename,errno);
    SHFree(file_path);
    HeapFree(GetProcessHeap(), 0, restore_path);
    return S_OK;
}

HRESULT TRASH_EraseItem(LPCITEMIDLIST pidl)
{
    int suffix_length = strlen(trashinfo_suffix);

    LPCSHITEMID id = &(pidl->mkid);
    const char *bucket_name = (const char*)(id->abID+1+sizeof(WIN32_FIND_DATAW));
    const char *filename = (const char*)(id->abID+1+sizeof(WIN32_FIND_DATAW)+strlen(bucket_name)+1);
    char *file_path;

    TRACE("(%p)\n",pidl);
    if(strcmp(filename+strlen(filename)-suffix_length,trashinfo_suffix))
    {
        ERR("pidl at %p is not a valid recycle bin entry\n",pidl);
        return E_INVALIDARG;
    }
    file_path = SHAlloc(max(strlen(home_trash->files_dir),strlen(home_trash->info_dir))+strlen(filename)+1);
    sprintf(file_path,"%s%s",home_trash->info_dir,filename);
    if(unlink(file_path))
        WARN("failed to delete the trashinfo file %s\n",filename);
    sprintf(file_path,"%s%s",home_trash->files_dir,filename);
    file_path[strlen(home_trash->files_dir)+strlen(filename)-suffix_length] = '\0';
    if(unlink(file_path))
        WARN("could not erase %s from the trash (errno=%i)\n",filename,errno);
    SHFree(file_path);
    return S_OK;
}

#endif /* HAVE_CORESERVICES_CORESERVICES_H */
