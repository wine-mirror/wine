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
 * pages i need
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

/*
 * These are hacks to get around the inability to write
 * to the database and the inability to select on string values
 * once those values are done then it will be possible to do
 * all this inside the database.
 */

#define MAX_PROP 1024

typedef struct {
    WCHAR *prop_name;
    WCHAR *prop_value;
    } internal_property;

static internal_property PropTableHack[MAX_PROP];
static INT PropCount = -1;

WINE_DEFAULT_DEBUG_CHANNEL(msi);

/*
 * Prototypes
 */
UINT ACTION_PerformAction(MSIHANDLE hPackage, const WCHAR *action);
static UINT ACTION_CostInitialize(MSIHANDLE hPackage);
static UINT ACTION_CreateFolders(MSIHANDLE hPackage);
static UINT ACTION_CostFinalize(MSIHANDLE hPackage);
static UINT ACTION_InstallFiles(MSIHANDLE hPackage);
static UINT ACTION_DuplicateFiles(MSIHANDLE hPackage);
static UINT ACTION_WriteRegistryValues(MSIHANDLE hPackage);
static UINT ACTION_CustomAction(MSIHANDLE hPackage,const WCHAR *action);

static UINT set_property(MSIHANDLE hPackage, const WCHAR* prop, 
                          const WCHAR* value);
UINT get_property(MSIHANDLE hPackage, const WCHAR* prop, WCHAR* value, 
                  DWORD* size);
static VOID blitz_propertytable();
static VOID set_installer_properties(MSIHANDLE hPackage);
static DWORD deformat_string(MSIHANDLE hPackage, WCHAR* ptr,WCHAR** data);

/*
 * consts and values used
 */
static const WCHAR cszSourceDir[] = {'S','o','u','r','c','e','D','i','r',0};
static const WCHAR cszRootDrive[] = {'R','O','O','T','D','R','I','V','E',0};
static const WCHAR cszTargetDir[] = {'T','A','R','G','E','T','D','I','R',0};

static const WCHAR cszlsb[]={'[',0};
static const WCHAR cszrsb[]={']',0};
static const WCHAR cszbs[]={'\\',0};


/******************************************************** 
 * helper functions to get around current HACKS and such
 ********************************************************/

static VOID blitz_propertytable()
{
    if (PropCount == -1)
    {
        PropCount = 0;
        memset(&PropTableHack,0,sizeof(PropTableHack));
    }
    else if (PropCount > 0)
    {
        int i;
        TRACE("Clearing %i properties\n",PropCount);
        for (i = 0; i < PropCount; i++)
        {
            HeapFree(GetProcessHeap(), 0, PropTableHack[i].prop_name);
            HeapFree(GetProcessHeap(), 0, PropTableHack[i].prop_value);
        }
        memset(&PropTableHack,0,sizeof(PropTableHack));
        PropCount = 0;
    }
}

UINT get_property(MSIHANDLE hPackage, const WCHAR* prop, WCHAR* value, 
                  DWORD* size)
{
    UINT rc = 1;
    int index = 0;
    WCHAR* pName = PropTableHack[0].prop_name;

    TRACE("Looking for property %s\n",debugstr_w(prop));

    /* prop table hacks take presidence */

    while (pName && strcmpW(pName,prop) && index < PropCount)
    {
        index ++;
        pName = PropTableHack[index].prop_name;
    }

    if (pName && index < PropCount)
    {
        if (*size > strlenW(PropTableHack[index].prop_value))
        {
            *size = strlenW(PropTableHack[index].prop_value)+1;
            TRACE("    index %i\n", index);
            strcpyW(value , PropTableHack[index].prop_value);
            TRACE("    found value %s\n",debugstr_w(value));
            return 0;
        }
        else
        {
            *size = strlenW(PropTableHack[index].prop_value);
            return ERROR_MORE_DATA;
        }
    }

    rc = MsiGetPropertyW(hPackage,prop,value,size);

    if (rc == ERROR_SUCCESS)
        TRACE("    found value %s\n",debugstr_w(value));
    else
        TRACE("    value not found\n");

    return rc;
}

