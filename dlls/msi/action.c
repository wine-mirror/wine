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
#include "ver.h"

#define CUSTOM_ACTION_TYPE_MASK 0x3F

WINE_DEFAULT_DEBUG_CHANNEL(msi);

typedef struct tagMSIFEATURE
{
    WCHAR Feature[96];
    WCHAR Feature_Parent[96];
    WCHAR Title[0x100];
    WCHAR Description[0x100];
    INT Display;
    INT Level;
    WCHAR Directory[96];
    INT Attributes;
    
    INSTALLSTATE State;
    INT ComponentCount;
    INT Components[1024]; /* yes hardcoded limit.... I am bad */
    INT Cost;
} MSIFEATURE;

typedef struct tagMSICOMPONENT
{
    WCHAR Component[96];
    WCHAR ComponentId[96];
    WCHAR Directory[96];
    INT Attributes;
    WCHAR Condition[0x100];
    WCHAR KeyPath[96];

    INSTALLSTATE State;
    BOOL FeatureState;
    BOOL Enabled;
    INT  Cost;
}MSICOMPONENT;

typedef struct tagMSIFOLDER
{
    WCHAR Directory[96];
    WCHAR TargetDefault[96];
    WCHAR SourceDefault[96];

    WCHAR ResolvedTarget[MAX_PATH];
    WCHAR ResolvedSource[MAX_PATH];
    WCHAR Property[MAX_PATH];   /* initialy set property */
    INT   ParentIndex;
    INT   State;
        /* 0 = uninitialized */
        /* 1 = existing */
        /* 2 = created remove if empty */
        /* 3 = created persist if empty */
    INT   Cost;
    INT   Space;
}MSIFOLDER;

typedef struct tagMSIFILE
{
    WCHAR File[72];
    INT ComponentIndex;
    WCHAR FileName[MAX_PATH];
    INT FileSize;
    WCHAR Version[72];
    WCHAR Language[20];
    INT Attributes;
    INT Sequence;   

    INT State;
       /* 0 = uninitialize */
       /* 1 = not present */
       /* 2 = present but replace */
       /* 3 = present do not replace */
       /* 4 = Installed */
    WCHAR   SourcePath[MAX_PATH];
    WCHAR   TargetPath[MAX_PATH];
    BOOL    Temporary; 
}MSIFILE;

/*
 * Prototypes
 */
static UINT ACTION_ProcessExecSequence(MSIHANDLE hPackage, BOOL UIran);
static UINT ACTION_ProcessUISequence(MSIHANDLE hPackage);

UINT ACTION_PerformAction(MSIHANDLE hPackage, const WCHAR *action);
static UINT ACTION_CostInitialize(MSIHANDLE hPackage);
static UINT ACTION_CreateFolders(MSIHANDLE hPackage);
static UINT ACTION_CostFinalize(MSIHANDLE hPackage);
static UINT ACTION_FileCost(MSIHANDLE hPackage);
static UINT ACTION_InstallFiles(MSIHANDLE hPackage);
static UINT ACTION_DuplicateFiles(MSIHANDLE hPackage);
static UINT ACTION_WriteRegistryValues(MSIHANDLE hPackage);
static UINT ACTION_CustomAction(MSIHANDLE hPackage,const WCHAR *action);
static UINT ACTION_InstallInitialize(MSIHANDLE hPackage);
static UINT ACTION_InstallValidate(MSIHANDLE hPackage);

static UINT HANDLE_CustomType1(MSIHANDLE hPackage, const LPWSTR source, 
                                const LPWSTR target, const INT type);
static UINT HANDLE_CustomType2(MSIHANDLE hPackage, const LPWSTR source, 
                                const LPWSTR target, const INT type);

static DWORD deformat_string(MSIHANDLE hPackage, WCHAR* ptr,WCHAR** data);
static UINT resolve_folder(MSIHANDLE hPackage, LPCWSTR name, LPWSTR path, 
                           BOOL source, BOOL set_prop, MSIFOLDER **folder);

static UINT track_tempfile(MSIHANDLE hPackage, LPCWSTR name, LPCWSTR path);
 
/*
 * consts and values used
 */
static const WCHAR cszSourceDir[] = {'S','o','u','r','c','e','D','i','r',0};
static const WCHAR cszRootDrive[] = {'R','O','O','T','D','R','I','V','E',0};
static const WCHAR cszTargetDir[] = {'T','A','R','G','E','T','D','I','R',0};
static const WCHAR cszTempFolder[]= {'T','e','m','p','F','o','l','d','e','r',0};
static const WCHAR cszDatabase[]={'D','A','T','A','B','A','S','E',0};
static const WCHAR c_collen[] = {'C',':','\\',0};
 
static const WCHAR cszlsb[]={'[',0};
static const WCHAR cszrsb[]={']',0};
static const WCHAR cszbs[]={'\\',0};


/******************************************************** 
 * helper functions to get around current HACKS and such
 ********************************************************/
inline static void reduce_to_longfilename(WCHAR* filename)
{
    if (strchrW(filename,'|'))
    {
        WCHAR newname[MAX_PATH];
        strcpyW(newname,strchrW(filename,'|')+1);
        strcpyW(filename,newname);
    }
}

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

inline static int get_loaded_component(MSIPACKAGE* package, LPCWSTR Component )
{
    INT rc = -1;
    INT i;

    for (i = 0; i < package->loaded_components; i++)
    {
        if (strcmpW(Component,package->components[i].Component)==0)
        {
            rc = i;
            break;
        }
    }
    return rc;
}

inline static int get_loaded_feature(MSIPACKAGE* package, LPCWSTR Feature )
{
    INT rc = -1;
    INT i;

    for (i = 0; i < package->loaded_features; i++)
    {
        if (strcmpW(Feature,package->features[i].Feature)==0)
        {
            rc = i;
            break;
        }
    }
    return rc;
}

