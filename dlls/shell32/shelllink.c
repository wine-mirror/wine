/*
 *
 *      Copyright 1997  Marcus Meissner
 *      Copyright 1998  Juergen Schmied
 *      Copyright 2005  Mike McCormack
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
 *
 * NOTES
 *   Nearly complete information about the binary formats
 *   of .lnk files available at http://www.wotsit.org
 *
 *  You can use winedump to examine the contents of a link file:
 *   winedump lnk sc.lnk
 *
 *  MSI advertised shortcuts are totally undocumented.  They provide an
 *   icon for a program that is not yet installed, and invoke MSI to
 *   install the program when the shortcut is clicked on.  They are
 *   created by passing a special string to SetPath, and the information
 *   in that string is parsed an stored.
 */

#define COBJMACROS
#include "wine/debug.h"
#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"

#include "winuser.h"
#include "wingdi.h"
#include "shlobj.h"

#include "pidl.h"
#include "shell32_main.h"
#include "shlguid.h"
#include "shlwapi.h"
#include "msi.h"
#include "appmgmt.h"

#include "initguid.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

DEFINE_GUID( SHELL32_AdvtShortcutProduct,
       0x9db1186f,0x40df,0x11d1,0xaa,0x8c,0x00,0xc0,0x4f,0xb6,0x78,0x63);
DEFINE_GUID( SHELL32_AdvtShortcutComponent,
       0x9db1186e,0x40df,0x11d1,0xaa,0x8c,0x00,0xc0,0x4f,0xb6,0x78,0x63);

/* link file formats */

#pragma pack(push,1)

typedef struct _LINK_HEADER
{
	DWORD    dwSize;	/* 0x00 size of the header - 0x4c */
	GUID     MagicGuid;	/* 0x04 is CLSID_ShellLink */
	DWORD    dwFlags;	/* 0x14 describes elements following */
	DWORD    dwFileAttr;	/* 0x18 attributes of the target file */
	FILETIME CreationTime;	/* 0x1c creation time of target file */
	FILETIME AccessTime;	/* 0x24 access time of target file */
	FILETIME WriteTime;	/* 0x2c write time of target file */
	DWORD    dwFileSize;	/* 0x34 File size of target file */
	DWORD    nIcon;		/* 0x38 icon number or index */
	DWORD	fStartup;	/* 0x3c startup type or window state of application */
	WORD	wHotKey;	/* 0x40 hotkey */
	WORD	Reserved1;	/* 0x42 reserved = 0 */
	DWORD	Reserved2;	/* 0x44 reserved = 0 */
	DWORD	Reserved3;	/* 0x48 reserved = 0 */
} LINK_HEADER, * PLINK_HEADER;

#define SHLINK_LOCAL  0
#define SHLINK_REMOTE 1

typedef struct _LOCATION_INFO
{
    DWORD  dwTotalSize;
    DWORD  dwHeaderSize;
    DWORD  dwFlags;
    DWORD  dwVolTableOfs;
    DWORD  dwLocalPathOfs;
    DWORD  dwNetworkVolTableOfs;
    DWORD  dwFinalPathOfs;
} LOCATION_INFO;

typedef struct _LOCAL_VOLUME_INFO
{
    DWORD dwSize;
    DWORD dwType;
    DWORD dwVolSerial;
    DWORD dwVolLabelOfs;
} LOCAL_VOLUME_INFO;

typedef struct volume_info_t
{
    DWORD type;
    DWORD serial;
    WCHAR label[12];  /* assume 8.3 */
} volume_info;

#pragma pack(pop)

/* IShellLink Implementation */

typedef struct
{
        IShellLinkA IShellLinkA_iface;
        IShellLinkW IShellLinkW_iface;
        IPersistFile IPersistFile_iface;
        IPersistStream IPersistStream_iface;
        IShellLinkDataList IShellLinkDataList_iface;
        IShellExtInit IShellExtInit_iface;
        IContextMenu IContextMenu_iface;
        IObjectWithSite IObjectWithSite_iface;
        IPropertyStore IPropertyStore_iface;

	LONG            ref;

	/* data structures according to the information in the link */
	LPITEMIDLIST	pPidl;
	WORD		wHotKey;
	SYSTEMTIME	CreationTime;
	SYSTEMTIME	AccessTime;
	SYSTEMTIME	WriteTime;

	DWORD         iShowCmd;
	LPWSTR        sIcoPath;
	INT           iIcoNdx;
	LPWSTR        sPath;
	LPWSTR        sArgs;
	LPWSTR        sWorkDir;
	LPWSTR        sDescription;
	LPWSTR        sPathRel;
 	LPWSTR        sProduct;
 	LPWSTR        sComponent;
	volume_info   volume;

	BOOL          bDirty;
        INT           iIdOpen;  /* id of the "Open" entry in the context menu */
	IUnknown      *site;

	LPOLESTR      filepath; /* file path returned by IPersistFile::GetCurFile */
} IShellLinkImpl;

static inline IShellLinkImpl *impl_from_IShellLinkA(IShellLinkA *iface)
{
    return CONTAINING_RECORD(iface, IShellLinkImpl, IShellLinkA_iface);
}

static inline IShellLinkImpl *impl_from_IShellLinkW(IShellLinkW *iface)
{
    return CONTAINING_RECORD(iface, IShellLinkImpl, IShellLinkW_iface);
}

static inline IShellLinkImpl *impl_from_IPersistFile(IPersistFile *iface)
{
    return CONTAINING_RECORD(iface, IShellLinkImpl, IPersistFile_iface);
}

static inline IShellLinkImpl *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, IShellLinkImpl, IPersistStream_iface);
}

static inline IShellLinkImpl *impl_from_IShellLinkDataList(IShellLinkDataList *iface)
{
    return CONTAINING_RECORD(iface, IShellLinkImpl, IShellLinkDataList_iface);
}

static inline IShellLinkImpl *impl_from_IShellExtInit(IShellExtInit *iface)
{
    return CONTAINING_RECORD(iface, IShellLinkImpl, IShellExtInit_iface);
}

static inline IShellLinkImpl *impl_from_IContextMenu(IContextMenu *iface)
{
    return CONTAINING_RECORD(iface, IShellLinkImpl, IContextMenu_iface);
}

static inline IShellLinkImpl *impl_from_IObjectWithSite(IObjectWithSite *iface)
{
    return CONTAINING_RECORD(iface, IShellLinkImpl, IObjectWithSite_iface);
}

static inline IShellLinkImpl *impl_from_IPropertyStore(IPropertyStore *iface)
{
    return CONTAINING_RECORD(iface, IShellLinkImpl, IPropertyStore_iface);
}

static HRESULT ShellLink_UpdatePath(LPCWSTR sPathRel, LPCWSTR path, LPCWSTR sWorkDir, LPWSTR* psPath);

/**************************************************************************
 *  IPersistFile_QueryInterface
 */
static HRESULT WINAPI IPersistFile_fnQueryInterface(
	IPersistFile* iface,
	REFIID riid,
	LPVOID *ppvObj)
{
    IShellLinkImpl *This = impl_from_IPersistFile(iface);
    return IShellLinkW_QueryInterface(&This->IShellLinkW_iface, riid, ppvObj);
}

/******************************************************************************
 * IPersistFile_AddRef
 */
static ULONG WINAPI IPersistFile_fnAddRef(IPersistFile* iface)
{
    IShellLinkImpl *This = impl_from_IPersistFile(iface);
    return IShellLinkW_AddRef(&This->IShellLinkW_iface);
}

/******************************************************************************
 * IPersistFile_Release
 */
static ULONG WINAPI IPersistFile_fnRelease(IPersistFile* iface)
{
    IShellLinkImpl *This = impl_from_IPersistFile(iface);
    return IShellLinkW_Release(&This->IShellLinkW_iface);
}

static HRESULT WINAPI IPersistFile_fnGetClassID(IPersistFile* iface, CLSID *pClassID)
{
    IShellLinkImpl *This = impl_from_IPersistFile(iface);

    TRACE("(%p)->(%p)\n", This, pClassID);

    *pClassID = CLSID_ShellLink;

    return S_OK;
}

static HRESULT WINAPI IPersistFile_fnIsDirty(IPersistFile* iface)
{
	IShellLinkImpl *This = impl_from_IPersistFile(iface);

	TRACE("(%p)\n",This);

	if (This->bDirty)
	    return S_OK;

	return S_FALSE;
}

static HRESULT WINAPI IPersistFile_fnLoad(IPersistFile* iface, LPCOLESTR pszFileName, DWORD dwMode)
{
	IShellLinkImpl *This = impl_from_IPersistFile(iface);
        IPersistStream *StreamThis = &This->IPersistStream_iface;
        HRESULT r;
        IStream *stm;

        TRACE("(%p, %s, %lx)\n",This, debugstr_w(pszFileName), dwMode);

        if( dwMode == 0 ) dwMode = STGM_READ;
        r = SHCreateStreamOnFileW(pszFileName, dwMode, &stm);
        if( SUCCEEDED( r ) )
        {
            r = IPersistStream_Load(StreamThis, stm);
            ShellLink_UpdatePath(This->sPathRel, pszFileName, This->sWorkDir, &This->sPath);
            IStream_Release( stm );

            /* update file path */
            free(This->filepath);
            This->filepath = wcsdup(pszFileName);

            This->bDirty = FALSE;
        }
        TRACE("-- returning hr %08lx\n", r);
        return r;
}

BOOL run_winemenubuilder( const WCHAR *args )
{
    LONG len;
    LPWSTR buffer;
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    BOOL ret;
    WCHAR app[MAX_PATH];
    void *redir;

    GetSystemDirectoryW( app, MAX_PATH );
    lstrcatW( app, L"\\winemenubuilder.exe" );

    len = (lstrlenW( app ) + lstrlenW( args ) + 1) * sizeof(WCHAR);
    buffer = malloc( len );
    if( !buffer )
        return FALSE;

    lstrcpyW( buffer, app );
    lstrcatW( buffer, args );

    TRACE("starting %s\n",debugstr_w(buffer));

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);

    Wow64DisableWow64FsRedirection( &redir );
    ret = CreateProcessW( app, buffer, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &si, &pi );
    Wow64RevertWow64FsRedirection( redir );

    free( buffer );

    if (ret)
    {
        CloseHandle( pi.hProcess );
        CloseHandle( pi.hThread );
    }

    return ret;
}

static BOOL StartLinkProcessor( LPCOLESTR szLink )
{
    LONG len;
    LPWSTR buffer;
    BOOL ret;

    len = (lstrlenW( szLink ) + 7) * sizeof(WCHAR);
    buffer = malloc( len );
    if( !buffer )
        return FALSE;

    swprintf( buffer, len, L" -w \"%s\"", szLink );
    ret = run_winemenubuilder( buffer );
    free( buffer );
    return ret;
}

