/*
 * The freedesktop.org Trash, implemented using the 0.7 spec version
 * (see http://www.ramendik.ru/docs/trashspec.html)
 *
 * Copyright (C) 2006 Mikolaj Zalewski
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

#include <stdarg.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "shlwapi.h"

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include "wine/debug.h"
#include "shell32_main.h"
#include "xdg.h"

WINE_DEFAULT_DEBUG_CHANNEL(trash);

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

static BOOL file_good_for_bucket(TRASH_BUCKET *pBucket, struct stat *file_stat)
{
    if (pBucket->device != file_stat->st_dev)
        return FALSE;
    return S_ISREG(file_stat->st_mode);
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
 * Try to create a single .trashinfo file. Return TRUE if successfull, else FALSE
 */
static BOOL try_create_trashinfo_file(const char *info_dir, const char *file_name,
    const char *original_file_name)
{
    struct tm curr_time;
    time_t curr_time_secs;
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
    
    time(&curr_time_secs);
    localtime_r(&curr_time_secs, &curr_time);
    wnsprintfA(datebuf, 200, "%04d-%02d-%02dT%02d:%02d:%02d",
        curr_time.tm_year+1900,
        curr_time.tm_mon+1,
        curr_time.tm_mday,
        curr_time.tm_hour,
        curr_time.tm_min,
        curr_time.tm_sec);
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
 * different filenames. It will return the filename that succeded or NULL if a file
 * couldn't be created.
 */
static char *create_trashinfo(const char *info_dir, const char *file_path)
{
    const char *base_name;
    char *filename_buffer;
    unsigned int seed = (unsigned int)time(NULL);
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
        sprintf(filename_buffer, "%s-%08x", base_name, rand_r(&seed));
        if (try_create_trashinfo_file(info_dir, filename_buffer, file_path))
            return filename_buffer;
    }
    
    WARN("Couldn't create trashinfo after 1031 tries (errno=%d)\n", errno);
    SHFree(filename_buffer);
    return NULL;
}

void remove_trashinfo_file(const char *info_dir, const char *base_name)
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
        TRACE("rename succeded\n");
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
