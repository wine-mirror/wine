/*
 * CHM Utility API
 *
 * Copyright 2005 James Hawkins
 * Copyright 2007 Jacek Caban
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

#include "hhctrl.h"
#include "stream.h"

#include "winreg.h"
#include "shlwapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(htmlhelp);

/* Reads a string from the #STRINGS section in the CHM file */
static LPCSTR GetChmString(CHMInfo *chm, DWORD offset)
{
    LPCSTR str;
    char **new_strings;
    DWORD new_strings_size;

    if(!chm->strings_stream)
        return NULL;

    if(chm->strings_size <= (offset >> BLOCK_BITS)) {
        new_strings_size = (offset >> BLOCK_BITS) + 1;
        new_strings = realloc(chm->strings, new_strings_size * sizeof(char*));
        if(!new_strings)
            return NULL;
        memset(new_strings + chm->strings_size, 0,
                (new_strings_size - chm->strings_size) * sizeof(char*));
        chm->strings = new_strings;
        chm->strings_size = new_strings_size;
    }

    if(!chm->strings[offset >> BLOCK_BITS]) {
        LARGE_INTEGER pos;
        DWORD read;
        HRESULT hres;

        pos.QuadPart = offset & ~BLOCK_MASK;
        hres = IStream_Seek(chm->strings_stream, pos, STREAM_SEEK_SET, NULL);
        if(FAILED(hres)) {
            WARN("Seek failed: %08lx\n", hres);
            return NULL;
        }

        chm->strings[offset >> BLOCK_BITS] = malloc(BLOCK_SIZE);

        hres = IStream_Read(chm->strings_stream, chm->strings[offset >> BLOCK_BITS],
                            BLOCK_SIZE, &read);
        if(FAILED(hres)) {
            WARN("Read failed: %08lx\n", hres);
            free(chm->strings[offset >> BLOCK_BITS]);
            chm->strings[offset >> BLOCK_BITS] = NULL;
            return NULL;
        }
    }

    str = chm->strings[offset >> BLOCK_BITS] + (offset & BLOCK_MASK);
    TRACE("offset %#lx => %s\n", offset, debugstr_a(str));
    return str;
}

static BOOL ReadChmSystem(CHMInfo *chm)
{
    IStream *stream;
    DWORD ver=0xdeadbeef, read, buf_size;
    char *buf, *new_buf;
    HRESULT hres;

    struct {
        WORD code;
        WORD len;
    } entry;

    hres = IStorage_OpenStream(chm->pStorage, L"#SYSTEM", NULL, STGM_READ, 0, &stream);
    if(FAILED(hres)) {
        WARN("Could not open #SYSTEM stream: %08lx\n", hres);
        return FALSE;
    }

    IStream_Read(stream, &ver, sizeof(ver), &read);
    TRACE("version is %lx\n", ver);

    buf_size = 8 * sizeof(DWORD);
    buf = malloc(buf_size);
    if(!buf) {
        IStream_Release(stream);
        return FALSE;
    }

    while(1) {
        hres = IStream_Read(stream, &entry, sizeof(entry), &read);
        if(hres != S_OK)
            break;

        if(entry.len > buf_size) {
            new_buf = realloc(buf, entry.len);
            if(!new_buf) {
                hres = E_OUTOFMEMORY;
                break;
            }
            buf = new_buf;
            buf_size = entry.len;
        }

        hres = IStream_Read(stream, buf, entry.len, &read);
        if(hres != S_OK)
            break;

        switch(entry.code) {
        case 0x0:
            TRACE("TOC is %s\n", debugstr_an(buf, entry.len));
            free(chm->defToc);
            chm->defToc = strdupnAtoW(buf, entry.len);
            break;
        case 0x2:
            TRACE("Default topic is %s\n", debugstr_an(buf, entry.len));
            free(chm->defTopic);
            chm->defTopic = strdupnAtoW(buf, entry.len);
            break;
        case 0x3:
            TRACE("Title is %s\n", debugstr_an(buf, entry.len));
            free(chm->defTitle);
            chm->defTitle = strdupnAtoW(buf, entry.len);
            break;
        case 0x4:
            /* TODO: Currently only the Locale ID is loaded from this field */
            TRACE("Locale is: %ld\n", *(LCID*)&buf[0]);
            if(!GetLocaleInfoW(*(LCID*)&buf[0], LOCALE_IDEFAULTANSICODEPAGE|LOCALE_RETURN_NUMBER,
                               (WCHAR *)&chm->codePage, sizeof(chm->codePage)/sizeof(WCHAR)))
                chm->codePage = CP_ACP;
            break;
        case 0x5:
            TRACE("Window name is %s\n", debugstr_an(buf, entry.len));
            chm->defWindow = strdupnAtoW(buf, entry.len);
            break;
        case 0x6:
            TRACE("Compiled file is %s\n", debugstr_an(buf, entry.len));
            free(chm->compiledFile);
            chm->compiledFile = strdupnAtoW(buf, entry.len);
            break;
        case 0x9:
            TRACE("Version is %s\n", debugstr_an(buf, entry.len));
            break;
        case 0xa:
            TRACE("Time is %08lx\n", *(DWORD*)buf);
            break;
        case 0xc:
            TRACE("Number of info types: %ld\n", *(DWORD*)buf);
            break;
        case 0xf:
            TRACE("Check sum: %lx\n", *(DWORD*)buf);
            break;
        default:
            TRACE("unhandled code %x, size %x\n", entry.code, entry.len);
        }
    }

    free(buf);
    IStream_Release(stream);

    return SUCCEEDED(hres);
}

