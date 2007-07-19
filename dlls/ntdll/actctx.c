/*
 * Activation contexts
 *
 * Copyright 2004 Jon Griffiths
 * Copyright 2007 Eric Pouech
 * Copyright 2007 Jacek Caban for CodeWeavers
 * Copyright 2007 Alexandre Julliard
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "ntdll_misc.h"
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(actctx);

#define ACTCTX_FLAGS_ALL (\
 ACTCTX_FLAG_PROCESSOR_ARCHITECTURE_VALID |\
 ACTCTX_FLAG_LANGID_VALID |\
 ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID |\
 ACTCTX_FLAG_RESOURCE_NAME_VALID |\
 ACTCTX_FLAG_SET_PROCESS_DEFAULT |\
 ACTCTX_FLAG_APPLICATION_NAME_VALID |\
 ACTCTX_FLAG_SOURCE_IS_ASSEMBLYREF |\
 ACTCTX_FLAG_HMODULE_VALID )

#define ACTCTX_MAGIC       0xC07E3E11

/* we don't want to include winuser.h */
#define RT_MANIFEST                        ((ULONG_PTR)24)
#define CREATEPROCESS_MANIFEST_RESOURCE_ID ((ULONG_PTR)1)

typedef struct
{
    const char*         ptr;
    unsigned int        len;
} xmlstr_t;

typedef struct
{
    const char*         ptr;
    const char*         end;
} xmlbuf_t;

struct file_info
{
    ULONG               type;
    WCHAR              *info;
};

struct version
{
    USHORT              major;
    USHORT              minor;
    USHORT              build;
    USHORT              revision;
};

enum assembly_id_type
{
    TYPE_NONE,
    TYPE_WIN32
};

struct assembly_identity
{
    WCHAR                *name;
    WCHAR                *arch;
    WCHAR                *public_key;
    WCHAR                *language;
    struct version        version;
    enum assembly_id_type type;
};

struct entity
{
    DWORD kind;
    union
    {
        struct
        {
            WCHAR *tlbid;
            WCHAR *version;
            WCHAR *helpdir;
	} typelib;
        struct
        {
            WCHAR *clsid;
	} comclass;
	struct {
            WCHAR *iid;
            WCHAR *name;
	} proxy;
        struct
        {
            WCHAR *name;
        } class;
        struct
        {
            WCHAR *name;
            WCHAR *clsid;
        } clrclass;
        struct
        {
            WCHAR *name;
            WCHAR *clsid;
        } clrsurrogate;
    } u;
};

struct entity_array
{
    struct entity        *base;
    unsigned int          num;
    unsigned int          allocated;
};

struct dll_redirect
{
    WCHAR                *name;
    WCHAR                *hash;
    struct entity_array   entities;
};

enum assembly_type
{
    APPLICATION_MANIFEST,
    ASSEMBLY_MANIFEST
};

struct assembly
{
    enum assembly_type       type;
    struct assembly_identity id;
    struct file_info         manifest;
    BOOL                     no_inherit;
    struct dll_redirect     *dlls;
    unsigned int             num_dlls;
    unsigned int             allocated_dlls;
    struct entity_array      entities;
};

typedef struct _ACTIVATION_CONTEXT
{
    ULONG               magic;
    int                 ref_count;
    struct file_info    config;
    struct file_info    appdir;
    struct assembly    *assemblies;
    unsigned int        num_assemblies;
    unsigned int        allocated_assemblies;
} ACTIVATION_CONTEXT;

struct actctx_loader
{
    ACTIVATION_CONTEXT       *actctx;
    struct assembly_identity *dependencies;
    unsigned int              num_dependencies;
    unsigned int              allocated_dependencies;
};

#define ASSEMBLY_ELEM                   "assembly"
#define ASSEMBLYIDENTITY_ELEM           "assemblyIdentity"
#define BINDINGREDIRECT_ELEM            "bindingRedirect"
#define CLRCLASS_ELEM                   "clrClass"
#define CLRSURROGATE_ELEM               "clrSurrogate"
#define COMCLASS_ELEM                   "comClass"
#define COMINTERFACEEXTERNALPROXYSTUB_ELEM "comInterfaceExternalProxyStub"
#define COMINTERFACEPROXYSTUB_ELEM      "comInterfaceProxyStub"
#define DEPENDENCY_ELEM                 "dependency"
#define DEPENDENTASSEMBLY_ELEM          "dependentAssembly"
#define DESCRIPTION_ELEM                "description"
#define FILE_ELEM                       "file"
#define HASH_ELEM                       "asmv2:hash"
#define NOINHERIT_ELEM                  "noInherit"
#define NOINHERITABLE_ELEM              "noInheritable"
#define TYPELIB_ELEM                    "typelib"
#define WINDOWCLASS_ELEM                "windowClass"

#define ELEM_END(elem) "/" elem

#define CLSID_ATTR                      "clsid"
#define HASH_ATTR                       "hash"
#define HASHALG_ATTR                    "hashalg"
#define HELPDIR_ATTR                    "helpdir"
#define IID_ATTR                        "iid"
#define LANGUAGE_ATTR                   "language"
#define MANIFESTVERSION_ATTR            "manifestVersion"
#define NAME_ATTR                       "name"
#define NEWVERSION_ATTR                 "newVersion"
#define OLDVERSION_ATTR                 "oldVersion"
#define PROCESSORARCHITECTURE_ATTR      "processorArchitecture"
#define PUBLICKEYTOKEN_ATTR             "publicKeyToken"
#define TLBID_ATTR                      "tlbid"
#define TYPE_ATTR                       "type"
#define VERSION_ATTR                    "version"
#define XMLNS_ATTR                      "xmlns"

#define MANIFEST_NAMESPACE              "urn:schemas-microsoft-com:asm.v1"

static const WCHAR dotManifestW[] = {'.','m','a','n','i','f','e','s','t',0};


static WCHAR *strdupW(const WCHAR* str)
{
    WCHAR*      ptr;

    if (!str) return NULL;
    if (!(ptr = RtlAllocateHeap(GetProcessHeap(), 0, (strlenW(str) + 1) * sizeof(WCHAR))))
        return NULL;
    return strcpyW(ptr, str);
}

static WCHAR *xmlstrdupW(const xmlstr_t* str)
{
    WCHAR *strW;
    int len = wine_utf8_mbstowcs( 0, str->ptr, str->len, NULL, 0 );

    if (len == -1) return NULL;
    if ((strW = RtlAllocateHeap(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR))))
    {
        wine_utf8_mbstowcs( 0, str->ptr, str->len, strW, len );
        strW[len] = 0;
    }
    return strW;
}

static inline BOOL xmlstr_cmp(const xmlstr_t* xmlstr, const char* str)
{
    return !strncmp(xmlstr->ptr, str, xmlstr->len) && !str[xmlstr->len];
}

static inline BOOL isxmlspace( char ch )
{
    return (ch == ' ' || ch == '\r' || ch == '\n' || ch == '\t');
}

static inline const char* debugstr_xmlstr(const xmlstr_t* str)
{
    return debugstr_an(str->ptr, str->len);
}

static struct assembly *add_assembly(ACTIVATION_CONTEXT *actctx, enum assembly_type at)
{
    struct assembly *assembly;

    if (actctx->num_assemblies == actctx->allocated_assemblies)
    {
        void *ptr;
        unsigned int new_count;
        if (actctx->assemblies)
        {
            new_count = actctx->allocated_assemblies * 2;
            ptr = RtlReAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                     actctx->assemblies, new_count * sizeof(*assembly) );
        }
        else
        {
            new_count = 4;
            ptr = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, new_count * sizeof(*assembly) );
        }
        if (!ptr) return NULL;
        actctx->assemblies = ptr;
        actctx->allocated_assemblies = new_count;
    }

    assembly = &actctx->assemblies[actctx->num_assemblies++];
    assembly->type = at;
    return assembly;
}

