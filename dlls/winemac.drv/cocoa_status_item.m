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


@interface WineStatusItem : NSObject
{
    NSStatusItem* item;
    WineEventQueue* queue;
}

@property (retain, nonatomic) NSStatusItem* item;
@property (assign, nonatomic) WineEventQueue* queue;

@end


@implementation WineStatusItem

@synthesize item, queue;

    - (id) initWithEventQueue:(WineEventQueue*)inQueue
    {
        self = [super init];
        if (self)
        {
            NSStatusBar* statusBar = [NSStatusBar systemStatusBar];
            item = [[statusBar statusItemWithLength:NSSquareStatusItemLength] retain];
            [item setTarget:self];
            [item setAction:@selector(clicked:)];
            [item setDoubleAction:@selector(doubleClicked:)];

            queue = inQueue;
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
        [super dealloc];
    }

    - (void) removeFromStatusBar
    {
        if (item)
        {
            NSStatusBar* statusBar = [NSStatusBar systemStatusBar];
            [statusBar removeStatusItem:item];
            self.item = nil;
        }
    }

    - (void) postClickedEventWithCount:(int)count
    {
        macdrv_event* event;
        event = macdrv_create_event(STATUS_ITEM_CLICKED, nil);
        event->status_item_clicked.item = (macdrv_status_item)self;
        event->status_item_clicked.count = count;
        [queue postEvent:event];
        macdrv_release_event(event);
    }

    - (void) clicked:(id)sender
    {
        [self postClickedEventWithCount:1];
    }

    - (void) doubleClicked:(id)sender
    {
        [self postClickedEventWithCount:2];
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
        [statusItem.item setImage:image];
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
        [statusItem.item setToolTip:tip];
    });
}
