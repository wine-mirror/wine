/*
 * Copyright 2001 Andreas Mohr
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

#ifndef __SETUPAPI_PRIVATE_H
#define __SETUPAPI_PRIVATE_H

#include "wine/windef16.h"

#define COPYFILEDLGORD	1000
#define SOURCESTRORD	500
#define DESTSTRORD	501
#define PROGRESSORD	502


#define REG_INSTALLEDFILES "System\\CurrentControlSet\\Control\\InstalledFiles"
#define REGPART_RENAME "\\Rename"
#define REG_VERSIONCONFLICT "Software\\Microsoft\\VersionConflictManager"

typedef struct tagLDD_LIST {
        LPLOGDISKDESC pldd;
        struct tagLDD_LIST *next;
} LDD_LIST;

#define INIT_LDD(ldd, LDID) \
  do { \
    memset(&(ldd), 0, sizeof(LOGDISKDESC_S)); \
   (ldd).cbSize = sizeof(LOGDISKDESC_S); \
   ldd.ldid = LDID; \
  } while(0)

typedef struct {
    HINF16 hInf;
    HFILE hInfFile;
    LPSTR lpInfFileName;
} INF_FILE;

extern INF_FILE *InfList;
extern WORD InfNumEntries;

extern LPCSTR IP_GetFileName(HINF16 hInf);

#endif /* __SETUPAPI_PRIVATE_H */
