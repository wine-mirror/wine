/*
 * This test program was copied from the former file documentation/cdrom-label
 *
 * Copyright 1998 Petter Reinholdtsen
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

#include <windows.h>
#include <stdio.h>
#include <string.h> /* for strcat() */

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpszCmdLine, int nCmdShow)
{
    char  drive, root[]="C:\\", label[1002], fsname[1002];
    DWORD serial, flags, filenamelen, labellen = 1000, fsnamelen = 1000;

    printf("Drive Serial     Flags      Filename-Length "
           "Label                 Fsname\n");
    for (drive = 'A'; drive <= 'Z'; drive++)
    {
        root[0] = drive;
        if (GetVolumeInformation(root,label,labellen,&serial,
                                  &filenamelen,&flags,fsname,fsnamelen))
        {
            strcat(label,"\""); strcat (fsname,"\"");
            printf("%c:\\   0x%08lx 0x%08lx %15ld \"%-20s \"%-20s\n",
                   drive, (long) serial, (long) flags, (long) filenamelen,
                   label, fsname);
        }
    }
    return 0;
}