static HRESULT WINAPI IPersistFile_fnSave(IPersistFile* iface, LPCOLESTR pszFileName, BOOL fRemember)
{
    IShellLinkImpl *This = impl_from_IPersistFile(iface);
    IPersistStream *StreamThis = &This->IPersistStream_iface;
    HRESULT r;
    IStream *stm;

    TRACE("(%p)->(%s)\n",This,debugstr_w(pszFileName));

    if (!pszFileName)
    {
        if (!This->filepath) return S_OK;

        pszFileName = This->filepath;
        fRemember = FALSE;
    }

    r = SHCreateStreamOnFileW( pszFileName, STGM_READWRITE | STGM_CREATE | STGM_SHARE_DENY_WRITE, &stm );
    if( SUCCEEDED( r ) )
    {
        r = IPersistStream_Save(StreamThis, stm, FALSE);
        IStream_Release( stm );

        if( SUCCEEDED( r ) )
	{
            StartLinkProcessor( pszFileName );

            if (fRemember)
            {
                /* update file path */
                free(This->filepath);
                This->filepath = wcsdup(pszFileName);
            }

            This->bDirty = FALSE;
        }
	else
        {
            DeleteFileW( pszFileName );
            WARN("Failed to create shortcut %s\n", debugstr_w(pszFileName) );
        }
    }

    return r;
}

static HRESULT WINAPI IPersistFile_fnSaveCompleted(IPersistFile* iface, LPCOLESTR filename)
{
    IShellLinkImpl *This = impl_from_IPersistFile(iface);
    FIXME("(%p)->(%s): stub\n", This, debugstr_w(filename));
    return S_OK;
}

static HRESULT WINAPI IPersistFile_fnGetCurFile(IPersistFile* iface, LPOLESTR *filename)
{
    IShellLinkImpl *This = impl_from_IPersistFile(iface);

    TRACE("(%p)->(%p)\n", This, filename);

    if (!This->filepath)
    {
        *filename = NULL;
        return S_FALSE;
    }

    *filename = CoTaskMemAlloc((lstrlenW(This->filepath) + 1) * sizeof(WCHAR));
    if (!*filename) return E_OUTOFMEMORY;

    lstrcpyW(*filename, This->filepath);

    return S_OK;
}

static const IPersistFileVtbl pfvt =
{
	IPersistFile_fnQueryInterface,
	IPersistFile_fnAddRef,
	IPersistFile_fnRelease,
	IPersistFile_fnGetClassID,
	IPersistFile_fnIsDirty,
	IPersistFile_fnLoad,
	IPersistFile_fnSave,
	IPersistFile_fnSaveCompleted,
	IPersistFile_fnGetCurFile
};

/************************************************************************
 * IPersistStream_QueryInterface
 */
static HRESULT WINAPI IPersistStream_fnQueryInterface(
	IPersistStream* iface,
	REFIID     riid,
	VOID**     ppvObj)
{
    IShellLinkImpl *This = impl_from_IPersistStream(iface);
    return IShellLinkW_QueryInterface(&This->IShellLinkW_iface, riid, ppvObj);
}

/************************************************************************
 * IPersistStream_Release
 */
static ULONG WINAPI IPersistStream_fnRelease(
	IPersistStream* iface)
{
    IShellLinkImpl *This = impl_from_IPersistStream(iface);
    return IShellLinkW_Release(&This->IShellLinkW_iface);
}

/************************************************************************
 * IPersistStream_AddRef
 */
static ULONG WINAPI IPersistStream_fnAddRef(
	IPersistStream* iface)
{
    IShellLinkImpl *This = impl_from_IPersistStream(iface);
    return IShellLinkW_AddRef(&This->IShellLinkW_iface);
}

/************************************************************************
 * IPersistStream_GetClassID
 *
 */
static HRESULT WINAPI IPersistStream_fnGetClassID(
	IPersistStream* iface,
	CLSID* pClassID)
{
    IShellLinkImpl *This = impl_from_IPersistStream(iface);
    return IPersistFile_GetClassID(&This->IPersistFile_iface, pClassID);
}

/************************************************************************
 * IPersistStream_IsDirty (IPersistStream)
 */
static HRESULT WINAPI IPersistStream_fnIsDirty(
	IPersistStream*  iface)
{
	IShellLinkImpl *This = impl_from_IPersistStream(iface);

	TRACE("(%p)\n", This);

	return S_OK;
}


static HRESULT Stream_LoadString( IStream* stm, BOOL unicode, LPWSTR *pstr )
{
    DWORD count;
    USHORT len;
    LPVOID temp;
    LPWSTR str;
    HRESULT r;

    TRACE("%p\n", stm);

    count = 0;
    r = IStream_Read(stm, &len, sizeof(len), &count);
    if ( FAILED (r) || ( count != sizeof(len) ) )
        return E_FAIL;

    if( unicode )
        len *= sizeof (WCHAR);

    TRACE("reading %d\n", len);
    temp = malloc(len + sizeof(WCHAR));
    if( !temp )
        return E_OUTOFMEMORY;
    count = 0;
    r = IStream_Read(stm, temp, len, &count);
    if( FAILED (r) || ( count != len ) )
    {
        free( temp );
        return E_FAIL;
    }

    TRACE("read %s\n", debugstr_an(temp,len));

    /* convert to unicode if necessary */
    if( !unicode )
    {
        count = MultiByteToWideChar( CP_ACP, 0, temp, len, NULL, 0 );
        str = malloc( (count + 1) * sizeof(WCHAR) );
        if( !str )
        {
            free( temp );
            return E_OUTOFMEMORY;
        }
        MultiByteToWideChar( CP_ACP, 0, temp, len, str, count );
        free( temp );
    }
    else
    {
        count /= 2;
        str = temp;
    }
    str[count] = 0;

    *pstr = str;

    return S_OK;
}

static HRESULT Stream_ReadChunk( IStream* stm, LPVOID *data )
{
    DWORD size;
    ULONG count;
    HRESULT r;
    struct sized_chunk {
        DWORD size;
        unsigned char data[1];
    } *chunk;

    TRACE("%p\n",stm);

    r = IStream_Read( stm, &size, sizeof(size), &count );
    if( FAILED( r )  || count != sizeof(size) )
        return E_FAIL;

    chunk = malloc( size );
    if( !chunk )
        return E_OUTOFMEMORY;

    chunk->size = size;
    r = IStream_Read( stm, chunk->data, size - sizeof(size), &count );
    if( FAILED( r ) || count != (size - sizeof(size)) )
    {
        free( chunk );
        return E_FAIL;
    }

    TRACE("Read %ld bytes\n",chunk->size);

    *data = chunk;

    return S_OK;
}

static BOOL Stream_LoadVolume( LOCAL_VOLUME_INFO *vol, volume_info *volume )
{
    const int label_sz = ARRAY_SIZE(volume->label);
    LPSTR label;
    int len;

    volume->serial = vol->dwVolSerial;
    volume->type = vol->dwType;

    if( !vol->dwVolLabelOfs )
        return FALSE;
    if( vol->dwSize <= vol->dwVolLabelOfs )
        return FALSE;
    len = vol->dwSize - vol->dwVolLabelOfs;

    label = (LPSTR) vol;
    label += vol->dwVolLabelOfs;
    MultiByteToWideChar( CP_ACP, 0, label, len, volume->label, label_sz-1);

    return TRUE;
}

static LPWSTR Stream_LoadPath( LPCSTR p, DWORD maxlen )
{
    int len = 0, wlen;
    LPWSTR path;

    while( (len < maxlen) && p[len] )
        len++;

    wlen = MultiByteToWideChar(CP_ACP, 0, p, len, NULL, 0);
    path = malloc((wlen + 1) * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, p, len, path, wlen);
    path[wlen] = 0;

    return path;
}

static HRESULT Stream_LoadLocation( IStream *stm,
                volume_info *volume, LPWSTR *path )
{
    char *p = NULL;
    LOCATION_INFO *loc;
    HRESULT r;
    DWORD n;

    r = Stream_ReadChunk( stm, (LPVOID*) &p );
    if( FAILED(r) )
        return r;

    loc = (LOCATION_INFO*) p;
    if (loc->dwTotalSize < sizeof(LOCATION_INFO))
    {
        free( p );
        return E_FAIL;
    }

    /* if there's valid local volume information, load it */
    if( loc->dwVolTableOfs && 
       ((loc->dwVolTableOfs + sizeof(LOCAL_VOLUME_INFO)) <= loc->dwTotalSize) )
    {
        LOCAL_VOLUME_INFO *volume_info;

        volume_info = (LOCAL_VOLUME_INFO*) &p[loc->dwVolTableOfs];
        Stream_LoadVolume( volume_info, volume );
    }

    /* if there's a local path, load it */
    n = loc->dwLocalPathOfs;
    if( n && (n < loc->dwTotalSize) )
        *path = Stream_LoadPath( &p[n], loc->dwTotalSize - n );

    TRACE("type %ld serial %08lx name %s path %s\n", volume->type,
          volume->serial, debugstr_w(volume->label), debugstr_w(*path));

    free( p );
    return S_OK;
}

/*
 *  The format of the advertised shortcut info seems to be:
 *
 *  Offset     Description
 *  ------     -----------
 *
 *    0          Length of the block (4 bytes, usually 0x314)
 *    4          tag (dword)
 *    8          string data in ANSI
 *    8+0x104    string data in UNICODE
 *
 * In the original Win32 implementation the buffers are not initialized
 *  to zero, so data trailing the string is random garbage.
 */
static HRESULT Stream_LoadAdvertiseInfo( IStream* stm, LPWSTR *str )
{
    DWORD size;
    ULONG count;
    HRESULT r;
    EXP_DARWIN_LINK buffer;
    
    TRACE("%p\n",stm);

    r = IStream_Read( stm, &buffer.dbh.cbSize, sizeof (DWORD), &count );
    if( FAILED( r ) )
        return r;

    /* make sure that we read the size of the structure even on error */
    size = sizeof buffer - sizeof (DWORD);
    if( buffer.dbh.cbSize != sizeof buffer )
    {
        ERR("Ooops.  This structure is not as expected...\n");
        return E_FAIL;
    }

    r = IStream_Read( stm, &buffer.dbh.dwSignature, size, &count );
    if( FAILED( r ) )
        return r;

    if( count != size )
        return E_FAIL;

    TRACE("magic %08lx  string = %s\n", buffer.dbh.dwSignature, debugstr_w(buffer.szwDarwinID));

    if( (buffer.dbh.dwSignature&0xffff0000) != 0xa0000000 )
    {
        ERR("Unknown magic number %08lx in advertised shortcut\n", buffer.dbh.dwSignature);
        return E_FAIL;
    }

    *str = wcsdup( buffer.szwDarwinID );

    return S_OK;
}

/************************************************************************
 * IPersistStream_Load (IPersistStream)
 */