static struct dll_redirect* add_dll_redirect(struct assembly* assembly)
{
    if (assembly->num_dlls == assembly->allocated_dlls)
    {
        void *ptr;
        unsigned int new_count;
        if (assembly->dlls)
        {
            new_count = assembly->allocated_dlls * 2;
            ptr = RtlReAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                     assembly->dlls, new_count * sizeof(*assembly->dlls) );
        }
        else
        {
            new_count = 4;
            ptr = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, new_count * sizeof(*assembly->dlls) );
        }
        if (!ptr) return NULL;
        assembly->dlls = ptr;
        assembly->allocated_dlls = new_count;
    }
    return &assembly->dlls[assembly->num_dlls++];
}

static void free_assembly_identity(struct assembly_identity *ai)
{
    RtlFreeHeap( GetProcessHeap(), 0, ai->name );
    RtlFreeHeap( GetProcessHeap(), 0, ai->arch );
    RtlFreeHeap( GetProcessHeap(), 0, ai->public_key );
    RtlFreeHeap( GetProcessHeap(), 0, ai->language );
}

static struct entity* add_entity(struct entity_array *array, DWORD kind)
{
    struct entity*      entity;

    if (array->num == array->allocated)
    {
        void *ptr;
        unsigned int new_count;
        if (array->base)
        {
            new_count = array->allocated * 2;
            ptr = RtlReAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                     array->base, new_count * sizeof(*array->base) );
        }
        else
        {
            new_count = 4;
            ptr = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, new_count * sizeof(*array->base) );
        }
        if (!ptr) return NULL;
        array->base = ptr;
        array->allocated = new_count;
    }
    entity = &array->base[array->num++];
    entity->kind = kind;
    return entity;
}

static void free_entity_array(struct entity_array *array)
{
    unsigned int i;
    for (i = 0; i < array->num; i++)
    {
        struct entity *entity = &array->base[i];
        switch (entity->kind)
        {
        case ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION:
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.comclass.clsid);
            break;
        case ACTIVATION_CONTEXT_SECTION_COM_INTERFACE_REDIRECTION:
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.proxy.iid);
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.proxy.name);
            break;
        case ACTIVATION_CONTEXT_SECTION_COM_TYPE_LIBRARY_REDIRECTION:
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.typelib.tlbid);
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.typelib.version);
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.typelib.helpdir);
            break;
        case ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION:
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.class.name);
            break;
        case ACTIVATION_CONTEXT_SECTION_COM_PROGID_REDIRECTION:
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.clrclass.name);
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.clrclass.clsid);
            break;
        case ACTIVATION_CONTEXT_SECTION_CLR_SURROGATES:
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.clrsurrogate.name);
            RtlFreeHeap(GetProcessHeap(), 0, entity->u.clrsurrogate.clsid);
            break;
        default:
            FIXME("Unknown entity kind %d\n", entity->kind);
        }
    }
    RtlFreeHeap( GetProcessHeap(), 0, array->base );
}

static BOOL add_dependent_assembly_id(struct actctx_loader* acl,
                                      struct assembly_identity* ai)
{
    /* FIXME: should check that the passed ai isn't already in the list */
    if (acl->num_dependencies == acl->allocated_dependencies)
    {
        void *ptr;
        unsigned int new_count;
        if (acl->dependencies)
        {
            new_count = acl->allocated_dependencies * 2;
            ptr = RtlReAllocateHeap(GetProcessHeap(), 0, acl->dependencies,
                                    new_count * sizeof(acl->dependencies[0]));
        }
        else
        {
            new_count = 4;
            ptr = RtlAllocateHeap(GetProcessHeap(), 0, new_count * sizeof(acl->dependencies[0]));
        }
        if (!ptr) return FALSE;
        acl->dependencies = ptr;
        acl->allocated_dependencies = new_count;
    }
    acl->dependencies[acl->num_dependencies++] = *ai;

    return TRUE;
}

static void free_depend_manifests(struct actctx_loader* acl)
{
    unsigned int i;
    for (i = 0; i < acl->num_dependencies; i++)
        free_assembly_identity(&acl->dependencies[i]);
    RtlFreeHeap(GetProcessHeap(), 0, acl->dependencies);
}

static ACTIVATION_CONTEXT *check_actctx( HANDLE h )
{
    ACTIVATION_CONTEXT *actctx = h;

    if (!h || h == INVALID_HANDLE_VALUE) return NULL;
    switch (actctx->magic)
    {
    case ACTCTX_MAGIC:
        return actctx;
    default:
        return NULL;
    }
}

static inline void actctx_addref( ACTIVATION_CONTEXT *actctx )
{
    interlocked_xchg_add( &actctx->ref_count, 1 );
}

static void actctx_release( ACTIVATION_CONTEXT *actctx )
{
    if (interlocked_xchg_add( &actctx->ref_count, -1 ) == 1)
    {
        unsigned int i, j;

        for (i = 0; i < actctx->num_assemblies; i++)
        {
            struct assembly *assembly = &actctx->assemblies[i];
            for (j = 0; j < assembly->num_dlls; j++)
            {
                struct dll_redirect *dll = &assembly->dlls[j];
                free_entity_array( &dll->entities );
                RtlFreeHeap( GetProcessHeap(), 0, dll->name );
                RtlFreeHeap( GetProcessHeap(), 0, dll->hash );
            }
            RtlFreeHeap( GetProcessHeap(), 0, assembly->dlls );
            RtlFreeHeap( GetProcessHeap(), 0, assembly->manifest.info );
            free_entity_array( &assembly->entities );
            free_assembly_identity(&assembly->id);
        }
        RtlFreeHeap( GetProcessHeap(), 0, actctx->config.info );
        RtlFreeHeap( GetProcessHeap(), 0, actctx->appdir.info );
        RtlFreeHeap( GetProcessHeap(), 0, actctx->assemblies );
        actctx->magic = 0;
        RtlFreeHeap( GetProcessHeap(), 0, actctx );
    }
}

static BOOL next_xml_attr(xmlbuf_t* xmlbuf, xmlstr_t* name, xmlstr_t* value,
                          BOOL* error, BOOL* end)
{
    const char* ptr;

    *error = TRUE;

    while (xmlbuf->ptr < xmlbuf->end && isxmlspace(*xmlbuf->ptr))
        xmlbuf->ptr++;

    if (xmlbuf->ptr == xmlbuf->end) return FALSE;

    if (*xmlbuf->ptr == '/')
    {
        xmlbuf->ptr++;
        if (xmlbuf->ptr == xmlbuf->end || *xmlbuf->ptr != '>')
            return FALSE;

        xmlbuf->ptr++;
        *end = TRUE;
        *error = FALSE;
        return FALSE;
    }

    if (*xmlbuf->ptr == '>')
    {
        xmlbuf->ptr++;
        *error = FALSE;
        return FALSE;
    }

    ptr = xmlbuf->ptr;
    while (ptr < xmlbuf->end && *ptr != '=' && *ptr != '>' && !isxmlspace(*ptr)) ptr++;

    if (ptr == xmlbuf->end || *ptr != '=') return FALSE;

    name->ptr = xmlbuf->ptr;
    name->len = ptr-xmlbuf->ptr;
    xmlbuf->ptr = ptr;

    ptr++;
    if (ptr == xmlbuf->end || *ptr != '\"') return FALSE;

    value->ptr = ++ptr;
    if (ptr == xmlbuf->end) return FALSE;

    ptr = memchr(ptr, '\"', xmlbuf->end - ptr);
    if (!ptr)
    {
        xmlbuf->ptr = xmlbuf->end;
        return FALSE;
    }

    value->len = ptr - value->ptr;
    xmlbuf->ptr = ptr + 1;

    if (xmlbuf->ptr == xmlbuf->end) return FALSE;

    *error = FALSE;
    return TRUE;
}

