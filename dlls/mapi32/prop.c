/*
 * Property functions
 *
 * Copyright 2004 Jon Griffiths
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

#include <stdarg.h>
#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winerror.h"
#include "winternl.h"
#include "objbase.h"
#include "shlwapi.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "mapival.h"

WINE_DEFAULT_DEBUG_CHANNEL(mapi);

BOOL WINAPI FBadRglpszA(LPSTR*,ULONG);
BOOL WINAPI FBadRglpszW(LPWSTR*,ULONG);

/* Internal: Check if a property value array is invalid */
static inline ULONG PROP_BadArray(LPSPropValue lpProp, size_t elemSize)
{
    return IsBadReadPtr(lpProp->Value.MVi.lpi, lpProp->Value.MVi.cValues * elemSize);
}

/*************************************************************************
 * PropCopyMore@16 (MAPI32.76)
 *
 * Copy a property value.
 *
 * PARAMS
 *  lpDest [O] Destination for the copied value
 *  lpSrc  [I] Property value to copy to lpDest
 *  lpMore [I] Linked memory allocation function (pass MAPIAllocateMore())
 *  lpOrig [I] Original allocation to which memory will be linked
 *
 * RETURNS
 *  Success: S_OK. lpDest contains a deep copy of lpSrc.
 *  Failure: MAPI_E_INVALID_PARAMETER, if any parameter is invalid,
 *           MAPI_E_NOT_ENOUGH_MEMORY, if memory allocation fails.
 *
 * NOTES
 *  Any elements within the property returned should not be individually
 *  freed, as they will be freed when lpOrig is.
 */
SCODE WINAPI PropCopyMore(LPSPropValue lpDest, LPSPropValue lpSrc, 
                          ALLOCATEMORE *lpMore, LPVOID lpOrig)
{
    ULONG ulLen, i;
    SCODE scode = S_OK;
    
    TRACE("(%p,%p,%p,%p)\n", lpDest, lpSrc, lpMore, lpOrig);
    
    if (!lpDest || IsBadWritePtr(lpDest, sizeof(SPropValue)) ||
        FBadProp(lpSrc) || !lpMore)
        return MAPI_E_INVALID_PARAMETER;

    /* Shallow copy first, this is sufficient for properties without pointers */
    *lpDest = *lpSrc;
        
   switch (PROP_TYPE(lpSrc->ulPropTag))
    {
    case PT_CLSID:
        scode = lpMore(sizeof(GUID), lpOrig, (LPVOID*)&lpDest->Value.lpguid);
        if (SUCCEEDED(scode))
            memcpy(lpDest->Value.lpguid, lpSrc->Value.lpguid, sizeof(GUID));
        break;
    case PT_STRING8:
        ulLen = lstrlenA(lpSrc->Value.lpszA) + 1u;
        scode = lpMore(ulLen, lpOrig, (LPVOID*)&lpDest->Value.lpszA);
        if (SUCCEEDED(scode))
            memcpy(lpDest->Value.lpszA, lpSrc->Value.lpszA, ulLen);
        break;
    case PT_UNICODE:
        ulLen = (strlenW(lpSrc->Value.lpszW) + 1u) * sizeof(WCHAR);
        scode = lpMore(ulLen, lpOrig, (LPVOID*)&lpDest->Value.lpszW);
        if (SUCCEEDED(scode))
            memcpy(lpDest->Value.lpszW, lpSrc->Value.lpszW, ulLen);
        break;
    case PT_BINARY:
        scode = lpMore(lpSrc->Value.bin.cb, lpOrig, (LPVOID*)&lpDest->Value.bin.lpb);
        if (SUCCEEDED(scode))
            memcpy(lpDest->Value.bin.lpb, lpSrc->Value.bin.lpb, lpSrc->Value.bin.cb);
        break;
    default:
        if (lpSrc->ulPropTag & MV_FLAG)
        {
            ulLen = UlPropSize(lpSrc);

            if (PROP_TYPE(lpSrc->ulPropTag) == PT_MV_STRING8 ||
                PROP_TYPE(lpSrc->ulPropTag) == PT_MV_UNICODE)
            {
                /* UlPropSize doesn't account for the string pointers */
                ulLen += lpSrc->Value.MVszA.cValues * sizeof(char*);
            }
            else if (PROP_TYPE(lpSrc->ulPropTag) == PT_MV_BINARY)
            {
               /* UlPropSize doesn't account for the SBinary structs */
               ulLen += lpSrc->Value.MVbin.cValues * sizeof(SBinary);
            }

            lpDest->Value.MVi.cValues = lpSrc->Value.MVi.cValues;
            scode = lpMore(ulLen, lpOrig, (LPVOID*)&lpDest->Value.MVi.lpi);
            if (FAILED(scode))
                break;

            /* Note that we could allocate the memory for each value in a 
             * multi-value property seperately, however if an allocation failed
             * we would be left with a bunch of allocated memory, which (while
             * not really leaked) is unusable until lpOrig is freed. So for 
             * strings and binary arrays we make a single allocation for all 
             * of the data. This is consistent since individual elements can't
             * be freed anyway.
             */
                                
            switch (PROP_TYPE(lpSrc->ulPropTag))
            {
            case PT_MV_STRING8:
            {
                char *lpNextStr = (char*)(lpDest->Value.MVszA.lppszA + 
                                          lpDest->Value.MVszA.cValues);
                
                for (i = 0; i < lpSrc->Value.MVszA.cValues; i++)
                {
                    ULONG ulStrLen = lstrlenA(lpSrc->Value.MVszA.lppszA[i]) + 1u;
                    
                    lpDest->Value.MVszA.lppszA[i] = lpNextStr;
                    memcpy(lpNextStr, lpSrc->Value.MVszA.lppszA[i], ulStrLen);
                    lpNextStr += ulStrLen;
                }                    
                break;
            }
            case PT_MV_UNICODE:
            {
                WCHAR *lpNextStr = (WCHAR*)(lpDest->Value.MVszW.lppszW + 
                                            lpDest->Value.MVszW.cValues);

                for (i = 0; i < lpSrc->Value.MVszW.cValues; i++)
                {
                    ULONG ulStrLen = strlenW(lpSrc->Value.MVszW.lppszW[i]) + 1u;
                    
                    lpDest->Value.MVszW.lppszW[i] = lpNextStr;
                    memcpy(lpNextStr, lpSrc->Value.MVszW.lppszW[i], ulStrLen * sizeof(WCHAR));
                    lpNextStr += ulStrLen;
                }                    
                break;
            }
            case PT_MV_BINARY:
            {
                LPBYTE lpNext = (LPBYTE)(lpDest->Value.MVbin.lpbin + 
                                         lpDest->Value.MVbin.cValues);
                
                for (i = 0; i < lpSrc->Value.MVszW.cValues; i++)
                {
                    lpDest->Value.MVbin.lpbin[i].cb = lpSrc->Value.MVbin.lpbin[i].cb;
                    lpDest->Value.MVbin.lpbin[i].lpb = lpNext;
                    memcpy(lpNext, lpSrc->Value.MVbin.lpbin[i].lpb, lpDest->Value.MVbin.lpbin[i].cb);
                    lpNext += lpDest->Value.MVbin.lpbin[i].cb;
                }                    
                break;
            }
            default:
                /* No embedded pointers, just copy the data over */
                memcpy(lpDest->Value.MVi.lpi, lpSrc->Value.MVi.lpi, ulLen);
                break;
            }
            break;
        }
    }
    return scode;
}
 
