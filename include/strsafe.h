/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the mingw-w64 runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
#ifndef _STRSAFE_H_INCLUDED_
#define _STRSAFE_H_INCLUDED_

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <specstrings.h>

#include <wchar.h>  /* needed for Unix build */

#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
#if !defined(__LP64__) && !defined(WINE_NO_LONG_TYPES) && !defined(WINE_UNIX_LIB)
typedef long HRESULT;
#else
typedef int HRESULT;
#endif
#endif

#if !defined(__LP64__) && !defined(WINE_NO_LONG_TYPES) && !defined(WINE_UNIX_LIB)
typedef unsigned long DWORD;
#else
typedef unsigned int DWORD;
#endif

#ifndef SUCCEEDED
#define SUCCEEDED(hr) ((HRESULT)(hr) >= 0)
#endif

#ifndef FAILED
#define FAILED(hr) ((HRESULT)(hr) < 0)
#endif

#ifndef S_OK
#define S_OK ((HRESULT)0x00000000)
#endif

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202311L)
#define C_ASSERT(e) static_assert(e, #e)
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define C_ASSERT(e) _Static_assert(e, #e)
#else
#define C_ASSERT(e) extern void __C_ASSERT__(int [(e)?1:-1])
#endif

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif

#define STRSAFEAPI  static inline HRESULT __stdcall
#define STRSAFEAPIV static inline HRESULT __cdecl

#if defined(STRSAFE_LIB_IMPL) || defined(STRSAFE_LIB)
#define STRSAFEWORKERAPI EXTERN_C HRESULT __stdcall
#else
#define STRSAFEWORKERAPI STRSAFEAPI
#endif

#ifdef STRSAFE_LOCALE_FUNCTIONS
#if defined(STRSAFE_LOCALE_LIB_IMPL) || defined(STRSAFE_LIB)
#define STRSAFELOCALEWORKERAPI  EXTERN_C HRESULT __stdcall
#else
#define STRSAFELOCALEWORKERAPI  STRSAFEAPI
#endif
#endif

#define STRSAFE_MAX_CCH 2147483647
#define STRSAFE_MAX_LENGTH  (STRSAFE_MAX_CCH - 1)

#define STRSAFE_IGNORE_NULLS 0x00000100
#define STRSAFE_FILL_BEHIND_NULL 0x00000200
#define STRSAFE_FILL_ON_FAILURE 0x00000400
#define STRSAFE_NULL_ON_FAILURE 0x00000800
#define STRSAFE_NO_TRUNCATION 0x00001000
#define STRSAFE_IGNORE_NULL_UNICODE_STRINGS 0x00010000
#define STRSAFE_UNICODE_STRING_DEST_NULL_TERMINATED 0x00020000

#define STRSAFE_VALID_FLAGS (0x000000FF | STRSAFE_IGNORE_NULLS | STRSAFE_FILL_BEHIND_NULL | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION)
#define STRSAFE_UNICODE_STRING_VALID_FLAGS (STRSAFE_VALID_FLAGS | STRSAFE_IGNORE_NULL_UNICODE_STRINGS | STRSAFE_UNICODE_STRING_DEST_NULL_TERMINATED)

#define STRSAFE_FILL_BYTE(x) ((DWORD)((x & 0x000000FF) | STRSAFE_FILL_BEHIND_NULL))
#define STRSAFE_FAILURE_BYTE(x) ((DWORD)((x & 0x000000FF) | STRSAFE_FILL_ON_FAILURE))

#define STRSAFE_GET_FILL_PATTERN(dwFlags) ((int)(dwFlags & 0x000000FF))

#define STRSAFE_E_INSUFFICIENT_BUFFER ((HRESULT)0x8007007A)
#define STRSAFE_E_INVALID_PARAMETER ((HRESULT)0x80070057)
#define STRSAFE_E_END_OF_FILE ((HRESULT)0x80070026)

typedef char *STRSAFE_LPSTR;
typedef const char *STRSAFE_LPCSTR;
typedef wchar_t *STRSAFE_LPWSTR;
typedef const wchar_t *STRSAFE_LPCWSTR;

STRSAFEWORKERAPI StringCopyWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPCSTR pszSrc);
STRSAFEWORKERAPI StringCopyWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPCWSTR pszSrc);
STRSAFEWORKERAPI StringCopyExWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCSTR pszSrc,STRSAFE_LPSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags);
STRSAFEWORKERAPI StringCopyExWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCWSTR pszSrc,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags);
STRSAFEWORKERAPI StringCopyNWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPCSTR pszSrc,size_t cchToCopy);
STRSAFEWORKERAPI StringCopyNWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPCWSTR pszSrc,size_t cchToCopy);
STRSAFEWORKERAPI StringCopyNExWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCSTR pszSrc,size_t cchToCopy,STRSAFE_LPSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags);
STRSAFEWORKERAPI StringCopyNExWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCWSTR pszSrc,size_t cchToCopy,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags);
STRSAFEWORKERAPI StringCatWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPCSTR pszSrc);
STRSAFEWORKERAPI StringCatWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPCWSTR pszSrc);
STRSAFEWORKERAPI StringCatExWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCSTR pszSrc,STRSAFE_LPSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags);
STRSAFEWORKERAPI StringCatExWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCWSTR pszSrc,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags);
STRSAFEWORKERAPI StringCatNWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPCSTR pszSrc,size_t cchToAppend);
STRSAFEWORKERAPI StringCatNWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPCWSTR pszSrc,size_t cchToAppend);
STRSAFEWORKERAPI StringCatNExWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCSTR pszSrc,size_t cchToAppend,STRSAFE_LPSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags);
STRSAFEWORKERAPI StringCatNExWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCWSTR pszSrc,size_t cchToAppend,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags);
STRSAFEWORKERAPI StringVPrintfWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPCSTR pszFormat,va_list argList);
STRSAFEWORKERAPI StringVPrintfWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPCWSTR pszFormat,va_list argList);
STRSAFEWORKERAPI StringVPrintfExWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags,STRSAFE_LPCSTR pszFormat,va_list argList);
STRSAFEWORKERAPI StringVPrintfExWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags,STRSAFE_LPCWSTR pszFormat,va_list argList);
STRSAFEWORKERAPI StringLengthWorkerA(STRSAFE_LPCSTR psz,size_t cchMax,size_t *pcchLength);
STRSAFEWORKERAPI StringLengthWorkerW(STRSAFE_LPCWSTR psz,size_t cchMax,size_t *pcchLength);

#ifndef STRSAFE_LIB_IMPL

STRSAFEAPI StringGetsExWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags);
STRSAFEAPI StringGetsExWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags);

#ifndef STRSAFE_NO_CCH_FUNCTIONS
STRSAFEAPI StringCchCopyA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPCSTR pszSrc) {
  return (cchDest > STRSAFE_MAX_CCH ? STRSAFE_E_INVALID_PARAMETER : StringCopyWorkerA(pszDest,cchDest,pszSrc));
}

STRSAFEAPI StringCchCopyW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPCWSTR pszSrc) {
  if(cchDest > STRSAFE_MAX_CCH) return STRSAFE_E_INVALID_PARAMETER;
  return StringCopyWorkerW(pszDest,cchDest,pszSrc);
}
#endif /* STRSAFE_NO_CCH_FUNCTIONS */

#ifndef STRSAFE_NO_CB_FUNCTIONS
STRSAFEAPI StringCbCopyA(STRSAFE_LPSTR pszDest,size_t cbDest,STRSAFE_LPCSTR pszSrc) {
  if(cbDest > STRSAFE_MAX_CCH) return STRSAFE_E_INVALID_PARAMETER;
  return StringCopyWorkerA(pszDest,cbDest,pszSrc);
}

STRSAFEAPI StringCbCopyW(STRSAFE_LPWSTR pszDest,size_t cbDest,STRSAFE_LPCWSTR pszSrc) {
  size_t cchDest = cbDest / sizeof(wchar_t);
  if(cchDest > STRSAFE_MAX_CCH) return STRSAFE_E_INVALID_PARAMETER;
  return StringCopyWorkerW(pszDest,cchDest,pszSrc);
}
#endif /* STRSAFE_NO_CB_FUNCTIONS */

#ifndef STRSAFE_NO_CCH_FUNCTIONS
STRSAFEAPI StringCchCopyExA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPCSTR pszSrc,STRSAFE_LPSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags) {
  if(cchDest > STRSAFE_MAX_CCH) return STRSAFE_E_INVALID_PARAMETER;
  return StringCopyExWorkerA(pszDest,cchDest,cchDest,pszSrc,ppszDestEnd,pcchRemaining,dwFlags);
}

STRSAFEAPI StringCchCopyExW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPCWSTR pszSrc,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags) {
  size_t cbDest;
  if(cchDest > STRSAFE_MAX_CCH) return STRSAFE_E_INVALID_PARAMETER;
  cbDest = cchDest * sizeof(wchar_t);
  return StringCopyExWorkerW(pszDest,cchDest,cbDest,pszSrc,ppszDestEnd,pcchRemaining,dwFlags);
}
#endif /* STRSAFE_NO_CCH_FUNCTIONS */

#ifndef STRSAFE_NO_CB_FUNCTIONS
STRSAFEAPI StringCbCopyExA(STRSAFE_LPSTR pszDest,size_t cbDest,STRSAFE_LPCSTR pszSrc,STRSAFE_LPSTR *ppszDestEnd,size_t *pcbRemaining,DWORD dwFlags) {
  HRESULT hr;
  size_t cchRemaining = 0;
  if(cbDest > STRSAFE_MAX_CCH) return STRSAFE_E_INVALID_PARAMETER;
  hr = StringCopyExWorkerA(pszDest,cbDest,cbDest,pszSrc,ppszDestEnd,&cchRemaining,dwFlags);
  if(SUCCEEDED(hr) || hr == STRSAFE_E_INSUFFICIENT_BUFFER) {
    if(pcbRemaining)
      *pcbRemaining = (cchRemaining*sizeof(char)) + (cbDest % sizeof(char));
  }
  return hr;
}

STRSAFEAPI StringCbCopyExW(STRSAFE_LPWSTR pszDest,size_t cbDest,STRSAFE_LPCWSTR pszSrc,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcbRemaining,DWORD dwFlags) {
  HRESULT hr;
  size_t cchDest = cbDest / sizeof(wchar_t);
  size_t cchRemaining = 0;

  if(cchDest > STRSAFE_MAX_CCH) return STRSAFE_E_INVALID_PARAMETER;
  hr = StringCopyExWorkerW(pszDest,cchDest,cbDest,pszSrc,ppszDestEnd,&cchRemaining,dwFlags);
  if(SUCCEEDED(hr) || (hr==STRSAFE_E_INSUFFICIENT_BUFFER)) {
    if(pcbRemaining)
      *pcbRemaining = (cchRemaining*sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
  }
  return hr;
}
#endif /* STRSAFE_NO_CB_FUNCTIONS */

#ifndef STRSAFE_NO_CCH_FUNCTIONS
STRSAFEAPI StringCchCopyNA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPCSTR pszSrc,size_t cchToCopy) {
  if(cchDest > STRSAFE_MAX_CCH || cchToCopy > STRSAFE_MAX_CCH)
    return STRSAFE_E_INVALID_PARAMETER;
  return StringCopyNWorkerA(pszDest,cchDest,pszSrc,cchToCopy);
}

