/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2004 Aric Stewart for CodeWeavers
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
 * Pages I need
 *
http://msdn.microsoft.com/library/default.asp?url=/library/en-us/msi/setup/installexecutesequence_table.asp

http://msdn.microsoft.com/library/default.asp?url=/library/en-us/msi/setup/standard_actions_reference.asp

 */

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "wine/debug.h"
#include "msi.h"
#include "msiquery.h"
#include "objbase.h"
#include "objidl.h"
#include "msipriv.h"
#include "winnls.h"
#include "winuser.h"
#include "shlobj.h"
#include "wine/unicode.h"

#define CUSTOM_ACTION_TYPE_MASK 0x3F

WINE_DEFAULT_DEBUG_CHANNEL(msi);

/*
 * Prototypes
 */
static UINT ACTION_ProcessExecSequence(MSIHANDLE hPackage, BOOL UIran);
static UINT ACTION_ProcessUISequence(MSIHANDLE hPackage);

UINT ACTION_PerformAction(MSIHANDLE hPackage, const WCHAR *action);
static UINT ACTION_CostInitialize(MSIHANDLE hPackage);
static UINT ACTION_CreateFolders(MSIHANDLE hPackage);
static UINT ACTION_CostFinalize(MSIHANDLE hPackage);
static UINT ACTION_InstallFiles(MSIHANDLE hPackage);
static UINT ACTION_DuplicateFiles(MSIHANDLE hPackage);
static UINT ACTION_WriteRegistryValues(MSIHANDLE hPackage);
static UINT ACTION_CustomAction(MSIHANDLE hPackage,const WCHAR *action);

static UINT HANDLE_CustomType1(MSIHANDLE hPackage, const LPWSTR source, 
                                const LPWSTR target, const INT type);
static UINT HANDLE_CustomType2(MSIHANDLE hPackage, const LPWSTR source, 
                                const LPWSTR target, const INT type);

static DWORD deformat_string(MSIHANDLE hPackage, WCHAR* ptr,WCHAR** data);

/*
 * consts and values used
 */
static const WCHAR cszSourceDir[] = {'S','o','u','r','c','e','D','i','r',0};
static const WCHAR cszRootDrive[] = {'R','O','O','T','D','R','I','V','E',0};
static const WCHAR cszTargetDir[] = {'T','A','R','G','E','T','D','I','R',0};
static const WCHAR c_collen[] = {'C',':','\\',0};
 
static const WCHAR cszlsb[]={'[',0};
static const WCHAR cszrsb[]={']',0};
static const WCHAR cszbs[]={'\\',0};


/******************************************************** 
 * helper functions to get around current HACKS and such
 ********************************************************/
inline static char *strdupWtoA( const WCHAR *str )
{
    char *ret = NULL;
    if (str)
    {
        DWORD len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL
);
        if ((ret = HeapAlloc( GetProcessHeap(), 0, len )))
            WideCharToMultiByte( CP_ACP, 0, str, -1, ret, len, NULL, NULL );
    }
    return ret;
}

/****************************************************
 * TOP level entry points 
 *****************************************************/

UINT ACTION_DoTopLevelINSTALL(MSIHANDLE hPackage, LPCWSTR szPackagePath,
                              LPCWSTR szCommandLine)
{
    DWORD sz;
    CHAR buffer[10];
    UINT rc;

    if (szPackagePath)   
    {
        LPWSTR p;
        WCHAR check[MAX_PATH];
        WCHAR pth[MAX_PATH];
        DWORD size;
 
        strcpyW(pth,szPackagePath);
        p = strrchrW(pth,'\\');    
        if (p)
        {
            p++;
            *p=0;
        }

        size = MAX_PATH;
        if (MsiGetPropertyW(hPackage,cszSourceDir,check,&size) 
            != ERROR_SUCCESS )
            MsiSetPropertyW(hPackage, cszSourceDir, pth);
    }

    if (szCommandLine)
    {
        LPWSTR ptr,ptr2;
        ptr = (LPWSTR)szCommandLine;
        
        while (*ptr)
        {
            BOOL quote=FALSE;
            WCHAR prop[0x100];
            WCHAR val[0x100];

            ptr2 = strchrW(ptr,'=');
            if (ptr2)
            {
                DWORD len = 0;
                strncpyW(prop,ptr,ptr2-ptr);
                prop[ptr2-ptr]=0;
                ptr2++;
            
                ptr = ptr2; 
                while (*ptr && (!quote && *ptr!=' '))
                {
                    if (*ptr == '"')
                        quote = !quote;
                    ptr++;
                    len++;
                }
               
                if (*ptr2=='"')
                {
                    ptr2++;
                    len -= 2;
                }
                strncpyW(val,ptr2,len);
                val[len]=0;

                if (*ptr)
                    ptr++;
            }            
            MsiSetPropertyW(hPackage,prop,val);
        }
    }
  
    sz = 10; 
    if (MsiGetPropertyA(hPackage,"UILevel",buffer,&sz) == ERROR_SUCCESS)
    {
        if (atoi(buffer) >= INSTALLUILEVEL_REDUCED)
        {
            rc = ACTION_ProcessUISequence(hPackage);
            if (rc == ERROR_SUCCESS)
                rc = ACTION_ProcessExecSequence(hPackage,TRUE);
        }
        else
            rc = ACTION_ProcessExecSequence(hPackage,FALSE);
    }
    else
        rc = ACTION_ProcessExecSequence(hPackage,FALSE);

    return rc;
}


static UINT ACTION_ProcessExecSequence(MSIHANDLE hPackage, BOOL UIran)
{
    MSIHANDLE view;
    UINT rc;
    static const CHAR *ExecSeqQuery = 
"select * from InstallExecuteSequence where Sequence > %i order by Sequence";
    CHAR Query[1024];
    MSIHANDLE db;
    MSIHANDLE row = 0;

    db = MsiGetActiveDatabase(hPackage);

    if (UIran)
    {
        INT seq = 0;
        static const CHAR *IVQuery = 
"select Sequence from InstallExecuteSequence where Action = `InstallValidate`" ;
        
        MsiDatabaseOpenViewA(db, IVQuery, &view);
        MsiViewExecute(view, 0);
        MsiViewFetch(view,&row);
        seq = MsiRecordGetInteger(row,1);
        MsiCloseHandle(row);
        MsiViewClose(view);
        MsiCloseHandle(view);
        sprintf(Query,ExecSeqQuery,0);
    }
    else
        sprintf(Query,ExecSeqQuery,0);

    rc = MsiDatabaseOpenViewA(db, Query, &view);
    MsiCloseHandle(db);
    
    if (rc == ERROR_SUCCESS)
    {
        rc = MsiViewExecute(view, 0);

        if (rc != ERROR_SUCCESS)
        {
            MsiViewClose(view);
            MsiCloseHandle(view);
            goto end;
        }
       
        TRACE("Running the actions \n"); 

        while (1)
        {
            WCHAR buffer[0x100];
            DWORD sz = 0x100;

            rc = MsiViewFetch(view,&row);
            if (rc != ERROR_SUCCESS)
            {
                rc = ERROR_SUCCESS;
                break;
            }

            /* check conditions */
            if (!MsiRecordIsNull(row,2))
            {
                sz=0x100;
                rc = MsiRecordGetStringW(row,2,buffer,&sz);
                if (rc != ERROR_SUCCESS)
                {
                    MsiCloseHandle(row);
                    break;
                }

                /* this is a hack to skip errors in the condition code */
                if (MsiEvaluateConditionW(hPackage, buffer) ==
                    MSICONDITION_FALSE)
                {
                    MsiCloseHandle(row);
                    continue; 
                }

            }

            sz=0x100;
            rc =  MsiRecordGetStringW(row,1,buffer,&sz);
            if (rc != ERROR_SUCCESS)
            {
                ERR("Error is %x\n",rc);
                MsiCloseHandle(row);
                break;
            }

            rc = ACTION_PerformAction(hPackage,buffer);

            if (rc != ERROR_SUCCESS)
            {
                ERR("Execution halted due to error (%i)\n",rc);
                MsiCloseHandle(row);
                break;
            }

            MsiCloseHandle(row);
        }

        MsiViewClose(view);
        MsiCloseHandle(view);
    }

end:
    return rc;
}