LPWSTR FindContextAlias(CHMInfo *chm, DWORD index)
{
    IStream *ivb_stream;
    DWORD size, read, i;
    DWORD *buf;
    LPCSTR ret = NULL;
    HRESULT hres;

    hres = IStorage_OpenStream(chm->pStorage, L"#IVB", NULL, STGM_READ, 0, &ivb_stream);
    if(FAILED(hres)) {
        WARN("Could not open #IVB stream: %08lx\n", hres);
        return NULL;
    }

    hres = IStream_Read(ivb_stream, &size, sizeof(size), &read);
    if(FAILED(hres)) {
        WARN("Read failed: %08lx\n", hres);
        IStream_Release(ivb_stream);
        return NULL;
    }

    buf = malloc(size);
    hres = IStream_Read(ivb_stream, buf, size, &read);
    IStream_Release(ivb_stream);
    if(FAILED(hres)) {
        WARN("Read failed: %08lx\n", hres);
        free(buf);
        return NULL;
    }

    size /= 2*sizeof(DWORD);

    for(i=0; i<size; i++) {
        if(buf[2*i] == index) {
            ret = GetChmString(chm, buf[2*i+1]);
            break;
        }
    }

    free(buf);

    TRACE("returning %s\n", debugstr_a(ret));
    return strdupAtoW(ret);
}

/*
 * Tests if the file <chmfile>.<ext> exists, used for loading Indices, Table of Contents, etc.
 * when these files are not available from the HH_WINTYPE structure.
 */
static WCHAR *FindHTMLHelpSetting(HHInfo *info, const WCHAR *extW)
{
    static const WCHAR periodW[] = {'.',0};
    IStorage *pStorage = info->pCHMInfo->pStorage;
    IStream *pStream;
    WCHAR *filename;
    HRESULT hr;

    filename = malloc( (wcslen(info->pCHMInfo->compiledFile)
                        + wcslen(periodW) + wcslen(extW) + 1) * sizeof(WCHAR) );
    lstrcpyW(filename, info->pCHMInfo->compiledFile);
    lstrcatW(filename, periodW);
    lstrcatW(filename, extW);
    hr = IStorage_OpenStream(pStorage, filename, NULL, STGM_READ, 0, &pStream);
    if (FAILED(hr))
    {
        free(filename);
        return strdupAtoW("");
    }
    IStream_Release(pStream);
    return filename;
}

static inline WCHAR *MergeChmString(LPCWSTR src, WCHAR **dst)
{
    if(*dst == NULL)
        *dst = wcsdup(src);
    return *dst;
}