static UINT set_property(MSIHANDLE hPackage, const WCHAR* prop, 
                          const WCHAR* value)
{
    /* prop table hacks take precedence */
    UINT rc;
    int index = 0;
    WCHAR* pName;

    if (PropCount == -1)
        blitz_propertytable();

    pName = PropTableHack[0].prop_name;

    TRACE("Setting property %s to %s\n",debugstr_w(prop),debugstr_w(value));

    while (pName  && strcmpW(pName,prop) &&  index < MAX_PROP)
    {
        index ++;
        pName = PropTableHack[index].prop_name;
    }

    if (pName && index < MAX_PROP)
    {
        TRACE("property index %i\n",index);
        strcpyW(PropTableHack[index].prop_value,value);
        return 0;
    }
    else
    {
        if (index >= MAX_PROP)
        {
            ERR("EXCEEDING MAX PROP!!!!\n");
            return ERROR_FUNCTION_FAILED;
        }
        PropTableHack[index].prop_name = HeapAlloc(GetProcessHeap(),0,1024);
        PropTableHack[index].prop_value= HeapAlloc(GetProcessHeap(),0,1024);
        strcpyW(PropTableHack[index].prop_name,prop);
        strcpyW(PropTableHack[index].prop_value,value);
        PropCount++;
        TRACE("new property index %i (%i)\n",index,PropCount);
        return 0;
    }

    /* currently unreachable */
    rc = MsiSetPropertyW(hPackage,prop,value);
    return rc;
}

/*
 * There are a whole slew of these we need to set
 *
 *
http://msdn.microsoft.com/library/default.asp?url=/library/en-us/msi/setup/properties.asp
 */

static VOID set_installer_properties(MSIHANDLE hPackage)
{
    WCHAR pth[MAX_PATH];

    static const WCHAR c_col[] = 
{'C',':','\\',0};
    static const WCHAR CFF[] = 
{'C','o','m','m','o','n','F','i','l','e','s','F','o','l','d','e','r',0};
    static const WCHAR PFF[] = 
{'P','r','o','g','r','a','m','F','i','l','e','s','F','o','l','d','e','r',0};
    static const WCHAR CADF[] = 
{'C','o','m','m','o','n','A','p','p','D','a','t','a','F','o','l','d','e','r',0};
    static const WCHAR ATF[] = 
{'A','d','m','i','n','T','o','o','l','s','F','o','l','d','e','r',0};
    static const WCHAR ADF[] = 
{'A','p','p','D','a','t','a','F','o','l','d','e','r',0};
    static const WCHAR SF[] = 
{'S','y','s','t','e','m','F','o','l','d','e','r',0};
    static const WCHAR LADF[] = 
{'L','o','c','a','l','A','p','p','D','a','t','a','F','o','l','d','e','r',0};
    static const WCHAR MPF[] = 
{'M','y','P','i','c','t','u','r','e','s','F','o','l','d','e','r',0};
    static const WCHAR PF[] = 
{'P','e','r','s','o','n','a','l','F','o','l','d','e','r',0};
    static const WCHAR WF[] = 
{'W','i','n','d','o','w','s','F','o','l','d','e','r',0};


/* Not yet set ...  but needed by iTunes
 *
    static const WCHAR DF[] = 
{'D','e','s','k','t','o','p','F','o','l','d','e','r',0};
    static const WCHAR FF[] = 
{'F','a','v','o','r','i','t','e','s','F','o','l','d','e','r',0};
    static const WCHAR FoF[] = 
{'F','o','n','t','s','F','o','l','d','e','r',0};
PrimaryVolumePath
ProgramFiles64Folder
ProgramMenuFolder
SendToFolder
StartMenuFolder
StartupFolder
System16Folder
System64Folder
TempFolder
TemplateFolder
 */

/* asked for by iTunes ... but are they installer set? 
 *
 *  GlobalAssemblyCache
 */

    set_property(hPackage, cszRootDrive, c_col);

    SHGetFolderPathW(NULL,CSIDL_PROGRAM_FILES_COMMON,NULL,0,pth);
    strcatW(pth,cszbs);
    set_property(hPackage, CFF, pth);

    SHGetFolderPathW(NULL,CSIDL_PROGRAM_FILES,NULL,0,pth);
    strcatW(pth,cszbs);
    set_property(hPackage, PFF, pth);

    SHGetFolderPathW(NULL,CSIDL_COMMON_APPDATA,NULL,0,pth);
    strcatW(pth,cszbs);
    set_property(hPackage, CADF, pth);

    SHGetFolderPathW(NULL,CSIDL_ADMINTOOLS,NULL,0,pth);
    strcatW(pth,cszbs);
    set_property(hPackage, ATF, pth);

    SHGetFolderPathW(NULL,CSIDL_APPDATA,NULL,0,pth);
    strcatW(pth,cszbs);
    set_property(hPackage, ADF, pth);

    SHGetFolderPathW(NULL,CSIDL_SYSTEM,NULL,0,pth);
    strcatW(pth,cszbs);
    set_property(hPackage, SF, pth);

    SHGetFolderPathW(NULL,CSIDL_LOCAL_APPDATA,NULL,0,pth);
    strcatW(pth,cszbs);
    set_property(hPackage, LADF, pth);

    SHGetFolderPathW(NULL,CSIDL_MYPICTURES,NULL,0,pth);
    strcatW(pth,cszbs);
    set_property(hPackage, MPF, pth);

    SHGetFolderPathW(NULL,CSIDL_PERSONAL,NULL,0,pth);
    strcatW(pth,cszbs);
    set_property(hPackage, PF, pth);

    SHGetFolderPathW(NULL,CSIDL_WINDOWS,NULL,0,pth);
    strcatW(pth,cszbs);
    set_property(hPackage, WF, pth);
}


