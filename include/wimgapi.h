/*
 * Copyright (C) 2015 Austin English
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

#ifndef _WIMGAPI_H_
#define _WIMGAPI_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _WIM_MOUNT_LIST
{
    WCHAR WimPath[MAX_PATH];
    WCHAR MountPath[MAX_PATH];
    DWORD ImageIndex;
    BOOL MountedForRW;
} WIM_MOUNT_LIST, *PWIM_MOUNT_LIST, *LPWIM_MOUNT_LIST;

#ifdef __cplusplus
}
#endif

#endif /* _WIMGAPI_H_ */
