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


static NSString* const WineEventQueueThreadDictionaryKey = @"WineEventQueueThreadDictionaryKey";


@interface MacDrvEvent : NSObject
{
@public
    macdrv_event* event;
}

    - (id) initWithEvent:(macdrv_event*)event;

@end

@implementation MacDrvEvent

    - (id) initWithEvent:(macdrv_event*)inEvent
    {
        self = [super init];
        if (self)
        {
            event = macdrv_retain_event(inEvent);
        }
        return self;
    }

    - (void) dealloc
    {
        if (event) macdrv_release_event(event);
        [super dealloc];
    }

@end


@implementation WineEventQueue

    - (id) init
    {
        [self doesNotRecognizeSelector:_cmd];
        [self release];
        return nil;
    }

    - (id) initWithEventHandler:(macdrv_event_handler)handler
    {
        NSParameterAssert(handler != nil);

        self = [super init];
        if (self != nil)
        {
            struct kevent kev;
            int rc;

            fds[0] = fds[1] = kq = -1;

            event_handler = handler;
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

            kq = kqueue();
            if (kq < 0)
            {
                [self release];
                return nil;
            }

            EV_SET(&kev, fds[0], EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0);
            do
            {
                rc = kevent(kq, &kev, 1, NULL, 0, NULL);
            } while (rc == -1 && errno == EINTR);
            if (rc == -1)
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

        if (kq != -1) close(kq);
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

        if ((event->event->type == MOUSE_MOVED ||
             event->event->type == MOUSE_MOVED_ABSOLUTE) &&
            (lastEvent = [events lastObject]) &&
            (lastEvent->event->type == MOUSE_MOVED ||
             lastEvent->event->type == MOUSE_MOVED_ABSOLUTE) &&
            lastEvent->event->window == event->event->window &&
            lastEvent->event->mouse_moved.drag == event->event->mouse_moved.drag)
        {
            if (event->event->type == MOUSE_MOVED)
            {
                lastEvent->event->mouse_moved.x += event->event->mouse_moved.x;
                lastEvent->event->mouse_moved.y += event->event->mouse_moved.y;
            }
            else
            {
                lastEvent->event->type = MOUSE_MOVED_ABSOLUTE;
                lastEvent->event->mouse_moved.x = event->event->mouse_moved.x;
                lastEvent->event->mouse_moved.y = event->event->mouse_moved.y;
            }

            lastEvent->event->mouse_moved.time_ms = event->event->mouse_moved.time_ms;
        }
        else
            [events addObject:event];

        [eventsLock unlock];

        [self signalEventAvailable];
    }

    - (void) postEvent:(macdrv_event*)inEvent
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
        MacDrvEvent* ret = nil;

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
        while (index < [events count])
        {
            MacDrvEvent* event = [events objectAtIndex:index];
            if (event_mask_for_type(event->event->type) & mask)
            {
                [[event retain] autorelease];
                [events removeObjectAtIndex:index];

                if (event->event->deliver == INT_MAX ||
                    OSAtomicDecrement32Barrier(&event->event->deliver) >= 0)
                {
                    ret = event;
                    break;
                }
            }
            else
                index++;
        }

        [eventsLock unlock];
        return ret;
    }

    - (void) discardEventsMatchingMask:(macdrv_event_mask)mask forWindow:(NSWindow*)window
    {
        NSIndexSet* indexes;

        [eventsLock lock];

        indexes = [events indexesOfObjectsPassingTest:^BOOL(id obj, NSUInteger idx, BOOL *stop){
            MacDrvEvent* event = obj;
            return ((event_mask_for_type(event->event->type) & mask) &&
                    (!window || event->event->window == (macdrv_window)window));
        }];

        [events removeObjectsAtIndexes:indexes];

        [eventsLock unlock];
    }

    - (BOOL) query:(macdrv_query*)query timeout:(NSTimeInterval)timeout processEvents:(BOOL)processEvents
    {
        macdrv_event* event;
        NSDate* timeoutDate = [NSDate dateWithTimeIntervalSinceNow:timeout];
        BOOL timedout;

        event = macdrv_create_event(QUERY_EVENT, (WineWindow*)query->window);
        event->query_event.query = macdrv_retain_query(query);
        query->done = FALSE;

        [self postEvent:event];
        macdrv_release_event(event);
        timedout = ![[WineApplicationController sharedController] waitUntilQueryDone:&query->done
                                                                             timeout:timeoutDate
                                                                       processEvents:processEvents];
        return !timedout && query->status;
    }

    - (BOOL) query:(macdrv_query*)query timeout:(NSTimeInterval)timeout
    {
        return [self query:query timeout:timeout processEvents:FALSE];
    }

    - (void) resetMouseEventPositions:(CGPoint)pos
    {
        MacDrvEvent* event;

        [eventsLock lock];

        for (event in events)
        {
            if (event->event->type == MOUSE_BUTTON)
            {
                event->event->mouse_button.x = pos.x;
                event->event->mouse_button.y = pos.y;
            }
            else if (event->event->type == MOUSE_SCROLL)
            {
                event->event->mouse_scroll.x = pos.x;
                event->event->mouse_scroll.y = pos.y;
            }
        }

        [eventsLock unlock];
    }


