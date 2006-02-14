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

#ifdef HAVE_LIBXML2

#ifdef HAVE_LIBXML_PARSER_H
#include <libxml/parser.h>
#endif

/* constructors */
extern IUnknown         *create_domdoc( void );
extern IUnknown         *create_xmldoc( void );
extern IXMLDOMNode      *create_node( xmlNodePtr node );
extern IUnknown         *create_basic_node( xmlNodePtr node, IUnknown *pUnkOuter );
extern IUnknown         *create_attribute( xmlNodePtr attribute );
extern IUnknown         *create_element( xmlNodePtr element );
extern IUnknown         *create_text( xmlNodePtr text );
extern IXMLDOMNodeList  *create_nodelist( xmlNodePtr node );
extern IXMLDOMNamedNodeMap *create_nodemap( IXMLDOMNode *node );
extern IXMLDOMNodeList  *create_filtered_nodelist( xmlNodePtr, const xmlChar *, BOOL );

extern void attach_xmlnode( IXMLDOMNode *node, xmlNodePtr xmlnode );

/* data accessors */
xmlNodePtr xmlNodePtr_from_domnode( IXMLDOMNode *iface, xmlElementType type );

/* helpers */
extern xmlChar *xmlChar_from_wchar( LPWSTR str );
extern BSTR bstr_from_xmlChar( const xmlChar *buf );

extern LONG xmldoc_add_ref( xmlDocPtr doc );
extern LONG xmldoc_release( xmlDocPtr doc );
#endif

extern IXMLDOMParseError *create_parseError( LONG code, BSTR url, BSTR reason, BSTR srcText,
                                             LONG line, LONG linepos, LONG filepos );
extern HRESULT DOMDocument_create( IUnknown *pUnkOuter, LPVOID *ppObj );

#endif /* __MSXML_PRIVATE__ */
