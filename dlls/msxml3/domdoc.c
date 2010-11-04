/*
 *    DOM Document implementation
 *
 * Copyright 2005 Mike McCormack
 * Copyright 2010 Adam Martinson for CodeWeavers
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

#define COBJMACROS
#define NONAMELESSUNION

#include "config.h"

#include <stdarg.h>
#include <assert.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "ole2.h"
#include "olectl.h"
#include "msxml6.h"
#include "wininet.h"
#include "winreg.h"
#include "shlwapi.h"
#include "ocidl.h"
#include "objsafe.h"
#include "dispex.h"

#include "wine/debug.h"
#include "wine/list.h"

#include "msxml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

#ifdef HAVE_LIBXML2

#include <libxml/xpathInternals.h>
#include <libxml/xmlsave.h>
#include <libxml/SAX2.h>
#include <libxml/parserInternals.h>

/* not defined in older versions */
#define XML_SAVE_FORMAT     1
#define XML_SAVE_NO_DECL    2
#define XML_SAVE_NO_EMPTY   4
#define XML_SAVE_NO_XHTML   8
#define XML_SAVE_XHTML     16
#define XML_SAVE_AS_XML    32
#define XML_SAVE_AS_HTML   64

static const WCHAR PropertySelectionLanguageW[] = {'S','e','l','e','c','t','i','o','n','L','a','n','g','u','a','g','e',0};
static const WCHAR PropertySelectionNamespacesW[] = {'S','e','l','e','c','t','i','o','n','N','a','m','e','s','p','a','c','e','s',0};
static const WCHAR PropertyProhibitDTDW[] = {'P','r','o','h','i','b','i','t','D','T','D',0};
static const WCHAR PropertyNewParserW[] = {'N','e','w','P','a','r','s','e','r',0};
static const WCHAR PropValueXPathW[] = {'X','P','a','t','h',0};
static const WCHAR PropValueXSLPatternW[] = {'X','S','L','P','a','t','t','e','r','n',0};

/* Data used by domdoc_getProperty()/domdoc_setProperty().
 * We need to preserve this when reloading a document,
 * and also need access to it from the libxml backend. */
typedef struct _domdoc_properties {
    VARIANT_BOOL preserving;
    struct list selectNsList;
    xmlChar const* selectNsStr;
    LONG selectNsStr_len;
    BOOL XPath;
} domdoc_properties;

typedef struct ConnectionPoint ConnectionPoint;
typedef struct domdoc domdoc;

struct ConnectionPoint
{
    const IConnectionPointVtbl  *lpVtblConnectionPoint;
    const IID *iid;

    ConnectionPoint *next;
    IConnectionPointContainer *container;
    domdoc *doc;

    union
    {
        IUnknown *unk;
        IDispatch *disp;
        IPropertyNotifySink *propnotif;
    } *sinks;
    DWORD sinks_size;
};

struct domdoc
{
    xmlnode node;
    const struct IXMLDOMDocument3Vtbl          *lpVtbl;
    const struct IPersistStreamInitVtbl        *lpvtblIPersistStreamInit;
    const struct IObjectWithSiteVtbl           *lpvtblIObjectWithSite;
    const struct IObjectSafetyVtbl             *lpvtblIObjectSafety;
    const struct ISupportErrorInfoVtbl         *lpvtblISupportErrorInfo;
    const struct IConnectionPointContainerVtbl *lpVtblConnectionPointContainer;
    LONG ref;
    VARIANT_BOOL async;
    VARIANT_BOOL validating;
    VARIANT_BOOL resolving;
    domdoc_properties* properties;
    IXMLDOMSchemaCollection2* schema;
    bsc_t *bsc;
    HRESULT error;

    /* IPersistStream */
    IStream *stream;

    /* IObjectWithSite*/
    IUnknown *site;

    /* IObjectSafety */
    DWORD safeopt;

    /* connection list */
    ConnectionPoint *cp_list;
    ConnectionPoint cp_domdocevents;
    ConnectionPoint cp_propnotif;
    ConnectionPoint cp_dispatch;
};

static inline ConnectionPoint *impl_from_IConnectionPoint(IConnectionPoint *iface)
{
    return (ConnectionPoint *)((char*)iface - FIELD_OFFSET(ConnectionPoint, lpVtblConnectionPoint));
}

/*
  In native windows, the whole lifetime management of XMLDOMNodes is
  managed automatically using reference counts. Wine emulates that by
  maintaining a reference count to the document that is increased for
  each IXMLDOMNode pointer passed out for this document. If all these
  pointers are gone, the document is unreachable and gets freed, that
  is, all nodes in the tree of the document get freed.

  You are able to create nodes that are associated to a document (in
  fact, in msxml's XMLDOM model, all nodes are associated to a document),
  but not in the tree of that document, for example using the createFoo
  functions from IXMLDOMDocument. These nodes do not get cleaned up
  by libxml, so we have to do it ourselves.

  To catch these nodes, a list of "orphan nodes" is introduced.
  It contains pointers to all roots of node trees that are
  associated with the document without being part of the document
  tree. All nodes with parent==NULL (except for the document root nodes)
  should be in the orphan node list of their document. All orphan nodes
  get freed together with the document itself.
 */

typedef struct _xmldoc_priv {
    LONG refs;
    struct list orphans;
    domdoc_properties* properties;
} xmldoc_priv;

typedef struct _orphan_entry {
    struct list entry;
    xmlNode * node;
} orphan_entry;

typedef struct _select_ns_entry {
    struct list entry;
    xmlChar const* prefix;
    xmlChar prefix_end;
    xmlChar const* href;
    xmlChar href_end;
} select_ns_entry;

static inline xmldoc_priv * priv_from_xmlDocPtr(const xmlDocPtr doc)
{
    return doc->_private;
}

static inline domdoc_properties * properties_from_xmlDocPtr(xmlDocPtr doc)
{
    return priv_from_xmlDocPtr(doc)->properties;
}

BOOL is_xpathmode(const xmlDocPtr doc)
{
    return properties_from_xmlDocPtr(doc)->XPath;
}

int registerNamespaces(xmlXPathContextPtr ctxt)
{
    int n = 0;
    const select_ns_entry* ns = NULL;
    const struct list* pNsList = &properties_from_xmlDocPtr(ctxt->doc)->selectNsList;

    TRACE("(%p)\n", ctxt);

    LIST_FOR_EACH_ENTRY( ns, pNsList, select_ns_entry, entry )
    {
        xmlXPathRegisterNs(ctxt, ns->prefix, ns->href);
        ++n;
    }

    return n;
}

static inline void clear_selectNsList(struct list* pNsList)
{
    select_ns_entry *ns, *ns2;
    LIST_FOR_EACH_ENTRY_SAFE( ns, ns2, pNsList, select_ns_entry, entry )
    {
        heap_free( ns );
    }
    list_init(pNsList);
}

static xmldoc_priv * create_priv(void)
{
    xmldoc_priv *priv;
    priv = heap_alloc( sizeof (*priv) );

    if (priv)
    {
        priv->refs = 0;
        list_init( &priv->orphans );
        priv->properties = NULL;
    }

    return priv;
}

static domdoc_properties * create_properties(const GUID *clsid)
{
    domdoc_properties *properties = heap_alloc(sizeof(domdoc_properties));

    list_init( &properties->selectNsList );
    properties->preserving = VARIANT_FALSE;
    properties->selectNsStr = heap_alloc_zero(sizeof(xmlChar));
    properties->selectNsStr_len = 0;
    properties->XPath = FALSE;

    /* properties that are dependent on object versions */
    if (IsEqualCLSID( clsid, &CLSID_DOMDocument40 ) ||
        IsEqualCLSID( clsid, &CLSID_DOMDocument60 ))
    {
        properties->XPath = TRUE;
    }

    return properties;
}

static domdoc_properties* copy_properties(domdoc_properties const* properties)
{
    domdoc_properties* pcopy = heap_alloc(sizeof(domdoc_properties));
    select_ns_entry const* ns = NULL;
    select_ns_entry* new_ns = NULL;
    int len = (properties->selectNsStr_len+1)*sizeof(xmlChar);
    ptrdiff_t offset;

    if (pcopy)
    {
        pcopy->preserving = properties->preserving;
        pcopy->XPath = properties->XPath;
        pcopy->selectNsStr_len = properties->selectNsStr_len;
        list_init( &pcopy->selectNsList );
        pcopy->selectNsStr = heap_alloc(len);
        memcpy((xmlChar*)pcopy->selectNsStr, properties->selectNsStr, len);
        offset = pcopy->selectNsStr - properties->selectNsStr;

        LIST_FOR_EACH_ENTRY( ns, (&properties->selectNsList), select_ns_entry, entry )
        {
            new_ns = heap_alloc(sizeof(select_ns_entry));
            memcpy(new_ns, ns, sizeof(select_ns_entry));
            new_ns->href += offset;
            new_ns->prefix += offset;
            list_add_tail(&pcopy->selectNsList, &new_ns->entry);
        }

    }

    return pcopy;
}

static void free_properties(domdoc_properties* properties)
{
    if (properties)
    {
        clear_selectNsList(&properties->selectNsList);
        heap_free((xmlChar*)properties->selectNsStr);
        heap_free(properties);
    }
}

/* links a "<?xml" node as a first child */
void xmldoc_link_xmldecl(xmlDocPtr doc, xmlNodePtr node)
{
    assert(doc != NULL);
    if (doc->standalone != -1) xmlAddPrevSibling( doc->children, node );
}

/* unlinks a first "<?xml" child if it was created */
xmlNodePtr xmldoc_unlink_xmldecl(xmlDocPtr doc)
{
    xmlNodePtr node;

    assert(doc != NULL);

    if (doc->standalone != -1)
    {
        node = doc->children;
        xmlUnlinkNode( node );
    }
    else
        node = NULL;

    return node;
}

BOOL is_preserving_whitespace(xmlNodePtr node)
{
    domdoc_properties* properties = NULL;
    /* during parsing the xmlDoc._private stuff is not there */
    if (priv_from_xmlDocPtr(node->doc))
        properties = properties_from_xmlDocPtr(node->doc);
    return ((properties && properties->preserving == VARIANT_TRUE) ||
            xmlNodeGetSpacePreserve(node) == 1);
}

static inline BOOL strn_isspace(xmlChar const* str, int len)
{
    for (; str && len > 0 && *str; ++str, --len)
        if (!isspace(*str))
            break;

    return len == 0;
}

static void sax_characters(void *ctx, const xmlChar *ch, int len)
{
    xmlParserCtxtPtr pctx;
    domdoc const* This;

    pctx = (xmlParserCtxtPtr) ctx;
    This = (domdoc const*) pctx->_private;

    /* during domdoc_loadXML() the xmlDocPtr->_private data is not available */
    if (!This->properties->preserving &&
        !is_preserving_whitespace(pctx->node) &&
        strn_isspace(ch, len))
        return;

    xmlSAX2Characters(ctx, ch, len);
}

static void LIBXML2_LOG_CALLBACK sax_error(void* ctx, char const* msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    LIBXML2_CALLBACK_ERR(doparse, msg, ap);
    va_end(ap);
}

static void LIBXML2_LOG_CALLBACK sax_warning(void* ctx, char const* msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    LIBXML2_CALLBACK_WARN(doparse, msg, ap);
    va_end(ap);
}

static void sax_serror(void* ctx, xmlErrorPtr err)
{
    LIBXML2_CALLBACK_SERROR(doparse, err);
}

