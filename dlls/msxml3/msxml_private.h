/*
 *    MSXML Class Factory
 *
 * Copyright 2005 Mike McCormack
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

#ifndef __MSXML_PRIVATE__
#define __MSXML_PRIVATE__

#include "xmldom.h"

#ifdef HAVE_LIBXML2

#include <libxml/parser.h>

extern HRESULT DOMElement_create( IXMLDOMElement** DOMElement, xmlDocPtr xmldoc );
extern HRESULT NodeMap_create(IXMLDOMNamedNodeMap** DomNamedNodeMap, xmlDocPtr xmldoc, xmlNodePtr node );

#endif

extern HRESULT DOMDocument_create( IUnknown *pUnkOuter, LPVOID *ppObj );

#endif /* __MSXML_PRIVATE__ */