STRSAFEAPI StringCchCopyNW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPCWSTR pszSrc,size_t cchToCopy) {
  if(cchDest > STRSAFE_MAX_CCH || cchToCopy > STRSAFE_MAX_CCH)
    return STRSAFE_E_INVALID_PARAMETER;
  return StringCopyNWorkerW(pszDest,cchDest,pszSrc,cchToCopy);
}
#endif /* STRSAFE_NO_CCH_FUNCTIONS */

#ifndef STRSAFE_NO_CB_FUNCTIONS
STRSAFEAPI StringCbCopyNA(STRSAFE_LPSTR pszDest,size_t cbDest,STRSAFE_LPCSTR pszSrc,size_t cbToCopy) {
  if(cbDest > STRSAFE_MAX_CCH || cbToCopy > STRSAFE_MAX_CCH)
    return STRSAFE_E_INVALID_PARAMETER;
  return StringCopyNWorkerA(pszDest,cbDest,pszSrc,cbToCopy);
}

STRSAFEAPI StringCbCopyNW(STRSAFE_LPWSTR pszDest,size_t cbDest,STRSAFE_LPCWSTR pszSrc,size_t cbToCopy) {
  size_t cchDest  = cbDest / sizeof(wchar_t);
  size_t cchToCopy = cbToCopy / sizeof(wchar_t);
  if(cchDest > STRSAFE_MAX_CCH || cchToCopy > STRSAFE_MAX_CCH)
    return STRSAFE_E_INVALID_PARAMETER;
  return StringCopyNWorkerW(pszDest,cchDest,pszSrc,cchToCopy);
}
#endif /* STRSAFE_NO_CB_FUNCTIONS */

#ifndef STRSAFE_NO_CCH_FUNCTIONS
STRSAFEAPI StringCchCopyNExA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPCSTR pszSrc,size_t cchToCopy,STRSAFE_LPSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags) {
  if(cchDest > STRSAFE_MAX_CCH) return STRSAFE_E_INVALID_PARAMETER;
  return StringCopyNExWorkerA(pszDest,cchDest,cchDest,pszSrc,cchToCopy,ppszDestEnd,pcchRemaining,dwFlags);
}

STRSAFEAPI StringCchCopyNExW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPCWSTR pszSrc,size_t cchToCopy,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags) {
  if(cchDest > STRSAFE_MAX_CCH) return STRSAFE_E_INVALID_PARAMETER;
  return StringCopyNExWorkerW(pszDest,cchDest,cchDest * sizeof(wchar_t),pszSrc,cchToCopy,ppszDestEnd,pcchRemaining,dwFlags);
}
#endif /* STRSAFE_NO_CCH_FUNCTIONS */

#ifndef STRSAFE_NO_CB_FUNCTIONS
STRSAFEAPI StringCbCopyNExA(STRSAFE_LPSTR pszDest,size_t cbDest,STRSAFE_LPCSTR pszSrc,size_t cbToCopy,STRSAFE_LPSTR *ppszDestEnd,size_t *pcbRemaining,DWORD dwFlags) {
  HRESULT hr;
  size_t cchRemaining = 0;
  if(cbDest > STRSAFE_MAX_CCH)
    hr = STRSAFE_E_INVALID_PARAMETER;
  else
    hr = StringCopyNExWorkerA(pszDest,cbDest,cbDest,pszSrc,cbToCopy,ppszDestEnd,&cchRemaining,dwFlags);
  if((SUCCEEDED(hr) || hr == STRSAFE_E_INSUFFICIENT_BUFFER) && pcbRemaining)
    *pcbRemaining = cchRemaining;
  return hr;
}

STRSAFEAPI StringCbCopyNExW(STRSAFE_LPWSTR pszDest,size_t cbDest,STRSAFE_LPCWSTR pszSrc,size_t cbToCopy,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcbRemaining,DWORD dwFlags) {
  HRESULT hr;
  size_t cchDest;
  size_t cchToCopy;
  size_t cchRemaining = 0;
  cchDest = cbDest / sizeof(wchar_t);
  cchToCopy = cbToCopy / sizeof(wchar_t);
  if(cchDest > STRSAFE_MAX_CCH) hr = STRSAFE_E_INVALID_PARAMETER;
  else hr = StringCopyNExWorkerW(pszDest,cchDest,cbDest,pszSrc,cchToCopy,ppszDestEnd,&cchRemaining,dwFlags);
  if((SUCCEEDED(hr) || hr == STRSAFE_E_INSUFFICIENT_BUFFER) && pcbRemaining)
    *pcbRemaining = (cchRemaining*sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
  return hr;
}
#endif /* STRSAFE_NO_CB_FUNCTIONS */

#ifndef STRSAFE_NO_CCH_FUNCTIONS
STRSAFEAPI StringCchCatA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPCSTR pszSrc) {
  if(cchDest > STRSAFE_MAX_CCH) return STRSAFE_E_INVALID_PARAMETER;
  return StringCatWorkerA(pszDest,cchDest,pszSrc);
}

STRSAFEAPI StringCchCatW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPCWSTR pszSrc) {
  if(cchDest > STRSAFE_MAX_CCH) return STRSAFE_E_INVALID_PARAMETER;
  return StringCatWorkerW(pszDest,cchDest,pszSrc);
}
#endif /* STRSAFE_NO_CCH_FUNCTIONS */

#ifndef STRSAFE_NO_CB_FUNCTIONS
STRSAFEAPI StringCbCatA(STRSAFE_LPSTR pszDest,size_t cbDest,STRSAFE_LPCSTR pszSrc) {
  if(cbDest > STRSAFE_MAX_CCH) return STRSAFE_E_INVALID_PARAMETER;
  return StringCatWorkerA(pszDest,cbDest,pszSrc);
}

STRSAFEAPI StringCbCatW(STRSAFE_LPWSTR pszDest,size_t cbDest,STRSAFE_LPCWSTR pszSrc) {
  size_t cchDest = cbDest / sizeof(wchar_t);
  if(cchDest > STRSAFE_MAX_CCH) return STRSAFE_E_INVALID_PARAMETER;
  return StringCatWorkerW(pszDest,cchDest,pszSrc);
}
#endif /* STRSAFE_NO_CB_FUNCTIONS */

#ifndef STRSAFE_NO_CCH_FUNCTIONS
STRSAFEAPI StringCchCatExA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPCSTR pszSrc,STRSAFE_LPSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags) {
  if(cchDest > STRSAFE_MAX_CCH) return STRSAFE_E_INVALID_PARAMETER;
  return StringCatExWorkerA(pszDest,cchDest,cchDest,pszSrc,ppszDestEnd,pcchRemaining,dwFlags);
}

STRSAFEAPI StringCchCatExW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPCWSTR pszSrc,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags) {
  size_t cbDest = cchDest*sizeof(wchar_t);
  if(cchDest > STRSAFE_MAX_CCH) return STRSAFE_E_INVALID_PARAMETER;
  return StringCatExWorkerW(pszDest,cchDest,cbDest,pszSrc,ppszDestEnd,pcchRemaining,dwFlags);
}
#endif /* STRSAFE_NO_CCH_FUNCTIONS */

#ifndef STRSAFE_NO_CB_FUNCTIONS
STRSAFEAPI StringCbCatExA(STRSAFE_LPSTR pszDest,size_t cbDest,STRSAFE_LPCSTR pszSrc,STRSAFE_LPSTR *ppszDestEnd,size_t *pcbRemaining,DWORD dwFlags) {
  HRESULT hr;
  size_t cchRemaining = 0;
  if(cbDest > STRSAFE_MAX_CCH) hr = STRSAFE_E_INVALID_PARAMETER;
  else hr = StringCatExWorkerA(pszDest,cbDest,cbDest,pszSrc,ppszDestEnd,&cchRemaining,dwFlags);
  if((SUCCEEDED(hr) || hr == STRSAFE_E_INSUFFICIENT_BUFFER) && pcbRemaining)
    *pcbRemaining = (cchRemaining*sizeof(char)) + (cbDest % sizeof(char));
  return hr;
}

STRSAFEAPI StringCbCatExW(STRSAFE_LPWSTR pszDest,size_t cbDest,STRSAFE_LPCWSTR pszSrc,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcbRemaining,DWORD dwFlags) {
  HRESULT hr;
  size_t cchDest = cbDest / sizeof(wchar_t);
  size_t cchRemaining = 0;

  if(cchDest > STRSAFE_MAX_CCH) hr = STRSAFE_E_INVALID_PARAMETER;
  else hr = StringCatExWorkerW(pszDest,cchDest,cbDest,pszSrc,ppszDestEnd,&cchRemaining,dwFlags);
  if((SUCCEEDED(hr) || hr == STRSAFE_E_INSUFFICIENT_BUFFER) && pcbRemaining)
    *pcbRemaining = (cchRemaining*sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
  return hr;
}
#endif /* STRSAFE_NO_CB_FUNCTIONS */

#ifndef STRSAFE_NO_CCH_FUNCTIONS
STRSAFEAPI StringCchCatNA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPCSTR pszSrc,size_t cchToAppend) {
  if(cchDest > STRSAFE_MAX_CCH) return STRSAFE_E_INVALID_PARAMETER;
  return StringCatNWorkerA(pszDest,cchDest,pszSrc,cchToAppend);
}

STRSAFEAPI StringCchCatNW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPCWSTR pszSrc,size_t cchToAppend) {
  if(cchDest > STRSAFE_MAX_CCH) return STRSAFE_E_INVALID_PARAMETER;
  return StringCatNWorkerW(pszDest,cchDest,pszSrc,cchToAppend);
}
#endif /* STRSAFE_NO_CCH_FUNCTIONS */

#ifndef STRSAFE_NO_CB_FUNCTIONS
STRSAFEAPI StringCbCatNA(STRSAFE_LPSTR pszDest,size_t cbDest,STRSAFE_LPCSTR pszSrc,size_t cbToAppend) {
  if(cbDest > STRSAFE_MAX_CCH) return STRSAFE_E_INVALID_PARAMETER;
  return StringCatNWorkerA(pszDest,cbDest,pszSrc,cbToAppend);
}

STRSAFEAPI StringCbCatNW(STRSAFE_LPWSTR pszDest,size_t cbDest,STRSAFE_LPCWSTR pszSrc,size_t cbToAppend) {
  size_t cchDest = cbDest / sizeof(wchar_t);
  size_t cchToAppend = cbToAppend / sizeof(wchar_t);

  if(cchDest > STRSAFE_MAX_CCH) return STRSAFE_E_INVALID_PARAMETER;
  return StringCatNWorkerW(pszDest,cchDest,pszSrc,cchToAppend);
}
#endif /* STRSAFE_NO_CB_FUNCTIONS */

#ifndef STRSAFE_NO_CCH_FUNCTIONS
STRSAFEAPI StringCchCatNExA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPCSTR pszSrc,size_t cchToAppend,STRSAFE_LPSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags) {
  if(cchDest > STRSAFE_MAX_CCH) return STRSAFE_E_INVALID_PARAMETER;
  return StringCatNExWorkerA(pszDest,cchDest,cchDest,pszSrc,cchToAppend,ppszDestEnd,pcchRemaining,dwFlags);
}

STRSAFEAPI StringCchCatNExW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPCWSTR pszSrc,size_t cchToAppend,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags) {
  if(cchDest > STRSAFE_MAX_CCH) return STRSAFE_E_INVALID_PARAMETER;
  return StringCatNExWorkerW(pszDest,cchDest,(cchDest*sizeof(wchar_t)),pszSrc,cchToAppend,ppszDestEnd,pcchRemaining,dwFlags);
}
#endif /* STRSAFE_NO_CCH_FUNCTIONS */