static UINT ACTION_ProcessUISequence(MSIHANDLE hPackage)
{
    MSIHANDLE view;
    UINT rc;
    static const CHAR *ExecSeqQuery = 
"select * from InstallUISequence where Sequence > 0 order by Sequence";
    MSIHANDLE db;
    
    db = MsiGetActiveDatabase(hPackage);
    rc = MsiDatabaseOpenViewA(db, ExecSeqQuery, &view);
    MsiCloseHandle(db);
    
    if (rc == ERROR_SUCCESS)
    {
        rc = MsiViewExecute(view, 0);

        if (rc != ERROR_SUCCESS)
        {
            MsiViewClose(view);
            MsiCloseHandle(view);
            goto end;
        }
       
        TRACE("Running the actions \n"); 

        while (1)
        {
            WCHAR buffer[0x100];
            DWORD sz = 0x100;
            MSIHANDLE row = 0;

            rc = MsiViewFetch(view,&row);
            if (rc != ERROR_SUCCESS)
            {
                rc = ERROR_SUCCESS;
                break;
            }

            /* check conditions */
            if (!MsiRecordIsNull(row,2))
            {
                sz=0x100;
                rc = MsiRecordGetStringW(row,2,buffer,&sz);
                if (rc != ERROR_SUCCESS)
                {
                    MsiCloseHandle(row);
                    break;
                }

                if (MsiEvaluateConditionW(hPackage, buffer) ==
                    MSICONDITION_FALSE)
                {
                    MsiCloseHandle(row);
                    continue; 
                }

            }

            sz=0x100;
            rc =  MsiRecordGetStringW(row,1,buffer,&sz);
            if (rc != ERROR_SUCCESS)
            {
                ERR("Error is %x\n",rc);
                MsiCloseHandle(row);
                break;
            }

            rc = ACTION_PerformAction(hPackage,buffer);

            if (rc != ERROR_SUCCESS)
            {
                ERR("Execution halted due to error (%i)\n",rc);
                MsiCloseHandle(row);
                break;
            }

            MsiCloseHandle(row);
        }

        MsiViewClose(view);
        MsiCloseHandle(view);
    }

end:
    return rc;
}

/********************************************************
 * ACTION helper functions and functions that perform the actions
 *******************************************************/

/* 
 * Alot of actions are really important even if they don't do anything
 * explicit.. Lots of properties are set at the beginning of the installation
 * CostFinalize does a bunch of work to translated the directories and such
 * 
 * But until I get write access to the database that is hard, so I am going to
 * hack it to see if I can get something to run.
 */
UINT ACTION_PerformAction(MSIHANDLE hPackage, const WCHAR *action)
{
    const static WCHAR szCreateFolders[] = 
        {'C','r','e','a','t','e','F','o','l','d','e','r','s',0};
    const static WCHAR szCostFinalize[] = 
        {'C','o','s','t','F','i','n','a','l','i','z','e',0};
    const static WCHAR szInstallFiles[] = 
        {'I','n','s','t','a','l','l','F','i','l','e','s',0};
    const static WCHAR szDuplicateFiles[] = 
        {'D','u','p','l','i','c','a','t','e','F','i','l','e','s',0};
    const static WCHAR szWriteRegistryValues[] = 
{'W','r','i','t','e','R','e','g','i','s','t','r','y','V','a','l','u','e','s',0};
    const static WCHAR szCostInitialize[] = 
        {'C','o','s','t','I','n','i','t','i','a','l','i','z','e',0};

    TRACE("Performing action (%s)\n",debugstr_w(action));

    if (strcmpW(action,szCostInitialize)==0)
        return ACTION_CostInitialize(hPackage);
    if (strcmpW(action,szCreateFolders)==0)
        return ACTION_CreateFolders(hPackage);
    if (strcmpW(action,szCostFinalize)==0)
        return ACTION_CostFinalize(hPackage);
    if (strcmpW(action,szInstallFiles)==0)
        return ACTION_InstallFiles(hPackage);
    if (strcmpW(action,szDuplicateFiles)==0)
        return ACTION_DuplicateFiles(hPackage);
    if (strcmpW(action,szWriteRegistryValues)==0)
        return ACTION_WriteRegistryValues(hPackage);
    /*
     Current called during itunes but unimplemented

     AppSearch
     LaunchConditions
     FindRelatedProducts
     CostInitialize
     MigrateFeatureStates
     ResolveSource  (sets SourceDir)
     FileCost
     ValidateProductID (sets ProductID)
     IsolateComponents (Empty)
     SetODBCFolders 
     MigrateFeatureStates
     InstallValidate 
     RemoveExistingProducts
     InstallInitialize
     AllocateRegistrySpace
     ProcessComponents
     UnpublishComponents
     UnpublishFeatures
     StopServices
     DeleteServices
     UnregisterComPlus
     SelfUnregModules (Empty)
     UnregisterTypeLibraries
     RemoveODBC
     UnregisterFonts
     RemoveRegistryValues
     UnregisterClassInfo
     UnregisterExtensionInfo
     UnregisterProgIdInfo
     UnregisterMIMEInfo
     RemoveIniValues
     RemoveShortcuts
     RemoveEnviromentStrings
     RemoveDuplicateFiles
     RemoveFiles (Empty)
     MoveFiles (Empty)
     RemoveRegistryValues (Empty)
     SelfRegModules (Empty)
     RemoveFolders
     PatchFiles
     BindImage (Empty)
     CreateShortcuts (would be nice to have soon)
     RegisterClassInfo
     RegisterExtensionInfo (Empty)
     RegisterProgIdInfo (Lots to do)
     RegisterMIMEInfo (Empty)
     WriteIniValues (Empty)
     WriteEnvironmentStrings (Empty)
     RegisterFonts(Empty)
     InstallODBC
     RegisterTypeLibraries
     SelfRegModules
     RegisterComPlus
     RegisterUser
     RegisterProduct
     PublishComponents
     PublishFeatures
     PublishProduct
     InstallFinalize
     .
     */
     if (ACTION_CustomAction(hPackage,action) != ERROR_SUCCESS)
        ERR("UNHANDLED MSI ACTION %s\n",debugstr_w(action));

    return ERROR_SUCCESS;
}


static UINT ACTION_CustomAction(MSIHANDLE hPackage,const WCHAR *action)
{
    UINT rc;
    MSIHANDLE view;
    MSIHANDLE row = 0;
    WCHAR ExecSeqQuery[1024] = 
    {'s','e','l','e','c','t',' ','*',' ','f','r','o','m',' ','C','u','s','t','o'
,'m','A','c','t','i','o','n',' ','w','h','e','r','e',' ','`','A','c','t','i'
,'o','n','`',' ','=',' ','`',0};
    static const WCHAR end[]={'`',0};
    UINT type;
    DWORD sz;
    WCHAR source[0x100];
    WCHAR target[0x200];
    WCHAR *deformated=NULL;
    MSIHANDLE db;

    strcatW(ExecSeqQuery,action);
    strcatW(ExecSeqQuery,end);

    db = MsiGetActiveDatabase(hPackage);
    rc = MsiDatabaseOpenViewW(db, ExecSeqQuery, &view);
    MsiCloseHandle(db);

    if (rc != ERROR_SUCCESS)
        return rc;

    rc = MsiViewExecute(view, 0);
    if (rc != ERROR_SUCCESS)
    {
        MsiViewClose(view);
        MsiCloseHandle(view);
        return rc;
    }

    rc = MsiViewFetch(view,&row);
    if (rc != ERROR_SUCCESS)
    {
        MsiViewClose(view);
        MsiCloseHandle(view);
        return rc;
    }

    type = MsiRecordGetInteger(row,2);

    sz=0x100;
    MsiRecordGetStringW(row,3,source,&sz);
    sz=0x200;
    MsiRecordGetStringW(row,4,target,&sz);

    TRACE("Handling custom action %s (%x %s %s)\n",debugstr_w(action),type,
          debugstr_w(source), debugstr_w(target));

    /* we are ignoring ALOT of flags and important synchronization stuff */
    switch (type & CUSTOM_ACTION_TYPE_MASK)
    {
        case 1: /* DLL file stored in a Binary table stream */
            rc = HANDLE_CustomType1(hPackage,source,target,type);
            break;
        case 2: /* Exe file stored in a Binary table strem */
            rc = HANDLE_CustomType2(hPackage,source,target,type);
            break;
        case 35: /* Directory set with formatted text. */
        case 51: /* Property set with formatted text. */
            deformat_string(hPackage,target,&deformated);
            MsiSetPropertyW(hPackage,source,deformated);
            HeapFree(GetProcessHeap(),0,deformated);
            break;
        default:
            ERR("UNHANDLED ACTION TYPE %i (%s %s)\n",
             type & CUSTOM_ACTION_TYPE_MASK, debugstr_w(source),
             debugstr_w(target));
    }

    MsiCloseHandle(row);
    MsiViewClose(view);
    MsiCloseHandle(view);
    return rc;
}

