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

#define NONAMELESSUNION

#include <stdarg.h>
#include <stdio.h>
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winnls.h"
#include "shlwapi.h"
#include "wine/debug.h"
#include "msi.h"
#include "msiquery.h"
#include "msipriv.h"
#include "objidl.h"
#include "wincrypt.h"
#include "winuser.h"
#include "shlobj.h"
#include "wine/unicode.h"
#include "objbase.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

/*
 * The MSVC headers define the MSIDBOPEN_* macros cast to LPCTSTR,
 *  which is a problem because LPCTSTR isn't defined when compiling wine.
 * To work around this problem, we need to define LPCTSTR as LPCWSTR here,
 *  and make sure to only use it in W functions.
 */
#define LPCTSTR LPCWSTR

void MSI_FreePackage( VOID *arg);

typedef struct tagMSIPACKAGE
{
    MSIHANDLE db;
} MSIPACKAGE;

void MSI_FreePackage( VOID *arg)
{
    MSIPACKAGE *package= arg;

    MsiCloseHandle(package->db);
}

UINT WINAPI MsiOpenPackageA(LPCSTR szPackage, MSIHANDLE *phPackage)
{
    LPWSTR szwPack = NULL;
    UINT len, ret;

    TRACE("%s %p\n",debugstr_a(szPackage), phPackage);

    if( szPackage )
    {
        len = MultiByteToWideChar( CP_ACP, 0, szPackage, -1, NULL, 0 );
        szwPack = HeapAlloc( GetProcessHeap(), 0, len * sizeof (WCHAR) );
        if( szwPack )
            MultiByteToWideChar( CP_ACP, 0, szPackage, -1, szwPack, len );
    }

    ret = MsiOpenPackageW( szwPack, phPackage );

    if( szwPack )
        HeapFree( GetProcessHeap(), 0, szwPack );

    return ret;
}


static const void clone_properties(MSIHANDLE db)
{
    MSIHANDLE view;
    UINT rc;
    static const CHAR CreateSql[] = "CREATE TABLE `_Property` ( `_Property` "
"CHAR(56) NOT NULL, `Value` CHAR(98) NOT NULL PRIMARY KEY `_Property`)";
    static const CHAR Query[] = "SELECT * from Property";
    static const CHAR Insert[] = 
      "INSERT into `_Property` (`_Property`,`Value`) VALUES (?)";

    /* create the temporary properties table */
    MsiDatabaseOpenViewA(db, CreateSql, &view);
    MsiViewExecute(view,0);   
    MsiViewClose(view);
    MsiCloseHandle(view); 

    /* clone the existing properties */
    MsiDatabaseOpenViewA(db, Query, &view);

    MsiViewExecute(view, 0);
    while (1)
    {
        MSIHANDLE row;
        MSIHANDLE view2;

        rc = MsiViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
            break;

        MsiDatabaseOpenViewA(db,Insert,&view2);  
        MsiViewExecute(view2,row);
        MsiViewClose(view2);
        MsiCloseHandle(view2);
 
        MsiCloseHandle(row); 
    }
    MsiViewClose(view);
    MsiCloseHandle(view);
    
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

    static const WCHAR cszbs[]={'\\',0};
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
    static const WCHAR TF[]=
{'T','e','m','p','F','o','l','d','e','r',0};

/* Not yet set ...  but needed by iTunes
 *
DesktopFolder
FavoritesFolder
FontsFolder
PrimaryVolumePath
ProgramFiles64Folder
ProgramMenuFolder
SendToFolder
StartMenuFolder
StartupFolder
System16Folder
System64Folder
TemplateFolder
 */

/* asked for by iTunes ... but are they installer set? 
 *
 *  GlobalAssemblyCache
 */

/*
 * Other things i notice set
 *
ScreenY
ScreenX
SystemLanguageID
ComputerName
UserLanguageID
LogonUser
VirtualMemory
PhysicalMemory
Intel
ShellAdvSupport
ServicePackLevel
WindowsBuild
Version9x
Version95
VersionNT
AdminUser
DefaultUIFont
VersionMsi
VersionDatabase
PackagecodeChanging
ProductState
CaptionHeight
BorderTop
BorderSide
TextHeight
ColorBits
RedirectedDllSupport
Time
Date
Privilaged
*/

    SHGetFolderPathW(NULL,CSIDL_PROGRAM_FILES_COMMON,NULL,0,pth);
    strcatW(pth,cszbs);
    MsiSetPropertyW(hPackage, CFF, pth);

    SHGetFolderPathW(NULL,CSIDL_PROGRAM_FILES,NULL,0,pth);
    strcatW(pth,cszbs);
    MsiSetPropertyW(hPackage, PFF, pth);

    SHGetFolderPathW(NULL,CSIDL_COMMON_APPDATA,NULL,0,pth);
    strcatW(pth,cszbs);
    MsiSetPropertyW(hPackage, CADF, pth);

    SHGetFolderPathW(NULL,CSIDL_ADMINTOOLS,NULL,0,pth);
    strcatW(pth,cszbs);
    MsiSetPropertyW(hPackage, ATF, pth);

    SHGetFolderPathW(NULL,CSIDL_APPDATA,NULL,0,pth);
    strcatW(pth,cszbs);
    MsiSetPropertyW(hPackage, ADF, pth);

    SHGetFolderPathW(NULL,CSIDL_SYSTEM,NULL,0,pth);
    strcatW(pth,cszbs);
    MsiSetPropertyW(hPackage, SF, pth);

    SHGetFolderPathW(NULL,CSIDL_LOCAL_APPDATA,NULL,0,pth);
    strcatW(pth,cszbs);
    MsiSetPropertyW(hPackage, LADF, pth);

    SHGetFolderPathW(NULL,CSIDL_MYPICTURES,NULL,0,pth);
    strcatW(pth,cszbs);
    MsiSetPropertyW(hPackage, MPF, pth);

    SHGetFolderPathW(NULL,CSIDL_PERSONAL,NULL,0,pth);
    strcatW(pth,cszbs);
    MsiSetPropertyW(hPackage, PF, pth);

    SHGetFolderPathW(NULL,CSIDL_WINDOWS,NULL,0,pth);
    strcatW(pth,cszbs);
    MsiSetPropertyW(hPackage, WF, pth);

    GetTempPathW(MAX_PATH,pth);
    MsiSetPropertyW(hPackage, TF, pth);
}


