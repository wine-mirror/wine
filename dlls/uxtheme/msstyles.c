/*
 * Win32 5.1 msstyles theme format
 *
 * Copyright (C) 2003 Kevin Koltzau
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
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "vfwmsgs.h"
#include "uxtheme.h"
#include "vssym32.h"

#include "msstyles.h"

#include "wine/exception.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(uxtheme);

/***********************************************************************
 * Defines and global variables
 */

static BOOL MSSTYLES_GetNextInteger(LPCWSTR lpStringStart, LPCWSTR lpStringEnd, LPCWSTR *lpValEnd, int *value);
static BOOL MSSTYLES_GetNextLong(LPCWSTR lpStringStart, LPCWSTR lpStringEnd, LPCWSTR *lpValEnd, LONG *value);
static BOOL MSSTYLES_GetNextToken(LPCWSTR lpStringStart, LPCWSTR lpStringEnd, LPCWSTR *lpValEnd, LPWSTR lpBuff, DWORD buffSize);
static void MSSTYLES_ParseThemeIni(PTHEME_FILE tf, BOOL setMetrics);
static HRESULT MSSTYLES_GetFont (LPCWSTR lpStringStart, LPCWSTR lpStringEnd, LPCWSTR *lpValEnd, LOGFONTW* logfont);

#define MSSTYLES_VERSION 0x0003

#define THEME_CLASS_SIGNATURE 0x12bc6d83

static PTHEME_FILE tfActiveTheme;

/***********************************************************************/

/**********************************************************************
 *      MSSTYLES_OpenThemeFile
 *
 * Load and validate a theme
 *
 * PARAMS
 *     lpThemeFile         Path to theme file to load
 *     pszColorName        Color name wanted, can be NULL
 *     pszSizeName         Size name wanted, can be NULL
 *
 * NOTES
 * If pszColorName or pszSizeName are NULL, the default color/size will be used.
 * If one/both are provided, they are validated against valid color/sizes and if
 * a match is not found, the function fails.
 */
HRESULT MSSTYLES_OpenThemeFile(LPCWSTR lpThemeFile, LPCWSTR pszColorName, LPCWSTR pszSizeName, PTHEME_FILE *tf)
{
    HMODULE hTheme;
    HRSRC hrsc;
    HRESULT hr = S_OK;

    WORD version;
    DWORD versize;
    LPWSTR pszColors;
    LPWSTR pszSelectedColor = NULL;
    LPWSTR pszSizes;
    LPWSTR pszSelectedSize = NULL;
    LPWSTR tmp;

    TRACE("Opening %s\n", debugstr_w(lpThemeFile));

    hTheme = LoadLibraryExW(lpThemeFile, NULL, LOAD_LIBRARY_AS_DATAFILE);

    /* Validate that this is really a theme */
    if(!hTheme) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto invalid_theme;
    }
    if(!(hrsc = FindResourceW(hTheme, MAKEINTRESOURCEW(1), L"PACKTHEM_VERSION"))) {
        TRACE("No version resource found\n");
        hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
        goto invalid_theme;
    }
    if((versize = SizeofResource(hTheme, hrsc)) != 2)
    {
        TRACE("Version resource found, but wrong size: %ld\n", versize);
        hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
        goto invalid_theme;
    }
    version = *(WORD*)LoadResource(hTheme, hrsc);
    if(version != MSSTYLES_VERSION)
    {
        TRACE("Version of theme file is unsupported: 0x%04x\n", version);
        hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
        goto invalid_theme;
    }

    if(!(hrsc = FindResourceW(hTheme, MAKEINTRESOURCEW(1), L"COLORNAMES"))) {
        TRACE("Color names resource not found\n");
        hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
        goto invalid_theme;
    }
    pszColors = LoadResource(hTheme, hrsc);

    if(!(hrsc = FindResourceW(hTheme, MAKEINTRESOURCEW(1), L"SIZENAMES"))) {
        TRACE("Size names resource not found\n");
        hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
        goto invalid_theme;
    }
    pszSizes = LoadResource(hTheme, hrsc);

    /* Validate requested color against what's available from the theme */
    if(pszColorName) {
        tmp = pszColors;
        while(*tmp) {
            if(!lstrcmpiW(pszColorName, tmp)) {
                pszSelectedColor = tmp;
                break;
            }
            tmp += lstrlenW(tmp)+1;
        }
    }
    else
        pszSelectedColor = pszColors; /* Use the default color */

    /* Validate requested size against what's available from the theme */
    if(pszSizeName) {
        tmp = pszSizes;
        while(*tmp) {
            if(!lstrcmpiW(pszSizeName, tmp)) {
                pszSelectedSize = tmp;
                break;
            }
            tmp += lstrlenW(tmp)+1;
        }
    }
    else
        pszSelectedSize = pszSizes; /* Use the default size */

    if(!pszSelectedColor || !pszSelectedSize) {
        TRACE("Requested color/size (%s/%s) not found in theme\n",
              debugstr_w(pszColorName), debugstr_w(pszSizeName));
        hr = E_PROP_ID_UNSUPPORTED;
        goto invalid_theme;
    }

    *tf = calloc(1, sizeof(THEME_FILE));
    (*tf)->hTheme = hTheme;
    
    GetFullPathNameW(lpThemeFile, MAX_PATH, (*tf)->szThemeFile, NULL);
    
    (*tf)->pszAvailColors = pszColors;
    (*tf)->pszAvailSizes = pszSizes;
    (*tf)->pszSelectedColor = pszSelectedColor;
    (*tf)->pszSelectedSize = pszSelectedSize;
    (*tf)->refcount = 1;
    return S_OK;

invalid_theme:
    *tf = NULL;
    if(hTheme) FreeLibrary(hTheme);
    return hr;
}

/***********************************************************************
 *      MSSTYLES_CloseThemeFile
 *
 * Close theme file and free resources
 */
void MSSTYLES_CloseThemeFile(PTHEME_FILE tf)
{
    LONG refcount;

    if(tf) {
        refcount = InterlockedDecrement(&tf->refcount);
        if (!refcount)
        {
            if(tf->hTheme) FreeLibrary(tf->hTheme);
            if(tf->classes) {
                while(tf->classes) {
                    PTHEME_CLASS pcls = tf->classes;
                    tf->classes = pcls->next;
                    while(pcls->partstate) {
                        PTHEME_PARTSTATE ps = pcls->partstate;

                        while(ps->properties) {
                            PTHEME_PROPERTY prop = ps->properties;
                            ps->properties = prop->next;
                            free(prop);
                        }

                        pcls->partstate = ps->next;
                        free(ps);
                    }
                    pcls->signature = 0;
                    free(pcls);
                }
            }
            while (tf->images)
            {
                PTHEME_IMAGE img = tf->images;
                tf->images = img->next;
                DeleteObject (img->image);
                free(img);
            }
            free(tf);
        }
    }
}

/***********************************************************************
 *      MSSTYLES_SetActiveTheme
 *
 * Set the current active theme
 */