#ifndef STRSAFE_NO_CB_FUNCTIONS
STRSAFEAPI StringCbCatNExA(STRSAFE_LPSTR pszDest,size_t cbDest,STRSAFE_LPCSTR pszSrc,size_t cbToAppend,STRSAFE_LPSTR *ppszDestEnd,size_t *pcbRemaining,DWORD dwFlags) {
  HRESULT hr;
  size_t cchRemaining = 0;
  if(cbDest > STRSAFE_MAX_CCH) hr = STRSAFE_E_INVALID_PARAMETER;
  else hr = StringCatNExWorkerA(pszDest,cbDest,cbDest,pszSrc,cbToAppend,ppszDestEnd,&cchRemaining,dwFlags);
  if((SUCCEEDED(hr) || hr == STRSAFE_E_INSUFFICIENT_BUFFER) && pcbRemaining)
    *pcbRemaining = (cchRemaining*sizeof(char)) + (cbDest % sizeof(char));
  return hr;
}

STRSAFEAPI StringCbCatNExW(STRSAFE_LPWSTR pszDest,size_t cbDest,STRSAFE_LPCWSTR pszSrc,size_t cbToAppend,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcbRemaining,DWORD dwFlags) {
  HRESULT hr;
  size_t cchDest = cbDest / sizeof(wchar_t);
  size_t cchToAppend = cbToAppend / sizeof(wchar_t);
  size_t cchRemaining = 0;
  if(cchDest > STRSAFE_MAX_CCH) hr = STRSAFE_E_INVALID_PARAMETER;
  else hr = StringCatNExWorkerW(pszDest,cchDest,cbDest,pszSrc,cchToAppend,ppszDestEnd,&cchRemaining,dwFlags);
  if((SUCCEEDED(hr) || hr == STRSAFE_E_INSUFFICIENT_BUFFER) && pcbRemaining)
    *pcbRemaining = (cchRemaining*sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
  return hr;
}
#endif /* STRSAFE_NO_CB_FUNCTIONS */

#ifndef STRSAFE_NO_CCH_FUNCTIONS
STRSAFEAPI StringCchVPrintfA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPCSTR pszFormat,va_list argList) {
  if(cchDest > STRSAFE_MAX_CCH) return STRSAFE_E_INVALID_PARAMETER;
  return StringVPrintfWorkerA(pszDest,cchDest,pszFormat,argList);
}

STRSAFEAPI StringCchVPrintfW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPCWSTR pszFormat,va_list argList) {
  if(cchDest > STRSAFE_MAX_CCH) return STRSAFE_E_INVALID_PARAMETER;
  return StringVPrintfWorkerW(pszDest,cchDest,pszFormat,argList);
}
#endif /* STRSAFE_NO_CCH_FUNCTIONS */

#ifndef STRSAFE_NO_CB_FUNCTIONS
STRSAFEAPI StringCbVPrintfA(STRSAFE_LPSTR pszDest,size_t cbDest,STRSAFE_LPCSTR pszFormat,va_list argList) {
  if(cbDest > STRSAFE_MAX_CCH) return STRSAFE_E_INVALID_PARAMETER;
  return StringVPrintfWorkerA(pszDest,cbDest,pszFormat,argList);
}

STRSAFEAPI StringCbVPrintfW(STRSAFE_LPWSTR pszDest,size_t cbDest,STRSAFE_LPCWSTR pszFormat,va_list argList) {
  size_t cchDest = cbDest / sizeof(wchar_t);
  if(cchDest > STRSAFE_MAX_CCH) return STRSAFE_E_INVALID_PARAMETER;
  return StringVPrintfWorkerW(pszDest,cchDest,pszFormat,argList);
}
#endif /* STRSAFE_NO_CB_FUNCTIONS */

#ifndef STRSAFE_NO_CCH_FUNCTIONS
STRSAFEAPIV StringCchPrintfA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPCSTR pszFormat,...) {
  HRESULT hr;
  va_list argList;
  if(cchDest > STRSAFE_MAX_CCH) return STRSAFE_E_INVALID_PARAMETER;
  va_start(argList,pszFormat);
  hr = StringVPrintfWorkerA(pszDest,cchDest,pszFormat,argList);
  va_end(argList);
  return hr;
}

STRSAFEAPIV StringCchPrintfW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPCWSTR pszFormat,...) {
  HRESULT hr;
  va_list argList;
  if(cchDest > STRSAFE_MAX_CCH) return STRSAFE_E_INVALID_PARAMETER;
  va_start(argList,pszFormat);
  hr = StringVPrintfWorkerW(pszDest,cchDest,pszFormat,argList);
  va_end(argList);
  return hr;
}
#endif /* STRSAFE_NO_CCH_FUNCTIONS */

#ifndef STRSAFE_NO_CB_FUNCTIONS
STRSAFEAPIV StringCbPrintfA(STRSAFE_LPSTR pszDest,size_t cbDest,STRSAFE_LPCSTR pszFormat,...) {
  HRESULT hr;
  va_list argList;
  if(cbDest > STRSAFE_MAX_CCH) return STRSAFE_E_INVALID_PARAMETER;
  va_start(argList,pszFormat);
  hr = StringVPrintfWorkerA(pszDest,cbDest,pszFormat,argList);
  va_end(argList);
  return hr;
}

STRSAFEAPIV StringCbPrintfW(STRSAFE_LPWSTR pszDest,size_t cbDest,STRSAFE_LPCWSTR pszFormat,...) {
  HRESULT hr;
  va_list argList;
  size_t cchDest = cbDest / sizeof(wchar_t);
  if(cchDest > STRSAFE_MAX_CCH) return STRSAFE_E_INVALID_PARAMETER;
  va_start(argList,pszFormat);
  hr = StringVPrintfWorkerW(pszDest,cchDest,pszFormat,argList);
  va_end(argList);
  return hr;
}
#endif /* STRSAFE_NO_CB_FUNCTIONS */

#ifndef STRSAFE_NO_CCH_FUNCTIONS
STRSAFEAPIV StringCchPrintfExA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags,STRSAFE_LPCSTR pszFormat,...) {
  HRESULT hr;
  va_list argList;
  if(cchDest > STRSAFE_MAX_CCH) return STRSAFE_E_INVALID_PARAMETER;
  va_start(argList,pszFormat);
  hr = StringVPrintfExWorkerA(pszDest,cchDest,cchDest,ppszDestEnd,pcchRemaining,dwFlags,pszFormat,argList);
  va_end(argList);
  return hr;
}

STRSAFEAPIV StringCchPrintfExW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags,STRSAFE_LPCWSTR pszFormat,...) {
  HRESULT hr;
  size_t cbDest = cchDest * sizeof(wchar_t);
  va_list argList;
  if(cchDest > STRSAFE_MAX_CCH) return STRSAFE_E_INVALID_PARAMETER;
  va_start(argList,pszFormat);
  hr = StringVPrintfExWorkerW(pszDest,cchDest,cbDest,ppszDestEnd,pcchRemaining,dwFlags,pszFormat,argList);
  va_end(argList);
  return hr;
}
#endif /* STRSAFE_NO_CCH_FUNCTIONS */

#ifndef STRSAFE_NO_CB_FUNCTIONS
STRSAFEAPIV StringCbPrintfExA(STRSAFE_LPSTR pszDest,size_t cbDest,STRSAFE_LPSTR *ppszDestEnd,size_t *pcbRemaining,DWORD dwFlags,STRSAFE_LPCSTR pszFormat,...) {
  HRESULT hr;
  size_t cchDest;
  size_t cchRemaining = 0;
  cchDest = cbDest / sizeof(char);
  if(cchDest > STRSAFE_MAX_CCH) hr = STRSAFE_E_INVALID_PARAMETER;
  else {
    va_list argList;
    va_start(argList,pszFormat);
    hr = StringVPrintfExWorkerA(pszDest,cchDest,cbDest,ppszDestEnd,&cchRemaining,dwFlags,pszFormat,argList);
    va_end(argList);
  }
  if(SUCCEEDED(hr) || (hr==STRSAFE_E_INSUFFICIENT_BUFFER)) {
    if(pcbRemaining) {
      *pcbRemaining = (cchRemaining*sizeof(char)) + (cbDest % sizeof(char));
    }
  }
  return hr;
}

STRSAFEAPIV StringCbPrintfExW(STRSAFE_LPWSTR pszDest,size_t cbDest,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcbRemaining,DWORD dwFlags,STRSAFE_LPCWSTR pszFormat,...) {
  HRESULT hr;
  size_t cchDest;
  size_t cchRemaining = 0;
  cchDest = cbDest / sizeof(wchar_t);
  if(cchDest > STRSAFE_MAX_CCH) hr = STRSAFE_E_INVALID_PARAMETER;
  else {
    va_list argList;
    va_start(argList,pszFormat);
    hr = StringVPrintfExWorkerW(pszDest,cchDest,cbDest,ppszDestEnd,&cchRemaining,dwFlags,pszFormat,argList);
    va_end(argList);
  }
  if(SUCCEEDED(hr) || (hr==STRSAFE_E_INSUFFICIENT_BUFFER)) {
    if(pcbRemaining) {
      *pcbRemaining = (cchRemaining*sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
    }
  }
  return hr;
}
#endif /* STRSAFE_NO_CB_FUNCTIONS */

#ifndef STRSAFE_NO_CCH_FUNCTIONS
STRSAFEAPI StringCchVPrintfExA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags,STRSAFE_LPCSTR pszFormat,va_list argList) {
  HRESULT hr;
  if(cchDest > STRSAFE_MAX_CCH) hr = STRSAFE_E_INVALID_PARAMETER;
  else {
    size_t cbDest;
    cbDest = cchDest*sizeof(char);
    hr = StringVPrintfExWorkerA(pszDest,cchDest,cbDest,ppszDestEnd,pcchRemaining,dwFlags,pszFormat,argList);
  }
  return hr;
}

STRSAFEAPI StringCchVPrintfExW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags,STRSAFE_LPCWSTR pszFormat,va_list argList) {
  HRESULT hr;
  if(cchDest > STRSAFE_MAX_CCH) hr = STRSAFE_E_INVALID_PARAMETER;
  else {
    size_t cbDest;
    cbDest = cchDest*sizeof(wchar_t);
    hr = StringVPrintfExWorkerW(pszDest,cchDest,cbDest,ppszDestEnd,pcchRemaining,dwFlags,pszFormat,argList);
  }
  return hr;
}
#endif /* STRSAFE_NO_CCH_FUNCTIONS */

#ifndef STRSAFE_NO_CB_FUNCTIONS
STRSAFEAPI StringCbVPrintfExA(STRSAFE_LPSTR pszDest,size_t cbDest,STRSAFE_LPSTR *ppszDestEnd,size_t *pcbRemaining,DWORD dwFlags,STRSAFE_LPCSTR pszFormat,va_list argList) {
  HRESULT hr;
  size_t cchDest;
  size_t cchRemaining = 0;
  cchDest = cbDest / sizeof(char);
  if(cchDest > STRSAFE_MAX_CCH) hr = STRSAFE_E_INVALID_PARAMETER;
  else hr = StringVPrintfExWorkerA(pszDest,cchDest,cbDest,ppszDestEnd,&cchRemaining,dwFlags,pszFormat,argList);
  if(SUCCEEDED(hr) || (hr==STRSAFE_E_INSUFFICIENT_BUFFER)) {
    if(pcbRemaining) {
      *pcbRemaining = (cchRemaining*sizeof(char)) + (cbDest % sizeof(char));
    }
  }
  return hr;
}

