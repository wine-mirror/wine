/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2005 Mike McCormack for CodeWeavers
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
http://msdn.microsoft.com/library/default.asp?url=/library/en-us/msi/setup/msiformatrecord.asp 
 */

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "wine/debug.h"
#include "fdi.h"
#include "msi.h"
#include "msiquery.h"
#include "msvcrt/fcntl.h"
#include "objbase.h"
#include "objidl.h"
#include "msipriv.h"
#include "winnls.h"
#include "winuser.h"
#include "shlobj.h"
#include "wine/unicode.h"
#include "ver.h"
#include "action.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

static const WCHAR* scanW(LPCWSTR buf, WCHAR token, DWORD len)
{
    DWORD i;
    for (i = 0; i < len; i++)
        if (buf[i] == token)
            return &buf[i];
    return NULL;
}

/*
 * This helper function should probably go alot of places
 *
 * Thinking about this, maybe this should become yet another Bison file
 *
 * len is in WCHARs
 * return is also in WCHARs
 */
static DWORD deformat_string_internal(MSIPACKAGE *package, LPCWSTR ptr, 
                                     WCHAR** data, DWORD len, MSIRECORD* record)
{
    const WCHAR* mark=NULL;
    WCHAR* mark2;
    DWORD size=0;
    DWORD chunk=0;
    WCHAR key[0x100];
    LPWSTR value = NULL;
    DWORD sz;
    UINT rc;
    INT index;
    LPWSTR newdata = NULL;

    if (ptr==NULL)
    {
        TRACE("Deformatting NULL string\n");
        *data = NULL;
        return 0;
    }

    TRACE("Starting with %s\n",debugstr_w(ptr));

    /* scan for special characters */
    if (!scanW(ptr,'[',len) || (scanW(ptr,'[',len) && !scanW(ptr,']',len)))
    {
        /* not formatted */
        *data = HeapAlloc(GetProcessHeap(),0,(len*sizeof(WCHAR)));
        memcpy(*data,ptr,len*sizeof(WCHAR));
        TRACE("Returning %s\n",debugstr_w(*data));
        return len;
    }
   
    /* formatted string located */ 
    mark = scanW(ptr,'[',len);
    if (mark != ptr)
    {
        INT cnt = (mark - ptr);
        TRACE("%i  (%i) characters before marker\n",cnt,(mark-ptr));
        size = cnt * sizeof(WCHAR);
        newdata = HeapAlloc(GetProcessHeap(),0,size);
        memcpy(newdata,ptr,(cnt * sizeof(WCHAR)));
    }
    else
    {
        size = 0;
        newdata = HeapAlloc(GetProcessHeap(),0,size);
        newdata[0]=0;
    }
    mark++;
    /* there should be no null characters in a key so strchrW is ok */
    mark2 = strchrW(mark,']');
    strncpyW(key,mark,mark2-mark);
    key[mark2-mark] = 0;
    mark = strchrW(mark,']');
    mark++;
    TRACE("Current %s .. %s\n",debugstr_w(newdata),debugstr_w(key));
    sz = 0;
    /* expand what we can deformat... Again, this should become a bison file */
    switch (key[0])
    {
        case '~':
            value = HeapAlloc(GetProcessHeap(),0,sizeof(WCHAR)*2);
            value[0] =  0;
            chunk = sizeof(WCHAR);
            rc = ERROR_SUCCESS;
            break;
        case '$':
            ERR("POORLY HANDLED DEFORMAT.. [$componentkey] \n");
            index = get_loaded_component(package,&key[1]);
            if (index >= 0)
            {
                value = resolve_folder(package, 
                                       package->components[index].Directory, 
                                       FALSE, FALSE, NULL);
                chunk = (strlenW(value)) * sizeof(WCHAR);
                rc = 0;
            }
            else
                rc = ERROR_FUNCTION_FAILED;
            break;
        case '#':
        case '!': /* should be short path */
            index = get_loaded_file(package,&key[1]);
            if (index >=0)
            {
                sz = strlenW(package->files[index].TargetPath);
                value = dupstrW(package->files[index].TargetPath);
                chunk = (strlenW(value)) * sizeof(WCHAR);
                rc= ERROR_SUCCESS;
            }
            else
                rc = ERROR_FUNCTION_FAILED;
            break;
        case '\\':
            value = HeapAlloc(GetProcessHeap(),0,sizeof(WCHAR)*2);
            value[0] =  key[1];
            chunk = sizeof(WCHAR);
            rc = ERROR_SUCCESS;
            break;
        case '%':
            sz  = GetEnvironmentVariableW(&key[1],NULL,0);
            if (sz > 0)
            {
                sz++;
                value = HeapAlloc(GetProcessHeap(),0,sz * sizeof(WCHAR));
                GetEnvironmentVariableW(&key[1],value,sz);
                chunk = (strlenW(value)) * sizeof(WCHAR);
                rc = ERROR_SUCCESS;
            }
            else
            {
                ERR("Unknown enviroment variable\n");
                chunk = 0;
                value = NULL;
                rc = ERROR_FUNCTION_FAILED;
            }
            break;
        default:
            /* check for numaric values */
            index = 0;
            while (isdigitW(key[index])) index++;
            if (key[index] == 0)
            {
                index = atoiW(key);
                TRACE("record index %i\n",index);
                value = load_dynamic_stringW(record,index);
                if (value)
                {
                    chunk = strlenW(value) * sizeof(WCHAR);
                    rc = ERROR_SUCCESS;
                }
                else
                {
                    value = NULL;
                    rc = ERROR_FUNCTION_FAILED;
                }
            }
            else
            {
                value = load_dynamic_property(package,key, &rc);
                if (rc == ERROR_SUCCESS)
                    chunk = (strlenW(value)) * sizeof(WCHAR);
            }
            break;      
    }
    if (((rc == ERROR_SUCCESS) || (rc == ERROR_MORE_DATA)) && value!=NULL)
    {
        LPWSTR nd2;
        TRACE("value %s, chunk %li size %li\n",debugstr_w(value),chunk,size);

        nd2= HeapReAlloc(GetProcessHeap(),0,newdata,(size + chunk));
        newdata = nd2;
        memcpy(&newdata[(size/sizeof(WCHAR))],value,chunk);
        size+=chunk;   
        HeapFree(GetProcessHeap(),0,value);
    }
    TRACE("after value %s .. %s\n",debugstr_w(newdata),debugstr_w(mark));
    if (mark - ptr < len)
    {
        LPWSTR nd2;
        chunk = (len - (mark - ptr)) * sizeof(WCHAR);
        TRACE("after chunk is %li\n",chunk);
        nd2 = HeapReAlloc(GetProcessHeap(),0,newdata,(size+chunk));
        newdata = nd2;
        memcpy(&newdata[(size/sizeof(WCHAR))],mark,chunk);
        size+=chunk;
    }
    TRACE("after trailing %s .. %s\n",debugstr_w(newdata),debugstr_w(mark));

    /* recursively do this to clean up */
    size = deformat_string_internal(package,newdata,data,(size/sizeof(WCHAR)),
                                    record);
    HeapFree(GetProcessHeap(),0,newdata);
    return size;
}


