/*
 *      msacmdrv.h   -       Declarations for MSACM driver
 */

#ifndef __WINE_MSACMDRV_H
#define __WINE_MSACMDRV_H

#include "wintypes.h"
#include "msacm.h"

/***********************************************************************
 * Types
 */

/***********************************************************************
 * Defines/Enums
 */

#define MAKE_ACM_VERSION(mjr, mnr, bld) \
  (((long)(mjr)<<24) | ((long)(mnr)<<16) | ((long)bld))

#define ACMDRVOPENDESC_SECTIONNAME_CHARS    

#define ACMDM_DRIVER_NOTIFY             (ACMDM_BASE + 1)
#define ACMDM_DRIVER_DETAILS            (ACMDM_BASE + 10)

#define ACMDM_HARDWARE_WAVE_CAPS_INPUT  (ACMDM_BASE + 20)
#define ACMDM_HARDWARE_WAVE_CAPS_OUTPUT (ACMDM_BASE + 21)

#define ACMDM_FORMATTAG_DETAILS         (ACMDM_BASE + 25)
#define ACMDM_FORMAT_DETAILS            (ACMDM_BASE + 26)
#define ACMDM_FORMAT_SUGGEST            (ACMDM_BASE + 27)

#define ACMDM_FILTERTAG_DETAILS         (ACMDM_BASE + 50)
#define ACMDM_FILTER_DETAILS            (ACMDM_BASE + 51)

#define ACMDM_STREAM_OPEN               (ACMDM_BASE + 76)
#define ACMDM_STREAM_CLOSE              (ACMDM_BASE + 77)
#define ACMDM_STREAM_SIZE               (ACMDM_BASE + 78)
#define ACMDM_STREAM_CONVERT            (ACMDM_BASE + 79)
#define ACMDM_STREAM_RESET              (ACMDM_BASE + 80)
#define ACMDM_STREAM_PREPARE            (ACMDM_BASE + 81)
#define ACMDM_STREAM_UNPREPARE          (ACMDM_BASE + 82)
#define ACMDM_STREAM_UPDATE             (ACMDM_BASE + 83)

/***********************************************************************
 * Structures
 */

typedef struct _ACMDRVOPENDESC32A
{
  DWORD  cbStruct;
  FOURCC fccType;
  FOURCC fccComp;
  DWORD  dwVersion;
  DWORD  dwFlags;
  DWORD  dwError;
  LPCSTR pszSectionName;
  LPCSTR pszAliasName;
  DWORD  dnDevNode;
} ACMDRVOPENDESC32A, *PACMDRVOPENDESC32A;

typedef struct _ACMDRVOPENDESC32W
{
  DWORD   cbStruct;
  FOURCC  fccType;
  FOURCC  fccComp;
  DWORD   dwVersion;
  DWORD   dwFlags;
  DWORD   dwError;
  LPCWSTR pszSectionName;
  LPCWSTR pszAliasName;
  DWORD   dnDevNode;
} ACMDRVOPENDESC32W, *PACMDRVOPENDESC32W;

typedef struct _ACMDRVOPENDESC16
{
  DWORD  cbStruct;
  FOURCC fccType;
  FOURCC fccComp;
  DWORD  dwVersion;
  DWORD  dwFlags;
  DWORD  dwError;
  LPCSTR pszSectionName;
  LPCSTR pszAliasName;
  DWORD  dnDevNode;
} ACMDRVOPENDESC16, *NPACMDRVOPENDESC16, *LPACMDRVOPENDESC16;

typedef struct _ACMDRVSTREAMINSTANCE16
{
  DWORD            cbStruct;
  LPWAVEFORMATEX16 pwfxSrc;
  LPWAVEFORMATEX16 pwfxDst;
  LPWAVEFILTER16   pwfltr;
  DWORD            dwCallback;
  DWORD            dwInstance;
  DWORD            fdwOpen;
  DWORD            fdwDriver;
  DWORD            dwDriver;
  HACMSTREAM16     has;
} ACMDRVSTREAMINSTANCE16, *NPACMDRVSTREAMINSTANCE16, *LPACMDRVSTREAMINSTANCE16;