static UINT track_tempfile(MSIHANDLE hPackage, LPCWSTR name, LPCWSTR path)
{
    MSIPACKAGE *package;
    int i;
    int index;

    package = msihandle2msiinfo(hPackage, MSIHANDLETYPE_PACKAGE);

    if (!package)
        return -2;

    for (i=0; i < package->loaded_files; i++)
        if (strcmpW(package->files[i].File,name)==0)
            return -1;

    index = package->loaded_files;
    package->loaded_files++;
    if (package->loaded_files== 1)
        package->files = HeapAlloc(GetProcessHeap(),0,sizeof(MSIFILE));
    else
        package->files = HeapReAlloc(GetProcessHeap(),0,
            package->files , package->loaded_files * sizeof(MSIFILE));

    memset(&package->files[index],0,sizeof(MSIFILE));

    strcpyW(package->files[index].File,name);
    strcpyW(package->files[index].TargetPath,path);
    package->files[index].Temporary = TRUE;

    TRACE("Tracking tempfile (%s)\n",debugstr_w(package->files[index].File));  

    return 0;
}

void ACTION_remove_tracked_tempfiles(MSIPACKAGE* package)
{
    int i;

    if (!package)
        return;

    for (i = 0; i < package->loaded_files; i++)
    {
        if (package->files[i].Temporary)
            DeleteFileW(package->files[i].TargetPath);

    }
}