static xmlDocPtr doparse(domdoc* This, char *ptr, int len, xmlChar const* encoding)
{
    xmlDocPtr doc = NULL;
    xmlParserCtxtPtr pctx;
    static xmlSAXHandler sax_handler = {
        xmlSAX2InternalSubset,          /* internalSubset */
        xmlSAX2IsStandalone,            /* isStandalone */
        xmlSAX2HasInternalSubset,       /* hasInternalSubset */
        xmlSAX2HasExternalSubset,       /* hasExternalSubset */
        xmlSAX2ResolveEntity,           /* resolveEntity */
        xmlSAX2GetEntity,               /* getEntity */
        xmlSAX2EntityDecl,              /* entityDecl */
        xmlSAX2NotationDecl,            /* notationDecl */
        xmlSAX2AttributeDecl,           /* attributeDecl */
        xmlSAX2ElementDecl,             /* elementDecl */
        xmlSAX2UnparsedEntityDecl,      /* unparsedEntityDecl */
        xmlSAX2SetDocumentLocator,      /* setDocumentLocator */
        xmlSAX2StartDocument,           /* startDocument */
        xmlSAX2EndDocument,             /* endDocument */
        xmlSAX2StartElement,            /* startElement */
        xmlSAX2EndElement,              /* endElement */
        xmlSAX2Reference,               /* reference */
        sax_characters,                 /* characters */
        sax_characters,                 /* ignorableWhitespace */
        xmlSAX2ProcessingInstruction,   /* processingInstruction */
        xmlSAX2Comment,                 /* comment */
        sax_warning,                    /* warning */
        sax_error,                      /* error */
        sax_error,                      /* fatalError */
        xmlSAX2GetParameterEntity,      /* getParameterEntity */
        xmlSAX2CDataBlock,              /* cdataBlock */
        xmlSAX2ExternalSubset,          /* externalSubset */
        0,                              /* initialized */
        NULL,                           /* _private */
        xmlSAX2StartElementNs,          /* startElementNs */
        xmlSAX2EndElementNs,            /* endElementNs */
        sax_serror                      /* serror */
    };
    xmlInitParser();

    pctx = xmlCreateMemoryParserCtxt(ptr, len);
    if (!pctx)
    {
        ERR("Failed to create parser context\n");
        return NULL;
    }

    if (pctx->sax) xmlFree(pctx->sax);
    pctx->sax = &sax_handler;
    pctx->_private = This;
    pctx->recovery = 0;
    pctx->encoding = xmlStrdup(encoding);
    xmlParseDocument(pctx);

    if (pctx->wellFormed)
    {
        doc = pctx->myDoc;
    }
    else
    {
       xmlFreeDoc(pctx->myDoc);
       pctx->myDoc = NULL;
    }
    pctx->sax = NULL;
    xmlFreeParserCtxt(pctx);

    /* TODO: put this in one of the SAX callbacks */
    /* create first child as a <?xml...?> */
    if (doc && doc->standalone != -1)
    {
        xmlNodePtr node;
        char buff[30];
        xmlChar *xmlbuff = (xmlChar*)buff;

        node = xmlNewDocPI( doc, (xmlChar*)"xml", NULL );

        /* version attribute can't be omitted */
        sprintf(buff, "version=\"%s\"", doc->version ? (char*)doc->version : "1.0");
        xmlNodeAddContent( node, xmlbuff );

        if (doc->encoding)
        {
            sprintf(buff, " encoding=\"%s\"", doc->encoding);
            xmlNodeAddContent( node, xmlbuff );
        }

        if (doc->standalone != -2)
        {
            sprintf(buff, " standalone=\"%s\"", doc->standalone == 0 ? "no" : "yes");
            xmlNodeAddContent( node, xmlbuff );
        }

        xmldoc_link_xmldecl( doc, node );
    }

    return doc;
}

void xmldoc_init(xmlDocPtr doc, const GUID *clsid)
{
    doc->_private = create_priv();
    priv_from_xmlDocPtr(doc)->properties = create_properties(clsid);
}

LONG xmldoc_add_ref(xmlDocPtr doc)
{
    LONG ref = InterlockedIncrement(&priv_from_xmlDocPtr(doc)->refs);
    TRACE("(%p)->(%d)\n", doc, ref);
    return ref;
}

LONG xmldoc_release(xmlDocPtr doc)
{
    xmldoc_priv *priv = priv_from_xmlDocPtr(doc);
    LONG ref = InterlockedDecrement(&priv->refs);
    TRACE("(%p)->(%d)\n", doc, ref);
    if(ref == 0)
    {
        orphan_entry *orphan, *orphan2;
        TRACE("freeing docptr %p\n", doc);

        LIST_FOR_EACH_ENTRY_SAFE( orphan, orphan2, &priv->orphans, orphan_entry, entry )
        {
            xmlFreeNode( orphan->node );
            heap_free( orphan );
        }
        free_properties(priv->properties);
        heap_free(doc->_private);

        xmlFreeDoc(doc);
    }

    return ref;
}

HRESULT xmldoc_add_orphan(xmlDocPtr doc, xmlNodePtr node)
{
    xmldoc_priv *priv = priv_from_xmlDocPtr(doc);
    orphan_entry *entry;

    entry = heap_alloc( sizeof (*entry) );
    if(!entry)
        return E_OUTOFMEMORY;

    entry->node = node;
    list_add_head( &priv->orphans, &entry->entry );
    return S_OK;
}

HRESULT xmldoc_remove_orphan(xmlDocPtr doc, xmlNodePtr node)
{
    xmldoc_priv *priv = priv_from_xmlDocPtr(doc);
    orphan_entry *entry, *entry2;

    LIST_FOR_EACH_ENTRY_SAFE( entry, entry2, &priv->orphans, orphan_entry, entry )
    {
        if( entry->node == node )
        {
            list_remove( &entry->entry );
            heap_free( entry );
            return S_OK;
        }
    }

    return S_FALSE;
}

static inline xmlDocPtr get_doc( domdoc *This )
{
    return (xmlDocPtr)This->node.node;
}

static HRESULT attach_xmldoc(domdoc *This, xmlDocPtr xml )
{
    if(This->node.node)
    {
        priv_from_xmlDocPtr(get_doc(This))->properties = NULL;
        if (xmldoc_release(get_doc(This)) != 0)
            priv_from_xmlDocPtr(get_doc(This))->properties =
                copy_properties(This->properties);
    }

    This->node.node = (xmlNodePtr) xml;

    if(This->node.node)
    {
        xmldoc_add_ref(get_doc(This));
        priv_from_xmlDocPtr(get_doc(This))->properties = This->properties;
    }

    return S_OK;
}

static inline domdoc *impl_from_IXMLDOMDocument3( IXMLDOMDocument3 *iface )
{
    return (domdoc *)((char*)iface - FIELD_OFFSET(domdoc, lpVtbl));
}

static inline domdoc *impl_from_IPersistStreamInit(IPersistStreamInit *iface)
{
    return (domdoc *)((char*)iface - FIELD_OFFSET(domdoc, lpvtblIPersistStreamInit));
}

static inline domdoc *impl_from_IObjectWithSite(IObjectWithSite *iface)
{
    return (domdoc *)((char*)iface - FIELD_OFFSET(domdoc, lpvtblIObjectWithSite));
}

static inline domdoc *impl_from_IObjectSafety(IObjectSafety *iface)
{
    return (domdoc *)((char*)iface - FIELD_OFFSET(domdoc, lpvtblIObjectSafety));
}

static inline domdoc *impl_from_ISupportErrorInfo(ISupportErrorInfo *iface)
{
    return (domdoc *)((char*)iface - FIELD_OFFSET(domdoc, lpvtblISupportErrorInfo));
}

static inline domdoc *impl_from_IConnectionPointContainer(IConnectionPointContainer *iface)
{
    return (domdoc *)((char*)iface - FIELD_OFFSET(domdoc, lpVtblConnectionPointContainer));
}

/************************************************************************
 * domdoc implementation of IPersistStream.
 */
static HRESULT WINAPI domdoc_IPersistStreamInit_QueryInterface(
    IPersistStreamInit *iface, REFIID riid, void **ppvObj)
{
    domdoc *this = impl_from_IPersistStreamInit(iface);
    return IXMLDOMDocument2_QueryInterface((IXMLDOMDocument2 *)this, riid, ppvObj);
}

static ULONG WINAPI domdoc_IPersistStreamInit_AddRef(
    IPersistStreamInit *iface)
{
    domdoc *this = impl_from_IPersistStreamInit(iface);
    return IXMLDOMDocument2_AddRef((IXMLDOMDocument2 *)this);
}

static ULONG WINAPI domdoc_IPersistStreamInit_Release(
    IPersistStreamInit *iface)
{
    domdoc *this = impl_from_IPersistStreamInit(iface);
    return IXMLDOMDocument2_Release((IXMLDOMDocument2 *)this);
}

static HRESULT WINAPI domdoc_IPersistStreamInit_GetClassID(
    IPersistStreamInit *iface, CLSID *classid)
{
    TRACE("(%p,%p): stub!\n", iface, classid);

    if(!classid)
        return E_POINTER;

    *classid = CLSID_DOMDocument2;

    return S_OK;
}

static HRESULT WINAPI domdoc_IPersistStreamInit_IsDirty(
    IPersistStreamInit *iface)
{
    domdoc *This = impl_from_IPersistStreamInit(iface);
    FIXME("(%p): stub!\n", This);
    return S_FALSE;
}

static HRESULT WINAPI domdoc_IPersistStreamInit_Load(
    IPersistStreamInit *iface, LPSTREAM pStm)
{
    domdoc *This = impl_from_IPersistStreamInit(iface);
    HRESULT hr;
    HGLOBAL hglobal;
    DWORD read, written, len;
    BYTE buf[4096];
    char *ptr;
    xmlDocPtr xmldoc = NULL;

    TRACE("(%p)->(%p)\n", This, pStm);

    if (!pStm)
        return E_INVALIDARG;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &This->stream);
    if (FAILED(hr))
        return hr;

    do
    {
        IStream_Read(pStm, buf, sizeof(buf), &read);
        hr = IStream_Write(This->stream, buf, read, &written);
    } while(SUCCEEDED(hr) && written != 0 && read != 0);

    if (FAILED(hr))
    {
        ERR("Failed to copy stream\n");
        return hr;
    }

    hr = GetHGlobalFromStream(This->stream, &hglobal);
    if (FAILED(hr))
        return hr;

    len = GlobalSize(hglobal);
    ptr = GlobalLock(hglobal);
    if (len != 0)
        xmldoc = doparse(This, ptr, len, NULL);
    GlobalUnlock(hglobal);

    if (!xmldoc)
    {
        ERR("Failed to parse xml\n");
        return E_FAIL;
    }

    xmldoc->_private = create_priv();

    return attach_xmldoc(This, xmldoc);
}

static HRESULT WINAPI domdoc_IPersistStreamInit_Save(
    IPersistStreamInit *iface, IStream *stream, BOOL clr_dirty)
{
    domdoc *This = impl_from_IPersistStreamInit(iface);
    BSTR xmlString;
    HRESULT hr;

    TRACE("(%p)->(%p %d)\n", This, stream, clr_dirty);

    hr = IXMLDOMDocument3_get_xml( (IXMLDOMDocument3*)&This->lpVtbl, &xmlString );
    if(hr == S_OK)
    {
        DWORD len = SysStringLen(xmlString) * sizeof(WCHAR);

        hr = IStream_Write( stream, xmlString, len, NULL );
        SysFreeString(xmlString);
    }

    TRACE("ret 0x%08x\n", hr);

    return hr;
}