/****************************************************
 * TOP level entry points 
 *****************************************************/

UINT ACTION_DoTopLevelINSTALL(MSIHANDLE hPackage, LPCWSTR szPackagePath,
                              LPCWSTR szCommandLine)
{
    MSIHANDLE view;
    UINT rc;
    static const CHAR *ExecSeqQuery = 
"select * from InstallExecuteSequence where Sequence > 0 order by Sequence";

    FIXME("****We do not do any of the UI level stuff yet***\n");

    /* reset our properties */
    blitz_propertytable();

    if (szPackagePath)   
    {
        static const WCHAR OriginalDatabase[] =
{'O','r','i','g','i','n','a','l','D','a','t','a','b','a','s','e',0};
        LPWSTR p;
        WCHAR check[MAX_PATH];
        WCHAR pth[MAX_PATH];
        DWORD size;
 
        set_property(hPackage, OriginalDatabase, szPackagePath);

        strcpyW(pth,szPackagePath);
        p = strrchrW(pth,'\\');    
        if (p)
        {
            p++;
            *p=0;
        }

        size = MAX_PATH;
        if (get_property(hPackage,cszSourceDir,check,&size) != ERROR_SUCCESS )
            set_property(hPackage, cszSourceDir, pth);
    }

    rc = MsiDatabaseOpenViewA(hPackage, ExecSeqQuery, &view);
    
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
    blitz_propertytable();
    return rc;
}


/********************************************************
 * ACTION helper functions and functions that perform the actions
 *******************************************************/

/* 
 * Alot of actions are really important even if they don't do anything
 * explicit.. Lots of properties are set at the beginning of the installation
 * CostFinalize does a bunch of work to translated the directorys and such
 * 
 * But until I get write access to the database that is hard. so I am going to
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
     .
     .
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

    strcatW(ExecSeqQuery,action);
    strcatW(ExecSeqQuery,end);
    rc = MsiDatabaseOpenViewW(hPackage, ExecSeqQuery, &view);
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

    TRACE("Handling custom action %s\n",debugstr_w(action));
    type = MsiRecordGetInteger(row,2);

    switch (type & CUSTOM_ACTION_TYPE_MASK)
    {
        case 35: /* Directory set with formatted text. */
        case 51: /* Property set with formatted text. */
            sz=0x100;
            MsiRecordGetStringW(row,3,source,&sz);
            sz=0x200;
            MsiRecordGetStringW(row,4,target,&sz);
            deformat_string(hPackage,target,&deformated);
            set_property(hPackage,source,deformated);
            HeapFree(GetProcessHeap(),0,deformated);
            break;
        default:
            ERR("UNHANDLED ACTION TYPE %i\n",type & CUSTOM_ACTION_TYPE_MASK);
    }

    MsiCloseHandle(row);
    MsiViewClose(view);
    MsiCloseHandle(view);
    return rc;
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
 * to do all the directorys for all the components.
 */