HRESULT MSSTYLES_SetActiveTheme(PTHEME_FILE tf, BOOL setMetrics)
{
    if(tfActiveTheme)
        MSSTYLES_CloseThemeFile(tfActiveTheme);
    tfActiveTheme = tf;
    if (tfActiveTheme)
    {
        InterlockedIncrement(&tfActiveTheme->refcount);
	if(!tfActiveTheme->classes)
	    MSSTYLES_ParseThemeIni(tfActiveTheme, setMetrics);
    }
    return S_OK;
}

/***********************************************************************
 *      MSSTYLES_GetThemeIni
 *
 * Retrieves themes.ini from a theme
 */
PUXINI_FILE MSSTYLES_GetThemeIni(PTHEME_FILE tf)
{
    return UXINI_LoadINI(tf->hTheme, L"themes_ini");
}

/***********************************************************************
 *      MSSTYLES_GetThemeDPI
 *
 * Retrieves the DPI from a theme handle when it was opened
 */
UINT MSSTYLES_GetThemeDPI(PTHEME_CLASS tc)
{
    return tc->dpi;
}

/***********************************************************************
 *      MSSTYLES_GetActiveThemeIni
 *
 * Retrieve the ini file for the selected color/style
 */
static PUXINI_FILE MSSTYLES_GetActiveThemeIni(PTHEME_FILE tf)
{
    DWORD dwColorCount = 0;
    DWORD dwSizeCount = 0;
    DWORD dwColorNum = 0;
    DWORD dwSizeNum = 0;
    DWORD i;
    DWORD dwResourceIndex;
    LPWSTR tmp;
    HRSRC hrsc;

    /* Count the number of available colors & styles, and determine the index number
       of the color/style we are interested in
    */
    tmp = tf->pszAvailColors;
    while(*tmp) {
        if(!lstrcmpiW(tf->pszSelectedColor, tmp))
            dwColorNum = dwColorCount;
        tmp += lstrlenW(tmp)+1;
        dwColorCount++;
    }
    tmp = tf->pszAvailSizes;
    while(*tmp) {
        if(!lstrcmpiW(tf->pszSelectedSize, tmp))
            dwSizeNum = dwSizeCount;
        tmp += lstrlenW(tmp)+1;
        dwSizeCount++;
    }

    if(!(hrsc = FindResourceW(tf->hTheme, MAKEINTRESOURCEW(1), L"FILERESNAMES"))) {
        TRACE("FILERESNAMES map not found\n");
        return NULL;
    }
    tmp = LoadResource(tf->hTheme, hrsc);
    dwResourceIndex = (dwSizeCount * dwColorNum) + dwSizeNum;
    for(i=0; i < dwResourceIndex; i++) {
        tmp += lstrlenW(tmp)+1;
    }
    return UXINI_LoadINI(tf->hTheme, tmp);
}


/***********************************************************************
 *      MSSTYLES_ParseIniSectionName
 *
 * Parse an ini section name into its component parts
 * Valid formats are:
 * [classname]
 * [classname(state)]
 * [classname.part]
 * [classname.part(state)]
 * [application::classname]
 * [application::classname(state)]
 * [application::classname.part]
 * [application::classname.part(state)]
 *
 * PARAMS
 *     lpSection           Section name
 *     dwLen               Length of section name
 *     szAppName           Location to store application name
 *     szClassName         Location to store class name
 *     iPartId             Location to store part id
 *     iStateId            Location to store state id
 */
static BOOL MSSTYLES_ParseIniSectionName(LPCWSTR lpSection, DWORD dwLen, LPWSTR szAppName, LPWSTR szClassName, int *iPartId, int *iStateId)
{
    WCHAR sec[255];
    WCHAR part[60] = {'\0'};
    WCHAR state[60] = {'\0'};
    LPWSTR tmp;
    LPWSTR comp;
    lstrcpynW(sec, lpSection, min(dwLen+1, ARRAY_SIZE(sec)));

    *szAppName = 0;
    *szClassName = 0;
    *iPartId = 0;
    *iStateId = 0;
    comp = sec;
    /* Get the application name */
    tmp = wcschr(comp, ':');
    if(tmp) {
        *tmp++ = 0;
        tmp++;
        lstrcpynW(szAppName, comp, MAX_THEME_APP_NAME);
        comp = tmp;
    }

    tmp = wcschr(comp, '.');
    if(tmp) {
        *tmp++ = 0;
        lstrcpynW(szClassName, comp, MAX_THEME_CLASS_NAME);
        comp = tmp;
        /* now get the part & state */
        tmp = wcschr(comp, '(');
        if(tmp) {
            *tmp++ = 0;
            lstrcpynW(part, comp, ARRAY_SIZE(part));
            comp = tmp;
            /* now get the state */
            tmp = wcschr(comp, ')');
            if (!tmp)
                return FALSE;
            *tmp = 0;
            lstrcpynW(state, comp, ARRAY_SIZE(state));
        }
        else {
            lstrcpynW(part, comp, ARRAY_SIZE(part));
        }
    }
    else {
        tmp = wcschr(comp, '(');
        if(tmp) {
            *tmp++ = 0;
            lstrcpynW(szClassName, comp, MAX_THEME_CLASS_NAME);
            comp = tmp;
            /* now get the state */
            tmp = wcschr(comp, ')');
            if (!tmp)
                return FALSE;
            *tmp = 0;
            lstrcpynW(state, comp, ARRAY_SIZE(state));
        }
        else {
            lstrcpynW(szClassName, comp, MAX_THEME_CLASS_NAME);
        }
    }
    if(!*szClassName) return FALSE;
    return MSSTYLES_LookupPartState(szClassName, part[0]?part:NULL, state[0]?state:NULL, iPartId, iStateId);
}

/***********************************************************************
 *      MSSTYLES_FindClass
 *
 * Find a class
 *
 * PARAMS
 *     tf                  Theme file
 *     pszAppName          App name to find
 *     pszClassName        Class name to find
 *
 * RETURNS
 *  The class found, or NULL
 */