static BOOL next_xml_elem(xmlbuf_t* xmlbuf, xmlstr_t* elem)
{
    const char* ptr;

    ptr = memchr(xmlbuf->ptr, '<', xmlbuf->end - xmlbuf->ptr);
    if (!ptr)
    {
        xmlbuf->ptr = xmlbuf->end;
        return FALSE;
    }

    xmlbuf->ptr = ++ptr;
    while (ptr < xmlbuf->end && !isxmlspace(*ptr) && *ptr != '>')
        ptr++;

    elem->ptr = xmlbuf->ptr;
    elem->len = ptr - xmlbuf->ptr;
    xmlbuf->ptr = ptr;
    return xmlbuf->ptr != xmlbuf->end;
}

static BOOL parse_xml_header(xmlbuf_t* xmlbuf)
{
    /* FIXME: parse attributes */
    const char *ptr;

    for (ptr = xmlbuf->ptr; ptr < xmlbuf->end - 1; ptr++)
    {
        if (ptr[0] == '?' && ptr[1] == '>')
        {
            xmlbuf->ptr = ptr + 2;
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL parse_text_content(xmlbuf_t* xmlbuf, xmlstr_t* content)
{
    const char *ptr = memchr(xmlbuf->ptr, '<', xmlbuf->end - xmlbuf->ptr);

    if (!ptr) return FALSE;

    content->ptr = xmlbuf->ptr;
    content->len = ptr - xmlbuf->ptr;
    xmlbuf->ptr = ptr;

    return TRUE;
}

static BOOL parse_version(const xmlstr_t *str, struct version *version)
{
    unsigned int ver[4];
    unsigned int pos;
    const char *curr;

    /* major.minor.build.revision */
    ver[0] = ver[1] = ver[2] = ver[3] = pos = 0;
    for (curr = str->ptr; curr < str->ptr + str->len; curr++)
    {
        if (*curr >= '0' && *curr <= '9')
        {
            ver[pos] = ver[pos] * 10 + *curr - '0';
            if (ver[pos] >= 0x10000) goto error;
        }
        else if (*curr == '.')
        {
            if (++pos >= 4) goto error;
        }
        else goto error;
    }
    version->major = ver[0];
    version->minor = ver[1];
    version->build = ver[2];
    version->revision = ver[3];
    return TRUE;

error:
    FIXME( "Wrong version definition in manifest file (%s)\n", debugstr_xmlstr(str) );
    return FALSE;
}

static BOOL parse_expect_elem(xmlbuf_t* xmlbuf, const char* name)
{
    xmlstr_t    elem;
    return next_xml_elem(xmlbuf, &elem) && xmlstr_cmp(&elem, name);
}

static BOOL parse_expect_no_attr(xmlbuf_t* xmlbuf, BOOL* end)
{
    xmlstr_t    attr_name, attr_value;
    BOOL        error;

    if (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, end))
    {
        WARN("unexpected attr %s=%s\n", debugstr_xmlstr(&attr_name),
             debugstr_xmlstr(&attr_value));
        return FALSE;
    }

    return !error;
}

static BOOL parse_end_element(xmlbuf_t *xmlbuf)
{
    BOOL end = FALSE;
    return parse_expect_no_attr(xmlbuf, &end) && !end;
}

static BOOL parse_unknown_elem(xmlbuf_t *xmlbuf, const char *name)
{
    xmlstr_t attr_name, attr_value, elem;
    BOOL end = FALSE, error, ret = TRUE;

    while(next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end));
    if(error || end) return end;

    while(ret && (ret = next_xml_elem(xmlbuf, &elem)))
    {
        if(*elem.ptr == '/' && !strncmp(elem.ptr+1, name, elem.len-1))
            break;
        else
            ret = parse_unknown_elem(xmlbuf, elem.ptr);
    }

    return ret && parse_end_element(xmlbuf);
}

static BOOL parse_assembly_identity_elem(xmlbuf_t* xmlbuf, ACTIVATION_CONTEXT* actctx,
                                         struct assembly_identity* ai)
{
    xmlstr_t    attr_name, attr_value;
    BOOL        end = FALSE, error;

    TRACE("\n");

    memset(ai, 0, sizeof(*ai));
    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        if (xmlstr_cmp(&attr_name, NAME_ATTR))
        {
            if (!(ai->name = xmlstrdupW(&attr_value))) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, TYPE_ATTR))
        {
            if (!xmlstr_cmp(&attr_value, "win32"))
            {
                WARN("wrong type attr %s\n", debugstr_xmlstr(&attr_value));
                return FALSE;
            }
            ai->type = TYPE_WIN32;
        }
        else if (xmlstr_cmp(&attr_name, VERSION_ATTR))
        {
            if (!parse_version(&attr_value, &ai->version)) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, PROCESSORARCHITECTURE_ATTR))
        {
            if (!(ai->arch = xmlstrdupW(&attr_value))) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, PUBLICKEYTOKEN_ATTR))
        {
            if (!(ai->public_key = xmlstrdupW(&attr_value))) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, LANGUAGE_ATTR))
        {
            WARN("Unsupported yet language attribute (%s)\n",
                 debugstr_xmlstr(&attr_value));
            if (!(ai->language = xmlstrdupW(&attr_value))) return FALSE;
        }
        else
        {
            WARN("unknown attr %s=%s\n", debugstr_xmlstr(&attr_name),
                 debugstr_xmlstr(&attr_value));
            return FALSE;
        }
    }

    if (error || end) return end;
    return parse_expect_elem(xmlbuf, ELEM_END(ASSEMBLYIDENTITY_ELEM)) && parse_end_element(xmlbuf);
}

static BOOL parse_com_class_elem(xmlbuf_t* xmlbuf, struct dll_redirect* dll)
{
    xmlstr_t    attr_name, attr_value;
    BOOL        end = FALSE, error;
    struct entity*      entity;

    entity = add_entity(&dll->entities, ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION);

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        if (xmlstr_cmp(&attr_name, CLSID_ATTR))
        {
            if (!(entity->u.comclass.clsid = xmlstrdupW(&attr_value))) return FALSE;
        }
        else
        {
            WARN("wrong attr %s=%s\n", debugstr_xmlstr(&attr_name),
                 debugstr_xmlstr(&attr_value));
            return FALSE;
        }
    }

    if (error || end) return end;
    return parse_expect_elem(xmlbuf, ELEM_END(COMCLASS_ELEM)) && parse_end_element(xmlbuf);
}

static BOOL parse_cominterface_proxy_stub_elem(xmlbuf_t* xmlbuf, struct dll_redirect* dll)
{
    xmlstr_t    attr_name, attr_value;
    BOOL        end = FALSE, error;
    struct entity*      entity;

    entity = add_entity(&dll->entities, ACTIVATION_CONTEXT_SECTION_COM_INTERFACE_REDIRECTION);

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        if (xmlstr_cmp(&attr_name, IID_ATTR))
        {
            if (!(entity->u.proxy.iid = xmlstrdupW(&attr_value))) return FALSE;
        }
        if (xmlstr_cmp(&attr_name, NAME_ATTR))
        {
            if (!(entity->u.proxy.name = xmlstrdupW(&attr_value))) return FALSE;
        }
        else
        {
            WARN("wrong attr %s=%s\n", debugstr_xmlstr(&attr_name),
                 debugstr_xmlstr(&attr_value));
            return FALSE;
        }
    }

    if (error || end) return end;
    return parse_expect_elem(xmlbuf, ELEM_END(COMINTERFACEPROXYSTUB_ELEM)) && parse_end_element(xmlbuf);
}