STRSAFEAPI StringCbVPrintfExW(STRSAFE_LPWSTR pszDest,size_t cbDest,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcbRemaining,DWORD dwFlags,STRSAFE_LPCWSTR pszFormat,va_list argList) {
  HRESULT hr;
  size_t cchDest;
  size_t cchRemaining = 0;
  cchDest = cbDest / sizeof(wchar_t);
  if(cchDest > STRSAFE_MAX_CCH) hr = STRSAFE_E_INVALID_PARAMETER;
  else hr = StringVPrintfExWorkerW(pszDest,cchDest,cbDest,ppszDestEnd,&cchRemaining,dwFlags,pszFormat,argList);
  if(SUCCEEDED(hr) || (hr==STRSAFE_E_INSUFFICIENT_BUFFER)) {
    if(pcbRemaining) {
      *pcbRemaining = (cchRemaining*sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
    }
  }
  return hr;
}
#endif /* STRSAFE_NO_CB_FUNCTIONS */

#ifndef STRSAFE_NO_CCH_FUNCTIONS
STRSAFEAPI StringCchGetsA(STRSAFE_LPSTR pszDest,size_t cchDest) {
  HRESULT hr;
  if(cchDest > STRSAFE_MAX_CCH) hr = STRSAFE_E_INVALID_PARAMETER;
  else {
    size_t cbDest;
    cbDest = cchDest*sizeof(char);
    hr = StringGetsExWorkerA(pszDest,cchDest,cbDest,NULL,NULL,0);
  }
  return hr;
}

STRSAFEAPI StringCchGetsW(STRSAFE_LPWSTR pszDest,size_t cchDest) {
  HRESULT hr;
  if(cchDest > STRSAFE_MAX_CCH) hr = STRSAFE_E_INVALID_PARAMETER;
  else {
    size_t cbDest;
    cbDest = cchDest*sizeof(wchar_t);
    hr = StringGetsExWorkerW(pszDest,cchDest,cbDest,NULL,NULL,0);
  }
  return hr;
}
#endif /* STRSAFE_NO_CCH_FUNCTIONS */

#ifndef STRSAFE_NO_CB_FUNCTIONS
STRSAFEAPI StringCbGetsA(STRSAFE_LPSTR pszDest,size_t cbDest) {
  HRESULT hr;
  size_t cchDest;
  cchDest = cbDest / sizeof(char);
  if(cchDest > STRSAFE_MAX_CCH) hr = STRSAFE_E_INVALID_PARAMETER;
  else hr = StringGetsExWorkerA(pszDest,cchDest,cbDest,NULL,NULL,0);
  return hr;
}

STRSAFEAPI StringCbGetsW(STRSAFE_LPWSTR pszDest,size_t cbDest) {
  HRESULT hr;
  size_t cchDest;
  cchDest = cbDest / sizeof(wchar_t);
  if(cchDest > STRSAFE_MAX_CCH) hr = STRSAFE_E_INVALID_PARAMETER;
  else hr = StringGetsExWorkerW(pszDest,cchDest,cbDest,NULL,NULL,0);
  return hr;
}
#endif /* STRSAFE_NO_CB_FUNCTIONS */

#ifndef STRSAFE_NO_CCH_FUNCTIONS
STRSAFEAPI StringCchGetsExA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags) {
  HRESULT hr;
  if(cchDest > STRSAFE_MAX_CCH) hr = STRSAFE_E_INVALID_PARAMETER;
  else {
    size_t cbDest;
    cbDest = cchDest*sizeof(char);
    hr = StringGetsExWorkerA(pszDest,cchDest,cbDest,ppszDestEnd,pcchRemaining,dwFlags);
  }
  return hr;
}

STRSAFEAPI StringCchGetsExW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags) {
  HRESULT hr;
  if(cchDest > STRSAFE_MAX_CCH) hr = STRSAFE_E_INVALID_PARAMETER;
  else {
    size_t cbDest;
    cbDest = cchDest*sizeof(wchar_t);
    hr = StringGetsExWorkerW(pszDest,cchDest,cbDest,ppszDestEnd,pcchRemaining,dwFlags);
  }
  return hr;
}
#endif /* STRSAFE_NO_CCH_FUNCTIONS */

#ifndef STRSAFE_NO_CB_FUNCTIONS
STRSAFEAPI StringCbGetsExA(STRSAFE_LPSTR pszDest,size_t cbDest,STRSAFE_LPSTR *ppszDestEnd,size_t *pcbRemaining,DWORD dwFlags) {
  HRESULT hr;
  size_t cchDest;
  size_t cchRemaining = 0;
  cchDest = cbDest / sizeof(char);
  if(cchDest > STRSAFE_MAX_CCH) hr = STRSAFE_E_INVALID_PARAMETER;
  else hr = StringGetsExWorkerA(pszDest,cchDest,cbDest,ppszDestEnd,&cchRemaining,dwFlags);
  if(SUCCEEDED(hr) || (hr==STRSAFE_E_INSUFFICIENT_BUFFER) || (hr==STRSAFE_E_END_OF_FILE)) {
    if(pcbRemaining) *pcbRemaining = (cchRemaining*sizeof(char)) + (cbDest % sizeof(char));
  }
  return hr;
}

STRSAFEAPI StringCbGetsExW(STRSAFE_LPWSTR pszDest,size_t cbDest,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcbRemaining,DWORD dwFlags) {
  HRESULT hr;
  size_t cchDest;
  size_t cchRemaining = 0;
  cchDest = cbDest / sizeof(wchar_t);
  if(cchDest > STRSAFE_MAX_CCH) hr = STRSAFE_E_INVALID_PARAMETER;
  else hr = StringGetsExWorkerW(pszDest,cchDest,cbDest,ppszDestEnd,&cchRemaining,dwFlags);
  if(SUCCEEDED(hr) || (hr==STRSAFE_E_INSUFFICIENT_BUFFER) || (hr==STRSAFE_E_END_OF_FILE)) {
    if(pcbRemaining) *pcbRemaining = (cchRemaining*sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
  }
  return hr;
}
#endif /* STRSAFE_NO_CB_FUNCTIONS */

#ifndef STRSAFE_NO_CCH_FUNCTIONS
STRSAFEAPI StringCchLengthA(STRSAFE_LPCSTR psz,size_t cchMax,size_t *pcchLength) {
  HRESULT hr;
  if(!psz || (cchMax > STRSAFE_MAX_CCH)) hr = STRSAFE_E_INVALID_PARAMETER;
  else hr = StringLengthWorkerA(psz,cchMax,pcchLength);
  if(FAILED(hr) && pcchLength) {
    *pcchLength = 0;
  }
  return hr;
}

STRSAFEAPI StringCchLengthW(STRSAFE_LPCWSTR psz,size_t cchMax,size_t *pcchLength) {
  HRESULT hr;
  if(!psz || (cchMax > STRSAFE_MAX_CCH)) hr = STRSAFE_E_INVALID_PARAMETER;
  else hr = StringLengthWorkerW(psz,cchMax,pcchLength);
  if(FAILED(hr) && pcchLength) {
    *pcchLength = 0;
  }
  return hr;
}
#endif /* STRSAFE_NO_CCH_FUNCTIONS */

#ifndef STRSAFE_NO_CB_FUNCTIONS
STRSAFEAPI StringCbLengthA(STRSAFE_LPCSTR psz,size_t cbMax,size_t *pcbLength) {
  HRESULT hr;
  size_t cchMax;
  size_t cchLength = 0;
  cchMax = cbMax / sizeof(char);
  if(!psz || (cchMax > STRSAFE_MAX_CCH)) hr = STRSAFE_E_INVALID_PARAMETER;
  else hr = StringLengthWorkerA(psz,cchMax,&cchLength);
  if(pcbLength) {
    if(SUCCEEDED(hr)) {
      *pcbLength = cchLength*sizeof(char);
    } else {
      *pcbLength = 0;
    }
  }
  return hr;
}

STRSAFEAPI StringCbLengthW(STRSAFE_LPCWSTR psz,size_t cbMax,size_t *pcbLength) {
  HRESULT hr;
  size_t cchMax;
  size_t cchLength = 0;
  cchMax = cbMax / sizeof(wchar_t);
  if(!psz || (cchMax > STRSAFE_MAX_CCH)) hr = STRSAFE_E_INVALID_PARAMETER;
  else hr = StringLengthWorkerW(psz,cchMax,&cchLength);
  if(pcbLength) {
    if(SUCCEEDED(hr)) {
      *pcbLength = cchLength*sizeof(wchar_t);
    } else {
      *pcbLength = 0;
    }
  }
  return hr;
}
#endif /* STRSAFE_NO_CB_FUNCTIONS */

#ifndef STRSAFE_NO_CCH_FUNCTIONS
#define StringCchCopy      WINELIB_NAME_AW(StringCchCopy)
#define StringCchCopyEx    WINELIB_NAME_AW(StringCchCopyEx)
#define StringCchCopyN     WINELIB_NAME_AW(StringCchCopyN)
#define StringCchCopyNEx   WINELIB_NAME_AW(StringCchCopyNEx)
#define StringCchCat       WINELIB_NAME_AW(StringCchCat)
#define StringCchCatEx     WINELIB_NAME_AW(StringCchCatEx)
#define StringCchCatN      WINELIB_NAME_AW(StringCchCatN)
#define StringCchCatNEx    WINELIB_NAME_AW(StringCchCatNEx)
#define StringCchVPrintf   WINELIB_NAME_AW(StringCchVPrintf)
#define StringCchVPrintfEx WINELIB_NAME_AW(StringCchVPrintfEx)
#define StringCchPrintf    WINELIB_NAME_AW(StringCchPrintf)
#define StringCchPrintfEx  WINELIB_NAME_AW(StringCchPrintfEx)
#define StringCchGets      WINELIB_NAME_AW(StringCchGets)
#define StringCchGetsEx    WINELIB_NAME_AW(StringCchGetsEx)
#define StringCchLength    WINELIB_NAME_AW(StringCchLength)
#endif /* STRSAFE_NO_CCH_FUNCTIONS */

#ifndef STRSAFE_NO_CB_FUNCTIONS
#define StringCbCopy      WINELIB_NAME_AW(StringCbCopy)
#define StringCbCopyEx    WINELIB_NAME_AW(StringCbCopyEx)
#define StringCbCopyN     WINELIB_NAME_AW(StringCbCopyN)
#define StringCbCopyNEx   WINELIB_NAME_AW(StringCbCopyNEx)
#define StringCbCat       WINELIB_NAME_AW(StringCbCat)
#define StringCbCatEx     WINELIB_NAME_AW(StringCbCatEx)
#define StringCbCatN      WINELIB_NAME_AW(StringCbCatN)
#define StringCbCatNEx    WINELIB_NAME_AW(StringCbCatNEx)
#define StringCbVPrintf   WINELIB_NAME_AW(StringCbVPrintf)
#define StringCbVPrintfEx WINELIB_NAME_AW(StringCbVPrintfEx)
#define StringCbPrintf    WINELIB_NAME_AW(StringCbPrintf)
#define StringCbPrintfEx  WINELIB_NAME_AW(StringCbPrintfEx)
#define StringCbGets      WINELIB_NAME_AW(StringCbGets)
#define StringCbGetsEx    WINELIB_NAME_AW(StringCbGetsEx)
#define StringCbLength    WINELIB_NAME_AW(StringCbLength)
#endif /* STRSAFE_NO_CB_FUNCTIONS */

#endif /* STRSAFE_LIB_IMPL */

#if defined(STRSAFE_LIB_IMPL) || !defined(STRSAFE_LIB)

STRSAFEWORKERAPI StringCopyWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPCSTR pszSrc) {
  HRESULT hr = S_OK;
  if(cchDest==0) hr = STRSAFE_E_INVALID_PARAMETER;
  else {
    while(cchDest && (*pszSrc!='\0')) {
      *pszDest++ = *pszSrc++;
      cchDest--;
    }
    if(cchDest==0) {
      pszDest--;
      hr = STRSAFE_E_INSUFFICIENT_BUFFER;
    }
    *pszDest= '\0';
  }
  return hr;
}

