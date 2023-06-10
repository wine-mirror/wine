/*
 * Manifest parser for WUSA
 *
 * Copyright 2015 Michael Müller
 * Copyright 2015 Sebastian Lackner
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

#include <windows.h>
#define COBJMACROS
#include <initguid.h>
#include <msxml.h>

#include "wine/debug.h"
#include "wine/list.h"
#include "wusa.h"

WINE_DEFAULT_DEBUG_CHANNEL(wusa);

static struct dependency_entry *alloc_dependency(void)
{
    struct dependency_entry *entry = calloc(1, sizeof(*entry));
    if (!entry) ERR("Failed to allocate memory for dependency\n");
    return entry;
}

static struct fileop_entry *alloc_fileop(void)
{
    struct fileop_entry *entry = calloc(1, sizeof(*entry));
    if (!entry) ERR("Failed to allocate memory for fileop\n");
    return entry;
}

static struct registrykv_entry *alloc_registrykv(void)
{
    struct registrykv_entry *entry = calloc(1, sizeof(*entry));
    if (!entry) ERR("Failed to allocate memory for registrykv\n");
    return entry;
}

static struct registryop_entry *alloc_registryop(void)
{
    struct registryop_entry *entry = calloc(1, sizeof(*entry));
    if (!entry) ERR("Failed to allocate memory for registryop\n");
    else
    {
        list_init(&entry->keyvalues);
    }
    return entry;
}

static struct assembly_entry *alloc_assembly(void)
{
    struct assembly_entry *entry = calloc(1, sizeof(*entry));
    if (!entry) ERR("Failed to allocate memory for assembly\n");
    else
    {
        list_init(&entry->dependencies);
        list_init(&entry->fileops);
        list_init(&entry->registryops);
    }
    return entry;
}

static void clear_identity(struct assembly_identity *entry)
{
    free(entry->name);
    free(entry->version);
    free(entry->architecture);
    free(entry->language);
    free(entry->pubkey_token);
}

void free_dependency(struct dependency_entry *entry)
{
    clear_identity(&entry->identity);
    free(entry);
}

static void free_fileop(struct fileop_entry *entry)
{
    free(entry->source);
    free(entry->target);
    free(entry);
}

static void free_registrykv(struct registrykv_entry *entry)
{
    free(entry->name);
    free(entry->value_type);
    free(entry->value);
    free(entry);
}

static void free_registryop(struct registryop_entry *entry)
{
    struct registrykv_entry *keyvalue, *keyvalue2;

    free(entry->key);

    LIST_FOR_EACH_ENTRY_SAFE(keyvalue, keyvalue2, &entry->keyvalues, struct registrykv_entry, entry)
    {
        list_remove(&keyvalue->entry);
        free_registrykv(keyvalue);
    }

    free(entry);
}

void free_assembly(struct assembly_entry *entry)
{
    struct dependency_entry *dependency, *dependency2;
    struct fileop_entry *fileop, *fileop2;
    struct registryop_entry *registryop, *registryop2;

    free(entry->filename);
    free(entry->displayname);
    clear_identity(&entry->identity);

    LIST_FOR_EACH_ENTRY_SAFE(dependency, dependency2, &entry->dependencies, struct dependency_entry, entry)
    {
        list_remove(&dependency->entry);
        free_dependency(dependency);
    }
    LIST_FOR_EACH_ENTRY_SAFE(fileop, fileop2, &entry->fileops, struct fileop_entry, entry)
    {
        list_remove(&fileop->entry);
        free_fileop(fileop);
    }
    LIST_FOR_EACH_ENTRY_SAFE(registryop, registryop2, &entry->registryops, struct registryop_entry, entry)
    {
        list_remove(&registryop->entry);
        free_registryop(registryop);
    }

    free(entry);
}

static WCHAR *get_xml_attribute(IXMLDOMElement *root, const WCHAR *name)
{
    WCHAR *ret = NULL;
    VARIANT var;
    BSTR bstr;

    if ((bstr = SysAllocString(name)))
    {
        VariantInit(&var);
        if (SUCCEEDED(IXMLDOMElement_getAttribute(root, bstr, &var)))
        {
            ret = (V_VT(&var) == VT_BSTR) ? wcsdup(V_BSTR(&var)) : NULL;
            VariantClear(&var);
        }
        SysFreeString(bstr);
    }

    return ret;
}

static BOOL check_xml_tagname(IXMLDOMElement *root, const WCHAR *tagname)
{
    BOOL ret = FALSE;
    BSTR bstr;

    if (SUCCEEDED(IXMLDOMElement_get_tagName(root, &bstr)))
    {
        ret = !wcscmp(bstr, tagname);
        SysFreeString(bstr);
    }

    return ret;
}

static IXMLDOMElement *select_xml_node(IXMLDOMElement *root, const WCHAR *name)
{
    IXMLDOMElement *ret = NULL;
    IXMLDOMNode *node;
    BSTR bstr;

    if ((bstr = SysAllocString(name)))
    {
        if (SUCCEEDED(IXMLDOMElement_selectSingleNode(root, bstr, &node)))
        {
            if (FAILED(IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMElement, (void **)&ret)))
                ret = NULL;
            IXMLDOMNode_Release(node);
        }
        SysFreeString(bstr);
    }

    return ret;
}

static BOOL call_xml_callbacks(IXMLDOMElement *root, BOOL (*func)(IXMLDOMElement *child, WCHAR *tagname, void *context), void *context)
{
    IXMLDOMNodeList *children;
    IXMLDOMElement *child;
    IXMLDOMNode *node;
    BSTR tagname;
    BOOL ret = TRUE;

    if (FAILED(IXMLDOMElement_get_childNodes(root, &children)))
        return FALSE;

    while (ret && IXMLDOMNodeList_nextNode(children, &node) == S_OK)
    {
        if (SUCCEEDED(IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMElement, (void **)&child)))
        {
            if (SUCCEEDED(IXMLDOMElement_get_tagName(child, &tagname)))
            {
                ret = func(child, tagname, context);
                SysFreeString(tagname);
            }
            IXMLDOMElement_Release(child);
        }
        IXMLDOMNode_Release(node);
    }

    IXMLDOMNodeList_Release(children);
    return ret;
}

static IXMLDOMElement *load_xml(const WCHAR *filename)
{
    IXMLDOMDocument *document = NULL;
    IXMLDOMElement *root = NULL;
    VARIANT_BOOL success;
    VARIANT variant;
    BSTR bstr;

    TRACE("Loading XML from %s\n", debugstr_w(filename));

    if (!(bstr = SysAllocString(filename)))
        return FALSE;

    if (SUCCEEDED(CoCreateInstance(&CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (void **)&document)))
    {
        VariantInit(&variant);
        V_VT(&variant) = VT_BSTR;
        V_BSTR(&variant) = bstr;

        if (SUCCEEDED(IXMLDOMDocument_load(document, variant, &success)) && success)
        {
            if (FAILED(IXMLDOMDocument_get_documentElement(document, &root)))
                root = NULL;
        }
        IXMLDOMDocument_Release(document);
    }

    SysFreeString(bstr);
    return root;
}

static BOOL read_identity(IXMLDOMElement *root, struct assembly_identity *identity)
{
    memset(identity, 0, sizeof(*identity));
    if (!(identity->name            = get_xml_attribute(root, L"name"))) goto error;
    if (!(identity->version         = get_xml_attribute(root, L"version"))) goto error;
    if (!(identity->architecture    = get_xml_attribute(root, L"processorArchitecture"))) goto error;
    if (!(identity->language        = get_xml_attribute(root, L"language"))) goto error;
    if (!(identity->pubkey_token    = get_xml_attribute(root, L"publicKeyToken"))) goto error;
    return TRUE;

error:
    clear_identity(identity);
    memset(identity, 0, sizeof(*identity));
    return FALSE;
}

/* <assembly><dependency><dependentAssembly> */
static BOOL read_dependent_assembly(IXMLDOMElement *root, struct assembly_identity *identity)
{
    IXMLDOMElement *child = NULL;
    WCHAR *dependency_type;
    BOOL ret = FALSE;

    if (!(dependency_type = get_xml_attribute(root, L"dependencyType")))
    {
        WARN("Failed to get dependency type, assuming install\n");
    }
    if (dependency_type && wcscmp(dependency_type, L"install") && wcscmp(dependency_type, L"prerequisite"))
    {
        FIXME("Unimplemented dependency type %s\n", debugstr_w(dependency_type));
        goto error;
    }
    if (!(child = select_xml_node(root, L".//assemblyIdentity")))
    {
        FIXME("Failed to find assemblyIdentity child node\n");
        goto error;
    }

    ret = read_identity(child, identity);

error:
    if (child) IXMLDOMElement_Release(child);
    free(dependency_type);
    return ret;
}

