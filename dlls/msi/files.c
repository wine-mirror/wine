/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2005 Aric Stewart for CodeWeavers
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
 * Actions dealing with files These are
 *
 * InstallFiles
 * DuplicateFiles
 * MoveFiles (TODO)
 * PatchFiles (TODO)
 * RemoveDuplicateFiles(TODO)
 * RemoveFiles(TODO)
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/debug.h"
#include "fdi.h"
#include "msidefs.h"
#include "msvcrt/fcntl.h"
#include "msipriv.h"
#include "winuser.h"
#include "wine/unicode.h"
#include "action.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

extern const WCHAR szInstallFiles[];
extern const WCHAR szDuplicateFiles[];
extern const WCHAR szMoveFiles[];
extern const WCHAR szPatchFiles[];
extern const WCHAR szRemoveDuplicateFiles[];
extern const WCHAR szRemoveFiles[];

static const WCHAR cszTempFolder[]= {'T','e','m','p','F','o','l','d','e','r',0};

inline static UINT create_component_directory ( MSIPACKAGE* package, INT component)
{
    UINT rc = ERROR_SUCCESS;
    MSIFOLDER *folder;
    LPWSTR install_path;

    install_path = resolve_folder(package, package->components[component].Directory,
                        FALSE, FALSE, &folder);
    if (!install_path)
        return ERROR_FUNCTION_FAILED; 

    /* create the path */
    if (folder->State == 0)
    {
        create_full_pathW(install_path);
        folder->State = 2;
    }
    HeapFree(GetProcessHeap(), 0, install_path);

    return rc;
}

/*
 * This is a helper function for handling embedded cabinet media
 */
static UINT writeout_cabinet_stream(MSIPACKAGE *package, LPCWSTR stream_name,
                                    WCHAR* source)
{
    UINT rc;
    USHORT* data;
    UINT    size;
    DWORD   write;
    HANDLE  the_file;
    WCHAR tmp[MAX_PATH];

    rc = read_raw_stream_data(package->db,stream_name,&data,&size); 
    if (rc != ERROR_SUCCESS)
        return rc;

    write = MAX_PATH;
    if (MSI_GetPropertyW(package, cszTempFolder, tmp, &write))
        GetTempPathW(MAX_PATH,tmp);

    GetTempFileNameW(tmp,stream_name,0,source);

    track_tempfile(package,strrchrW(source,'\\'), source);
    the_file = CreateFileW(source, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, NULL);

    if (the_file == INVALID_HANDLE_VALUE)
    {
        ERR("Unable to create file %s\n",debugstr_w(source));
        rc = ERROR_FUNCTION_FAILED;
        goto end;
    }

    WriteFile(the_file,data,size,&write,NULL);
    CloseHandle(the_file);
    TRACE("wrote %li bytes to %s\n",write,debugstr_w(source));
end:
    HeapFree(GetProcessHeap(),0,data);
    return rc;
}


/* Support functions for FDI functions */
typedef struct
{
    MSIPACKAGE* package;
    LPCSTR cab_path;
    LPCSTR file_name;
} CabData;

static void * cabinet_alloc(ULONG cb)
{
    return HeapAlloc(GetProcessHeap(), 0, cb);
}

static void cabinet_free(void *pv)
{
    HeapFree(GetProcessHeap(), 0, pv);
}

static INT_PTR cabinet_open(char *pszFile, int oflag, int pmode)
{
    DWORD dwAccess = 0;
    DWORD dwShareMode = 0;
    DWORD dwCreateDisposition = OPEN_EXISTING;
    switch (oflag & _O_ACCMODE)
    {
    case _O_RDONLY:
        dwAccess = GENERIC_READ;
        dwShareMode = FILE_SHARE_READ | FILE_SHARE_DELETE;
        break;
    case _O_WRONLY:
        dwAccess = GENERIC_WRITE;
        dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
        break;
    case _O_RDWR:
        dwAccess = GENERIC_READ | GENERIC_WRITE;
        dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
        break;
    }
    if ((oflag & (_O_CREAT | _O_EXCL)) == (_O_CREAT | _O_EXCL))
        dwCreateDisposition = CREATE_NEW;
    else if (oflag & _O_CREAT)
        dwCreateDisposition = CREATE_ALWAYS;
    return (INT_PTR)CreateFileA(pszFile, dwAccess, dwShareMode, NULL, 
                                dwCreateDisposition, 0, NULL);
}

