/* DirectShow private capture header (QCAP.DLL)
 *
 * Copyright 2005 Maarten Lankhorst
 *
 * This file contains the (internal) driver registration functions,
 * driver enumeration APIs and DirectDraw creation functions.
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

#ifndef __QCAP_CAPTURE_H__
#define __QCAP_CAPTURE_H__

struct _Capture;
typedef struct _Capture Capture;

Capture *qcap_driver_init(IPin*,USHORT);
HRESULT qcap_driver_destroy(Capture*);
HRESULT qcap_driver_set_format(Capture*,AM_MEDIA_TYPE*);
HRESULT qcap_driver_get_format(const Capture*,AM_MEDIA_TYPE**);
HRESULT qcap_driver_get_prop_range(Capture*,long,long*,long*,long*,long*,long*);
HRESULT qcap_driver_get_prop(Capture*,long,long*,long*);
HRESULT qcap_driver_set_prop(Capture*,long,long,long);
HRESULT qcap_driver_run(Capture*,FILTER_STATE*);
HRESULT qcap_driver_pause(Capture*,FILTER_STATE*);
HRESULT qcap_driver_stop(Capture*,FILTER_STATE*);

#endif /* __QCAP_CAPTURE_H__ */
