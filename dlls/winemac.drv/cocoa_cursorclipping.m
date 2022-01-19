/*
 * MACDRV CGEventTap-based cursor clipping class
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

#import "cocoa_app.h"
#import "cocoa_cursorclipping.h"


/* Neither Quartz nor Cocoa has an exact analog for Win32 cursor clipping.
 * For one simple case, clipping to a 1x1 rectangle, Quartz does have an
 * equivalent: CGAssociateMouseAndMouseCursorPosition(false).  For the
 * general case, we leverage that.  We disassociate mouse movements from
 * the cursor position and then move the cursor manually, keeping it within
 * the clipping rectangle.
 *
 * Moving the cursor manually isn't enough.  We need to modify the event
 * stream so that the events have the new location, too.  We need to do
 * this at a point before the events enter Cocoa, so that Cocoa will assign
 * the correct window to the event.  So, we install a Quartz event tap to
 * do that.
 *
 * Also, there's a complication when we move the cursor.  We use
 * CGWarpMouseCursorPosition().  That doesn't generate mouse movement
 * events, but the change of cursor position is incorporated into the
 * deltas of the next mouse move event.  When the mouse is disassociated
 * from the cursor position, we need the deltas to only reflect actual
 * device movement, not programmatic changes.  So, the event tap cancels
 * out the change caused by our calls to CGWarpMouseCursorPosition().
 */


@interface WarpRecord : NSObject
{
    CGEventTimestamp timeBefore, timeAfter;
    CGPoint from, to;
}

@property (nonatomic) CGEventTimestamp timeBefore;
@property (nonatomic) CGEventTimestamp timeAfter;
@property (nonatomic) CGPoint from;
@property (nonatomic) CGPoint to;

@end


@implementation WarpRecord

@synthesize timeBefore, timeAfter, from, to;

@end;


@implementation WineEventTapClipCursorHandler