/* <assembly><dependency> */
static BOOL read_dependency(IXMLDOMElement *child, WCHAR *tagname, void *context)
{
    struct assembly_entry *assembly = context;
    struct dependency_entry *entry;

    if (wcscmp(tagname, L"dependentAssembly"))
    {
        FIXME("Don't know how to handle dependency tag %s\n", debugstr_w(tagname));
        return FALSE;
    }

    if ((entry = alloc_dependency()))
    {
        if (read_dependent_assembly(child, &entry->identity))
        {
            TRACE("Found dependency %s\n", debugstr_w(entry->identity.name));
            list_add_tail(&assembly->dependencies, &entry->entry);
            return TRUE;
        }
        free_dependency(entry);
    }

    return FALSE;
}

static BOOL iter_dependency(IXMLDOMElement *root, struct assembly_entry *assembly)
{
    return call_xml_callbacks(root, read_dependency, assembly);
}

/* <assembly><package><update><component> */
/* <assembly><package><update><package> */
static BOOL read_components(IXMLDOMElement *child, WCHAR *tagname, void *context)
{
    struct assembly_entry *assembly = context;
    struct dependency_entry *entry;

    if (wcscmp(tagname, L"assemblyIdentity"))
    {
        FIXME("Ignoring unexpected tag %s\n", debugstr_w(tagname));
        return TRUE;
    }

    if ((entry = alloc_dependency()))
    {
        if (read_identity(child, &entry->identity))
        {
            TRACE("Found identity %s\n", debugstr_w(entry->identity.name));
            list_add_tail(&assembly->dependencies, &entry->entry);
            return TRUE;
        }
        free_dependency(entry);
    }

    return FALSE;
}