static HRESULT WINAPI domdoc_IPersistStreamInit_GetSizeMax(
    IPersistStreamInit *iface, ULARGE_INTEGER *pcbSize)
{
    domdoc *This = impl_from_IPersistStreamInit(iface);
    TRACE("(%p)->(%p): stub!\n", This, pcbSize);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoc_IPersistStreamInit_InitNew(
    IPersistStreamInit *iface)
{
    domdoc *This = impl_from_IPersistStreamInit(iface);
    TRACE("(%p)\n", This);
    return S_OK;
}

static const IPersistStreamInitVtbl xmldoc_IPersistStreamInit_VTable =
{
    domdoc_IPersistStreamInit_QueryInterface,
    domdoc_IPersistStreamInit_AddRef,
    domdoc_IPersistStreamInit_Release,
    domdoc_IPersistStreamInit_GetClassID,
    domdoc_IPersistStreamInit_IsDirty,
    domdoc_IPersistStreamInit_Load,
    domdoc_IPersistStreamInit_Save,
    domdoc_IPersistStreamInit_GetSizeMax,
    domdoc_IPersistStreamInit_InitNew
};

/* ISupportErrorInfo interface */
static HRESULT WINAPI support_error_QueryInterface(
    ISupportErrorInfo *iface,
    REFIID riid, void** ppvObj )
{
    domdoc *This = impl_from_ISupportErrorInfo(iface);
    return IXMLDOMDocument3_QueryInterface((IXMLDOMDocument3 *)This, riid, ppvObj);
}

static ULONG WINAPI support_error_AddRef(
    ISupportErrorInfo *iface )
{
    domdoc *This = impl_from_ISupportErrorInfo(iface);
    return IXMLDOMDocument3_AddRef((IXMLDOMDocument3 *)This);
}

static ULONG WINAPI support_error_Release(
    ISupportErrorInfo *iface )
{
    domdoc *This = impl_from_ISupportErrorInfo(iface);
    return IXMLDOMDocument3_Release((IXMLDOMDocument3 *)This);
}

static HRESULT WINAPI support_error_InterfaceSupportsErrorInfo(
    ISupportErrorInfo *iface,
    REFIID riid )
{
    FIXME("(%p)->(%s)\n", iface, debugstr_guid(riid));
    return S_FALSE;
}

static const struct ISupportErrorInfoVtbl support_error_vtbl =
{
    support_error_QueryInterface,
    support_error_AddRef,
    support_error_Release,
    support_error_InterfaceSupportsErrorInfo
};

/* IXMLDOMDocument2 interface */
static HRESULT WINAPI domdoc_QueryInterface( IXMLDOMDocument3 *iface, REFIID riid, void** ppvObject )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid( riid ), ppvObject );

    *ppvObject = NULL;

    if ( IsEqualGUID( riid, &IID_IUnknown ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IXMLDOMNode ) ||
         IsEqualGUID( riid, &IID_IXMLDOMDocument ) ||
         IsEqualGUID( riid, &IID_IXMLDOMDocument2 )||
         IsEqualGUID( riid, &IID_IXMLDOMDocument3 ))
    {
        *ppvObject = iface;
    }
    else if (IsEqualGUID(&IID_IPersistStream, riid) ||
             IsEqualGUID(&IID_IPersistStreamInit, riid))
    {
        *ppvObject = &(This->lpvtblIPersistStreamInit);
    }
    else if (IsEqualGUID(&IID_IObjectWithSite, riid))
    {
        *ppvObject = &(This->lpvtblIObjectWithSite);
    }
    else if (IsEqualGUID(&IID_IObjectSafety, riid))
    {
        *ppvObject = &(This->lpvtblIObjectSafety);
    }
    else if( IsEqualGUID( riid, &IID_ISupportErrorInfo ))
    {
        *ppvObject = &This->lpvtblISupportErrorInfo;
    }
    else if(node_query_interface(&This->node, riid, ppvObject))
    {
        return *ppvObject ? S_OK : E_NOINTERFACE;
    }
    else if (IsEqualGUID( riid, &IID_IConnectionPointContainer ))
    {
        *ppvObject = &This->lpVtblConnectionPointContainer;
    }
    else if(IsEqualGUID(&IID_IRunnableObject, riid))
    {
        TRACE("IID_IRunnableObject not supported returning NULL\n");
        return E_NOINTERFACE;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppvObject);

    return S_OK;
}


static ULONG WINAPI domdoc_AddRef(
     IXMLDOMDocument3 *iface )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    TRACE("%p\n", This );
    return InterlockedIncrement( &This->ref );
}


static ULONG WINAPI domdoc_Release(
     IXMLDOMDocument3 *iface )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    LONG ref;

    TRACE("%p\n", This );

    ref = InterlockedDecrement( &This->ref );
    if ( ref == 0 )
    {
        if(This->bsc)
            detach_bsc(This->bsc);

        if (This->site)
            IUnknown_Release( This->site );
        destroy_xmlnode(&This->node);
        if(This->schema) IXMLDOMSchemaCollection2_Release(This->schema);
        if (This->stream) IStream_Release(This->stream);
        HeapFree( GetProcessHeap(), 0, This );
    }

    return ref;
}

static HRESULT WINAPI domdoc_GetTypeInfoCount( IXMLDOMDocument3 *iface, UINT* pctinfo )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;

    return S_OK;
}

static HRESULT WINAPI domdoc_GetTypeInfo(
    IXMLDOMDocument3 *iface,
    UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    HRESULT hr;

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(IXMLDOMDocument2_tid, ppTInfo);

    return hr;
}

static HRESULT WINAPI domdoc_GetIDsOfNames(
    IXMLDOMDocument3 *iface,
    REFIID riid,
    LPOLESTR* rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID* rgDispId)
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IXMLDOMDocument2_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}


static HRESULT WINAPI domdoc_Invoke(
    IXMLDOMDocument3 *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS* pDispParams,
    VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo,
    UINT* puArgErr)
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IXMLDOMDocument2_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &(This->lpVtbl), dispIdMember, wFlags, pDispParams,
                pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}


static HRESULT WINAPI domdoc_get_nodeName(
    IXMLDOMDocument3 *iface,
    BSTR* name )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );

    static const WCHAR documentW[] = {'#','d','o','c','u','m','e','n','t',0};

    TRACE("(%p)->(%p)\n", This, name);

    return return_bstr(documentW, name);
}


static HRESULT WINAPI domdoc_get_nodeValue(
    IXMLDOMDocument3 *iface,
    VARIANT* value )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );

    TRACE("(%p)->(%p)\n", This, value);

    if(!value)
        return E_INVALIDARG;

    V_VT(value) = VT_NULL;
    V_BSTR(value) = NULL; /* tests show that we should do this */
    return S_FALSE;
}


static HRESULT WINAPI domdoc_put_nodeValue(
    IXMLDOMDocument3 *iface,
    VARIANT value)
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    TRACE("(%p)->(v%d)\n", This, V_VT(&value));
    return E_FAIL;
}


static HRESULT WINAPI domdoc_get_nodeType(
    IXMLDOMDocument3 *iface,
    DOMNodeType* type )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );

    TRACE("(%p)->(%p)\n", This, type);

    *type = NODE_DOCUMENT;
    return S_OK;
}


static HRESULT WINAPI domdoc_get_parentNode(
    IXMLDOMDocument3 *iface,
    IXMLDOMNode** parent )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );

    TRACE("(%p)->(%p)\n", This, parent);

    return node_get_parent(&This->node, parent);
}


static HRESULT WINAPI domdoc_get_childNodes(
    IXMLDOMDocument3 *iface,
    IXMLDOMNodeList** childList )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );

    TRACE("(%p)->(%p)\n", This, childList);

    return node_get_child_nodes(&This->node, childList);
}


static HRESULT WINAPI domdoc_get_firstChild(
    IXMLDOMDocument3 *iface,
    IXMLDOMNode** firstChild )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );

    TRACE("(%p)->(%p)\n", This, firstChild);

    return node_get_first_child(&This->node, firstChild);
}


static HRESULT WINAPI domdoc_get_lastChild(
    IXMLDOMDocument3 *iface,
    IXMLDOMNode** lastChild )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );

    TRACE("(%p)->(%p)\n", This, lastChild);

    return node_get_last_child(&This->node, lastChild);
}


static HRESULT WINAPI domdoc_get_previousSibling(
    IXMLDOMDocument3 *iface,
    IXMLDOMNode** previousSibling )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );

    TRACE("(%p)->(%p)\n", This, previousSibling);

    return return_null_node(previousSibling);
}


static HRESULT WINAPI domdoc_get_nextSibling(
    IXMLDOMDocument3 *iface,
    IXMLDOMNode** nextSibling )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );

    TRACE("(%p)->(%p)\n", This, nextSibling);

    return return_null_node(nextSibling);
}


static HRESULT WINAPI domdoc_get_attributes(
    IXMLDOMDocument3 *iface,
    IXMLDOMNamedNodeMap** attributeMap )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );

    TRACE("(%p)->(%p)\n", This, attributeMap);

    return return_null_ptr((void**)attributeMap);
}


static HRESULT WINAPI domdoc_insertBefore(
    IXMLDOMDocument3 *iface,
    IXMLDOMNode* newChild,
    VARIANT refChild,
    IXMLDOMNode** outNewChild )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );

    TRACE("(%p)->(%p x%d %p)\n", This, newChild, V_VT(&refChild), outNewChild);

    return node_insert_before(&This->node, newChild, &refChild, outNewChild);
}


static HRESULT WINAPI domdoc_replaceChild(
    IXMLDOMDocument3 *iface,
    IXMLDOMNode* newChild,
    IXMLDOMNode* oldChild,
    IXMLDOMNode** outOldChild)
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );

    TRACE("(%p)->(%p %p %p)\n", This, newChild, oldChild, outOldChild);

    return node_replace_child(&This->node, newChild, oldChild, outOldChild);
}


static HRESULT WINAPI domdoc_removeChild(
    IXMLDOMDocument3 *iface,
    IXMLDOMNode* childNode,
    IXMLDOMNode** oldChild)
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    return IXMLDOMNode_removeChild( IXMLDOMNode_from_impl(&This->node), childNode, oldChild );
}


static HRESULT WINAPI domdoc_appendChild(
    IXMLDOMDocument3 *iface,
    IXMLDOMNode* newChild,
    IXMLDOMNode** outNewChild)
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    return IXMLDOMNode_appendChild( IXMLDOMNode_from_impl(&This->node), newChild, outNewChild );
}


static HRESULT WINAPI domdoc_hasChildNodes(
    IXMLDOMDocument3 *iface,
    VARIANT_BOOL* hasChild)
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    return IXMLDOMNode_hasChildNodes( IXMLDOMNode_from_impl(&This->node), hasChild );
}


static HRESULT WINAPI domdoc_get_ownerDocument(
    IXMLDOMDocument3 *iface,
    IXMLDOMDocument** DOMDocument)
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    return IXMLDOMNode_get_ownerDocument( IXMLDOMNode_from_impl(&This->node), DOMDocument );
}


static HRESULT WINAPI domdoc_cloneNode(
    IXMLDOMDocument3 *iface,
    VARIANT_BOOL deep,
    IXMLDOMNode** outNode)
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    TRACE("(%p)->(%d %p)\n", This, deep, outNode);
    return node_clone( &This->node, deep, outNode );
}


static HRESULT WINAPI domdoc_get_nodeTypeString(
    IXMLDOMDocument3 *iface,
    BSTR* nodeType )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    return IXMLDOMNode_get_nodeTypeString( IXMLDOMNode_from_impl(&This->node), nodeType );
}


static HRESULT WINAPI domdoc_get_text(
    IXMLDOMDocument3 *iface,
    BSTR* text )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    return IXMLDOMNode_get_text( IXMLDOMNode_from_impl(&This->node), text );
}


static HRESULT WINAPI domdoc_put_text(
    IXMLDOMDocument3 *iface,
    BSTR text )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    TRACE("(%p)->(%s)\n", This, debugstr_w(text));
    return E_FAIL;
}


static HRESULT WINAPI domdoc_get_specified(
    IXMLDOMDocument3 *iface,
    VARIANT_BOOL* isSpecified )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    FIXME("(%p)->(%p) stub!\n", This, isSpecified);
    *isSpecified = VARIANT_TRUE;
    return S_OK;
}


static HRESULT WINAPI domdoc_get_definition(
    IXMLDOMDocument3 *iface,
    IXMLDOMNode** definitionNode )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    FIXME("(%p)->(%p)\n", This, definitionNode);
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_nodeTypedValue(
    IXMLDOMDocument3 *iface,
    VARIANT* typedValue )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    return IXMLDOMNode_get_nodeTypedValue( IXMLDOMNode_from_impl(&This->node), typedValue );
}

static HRESULT WINAPI domdoc_put_nodeTypedValue(
    IXMLDOMDocument3 *iface,
    VARIANT typedValue )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    return IXMLDOMNode_put_nodeTypedValue( IXMLDOMNode_from_impl(&This->node), typedValue );
}


static HRESULT WINAPI domdoc_get_dataType(
    IXMLDOMDocument3 *iface,
    VARIANT* typename )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    TRACE("(%p)->(%p)\n", This, typename);
    return return_null_var( typename );
}


static HRESULT WINAPI domdoc_put_dataType(
    IXMLDOMDocument3 *iface,
    BSTR dataTypeName )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    return IXMLDOMNode_put_dataType( IXMLDOMNode_from_impl(&This->node), dataTypeName );
}