/*************************************************************************
 * UlPropSize@4 (MAPI32.77)
 *
 * Determine the size of a property in bytes.
 *
 * PARAMS
 *  lpProp [I] Property to determine the size of
 *
 * RETURNS
 *  Success: The size of the value in lpProp.
 *  Failure: 0, if a multi-value (array) property is invalid or the type of lpProp
 *           is unknown.
 *
 * NOTES
 *  - The size returned does not include the size of the SPropValue struct
 *    or the size of the array of pointers for multi-valued properties that
 *    contain pointers (such as PT_MV_STRING8 or PT-MV_UNICODE).
 *  - MSDN incorrectly states that this function returns MAPI_E_CALL_FAILED if
 *    lpProp is invalid. In reality no checking is performed and this function
 *    will crash if passed an invalid property, or return 0 if the property
 *    type is PT_OBJECT or is unknown.
 */
ULONG WINAPI UlPropSize(LPSPropValue lpProp)
{
    ULONG ulRet = 1u, i;

    TRACE("(%p)\n", lpProp);

    switch (PROP_TYPE(lpProp->ulPropTag))
    {
    case PT_MV_I2:       ulRet = lpProp->Value.MVi.cValues;
    case PT_BOOLEAN:
    case PT_I2:          ulRet *= sizeof(USHORT);
                         break;
    case PT_MV_I4:       ulRet = lpProp->Value.MVl.cValues;
    case PT_ERROR:
    case PT_I4:          ulRet *= sizeof(LONG);
                         break;
    case PT_MV_I8:       ulRet = lpProp->Value.MVli.cValues;
    case PT_I8:          ulRet *= sizeof(LONG64);
                         break;
    case PT_MV_R4:       ulRet = lpProp->Value.MVflt.cValues;
    case PT_R4:          ulRet *= sizeof(float);
                         break;
    case PT_MV_APPTIME:
    case PT_MV_R8:       ulRet = lpProp->Value.MVdbl.cValues;
    case PT_APPTIME:
    case PT_R8:          ulRet *= sizeof(double);
                         break;
    case PT_MV_CURRENCY: ulRet = lpProp->Value.MVcur.cValues;
    case PT_CURRENCY:    ulRet *= sizeof(CY);
                         break;
    case PT_MV_SYSTIME:  ulRet = lpProp->Value.MVft.cValues;
    case PT_SYSTIME:     ulRet *= sizeof(FILETIME);
                         break;
    case PT_MV_CLSID:    ulRet = lpProp->Value.MVguid.cValues;
    case PT_CLSID:       ulRet *= sizeof(GUID);
                         break;
    case PT_MV_STRING8:  ulRet = 0u;
                         for (i = 0; i < lpProp->Value.MVszA.cValues; i++)
                             ulRet += (lstrlenA(lpProp->Value.MVszA.lppszA[i]) + 1u);
                         break;
    case PT_STRING8:     ulRet = lstrlenA(lpProp->Value.lpszA) + 1u;
                         break;
    case PT_MV_UNICODE:  ulRet = 0u;
                         for (i = 0; i < lpProp->Value.MVszW.cValues; i++)
                             ulRet += (strlenW(lpProp->Value.MVszW.lppszW[i]) + 1u);
                         ulRet *= sizeof(WCHAR);
                         break;
    case PT_UNICODE:     ulRet = (lstrlenW(lpProp->Value.lpszW) + 1u) * sizeof(WCHAR);
                         break;
    case PT_MV_BINARY:   ulRet = 0u;
                         for (i = 0; i < lpProp->Value.MVbin.cValues; i++)
                             ulRet += lpProp->Value.MVbin.lpbin[i].cb;
                         break;
    case PT_BINARY:      ulRet = lpProp->Value.bin.cb;
                         break;
        break;
    case PT_OBJECT:
    default:             ulRet = 0u;
                         break;
    }

    return ulRet;
}

/*************************************************************************
 * FPropContainsProp@12 (MAPI32.78)
 *
 * Find a property with a given property tag in a property array.
 *
 * PARAMS
 *  lpHaystack [I] Property to match to
 *  lpNeedle   [I] Property to find in lpHaystack
 *  ulFuzzy    [I] Flags controlling match type and strictness (FL_* flags from "mapidefs.h")
 *
 * RETURNS
 *  TRUE, if lpNeedle matches lpHaystack according to the criteria of ulFuzzy.
 *
 * NOTES
 *  Only property types of PT_STRING8 and PT_BINARY are handled by this function.
 */