UINT WINAPI MsiOpenPackageW(LPCWSTR szPackage, MSIHANDLE *phPackage)
{
    UINT rc;
    MSIHANDLE handle;
    MSIHANDLE db;
    MSIPACKAGE *package;
    CHAR uilevel[10];

    static const WCHAR OriginalDatabase[] =
{'O','r','i','g','i','n','a','l','D','a','t','a','b','a','s','e',0};
    static const WCHAR Database[] =
{'D','A','T','A','B','A','S','E',0};

    TRACE("%s %p\n",debugstr_w(szPackage), phPackage);

    rc = MsiOpenDatabaseW(szPackage, MSIDBOPEN_READONLY, &db);

    if (rc != ERROR_SUCCESS)
        return ERROR_FUNCTION_FAILED;

    handle = alloc_msihandle(MSIHANDLETYPE_PACKAGE, sizeof (MSIPACKAGE),
                             MSI_FreePackage, (void**)&package);

    if (!handle)
    {
        MsiCloseHandle(db);
        return ERROR_FUNCTION_FAILED;
    }

    package->db = db;

    /* ok here is where we do a slew of things to the database to 
     * prep for all that is to come as a package */

    clone_properties(db);
    set_installer_properties(handle);
    MsiSetPropertyW(handle, OriginalDatabase, szPackage);
    MsiSetPropertyW(handle, Database, szPackage);
    sprintf(uilevel,"%i",gUILevel);
    MsiSetPropertyA(handle, "UILevel", uilevel);

    *phPackage = handle;

    return ERROR_SUCCESS;
}