STRSAFEWORKERAPI StringCopyWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPCWSTR pszSrc) {
  HRESULT hr = S_OK;
  if(cchDest==0) hr = STRSAFE_E_INVALID_PARAMETER;
  else {
    while(cchDest && (*pszSrc!=L'\0')) {
      *pszDest++ = *pszSrc++;
      cchDest--;
    }
    if(cchDest==0) {
      pszDest--;
      hr = STRSAFE_E_INSUFFICIENT_BUFFER;
    }
    *pszDest= L'\0';
  }
  return hr;
}

STRSAFEWORKERAPI StringCopyExWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCSTR pszSrc,STRSAFE_LPSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags) {
  HRESULT hr = S_OK;
  STRSAFE_LPSTR pszDestEnd = pszDest;
  size_t cchRemaining = 0;
  if(dwFlags & (~STRSAFE_VALID_FLAGS)) hr = STRSAFE_E_INVALID_PARAMETER;
  else {
    if(dwFlags & STRSAFE_IGNORE_NULLS) {
      if(!pszDest) {
	if((cchDest!=0) || (cbDest!=0)) hr = STRSAFE_E_INVALID_PARAMETER;
      }
      if(!pszSrc) pszSrc = "";
    }
    if(SUCCEEDED(hr)) {
      if(cchDest==0) {
	pszDestEnd = pszDest;
	cchRemaining = 0;
	if(*pszSrc!='\0') {
	  if(!pszDest) hr = STRSAFE_E_INVALID_PARAMETER;
	  else hr = STRSAFE_E_INSUFFICIENT_BUFFER;
	}
      } else {
	pszDestEnd = pszDest;
	cchRemaining = cchDest;
	while(cchRemaining && (*pszSrc!='\0')) {
	  *pszDestEnd++ = *pszSrc++;
	  cchRemaining--;
	}
	if(cchRemaining > 0) {
	  if(dwFlags & STRSAFE_FILL_BEHIND_NULL) {
	    memset(pszDestEnd + 1,STRSAFE_GET_FILL_PATTERN(dwFlags),((cchRemaining - 1)*sizeof(char)) + (cbDest % sizeof(char)));
	  }
	} else {
	  pszDestEnd--;
	  cchRemaining++;
	  hr = STRSAFE_E_INSUFFICIENT_BUFFER;
	}
	*pszDestEnd = '\0';
      }
    }
  }
  if(FAILED(hr)) {
    if(pszDest) {
      if(dwFlags & STRSAFE_FILL_ON_FAILURE) {
	memset(pszDest,STRSAFE_GET_FILL_PATTERN(dwFlags),cbDest);
	if(STRSAFE_GET_FILL_PATTERN(dwFlags)==0) {
	  pszDestEnd = pszDest;
	  cchRemaining = cchDest;
	} else if(cchDest > 0) {
	  pszDestEnd = pszDest + cchDest - 1;
	  cchRemaining = 1;
	  *pszDestEnd = '\0';
	}
      }
      if(dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION)) {
	if(cchDest > 0) {
	  pszDestEnd = pszDest;
	  cchRemaining = cchDest;
	  *pszDestEnd = '\0';
	}
      }
    }
  }
  if(SUCCEEDED(hr) || (hr==STRSAFE_E_INSUFFICIENT_BUFFER)) {
    if(ppszDestEnd) *ppszDestEnd = pszDestEnd;
    if(pcchRemaining) *pcchRemaining = cchRemaining;
  }
  return hr;
}

STRSAFEWORKERAPI StringCopyExWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCWSTR pszSrc,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags) {
  HRESULT hr = S_OK;
  STRSAFE_LPWSTR pszDestEnd = pszDest;
  size_t cchRemaining = 0;
  if(dwFlags & (~STRSAFE_VALID_FLAGS)) hr = STRSAFE_E_INVALID_PARAMETER;
  else {
    if(dwFlags & STRSAFE_IGNORE_NULLS) {
      if(!pszDest) {
	if((cchDest!=0) || (cbDest!=0)) hr = STRSAFE_E_INVALID_PARAMETER;
      }
      if(!pszSrc) pszSrc = L"";
    }
    if(SUCCEEDED(hr)) {
      if(cchDest==0) {
	pszDestEnd = pszDest;
	cchRemaining = 0;
	if(*pszSrc!=L'\0') {
	  if(!pszDest) hr = STRSAFE_E_INVALID_PARAMETER;
	  else hr = STRSAFE_E_INSUFFICIENT_BUFFER;
	}
      } else {
	pszDestEnd = pszDest;
	cchRemaining = cchDest;
	while(cchRemaining && (*pszSrc!=L'\0')) {
	  *pszDestEnd++ = *pszSrc++;
	  cchRemaining--;
	}
	if(cchRemaining > 0) {
	  if(dwFlags & STRSAFE_FILL_BEHIND_NULL) {
	    memset(pszDestEnd + 1,STRSAFE_GET_FILL_PATTERN(dwFlags),((cchRemaining - 1)*sizeof(wchar_t)) + (cbDest % sizeof(wchar_t)));
	  }
	} else {
	  pszDestEnd--;
	  cchRemaining++;
	  hr = STRSAFE_E_INSUFFICIENT_BUFFER;
	}
	*pszDestEnd = L'\0';
      }
    }
  }
  if(FAILED(hr)) {
    if(pszDest) {
      if(dwFlags & STRSAFE_FILL_ON_FAILURE) {
	memset(pszDest,STRSAFE_GET_FILL_PATTERN(dwFlags),cbDest);
	if(STRSAFE_GET_FILL_PATTERN(dwFlags)==0) {
	  pszDestEnd = pszDest;
	  cchRemaining = cchDest;
	} else if(cchDest > 0) {
	  pszDestEnd = pszDest + cchDest - 1;
	  cchRemaining = 1;
	  *pszDestEnd = L'\0';
	}
      }
      if(dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION)) {
	if(cchDest > 0) {
	  pszDestEnd = pszDest;
	  cchRemaining = cchDest;
	  *pszDestEnd = L'\0';
	}
      }
    }
  }
  if(SUCCEEDED(hr) || (hr==STRSAFE_E_INSUFFICIENT_BUFFER)) {
    if(ppszDestEnd) *ppszDestEnd = pszDestEnd;
    if(pcchRemaining) *pcchRemaining = cchRemaining;
  }
  return hr;
}

STRSAFEWORKERAPI StringCopyNWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPCSTR pszSrc,size_t cchSrc) {
  HRESULT hr = S_OK;
  if(cchDest==0) hr = STRSAFE_E_INVALID_PARAMETER;
  else {
    while(cchDest && cchSrc && (*pszSrc!='\0')) {
      *pszDest++ = *pszSrc++;
      cchDest--;
      cchSrc--;
    }
    if(cchDest==0) {
      pszDest--;
      hr = STRSAFE_E_INSUFFICIENT_BUFFER;
    }
    *pszDest= '\0';
  }
  return hr;
}

STRSAFEWORKERAPI StringCopyNWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPCWSTR pszSrc,size_t cchToCopy) {
  HRESULT hr = S_OK;
  if(cchDest==0) hr = STRSAFE_E_INVALID_PARAMETER;
  else {
    while(cchDest && cchToCopy && (*pszSrc!=L'\0')) {
      *pszDest++ = *pszSrc++;
      cchDest--;
      cchToCopy--;
    }
    if(cchDest==0) {
      pszDest--;
      hr = STRSAFE_E_INSUFFICIENT_BUFFER;
    }
    *pszDest= L'\0';
  }
  return hr;
}

STRSAFEWORKERAPI StringCopyNExWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCSTR pszSrc,size_t cchToCopy,STRSAFE_LPSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags) {
  HRESULT hr = S_OK;
  STRSAFE_LPSTR pszDestEnd = pszDest;
  size_t cchRemaining = 0;
  if(dwFlags & (~STRSAFE_VALID_FLAGS)) hr = STRSAFE_E_INVALID_PARAMETER;
  else if(cchToCopy > STRSAFE_MAX_CCH) hr = STRSAFE_E_INVALID_PARAMETER;
  else {
    if(dwFlags & STRSAFE_IGNORE_NULLS) {
      if(!pszDest) {
	if((cchDest!=0) || (cbDest!=0)) hr = STRSAFE_E_INVALID_PARAMETER;
      }
      if(!pszSrc) pszSrc = "";
    }
    if(SUCCEEDED(hr)) {
      if(cchDest==0) {
	pszDestEnd = pszDest;
	cchRemaining = 0;
	if((cchToCopy!=0) && (*pszSrc!='\0')) {
	  if(!pszDest) hr = STRSAFE_E_INVALID_PARAMETER;
	  else hr = STRSAFE_E_INSUFFICIENT_BUFFER;
	}
      } else {
	pszDestEnd = pszDest;
	cchRemaining = cchDest;
	while(cchRemaining && cchToCopy && (*pszSrc!='\0')) {
	  *pszDestEnd++ = *pszSrc++;
	  cchRemaining--;
	  cchToCopy--;
	}
	if(cchRemaining > 0) {
	  if(dwFlags & STRSAFE_FILL_BEHIND_NULL) {
	    memset(pszDestEnd + 1,STRSAFE_GET_FILL_PATTERN(dwFlags),((cchRemaining - 1)*sizeof(char)) + (cbDest % sizeof(char)));
	  }
	} else {
	  pszDestEnd--;
	  cchRemaining++;
	  hr = STRSAFE_E_INSUFFICIENT_BUFFER;
	}
	*pszDestEnd = '\0';
      }
    }
  }
  if(FAILED(hr)) {
    if(pszDest) {
      if(dwFlags & STRSAFE_FILL_ON_FAILURE) {
	memset(pszDest,STRSAFE_GET_FILL_PATTERN(dwFlags),cbDest);
	if(STRSAFE_GET_FILL_PATTERN(dwFlags)==0) {
	  pszDestEnd = pszDest;
	  cchRemaining = cchDest;
	} else if(cchDest > 0) {
	  pszDestEnd = pszDest + cchDest - 1;
	  cchRemaining = 1;
	  *pszDestEnd = '\0';
	}
      }
      if(dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION)) {
	if(cchDest > 0) {
	  pszDestEnd = pszDest;
	  cchRemaining = cchDest;
	  *pszDestEnd = '\0';
	}
      }
    }
  }
  if(SUCCEEDED(hr) || (hr==STRSAFE_E_INSUFFICIENT_BUFFER)) {
    if(ppszDestEnd) *ppszDestEnd = pszDestEnd;
    if(pcchRemaining) *pcchRemaining = cchRemaining;
  }
  return hr;
}