static BOOL iter_components(IXMLDOMElement *root, struct assembly_entry *assembly)
{
    return call_xml_callbacks(root, read_components, assembly);
}

/* <assembly><package><update> */
static BOOL read_update(IXMLDOMElement *child, WCHAR *tagname, void *context)
{
    struct assembly_entry *assembly = context;

    if (!wcscmp(tagname, L"component"))
        return iter_components(child, assembly);
    if (!wcscmp(tagname, L"package"))
        return iter_components(child, assembly);
    if (!wcscmp(tagname, L"applicable"))
        return TRUE;

    FIXME("Ignoring unexpected tag %s\n", debugstr_w(tagname));
    return FALSE;
}

static BOOL iter_update(IXMLDOMElement *root, struct assembly_entry *assembly)
{
    return call_xml_callbacks(root, read_update, assembly);
}

/* <assembly><package> */
static BOOL read_package(IXMLDOMElement *child, WCHAR *tagname, void *context)
{
    struct assembly_entry *assembly = context;

    if (!wcscmp(tagname, L"update"))
        return iter_update(child, assembly);
    if (!wcscmp(tagname, L"parent"))
        return TRUE;

    FIXME("Ignoring unexpected tag %s\n", debugstr_w(tagname));
    return TRUE;
}

static BOOL iter_package(IXMLDOMElement *root, struct assembly_entry *assembly)
{
    return call_xml_callbacks(root, read_package, assembly);
}

