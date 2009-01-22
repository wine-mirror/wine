/* A few helpful macros for implementing COM objects.
 *
 * Copyright 2000 TransGaming Technologies Inc.
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

#ifndef _DDCOMIMPL_H_
#define _DDCOMIMPL_H_

#include <stddef.h>

/* Given an interface pointer, returns the implementation pointer. */
#define ICOM_OBJECT(impltype, ifacename, ifaceptr)		\
	(impltype*)((ifaceptr) == NULL ? NULL			\
		  : (char*)(ifaceptr) - offsetof(impltype, ifacename##_vtbl))

#define COM_INTERFACE_CAST(impltype, ifnamefrom, ifnameto, ifaceptr) \
    ((ifaceptr) ? (ifnameto *)&(((impltype *)((char *)(ifaceptr) \
    - offsetof(impltype, ifnamefrom##_vtbl)))->ifnameto##_vtbl) : NULL)
#endif /* _DDCOMIMPL_H_ */
