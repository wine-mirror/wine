/*
 * Copyright 2009 Jeremy White <jwhite@codeweavers.com> for CodeWeavers
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

#include <stdlib.h>
#include "twain.h"
#include "sane_i.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(twain);

#ifdef SONAME_LIBSANE
SANE_Status sane_option_get_int(SANE_Handle h, const char *option_name, SANE_Int *val)
{
    SANE_Status rc;
    SANE_Int optcount;
    const SANE_Option_Descriptor *opt;
    int i;

    rc = psane_control_option(h, 0, SANE_ACTION_GET_VALUE, &optcount, NULL);
    if (rc != SANE_STATUS_GOOD)
        return rc;

    for (i = 1; i < optcount; i++)
    {
        opt = psane_get_option_descriptor(h, i);
        if (opt && (opt->name && strcmp(opt->name, option_name) == 0) &&
               opt->type == SANE_TYPE_INT )
            return psane_control_option(h, i, SANE_ACTION_GET_VALUE, val, NULL);
    }
    return SANE_STATUS_EOF;
}
#endif