static void progress_message(MSIHANDLE hPackage, int a, int b, int c, int d )
{
    MSIHANDLE row;

    row = MsiCreateRecord(4);
    MsiRecordSetInteger(row,1,a);
    MsiRecordSetInteger(row,2,b);
    MsiRecordSetInteger(row,3,c);
    MsiRecordSetInteger(row,4,d);
    MsiProcessMessage(hPackage, INSTALLMESSAGE_PROGRESS, row);
    MsiCloseHandle(row);
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
            WCHAR prop[0x100];
            WCHAR val[0x100];

            TRACE("Looking at %s\n",debugstr_w(ptr));

            ptr2 = strchrW(ptr,'=');
            if (ptr2)
            {
                BOOL quote=FALSE;
                DWORD len = 0;
                strncpyW(prop,ptr,ptr2-ptr);
                prop[ptr2-ptr]=0;
                ptr2++;
            
                ptr = ptr2; 
                while (*ptr && (quote || (!quote && *ptr!=' ')))
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
            TRACE("Found commandline property (%s) = (%s)\n", debugstr_w(prop),
                  debugstr_w(val));
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
    const static WCHAR szFileCost[] = 
        {'F','i','l','e','C','o','s','t',0};
    const static WCHAR szInstallInitialize[] = 
{'I','n','s','t','a','l','l','I','n','i','t','i','a','l','i','z','e',0};
    const static WCHAR szInstallValidate[] = 
{'I','n','s','t','a','l','l','V','a','l','i','d','a','t','e',0};

    TRACE("Performing action (%s)\n",debugstr_w(action));
    progress_message(hPackage,2,25,0,0);

    /* pre install, setup and configureation block */
    if (strcmpW(action,szCostInitialize)==0)
        return ACTION_CostInitialize(hPackage);
    if (strcmpW(action,szFileCost)==0)
        return ACTION_FileCost(hPackage);
    if (strcmpW(action,szCostFinalize)==0)
        return ACTION_CostFinalize(hPackage);
    if (strcmpW(action,szInstallValidate)==0)
        return ACTION_InstallValidate(hPackage);

    /* install block */
    if (strcmpW(action,szInstallInitialize)==0)
        return ACTION_InstallInitialize(hPackage);
    if (strcmpW(action,szCreateFolders)==0)
        return ACTION_CreateFolders(hPackage);
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
     ValidateProductID (sets ProductID)
     IsolateComponents (Empty)
     SetODBCFolders 
     MigrateFeatureStates
     RemoveExistingProducts
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
        FIXME("UNHANDLED MSI ACTION %s\n",debugstr_w(action));

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
            FIXME("UNHANDLED ACTION TYPE %i (%s %s)\n",
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
    DWORD sz=MAX_PATH;

    if (MsiGetPropertyW(hPackage, cszTempFolder, tmp_file, &sz) 
        != ERROR_SUCCESS)
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

        if (track_tempfile(hPackage, source, tmp_file)!=0)
            FIXME("File Name in temp tracking collision\n");

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
        FIXME("Asynchronous execution.. UNHANDLED\n");
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
    MSIFOLDER *folder;

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
        rc = resolve_folder(hPackage,dir,full_path,FALSE,FALSE,&folder);

        if (rc != ERROR_SUCCESS)
        {
            ERR("Unable to resolve folder id %s\n",debugstr_w(dir));
            MsiCloseHandle(row);
            continue;
        }

        TRACE("Folder is %s\n",debugstr_w(full_path));

        if (folder->State == 0)
            create_full_pathW(full_path);

        folder->State = 3;

        MsiCloseHandle(row);
    }
    MsiViewClose(view);
    MsiCloseHandle(view);
   
    return rc;
}

static int load_component(MSIPACKAGE* package, MSIHANDLE row)
{
    int index = package->loaded_components;
    DWORD sz;

    /* fill in the data */

    package->loaded_components++;
    if (package->loaded_components == 1)
        package->components = HeapAlloc(GetProcessHeap(),0,
                                        sizeof(MSICOMPONENT));
    else
        package->components = HeapReAlloc(GetProcessHeap(),0,
            package->components, package->loaded_components * 
            sizeof(MSICOMPONENT));

    memset(&package->components[index],0,sizeof(MSICOMPONENT));

    sz = 96;       
    MsiRecordGetStringW(row,1,package->components[index].Component,&sz);

    TRACE("Loading Component %s\n",
           debugstr_w(package->components[index].Component));

    sz = 0x100;
    if (!MsiRecordIsNull(row,2))
        MsiRecordGetStringW(row,2,package->components[index].ComponentId,&sz);
            
    sz = 96;       
    MsiRecordGetStringW(row,3,package->components[index].Directory,&sz);

    package->components[index].Attributes = MsiRecordGetInteger(row,4);

    sz = 0x100;       
    MsiRecordGetStringW(row,5,package->components[index].Condition,&sz);

    sz = 96;       
    MsiRecordGetStringW(row,6,package->components[index].KeyPath,&sz);

    package->components[index].State = INSTALLSTATE_UNKNOWN;
    package->components[index].Enabled = TRUE;
    package->components[index].FeatureState= FALSE;

    return index;
}

static void load_feature(MSIPACKAGE* package, MSIHANDLE row)
{
    int index = package->loaded_features;
    DWORD sz;
    static const WCHAR Query1[] = {'S','E','L','E','C','T',' ','C','o','m','p',
'o','n','e','n','t','_',' ','F','R','O','M',' ','F','e','a','t','u','r','e',
'C','o','m','p','o','n','e','n','t','s',' ','W','H','E','R','E',' ','F','e',
'a','t','u','r','e','_','=','\'','%','s','\'',0};
    static const WCHAR Query2[] = {'S','E','L','E','C','T',' ','*',' ','F','R',
'O','M',' ','C','o','m','p','o','n','e','n','t',' ','W','H','E','R','E',' ','C',
'o','m','p','o','n','e','n','t','=','\'','%','s','\'',0};
    WCHAR Query[1024];
    MSIHANDLE view;
    MSIHANDLE view2;
    MSIHANDLE row2;
    MSIHANDLE row3;

    /* fill in the data */

    package->loaded_features ++;
    if (package->loaded_features == 1)
        package->features = HeapAlloc(GetProcessHeap(),0,sizeof(MSIFEATURE));
    else
        package->features = HeapReAlloc(GetProcessHeap(),0,package->features,
                                package->loaded_features * sizeof(MSIFEATURE));

    memset(&package->features[index],0,sizeof(MSIFEATURE));
    
    sz = 96;       
    MsiRecordGetStringW(row,1,package->features[index].Feature,&sz);

    TRACE("Loading feature %s\n",debugstr_w(package->features[index].Feature));

    sz = 96;
    if (!MsiRecordIsNull(row,2))
        MsiRecordGetStringW(row,2,package->features[index].Feature_Parent,&sz);

    sz = 0x100;
     if (!MsiRecordIsNull(row,3))
        MsiRecordGetStringW(row,3,package->features[index].Title,&sz);

     sz = 0x100;
     if (!MsiRecordIsNull(row,4))
        MsiRecordGetStringW(row,4,package->features[index].Description,&sz);

    if (!MsiRecordIsNull(row,5))
        package->features[index].Display = MsiRecordGetInteger(row,5);
  
    package->features[index].Level= MsiRecordGetInteger(row,6);

     sz = 96;
     if (!MsiRecordIsNull(row,7))
        MsiRecordGetStringW(row,7,package->features[index].Directory,&sz);

    package->features[index].Attributes= MsiRecordGetInteger(row,8);
    package->features[index].State = INSTALLSTATE_UNKNOWN;

    /* load feature components */

    sprintfW(Query,Query1,package->features[index].Feature);
    MsiDatabaseOpenViewW(package->db,Query,&view);
    MsiViewExecute(view,0);
    while (1)
    {
        DWORD sz = 0x100;
        WCHAR buffer[0x100];
        DWORD rc;
        INT c_indx;
        INT cnt = package->features[index].ComponentCount;

        rc = MsiViewFetch(view,&row2);
        if (rc != ERROR_SUCCESS)
            break;

        sz = 0x100;
        MsiRecordGetStringW(row2,1,buffer,&sz);

        /* check to see if the component is already loaded */
        c_indx = get_loaded_component(package,buffer);
        if (c_indx != -1)
        {
            TRACE("Component %s already loaded at %i\n", debugstr_w(buffer),
                  c_indx);
            package->features[index].Components[cnt] = c_indx;
            package->features[index].ComponentCount ++;
        }

        sprintfW(Query,Query2,buffer);
   
        MsiDatabaseOpenViewW(package->db,Query,&view2);
        MsiViewExecute(view2,0);
        while (1)
        {
            DWORD rc;

            rc = MsiViewFetch(view2,&row3);
            if (rc != ERROR_SUCCESS)
                break;
            c_indx = load_component(package,row3);
            MsiCloseHandle(row3);

            package->features[index].Components[cnt] = c_indx;
            package->features[index].ComponentCount ++;
        }
        MsiViewClose(view2);
        MsiCloseHandle(view2);
        MsiCloseHandle(row2);
    }
    MsiViewClose(view);
    MsiCloseHandle(view);
}

/*
 * I am not doing any of the costing functionality yet. 
 * Mostly looking at doing the Component and Feature loading
 *
 * The native MSI does ALOT of modification to tables here. Mostly adding alot
 * of temporary columns to the Feature and Component tables. 
 *
 *    note: native msi also tracks the short filename. but i am only going to
 *          track the long ones.  Also looking at this directory table
 *          it appears that the directory table does not get the parents
 *          resolved base on property only based on their entrys in the 
 *          directory table.
 */
static UINT ACTION_CostInitialize(MSIHANDLE hPackage)
{
    MSIHANDLE view;
    MSIHANDLE row;
    DWORD sz;
    MSIPACKAGE *package;

    static const CHAR Query_all[] = "SELECT * FROM Feature";

    MsiSetPropertyA(hPackage,"CostingComplete","0");
    MsiSetPropertyW(hPackage, cszRootDrive , c_collen);

    package = msihandle2msiinfo(hPackage, MSIHANDLETYPE_PACKAGE);

    sz = 0x100;
    MsiDatabaseOpenViewA(package->db,Query_all,&view);
    MsiViewExecute(view,0);
    while (1)
    {
        DWORD rc;

        rc = MsiViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
            break;
       
        load_feature(package,row); 
        MsiCloseHandle(row);
    }
    MsiViewClose(view);
    MsiCloseHandle(view);

    return ERROR_SUCCESS;
}

static int load_file(MSIPACKAGE* package, MSIHANDLE row)
{
    int index = package->loaded_files;
    int i;
    WCHAR buffer[0x100];
    DWORD sz;

    /* fill in the data */

    package->loaded_files++;
    if (package->loaded_files== 1)
        package->files = HeapAlloc(GetProcessHeap(),0,sizeof(MSIFILE));
    else
        package->files = HeapReAlloc(GetProcessHeap(),0,
            package->files , package->loaded_files * sizeof(MSIFILE));

    memset(&package->files[index],0,sizeof(MSIFILE));

    sz = 72;       
    MsiRecordGetStringW(row,1,package->files[index].File,&sz);

    sz = 0x100;       
    MsiRecordGetStringW(row,2,buffer,&sz);

    package->files[index].ComponentIndex = -1;
    for (i = 0; i < package->loaded_components; i++)
        if (strcmpW(package->components[i].Component,buffer)==0)
        {
            package->files[index].ComponentIndex = i;
            break;
        }
    if (package->files[index].ComponentIndex == -1)
        ERR("Unfound Component %s\n",debugstr_w(buffer));

    sz = MAX_PATH;       
    MsiRecordGetStringW(row,3,package->files[index].FileName,&sz);

    reduce_to_longfilename(package->files[index].FileName);
    
    package->files[index].FileSize = MsiRecordGetInteger(row,4);

    sz = 72;       
    if (!MsiRecordIsNull(row,5))
        MsiRecordGetStringW(row,5,package->files[index].Version,&sz);

    sz = 20;       
    if (!MsiRecordIsNull(row,6))
        MsiRecordGetStringW(row,6,package->files[index].Language,&sz);

    if (!MsiRecordIsNull(row,7))
        package->files[index].Attributes= MsiRecordGetInteger(row,7);

    package->files[index].Sequence= MsiRecordGetInteger(row,8);

    package->files[index].Temporary = FALSE;
    package->files[index].State = 0;

    TRACE("File Loaded (%s)\n",debugstr_w(package->files[index].File));  
 
    return ERROR_SUCCESS;
}

static UINT ACTION_FileCost(MSIHANDLE hPackage)
{
    MSIHANDLE view;
    MSIHANDLE row;
    MSIPACKAGE *package;
    UINT rc;
    static const CHAR Query[] = "SELECT * FROM File Order by Sequence";

    package = msihandle2msiinfo(hPackage, MSIHANDLETYPE_PACKAGE);
    if (!package)
        return ERROR_INVALID_HANDLE;

    rc = MsiDatabaseOpenViewA(package->db, Query, &view);
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
        load_file(package,row);
        MsiCloseHandle(row);
    }
    MsiViewClose(view);
    MsiCloseHandle(view);

    return ERROR_SUCCESS;
}