void MergeChmProperties(HH_WINTYPEW *src, HHInfo *info, BOOL override)
{
    DWORD unhandled_params = src->fsValidMembers & ~(HHWIN_PARAM_PROPERTIES|HHWIN_PARAM_STYLES
                             |HHWIN_PARAM_EXSTYLES|HHWIN_PARAM_RECT|HHWIN_PARAM_NAV_WIDTH
                             |HHWIN_PARAM_SHOWSTATE|HHWIN_PARAM_INFOTYPES|HHWIN_PARAM_TB_FLAGS
                             |HHWIN_PARAM_EXPANSION|HHWIN_PARAM_TABPOS|HHWIN_PARAM_TABORDER
                             |HHWIN_PARAM_HISTORY_COUNT|HHWIN_PARAM_CUR_TAB);
    HH_WINTYPEW *dst = &info->WinType;
    DWORD merge = override ? src->fsValidMembers : src->fsValidMembers & ~dst->fsValidMembers;

    if (unhandled_params)
        FIXME("Unsupported fsValidMembers fields: 0x%lx\n", unhandled_params);

    dst->fsValidMembers |= merge;
    if (dst->cbStruct == 0)
    {
        /* If the structure has not been filled in yet then use all of the values */
        dst->cbStruct = sizeof(HH_WINTYPEW);
        merge = ~0;
    }
    if (merge & HHWIN_PARAM_PROPERTIES) dst->fsWinProperties = src->fsWinProperties;
    if (merge & HHWIN_PARAM_STYLES) dst->dwStyles = src->dwStyles;
    if (merge & HHWIN_PARAM_EXSTYLES) dst->dwExStyles = src->dwExStyles;
    if (merge & HHWIN_PARAM_RECT) dst->rcWindowPos = src->rcWindowPos;
    if (merge & HHWIN_PARAM_NAV_WIDTH) dst->iNavWidth = src->iNavWidth;
    if (merge & HHWIN_PARAM_SHOWSTATE) dst->nShowState = src->nShowState;
    if (merge & HHWIN_PARAM_INFOTYPES) dst->paInfoTypes = src->paInfoTypes;
    if (merge & HHWIN_PARAM_TB_FLAGS) dst->fsToolBarFlags = src->fsToolBarFlags;
    if (merge & HHWIN_PARAM_EXPANSION) dst->fNotExpanded = src->fNotExpanded;
    if (merge & HHWIN_PARAM_TABPOS) dst->tabpos = src->tabpos;
    if (merge & HHWIN_PARAM_TABORDER) memcpy(dst->tabOrder, src->tabOrder, sizeof(src->tabOrder));
    if (merge & HHWIN_PARAM_HISTORY_COUNT) dst->cHistory = src->cHistory;
    if (merge & HHWIN_PARAM_CUR_TAB) dst->curNavType = src->curNavType;

    /*
     * Note: We assume that hwndHelp, hwndCaller, hwndToolBar, hwndNavigation, and hwndHTML cannot be
     * modified by the user.  rcHTML and rcMinSize are not currently supported, so don't bother to copy them.
     */

    dst->pszType       = MergeChmString(src->pszType, &info->stringsW.pszType);
    dst->pszFile       = MergeChmString(src->pszFile, &info->stringsW.pszFile);
    dst->pszToc        = MergeChmString(src->pszToc, &info->stringsW.pszToc);
    dst->pszIndex      = MergeChmString(src->pszIndex, &info->stringsW.pszIndex);
    dst->pszCaption    = MergeChmString(src->pszCaption, &info->stringsW.pszCaption);
    dst->pszHome       = MergeChmString(src->pszHome, &info->stringsW.pszHome);
    dst->pszJump1      = MergeChmString(src->pszJump1, &info->stringsW.pszJump1);
    dst->pszJump2      = MergeChmString(src->pszJump2, &info->stringsW.pszJump2);
    dst->pszUrlJump1   = MergeChmString(src->pszUrlJump1, &info->stringsW.pszUrlJump1);
    dst->pszUrlJump2   = MergeChmString(src->pszUrlJump2, &info->stringsW.pszUrlJump2);

    /* FIXME: pszCustomTabs is a list of multiple zero-terminated strings so ReadString won't
     * work in this case
     */
#if 0
    dst->pszCustomTabs = MergeChmString(src->pszCustomTabs, &info->pszCustomTabs);
#endif
}

static inline WCHAR *ConvertChmString(HHInfo *info, DWORD id)
{
    WCHAR *ret = NULL;

    if(id)
        ret = strdupAtoW(GetChmString(info->pCHMInfo, id));
    return ret;
}

static inline void wintype_free(HH_WINTYPEW *wintype)
{
    free((void *)wintype->pszType);
    free((void *)wintype->pszCaption);
    free(wintype->paInfoTypes);
    free((void *)wintype->pszToc);
    free((void *)wintype->pszIndex);
    free((void *)wintype->pszFile);
    free((void *)wintype->pszHome);
    free((void *)wintype->pszJump1);
    free((void *)wintype->pszJump2);
    free((void *)wintype->pszUrlJump1);
    free((void *)wintype->pszUrlJump2);
    free((void *)wintype->pszCustomTabs);
}