static UINT cabinet_read(INT_PTR hf, void *pv, UINT cb)
{
    DWORD dwRead;
    if (ReadFile((HANDLE)hf, pv, cb, &dwRead, NULL))
        return dwRead;
    return 0;
}

static UINT cabinet_write(INT_PTR hf, void *pv, UINT cb)
{
    DWORD dwWritten;
    if (WriteFile((HANDLE)hf, pv, cb, &dwWritten, NULL))
        return dwWritten;
    return 0;
}

static int cabinet_close(INT_PTR hf)
{
    return CloseHandle((HANDLE)hf) ? 0 : -1;
}

static long cabinet_seek(INT_PTR hf, long dist, int seektype)
{
    /* flags are compatible and so are passed straight through */
    return SetFilePointer((HANDLE)hf, dist, NULL, seektype);
}

static INT_PTR cabinet_notify(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin)
{
    /* FIXME: try to do more processing in this function */
    switch (fdint)
    {
    case fdintCOPY_FILE:
    {
        CabData *data = (CabData*) pfdin->pv;
        ULONG len = strlen(data->cab_path) + strlen(pfdin->psz1);
        char *file;

        LPWSTR trackname;
        LPWSTR trackpath;
        LPWSTR tracknametmp;
        static const WCHAR tmpprefix[] = {'C','A','B','T','M','P','_',0};
       
        if (data->file_name && lstrcmpiA(data->file_name,pfdin->psz1))
                return 0;
        
        file = cabinet_alloc((len+1)*sizeof(char));
        strcpy(file, data->cab_path);
        strcat(file, pfdin->psz1);

        TRACE("file: %s\n", debugstr_a(file));

        /* track this file so it can be deleted if not installed */
        trackpath=strdupAtoW(file);
        tracknametmp=strdupAtoW(strrchr(file,'\\')+1);
        trackname = HeapAlloc(GetProcessHeap(),0,(strlenW(tracknametmp) + 
                                  strlenW(tmpprefix)+1) * sizeof(WCHAR));

        strcpyW(trackname,tmpprefix);
        strcatW(trackname,tracknametmp);

        track_tempfile(data->package, trackname, trackpath);

        HeapFree(GetProcessHeap(),0,trackpath);
        HeapFree(GetProcessHeap(),0,trackname);
        HeapFree(GetProcessHeap(),0,tracknametmp);

        return cabinet_open(file, _O_WRONLY | _O_CREAT, 0);
    }
    case fdintCLOSE_FILE_INFO:
    {
        FILETIME ft;
	    FILETIME ftLocal;
        if (!DosDateTimeToFileTime(pfdin->date, pfdin->time, &ft))
            return -1;
        if (!LocalFileTimeToFileTime(&ft, &ftLocal))
            return -1;
        if (!SetFileTime((HANDLE)pfdin->hf, &ftLocal, 0, &ftLocal))
            return -1;

        cabinet_close(pfdin->hf);
        return 1;
    }
    default:
        return 0;
    }
}

/***********************************************************************
 *            extract_cabinet_file
 *
 * Extract files from a cab file.
 */
