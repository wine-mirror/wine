/*
 *	IMAGEHLP library
 *
 *	Copyright 1998	Patrik Stridvall
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

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "imagehlp.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(imagehlp);

/***********************************************************************
 *		ImageAddCertificate (IMAGEHLP.@)
 */

BOOL WINAPI ImageAddCertificate(
  HANDLE FileHandle, PWIN_CERTIFICATE Certificate, PDWORD Index)
{
  FIXME("(%p, %p, %p): stub\n",
    FileHandle, Certificate, Index
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImageEnumerateCertificates (IMAGEHLP.@)
 */
BOOL WINAPI ImageEnumerateCertificates(
  HANDLE FileHandle, WORD TypeFilter, PDWORD CertificateCount,
  PDWORD Indices, DWORD IndexCount)
{
  FIXME("(%p, %hd, %p, %p, %ld): stub\n",
    FileHandle, TypeFilter, CertificateCount, Indices, IndexCount
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImageGetCertificateData (IMAGEHLP.@)
 */
BOOL WINAPI ImageGetCertificateData(
  HANDLE FileHandle, DWORD CertificateIndex,
  PWIN_CERTIFICATE Certificate, PDWORD RequiredLength)
{
  FIXME("(%p, %ld, %p, %p): stub\n",
    FileHandle, CertificateIndex, Certificate, RequiredLength
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImageGetCertificateHeader (IMAGEHLP.@)
 */
BOOL WINAPI ImageGetCertificateHeader(
  HANDLE FileHandle, DWORD CertificateIndex,
  PWIN_CERTIFICATE Certificateheader)
{
  FIXME("(%p, %ld, %p): stub\n",
    FileHandle, CertificateIndex, Certificateheader
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImageGetDigestStream (IMAGEHLP.@)
 */
BOOL WINAPI ImageGetDigestStream(
  HANDLE FileHandle, DWORD DigestLevel,
  DIGEST_FUNCTION DigestFunction, DIGEST_HANDLE DigestHandle)
{
  FIXME("(%p, %ld, %p, %p): stub\n",
    FileHandle, DigestLevel, DigestFunction, DigestHandle
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImageRemoveCertificate (IMAGEHLP.@)
 */
BOOL WINAPI ImageRemoveCertificate(HANDLE FileHandle, DWORD Index)
{
  FIXME("(%p, %ld): stub\n", FileHandle, Index);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}