static UINT store_binary_to_temp(MSIHANDLE hPackage, const LPWSTR source, 
                                LPWSTR tmp_file)
{
    static const WCHAR TF[]= {'T','e','m','p','F','o','l','d','e','r',0};
    DWORD sz=MAX_PATH;

    if (MsiGetPropertyW(hPackage, TF,tmp_file, &sz) != ERROR_SUCCESS)
        GetTempPathW(MAX_PATH,tmp_file);

    strcatW(tmp_file,source);

    if (GetFileAttributesW(tmp_file) != INVALID_FILE_ATTRIBUTES)
    {
        TRACE("File already exists\n");
        return ERROR_SUCCESS;
    }
    else
    {
        /* write out the file */
        UINT rc;
        MSIHANDLE view;
        MSIHANDLE row = 0;
        WCHAR Query[1024] =
        {'s','e','l','e','c','t',' ','*',' ','f','r','o','m',' ','B','i'
,'n','a','r','y',' ','w','h','e','r','e',' ','N','a','m','e','=','`',0};
        static const WCHAR end[]={'`',0};
        HANDLE the_file;
        CHAR buffer[1024];
        MSIHANDLE db;

        the_file = CreateFileW(tmp_file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, NULL);
    
        if (the_file == INVALID_HANDLE_VALUE)
            return ERROR_FUNCTION_FAILED;

        strcatW(Query,source);
        strcatW(Query,end);

        db = MsiGetActiveDatabase(hPackage);
        rc = MsiDatabaseOpenViewW(db, Query, &view);
        MsiCloseHandle(db);

        if (rc != ERROR_SUCCESS)
            return rc;

        rc = MsiViewExecute(view, 0);
        if (rc != ERROR_SUCCESS)
        {
            MsiViewClose(view);
            MsiCloseHandle(view);
            return rc;
        }

        rc = MsiViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            MsiViewClose(view);
            MsiCloseHandle(view);
            return rc;
        }

        do 
        {
            DWORD write;
            sz = 1024;
            rc = MsiRecordReadStream(row,2,buffer,&sz);
            if (rc != ERROR_SUCCESS)
            {
                ERR("Failed to get stream\n");
                CloseHandle(the_file);  
                DeleteFileW(tmp_file);
                break;
            }
            WriteFile(the_file,buffer,sz,&write,NULL);
        } while (sz == 1024);

        CloseHandle(the_file);

        MsiCloseHandle(row);
        MsiViewClose(view);
        MsiCloseHandle(view);
    }

    return ERROR_SUCCESS;
}


typedef UINT CustomEntry(MSIHANDLE);

static UINT HANDLE_CustomType1(MSIHANDLE hPackage, const LPWSTR source, 
                                const LPWSTR target, const INT type)
{
    WCHAR tmp_file[MAX_PATH];
    CustomEntry *fn;
    HANDLE DLL;
    LPSTR proc;

    store_binary_to_temp(hPackage, source, tmp_file);

    TRACE("Calling function %s from %s\n",debugstr_w(target),
          debugstr_w(tmp_file));

    if (type & 0xc0)
    {
        ERR("Asynchronous execution.. UNHANDLED\n");
        return ERROR_SUCCESS;
    }

    if (!strchrW(tmp_file,'.'))
    {
        static const WCHAR dot[]={'.',0};
        strcatW(tmp_file,dot);
    } 
 
    DLL = LoadLibraryW(tmp_file);
    if (DLL)
    {
        proc = strdupWtoA( target );
        fn = (CustomEntry*)GetProcAddress(DLL,proc);
        if (fn)
        {
            TRACE("Calling function\n");
            fn(hPackage);
        }
        else
            ERR("Cannot load functon\n");

        HeapFree(GetProcessHeap(),0,proc);
        FreeLibrary(DLL);
    }
    else
        ERR("Unable to load library\n");

    return ERROR_SUCCESS;
}

static UINT HANDLE_CustomType2(MSIHANDLE hPackage, const LPWSTR source, 
                                const LPWSTR target, const INT type)
{
    WCHAR tmp_file[MAX_PATH*2];
    STARTUPINFOW si;
    PROCESS_INFORMATION info;
    BOOL rc;
    WCHAR *deformated;
    static const WCHAR spc[] = {' ',0};

    memset(&si,0,sizeof(STARTUPINFOW));
    memset(&info,0,sizeof(PROCESS_INFORMATION));

    store_binary_to_temp(hPackage, source, tmp_file);

    strcatW(tmp_file,spc);
    deformat_string(hPackage,target,&deformated);
    strcatW(tmp_file,deformated);

    HeapFree(GetProcessHeap(),0,deformated);

    TRACE("executing exe %s \n",debugstr_w(tmp_file));

    rc = CreateProcessW(NULL, tmp_file, NULL, NULL, FALSE, 0, NULL,
                  c_collen, &si, &info);

    if ( !rc )
    {
        ERR("Unable to execute command\n");
        return ERROR_SUCCESS;
    }

    if (!(type & 0xc0))
        WaitForSingleObject(info.hProcess,INFINITE);

    return ERROR_SUCCESS;
}

/***********************************************************************
 *            create_full_pathW
 *
 * Recursively create all directories in the path.
 *
 * shamelessly stolen from setupapi/queue.c
 */
static BOOL create_full_pathW(const WCHAR *path)
{
    BOOL ret = TRUE;
    int len;
    WCHAR *new_path;

    new_path = HeapAlloc(GetProcessHeap(), 0, (strlenW(path) + 1) *
sizeof(WCHAR));
    strcpyW(new_path, path);

    while((len = strlenW(new_path)) && new_path[len - 1] == '\\')
    new_path[len - 1] = 0;

    while(!CreateDirectoryW(new_path, NULL))
    {
    WCHAR *slash;
    DWORD last_error = GetLastError();
    if(last_error == ERROR_ALREADY_EXISTS)
        break;

    if(last_error != ERROR_PATH_NOT_FOUND)
    {
        ret = FALSE;
        break;
    }

    if(!(slash = strrchrW(new_path, '\\')))
    {
        ret = FALSE;
        break;
    }

    len = slash - new_path;
    new_path[len] = 0;
    if(!create_full_pathW(new_path))
    {
        ret = FALSE;
        break;
    }
    new_path[len] = '\\';
    }

    HeapFree(GetProcessHeap(), 0, new_path);
    return ret;
}

/*
 * Also we cannot enable/disable components either, so for now I am just going 
 * to do all the directories for all the components.
 */
static UINT ACTION_CreateFolders(MSIHANDLE hPackage)
{
    static const CHAR *ExecSeqQuery = "select Directory_ from CreateFolder";
    UINT rc;
    MSIHANDLE view;
    MSIHANDLE db;

    db = MsiGetActiveDatabase(hPackage);
    rc = MsiDatabaseOpenViewA(db, ExecSeqQuery, &view);
    MsiCloseHandle(db);

    if (rc != ERROR_SUCCESS)
        return rc;

    rc = MsiViewExecute(view, 0);
    if (rc != ERROR_SUCCESS)
    {
        MsiViewClose(view);
        MsiCloseHandle(view);
        return rc;
    }
    
    while (1)
    {
        WCHAR dir[0x100];
        WCHAR full_path[MAX_PATH];
        DWORD sz;
        MSIHANDLE row = 0;

        rc = MsiViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            rc = ERROR_SUCCESS;
            break;
        }

        sz=0x100;
        rc = MsiRecordGetStringW(row,1,dir,&sz);

        if (rc!= ERROR_SUCCESS)
        {
            ERR("Unable to get folder id \n");
            MsiCloseHandle(row);
            continue;
        }

        sz = MAX_PATH;
        rc = MsiGetPropertyW(hPackage, dir,full_path,&sz);

        if (rc != ERROR_SUCCESS)
        {
            ERR("Unable to resolve folder id %s\n",debugstr_w(dir));
            MsiCloseHandle(row);
            continue;
        }

        TRACE("Folder is %s\n",debugstr_w(full_path));
        create_full_pathW(full_path);

        MsiCloseHandle(row);
    }
    MsiViewClose(view);
    MsiCloseHandle(view);
   
    return rc;
}

/*
 * Workhorse function for creating the directories
 * during Costing
 */