static BOOL parse_typelib_elem(xmlbuf_t* xmlbuf, struct dll_redirect* dll)
{
    xmlstr_t    attr_name, attr_value;
    BOOL        end = FALSE, error;
    struct entity*      entity;

    entity = add_entity(&dll->entities, ACTIVATION_CONTEXT_SECTION_COM_TYPE_LIBRARY_REDIRECTION);

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        if (xmlstr_cmp(&attr_name, TLBID_ATTR))
        {
            if (!(entity->u.typelib.tlbid = xmlstrdupW(&attr_value))) return FALSE;
        }
        if (xmlstr_cmp(&attr_name, VERSION_ATTR))
        {
            if (!(entity->u.typelib.version = xmlstrdupW(&attr_value))) return FALSE;
        }
        if (xmlstr_cmp(&attr_name, HELPDIR_ATTR))
        {
            if (!(entity->u.typelib.helpdir = xmlstrdupW(&attr_value))) return FALSE;
        }
        else
        {
            WARN("wrong attr %s=%s\n", debugstr_xmlstr(&attr_name),
                 debugstr_xmlstr(&attr_value));
            return FALSE;
        }
    }

    if (error || end) return end;
    return parse_expect_elem(xmlbuf, ELEM_END(TYPELIB_ELEM)) && parse_end_element(xmlbuf);
}

static BOOL parse_window_class_elem(xmlbuf_t* xmlbuf, struct dll_redirect* dll)
{
    xmlstr_t    elem, content;
    BOOL        end = FALSE, ret = TRUE;
    struct entity*      entity;

    entity = add_entity(&dll->entities, ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION);

    if (!parse_expect_no_attr(xmlbuf, &end)) return FALSE;
    if (end) return FALSE;

    if (!parse_text_content(xmlbuf, &content)) return FALSE;

    if (!(entity->u.class.name = xmlstrdupW(&content))) return FALSE;

    while (ret && (ret = next_xml_elem(xmlbuf, &elem)))
    {
        if (xmlstr_cmp(&elem, ELEM_END(WINDOWCLASS_ELEM)))
        {
            ret = parse_end_element(xmlbuf);
            break;
        }
        else
        {
            WARN("wrong elem %s\n", debugstr_xmlstr(&elem));
            ret = FALSE;
        }
    }

    return ret;
}

static BOOL parse_binding_redirect_elem(xmlbuf_t* xmlbuf)
{
    xmlstr_t    attr_name, attr_value;
    BOOL        end = FALSE, error;

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        if (xmlstr_cmp(&attr_name, OLDVERSION_ATTR))
        {
            FIXME("Not stored yet oldVersion=%s\n", debugstr_xmlstr(&attr_value));
        }
        else if (xmlstr_cmp(&attr_name, NEWVERSION_ATTR))
        {
            FIXME("Not stored yet newVersion=%s\n", debugstr_xmlstr(&attr_value));
        }
        else
        {
            WARN("wrong attr %s=%s\n", debugstr_xmlstr(&attr_name),
                 debugstr_xmlstr(&attr_value));
            return FALSE;
        }
    }

    if (error || end) return end;
    return parse_expect_elem(xmlbuf, ELEM_END(BINDINGREDIRECT_ELEM)) && parse_end_element(xmlbuf);
}

static BOOL parse_description_elem(xmlbuf_t* xmlbuf)
{
    xmlstr_t    elem, content;
    BOOL        end = FALSE, ret = TRUE;

    if (!parse_expect_no_attr(xmlbuf, &end) || end ||
        !parse_text_content(xmlbuf, &content))
        return FALSE;

    TRACE("Got description %s\n", debugstr_xmlstr(&content));

    while (ret && (ret = next_xml_elem(xmlbuf, &elem)))
    {
        if (xmlstr_cmp(&elem, ELEM_END(DESCRIPTION_ELEM)))
        {
            ret = parse_end_element(xmlbuf);
            break;
        }
        else
        {
            WARN("wrong elem %s\n", debugstr_xmlstr(&elem));
            ret = FALSE;
        }
    }

    return ret;
}

static BOOL parse_com_interface_external_proxy_stub_elem(xmlbuf_t* xmlbuf,
                                                         struct assembly* assembly)
{
    xmlstr_t            attr_name, attr_value;
    BOOL                end = FALSE, error;
    struct entity*      entity;

    entity = add_entity(&assembly->entities, ACTIVATION_CONTEXT_SECTION_COM_INTERFACE_REDIRECTION);
    if (!entity) return FALSE;

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        if (xmlstr_cmp(&attr_name, IID_ATTR))
        {
            if (!(entity->u.proxy.iid = xmlstrdupW(&attr_value))) return FALSE;
        }
        if (xmlstr_cmp(&attr_name, NAME_ATTR))
        {
            if (!(entity->u.proxy.name = xmlstrdupW(&attr_value))) return FALSE;
        }
        else
        {
            WARN("wrong attr %s=%s\n", debugstr_xmlstr(&attr_name),
                 debugstr_xmlstr(&attr_value));
            return FALSE;
        }
    }

    if (error || end) return end;
    return parse_expect_elem(xmlbuf, ELEM_END(COMINTERFACEEXTERNALPROXYSTUB_ELEM)) &&
        parse_end_element(xmlbuf);
}

static BOOL parse_clr_class_elem(xmlbuf_t* xmlbuf, struct assembly* assembly)
{
    xmlstr_t    attr_name, attr_value;
    BOOL        end = FALSE, error;
    struct entity*      entity;

    entity = add_entity(&assembly->entities, ACTIVATION_CONTEXT_SECTION_COM_PROGID_REDIRECTION);
    if (!entity) return FALSE;

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        if (xmlstr_cmp(&attr_name, NAME_ATTR))
        {
            if (!(entity->u.clrclass.name = xmlstrdupW(&attr_value))) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, CLSID_ATTR))
        {
            if (!(entity->u.clrclass.clsid = xmlstrdupW(&attr_value))) return FALSE;
        }
        else
        {
            WARN("wrong attr %s=%s\n", debugstr_xmlstr(&attr_name),
                 debugstr_xmlstr(&attr_value));
            return FALSE;
        }
    }

    if (error || end) return end;
    return parse_expect_elem(xmlbuf, ELEM_END(CLRCLASS_ELEM)) && parse_end_element(xmlbuf);
}

static BOOL parse_clr_surrogate_elem(xmlbuf_t* xmlbuf, struct assembly* assembly)
{
    xmlstr_t    attr_name, attr_value;
    BOOL        end = FALSE, error;
    struct entity*      entity;

    entity = add_entity(&assembly->entities, ACTIVATION_CONTEXT_SECTION_CLR_SURROGATES);
    if (!entity) return FALSE;

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        if (xmlstr_cmp(&attr_name, NAME_ATTR))
        {
            if (!(entity->u.clrsurrogate.name = xmlstrdupW(&attr_value))) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, CLSID_ATTR))
        {
            if (!(entity->u.clrsurrogate.clsid = xmlstrdupW(&attr_value))) return FALSE;
        }
        else
        {
            WARN("wrong attr %s=%s\n", debugstr_xmlstr(&attr_name),
                 debugstr_xmlstr(&attr_value));
            return FALSE;
        }
    }

    if (error || end) return end;
    return parse_expect_elem(xmlbuf, ELEM_END(CLRSURROGATE_ELEM)) && parse_end_element(xmlbuf);
}

static BOOL parse_dependent_assembly_elem(xmlbuf_t* xmlbuf,
                                          struct actctx_loader* acl)
{
    struct assembly_identity    ai;
    xmlstr_t                    elem;
    BOOL                        end = FALSE, ret = TRUE;

    TRACE("\n");

    if (!parse_expect_no_attr(xmlbuf, &end) || end) return end;

    if (!parse_expect_elem(xmlbuf, ASSEMBLYIDENTITY_ELEM) ||
        !parse_assembly_identity_elem(xmlbuf, acl->actctx, &ai))
        return FALSE;