/* Loads the HH_WINTYPE data from the CHM file
 *
 * FIXME: There may be more than one window type in the file, so
 *        add the ability to choose a certain window type
 */
BOOL LoadWinTypeFromCHM(HHInfo *info)
{
    LARGE_INTEGER liOffset;
    IStorage *pStorage = info->pCHMInfo->pStorage;
    IStream *pStream = NULL;
    HH_WINTYPEW wintype;
    HRESULT hr;
    DWORD cbRead;
    BOOL ret = FALSE;

    /* HH_WINTYPE as stored on disk.  It's identical to HH_WINTYPE except that the pointer fields
       have been changed to DWORDs, so that the layout on 64-bit remains unchanged. */
    struct file_wintype
    {
        int          cbStruct;
        BOOL         fUniCodeStrings;
        DWORD        pszType;
        DWORD        fsValidMembers;
        DWORD        fsWinProperties;
        DWORD        pszCaption;
        DWORD        dwStyles;
        DWORD        dwExStyles;
        RECT         rcWindowPos;
        int          nShowState;
        DWORD        hwndHelp;
        DWORD        hwndCaller;
        DWORD        paInfoTypes;
        DWORD        hwndToolBar;
        DWORD        hwndNavigation;
        DWORD        hwndHTML;
        int          iNavWidth;
        RECT         rcHTML;
        DWORD        pszToc;
        DWORD        pszIndex;
        DWORD        pszFile;
        DWORD        pszHome;
        DWORD        fsToolBarFlags;
        BOOL         fNotExpanded;
        int          curNavType;
        int          tabpos;
        int          idNotify;
        BYTE         tabOrder[HH_MAX_TABS+1];
        int          cHistory;
        DWORD        pszJump1;
        DWORD        pszJump2;
        DWORD        pszUrlJump1;
        DWORD        pszUrlJump2;
        RECT         rcMinSize;
        int          cbInfoTypes;
        DWORD        pszCustomTabs;
    } file_wintype;

    memset(&wintype, 0, sizeof(wintype));
    wintype.cbStruct = sizeof(wintype);
    wintype.fUniCodeStrings = TRUE;

    hr = IStorage_OpenStream(pStorage, L"#WINDOWS", NULL, STGM_READ, 0, &pStream);
    if (SUCCEEDED(hr))
    {
        /* jump past the #WINDOWS header */
        liOffset.QuadPart = sizeof(DWORD) * 2;

        hr = IStream_Seek(pStream, liOffset, STREAM_SEEK_SET, NULL);
        if (FAILED(hr)) goto done;

        /* read the HH_WINTYPE struct data */
        hr = IStream_Read(pStream, &file_wintype, sizeof(file_wintype), &cbRead);
        if (FAILED(hr)) goto done;

        /* convert the #STRINGS offsets to actual strings */
        wintype.pszType         = ConvertChmString(info, file_wintype.pszType);
        wintype.fsValidMembers  = file_wintype.fsValidMembers;
        wintype.fsWinProperties = file_wintype.fsWinProperties;
        wintype.pszCaption      = ConvertChmString(info, file_wintype.pszCaption);
        wintype.dwStyles        = file_wintype.dwStyles;
        wintype.dwExStyles      = file_wintype.dwExStyles;
        wintype.rcWindowPos     = file_wintype.rcWindowPos;
        wintype.nShowState      = file_wintype.nShowState;
        wintype.iNavWidth       = file_wintype.iNavWidth;
        wintype.rcHTML          = file_wintype.rcHTML;
        wintype.pszToc          = ConvertChmString(info, file_wintype.pszToc);
        wintype.pszIndex        = ConvertChmString(info, file_wintype.pszIndex);
        wintype.pszFile         = ConvertChmString(info, file_wintype.pszFile);
        wintype.pszHome         = ConvertChmString(info, file_wintype.pszHome);
        wintype.fsToolBarFlags  = file_wintype.fsToolBarFlags;
        wintype.fNotExpanded    = file_wintype.fNotExpanded;
        wintype.curNavType      = file_wintype.curNavType;
        wintype.tabpos          = file_wintype.tabpos;
        wintype.idNotify        = file_wintype.idNotify;
        memcpy(&wintype.tabOrder, file_wintype.tabOrder, sizeof(wintype.tabOrder));
        wintype.cHistory        = file_wintype.cHistory;
        wintype.pszJump1        = ConvertChmString(info, file_wintype.pszJump1);
        wintype.pszJump2        = ConvertChmString(info, file_wintype.pszJump2);
        wintype.pszUrlJump1     = ConvertChmString(info, file_wintype.pszUrlJump1);
        wintype.pszUrlJump2     = ConvertChmString(info, file_wintype.pszUrlJump2);
        wintype.rcMinSize       = file_wintype.rcMinSize;
        wintype.cbInfoTypes     = file_wintype.cbInfoTypes;
    }
    else
    {
        /* no defined window types so use (hopefully) sane defaults */
        wintype.pszType    = wcsdup(info->pCHMInfo->defWindow ? info->pCHMInfo->defWindow : L"defaultwin");
        wintype.pszToc     = wcsdup(info->pCHMInfo->defToc ? info->pCHMInfo->defToc : L"");
        wintype.pszIndex   = wcsdup(L"");
        wintype.fsValidMembers = 0;
        wintype.fsWinProperties = HHWIN_PROP_TRI_PANE;
        wintype.dwStyles = WS_POPUP;
        wintype.dwExStyles = 0;
        wintype.nShowState = SW_SHOW;
        wintype.curNavType = HHWIN_NAVTYPE_TOC;
    }

    /* merge the new data with any pre-existing HH_WINTYPE structure */
    MergeChmProperties(&wintype, info, FALSE);
    if (!info->WinType.pszCaption)
        info->WinType.pszCaption = info->stringsW.pszCaption = wcsdup(info->pCHMInfo->defTitle ? info->pCHMInfo->defTitle : L"");
    if (!info->WinType.pszFile)
        info->WinType.pszFile    = info->stringsW.pszFile    = wcsdup(info->pCHMInfo->defTopic ? info->pCHMInfo->defTopic : L"");
    if (!info->WinType.pszToc)
        info->WinType.pszToc     = info->stringsW.pszToc     = FindHTMLHelpSetting(info, L"hhc");
    if (!info->WinType.pszIndex)
        info->WinType.pszIndex   = info->stringsW.pszIndex   = FindHTMLHelpSetting(info, L"hhk");

    wintype_free(&wintype);
    ret = TRUE;

done:
    if (pStream)
        IStream_Release(pStream);

    return ret;
}

