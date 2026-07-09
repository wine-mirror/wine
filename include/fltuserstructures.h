/*
* Copyright (C) 2026 Alistair Leslie-Hughes
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
#ifndef __FLTUSERSTRUCTURES_H__
#define __FLTUSERSTRUCTURES_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _FLT_FILESYSTEM_TYPE
{
    FLT_FSTYPE_UNKNOWN,
    FLT_FSTYPE_RAW,
    FLT_FSTYPE_NTFS,
    FLT_FSTYPE_FAT,
    FLT_FSTYPE_CDFS,
    FLT_FSTYPE_UDFS,
    FLT_FSTYPE_LANMAN,
    FLT_FSTYPE_WEBDAV,
    FLT_FSTYPE_RDPDR,
    FLT_FSTYPE_NFS,
    FLT_FSTYPE_MS_NETWARE,
    FLT_FSTYPE_NETWARE,
    FLT_FSTYPE_BSUDF,
    FLT_FSTYPE_MUP,
    FLT_FSTYPE_RSFX,
    FLT_FSTYPE_ROXIO_UDF1,
    FLT_FSTYPE_ROXIO_UDF2,
    FLT_FSTYPE_ROXIO_UDF3,
    FLT_FSTYPE_TACIT,
    FLT_FSTYPE_FS_REC,
    FLT_FSTYPE_INCD,
    FLT_FSTYPE_INCD_FAT,
    FLT_FSTYPE_EXFAT,
    FLT_FSTYPE_PSFS,
    FLT_FSTYPE_GPFS,
    FLT_FSTYPE_NPFS,
    FLT_FSTYPE_MSFS,
    FLT_FSTYPE_CSVFS,
    FLT_FSTYPE_REFS,
    FLT_FSTYPE_OPENAFS
} FLT_FILESYSTEM_TYPE, *PFLT_FILESYSTEM_TYPE;

typedef struct _FILTER_MESSAGE_HEADER
{
    ULONG     ReplyLength;
    ULONGLONG MessageId;
} FILTER_MESSAGE_HEADER, *PFILTER_MESSAGE_HEADER;

typedef struct _FILTER_REPLY_HEADER
{
    NTSTATUS  Status;
    ULONGLONG MessageId;
} FILTER_REPLY_HEADER, *PFILTER_REPLY_HEADER;


#ifdef __cplusplus
}
#endif


#endif