BOOL WINAPI FPropContainsProp(LPSPropValue lpHaystack, LPSPropValue lpNeedle, ULONG ulFuzzy)
{
    TRACE("(%p,%p,0x%08lx)\n", lpHaystack, lpNeedle, ulFuzzy);

    if (FBadProp(lpHaystack) || FBadProp(lpNeedle) ||
        PROP_TYPE(lpHaystack->ulPropTag) != PROP_TYPE(lpNeedle->ulPropTag))
        return FALSE;

    /* FIXME: Do later versions support Unicode as well? */

    if (PROP_TYPE(lpHaystack->ulPropTag) == PT_STRING8)
    {
        DWORD dwFlags = 0, dwNeedleLen, dwHaystackLen;

        if (ulFuzzy & FL_IGNORECASE)
            dwFlags |= NORM_IGNORECASE;
        if (ulFuzzy & FL_IGNORENONSPACE)
            dwFlags |= NORM_IGNORENONSPACE;
        if (ulFuzzy & FL_LOOSE)
            dwFlags |= (NORM_IGNORECASE|NORM_IGNORENONSPACE|NORM_IGNORESYMBOLS);

        dwNeedleLen = lstrlenA(lpNeedle->Value.lpszA);
        dwHaystackLen = lstrlenA(lpHaystack->Value.lpszA);

        if ((ulFuzzy & (FL_SUBSTRING|FL_PREFIX)) == FL_PREFIX)
        {
            if (dwNeedleLen <= dwHaystackLen &&
                CompareStringA(LOCALE_USER_DEFAULT, dwFlags,
                               lpHaystack->Value.lpszA, dwNeedleLen,
                               lpNeedle->Value.lpszA, dwNeedleLen) == CSTR_EQUAL)
                return TRUE; /* needle is a prefix of haystack */
        }
        else if ((ulFuzzy & (FL_SUBSTRING|FL_PREFIX)) == FL_SUBSTRING)
        {
            LPSTR (WINAPI *pStrChrFn)(LPCSTR,WORD) = StrChrA;
            LPSTR lpStr = lpHaystack->Value.lpszA;

            if (dwFlags & NORM_IGNORECASE)
                pStrChrFn = StrChrIA;

            while ((lpStr = pStrChrFn(lpStr, *lpNeedle->Value.lpszA)) != NULL)
            {
                dwHaystackLen -= (lpStr - lpHaystack->Value.lpszA);
                if (dwNeedleLen <= dwHaystackLen &&
                    CompareStringA(LOCALE_USER_DEFAULT, dwFlags,
                               lpStr, dwNeedleLen,
                               lpNeedle->Value.lpszA, dwNeedleLen) == CSTR_EQUAL)
                    return TRUE; /* needle is a substring of haystack */
                lpStr++;
            }
        }
        else if (CompareStringA(LOCALE_USER_DEFAULT, dwFlags,
                                lpHaystack->Value.lpszA, dwHaystackLen,
                                lpNeedle->Value.lpszA, dwNeedleLen) == CSTR_EQUAL)
            return TRUE; /* full string match */
    }
    else if (PROP_TYPE(lpHaystack->ulPropTag) == PT_BINARY)
    {
        if ((ulFuzzy & (FL_SUBSTRING|FL_PREFIX)) == FL_PREFIX)
        {
            if (lpNeedle->Value.bin.cb <= lpHaystack->Value.bin.cb &&
                !memcmp(lpNeedle->Value.bin.lpb, lpHaystack->Value.bin.lpb,
                        lpNeedle->Value.bin.cb))
                return TRUE; /* needle is a prefix of haystack */
        }
        else if ((ulFuzzy & (FL_SUBSTRING|FL_PREFIX)) == FL_SUBSTRING)
        {
            ULONG ulLen = lpHaystack->Value.bin.cb;
            LPBYTE lpb = lpHaystack->Value.bin.lpb;

            while ((lpb = memchr(lpb, *lpNeedle->Value.bin.lpb, ulLen)) != NULL)
            {
                ulLen = lpHaystack->Value.bin.cb - (lpb - lpHaystack->Value.bin.lpb);
                if (lpNeedle->Value.bin.cb <= ulLen &&
                    !memcmp(lpNeedle->Value.bin.lpb, lpb, lpNeedle->Value.bin.cb))
                    return TRUE; /* needle is a substring of haystack */
                lpb++;
            }
        }
        else if (!LPropCompareProp(lpHaystack, lpNeedle))
            return TRUE; /* needle is an exact match with haystack */

    }
    return FALSE;
}

/*************************************************************************
 * FPropCompareProp@12 (MAPI32.79)
 *
 * Compare two properties.
 *
 * PARAMS
 *  lpPropLeft  [I] Left hand property to compare to lpPropRight
 *  ulOp        [I] Comparison operator (RELOP_* enum from "mapidefs.h")
 *  lpPropRight [I] Right hand property to compare to lpPropLeft
 *
 * RETURNS
 *  TRUE, if the comparison is true, FALSE otherwise.
 */
BOOL WINAPI FPropCompareProp(LPSPropValue lpPropLeft, ULONG ulOp, LPSPropValue lpPropRight)
{
    LONG iCmp;

    TRACE("(%p,%ld,%p)\n", lpPropLeft, ulOp, lpPropRight);

    if (ulOp > RELOP_RE || FBadProp(lpPropLeft) || FBadProp(lpPropRight))
        return FALSE;

    if (ulOp == RELOP_RE)
    {
        FIXME("Comparison operator RELOP_RE not yet implemented!\n");
        return FALSE;
    }

    iCmp = LPropCompareProp(lpPropLeft, lpPropRight);

    switch (ulOp)
    {
    case RELOP_LT: return iCmp <  0 ? TRUE : FALSE;
    case RELOP_LE: return iCmp <= 0 ? TRUE : FALSE;
    case RELOP_GT: return iCmp >  0 ? TRUE : FALSE;
    case RELOP_GE: return iCmp >= 0 ? TRUE : FALSE;
    case RELOP_EQ: return iCmp == 0 ? TRUE : FALSE;
    case RELOP_NE: return iCmp != 0 ? TRUE : FALSE;
    }
    return FALSE;
}

/*************************************************************************
 * LPropCompareProp@8 (MAPI32.80)
 *
 * Compare two properties.
 *
 * PARAMS
 *  lpPropLeft  [I] Left hand property to compare to lpPropRight
 *  lpPropRight [I] Right hand property to compare to lpPropLeft
 *
 * RETURNS
 *  An integer less than, equal to or greater than 0, indicating that
 *  lpszStr is less than, the same, or greater than lpszComp.
 */