    /* store the newly found identity for later loading */
    if (!add_dependent_assembly_id(acl, &ai)) return FALSE;

    while (ret && (ret = next_xml_elem(xmlbuf, &elem)))
    {
        if (xmlstr_cmp(&elem, ELEM_END(DEPENDENTASSEMBLY_ELEM)))
        {
            ret = parse_end_element(xmlbuf);
            break;
        }
        else if (xmlstr_cmp(&elem, BINDINGREDIRECT_ELEM))
        {
            ret = parse_binding_redirect_elem(xmlbuf);
        }
        else
        {
            WARN("wrong elem %s\n", debugstr_xmlstr(&elem));
            ret = FALSE;
        }
    }

    return ret;
}

static BOOL parse_dependency_elem(xmlbuf_t* xmlbuf, struct actctx_loader* acl)
{
    xmlstr_t elem;
    BOOL end = FALSE, ret = TRUE;

    TRACE("\n");

    if (!parse_expect_no_attr(xmlbuf, &end) || end) return end;

    while (ret && (ret = next_xml_elem(xmlbuf, &elem)))
    {
        if (xmlstr_cmp(&elem, ELEM_END(DEPENDENCY_ELEM)))
        {
            ret = parse_end_element(xmlbuf);
            break;
        }
        else if (xmlstr_cmp(&elem, DEPENDENTASSEMBLY_ELEM))
        {
            ret = parse_dependent_assembly_elem(xmlbuf, acl);
        }
        else
        {
            WARN("wrong element %s\n", debugstr_xmlstr(&elem));
            ret = FALSE;
        }
    }

    return ret;
}

static BOOL parse_noinherit_elem(xmlbuf_t* xmlbuf)
{
    BOOL end = FALSE;

    if (!parse_expect_no_attr(xmlbuf, &end)) return FALSE;
    return end ||
        (parse_expect_elem(xmlbuf, ELEM_END(NOINHERIT_ELEM)) && parse_end_element(xmlbuf));
}

static BOOL parse_noinheritable_elem(xmlbuf_t* xmlbuf)
{
    BOOL end = FALSE;

    if (!parse_expect_no_attr(xmlbuf, &end)) return FALSE;
    return end ||
        (parse_expect_elem(xmlbuf, ELEM_END(NOINHERITABLE_ELEM)) && parse_end_element(xmlbuf));
}

static BOOL parse_file_elem(xmlbuf_t* xmlbuf, struct assembly* assembly)
{
    xmlstr_t    attr_name, attr_value, elem;
    BOOL        end = FALSE, error, ret = TRUE;
    struct dll_redirect* dll;

    if (!(dll = add_dll_redirect(assembly))) return FALSE;

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        if (xmlstr_cmp(&attr_name, NAME_ATTR))
        {
            if (!(dll->name = xmlstrdupW(&attr_value))) return FALSE;
            TRACE("name=%s\n", debugstr_xmlstr(&attr_value));
        }
        else if (xmlstr_cmp(&attr_name, HASH_ATTR))
        {
            if (!(dll->hash = xmlstrdupW(&attr_value))) return FALSE;
        }
        else if (xmlstr_cmp(&attr_name, HASHALG_ATTR))
        {
            if (!xmlstr_cmp(&attr_value, "SHA1"))
                FIXME("hashalg should be SHA1, got %s\n", debugstr_xmlstr(&attr_value));
        }
        else
        {
            WARN("wrong attr %s=%s\n", debugstr_xmlstr(&attr_name),
                 debugstr_xmlstr(&attr_value));
            return FALSE;
        }
    }

    if (error || !dll->name) return FALSE;
    if (end) return TRUE;

    while (ret && (ret = next_xml_elem(xmlbuf, &elem)))
    {
        if (xmlstr_cmp(&elem, ELEM_END(FILE_ELEM)))
        {
            ret = parse_end_element(xmlbuf);
            break;
        }
        else if (xmlstr_cmp(&elem, COMCLASS_ELEM))
        {
            ret = parse_com_class_elem(xmlbuf, dll);
        }
        else if (xmlstr_cmp(&elem, COMINTERFACEPROXYSTUB_ELEM))
        {
            ret = parse_cominterface_proxy_stub_elem(xmlbuf, dll);
        }
        else if (xmlstr_cmp(&elem, HASH_ELEM))
        {
            WARN(HASH_ELEM " (undocumented) not supported\n");
            ret = parse_unknown_elem(xmlbuf, HASH_ELEM);
        }
        else if (xmlstr_cmp(&elem, TYPELIB_ELEM))
        {
            ret = parse_typelib_elem(xmlbuf, dll);
        }
        else if (xmlstr_cmp(&elem, WINDOWCLASS_ELEM))
        {
            ret = parse_window_class_elem(xmlbuf, dll);
        }
        else
        {
            WARN("wrong elem %s\n", debugstr_xmlstr(&elem));
            ret = FALSE;
        }
    }

    return ret;
}

static BOOL parse_assembly_elem(xmlbuf_t* xmlbuf, struct actctx_loader* acl,
                                struct assembly* assembly,
                                struct assembly_identity* expected_ai)
{
    xmlstr_t    attr_name, attr_value, elem;
    BOOL        end = FALSE, error, version = FALSE, xmlns = FALSE, ret = TRUE;
    struct assembly_identity ai;

    TRACE("(%p)\n", xmlbuf);

    while (next_xml_attr(xmlbuf, &attr_name, &attr_value, &error, &end))
    {
        if (xmlstr_cmp(&attr_name, MANIFESTVERSION_ATTR))
        {
            if (!xmlstr_cmp(&attr_value, "1.0"))
            {
                WARN("wrong version %s\n", debugstr_xmlstr(&attr_value));
                return FALSE;
            }
            version = TRUE;
        }
        else if (xmlstr_cmp(&attr_name, XMLNS_ATTR))
        {
            if (!xmlstr_cmp(&attr_value, MANIFEST_NAMESPACE))
            {
                WARN("wrong namespace %s\n", debugstr_xmlstr(&attr_value));
                return FALSE;
            }
            xmlns = TRUE;
        }
        else
        {
            WARN("wrong attr %s=%s\n", debugstr_xmlstr(&attr_name),
                 debugstr_xmlstr(&attr_value));
            return FALSE;
        }
    }

    if (error || end || !xmlns || !version) return FALSE;
    if (!next_xml_elem(xmlbuf, &elem)) return FALSE;

    if (assembly->type == APPLICATION_MANIFEST && xmlstr_cmp(&elem, NOINHERIT_ELEM))
    {
        if (!parse_noinherit_elem(xmlbuf) || !next_xml_elem(xmlbuf, &elem))
            return FALSE;
        assembly->no_inherit = TRUE;
    }

    if (xmlstr_cmp(&elem, NOINHERITABLE_ELEM))
    {
        if (!parse_noinheritable_elem(xmlbuf) || !next_xml_elem(xmlbuf, &elem))
            return FALSE;
    }
    else if (assembly->type == ASSEMBLY_MANIFEST && assembly->no_inherit)
        return FALSE;

    if (!xmlstr_cmp(&elem, ASSEMBLYIDENTITY_ELEM))
    {
        WARN("expected assemblyIdentity element, got %s\n", debugstr_xmlstr(&elem));
        return FALSE;
    }

    if (!parse_assembly_identity_elem(xmlbuf, acl->actctx, &ai)) return FALSE;

    if (expected_ai)
    {
        /* FIXME: more tests */
        if (assembly->type == ASSEMBLY_MANIFEST &&
            memcmp(&ai.version, &expected_ai->version, sizeof(ai.version)))
        {
            WARN("wrong version\n");
            return FALSE;
        }
    }

    assembly->id = ai;

