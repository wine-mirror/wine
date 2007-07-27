/*
 * Copyright (C) 2007 Tim Schwartz
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
#ifndef _LMUSE_H
#define _LMUSE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _USE_INFO_1 {
  LMSTR ui1_local, ui1_remote, ui1_password;
  DWORD ui1_status, ui1_asg_type, ui1_refcount, ui1_usecount;
} USE_INFO_1, *PUSE_INFO_1, *LPUSE_INFO_1;

typedef struct _USE_INFO_2 {
  LMSTR ui2_local, ui2_remote, ui2_password;
  DWORD ui2_status, ui2_asg_type, ui2_refcount, ui2_usecount;
  LPWSTR ui2_username;
  LMSTR ui2_domainname;
} USE_INFO_2, *PUSE_INFO_2, *LPUSE_INFO_2;

#ifdef __cplusplus
}
#endif

#endif /* ndef _LMUSE_H */