LONG WINAPI LPropCompareProp(LPSPropValue lpPropLeft, LPSPropValue lpPropRight)
{
    LONG iRet;

    TRACE("(%p->0x%08lx,%p->0x%08lx)\n", lpPropLeft, lpPropLeft->ulPropTag,
          lpPropRight, lpPropRight->ulPropTag);

    /* If the properties are not the same, sort by property type */
    if (PROP_TYPE(lpPropLeft->ulPropTag) != PROP_TYPE(lpPropRight->ulPropTag))
        return (LONG)PROP_TYPE(lpPropLeft->ulPropTag) - (LONG)PROP_TYPE(lpPropRight->ulPropTag);

    switch (PROP_TYPE(lpPropLeft->ulPropTag))
    {
    case PT_UNSPECIFIED:
    case PT_NULL:
        return 0; /* NULLs are equal */
    case PT_I2:
        return lpPropLeft->Value.i - lpPropRight->Value.i;
    case PT_I4:
        return lpPropLeft->Value.l - lpPropRight->Value.l;
    case PT_I8:
        if (lpPropLeft->Value.li.QuadPart > lpPropRight->Value.li.QuadPart)
            return 1;
        if (lpPropLeft->Value.li.QuadPart == lpPropRight->Value.li.QuadPart)
            return 0;
        return -1;
    case PT_R4:
        if (lpPropLeft->Value.flt > lpPropRight->Value.flt)
            return 1;
        if (lpPropLeft->Value.flt == lpPropRight->Value.flt)
            return 0;
        return -1;
    case PT_APPTIME:
    case PT_R8:
        if (lpPropLeft->Value.dbl > lpPropRight->Value.dbl)
            return 1;
        if (lpPropLeft->Value.dbl == lpPropRight->Value.dbl)
            return 0;
        return -1;
    case PT_CURRENCY:
        if (lpPropLeft->Value.cur.int64 > lpPropRight->Value.cur.int64)
            return 1;
        if (lpPropLeft->Value.cur.int64 == lpPropRight->Value.cur.int64)
            return 0;
        return -1;
    case PT_SYSTIME:
        return CompareFileTime(&lpPropLeft->Value.ft, &lpPropRight->Value.ft);
    case PT_BOOLEAN:
        return (lpPropLeft->Value.b ? 1 : 0) - (lpPropRight->Value.b ? 1 : 0);
    case PT_BINARY:
        if (lpPropLeft->Value.bin.cb == lpPropRight->Value.bin.cb)
            iRet = memcmp(lpPropLeft->Value.bin.lpb, lpPropRight->Value.bin.lpb,
                          lpPropLeft->Value.bin.cb);
        else
        {
            iRet = memcmp(lpPropLeft->Value.bin.lpb, lpPropRight->Value.bin.lpb,
                          min(lpPropLeft->Value.bin.cb, lpPropRight->Value.bin.cb));

            if (!iRet)
                iRet = lpPropLeft->Value.bin.cb - lpPropRight->Value.bin.cb;
        }
        return iRet;
    case PT_STRING8:
        return lstrcmpA(lpPropLeft->Value.lpszA, lpPropRight->Value.lpszA);
    case PT_UNICODE:
        return strcmpW(lpPropLeft->Value.lpszW, lpPropRight->Value.lpszW);
    case PT_ERROR:
        if (lpPropLeft->Value.err > lpPropRight->Value.err)
            return 1;
        if (lpPropLeft->Value.err == lpPropRight->Value.err)
            return 0;
        return -1;
    case PT_CLSID:
        return memcmp(lpPropLeft->Value.lpguid, lpPropRight->Value.lpguid,
                      sizeof(GUID));
    }
    FIXME("Unhandled property type %ld", PROP_TYPE(lpPropLeft->ulPropTag));
    return 0;
}

/*************************************************************************
 * HrGetOneProp@8 (MAPI32.135)
 *
 * Get a property value from an IMAPIProp object.
 *
 * PARAMS
 *  lpIProp   [I] IMAPIProp object to get the property value in
 *  ulPropTag [I] Property tag of the property to get
 *  lppProp   [O] Destination for the returned property
 *
 * RETURNS
 *  Success: S_OK. *lppProp contains the property value requested.
 *  Failure: MAPI_E_NOT_FOUND, if no property value has the tag given by ulPropTag.
 */
HRESULT WINAPI HrGetOneProp(LPMAPIPROP lpIProp, ULONG ulPropTag, LPSPropValue *lppProp)
{
    SPropTagArray pta;
    ULONG ulCount;
    HRESULT hRet;

    TRACE("(%p,%ld,%p)\n", lpIProp, ulPropTag, lppProp);

    pta.cValues = 1u;
    pta.aulPropTag[0] = ulPropTag;
    hRet = IMAPIProp_GetProps(lpIProp, &pta, 0u, &ulCount, lppProp);
    if (hRet == MAPI_W_ERRORS_RETURNED)
    {
        MAPIFreeBuffer(*lppProp);
        *lppProp = NULL;
        hRet = MAPI_E_NOT_FOUND;
    }
    return hRet;
}

/*************************************************************************
 * HrSetOneProp@8 (MAPI32.136)
 *
 * Set a property value in an IMAPIProp object.
 *
 * PARAMS
 *  lpIProp [I] IMAPIProp object to set the property value in
 *  lpProp  [I] Property value to set
 *
 * RETURNS
 *  Success: S_OK. The value in lpProp is set in lpIProp.
 *  Failure: An error result from IMAPIProp_SetProps().
 */
HRESULT WINAPI HrSetOneProp(LPMAPIPROP lpIProp, LPSPropValue lpProp)
{
    TRACE("(%p,%p)\n", lpIProp, lpProp);

    return IMAPIProp_SetProps(lpIProp, 1u, lpProp, NULL);
}

/*************************************************************************
 * FPropExists@8 (MAPI32.137)
 *
 * Find a property with a given property tag in an IMAPIProp object.
 *
 * PARAMS
 *  lpIProp   [I] IMAPIProp object to find the property tag in
 *  ulPropTag [I] Property tag to find
 *
 * RETURNS
 *  TRUE, if ulPropTag matches a property held in lpIProp,
 *  FALSE, otherwise.
 *
 * NOTES
 *  if ulPropTag has a property type of PT_UNSPECIFIED, then only the property
 *  Ids need to match for a successful match to occur.
 */
 BOOL WINAPI FPropExists(LPMAPIPROP lpIProp, ULONG ulPropTag)
 {
    BOOL bRet = FALSE;

    TRACE("(%p,%ld)\n", lpIProp, ulPropTag);

    if (lpIProp)
    {
        LPSPropTagArray lpTags;
        ULONG i;

        if (FAILED(IMAPIProp_GetPropList(lpIProp, 0u, &lpTags)))
            return FALSE;

        for (i = 0; i < lpTags->cValues; i++)
        {
            if (!FBadPropTag(lpTags->aulPropTag[i]) &&
                (lpTags->aulPropTag[i] == ulPropTag ||
                 (PROP_TYPE(ulPropTag) == PT_UNSPECIFIED &&
                  PROP_ID(lpTags->aulPropTag[i]) == lpTags->aulPropTag[i])))
            {
                bRet = TRUE;
                break;
            }
        }
        MAPIFreeBuffer(lpTags);
    }
    return bRet;
}

/*************************************************************************
 * PpropFindProp@12 (MAPI32.138)
 *
 * Find a property with a given property tag in a property array.
 *
 * PARAMS
 *  lpProps   [I] Property array to search
 *  cValues   [I] Number of properties in lpProps
 *  ulPropTag [I] Property tag to find
 *
 * RETURNS
 *  A pointer to the matching property, or NULL if none was found.
 *
 * NOTES
 *  if ulPropTag has a property type of PT_UNSPECIFIED, then only the property
 *  Ids need to match for a successful match to occur.
 */
LPSPropValue WINAPI PpropFindProp(LPSPropValue lpProps, ULONG cValues, ULONG ulPropTag)
{
    TRACE("(%p,%ld,%ld)\n", lpProps, cValues, ulPropTag);

    if (lpProps && cValues)
    {
        ULONG i;
        for (i = 0; i < cValues; i++)
        {
            if (!FBadPropTag(lpProps[i].ulPropTag) &&
                (lpProps[i].ulPropTag == ulPropTag ||
                 (PROP_TYPE(ulPropTag) == PT_UNSPECIFIED &&
                  PROP_ID(lpProps[i].ulPropTag) == PROP_ID(ulPropTag))))
                return &lpProps[i];
        }
    }
    return NULL;
}