    while (ret && (ret = next_xml_elem(xmlbuf, &elem)))
    {
        if (xmlstr_cmp(&elem, ELEM_END(ASSEMBLY_ELEM)))
        {
            ret = parse_end_element(xmlbuf);
            break;
        }
        else if (xmlstr_cmp(&elem, DESCRIPTION_ELEM))
        {
            ret = parse_description_elem(xmlbuf);
        }
        else if (xmlstr_cmp(&elem, COMINTERFACEEXTERNALPROXYSTUB_ELEM))
        {
            ret = parse_com_interface_external_proxy_stub_elem(xmlbuf, assembly);
        }
        else if (xmlstr_cmp(&elem, DEPENDENCY_ELEM))
        {
            ret = parse_dependency_elem(xmlbuf, acl);
        }
        else if (xmlstr_cmp(&elem, FILE_ELEM))
        {
            ret = parse_file_elem(xmlbuf, assembly);
        }
        else if (xmlstr_cmp(&elem, CLRCLASS_ELEM))
        {
            ret = parse_clr_class_elem(xmlbuf, assembly);
        }
        else if (xmlstr_cmp(&elem, CLRSURROGATE_ELEM))
        {
            ret = parse_clr_surrogate_elem(xmlbuf, assembly);
        }
        else
        {
            WARN("wrong element %s\n", debugstr_xmlstr(&elem));
            ret = FALSE;
        }
    }

    return ret;
}

