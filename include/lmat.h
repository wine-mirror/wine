/*
 * Schedule Service Functions
 *
 * Copyright (C) 2011 Louis Lenders
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

#ifndef __WINE_LMAT_H
#define __WINE_LMAT_H

#ifdef __cplusplus
extern "C" {
#endif

#define JOB_RUN_PERIODICALLY 0x01
#define JOB_EXEC_ERROR       0x02
#define JOB_RUNS_TODAY       0x04
#define JOB_ADD_CURRENT_DATE 0x08
#define JOB_NONINTERACTIVE   0x10

#define JOB_INPUT_FLAGS (JOB_RUN_PERIODICALLY | JOB_ADD_CURRENT_DATE | JOB_NONINTERACTIVE)
#define JOB_OUTPUT_FLAGS (JOB_RUN_PERIODICALLY | JOB_EXEC_ERROR | JOB_RUNS_TODAY | JOB_NONINTERACTIVE)

typedef struct _AT_INFO
{
    DWORD_PTR JobTime;
    DWORD DaysOfMonth;
    UCHAR DaysOfWeek;
    UCHAR Flags;
    LPWSTR Command;
} AT_INFO, *PAT_INFO, *LPAT_INFO;

typedef struct _AT_ENUM
{
    DWORD JobId;
    DWORD_PTR JobTime;
    DWORD DaysOfMonth;
    UCHAR DaysOfWeek;
    UCHAR Flags;
    LPWSTR Command;
} AT_ENUM, *PAT_ENUM, *LPAT_ENUM;

NET_API_STATUS WINAPI NetScheduleJobAdd(LPCWSTR,LPBYTE,LPDWORD);
NET_API_STATUS WINAPI NetScheduleJobDel(LPCWSTR,DWORD,DWORD);
NET_API_STATUS WINAPI NetScheduleJobEnum(LPCWSTR,LPBYTE*,DWORD,LPDWORD,LPDWORD,LPDWORD);

#ifdef __cplusplus
}
#endif

#endif /* __WINE_LMAT_H */