typedef struct _ACMDRVSTREAMINSTANCE32
{
  DWORD           cbStruct;
  PWAVEFORMATEX32 pwfxSrc;
  PWAVEFORMATEX32 pwfxDst;
  PWAVEFILTER32   pwfltr;
  DWORD           dwCallback;
  DWORD           dwInstance;
  DWORD           fdwOpen;
  DWORD           fdwDriver;
  DWORD           dwDriver;
  HACMSTREAM32    has;
} ACMDRVSTREAMINSTANCE32, *PACMDRVSTREAMINSTANCE32;


typedef struct _ACMDRVSTREAMHEADER16 *LPACMDRVSTREAMHEADER16;
typedef struct _ACMDRVSTREAMHEADER16 {
  DWORD  cbStruct;
  DWORD  fdwStatus;
  DWORD  dwUser;
  LPBYTE pbSrc;
  DWORD  cbSrcLength;
  DWORD  cbSrcLengthUsed;
  DWORD  dwSrcUser;
  LPBYTE pbDst;
  DWORD  cbDstLength;
  DWORD  cbDstLengthUsed;
  DWORD  dwDstUser;

  DWORD fdwConvert;
  LPACMDRVSTREAMHEADER16 *padshNext;
  DWORD fdwDriver;
  DWORD dwDriver;

  /* Internal fields for ACM */
  DWORD  fdwPrepared;
  DWORD  dwPrepared;
  LPBYTE pbPreparedSrc;
  DWORD  cbPreparedSrcLength;
  LPBYTE pbPreparedDst;
  DWORD  cbPreparedDstLength;
} ACMDRVSTREAMHEADER16, *NPACMDRVSTREAMHEADER16;

typedef struct _ACMDRVSTREAMHEADER32 *PACMDRVSTREAMHEADER32;
typedef struct _ACMDRVSTREAMHEADER32 {
  DWORD  cbStruct;
  DWORD  fdwStatus;
  DWORD  dwUser;
  LPBYTE pbSrc;
  DWORD  cbSrcLength;
  DWORD  cbSrcLengthUsed;
  DWORD  dwSrcUser;
  LPBYTE pbDst;
  DWORD  cbDstLength;
  DWORD  cbDstLengthUsed;
  DWORD  dwDstUser;

  DWORD fdwConvert;
  PACMDRVSTREAMHEADER32 *padshNext;
  DWORD fdwDriver;
  DWORD dwDriver;

  /* Internal fields for ACM */
  DWORD  fdwPrepared;
  DWORD  dwPrepared;
  LPBYTE pbPreparedSrc;
  DWORD  cbPreparedSrcLength;
  LPBYTE pbPreparedDst;
  DWORD  cbPreparedDstLength;
} ACMDRVSTREAMHEADER32;

typedef struct _ACMDRVSTREAMSIZE
{
  DWORD cbStruct;
  DWORD fdwSize;
  DWORD cbSrcLength;
  DWORD cbDstLength;
} ACMDRVSTREAMSIZE16, *NPACMDRVSTREAMSIZE16, *LPACMDRVSTREAMSIZE16,
  ACMDRVSTREAMSIZE32, *PACMDRVSTREAMSIZE;

typedef struct _ACMDRVFORMATSUGGEST16
{
  DWORD            cbStruct;
  DWORD            fdwSuggest;
  LPWAVEFORMATEX16 pwfxSrc;
  DWORD            cbwfxSrc;
  LPWAVEFORMATEX16 pwfxDst;
  DWORD            cbwfxDst;
} ACMDRVFORMATSUGGEST16, *NPACMDRVFORMATSUGGEST, *LPACMDRVFORMATSUGGEST;

typedef struct _ACMDRVFORMATSUGGEST32
{
  DWORD           cbStruct;
  DWORD           fdwSuggest;
  PWAVEFORMATEX32 pwfxSrc;
  DWORD           cbwfxSrc;
  PWAVEFORMATEX32 pwfxDst;
  DWORD           cbwfxDst;
} ACMDRVFORMATSUGGEST32, *PACMDRVFORMATSUGGEST32;

#endif  /* __WINE_MSACMDRV_H */
