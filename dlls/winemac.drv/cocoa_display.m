/*
 * MACDRV Cocoa display settings
 *
 * Copyright 2011, 2012 Ken Thomases for CodeWeavers Inc.
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

#import <AppKit/AppKit.h>
#include "macdrv_cocoa.h"


/***********************************************************************
 *              convert_display_rect
 *
 * Converts an NSRect in Cocoa's y-goes-up-from-bottom coordinate system
 * to a CGRect in y-goes-down-from-top coordinates.
 */
static inline void convert_display_rect(CGRect* out_rect, NSRect in_rect,
                                        NSRect primary_frame)
{
    *out_rect = NSRectToCGRect(in_rect);
    out_rect->origin.y = NSMaxY(primary_frame) - NSMaxY(in_rect);
}


/***********************************************************************
 *              macdrv_get_displays
 *
 * Returns information about the displays.
 *
 * Returns 0 on success and *displays contains a newly-allocated array
 * of macdrv_display structures and *count contains the number of
 * elements in that array.  The first element of the array is the
 * primary display.  When the caller is done with the array, it should
 * use macdrv_free_displays() to deallocate it.
 *
 * Returns non-zero on failure and *displays and *count are unchanged.
 */
int macdrv_get_displays(struct macdrv_display** displays, int* count)
{
    int ret = -1;
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

    NSArray* screens = [NSScreen screens];
    if (screens)
    {
        NSUInteger num_screens = [screens count];
        struct macdrv_display* disps = malloc(num_screens * sizeof(disps[0]));

        if (disps)
        {
            NSRect primary_frame;

            NSUInteger i;
            for (i = 0; i < num_screens; i++)
            {
                NSScreen* screen = [screens objectAtIndex:i];
                NSRect frame = [screen frame];
                NSRect visible_frame = [screen visibleFrame];

                if (i == 0)
                    primary_frame = frame;

                disps[i].displayID = [[[screen deviceDescription] objectForKey:@"NSScreenNumber"] unsignedIntValue];
                convert_display_rect(&disps[i].frame, frame, primary_frame);
                convert_display_rect(&disps[i].work_frame, visible_frame,
                                     primary_frame);
                disps[i].frame = cgrect_win_from_mac(disps[i].frame);
                disps[i].work_frame = cgrect_win_from_mac(disps[i].work_frame);
            }

            *displays = disps;
            *count = num_screens;
            ret = 0;
        }
    }

    [pool release];
    return ret;
}


/***********************************************************************
 *              macdrv_free_displays
 *
 * Deallocates an array of macdrv_display structures previously returned
 * from macdrv_get_displays().
 */
void macdrv_free_displays(struct macdrv_display* displays)
{
    free(displays);
}
