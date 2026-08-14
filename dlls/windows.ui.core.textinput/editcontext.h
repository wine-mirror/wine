/* WinRT Windows.UI.Text.Core.CoreTextEditContext Implementation
 *
 * Written by Weather
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

#ifndef EDITCONTEXT_H
#define EDITCONTEXT_H

#include <stdarg.h>
#include <stddef.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winstring.h"
#include "activation.h"
#include "wine/debug.h"

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_UI_Text_Core
#include "windows.ui.text.core.h"

struct core_text_edit_context
{
    ICoreTextEditContext ICoreTextEditContext_iface;
    LONG ref;
};

HRESULT core_text_edit_context_create( ICoreTextEditContext **out );

#endif
