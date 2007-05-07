/*
 *	IMAGEHLP library
 *
 *	Copyright 1998	Patrik Stridvall
 *	Copyright 2003	Mike McCormack
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
#include "winerror.h"
#include "winternl.h"
#include "winnt.h"
#include "imagehlp.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(imagehlp);

/*
 * These functions are partially documented at:
 *   http://www.cs.auckland.ac.nz/~pgut001/pubs/authenticode.txt
 */

/***********************************************************************
 * IMAGEHLP_GetSecurityDirOffset (INTERNAL)
 *
 * Read a file's PE header, and return the offset and size of the 
 *  security directory.
 */
static BOOL IMAGEHLP_GetSecurityDirOffset( HANDLE handle, 
                                           DWORD *pdwOfs, DWORD *pdwSize )
{
    IMAGE_DOS_HEADER dos_hdr;
    IMAGE_NT_HEADERS nt_hdr;
    DWORD count;
    BOOL r;
    IMAGE_DATA_DIRECTORY *sd;

    TRACE("handle %p\n", handle );

    /* read the DOS header */
    count = SetFilePointer( handle, 0, NULL, FILE_BEGIN );
    if( count == INVALID_SET_FILE_POINTER )
        return FALSE;
    count = 0;
    r = ReadFile( handle, &dos_hdr, sizeof dos_hdr, &count, NULL );
    if( !r )
        return FALSE;
    if( count != sizeof dos_hdr )
        return FALSE;

    /* read the PE header */
    count = SetFilePointer( handle, dos_hdr.e_lfanew, NULL, FILE_BEGIN );
    if( count == INVALID_SET_FILE_POINTER )
        return FALSE;
    count = 0;
    r = ReadFile( handle, &nt_hdr, sizeof nt_hdr, &count, NULL );
    if( !r )
        return FALSE;
    if( count != sizeof nt_hdr )
        return FALSE;

    sd = &nt_hdr.OptionalHeader.
                    DataDirectory[IMAGE_FILE_SECURITY_DIRECTORY];

    TRACE("size = %x addr = %x\n", sd->Size, sd->VirtualAddress);
    *pdwSize = sd->Size;
    *pdwOfs = sd->VirtualAddress;

    return TRUE;
}

/***********************************************************************
 * IMAGEHLP_GetCertificateOffset (INTERNAL)
 *
 * Read a file's PE header, and return the offset and size of the 
 *  security directory.
 */
static BOOL IMAGEHLP_GetCertificateOffset( HANDLE handle, DWORD num,
                                           DWORD *pdwOfs, DWORD *pdwSize )
{
    DWORD size, count, offset, len, sd_VirtualAddr;
    BOOL r;

    r = IMAGEHLP_GetSecurityDirOffset( handle, &sd_VirtualAddr, &size );
    if( !r )
        return FALSE;

    offset = 0;
    /* take the n'th certificate */
    while( 1 )
    {
        /* read the length of the current certificate */
        count = SetFilePointer( handle, sd_VirtualAddr + offset,
                                 NULL, FILE_BEGIN );
        if( count == INVALID_SET_FILE_POINTER )
            return FALSE;
        r = ReadFile( handle, &len, sizeof len, &count, NULL );
        if( !r )
            return FALSE;
        if( count != sizeof len )
            return FALSE;

        /* check the certificate is not too big or too small */
        if( len < sizeof len )
            return FALSE;
        if( len > (size-offset) )
            return FALSE;
        if( !num-- )
            break;

        /* calculate the offset of the next certificate */
        offset += len;
        if( offset >= size )
            return FALSE;
    }

    *pdwOfs = sd_VirtualAddr + offset;
    *pdwSize = len;

    TRACE("len = %x addr = %x\n", len, sd_VirtualAddr + offset);

    return TRUE;
}


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
    HANDLE handle, WORD TypeFilter, PDWORD CertificateCount,
    PDWORD Indices, DWORD IndexCount)
{
    DWORD size, count, offset, sd_VirtualAddr, index;
    WIN_CERTIFICATE hdr;
    const size_t cert_hdr_size = sizeof hdr - sizeof hdr.bCertificate;
    BOOL r;

    TRACE("%p %hd %p %p %d\n",
           handle, TypeFilter, CertificateCount, Indices, IndexCount);

    r = IMAGEHLP_GetSecurityDirOffset( handle, &sd_VirtualAddr, &size );
    if( !r )
        return FALSE;

    offset = 0;
    index = 0;
    *CertificateCount = 0;
    while( offset < size )
    {
        /* read the length of the current certificate */
        count = SetFilePointer( handle, sd_VirtualAddr + offset,
                                 NULL, FILE_BEGIN );
        if( count == INVALID_SET_FILE_POINTER )
            return FALSE;
        r = ReadFile( handle, &hdr, cert_hdr_size, &count, NULL );
        if( !r )
            return FALSE;
        if( count != cert_hdr_size )
            return FALSE;

        TRACE("Size = %08x  id = %08hx\n",
               hdr.dwLength, hdr.wCertificateType );

        /* check the certificate is not too big or too small */
        if( hdr.dwLength < cert_hdr_size )
            return FALSE;
        if( hdr.dwLength > (size-offset) )
            return FALSE;
       
        if( (TypeFilter == CERT_SECTION_TYPE_ANY) ||
            (TypeFilter == hdr.wCertificateType) )
        {
            (*CertificateCount)++;
            if(Indices && *CertificateCount <= IndexCount)
                *Indices++ = index;
        }

        /* next certificate */
        offset += hdr.dwLength;
        index++;
    }

    return TRUE;
}