STRSAFEWORKERAPI StringCopyNExWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCWSTR pszSrc,size_t cchToCopy,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags) {
  HRESULT hr = S_OK;
  STRSAFE_LPWSTR pszDestEnd = pszDest;
  size_t cchRemaining = 0;
  if(dwFlags & (~STRSAFE_VALID_FLAGS)) hr = STRSAFE_E_INVALID_PARAMETER;
  else if(cchToCopy > STRSAFE_MAX_CCH) hr = STRSAFE_E_INVALID_PARAMETER;
  else {
    if(dwFlags & STRSAFE_IGNORE_NULLS) {
      if(!pszDest) {
	if((cchDest!=0) || (cbDest!=0)) hr = STRSAFE_E_INVALID_PARAMETER;
      }
      if(!pszSrc) pszSrc = L"";
    }
    if(SUCCEEDED(hr)) {
      if(cchDest==0) {
	pszDestEnd = pszDest;
	cchRemaining = 0;
	if((cchToCopy!=0) && (*pszSrc!=L'\0')) {
	  if(!pszDest) hr = STRSAFE_E_INVALID_PARAMETER;
	  else hr = STRSAFE_E_INSUFFICIENT_BUFFER;
	}
      } else {
	pszDestEnd = pszDest;
	cchRemaining = cchDest;
	while(cchRemaining && cchToCopy && (*pszSrc!=L'\0')) {
	  *pszDestEnd++ = *pszSrc++;
	  cchRemaining--;
	  cchToCopy--;
	}
	if(cchRemaining > 0) {
	  if(dwFlags & STRSAFE_FILL_BEHIND_NULL) {
	    memset(pszDestEnd + 1,STRSAFE_GET_FILL_PATTERN(dwFlags),((cchRemaining - 1)*sizeof(wchar_t)) + (cbDest % sizeof(wchar_t)));
	  }
	} else {
	  pszDestEnd--;
	  cchRemaining++;
	  hr = STRSAFE_E_INSUFFICIENT_BUFFER;
	}
	*pszDestEnd = L'\0';
      }
    }
  }
  if(FAILED(hr)) {
    if(pszDest) {
      if(dwFlags & STRSAFE_FILL_ON_FAILURE) {
	memset(pszDest,STRSAFE_GET_FILL_PATTERN(dwFlags),cbDest);
	if(STRSAFE_GET_FILL_PATTERN(dwFlags)==0) {
	  pszDestEnd = pszDest;
	  cchRemaining = cchDest;
	} else if(cchDest > 0) {
	  pszDestEnd = pszDest + cchDest - 1;
	  cchRemaining = 1;
	  *pszDestEnd = L'\0';
	}
      }
      if(dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION)) {
	if(cchDest > 0) {
	  pszDestEnd = pszDest;
	  cchRemaining = cchDest;
	  *pszDestEnd = L'\0';
	}
      }
    }
  }
  if(SUCCEEDED(hr) || (hr==STRSAFE_E_INSUFFICIENT_BUFFER)) {
    if(ppszDestEnd) *ppszDestEnd = pszDestEnd;
    if(pcchRemaining) *pcchRemaining = cchRemaining;
  }
  return hr;
}

STRSAFEWORKERAPI StringCatWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPCSTR pszSrc) {
  HRESULT hr;
  size_t cchDestLength;
  hr = StringLengthWorkerA(pszDest,cchDest,&cchDestLength);
  if(SUCCEEDED(hr)) hr = StringCopyWorkerA(pszDest + cchDestLength,cchDest - cchDestLength,pszSrc);
  return hr;
}

STRSAFEWORKERAPI StringCatWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPCWSTR pszSrc) {
  HRESULT hr;
  size_t cchDestLength;
  hr = StringLengthWorkerW(pszDest,cchDest,&cchDestLength);
  if(SUCCEEDED(hr)) hr = StringCopyWorkerW(pszDest + cchDestLength,cchDest - cchDestLength,pszSrc);
  return hr;
}

STRSAFEWORKERAPI StringCatExWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCSTR pszSrc,STRSAFE_LPSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags) {
  HRESULT hr = S_OK;
  STRSAFE_LPSTR pszDestEnd = pszDest;
  size_t cchRemaining = 0;
  if(dwFlags & (~STRSAFE_VALID_FLAGS)) hr = STRSAFE_E_INVALID_PARAMETER;
  else {
    size_t cchDestLength;
    if(dwFlags & STRSAFE_IGNORE_NULLS) {
      if(!pszDest) {
	if((cchDest==0) && (cbDest==0)) cchDestLength = 0;
	else hr = STRSAFE_E_INVALID_PARAMETER;
      } else {
	hr = StringLengthWorkerA(pszDest,cchDest,&cchDestLength);
	if(SUCCEEDED(hr)) {
	  pszDestEnd = pszDest + cchDestLength;
	  cchRemaining = cchDest - cchDestLength;
	}
      }
      if(!pszSrc) pszSrc = "";
    } else {
      hr = StringLengthWorkerA(pszDest,cchDest,&cchDestLength);
      if(SUCCEEDED(hr)) {
	pszDestEnd = pszDest + cchDestLength;
	cchRemaining = cchDest - cchDestLength;
      }
    }
    if(SUCCEEDED(hr)) {
      if(cchDest==0) {
	if(*pszSrc!='\0') {
	  if(!pszDest) hr = STRSAFE_E_INVALID_PARAMETER;
	  else hr = STRSAFE_E_INSUFFICIENT_BUFFER;
	}
      } else hr = StringCopyExWorkerA(pszDestEnd,cchRemaining,(cchRemaining*sizeof(char)) + (cbDest % sizeof(char)),pszSrc,&pszDestEnd,&cchRemaining,dwFlags & (~(STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)));
    }
  }
  if(FAILED(hr)) {
    if(pszDest) {
      if(dwFlags & STRSAFE_FILL_ON_FAILURE) {
	memset(pszDest,STRSAFE_GET_FILL_PATTERN(dwFlags),cbDest);
	if(STRSAFE_GET_FILL_PATTERN(dwFlags)==0) {
	  pszDestEnd = pszDest;
	  cchRemaining = cchDest;
	} else if(cchDest > 0) {
	  pszDestEnd = pszDest + cchDest - 1;
	  cchRemaining = 1;
	  *pszDestEnd = '\0';
	}
      }
      if(dwFlags & STRSAFE_NULL_ON_FAILURE) {
	if(cchDest > 0) {
	  pszDestEnd = pszDest;
	  cchRemaining = cchDest;
	  *pszDestEnd = '\0';
	}
      }
    }
  }
  if(SUCCEEDED(hr) || (hr==STRSAFE_E_INSUFFICIENT_BUFFER)) {
    if(ppszDestEnd) *ppszDestEnd = pszDestEnd;
    if(pcchRemaining) *pcchRemaining = cchRemaining;
  }
  return hr;
}

STRSAFEWORKERAPI StringCatExWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCWSTR pszSrc,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags) {
  HRESULT hr = S_OK;
  STRSAFE_LPWSTR pszDestEnd = pszDest;
  size_t cchRemaining = 0;
  if(dwFlags & (~STRSAFE_VALID_FLAGS)) hr = STRSAFE_E_INVALID_PARAMETER;
  else {
    size_t cchDestLength;
    if(dwFlags & STRSAFE_IGNORE_NULLS) {
      if(!pszDest) {
	if((cchDest==0) && (cbDest==0)) cchDestLength = 0;
	else hr = STRSAFE_E_INVALID_PARAMETER;
      } else {
	hr = StringLengthWorkerW(pszDest,cchDest,&cchDestLength);
	if(SUCCEEDED(hr)) {
	  pszDestEnd = pszDest + cchDestLength;
	  cchRemaining = cchDest - cchDestLength;
	}
      }
      if(!pszSrc) pszSrc = L"";
    } else {
      hr = StringLengthWorkerW(pszDest,cchDest,&cchDestLength);
      if(SUCCEEDED(hr)) {
	pszDestEnd = pszDest + cchDestLength;
	cchRemaining = cchDest - cchDestLength;
      }
    }
    if(SUCCEEDED(hr)) {
      if(cchDest==0) {
	if(*pszSrc!=L'\0') {
	  if(!pszDest) hr = STRSAFE_E_INVALID_PARAMETER;
	  else hr = STRSAFE_E_INSUFFICIENT_BUFFER;
	}
      } else hr = StringCopyExWorkerW(pszDestEnd,cchRemaining,(cchRemaining*sizeof(wchar_t)) + (cbDest % sizeof(wchar_t)),pszSrc,&pszDestEnd,&cchRemaining,dwFlags & (~(STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)));
    }
  }
  if(FAILED(hr)) {
    if(pszDest) {
      if(dwFlags & STRSAFE_FILL_ON_FAILURE) {
	memset(pszDest,STRSAFE_GET_FILL_PATTERN(dwFlags),cbDest);
	if(STRSAFE_GET_FILL_PATTERN(dwFlags)==0) {
	  pszDestEnd = pszDest;
	  cchRemaining = cchDest;
	} else if(cchDest > 0) {
	  pszDestEnd = pszDest + cchDest - 1;
	  cchRemaining = 1;
	  *pszDestEnd = L'\0';
	}
      }
      if(dwFlags & STRSAFE_NULL_ON_FAILURE) {
	if(cchDest > 0) {
	  pszDestEnd = pszDest;
	  cchRemaining = cchDest;
	  *pszDestEnd = L'\0';
	}
      }
    }
  }
  if(SUCCEEDED(hr) || (hr==STRSAFE_E_INSUFFICIENT_BUFFER)) {
    if(ppszDestEnd) *ppszDestEnd = pszDestEnd;
    if(pcchRemaining) *pcchRemaining = cchRemaining;
  }
  return hr;
}

STRSAFEWORKERAPI StringCatNWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPCSTR pszSrc,size_t cchToAppend) {
  HRESULT hr;
  size_t cchDestLength;
  hr = StringLengthWorkerA(pszDest,cchDest,&cchDestLength);
  if(SUCCEEDED(hr)) hr = StringCopyNWorkerA(pszDest + cchDestLength,cchDest - cchDestLength,pszSrc,cchToAppend);
  return hr;
}

STRSAFEWORKERAPI StringCatNWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPCWSTR pszSrc,size_t cchToAppend) {
  HRESULT hr;
  size_t cchDestLength;
  hr = StringLengthWorkerW(pszDest,cchDest,&cchDestLength);
  if(SUCCEEDED(hr)) hr = StringCopyNWorkerW(pszDest + cchDestLength,cchDest - cchDestLength,pszSrc,cchToAppend);
  return hr;
}

