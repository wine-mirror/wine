/*
 * Copyright (C) 2023 Mohamad Al-Jaf
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

#include <stdarg.h>

typedef struct {
    size_t id;
} locale_id;

/* ?id@?$codecvt@_SDU_Mbstatet@@@std@@2V0locale@2@A */
locale_id codecvt_char16_id = {0};
/* ?id@?$codecvt@_S_QU_Mbstatet@@@std@@2V0locale@2@A */
locale_id codecvt_char16_char8_id = {0};
/* ?id@?$codecvt@_UDU_Mbstatet@@@std@@2V0locale@2@A */
locale_id codecvt_char32_id = {0};
/* ?id@?$codecvt@_U_QU_Mbstatet@@@std@@2V0locale@2@A */
locale_id codecvt_char32_char8_id = {0};
