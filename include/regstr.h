/*
 * Win32 registry string defines (see also winnt.h)
 *
 * Copyright (C) 2000 Andreas Mohr
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

#ifndef _INC_REGSTR
#define _INC_REGSTR


#define REGSTR_PATH_UNINSTALL			TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall")
#define REGSTR_VAL_MAX_HCID_LEN 1024

/* DisplayName <= 32 chars in Windows (otherwise not displayed for uninstall) */
#define REGSTR_VAL_UNINSTALLER_DISPLAYNAME	TEXT("DisplayName")
/* UninstallString <= 63 chars in Windows (otherwise problems) */
#define REGSTR_VAL_UNINSTALLER_COMMANDLINE	TEXT("UninstallString")

#define CONFIGFLAG_DISABLED             0x00000001
#define CONFIGFLAG_REMOVED              0x00000002
#define CONFIGFLAG_MANUAL_INSTALL       0x00000004
#define CONFIGFLAG_IGNORE_BOOT_LC       0x00000008
#define CONFIGFLAG_NET_BOOT             0x00000010
#define CONFIGFLAG_REINSTALL            0x00000020
#define CONFIGFLAG_FAILEDINSTALL        0x00000040
#define CONFIGFLAG_CANTSTOPACHILD       0x00000080
#define CONFIGFLAG_OKREMOVEROM          0x00000100
#define CONFIGFLAG_NOREMOVEEXIT         0x00000200
#define CONFIGFLAG_FINISH_INSTALL       0x00000400
#define CONFIGFLAG_NEEDS_FORCED_CONFIG  0x00000800
#define CONFIGFLAG_NETBOOT_CARD         0x00001000
#define CONFIGFLAG_PARTIAL_LOG_CONF     0x00002000
#define CONFIGFLAG_SUPPRESS_SURPRISE    0x00004000
#define CONFIGFLAG_VERIFY_HARDWARE      0x00008000
#define CONFIGFLAG_FINISHINSTALL_UI     0x00010000
#define CONFIGFLAG_FINISHINSTALL_ACTION 0x00020000
#define CONFIGFLAG_BOOT_DEVICE          0x00040000
#define CONFIGFLAG_NEEDS_CLASS_CONFIG   0x00080000

#endif  /* _INC_REGSTR_H */