STRSAFEWORKERAPI StringCatNExWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCSTR pszSrc,size_t cchToAppend,STRSAFE_LPSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags) {
  HRESULT hr = S_OK;
  STRSAFE_LPSTR pszDestEnd = pszDest;
  size_t cchRemaining = 0;
  size_t cchDestLength = 0;
  if(dwFlags & (~STRSAFE_VALID_FLAGS)) hr = STRSAFE_E_INVALID_PARAMETER;
  else if(cchToAppend > STRSAFE_MAX_CCH) hr = STRSAFE_E_INVALID_PARAMETER;
  else {
    if(dwFlags & STRSAFE_IGNORE_NULLS) {
      if(!pszDest) {
	if((cchDest==0) && (cbDest==0)) cchDestLength = 0;
	else hr = STRSAFE_E_INVALID_PARAMETER;
      } else {
	hr = StringLengthWorkerA(pszDest,cchDest,&cchDestLength);
	if(SUCCEEDED(hr)) {
	  pszDestEnd = pszDest + cchDestLength;
	  cchRemaining = cchDest - cchDestLength;
	}
      }
      if(!pszSrc) pszSrc = "";
    } else {
      hr = StringLengthWorkerA(pszDest,cchDest,&cchDestLength);
      if(SUCCEEDED(hr)) {
	pszDestEnd = pszDest + cchDestLength;
	cchRemaining = cchDest - cchDestLength;
      }
    }
    if(SUCCEEDED(hr)) {
      if(cchDest==0) {
	if((cchToAppend!=0) && (*pszSrc!='\0')) {
	  if(!pszDest) hr = STRSAFE_E_INVALID_PARAMETER;
	  else hr = STRSAFE_E_INSUFFICIENT_BUFFER;
	}
      } else hr = StringCopyNExWorkerA(pszDestEnd,cchRemaining,(cchRemaining*sizeof(char)) + (cbDest % sizeof(char)),pszSrc,cchToAppend,&pszDestEnd,&cchRemaining,dwFlags & (~(STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)));
    }
  }
  if(FAILED(hr)) {
    if(pszDest) {
      if(dwFlags & STRSAFE_FILL_ON_FAILURE) {
	memset(pszDest,STRSAFE_GET_FILL_PATTERN(dwFlags),cbDest);
	if(STRSAFE_GET_FILL_PATTERN(dwFlags)==0) {
	  pszDestEnd = pszDest;
	  cchRemaining = cchDest;
	} else if(cchDest > 0) {
	  pszDestEnd = pszDest + cchDest - 1;
	  cchRemaining = 1;
	  *pszDestEnd = '\0';
	}
      }
      if(dwFlags & (STRSAFE_NULL_ON_FAILURE)) {
	if(cchDest > 0) {
	  pszDestEnd = pszDest;
	  cchRemaining = cchDest;
	  *pszDestEnd = '\0';
	}
      }
    }
  }
  if(SUCCEEDED(hr) || (hr==STRSAFE_E_INSUFFICIENT_BUFFER)) {
    if(ppszDestEnd) *ppszDestEnd = pszDestEnd;
    if(pcchRemaining) *pcchRemaining = cchRemaining;
  }
  return hr;
}

STRSAFEWORKERAPI StringCatNExWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCWSTR pszSrc,size_t cchToAppend,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags) {
  HRESULT hr = S_OK;
  STRSAFE_LPWSTR pszDestEnd = pszDest;
  size_t cchRemaining = 0;
  size_t cchDestLength = 0;
  if(dwFlags & (~STRSAFE_VALID_FLAGS)) hr = STRSAFE_E_INVALID_PARAMETER;
  else if(cchToAppend > STRSAFE_MAX_CCH) hr = STRSAFE_E_INVALID_PARAMETER;
  else {
    if(dwFlags & STRSAFE_IGNORE_NULLS) {
      if(!pszDest) {
	if((cchDest==0) && (cbDest==0)) cchDestLength = 0;
	else hr = STRSAFE_E_INVALID_PARAMETER;
      } else {
	hr = StringLengthWorkerW(pszDest,cchDest,&cchDestLength);
	if(SUCCEEDED(hr)) {
	  pszDestEnd = pszDest + cchDestLength;
	  cchRemaining = cchDest - cchDestLength;
	}
      }
      if(!pszSrc) pszSrc = L"";
    } else {
      hr = StringLengthWorkerW(pszDest,cchDest,&cchDestLength);
      if(SUCCEEDED(hr)) {
	pszDestEnd = pszDest + cchDestLength;
	cchRemaining = cchDest - cchDestLength;
      }
    }
    if(SUCCEEDED(hr)) {
      if(cchDest==0) {
	if((cchToAppend!=0) && (*pszSrc!=L'\0')) {
	  if(!pszDest) hr = STRSAFE_E_INVALID_PARAMETER;
	  else hr = STRSAFE_E_INSUFFICIENT_BUFFER;
	}
      } else hr = StringCopyNExWorkerW(pszDestEnd,cchRemaining,(cchRemaining*sizeof(wchar_t)) + (cbDest % sizeof(wchar_t)),pszSrc,cchToAppend,&pszDestEnd,&cchRemaining,dwFlags & (~(STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)));
    }
  }
  if(FAILED(hr)) {
    if(pszDest) {
      if(dwFlags & STRSAFE_FILL_ON_FAILURE) {
	memset(pszDest,STRSAFE_GET_FILL_PATTERN(dwFlags),cbDest);
	if(STRSAFE_GET_FILL_PATTERN(dwFlags)==0) {
	  pszDestEnd = pszDest;
	  cchRemaining = cchDest;
	} else if(cchDest > 0) {
	  pszDestEnd = pszDest + cchDest - 1;
	  cchRemaining = 1;
	  *pszDestEnd = L'\0';
	}
      }
      if(dwFlags & (STRSAFE_NULL_ON_FAILURE)) {
	if(cchDest > 0) {
	  pszDestEnd = pszDest;
	  cchRemaining = cchDest;
	  *pszDestEnd = L'\0';
	}
      }
    }
  }
  if(SUCCEEDED(hr) || (hr==STRSAFE_E_INSUFFICIENT_BUFFER)) {
    if(ppszDestEnd) *ppszDestEnd = pszDestEnd;
    if(pcchRemaining) *pcchRemaining = cchRemaining;
  }
  return hr;
}

STRSAFEWORKERAPI StringVPrintfWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPCSTR pszFormat,va_list argList) {
  HRESULT hr = S_OK;
  if(cchDest==0) hr = STRSAFE_E_INVALID_PARAMETER;
  else {
    int iRet;
    size_t cchMax;
    cchMax = cchDest - 1;
    iRet = vsnprintf(pszDest,cchMax,pszFormat,argList);
    if((iRet < 0) || (((size_t)iRet) > cchMax)) {
      pszDest += cchMax;
      *pszDest = '\0';
      hr = STRSAFE_E_INSUFFICIENT_BUFFER;
    } else if(((size_t)iRet)==cchMax) {
      pszDest += cchMax;
      *pszDest = '\0';
    }
  }
  return hr;
}

STRSAFEWORKERAPI StringVPrintfWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPCWSTR pszFormat,va_list argList) {
  HRESULT hr = S_OK;
  if(cchDest==0) hr = STRSAFE_E_INVALID_PARAMETER;
  else {
    int iRet;
    size_t cchMax;
    cchMax = cchDest - 1;
    iRet = vswprintf(pszDest,cchMax,pszFormat,argList);
    if((iRet < 0) || (((size_t)iRet) > cchMax)) {
      pszDest += cchMax;
      *pszDest = L'\0';
      hr = STRSAFE_E_INSUFFICIENT_BUFFER;
    } else if(((size_t)iRet)==cchMax) {
      pszDest += cchMax;
      *pszDest = L'\0';
    }
  }
  return hr;
}

STRSAFEWORKERAPI StringVPrintfExWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags,STRSAFE_LPCSTR pszFormat,va_list argList) {
  HRESULT hr = S_OK;
  STRSAFE_LPSTR pszDestEnd = pszDest;
  size_t cchRemaining = 0;
  if(dwFlags & (~STRSAFE_VALID_FLAGS)) hr = STRSAFE_E_INVALID_PARAMETER;
  else {
    if(dwFlags & STRSAFE_IGNORE_NULLS) {
      if(!pszDest) {
	if((cchDest!=0) || (cbDest!=0)) hr = STRSAFE_E_INVALID_PARAMETER;
      }
      if(!pszFormat) pszFormat = "";
    }
    if(SUCCEEDED(hr)) {
      if(cchDest==0) {
	pszDestEnd = pszDest;
	cchRemaining = 0;
	if(*pszFormat!='\0') {
	  if(!pszDest) hr = STRSAFE_E_INVALID_PARAMETER;
	  else hr = STRSAFE_E_INSUFFICIENT_BUFFER;
	}
      } else {
	int iRet;
	size_t cchMax;
	cchMax = cchDest - 1;
	iRet = vsnprintf(pszDest,cchMax,pszFormat,argList);
	if((iRet < 0) || (((size_t)iRet) > cchMax)) {
	  pszDestEnd = pszDest + cchMax;
	  cchRemaining = 1;
	  *pszDestEnd = '\0';
	  hr = STRSAFE_E_INSUFFICIENT_BUFFER;
	} else if(((size_t)iRet)==cchMax) {
	  pszDestEnd = pszDest + cchMax;
	  cchRemaining = 1;
	  *pszDestEnd = '\0';
	} else if(((size_t)iRet) < cchMax) {
	  pszDestEnd = pszDest + iRet;
	  cchRemaining = cchDest - iRet;
	  if(dwFlags & STRSAFE_FILL_BEHIND_NULL) {
	    memset(pszDestEnd + 1,STRSAFE_GET_FILL_PATTERN(dwFlags),((cchRemaining - 1)*sizeof(char)) + (cbDest % sizeof(char)));
	  }
	}
      }
    }
  }
  if(FAILED(hr)) {
    if(pszDest) {
      if(dwFlags & STRSAFE_FILL_ON_FAILURE) {
	memset(pszDest,STRSAFE_GET_FILL_PATTERN(dwFlags),cbDest);
	if(STRSAFE_GET_FILL_PATTERN(dwFlags)==0) {
	  pszDestEnd = pszDest;
	  cchRemaining = cchDest;
	} else if(cchDest > 0) {
	  pszDestEnd = pszDest + cchDest - 1;
	  cchRemaining = 1;
	  *pszDestEnd = '\0';
	}
      }
      if(dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION)) {
	if(cchDest > 0) {
	  pszDestEnd = pszDest;
	  cchRemaining = cchDest;
	  *pszDestEnd = '\0';
	}
      }
    }
  }
  if(SUCCEEDED(hr) || (hr==STRSAFE_E_INSUFFICIENT_BUFFER)) {
    if(ppszDestEnd) *ppszDestEnd = pszDestEnd;
    if(pcchRemaining) *pcchRemaining = cchRemaining;
  }
  return hr;
}

