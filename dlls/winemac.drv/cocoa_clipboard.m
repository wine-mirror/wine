/*
 * MACDRV Cocoa clipboard code
 *
 * Copyright 2012, 2013 Ken Thomases for CodeWeavers Inc.
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

#include "macdrv_cocoa.h"
#import "cocoa_app.h"


/***********************************************************************
 *              macdrv_copy_pasteboard_types
 *
 * Returns an array of UTI strings for the types of data available on
 * the pasteboard, or NULL on error.  The caller is responsible for
 * releasing the returned array with CFRelease().
 */
CFArrayRef macdrv_copy_pasteboard_types(void)
{
    __block CFArrayRef ret = NULL;
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

    OnMainThread(^{
        @try
        {
            NSPasteboard* pb = [NSPasteboard generalPasteboard];
            NSArray* types = [pb types];
            ret = (CFArrayRef)[types copy];
        }
        @catch (id e)
        {
            ERR(@"Exception discarded while copying pasteboard types: %@\n", e);
        }
    });

    [pool release];
    return ret;
}