static HRESULT WINAPI IPersistStream_fnLoad(
    IPersistStream*  iface,
    IStream*         stm)
{
    LINK_HEADER hdr;
    ULONG    dwBytesRead;
    BOOL     unicode;
    HRESULT  r;
    DWORD    zero;

    IShellLinkImpl *This = impl_from_IPersistStream(iface);

    TRACE("%p %p\n", This, stm);

    if( !stm )
        return STG_E_INVALIDPOINTER;

    dwBytesRead = 0;
    r = IStream_Read(stm, &hdr, sizeof(hdr), &dwBytesRead);
    if( FAILED( r ) )
        return r;

    if( dwBytesRead != sizeof(hdr))
        return E_FAIL;
    if( hdr.dwSize != sizeof(hdr))
        return E_FAIL;
    if( !IsEqualIID(&hdr.MagicGuid, &CLSID_ShellLink) )
        return E_FAIL;

    /* free all the old stuff */
    ILFree(This->pPidl);
    This->pPidl = NULL;
    memset( &This->volume, 0, sizeof This->volume );
    free(This->sPath);
    This->sPath = NULL;
    free(This->sDescription);
    This->sDescription = NULL;
    free(This->sPathRel);
    This->sPathRel = NULL;
    free(This->sWorkDir);
    This->sWorkDir = NULL;
    free(This->sArgs);
    This->sArgs = NULL;
    free(This->sIcoPath);
    This->sIcoPath = NULL;
    free(This->sProduct);
    This->sProduct = NULL;
    free(This->sComponent);
    This->sComponent = NULL;
        
    This->wHotKey = hdr.wHotKey;
    This->iIcoNdx = hdr.nIcon;
    FileTimeToSystemTime (&hdr.CreationTime, &This->CreationTime);
    FileTimeToSystemTime (&hdr.AccessTime, &This->AccessTime);
    FileTimeToSystemTime (&hdr.WriteTime, &This->WriteTime);
    if (TRACE_ON(shell))
    {
        WCHAR sTemp[MAX_PATH];
        GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &This->CreationTime, NULL, sTemp, ARRAY_SIZE(sTemp));
        TRACE("-- CreationTime: %s\n", debugstr_w(sTemp) );
        GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &This->AccessTime, NULL, sTemp, ARRAY_SIZE(sTemp));
        TRACE("-- AccessTime: %s\n", debugstr_w(sTemp) );
        GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &This->WriteTime, NULL, sTemp, ARRAY_SIZE(sTemp));
        TRACE("-- WriteTime: %s\n", debugstr_w(sTemp) );
    }

    /* load all the new stuff */
    if( hdr.dwFlags & SLDF_HAS_ID_LIST )
    {
        r = ILLoadFromStream( stm, &This->pPidl );
        if( FAILED( r ) )
            return r;
    }
    pdump(This->pPidl);

    /* load the location information */
    if( hdr.dwFlags & SLDF_HAS_LINK_INFO )
        r = Stream_LoadLocation( stm, &This->volume, &This->sPath );
    if( FAILED( r ) )
        goto end;

    unicode = hdr.dwFlags & SLDF_UNICODE;
    if( hdr.dwFlags & SLDF_HAS_NAME )
    {
        r = Stream_LoadString( stm, unicode, &This->sDescription );
        TRACE("Description  -> %s\n",debugstr_w(This->sDescription));
    }
    if( FAILED( r ) )
        goto end;

    if( hdr.dwFlags & SLDF_HAS_RELPATH )
    {
        r = Stream_LoadString( stm, unicode, &This->sPathRel );
        TRACE("Relative Path-> %s\n",debugstr_w(This->sPathRel));
    }
    if( FAILED( r ) )
        goto end;

    if( hdr.dwFlags & SLDF_HAS_WORKINGDIR )
    {
        r = Stream_LoadString( stm, unicode, &This->sWorkDir );
        TRACE("Working Dir  -> %s\n",debugstr_w(This->sWorkDir));
    }
    if( FAILED( r ) )
        goto end;

    if( hdr.dwFlags & SLDF_HAS_ARGS )
    {
        r = Stream_LoadString( stm, unicode, &This->sArgs );
        TRACE("Working Dir  -> %s\n",debugstr_w(This->sArgs));
    }
    if( FAILED( r ) )
        goto end;

    if( hdr.dwFlags & SLDF_HAS_ICONLOCATION )
    {
        r = Stream_LoadString( stm, unicode, &This->sIcoPath );
        TRACE("Icon file    -> %s\n",debugstr_w(This->sIcoPath));
    }
    if( FAILED( r ) )
        goto end;

    if( hdr.dwFlags & SLDF_HAS_LOGO3ID )
    {
        r = Stream_LoadAdvertiseInfo( stm, &This->sProduct );
        TRACE("Product      -> %s\n",debugstr_w(This->sProduct));
    }
    if( FAILED( r ) )
        goto end;

    if( hdr.dwFlags & SLDF_HAS_DARWINID )
    {
        r = Stream_LoadAdvertiseInfo( stm, &This->sComponent );
        TRACE("Component    -> %s\n",debugstr_w(This->sComponent));
    }
    if( FAILED( r ) )
        goto end;

    r = IStream_Read(stm, &zero, sizeof zero, &dwBytesRead);
    if( FAILED( r ) || zero || dwBytesRead != sizeof zero )
    {
        /* Some lnk files have extra data blocks starting with a
         * DATABLOCK_HEADER. For instance EXP_SPECIAL_FOLDER and an unknown
         * one with a 0xa0000003 signature. However these don't seem to matter
         * too much.
         */
        WARN("Last word was not zero\n");
    }

    TRACE("OK\n");

    pdump (This->pPidl);

    return S_OK;
end:
    return r;
}

/************************************************************************
 * Stream_WriteString
 *
 * Helper function for IPersistStream_Save. Writes a unicode string 
 *  with terminating nul byte to a stream, preceded by the its length.
 */
static HRESULT Stream_WriteString( IStream* stm, LPCWSTR str )
{
    USHORT len = lstrlenW( str ) + 1;
    DWORD count;
    HRESULT r;

    r = IStream_Write( stm, &len, sizeof(len), &count );
    if( FAILED( r ) )
        return r;

    len *= sizeof(WCHAR);

    r = IStream_Write( stm, str, len, &count );
    if( FAILED( r ) )
        return r;

    return S_OK;
}

/************************************************************************
 * Stream_WriteLocationInfo
 *
 * Writes the location info to a stream
 *
 * FIXME: One day we might want to write the network volume information
 *        and the final path.
 *        Figure out how Windows deals with unicode paths here.
 */
static HRESULT Stream_WriteLocationInfo( IStream* stm, LPCWSTR path,
                                         volume_info *volume )
{
    DWORD total_size, path_size, volume_info_size, label_size, final_path_size;
    LOCAL_VOLUME_INFO *vol;
    LOCATION_INFO *loc;
    LPSTR szLabel, szPath, szFinalPath;
    ULONG count = 0;
    HRESULT hr;

    TRACE("%p %s %p\n", stm, debugstr_w(path), volume);

    /* figure out the size of everything */
    label_size = WideCharToMultiByte( CP_ACP, 0, volume->label, -1,
                                      NULL, 0, NULL, NULL );
    path_size = WideCharToMultiByte( CP_ACP, 0, path, -1,
                                     NULL, 0, NULL, NULL );
    volume_info_size = sizeof *vol + label_size;
    final_path_size = 1;
    total_size = sizeof *loc + volume_info_size + path_size + final_path_size;

    /* create pointers to everything */
    loc = calloc(1, total_size);
    vol = (LOCAL_VOLUME_INFO*) &loc[1];
    szLabel = (LPSTR) &vol[1];
    szPath = &szLabel[label_size];
    szFinalPath = &szPath[path_size];

    /* fill in the location information header */
    loc->dwTotalSize = total_size;
    loc->dwHeaderSize = sizeof (*loc);
    loc->dwFlags = 1;
    loc->dwVolTableOfs = sizeof (*loc);
    loc->dwLocalPathOfs = sizeof (*loc) + volume_info_size;
    loc->dwNetworkVolTableOfs = 0;
    loc->dwFinalPathOfs = sizeof (*loc) + volume_info_size + path_size;

    /* fill in the volume information */
    vol->dwSize = volume_info_size;
    vol->dwType = volume->type;
    vol->dwVolSerial = volume->serial;
    vol->dwVolLabelOfs = sizeof (*vol);

    /* copy in the strings */
    WideCharToMultiByte( CP_ACP, 0, volume->label, -1,
                         szLabel, label_size, NULL, NULL );
    WideCharToMultiByte( CP_ACP, 0, path, -1,
                         szPath, path_size, NULL, NULL );
    szFinalPath[0] = 0;

    hr = IStream_Write( stm, loc, total_size, &count );
    free(loc);

    return hr;
}

static EXP_DARWIN_LINK* shelllink_build_darwinid( LPCWSTR string, DWORD magic )
{
    EXP_DARWIN_LINK *buffer;
    
    buffer = LocalAlloc( LMEM_ZEROINIT, sizeof *buffer );
    buffer->dbh.cbSize = sizeof *buffer;
    buffer->dbh.dwSignature = magic;
    lstrcpynW( buffer->szwDarwinID, string, MAX_PATH );
    WideCharToMultiByte(CP_ACP, 0, string, -1, buffer->szDarwinID, MAX_PATH, NULL, NULL );

    return buffer;
}

static HRESULT Stream_WriteAdvertiseInfo( IStream* stm, LPCWSTR string, DWORD magic )
{
    EXP_DARWIN_LINK *buffer;
    ULONG count;
    
    TRACE("%p\n",stm);

    buffer = shelllink_build_darwinid( string, magic );

    return IStream_Write( stm, buffer, buffer->dbh.cbSize, &count );
}

/************************************************************************
 * IPersistStream_Save (IPersistStream)
 *
 * FIXME: makes assumptions about byte order
 */