static HRESULT WINAPI domdoc_get_xml(
    IXMLDOMDocument3 *iface,
    BSTR* p)
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );

    TRACE("(%p)->(%p)\n", This, p);

    return node_get_xml(&This->node, TRUE, TRUE, p);
}


static HRESULT WINAPI domdoc_transformNode(
    IXMLDOMDocument3 *iface,
    IXMLDOMNode* styleSheet,
    BSTR* xmlString )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    return IXMLDOMNode_transformNode( IXMLDOMNode_from_impl(&This->node), styleSheet, xmlString );
}


static HRESULT WINAPI domdoc_selectNodes(
    IXMLDOMDocument3 *iface,
    BSTR queryString,
    IXMLDOMNodeList** resultList )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    return IXMLDOMNode_selectNodes( IXMLDOMNode_from_impl(&This->node), queryString, resultList );
}


static HRESULT WINAPI domdoc_selectSingleNode(
    IXMLDOMDocument3 *iface,
    BSTR queryString,
    IXMLDOMNode** resultNode )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    return IXMLDOMNode_selectSingleNode( IXMLDOMNode_from_impl(&This->node), queryString, resultNode );
}


static HRESULT WINAPI domdoc_get_parsed(
    IXMLDOMDocument3 *iface,
    VARIANT_BOOL* isParsed )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    FIXME("(%p)->(%p) stub!\n", This, isParsed);
    *isParsed = VARIANT_TRUE;
    return S_OK;
}


static HRESULT WINAPI domdoc_get_namespaceURI(
    IXMLDOMDocument3 *iface,
    BSTR* namespaceURI )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    return IXMLDOMNode_get_namespaceURI( IXMLDOMNode_from_impl(&This->node), namespaceURI );
}


static HRESULT WINAPI domdoc_get_prefix(
    IXMLDOMDocument3 *iface,
    BSTR* prefix )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    TRACE("(%p)->(%p)\n", This, prefix);
    return return_null_bstr( prefix );
}


static HRESULT WINAPI domdoc_get_baseName(
    IXMLDOMDocument3 *iface,
    BSTR* name )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    TRACE("(%p)->(%p)\n", This, name);
    return return_null_bstr( name );
}


static HRESULT WINAPI domdoc_transformNodeToObject(
    IXMLDOMDocument3 *iface,
    IXMLDOMNode* stylesheet,
    VARIANT outputObject)
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    return IXMLDOMNode_transformNodeToObject( IXMLDOMNode_from_impl(&This->node), stylesheet, outputObject );
}


static HRESULT WINAPI domdoc_get_doctype(
    IXMLDOMDocument3 *iface,
    IXMLDOMDocumentType** documentType )
{
    domdoc *This = impl_from_IXMLDOMDocument3(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_implementation(
    IXMLDOMDocument3 *iface,
    IXMLDOMImplementation** impl )
{
    domdoc *This = impl_from_IXMLDOMDocument3(iface);

    TRACE("(%p)->(%p)\n", This, impl);

    if(!impl)
        return E_INVALIDARG;

    *impl = (IXMLDOMImplementation*)create_doc_Implementation();

    return S_OK;
}

static HRESULT WINAPI domdoc_get_documentElement(
    IXMLDOMDocument3 *iface,
    IXMLDOMElement** DOMElement )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    IXMLDOMNode *element_node;
    xmlNodePtr root;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, DOMElement);

    if(!DOMElement)
        return E_INVALIDARG;

    *DOMElement = NULL;

    root = xmlDocGetRootElement( get_doc(This) );
    if ( !root )
        return S_FALSE;

    element_node = create_node( root );
    if(!element_node) return S_FALSE;

    hr = IXMLDOMNode_QueryInterface(element_node, &IID_IXMLDOMElement, (void**)DOMElement);
    IXMLDOMNode_Release(element_node);

    return hr;
}


static HRESULT WINAPI domdoc_put_documentElement(
    IXMLDOMDocument3 *iface,
    IXMLDOMElement* DOMElement )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    IXMLDOMNode *elementNode;
    xmlNodePtr oldRoot;
    xmlnode *xmlNode;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, DOMElement);

    hr = IXMLDOMElement_QueryInterface( DOMElement, &IID_IXMLDOMNode, (void**)&elementNode );
    if(FAILED(hr))
        return hr;

    xmlNode = get_node_obj( elementNode );
    if(!xmlNode) {
        FIXME("elementNode is not our object\n");
        return E_FAIL;
    }

    if(!xmlNode->node->parent)
        if(xmldoc_remove_orphan(xmlNode->node->doc, xmlNode->node) != S_OK)
            WARN("%p is not an orphan of %p\n", xmlNode->node->doc, xmlNode->node);

    oldRoot = xmlDocSetRootElement( get_doc(This), xmlNode->node);
    IXMLDOMNode_Release( elementNode );

    if(oldRoot)
        xmldoc_add_orphan(oldRoot->doc, oldRoot);

    return S_OK;
}


static HRESULT WINAPI domdoc_createElement(
    IXMLDOMDocument3 *iface,
    BSTR tagname,
    IXMLDOMElement** element )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    IXMLDOMNode *node;
    VARIANT type;
    HRESULT hr;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(tagname), element);

    if (!element || !tagname) return E_INVALIDARG;

    V_VT(&type) = VT_I1;
    V_I1(&type) = NODE_ELEMENT;

    hr = IXMLDOMDocument3_createNode(iface, type, tagname, NULL, &node);
    if (hr == S_OK)
    {
        IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMElement, (void**)element);
        IXMLDOMNode_Release(node);
    }

    return hr;
}


static HRESULT WINAPI domdoc_createDocumentFragment(
    IXMLDOMDocument3 *iface,
    IXMLDOMDocumentFragment** frag )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    IXMLDOMNode *node;
    VARIANT type;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, frag);

    if (!frag) return E_INVALIDARG;

    *frag = NULL;

    V_VT(&type) = VT_I1;
    V_I1(&type) = NODE_DOCUMENT_FRAGMENT;

    hr = IXMLDOMDocument3_createNode(iface, type, NULL, NULL, &node);
    if (hr == S_OK)
    {
        IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMDocumentFragment, (void**)frag);
        IXMLDOMNode_Release(node);
    }

    return hr;
}


static HRESULT WINAPI domdoc_createTextNode(
    IXMLDOMDocument3 *iface,
    BSTR data,
    IXMLDOMText** text )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    IXMLDOMNode *node;
    VARIANT type;
    HRESULT hr;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(data), text);

    if (!text) return E_INVALIDARG;

    *text = NULL;

    V_VT(&type) = VT_I1;
    V_I1(&type) = NODE_TEXT;

    hr = IXMLDOMDocument3_createNode(iface, type, NULL, NULL, &node);
    if (hr == S_OK)
    {
        IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMText, (void**)text);
        IXMLDOMNode_Release(node);
        hr = IXMLDOMText_put_data(*text, data);
    }

    return hr;
}


static HRESULT WINAPI domdoc_createComment(
    IXMLDOMDocument3 *iface,
    BSTR data,
    IXMLDOMComment** comment )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    VARIANT type;
    HRESULT hr;
    IXMLDOMNode *node;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(data), comment);

    if (!comment) return E_INVALIDARG;

    *comment = NULL;

    V_VT(&type) = VT_I1;
    V_I1(&type) = NODE_COMMENT;

    hr = IXMLDOMDocument3_createNode(iface, type, NULL, NULL, &node);
    if (hr == S_OK)
    {
        IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMComment, (void**)comment);
        IXMLDOMNode_Release(node);
        hr = IXMLDOMComment_put_data(*comment, data);
    }

    return hr;
}


static HRESULT WINAPI domdoc_createCDATASection(
    IXMLDOMDocument3 *iface,
    BSTR data,
    IXMLDOMCDATASection** cdata )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    IXMLDOMNode *node;
    VARIANT type;
    HRESULT hr;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(data), cdata);

    if (!cdata) return E_INVALIDARG;

    *cdata = NULL;

    V_VT(&type) = VT_I1;
    V_I1(&type) = NODE_CDATA_SECTION;

    hr = IXMLDOMDocument3_createNode(iface, type, NULL, NULL, &node);
    if (hr == S_OK)
    {
        IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMCDATASection, (void**)cdata);
        IXMLDOMNode_Release(node);
        hr = IXMLDOMCDATASection_put_data(*cdata, data);
    }

    return hr;
}


static HRESULT WINAPI domdoc_createProcessingInstruction(
    IXMLDOMDocument3 *iface,
    BSTR target,
    BSTR data,
    IXMLDOMProcessingInstruction** pi )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    IXMLDOMNode *node;
    VARIANT type;
    HRESULT hr;

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_w(target), debugstr_w(data), pi);

    if (!pi) return E_INVALIDARG;

    *pi = NULL;

    V_VT(&type) = VT_I1;
    V_I1(&type) = NODE_PROCESSING_INSTRUCTION;

    hr = IXMLDOMDocument3_createNode(iface, type, target, NULL, &node);
    if (hr == S_OK)
    {
        xmlnode *node_obj;

        /* this is to bypass check in ::put_data() that blocks "<?xml" PIs */
        node_obj = get_node_obj(node);
        hr = node_set_content(node_obj, data);

        IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMProcessingInstruction, (void**)pi);
        IXMLDOMNode_Release(node);
    }

    return hr;
}


static HRESULT WINAPI domdoc_createAttribute(
    IXMLDOMDocument3 *iface,
    BSTR name,
    IXMLDOMAttribute** attribute )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    IXMLDOMNode *node;
    VARIANT type;
    HRESULT hr;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(name), attribute);

    if (!attribute || !name) return E_INVALIDARG;

    V_VT(&type) = VT_I1;
    V_I1(&type) = NODE_ATTRIBUTE;

    hr = IXMLDOMDocument3_createNode(iface, type, name, NULL, &node);
    if (hr == S_OK)
    {
        IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMAttribute, (void**)attribute);
        IXMLDOMNode_Release(node);
    }

    return hr;
}


static HRESULT WINAPI domdoc_createEntityReference(
    IXMLDOMDocument3 *iface,
    BSTR name,
    IXMLDOMEntityReference** entityref )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    IXMLDOMNode *node;
    VARIANT type;
    HRESULT hr;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(name), entityref);

    if (!entityref) return E_INVALIDARG;

    *entityref = NULL;

    V_VT(&type) = VT_I1;
    V_I1(&type) = NODE_ENTITY_REFERENCE;

    hr = IXMLDOMDocument3_createNode(iface, type, name, NULL, &node);
    if (hr == S_OK)
    {
        IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMEntityReference, (void**)entityref);
        IXMLDOMNode_Release(node);
    }

    return hr;
}


static HRESULT WINAPI domdoc_getElementsByTagName(
    IXMLDOMDocument3 *iface,
    BSTR tagName,
    IXMLDOMNodeList** resultList )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    HRESULT hr;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(tagName), resultList);

    if (!tagName || !resultList) return E_INVALIDARG;

    if (tagName[0] == '*' && tagName[1] == 0)
    {
        static const WCHAR formatallW[] = {'/','/','*',0};
        hr = queryresult_create((xmlNodePtr)get_doc(This), formatallW, resultList);
    }
    else
    {
        static const WCHAR xpathformat[] =
            { '/','/','*','[','l','o','c','a','l','-','n','a','m','e','(',')','=','\'' };
        static const WCHAR closeW[] = { '\'',']',0 };

        LPWSTR pattern;
        WCHAR *ptr;
        INT length;

        length = lstrlenW(tagName);

        /* without two WCHARs from format specifier */
        ptr = pattern = heap_alloc(sizeof(xpathformat) + length*sizeof(WCHAR) + sizeof(closeW));

        memcpy(ptr, xpathformat, sizeof(xpathformat));
        ptr += sizeof(xpathformat)/sizeof(WCHAR);
        memcpy(ptr, tagName, length*sizeof(WCHAR));
        ptr += length;
        memcpy(ptr, closeW, sizeof(closeW));

        hr = queryresult_create((xmlNodePtr)get_doc(This), pattern, resultList);
        heap_free(pattern);
    }

    return hr;
}

