/*
 * MACDRV Cocoa event queue declarations
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

#import <AppKit/AppKit.h>
#include "macdrv_cocoa.h"


enum {
    WineQueryProcessEvents = 1 << 0,
    WineQueryNoPreemptWait = 1 << 1,
};


@interface NSEvent (WineExtensions)
    + (BOOL) wine_commandKeyDown;
    - (BOOL) wine_commandKeyDown;
@end


@class WineWindow;


@interface WineEventQueue : NSObject
{
    NSMutableArray* events;
    NSLock*         eventsLock;

    int             fds[2]; /* Pipe signaling when there are events queued. */
    int             kq; /* kqueue for waiting in OnMainThread(). */

    macdrv_event_handler event_handler;

    NSMutableDictionary* hotKeysByMacID;
    NSMutableDictionary* hotKeysByWinID;
}

    - (void) postEvent:(macdrv_event*)inEvent;
    - (void) discardEventsMatchingMask:(macdrv_event_mask)mask forWindow:(NSWindow*)window;
    - (void) discardEventsPassingTest:(BOOL (^)(macdrv_event* event))block;

    - (BOOL) query:(macdrv_query*)query timeout:(NSTimeInterval)timeout flags:(NSUInteger)flags;
    - (BOOL) query:(macdrv_query*)query timeout:(NSTimeInterval)timeout;

    - (void) resetMouseEventPositions:(CGPoint)pos;

@end

void OnMainThread(dispatch_block_t block);

macdrv_event* macdrv_create_event(int type, WineWindow* window);
macdrv_event* macdrv_retain_event(macdrv_event *event);