static HRESULT WINAPI IPersistStream_fnSave(
	IPersistStream*  iface,
	IStream*         stm,
	BOOL             fClearDirty)
{
    LINK_HEADER header;
    ULONG   count;
    DWORD   zero;
    HRESULT r;

    IShellLinkImpl *This = impl_from_IPersistStream(iface);

    TRACE("%p %p %x\n", This, stm, fClearDirty);

    memset(&header, 0, sizeof(header));
    header.dwSize = sizeof(header);
    header.fStartup = This->iShowCmd;
    header.MagicGuid = CLSID_ShellLink;

    header.wHotKey = This->wHotKey;
    header.nIcon = This->iIcoNdx;
    header.dwFlags = SLDF_UNICODE;   /* strings are in unicode */
    if( This->pPidl )
        header.dwFlags |= SLDF_HAS_ID_LIST;
    if( This->sPath )
        header.dwFlags |= SLDF_HAS_LINK_INFO;
    if( This->sDescription )
        header.dwFlags |= SLDF_HAS_NAME;
    if( This->sWorkDir )
        header.dwFlags |= SLDF_HAS_WORKINGDIR;
    if( This->sArgs )
        header.dwFlags |= SLDF_HAS_ARGS;
    if( This->sIcoPath )
        header.dwFlags |= SLDF_HAS_ICONLOCATION;
    if( This->sProduct )
        header.dwFlags |= SLDF_HAS_LOGO3ID;
    if( This->sComponent )
        header.dwFlags |= SLDF_HAS_DARWINID;

    SystemTimeToFileTime ( &This->CreationTime, &header.CreationTime );
    SystemTimeToFileTime ( &This->AccessTime, &header.AccessTime );
    SystemTimeToFileTime ( &This->WriteTime, &header.WriteTime );

    /* write the Shortcut header */
    r = IStream_Write( stm, &header, sizeof(header), &count );
    if( FAILED( r ) )
    {
        ERR("Write failed at %d\n",__LINE__);
        return r;
    }

    TRACE("Writing pidl\n");

    /* write the PIDL to the shortcut */
    if( This->pPidl )
    {
        r = ILSaveToStream( stm, This->pPidl );
        if( FAILED( r ) )
        {
            ERR("Failed to write PIDL at %d\n",__LINE__);
            return r;
        }
    }

    if( This->sPath )
        Stream_WriteLocationInfo( stm, This->sPath, &This->volume );

    if( This->sDescription )
        r = Stream_WriteString( stm, This->sDescription );

    if( This->sPathRel )
        r = Stream_WriteString( stm, This->sPathRel );

    if( This->sWorkDir )
        r = Stream_WriteString( stm, This->sWorkDir );

    if( This->sArgs )
        r = Stream_WriteString( stm, This->sArgs );

    if( This->sIcoPath )
        r = Stream_WriteString( stm, This->sIcoPath );

    if( This->sProduct )
        r = Stream_WriteAdvertiseInfo( stm, This->sProduct, EXP_SZ_ICON_SIG );

    if( This->sComponent )
        r = Stream_WriteAdvertiseInfo( stm, This->sComponent, EXP_DARWIN_ID_SIG );

    /* the last field is a single zero dword */
    zero = 0;
    r = IStream_Write( stm, &zero, sizeof zero, &count );

    return S_OK;
}

/************************************************************************
 * IPersistStream_GetSizeMax (IPersistStream)
 */
static HRESULT WINAPI IPersistStream_fnGetSizeMax(
	IPersistStream*  iface,
	ULARGE_INTEGER*  pcbSize)
{
	IShellLinkImpl *This = impl_from_IPersistStream(iface);

	TRACE("(%p)\n", This);

	return E_NOTIMPL;
}

static const IPersistStreamVtbl psvt =
{
	IPersistStream_fnQueryInterface,
	IPersistStream_fnAddRef,
	IPersistStream_fnRelease,
	IPersistStream_fnGetClassID,
	IPersistStream_fnIsDirty,
	IPersistStream_fnLoad,
	IPersistStream_fnSave,
	IPersistStream_fnGetSizeMax
};

static BOOL SHELL_ExistsFileW(LPCWSTR path)
{
    if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(path))
        return FALSE;
    return TRUE;
}

/**************************************************************************
 *  ShellLink_UpdatePath
 *	update absolute path in sPath using relative path in sPathRel
 */
static HRESULT ShellLink_UpdatePath(LPCWSTR sPathRel, LPCWSTR path, LPCWSTR sWorkDir, LPWSTR* psPath)
{
    if (!path || !psPath)
	return E_INVALIDARG;

    if (!*psPath && sPathRel) {
	WCHAR buffer[2*MAX_PATH], abs_path[2*MAX_PATH];
	LPWSTR final = NULL;

	/* first try if [directory of link file] + [relative path] finds an existing file */

        GetFullPathNameW( path, MAX_PATH*2, buffer, &final );
        if( !final )
            final = buffer;
	lstrcpyW(final, sPathRel);

	*abs_path = '\0';

	if (SHELL_ExistsFileW(buffer)) {
	    if (!GetFullPathNameW(buffer, MAX_PATH, abs_path, &final))
		lstrcpyW(abs_path, buffer);
	} else {
	    /* try if [working directory] + [relative path] finds an existing file */
	    if (sWorkDir) {
		lstrcpyW(buffer, sWorkDir);
		lstrcpyW(PathAddBackslashW(buffer), sPathRel);

		if (SHELL_ExistsFileW(buffer))
		    if (!GetFullPathNameW(buffer, MAX_PATH, abs_path, &final))
			lstrcpyW(abs_path, buffer);
	    }
	}

	/* FIXME: This is even not enough - not all shell links can be resolved using this algorithm. */
	if (!*abs_path)
	    lstrcpyW(abs_path, sPathRel);

	*psPath = wcsdup(abs_path);
	if (!*psPath)
	    return E_OUTOFMEMORY;
    }

    return S_OK;
}

/**************************************************************************
 *	  IShellLink_ConstructFromFile
 */
HRESULT IShellLink_ConstructFromFile( IUnknown* pUnkOuter, REFIID riid,
               LPCITEMIDLIST pidl, IUnknown **ppv)
{
    IShellLinkW* psl;

    HRESULT hr = IShellLink_Constructor(NULL, riid, (LPVOID*)&psl);

    if (SUCCEEDED(hr)) {
	IPersistFile* ppf;

	*ppv = NULL;

	hr = IShellLinkW_QueryInterface(psl, &IID_IPersistFile, (LPVOID*)&ppf);

	if (SUCCEEDED(hr)) {
	    WCHAR path[MAX_PATH];

	    if (SHGetPathFromIDListW(pidl, path)) 
		hr = IPersistFile_Load(ppf, path, 0);
            else
                hr = E_FAIL;

	    if (SUCCEEDED(hr))
                *ppv = (IUnknown*)psl;

	    IPersistFile_Release(ppf);
	}

	if (!*ppv)
	    IShellLinkW_Release(psl);
    }

    return hr;
}

/**************************************************************************
 *  IShellLinkA_QueryInterface
 */
static HRESULT WINAPI IShellLinkA_fnQueryInterface(IShellLinkA *iface, REFIID riid, void **ppvObj)
{
    IShellLinkImpl *This = impl_from_IShellLinkA(iface);
    return IShellLinkW_QueryInterface(&This->IShellLinkW_iface, riid, ppvObj);
}

/******************************************************************************
 * IShellLinkA_AddRef
 */
static ULONG WINAPI IShellLinkA_fnAddRef(IShellLinkA *iface)
{
    IShellLinkImpl *This = impl_from_IShellLinkA(iface);
    return IShellLinkW_AddRef(&This->IShellLinkW_iface);
}

/******************************************************************************
 *	IShellLinkA_Release
 */
static ULONG WINAPI IShellLinkA_fnRelease(IShellLinkA *iface)
{
    IShellLinkImpl *This = impl_from_IShellLinkA(iface);
    return IShellLinkW_Release(&This->IShellLinkW_iface);
}

static HRESULT WINAPI IShellLinkA_fnGetPath(IShellLinkA *iface, LPSTR pszFile, INT cchMaxPath,
        WIN32_FIND_DATAA *pfd, DWORD fFlags)
{
    IShellLinkImpl *This = impl_from_IShellLinkA(iface);
    HRESULT res = S_OK;

    TRACE("(%p)->(pfile=%p len=%u find_data=%p flags=%lu)(%s)\n",
          This, pszFile, cchMaxPath, pfd, fFlags, debugstr_w(This->sPath));

    if (This->sComponent || This->sProduct)
        return S_FALSE;

    if (cchMaxPath)
        pszFile[0] = 0;
    if (This->sPath && This->sPath[0])
        WideCharToMultiByte( CP_ACP, 0, This->sPath, -1,
                             pszFile, cchMaxPath, NULL, NULL);
    else
        res = S_FALSE;

    if (pfd)
    {
        memset(pfd, 0, sizeof(*pfd));

        if (res == S_OK)
        {
            char path[MAX_PATH];
            WIN32_FILE_ATTRIBUTE_DATA fad;

            WideCharToMultiByte(CP_ACP, 0, This->sPath, -1, path, MAX_PATH, NULL, NULL);

            if (GetFileAttributesExW(This->sPath, GetFileExInfoStandard, &fad))
            {
                pfd->dwFileAttributes = fad.dwFileAttributes;
                pfd->ftCreationTime = fad.ftCreationTime;
                pfd->ftLastAccessTime = fad.ftLastAccessTime;
                pfd->ftLastWriteTime = fad.ftLastWriteTime;
                pfd->nFileSizeHigh = fad.nFileSizeHigh;
                pfd->nFileSizeLow = fad.nFileSizeLow;
            }

            lstrcpyA(pfd->cFileName, PathFindFileNameA(path));

            if (GetShortPathNameA(path, path, MAX_PATH))
            {
                lstrcpyA(pfd->cAlternateFileName, PathFindFileNameA(path));
            }
        }

        TRACE("attr 0x%08lx size 0x%08lx%08lx name %s shortname %s\n", pfd->dwFileAttributes,
            pfd->nFileSizeHigh, pfd->nFileSizeLow, wine_dbgstr_a(pfd->cFileName),
            wine_dbgstr_a(pfd->cAlternateFileName));
    }

    return res;
}

static HRESULT WINAPI IShellLinkA_fnGetIDList(IShellLinkA *iface, LPITEMIDLIST *ppidl)
{
    IShellLinkImpl *This = impl_from_IShellLinkA(iface);
    return IShellLinkW_GetIDList(&This->IShellLinkW_iface, ppidl);
}

static HRESULT WINAPI IShellLinkA_fnSetIDList(IShellLinkA *iface, LPCITEMIDLIST pidl)
{
    IShellLinkImpl *This = impl_from_IShellLinkA(iface);
    return IShellLinkW_SetIDList(&This->IShellLinkW_iface, pidl);
}

static HRESULT WINAPI IShellLinkA_fnGetDescription(IShellLinkA *iface, LPSTR pszName,
        INT cchMaxName)
{
    IShellLinkImpl *This = impl_from_IShellLinkA(iface);

    TRACE("(%p)->(%p len=%u)\n",This, pszName, cchMaxName);

    if( cchMaxName )
        pszName[0] = 0;
    if( This->sDescription )
        WideCharToMultiByte( CP_ACP, 0, This->sDescription, -1,
            pszName, cchMaxName, NULL, NULL);

    return S_OK;
}

static HRESULT WINAPI IShellLinkA_fnSetDescription(IShellLinkA *iface, LPCSTR pszName)
{
    IShellLinkImpl *This = impl_from_IShellLinkA(iface);
    WCHAR *descrW;
    HRESULT hr;

    TRACE("(%p)->(pName=%s)\n", This, debugstr_a(pszName));

    if (pszName)
    {
        descrW = strdupAtoW(pszName);
        if (!descrW) return E_OUTOFMEMORY;
    }
    else
        descrW = NULL;

    hr = IShellLinkW_SetDescription(&This->IShellLinkW_iface, descrW);
    free(descrW);

    return hr;
}