static INT load_folder(MSIHANDLE hPackage, const WCHAR* dir)

{
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
    WCHAR parent[0x100];
    DWORD sz=0x100;
    MSIHANDLE row = 0;
    MSIPACKAGE *package;
    INT i,index = -1;

    package = msihandle2msiinfo(hPackage, MSIHANDLETYPE_PACKAGE);

    TRACE("Looking for dir %s\n",debugstr_w(dir));

    for (i = 0; i < package->loaded_folders; i++)
    {
        if (strcmpW(package->folders[i].Directory,dir)==0)
        {
            TRACE(" %s retuning on index %i\n",debugstr_w(dir),i);
            return i;
        }
    }

    TRACE("Working to load %s\n",debugstr_w(dir));

    index = package->loaded_folders; 

    package->loaded_folders++;
    if (package->loaded_folders== 1)
        package->folders = HeapAlloc(GetProcessHeap(),0,
                                        sizeof(MSIFOLDER));
    else
        package->folders= HeapReAlloc(GetProcessHeap(),0,
            package->folders, package->loaded_folders* 
            sizeof(MSIFOLDER));

    memset(&package->folders[index],0,sizeof(MSIFOLDER));

    strcpyW(package->folders[index].Directory,dir);

    strcatW(Query,dir);
    strcatW(Query,end);

    rc = MsiDatabaseOpenViewW(package->db, Query, &view);

    if (rc != ERROR_SUCCESS)
        return -1;

    rc = MsiViewExecute(view, 0);
    if (rc != ERROR_SUCCESS)
    {
        MsiViewClose(view);
        MsiCloseHandle(view);
        return -1;
    }

    rc = MsiViewFetch(view,&row);
    if (rc != ERROR_SUCCESS)
    {
        MsiViewClose(view);
        MsiCloseHandle(view);
        return -1;
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

     if (targetdir)
        strcpyW(package->folders[index].TargetDefault,targetdir);

     if (srcdir)
        strcpyW(package->folders[index].SourceDefault,srcdir);
     else if (targetdir)
        strcpyW(package->folders[index].SourceDefault,targetdir);

    if (MsiRecordIsNull(row,2))
        parent[0]=0;
    else
    {
            sz=0x100;
            MsiRecordGetStringW(row,2,parent,&sz);
    }

    if (parent[0]) 
    {
        i = load_folder(hPackage,parent);
        package->folders[index].ParentIndex = i;
        TRACE("Parent is index %i... %s %s\n",
                    package->folders[index].ParentIndex,
    debugstr_w(package->folders[package->folders[index].ParentIndex].Directory),
                    debugstr_w(parent));
    }
    else
        package->folders[index].ParentIndex = -2;

    sz = MAX_PATH;
    rc = MsiGetPropertyW(hPackage, dir, package->folders[index].Property, &sz);
    if (rc != ERROR_SUCCESS)
        package->folders[index].Property[0]=0;

    MsiCloseHandle(row);
    MsiViewClose(view);
    MsiCloseHandle(view);
    TRACE(" %s retuning on index %i\n",debugstr_w(dir),index);
    return index;
}

static UINT resolve_folder(MSIHANDLE hPackage, LPCWSTR name, LPWSTR path, 
                           BOOL source, BOOL set_prop, MSIFOLDER **folder)
{
    MSIPACKAGE *package;
    INT i;
    UINT rc = ERROR_SUCCESS;
    DWORD sz;

    package = msihandle2msiinfo(hPackage, MSIHANDLETYPE_PACKAGE);

    TRACE("Working to resolve %s\n",debugstr_w(name));

    if (!path)
        return rc;

    /* special resolving for Target and Source root dir */
    if (strcmpW(name,cszTargetDir)==0 || strcmpW(name,cszSourceDir)==0)
    {
        if (!source)
        {
            sz = MAX_PATH;
            rc = MsiGetPropertyW(hPackage,cszTargetDir,path,&sz);
            if (rc != ERROR_SUCCESS)
            {
                sz = MAX_PATH;
                rc = MsiGetPropertyW(hPackage,cszRootDrive,path,&sz);
                if (set_prop)
                    MsiSetPropertyW(hPackage,cszTargetDir,path);
            }
            if (folder)
                *folder = &(package->folders[0]);
            return rc;
        }
        else
        {
            sz = MAX_PATH;
            rc = MsiGetPropertyW(hPackage,cszSourceDir,path,&sz);
            if (rc != ERROR_SUCCESS)
            {
                sz = MAX_PATH;
                rc = MsiGetPropertyW(hPackage,cszDatabase,path,&sz);
                if (rc == ERROR_SUCCESS)
                {
                    LPWSTR ptr = strrchrW(path,'\\');
                    if (ptr)
                    {
                        ptr++;
                        *ptr = 0;
                    }
                }
            }
            if (folder)
                *folder = &(package->folders[0]);
            return rc;
        }
    }

    for (i = 0; i < package->loaded_folders; i++)
    {
        if (strcmpW(package->folders[i].Directory,name)==0)
            break;
    }

    if (i >= package->loaded_folders)
        return ERROR_FUNCTION_FAILED;

    if (folder)
        *folder = &(package->folders[i]);

    if (!source && package->folders[i].ResolvedTarget[0])
    {
        strcpyW(path,package->folders[i].ResolvedTarget);
        TRACE("   already resolved to %s\n",debugstr_w(path));
        return ERROR_SUCCESS;
    }
    else if (source && package->folders[i].ResolvedSource[0])
    {
        strcpyW(path,package->folders[i].ResolvedSource);
        return ERROR_SUCCESS;
    }
    else if (!source && package->folders[i].Property[0])
    {
        strcpyW(path,package->folders[i].Property);
        TRACE("   internally set to %s\n",debugstr_w(path));
        if (set_prop)
            MsiSetPropertyW(hPackage,name,path);
        return ERROR_SUCCESS;
    }

    if (package->folders[i].ParentIndex >= 0)
    {
        TRACE(" ! Parent is %s\n", debugstr_w(package->folders[
                   package->folders[i].ParentIndex].Directory));
        resolve_folder(hPackage, package->folders[
                       package->folders[i].ParentIndex].Directory, path,source,
                       set_prop, NULL);

        if (!source && package->folders[i].TargetDefault[0])
        {
            strcatW(path,package->folders[i].TargetDefault);
            strcatW(path,cszbs);
            strcpyW(package->folders[i].ResolvedTarget,path);
            TRACE("   resolved into %s\n",debugstr_w(path));
            if (set_prop)
                MsiSetPropertyW(hPackage,name,path);
        }
        else if (package->folders[i].SourceDefault[0])
        {
            strcatW(path,package->folders[i].SourceDefault);
            strcatW(path,cszbs);
            strcpyW(package->folders[i].ResolvedSource,path);
        }
    }
    return rc;
}

/* 
 * Alot is done in this function aside from just the costing.
 * The costing needs to be implemented at some point but for now I am going
 * to focus on the directory building
 *
 */
static UINT ACTION_CostFinalize(MSIHANDLE hPackage)
{
    static const CHAR *ExecSeqQuery = "select * from Directory";
    static const CHAR *ConditionQuery = "select * from Condition";
    UINT rc;
    MSIHANDLE view;
    MSIPACKAGE *package;
    INT i;

    TRACE("Building Directory properties\n");

    package = msihandle2msiinfo(hPackage, MSIHANDLETYPE_PACKAGE);

    rc = MsiDatabaseOpenViewA(package->db, ExecSeqQuery, &view);

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
        load_folder(hPackage,name);
        resolve_folder(hPackage,name,path,FALSE,TRUE,NULL);
        TRACE("resolves to %s\n",debugstr_w(path));

        MsiCloseHandle(row);
     }
    MsiViewClose(view);
    MsiCloseHandle(view);

    TRACE("File calculations %i files\n",package->loaded_files);

    for (i = 0; i < package->loaded_files; i++)
    {
        MSICOMPONENT* comp = NULL;
        MSIFILE* file= NULL;

        file = &package->files[i];
        if (file->ComponentIndex >= 0)
            comp = &package->components[file->ComponentIndex];

        if (comp)
        {
            /* calculate target */
            resolve_folder(hPackage, comp->Directory, file->TargetPath, FALSE,
                       FALSE, NULL);
            strcatW(file->TargetPath,file->FileName);

            TRACE("file %s resolves to %s\n",
                   debugstr_w(file->File),debugstr_w(file->TargetPath));       
 
            if (GetFileAttributesW(file->TargetPath) == INVALID_FILE_ATTRIBUTES)
            {
                file->State = 1;
                comp->Cost += file->FileSize;
            }
            else
            {
                if (file->Version[0])
                {
                    DWORD handle;
                    DWORD versize;
                    UINT sz;
                    LPVOID version;
                    WCHAR filever[0x100];
                    static const WCHAR name[] =
                        {'\\','V','a','r','F','i','l','e','I','n','f','o',
                         '\\','F','i','l','e','V','e','r','s','i','o','n',0};

                    FIXME("Version comparison.. Untried Untested and most "
                          "likely very very wrong\n");
                    versize = GetFileVersionInfoSizeW(file->TargetPath,&handle);
                    version = HeapAlloc(GetProcessHeap(),0,versize);
                    GetFileVersionInfoW(file->TargetPath, 0, versize, version);
                    sz = 0x100;
                    VerQueryValueW(version,name,(LPVOID)filever,&sz);
                    HeapFree(GetProcessHeap(),0,version);
                
                    if (strcmpW(version,file->Version)<0)
                    {
                        file->State = 2;
                        FIXME("cost should be diff in size\n");
                        comp->Cost += file->FileSize;
                    }
                    else
                        file->State = 3;
                }
                else
                    file->State = 3;
            }
        } 
    }

    TRACE("Evaluating Condition Table\n");

    rc = MsiDatabaseOpenViewA(package->db, ConditionQuery, &view);

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
        WCHAR Feature[0x100];
        WCHAR Condition[0x100];
        MSIHANDLE row = 0;
        DWORD sz;
        int feature_index;

        rc = MsiViewFetch(view,&row);

        if (rc != ERROR_SUCCESS)
        {
            rc = ERROR_SUCCESS;
            break;
        }

        sz = 0x100;
        MsiRecordGetStringW(row,1,Feature,&sz);
        sz = 0x100;
        MsiRecordGetStringW(row,3,Condition,&sz);

        feature_index = get_loaded_feature(package,Feature);
        if (feature_index < 0)
            ERR("FAILED to find loaded feature %s\n",debugstr_w(Feature));
        else
        {
            if (MsiEvaluateConditionW(hPackage,Condition) == MSICONDITION_TRUE)
            {
                int level = MsiRecordGetInteger(row,2);
                TRACE("Reseting feature %s to level %i\n",debugstr_w(Feature),
                       level);
                package->features[feature_index].Level = level;
            }
        }

        MsiCloseHandle(row);
    }
    MsiViewClose(view);
    MsiCloseHandle(view);

    TRACE("Enabling or Disabling Components\n");
    for (i = 0; i < package->loaded_components; i++)
    {
        if (package->components[i].Condition[0])
        {
            if (MsiEvaluateConditionW(hPackage,
                package->components[i].Condition) == MSICONDITION_FALSE)
            {
                TRACE("Disabling component %s\n",
                      debugstr_w(package->components[i].Component));
                package->components[i].Enabled = FALSE;
            }
        }
    }

    MsiSetPropertyA(hPackage,"CostingComplete","1");
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
    WCHAR tmp[MAX_PATH];

    db = MsiGetActiveDatabase(hPackage);
    rc = read_raw_stream_data(db,stream_name,&data,&size); 
    MsiCloseHandle(db);

    if (rc != ERROR_SUCCESS)
        return rc;

    write = MAX_PATH;
    if (MsiGetPropertyW(hPackage, cszTempFolder, tmp, &write))
        GetTempPathW(MAX_PATH,tmp);

    GetTempFileNameW(tmp,stream_name,0,source);

    track_tempfile(hPackage,strrchrW(source,'\\'), source);
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
 * Extract  files from a cab file.
 */