static UINT resolve_directory(MSIHANDLE hPackage, const WCHAR* dir, 
                           WCHAR* path, BOOL source)
{
    static const WCHAR cszsrc[]={'_','S','o','u','r','c','e',0};
    static const WCHAR cszsrcroot[]=
        {'[','S','o','u','r','c','e','D','i','r',']',0};

    WCHAR Query[1024] = 
{'s','e','l','e','c','t',' ','*',' ','f','r','o','m',' ','D','i','r','e','c',
't','o','r','y',' ','w','h','e','r','e',' ','`','D','i','r','e','c','t',
'o','r','y','`',' ','=',' ','`',0};
    static const WCHAR end[]={'`',0};
    UINT rc;
    MSIHANDLE view;
    WCHAR targetbuffer[0x100];
    WCHAR *srcdir = NULL;
    WCHAR *targetdir = NULL;
    WCHAR buffer[0x100];
    WCHAR parent[0x100];
    WCHAR parent_path[MAX_PATH];
    DWORD sz=0x100;
    MSIHANDLE row = 0;
    WCHAR full_path[MAX_PATH];
    WCHAR name_source[0x100];
    MSIHANDLE db;

    sz = MAX_PATH; 
    if (MsiGetPropertyW(hPackage,dir,path,&sz)==ERROR_SUCCESS)
        return ERROR_SUCCESS;

    TRACE("Working to resolve %s\n",debugstr_w(dir));

    /* special case... root drive */       
    if (strcmpW(dir,cszTargetDir)==0)
    {
        if (!source)
        {
            sz = 0x100;
            if(!MsiGetPropertyW(hPackage,cszRootDrive,buffer,&sz))
            {
                MsiSetPropertyW(hPackage,cszTargetDir,buffer);
                strcpyW(path,buffer);
            }
            else
            {
                strcpyW(path,c_collen);
            }
        }
        else
            strcpyW(path,cszsrcroot);

        return ERROR_SUCCESS;
    }

    strcatW(Query,dir);
    strcatW(Query,end);

    db = MsiGetActiveDatabase(hPackage);
    rc = MsiDatabaseOpenViewW(db, Query, &view);
    MsiCloseHandle(db);

    if (rc != ERROR_SUCCESS)
        return rc;

    rc = MsiViewExecute(view, 0);
    if (rc != ERROR_SUCCESS)
    {
        MsiViewClose(view);
        MsiCloseHandle(view);
        return rc;
    }

    rc = MsiViewFetch(view,&row);
    if (rc != ERROR_SUCCESS)
    {
        MsiViewClose(view);
        MsiCloseHandle(view);
        return rc;
    }

    sz=0x100;
    MsiRecordGetStringW(row,3,targetbuffer,&sz);
    targetdir=targetbuffer;

    /* split src and target dir */
    if (strchrW(targetdir,':'))
    {
        srcdir=strchrW(targetdir,':');
        *srcdir=0;
        srcdir ++;
    }
    else
        srcdir=NULL;

    /* for now only pick long filename versions */
    if (strchrW(targetdir,'|'))
    {
        targetdir = strchrW(targetdir,'|'); 
        *targetdir = 0;
        targetdir ++;
    }
    if (srcdir && strchrW(srcdir,'|'))
    {
        srcdir= strchrW(srcdir,'|'); 
        *srcdir= 0;
        srcdir ++;
    }

    /* now check for root dirs */
    if (targetdir[0] == '.' && targetdir[1] == 0)
        targetdir = NULL;
        
    if (srcdir && srcdir[0] == '.' && srcdir[1] == 0)
        srcdir = NULL;

    if (MsiRecordIsNull(row,2))
        parent[0]=0;
    else
    {
            sz=0x100;
            MsiRecordGetStringW(row,2,parent,&sz);
    }

    if (parent[0]) 
    {
        resolve_directory(hPackage,parent,parent_path,FALSE);
        strcpyW(full_path,parent_path);
        if (targetdir)
        {
            strcatW(full_path,targetdir);
            strcatW(full_path,cszbs);
        }
        MsiSetPropertyW(hPackage,dir,full_path);
        if (!source)
            strcpyW(path,full_path);

        resolve_directory(hPackage,parent,parent_path,TRUE);
        strcpyW(full_path,parent_path);
        if (srcdir)
        {
            strcatW(full_path,srcdir);
            strcatW(full_path,cszbs); 
        }
        else if (targetdir)
        {
            strcatW(full_path,targetdir);
            strcatW(full_path,cszbs);
        }
        
        strcpyW(name_source,dir);
        strcatW(name_source,cszsrc);
        MsiSetPropertyW(hPackage,name_source,full_path);
        if (source)
            strcpyW(path,full_path);
    }

    MsiCloseHandle(row);
    MsiViewClose(view);
    MsiCloseHandle(view);
    return rc;
}

/*
 * I am not doing any of the costing functionality yet. 
 * Mostly looking at doing the Component and Feature loading
 *
 * The native MSI does ALOT of modification to tables here. Mostly adding alot
 * of temporary columns to the Feature and Component tables. 
 * Unfortunately I cannot add temporary columns yet, nor can I really figure
 * out what all the columns are for. So I am going to attack this another way
 * and make some temporary tables to hold the data I think I need.
 *
 * WINE_Feature
 *   Feature Identifier : key into the Feature table
 *   Enabled Int : 1 if being installed, 0 if not
 */
static UINT ACTION_CostInitialize(MSIHANDLE hPackage)
{
    MSIHANDLE db;
    MSIHANDLE view;
    MSIHANDLE row;
    CHAR local[0x100]; 
    WCHAR buffer[0x100];
    DWORD sz;

    static const CHAR CreateSql[] = "CREATE TABLE `WINE_Feature` ( `Feature`"
        "CHAR(56) NOT NULL, `Enabled` INT NOT NULL PRIMARY KEY `Feature`)";
    static const CHAR Insert[] =
      "INSERT into `WINE_Feature` (`Feature`, `Enabled`) VALUES (?)";
    static const CHAR Query_all[] = "SELECT * FROM Feature";
    static const CHAR Query_one[] = "SELECT * FROM Feature WHERE Feature='%s'";
    CHAR Query[1023];

    MsiSetPropertyA(hPackage,"CostingComplete","0");
    MsiSetPropertyW(hPackage, cszRootDrive , c_collen);

    db = MsiGetActiveDatabase(hPackage);
    MsiDatabaseOpenViewA(db,CreateSql,&view);
    MsiViewExecute(view,0); 
    MsiViewClose(view);
    MsiCloseHandle(view);

    sz = 0x100;
    if (MsiGetPropertyA(hPackage,"ADDLOCAL",local,&sz)==ERROR_SUCCESS)
    {
        if (strcasecmp(local,"ALL")==0)
        {
            MsiDatabaseOpenViewA(db,Query_all,&view);
            MsiViewExecute(view,0);
            while (1)
            {
                MSIHANDLE view2;
                MSIHANDLE row2;
                DWORD rc;

                rc = MsiViewFetch(view,&row);
                if (rc != ERROR_SUCCESS)
                    break;
        
                row2 = MsiCreateRecord(2);

                sz=0x100;
                MsiRecordGetStringW(row,1,buffer,&sz);                
                MsiRecordSetStringW(row2,1,buffer);
                MsiRecordSetInteger(row2,2,1);

                MsiDatabaseOpenViewA(db,Insert,&view2);
                MsiViewExecute(view2,row2);
                MsiCloseHandle(row2);
                MsiCloseHandle(row);
                MsiViewClose(view2);
                MsiCloseHandle(view2);
                TRACE("Enabling feature %s\n",debugstr_w(buffer));
            }
            MsiViewClose(view);
            MsiCloseHandle(view);
        }
        else
        {
            LPSTR ptr,ptr2;
            ptr = local;

            while (ptr && *ptr)
            {
                CHAR feature[0x100];
                MSIHANDLE view2;
                MSIHANDLE row2;
                DWORD rc;

                ptr2 = strchr(ptr,',');

                if (ptr2)
                {
                    strncpy(feature,ptr,ptr2-ptr);
                    feature[ptr2-ptr]=0;
                    ptr2++;
                    ptr = ptr2;
                }
                else
                {
                    strcpy(feature,ptr);
                    ptr = NULL;
                }

                sprintf(Query,Query_one,feature);

                MsiDatabaseOpenViewA(db,Query,&view);
                MsiViewExecute(view,0);
                rc = MsiViewFetch(view,&row);
                if (rc != ERROR_SUCCESS)
                    break;
        
                row2 = MsiCreateRecord(2);

                sz=0x100;
                MsiRecordGetStringW(row,1,buffer,&sz);                
                MsiRecordSetStringW(row2,1,buffer);
                MsiRecordSetInteger(row2,2,1);

                MsiDatabaseOpenViewA(db,Insert,&view2);
                MsiViewExecute(view,row2);
                MsiCloseHandle(row2);
                MsiCloseHandle(row);
                MsiViewClose(view2);
                MsiCloseHandle(view2);
                MsiViewClose(view);
                MsiCloseHandle(view);
                TRACE("Enabling feature %s\n",feature);
            }
        }
    }

    MsiCloseHandle(db);
    return ERROR_SUCCESS;
}