static BOOL extract_a_cabinet_file(MSIPACKAGE* package, const WCHAR* source, 
                                 const WCHAR* path, const WCHAR* file)
{
    HFDI hfdi;
    ERF erf;
    BOOL ret;
    char *cabinet;
    char *cab_path;
    char *file_name;
    CabData data;

    TRACE("Extracting %s (%s) to %s\n",debugstr_w(source), 
                    debugstr_w(file), debugstr_w(path));

    hfdi = FDICreate(cabinet_alloc,
                     cabinet_free,
                     cabinet_open,
                     cabinet_read,
                     cabinet_write,
                     cabinet_close,
                     cabinet_seek,
                     0,
                     &erf);
    if (!hfdi)
    {
        ERR("FDICreate failed\n");
        return FALSE;
    }

    if (!(cabinet = strdupWtoA( source )))
    {
        FDIDestroy(hfdi);
        return FALSE;
    }
    if (!(cab_path = strdupWtoA( path )))
    {
        FDIDestroy(hfdi);
        HeapFree(GetProcessHeap(), 0, cabinet);
        return FALSE;
    }

    data.package = package;
    data.cab_path = cab_path;
    if (file)
        file_name = strdupWtoA(file);
    else
        file_name = NULL;
    data.file_name = file_name;

    ret = FDICopy(hfdi, cabinet, "", 0, cabinet_notify, NULL, &data);

    if (!ret)
        ERR("FDICopy failed\n");

    FDIDestroy(hfdi);

    HeapFree(GetProcessHeap(), 0, cabinet);
    HeapFree(GetProcessHeap(), 0, cab_path);
    HeapFree(GetProcessHeap(), 0, file_name);

    return ret;
}

static UINT ready_media_for_file(MSIPACKAGE *package, WCHAR* path, 
                                 MSIFILE* file)
{
    UINT rc = ERROR_SUCCESS;
    MSIRECORD * row = 0;
    static WCHAR source[MAX_PATH];
    static const WCHAR ExecSeqQuery[] =
        {'S','E','L','E','C','T',' ','*',' ', 'F','R','O','M',' ',
         '`','M','e','d','i','a','`',' ','W','H','E','R','E',' ',
         '`','L','a','s','t','S','e','q','u','e','n','c','e','`',' ','>','=',
         ' ','%', 'i',' ','O','R','D','E','R',' ','B','Y',' ',
         '`','L','a','s','t','S','e','q','u','e','n','c','e','`',0};
    LPCWSTR cab;
    DWORD sz;
    INT seq;
    static UINT last_sequence = 0; 

    if (file->Attributes & msidbFileAttributesNoncompressed)
    {
        TRACE("Uncompressed File, no media to ready.\n");
        return ERROR_SUCCESS;
    }

    if (file->Sequence <= last_sequence)
    {
        TRACE("Media already ready (%u, %u)\n",file->Sequence,last_sequence);
        /*extract_a_cabinet_file(package, source,path,file->File); */
        return ERROR_SUCCESS;
    }

    row = MSI_QueryGetRecord(package->db, ExecSeqQuery, file->Sequence);
    if (!row)
        return ERROR_FUNCTION_FAILED;

    seq = MSI_RecordGetInteger(row,2);
    last_sequence = seq;

    cab = MSI_RecordGetString(row,4);
    if (cab)
    {
        TRACE("Source is CAB %s\n",debugstr_w(cab));
        /* the stream does not contain the # character */
        if (cab[0]=='#')
        {
            writeout_cabinet_stream(package,&cab[1],source);
            strcpyW(path,source);
            *(strrchrW(path,'\\')+1)=0;
        }
        else
        {
            sz = MAX_PATH;
            if (MSI_GetPropertyW(package, cszSourceDir, source, &sz))
            {
                ERR("No Source dir defined \n");
                rc = ERROR_FUNCTION_FAILED;
            }
            else
            {
                strcpyW(path,source);
                strcatW(source,cab);
                /* extract the cab file into a folder in the temp folder */
                sz = MAX_PATH;
                if (MSI_GetPropertyW(package, cszTempFolder,path, &sz) 
                                    != ERROR_SUCCESS)
                    GetTempPathW(MAX_PATH,path);
            }
        }
        rc = !extract_a_cabinet_file(package, source,path,NULL);
    }
    else
    {
        sz = MAX_PATH;
        MSI_GetPropertyW(package,cszSourceDir,source,&sz);
        strcpyW(path,source);
    }
    msiobj_release(&row->hdr);
    return rc;
}

inline static UINT get_file_target(MSIPACKAGE *package, LPCWSTR file_key, 
                                   LPWSTR* file_source)
{
    DWORD index;

    if (!package)
        return ERROR_INVALID_HANDLE;

    for (index = 0; index < package->loaded_files; index ++)
    {
        if (strcmpW(file_key,package->files[index].File)==0)
        {
            if (package->files[index].State >= 2)
            {
                *file_source = strdupW(package->files[index].TargetPath);
                return ERROR_SUCCESS;
            }
            else
                return ERROR_FILE_NOT_FOUND;
        }
    }

    return ERROR_FUNCTION_FAILED;
}