/***********************************************************************
 *              OnMainThread
 *
 * Run a block on the main thread synchronously.
 */
void OnMainThread(dispatch_block_t block)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    NSMutableDictionary* threadDict = [[NSThread currentThread] threadDictionary];
    WineEventQueue* queue = [threadDict objectForKey:WineEventQueueThreadDictionaryKey];
    __block BOOL finished;

    if (!queue)
    {
        /* Fall back to synchronous dispatch without handling query events. */
        dispatch_sync(dispatch_get_main_queue(), block);
        [pool release];
        return;
    }

    finished = FALSE;
    OnMainThreadAsync(^{
        block();
        finished = TRUE;
        [queue signalEventAvailable];
    });

    while (!finished)
    {
        MacDrvEvent* macDrvEvent;
        struct kevent kev;

        while (!finished &&
               (macDrvEvent = [queue getEventMatchingMask:event_mask_for_type(QUERY_EVENT)]))
        {
            queue->event_handler(macDrvEvent->event);
        }

        if (!finished)
        {
            [pool release];
            pool = [[NSAutoreleasePool alloc] init];

            kevent(queue->kq, NULL, 0, &kev, 1, NULL);
        }
    }

    [pool release];
}


/***********************************************************************
 *              macdrv_create_event_queue
 *
 * Register this thread with the application on the main thread, and set
 * up an event queue on which it can deliver events to this thread.
 */
macdrv_event_queue macdrv_create_event_queue(macdrv_event_handler handler)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    NSMutableDictionary* threadDict = [[NSThread currentThread] threadDictionary];

    WineEventQueue* queue = [threadDict objectForKey:WineEventQueueThreadDictionaryKey];
    if (!queue)
    {
        queue = [[[WineEventQueue alloc] initWithEventHandler:handler] autorelease];
        if (queue)
        {
            if ([[WineApplicationController sharedController] registerEventQueue:queue])
                [threadDict setObject:queue forKey:WineEventQueueThreadDictionaryKey];
            else
                queue = nil;
        }
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
    NSMutableDictionary* threadDict = [[NSThread currentThread] threadDictionary];

    [[WineApplicationController sharedController] unregisterEventQueue:q];
    [threadDict removeObjectForKey:WineEventQueueThreadDictionaryKey];

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
 *              macdrv_copy_event_from_queue
 *
 * Pull an event matching the event mask from the event queue and store
 * it in the event record pointed to by the event parameter.  If a
 * matching event was found, return non-zero; otherwise, return 0.
 *
 * The caller is responsible for calling macdrv_release_event on any
 * event returned by this function.
 */
int macdrv_copy_event_from_queue(macdrv_event_queue queue,
        macdrv_event_mask mask, macdrv_event **event)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    WineEventQueue* q = (WineEventQueue*)queue;

    MacDrvEvent* macDrvEvent = [q getEventMatchingMask:mask];
    if (macDrvEvent)
        *event = macdrv_retain_event(macDrvEvent->event);

    [pool release];
    return (macDrvEvent != nil);
}