static UINT ACTION_CreateFolders(MSIHANDLE hPackage)
{
    static const CHAR *ExecSeqQuery = "select * from CreateFolder";
    UINT rc;
    MSIHANDLE view;

    rc = MsiDatabaseOpenViewA(hPackage, ExecSeqQuery, &view);
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
        rc = get_property(hPackage, dir,full_path,&sz);

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

    sz = MAX_PATH; 
    if (get_property(hPackage,dir,path,&sz)==ERROR_SUCCESS)
        return ERROR_SUCCESS;

    TRACE("Working to resolve %s\n",debugstr_w(dir));

    /* special case... root drive */       
    if (strcmpW(dir,cszTargetDir)==0)
    {
        if (!source)
        {
            sz = 0x100;
            if(!get_property(hPackage,cszRootDrive,buffer,&sz))
            {
                set_property(hPackage,cszTargetDir,buffer);
                strcpyW(path,buffer);
            }
            else
            {
                ERR("No RootDrive property defined disaster!\n");
                MsiCloseHandle(row);
                MsiViewClose(view);
                MsiCloseHandle(view);
                return ERROR_FUNCTION_FAILED;
            }
        }
        else
            strcpyW(path,cszsrcroot);

        return ERROR_SUCCESS;
    }

    strcatW(Query,dir);
    strcatW(Query,end);
    rc = MsiDatabaseOpenViewW(hPackage, Query, &view);
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
        set_property(hPackage,dir,full_path);
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
        set_property(hPackage,name_source,full_path);
        if (source)
            strcpyW(path,full_path);
    }

    MsiCloseHandle(row);
    MsiViewClose(view);
    MsiCloseHandle(view);
    return rc;
}

/*
 * This is the action where all the features and components are loaded into
 * memory... so when we start doing that that will be important.
 * 
 */
static UINT ACTION_CostInitialize(MSIHANDLE hPackage)
{
    return ERROR_SUCCESS;
}

/* 
 * Alot is done in this function aside from just the costing.
 * The costing needs to be implemented at some point but for now i am going
 * to focus on the directory building
 *
 * Note about directory names: I know that directory names get processed into 
 * properties.  I am still very unclear where the name_source
 * part is used but I am preserving it just as a precaution
 */
