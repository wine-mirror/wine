/*
 * MACDRV Cocoa status item class
 *
 * Copyright 2011, 2012, 2013 Ken Thomases for CodeWeavers Inc.
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

#import <Cocoa/Cocoa.h>
#include "macdrv_cocoa.h"
#import "cocoa_app.h"
#import "cocoa_event.h"

#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"


@interface WineStatusItem : NSView
{
    NSStatusItem* item;
    WineEventQueue* queue;
    NSTrackingArea* trackingArea;
    NSImage* image;
}

@property (retain, nonatomic) NSStatusItem* item;
@property (assign, nonatomic) WineEventQueue* queue;
@property (retain, nonatomic) NSImage* image;

@end


@implementation WineStatusItem

@synthesize item, queue, image;

    - (id) initWithEventQueue:(WineEventQueue*)inQueue
    {
        NSStatusBar* statusBar = [NSStatusBar systemStatusBar];
        CGFloat thickness = [statusBar thickness];

        self = [super initWithFrame:NSMakeRect(0, 0, thickness, thickness)];
        if (self)
        {
            item = [[statusBar statusItemWithLength:NSSquareStatusItemLength] retain];
            // This is a retain cycle which is broken in -removeFromStatusBar.
            [item setView:self];

            queue = inQueue;

            trackingArea = [[NSTrackingArea alloc] initWithRect:[self bounds]
                                                        options:NSTrackingMouseMoved | NSTrackingActiveAlways | NSTrackingInVisibleRect
                                                          owner:self
                                                       userInfo:nil];
            [self addTrackingArea:trackingArea];
        }
        return self;
    }

    - (void) dealloc
    {
        if (item)
        {
            NSStatusBar* statusBar = [NSStatusBar systemStatusBar];
            [statusBar removeStatusItem:item];
            [item release];
        }
        [image release];
        [trackingArea release];
        [super dealloc];
    }

    - (void) setImage:(NSImage*)inImage
    {
        if (image != inImage)
        {
            [image release];
            image = [inImage retain];
            [self setNeedsDisplay:YES];
        }
    }

    - (void) removeFromStatusBar
    {
        if (item)
        {
            NSStatusBar* statusBar = [NSStatusBar systemStatusBar];
            [statusBar removeStatusItem:item];
            [item setView:nil];

            [queue discardEventsPassingTest:^BOOL (macdrv_event* event){
                return ((event->type == STATUS_ITEM_MOUSE_BUTTON && event->status_item_mouse_button.item == (macdrv_status_item)self) ||
                        (event->type == STATUS_ITEM_MOUSE_MOVE && event->status_item_mouse_move.item == (macdrv_status_item)self));
            }];

            self.item = nil;
        }
    }

    - (void) postMouseButtonEvent:(NSEvent*)nsevent;
    {
        macdrv_event* event;
        NSUInteger typeMask = NSEventMaskFromType([nsevent type]);
        CGPoint point = CGEventGetLocation([nsevent CGEvent]);

        point = cgpoint_win_from_mac(point);

        event = macdrv_create_event(STATUS_ITEM_MOUSE_BUTTON, nil);
        event->status_item_mouse_button.item = (macdrv_status_item)self;
        event->status_item_mouse_button.button = [nsevent buttonNumber];
        event->status_item_mouse_button.down = (typeMask & (NSEventMaskLeftMouseDown |
                                                            NSEventMaskRightMouseDown |
                                                            NSEventMaskOtherMouseDown)) != 0;
        event->status_item_mouse_button.count = [nsevent clickCount];
        event->status_item_mouse_button.x = floor(point.x);
        event->status_item_mouse_button.y = floor(point.y);
        [queue postEvent:event];
        macdrv_release_event(event);
    }


    /*
     * ---------- NSView methods ----------
     */
    - (void) drawRect:(NSRect)rect
    {
        [item drawStatusBarBackgroundInRect:[self bounds] withHighlight:NO];

        if (image)
        {
            NSSize imageSize = [image size];
            NSRect bounds = [self bounds];
            NSPoint imageOrigin = NSMakePoint(NSMidX(bounds) - imageSize.width / 2,
                                              NSMidY(bounds) - imageSize.height / 2);

            imageOrigin = [self convertPointToBase:imageOrigin];
            imageOrigin.x = floor(imageOrigin.x);
            imageOrigin.y = floor(imageOrigin.y);
            imageOrigin = [self convertPointFromBase:imageOrigin];

            [image drawAtPoint:imageOrigin
                      fromRect:NSZeroRect
                     operation:NSCompositingOperationSourceOver
                      fraction:1];
        }
    }


    /*
     * ---------- NSResponder methods ----------
     */
    - (void) mouseDown:(NSEvent*)event
    {
        [self postMouseButtonEvent:event];
    }

    - (void) mouseDragged:(NSEvent*)event
    {
        [self mouseMoved:event];
    }

    - (void) mouseMoved:(NSEvent*)nsevent
    {
        macdrv_event* event;
        CGPoint point = CGEventGetLocation([nsevent CGEvent]);

        point = cgpoint_win_from_mac(point);

        event = macdrv_create_event(STATUS_ITEM_MOUSE_MOVE, nil);
        event->status_item_mouse_move.item = (macdrv_status_item)self;
        event->status_item_mouse_move.x = floor(point.x);
        event->status_item_mouse_move.y = floor(point.y);
        [queue postEvent:event];
        macdrv_release_event(event);
    }

    - (void) mouseUp:(NSEvent*)event
    {
        [self postMouseButtonEvent:event];
    }

    - (void) otherMouseDown:(NSEvent*)event
    {
        [self postMouseButtonEvent:event];
    }

    - (void) otherMouseDragged:(NSEvent*)event
    {
        [self mouseMoved:event];
    }

    - (void) otherMouseUp:(NSEvent*)event
    {
        [self postMouseButtonEvent:event];
    }

    - (void) rightMouseDown:(NSEvent*)event
    {
        [self postMouseButtonEvent:event];
    }

    - (void) rightMouseDragged:(NSEvent*)event
    {
        [self mouseMoved:event];
    }

    - (void) rightMouseUp:(NSEvent*)event
    {
        [self postMouseButtonEvent:event];
    }