static HRESULT WINAPI IShellLinkA_fnGetWorkingDirectory(IShellLinkA *iface, LPSTR pszDir,
        INT cchMaxPath)
{
    IShellLinkImpl *This = impl_from_IShellLinkA(iface);

    TRACE("(%p)->(%p len=%u)\n", This, pszDir, cchMaxPath);

    if( cchMaxPath )
        pszDir[0] = 0;
    if( This->sWorkDir )
        WideCharToMultiByte( CP_ACP, 0, This->sWorkDir, -1,
                             pszDir, cchMaxPath, NULL, NULL);

    return S_OK;
}

static HRESULT WINAPI IShellLinkA_fnSetWorkingDirectory(IShellLinkA *iface, LPCSTR pszDir)
{
    IShellLinkImpl *This = impl_from_IShellLinkA(iface);
    WCHAR *dirW;
    HRESULT hr;

    TRACE("(%p)->(dir=%s)\n",This, pszDir);

    dirW = strdupAtoW(pszDir);
    if (!dirW) return E_OUTOFMEMORY;

    hr = IShellLinkW_SetWorkingDirectory(&This->IShellLinkW_iface, dirW);
    free(dirW);

    return hr;
}

static HRESULT WINAPI IShellLinkA_fnGetArguments(IShellLinkA *iface, LPSTR pszArgs, INT cchMaxPath)
{
    IShellLinkImpl *This = impl_from_IShellLinkA(iface);

    TRACE("(%p)->(%p len=%u)\n", This, pszArgs, cchMaxPath);

    if( cchMaxPath )
        pszArgs[0] = 0;
    if( This->sArgs )
        WideCharToMultiByte( CP_ACP, 0, This->sArgs, -1,
                             pszArgs, cchMaxPath, NULL, NULL);

    return S_OK;
}

static HRESULT WINAPI IShellLinkA_fnSetArguments(IShellLinkA *iface, LPCSTR pszArgs)
{
    IShellLinkImpl *This = impl_from_IShellLinkA(iface);
    WCHAR *argsW;
    HRESULT hr;

    TRACE("(%p)->(args=%s)\n",This, debugstr_a(pszArgs));

    if (pszArgs)
    {
        argsW = strdupAtoW(pszArgs);
        if (!argsW) return E_OUTOFMEMORY;
    }
    else
        argsW = NULL;

    hr = IShellLinkW_SetArguments(&This->IShellLinkW_iface, argsW);
    free(argsW);

    return hr;
}

static HRESULT WINAPI IShellLinkA_fnGetHotkey(IShellLinkA *iface, WORD *pwHotkey)
{
    IShellLinkImpl *This = impl_from_IShellLinkA(iface);
    return IShellLinkW_GetHotkey(&This->IShellLinkW_iface, pwHotkey);
}

static HRESULT WINAPI IShellLinkA_fnSetHotkey(IShellLinkA *iface, WORD wHotkey)
{
    IShellLinkImpl *This = impl_from_IShellLinkA(iface);
    return IShellLinkW_SetHotkey(&This->IShellLinkW_iface, wHotkey);
}

static HRESULT WINAPI IShellLinkA_fnGetShowCmd(IShellLinkA *iface, INT *piShowCmd)
{
    IShellLinkImpl *This = impl_from_IShellLinkA(iface);
    return IShellLinkW_GetShowCmd(&This->IShellLinkW_iface, piShowCmd);
}

static HRESULT WINAPI IShellLinkA_fnSetShowCmd(IShellLinkA *iface, INT iShowCmd)
{
    IShellLinkImpl *This = impl_from_IShellLinkA(iface);
    return IShellLinkW_SetShowCmd(&This->IShellLinkW_iface, iShowCmd);
}

static HRESULT WINAPI IShellLinkA_fnGetIconLocation(IShellLinkA *iface, LPSTR pszIconPath,
        INT cchIconPath, INT *piIcon)
{
    IShellLinkImpl *This = impl_from_IShellLinkA(iface);

    TRACE("(%p)->(%p len=%u iicon=%p)\n", This, pszIconPath, cchIconPath, piIcon);

    *piIcon = This->iIcoNdx;

    if (This->sIcoPath)
        WideCharToMultiByte(CP_ACP, 0, This->sIcoPath, -1, pszIconPath, cchIconPath, NULL, NULL);
    else
        pszIconPath[0] = 0;

    return S_OK;
}

static HRESULT WINAPI IShellLinkA_fnSetIconLocation(IShellLinkA *iface, LPCSTR path, INT icon)
{
    IShellLinkImpl *This = impl_from_IShellLinkA(iface);
    WCHAR *pathW = NULL;
    HRESULT hr;

    TRACE("(%p)->(path=%s icon=%u)\n", This, debugstr_a(path), icon);

    if (path)
    {
        pathW = strdupAtoW(path);
        if (!pathW)
            return E_OUTOFMEMORY;
    }

    hr = IShellLinkW_SetIconLocation(&This->IShellLinkW_iface, path ? pathW : NULL, icon);
    free(pathW);

    return hr;
}

static HRESULT WINAPI IShellLinkA_fnSetRelativePath(IShellLinkA *iface, LPCSTR pszPathRel,
        DWORD dwReserved)
{
    IShellLinkImpl *This = impl_from_IShellLinkA(iface);
    WCHAR *pathW;
    HRESULT hr;

    TRACE("(%p)->(path=%s %lx)\n",This, pszPathRel, dwReserved);

    pathW = strdupAtoW(pszPathRel);
    if (!pathW) return E_OUTOFMEMORY;

    hr = IShellLinkW_SetRelativePath(&This->IShellLinkW_iface, pathW, dwReserved);
    free(pathW);

    return hr;
}

static HRESULT WINAPI IShellLinkA_fnResolve(IShellLinkA *iface, HWND hwnd, DWORD fFlags)
{
    IShellLinkImpl *This = impl_from_IShellLinkA(iface);

    TRACE("(%p)->(hwnd=%p flags=%lx)\n",This, hwnd, fFlags);

    return IShellLinkW_Resolve(&This->IShellLinkW_iface, hwnd, fFlags);
}

static HRESULT WINAPI IShellLinkA_fnSetPath(IShellLinkA *iface, LPCSTR pszFile)
{
    IShellLinkImpl *This = impl_from_IShellLinkA(iface);
    HRESULT r;
    LPWSTR str;

    TRACE("(%p)->(path=%s)\n",This, debugstr_a(pszFile));

    if (!pszFile) return E_INVALIDARG;

    str = strdupAtoW(pszFile);
    if (!str) return E_OUTOFMEMORY;

    r = IShellLinkW_SetPath(&This->IShellLinkW_iface, str);
    free(str);

    return r;
}

/**************************************************************************
* IShellLink Implementation
*/

static const IShellLinkAVtbl slvt =
{
    IShellLinkA_fnQueryInterface,
    IShellLinkA_fnAddRef,
    IShellLinkA_fnRelease,
    IShellLinkA_fnGetPath,
    IShellLinkA_fnGetIDList,
    IShellLinkA_fnSetIDList,
    IShellLinkA_fnGetDescription,
    IShellLinkA_fnSetDescription,
    IShellLinkA_fnGetWorkingDirectory,
    IShellLinkA_fnSetWorkingDirectory,
    IShellLinkA_fnGetArguments,
    IShellLinkA_fnSetArguments,
    IShellLinkA_fnGetHotkey,
    IShellLinkA_fnSetHotkey,
    IShellLinkA_fnGetShowCmd,
    IShellLinkA_fnSetShowCmd,
    IShellLinkA_fnGetIconLocation,
    IShellLinkA_fnSetIconLocation,
    IShellLinkA_fnSetRelativePath,
    IShellLinkA_fnResolve,
    IShellLinkA_fnSetPath
};


/**************************************************************************
 *  IShellLinkW_fnQueryInterface
 */
static HRESULT WINAPI IShellLinkW_fnQueryInterface(
  IShellLinkW * iface, REFIID riid, LPVOID *ppvObj)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_guid(riid));

    *ppvObj = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IShellLinkA))
    {
        *ppvObj = &This->IShellLinkA_iface;
    }
    else if(IsEqualIID(riid, &IID_IShellLinkW))
    {
        *ppvObj = &This->IShellLinkW_iface;
    }
    else if(IsEqualIID(riid, &IID_IPersistFile))
    {
        *ppvObj = &This->IPersistFile_iface;
    }
    else if(IsEqualIID(riid, &IID_IPersistStream))
    {
        *ppvObj = &This->IPersistStream_iface;
    }
    else if(IsEqualIID(riid, &IID_IShellLinkDataList))
    {
        *ppvObj = &This->IShellLinkDataList_iface;
    }
    else if(IsEqualIID(riid, &IID_IShellExtInit))
    {
        *ppvObj = &This->IShellExtInit_iface;
    }
    else if(IsEqualIID(riid, &IID_IContextMenu))
    {
        *ppvObj = &This->IContextMenu_iface;
    }
    else if(IsEqualIID(riid, &IID_IObjectWithSite))
    {
        *ppvObj = &This->IObjectWithSite_iface;
    }
    else if(IsEqualIID(riid, &IID_IPropertyStore))
    {
        *ppvObj = &This->IPropertyStore_iface;
    }

    if(*ppvObj)
    {
        IUnknown_AddRef((IUnknown*)*ppvObj);
        TRACE("-- Interface: (%p)->(%p)\n", ppvObj, *ppvObj);
        return S_OK;
    }
    ERR("-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

/******************************************************************************
 * IShellLinkW_fnAddRef
 */
static ULONG WINAPI IShellLinkW_fnAddRef(IShellLinkW * iface)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(count=%lu)\n", This, ref - 1);

    return ref;
}

/******************************************************************************
 * IShellLinkW_fnRelease
 */
static ULONG WINAPI IShellLinkW_fnRelease(IShellLinkW * iface)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(count=%lu)\n", This, refCount + 1);

    if (refCount)
        return refCount;

    TRACE("-- destroying IShellLink(%p)\n",This);

    free(This->sIcoPath);
    free(This->sArgs);
    free(This->sWorkDir);
    free(This->sDescription);
    free(This->sPath);
    free(This->sPathRel);
    free(This->sProduct);
    free(This->sComponent);
    free(This->filepath);

    if (This->site)
        IUnknown_Release( This->site );

    if (This->pPidl)
        ILFree(This->pPidl);

    LocalFree(This);

    return 0;
}