UINT ACTION_InstallFiles(MSIPACKAGE *package)
{
    UINT rc = ERROR_SUCCESS;
    DWORD index;
    MSIRECORD * uirow;
    WCHAR uipath[MAX_PATH];

    if (!package)
        return ERROR_INVALID_HANDLE;

    /* increment progress bar each time action data is sent */
    ui_progress(package,1,1,0,0);

    for (index = 0; index < package->loaded_files; index++)
    {
        WCHAR path_to_source[MAX_PATH];
        MSIFILE *file;
        
        file = &package->files[index];

        if (file->Temporary)
            continue;


        if (!ACTION_VerifyComponentForAction(package, file->ComponentIndex, 
                                       INSTALLSTATE_LOCAL))
        {
            ui_progress(package,2,file->FileSize,0,0);
            TRACE("File %s is not scheduled for install\n",
                   debugstr_w(file->File));

            continue;
        }

        if ((file->State == 1) || (file->State == 2))
        {
            LPWSTR p;
            MSICOMPONENT* comp = NULL;

            TRACE("Installing %s\n",debugstr_w(file->File));
            rc = ready_media_for_file(package, path_to_source, file);
            /* 
             * WARNING!
             * our file table could change here because a new temp file
             * may have been created
             */
            file = &package->files[index];
            if (rc != ERROR_SUCCESS)
            {
                ERR("Unable to ready media\n");
                rc = ERROR_FUNCTION_FAILED;
                break;
            }

            create_component_directory( package, file->ComponentIndex);

            /* recalculate file paths because things may have changed */

            if (file->ComponentIndex >= 0)
                comp = &package->components[file->ComponentIndex];

            p = resolve_folder(package, comp->Directory, FALSE, FALSE, NULL);
            HeapFree(GetProcessHeap(),0,file->TargetPath);

            file->TargetPath = build_directory_name(2, p, file->FileName);
            HeapFree(GetProcessHeap(),0,p);

            if (file->Attributes & msidbFileAttributesNoncompressed)
            {
                p = resolve_folder(package, comp->Directory, TRUE, FALSE, NULL);
                file->SourcePath = build_directory_name(2, p, file->ShortName);
                HeapFree(GetProcessHeap(),0,p);
            }
            else
                file->SourcePath = build_directory_name(2, path_to_source, 
                            file->File);


            TRACE("file paths %s to %s\n",debugstr_w(file->SourcePath),
                  debugstr_w(file->TargetPath));

            /* the UI chunk */
            uirow=MSI_CreateRecord(9);
            MSI_RecordSetStringW(uirow,1,file->File);
            strcpyW(uipath,file->TargetPath);
            *(strrchrW(uipath,'\\')+1)=0;
            MSI_RecordSetStringW(uirow,9,uipath);
            MSI_RecordSetInteger(uirow,6,file->FileSize);
            ui_actiondata(package,szInstallFiles,uirow);
            msiobj_release( &uirow->hdr );
            ui_progress(package,2,file->FileSize,0,0);

            
            if (file->Attributes & msidbFileAttributesNoncompressed)
                rc = CopyFileW(file->SourcePath,file->TargetPath,FALSE);
            else
                rc = MoveFileW(file->SourcePath, file->TargetPath);

            if (!rc)
            {
                rc = GetLastError();
                ERR("Unable to move/copy file (%s -> %s) (error %d)\n",
                     debugstr_w(file->SourcePath), debugstr_w(file->TargetPath),
                      rc);
                if (rc == ERROR_ALREADY_EXISTS && file->State == 2)
                {
                    if (!CopyFileW(file->SourcePath,file->TargetPath,FALSE))
                        ERR("Unable to copy file (%s -> %s) (error %ld)\n",
                            debugstr_w(file->SourcePath), 
                            debugstr_w(file->TargetPath), GetLastError());
                    if (!(file->Attributes & msidbFileAttributesNoncompressed))
                        DeleteFileW(file->SourcePath);
                    rc = 0;
                }
                else if (rc == ERROR_FILE_NOT_FOUND)
                {
                    ERR("Source File Not Found!  Continuing\n");
                    rc = 0;
                }
                else if (file->Attributes & msidbFileAttributesVital)
                {
                    ERR("Ignoring Error and continuing (nonvital file)...\n");
                    rc = 0;
                }
            }
            else
            {
                file->State = 4;
                rc = ERROR_SUCCESS;
            }
        }
    }

    return rc;
}

