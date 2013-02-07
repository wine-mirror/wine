/*
 * MACDRV Cocoa event queue code
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

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <libkern/OSAtomic.h>

#include "macdrv_cocoa.h"
#import "cocoa_event.h"
#import "cocoa_app.h"
#import "cocoa_window.h"


@interface MacDrvEvent : NSObject
{
@public
    macdrv_event event;
}

    - (id) initWithEvent:(const macdrv_event*)event;

@end

@implementation MacDrvEvent

    - (id) initWithEvent:(const macdrv_event*)inEvent
    {
        self = [super init];
        if (self)
        {
            event = *inEvent;
        }
        return self;
    }

@end


@implementation WineEventQueue

    - (id) init
    {
        self = [super init];
        if (self != nil)
        {
            fds[0] = fds[1] = -1;

            events = [[NSMutableArray alloc] init];
            eventsLock = [[NSLock alloc] init];

            if (!events || !eventsLock)
            {
                [self release];
                return nil;
            }

            if (pipe(fds) ||
                fcntl(fds[0], F_SETFD, 1) == -1 ||
                fcntl(fds[0], F_SETFL, O_NONBLOCK) == -1 ||
                fcntl(fds[1], F_SETFD, 1) == -1 ||
                fcntl(fds[1], F_SETFL, O_NONBLOCK) == -1)
            {
                [self release];
                return nil;
            }
        }
        return self;
    }

    - (void) dealloc
    {
        [events release];
        [eventsLock release];

        if (fds[0] != -1) close(fds[0]);
        if (fds[1] != -1) close(fds[1]);

        [super dealloc];
    }

    - (void) signalEventAvailable
    {
        char junk = 1;
        int rc;

        do
        {
            rc = write(fds[1], &junk, 1);
        } while (rc < 0 && errno == EINTR);

        if (rc < 0 && errno != EAGAIN)
            ERR(@"%@: got error writing to event queue signaling pipe: %s\n", self, strerror(errno));
    }

    - (void) postEventObject:(MacDrvEvent*)event
    {
        MacDrvEvent* lastEvent;

        [eventsLock lock];

        if ((event->event.type == MOUSE_MOVED ||
             event->event.type == MOUSE_MOVED_ABSOLUTE) &&
            (lastEvent = [events lastObject]) &&
            (lastEvent->event.type == MOUSE_MOVED ||
             lastEvent->event.type == MOUSE_MOVED_ABSOLUTE) &&
            lastEvent->event.window == event->event.window)
        {
            if (event->event.type == MOUSE_MOVED)
            {
                lastEvent->event.mouse_moved.x += event->event.mouse_moved.x;
                lastEvent->event.mouse_moved.y += event->event.mouse_moved.y;
            }
            else
            {
                lastEvent->event.type = MOUSE_MOVED_ABSOLUTE;
                lastEvent->event.mouse_moved.x = event->event.mouse_moved.x;
                lastEvent->event.mouse_moved.y = event->event.mouse_moved.y;
            }

            lastEvent->event.mouse_moved.time_ms = event->event.mouse_moved.time_ms;

            macdrv_cleanup_event(&event->event);
        }
        else
            [events addObject:event];

        [eventsLock unlock];

        [self signalEventAvailable];
    }

    - (void) postEvent:(const macdrv_event*)inEvent
    {
        MacDrvEvent* event = [[MacDrvEvent alloc] initWithEvent:inEvent];
        [self postEventObject:event];
        [event release];
    }

    - (MacDrvEvent*) getEventMatchingMask:(macdrv_event_mask)mask
    {
        char buf[512];
        int rc;
        NSUInteger index;
        MacDrvEvent* event;

        /* Clear the pipe which signals there are pending events. */
        do
        {
            rc = read(fds[0], buf, sizeof(buf));
        } while (rc > 0 || (rc < 0 && errno == EINTR));
        if (rc == 0 || (rc < 0 && errno != EAGAIN))
        {
            if (rc == 0)
                ERR(@"%@: event queue signaling pipe unexpectedly closed\n", self);
            else
                ERR(@"%@: got error reading from event queue signaling pipe: %s\n", self, strerror(errno));
            return nil;
        }

        [eventsLock lock];

        index = 0;
        for (event in events)
        {
            if (event_mask_for_type(event->event.type) & mask)
                break;

            index++;
        }

        if (event)
        {
            [event retain];
            [events removeObjectAtIndex:index];
        }

        [eventsLock unlock];
        return [event autorelease];
    }

    - (void) discardEventsMatchingMask:(macdrv_event_mask)mask forWindow:(NSWindow*)window
    {
        NSMutableIndexSet* indexes = [[[NSMutableIndexSet alloc] init] autorelease];

        [eventsLock lock];

        [events enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop){
            MacDrvEvent* event = obj;
            if ((event_mask_for_type(event->event.type) & mask) &&
                (!window || event->event.window == (macdrv_window)window))
            {
                macdrv_cleanup_event(&event->event);
                [indexes addIndex:idx];
            }
        }];

        [events removeObjectsAtIndexes:indexes];

        [eventsLock unlock];
    }


/***********************************************************************
 *              macdrv_create_event_queue
 *
 * Register this thread with the application on the main thread, and set
 * up an event queue on which it can deliver events to this thread.
 */
macdrv_event_queue macdrv_create_event_queue(void)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

    WineEventQueue* queue = [[WineEventQueue alloc] init];
    if (queue && ![NSApp registerEventQueue:queue])
    {
        [queue release];
        queue = nil;
    }

    [pool release];
    return (macdrv_event_queue)queue;
}

/***********************************************************************
 *              macdrv_destroy_event_queue
 *
 * Tell the application that this thread is exiting and destroy the
 * associated event queue.
 */
void macdrv_destroy_event_queue(macdrv_event_queue queue)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    WineEventQueue* q = (WineEventQueue*)queue;

    [NSApp unregisterEventQueue:q];
    [q release];

    [pool release];
}

/***********************************************************************
 *              macdrv_get_event_queue_fd
 *
 * Get the file descriptor whose readability signals that there are
 * events on the event queue.
 */
int macdrv_get_event_queue_fd(macdrv_event_queue queue)
{
    WineEventQueue* q = (WineEventQueue*)queue;
    return q->fds[0];
}

/***********************************************************************
 *              macdrv_get_event_from_queue
 *
 * Pull an event matching the event mask from the event queue and store
 * it in the event record pointed to by the event parameter.  If a
 * matching event was found, return non-zero; otherwise, return 0.
 *
 * The caller is responsible for calling macdrv_cleanup_event on any
 * event returned by this function.
 */
int macdrv_get_event_from_queue(macdrv_event_queue queue,
        macdrv_event_mask mask, macdrv_event *event)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    WineEventQueue* q = (WineEventQueue*)queue;

    MacDrvEvent* macDrvEvent = [q getEventMatchingMask:mask];
    if (macDrvEvent)
        *event = macDrvEvent->event;

    [pool release];
    return (macDrvEvent != nil);
}

/***********************************************************************
 *              macdrv_cleanup_event
 *
 * Performs cleanup of an event.  For event types which carry resources
 * such as allocated memory or retained objects, frees/releases those
 * resources.
 */
void macdrv_cleanup_event(macdrv_event *event)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

    switch (event->type)
    {
        case KEYBOARD_CHANGED:
            CFRelease(event->keyboard_changed.uchr);
            break;
        case WINDOW_GOT_FOCUS:
            [(NSMutableSet*)event->window_got_focus.tried_windows release];
            break;
    }

    [(WineWindow*)event->window release];

    [pool release];
}

@end