static PTHEME_CLASS MSSTYLES_FindClass(PTHEME_FILE tf, LPCWSTR pszAppName, LPCWSTR pszClassName)
{
    PTHEME_CLASS cur = tf->classes;
    while(cur) {
        if(!pszAppName) {
            if(!*cur->szAppName && !lstrcmpiW(pszClassName, cur->szClassName))
                return cur;
        }
        else {
            if(!lstrcmpiW(pszAppName, cur->szAppName) && !lstrcmpiW(pszClassName, cur->szClassName))
                return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

/***********************************************************************
 *      MSSTYLES_AddClass
 *
 * Add a class to a theme file
 *
 * PARAMS
 *     tf                  Theme file
 *     pszAppName          App name to add
 *     pszClassName        Class name to add
 *
 * RETURNS
 *  The class added, or a class previously added with the same name
 */
static PTHEME_CLASS MSSTYLES_AddClass(PTHEME_FILE tf, LPCWSTR pszAppName, LPCWSTR pszClassName)
{
    PTHEME_CLASS cur = MSSTYLES_FindClass(tf, pszAppName, pszClassName);
    if(cur) return cur;

    cur = malloc(sizeof(*cur));
    cur->signature = THEME_CLASS_SIGNATURE;
    cur->refcount = 0;
    cur->hTheme = tf->hTheme;
    lstrcpyW(cur->szAppName, pszAppName);
    lstrcpyW(cur->szClassName, pszClassName);
    cur->next = tf->classes;
    cur->partstate = NULL;
    cur->overrides = NULL;
    tf->classes = cur;
    return cur;
}

/***********************************************************************
 *      MSSTYLES_FindPart
 *
 * Find a part
 *
 * PARAMS
 *     tc                  Class to search
 *     iPartId             Part ID to find
 *
 * RETURNS
 *  The part found, or NULL
 */
PTHEME_PARTSTATE MSSTYLES_FindPart(PTHEME_CLASS tc, int iPartId)
{
    PTHEME_PARTSTATE cur = tc->partstate;

    while (cur)
    {
        if (cur->iPartId == iPartId)
            return cur;

        cur = cur->next;
    }

    if (tc->overrides)
        return MSSTYLES_FindPart(tc->overrides, iPartId);

    return NULL;
}

/***********************************************************************
 *      MSSTYLES_FindPartState
 *
 * Find a part/state
 *
 * PARAMS
 *     tc                  Class to search
 *     iPartId             Part ID to find
 *     iStateId            State ID to find
 *     tcNext              Receives the next class in the override chain
 *
 * RETURNS
 *  The part/state found, or NULL
 */
PTHEME_PARTSTATE MSSTYLES_FindPartState(PTHEME_CLASS tc, int iPartId, int iStateId, PTHEME_CLASS *tcNext)
{
    PTHEME_PARTSTATE cur = tc->partstate;
    while(cur) {
        if(cur->iPartId == iPartId && cur->iStateId == iStateId) {
            if(tcNext) *tcNext = tc->overrides;
            return cur;
        }
        cur = cur->next;
    }
    if(tc->overrides) return MSSTYLES_FindPartState(tc->overrides, iPartId, iStateId, tcNext);
    return NULL;
}

/***********************************************************************
 *      MSSTYLES_AddPartState
 *
 * Add a part/state to a class
 *
 * PARAMS
 *     tc                  Theme class
 *     iPartId             Part ID to add
 *     iStateId            State ID to add
 *
 * RETURNS
 *  The part/state added, or a part/state previously added with the same IDs
 */
static PTHEME_PARTSTATE MSSTYLES_AddPartState(PTHEME_CLASS tc, int iPartId, int iStateId)
{
    PTHEME_PARTSTATE cur = MSSTYLES_FindPartState(tc, iPartId, iStateId, NULL);
    if(cur) return cur;

    cur = malloc(sizeof(*cur));
    cur->iPartId = iPartId;
    cur->iStateId = iStateId;
    cur->properties = NULL;
    cur->next = tc->partstate;
    tc->partstate = cur;
    return cur;
}

/***********************************************************************
 *      MSSTYLES_LFindProperty
 *
 * Find a property within a property list
 *
 * PARAMS
 *     tp                  property list to scan
 *     iPropertyPrimitive  Type of value expected
 *     iPropertyId         ID of the required value
 *
 * RETURNS
 *  The property found, or NULL
 */
static PTHEME_PROPERTY MSSTYLES_LFindProperty(PTHEME_PROPERTY tp, int iPropertyPrimitive, int iPropertyId)
{
    PTHEME_PROPERTY cur = tp;
    while(cur) {
        if(cur->iPropertyId == iPropertyId) {
            if(cur->iPrimitiveType == iPropertyPrimitive) {
                return cur;
            }
            else {
                if(!iPropertyPrimitive)
                    return cur;
                return NULL;
            }
        }
        cur = cur->next;
    }
    return NULL;
}

/***********************************************************************
 *      MSSTYLES_PSFindProperty
 *
 * Find a value within a part/state
 *
 * PARAMS
 *     ps                  Part/state to search
 *     iPropertyPrimitive  Type of value expected
 *     iPropertyId         ID of the required value
 *
 * RETURNS
 *  The property found, or NULL
 */
static inline PTHEME_PROPERTY MSSTYLES_PSFindProperty(PTHEME_PARTSTATE ps, int iPropertyPrimitive, int iPropertyId)
{
    return MSSTYLES_LFindProperty(ps->properties, iPropertyPrimitive, iPropertyId);
}

/***********************************************************************
 *      MSSTYLES_FFindMetric
 *
 * Find a metric property for a theme file
 *
 * PARAMS
 *     tf                  Theme file
 *     iPropertyPrimitive  Type of value expected
 *     iPropertyId         ID of the required value
 *
 * RETURNS
 *  The property found, or NULL
 */
static inline PTHEME_PROPERTY MSSTYLES_FFindMetric(PTHEME_FILE tf, int iPropertyPrimitive, int iPropertyId)
{
    return MSSTYLES_LFindProperty(tf->metrics, iPropertyPrimitive, iPropertyId);
}

/***********************************************************************
 *      MSSTYLES_FindMetric
 *
 * Find a metric property for the current installed theme
 *
 * PARAMS
 *     tf                  Theme file
 *     iPropertyPrimitive  Type of value expected
 *     iPropertyId         ID of the required value
 *
 * RETURNS
 *  The property found, or NULL
 */
PTHEME_PROPERTY MSSTYLES_FindMetric(int iPropertyPrimitive, int iPropertyId)
{
    if(!tfActiveTheme) return NULL;
    return MSSTYLES_FFindMetric(tfActiveTheme, iPropertyPrimitive, iPropertyId);
}

/***********************************************************************
 *      MSSTYLES_AddProperty
 *
 * Add a property to a part/state
 *
 * PARAMS
 *     ps                  Part/state
 *     iPropertyPrimitive  Primitive type of the property
 *     iPropertyId         ID of the property
 *     lpValue             Raw value (non-NULL terminated)
 *     dwValueLen          Length of the value
 *
 * RETURNS
 *  The property added, or a property previously added with the same IDs
 */
static PTHEME_PROPERTY MSSTYLES_AddProperty(PTHEME_PARTSTATE ps, int iPropertyPrimitive, int iPropertyId, LPCWSTR lpValue, DWORD dwValueLen, BOOL isGlobal)
{
    PTHEME_PROPERTY cur = MSSTYLES_PSFindProperty(ps, iPropertyPrimitive, iPropertyId);
    /* Should duplicate properties overwrite the original, or be ignored? */
    if(cur) return cur;

    cur = malloc(sizeof(*cur));
    cur->iPrimitiveType = iPropertyPrimitive;
    cur->iPropertyId = iPropertyId;
    cur->lpValue = lpValue;
    cur->dwValueLen = dwValueLen;

    if(ps->iStateId)
        cur->origin = PO_STATE;
    else if(ps->iPartId)
        cur->origin = PO_PART;
    else if(isGlobal)
        cur->origin = PO_GLOBAL;
    else
        cur->origin = PO_CLASS;

    cur->next = ps->properties;
    ps->properties = cur;
    return cur;
}

/***********************************************************************
 *      MSSTYLES_AddMetric
 *
 * Add a property to a part/state
 *
 * PARAMS
 *     tf                  Theme file
 *     iPropertyPrimitive  Primitive type of the property
 *     iPropertyId         ID of the property
 *     lpValue             Raw value (non-NULL terminated)
 *     dwValueLen          Length of the value
 *
 * RETURNS
 *  The property added, or a property previously added with the same IDs
 */
static PTHEME_PROPERTY MSSTYLES_AddMetric(PTHEME_FILE tf, int iPropertyPrimitive, int iPropertyId, LPCWSTR lpValue, DWORD dwValueLen)
{
    PTHEME_PROPERTY cur = MSSTYLES_FFindMetric(tf, iPropertyPrimitive, iPropertyId);
    /* Should duplicate properties overwrite the original, or be ignored? */
    if(cur) return cur;

    cur = malloc(sizeof(*cur));
    cur->iPrimitiveType = iPropertyPrimitive;
    cur->iPropertyId = iPropertyId;
    cur->lpValue = lpValue;
    cur->dwValueLen = dwValueLen;

    cur->origin = PO_GLOBAL;

    cur->next = tf->metrics;
    tf->metrics = cur;
    return cur;
}

/* Color-related state for theme ini parsing */
struct PARSECOLORSTATE
{
    int colorCount;
    int colorElements[TMT_LASTCOLOR-TMT_FIRSTCOLOR+1];
    COLORREF colorRgb[TMT_LASTCOLOR-TMT_FIRSTCOLOR+1];
    int captionColors;
};

static inline void parse_init_color (struct PARSECOLORSTATE* state)
{
    memset (state, 0, sizeof (*state));
}

static BOOL parse_handle_color_property (struct PARSECOLORSTATE* state, 
                                         int iPropertyId, LPCWSTR lpValue,
                                         DWORD dwValueLen)
{
    int r,g,b;
    LPCWSTR lpValueEnd = lpValue + dwValueLen;
    if(MSSTYLES_GetNextInteger(lpValue, lpValueEnd, &lpValue, &r) &&
    MSSTYLES_GetNextInteger(lpValue, lpValueEnd, &lpValue, &g) &&
    MSSTYLES_GetNextInteger(lpValue, lpValueEnd, &lpValue, &b)) {
	state->colorElements[state->colorCount] = iPropertyId - TMT_FIRSTCOLOR;
	state->colorRgb[state->colorCount++] = RGB(r,g,b);
	switch (iPropertyId)
	{
	  case TMT_ACTIVECAPTION: 
	    state->captionColors |= 0x1; 
	    break;
	  case TMT_INACTIVECAPTION: 
	    state->captionColors |= 0x2; 
	    break;
	  case TMT_GRADIENTACTIVECAPTION: 
	    state->captionColors |= 0x4; 
	    break;
	  case TMT_GRADIENTINACTIVECAPTION: 
	    state->captionColors |= 0x8; 
	    break;
	}
	return TRUE;
    }
    else {
	return FALSE;
    }
}

static void parse_apply_color (struct PARSECOLORSTATE* state)
{
    if (state->colorCount > 0)
	SetSysColors(state->colorCount, state->colorElements, state->colorRgb);
    if (state->captionColors == 0xf)
	SystemParametersInfoW (SPI_SETGRADIENTCAPTIONS, 0, (PVOID)TRUE, 0);
}

/* Non-client-metrics-related state for theme ini parsing */
struct PARSENONCLIENTSTATE
{
    NONCLIENTMETRICSW metrics;
    BOOL metricsDirty;
    LOGFONTW iconTitleFont;
};

static inline void parse_init_nonclient (struct PARSENONCLIENTSTATE* state)
{
    DPI_AWARENESS_CONTEXT old_context;

    old_context = SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE);

    memset (state, 0, sizeof (*state));
    state->metrics.cbSize = sizeof (NONCLIENTMETRICSW);
    SystemParametersInfoW (SPI_GETNONCLIENTMETRICS, sizeof (NONCLIENTMETRICSW),
        &state->metrics, 0);
    SystemParametersInfoW (SPI_GETICONTITLELOGFONT, sizeof (LOGFONTW),
        &state->iconTitleFont, 0);

    SetThreadDpiAwarenessContext(old_context);
}

static BOOL parse_handle_nonclient_font (struct PARSENONCLIENTSTATE* state, 
                                         int iPropertyId, LPCWSTR lpValue,
                                         DWORD dwValueLen)
{
    LOGFONTW font;
    
    memset (&font, 0, sizeof (font));
    if (SUCCEEDED (MSSTYLES_GetFont (lpValue, lpValue + dwValueLen, &lpValue,
        &font)))
    {
        switch (iPropertyId)
        {
	  case TMT_CAPTIONFONT:
	      state->metrics.lfCaptionFont = font;
	      state->metricsDirty = TRUE;
	      break;
	  case TMT_SMALLCAPTIONFONT:
	      state->metrics.lfSmCaptionFont = font;
	      state->metricsDirty = TRUE;
	      break;
	  case TMT_MENUFONT:
	      state->metrics.lfMenuFont = font;
	      state->metricsDirty = TRUE;
	      break;
	  case TMT_STATUSFONT:
	      state->metrics.lfStatusFont = font;
	      state->metricsDirty = TRUE;
	      break;
	  case TMT_MSGBOXFONT:
	      state->metrics.lfMessageFont = font;
	      state->metricsDirty = TRUE;
	      break;
	  case TMT_ICONTITLEFONT:
	      state->iconTitleFont = font;
	      state->metricsDirty = TRUE;
	      break;
        }
        return TRUE;
    }
    else
        return FALSE;
}

static BOOL parse_handle_nonclient_size (struct PARSENONCLIENTSTATE* state, 
                                         int iPropertyId, LPCWSTR lpValue,
                                         DWORD dwValueLen)
{
    int size;
    LPCWSTR lpValueEnd = lpValue + dwValueLen;
    if(MSSTYLES_GetNextInteger(lpValue, lpValueEnd, &lpValue, &size)) {
        switch (iPropertyId)
        {
            case TMT_SIZINGBORDERWIDTH:
                state->metrics.iBorderWidth = size;
                state->metricsDirty = TRUE;
                break;
            case TMT_SCROLLBARWIDTH:
                state->metrics.iScrollWidth = size;
                state->metricsDirty = TRUE;
                break;
            case TMT_SCROLLBARHEIGHT:
                state->metrics.iScrollHeight = size;
                state->metricsDirty = TRUE;
                break;
            case TMT_CAPTIONBARWIDTH:
                state->metrics.iCaptionWidth = size;
                state->metricsDirty = TRUE;
                break;
            case TMT_CAPTIONBARHEIGHT:
                state->metrics.iCaptionHeight = size;
                state->metricsDirty = TRUE;
                break;
            case TMT_SMCAPTIONBARWIDTH:
                state->metrics.iSmCaptionWidth = size;
                state->metricsDirty = TRUE;
                break;
            case TMT_SMCAPTIONBARHEIGHT:
                state->metrics.iSmCaptionHeight = size;
                state->metricsDirty = TRUE;
                break;
            case TMT_MENUBARWIDTH:
                state->metrics.iMenuWidth = size;
                state->metricsDirty = TRUE;
                break;
            case TMT_MENUBARHEIGHT:
                state->metrics.iMenuHeight = size;
                state->metricsDirty = TRUE;
                break;
        }
        return TRUE;
    }
    else
        return FALSE;
}

static void parse_apply_nonclient (struct PARSENONCLIENTSTATE* state)
{
    DPI_AWARENESS_CONTEXT old_context;

    if (state->metricsDirty)
    {
        old_context = SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE);
        SystemParametersInfoW (SPI_SETNONCLIENTMETRICS, sizeof (state->metrics),
            &state->metrics, 0);
        SystemParametersInfoW (SPI_SETICONTITLELOGFONT, sizeof (state->iconTitleFont),
            &state->iconTitleFont, 0);
        SetThreadDpiAwarenessContext(old_context);
    }
}

/***********************************************************************
 *      MSSTYLES_ParseThemeIni
 *
 * Parse the theme ini for the selected color/style
 *
 * PARAMS
 *     tf                  Theme to parse
 */
static void MSSTYLES_ParseThemeIni(PTHEME_FILE tf, BOOL setMetrics)
{
    PTHEME_CLASS cls;
    PTHEME_CLASS globals;
    PTHEME_PARTSTATE ps;
    PUXINI_FILE ini;
    WCHAR szAppName[MAX_THEME_APP_NAME];
    WCHAR szClassName[MAX_THEME_CLASS_NAME];
    WCHAR szPropertyName[MAX_THEME_VALUE_NAME];
    int iPartId;
    int iStateId;
    int iPropertyPrimitive;
    int iPropertyId;
    DWORD dwLen;
    LPCWSTR lpName;
    DWORD dwValueLen;
    LPCWSTR lpValue;

    ini = MSSTYLES_GetActiveThemeIni(tf);

    while((lpName=UXINI_GetNextSection(ini, &dwLen))) {
        if(CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, lpName, dwLen, L"SysMetrics", -1) == CSTR_EQUAL) {
            struct PARSECOLORSTATE colorState;
            struct PARSENONCLIENTSTATE nonClientState;
            
            parse_init_color (&colorState);
            parse_init_nonclient (&nonClientState);

            while((lpName=UXINI_GetNextValue(ini, &dwLen, &lpValue, &dwValueLen))) {
                lstrcpynW(szPropertyName, lpName, min(dwLen+1, ARRAY_SIZE(szPropertyName)));
                if(MSSTYLES_LookupProperty(szPropertyName, &iPropertyPrimitive, &iPropertyId)) {
                    if(iPropertyId >= TMT_FIRSTCOLOR && iPropertyId <= TMT_LASTCOLOR) {
                        if (!parse_handle_color_property (&colorState, iPropertyId, 
                            lpValue, dwValueLen))
                            FIXME("Invalid color value for %s\n", 
                                debugstr_w(szPropertyName)); 
                    }
		    else if (setMetrics && (iPropertyId == TMT_FLATMENUS)) {
			BOOL flatMenus = (*lpValue == 'T') || (*lpValue == 't');
			SystemParametersInfoW (SPI_SETFLATMENU, 0, (PVOID)(INT_PTR)flatMenus, 0);
		    }
		    else if ((iPropertyId >= TMT_FIRSTFONT) 
			&& (iPropertyId <= TMT_LASTFONT))
		    {
		        if (!parse_handle_nonclient_font (&nonClientState,
		            iPropertyId, lpValue, dwValueLen))
                            FIXME("Invalid font value for %s\n", 
                                debugstr_w(szPropertyName)); 
		    }
		    else if ((iPropertyId >= TMT_FIRSTSIZE)
			&& (iPropertyId <= TMT_LASTSIZE))
		    {
		        if (!parse_handle_nonclient_size (&nonClientState,
		            iPropertyId, lpValue, dwValueLen))
                            FIXME("Invalid size value for %s\n", 
                                debugstr_w(szPropertyName)); 
		    }
                    /* Catch all metrics, including colors */
                    MSSTYLES_AddMetric(tf, iPropertyPrimitive, iPropertyId, lpValue, dwValueLen);
                }
                else {
                    TRACE("Unknown system metric %s\n", debugstr_w(szPropertyName));
                }
            }
            if (setMetrics) 
            {
                parse_apply_color (&colorState);
		parse_apply_nonclient (&nonClientState);
	    }
            continue;
        }
        if(MSSTYLES_ParseIniSectionName(lpName, dwLen, szAppName, szClassName, &iPartId, &iStateId)) {
            BOOL isGlobal = FALSE;
            if(!lstrcmpiW(szClassName, L"globals")) {
                isGlobal = TRUE;
            }
            cls = MSSTYLES_AddClass(tf, szAppName, szClassName);
            ps = MSSTYLES_AddPartState(cls, iPartId, iStateId);

            while((lpName=UXINI_GetNextValue(ini, &dwLen, &lpValue, &dwValueLen))) {
                lstrcpynW(szPropertyName, lpName, min(dwLen+1, ARRAY_SIZE(szPropertyName)));
                if(MSSTYLES_LookupProperty(szPropertyName, &iPropertyPrimitive, &iPropertyId)) {
                    MSSTYLES_AddProperty(ps, iPropertyPrimitive, iPropertyId, lpValue, dwValueLen, isGlobal);
                }
                else {
                    TRACE("Unknown property %s\n", debugstr_w(szPropertyName));
                }
            }
        }
    }

    /* App/Class combos override values defined by the base class, map these overrides */
    globals = MSSTYLES_FindClass(tf, NULL, L"globals");
    cls = tf->classes;
    while(cls) {
        if(*cls->szAppName) {
            cls->overrides = MSSTYLES_FindClass(tf, NULL, cls->szClassName);
            if(!cls->overrides) {
                TRACE("No overrides found for app %s class %s\n", debugstr_w(cls->szAppName), debugstr_w(cls->szClassName));
            }
            else {
                cls->overrides = globals;
            }
        }
        else {
            /* Everything overrides globals..except globals */
            if(cls != globals) cls->overrides = globals;
        }
        cls = cls->next;
    }
    UXINI_CloseINI(ini);

    if(!tf->classes) {
        ERR("Failed to parse theme ini\n");
    }
}