/* <assembly><file> */
static BOOL read_file(IXMLDOMElement *root, struct assembly_entry *assembly)
{
    struct fileop_entry *entry;

    if (!(entry = alloc_fileop()))
        return FALSE;

    if (!(entry->source = get_xml_attribute(root, L"sourceName"))) goto error;
    if (!(entry->target = get_xml_attribute(root, L"destinationPath"))) goto error;

    TRACE("Found fileop %s -> %s\n", debugstr_w(entry->source), debugstr_w(entry->target));
    list_add_tail(&assembly->fileops, &entry->entry);
    return TRUE;

error:
    free_fileop(entry);
    return FALSE;
}

/* <assembly><registryKeys><registryKey> */
static BOOL read_registry_key(IXMLDOMElement *child, WCHAR *tagname, void *context)
{
    struct registryop_entry *registryop = context;
    struct registrykv_entry *entry;

    if (!wcscmp(tagname, L"securityDescriptor")) return TRUE;
    if (!wcscmp(tagname, L"systemProtection")) return TRUE;
    if (wcscmp(tagname, L"registryValue"))
    {
        FIXME("Ignoring unexpected tag %s\n", debugstr_w(tagname));
        return TRUE;
    }

    if (!(entry = alloc_registrykv()))
        return FALSE;

    if (!(entry->value_type = get_xml_attribute(child, L"valueType"))) goto error;
    entry->name = get_xml_attribute(child, L"name");      /* optional */
    entry->value = get_xml_attribute(child, L"value");    /* optional */

    TRACE("Found registry %s -> %s\n", debugstr_w(entry->name), debugstr_w(entry->value));
    list_add_tail(&registryop->keyvalues, &entry->entry);
    return TRUE;

error:
    free_registrykv(entry);
    return FALSE;
}

static BOOL iter_registry_key(IXMLDOMElement *root, struct registryop_entry *registryop)
{
    return call_xml_callbacks(root, read_registry_key, registryop);
}

/* <assembly><registryKeys> */
static BOOL read_registry_keys(IXMLDOMElement *child, WCHAR *tagname, void *context)
{
    struct assembly_entry *assembly = context;
    struct registryop_entry *entry;
    WCHAR *keyname;

    if (wcscmp(tagname, L"registryKey"))
    {
        FIXME("Ignoring unexpected tag %s\n", debugstr_w(tagname));
        return TRUE;
    }

    if (!(keyname = get_xml_attribute(child, L"keyName")))
    {
        FIXME("RegistryKey tag doesn't specify keyName\n");
        return FALSE;
    }

    if ((entry = alloc_registryop()))
    {
        list_init(&entry->keyvalues);
        if (iter_registry_key(child, entry))
        {
            entry->key = keyname;
            TRACE("Found registryop %s\n", debugstr_w(entry->key));
            list_add_tail(&assembly->registryops, &entry->entry);
            return TRUE;
        }
        free_registryop(entry);
    }

    free(keyname);
    return FALSE;
}

static BOOL iter_registry_keys(IXMLDOMElement *root, struct assembly_entry *assembly)
{
    return call_xml_callbacks(root, read_registry_keys, assembly);
}

/* <assembly> */
static BOOL read_assembly(IXMLDOMElement *child, WCHAR *tagname, void *context)
{
    struct assembly_entry *assembly = context;

    if (!wcscmp(tagname, L"assemblyIdentity") && !assembly->identity.name)
        return read_identity(child, &assembly->identity);
    if (!wcscmp(tagname, L"dependency"))
        return iter_dependency(child, assembly);
    if (!wcscmp(tagname, L"package"))
        return iter_package(child, assembly);
    if (!wcscmp(tagname, L"file"))
        return read_file(child, assembly);
    if (!wcscmp(tagname, L"registryKeys"))
        return iter_registry_keys(child, assembly);
    if (!wcscmp(tagname, L"trustInfo"))
        return TRUE;
    if (!wcscmp(tagname, L"configuration"))
        return TRUE;
    if (!wcscmp(tagname, L"deployment"))
        return TRUE;

    FIXME("Ignoring unexpected tag %s\n", debugstr_w(tagname));
    return TRUE;
}

