/*
 * Copyright (C) 2002 Mike McCormack
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

#ifndef __WINE_MSIQUERY_H
#define __WINE_MSIQUERY_H

#include <msi.h>

typedef enum tagMSICONDITION
{
    MSICONDITION_FALSE = 0,
    MSICONDITION_TRUE  = 1,
    MSICONDITION_NONE  = 2,
    MSICONDITION_ERROR = 3,
} MSICONDITION;

#define MSI_NULL_INTEGER 0x80000000

typedef enum tagMSICOLINFO
{
    MSICOLINFO_NAMES = 0,
    MSICOLINFO_TYPES = 1
} MSICOLINFO;

typedef enum tagMSIMODIFY
{
    MSIMODIFY_REFRESH = 0,
    MSIMODIFY_INSERT = 1,
    MSIMODIFY_UPDATE = 2,
    MSIMODIFY_ASSIGN = 3,
    MSIMODIFY_REPLACE = 4,
    MSIMODIFY_MERGE = 5,
    MSIMODIFY_DELETE = 6,
    MSIMODIFY_INSERT_TEMPORARY = 7,
    MSIMODIFY_VALIDATE = 8,
    MSIMODIFY_VALIDATE_NEW = 9,
    MSIMODIFY_VALIDATE_FIELD = 10,
    MSIMODIFY_VALIDATE_DELETE = 11
} MSIMODIFY;

#define MSI_NULL_INTEGER 0x80000000

/* view manipulation */
UINT WINAPI MsiViewFetch(MSIHANDLE,MSIHANDLE*);
UINT WINAPI MsiViewExecute(MSIHANDLE,MSIHANDLE);
UINT WINAPI MsiViewClose(MSIHANDLE);
UINT WINAPI MsiDatabaseOpenViewA(MSIHANDLE,LPCSTR,MSIHANDLE*);
UINT WINAPI MsiDatabaseOpenViewW(MSIHANDLE,LPCWSTR,MSIHANDLE*);

/* record manipulation */
MSIHANDLE WINAPI MsiCreateRecord(unsigned int);
UINT WINAPI MsiRecordClearData(MSIHANDLE);
UINT WINAPI MsiRecordSetInteger(MSIHANDLE,unsigned int,int);
UINT WINAPI MsiRecordSetStringA(MSIHANDLE,unsigned int,LPCSTR);
UINT WINAPI MsiRecordSetStringW(MSIHANDLE,unsigned int,LPCWSTR);
UINT WINAPI MsiRecordGetStringA(MSIHANDLE,unsigned int,LPSTR,DWORD*);
UINT WINAPI MsiRecordGetStringW(MSIHANDLE,unsigned int,LPWSTR,DWORD*);
UINT WINAPI MsiRecordGetFieldCount(MSIHANDLE);
int WINAPI MsiRecordGetInteger(MSIHANDLE,unsigned int);
UINT WINAPI MsiRecordDataSize(MSIHANDLE,unsigned int);
BOOL WINAPI MsiRecordIsNull(MSIHANDLE,unsigned int);
UINT WINAPI MsiFormatRecordA(MSIHANDLE,MSIHANDLE,LPSTR,DWORD*);
UINT WINAPI MsiFormatRecordW(MSIHANDLE,MSIHANDLE,LPWSTR,DWORD*);
UINT WINAPI MsiRecordSetStreamA(MSIHANDLE,unsigned int,LPCSTR);
UINT WINAPI MsiRecordSetStreamW(MSIHANDLE,unsigned int,LPCWSTR);
UINT WINAPI MsiRecordReadStream(MSIHANDLE,unsigned int,char*,DWORD *);

UINT WINAPI MsiDatabaseGetPrimaryKeysA(MSIHANDLE,LPCSTR,MSIHANDLE*);
UINT WINAPI MsiDatabaseGetPrimaryKeysW(MSIHANDLE,LPCWSTR,MSIHANDLE*);

/* installing */
UINT WINAPI MsiDoActionA(MSIHANDLE,LPCSTR );
UINT WINAPI MsiDoActionW(MSIHANDLE,LPCWSTR );

/* database transforms */
UINT WINAPI MsiDatabaseApplyTransformA(MSIHANDLE,LPCSTR,int);
UINT WINAPI MsiDatabaseApplyTransformW(MSIHANDLE,LPCWSTR,int);
UINT WINAPI MsiDatabaseGenerateTransformA(MSIHANDLE,MSIHANDLE,LPCSTR,int,int);
UINT WINAPI MsiDatabaseGenerateTransformW(MSIHANDLE,MSIHANDLE,LPCWSTR,int,int);

UINT WINAPI MsiDatabaseCommit(MSIHANDLE);

#endif /* __WINE_MSIQUERY_H */
