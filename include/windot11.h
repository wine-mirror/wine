/*
 * Copyright (C) 2021 Alex Henrie
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

#ifndef _WINDOT11_H
#define _WINDOT11_H

typedef enum _DOT11_PHY_TYPE
{
    dot11_phy_type_unknown = 0x00,
    dot11_phy_type_any = 0x00,
    dot11_phy_type_fhss = 0x01,
    dot11_phy_type_dsss = 0x02,
    dot11_phy_type_irbaseband = 0x03,
    dot11_phy_type_ofdm = 0x04,
    dot11_phy_type_hrdsss = 0x05,
    dot11_phy_type_erp = 0x06,
    dot11_phy_type_ht = 0x07,
    dot11_phy_type_vht = 0x08,
    dot11_phy_type_IHV_start = 0x80000000,
    dot11_phy_type_IHV_end = 0xFFFFFFFF
} DOT11_PHY_TYPE, *PDOT11_PHY_TYPE;

typedef UCHAR DOT11_MAC_ADDRESS[6];
typedef DOT11_MAC_ADDRESS *PDOT11_MAC_ADDRESS;

#endif /* _WINDOT11_H */
