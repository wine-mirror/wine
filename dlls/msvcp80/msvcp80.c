/*
 * msvcp80 specific functions
 *
 * Copyright 2010 Piotr Caban for CodeWeavers
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

#include "windef.h"
#include "winbase.h"

typedef SIZE_T MSVCP_size_t;
struct locale_facet;
struct locale;

const struct locale* (CDECL *plocale_classic)(void);
MSVCP_size_t (CDECL *pcollate_char__Getcat)(const struct locale_facet**,const struct locale*);
MSVCP_size_t (CDECL *pcollate_wchar__Getcat)(const struct locale_facet**,const struct locale*);
MSVCP_size_t (CDECL *pcollate_short__Getcat)(const struct locale_facet**,const struct locale*);
MSVCP_size_t (CDECL *pctype_char__Getcat)(const struct locale_facet**,const struct locale*);
MSVCP_size_t (CDECL *pctype_wchar__Getcat)(const struct locale_facet**,const struct locale*);
MSVCP_size_t (CDECL *pctype_short__Getcat)(const struct locale_facet**,const struct locale*);
MSVCP_size_t (CDECL *pcodecvt_char__Getcat)(const struct locale_facet**,const struct locale*);
MSVCP_size_t (CDECL *pnumpunct_char__Getcat)(const struct locale_facet**,const struct locale*);
MSVCP_size_t (CDECL *pnumpunct_wchar__Getcat)(const struct locale_facet**,const struct locale*);
MSVCP_size_t (CDECL *pnumpunct_short__Getcat)(const struct locale_facet**,const struct locale*);

static BOOL init_funcs(void)
{
    HMODULE hmod = GetModuleHandleA("msvcp90.dll");
    if(!hmod)
        return FALSE;

    if(sizeof(void*) > sizeof(int)) { /* 64-bit has different names */
        plocale_classic = (void*)GetProcAddress(hmod, "?classic@locale@std@@SAAEBV12@XZ");
        pcollate_char__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$collate@D@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z");
        pcollate_wchar__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$collate@_W@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z");
        pcollate_short__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$collate@G@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z");
        pctype_char__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$ctype@D@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z");
        pctype_wchar__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$ctype@_W@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z");
        pctype_short__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$ctype@G@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z");
        pcodecvt_char__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$codecvt@DDH@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z");
        pnumpunct_char__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$numpunct@D@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z");
        pnumpunct_wchar__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$numpunct@_W@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z");
        pnumpunct_short__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$numpunct@G@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z");
    }else {
        plocale_classic = (void*)GetProcAddress(hmod, "?classic@locale@std@@SAABV12@XZ");
        pcollate_char__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$collate@D@std@@SAIPAPBVfacet@locale@2@PBV42@@Z");
        pcollate_wchar__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$collate@_W@std@@SAIPAPBVfacet@locale@2@PBV42@@Z");
        pcollate_short__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$collate@G@std@@SAIPAPBVfacet@locale@2@PBV42@@Z");
        pctype_char__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$ctype@D@std@@SAIPAPBVfacet@locale@2@PBV42@@Z");
        pctype_wchar__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$ctype@_W@std@@SAIPAPBVfacet@locale@2@PBV42@@Z");
        pctype_short__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$ctype@G@std@@SAIPAPBVfacet@locale@2@PBV42@@Z(ptr");
        pcodecvt_char__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$codecvt@DDH@std@@SAIPAPBVfacet@locale@2@PBV42@@Z");
        pnumpunct_char__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$numpunct@D@std@@SAIPAPBVfacet@locale@2@PBV42@@Z");
        pnumpunct_wchar__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$numpunct@_W@std@@SAIPAPBVfacet@locale@2@PBV42@@Z");
        pnumpunct_short__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$numpunct@G@std@@SAIPAPBVfacet@locale@2@PBV42@@Z");
    }

    return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE hdll, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;  /* prefer native version */

        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hdll);
            if(!init_funcs())
                return FALSE;
    }
    return TRUE;
}

/* ?_Getcat@?$collate@D@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$collate@D@std@@SA_KPEAPEBVfacet@locale@2@@Z */
MSVCP_size_t __cdecl collate_char__Getcat(const struct locale_facet **facet)
{
    return pcollate_char__Getcat(facet, plocale_classic());
}

/* ?_Getcat@?$collate@_W@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$collate@_W@std@@SA_KPEAPEBVfacet@locale@2@@Z */
MSVCP_size_t __cdecl collate_wchar__Getcat(const struct locale_facet **facet)
{
    return pcollate_wchar__Getcat(facet, plocale_classic());
}

/* ?_Getcat@?$collate@G@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$collate@G@std@@SA_KPEAPEBVfacet@locale@2@@Z */
MSVCP_size_t __cdecl collate_short__Getcat(const struct locale_facet **facet)
{
    return pcollate_short__Getcat(facet, plocale_classic());
}

/* ?_Getcat@?$ctype@D@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$ctype@D@std@@SA_KPEAPEBVfacet@locale@2@@Z */
MSVCP_size_t __cdecl ctype_char__Getcat(const struct locale_facet **facet)
{
    return pctype_char__Getcat(facet, plocale_classic());
}

/* ?_Getcat@?$ctype@_W@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$ctype@_W@std@@SA_KPEAPEBVfacet@locale@2@@Z */
MSVCP_size_t __cdecl ctype_wchar__Getcat(const struct locale_facet **facet)
{
    return pctype_wchar__Getcat(facet, plocale_classic());
}

/* ?_Getcat@?$ctype@G@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$ctype@G@std@@SA_KPEAPEBVfacet@locale@2@@Z */
MSVCP_size_t __cdecl ctype_short__Getcat(const struct locale_facet **facet)
{
    return pctype_short__Getcat(facet, plocale_classic());
}

/* ?_Getcat@?$codecvt@DDH@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$codecvt@DDH@std@@SA_KPEAPEBVfacet@locale@2@@Z */
MSVCP_size_t __cdecl codecvt_char__Getcat(const struct locale_facet **facet)
{
    return pcodecvt_char__Getcat(facet, plocale_classic());
}

/* ?_Getcat@?$numpunct@D@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$numpunct@D@std@@SA_KPEAPEBVfacet@locale@2@@Z */
MSVCP_size_t __cdecl numpunct_char__Getcat(const struct locale_facet **facet)
{
    return pnumpunct_char__Getcat(facet, plocale_classic());
}

/* ?_Getcat@?$numpunct@_W@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$numpunct@_W@std@@SA_KPEAPEBVfacet@locale@2@@Z */
MSVCP_size_t __cdecl numpunct_wchar__Getcat(const struct locale_facet **facet)
{
    return pnumpunct_wchar__Getcat(facet, plocale_classic());
}

/* ?_Getcat@?$numpunct@G@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$numpunct@G@std@@SA_KPEAPEBVfacet@locale@2@@Z */
MSVCP_size_t __cdecl numpunct_short__Getcat(const struct locale_facet **facet)
{
    return pnumpunct_short__Getcat(facet, plocale_classic());
}