@synthesize clippingCursor, cursorClipRect;

    - (id) init
    {
        self = [super init];
        if (self)
        {
            warpRecords = [[NSMutableArray alloc] init];
        }

        return self;
    }

    - (void) dealloc
    {
        [warpRecords release];
        [super dealloc];
    }

    - (void) clipCursorLocation:(CGPoint*)location
    {
        if (location->x < CGRectGetMinX(cursorClipRect))
            location->x = CGRectGetMinX(cursorClipRect);
        if (location->y < CGRectGetMinY(cursorClipRect))
            location->y = CGRectGetMinY(cursorClipRect);
        if (location->x > CGRectGetMaxX(cursorClipRect) - 1)
            location->x = CGRectGetMaxX(cursorClipRect) - 1;
        if (location->y > CGRectGetMaxY(cursorClipRect) - 1)
            location->y = CGRectGetMaxY(cursorClipRect) - 1;
    }

    - (BOOL) warpCursorTo:(CGPoint*)newLocation from:(const CGPoint*)currentLocation
    {
        CGPoint oldLocation;

        if (currentLocation)
            oldLocation = *currentLocation;
        else
            oldLocation = NSPointToCGPoint([[WineApplicationController sharedController] flippedMouseLocation:[NSEvent mouseLocation]]);

        if (!CGPointEqualToPoint(oldLocation, *newLocation))
        {
            WarpRecord* warpRecord = [[[WarpRecord alloc] init] autorelease];
            CGError err;

            warpRecord.from = oldLocation;
            warpRecord.timeBefore = [[NSProcessInfo processInfo] systemUptime] * NSEC_PER_SEC;

            /* Actually move the cursor. */
            err = CGWarpMouseCursorPosition(*newLocation);
            if (err != kCGErrorSuccess)
                return FALSE;

            warpRecord.timeAfter = [[NSProcessInfo processInfo] systemUptime] * NSEC_PER_SEC;
            *newLocation = NSPointToCGPoint([[WineApplicationController sharedController] flippedMouseLocation:[NSEvent mouseLocation]]);

            if (!CGPointEqualToPoint(oldLocation, *newLocation))
            {
                warpRecord.to = *newLocation;
                [warpRecords addObject:warpRecord];
            }
        }

        return TRUE;
    }

    - (BOOL) isMouseMoveEventType:(CGEventType)type
    {
        switch(type)
        {
        case kCGEventMouseMoved:
        case kCGEventLeftMouseDragged:
        case kCGEventRightMouseDragged:
        case kCGEventOtherMouseDragged:
            return TRUE;
        default:
            return FALSE;
        }
    }

    - (int) warpsFinishedByEventTime:(CGEventTimestamp)eventTime location:(CGPoint)eventLocation
    {
        int warpsFinished = 0;
        for (WarpRecord* warpRecord in warpRecords)
        {
            if (warpRecord.timeAfter < eventTime ||
                (warpRecord.timeBefore <= eventTime && CGPointEqualToPoint(eventLocation, warpRecord.to)))
                warpsFinished++;
            else
                break;
        }

        return warpsFinished;
    }

    - (CGEventRef) eventTapWithProxy:(CGEventTapProxy)proxy
                                type:(CGEventType)type
                               event:(CGEventRef)event
    {
        CGEventTimestamp eventTime;
        CGPoint eventLocation, cursorLocation;

        if (type == kCGEventTapDisabledByUserInput)
            return event;
        if (type == kCGEventTapDisabledByTimeout)
        {
            CGEventTapEnable(cursorClippingEventTap, TRUE);
            return event;
        }

        if (!clippingCursor)
            return event;

        eventTime = CGEventGetTimestamp(event);
        lastEventTapEventTime = eventTime / (double)NSEC_PER_SEC;

        eventLocation = CGEventGetLocation(event);

        cursorLocation = NSPointToCGPoint([[WineApplicationController sharedController] flippedMouseLocation:[NSEvent mouseLocation]]);

        if ([self isMouseMoveEventType:type])
        {
            double deltaX, deltaY;
            int warpsFinished = [self warpsFinishedByEventTime:eventTime location:eventLocation];
            int i;

            deltaX = CGEventGetDoubleValueField(event, kCGMouseEventDeltaX);
            deltaY = CGEventGetDoubleValueField(event, kCGMouseEventDeltaY);

            for (i = 0; i < warpsFinished; i++)
            {
                WarpRecord* warpRecord = [warpRecords objectAtIndex:0];
                deltaX -= warpRecord.to.x - warpRecord.from.x;
                deltaY -= warpRecord.to.y - warpRecord.from.y;
                [warpRecords removeObjectAtIndex:0];
            }

            if (warpsFinished)
            {
                CGEventSetDoubleValueField(event, kCGMouseEventDeltaX, deltaX);
                CGEventSetDoubleValueField(event, kCGMouseEventDeltaY, deltaY);
            }

            synthesizedLocation.x += deltaX;
            synthesizedLocation.y += deltaY;
        }

        // If the event is destined for another process, don't clip it.  This may
        // happen if the user activates Exposé or Mission Control.  In that case,
        // our app does not resign active status, so clipping is still in effect,
        // but the cursor should not actually be clipped.
        //
        // In addition, the fact that mouse moves may have been delivered to a
        // different process means we have to treat the next one we receive as
        // absolute rather than relative.
        if (CGEventGetIntegerValueField(event, kCGEventTargetUnixProcessID) == getpid())
            [self clipCursorLocation:&synthesizedLocation];
        else
            [WineApplicationController sharedController].lastSetCursorPositionTime = lastEventTapEventTime;

        [self warpCursorTo:&synthesizedLocation from:&cursorLocation];
        if (!CGPointEqualToPoint(eventLocation, synthesizedLocation))
            CGEventSetLocation(event, synthesizedLocation);

        return event;
    }

    CGEventRef WineAppEventTapCallBack(CGEventTapProxy proxy, CGEventType type,
                                       CGEventRef event, void *refcon)
    {
        WineEventTapClipCursorHandler* handler = refcon;
        return [handler eventTapWithProxy:proxy type:type event:event];
    }

    - (BOOL) installEventTap
    {
        CGEventMask mask = CGEventMaskBit(kCGEventLeftMouseDown)        |
                           CGEventMaskBit(kCGEventLeftMouseUp)          |
                           CGEventMaskBit(kCGEventRightMouseDown)       |
                           CGEventMaskBit(kCGEventRightMouseUp)         |
                           CGEventMaskBit(kCGEventMouseMoved)           |
                           CGEventMaskBit(kCGEventLeftMouseDragged)     |
                           CGEventMaskBit(kCGEventRightMouseDragged)    |
                           CGEventMaskBit(kCGEventOtherMouseDown)       |
                           CGEventMaskBit(kCGEventOtherMouseUp)         |
                           CGEventMaskBit(kCGEventOtherMouseDragged)    |
                           CGEventMaskBit(kCGEventScrollWheel);
        CFRunLoopSourceRef source;

        if (cursorClippingEventTap)
            return TRUE;

        // We create an annotated session event tap rather than a process-specific
        // event tap because we need to programmatically move the cursor even when
        // mouse moves are directed to other processes.  We disable our tap when
        // other processes are active, but things like Exposé are handled by other
        // processes even when we remain active.
        cursorClippingEventTap = CGEventTapCreate(kCGAnnotatedSessionEventTap, kCGHeadInsertEventTap,
            kCGEventTapOptionDefault, mask, WineAppEventTapCallBack, self);
        if (!cursorClippingEventTap)
            return FALSE;

        CGEventTapEnable(cursorClippingEventTap, FALSE);

        source = CFMachPortCreateRunLoopSource(NULL, cursorClippingEventTap, 0);
        if (!source)
        {
            CFRelease(cursorClippingEventTap);
            cursorClippingEventTap = NULL;
            return FALSE;
        }

        CFRunLoopAddSource(CFRunLoopGetCurrent(), source, kCFRunLoopCommonModes);
        CFRelease(source);
        return TRUE;
    }

    - (BOOL) setCursorPosition:(CGPoint)pos
    {
        BOOL ret;

        [self clipCursorLocation:&pos];

        ret = [self warpCursorTo:&pos from:NULL];
        synthesizedLocation = pos;
        if (ret)
        {
            // We want to discard mouse-move events that have already been
            // through the event tap, because it's too late to account for
            // the setting of the cursor position with them.  However, the
            // events that may be queued with times after that but before
            // the above warp can still be used.  So, use the last event
            // tap event time so that -sendEvent: doesn't discard them.
            [WineApplicationController sharedController].lastSetCursorPositionTime = lastEventTapEventTime;
        }

        return ret;
    }

    - (BOOL) startClippingCursor:(CGRect)rect
    {
        CGError err;

        if (!cursorClippingEventTap && ![self installEventTap])
            return FALSE;

        err = CGAssociateMouseAndMouseCursorPosition(false);
        if (err != kCGErrorSuccess)
            return FALSE;

        clippingCursor = TRUE;
        cursorClipRect = rect;

        CGEventTapEnable(cursorClippingEventTap, TRUE);

        return TRUE;
    }

    - (BOOL) stopClippingCursor
    {
        CGError err = CGAssociateMouseAndMouseCursorPosition(true);
        if (err != kCGErrorSuccess)
            return FALSE;

        clippingCursor = FALSE;

        CGEventTapEnable(cursorClippingEventTap, FALSE);
        [warpRecords removeAllObjects];

        return TRUE;
    }

    - (void) setRetinaMode:(int)mode
    {
        double scale = mode ? 0.5 : 2.0;
        cursorClipRect.origin.x *= scale;
        cursorClipRect.origin.y *= scale;
        cursorClipRect.size.width *= scale;
        cursorClipRect.size.height *= scale;
    }

@end