static HRESULT get_node_type(VARIANT Type, DOMNodeType * type)
{
    VARIANT tmp;
    HRESULT hr;

    VariantInit(&tmp);
    hr = VariantChangeType(&tmp, &Type, 0, VT_I4);
    if(FAILED(hr))
        return E_INVALIDARG;

    *type = V_I4(&tmp);

    return S_OK;
}

static HRESULT WINAPI domdoc_createNode(
    IXMLDOMDocument3 *iface,
    VARIANT Type,
    BSTR name,
    BSTR namespaceURI,
    IXMLDOMNode** node )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    DOMNodeType node_type;
    xmlNodePtr xmlnode;
    xmlChar *xml_name, *href;
    HRESULT hr;

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_w(name), debugstr_w(namespaceURI), node);

    if(!node) return E_INVALIDARG;

    hr = get_node_type(Type, &node_type);
    if(FAILED(hr)) return hr;

    if(namespaceURI && namespaceURI[0] && node_type != NODE_ELEMENT)
        FIXME("nodes with namespaces currently not supported.\n");

    TRACE("node_type %d\n", node_type);

    /* exit earlier for types that need name */
    switch(node_type)
    {
    case NODE_ELEMENT:
    case NODE_ATTRIBUTE:
    case NODE_ENTITY_REFERENCE:
    case NODE_PROCESSING_INSTRUCTION:
        if (!name || *name == 0) return E_FAIL;
    default:
        break;
    }

    xml_name = xmlChar_from_wchar(name);
    /* prevent empty href to be allocated */
    href = namespaceURI ? xmlChar_from_wchar(namespaceURI) : NULL;

    switch(node_type)
    {
    case NODE_ELEMENT:
    {
        xmlChar *local, *prefix;

        local = xmlSplitQName2(xml_name, &prefix);

        xmlnode = xmlNewDocNode(get_doc(This), NULL, local ? local : xml_name, NULL);

        /* allow to create default namespace xmlns= */
        if (local || (href && *href))
        {
            xmlNsPtr ns = xmlNewNs(xmlnode, href, prefix);
            xmlSetNs(xmlnode, ns);
        }

        xmlFree(local);
        xmlFree(prefix);

        break;
    }
    case NODE_ATTRIBUTE:
        xmlnode = (xmlNodePtr)xmlNewDocProp(get_doc(This), xml_name, NULL);
        break;
    case NODE_TEXT:
        xmlnode = (xmlNodePtr)xmlNewDocText(get_doc(This), NULL);
        break;
    case NODE_CDATA_SECTION:
        xmlnode = xmlNewCDataBlock(get_doc(This), NULL, 0);
        break;
    case NODE_ENTITY_REFERENCE:
        xmlnode = xmlNewReference(get_doc(This), xml_name);
        break;
    case NODE_PROCESSING_INSTRUCTION:
#ifdef HAVE_XMLNEWDOCPI
        xmlnode = xmlNewDocPI(get_doc(This), xml_name, NULL);
#else
        FIXME("xmlNewDocPI() not supported, use libxml2 2.6.15 or greater\n");
        xmlnode = NULL;
#endif
        break;
    case NODE_COMMENT:
        xmlnode = xmlNewDocComment(get_doc(This), NULL);
        break;
    case NODE_DOCUMENT_FRAGMENT:
        xmlnode = xmlNewDocFragment(get_doc(This));
        break;
    /* unsupported types */
    case NODE_DOCUMENT:
    case NODE_DOCUMENT_TYPE:
    case NODE_ENTITY:
    case NODE_NOTATION:
        heap_free(xml_name);
        return E_INVALIDARG;
    default:
        FIXME("unhandled node type %d\n", node_type);
        xmlnode = NULL;
        break;
    }

    *node = create_node(xmlnode);
    heap_free(xml_name);
    heap_free(href);

    if(*node)
    {
        TRACE("created node (%d, %p, %p)\n", node_type, *node, xmlnode);
        xmldoc_add_orphan(xmlnode->doc, xmlnode);
        return S_OK;
    }

    return E_FAIL;
}

static HRESULT WINAPI domdoc_nodeFromID(
    IXMLDOMDocument3 *iface,
    BSTR idString,
    IXMLDOMNode** node )
{
    domdoc *This = impl_from_IXMLDOMDocument3(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(idString), node);
    return E_NOTIMPL;
}

static HRESULT domdoc_onDataAvailable(void *obj, char *ptr, DWORD len)
{
    domdoc *This = obj;
    xmlDocPtr xmldoc;

    xmldoc = doparse(This, ptr, len, NULL);
    if(xmldoc) {
        xmldoc->_private = create_priv();
        return attach_xmldoc(This, xmldoc);
    }

    return S_OK;
}

static HRESULT doread( domdoc *This, LPWSTR filename )
{
    bsc_t *bsc;
    HRESULT hr;

    hr = bind_url(filename, domdoc_onDataAvailable, This, &bsc);
    if(FAILED(hr))
        return hr;

    if(This->bsc)
        detach_bsc(This->bsc);

    This->bsc = bsc;
    return S_OK;
}

static HRESULT WINAPI domdoc_load(
    IXMLDOMDocument3 *iface,
    VARIANT xmlSource,
    VARIANT_BOOL* isSuccessful )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    LPWSTR filename = NULL;
    HRESULT hr = S_FALSE;
    IXMLDOMDocument3 *pNewDoc = NULL;
    IStream *pStream = NULL;
    xmlDocPtr xmldoc;

    TRACE("(%p)->type %d\n", This, V_VT(&xmlSource) );

    *isSuccessful = VARIANT_FALSE;

    assert( &This->node );

    switch( V_VT(&xmlSource) )
    {
    case VT_BSTR:
        filename = V_BSTR(&xmlSource);
        break;
    case VT_UNKNOWN:
        hr = IUnknown_QueryInterface(V_UNKNOWN(&xmlSource), &IID_IXMLDOMDocument3, (void**)&pNewDoc);
        if(hr == S_OK)
        {
            if(pNewDoc)
            {
                domdoc *newDoc = impl_from_IXMLDOMDocument3( pNewDoc );
                xmldoc = xmlCopyDoc(get_doc(newDoc), 1);
                hr = attach_xmldoc(This, xmldoc);

                if(SUCCEEDED(hr))
                    *isSuccessful = VARIANT_TRUE;

                return hr;
            }
        }
        hr = IUnknown_QueryInterface(V_UNKNOWN(&xmlSource), &IID_IStream, (void**)&pStream);
        if(hr == S_OK)
        {
            IPersistStream *pDocStream;
            hr = IUnknown_QueryInterface(iface, &IID_IPersistStream, (void**)&pDocStream);
            if(hr == S_OK)
            {
                hr = IPersistStream_Load(pDocStream, pStream);
                IStream_Release(pStream);
                if(hr == S_OK)
                {
                    *isSuccessful = VARIANT_TRUE;

                    TRACE("Using IStream to load Document\n");
                    return S_OK;
                }
                else
                {
                    ERR("xmldoc_IPersistStream_Load failed (%d)\n", hr);
                }
            }
            else
            {
                ERR("QueryInterface IID_IPersistStream failed (%d)\n", hr);
            }
        }
        else
        {
            /* ISequentialStream */
            FIXME("Unknown type not supported (%d) (%p)(%p)\n", hr, pNewDoc, V_UNKNOWN(&xmlSource)->lpVtbl);
        }
        break;
     default:
            FIXME("VT type not supported (%d)\n", V_VT(&xmlSource));
     }

    TRACE("filename (%s)\n", debugstr_w(filename));

    if ( filename )
    {
        hr = doread( This, filename );

        if ( FAILED(hr) )
            This->error = E_FAIL;
        else
        {
            hr = This->error = S_OK;
            *isSuccessful = VARIANT_TRUE;
        }
    }

    if(!filename || FAILED(hr)) {
        xmldoc = xmlNewDoc(NULL);
        xmldoc->_private = create_priv();
        hr = attach_xmldoc(This, xmldoc);
        if(SUCCEEDED(hr))
            hr = S_FALSE;
    }

    TRACE("ret (%d)\n", hr);

    return hr;
}


static HRESULT WINAPI domdoc_get_readyState(
    IXMLDOMDocument3 *iface,
    LONG *value )
{
    domdoc *This = impl_from_IXMLDOMDocument3(iface);
    FIXME("stub! (%p)->(%p)\n", This, value);

    if (!value)
        return E_INVALIDARG;

    *value = READYSTATE_COMPLETE;
    return S_OK;
}


static HRESULT WINAPI domdoc_get_parseError(
    IXMLDOMDocument3 *iface,
    IXMLDOMParseError** errorObj )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    static const WCHAR err[] = {'e','r','r','o','r',0};
    BSTR error_string = NULL;

    FIXME("(%p)->(%p): creating a dummy parseError\n", iface, errorObj);

    if(This->error)
        error_string = SysAllocString(err);

    *errorObj = create_parseError(This->error, NULL, error_string, NULL, 0, 0, 0);
    if(!*errorObj) return E_OUTOFMEMORY;
    return S_OK;
}


static HRESULT WINAPI domdoc_get_url(
    IXMLDOMDocument3 *iface,
    BSTR* urlString )
{
    domdoc *This = impl_from_IXMLDOMDocument3(iface);
    FIXME("(%p)->(%p)\n", This, urlString);
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_async(
    IXMLDOMDocument3 *iface,
    VARIANT_BOOL* isAsync )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );

    TRACE("(%p)->(%p: %d)\n", This, isAsync, This->async);
    *isAsync = This->async;
    return S_OK;
}


static HRESULT WINAPI domdoc_put_async(
    IXMLDOMDocument3 *iface,
    VARIANT_BOOL isAsync )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );

    TRACE("(%p)->(%d)\n", This, isAsync);
    This->async = isAsync;
    return S_OK;
}


static HRESULT WINAPI domdoc_abort(
    IXMLDOMDocument3 *iface )
{
    domdoc *This = impl_from_IXMLDOMDocument3(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}


static BOOL bstr_to_utf8( BSTR bstr, char **pstr, int *plen )
{
    UINT len;
    LPSTR str;

    len = WideCharToMultiByte( CP_UTF8, 0, bstr, -1, NULL, 0, NULL, NULL );
    str = heap_alloc( len );
    if ( !str )
        return FALSE;
    WideCharToMultiByte( CP_UTF8, 0, bstr, -1, str, len, NULL, NULL );
    *plen = len;
    *pstr = str;
    return TRUE;
}

/* don't rely on data to be in BSTR format, treat it as WCHAR string */
static HRESULT WINAPI domdoc_loadXML(
    IXMLDOMDocument3 *iface,
    BSTR bstrXML,
    VARIANT_BOOL* isSuccessful )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    static const xmlChar encoding[] = "UTF-8";
    xmlDocPtr xmldoc = NULL;
    HRESULT hr = S_FALSE, hr2;
    char *str;
    int len;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w( bstrXML ), isSuccessful );

    assert ( &This->node );

    if ( isSuccessful )
    {
        *isSuccessful = VARIANT_FALSE;

        if ( bstrXML && bstr_to_utf8( bstrXML, &str, &len ) )
        {
            xmldoc = doparse(This, str, len, encoding);
            heap_free( str );
            if ( !xmldoc )
            {
                This->error = E_FAIL;
                TRACE("failed to parse document\n");
            }
            else
            {
                hr = This->error = S_OK;
                *isSuccessful = VARIANT_TRUE;
                TRACE("parsed document %p\n", xmldoc);
            }
        }
    }
    if(!xmldoc)
        xmldoc = xmlNewDoc(NULL);

    xmldoc->_private = create_priv();

    hr2 = attach_xmldoc(This, xmldoc);
    if( FAILED(hr2) )
        hr = hr2;

    return hr;
}

static int XMLCALL domdoc_save_writecallback(void *ctx, const char *buffer, int len)
{
    DWORD written = -1;

    if(!WriteFile(ctx, buffer, len, &written, NULL))
    {
        WARN("write error\n");
        return -1;
    }
    else
        return written;
}

static int XMLCALL domdoc_save_closecallback(void *ctx)
{
    return CloseHandle(ctx) ? 0 : -1;
}