UINT WINAPI MsiOpenPackageExA(LPCSTR szPackage, DWORD dwOptions, MSIHANDLE *phPackage)
{
    FIXME("%s 0x%08lx %p\n",debugstr_a(szPackage), dwOptions, phPackage);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiOpenPackageExW(LPCWSTR szPackage, DWORD dwOptions, MSIHANDLE *phPackage)
{
    FIXME("%s 0x%08lx %p\n",debugstr_w(szPackage), dwOptions, phPackage);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

MSIHANDLE WINAPI MsiGetActiveDatabase(MSIHANDLE hInstall)
{
    MSIPACKAGE *package;

    TRACE("(%ld)\n",hInstall);

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE);

    if( !package)
        return ERROR_INVALID_HANDLE;

    msihandle_addref(package->db);
    return package->db;
}

INT WINAPI MsiProcessMessage( MSIHANDLE hInstall, INSTALLMESSAGE eMessageType,
                              MSIHANDLE hRecord)
{
    DWORD log_type = 0;
    LPSTR message;
    DWORD sz;
    INT msg_field=1;
    FIXME("STUB: %x \n",eMessageType);

    if ((eMessageType & 0xff000000) == INSTALLMESSAGE_ERROR)
        log_type |= INSTALLLOGMODE_ERROR;
    if ((eMessageType & 0xff000000) == INSTALLMESSAGE_WARNING)
        log_type |= INSTALLLOGMODE_WARNING;
    if ((eMessageType & 0xff000000) == INSTALLMESSAGE_USER)
        log_type |= INSTALLLOGMODE_USER;
    if ((eMessageType & 0xff000000) == INSTALLMESSAGE_INFO)
        log_type |= INSTALLLOGMODE_INFO;
    if ((eMessageType & 0xff000000) == INSTALLMESSAGE_COMMONDATA)
        log_type |= INSTALLLOGMODE_COMMONDATA;
    if ((eMessageType & 0xff000000) == INSTALLMESSAGE_ACTIONSTART)
    {
        log_type |= INSTALLLOGMODE_ACTIONSTART;
        msg_field = 2;
    }
    if ((eMessageType & 0xff000000) == INSTALLMESSAGE_ACTIONDATA)
        log_type |= INSTALLLOGMODE_ACTIONDATA;

    sz = 0;
    MsiRecordGetStringA(hRecord,msg_field,NULL,&sz);
    sz++;
    message = HeapAlloc(GetProcessHeap(),0,sz);
    MsiRecordGetStringA(hRecord,msg_field,message,&sz);

    TRACE("(%p %lx %lx)\n",gUIHandler, gUIFilter, log_type);
    if (gUIHandler && (gUIFilter & log_type))
        gUIHandler(gUIContext,eMessageType,message);
    else
        TRACE("%s\n",debugstr_a(message));

    HeapFree(GetProcessHeap(),0,message);
    return ERROR_SUCCESS;
}

/* property code */
UINT WINAPI MsiSetPropertyA( MSIHANDLE hInstall, LPCSTR szName, LPCSTR szValue)
{
    LPWSTR szwName = NULL, szwValue = NULL;
    UINT hr = ERROR_INSTALL_FAILURE;
    UINT len;

    if (0 == hInstall) {
      return ERROR_INVALID_HANDLE;
    }
    if (NULL == szName) {
      return ERROR_INVALID_PARAMETER;
    }
    if (NULL == szValue) {
      return ERROR_INVALID_PARAMETER;
    }

    len = MultiByteToWideChar( CP_ACP, 0, szName, -1, NULL, 0 );
    szwName = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
    if( !szwName )
        goto end;
    MultiByteToWideChar( CP_ACP, 0, szName, -1, szwName, len );

    len = MultiByteToWideChar( CP_ACP, 0, szValue, -1, NULL, 0 );
    szwValue = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
    if( !szwValue)
        goto end;
    MultiByteToWideChar( CP_ACP, 0, szValue , -1, szwValue, len );

    hr = MsiSetPropertyW( hInstall, szwName, szwValue);

end:
    if( szwName )
        HeapFree( GetProcessHeap(), 0, szwName );
    if( szwValue )
        HeapFree( GetProcessHeap(), 0, szwValue );

    return hr;
}

UINT WINAPI MsiSetPropertyW( MSIHANDLE hInstall, LPCWSTR szName, LPCWSTR szValue)
{
    MSIPACKAGE *package;
    MSIHANDLE view,row;
    UINT rc;
    DWORD sz = 0;
    static const WCHAR Insert[]=
     {'I','N','S','E','R','T',' ','i','n','t','o',' ','`','_','P','r','o','p'
,'e','r','t','y','`',' ','(','`','_','P','r','o','p','e','r','t','y','`'
,',','`','V','a','l','u','e','`',')',' ','V','A','L','U','E','S'
,' ','(','?',')',0};
    static const WCHAR Update[]=
     {'U','P','D','A','T','E',' ','_','P','r','o','p','e'
,'r','t','y',' ','s','e','t',' ','`','V','a','l','u','e','`',' ','='
,' ','?',' ','w','h','e','r','e',' ','`','_','P','r','o','p'
,'e','r','t','y','`',' ','=',' ','\'','%','s','\'',0};
    WCHAR Query[1024];

    TRACE("Setting property (%s %s)\n",debugstr_w(szName),
          debugstr_w(szValue));

    if (!hInstall)
        return ERROR_INVALID_HANDLE;

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE);
    if( !package)
        return ERROR_INVALID_HANDLE;

    rc = MsiGetPropertyW(hInstall,szName,0,&sz);
    if (rc==ERROR_MORE_DATA || rc == ERROR_SUCCESS)
    {
        sprintfW(Query,Update,szName);

        row = MsiCreateRecord(1);
        MsiRecordSetStringW(row,1,szValue);

    }
    else
    {
       strcpyW(Query,Insert);

        row = MsiCreateRecord(2);
        MsiRecordSetStringW(row,1,szName);
        MsiRecordSetStringW(row,2,szValue);
    }


    rc = MsiDatabaseOpenViewW(package->db,Query,&view);
    if (rc!= ERROR_SUCCESS)
    {
        MsiCloseHandle(row);
        return rc;
    }

    rc = MsiViewExecute(view,row);

    MsiCloseHandle(row);
    MsiViewClose(view);
    MsiCloseHandle(view);

    return rc;
}

