/*
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
#ifndef __XDG_H__
#define __XDG_H__

/*
 * XDG directories access
 */
#define XDG_DATA_HOME 0
#define XDG_CONFIG_HOME 1
#define XDG_DATA_DIRS 2
#define XDG_CONFIG_DIRS 3
#define XDG_CACHE_HOME 4

const char *XDG_GetPath(int path_id);
char *XDG_BuildPath(int root_id, const char *subpath);
int XDG_MakeDirs(const char *path);

#define XDG_URLENCODE 0x1
BOOL XDG_WriteDesktopStringEntry(int fd, const char *keyName, DWORD dwFlags, const char *value);

/* implemented in trash.c */
BOOL TRASH_CanTrashFile(LPCWSTR wszPath);
BOOL TRASH_TrashFile(LPCWSTR wszPath);

#endif /* ndef __XDG_H__ */