static void parse_app_class_name(LPCWSTR name, LPWSTR app_name, LPWSTR class_name)
{
    LPCWSTR p;

    app_name[0] = class_name[0] = 0;

    p = wcsstr(name, L"::");
    if (p)
    {
        lstrcpynW(app_name, name, min(p - name + 1, MAX_THEME_APP_NAME));
        p += 2;
        lstrcpynW(class_name, p, min(wcslen(p) + 1, MAX_THEME_CLASS_NAME));
    }
    else
        lstrcpynW(class_name, name, MAX_THEME_CLASS_NAME);
}

/***********************************************************************
 *      MSSTYLES_OpenThemeClass
 *
 * Open a theme class, uses the current active theme
 *
 * PARAMS
 *     pszAppName          Application name, for theme styles specific
 *                         to a particular application
 *     pszClassList        List of requested classes, semicolon delimited
 *     dpi                 DPI for theme parts
 */
PTHEME_CLASS MSSTYLES_OpenThemeClass(LPCWSTR pszAppName, LPCWSTR pszClassList, UINT dpi)
{
    PTHEME_CLASS cls = NULL;
    WCHAR buf[MAX_THEME_APP_NAME + MAX_THEME_CLASS_NAME];
    WCHAR szAppName[MAX_THEME_APP_NAME];
    WCHAR szClassName[MAX_THEME_CLASS_NAME];
    LPCWSTR start;
    LPCWSTR end;
    DWORD len;

    if(!tfActiveTheme) {
        TRACE("there is no active theme\n");
        return NULL;
    }
    if(!tfActiveTheme->classes) {
	return NULL;
    }

    start = pszClassList;
    while((end = wcschr(start, ';'))) {
        len = end-start;
        lstrcpynW(buf, start, min(len+1, ARRAY_SIZE(buf)));
        start = end+1;

        parse_app_class_name(buf, szAppName, szClassName);

        /* If the window application name is set then fail */
        if (szAppName[0] && pszAppName)
            return NULL;

        cls = MSSTYLES_FindClass(tfActiveTheme, szAppName[0] ? szAppName : pszAppName, szClassName);
        /* Fall back to default class if the specified subclass is not found */
        if (!cls) cls = MSSTYLES_FindClass(tfActiveTheme, NULL, szClassName);

        if(cls) break;
    }
    if(!cls && *start) {
        parse_app_class_name(start, szAppName, szClassName);

        /* If the window application name is set then fail */
        if (szAppName[0] && pszAppName)
            return NULL;

        cls = MSSTYLES_FindClass(tfActiveTheme, szAppName[0] ? szAppName : pszAppName, szClassName);
        /* Fall back to default class if the specified subclass is not found */
        if (!cls) cls = MSSTYLES_FindClass(tfActiveTheme, NULL, szClassName);
    }
    if(cls) {
        TRACE("Opened app %s, class %s from list %s\n", debugstr_w(cls->szAppName), debugstr_w(cls->szClassName), debugstr_w(pszClassList));
	cls->tf = tfActiveTheme;
        InterlockedIncrement(&cls->tf->refcount);
        InterlockedIncrement(&cls->refcount);
        cls->dpi = dpi;
    }
    return cls;
}

