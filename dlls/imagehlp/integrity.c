/*
 *	IMAGEHLP library
 *
 *	Copyright 1998	Patrik Stridvall
 */

#include "wintypes.h"
#include "winbase.h"
#include "winerror.h"
#include "imagehlp.h"
#include "debug.h"

/***********************************************************************
 *           ImageAddCertificate32 (IMAGEHLP.10)
 */

BOOL WINAPI ImageAddCertificate(
  HANDLE FileHandle, PWIN_CERTIFICATE Certificate, PDWORD Index)
{
  FIXME(imagehlp, "(0x%08x, %p, %p): stub\n",
    FileHandle, Certificate, Index
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImageEnumerateCertificates32 (IMAGEHLP.12)
 */
BOOL WINAPI ImageEnumerateCertificates(
  HANDLE FileHandle, WORD TypeFilter, PDWORD CertificateCount,
  PDWORD Indices, DWORD IndexCount)
{
  FIXME(imagehlp, "(0x%08x, %hd, %p, %p, %ld): stub\n",
    FileHandle, TypeFilter, CertificateCount, Indices, IndexCount
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImageGetCertificateData32 (IMAGEHLP.13)
 */
BOOL WINAPI ImageGetCertificateData(
  HANDLE FileHandle, DWORD CertificateIndex,
  PWIN_CERTIFICATE Certificate, PDWORD RequiredLength)
{
  FIXME(imagehlp, "(0x%08x, %ld, %p, %p): stub\n",
    FileHandle, CertificateIndex, Certificate, RequiredLength
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImageGetCertificateHeader32 (IMAGEHLP.14)
 */
BOOL WINAPI ImageGetCertificateHeader(
  HANDLE FileHandle, DWORD CertificateIndex,
  PWIN_CERTIFICATE Certificateheader)
{
  FIXME(imagehlp, "(0x%08x, %ld, %p): stub\n",
    FileHandle, CertificateIndex, Certificateheader
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImageGetDigestStream32 (IMAGEHLP.15)
 */
BOOL WINAPI ImageGetDigestStream(
  HANDLE FileHandle, DWORD DigestLevel,
  DIGEST_FUNCTION DigestFunction, DIGEST_HANDLE DigestHandle)
{
  FIXME(imagehlp, "(%0x08x, %ld, %p, %p): stub\n",
    FileHandle, DigestLevel, DigestFunction, DigestHandle
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImageRemoveCertificate32 (IMAGEHLP.18)
 */
BOOL WINAPI ImageRemoveCertificate(HANDLE FileHandle, DWORD Index)
{
  FIXME(imagehlp, "(0x%08x, %ld): stub\n", FileHandle, Index);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}
