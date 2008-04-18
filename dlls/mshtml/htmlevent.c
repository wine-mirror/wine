/*
 * Copyright 2008 Jacek Caban for CodeWeavers
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "mshtml_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef enum {
    EVENTID_LOAD,
    EVENTID_LAST
} eventid;

struct event_target_t {
    IDispatch *event_table[EVENTID_LAST];
};

static const WCHAR loadW[] = {'l','o','a','d',0};
static const WCHAR onloadW[] = {'o','n','l','o','a','d',0};

typedef struct {
    LPCWSTR name;
    LPCWSTR attr_name;
} event_info_t;

static const event_info_t event_info[] = {
    {loadW, onloadW}
};

void check_event_attr(HTMLDocument *doc, nsIDOMElement *nselem)
{
    const PRUnichar *attr_value;
    nsAString attr_name_str, attr_value_str;
    IDispatch *disp;
    HTMLDOMNode *node;
    int i;
    nsresult nsres;

    nsAString_Init(&attr_value_str, NULL);
    nsAString_Init(&attr_name_str, NULL);

    for(i=0; i < EVENTID_LAST; i++) {
        nsAString_SetData(&attr_name_str, event_info[i].attr_name);
        nsres = nsIDOMElement_GetAttribute(nselem, &attr_name_str, &attr_value_str);
        if(NS_SUCCEEDED(nsres)) {
            nsAString_GetData(&attr_value_str, &attr_value);
            if(!*attr_value)
                continue;

            TRACE("%p.%s = %s\n", nselem, debugstr_w(event_info[i].attr_name), debugstr_w(attr_value));

            disp = script_parse_event(doc, attr_value);
            if(disp) {
                node = get_node(doc, (nsIDOMNode*)nselem, TRUE);
                if(!node->event_target)
                    node->event_target = heap_alloc_zero(sizeof(event_target_t));
                node->event_target->event_table[i] = disp;
            }
        }
    }

    nsAString_Finish(&attr_value_str);
    nsAString_Finish(&attr_name_str);
}

void release_event_target(event_target_t *event_target)
{
    int i;

    for(i=0; i < EVENTID_LAST; i++) {
        if(event_target->event_table[i])
            IDispatch_Release(event_target->event_table[i]);
    }

    heap_free(event_target);
}