static int XMLCALL domdoc_stream_save_writecallback(void *ctx, const char *buffer, int len)
{
    ULONG written = 0;
    HRESULT hr;

    hr = IStream_Write((IStream*)ctx, buffer, len, &written);
    if (hr != S_OK)
    {
        WARN("stream write error: 0x%08x\n", hr);
        return -1;
    }
    else
        return written;
}

static int XMLCALL domdoc_stream_save_closecallback(void *ctx)
{
    IStream_Release((IStream*)ctx);
    return 0;
}

static HRESULT WINAPI domdoc_save(
    IXMLDOMDocument3 *iface,
    VARIANT destination )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    xmlSaveCtxtPtr ctx = NULL;
    xmlNodePtr xmldecl;
    HRESULT ret = S_OK;

    TRACE("(%p)->(var(vt %d, %s))\n", This, V_VT(&destination),
          V_VT(&destination) == VT_BSTR ? debugstr_w(V_BSTR(&destination)) : NULL);

    if(V_VT(&destination) != VT_BSTR && V_VT(&destination) != VT_UNKNOWN)
    {
        FIXME("Unhandled vt %d\n", V_VT(&destination));
        return S_FALSE;
    }

    if(V_VT(&destination) == VT_UNKNOWN)
    {
        IUnknown *pUnk = V_UNKNOWN(&destination);
        IXMLDOMDocument2 *document;
        IStream *stream;

        ret = IUnknown_QueryInterface(pUnk, &IID_IXMLDOMDocument3, (void**)&document);
        if(ret == S_OK)
        {
            VARIANT_BOOL success;
            BSTR xml;

            ret = IXMLDOMDocument3_get_xml(iface, &xml);
            if(ret == S_OK)
            {
                ret = IXMLDOMDocument3_loadXML(document, xml, &success);
                SysFreeString(xml);
            }

            IXMLDOMDocument3_Release(document);
            return ret;
        }

        ret = IUnknown_QueryInterface(pUnk, &IID_IStream, (void**)&stream);
        if(ret == S_OK)
        {
            ctx = xmlSaveToIO(domdoc_stream_save_writecallback,
                domdoc_stream_save_closecallback, stream, NULL, XML_SAVE_NO_DECL);

            if(!ctx)
            {
                IStream_Release(stream);
                return E_FAIL;
            }
        }
    }
    else
    {
        /* save with file path */
        HANDLE handle = CreateFileW( V_BSTR(&destination), GENERIC_WRITE, 0,
                                    NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
        if( handle == INVALID_HANDLE_VALUE )
        {
            WARN("failed to create file\n");
            return E_FAIL;
        }

        /* disable top XML declaration */
        ctx = xmlSaveToIO(domdoc_save_writecallback, domdoc_save_closecallback,
                          handle, NULL, XML_SAVE_NO_DECL);
        if (!ctx)
        {
            CloseHandle(handle);
            return E_FAIL;
        }
    }

    xmldecl = xmldoc_unlink_xmldecl(get_doc(This));
    if (xmlSaveDoc(ctx, get_doc(This)) == -1) ret = S_FALSE;
    xmldoc_link_xmldecl(get_doc(This), xmldecl);

    /* will release resources through close callback */
    xmlSaveClose(ctx);

    return ret;
}

static HRESULT WINAPI domdoc_get_validateOnParse(
    IXMLDOMDocument3 *iface,
    VARIANT_BOOL* isValidating )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    TRACE("(%p)->(%p: %d)\n", This, isValidating, This->validating);
    *isValidating = This->validating;
    return S_OK;
}


static HRESULT WINAPI domdoc_put_validateOnParse(
    IXMLDOMDocument3 *iface,
    VARIANT_BOOL isValidating )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    TRACE("(%p)->(%d)\n", This, isValidating);
    This->validating = isValidating;
    return S_OK;
}


static HRESULT WINAPI domdoc_get_resolveExternals(
    IXMLDOMDocument3 *iface,
    VARIANT_BOOL* isResolving )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    TRACE("(%p)->(%p: %d)\n", This, isResolving, This->resolving);
    *isResolving = This->resolving;
    return S_OK;
}


static HRESULT WINAPI domdoc_put_resolveExternals(
    IXMLDOMDocument3 *iface,
    VARIANT_BOOL isResolving )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    TRACE("(%p)->(%d)\n", This, isResolving);
    This->resolving = isResolving;
    return S_OK;
}


static HRESULT WINAPI domdoc_get_preserveWhiteSpace(
    IXMLDOMDocument3 *iface,
    VARIANT_BOOL* isPreserving )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    TRACE("(%p)->(%p: %d)\n", This, isPreserving, This->properties->preserving);
    *isPreserving = This->properties->preserving;
    return S_OK;
}


static HRESULT WINAPI domdoc_put_preserveWhiteSpace(
    IXMLDOMDocument3 *iface,
    VARIANT_BOOL isPreserving )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    TRACE("(%p)->(%d)\n", This, isPreserving);
    This->properties->preserving = isPreserving;
    return S_OK;
}


static HRESULT WINAPI domdoc_put_onReadyStateChange(
    IXMLDOMDocument3 *iface,
    VARIANT readyStateChangeSink )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    FIXME("%p\n", This);
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_put_onDataAvailable(
    IXMLDOMDocument3 *iface,
    VARIANT onDataAvailableSink )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoc_put_onTransformNode(
    IXMLDOMDocument3 *iface,
    VARIANT onTransformNodeSink )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoc_get_namespaces(
    IXMLDOMDocument3* iface,
    IXMLDOMSchemaCollection** schemaCollection )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    FIXME("(%p)->(%p)\n", This, schemaCollection);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoc_get_schemas(
    IXMLDOMDocument3* iface,
    VARIANT* var1 )
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    HRESULT hr = S_FALSE;
    IXMLDOMSchemaCollection2* cur_schema = This->schema;

    TRACE("(%p)->(%p)\n", This, var1);

    VariantInit(var1); /* Test shows we don't call VariantClear here */
    V_VT(var1) = VT_NULL;

    if(cur_schema)
    {
        hr = IXMLDOMSchemaCollection2_QueryInterface(cur_schema, &IID_IDispatch, (void**)&V_DISPATCH(var1));
        if(SUCCEEDED(hr))
            V_VT(var1) = VT_DISPATCH;
    }
    return hr;
}

static HRESULT WINAPI domdoc_putref_schemas(
    IXMLDOMDocument3* iface,
    VARIANT var1)
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    HRESULT hr = E_FAIL;
    IXMLDOMSchemaCollection2* new_schema = NULL;

    FIXME("(%p): semi-stub\n", This);
    switch(V_VT(&var1))
    {
    case VT_UNKNOWN:
        hr = IUnknown_QueryInterface(V_UNKNOWN(&var1), &IID_IXMLDOMSchemaCollection, (void**)&new_schema);
        break;

    case VT_DISPATCH:
        hr = IDispatch_QueryInterface(V_DISPATCH(&var1), &IID_IXMLDOMSchemaCollection, (void**)&new_schema);
        break;

    case VT_NULL:
    case VT_EMPTY:
        hr = S_OK;
        break;

    default:
        WARN("Can't get schema from vt %x\n", V_VT(&var1));
    }

    if(SUCCEEDED(hr))
    {
        IXMLDOMSchemaCollection2* old_schema = InterlockedExchangePointer((void**)&This->schema, new_schema);
        if(old_schema) IXMLDOMSchemaCollection2_Release(old_schema);
    }

    return hr;
}

static inline BOOL is_wellformed(xmlDocPtr doc)
{
#ifdef HAVE_XMLDOC_PROPERTIES
    return doc->properties & XML_DOC_WELLFORMED;
#else
    /* Not a full check, but catches the worst violations */
    xmlNodePtr child;
    int root = 0;

    for (child = doc->children; child != NULL; child = child->next)
    {
        switch (child->type)
        {
        case XML_ELEMENT_NODE:
            if (++root > 1)
                return FALSE;
            break;
        case XML_TEXT_NODE:
        case XML_CDATA_SECTION_NODE:
            return FALSE;
            break;
        default:
            break;
        }
    }

    return root == 1;
#endif
}

static void LIBXML2_LOG_CALLBACK validate_error(void* ctx, char const* msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    LIBXML2_CALLBACK_ERR(domdoc_validateNode, msg, ap);
    va_end(ap);
}

static void LIBXML2_LOG_CALLBACK validate_warning(void* ctx, char const* msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    LIBXML2_CALLBACK_WARN(domdoc_validateNode, msg, ap);
    va_end(ap);
}

static HRESULT WINAPI domdoc_validateNode(
    IXMLDOMDocument3* iface,
    IXMLDOMNode* node,
    IXMLDOMParseError** err)
{
    domdoc* This = impl_from_IXMLDOMDocument3(iface);
    LONG state, err_code = 0;
    HRESULT hr = S_OK;
    int validated = 0;

    TRACE("(%p)->(%p, %p)\n", This, node, err);
    domdoc_get_readyState(iface, &state);
    if (state != READYSTATE_COMPLETE)
    {
        if (err)
           *err = create_parseError(err_code, NULL, NULL, NULL, 0, 0, 0);
        return E_PENDING;
    }

    if (!node)
    {
        if (err)
            *err = create_parseError(err_code, NULL, NULL, NULL, 0, 0, 0);
        return E_POINTER;
    }

    if (!get_node_obj(node)->node || get_node_obj(node)->node->doc != get_doc(This))
    {
        if (err)
            *err = create_parseError(err_code, NULL, NULL, NULL, 0, 0, 0);
        return E_FAIL;
    }

    if (!is_wellformed(get_doc(This)))
    {
        ERR("doc not well-formed");
        if (err)
            *err = create_parseError(E_XML_NOTWF, NULL, NULL, NULL, 0, 0, 0);
        return S_FALSE;
    }

    /* DTD validation */
    if (get_doc(This)->intSubset || get_doc(This)->extSubset)
    {
        xmlValidCtxtPtr vctx = xmlNewValidCtxt();
        vctx->error = validate_error;
        vctx->warning = validate_warning;
        ++validated;

        if (!((node == (IXMLDOMNode*)iface)?
              xmlValidateDocument(vctx, get_doc(This)) :
              xmlValidateElement(vctx, get_doc(This), get_node_obj(node)->node)))
        {
            /* TODO: get a real error code here */
            TRACE("DTD validation failed\n");
            err_code = E_XML_INVALID;
            hr = S_FALSE;
        }
        xmlFreeValidCtxt(vctx);
    }

    /* Schema validation */
    if (hr == S_OK && This->schema != NULL)
    {

        hr = SchemaCache_validate_tree(This->schema, get_node_obj(node)->node);
        if (!FAILED(hr))
        {
            ++validated;
            /* TODO: get a real error code here */
            TRACE("schema validation failed\n");
            if (hr != S_OK)
                err_code = E_XML_INVALID;
        }
        else
        {
            /* not really OK, just didn't find a schema for the ns */
            hr = S_OK;
        }
    }

    if (!validated)
    {
        TRACE("no DTD or schema found\n");
        err_code = E_XML_NODTD;
        hr = S_FALSE;
    }

    if (err)
        *err = create_parseError(err_code, NULL, NULL, NULL, 0, 0, 0);

    return hr;
}

static HRESULT WINAPI domdoc_validate(
    IXMLDOMDocument3* iface,
    IXMLDOMParseError** err)
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    TRACE("(%p)->(%p)\n", This, err);
    return domdoc_validateNode(iface, (IXMLDOMNode*)iface, err);
}