/*************************************************************************
 * FreePadrlist@4 (MAPI32.139)
 *
 * Free the memory used by an address book list.
 *
 * PARAMS
 *  lpAddrs [I] Address book list to free
 *
 * RETURNS
 *  Nothing.
 */
VOID WINAPI FreePadrlist(LPADRLIST lpAddrs)
{
    TRACE("(%p)\n", lpAddrs);

    /* Structures are binary compatible; use the same implementation */
    return FreeProws((LPSRowSet)lpAddrs);
}

/*************************************************************************
 * FreeProws@4 (MAPI32.140)
 *
 * Free the memory used by a row set.
 *
 * PARAMS
 *  lpRowSet [I] Row set to free
 *
 * RETURNS
 *  Nothing.
 */
VOID WINAPI FreeProws(LPSRowSet lpRowSet)
{
    TRACE("(%p)\n", lpRowSet);

    if (lpRowSet)
    {
        ULONG i;

        for (i = 0; i < lpRowSet->cRows; i++)
            MAPIFreeBuffer(lpRowSet->aRow[i].lpProps);

        MAPIFreeBuffer(lpRowSet);
    }
}

/*************************************************************************
 * ScCountProps@12 (MAPI32.170)
 *
 * Validate and determine the length of an array of properties.
 *
 * PARAMS
 *  iCount  [I] Length of the lpProps array
 *  lpProps [I] Array of properties to validate/size
 *  pcBytes [O] If non-NULL, destination for the size of the property array
 *
 * RETURNS
 *  Success: S_OK. If pcBytes is non-NULL, it contains the size of the propery array.
 *  Failure: MAPI_E_INVALID_PARAMETER, if any parameter is invalid or validation
 *           of the property array fails.
 */
SCODE WINAPI ScCountProps(INT iCount, LPSPropValue lpProps, ULONG *pcBytes)
{
    ULONG i, ulCount = iCount, ulBytes = 0;

    TRACE("(%d,%p,%p)\n", iCount, lpProps, pcBytes);

    if (iCount <= 0 || !lpProps ||
        IsBadReadPtr(lpProps, iCount * sizeof(SPropValue)))
        return MAPI_E_INVALID_PARAMETER;

    for (i = 0; i < ulCount; i++)
    {
        ULONG ulPropSize = 0;
        
        if (FBadProp(&lpProps[i]) || lpProps[i].ulPropTag == PROP_ID_NULL ||
            lpProps[i].ulPropTag == PROP_ID_INVALID)
            return MAPI_E_INVALID_PARAMETER;

            if (PROP_TYPE(lpProps[i].ulPropTag) != PT_OBJECT)
            {
                ulPropSize = UlPropSize(&lpProps[i]);
                if (!ulPropSize)
                    return MAPI_E_INVALID_PARAMETER;
            }
            
            switch (PROP_TYPE(lpProps[i].ulPropTag))
            {
            case PT_STRING8:
            case PT_UNICODE:
            case PT_CLSID:
            case PT_BINARY:
            case PT_MV_I2:       
            case PT_MV_I4:       
            case PT_MV_I8:       
            case PT_MV_R4:       
            case PT_MV_R8:       
            case PT_MV_CURRENCY: 
            case PT_MV_SYSTIME:  
            case PT_MV_APPTIME:
                ulPropSize += sizeof(SPropValue);
                break;
            case PT_MV_CLSID:
                ulPropSize += lpProps[i].Value.MVszA.cValues * sizeof(char*) + sizeof(SPropValue);
                break;
            case PT_MV_STRING8:
            case PT_MV_UNICODE:
                ulPropSize += lpProps[i].Value.MVszA.cValues * sizeof(char*) + sizeof(SPropValue);
                break;
            case PT_MV_BINARY:
                ulPropSize += lpProps[i].Value.MVbin.cValues * sizeof(SBinary) + sizeof(SPropValue);
                break;
            default:
                ulPropSize = sizeof(SPropValue);
                break;
            }
            ulBytes += ulPropSize;
    }
    if (pcBytes)
        *pcBytes = ulBytes;

    return S_OK;
}

/*************************************************************************
 * ScCopyProps@16 (MAPI32.171)
 *
 * Copy an array of property values into a buffer suited for serialisation.
 *
 * PARAMS
 *  cValues   [I] Number of properties in lpProps
 *  lpProps   [I] Property array to copy
 *  lpDst     [O] Destination for the serialised data
 *  lpCount   [O] If non-NULL, destination for the number of bytes of data written to lpDst
 *
 * RETURNS
 *  Success: S_OK. lpDst contains the serialised data from lpProps.
 *  Failure: MAPI_E_INVALID_PARAMETER, if any parameter is invalid.
 *
 * NOTES
 *  The resulting property value array is stored in a contigous block starting at lpDst.
 */