/***********************************************************************
 *      MSSTYLES_CloseThemeClass
 *
 * Close a theme class
 *
 * PARAMS
 *     tc                  Theme class to close
 *
 * NOTES
 *  The MSSTYLES_CloseThemeFile decreases the refcount of the owning
 *  theme file and cleans it up, if needed.
 */
HRESULT MSSTYLES_CloseThemeClass(PTHEME_CLASS tc)
{
    LONG refcount;

    __TRY
    {
        if (tc->signature != THEME_CLASS_SIGNATURE)
            tc = NULL;
    }
    __EXCEPT_PAGE_FAULT
    {
        tc = NULL;
    }
    __ENDTRY

    if (!tc)
    {
        WARN("Invalid theme class handle\n");
        return E_HANDLE;
    }

    refcount = InterlockedDecrement(&tc->refcount);
    /* Some buggy apps may double free HTHEME handles */
    if (refcount >= 0)
        MSSTYLES_CloseThemeFile(tc->tf);
    return S_OK;
}

/***********************************************************************
 *      MSSTYLES_FindProperty
 *
 * Locate a property in a class. Part and state IDs will be used as a
 * preference, but may be ignored in the attempt to locate the property.
 * Will scan the entire chain of overrides for this class.
 */
PTHEME_PROPERTY MSSTYLES_FindProperty(PTHEME_CLASS tc, int iPartId, int iStateId, int iPropertyPrimitive, int iPropertyId)
{
    PTHEME_CLASS next = tc;
    PTHEME_PARTSTATE ps;
    PTHEME_PROPERTY tp;

    TRACE("(%p, %d, %d, %d)\n", tc, iPartId, iStateId, iPropertyId);
     /* Try and find an exact match on part & state */
    while(next && (ps = MSSTYLES_FindPartState(next, iPartId, iStateId, &next))) {
        if((tp = MSSTYLES_PSFindProperty(ps, iPropertyPrimitive, iPropertyId))) {
            return tp;
        }
    }
    /* If that fails, and we didn't already try it, search for just part */
    if(iStateId != 0)
        iStateId = 0;
    /* As a last ditch attempt..go for just class */
    else if(iPartId != 0)
        iPartId = 0;
    else
        return NULL;

    if((tp = MSSTYLES_FindProperty(tc, iPartId, iStateId, iPropertyPrimitive, iPropertyId)))
        return tp;
    return NULL;
}