static HRESULT WINAPI domdoc_setProperty(
    IXMLDOMDocument3* iface,
    BSTR p,
    VARIANT var)
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );

    TRACE("(%p)->(%s)\n", This, debugstr_w(p));

    if (lstrcmpiW(p, PropertySelectionLanguageW) == 0)
    {
        VARIANT varStr;
        HRESULT hr;
        BSTR bstr;

        V_VT(&varStr) = VT_EMPTY;
        if (V_VT(&var) != VT_BSTR)
        {
            if (FAILED(hr = VariantChangeType(&varStr, &var, 0, VT_BSTR)))
                return hr;
            bstr = V_BSTR(&varStr);
        }
        else
            bstr = V_BSTR(&var);

        hr = S_OK;
        if (lstrcmpiW(bstr, PropValueXPathW) == 0)
            This->properties->XPath = TRUE;
        else if (lstrcmpiW(bstr, PropValueXSLPatternW) == 0)
            This->properties->XPath = FALSE;
        else
            hr = E_FAIL;

        VariantClear(&varStr);
        return hr;
    }
    else if (lstrcmpiW(p, PropertySelectionNamespacesW) == 0)
    {
        VARIANT varStr;
        HRESULT hr;
        BSTR bstr;
        xmlChar *pTokBegin, *pTokEnd, *pTokInner;
        xmlChar *nsStr = (xmlChar*)This->properties->selectNsStr;
        xmlXPathContextPtr ctx;
        struct list *pNsList;
        select_ns_entry* pNsEntry = NULL;

        V_VT(&varStr) = VT_EMPTY;
        if (V_VT(&var) != VT_BSTR)
        {
            if (FAILED(hr = VariantChangeType(&varStr, &var, 0, VT_BSTR)))
                return hr;
            bstr = V_BSTR(&varStr);
        }
        else
            bstr = V_BSTR(&var);

        hr = S_OK;

        pNsList = &(This->properties->selectNsList);
        clear_selectNsList(pNsList);
        heap_free(nsStr);
        nsStr = xmlChar_from_wchar(bstr);


        TRACE("Setting SelectionNamespaces property to: %s\n", nsStr);

        This->properties->selectNsStr = nsStr;
        This->properties->selectNsStr_len = xmlStrlen(nsStr);
        if (bstr && *bstr)
        {
            ctx = xmlXPathNewContext(This->node.node->doc);
            pTokBegin = nsStr;
            pTokEnd = nsStr;
            for (; *pTokBegin; pTokBegin = pTokEnd)
            {
                if (pNsEntry != NULL)
                    memset(pNsEntry, 0, sizeof(select_ns_entry));
                else
                    pNsEntry = heap_alloc_zero(sizeof(select_ns_entry));

                while (*pTokBegin == ' ')
                    ++pTokBegin;
                pTokEnd = pTokBegin;
                while (*pTokEnd != ' ' && *pTokEnd != 0)
                    ++pTokEnd;

                if (xmlStrncmp(pTokBegin, (xmlChar const*)"xmlns", 5) != 0)
                {
                    hr = E_FAIL;
                    WARN("Syntax error in xmlns string: %s\n\tat token: %s\n",
                          wine_dbgstr_w(bstr), wine_dbgstr_an((const char*)pTokBegin, pTokEnd-pTokBegin));
                    continue;
                }

                pTokBegin += 5;
                if (*pTokBegin == '=')
                {
                    /*valid for XSLPattern?*/
                    FIXME("Setting default xmlns not supported - skipping.\n");
                    pTokBegin = pTokEnd;
                    continue;
                }
                else if (*pTokBegin == ':')
                {
                    pNsEntry->prefix = ++pTokBegin;
                    for (pTokInner = pTokBegin; pTokInner != pTokEnd && *pTokInner != '='; ++pTokInner)
                        ;

                    if (pTokInner == pTokEnd)
                    {
                        hr = E_FAIL;
                        WARN("Syntax error in xmlns string: %s\n\tat token: %s\n",
                              wine_dbgstr_w(bstr), wine_dbgstr_an((const char*)pTokBegin, pTokEnd-pTokBegin));
                        continue;
                    }

                    pNsEntry->prefix_end = *pTokInner;
                    *pTokInner = 0;
                    ++pTokInner;

                    if (pTokEnd-pTokInner > 1 &&
                        ((*pTokInner == '\'' && *(pTokEnd-1) == '\'') ||
                         (*pTokInner == '"' && *(pTokEnd-1) == '"')))
                    {
                        pNsEntry->href = ++pTokInner;
                        pNsEntry->href_end = *(pTokEnd-1);
                        *(pTokEnd-1) = 0;
                        list_add_tail(pNsList, &pNsEntry->entry);
                        /*let libxml figure out if they're valid from here ;)*/
                        if (xmlXPathRegisterNs(ctx, pNsEntry->prefix, pNsEntry->href) != 0)
                        {
                            hr = E_FAIL;
                        }
                        pNsEntry = NULL;
                        continue;
                    }
                    else
                    {
                        WARN("Syntax error in xmlns string: %s\n\tat token: %s\n",
                              wine_dbgstr_w(bstr), wine_dbgstr_an((const char*)pTokInner, pTokEnd-pTokInner));
                        list_add_tail(pNsList, &pNsEntry->entry);

                        pNsEntry = NULL;
                        hr = E_FAIL;
                        continue;
                    }
                }
                else
                {
                    hr = E_FAIL;
                    continue;
                }
            }
            heap_free(pNsEntry);
            xmlXPathFreeContext(ctx);
        }

        VariantClear(&varStr);
        return hr;
    }
    else if (lstrcmpiW(p, PropertyProhibitDTDW) == 0 ||
             lstrcmpiW(p, PropertyNewParserW) == 0)
    {
        /* Ignore */
        FIXME("Ignoring property %s, value %d\n", debugstr_w(p), V_BOOL(&var));
        return S_OK;
    }

    FIXME("Unknown property %s\n", wine_dbgstr_w(p));
    return E_FAIL;
}

static HRESULT WINAPI domdoc_getProperty(
    IXMLDOMDocument3* iface,
    BSTR p,
    VARIANT* var)
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );

    TRACE("(%p)->(%p)\n", This, debugstr_w(p));

    if (!var)
        return E_INVALIDARG;

    if (lstrcmpiW(p, PropertySelectionLanguageW) == 0)
    {
        V_VT(var) = VT_BSTR;
        V_BSTR(var) = This->properties->XPath ?
                      SysAllocString(PropValueXPathW) :
                      SysAllocString(PropValueXSLPatternW);
        return V_BSTR(var) ? S_OK : E_OUTOFMEMORY;
    }
    else if (lstrcmpiW(p, PropertySelectionNamespacesW) == 0)
    {
        int lenA, lenW;
        BSTR rebuiltStr, cur;
        const xmlChar *nsStr;
        struct list *pNsList;
        select_ns_entry* pNsEntry;

        V_VT(var) = VT_BSTR;
        nsStr = This->properties->selectNsStr;
        pNsList = &This->properties->selectNsList;
        lenA = This->properties->selectNsStr_len;
        lenW = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)nsStr, lenA+1, NULL, 0);
        rebuiltStr = heap_alloc(lenW*sizeof(WCHAR));
        MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)nsStr, lenA+1, rebuiltStr, lenW);
        cur = rebuiltStr;
        /* this is fine because all of the chars that end tokens are ASCII*/
        LIST_FOR_EACH_ENTRY(pNsEntry, pNsList, select_ns_entry, entry)
        {
            while (*cur != 0) ++cur;
            if (pNsEntry->prefix_end)
            {
                *cur = pNsEntry->prefix_end;
                while (*cur != 0) ++cur;
            }

            if (pNsEntry->href_end)
            {
                *cur = pNsEntry->href_end;
            }
        }
        V_BSTR(var) = SysAllocString(rebuiltStr);
        heap_free(rebuiltStr);
        return S_OK;
    }

    FIXME("Unknown property %s\n", wine_dbgstr_w(p));
    return E_FAIL;
}

static HRESULT WINAPI domdoc_importNode(
    IXMLDOMDocument3* iface,
    IXMLDOMNode* node,
    VARIANT_BOOL deep,
    IXMLDOMNode** clone)
{
    domdoc *This = impl_from_IXMLDOMDocument3( iface );
    FIXME("(%p)->(%p %d %p): stub\n", This, node, deep, clone);
    return E_NOTIMPL;
}

static const struct IXMLDOMDocument3Vtbl domdoc_vtbl =
{
    domdoc_QueryInterface,
    domdoc_AddRef,
    domdoc_Release,
    domdoc_GetTypeInfoCount,
    domdoc_GetTypeInfo,
    domdoc_GetIDsOfNames,
    domdoc_Invoke,
    domdoc_get_nodeName,
    domdoc_get_nodeValue,
    domdoc_put_nodeValue,
    domdoc_get_nodeType,
    domdoc_get_parentNode,
    domdoc_get_childNodes,
    domdoc_get_firstChild,
    domdoc_get_lastChild,
    domdoc_get_previousSibling,
    domdoc_get_nextSibling,
    domdoc_get_attributes,
    domdoc_insertBefore,
    domdoc_replaceChild,
    domdoc_removeChild,
    domdoc_appendChild,
    domdoc_hasChildNodes,
    domdoc_get_ownerDocument,
    domdoc_cloneNode,
    domdoc_get_nodeTypeString,
    domdoc_get_text,
    domdoc_put_text,
    domdoc_get_specified,
    domdoc_get_definition,
    domdoc_get_nodeTypedValue,
    domdoc_put_nodeTypedValue,
    domdoc_get_dataType,
    domdoc_put_dataType,
    domdoc_get_xml,
    domdoc_transformNode,
    domdoc_selectNodes,
    domdoc_selectSingleNode,
    domdoc_get_parsed,
    domdoc_get_namespaceURI,
    domdoc_get_prefix,
    domdoc_get_baseName,
    domdoc_transformNodeToObject,
    domdoc_get_doctype,
    domdoc_get_implementation,
    domdoc_get_documentElement,
    domdoc_put_documentElement,
    domdoc_createElement,
    domdoc_createDocumentFragment,
    domdoc_createTextNode,
    domdoc_createComment,
    domdoc_createCDATASection,
    domdoc_createProcessingInstruction,
    domdoc_createAttribute,
    domdoc_createEntityReference,
    domdoc_getElementsByTagName,
    domdoc_createNode,
    domdoc_nodeFromID,
    domdoc_load,
    domdoc_get_readyState,
    domdoc_get_parseError,
    domdoc_get_url,
    domdoc_get_async,
    domdoc_put_async,
    domdoc_abort,
    domdoc_loadXML,
    domdoc_save,
    domdoc_get_validateOnParse,
    domdoc_put_validateOnParse,
    domdoc_get_resolveExternals,
    domdoc_put_resolveExternals,
    domdoc_get_preserveWhiteSpace,
    domdoc_put_preserveWhiteSpace,
    domdoc_put_onReadyStateChange,
    domdoc_put_onDataAvailable,
    domdoc_put_onTransformNode,
    domdoc_get_namespaces,
    domdoc_get_schemas,
    domdoc_putref_schemas,
    domdoc_validate,
    domdoc_setProperty,
    domdoc_getProperty,
    domdoc_validateNode,
    domdoc_importNode
};

/* IConnectionPointContainer */
static HRESULT WINAPI ConnectionPointContainer_QueryInterface(IConnectionPointContainer *iface,
                                                              REFIID riid, void **ppv)
{
    domdoc *This = impl_from_IConnectionPointContainer(iface);
    return IXMLDOMDocument3_QueryInterface((IXMLDOMDocument3*)This, riid, ppv);
}

static ULONG WINAPI ConnectionPointContainer_AddRef(IConnectionPointContainer *iface)
{
    domdoc *This = impl_from_IConnectionPointContainer(iface);
    return IXMLDOMDocument3_AddRef((IXMLDOMDocument3*)This);
}

static ULONG WINAPI ConnectionPointContainer_Release(IConnectionPointContainer *iface)
{
    domdoc *This = impl_from_IConnectionPointContainer(iface);
    return IXMLDOMDocument3_Release((IXMLDOMDocument3*)This);
}

static HRESULT WINAPI ConnectionPointContainer_EnumConnectionPoints(IConnectionPointContainer *iface,
        IEnumConnectionPoints **ppEnum)
{
    domdoc *This = impl_from_IConnectionPointContainer(iface);
    FIXME("(%p)->(%p): stub\n", This, ppEnum);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConnectionPointContainer_FindConnectionPoint(IConnectionPointContainer *iface,
        REFIID riid, IConnectionPoint **cp)
{
    domdoc *This = impl_from_IConnectionPointContainer(iface);
    ConnectionPoint *iter;

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), cp);

    *cp = NULL;

    for(iter = This->cp_list; iter; iter = iter->next)
    {
        if (IsEqualGUID(iter->iid, riid))
            *cp = (IConnectionPoint*)&iter->lpVtblConnectionPoint;
    }

    if (*cp)
    {
        IConnectionPoint_AddRef(*cp);
        return S_OK;
    }

    FIXME("unsupported riid %s\n", debugstr_guid(riid));
    return CONNECT_E_NOCONNECTION;

}