/* 
 * Alot is done in this function aside from just the costing.
 * The costing needs to be implemented at some point but for now I am going
 * to focus on the directory building
 *
 * WINE_Directory
 *    Directory Identifier: key into the Directory Table
 *    Source    Path : resolved source path without SourceDir
 *    Target    Path : resolved target path wihout  TARGETDIR
 *    Created   Int  : 0 uncreated, 1 created but if empty remove, 2 created
 *
 */
static UINT ACTION_CostFinalize(MSIHANDLE hPackage)
{
    static const CHAR *ExecSeqQuery = "select * from Directory";
    UINT rc;
    MSIHANDLE view;
    MSIHANDLE db;

    TRACE("Building Directory properties\n");

    db = MsiGetActiveDatabase(hPackage);
    rc = MsiDatabaseOpenViewA(db, ExecSeqQuery, &view);
    MsiCloseHandle(db);

    if (rc != ERROR_SUCCESS)
        return rc;

    rc = MsiViewExecute(view, 0);
    if (rc != ERROR_SUCCESS)
    {
        MsiViewClose(view);
        MsiCloseHandle(view);
        return rc;
    }
    
    while (1)
    {
        WCHAR name[0x100];
        WCHAR path[MAX_PATH];
        MSIHANDLE row = 0;
        DWORD sz;

        rc = MsiViewFetch(view,&row);

        if (rc != ERROR_SUCCESS)
        {
            rc = ERROR_SUCCESS;
            break;
        }

        sz=0x100;
        MsiRecordGetStringW(row,1,name,&sz);

        /* This helper function now does ALL the work */
        TRACE("Dir %s ...\n",debugstr_w(name));
        resolve_directory(hPackage,name,path,FALSE);
        TRACE("resolves to %s\n",debugstr_w(path));

        MsiCloseHandle(row);
     }
    MsiViewClose(view);
    MsiCloseHandle(view);

    return ERROR_SUCCESS;
}

/*
 * This is a helper function for handling embedded cabinet media
 */