SCODE WINAPI ScCopyProps(int cValues, LPSPropValue lpProps, LPVOID lpDst, ULONG *lpCount)
{
    LPSPropValue lpDest = (LPSPropValue)lpDst;
    char *lpDataDest = (char *)(lpDest + cValues);
    ULONG ulLen, i, iter;

    TRACE("(%d,%p,%p,%p)\n", cValues, lpProps, lpDst, lpCount);

    if (!lpProps || cValues < 0 || !lpDest)
        return MAPI_E_INVALID_PARAMETER;

    memcpy(lpDst, lpProps, cValues * sizeof(SPropValue));

    for (iter = 0; iter < cValues; iter++)
    {
        switch (PROP_TYPE(lpProps->ulPropTag))
        {
        case PT_CLSID:
            lpDest->Value.lpguid = (LPGUID)lpDataDest;
            memcpy(lpDest->Value.lpguid, lpProps->Value.lpguid, sizeof(GUID));
            lpDataDest += sizeof(GUID);
            break;
        case PT_STRING8:
            ulLen = lstrlenA(lpProps->Value.lpszA) + 1u;
            lpDest->Value.lpszA = lpDataDest;
            memcpy(lpDest->Value.lpszA, lpProps->Value.lpszA, ulLen);
            lpDataDest += ulLen;
            break;
        case PT_UNICODE:
            ulLen = (strlenW(lpProps->Value.lpszW) + 1u) * sizeof(WCHAR);
            lpDest->Value.lpszW = (LPWSTR)lpDataDest;
            memcpy(lpDest->Value.lpszW, lpProps->Value.lpszW, ulLen);
            lpDataDest += ulLen;
            break;
        case PT_BINARY:
            lpDest->Value.bin.lpb = (LPBYTE)lpDataDest;
            memcpy(lpDest->Value.bin.lpb, lpProps->Value.bin.lpb, lpProps->Value.bin.cb);
            lpDataDest += lpProps->Value.bin.cb;
            break;
        default:
            if (lpProps->ulPropTag & MV_FLAG)
            {
                lpDest->Value.MVi.cValues = lpProps->Value.MVi.cValues;
                /* Note: Assignment uses lppszA but covers all cases by union aliasing */
                lpDest->Value.MVszA.lppszA = (char**)lpDataDest;

                switch (PROP_TYPE(lpProps->ulPropTag))
                {
                case PT_MV_STRING8:
                {
                    lpDataDest += lpProps->Value.MVszA.cValues * sizeof(char *);

                    for (i = 0; i < lpProps->Value.MVszA.cValues; i++)
                    {
                        ULONG ulStrLen = lstrlenA(lpProps->Value.MVszA.lppszA[i]) + 1u;

                        lpDest->Value.MVszA.lppszA[i] = lpDataDest;
                        memcpy(lpDataDest, lpProps->Value.MVszA.lppszA[i], ulStrLen);
                        lpDataDest += ulStrLen;
                    }
                    break;
                }
                case PT_MV_UNICODE:
                {
                    lpDataDest += lpProps->Value.MVszW.cValues * sizeof(WCHAR *);

                    for (i = 0; i < lpProps->Value.MVszW.cValues; i++)
                    {
                        ULONG ulStrLen = (strlenW(lpProps->Value.MVszW.lppszW[i]) + 1u) * sizeof(WCHAR);

                        lpDest->Value.MVszW.lppszW[i] = (LPWSTR)lpDataDest;
                        memcpy(lpDataDest, lpProps->Value.MVszW.lppszW[i], ulStrLen);
                        lpDataDest += ulStrLen;
                    }
                    break;
                }
                case PT_MV_BINARY:
                {
                    lpDataDest += lpProps->Value.MVszW.cValues * sizeof(SBinary);

                    for (i = 0; i < lpProps->Value.MVszW.cValues; i++)
                    {
                        lpDest->Value.MVbin.lpbin[i].cb = lpProps->Value.MVbin.lpbin[i].cb;
                        lpDest->Value.MVbin.lpbin[i].lpb = (LPBYTE)lpDataDest;
                        memcpy(lpDataDest, lpProps->Value.MVbin.lpbin[i].lpb, lpDest->Value.MVbin.lpbin[i].cb);
                        lpDataDest += lpDest->Value.MVbin.lpbin[i].cb;
                    }
                    break;
                }
                default:
                    /* No embedded pointers, just copy the data over */
                    ulLen = UlPropSize(lpProps);
                    memcpy(lpDest->Value.MVi.lpi, lpProps->Value.MVi.lpi, ulLen);
                    lpDataDest += ulLen;
                    break;
                }
                break;
            }
        }
        lpDest++;
        lpProps++;
    }
    if (lpCount)
        *lpCount = lpDataDest - (char *)lpDst;

    return S_OK;
}

/*************************************************************************
 * ScRelocProps@20 (MAPI32.172)
 *
 * Relocate the pointers in an array of property values after it has been copied.
 *
 * PARAMS
 *  cValues   [I] Number of properties in lpProps
 *  lpProps   [O] Property array to relocate the pointers in.
 *  lpOld     [I] Position where the data was copied from
 *  lpNew     [I] Position where the data was copied to
 *  lpCount   [O] If non-NULL, destination for the number of bytes of data at lpDst
 *
 * RETURNS
 *  Success: S_OK. Any pointers in lpProps are relocated.
 *  Failure: MAPI_E_INVALID_PARAMETER, if any parameter is invalid.
 *
 * NOTES
 *  MSDN states that this function can be used for serialisation by passing
 *  NULL as either lpOld or lpNew, thus converting any pointers in lpProps
 *  between offsets and pointers. This does not work in native (it crashes),
 *  and cannot be made to work in Wine because the original interface design 
 *  is deficient. The only use left for this function is to remap pointers
 *  in a contigous property array that has been copied with memcpy() to another
 *  memory location.
 */
SCODE WINAPI ScRelocProps(int cValues, LPSPropValue lpProps, LPVOID lpOld, 
                          LPVOID lpNew, ULONG *lpCount)
{
    static const BOOL bBadPtr = TRUE; /* Windows bug - Assumes source is bad */
    LPSPropValue lpDest = (LPSPropValue)lpProps;
    ULONG ulCount = cValues * sizeof(SPropValue);    
    ULONG ulLen, i, iter;

    TRACE("(%d,%p,%p,%p,%p)\n", cValues, lpProps, lpOld, lpNew, lpCount);

    if (!lpProps || cValues < 0 || !lpOld || !lpNew)
        return MAPI_E_INVALID_PARAMETER;

    /* The reason native doesn't work as MSDN states is that it assumes that
     * the lpProps pointer contains valid pointers. This is obviously not
     * true if the array is being read back from serialisation (the pointers
     * are just offsets). Native can't actually work converting the pointers to
     * offsets either, because it converts any array pointers to offsets then
     * _dereferences the offset_ in order to convert the array elements!
     *
     * The code below would handle both cases except that the design of this
     * function makes it impossible to know when the pointers in lpProps are
     * valid. If both lpOld and lpNew are non-NULL, native reads the pointers
     * after converting them, so we must do the same. Its seems this
     * functionality was never tested by MS.
     */

#define RELOC_PTR(p) (((char*)(p)) - (char*)lpOld + (char*)lpNew)

    for (iter = 0; iter < cValues; iter++)
    {
        switch (PROP_TYPE(lpDest->ulPropTag))
        {
        case PT_CLSID:
            lpDest->Value.lpguid = (LPGUID)RELOC_PTR(lpDest->Value.lpguid);
            ulCount += sizeof(GUID);
            break;
        case PT_STRING8:
            ulLen = bBadPtr ? 0 : lstrlenA(lpDest->Value.lpszA) + 1u;
            lpDest->Value.lpszA = (LPSTR)RELOC_PTR(lpDest->Value.lpszA);
            if (bBadPtr)
                ulLen = lstrlenA(lpDest->Value.lpszA) + 1u;
            ulCount += ulLen;
            break;
        case PT_UNICODE:
            ulLen = bBadPtr ? 0 : (lstrlenW(lpDest->Value.lpszW) + 1u) * sizeof(WCHAR);
            lpDest->Value.lpszW = (LPWSTR)RELOC_PTR(lpDest->Value.lpszW);
            if (bBadPtr)
                ulLen = (strlenW(lpDest->Value.lpszW) + 1u) * sizeof(WCHAR);
            ulCount += ulLen;
            break;
        case PT_BINARY:
            lpDest->Value.bin.lpb = (LPBYTE)RELOC_PTR(lpDest->Value.bin.lpb);
            ulCount += lpDest->Value.bin.cb;
            break;
        default:
            if (lpDest->ulPropTag & MV_FLAG)
            {
                /* Since we have to access the array elements, don't map the
                 * array unless it is invalid (otherwise, map it at the end)
                 */
                if (bBadPtr)
                    lpDest->Value.MVszA.lppszA = (LPSTR*)RELOC_PTR(lpDest->Value.MVszA.lppszA);

                switch (PROP_TYPE(lpProps->ulPropTag))
                {
                case PT_MV_STRING8:
                {
                    ulCount += lpDest->Value.MVszA.cValues * sizeof(char *);

                    for (i = 0; i < lpDest->Value.MVszA.cValues; i++)
                    {
                        ULONG ulStrLen = bBadPtr ? 0 : lstrlenA(lpDest->Value.MVszA.lppszA[i]) + 1u;

                        lpDest->Value.MVszA.lppszA[i] = (LPSTR)RELOC_PTR(lpDest->Value.MVszA.lppszA[i]);
                        if (bBadPtr)
                            ulStrLen = lstrlenA(lpDest->Value.MVszA.lppszA[i]) + 1u;
                        ulCount += ulStrLen;
                    }
                    break;
                }
                case PT_MV_UNICODE:
                {
                    ulCount += lpDest->Value.MVszW.cValues * sizeof(WCHAR *);

                    for (i = 0; i < lpDest->Value.MVszW.cValues; i++)
                    {
                        ULONG ulStrLen = bBadPtr ? 0 : (strlenW(lpDest->Value.MVszW.lppszW[i]) + 1u) * sizeof(WCHAR);

                        lpDest->Value.MVszW.lppszW[i] = (LPWSTR)RELOC_PTR(lpDest->Value.MVszW.lppszW[i]);
                        if (bBadPtr)
                            ulStrLen = (strlenW(lpDest->Value.MVszW.lppszW[i]) + 1u) * sizeof(WCHAR);
                        ulCount += ulStrLen;
                    }
                    break;
                }
                case PT_MV_BINARY:
                {
                    ulCount += lpDest->Value.MVszW.cValues * sizeof(SBinary);

                    for (i = 0; i < lpDest->Value.MVszW.cValues; i++)
                    {
                        lpDest->Value.MVbin.lpbin[i].lpb = (LPBYTE)RELOC_PTR(lpDest->Value.MVbin.lpbin[i].lpb);
                        ulCount += lpDest->Value.MVbin.lpbin[i].cb;
                    }
                    break;
                }
                default:
                    ulCount += UlPropSize(lpDest);
                    break;
                }
                if (!bBadPtr)
                    lpDest->Value.MVszA.lppszA = (LPSTR*)RELOC_PTR(lpDest->Value.MVszA.lppszA);
                break;
            }
        }
        lpDest++;
    }
    if (lpCount)
        *lpCount = ulCount;

    return S_OK;
}