STRSAFEWORKERAPI StringVPrintfExWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags,STRSAFE_LPCWSTR pszFormat,va_list argList) {
  HRESULT hr = S_OK;
  STRSAFE_LPWSTR pszDestEnd = pszDest;
  size_t cchRemaining = 0;
  if(dwFlags & (~STRSAFE_VALID_FLAGS)) hr = STRSAFE_E_INVALID_PARAMETER;
  else {
    if(dwFlags & STRSAFE_IGNORE_NULLS) {
      if(!pszDest) {
	if((cchDest!=0) || (cbDest!=0)) hr = STRSAFE_E_INVALID_PARAMETER;
      }
      if(!pszFormat) pszFormat = L"";
    }
    if(SUCCEEDED(hr)) {
      if(cchDest==0) {
	pszDestEnd = pszDest;
	cchRemaining = 0;
	if(*pszFormat!=L'\0') {
	  if(!pszDest) hr = STRSAFE_E_INVALID_PARAMETER;
	  else hr = STRSAFE_E_INSUFFICIENT_BUFFER;
	}
      } else {
	int iRet;
	size_t cchMax;
	cchMax = cchDest - 1;
	iRet = vswprintf(pszDest,cchMax,pszFormat,argList);
	if((iRet < 0) || (((size_t)iRet) > cchMax)) {
	  pszDestEnd = pszDest + cchMax;
	  cchRemaining = 1;
	  *pszDestEnd = L'\0';
	  hr = STRSAFE_E_INSUFFICIENT_BUFFER;
	} else if(((size_t)iRet)==cchMax) {
	  pszDestEnd = pszDest + cchMax;
	  cchRemaining = 1;
	  *pszDestEnd = L'\0';
	} else if(((size_t)iRet) < cchMax) {
	  pszDestEnd = pszDest + iRet;
	  cchRemaining = cchDest - iRet;
	  if(dwFlags & STRSAFE_FILL_BEHIND_NULL) {
	    memset(pszDestEnd + 1,STRSAFE_GET_FILL_PATTERN(dwFlags),((cchRemaining - 1)*sizeof(wchar_t)) + (cbDest % sizeof(wchar_t)));
	  }
	}
      }
    }
  }
  if(FAILED(hr)) {
    if(pszDest) {
      if(dwFlags & STRSAFE_FILL_ON_FAILURE) {
	memset(pszDest,STRSAFE_GET_FILL_PATTERN(dwFlags),cbDest);
	if(STRSAFE_GET_FILL_PATTERN(dwFlags)==0) {
	  pszDestEnd = pszDest;
	  cchRemaining = cchDest;
	} else if(cchDest > 0) {
	  pszDestEnd = pszDest + cchDest - 1;
	  cchRemaining = 1;
	  *pszDestEnd = L'\0';
	}
      }
      if(dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION)) {
	if(cchDest > 0) {
	  pszDestEnd = pszDest;
	  cchRemaining = cchDest;
	  *pszDestEnd = L'\0';
	}
      }
    }
  }
  if(SUCCEEDED(hr) || (hr==STRSAFE_E_INSUFFICIENT_BUFFER)) {
    if(ppszDestEnd) *ppszDestEnd = pszDestEnd;
    if(pcchRemaining) *pcchRemaining = cchRemaining;
  }
  return hr;
}

STRSAFEWORKERAPI StringLengthWorkerA(STRSAFE_LPCSTR psz,size_t cchMax,size_t *pcchLength) {
  HRESULT hr = S_OK;
  size_t cchMaxPrev = cchMax;
  while(cchMax && (*psz!='\0')) {
    psz++;
    cchMax--;
  }
  if(cchMax==0) hr = STRSAFE_E_INVALID_PARAMETER;
  if(pcchLength) {
    if(SUCCEEDED(hr)) *pcchLength = cchMaxPrev - cchMax;
    else *pcchLength = 0;
  }
  return hr;
}

STRSAFEWORKERAPI StringLengthWorkerW(STRSAFE_LPCWSTR psz,size_t cchMax,size_t *pcchLength) {
  HRESULT hr = S_OK;
  size_t cchMaxPrev = cchMax;
  while(cchMax && (*psz!=L'\0')) {
    psz++;
    cchMax--;
  }
  if(cchMax==0) hr = STRSAFE_E_INVALID_PARAMETER;
  if(pcchLength) {
    if(SUCCEEDED(hr)) *pcchLength = cchMaxPrev - cchMax;
    else *pcchLength = 0;
  }
  return hr;
}

#endif /* STRSAFE_LIB_IMPL || !STRSAFE_LIB */

#ifndef STRSAFE_LIB_IMPL

STRSAFEAPI StringGetsExWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags) {
  HRESULT hr = S_OK;
  STRSAFE_LPSTR pszDestEnd = pszDest;
  size_t cchRemaining = 0;

  if(dwFlags & (~STRSAFE_VALID_FLAGS)) hr = STRSAFE_E_INVALID_PARAMETER;
  else {
    if(dwFlags & STRSAFE_IGNORE_NULLS) {
      if(!pszDest) {
	if((cchDest!=0) || (cbDest!=0)) hr = STRSAFE_E_INVALID_PARAMETER;
      }
    }
    if(SUCCEEDED(hr)) {
      if(cchDest <= 1) {
	pszDestEnd = pszDest;
	cchRemaining = cchDest;
	if(cchDest==1) *pszDestEnd = '\0';
	hr = STRSAFE_E_INSUFFICIENT_BUFFER;
      } else {
	pszDestEnd = pszDest;
	cchRemaining = cchDest;
	while(cchRemaining > 1) {
	  char ch;
	  int i = getc(stdin);
	  if(i==EOF) {
	    if(pszDestEnd==pszDest) hr = STRSAFE_E_END_OF_FILE;
	    break;
	  }
	  ch = (char)i;
	  if(ch=='\n') break;
	  *pszDestEnd = ch;
	  pszDestEnd++;
	  cchRemaining--;
	}
	if(cchRemaining > 0) {
	  if(dwFlags & STRSAFE_FILL_BEHIND_NULL) {
	    memset(pszDestEnd + 1,STRSAFE_GET_FILL_PATTERN(dwFlags),((cchRemaining - 1)*sizeof(char)) + (cbDest % sizeof(char)));
	  }
	}
	*pszDestEnd = '\0';
      }
    }
  }
  if(FAILED(hr)) {
    if(pszDest) {
      if(dwFlags & STRSAFE_FILL_ON_FAILURE) {
	memset(pszDest,STRSAFE_GET_FILL_PATTERN(dwFlags),cbDest);
	if(STRSAFE_GET_FILL_PATTERN(dwFlags)==0) {
	  pszDestEnd = pszDest;
	  cchRemaining = cchDest;
	} else if(cchDest > 0) {
	  pszDestEnd = pszDest + cchDest - 1;
	  cchRemaining = 1;
	  *pszDestEnd = '\0';
	}
      }
      if(dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION)) {
	if(cchDest > 0) {
	  pszDestEnd = pszDest;
	  cchRemaining = cchDest;
	  *pszDestEnd = '\0';
	}
      }
    }
  }
  if(SUCCEEDED(hr) || (hr==STRSAFE_E_INSUFFICIENT_BUFFER) || (hr==STRSAFE_E_END_OF_FILE)) {
    if(ppszDestEnd) *ppszDestEnd = pszDestEnd;
    if(pcchRemaining) *pcchRemaining = cchRemaining;
  }
  return hr;
}

STRSAFEAPI StringGetsExWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcchRemaining,DWORD dwFlags) {
  HRESULT hr = S_OK;
  STRSAFE_LPWSTR pszDestEnd = pszDest;
  size_t cchRemaining = 0;
  if(dwFlags & (~STRSAFE_VALID_FLAGS)) {
    hr = STRSAFE_E_INVALID_PARAMETER;
  } else {
    if(dwFlags & STRSAFE_IGNORE_NULLS) {
      if(!pszDest) {
	if((cchDest!=0) || (cbDest!=0)) hr = STRSAFE_E_INVALID_PARAMETER;
      }
    }
    if(SUCCEEDED(hr)) {
      if(cchDest <= 1) {
	pszDestEnd = pszDest;
	cchRemaining = cchDest;
	if(cchDest==1) *pszDestEnd = L'\0';
	hr = STRSAFE_E_INSUFFICIENT_BUFFER;
      } else {
	pszDestEnd = pszDest;
	cchRemaining = cchDest;
	while(cchRemaining > 1) {
	  wchar_t ch = getwc(stdin);
	  if(ch==WEOF) {
	    if(pszDestEnd==pszDest) hr = STRSAFE_E_END_OF_FILE;
	    break;
	  }
	  if(ch==L'\n') break;
	  *pszDestEnd = ch;
	  pszDestEnd++;
	  cchRemaining--;
	}
	if(cchRemaining > 0) {
	  if(dwFlags & STRSAFE_FILL_BEHIND_NULL) {
	    memset(pszDestEnd + 1,STRSAFE_GET_FILL_PATTERN(dwFlags),((cchRemaining - 1)*sizeof(wchar_t)) + (cbDest % sizeof(wchar_t)));
	  }
	}
	*pszDestEnd = L'\0';
      }
    }
  }
  if(FAILED(hr)) {
    if(pszDest) {
      if(dwFlags & STRSAFE_FILL_ON_FAILURE) {
	memset(pszDest,STRSAFE_GET_FILL_PATTERN(dwFlags),cbDest);
	if(STRSAFE_GET_FILL_PATTERN(dwFlags)==0) {
	  pszDestEnd = pszDest;
	  cchRemaining = cchDest;
	} else if(cchDest > 0) {
	  pszDestEnd = pszDest + cchDest - 1;
	  cchRemaining = 1;
	  *pszDestEnd = L'\0';
	}
      }
      if(dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION)) {
	if(cchDest > 0) {
	  pszDestEnd = pszDest;
	  cchRemaining = cchDest;
	  *pszDestEnd = L'\0';
	}
      }
    }
  }
  if(SUCCEEDED(hr) || (hr==STRSAFE_E_INSUFFICIENT_BUFFER) || (hr==STRSAFE_E_END_OF_FILE)) {
    if(ppszDestEnd) *ppszDestEnd = pszDestEnd;
    if(pcchRemaining) *pcchRemaining = cchRemaining;
  }
  return hr;
}

#endif /* STRSAFE_LIB_IMPL */

#ifndef DEPRECATE_SUPPORTED
#define StringCopyWorkerA StringCopyWorkerA_instead_use_StringCchCopyA_or_StringCchCopyExA
#define StringCopyWorkerW StringCopyWorkerW_instead_use_StringCchCopyW_or_StringCchCopyExW
#define StringCopyExWorkerA StringCopyExWorkerA_instead_use_StringCchCopyA_or_StringCchCopyExA
#define StringCopyExWorkerW StringCopyExWorkerW_instead_use_StringCchCopyW_or_StringCchCopyExW
#define StringCatWorkerA StringCatWorkerA_instead_use_StringCchCatA_or_StringCchCatExA
#define StringCatWorkerW StringCatWorkerW_instead_use_StringCchCatW_or_StringCchCatExW
#define StringCatExWorkerA StringCatExWorkerA_instead_use_StringCchCatA_or_StringCchCatExA
#define StringCatExWorkerW StringCatExWorkerW_instead_use_StringCchCatW_or_StringCchCatExW
#define StringCatNWorkerA StringCatNWorkerA_instead_use_StringCchCatNA_or_StrincCbCatNA
#define StringCatNWorkerW StringCatNWorkerW_instead_use_StringCchCatNW_or_StringCbCatNW
#define StringCatNExWorkerA StringCatNExWorkerA_instead_use_StringCchCatNExA_or_StringCbCatNExA
#define StringCatNExWorkerW StringCatNExWorkerW_instead_use_StringCchCatNExW_or_StringCbCatNExW
#define StringVPrintfWorkerA StringVPrintfWorkerA_instead_use_StringCchVPrintfA_or_StringCchVPrintfExA
#define StringVPrintfWorkerW StringVPrintfWorkerW_instead_use_StringCchVPrintfW_or_StringCchVPrintfExW
#define StringVPrintfExWorkerA StringVPrintfExWorkerA_instead_use_StringCchVPrintfA_or_StringCchVPrintfExA
#define StringVPrintfExWorkerW StringVPrintfExWorkerW_instead_use_StringCchVPrintfW_or_StringCchVPrintfExW
#define StringLengthWorkerA StringLengthWorkerA_instead_use_StringCchLengthA_or_StringCbLengthA
#define StringLengthWorkerW StringLengthWorkerW_instead_use_StringCchLengthW_or_StringCbLengthW
#define StringGetsExWorkerA StringGetsExWorkerA_instead_use_StringCchGetsA_or_StringCbGetsA
#define StringGetsExWorkerW StringGetsExWorkerW_instead_use_StringCchGetsW_or_StringCbGetsW
#endif /* !DEPRECATE_SUPPORTED */

#endif