LPCWSTR skip_schema(LPCWSTR url)
{
    static const WCHAR its_schema[] = {'i','t','s',':'};
    static const WCHAR msits_schema[] = {'m','s','-','i','t','s',':'};
    static const WCHAR mk_schema[] = {'m','k',':','@','M','S','I','T','S','t','o','r','e',':'};

    if(!wcsnicmp(its_schema, url, ARRAY_SIZE(its_schema)))
        return url + ARRAY_SIZE(its_schema);
    if(!wcsnicmp(msits_schema, url, ARRAY_SIZE(msits_schema)))
        return url + ARRAY_SIZE(msits_schema);
    if(!wcsnicmp(mk_schema, url, ARRAY_SIZE(mk_schema)))
        return url + ARRAY_SIZE(mk_schema);

    return url;
}

void SetChmPath(ChmPath *file, LPCWSTR base_file, LPCWSTR path)
{
    LPCWSTR ptr;

    path = skip_schema(path);

    ptr = wcsstr(path, L"::");
    if(ptr) {
        WCHAR chm_file[MAX_PATH];
        WCHAR rel_path[MAX_PATH];
        WCHAR base_path[MAX_PATH];
        LPWSTR p;

        lstrcpyW(base_path, base_file);
        p = wcsrchr(base_path, '\\');
        if(p)
            *p = 0;

        memcpy(rel_path, path, (ptr-path)*sizeof(WCHAR));
        rel_path[ptr-path] = 0;

        PathCombineW(chm_file, base_path, rel_path);

        file->chm_file = wcsdup(chm_file);
        ptr += 2;
    }else {
        file->chm_file = wcsdup(base_file);
        ptr = path;
    }

    file->chm_index = wcsdup(ptr);

    TRACE("ChmFile = {%s %s}\n", debugstr_w(file->chm_file), debugstr_w(file->chm_index));
}