static NTSTATUS parse_manifest( struct actctx_loader* acl, struct assembly_identity* ai,
                                LPCWSTR filename, xmlbuf_t* xmlbuf )
{
    xmlstr_t            elem;
    struct assembly*    assembly;

    TRACE( "parsing manifest loaded from %s\n", debugstr_w(filename) );

    if (!(assembly = add_assembly(acl->actctx, ASSEMBLY_MANIFEST)))
        return STATUS_SXS_CANT_GEN_ACTCTX;

    if (filename) assembly->manifest.info = strdupW( filename + 4 /* skip \??\ prefix */ );
    assembly->manifest.type = assembly->manifest.info ? ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE
                                                      : ACTIVATION_CONTEXT_PATH_TYPE_NONE;

    if (!next_xml_elem(xmlbuf, &elem)) return STATUS_SXS_CANT_GEN_ACTCTX;

    if (xmlstr_cmp(&elem, "?xml") &&
        (!parse_xml_header(xmlbuf) || !next_xml_elem(xmlbuf, &elem)))
        return STATUS_SXS_CANT_GEN_ACTCTX;

    if (!xmlstr_cmp(&elem, ASSEMBLY_ELEM))
    {
        WARN("root element is %s, not <assembly>\n", debugstr_xmlstr(&elem));
        return STATUS_SXS_CANT_GEN_ACTCTX;
    }

    if (!parse_assembly_elem(xmlbuf, acl, assembly, ai))
        return STATUS_SXS_CANT_GEN_ACTCTX;

    if (next_xml_elem(xmlbuf, &elem))
    {
        WARN("unexpected element %s\n", debugstr_xmlstr(&elem));
        return STATUS_SXS_CANT_GEN_ACTCTX;
    }

    if (xmlbuf->ptr != xmlbuf->end)
    {
        WARN("parse error\n");
        return STATUS_SXS_CANT_GEN_ACTCTX;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS open_nt_file( HANDLE handle, UNICODE_STRING *name )
{
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    return NtOpenFile( handle, GENERIC_READ, &attr, &io, FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_ALERT );
}

static NTSTATUS get_module_filename( HMODULE module, UNICODE_STRING *str, unsigned int extra_len )
{
    NTSTATUS status;
    ULONG magic;
    LDR_MODULE *pldr;

    LdrLockLoaderLock(0, NULL, &magic);
    status = LdrFindEntryForAddress( module, &pldr );
    if (status == STATUS_SUCCESS)
    {
        if ((str->Buffer = RtlAllocateHeap( GetProcessHeap(), 0,
                                            pldr->FullDllName.Length + extra_len + sizeof(WCHAR) )))
        {
            memcpy( str->Buffer, pldr->FullDllName.Buffer, pldr->FullDllName.Length + sizeof(WCHAR) );
            str->Length = pldr->FullDllName.Length;
            str->MaximumLength = pldr->FullDllName.Length + extra_len + sizeof(WCHAR);
        }
        else status = STATUS_NO_MEMORY;
    }
    LdrUnlockLoaderLock(0, magic);
    return status;
}

static NTSTATUS get_manifest_in_module( struct actctx_loader* acl, struct assembly_identity* ai,
                                        LPCWSTR filename, HANDLE hModule, LPCWSTR resname, ULONG lang )
{
    NTSTATUS status;
    UNICODE_STRING nameW;
    LDR_RESOURCE_INFO info;
    const IMAGE_RESOURCE_DATA_ENTRY* entry = NULL;
    void *ptr;

    TRACE( "looking for res %s in module %p %s\n", debugstr_w(resname), hModule, debugstr_w(filename) );

    if (!resname) return STATUS_INVALID_PARAMETER;

    info.Type = RT_MANIFEST;
    info.Language = lang;
    if (!((ULONG_PTR)resname >> 16))
    {
        info.Name = (ULONG_PTR)resname;
        status = LdrFindResource_U(hModule, &info, 3, &entry);
    }
    else if (resname[0] == '#')
    {
        ULONG value;
        RtlInitUnicodeString(&nameW, resname + 1);
        if (RtlUnicodeStringToInteger(&nameW, 10, &value) != STATUS_SUCCESS || HIWORD(value))
            return STATUS_INVALID_PARAMETER;
        info.Name = value;
        status = LdrFindResource_U(hModule, &info, 3, &entry);
    }
    else
    {
        RtlCreateUnicodeString(&nameW, resname);
        RtlUpcaseUnicodeString(&nameW, &nameW, FALSE);
        info.Name = (ULONG_PTR)nameW.Buffer;
        status = LdrFindResource_U(hModule, &info, 3, &entry);
        RtlFreeUnicodeString(&nameW);
    }
    if (status == STATUS_SUCCESS) status = LdrAccessResource(hModule, entry, &ptr, NULL);

    if (status == STATUS_SUCCESS)
    {
        xmlbuf_t buf;
        buf.ptr = ptr;
        buf.end = buf.ptr + entry->Size;
        status = parse_manifest(acl, ai, filename, &buf);
    }
    return status;
}

static NTSTATUS get_manifest_in_pe_file( struct actctx_loader* acl, struct assembly_identity* ai,
                                         LPCWSTR filename, HANDLE file, LPCWSTR resname, ULONG lang )
{
    HANDLE              mapping;
    OBJECT_ATTRIBUTES   attr;
    LARGE_INTEGER       size;
    LARGE_INTEGER       offset;
    NTSTATUS            status;
    SIZE_T              count;
    void               *base;

    TRACE( "looking for res %s in %s\n", debugstr_w(resname), debugstr_w(filename) );

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = NULL;
    attr.Attributes               = OBJ_CASE_INSENSITIVE | OBJ_OPENIF;
    attr.SecurityDescriptor       = NULL;
    attr.SecurityQualityOfService = NULL;

    size.QuadPart = 0;
    status = NtCreateSection( &mapping, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ,
                              &attr, &size, PAGE_READONLY, SEC_COMMIT, file );
    if (status != STATUS_SUCCESS) return status;

    offset.QuadPart = 0;
    count = 0;
    base = NULL;
    status = NtMapViewOfSection( mapping, GetCurrentProcess(), &base, 0, 0, &offset,
                                 &count, ViewShare, 0, PAGE_READONLY );
    NtClose( mapping );
    if (status != STATUS_SUCCESS) return status;

    if (RtlImageNtHeader(base)) /* we got a PE file */
    {
        HANDLE module = (HMODULE)((ULONG_PTR)base | 1);  /* make it a LOAD_LIBRARY_AS_DATAFILE handle */
        status = get_manifest_in_module( acl, ai, filename, module, resname, lang );
    }
    else status = STATUS_INVALID_IMAGE_FORMAT;

    NtUnmapViewOfSection( GetCurrentProcess(), base );
    return status;
}

static NTSTATUS get_manifest_in_manifest_file( struct actctx_loader* acl, struct assembly_identity* ai,
                                               LPCWSTR filename, HANDLE file )
{
    HANDLE              mapping;
    OBJECT_ATTRIBUTES   attr;
    LARGE_INTEGER       size;
    LARGE_INTEGER       offset;
    NTSTATUS            status;
    SIZE_T              count;
    void               *base;
    xmlbuf_t            buf;

    TRACE( "loading manifest file %s\n", debugstr_w(filename) );

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = NULL;
    attr.Attributes               = OBJ_CASE_INSENSITIVE | OBJ_OPENIF;
    attr.SecurityDescriptor       = NULL;
    attr.SecurityQualityOfService = NULL;

    size.QuadPart = 0;
    status = NtCreateSection( &mapping, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ,
                              &attr, &size, PAGE_READONLY, SEC_COMMIT, file );
    if (status != STATUS_SUCCESS) return status;

    offset.QuadPart = 0;
    count = 0;
    base = NULL;
    status = NtMapViewOfSection( mapping, GetCurrentProcess(), &base, 0, 0, &offset,
                                 &count, ViewShare, 0, PAGE_READONLY );
    NtClose( mapping );
    if (status != STATUS_SUCCESS) return status;

    buf.ptr = base;
    buf.end = buf.ptr + count;
    status = parse_manifest(acl, ai, filename, &buf);

    NtUnmapViewOfSection( GetCurrentProcess(), base );
    return status;
}

/* try to load the .manifest file associated to the file */
static NTSTATUS get_manifest_in_associated_manifest( struct actctx_loader* acl, struct assembly_identity* ai,
                                                     LPCWSTR filename, HMODULE module, LPCWSTR resname )
{
    static const WCHAR fmtW[] = { '.','%','l','u',0 };
    WCHAR *buffer;
    NTSTATUS status;
    UNICODE_STRING nameW;
    HANDLE file;
    ULONG_PTR resid = CREATEPROCESS_MANIFEST_RESOURCE_ID;

    if (!((ULONG_PTR)resname >> 16)) resid = (ULONG_PTR)resname & 0xffff;

    TRACE( "looking for manifest associated with %s id %lu\n", debugstr_w(filename), resid );

    if (module) /* use the module filename */
    {
        UNICODE_STRING name;

        if (!(status = get_module_filename( module, &name, sizeof(dotManifestW) + 10*sizeof(WCHAR) )))
        {
            if (resid != 1) sprintfW( name.Buffer + strlenW(name.Buffer), fmtW, resid );
            strcatW( name.Buffer, dotManifestW );
            if (!RtlDosPathNameToNtPathName_U( name.Buffer, &nameW, NULL, NULL ))
                status = STATUS_NO_SUCH_FILE;
            RtlFreeUnicodeString( &name );
        }
        if (status) return status;
    }
    else
    {
        if (!(buffer = RtlAllocateHeap( GetProcessHeap(), 0,
                                        (strlenW(filename) + 10) * sizeof(WCHAR) + sizeof(dotManifestW) )))
            return STATUS_NO_MEMORY;
        strcpyW( buffer, filename );
        if (resid != 1) sprintfW( buffer + strlenW(buffer), fmtW, resid );
        strcatW( buffer, dotManifestW );
        RtlInitUnicodeString( &nameW, buffer );
    }

    status = open_nt_file( &file, &nameW );
    if (status == STATUS_SUCCESS)
    {
        status = get_manifest_in_manifest_file( acl, ai, nameW.Buffer, file );
        NtClose( file );
    }
    RtlFreeUnicodeString( &nameW );
    return status;
}

static NTSTATUS lookup_assembly(struct actctx_loader* acl,
                                struct assembly_identity* ai)
{
    static const WCHAR dotDllW[] = {'.','d','l','l',0};
    unsigned int i;
    WCHAR *buffer, *p;
    NTSTATUS status;
    UNICODE_STRING nameW;
    HANDLE file;

    /* FIXME: add support for language specific lookup */

    nameW.Buffer = NULL;
    if (!(buffer = RtlAllocateHeap( GetProcessHeap(), 0,
                                    (strlenW(acl->actctx->appdir.info) + 2 * strlenW(ai->name) + 2) * sizeof(WCHAR) + sizeof(dotManifestW) )))
        return STATUS_NO_MEMORY;

    /* lookup in appdir\name.dll
     *           appdir\name.manifest
     *           appdir\name\name.dll
     *           appdir\name\name.manifest
     */
    strcpyW( buffer, acl->actctx->appdir.info );
    p = buffer + strlenW(buffer);
    for (i = 0; i < 2; i++)
    {
        *p++ = '\\';
        strcpyW( p, ai->name );
        p += strlenW(p);

        strcpyW( p, dotDllW );
        if (RtlDosPathNameToNtPathName_U( buffer, &nameW, NULL, NULL ))
        {
            status = open_nt_file( &file, &nameW );
            if (!status)
            {
                status = get_manifest_in_pe_file( acl, ai, nameW.Buffer, file,
                                                  (LPCWSTR)CREATEPROCESS_MANIFEST_RESOURCE_ID, 0 );
                NtClose( file );
                break;
            }
            RtlFreeUnicodeString( &nameW );
        }

        strcpyW( p, dotManifestW );
        if (RtlDosPathNameToNtPathName_U( buffer, &nameW, NULL, NULL ))
        {
            status = open_nt_file( &file, &nameW );
            if (!status)
            {
                status = get_manifest_in_manifest_file( acl, ai, nameW.Buffer, file );
                NtClose( file );
                break;
            }
            RtlFreeUnicodeString( &nameW );
        }
    }
    RtlFreeUnicodeString( &nameW );
    return STATUS_SXS_ASSEMBLY_NOT_FOUND;
}

static NTSTATUS parse_depend_manifests(struct actctx_loader* acl)
{
    NTSTATUS status = STATUS_SUCCESS;
    unsigned int i;

    for (i = 0; i < acl->num_dependencies; i++)
    {
        if (lookup_assembly(acl, &acl->dependencies[i]) != STATUS_SUCCESS)
        {
            FIXME( "Could not find assembly %s\n", debugstr_w(acl->dependencies[i].name) );
            status = STATUS_SXS_CANT_GEN_ACTCTX;
            break;
        }
    }
    /* FIXME should now iterate through all refs */
    return status;
}


/***********************************************************************
 * RtlCreateActivationContext (NTDLL.@)
 *
 * Create an activation context.
 *
 * FIXME: function signature/prototype is wrong
 */
NTSTATUS WINAPI RtlCreateActivationContext( HANDLE *handle, const void *ptr )
{
    const ACTCTXW *pActCtx = ptr;  /* FIXME: not the right structure */
    ACTIVATION_CONTEXT *actctx;
    UNICODE_STRING nameW;
    struct assembly *assembly;
    ULONG lang = 0;
    NTSTATUS status = STATUS_NO_MEMORY;
    HANDLE file = 0;
    struct actctx_loader acl;

    TRACE("%p %08x\n", pActCtx, pActCtx ? pActCtx->dwFlags : 0);

    if (!pActCtx || pActCtx->cbSize != sizeof(*pActCtx) ||
        (pActCtx->dwFlags & ~ACTCTX_FLAGS_ALL))
        return STATUS_INVALID_PARAMETER;

    if (!(actctx = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*actctx) )))
        return STATUS_NO_MEMORY;

    actctx->magic = ACTCTX_MAGIC;
    actctx->ref_count = 1;

    if (!(assembly = add_assembly( actctx, APPLICATION_MANIFEST ))) goto error;
    if (!(assembly->id.name = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WCHAR) )))
        goto error;
    assembly->id.version.major = 1;
    assembly->id.version.minor = 0;
    assembly->id.version.build = 0;
    assembly->id.version.revision = 0;
    assembly->manifest.type = ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE;
    assembly->manifest.info = NULL;

    actctx->config.type = ACTIVATION_CONTEXT_PATH_TYPE_NONE;
    actctx->config.info = NULL;
    actctx->appdir.type = ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE;
    if (pActCtx->dwFlags & ACTCTX_FLAG_APPLICATION_NAME_VALID)
    {
        if (!(actctx->appdir.info = strdupW( pActCtx->lpApplicationName ))) goto error;
    }
    else
    {
        UNICODE_STRING dir;
        WCHAR *p;

        if ((status = get_module_filename( NtCurrentTeb()->Peb->ImageBaseAddress, &dir, 0 )))
            goto error;
        if ((p = strrchrW( dir.Buffer, '\\' ))) *p = 0;
        actctx->appdir.info = dir.Buffer;
    }

    nameW.Buffer = NULL;
    if (pActCtx->lpSource)
    {
        if (!RtlDosPathNameToNtPathName_U(pActCtx->lpSource, &nameW, NULL, NULL))
        {
            status = STATUS_NO_SUCH_FILE;
            goto error;
        }
        status = open_nt_file( &file, &nameW );
        if (status)
        {
            RtlFreeUnicodeString( &nameW );
            goto error;
        }
    }

    acl.actctx = actctx;
    acl.dependencies = NULL;
    acl.num_dependencies = 0;
    acl.allocated_dependencies = 0;

    if (pActCtx->dwFlags & ACTCTX_FLAG_LANGID_VALID) lang = pActCtx->wLangId;

    if (pActCtx->dwFlags & ACTCTX_FLAG_RESOURCE_NAME_VALID)
    {
        /* if we have a resource it's a PE file */
        if (pActCtx->dwFlags & ACTCTX_FLAG_HMODULE_VALID)
        {
            status = get_manifest_in_module( &acl, NULL, NULL, pActCtx->hModule,
                                             pActCtx->lpResourceName, lang );
            if (status)
                /* FIXME: what to do if pActCtx->lpSource is set */
                status = get_manifest_in_associated_manifest( &acl, NULL, NULL, pActCtx->hModule,
                                                              pActCtx->lpResourceName );
        }
        else if (pActCtx->lpSource)
        {
            status = get_manifest_in_pe_file( &acl, NULL, nameW.Buffer, file,
                                              pActCtx->lpResourceName, lang );
            if (status)
                status = get_manifest_in_associated_manifest( &acl, NULL, nameW.Buffer, NULL,
                                                              pActCtx->lpResourceName );
        }
        else status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = get_manifest_in_manifest_file( &acl, NULL, nameW.Buffer, file );
    }

    if (file) NtClose( file );
    RtlFreeUnicodeString( &nameW );

    if (status == STATUS_SUCCESS) status = parse_depend_manifests(&acl);
    free_depend_manifests( &acl );

    if (status == STATUS_SUCCESS) *handle = actctx;
    else actctx_release( actctx );
    return status;