UINT WINAPI MsiGetPropertyA(MSIHANDLE hInstall, LPCSTR szName, LPSTR szValueBuf, DWORD* pchValueBuf) 
{
    LPWSTR szwName = NULL, szwValueBuf = NULL;
    UINT hr = ERROR_INSTALL_FAILURE;

    if (0 == hInstall) {
      return ERROR_INVALID_HANDLE;
    }
    if (NULL == szName) {
      return ERROR_INVALID_PARAMETER;
    }

    TRACE("%lu %s %lu\n", hInstall, debugstr_a(szName), *pchValueBuf);

    if (NULL != szValueBuf && NULL == pchValueBuf) {
      return ERROR_INVALID_PARAMETER;
    }
    if( szName )
    {
        UINT len = MultiByteToWideChar( CP_ACP, 0, szName, -1, NULL, 0 );
        szwName = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        if( !szwName )
            goto end;
        MultiByteToWideChar( CP_ACP, 0, szName, -1, szwName, len );
    } else {
      return ERROR_INVALID_PARAMETER;
    }
    if( szValueBuf )
    {
        szwValueBuf = HeapAlloc( GetProcessHeap(), 0, (*pchValueBuf) * sizeof(WCHAR) );
        if( !szwValueBuf )	 
            goto end;
    }

    hr = MsiGetPropertyW( hInstall, szwName, szwValueBuf, pchValueBuf );

    if(  *pchValueBuf > 0 )
    {
        WideCharToMultiByte(CP_ACP, 0, szwValueBuf, -1, szValueBuf, *pchValueBuf, NULL, NULL);
    }

end:
    if( szwName )
        HeapFree( GetProcessHeap(), 0, szwName );
    if( szwValueBuf )
        HeapFree( GetProcessHeap(), 0, szwValueBuf );

    return hr;
}

UINT WINAPI MsiGetPropertyW(MSIHANDLE hInstall, LPCWSTR szName, 
                           LPWSTR szValueBuf, DWORD* pchValueBuf)
{
    MSIHANDLE view,row;
    UINT rc;
    WCHAR Query[1024]=
    {'s','e','l','e','c','t',' ','V','a','l','u','e',' ','f','r','o','m',' '
     ,'_','P','r','o','p','e','r','t','y',' ','w','h','e','r','e',' '
     ,'_','P','r','o','p','e','r','t','y','=','`',0};

    static const WCHAR szEnd[]={'`',0};
    MSIPACKAGE *package;

    if (0 == hInstall) {
      return ERROR_INVALID_HANDLE;
    }
    if (NULL == szName) {
      return ERROR_INVALID_PARAMETER;
    }

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE);
    if( !package)
        return ERROR_INVALID_HANDLE;

    strcatW(Query,szName);
    strcatW(Query,szEnd);
    
    rc = MsiDatabaseOpenViewW(package->db, Query, &view);
    if (rc == ERROR_SUCCESS)
    {
        DWORD sz;
        WCHAR value[0x100];

        rc = MsiViewExecute(view, 0);
        if (rc != ERROR_SUCCESS)
        {
            MsiViewClose(view);
            MsiCloseHandle(view);
            return rc;
        }

        rc = MsiViewFetch(view,&row);
        if (rc == ERROR_SUCCESS)
        {
            sz=0x100;
            rc = MsiRecordGetStringW(row,1,value,&sz);
            strncpyW(szValueBuf,value,min(sz+1,*pchValueBuf));
            *pchValueBuf = sz+1;
            MsiCloseHandle(row);
        }
        MsiViewClose(view);
        MsiCloseHandle(view);
    }

    if (rc == ERROR_SUCCESS)
        TRACE("returning %s for property %s\n", debugstr_w(szValueBuf),
            debugstr_w(szName));
    else
    {
        *pchValueBuf = 0;
        TRACE("property not found\n");
    }

    return rc;
}