/*************************************************************************
 * LpValFindProp@12 (MAPI32.173)
 *
 * Find a property with a given property id in a property array.
 *
 * PARAMS
 *  ulPropTag [I] Property tag containing property id to find
 *  cValues   [I] Number of properties in lpProps
 *  lpProps   [I] Property array to search
 *
 * RETURNS
 *  A pointer to the matching property, or NULL if none was found.
 *
 * NOTES
 *  This function matches only on the property id and does not care if the
 *  property types differ.
 */
LPSPropValue WINAPI LpValFindProp(ULONG ulPropTag, ULONG cValues, LPSPropValue lpProps)
{
    TRACE("(%ld,%ld,%p)\n", ulPropTag, cValues, lpProps);

    if (lpProps && cValues)
    {
        ULONG i;
        for (i = 0; i < cValues; i++)
        {
            if (PROP_ID(ulPropTag) == PROP_ID(lpProps[i].ulPropTag))
                return &lpProps[i];
        }
    }
    return NULL;
}

/*************************************************************************
 * ScDupPropset@16 (MAPI32.174)
 *
 * Duplicate a property value array into a contigous block of memory.
 *
 * PARAMS
 *  cValues   [I] Number of properties in lpProps
 *  lpProps   [I] Property array to duplicate
 *  lpAlloc   [I] Memory allocation function, use MAPIAllocateBuffer()
 *  lpNewProp [O] Destination for the newly duplicated property value array
 *
 * RETURNS
 *  Success: S_OK. *lpNewProp contains the duplicated array.
 *  Failure: MAPI_E_INVALID_PARAMETER, if any parameter is invalid,
 *           MAPI_E_NOT_ENOUGH_MEMORY, if memory allocation fails.
 */
SCODE WINAPI ScDupPropset(int cValues, LPSPropValue lpProps,
                          LPALLOCATEBUFFER lpAlloc, LPSPropValue *lpNewProp)
{
    ULONG ulCount;
    SCODE sc;

    TRACE("(%d,%p,%p,%p)\n", cValues, lpProps, lpAlloc, lpNewProp);

    sc = ScCountProps(cValues, lpProps, &ulCount);
    if (SUCCEEDED(sc))
    {
        sc = lpAlloc(ulCount, (LPVOID*)lpNewProp);
        if (SUCCEEDED(sc))
            sc = ScCopyProps(cValues, lpProps, *lpNewProp, &ulCount);
    }
    return sc;
}

/*************************************************************************
 * FBadRglpszA@8 (MAPI32.175)
 *
 * Determine if an array of strings is invalid
 *
 * PARAMS
 *  lppszStrs [I] Array of strings to check
 *  ulCount   [I] Number of strings in lppszStrs
 *
 * RETURNS
 *  TRUE, if lppszStrs is invalid, FALSE otherwise.
 */
BOOL WINAPI FBadRglpszA(LPSTR *lppszStrs, ULONG ulCount)
{
    ULONG i;

    TRACE("(%p,%ld)\n", lppszStrs, ulCount);

    if (!ulCount)
        return FALSE;

    if (!lppszStrs || IsBadReadPtr(lppszStrs, ulCount * sizeof(LPWSTR)))
        return TRUE;

    for (i = 0; i < ulCount; i++)
    {
        if (!lppszStrs[i] || IsBadStringPtrA(lppszStrs[i], -1))
            return TRUE;
    }
    return FALSE;
}

/*************************************************************************
 * FBadRglpszW@8 (MAPI32.176)
 *
 * See FBadRglpszA.
 */
BOOL WINAPI FBadRglpszW(LPWSTR *lppszStrs, ULONG ulCount)
{
    ULONG i;

    TRACE("(%p,%ld)\n", lppszStrs, ulCount);

    if (!ulCount)
        return FALSE;

    if (!lppszStrs || IsBadReadPtr(lppszStrs, ulCount * sizeof(LPWSTR)))
        return TRUE;

    for (i = 0; i < ulCount; i++)
    {
        if (!lppszStrs[i] || IsBadStringPtrW(lppszStrs[i], -1))
            return TRUE;
    }
    return FALSE;
}