/* Prepare a bitmap to be used for alpha blending */
static BOOL prepare_alpha (HBITMAP bmp, BOOL* hasAlpha, BOOL *hasDefaultTransparentColour)
{
    DIBSECTION dib;
    int n, stride;
    BYTE* p;

    *hasAlpha = FALSE;
    *hasDefaultTransparentColour = FALSE;

    if (!bmp || GetObjectW( bmp, sizeof(dib), &dib ) != sizeof(dib))
        return FALSE;

    if (dib.dsBm.bmBitsPixel <= 8)
    {
        RGBQUAD p[256];
        HDC hdc = CreateCompatibleDC(NULL);
        HBITMAP prev = SelectObject(hdc, bmp);
        UINT count = GetDIBColorTable(hdc, 0, 256, p);

        SelectObject(hdc, prev);
        DeleteDC(hdc);

        for (n = 0; n < count; ++n)
        {
            if (RGB(p[n].rgbRed, p[n].rgbGreen, p[n].rgbBlue) == DEFAULT_TRANSPARENT_COLOR)
            {
                *hasDefaultTransparentColour = TRUE;
                return TRUE;
            }
        }
        return TRUE;
    }

    if (dib.dsBm.bmBitsPixel == 16)
    {
        unsigned short transparent_color;
        int y;

        if (dib.dsBmih.biCompression == BI_RGB)
        {
            transparent_color = 0x7c00 | 0x001f;
        }
        else if (dib.dsBmih.biCompression == BI_BITFIELDS && dib.dsBitfields[0])
        {
            transparent_color = dib.dsBitfields[0] | dib.dsBitfields[2];
        }
        else
        {
            WARN("biCompression %ld, bpp %d not checked for default transparent colour.\n", dib.dsBmih.biCompression, dib.dsBm.bmBitsPixel);
            return TRUE;
        }
        stride = (dib.dsBmih.biWidth * 2 + 3) & ~3;
        p = dib.dsBm.bmBits;
        for (y = 0; y < dib.dsBmih.biHeight; ++y)
        {
            p = (BYTE *)dib.dsBm.bmBits + stride * y;
            for (n = 0; n < dib.dsBmih.biWidth; ++n, p += 2)
            {
                if ((p[0] | (p[1] << 8)) == transparent_color)
                {
                    *hasDefaultTransparentColour = TRUE;
                    return TRUE;
                }
            }
        }
        return TRUE;
    }

    if (dib.dsBmih.biCompression != BI_RGB)
    {
        WARN("biCompression %ld, bpp %d not checked for default transparent colour.\n", dib.dsBmih.biCompression, dib.dsBm.bmBitsPixel);
        return TRUE;
    }

    if (dib.dsBm.bmBitsPixel == 24)
    {
        int y;

        stride = (dib.dsBmih.biWidth * 3 + 3) & ~3;
        p = dib.dsBm.bmBits;
        for (y = 0; y < dib.dsBmih.biHeight; ++y)
        {
            p = (BYTE *)dib.dsBm.bmBits + stride * y;
            for (n = 0; n < dib.dsBmih.biWidth; ++n, p += 3)
            {
                if (RGB(p[0], p[1], p[2]) == DEFAULT_TRANSPARENT_COLOR)
                {
                    *hasDefaultTransparentColour = TRUE;
                    return TRUE;
                }
            }
        }
        return TRUE;
    }

    if (dib.dsBm.bmBitsPixel != 32)
        return TRUE;

    /* If all alpha values are 0xff, don't use alpha blending */
    for (n = 0, p = dib.dsBm.bmBits; n < dib.dsBmih.biWidth * dib.dsBmih.biHeight; n++, p += 4)
    {
        if ((*hasAlpha = (p[3] != 0xff)))
            break;
        if (RGB(p[0], p[1], p[2]) == DEFAULT_TRANSPARENT_COLOR)
            *hasDefaultTransparentColour = TRUE;
    }

    if (!*hasAlpha)
        return TRUE;

    p = dib.dsBm.bmBits;
    n = dib.dsBmih.biHeight * dib.dsBmih.biWidth;
    /* AlphaBlend() wants premultiplied alpha, so do that now */
    while (n-- > 0)
    {
        int a = p[3]+1;
        p[0] = (p[0] * a) >> 8;
        p[1] = (p[1] * a) >> 8;
        p[2] = (p[2] * a) >> 8;
        p += 4;
    }

    return TRUE;
}

