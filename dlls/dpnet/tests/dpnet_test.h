/*
 * Copyright 2016 Alistair Leslie-Hughes
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
#ifndef __DPNET_PRIVATE_H__
#define __DPNET_PRIVATE_H__

enum firewall_op
{
    APP_ADD,
    APP_REMOVE
};

extern BOOL is_firewall_enabled(void);
extern BOOL is_process_elevated(void);
extern HRESULT set_firewall( enum firewall_op op );
extern BOOL is_stub_dll(const char *filename);

#endif