static HRESULT WINAPI IShellLinkW_fnGetPath(IShellLinkW * iface, LPWSTR pszFile,INT cchMaxPath, WIN32_FIND_DATAW *pfd, DWORD fFlags)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);
    HRESULT res = S_OK;

    TRACE("(%p)->(pfile=%p len=%u find_data=%p flags=%lu)(%s)\n",
          This, pszFile, cchMaxPath, pfd, fFlags, debugstr_w(This->sPath));

    if (This->sComponent || This->sProduct)
        return S_FALSE;

    if (cchMaxPath)
        pszFile[0] = 0;
    if (This->sPath)
        lstrcpynW( pszFile, This->sPath, cchMaxPath );
    else
        res = S_FALSE;

    if (pfd)
    {
        memset(pfd, 0, sizeof(*pfd));

        if (res == S_OK)
        {
            WCHAR path[MAX_PATH];
            WIN32_FILE_ATTRIBUTE_DATA fad;

            if (GetFileAttributesExW(This->sPath, GetFileExInfoStandard, &fad))
            {
                pfd->dwFileAttributes = fad.dwFileAttributes;
                pfd->ftCreationTime = fad.ftCreationTime;
                pfd->ftLastAccessTime = fad.ftLastAccessTime;
                pfd->ftLastWriteTime = fad.ftLastWriteTime;
                pfd->nFileSizeHigh = fad.nFileSizeHigh;
                pfd->nFileSizeLow = fad.nFileSizeLow;
            }

            lstrcpyW(pfd->cFileName, PathFindFileNameW(This->sPath));

            if (GetShortPathNameW(This->sPath, path, MAX_PATH))
            {
                lstrcpyW(pfd->cAlternateFileName, PathFindFileNameW(path));
            }
        }

        TRACE("attr 0x%08lx size 0x%08lx%08lx name %s shortname %s\n", pfd->dwFileAttributes,
            pfd->nFileSizeHigh, pfd->nFileSizeLow, wine_dbgstr_w(pfd->cFileName),
            wine_dbgstr_w(pfd->cAlternateFileName));
    }

    return res;
}

static HRESULT WINAPI IShellLinkW_fnGetIDList(IShellLinkW * iface, LPITEMIDLIST * ppidl)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(ppidl=%p)\n",This, ppidl);

    if (!This->pPidl)
    {
	*ppidl = NULL;
        return S_FALSE;
    }
    *ppidl = ILClone(This->pPidl);
    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnSetIDList(IShellLinkW * iface, LPCITEMIDLIST pidl)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);
    WCHAR path[MAX_PATH];

    TRACE("(%p)->(pidl=%p)\n",This, pidl);

    if( This->pPidl )
        ILFree( This->pPidl );
    This->pPidl = ILClone( pidl );
    if( !This->pPidl )
        return E_FAIL;

    free( This->sPath );
    This->sPath = NULL;

    if ( SHGetPathFromIDListW( pidl, path ) )
    {
        This->sPath = wcsdup(path);
        if (!This->sPath)
            return E_OUTOFMEMORY;
    }

    This->bDirty = TRUE;

    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnGetDescription(IShellLinkW * iface, LPWSTR pszName,INT cchMaxName)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(%p len=%u)\n",This, pszName, cchMaxName);

    pszName[0] = 0;
    if( This->sDescription )
        lstrcpynW( pszName, This->sDescription, cchMaxName );

    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnSetDescription(IShellLinkW * iface, LPCWSTR pszName)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(desc=%s)\n",This, debugstr_w(pszName));

    free(This->sDescription);
    if (pszName)
    {
        This->sDescription = wcsdup(pszName);
        if ( !This->sDescription )
            return E_OUTOFMEMORY;
    }
    else
        This->sDescription = NULL;
    This->bDirty = TRUE;

    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnGetWorkingDirectory(IShellLinkW * iface, LPWSTR pszDir,INT cchMaxPath)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(%p len %u)\n", This, pszDir, cchMaxPath);

    if( cchMaxPath )
        pszDir[0] = 0;
    if( This->sWorkDir )
        lstrcpynW( pszDir, This->sWorkDir, cchMaxPath );

    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnSetWorkingDirectory(IShellLinkW * iface, LPCWSTR pszDir)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(dir=%s)\n",This, debugstr_w(pszDir));

    free(This->sWorkDir);
    This->sWorkDir = wcsdup(pszDir);
    if ( !This->sWorkDir )
        return E_OUTOFMEMORY;
    This->bDirty = TRUE;

    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnGetArguments(IShellLinkW * iface, LPWSTR pszArgs,INT cchMaxPath)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(%p len=%u)\n", This, pszArgs, cchMaxPath);

    if( cchMaxPath )
        pszArgs[0] = 0;
    if( This->sArgs )
        lstrcpynW( pszArgs, This->sArgs, cchMaxPath );

    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnSetArguments(IShellLinkW * iface, LPCWSTR pszArgs)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(args=%s)\n",This, debugstr_w(pszArgs));

    free(This->sArgs);
    if (pszArgs)
    {
        This->sArgs = wcsdup(pszArgs);
        if ( !This->sArgs )
            return E_OUTOFMEMORY;
    }
    else This->sArgs = NULL;

    This->bDirty = TRUE;

    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnGetHotkey(IShellLinkW * iface, WORD *pwHotkey)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(%p)\n",This, pwHotkey);

    *pwHotkey=This->wHotKey;

    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnSetHotkey(IShellLinkW * iface, WORD wHotkey)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(hotkey=%x)\n",This, wHotkey);

    This->wHotKey = wHotkey;
    This->bDirty = TRUE;

    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnGetShowCmd(IShellLinkW * iface, INT *piShowCmd)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(%p)\n",This, piShowCmd);

    *piShowCmd = This->iShowCmd;

    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnSetShowCmd(IShellLinkW * iface, INT iShowCmd)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(%d)\n", This, iShowCmd);

    This->iShowCmd = iShowCmd;
    This->bDirty = TRUE;

    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnGetIconLocation(IShellLinkW * iface, LPWSTR pszIconPath,INT cchIconPath,INT *piIcon)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(%p len=%u iicon=%p)\n", This, pszIconPath, cchIconPath, piIcon);

    *piIcon = This->iIcoNdx;

    if (This->sIcoPath)
	lstrcpynW(pszIconPath, This->sIcoPath, cchIconPath);
    else
	pszIconPath[0] = 0;

    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnSetIconLocation(IShellLinkW * iface, const WCHAR *path, INT icon)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(path=%s icon=%u)\n", This, debugstr_w(path), icon);

    free(This->sIcoPath);
    if (path)
    {
        This->sIcoPath = wcsdup(path);
        if (!This->sIcoPath)
            return E_OUTOFMEMORY;
    }
    else
        This->sIcoPath = NULL;
    This->iIcoNdx = icon;
    This->bDirty = TRUE;

    return S_OK;
}

static HRESULT WINAPI IShellLinkW_fnSetRelativePath(IShellLinkW * iface, LPCWSTR pszPathRel, DWORD dwReserved)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(path=%s %lx)\n",This, debugstr_w(pszPathRel), dwReserved);

    free(This->sPathRel);
    This->sPathRel = wcsdup(pszPathRel);
    if ( !This->sPathRel )
        return E_OUTOFMEMORY;
    This->bDirty = TRUE;

    return ShellLink_UpdatePath(This->sPathRel, This->sPath, This->sWorkDir, &This->sPath);
}

static HRESULT WINAPI IShellLinkW_fnResolve(IShellLinkW * iface, HWND hwnd, DWORD fFlags)
{
    HRESULT hr = S_OK;
    BOOL bSuccess;

    IShellLinkImpl *This = impl_from_IShellLinkW(iface);

    TRACE("(%p)->(hwnd=%p flags=%lx)\n",This, hwnd, fFlags);

    /*FIXME: use IResolveShellLink interface */

    if (!This->sPath && This->pPidl) {
	WCHAR buffer[MAX_PATH];

	bSuccess = SHGetPathFromIDListW(This->pPidl, buffer);

	if (bSuccess && *buffer) {
	    This->sPath = wcsdup(buffer);
	    if (!This->sPath)
		return E_OUTOFMEMORY;

	    This->bDirty = TRUE;
	} else
	    hr = S_OK;    /* don't report an error occurred while just caching information */
    }

    if (!This->sIcoPath && This->sPath) {
	This->sIcoPath = wcsdup(This->sPath);
	if (!This->sIcoPath)
	    return E_OUTOFMEMORY;

	This->iIcoNdx = 0;

	This->bDirty = TRUE;
    }

    return hr;
}

static LPWSTR ShellLink_GetAdvertisedArg(LPCWSTR str)
{
    LPWSTR ret;
    LPCWSTR p;
    DWORD len;

    if( !str )
        return NULL;

    p = wcschr( str, ':' );
    if( !p )
        return NULL;
    len = p - str;
    ret = malloc(sizeof(WCHAR) * (len + 1));
    if( !ret )
        return ret;
    memcpy( ret, str, sizeof(WCHAR)*len );
    ret[len] = 0;
    return ret;
}

static HRESULT ShellLink_SetAdvertiseInfo(IShellLinkImpl *This, LPCWSTR str)
{
    LPCWSTR szComponent = NULL, szProduct = NULL, p;
    WCHAR szGuid[39];
    HRESULT r;
    GUID guid;
    int len;

    while( str[0] )
    {
        /* each segment must start with two colons */
        if( str[0] != ':' || str[1] != ':' )
            return E_FAIL;

        /* the last segment is just two colons */
        if( !str[2] )
            break;
        str += 2;

        /* there must be a colon straight after a guid */
        p = wcschr( str, ':' );
        if( !p )
            return E_FAIL;
        len = p - str;
        if( len != 38 )
            return E_FAIL;

        /* get the guid, and check it's validly formatted */
        memcpy( szGuid, str, sizeof(WCHAR)*len );
        szGuid[len] = 0;
        r = CLSIDFromString( szGuid, &guid );
        if( r != S_OK )
            return r;
        str = p + 1;

        /* match it up to a guid that we care about */
        if( IsEqualGUID( &guid, &SHELL32_AdvtShortcutComponent ) && !szComponent )
            szComponent = str;
        else if( IsEqualGUID( &guid, &SHELL32_AdvtShortcutProduct ) && !szProduct )
            szProduct = str;
        else
            return E_FAIL;

        /* skip to the next field */
        str = wcschr( str, ':' );
        if( !str )
            return E_FAIL;
    }

    /* we have to have a component for an advertised shortcut */
    if( !szComponent )
        return E_FAIL;

    This->sComponent = ShellLink_GetAdvertisedArg( szComponent );
    This->sProduct = ShellLink_GetAdvertisedArg( szProduct );

    TRACE("Component = %s\n", debugstr_w(This->sComponent));
    TRACE("Product = %s\n", debugstr_w(This->sProduct));

    return S_OK;
}

static BOOL ShellLink_GetVolumeInfo(LPCWSTR path, volume_info *volume)
{
    const int label_sz = ARRAY_SIZE(volume->label);
    WCHAR drive[] = { path[0], ':', '\\', 0 };
    BOOL r;

    volume->type = GetDriveTypeW(drive);
    r = GetVolumeInformationW(drive, volume->label, label_sz,
                              &volume->serial, NULL, NULL, NULL, 0);
    TRACE("r = %d type %ld serial %08lx name %s\n", r,
          volume->type, volume->serial, debugstr_w(volume->label));
    return r;
}

