/*
 * Copyright 1999 Bertho Stultiens
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

#define OEMRESOURCE
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winnls.h>
#include <dlgs.h>

#define MDI_IDC_LISTBOX         100
#define IDS_MDI_MOREWINDOWS     13
#define MSGBOX_IDICON           stc1
#define MSGBOX_IDTEXT           0xffff
#define IDS_ERROR               2
#define IDS_STRING_OK           800
#define IDS_STRING_CANCEL       801
#define IDS_STRING_ABORT        802
#define IDS_STRING_RETRY        803
#define IDS_STRING_IGNORE       804
#define IDS_STRING_YES          805
#define IDS_STRING_NO           806
#define IDS_STRING_CLOSE        807
#define IDS_STRING_HELP         808
#define IDS_STRING_TRYAGAIN     809
#define IDS_STRING_CONTINUE     810