/***********************************************************************
 *              macdrv_create_event
 */
macdrv_event* macdrv_create_event(int type, WineWindow* window)
{
    macdrv_event *event;

    event = calloc(1, sizeof(*event));
    event->refs = 1;
    event->deliver = INT_MAX;
    event->type = type;
    event->window = (macdrv_window)[window retain];
    return event;
}

/***********************************************************************
 *              macdrv_retain_event
 */
macdrv_event* macdrv_retain_event(macdrv_event *event)
{
    OSAtomicIncrement32Barrier(&event->refs);
    return event;
}

/***********************************************************************
 *              macdrv_release_event
 *
 * Decrements the reference count of an event.  If the count falls to
 * zero, cleans up any resources, such as allocated memory or retained
 * objects, held by the event and deallocates it
 */
void macdrv_release_event(macdrv_event *event)
{
    if (OSAtomicDecrement32Barrier(&event->refs) <= 0)
    {
        NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

        switch (event->type)
        {
            case IM_SET_TEXT:
                if (event->im_set_text.text)
                    CFRelease(event->im_set_text.text);
                break;
            case KEYBOARD_CHANGED:
                CFRelease(event->keyboard_changed.uchr);
                break;
            case QUERY_EVENT:
                macdrv_release_query(event->query_event.query);
                break;
            case WINDOW_GOT_FOCUS:
                [(NSMutableSet*)event->window_got_focus.tried_windows release];
                break;
        }

        [(WineWindow*)event->window release];
        free(event);

        [pool release];
    }
}

/***********************************************************************
 *              macdrv_create_query
 */
macdrv_query* macdrv_create_query(void)
{
    macdrv_query *query;

    query = calloc(1, sizeof(*query));
    query->refs = 1;
    return query;
}

/***********************************************************************
 *              macdrv_retain_query
 */
macdrv_query* macdrv_retain_query(macdrv_query *query)
{
    OSAtomicIncrement32Barrier(&query->refs);
    return query;
}

/***********************************************************************
 *              macdrv_release_query
 */
void macdrv_release_query(macdrv_query *query)
{
    if (OSAtomicDecrement32Barrier(&query->refs) <= 0)
    {
        switch (query->type)
        {
            case QUERY_DRAG_OPERATION:
                if (query->drag_operation.pasteboard)
                    CFRelease(query->drag_operation.pasteboard);
                break;
            case QUERY_DRAG_DROP:
                if (query->drag_drop.pasteboard)
                    CFRelease(query->drag_drop.pasteboard);
                break;
            case QUERY_PASTEBOARD_DATA:
                if (query->pasteboard_data.type)
                    CFRelease(query->pasteboard_data.type);
                break;
        }
        [(WineWindow*)query->window release];
        free(query);
    }
}

/***********************************************************************
 *              macdrv_set_query_done
 */
void macdrv_set_query_done(macdrv_query *query)
{
    macdrv_retain_query(query);

    OnMainThreadAsync(^{
        NSEvent* event;

        query->done = TRUE;
        macdrv_release_query(query);

        event = [NSEvent otherEventWithType:NSApplicationDefined
                                   location:NSZeroPoint
                              modifierFlags:0
                                  timestamp:[[NSProcessInfo processInfo] systemUptime]
                               windowNumber:0
                                    context:nil
                                    subtype:WineApplicationEventWakeQuery
                                      data1:0
                                      data2:0];
        [NSApp postEvent:event atStart:TRUE];
    });
}

@end