static HRESULT WINAPI IShellLinkW_fnSetPath(IShellLinkW * iface, LPCWSTR pszFile)
{
    IShellLinkImpl *This = impl_from_IShellLinkW(iface);
    WCHAR buffer[MAX_PATH];
    LPWSTR fname, unquoted = NULL;
    HRESULT hr = S_OK;
    UINT len;

    TRACE("(%p)->(path=%s)\n",This, debugstr_w(pszFile));

    if (!pszFile) return E_INVALIDARG;

    /* quotes at the ends of the string are stripped */
    len = lstrlenW(pszFile);
    if (pszFile[0] == '"' && pszFile[len-1] == '"')
    {
        unquoted = wcsdup(pszFile);
        PathUnquoteSpacesW(unquoted);
        pszFile = unquoted;
    }

    /* any other quote marks are invalid */
    if (wcschr(pszFile, '"'))
    {
        free(unquoted);
        return S_FALSE;
    }

    free(This->sPath);
    This->sPath = NULL;

    free(This->sComponent);
    This->sComponent = NULL;

    if (This->pPidl)
        ILFree(This->pPidl);
    This->pPidl = NULL;

    if (S_OK != ShellLink_SetAdvertiseInfo( This, pszFile ))
    {
        if (*pszFile == '\0')
            *buffer = '\0';
        else if (!GetFullPathNameW(pszFile, MAX_PATH, buffer, &fname))
        {
            free(unquoted);
            return E_FAIL;
        }
        else if(!PathFileExistsW(buffer) &&
		!SearchPathW(NULL, pszFile, NULL, MAX_PATH, buffer, NULL))
	  hr = S_FALSE;

        This->pPidl = SHSimpleIDListFromPathW(pszFile);
        ShellLink_GetVolumeInfo(buffer, &This->volume);

        This->sPath = wcsdup(buffer);
        if (!This->sPath)
        {
            free(unquoted);
            return E_OUTOFMEMORY;
        }
    }
    This->bDirty = TRUE;
    free(unquoted);

    return hr;
}

/**************************************************************************
* IShellLinkW Implementation
*/

static const IShellLinkWVtbl slvtw =
{
    IShellLinkW_fnQueryInterface,
    IShellLinkW_fnAddRef,
    IShellLinkW_fnRelease,
    IShellLinkW_fnGetPath,
    IShellLinkW_fnGetIDList,
    IShellLinkW_fnSetIDList,
    IShellLinkW_fnGetDescription,
    IShellLinkW_fnSetDescription,
    IShellLinkW_fnGetWorkingDirectory,
    IShellLinkW_fnSetWorkingDirectory,
    IShellLinkW_fnGetArguments,
    IShellLinkW_fnSetArguments,
    IShellLinkW_fnGetHotkey,
    IShellLinkW_fnSetHotkey,
    IShellLinkW_fnGetShowCmd,
    IShellLinkW_fnSetShowCmd,
    IShellLinkW_fnGetIconLocation,
    IShellLinkW_fnSetIconLocation,
    IShellLinkW_fnSetRelativePath,
    IShellLinkW_fnResolve,
    IShellLinkW_fnSetPath
};

static HRESULT WINAPI
ShellLink_DataList_QueryInterface( IShellLinkDataList* iface, REFIID riid, void** ppvObject)
{
    IShellLinkImpl *This = impl_from_IShellLinkDataList(iface);
    return IShellLinkW_QueryInterface(&This->IShellLinkW_iface, riid, ppvObject);
}

static ULONG WINAPI
ShellLink_DataList_AddRef( IShellLinkDataList* iface )
{
    IShellLinkImpl *This = impl_from_IShellLinkDataList(iface);
    return IShellLinkW_AddRef(&This->IShellLinkW_iface);
}

static ULONG WINAPI
ShellLink_DataList_Release( IShellLinkDataList* iface )
{
    IShellLinkImpl *This = impl_from_IShellLinkDataList(iface);
    return IShellLinkW_Release(&This->IShellLinkW_iface);
}