static BOOL iter_assembly(IXMLDOMElement *root, struct assembly_entry *assembly)
{
    return call_xml_callbacks(root, read_assembly, assembly);
}

struct assembly_entry *load_manifest(const WCHAR *filename)
{
    struct assembly_entry *entry = NULL;
    IXMLDOMElement *root = NULL;

    TRACE("Loading manifest %s\n", debugstr_w(filename));

    if (!(root = load_xml(filename))) return NULL;
    if (!check_xml_tagname(root, L"assembly"))
    {
        FIXME("Didn't find assembly root node?\n");
        goto done;
    }

    if ((entry = alloc_assembly()))
    {
        entry->filename = wcsdup(filename);
        entry->displayname = get_xml_attribute(root, L"displayName");
        if (iter_assembly(root, entry)) goto done;
        free_assembly(entry);
        entry = NULL;
    }

done:
    IXMLDOMElement_Release(root);
    return entry;
}

/* <unattend><servicing><package> */
static BOOL read_update_package(IXMLDOMElement *child, WCHAR *tagname, void *context)
{
    struct dependency_entry *entry;
    struct list *update_list = context;

    if (!wcscmp(tagname, L"source")) return TRUE;
    if (wcscmp(tagname, L"assemblyIdentity"))
    {
        TRACE("Ignoring unexpected tag %s\n", debugstr_w(tagname));
        return TRUE;
    }

    if ((entry = alloc_dependency()))
    {
        if (read_identity(child, &entry->identity))
        {
            TRACE("Found update %s\n", debugstr_w(entry->identity.name));
            list_add_tail(update_list, &entry->entry);
            return TRUE;
        }
        free(entry);
    }

    return FALSE;
}

static BOOL iter_update_package(IXMLDOMElement *root, struct list *update_list)
{
    return call_xml_callbacks(root, read_update_package, update_list);
}

/* <unattend><servicing> */
static BOOL read_servicing(IXMLDOMElement *child, WCHAR *tagname, void *context)
{
    struct list *update_list = context;
    WCHAR *action;
    BOOL ret = TRUE;

    if (wcscmp(tagname, L"package"))
    {
        FIXME("Ignoring unexpected tag %s\n", debugstr_w(tagname));
        return TRUE;
    }

    if (!(action = get_xml_attribute(child, L"action")))
    {
        FIXME("Servicing tag doesn't specify action\n");
        return FALSE;
    }

    if (!wcscmp(action, L"install"))
        ret = iter_update_package(child, update_list);
    else
        FIXME("action %s not supported\n", debugstr_w(action));

    free(action);
    return ret;
}

static BOOL iter_servicing(IXMLDOMElement *root, struct list *update_list)
{
    return call_xml_callbacks(root, read_servicing, update_list);
}

/* <unattend> */
static BOOL read_unattend(IXMLDOMElement *child, WCHAR *tagname, void *context)
{
    struct list *update_list = context;

    if (wcscmp(tagname, L"servicing"))
    {
        FIXME("Ignoring unexpected tag %s\n", debugstr_w(tagname));
        return TRUE;
    }

    return iter_servicing(child, update_list);

}

static BOOL iter_unattend(IXMLDOMElement *root, struct list *update_list)
{
    return call_xml_callbacks(root, read_unattend, update_list);
}

BOOL load_update(const WCHAR *filename, struct list *update_list)
{
    IXMLDOMElement *root = NULL;
    BOOL ret = FALSE;

    TRACE("Reading update %s\n", debugstr_w(filename));

    if (!(root = load_xml(filename))) return FALSE;
    if (!check_xml_tagname(root, L"unattend"))
    {
        FIXME("Didn't find unattend root node?\n");
        goto done;
    }

    ret = iter_unattend(root, update_list);

done:
    IXMLDOMElement_Release(root);
    return ret;
}