UINT ACTION_DuplicateFiles(MSIPACKAGE *package)
{
    UINT rc;
    MSIQUERY * view;
    MSIRECORD * row = 0;
    static const WCHAR ExecSeqQuery[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','D','u','p','l','i','c','a','t','e','F','i','l','e','`',0};

    if (!package)
        return ERROR_INVALID_HANDLE;

    rc = MSI_DatabaseOpenViewW(package->db, ExecSeqQuery, &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_ViewExecute(view, 0);
    if (rc != ERROR_SUCCESS)
    {
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
        return rc;
    }

    while (1)
    {
        WCHAR *file_source = NULL;
        WCHAR dest_name[0x100];
        LPWSTR dest_path, dest;
        LPCWSTR file_key, component;
        INT component_index;

        DWORD sz;

        rc = MSI_ViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            rc = ERROR_SUCCESS;
            break;
        }

        component = MSI_RecordGetString(row,2);
        component_index = get_loaded_component(package,component);

        if (!ACTION_VerifyComponentForAction(package, component_index, 
                                       INSTALLSTATE_LOCAL))
        {
            TRACE("Skipping copy due to disabled component %s\n",
                            debugstr_w(component));

            /* the action taken was the same as the current install state */        
            package->components[component_index].Action =
                package->components[component_index].Installed;

            msiobj_release(&row->hdr);
            continue;
        }

        package->components[component_index].Action = INSTALLSTATE_LOCAL;

        file_key = MSI_RecordGetString(row,3);
        if (!file_key)
        {
            ERR("Unable to get file key\n");
            msiobj_release(&row->hdr);
            break;
        }

        rc = get_file_target(package,file_key,&file_source);

        if (rc != ERROR_SUCCESS)
        {
            ERR("Original file unknown %s\n",debugstr_w(file_key));
            msiobj_release(&row->hdr);
            HeapFree(GetProcessHeap(),0,file_source);
            continue;
        }

        if (MSI_RecordIsNull(row,4))
        {
            strcpyW(dest_name,strrchrW(file_source,'\\')+1);
        }
        else
        {
            sz=0x100;
            MSI_RecordGetStringW(row,4,dest_name,&sz);
            reduce_to_longfilename(dest_name);
         }

        if (MSI_RecordIsNull(row,5))
        {
            LPWSTR p;
            dest_path = strdupW(file_source);
            p = strrchrW(dest_path,'\\');
            if (p)
                *p=0;
        }
        else
        {
            LPCWSTR destkey;
            destkey = MSI_RecordGetString(row,5);
            dest_path = resolve_folder(package, destkey, FALSE,FALSE,NULL);
            if (!dest_path)
            {
                ERR("Unable to get destination folder\n");
                msiobj_release(&row->hdr);
                HeapFree(GetProcessHeap(),0,file_source);
                break;
            }
        }

        dest = build_directory_name(2, dest_path, dest_name);
           
        TRACE("Duplicating file %s to %s\n",debugstr_w(file_source),
              debugstr_w(dest)); 
        
        if (strcmpW(file_source,dest))
            rc = !CopyFileW(file_source,dest,TRUE);
        else
            rc = ERROR_SUCCESS;
        
        if (rc != ERROR_SUCCESS)
            ERR("Failed to copy file %s -> %s, last error %ld\n", debugstr_w(file_source), debugstr_w(dest_path), GetLastError());

        FIXME("We should track these duplicate files as well\n");   
 
        msiobj_release(&row->hdr);
        HeapFree(GetProcessHeap(),0,dest_path);
        HeapFree(GetProcessHeap(),0,dest);
        HeapFree(GetProcessHeap(),0,file_source);
    }
    MSI_ViewClose(view);
    msiobj_release(&view->hdr);
    return rc;
}