static void (WINAPI *pExtractFiles)( LPSTR, LPSTR, DWORD, DWORD, DWORD, DWORD );

static BOOL extract_cabinet_file_advpack( const WCHAR *cabinet, 
                                          const WCHAR *root)
{
    static HMODULE advpack;

    char *cab_path, *cab_file;

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
    EXTRACTdest exd;
    struct ExtractFileList fl;

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
    TRACE("Extracting %s to %s\n",debugstr_w(source), debugstr_w(path));
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
        "select * from Media where LastSequence >= %i order by LastSequence";
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
        TRACE("Source is CAB %s\n",debugstr_w(cab));
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
                /* extract the cab file into a folder in the temp folder */
                sz = MAX_PATH;
                if (MsiGetPropertyW(hPackage, cszTempFolder,path, &sz) 
                                    != ERROR_SUCCESS)
                    GetTempPathW(MAX_PATH,path);
            }
        }
        rc = !extract_cabinet_file(source,path);
    }
    MsiCloseHandle(row);
    MsiViewClose(view);
    MsiCloseHandle(view);
    return rc;
}

inline static UINT create_component_directory (MSIHANDLE hPackage, MSIPACKAGE*
                                               package, INT component)
{
    UINT rc;
    MSIFOLDER *folder;
    WCHAR install_path[MAX_PATH];

    rc = resolve_folder(hPackage, package->components[component].Directory,
                        install_path, FALSE, FALSE, &folder);

    if (rc != ERROR_SUCCESS)
        return rc; 

    /* create the path */
    if (folder->State == 0)
    {
        create_full_pathW(install_path);
        folder->State = 2;
    }

    return rc;
}