/***********************************************************************
 *		ImageGetCertificateData (IMAGEHLP.@)
 *
 *  FIXME: not sure that I'm dealing with the Index the right way
 */
BOOL WINAPI ImageGetCertificateData(
                HANDLE handle, DWORD Index,
                PWIN_CERTIFICATE Certificate, PDWORD RequiredLength)
{
    DWORD r, offset, ofs, size, count;

    TRACE("%p %d %p %p\n", handle, Index, Certificate, RequiredLength);

    if( !IMAGEHLP_GetCertificateOffset( handle, Index, &ofs, &size ) )
        return FALSE;

    if( !Certificate )
    {
        *RequiredLength = size;
        return TRUE;
    }

    if( *RequiredLength < size )
    {
        *RequiredLength = size;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    *RequiredLength = size;

    offset = SetFilePointer( handle, ofs, NULL, FILE_BEGIN );
    if( offset == INVALID_SET_FILE_POINTER )
        return FALSE;

    r = ReadFile( handle, Certificate, size, &count, NULL );
    if( !r )
        return FALSE;
    if( count != size )
        return FALSE;

    TRACE("OK\n");

    return TRUE;
}

/***********************************************************************
 *		ImageGetCertificateHeader (IMAGEHLP.@)
 */
BOOL WINAPI ImageGetCertificateHeader(
    HANDLE handle, DWORD index, PWIN_CERTIFICATE pCert)
{
    DWORD r, offset, ofs, size, count;
    const size_t cert_hdr_size = sizeof *pCert - sizeof pCert->bCertificate;

    TRACE("%p %d %p\n", handle, index, pCert);

    if( !IMAGEHLP_GetCertificateOffset( handle, index, &ofs, &size ) )
        return FALSE;

    if( size < cert_hdr_size )
        return FALSE;

    offset = SetFilePointer( handle, ofs, NULL, FILE_BEGIN );
    if( offset == INVALID_SET_FILE_POINTER )
        return FALSE;

    r = ReadFile( handle, pCert, cert_hdr_size, &count, NULL );
    if( !r )
        return FALSE;
    if( count != cert_hdr_size )
        return FALSE;

    TRACE("OK\n");

    return TRUE;
}

/***********************************************************************
 *		ImageGetDigestStream (IMAGEHLP.@)
 */
BOOL WINAPI ImageGetDigestStream(
  HANDLE FileHandle, DWORD DigestLevel,
  DIGEST_FUNCTION DigestFunction, DIGEST_HANDLE DigestHandle)
{
  FIXME("(%p, %d, %p, %p): stub\n",
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
  FIXME("(%p, %d): stub\n", FileHandle, Index);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}
