/*
 * Copyright (c) 2011 Akihiro Sagawa
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

#include <windef.h>

/* these are in the order of the fsCsb[0] bits */
#define IDS_FIRST_SCRIPT     16
#define IDS_WESTERN          (IDS_FIRST_SCRIPT + 0)
#define IDS_CENTRAL_EUROPEAN (IDS_FIRST_SCRIPT + 1)
#define IDS_CYRILLIC         (IDS_FIRST_SCRIPT + 2)
#define IDS_GREEK            (IDS_FIRST_SCRIPT + 3)
#define IDS_TURKISH          (IDS_FIRST_SCRIPT + 4)
#define IDS_HEBREW           (IDS_FIRST_SCRIPT + 5)
#define IDS_ARABIC           (IDS_FIRST_SCRIPT + 6)
#define IDS_BALTIC           (IDS_FIRST_SCRIPT + 7)
#define IDS_VIETNAMESE       (IDS_FIRST_SCRIPT + 8)
#define IDS_THAI             (IDS_FIRST_SCRIPT + 16)
#define IDS_JAPANESE         (IDS_FIRST_SCRIPT + 17)
#define IDS_CHINESE_GB2312   (IDS_FIRST_SCRIPT + 18)
#define IDS_HANGUL           (IDS_FIRST_SCRIPT + 19)
#define IDS_CHINESE_BIG5     (IDS_FIRST_SCRIPT + 20)
#define IDS_HANGUL_JOHAB     (IDS_FIRST_SCRIPT + 21)
#define IDS_SYMBOL           (IDS_FIRST_SCRIPT + 31)
#define IDS_OEM_DOS          (IDS_FIRST_SCRIPT + 32)
#define IDS_OTHER            (IDS_FIRST_SCRIPT + 33)