static UINT ACTION_InstallFiles(MSIHANDLE hPackage)
{
    UINT rc = ERROR_SUCCESS;
    INT index;
    MSIPACKAGE *package;

    package = msihandle2msiinfo(hPackage, MSIHANDLETYPE_PACKAGE);

    if (!package)
        return ERROR_INVALID_HANDLE;

    for (index = 0; index < package->loaded_files; index++)
    {
        WCHAR path_to_source[MAX_PATH];
        MSIFILE *file;
        
        file = &package->files[index];

        if (file->Temporary)
            continue;

        if (!package->components[file->ComponentIndex].Enabled ||
            !package->components[file->ComponentIndex].FeatureState)
        {
            TRACE("File %s is not scheduled for install\n",
                   debugstr_w(file->File));
            continue;
        }

        if ((file->State == 1) || (file->State == 2))
        {
            TRACE("Installing %s\n",debugstr_w(file->File));
            rc = ready_media_for_file(hPackage,file->Sequence,path_to_source);
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

            create_component_directory(hPackage, package, file->ComponentIndex);

            strcpyW(file->SourcePath, path_to_source);
            strcatW(file->SourcePath, file->File);

            TRACE("file paths %s to %s\n",debugstr_w(file->SourcePath),
                  debugstr_w(file->TargetPath));

            progress_message(hPackage,2,1,0,0);
            rc = !MoveFileW(file->SourcePath,file->TargetPath);
            if (rc)
                ERR("Unable to move file\n");
            else
                file->State = 4;
        }
    }

    return rc;
}