static const struct IConnectionPointContainerVtbl ConnectionPointContainerVtbl =
{
    ConnectionPointContainer_QueryInterface,
    ConnectionPointContainer_AddRef,
    ConnectionPointContainer_Release,
    ConnectionPointContainer_EnumConnectionPoints,
    ConnectionPointContainer_FindConnectionPoint
};

/* IConnectionPoint */
static HRESULT WINAPI ConnectionPoint_QueryInterface(IConnectionPoint *iface,
                                                     REFIID riid, void **ppv)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv );

    *ppv = NULL;

    if (IsEqualGUID(&IID_IUnknown, riid) ||
        IsEqualGUID(&IID_IConnectionPoint, riid))
    {
        *ppv = iface;
    }

    if (*ppv)
    {
        IConnectionPoint_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ConnectionPoint_AddRef(IConnectionPoint *iface)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);
    return IConnectionPointContainer_AddRef(This->container);
}

static ULONG WINAPI ConnectionPoint_Release(IConnectionPoint *iface)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);
    return IConnectionPointContainer_Release(This->container);
}

static HRESULT WINAPI ConnectionPoint_GetConnectionInterface(IConnectionPoint *iface, IID *iid)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);

    TRACE("(%p)->(%p)\n", This, iid);

    if (!iid) return E_POINTER;

    *iid = *This->iid;
    return S_OK;
}

static HRESULT WINAPI ConnectionPoint_GetConnectionPointContainer(IConnectionPoint *iface,
        IConnectionPointContainer **container)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);

    TRACE("(%p)->(%p)\n", This, container);

    if (!container) return E_POINTER;

    *container = This->container;
    IConnectionPointContainer_AddRef(*container);
    return S_OK;
}

static HRESULT WINAPI ConnectionPoint_Advise(IConnectionPoint *iface, IUnknown *pUnkSink,
                                             DWORD *pdwCookie)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);
    FIXME("(%p)->(%p %p): stub\n", This, pUnkSink, pdwCookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConnectionPoint_Unadvise(IConnectionPoint *iface, DWORD cookie)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);

    TRACE("(%p)->(%d)\n", This, cookie);

    if (cookie == 0 || cookie > This->sinks_size || !This->sinks[cookie-1].unk)
        return CONNECT_E_NOCONNECTION;

    IUnknown_Release(This->sinks[cookie-1].unk);
    This->sinks[cookie-1].unk = NULL;

    return S_OK;
}

static HRESULT WINAPI ConnectionPoint_EnumConnections(IConnectionPoint *iface,
                                                      IEnumConnections **ppEnum)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);
    FIXME("(%p)->(%p): stub\n", This, ppEnum);
    return E_NOTIMPL;
}

static const IConnectionPointVtbl ConnectionPointVtbl =
{
    ConnectionPoint_QueryInterface,
    ConnectionPoint_AddRef,
    ConnectionPoint_Release,
    ConnectionPoint_GetConnectionInterface,
    ConnectionPoint_GetConnectionPointContainer,
    ConnectionPoint_Advise,
    ConnectionPoint_Unadvise,
    ConnectionPoint_EnumConnections
};

void ConnectionPoint_Init(ConnectionPoint *cp, struct domdoc *doc, REFIID riid)
{
    cp->lpVtblConnectionPoint = &ConnectionPointVtbl;
    cp->doc = doc;
    cp->iid = riid;
    cp->sinks = NULL;
    cp->sinks_size = 0;

    cp->next = doc->cp_list;
    doc->cp_list = cp;

    cp->container = (IConnectionPointContainer*)&doc->lpVtblConnectionPointContainer;
}

/* domdoc implementation of IObjectWithSite */
static HRESULT WINAPI
domdoc_ObjectWithSite_QueryInterface( IObjectWithSite* iface, REFIID riid, void** ppvObject )
{
    domdoc *This = impl_from_IObjectWithSite(iface);
    return IXMLDOMDocument3_QueryInterface( (IXMLDOMDocument3 *)This, riid, ppvObject );
}

static ULONG WINAPI domdoc_ObjectWithSite_AddRef( IObjectWithSite* iface )
{
    domdoc *This = impl_from_IObjectWithSite(iface);
    return IXMLDOMDocument3_AddRef((IXMLDOMDocument3 *)This);
}

static ULONG WINAPI domdoc_ObjectWithSite_Release( IObjectWithSite* iface )
{
    domdoc *This = impl_from_IObjectWithSite(iface);
    return IXMLDOMDocument3_Release((IXMLDOMDocument3 *)This);
}

static HRESULT WINAPI domdoc_ObjectWithSite_GetSite( IObjectWithSite *iface, REFIID iid, void **ppvSite )
{
    domdoc *This = impl_from_IObjectWithSite(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid( iid ), ppvSite );

    if ( !This->site )
        return E_FAIL;

    return IUnknown_QueryInterface( This->site, iid, ppvSite );
}

static HRESULT WINAPI domdoc_ObjectWithSite_SetSite( IObjectWithSite *iface, IUnknown *punk )
{
    domdoc *This = impl_from_IObjectWithSite(iface);

    TRACE("(%p)->(%p)\n", iface, punk);

    if(!punk)
    {
        if(This->site)
        {
            IUnknown_Release( This->site );
            This->site = NULL;
        }

        return S_OK;
    }

    IUnknown_AddRef( punk );

    if(This->site)
        IUnknown_Release( This->site );

    This->site = punk;

    return S_OK;
}

static const IObjectWithSiteVtbl domdocObjectSite =
{
    domdoc_ObjectWithSite_QueryInterface,
    domdoc_ObjectWithSite_AddRef,
    domdoc_ObjectWithSite_Release,
    domdoc_ObjectWithSite_SetSite,
    domdoc_ObjectWithSite_GetSite
};

static HRESULT WINAPI domdoc_Safety_QueryInterface(IObjectSafety *iface, REFIID riid, void **ppv)
{
    domdoc *This = impl_from_IObjectSafety(iface);
    return IXMLDOMDocument3_QueryInterface( (IXMLDOMDocument3 *)This, riid, ppv );
}

static ULONG WINAPI domdoc_Safety_AddRef(IObjectSafety *iface)
{
    domdoc *This = impl_from_IObjectSafety(iface);
    return IXMLDOMDocument3_AddRef((IXMLDOMDocument3 *)This);
}

static ULONG WINAPI domdoc_Safety_Release(IObjectSafety *iface)
{
    domdoc *This = impl_from_IObjectSafety(iface);
    return IXMLDOMDocument3_Release((IXMLDOMDocument3 *)This);
}

#define SAFETY_SUPPORTED_OPTIONS (INTERFACESAFE_FOR_UNTRUSTED_CALLER|INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_SECURITY_MANAGER)

static HRESULT WINAPI domdoc_Safety_GetInterfaceSafetyOptions(IObjectSafety *iface, REFIID riid,
        DWORD *supported, DWORD *enabled)
{
    domdoc *This = impl_from_IObjectSafety(iface);

    TRACE("(%p)->(%s %p %p)\n", This, debugstr_guid(riid), supported, enabled);

    if(!supported || !enabled) return E_POINTER;

    *supported = SAFETY_SUPPORTED_OPTIONS;
    *enabled = This->safeopt;

    return S_OK;
}

static HRESULT WINAPI domdoc_Safety_SetInterfaceSafetyOptions(IObjectSafety *iface, REFIID riid,
        DWORD mask, DWORD enabled)
{
    domdoc *This = impl_from_IObjectSafety(iface);
    TRACE("(%p)->(%s %x %x)\n", This, debugstr_guid(riid), mask, enabled);

    if ((mask & ~SAFETY_SUPPORTED_OPTIONS) != 0)
        return E_FAIL;

    This->safeopt = enabled & mask & SAFETY_SUPPORTED_OPTIONS;
    return S_OK;
}

#undef SAFETY_SUPPORTED_OPTIONS

static const IObjectSafetyVtbl domdocObjectSafetyVtbl = {
    domdoc_Safety_QueryInterface,
    domdoc_Safety_AddRef,
    domdoc_Safety_Release,
    domdoc_Safety_GetInterfaceSafetyOptions,
    domdoc_Safety_SetInterfaceSafetyOptions
};

static const tid_t domdoc_iface_tids[] = {
    IXMLDOMNode_tid,
    IXMLDOMDocument_tid,
    IXMLDOMDocument2_tid,
    0
};
static dispex_static_data_t domdoc_dispex = {
    NULL,
    IXMLDOMDocument2_tid,
    NULL,
    domdoc_iface_tids
};

HRESULT DOMDocument_create_from_xmldoc(xmlDocPtr xmldoc, IXMLDOMDocument3 **document)
{
    domdoc *doc;

    doc = heap_alloc( sizeof (*doc) );
    if( !doc )
        return E_OUTOFMEMORY;

    doc->lpVtbl = &domdoc_vtbl;
    doc->lpvtblIPersistStreamInit = &xmldoc_IPersistStreamInit_VTable;
    doc->lpvtblIObjectWithSite = &domdocObjectSite;
    doc->lpvtblIObjectSafety = &domdocObjectSafetyVtbl;
    doc->lpvtblISupportErrorInfo = &support_error_vtbl;
    doc->lpVtblConnectionPointContainer = &ConnectionPointContainerVtbl;
    doc->ref = 1;
    doc->async = VARIANT_TRUE;
    doc->validating = 0;
    doc->resolving = 0;
    doc->properties = properties_from_xmlDocPtr(xmldoc);
    doc->error = S_OK;
    doc->schema = NULL;
    doc->stream = NULL;
    doc->site = NULL;
    doc->safeopt = 0;
    doc->bsc = NULL;
    doc->cp_list = NULL;

    /* events connection points */
    ConnectionPoint_Init(&doc->cp_dispatch, doc, &IID_IDispatch);
    ConnectionPoint_Init(&doc->cp_propnotif, doc, &IID_IPropertyNotifySink);
    ConnectionPoint_Init(&doc->cp_domdocevents, doc, &DIID_XMLDOMDocumentEvents);

    init_xmlnode(&doc->node, (xmlNodePtr)xmldoc, (IXMLDOMNode*)&doc->lpVtbl, &domdoc_dispex);

    *document = (IXMLDOMDocument3*)&doc->lpVtbl;

    TRACE("returning iface %p\n", *document);
    return S_OK;
}

HRESULT DOMDocument_create(const GUID *clsid, IUnknown *pUnkOuter, void **ppObj)
{
    xmlDocPtr xmldoc;
    HRESULT hr;

    TRACE("(%s, %p, %p)\n", debugstr_guid(clsid), pUnkOuter, ppObj);

    xmldoc = xmlNewDoc(NULL);
    if(!xmldoc)
        return E_OUTOFMEMORY;

    xmldoc->_private = create_priv();
    priv_from_xmlDocPtr(xmldoc)->properties = create_properties(clsid);

    hr = DOMDocument_create_from_xmldoc(xmldoc, (IXMLDOMDocument3**)ppObj);
    if(FAILED(hr))
    {
        free_properties(properties_from_xmlDocPtr(xmldoc));
        heap_free(xmldoc->_private);
        xmlFreeDoc(xmldoc);
        return hr;
    }

    return hr;
}

IUnknown* create_domdoc( xmlNodePtr document )
{
    void* pObj = NULL;
    HRESULT hr;

    TRACE("(%p)\n", document);

    hr = DOMDocument_create_from_xmldoc((xmlDocPtr)document, (IXMLDOMDocument3**)&pObj);
    if (FAILED(hr))
        return NULL;

    return pObj;
}

#else

HRESULT DOMDocument_create(const GUID *clsid, IUnknown *pUnkOuter, void **ppObj)
{
    MESSAGE("This program tried to use a DOMDocument object, but\n"
            "libxml2 support was not present at compile time.\n");
    return E_NOTIMPL;
}

#endif