IStream *GetChmStream(CHMInfo *info, LPCWSTR parent_chm, ChmPath *chm_file)
{
    IStorage *storage;
    IStream *stream = NULL;
    HRESULT hres;

    TRACE("%s (%s :: %s)\n", debugstr_w(parent_chm), debugstr_w(chm_file->chm_file),
          debugstr_w(chm_file->chm_index));

    if(parent_chm || chm_file->chm_file) {
        hres = IITStorage_StgOpenStorage(info->pITStorage,
                chm_file->chm_file ? chm_file->chm_file : parent_chm, NULL,
                STGM_READ | STGM_SHARE_DENY_WRITE, NULL, 0, &storage);
        if(FAILED(hres)) {
            WARN("Could not open storage: %08lx\n", hres);
            return NULL;
        }
    }else {
        storage = info->pStorage;
        IStorage_AddRef(info->pStorage);
    }

    hres = IStorage_OpenStream(storage, chm_file->chm_index, NULL, STGM_READ, 0, &stream);
    IStorage_Release(storage);
    if(FAILED(hres))
        WARN("Could not open stream: %08lx\n", hres);

    return stream;
}

/*
 * Retrieve a CHM document and parse the data from the <title> element to get the document's title.
 */
WCHAR *GetDocumentTitle(CHMInfo *info, LPCWSTR document)
{
    strbuf_t node, node_name, content;
    WCHAR *document_title = NULL;
    IStream *str = NULL;
    IStorage *storage;
    stream_t stream;
    HRESULT hres;

    TRACE("%s\n", debugstr_w(document));

    storage = info->pStorage;
    if(!storage) {
        WARN("Could not open storage to obtain the title for a document.\n");
        return NULL;
    }
    IStorage_AddRef(storage);

    hres = IStorage_OpenStream(storage, document, NULL, STGM_READ, 0, &str);
    IStorage_Release(storage);
    if(FAILED(hres))
        WARN("Could not open stream: %08lx\n", hres);

    stream_init(&stream, str);
    strbuf_init(&node);
    strbuf_init(&content);
    strbuf_init(&node_name);

    while(next_node(&stream, &node)) {
        get_node_name(&node, &node_name);

        TRACE("%s\n", node.buf);

        if(!stricmp(node_name.buf, "title")) {
            if(next_content(&stream, &content) && content.len > 1)
            {
                document_title = strdupnAtoW(&content.buf[1], content.len-1);
                FIXME("magic: %s\n", debugstr_w(document_title));
                break;
            }
        }

        strbuf_zero(&node);
    }

    strbuf_free(&node);
    strbuf_free(&content);
    strbuf_free(&node_name);
    IStream_Release(str);

    return document_title;
}

/* Opens the CHM file for reading */
CHMInfo *OpenCHM(LPCWSTR szFile)
{
    HRESULT hres;
    CHMInfo *ret;

    if (!(ret = calloc(1, sizeof(CHMInfo))))
        return NULL;
    ret->codePage = CP_ACP;

    if (!(ret->szFile = wcsdup(szFile))) {
        free(ret);
        return NULL;
    }

    hres = CoCreateInstance(&CLSID_ITStorage, NULL, CLSCTX_INPROC_SERVER,
            &IID_IITStorage, (void **) &ret->pITStorage) ;
    if(FAILED(hres)) {
        WARN("Could not create ITStorage: %08lx\n", hres);
        return CloseCHM(ret);
    }

    hres = IITStorage_StgOpenStorage(ret->pITStorage, szFile, NULL,
            STGM_READ | STGM_SHARE_DENY_WRITE, NULL, 0, &ret->pStorage);
    if(FAILED(hres)) {
        WARN("Could not open storage: %08lx\n", hres);
        return CloseCHM(ret);
    }
    hres = IStorage_OpenStream(ret->pStorage, L"#STRINGS", NULL, STGM_READ, 0,
            &ret->strings_stream);
    if(FAILED(hres)) {
        WARN("Could not open #STRINGS stream: %08lx\n", hres);
        /* It's not critical, so we pass */
    }

    if(!ReadChmSystem(ret)) {
        WARN("Could not read #SYSTEM\n");
        return CloseCHM(ret);
    }

    return ret;
}

CHMInfo *CloseCHM(CHMInfo *chm)
{
    if(chm->pITStorage)
        IITStorage_Release(chm->pITStorage);

    if(chm->pStorage)
        IStorage_Release(chm->pStorage);

    if(chm->strings_stream)
        IStream_Release(chm->strings_stream);

    if(chm->strings_size) {
        DWORD i;

        for(i=0; i<chm->strings_size; i++)
            free(chm->strings[i]);
    }

    free(chm->strings);
    free(chm->defWindow);
    free(chm->defTitle);
    free(chm->defTopic);
    free(chm->defToc);
    free(chm->szFile);
    free(chm->compiledFile);
    free(chm);

    return NULL;
}