HBITMAP MSSTYLES_LoadBitmap (PTHEME_CLASS tc, LPCWSTR lpFilename, BOOL* hasAlpha, BOOL *hasDefaultTransparentColour)
{
    WCHAR szFile[MAX_PATH];
    LPWSTR tmp;
    PTHEME_IMAGE img;
    BOOL has_default;

    lstrcpynW(szFile, lpFilename, ARRAY_SIZE(szFile));
    tmp = szFile;
    do {
        if(*tmp == '\\') *tmp = '_';
        if(*tmp == '/') *tmp = '_';
        if(*tmp == '.') *tmp = '_';
    } while(*tmp++);

    /* Try to locate in list of loaded images */
    img = tc->tf->images;
    while (img)
    {
        if (lstrcmpiW (szFile, img->name) == 0)
        {
            TRACE ("found %p %s: %p\n", img, debugstr_w (img->name), img->image);
            *hasAlpha = img->hasAlpha;
            if (hasDefaultTransparentColour)
                *hasDefaultTransparentColour = img->hasDefaultTransparentColour;
            return img->image;
        }
        img = img->next;
    }
    /* Not found? Load from resources */
    img = malloc(sizeof(*img));
    img->image = LoadImageW(tc->hTheme, szFile, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    prepare_alpha (img->image, hasAlpha, &has_default);
    img->hasAlpha = *hasAlpha;
    img->hasDefaultTransparentColour = has_default;
    if (hasDefaultTransparentColour)
        *hasDefaultTransparentColour = has_default;
    /* ...and stow away for later reuse. */
    lstrcpyW (img->name, szFile);
    img->next = tc->tf->images;
    tc->tf->images = img;
    TRACE ("new %p %s: %p\n", img, debugstr_w (img->name), img->image);
    return img->image;
}

static BOOL MSSTYLES_GetNextLong(LPCWSTR lpStringStart, LPCWSTR lpStringEnd, LPCWSTR *lpValEnd, LONG *value)
{
    LPCWSTR cur = lpStringStart;
    LONG total = 0;
    BOOL gotNeg = FALSE;

    while(cur < lpStringEnd && (*cur < '0' || *cur > '9' || *cur == '-')) cur++;
    if(cur >= lpStringEnd) {
        return FALSE;
    }
    if(*cur == '-') {
        cur++;
        gotNeg = TRUE;
    }
    while(cur < lpStringEnd && (*cur >= '0' && *cur <= '9')) {
        total = total * 10 + (*cur - '0');
        cur++;
    }
    if(gotNeg) total = -total;
    *value = total;
    if(lpValEnd) *lpValEnd = cur;
    return TRUE;
}

static BOOL MSSTYLES_GetNextInteger(LPCWSTR lpStringStart, LPCWSTR lpStringEnd, LPCWSTR *lpValEnd, int *value)
{
    return MSSTYLES_GetNextLong(lpStringStart, lpStringEnd, lpValEnd, (LONG *)value);
}

static inline BOOL isSpace(WCHAR c)
{
    return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v';
}

static BOOL MSSTYLES_GetNextToken(LPCWSTR lpStringStart, LPCWSTR lpStringEnd, LPCWSTR *lpValEnd, LPWSTR lpBuff, DWORD buffSize) {
    LPCWSTR cur = lpStringStart;
    LPCWSTR start;
    LPCWSTR end;

    while(cur < lpStringEnd && (isSpace(*cur) || *cur == ',')) cur++;
    if(cur >= lpStringEnd) {
        return FALSE;
    }
    start = cur;
    while(cur < lpStringEnd && *cur != ',') cur++;
    end = cur;
    while(isSpace(*end)) end--;

    lstrcpynW(lpBuff, start, min(buffSize, end-start+1));

    if(lpValEnd) *lpValEnd = cur;
    return TRUE;
}

/***********************************************************************
 *      MSSTYLES_GetPropertyBool
 *
 * Retrieve a color value for a property 
 */
HRESULT MSSTYLES_GetPropertyBool(PTHEME_PROPERTY tp, BOOL *pfVal)
{
    *pfVal = FALSE;
    if(*tp->lpValue == 't' || *tp->lpValue == 'T')
        *pfVal = TRUE;
    return S_OK;
}

/***********************************************************************
 *      MSSTYLES_GetPropertyColor
 *
 * Retrieve a color value for a property 
 */
HRESULT MSSTYLES_GetPropertyColor(PTHEME_PROPERTY tp, COLORREF *pColor)
{
    LPCWSTR lpEnd;
    LPCWSTR lpCur;
    int red, green, blue;

    lpCur = tp->lpValue;
    lpEnd = tp->lpValue + tp->dwValueLen;

    if(!MSSTYLES_GetNextInteger(lpCur, lpEnd, &lpCur, &red)) {
        TRACE("Could not parse color property\n");
        return E_PROP_ID_UNSUPPORTED;
    }
    if(!MSSTYLES_GetNextInteger(lpCur, lpEnd, &lpCur, &green)) {
        TRACE("Could not parse color property\n");
        return E_PROP_ID_UNSUPPORTED;
    }
    if(!MSSTYLES_GetNextInteger(lpCur, lpEnd, &lpCur, &blue)) {
        TRACE("Could not parse color property\n");
        return E_PROP_ID_UNSUPPORTED;
    }
    *pColor = RGB(red,green,blue);
    return S_OK;
}

/***********************************************************************
 *      MSSTYLES_GetPropertyColor
 *
 * Retrieve a color value for a property 
 */
static HRESULT MSSTYLES_GetFont (LPCWSTR lpCur, LPCWSTR lpEnd,
                                 LPCWSTR *lpValEnd, LOGFONTW* pFont)
{
    int pointSize;
    WCHAR attr[32];

    if(!MSSTYLES_GetNextToken(lpCur, lpEnd, &lpCur, pFont->lfFaceName, LF_FACESIZE)) {
        TRACE("Property is there, but failed to get face name\n");
        *lpValEnd = lpCur;
        return E_PROP_ID_UNSUPPORTED;
    }
    if(!MSSTYLES_GetNextInteger(lpCur, lpEnd, &lpCur, &pointSize)) {
        TRACE("Property is there, but failed to get point size\n");
        *lpValEnd = lpCur;
        return E_PROP_ID_UNSUPPORTED;
    }
    pFont->lfHeight = pointSize;
    pFont->lfWeight = FW_REGULAR;
    pFont->lfCharSet = DEFAULT_CHARSET;
    while(MSSTYLES_GetNextToken(lpCur, lpEnd, &lpCur, attr, ARRAY_SIZE(attr))) {
        if(!lstrcmpiW(L"bold", attr)) pFont->lfWeight = FW_BOLD;
        else if(!lstrcmpiW(L"italic", attr)) pFont->lfItalic = TRUE;
        else if(!lstrcmpiW(L"underline", attr)) pFont->lfUnderline = TRUE;
        else if(!lstrcmpiW(L"strikeout", attr)) pFont->lfStrikeOut = TRUE;
    }
    *lpValEnd = lpCur;
    return S_OK;
}

HRESULT MSSTYLES_GetPropertyFont(PTHEME_PROPERTY tp, HDC hdc, LOGFONTW *pFont)
{
    LPCWSTR lpCur = tp->lpValue;
    LPCWSTR lpEnd = tp->lpValue + tp->dwValueLen;
    HRESULT hr; 

    ZeroMemory(pFont, sizeof(LOGFONTW));
    hr = MSSTYLES_GetFont (lpCur, lpEnd, &lpCur, pFont);
    if (SUCCEEDED (hr))
        pFont->lfHeight = -MulDiv(pFont->lfHeight, GetDeviceCaps(hdc, LOGPIXELSY), 72);

    return hr;
}

/***********************************************************************
 *      MSSTYLES_GetPropertyInt
 *
 * Retrieve an int value for a property 
 */
HRESULT MSSTYLES_GetPropertyInt(PTHEME_PROPERTY tp, int *piVal)
{
    if(!MSSTYLES_GetNextInteger(tp->lpValue, (tp->lpValue + tp->dwValueLen), NULL, piVal)) {
        TRACE("Could not parse int property\n");
        return E_PROP_ID_UNSUPPORTED;
    }
    return S_OK;
}

/***********************************************************************
 *      MSSTYLES_GetPropertyIntList
 *
 * Retrieve an int list value for a property 
 */
HRESULT MSSTYLES_GetPropertyIntList(PTHEME_PROPERTY tp, INTLIST *pIntList)
{
    int i;
    LPCWSTR lpCur = tp->lpValue;
    LPCWSTR lpEnd = tp->lpValue + tp->dwValueLen;

    for(i=0; i < MAX_INTLIST_COUNT; i++) {
        if(!MSSTYLES_GetNextInteger(lpCur, lpEnd, &lpCur, &pIntList->iValues[i]))
            break;
    }
    pIntList->iValueCount = i;
    return S_OK;
}

/***********************************************************************
 *      MSSTYLES_GetPropertyPosition
 *
 * Retrieve a position value for a property 
 */
HRESULT MSSTYLES_GetPropertyPosition(PTHEME_PROPERTY tp, POINT *pPoint)
{
    int x,y;
    LPCWSTR lpCur = tp->lpValue;
    LPCWSTR lpEnd = tp->lpValue + tp->dwValueLen;

    if(!MSSTYLES_GetNextInteger(lpCur, lpEnd, &lpCur, &x)) {
        TRACE("Could not parse position property\n");
        return E_PROP_ID_UNSUPPORTED;
    }
    if(!MSSTYLES_GetNextInteger(lpCur, lpEnd, &lpCur, &y)) {
        TRACE("Could not parse position property\n");
        return E_PROP_ID_UNSUPPORTED;
    }
    pPoint->x = x;
    pPoint->y = y;
    return S_OK;
}

/***********************************************************************
 *      MSSTYLES_GetPropertyString
 *
 * Retrieve a string value for a property 
 */
HRESULT MSSTYLES_GetPropertyString(PTHEME_PROPERTY tp, LPWSTR pszBuff, int cchMaxBuffChars)
{
    lstrcpynW(pszBuff, tp->lpValue, min(tp->dwValueLen+1, cchMaxBuffChars));
    return S_OK;
}

/***********************************************************************
 *      MSSTYLES_GetPropertyRect
 *
 * Retrieve a rect value for a property 
 */
HRESULT MSSTYLES_GetPropertyRect(PTHEME_PROPERTY tp, RECT *pRect)
{
    LPCWSTR lpCur = tp->lpValue;
    LPCWSTR lpEnd = tp->lpValue + tp->dwValueLen;

    MSSTYLES_GetNextLong(lpCur, lpEnd, &lpCur, &pRect->left);
    MSSTYLES_GetNextLong(lpCur, lpEnd, &lpCur, &pRect->top);
    MSSTYLES_GetNextLong(lpCur, lpEnd, &lpCur, &pRect->right);
    if(!MSSTYLES_GetNextLong(lpCur, lpEnd, &lpCur, &pRect->bottom)) {
        TRACE("Could not parse rect property\n");
        return E_PROP_ID_UNSUPPORTED;
    }
    return S_OK;
}

/***********************************************************************
 *      MSSTYLES_GetPropertyMargins
 *
 * Retrieve a margins value for a property 
 */
HRESULT MSSTYLES_GetPropertyMargins(PTHEME_PROPERTY tp, RECT *prc, MARGINS *pMargins)
{
    LPCWSTR lpCur = tp->lpValue;
    LPCWSTR lpEnd = tp->lpValue + tp->dwValueLen;

    MSSTYLES_GetNextInteger(lpCur, lpEnd, &lpCur, &pMargins->cxLeftWidth);
    MSSTYLES_GetNextInteger(lpCur, lpEnd, &lpCur, &pMargins->cxRightWidth);
    MSSTYLES_GetNextInteger(lpCur, lpEnd, &lpCur, &pMargins->cyTopHeight);
    if(!MSSTYLES_GetNextInteger(lpCur, lpEnd, &lpCur, &pMargins->cyBottomHeight)) {
        TRACE("Could not parse margins property\n");
        return E_PROP_ID_UNSUPPORTED;
    }
    return S_OK;
}