error:
    if (file) NtClose( file );
    actctx_release( actctx );
    return status;
}


/***********************************************************************
 *		RtlAddRefActivationContext (NTDLL.@)
 */
void WINAPI RtlAddRefActivationContext( HANDLE handle )
{
    ACTIVATION_CONTEXT *actctx;

    if ((actctx = check_actctx( handle ))) actctx_addref( actctx );
}


/******************************************************************
 *		RtlReleaseActivationContext (NTDLL.@)
 */
void WINAPI RtlReleaseActivationContext( HANDLE handle )
{
    ACTIVATION_CONTEXT *actctx;

    if ((actctx = check_actctx( handle ))) actctx_release( actctx );
}


/******************************************************************
 *		RtlActivateActivationContext (NTDLL.@)
 */
NTSTATUS WINAPI RtlActivateActivationContext( ULONG unknown, HANDLE handle, ULONG_PTR *cookie )
{
    RTL_ACTIVATION_CONTEXT_STACK_FRAME *frame;

    TRACE( "%p %p\n", handle, cookie );

    if (!(frame = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*frame) )))
        return STATUS_NO_MEMORY;

    frame->Previous = NtCurrentTeb()->ActivationContextStack.ActiveFrame;
    frame->ActivationContext = handle;
    frame->Flags = 0;
    NtCurrentTeb()->ActivationContextStack.ActiveFrame = frame;
    RtlAddRefActivationContext( handle );

    *cookie = (ULONG_PTR)frame;
    return STATUS_SUCCESS;
}


/***********************************************************************
 *		RtlDeactivateActivationContext (NTDLL.@)
 */
void WINAPI RtlDeactivateActivationContext( ULONG flags, ULONG_PTR cookie )
{
    RTL_ACTIVATION_CONTEXT_STACK_FRAME *frame, *top;

    TRACE( "%x %lx\n", flags, cookie );

    /* find the right frame */
    top = NtCurrentTeb()->ActivationContextStack.ActiveFrame;
    for (frame = top; frame; frame = frame->Previous)
        if ((ULONG_PTR)frame == cookie) break;

    if (!frame)
        RtlRaiseStatus( STATUS_SXS_INVALID_DEACTIVATION );

    if (frame != top && !(flags & DEACTIVATE_ACTCTX_FLAG_FORCE_EARLY_DEACTIVATION))
        RtlRaiseStatus( STATUS_SXS_EARLY_DEACTIVATION );

    /* pop everything up to and including frame */
    NtCurrentTeb()->ActivationContextStack.ActiveFrame = frame->Previous;

    while (top != NtCurrentTeb()->ActivationContextStack.ActiveFrame)
    {
        frame = top->Previous;
        RtlReleaseActivationContext( top->ActivationContext );
        RtlFreeHeap( GetProcessHeap(), 0, top );
        top = frame;
    }
}


/******************************************************************
 *		RtlFreeThreadActivationContextStack (NTDLL.@)
 */
void WINAPI RtlFreeThreadActivationContextStack(void)
{
    RTL_ACTIVATION_CONTEXT_STACK_FRAME *frame;

    frame = NtCurrentTeb()->ActivationContextStack.ActiveFrame;
    while (frame)
    {
        RTL_ACTIVATION_CONTEXT_STACK_FRAME *prev = frame->Previous;
        RtlReleaseActivationContext( frame->ActivationContext );
        RtlFreeHeap( GetProcessHeap(), 0, frame );
        frame = prev;
    }
    NtCurrentTeb()->ActivationContextStack.ActiveFrame = NULL;
}


/******************************************************************
 *		RtlGetActiveActivationContext (NTDLL.@)
 */
NTSTATUS WINAPI RtlGetActiveActivationContext( HANDLE *handle )
{
    if (NtCurrentTeb()->ActivationContextStack.ActiveFrame)
    {
        *handle = NtCurrentTeb()->ActivationContextStack.ActiveFrame->ActivationContext;
        RtlAddRefActivationContext( *handle );
    }
    else
        *handle = 0;

    return STATUS_SUCCESS;
}


/******************************************************************
 *		RtlIsActivationContextActive (NTDLL.@)
 */
BOOLEAN WINAPI RtlIsActivationContextActive( HANDLE handle )
{
    RTL_ACTIVATION_CONTEXT_STACK_FRAME *frame;

    for (frame = NtCurrentTeb()->ActivationContextStack.ActiveFrame; frame; frame = frame->Previous)
        if (frame->ActivationContext == handle) return TRUE;
    return FALSE;
}