static UINT ACTION_CostFinalize(MSIHANDLE hPackage)
{
    static const CHAR *ExecSeqQuery = "select * from Directory";
    UINT rc;
    MSIHANDLE view;

    /* According to MSDN these properties are set when CostFinalize is run  
     * or MsiSetInstallLevel is called */
    TRACE("Setting installer properties\n");
    set_installer_properties(hPackage);    

    TRACE("Building Directory properties\n");

    rc = MsiDatabaseOpenViewA(hPackage, ExecSeqQuery, &view);
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

    rc = read_raw_stream_data(hPackage,stream_name,&data,&size); 

    if (rc != ERROR_SUCCESS)
        return rc;

    write = 0x100;
    if (get_property(hPackage, cszSourceDir, source, &write))
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

/***********************************************************************
 *            extract_cabinet_file
 *
 * Extract a file from a .cab file.
 */
static BOOL extract_cabinet_file( const WCHAR *cabinet, const WCHAR *root)
                                  
{
    static const WCHAR extW[] = {'.','c','a','b',0};

    /* from cabinet.h */
    typedef struct {
        long  result1;          /* 0x000 */
        long  unknown1[3];      /* 0x004 */
        void* filelist;         /* 0x010 */
        long  filecount;        /* 0x014 */
        long  unknown2;         /* 0x018 */
        char  directory[0x104]; /* 0x01c */
        char  lastfile[0x20c];  /* 0x120 */
    } EXTRACTdest;
    extern HRESULT Extract(EXTRACTdest*, LPCSTR);

    char *cab_path, *src_path;
    int len = strlenW( cabinet );
    EXTRACTdest exd;

    /* make sure the cabinet file has a .cab extension */
    if (len <= 4 || strcmpiW( cabinet + len - 4, extW )) return FALSE;

    if (!(cab_path = strdupWtoA( cabinet ))) return FALSE;
    if (!(src_path = strdupWtoA( root ))) return FALSE;

    memset(&exd,0,sizeof(exd));
    strcpy(exd.directory,src_path);
    Extract(&exd,cab_path);
    HeapFree( GetProcessHeap(), 0, cab_path );
    HeapFree( GetProcessHeap(), 0, src_path );
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

    if (sequence <= last_sequence)
    {
        TRACE("Media already ready (%i, %i)\n",sequence,last_sequence);
        return ERROR_SUCCESS;
    }

    sprintf(Query,ExecSeqQuery,sequence);

    rc = MsiDatabaseOpenViewA(hPackage, Query, &view);
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
            if (get_property(hPackage, cszSourceDir, source, &sz))
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

    strcatW(ExecSeqQuery,component);
    strcatW(ExecSeqQuery,end);

    rc = MsiDatabaseOpenViewW(hPackage, ExecSeqQuery, &view);

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
    rc = get_property(hPackage, dir, install_path, &sz);

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

    /* REALLY what we want to do is go through all the enabled
     * features and check all the components of that feature and
     * make sure that component is not already install and blah
     * blah blah... I will do it that way some day.. really
     * but for sheer gratification I am going to just brute force
     * install all the files
     */

    rc = MsiDatabaseOpenViewA(hPackage, ExecSeqQuery, &view);
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
        strcatW(install_path,filename);

        TRACE("Installing file %s to %s\n",debugstr_w(src_path),
              debugstr_w(install_path));

        rc = !MoveFileW(src_path,install_path);
        if (rc)
        {
            ERR("Unable to move file\n");
        }

        /* for future use lets keep track of this file and where it went */
        set_property(hPackage,sourcename,install_path);

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


    /*
     * Yes we should only do this for componenets that are installed
     * but again I need to do that went I track components.
     */

    rc = MsiDatabaseOpenViewA(hPackage, ExecSeqQuery, &view);
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
        rc = get_property(hPackage,file_key,file_source,&sz);
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
            rc = get_property(hPackage, destkey, dest_path, &sz);
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


static LPSTR parse_value(MSIHANDLE hPackage, WCHAR *value, DWORD *type, 
                         DWORD *size)
{
    LPSTR data = NULL;
    if (value[0]=='#' && value[1]!='#')
    {
        ERR("UNHANDLED VALUE TYPE\n"); 
    }
    else
    {
        WCHAR *ptr;
        if (value[0]=='#')
            ptr = &value[1];
        else
            ptr=value;

        *type=REG_SZ;
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

    /* Again here we want to key off of the components being installed...
     * oh well
     */
    
    rc = MsiDatabaseOpenViewA(hPackage, ExecSeqQuery, &view);
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
        WCHAR value[0x100];
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
   
        sz=0x100; 
        MsiRecordGetStringW(row,5,value,&sz);


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
      
        value_data = parse_value(hPackage, value, &type, &size); 

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
    if (get_property(hPackage, key, value,&sz) == ERROR_SUCCESS)
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

    /* recursivly do this to clean up */
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
    return get_property(hInstall,szFolder,szPathBuf,pcchPathBuf);
}


#if 0
static UINT ACTION_Template(MSIHANDLE hPackage)
{
    UINT rc;
    MSIHANDLE view;
    MSIHANDLE row = 0;
    static const CHAR *ExecSeqQuery;

    rc = MsiDatabaseOpenViewA(hPackage, ExecSeqQuery, &view);
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