static UINT writeout_cabinet_stream(MSIHANDLE hPackage, WCHAR* stream_name,
                                    WCHAR* source)
{
    UINT rc;
    USHORT* data;
    UINT    size;
    DWORD   write;
    HANDLE  the_file;
    MSIHANDLE db;

    db = MsiGetActiveDatabase(hPackage);
    rc = read_raw_stream_data(db,stream_name,&data,&size); 
    MsiCloseHandle(db);

    if (rc != ERROR_SUCCESS)
        return rc;

    write = 0x100;
    if (MsiGetPropertyW(hPackage, cszSourceDir, source, &write))
    {
        ERR("No Source dir defined \n");
        rc = ERROR_FUNCTION_FAILED;
        goto end; 
    }

    strcatW(source,stream_name);
    the_file = CreateFileW(source, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, NULL);

    if (the_file == INVALID_HANDLE_VALUE)
    {
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


/***********************************************************************
 *            extract_cabinet_file
 *
 * Extract a file from a .cab file.
 */
static void (WINAPI *pExtractFiles)( LPSTR, LPSTR, DWORD, DWORD, DWORD, DWORD );

static BOOL extract_cabinet_file_advpack( const WCHAR *cabinet, 
                                          const WCHAR *root)
{
    static const WCHAR extW[] = {'.','c','a','b',0};
    static HMODULE advpack;

    char *cab_path, *cab_file;
    int len = strlenW( cabinet );

    /* make sure the cabinet file has a .cab extension */
    if (len <= 4 || strcmpiW( cabinet + len - 4, extW )) return FALSE;
    if (!pExtractFiles)
    {
        if (!advpack && !(advpack = LoadLibraryA( "advpack.dll" )))
        {
            ERR( "could not load advpack.dll\n" );
            return FALSE;
        }
        if (!(pExtractFiles = (void *)GetProcAddress( advpack, "ExtractFiles"
)))
        {
            ERR( "could not find ExtractFiles in advpack.dll\n" );
            return FALSE;
        }
    }

    if (!(cab_file = strdupWtoA( cabinet ))) return FALSE;
    if (!(cab_path = strdupWtoA( root ))) return FALSE;

    FIXME( "awful hack: extracting cabinet %s\n", debugstr_a(cab_file) );
    pExtractFiles( cab_file, cab_path, 0, 0, 0, 0 );
    HeapFree( GetProcessHeap(), 0, cab_file );
    HeapFree( GetProcessHeap(), 0, cab_path );
    return TRUE;
}

static BOOL extract_cabinet_file_cabinet( const WCHAR *cabinet, 
                                          const WCHAR *root)
                                  
{
    static const WCHAR extW[] = {'.','c','a','b',0};

    /* from cabinet.h */

    struct ExtractFileList {
        LPSTR  filename;
        struct ExtractFileList *next;
        BOOL   unknown;  /* always 1L */
    } ;

    typedef struct {
        long  result1;          /* 0x000 */
        long  unknown1[3];      /* 0x004 */
        struct ExtractFileList* filelist;         /* 0x010 */
        long  filecount;        /* 0x014 */
        long  unknown2;         /* 0x018 */
        char  directory[0x104]; /* 0x01c */
        char  lastfile[0x20c];  /* 0x120 */
    } EXTRACTdest;

    HRESULT WINAPI Extract(EXTRACTdest *dest, LPCSTR what);

    char *cab_path, *src_path;
    int len = strlenW( cabinet );
    EXTRACTdest exd;
    struct ExtractFileList fl;

    /* make sure the cabinet file has a .cab extension */
    if (len <= 4 || strcmpiW( cabinet + len - 4, extW )) return FALSE;

    if (!(cab_path = strdupWtoA( cabinet ))) return FALSE;
    if (!(src_path = strdupWtoA( root ))) return FALSE;

    memset(&exd,0,sizeof(exd));
    strcpy(exd.directory,src_path);
    exd.unknown2 = 0x1;
    fl.filename = cab_path;
    fl.next = NULL;
    fl.unknown = 1;
    exd.filelist = &fl;
    FIXME( "more aweful hack: extracting cabinet %s\n", debugstr_a(cab_path) );
    Extract(&exd,cab_path);

    HeapFree( GetProcessHeap(), 0, cab_path );
    HeapFree( GetProcessHeap(), 0, src_path );
    return TRUE;
}


static BOOL extract_cabinet_file(const WCHAR* source, const WCHAR* path)
{
    if (!extract_cabinet_file_advpack(source,path))
        return extract_cabinet_file_cabinet(source,path);
    return TRUE;
}

static UINT ready_media_for_file(MSIHANDLE hPackage, UINT sequence, 
                                 WCHAR* path)
{
    UINT rc;
    MSIHANDLE view;
    MSIHANDLE row = 0;
    WCHAR source[MAX_PATH];
    static const CHAR *ExecSeqQuery = 
        "select * from Media where LastSequence > %i order by LastSequence";
    CHAR Query[1024];
    WCHAR cab[0x100];
    DWORD sz=0x100;
    INT seq;
    static INT last_sequence = 0; 
    MSIHANDLE db;

    if (sequence <= last_sequence)
    {
        TRACE("Media already ready (%i, %i)\n",sequence,last_sequence);
        return ERROR_SUCCESS;
    }

    sprintf(Query,ExecSeqQuery,sequence);

    db = MsiGetActiveDatabase(hPackage);
    rc = MsiDatabaseOpenViewA(db, Query, &view);
    MsiCloseHandle(db);

    if (rc != ERROR_SUCCESS)
        return rc;

    rc = MsiViewExecute(view, 0);
    if (rc != ERROR_SUCCESS)
    {
        MsiViewClose(view);
        MsiCloseHandle(view);
        return rc;
    }

    rc = MsiViewFetch(view,&row);
    if (rc != ERROR_SUCCESS)
    {
        MsiViewClose(view);
        MsiCloseHandle(view);
        return rc;
    }
    seq = MsiRecordGetInteger(row,2);
    last_sequence = seq;

    if (!MsiRecordIsNull(row,4))
    {
        sz=0x100;
        MsiRecordGetStringW(row,4,cab,&sz);
        /* the stream does not contain the # character */
        if (cab[0]=='#')
        {
            writeout_cabinet_stream(hPackage,&cab[1],source);
            strcpyW(path,source);
            *(strrchrW(path,'\\')+1)=0;
        }
        else
        {
            sz = 0x100;
            if (MsiGetPropertyW(hPackage, cszSourceDir, source, &sz))
            {
                ERR("No Source dir defined \n");
                rc = ERROR_FUNCTION_FAILED;
            }
            else
            {
                strcpyW(path,source);
                strcatW(source,cab);
            }
        }
        rc = !extract_cabinet_file(source,path);
    }
    MsiCloseHandle(row);
    MsiViewClose(view);
    MsiCloseHandle(view);
    return rc;
}

static void reduce_to_longfilename(WCHAR* filename)
{
    if (strchrW(filename,'|'))
    {
        WCHAR newname[MAX_PATH];
        strcpyW(newname,strchrW(filename,'|')+1);
        strcpyW(filename,newname);
    }
}

static UINT get_directory_for_component(MSIHANDLE hPackage, 
    const WCHAR* component, WCHAR* install_path)
{
    UINT rc;
    MSIHANDLE view;
    MSIHANDLE row = 0;
    WCHAR ExecSeqQuery[1023] = 
{'s','e','l','e','c','t',' ','*',' ','f','r','o','m',' ','C','o','m'
,'p','o','n','e','n','t',' ','w','h','e','r','e',' ','C','o','m'
,'p','o','n','e','n','t',' ','=',' ','`',0};
    static const WCHAR end[]={'`',0};
    WCHAR dir[0x100];
    DWORD sz=0x100;
    MSIHANDLE db;

    strcatW(ExecSeqQuery,component);
    strcatW(ExecSeqQuery,end);

    db = MsiGetActiveDatabase(hPackage);
    rc = MsiDatabaseOpenViewW(db, ExecSeqQuery, &view);
    MsiCloseHandle(db);

    if (rc != ERROR_SUCCESS)
        return rc;

    rc = MsiViewExecute(view, 0);
    if (rc != ERROR_SUCCESS)
    {
        MsiViewClose(view);
        MsiCloseHandle(view);
        return rc;
    }

    rc = MsiViewFetch(view,&row);
    if (rc != ERROR_SUCCESS)
    {
        MsiViewClose(view);
        MsiCloseHandle(view);
        MsiCloseHandle(row);
        return rc;
    }

    sz=0x100;
    MsiRecordGetStringW(row,3,dir,&sz);
    sz=MAX_PATH;
    rc = MsiGetPropertyW(hPackage, dir, install_path, &sz);

    MsiCloseHandle(row);
    MsiViewClose(view);
    MsiCloseHandle(view);
    return rc;
}

static UINT ACTION_InstallFiles(MSIHANDLE hPackage)
{
    UINT rc;
    MSIHANDLE view;
    MSIHANDLE row = 0;
    static const CHAR *ExecSeqQuery = 
        "select * from File order by Sequence";
    MSIHANDLE db;

    /* REALLY what we want to do is go through all the enabled
     * features and check all the components of that feature and
     * make sure that component is not already install and blah
     * blah blah... I will do it that way some day.. really
     * but for sheer gratification I am going to just brute force
     * install all the files
     */
    db = MsiGetActiveDatabase(hPackage);
    rc = MsiDatabaseOpenViewA(db, ExecSeqQuery, &view);
    MsiCloseHandle(db);

    if (rc != ERROR_SUCCESS)
        return rc;

    rc = MsiViewExecute(view, 0);
    if (rc != ERROR_SUCCESS)
    {
        MsiViewClose(view);
        MsiCloseHandle(view);
        return rc;
    }

    while (1)
    {
        INT seq = 0;
        WCHAR component[0x100];
        WCHAR install_path[MAX_PATH]; 
        WCHAR path_to_source[MAX_PATH];
        WCHAR src_path[MAX_PATH];
        WCHAR filename[0x100]; 
        WCHAR sourcename[0x100]; 
        DWORD sz=0x100;

        rc = MsiViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            rc = ERROR_SUCCESS;
            break;
        }

        seq = MsiRecordGetInteger(row,8);
        rc = ready_media_for_file(hPackage,seq,path_to_source);
        if (rc != ERROR_SUCCESS)
        {
            ERR("Unable to ready media\n");
            MsiCloseHandle(row);
            break;
        }
        sz=0x100;
        rc = MsiRecordGetStringW(row,2,component,&sz);
        if (rc != ERROR_SUCCESS)
        {
            ERR("Unable to read component\n");
            MsiCloseHandle(row);
            break;
        }
        rc = get_directory_for_component(hPackage,component,install_path);
        if (rc != ERROR_SUCCESS)
        {
            ERR("Unable to get directory\n");
            MsiCloseHandle(row);
            break;
        }

        sz=0x100;
        rc = MsiRecordGetStringW(row,1,sourcename,&sz);
        if (rc != ERROR_SUCCESS)
        {
            ERR("Unable to get sourcename\n");
            MsiCloseHandle(row);
            break;
        }
        strcpyW(src_path,path_to_source);
        strcatW(src_path,sourcename);

        sz=0x100;
        rc = MsiRecordGetStringW(row,3,filename,&sz);
        if (rc != ERROR_SUCCESS)
        {
            ERR("Unable to get filename\n");
            MsiCloseHandle(row);
            break;
        }
        reduce_to_longfilename(filename);

        /* create the path */
        create_full_pathW(install_path);

        strcatW(install_path,filename);

        TRACE("Installing file %s to %s\n",debugstr_w(src_path),
              debugstr_w(install_path));

        rc = !MoveFileW(src_path,install_path);
        if (rc)
        {
            ERR("Unable to move file\n");
        }

        /* for future use lets keep track of this file and where it went */
        MsiSetPropertyW(hPackage,sourcename,install_path);

        MsiCloseHandle(row);
    }
    MsiViewClose(view);
    MsiCloseHandle(view);

    return rc;
}

static UINT ACTION_DuplicateFiles(MSIHANDLE hPackage)
{
    UINT rc;
    MSIHANDLE view;
    MSIHANDLE row = 0;
    static const CHAR *ExecSeqQuery = "select * from DuplicateFile";
    MSIHANDLE db;


    /*
     * Yes we should only do this for components that are installed
     * but again I need to do that went I track components.
     */
    db = MsiGetActiveDatabase(hPackage);
    rc = MsiDatabaseOpenViewA(db, ExecSeqQuery, &view);
    MsiCloseHandle(db);

    if (rc != ERROR_SUCCESS)
        return rc;

    rc = MsiViewExecute(view, 0);
    if (rc != ERROR_SUCCESS)
    {
        MsiViewClose(view);
        MsiCloseHandle(view);
        return rc;
    }

    while (1)
    {
        WCHAR file_key[0x100];
        WCHAR file_source[MAX_PATH];
        WCHAR dest_name[0x100];
        WCHAR dest_path[MAX_PATH];

        DWORD sz=0x100;

        rc = MsiViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            rc = ERROR_SUCCESS;
            break;
        }

        sz=0x100;
        rc = MsiRecordGetStringW(row,3,file_key,&sz);
        if (rc != ERROR_SUCCESS)
        {
            ERR("Unable to get file key\n");
            MsiCloseHandle(row);
            break;
        }

        sz = 0x100;
        rc = MsiGetPropertyW(hPackage,file_key,file_source,&sz);
        if (rc != ERROR_SUCCESS)
        {
            ERR("Original file unknown %s\n",debugstr_w(file_key));
            MsiCloseHandle(row);
            break;
        }

        if (MsiRecordIsNull(row,4))
            strcpyW(dest_name,strrchrW(file_source,'\\')+1);
        else
        {
            sz=0x100;
            MsiRecordGetStringW(row,4,dest_name,&sz);
            reduce_to_longfilename(dest_name);
         }

        if (MsiRecordIsNull(row,5))
        {
            strcpyW(dest_path,file_source);
            *strrchrW(dest_path,'\\')=0;
        }
        else
        {
            WCHAR destkey[0x100];
            sz=0x100;
            MsiRecordGetStringW(row,5,destkey,&sz);
            sz = 0x100;
            rc = MsiGetPropertyW(hPackage, destkey, dest_path, &sz);
            if (rc != ERROR_SUCCESS)
            {
                ERR("Unable to get destination folder\n");
                MsiCloseHandle(row);
                break;
            }
        }

        strcatW(dest_path,dest_name);
           
        TRACE("Duplicating file %s to %s\n",debugstr_w(file_source),
              debugstr_w(dest_path)); 
        
        if (strcmpW(file_source,dest_path))
            rc = !CopyFileW(file_source,dest_path,TRUE);
        else
            rc = ERROR_SUCCESS;
        
        if (rc != ERROR_SUCCESS)
            ERR("Failed to copy file\n");
    
        MsiCloseHandle(row);
    }
    MsiViewClose(view);
    MsiCloseHandle(view);
    return rc;

}