static HRESULT WINAPI
ShellLink_AddDataBlock( IShellLinkDataList* iface, void* pDataBlock )
{
    FIXME("(%p)->(%p): stub\n", iface, pDataBlock);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ShellLink_CopyDataBlock( IShellLinkDataList* iface, DWORD dwSig, void** ppDataBlock )
{
    IShellLinkImpl *This = impl_from_IShellLinkDataList(iface);
    LPVOID block = NULL;
    HRESULT r = E_FAIL;

    TRACE("%p %08lx %p\n", iface, dwSig, ppDataBlock );

    switch (dwSig)
    {
    case EXP_DARWIN_ID_SIG:
        if (!This->sComponent)
            break;
        block = shelllink_build_darwinid( This->sComponent, dwSig );
        r = S_OK;
        break;
    case EXP_SZ_LINK_SIG:
    case NT_CONSOLE_PROPS_SIG:
    case NT_FE_CONSOLE_PROPS_SIG:
    case EXP_SPECIAL_FOLDER_SIG:
    case EXP_SZ_ICON_SIG:
        FIXME("valid but unhandled datablock %08lx\n", dwSig);
        break;
    default:
        ERR("unknown datablock %08lx\n", dwSig);
    }
    *ppDataBlock = block;
    return r;
}

static HRESULT WINAPI
ShellLink_RemoveDataBlock( IShellLinkDataList* iface, DWORD dwSig )
{
    FIXME("(%p)->(%lu): stub\n", iface, dwSig);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ShellLink_GetFlags( IShellLinkDataList* iface, DWORD* pdwFlags )
{
    IShellLinkImpl *This = impl_from_IShellLinkDataList(iface);
    DWORD flags = 0;

    FIXME("(%p)->(%p): partially implemented\n", This, pdwFlags);

    /* FIXME: add more */
    if (This->sArgs)
        flags |= SLDF_HAS_ARGS;
    if (This->sComponent)
        flags |= SLDF_HAS_DARWINID;
    if (This->sIcoPath)
        flags |= SLDF_HAS_ICONLOCATION;
    if (This->sProduct)
        flags |= SLDF_HAS_LOGO3ID;
    if (This->pPidl)
        flags |= SLDF_HAS_ID_LIST;

    *pdwFlags = flags;

    return S_OK;
}

static HRESULT WINAPI
ShellLink_SetFlags( IShellLinkDataList* iface, DWORD dwFlags )
{
    FIXME("(%p)->(%lu): stub\n", iface, dwFlags);
    return S_OK;
}

static const IShellLinkDataListVtbl dlvt =
{
    ShellLink_DataList_QueryInterface,
    ShellLink_DataList_AddRef,
    ShellLink_DataList_Release,
    ShellLink_AddDataBlock,
    ShellLink_CopyDataBlock,
    ShellLink_RemoveDataBlock,
    ShellLink_GetFlags,
    ShellLink_SetFlags
};

static HRESULT WINAPI
ShellLink_ExtInit_QueryInterface( IShellExtInit* iface, REFIID riid, void** ppvObject )
{
    IShellLinkImpl *This = impl_from_IShellExtInit(iface);
    return IShellLinkW_QueryInterface(&This->IShellLinkW_iface, riid, ppvObject);
}

static ULONG WINAPI
ShellLink_ExtInit_AddRef( IShellExtInit* iface )
{
    IShellLinkImpl *This = impl_from_IShellExtInit(iface);
    return IShellLinkW_AddRef(&This->IShellLinkW_iface);
}

static ULONG WINAPI
ShellLink_ExtInit_Release( IShellExtInit* iface )
{
    IShellLinkImpl *This = impl_from_IShellExtInit(iface);
    return IShellLinkW_Release(&This->IShellLinkW_iface);
}

/**************************************************************************
 * ShellLink implementation of IShellExtInit::Initialize()
 *
 * Loads the shelllink from the dataobject the shell is pointing to.
 */
static HRESULT WINAPI
ShellLink_ExtInit_Initialize( IShellExtInit* iface, LPCITEMIDLIST pidlFolder,
                              IDataObject *pdtobj, HKEY hkeyProgID )
{
    IShellLinkImpl *This = impl_from_IShellExtInit(iface);
    FORMATETC format;
    STGMEDIUM stgm;
    UINT count;
    HRESULT r = E_FAIL;

    TRACE("%p %p %p %p\n", This, pidlFolder, pdtobj, hkeyProgID );

    if( !pdtobj )
        return r;

    format.cfFormat = CF_HDROP;
    format.ptd = NULL;
    format.dwAspect = DVASPECT_CONTENT;
    format.lindex = -1;
    format.tymed = TYMED_HGLOBAL;

    if( FAILED( IDataObject_GetData( pdtobj, &format, &stgm ) ) )
        return r;

    count = DragQueryFileW( stgm.hGlobal, -1, NULL, 0 );
    if( count == 1 )
    {
        LPWSTR path;

        count = DragQueryFileW( stgm.hGlobal, 0, NULL, 0 );
        count++;
        path = malloc( count * sizeof(WCHAR) );
        if( path )
        {
            IPersistFile *pf = &This->IPersistFile_iface;

            count = DragQueryFileW( stgm.hGlobal, 0, path, count );
            r = IPersistFile_Load( pf, path, 0 );
            free( path );
        }
    }
    ReleaseStgMedium( &stgm );

    return r;
}

static const IShellExtInitVtbl eivt =
{
    ShellLink_ExtInit_QueryInterface,
    ShellLink_ExtInit_AddRef,
    ShellLink_ExtInit_Release,
    ShellLink_ExtInit_Initialize
};

static HRESULT WINAPI
ShellLink_ContextMenu_QueryInterface( IContextMenu* iface, REFIID riid, void** ppvObject )
{
    IShellLinkImpl *This = impl_from_IContextMenu(iface);
    return IShellLinkW_QueryInterface(&This->IShellLinkW_iface, riid, ppvObject);
}

static ULONG WINAPI
ShellLink_ContextMenu_AddRef( IContextMenu* iface )
{
    IShellLinkImpl *This = impl_from_IContextMenu(iface);
    return IShellLinkW_AddRef(&This->IShellLinkW_iface);
}

static ULONG WINAPI
ShellLink_ContextMenu_Release( IContextMenu* iface )
{
    IShellLinkImpl *This = impl_from_IContextMenu(iface);
    return IShellLinkW_Release(&This->IShellLinkW_iface);
}

static HRESULT WINAPI
ShellLink_QueryContextMenu( IContextMenu* iface, HMENU hmenu, UINT indexMenu,
                            UINT idCmdFirst, UINT idCmdLast, UINT uFlags )
{
    IShellLinkImpl *This = impl_from_IContextMenu(iface);
    MENUITEMINFOW mii;
    int id = 1;

    TRACE("%p %p %u %u %u %u\n", This,
          hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags );

    if ( !hmenu )
        return E_INVALIDARG;

    memset( &mii, 0, sizeof mii );
    mii.cbSize = sizeof mii;
    mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
    mii.dwTypeData = (LPWSTR)L"Open";
    mii.cch = lstrlenW( mii.dwTypeData );
    mii.wID = idCmdFirst + id++;
    mii.fState = MFS_DEFAULT | MFS_ENABLED;
    mii.fType = MFT_STRING;
    if (!InsertMenuItemW( hmenu, indexMenu, TRUE, &mii ))
        return E_FAIL;
    This->iIdOpen = 0;

    return MAKE_HRESULT( SEVERITY_SUCCESS, 0, id );
}

static LPWSTR
shelllink_get_msi_component_path( LPWSTR component )
{
    LPWSTR path;
    DWORD r, sz = 0;

    r = CommandLineFromMsiDescriptor( component, NULL, &sz );
    if (r != ERROR_SUCCESS)
         return NULL;

    sz++;
    path = malloc( sz * sizeof(WCHAR) );
    r = CommandLineFromMsiDescriptor( component, path, &sz );
    if (r != ERROR_SUCCESS)
    {
        free( path );
        path = NULL;
    }

    TRACE("returning %s\n", debugstr_w( path ) );

    return path;
}

static HRESULT WINAPI
ShellLink_InvokeCommand( IContextMenu* iface, LPCMINVOKECOMMANDINFO lpici )
{
    IShellLinkImpl *This = impl_from_IContextMenu(iface);
    SHELLEXECUTEINFOW sei;
    HWND hwnd = NULL; /* FIXME: get using interface set from IObjectWithSite */
    LPWSTR args = NULL;
    LPWSTR path = NULL;
    HRESULT r;

    TRACE("%p %p\n", This, lpici );

    if ( lpici->cbSize < sizeof (CMINVOKECOMMANDINFO) )
        return E_INVALIDARG;

    if ( lpici->lpVerb != MAKEINTRESOURCEA(This->iIdOpen) )
    {
        ERR("Unknown id %p != %d\n", lpici->lpVerb, This->iIdOpen );
        return E_INVALIDARG;
    }

    r = IShellLinkW_Resolve(&This->IShellLinkW_iface, hwnd, 0);
    if ( FAILED( r ) )
        return r;

    if ( This->sComponent )
    {
        path = shelllink_get_msi_component_path( This->sComponent );
        if (!path)
            return E_FAIL;
    }
    else
        path = wcsdup( This->sPath );

    if ( lpici->cbSize == sizeof (CMINVOKECOMMANDINFOEX) &&
         ( lpici->fMask & CMIC_MASK_UNICODE ) )
    {
        LPCMINVOKECOMMANDINFOEX iciex = (LPCMINVOKECOMMANDINFOEX) lpici;
        DWORD len = 2;

        if ( This->sArgs )
            len += lstrlenW( This->sArgs );
        if ( iciex->lpParametersW )
            len += lstrlenW( iciex->lpParametersW );

        args = malloc( len * sizeof(WCHAR) );
        args[0] = 0;
        if ( This->sArgs )
            lstrcatW( args, This->sArgs );
        if ( iciex->lpParametersW && iciex->lpParametersW[0] )
        {
            lstrcatW( args, L" " );
            lstrcatW( args, iciex->lpParametersW );
        }
    }

    memset( &sei, 0, sizeof sei );
    sei.cbSize = sizeof sei;
    sei.fMask = SEE_MASK_UNICODE | (lpici->fMask & (SEE_MASK_NOASYNC|SEE_MASK_NO_CONSOLE|SEE_MASK_ASYNCOK|SEE_MASK_FLAG_NO_UI));
    sei.lpFile = path;
    sei.nShow = This->iShowCmd;
    sei.lpIDList = This->pPidl;
    sei.lpDirectory = This->sWorkDir;
    sei.lpParameters = args;
    sei.lpVerb = L"Open";

    if( ShellExecuteExW( &sei ) )
        r = S_OK;
    else
        r = E_FAIL;

    free( args );
    free( path );

    return r;
}

static HRESULT WINAPI
ShellLink_GetCommandString( IContextMenu* iface, UINT_PTR idCmd, UINT uType,
                            UINT* pwReserved, LPSTR pszName, UINT cchMax )
{
    IShellLinkImpl *This = impl_from_IContextMenu(iface);

    FIXME("(%p)->(%Iu %u %p %p %u): stub\n", This,
          idCmd, uType, pwReserved, pszName, cchMax );

    return E_NOTIMPL;
}

static const IContextMenuVtbl cmvt =
{
    ShellLink_ContextMenu_QueryInterface,
    ShellLink_ContextMenu_AddRef,
    ShellLink_ContextMenu_Release,
    ShellLink_QueryContextMenu,
    ShellLink_InvokeCommand,
    ShellLink_GetCommandString
};

static HRESULT WINAPI
ShellLink_ObjectWithSite_QueryInterface( IObjectWithSite* iface, REFIID riid, void** ppvObject )
{
    IShellLinkImpl *This = impl_from_IObjectWithSite(iface);
    return IShellLinkW_QueryInterface(&This->IShellLinkW_iface, riid, ppvObject );
}

static ULONG WINAPI
ShellLink_ObjectWithSite_AddRef( IObjectWithSite* iface )
{
    IShellLinkImpl *This = impl_from_IObjectWithSite(iface);
    return IShellLinkW_AddRef(&This->IShellLinkW_iface);
}

static ULONG WINAPI
ShellLink_ObjectWithSite_Release( IObjectWithSite* iface )
{
    IShellLinkImpl *This = impl_from_IObjectWithSite(iface);
    return IShellLinkW_Release(&This->IShellLinkW_iface);
}

static HRESULT WINAPI
ShellLink_GetSite( IObjectWithSite *iface, REFIID iid, void ** ppvSite )
{
    IShellLinkImpl *This = impl_from_IObjectWithSite(iface);

    TRACE("%p %s %p\n", This, debugstr_guid( iid ), ppvSite );

    if ( !This->site )
        return E_FAIL;
    return IUnknown_QueryInterface( This->site, iid, ppvSite );
}

static HRESULT WINAPI
ShellLink_SetSite( IObjectWithSite *iface, IUnknown *punk )
{
    IShellLinkImpl *This = impl_from_IObjectWithSite(iface);

    TRACE("%p %p\n", iface, punk);

    if ( punk )
        IUnknown_AddRef( punk );

    if( This->site )
        IUnknown_Release( This->site );

    This->site = punk;

    return S_OK;
}

static const IObjectWithSiteVtbl owsvt =
{
    ShellLink_ObjectWithSite_QueryInterface,
    ShellLink_ObjectWithSite_AddRef,
    ShellLink_ObjectWithSite_Release,
    ShellLink_SetSite,
    ShellLink_GetSite,
};

static HRESULT WINAPI propertystore_QueryInterface(IPropertyStore *iface, REFIID riid, void **obj)
{
    IShellLinkImpl *This = impl_from_IPropertyStore(iface);
    return IShellLinkW_QueryInterface(&This->IShellLinkW_iface, riid, obj);
}

static ULONG WINAPI propertystore_AddRef(IPropertyStore *iface)
{
    IShellLinkImpl *This = impl_from_IPropertyStore(iface);
    return IShellLinkW_AddRef(&This->IShellLinkW_iface);
}

static ULONG WINAPI propertystore_Release(IPropertyStore *iface)
{
    IShellLinkImpl *This = impl_from_IPropertyStore(iface);
    return IShellLinkW_Release(&This->IShellLinkW_iface);
}

static HRESULT WINAPI propertystore_GetCount(IPropertyStore *iface, DWORD *props)
{
    IShellLinkImpl *This = impl_from_IPropertyStore(iface);
    FIXME("(%p)->(%p): stub\n", This, props);
    return E_NOTIMPL;
}

static HRESULT WINAPI propertystore_GetAt(IPropertyStore *iface, DWORD propid, PROPERTYKEY *key)
{
    IShellLinkImpl *This = impl_from_IPropertyStore(iface);
    FIXME("(%p)->(%ld %p): stub\n", This, propid, key);
    return E_NOTIMPL;
}

static HRESULT WINAPI propertystore_GetValue(IPropertyStore *iface, REFPROPERTYKEY key, PROPVARIANT *value)
{
    IShellLinkImpl *This = impl_from_IPropertyStore(iface);
    FIXME("(%p)->(%p %p): stub\n", This, key, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI propertystore_SetValue(IPropertyStore *iface, REFPROPERTYKEY key, REFPROPVARIANT value)
{
    IShellLinkImpl *This = impl_from_IPropertyStore(iface);
    FIXME("(%p)->(%p %p): stub\n", This, key, value);
    return S_OK;
}

static HRESULT WINAPI propertystore_Commit(IPropertyStore *iface)
{
    IShellLinkImpl *This = impl_from_IPropertyStore(iface);
    FIXME("(%p): stub\n", This);
    return S_OK;
}

static const IPropertyStoreVtbl propertystorevtbl = {
    propertystore_QueryInterface,
    propertystore_AddRef,
    propertystore_Release,
    propertystore_GetCount,
    propertystore_GetAt,
    propertystore_GetValue,
    propertystore_SetValue,
    propertystore_Commit
};

HRESULT WINAPI IShellLink_Constructor(IUnknown *outer, REFIID riid, void **obj)
{
    IShellLinkImpl * sl;
    HRESULT r;

    TRACE("outer=%p riid=%s\n", outer, debugstr_guid(riid));

    *obj = NULL;

    if (outer)
        return CLASS_E_NOAGGREGATION;

    sl = LocalAlloc(LMEM_ZEROINIT,sizeof(IShellLinkImpl));
    if (!sl)
        return E_OUTOFMEMORY;

    sl->ref = 1;
    sl->IShellLinkA_iface.lpVtbl = &slvt;
    sl->IShellLinkW_iface.lpVtbl = &slvtw;
    sl->IPersistFile_iface.lpVtbl = &pfvt;
    sl->IPersistStream_iface.lpVtbl = &psvt;
    sl->IShellLinkDataList_iface.lpVtbl = &dlvt;
    sl->IShellExtInit_iface.lpVtbl = &eivt;
    sl->IContextMenu_iface.lpVtbl = &cmvt;
    sl->IObjectWithSite_iface.lpVtbl = &owsvt;
    sl->IPropertyStore_iface.lpVtbl = &propertystorevtbl;
    sl->iShowCmd = SW_SHOWNORMAL;
    sl->bDirty = FALSE;
    sl->iIdOpen = -1;
    sl->site = NULL;
    sl->filepath = NULL;

    TRACE("(%p)\n", sl);

    r = IShellLinkW_QueryInterface( &sl->IShellLinkW_iface, riid, obj );
    IShellLinkW_Release( &sl->IShellLinkW_iface );
    return r;
}