@end


/***********************************************************************
 *              macdrv_create_status_item
 *
 * Creates a new status item in the status bar.
 */
macdrv_status_item macdrv_create_status_item(macdrv_event_queue q)
{
    WineEventQueue* queue = (WineEventQueue*)q;
    __block WineStatusItem* statusItem;

    OnMainThread(^{
        statusItem = [[WineStatusItem alloc] initWithEventQueue:queue];
    });

    return (macdrv_status_item)statusItem;
}

/***********************************************************************
 *              macdrv_destroy_status_item
 *
 * Removes a status item previously returned by
 * macdrv_create_status_item() from the status bar and destroys it.
 */
void macdrv_destroy_status_item(macdrv_status_item s)
{
    WineStatusItem* statusItem = (WineStatusItem*)s;

    OnMainThreadAsync(^{
        [statusItem removeFromStatusBar];
        [statusItem release];
    });
}

/***********************************************************************
 *              macdrv_set_status_item_image
 *
 * Sets the image for a status item.  If cgimage is NULL, clears the
 * image of the status item (leaving it a blank spot on the menu bar).
 */
void macdrv_set_status_item_image(macdrv_status_item s, CGImageRef cgimage)
{
    WineStatusItem* statusItem = (WineStatusItem*)s;

    CGImageRetain(cgimage);

    OnMainThreadAsync(^{
        NSImage* image = nil;
        if (cgimage)
        {
            NSSize size;
            CGFloat maxSize = [[NSStatusBar systemStatusBar] thickness];
            BOOL changed = FALSE;

            image = [[NSImage alloc] initWithCGImage:cgimage size:NSZeroSize];
            CGImageRelease(cgimage);
            size = [image size];
            while (size.width > maxSize || size.height > maxSize)
            {
                size.width /= 2.0;
                size.height /= 2.0;
                changed = TRUE;
            }
            if (changed)
                [image setSize:size];
        }
        statusItem.image = image;
        [image release];
    });
}

/***********************************************************************
 *              macdrv_set_status_item_tooltip
 *
 * Sets the tooltip string for a status item.  If cftip is NULL, clears
 * the tooltip string for the status item.
 */
void macdrv_set_status_item_tooltip(macdrv_status_item s, CFStringRef cftip)
{
    WineStatusItem* statusItem = (WineStatusItem*)s;
    NSString* tip = (NSString*)cftip;

    if (![tip length]) tip = nil;
    OnMainThreadAsync(^{
        [statusItem setToolTip:tip];
    });
}