UINT MSI_FormatRecordW(MSIPACKAGE* package, MSIRECORD* record, LPWSTR buffer,
                        DWORD *size)
{
    LPWSTR deformated;
    LPWSTR rec;
    DWORD len;
    UINT rc = ERROR_INVALID_PARAMETER;

    TRACE("%p %p %p %li\n",package, record ,buffer, *size);

    rec = load_dynamic_stringW(record,0);
    if (!rec)
        return rc;

    TRACE("(%s)\n",debugstr_w(rec));

    len = deformat_string_internal(package,rec,&deformated,(strlenW(rec)+1),
                                   record);
    if (len <= *size)
    {
        *size = len;
        memcpy(buffer,deformated,len*sizeof(WCHAR));
        rc = ERROR_SUCCESS;
    }
    else
    {
        *size = len;
        rc = ERROR_MORE_DATA;
    }

    HeapFree(GetProcessHeap(),0,rec);
    HeapFree(GetProcessHeap(),0,deformated);
    return rc;
}

UINT WINAPI MsiFormatRecordA(MSIHANDLE hInstall, MSIHANDLE hRecord, LPSTR
szResult, DWORD *sz)
{
    UINT rc;
    MSIPACKAGE* package;
    MSIRECORD* record;
    LPWSTR szwResult;
    DWORD original_len;

    TRACE("%ld %ld %p %p\n", hInstall, hRecord, szResult, sz);

    package = msihandle2msiinfo(hInstall,MSIHANDLETYPE_PACKAGE);
    record = msihandle2msiinfo(hRecord,MSIHANDLETYPE_RECORD);

    if (!package || !record)
        return ERROR_INVALID_HANDLE;

    original_len = *sz;
    /* +1 just to make sure we have a buffer incase the len is 0 */
    szwResult = HeapAlloc(GetProcessHeap(),0,(original_len+1) * sizeof(WCHAR));

    rc = MSI_FormatRecordW(package, record, szwResult, sz);
    
    WideCharToMultiByte(CP_ACP,0,szwResult,original_len, szResult, original_len,
                        NULL,NULL);

    HeapFree(GetProcessHeap(),0,szwResult);

    return rc;

}

UINT WINAPI MsiFormatRecordW(MSIHANDLE hInstall, MSIHANDLE hRecord, 
                             LPWSTR szResult, DWORD *sz)
{
    UINT rc;
    MSIPACKAGE* package;
    MSIRECORD* record;

    TRACE("%ld %ld %p %p\n", hInstall, hRecord, szResult, sz);

    package = msihandle2msiinfo(hInstall,MSIHANDLETYPE_PACKAGE);
    record = msihandle2msiinfo(hRecord,MSIHANDLETYPE_RECORD);

    if (!package || !record)
        return ERROR_INVALID_HANDLE;

    rc = MSI_FormatRecordW(package, record, szResult, sz);

    return rc;
}