/* OK this value is "interpretted" and then formatted based on the 
   first few characters */
static LPSTR parse_value(MSIHANDLE hPackage, WCHAR *value, DWORD *type, 
                         DWORD *size)
{
    LPSTR data = NULL;
    if (value[0]=='#' && value[1]!='#' && value[1]!='%')
    {
        if (value[1]=='x')
        {
            LPWSTR ptr;
            CHAR byte[5];
            LPWSTR deformated;
            int count;

            deformat_string(hPackage, &value[2], &deformated);

            /* binary value type */
            ptr = deformated; 
            *type=REG_BINARY;
            *size = strlenW(ptr)/2;
            data = HeapAlloc(GetProcessHeap(),0,*size);
          
            byte[0] = '0'; 
            byte[1] = 'x'; 
            byte[4] = 0; 
            count = 0;
            while (*ptr)
            {
                byte[2]= *ptr;
                ptr++;
                byte[3]= *ptr;
                ptr++;
                data[count] = (BYTE)strtol(byte,NULL,0);
                count ++;
            }
            HeapFree(GetProcessHeap(),0,deformated);

            TRACE("Data %li bytes(%i)\n",*size,count);
        }
        else
        {
            LPWSTR deformated;
            deformat_string(hPackage, &value[1], &deformated);

            *type=REG_DWORD; 
            *size = sizeof(DWORD);
            data = HeapAlloc(GetProcessHeap(),0,*size);
            *(LPDWORD)data = atoiW(deformated); 
            TRACE("DWORD %i\n",*data);

            HeapFree(GetProcessHeap(),0,deformated);
        }
    }
    else
    {
        WCHAR *ptr;
        *type=REG_SZ;

        if (value[0]=='#')
        {
            if (value[1]=='%')
            {
                ptr = &value[2];
                *type=REG_EXPAND_SZ;
            }
            else
                ptr = &value[1];
         }
         else
            ptr=value;

        *size = deformat_string(hPackage, ptr,(LPWSTR*)&data);
    }
    return data;
}

static UINT ACTION_WriteRegistryValues(MSIHANDLE hPackage)
{
    UINT rc;
    MSIHANDLE view;
    MSIHANDLE row = 0;
    static const CHAR *ExecSeqQuery = "select * from Registry";
    MSIHANDLE db;

    /* Again here we want to key off of the components being installed...
     * oh well
     */
    db = MsiGetActiveDatabase(hPackage);
    rc = MsiDatabaseOpenViewA(db, ExecSeqQuery, &view);
    MsiCloseHandle(db);

    if (rc != ERROR_SUCCESS)
        return rc;

    rc = MsiViewExecute(view, 0);
    if (rc != ERROR_SUCCESS)
    {
        MsiViewClose(view);
        MsiCloseHandle(view);
        return rc;
    }

    while (1)
    {
        WCHAR key[0x100];
        WCHAR name[0x100];
        LPWSTR value;
        LPSTR value_data = NULL;
        HKEY  root_key, hkey;
        DWORD type,size;

        INT   root;
        DWORD sz=0x100;

        rc = MsiViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            rc = ERROR_SUCCESS;
            break;
        }

        /* null values have special meanings during uninstalls and such */
        
        if(MsiRecordIsNull(row,5))
        {
            MsiCloseHandle(row);
            continue;
        }

        root = MsiRecordGetInteger(row,2);
        sz = 0x100;
        MsiRecordGetStringW(row,3,key,&sz);
      
        sz = 0x100; 
        if (MsiRecordIsNull(row,4))
            name[0]=0;
        else
            MsiRecordGetStringW(row,4,name,&sz);
   

        /* get the root key */
        switch (root)
        {
            case 0:  root_key = HKEY_CLASSES_ROOT; break;
            case 1:  root_key = HKEY_CURRENT_USER; break;
            case 2:  root_key = HKEY_LOCAL_MACHINE; break;
            case 3:  root_key = HKEY_USERS; break;
            default:
                 ERR("Unknown root %i\n",root);
                 root_key=NULL;
                 break;
        }
        if (!root_key)
        {
            MsiCloseHandle(row);
            continue;
        }

        if (RegCreateKeyW( root_key, key, &hkey))
        {
            ERR("Could not create key %s\n",debugstr_w(key));
            MsiCloseHandle(row);
            continue;
        }

        sz = 0;
        MsiRecordGetStringW(row,5,NULL,&sz);
        sz++;
        value = HeapAlloc(GetProcessHeap(),0,sz * sizeof(WCHAR));
        MsiRecordGetStringW(row,5,value,&sz);
        value_data = parse_value(hPackage, value, &type, &size); 
        HeapFree(GetProcessHeap(),0,value);

        if (value_data)
        {
            TRACE("Setting value %s\n",debugstr_w(name));
            RegSetValueExW(hkey, name, 0, type, value_data, size);
            HeapFree(GetProcessHeap(),0,value_data);
        }

        MsiCloseHandle(row);
    }
    MsiViewClose(view);
    MsiCloseHandle(view);
    return rc;
}

/*
 * This helper function should probably go alot of places
 *
 * Thinking about this, maybe this should become yet another Bison file
 */
static DWORD deformat_string(MSIHANDLE hPackage, WCHAR* ptr,WCHAR** data)
{
    WCHAR* mark=NULL;
    DWORD size=0;
    DWORD chunk=0;
    WCHAR key[0x100];
    WCHAR value[0x100];
    DWORD sz;

    /* scan for special characters */
    if (!strchrW(ptr,'[') || (strchrW(ptr,'[') && !strchrW(ptr,']')))
    {
        /* not formatted */
        size = (strlenW(ptr)+1) * sizeof(WCHAR);
        *data = HeapAlloc(GetProcessHeap(),0,size);
        strcpyW(*data,ptr);
        return size;
    }
   
    /* formatted string located */ 
    mark = strchrW(ptr,'[');
    if (mark != ptr)
    {
        INT cnt = (mark - ptr);
        TRACE("%i  (%i) characters before marker\n",cnt,(mark-ptr));
        size = cnt * sizeof(WCHAR);
        size += sizeof(WCHAR);
        *data = HeapAlloc(GetProcessHeap(),0,size);
        strncpyW(*data,ptr,cnt);
        (*data)[cnt]=0;
    }
    else
    {
        size = sizeof(WCHAR);
        *data = HeapAlloc(GetProcessHeap(),0,size);
        (*data)[0]=0;
    }
    mark++;
    strcpyW(key,mark);
    *strchrW(key,']')=0;
    mark = strchrW(mark,']');
    mark++;
    TRACE("Current %s .. %s\n",debugstr_w(*data),debugstr_w(mark));
    sz = 0x100;
    if (MsiGetPropertyW(hPackage, key, value,&sz) == ERROR_SUCCESS)
    {
        LPWSTR newdata;
        chunk = (strlenW(value)+1) * sizeof(WCHAR);
        size+=chunk;   
        newdata = HeapReAlloc(GetProcessHeap(),0,*data,size);
        *data = newdata;
        strcatW(*data,value);
    }
    TRACE("Current %s .. %s\n",debugstr_w(*data),debugstr_w(mark));
    if (*mark!=0)
    {
        LPWSTR newdata;
        chunk = (strlenW(mark)+1) * sizeof(WCHAR);
        size+=chunk;
        newdata = HeapReAlloc(GetProcessHeap(),0,*data,size);
        *data = newdata;
        strcatW(*data,mark);
    }
    (*data)[strlenW(*data)]=0;
    TRACE("Current %s .. %s\n",debugstr_w(*data),debugstr_w(mark));

    /* recursively do this to clean up */
    mark = HeapAlloc(GetProcessHeap(),0,size);
    strcpyW(mark,*data);
    TRACE("String at this point %s\n",debugstr_w(mark));
    size = deformat_string(hPackage,mark,data);
    HeapFree(GetProcessHeap(),0,mark);
    return size;
}

