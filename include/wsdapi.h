/*
 * Copyright 2017 Owen Rudge for CodeWeavers
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

#ifndef WSDAPI_H
#define WSDAPI_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct IWSDMessageParameters IWSDMessageParameters;

typedef struct _WSD_URI_LIST WSD_URI_LIST;
typedef struct _WSD_NAME_LIST WSD_NAME_LIST;

typedef struct _WSDXML_NAME WSDXML_NAME;
typedef struct _WSDXML_ELEMENT WSDXML_ELEMENT;
typedef struct _WSDXML_NODE WSDXML_NODE;
typedef struct _WSDXML_ATTRIBUTE WSDXML_ATTRIBUTE;
typedef struct _WSDXML_PREFIX_MAPPING WSDXML_PREFIX_MAPPING;
typedef struct _WSDXML_ELEMENT_LIST WSDXML_ELEMENT_LIST;
typedef struct _WSDXML_TYPE WSDXML_TYPE;

#ifdef __cplusplus
}
#endif

#include <wsdtypes.h>
#include <wsdbase.h>
#include <wsdxmldom.h>
#include <wsdxml.h>
#include <wsddisco.h>

#endif