/*************************************************************************
 * FBadRowSet@4 (MAPI32.177)
 *
 * Determine if a row is invalid
 *
 * PARAMS
 *  lpRow [I] Row to check
 *
 * RETURNS
 *  TRUE, if lpRow is invalid, FALSE otherwise.
 */
BOOL WINAPI FBadRowSet(LPSRowSet lpRowSet)
{
    ULONG i;
    TRACE("(%p)\n", lpRowSet);

    if (!lpRowSet || IsBadReadPtr(lpRowSet, CbSRowSet(lpRowSet)))
        return TRUE;

    for (i = 0; i < lpRowSet->cRows; i++)
    {
        if (FBadRow(&lpRowSet->aRow[i]))
            return TRUE;
    }
    return FALSE;
}

/*************************************************************************
 * FBadPropTag@4 (MAPI32.179)
 *
 * Determine if a property tag is invalid
 *
 * PARAMS
 *  ulPropTag [I] Property tag to check
 *
 * RETURNS
 *  TRUE, if ulPropTag is invalid, FALSE otherwise.
 */
ULONG WINAPI FBadPropTag(ULONG ulPropTag)
{
    TRACE("(0x%08lx)\n", ulPropTag);

    switch (ulPropTag & (~MV_FLAG & PROP_TYPE_MASK))
    {
    case PT_UNSPECIFIED:
    case PT_NULL:
    case PT_I2:
    case PT_LONG:
    case PT_R4:
    case PT_DOUBLE:
    case PT_CURRENCY:
    case PT_APPTIME:
    case PT_ERROR:
    case PT_BOOLEAN:
    case PT_OBJECT:
    case PT_I8:
    case PT_STRING8:
    case PT_UNICODE:
    case PT_SYSTIME:
    case PT_CLSID:
    case PT_BINARY:
        return FALSE;
    }
    return TRUE;
}

/*************************************************************************
 * FBadRow@4 (MAPI32.180)
 *
 * Determine if a row is invalid
 *
 * PARAMS
 *  lpRow [I] Row to check
 *
 * RETURNS
 *  TRUE, if lpRow is invalid, FALSE otherwise.
 */
ULONG WINAPI FBadRow(LPSRow lpRow)
{
    ULONG i;
    TRACE("(%p)\n", lpRow);

    if (!lpRow || IsBadReadPtr(lpRow, sizeof(SRow)) || !lpRow->lpProps ||
        IsBadReadPtr(lpRow->lpProps, lpRow->cValues * sizeof(SPropValue)))
        return TRUE;

    for (i = 0; i < lpRow->cValues; i++)
    {
        if (FBadProp(&lpRow->lpProps[i]))
            return TRUE;
    }
    return FALSE;
}

/*************************************************************************
 * FBadProp@4 (MAPI32.181)
 *
 * Determine if a property is invalid
 *
 * PARAMS
 *  lpProp [I] Property to check
 *
 * RETURNS
 *  TRUE, if lpProp is invalid, FALSE otherwise.
 */
ULONG WINAPI FBadProp(LPSPropValue lpProp)
{
    ULONG i;

    if (!lpProp || IsBadReadPtr(lpProp, sizeof(SPropValue)) ||
        FBadPropTag(lpProp->ulPropTag))
        return TRUE;

    switch (PROP_TYPE(lpProp->ulPropTag))
    {
    /* Single value properties containing pointers */
    case PT_STRING8:
        if (!lpProp->Value.lpszA || IsBadStringPtrA(lpProp->Value.lpszA, -1))
            return TRUE;
        break;
    case PT_UNICODE:
        if (!lpProp->Value.lpszW || IsBadStringPtrW(lpProp->Value.lpszW, -1))
            return TRUE;
        break;
    case PT_BINARY:
        if (IsBadReadPtr(lpProp->Value.bin.lpb, lpProp->Value.bin.cb))
            return TRUE;
        break;
    case PT_CLSID:
        if (IsBadReadPtr(lpProp->Value.lpguid, sizeof(GUID)))
            return TRUE;
        break;

    /* Multiple value properties (arrays) containing no pointers */
    case PT_MV_I2:
        return PROP_BadArray(lpProp, sizeof(SHORT));
    case PT_MV_LONG:
        return PROP_BadArray(lpProp, sizeof(LONG));
    case PT_MV_LONGLONG:
        return PROP_BadArray(lpProp, sizeof(LONG64));
    case PT_MV_FLOAT:
        return PROP_BadArray(lpProp, sizeof(float));
    case PT_MV_SYSTIME:
        return PROP_BadArray(lpProp, sizeof(FILETIME));
    case PT_MV_APPTIME:
    case PT_MV_DOUBLE:
        return PROP_BadArray(lpProp, sizeof(double));
    case PT_MV_CURRENCY:
        return PROP_BadArray(lpProp, sizeof(CY));
    case PT_MV_CLSID:
        return PROP_BadArray(lpProp, sizeof(GUID));

    /* Multiple value properties containing pointers */
    case PT_MV_STRING8:
        return FBadRglpszA(lpProp->Value.MVszA.lppszA,
                           lpProp->Value.MVszA.cValues);
    case PT_MV_UNICODE:
        return FBadRglpszW(lpProp->Value.MVszW.lppszW,
                           lpProp->Value.MVszW.cValues);
    case PT_MV_BINARY:
        if (PROP_BadArray(lpProp, sizeof(SBinary)))
            return TRUE;

        for (i = 0; i < lpProp->Value.MVszW.cValues; i++)
        {
            if (IsBadReadPtr(lpProp->Value.MVbin.lpbin[i].lpb,
                             lpProp->Value.MVbin.lpbin[i].cb))
                return TRUE;
        }
        break;
    }
    return FALSE;
}

/*************************************************************************
 * FBadColumnSet@4 (MAPI32.182)
 *
 * Determine if an array of property tags is invalid
 *
 * PARAMS
 *  lpCols [I] Property tag array to check
 *
 * RETURNS
 *  TRUE, if lpCols is invalid, FALSE otherwise.
 */
ULONG WINAPI FBadColumnSet(LPSPropTagArray lpCols)
{
    ULONG ulRet = FALSE, i;

    TRACE("(%p)\n", lpCols);

    if (!lpCols || IsBadReadPtr(lpCols, CbSPropTagArray(lpCols)))
        ulRet = TRUE;
    else
    {
        for (i = 0; i < lpCols->cValues; i++)
        {
            if ((lpCols->aulPropTag[i] & PROP_TYPE_MASK) == PT_ERROR ||
                FBadPropTag(lpCols->aulPropTag[i]))
            {
                ulRet = TRUE;
                break;
            }
        }
    }
    TRACE("Returning %s\n", ulRet ? "TRUE" : "FALSE");
    return ulRet;
}
