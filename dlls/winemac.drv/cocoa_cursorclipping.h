/*
 * MACDRV CGEventTap-based cursor clipping class declaration
 *
 * Copyright 2011, 2012, 2013 Ken Thomases for CodeWeavers Inc.
 * Copyright 2021 Tim Clem for CodeWeavers Inc.
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


@protocol WineClipCursorHandler <NSObject>

@property (readonly, nonatomic) BOOL clippingCursor;
@property (readonly, nonatomic) CGRect cursorClipRect;

    - (BOOL) startClippingCursor:(CGRect)rect;
    - (BOOL) stopClippingCursor;

    - (void) clipCursorLocation:(CGPoint*)location;

    - (void) setRetinaMode:(int)mode;

    @optional
    /* If provided, should reposition the cursor as needed given the current
     * clipping rect. If not provided, the location will be clipped by
     * -clipCursorLocation, and the cursor will be warped normally.
     */
    - (BOOL) setCursorPosition:(CGPoint)pos;

@end


@interface WineEventTapClipCursorHandler : NSObject <WineClipCursorHandler>
{
    BOOL clippingCursor;
    CGRect cursorClipRect;
    CFMachPortRef cursorClippingEventTap;
    NSMutableArray* warpRecords;
    CGPoint synthesizedLocation;
    NSTimeInterval lastEventTapEventTime;
}

@end


@interface WineConfinementClipCursorHandler : NSObject <WineClipCursorHandler>
{
    BOOL clippingCursor;
    CGRect cursorClipRect;
    /* The number of the window that "owns" the clipping (i.e., the one with a
     * mouseConfinementRect set). Using this rather than a WineWindow* to avoid
     * tricky retain situations. */
    NSInteger clippingWindowNumber;
}

    /* Returns true if the API in use by this handler is available. */
    + (BOOL) isAvailable;

@end