inline static UINT get_file_target(MSIHANDLE hPackage, LPCWSTR file_key, 
                                   LPWSTR file_source)
{
    MSIPACKAGE *package;
    INT index;

    package = msihandle2msiinfo(hPackage, MSIHANDLETYPE_PACKAGE);
    if (!package)
        return ERROR_INVALID_HANDLE;

    for (index = 0; index < package->loaded_files; index ++)
    {
        if (strcmpW(file_key,package->files[index].File)==0)
        {
            if (package->files[index].State >= 3)
            {
                strcmpW(file_source,package->files[index].TargetPath);
                return ERROR_SUCCESS;
            }
            else
                return ERROR_FILE_NOT_FOUND;
        }
    }

    return ERROR_FUNCTION_FAILED;
}

static UINT ACTION_DuplicateFiles(MSIHANDLE hPackage)
{
    UINT rc;
    MSIHANDLE view;
    MSIHANDLE row = 0;
    static const CHAR *ExecSeqQuery = "select * from DuplicateFile";
    MSIPACKAGE* package;

    package = msihandle2msiinfo(hPackage, MSIHANDLETYPE_PACKAGE);
    if (!package)
        return ERROR_INVALID_HANDLE;

    rc = MsiDatabaseOpenViewA(package->db, ExecSeqQuery, &view);

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
        WCHAR component[0x100];
        INT component_index;

        DWORD sz=0x100;

        rc = MsiViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            rc = ERROR_SUCCESS;
            break;
        }

        sz=0x100;
        rc = MsiRecordGetStringW(row,2,component,&sz);
        if (rc != ERROR_SUCCESS)
        {
            ERR("Unable to get component\n");
            MsiCloseHandle(row);
            break;
        }

        component_index = get_loaded_component(package,component);
        if (!package->components[component_index].Enabled ||
            !package->components[component_index].FeatureState)
        {
            TRACE("Skipping copy due to disabled component\n");
            MsiCloseHandle(row);
            continue;
        }

        sz=0x100;
        rc = MsiRecordGetStringW(row,3,file_key,&sz);
        if (rc != ERROR_SUCCESS)
        {
            ERR("Unable to get file key\n");
            MsiCloseHandle(row);
            break;
        }

        rc = get_file_target(hPackage,file_key,file_source);

        if (rc != ERROR_SUCCESS)
        {
            ERR("Original file unknown %s\n",debugstr_w(file_key));
            MsiCloseHandle(row);
            break;
        }

        if (MsiRecordIsNull(row,4))
        {
ERR("HERE target path %s\n",debugstr_w(file_source));
            strcpyW(dest_name,strrchrW(file_source,'\\')+1);
        }
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
            rc = resolve_folder(hPackage, destkey, dest_path,FALSE,FALSE,NULL);
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

        FIXME("We should track these duplicate files as well\n");   
 
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
    MSIPACKAGE *package;

    package = msihandle2msiinfo(hPackage, MSIHANDLETYPE_PACKAGE);
    if (!package)
        return ERROR_INVALID_HANDLE;

    rc = MsiDatabaseOpenViewA(package->db, ExecSeqQuery, &view);

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
        WCHAR component[0x100];
        INT component_index;

        INT   root;
        DWORD sz=0x100;

        rc = MsiViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            rc = ERROR_SUCCESS;
            break;
        }

        sz= 0x100;
        MsiRecordGetStringW(row,6,component,&sz);
        component_index = get_loaded_component(package,component);

        if (!package->components[component_index].Enabled ||
            !package->components[component_index].FeatureState)
        {
            TRACE("Skipping write due to disabled component\n");
            MsiCloseHandle(row);
            continue;
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
        progress_message(hPackage,2,1,0,0);

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

static UINT ACTION_InstallInitialize(MSIHANDLE hPackage)
{
    CHAR level[10000];
    INT install_level;
    DWORD sz;
    MSIPACKAGE *package; 
    INT i,j;
    /* I do not know if this is where it should happen.. but */

    TRACE("Checking Install Level\n");
    FIXME("The Attributes of the feature OVERRIDE THIS... unimplemented\n");
    FIXME("As does the ALLLOCAL and such propertys\n");

    sz = 10000;
    if (MsiGetPropertyA(hPackage,"INSTALLLEVEL",level,&sz)==ERROR_SUCCESS)
        install_level = atoi(level);
    else
        install_level = 1;
   
    package = msihandle2msiinfo(hPackage, MSIHANDLETYPE_PACKAGE);
    if (!package)
        return ERROR_INVALID_HANDLE;

    /*
     * components FeatureState defaults to FALSE. the idea is we want to 
     * enable the component is ANY feature that uses it is enabled to install
     */
    for(i = 0; i < package->loaded_features; i++)
    {
        BOOL feature_state= ((package->features[i].Level > 0) &&
                             (package->features[i].Level <= install_level));
        TRACE("Feature %s has a state of %i\n",
               debugstr_w(package->features[i].Feature), feature_state);
        for( j = 0; j < package->features[i].ComponentCount; j++)
        {
            package->components[package->features[i].Components[j]].FeatureState
            |= feature_state;
        }
    } 
    /* 
     * so basically we ONLY want to install a component if its Enabled AND
     * FeatureState are both TRUE 
     */
    return ERROR_SUCCESS;
}

static UINT ACTION_InstallValidate(MSIHANDLE hPackage)
{
    DWORD progress = 0;
    static const CHAR q1[]="SELECT * FROM Registry";
    static const CHAR q2[]=
"select Action from InstallExecuteSequence where Sequence > 0 order by Sequence";
    UINT rc;
    MSIHANDLE view;
    MSIHANDLE row = 0;
    MSIHANDLE db;
    BOOL flipit= FALSE;
    MSIPACKAGE* package;

    TRACE(" InstallValidate \n");

    db = MsiGetActiveDatabase(hPackage);
    rc = MsiDatabaseOpenViewA(db, q2, &view);
    rc = MsiViewExecute(view, 0);

    while (1)
    {
        rc = MsiViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            rc = ERROR_SUCCESS;
            break;
        }
        if (!flipit)
        {
            CHAR buf[0x100];
            DWORD sz=0x100;
            MsiRecordGetStringA(row,1,buf,&sz);
            if (strcmp(buf,"InstallValidate")==0)
                flipit=TRUE;
        }
        else
            progress +=25;

        MsiCloseHandle(row);
    }
    MsiViewClose(view);
    MsiCloseHandle(view);

    rc = MsiDatabaseOpenViewA(db, q1, &view);
    rc = MsiViewExecute(view, 0);
    while (1)
    {
        rc = MsiViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            rc = ERROR_SUCCESS;
            break;
        }
        progress +=1;

        MsiCloseHandle(row);
    }
    MsiViewClose(view);
    MsiCloseHandle(view);
    MsiCloseHandle(db);

    package = msihandle2msiinfo(hPackage, MSIHANDLETYPE_PACKAGE);
    progress_message(hPackage,0,progress+package->loaded_files,0,0);

    return ERROR_SUCCESS;
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
    WCHAR path[MAX_PATH];
    UINT rc;

    TRACE("(%s %p %li)\n",debugstr_w(szFolder),szPathBuf,*pcchPathBuf);

    rc = resolve_folder(hInstall, szFolder, path, FALSE, FALSE, NULL);

    if (rc == ERROR_SUCCESS && strlenW(path) > *pcchPathBuf)
    {
        *pcchPathBuf = strlenW(path)+1;
        return ERROR_MORE_DATA;
    }
    else if (rc == ERROR_SUCCESS)
    {
        *pcchPathBuf = strlenW(path)+1;
        strcpyW(szPathBuf,path);
        TRACE("Returning Path %s\n",debugstr_w(path));
    }
    
    return rc;
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
    WCHAR path[MAX_PATH];
    UINT rc;

    TRACE("(%s %p %li)\n",debugstr_w(szFolder),szPathBuf,*pcchPathBuf);
    rc = resolve_folder(hInstall, szFolder, path, TRUE, FALSE, NULL);

    if (rc == ERROR_SUCCESS && strlenW(path) > *pcchPathBuf)
    {
        *pcchPathBuf = strlenW(path)+1;
        return ERROR_MORE_DATA;
    }
    else if (rc == ERROR_SUCCESS)
    {
        *pcchPathBuf = strlenW(path)+1;
        strcpyW(szPathBuf,path);
        TRACE("Returning Path %s\n",debugstr_w(path));
    }
    
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
    MSIPACKAGE *package;
    INT i;
    WCHAR path[MAX_PATH];
    MSIFOLDER *folder;

    TRACE("(%s %s)\n",debugstr_w(szFolder),debugstr_w(szFolderPath));

    if (szFolderPath[0]==0)
        return ERROR_FUNCTION_FAILED;

    if (GetFileAttributesW(szFolderPath) == INVALID_FILE_ATTRIBUTES)
        return ERROR_FUNCTION_FAILED;

    package = msihandle2msiinfo(hInstall, MSIHANDLETYPE_PACKAGE);

    if (package==NULL)
        return ERROR_INVALID_HANDLE;

    resolve_folder(hInstall,szFolder,path,FALSE,FALSE,&folder);

    if (!folder)
        return ERROR_INVALID_PARAMETER;

    strcpyW(folder->Property,szFolderPath);

    for (i = 0; i < package->loaded_folders; i++)
        package->folders[i].ResolvedTarget[0]=0;

    for (i = 0; i < package->loaded_folders; i++)
        resolve_folder(hInstall, package->folders[i].Directory, path, FALSE,
                       TRUE, NULL);

    return ERROR_SUCCESS;
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
