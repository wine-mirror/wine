/*
 *	self-registerable dll helper functions
 *
 * Copyright (C) 2002 John K. Hohm
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

struct regsvr_entry {
    int     parent;		/* entry number, or predefined HKEY_ */
    int     unreg_del;		/* recursively delete key on unregister? */
    LPCWSTR name;		/* key or value name */
    LPCWSTR value;		/* NULL for key */
};

HRESULT regsvr_register(struct regsvr_entry const *entries, size_t count);
HRESULT regsvr_unregister(struct regsvr_entry const *entries, size_t count);