/* Msi functions that seem approperate here */
UINT WINAPI MsiDoActionA( MSIHANDLE hInstall, LPCSTR szAction )
{
    LPWSTR szwAction;
    UINT len,rc;

    TRACE(" exteral attempt at action %s\n",szAction);

    if (!szAction)
        return ERROR_FUNCTION_FAILED;
    if (hInstall == 0)
        return ERROR_FUNCTION_FAILED;

    len = MultiByteToWideChar( CP_ACP, 0, szAction, -1, NULL, 0);
    szwAction = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR));

    if (!szwAction)
        return ERROR_FUNCTION_FAILED; 

    MultiByteToWideChar( CP_ACP, 0, szAction, -1, szwAction, len);

    rc = MsiDoActionW(hInstall, szwAction);
    HeapFree(GetProcessHeap(),0,szwAction);
    return rc;
}

UINT WINAPI MsiDoActionW( MSIHANDLE hInstall, LPCWSTR szAction )
{
    TRACE(" exteral attempt at action %s \n",debugstr_w(szAction));
    return ACTION_PerformAction(hInstall,szAction);
}

UINT WINAPI MsiGetTargetPathA( MSIHANDLE hInstall, LPCSTR szFolder, 
                               LPSTR szPathBuf, DWORD* pcchPathBuf) 
{
    LPWSTR szwFolder;
    LPWSTR szwPathBuf;
    UINT len,rc;

    TRACE("getting folder %s %p %li\n",szFolder,szPathBuf, *pcchPathBuf);

    if (!szFolder)
        return ERROR_FUNCTION_FAILED;
    if (hInstall == 0)
        return ERROR_FUNCTION_FAILED;

    len = MultiByteToWideChar( CP_ACP, 0, szFolder, -1, NULL, 0);
    szwFolder= HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR));

    if (!szwFolder)
        return ERROR_FUNCTION_FAILED; 

    szwPathBuf = HeapAlloc( GetProcessHeap(), 0 , *pcchPathBuf * sizeof(WCHAR));

    MultiByteToWideChar( CP_ACP, 0, szFolder, -1, szwFolder, len);

    rc = MsiGetTargetPathW(hInstall, szwFolder, szwPathBuf,pcchPathBuf);

    WideCharToMultiByte( CP_ACP, 0, szwPathBuf, *pcchPathBuf, szPathBuf,
                         *pcchPathBuf, NULL, NULL );

    HeapFree(GetProcessHeap(),0,szwFolder);
    HeapFree(GetProcessHeap(),0,szwPathBuf);

    return rc;
}

UINT WINAPI MsiGetTargetPathW( MSIHANDLE hInstall, LPCWSTR szFolder, LPWSTR
                                szPathBuf, DWORD* pcchPathBuf) 
{
    TRACE("(%s %p %li)\n",debugstr_w(szFolder),szPathBuf,*pcchPathBuf);
    return MsiGetPropertyW(hInstall,szFolder,szPathBuf,pcchPathBuf);
}


UINT WINAPI MsiGetSourcePathA( MSIHANDLE hInstall, LPCSTR szFolder, 
                               LPSTR szPathBuf, DWORD* pcchPathBuf) 
{
    LPWSTR szwFolder;
    LPWSTR szwPathBuf;
    UINT len,rc;

    TRACE("getting source %s %p %li\n",szFolder,szPathBuf, *pcchPathBuf);

    if (!szFolder)
        return ERROR_FUNCTION_FAILED;
    if (hInstall == 0)
        return ERROR_FUNCTION_FAILED;

    len = MultiByteToWideChar( CP_ACP, 0, szFolder, -1, NULL, 0);
    szwFolder= HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR));

    if (!szwFolder)
        return ERROR_FUNCTION_FAILED; 

    szwPathBuf = HeapAlloc( GetProcessHeap(), 0 , *pcchPathBuf * sizeof(WCHAR));

    MultiByteToWideChar( CP_ACP, 0, szFolder, -1, szwFolder, len);

    rc = MsiGetSourcePathW(hInstall, szwFolder, szwPathBuf,pcchPathBuf);

    WideCharToMultiByte( CP_ACP, 0, szwPathBuf, *pcchPathBuf, szPathBuf,
                         *pcchPathBuf, NULL, NULL );

    HeapFree(GetProcessHeap(),0,szwFolder);
    HeapFree(GetProcessHeap(),0,szwPathBuf);

    return rc;
}

UINT WINAPI MsiGetSourcePathW( MSIHANDLE hInstall, LPCWSTR szFolder, LPWSTR
                                szPathBuf, DWORD* pcchPathBuf) 
{
    static const WCHAR cszsrc[]={'_','S','o','u','r','c','e',0};
    LPWSTR newfolder;
    UINT rc;

    TRACE("(%s %p %li)\n",debugstr_w(szFolder),szPathBuf,*pcchPathBuf);

    if (strcmpW(szFolder, cszSourceDir) != 0)
    {
        newfolder = HeapAlloc(GetProcessHeap(),0,
                    (strlenW(szFolder)+8)*sizeof(WCHAR));
        strcpyW(newfolder,szFolder);
        strcatW(newfolder,cszsrc);
        rc = MsiGetPropertyW(hInstall,newfolder,szPathBuf,pcchPathBuf);
        HeapFree(GetProcessHeap(),0,newfolder);
    }
    else
        rc = MsiGetPropertyW(hInstall,szFolder,szPathBuf,pcchPathBuf);
    return rc;
}


UINT WINAPI MsiSetTargetPathA(MSIHANDLE hInstall, LPCSTR szFolder, 
                             LPCSTR szFolderPath)
{
    LPWSTR szwFolder;
    LPWSTR szwFolderPath;
    UINT rc,len;

    if (!szFolder)
        return ERROR_FUNCTION_FAILED;
    if (hInstall == 0)
        return ERROR_FUNCTION_FAILED;

    len = MultiByteToWideChar( CP_ACP, 0, szFolder, -1, NULL, 0);
    szwFolder= HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR));

    if (!szwFolder)
        return ERROR_FUNCTION_FAILED; 

    MultiByteToWideChar( CP_ACP, 0, szFolder, -1, szwFolder, len);

    len = MultiByteToWideChar( CP_ACP, 0, szFolderPath, -1, NULL, 0);
    szwFolderPath= HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR));

    if (!szwFolderPath)
    {
        HeapFree(GetProcessHeap(),0,szwFolder);
        return ERROR_FUNCTION_FAILED; 
    }

    MultiByteToWideChar( CP_ACP, 0, szFolderPath, -1, szwFolderPath, len);

    rc = MsiSetTargetPathW(hInstall, szwFolder, szwFolderPath);

    HeapFree(GetProcessHeap(),0,szwFolder);
    HeapFree(GetProcessHeap(),0,szwFolderPath);

    return rc;
}

UINT WINAPI MsiSetTargetPathW(MSIHANDLE hInstall, LPCWSTR szFolder, 
                             LPCWSTR szFolderPath)
{
    TRACE("(%s %s)\n",debugstr_w(szFolder),debugstr_w(szFolderPath));

    return MsiSetPropertyW(hInstall,szFolder,szFolderPath);
}

BOOL WINAPI MsiGetMode(MSIHANDLE hInstall, DWORD iRunMode)
{
    FIXME("STUB (%li)\n",iRunMode);
    return FALSE;
}

#if 0
static UINT ACTION_Template(MSIHANDLE hPackage)
{
    UINT rc;
    MSIHANDLE view;
    MSIHANDLE row = 0;
    static const CHAR *ExecSeqQuery;
    MSIHANDLE db;

    db = MsiGetActiveDatabase(hPackage);
    rc = MsiDatabaseOpenViewA(db, ExecSeqQuery, &view);
    MsiCloseHandle(db);

    if (rc != ERROR_SUCCESS)
        return rc;

    rc = MsiViewExecute(view, 0);
    if (rc != ERROR_SUCCESS)
    {
        MsiViewClose(view);
        MsiCloseHandle(view);
        return rc;
    }

    while (1)
    {
        rc = MsiViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            rc = ERROR_SUCCESS;
            break;
        }

        MsiCloseHandle(row);
    }
    MsiViewClose(view);
    MsiCloseHandle(view);
    return rc;
}
#endif
