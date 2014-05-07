/*
 * MACDRV Cocoa application class
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

#import <Carbon/Carbon.h>
#include <dlfcn.h>

#import "cocoa_app.h"
#import "cocoa_event.h"
#import "cocoa_window.h"


static NSString* const WineAppWaitQueryResponseMode = @"WineAppWaitQueryResponseMode";


int macdrv_err_on;


@implementation WineApplication

@synthesize wineController;

    - (void) sendEvent:(NSEvent*)anEvent
    {
        if (![wineController handleEvent:anEvent])
        {
            [super sendEvent:anEvent];
            [wineController didSendEvent:anEvent];
        }
    }

    - (void) setWineController:(WineApplicationController*)newController
    {
        wineController = newController;
        [self setDelegate:wineController];
    }

@end


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


@interface WineApplicationController ()

@property (readwrite, copy, nonatomic) NSEvent* lastFlagsChanged;
@property (copy, nonatomic) NSArray* cursorFrames;
@property (retain, nonatomic) NSTimer* cursorTimer;
@property (retain, nonatomic) NSCursor* cursor;
@property (retain, nonatomic) NSImage* applicationIcon;
@property (readonly, nonatomic) BOOL inputSourceIsInputMethod;
@property (retain, nonatomic) WineWindow* mouseCaptureWindow;

    - (void) setupObservations;
    - (void) applicationDidBecomeActive:(NSNotification *)notification;

    static void PerformRequest(void *info);

@end


@implementation WineApplicationController

    @synthesize keyboardType, lastFlagsChanged;
    @synthesize applicationIcon;
    @synthesize cursorFrames, cursorTimer, cursor;
    @synthesize mouseCaptureWindow;

    @synthesize clippingCursor;

    + (void) initialize
    {
        if (self == [WineApplicationController class])
        {
            NSDictionary* defaults = [NSDictionary dictionaryWithObjectsAndKeys:
                                      @"", @"NSQuotedKeystrokeBinding",
                                      @"", @"NSRepeatCountBinding",
                                      [NSNumber numberWithBool:NO], @"ApplePressAndHoldEnabled",
                                      nil];
            [[NSUserDefaults standardUserDefaults] registerDefaults:defaults];
        }
    }

    + (WineApplicationController*) sharedController
    {
        static WineApplicationController* sharedController;
        static dispatch_once_t once;

        dispatch_once(&once, ^{
            sharedController = [[self alloc] init];
        });

        return sharedController;
    }

    - (id) init
    {
        self = [super init];
        if (self != nil)
        {
            CFRunLoopSourceContext context = { 0 };
            context.perform = PerformRequest;
            requestSource = CFRunLoopSourceCreate(NULL, 0, &context);
            if (!requestSource)
            {
                [self release];
                return nil;
            }
            CFRunLoopAddSource(CFRunLoopGetMain(), requestSource, kCFRunLoopCommonModes);
            CFRunLoopAddSource(CFRunLoopGetMain(), requestSource, (CFStringRef)WineAppWaitQueryResponseMode);

            requests =  [[NSMutableArray alloc] init];
            requestsManipQueue = dispatch_queue_create("org.winehq.WineAppRequestManipQueue", NULL);

            eventQueues = [[NSMutableArray alloc] init];
            eventQueuesLock = [[NSLock alloc] init];

            keyWindows = [[NSMutableArray alloc] init];

            originalDisplayModes = [[NSMutableDictionary alloc] init];
            latentDisplayModes = [[NSMutableDictionary alloc] init];

            warpRecords = [[NSMutableArray alloc] init];

            windowsBeingDragged = [[NSMutableSet alloc] init];

            if (!requests || !requestsManipQueue || !eventQueues || !eventQueuesLock ||
                !keyWindows || !originalDisplayModes || !latentDisplayModes || !warpRecords)
            {
                [self release];
                return nil;
            }

            [self setupObservations];

            keyboardType = LMGetKbdType();

            if ([NSApp isActive])
                [self applicationDidBecomeActive:nil];
        }
        return self;
    }

    - (void) dealloc
    {
        [windowsBeingDragged release];
        [cursor release];
        [screenFrameCGRects release];
        [applicationIcon release];
        [warpRecords release];
        [cursorTimer release];
        [cursorFrames release];
        [latentDisplayModes release];
        [originalDisplayModes release];
        [keyWindows release];
        [eventQueues release];
        [eventQueuesLock release];
        if (requestsManipQueue) dispatch_release(requestsManipQueue);
        [requests release];
        if (requestSource)
        {
            CFRunLoopSourceInvalidate(requestSource);
            CFRelease(requestSource);
        }
        [super dealloc];
    }

    - (void) transformProcessToForeground
    {
        if ([NSApp activationPolicy] != NSApplicationActivationPolicyRegular)
        {
            NSMenu* mainMenu;
            NSMenu* submenu;
            NSString* bundleName;
            NSString* title;
            NSMenuItem* item;

            [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
            [NSApp activateIgnoringOtherApps:YES];

            mainMenu = [[[NSMenu alloc] init] autorelease];

            // Application menu
            submenu = [[[NSMenu alloc] initWithTitle:@"Wine"] autorelease];
            bundleName = [[NSBundle mainBundle] objectForInfoDictionaryKey:(NSString*)kCFBundleNameKey];

            if ([bundleName length])
                title = [NSString stringWithFormat:@"Hide %@", bundleName];
            else
                title = @"Hide";
            item = [submenu addItemWithTitle:title action:@selector(hide:) keyEquivalent:@""];

            item = [submenu addItemWithTitle:@"Hide Others" action:@selector(hideOtherApplications:) keyEquivalent:@"h"];
            [item setKeyEquivalentModifierMask:NSCommandKeyMask | NSAlternateKeyMask];

            item = [submenu addItemWithTitle:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@""];

            [submenu addItem:[NSMenuItem separatorItem]];

            if ([bundleName length])
                title = [NSString stringWithFormat:@"Quit %@", bundleName];
            else
                title = @"Quit";
            item = [submenu addItemWithTitle:title action:@selector(terminate:) keyEquivalent:@"q"];
            [item setKeyEquivalentModifierMask:NSCommandKeyMask | NSAlternateKeyMask];
            item = [[[NSMenuItem alloc] init] autorelease];
            [item setTitle:@"Wine"];
            [item setSubmenu:submenu];
            [mainMenu addItem:item];

            // Window menu
            submenu = [[[NSMenu alloc] initWithTitle:@"Window"] autorelease];
            [submenu addItemWithTitle:@"Minimize" action:@selector(performMiniaturize:) keyEquivalent:@""];
            [submenu addItemWithTitle:@"Zoom" action:@selector(performZoom:) keyEquivalent:@""];
            if ([NSWindow instancesRespondToSelector:@selector(toggleFullScreen:)])
            {
                item = [submenu addItemWithTitle:@"Enter Full Screen" action:@selector(toggleFullScreen:) keyEquivalent:@"f"];
                [item setKeyEquivalentModifierMask:NSCommandKeyMask | NSAlternateKeyMask | NSControlKeyMask];
            }
            [submenu addItem:[NSMenuItem separatorItem]];
            [submenu addItemWithTitle:@"Bring All to Front" action:@selector(arrangeInFront:) keyEquivalent:@""];
            item = [[[NSMenuItem alloc] init] autorelease];
            [item setTitle:@"Window"];
            [item setSubmenu:submenu];
            [mainMenu addItem:item];

            [NSApp setMainMenu:mainMenu];
            [NSApp setWindowsMenu:submenu];

            [NSApp setApplicationIconImage:self.applicationIcon];
        }
    }

    - (BOOL) waitUntilQueryDone:(int*)done timeout:(NSDate*)timeout processEvents:(BOOL)processEvents
    {
        PerformRequest(NULL);

        do
        {
            if (processEvents)
            {
                NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
                NSEvent* event = [NSApp nextEventMatchingMask:NSAnyEventMask
                                                    untilDate:timeout
                                                       inMode:NSDefaultRunLoopMode
                                                      dequeue:YES];
                if (event)
                    [NSApp sendEvent:event];
                [pool release];
            }
            else
                [[NSRunLoop currentRunLoop] runMode:WineAppWaitQueryResponseMode beforeDate:timeout];
        } while (!*done && [timeout timeIntervalSinceNow] >= 0);

        return *done;
    }

    - (BOOL) registerEventQueue:(WineEventQueue*)queue
    {
        [eventQueuesLock lock];
        [eventQueues addObject:queue];
        [eventQueuesLock unlock];
        return TRUE;
    }

    - (void) unregisterEventQueue:(WineEventQueue*)queue
    {
        [eventQueuesLock lock];
        [eventQueues removeObjectIdenticalTo:queue];
        [eventQueuesLock unlock];
    }

    - (void) computeEventTimeAdjustmentFromTicks:(unsigned long long)tickcount uptime:(uint64_t)uptime_ns
    {
        eventTimeAdjustment = (tickcount / 1000.0) - (uptime_ns / (double)NSEC_PER_SEC);
    }

    - (double) ticksForEventTime:(NSTimeInterval)eventTime
    {
        return (eventTime + eventTimeAdjustment) * 1000;
    }

    /* Invalidate old focus offers across all queues. */
    - (void) invalidateGotFocusEvents
    {
        WineEventQueue* queue;

        windowFocusSerial++;

        [eventQueuesLock lock];
        for (queue in eventQueues)
        {
            [queue discardEventsMatchingMask:event_mask_for_type(WINDOW_GOT_FOCUS)
                                   forWindow:nil];
        }
        [eventQueuesLock unlock];
    }

    - (void) windowGotFocus:(WineWindow*)window
    {
        macdrv_event* event;

        [self invalidateGotFocusEvents];

        event = macdrv_create_event(WINDOW_GOT_FOCUS, window);
        event->window_got_focus.serial = windowFocusSerial;
        if (triedWindows)
            event->window_got_focus.tried_windows = [triedWindows retain];
        else
            event->window_got_focus.tried_windows = [[NSMutableSet alloc] init];
        [window.queue postEvent:event];
        macdrv_release_event(event);
    }

    - (void) windowRejectedFocusEvent:(const macdrv_event*)event
    {
        if (event->window_got_focus.serial == windowFocusSerial)
        {
            NSMutableArray* windows = [keyWindows mutableCopy];
            NSNumber* windowNumber;
            WineWindow* window;

            for (windowNumber in [NSWindow windowNumbersWithOptions:NSWindowNumberListAllSpaces])
            {
                window = (WineWindow*)[NSApp windowWithWindowNumber:[windowNumber integerValue]];
                if ([window isKindOfClass:[WineWindow class]] && [window screen] &&
                    ![windows containsObject:window])
                    [windows addObject:window];
            }

            triedWindows = (NSMutableSet*)event->window_got_focus.tried_windows;
            [triedWindows addObject:(WineWindow*)event->window];
            for (window in windows)
            {
                if (![triedWindows containsObject:window] && [window canBecomeKeyWindow])
                {
                    [window makeKeyWindow];
                    break;
                }
            }
            triedWindows = nil;
            [windows release];
        }
    }

    - (void) keyboardSelectionDidChange
    {
        TISInputSourceRef inputSourceLayout;

        inputSourceIsInputMethodValid = FALSE;

        inputSourceLayout = TISCopyCurrentKeyboardLayoutInputSource();
        if (inputSourceLayout)
        {
            CFDataRef uchr;
            uchr = TISGetInputSourceProperty(inputSourceLayout,
                    kTISPropertyUnicodeKeyLayoutData);
            if (uchr)
            {
                macdrv_event* event;
                WineEventQueue* queue;

                event = macdrv_create_event(KEYBOARD_CHANGED, nil);
                event->keyboard_changed.keyboard_type = self.keyboardType;
                event->keyboard_changed.iso_keyboard = (KBGetLayoutType(self.keyboardType) == kKeyboardISO);
                event->keyboard_changed.uchr = CFDataCreateCopy(NULL, uchr);
                event->keyboard_changed.input_source = TISCopyCurrentKeyboardInputSource();

                if (event->keyboard_changed.uchr)
                {
                    [eventQueuesLock lock];

                    for (queue in eventQueues)
                        [queue postEvent:event];

                    [eventQueuesLock unlock];
                }

                macdrv_release_event(event);
            }

            CFRelease(inputSourceLayout);
        }
    }

    - (void) enabledKeyboardInputSourcesChanged
    {
        macdrv_layout_list_needs_update = TRUE;
    }

    - (CGFloat) primaryScreenHeight
    {
        if (!primaryScreenHeightValid)
        {
            NSArray* screens = [NSScreen screens];
            NSUInteger count = [screens count];
            if (count)
            {
                NSUInteger size;
                CGRect* rect;
                NSScreen* screen;

                primaryScreenHeight = NSHeight([[screens objectAtIndex:0] frame]);
                primaryScreenHeightValid = TRUE;

                size = count * sizeof(CGRect);
                if (!screenFrameCGRects)
                    screenFrameCGRects = [[NSMutableData alloc] initWithLength:size];
                else
                    [screenFrameCGRects setLength:size];

                rect = [screenFrameCGRects mutableBytes];
                for (screen in screens)
                {
                    CGRect temp = NSRectToCGRect([screen frame]);
                    temp.origin.y = primaryScreenHeight - CGRectGetMaxY(temp);
                    *rect++ = temp;
                }
            }
            else
                return 1280; /* arbitrary value */
        }

        return primaryScreenHeight;
    }

    - (NSPoint) flippedMouseLocation:(NSPoint)point
    {
        /* This relies on the fact that Cocoa's mouse location points are
           actually off by one (precisely because they were flipped from
           Quartz screen coordinates using this same technique). */
        point.y = [self primaryScreenHeight] - point.y;
        return point;
    }

    - (void) flipRect:(NSRect*)rect
    {
        // We don't use -primaryScreenHeight here so there's no chance of having
        // out-of-date cached info.  This method is called infrequently enough
        // that getting the screen height each time is not prohibitively expensive.
        rect->origin.y = NSMaxY([[[NSScreen screens] objectAtIndex:0] frame]) - NSMaxY(*rect);
    }

    - (WineWindow*) frontWineWindow
    {
        NSNumber* windowNumber;
        for (windowNumber in [NSWindow windowNumbersWithOptions:NSWindowNumberListAllSpaces])
        {
            NSWindow* window = [NSApp windowWithWindowNumber:[windowNumber integerValue]];
            if ([window isKindOfClass:[WineWindow class]] && [window screen])
                return (WineWindow*)window;
        }

        return nil;
    }

    - (void) adjustWindowLevels:(BOOL)active
    {
        NSArray* windowNumbers;
        NSMutableArray* wineWindows;
        NSNumber* windowNumber;
        NSUInteger nextFloatingIndex = 0;
        __block NSInteger maxLevel = NSIntegerMin;
        __block NSInteger maxNonfloatingLevel = NSNormalWindowLevel;
        __block WineWindow* prev = nil;
        WineWindow* window;

        if ([NSApp isHidden]) return;

        windowNumbers = [NSWindow windowNumbersWithOptions:0];
        wineWindows = [[NSMutableArray alloc] initWithCapacity:[windowNumbers count]];

        // For the most part, we rely on the window server's ordering of the windows
        // to be authoritative.  The one exception is if the "floating" property of
        // one of the windows has been changed, it may be in the wrong level and thus
        // in the order.  This method is what's supposed to fix that up.  So build
        // a list of Wine windows sorted first by floating-ness and then by order
        // as indicated by the window server.
        for (windowNumber in windowNumbers)
        {
            window = (WineWindow*)[NSApp windowWithWindowNumber:[windowNumber integerValue]];
            if ([window isKindOfClass:[WineWindow class]])
            {
                if (window.floating)
                    [wineWindows insertObject:window atIndex:nextFloatingIndex++];
                else
                    [wineWindows addObject:window];
            }
        }

        NSDisableScreenUpdates();

        // Go from back to front so that all windows in front of one which is
        // elevated for full-screen are also elevated.
        [wineWindows enumerateObjectsWithOptions:NSEnumerationReverse
                                      usingBlock:^(id obj, NSUInteger idx, BOOL *stop){
            WineWindow* window = (WineWindow*)obj;
            NSInteger origLevel = [window level];
            NSInteger newLevel = [window minimumLevelForActive:active];

            if (newLevel < maxLevel)
                newLevel = maxLevel;
            else
                maxLevel = newLevel;

            if (!window.floating && maxNonfloatingLevel < newLevel)
                maxNonfloatingLevel = newLevel;

            if (newLevel != origLevel)
            {
                [window setLevel:newLevel];

                // -setLevel: puts the window at the front of its new level.  If
                // we decreased the level, that's good (it was in front of that
                // level before, so it should still be now).  But if we increased
                // the level, the window should be toward the back (but still
                // ahead of the previous windows we did this to).
                if (origLevel < newLevel)
                {
                    if (prev)
                        [window orderWindow:NSWindowAbove relativeTo:[prev windowNumber]];
                    else
                        [window orderBack:nil];
                }
            }

            prev = window;
        }];

        NSEnableScreenUpdates();

        [wineWindows release];

        // The above took care of the visible windows on the current space.  That
        // leaves windows on other spaces, minimized windows, and windows which
        // are not ordered in.  We want to leave windows on other spaces alone
        // so the space remains just as they left it (when viewed in Exposé or
        // Mission Control, for example).  We'll adjust the window levels again
        // after we switch to another space, anyway.  Windows which aren't
        // ordered in will be handled when we order them in.  Minimized windows
        // on the current space should be set to the level they would have gotten
        // if they were at the front of the windows with the same floating-ness,
        // because that's where they'll go if/when they are unminimized.  Again,
        // for good measure we'll adjust window levels again when a window is
        // unminimized, too.
        for (window in [NSApp windows])
        {
            if ([window isKindOfClass:[WineWindow class]] && [window isMiniaturized] &&
                [window isOnActiveSpace])
            {
                NSInteger origLevel = [window level];
                NSInteger newLevel = [window minimumLevelForActive:YES];
                NSInteger maxLevelForType = window.floating ? maxLevel : maxNonfloatingLevel;

                if (newLevel < maxLevelForType)
                    newLevel = maxLevelForType;

                if (newLevel != origLevel)
                    [window setLevel:newLevel];
            }
        }
    }

    - (void) adjustWindowLevels
    {
        [self adjustWindowLevels:[NSApp isActive]];
    }

    - (void) updateFullscreenWindows
    {
        if (capture_displays_for_fullscreen && [NSApp isActive])
        {
            BOOL anyFullscreen = FALSE;
            NSNumber* windowNumber;
            for (windowNumber in [NSWindow windowNumbersWithOptions:0])
            {
                WineWindow* window = (WineWindow*)[NSApp windowWithWindowNumber:[windowNumber integerValue]];
                if ([window isKindOfClass:[WineWindow class]] && window.fullscreen)
                {
                    anyFullscreen = TRUE;
                    break;
                }
            }

            if (anyFullscreen)
            {
                if ([self areDisplaysCaptured] || CGCaptureAllDisplays() == CGDisplayNoErr)
                    displaysCapturedForFullscreen = TRUE;
            }
            else if (displaysCapturedForFullscreen)
            {
                if ([originalDisplayModes count] || CGReleaseAllDisplays() == CGDisplayNoErr)
                    displaysCapturedForFullscreen = FALSE;
            }
        }
    }

    - (void) activeSpaceDidChange
    {
        [self updateFullscreenWindows];
        [self adjustWindowLevels];
    }

    - (void) sendDisplaysChanged:(BOOL)activating
    {
        macdrv_event* event;
        WineEventQueue* queue;

        event = macdrv_create_event(DISPLAYS_CHANGED, nil);
        event->displays_changed.activating = activating;

        [eventQueuesLock lock];

        // If we're activating, then we just need one of our threads to get the
        // event, so it can send it directly to the desktop window.  Otherwise,
        // we need all of the threads to get it because we don't know which owns
        // the desktop window and only that one will do anything with it.
        if (activating) event->deliver = 1;

        for (queue in eventQueues)
            [queue postEvent:event];
        [eventQueuesLock unlock];

        macdrv_release_event(event);
    }

    // We can compare two modes directly using CFEqual, but that may require that
    // they are identical to a level that we don't need.  In particular, when the
    // OS switches between the integrated and discrete GPUs, the set of display
    // modes can change in subtle ways.  We're interested in whether two modes
    // match in their most salient features, even if they aren't identical.
    - (BOOL) mode:(CGDisplayModeRef)mode1 matchesMode:(CGDisplayModeRef)mode2
    {
        NSString *encoding1, *encoding2;
        uint32_t ioflags1, ioflags2, different;
        double refresh1, refresh2;

        if (CGDisplayModeGetWidth(mode1) != CGDisplayModeGetWidth(mode2)) return FALSE;
        if (CGDisplayModeGetHeight(mode1) != CGDisplayModeGetHeight(mode2)) return FALSE;

        encoding1 = [(NSString*)CGDisplayModeCopyPixelEncoding(mode1) autorelease];
        encoding2 = [(NSString*)CGDisplayModeCopyPixelEncoding(mode2) autorelease];
        if (![encoding1 isEqualToString:encoding2]) return FALSE;

        ioflags1 = CGDisplayModeGetIOFlags(mode1);
        ioflags2 = CGDisplayModeGetIOFlags(mode2);
        different = ioflags1 ^ ioflags2;
        if (different & (kDisplayModeValidFlag | kDisplayModeSafeFlag | kDisplayModeStretchedFlag |
                         kDisplayModeInterlacedFlag | kDisplayModeTelevisionFlag))
            return FALSE;

        refresh1 = CGDisplayModeGetRefreshRate(mode1);
        if (refresh1 == 0) refresh1 = 60;
        refresh2 = CGDisplayModeGetRefreshRate(mode2);
        if (refresh2 == 0) refresh2 = 60;
        if (fabs(refresh1 - refresh2) > 0.1) return FALSE;

        return TRUE;
    }

    - (CGDisplayModeRef)modeMatchingMode:(CGDisplayModeRef)mode forDisplay:(CGDirectDisplayID)displayID
    {
        CGDisplayModeRef ret = NULL;
        NSArray *modes = [(NSArray*)CGDisplayCopyAllDisplayModes(displayID, NULL) autorelease];
        for (id candidateModeObject in modes)
        {
            CGDisplayModeRef candidateMode = (CGDisplayModeRef)candidateModeObject;
            if ([self mode:candidateMode matchesMode:mode])
            {
                ret = candidateMode;
                break;
            }
        }
        return ret;
    }

    - (BOOL) setMode:(CGDisplayModeRef)mode forDisplay:(CGDirectDisplayID)displayID
    {
        BOOL ret = FALSE;
        NSNumber* displayIDKey = [NSNumber numberWithUnsignedInt:displayID];
        CGDisplayModeRef originalMode;

        originalMode = (CGDisplayModeRef)[originalDisplayModes objectForKey:displayIDKey];

        if (originalMode && [self mode:mode matchesMode:originalMode])
        {
            if ([originalDisplayModes count] == 1) // If this is the last changed display, do a blanket reset
            {
                CGRestorePermanentDisplayConfiguration();
                if (!displaysCapturedForFullscreen)
                    CGReleaseAllDisplays();
                [originalDisplayModes removeAllObjects];
                ret = TRUE;
            }
            else // ... otherwise, try to restore just the one display
            {
                mode = [self modeMatchingMode:mode forDisplay:displayID];
                if (mode && CGDisplaySetDisplayMode(displayID, mode, NULL) == CGDisplayNoErr)
                {
                    [originalDisplayModes removeObjectForKey:displayIDKey];
                    ret = TRUE;
                }
            }
        }
        else
        {
            BOOL active = [NSApp isActive];
            CGDisplayModeRef currentMode;

            currentMode = CGDisplayModeRetain((CGDisplayModeRef)[latentDisplayModes objectForKey:displayIDKey]);
            if (!currentMode)
                currentMode = CGDisplayCopyDisplayMode(displayID);
            if (!currentMode) // Invalid display ID
                return FALSE;

            if ([self mode:mode matchesMode:currentMode]) // Already there!
            {
                CGDisplayModeRelease(currentMode);
                return TRUE;
            }

            CGDisplayModeRelease(currentMode);
            currentMode = NULL;

            mode = [self modeMatchingMode:mode forDisplay:displayID];
            if (!mode)
                return FALSE;

            if ([originalDisplayModes count] || displaysCapturedForFullscreen ||
                !active || CGCaptureAllDisplays() == CGDisplayNoErr)
            {
                if (active)
                {
                    // If we get here, we have the displays captured.  If we don't
                    // know the original mode of the display, the current mode must
                    // be the original.  We should re-query the current mode since
                    // another process could have changed it between when we last
                    // checked and when we captured the displays.
                    if (!originalMode)
                        originalMode = currentMode = CGDisplayCopyDisplayMode(displayID);

                    if (originalMode)
                        ret = (CGDisplaySetDisplayMode(displayID, mode, NULL) == CGDisplayNoErr);
                    if (ret && !(currentMode && [self mode:mode matchesMode:currentMode]))
                        [originalDisplayModes setObject:(id)originalMode forKey:displayIDKey];
                    else if (![originalDisplayModes count])
                    {
                        CGRestorePermanentDisplayConfiguration();
                        if (!displaysCapturedForFullscreen)
                            CGReleaseAllDisplays();
                    }

                    if (currentMode)
                        CGDisplayModeRelease(currentMode);
                }
                else
                {
                    [latentDisplayModes setObject:(id)mode forKey:displayIDKey];
                    ret = TRUE;
                }
            }
        }

        if (ret)
            [self adjustWindowLevels];

        return ret;
    }

    - (BOOL) areDisplaysCaptured
    {
        return ([originalDisplayModes count] > 0 || displaysCapturedForFullscreen);
    }

    - (void) updateCursor:(BOOL)force
    {
        if (force || lastTargetWindow)
        {
            if (clientWantsCursorHidden && !cursorHidden)
            {
                [NSCursor hide];
                cursorHidden = TRUE;
            }

            if (!cursorIsCurrent)
            {
                [cursor set];
                cursorIsCurrent = TRUE;
            }

            if (!clientWantsCursorHidden && cursorHidden)
            {
                [NSCursor unhide];
                cursorHidden = FALSE;
            }
        }
        else
        {
            if (cursorIsCurrent)
            {
                [[NSCursor arrowCursor] set];
                cursorIsCurrent = FALSE;
            }
            if (cursorHidden)
            {
                [NSCursor unhide];
                cursorHidden = FALSE;
            }
        }
    }

    - (void) hideCursor
    {
        if (!clientWantsCursorHidden)
        {
            clientWantsCursorHidden = TRUE;
            [self updateCursor:TRUE];
        }
    }

    - (void) unhideCursor
    {
        if (clientWantsCursorHidden)
        {
            clientWantsCursorHidden = FALSE;
            [self updateCursor:FALSE];
        }
    }

    - (void) setCursor:(NSCursor*)newCursor
    {
        if (newCursor != cursor)
        {
            [cursor release];
            cursor = [newCursor retain];
            cursorIsCurrent = FALSE;
            [self updateCursor:FALSE];
        }
    }

    - (void) setCursor
    {
        NSDictionary* frame = [cursorFrames objectAtIndex:cursorFrame];
        CGImageRef cgimage = (CGImageRef)[frame objectForKey:@"image"];
        NSImage* image = [[NSImage alloc] initWithCGImage:cgimage size:NSZeroSize];
        CFDictionaryRef hotSpotDict = (CFDictionaryRef)[frame objectForKey:@"hotSpot"];
        CGPoint hotSpot;

        if (!CGPointMakeWithDictionaryRepresentation(hotSpotDict, &hotSpot))
            hotSpot = CGPointZero;
        self.cursor = [[[NSCursor alloc] initWithImage:image hotSpot:NSPointFromCGPoint(hotSpot)] autorelease];
        [image release];
        [self unhideCursor];
    }

    - (void) nextCursorFrame:(NSTimer*)theTimer
    {
        NSDictionary* frame;
        NSTimeInterval duration;
        NSDate* date;

        cursorFrame++;
        if (cursorFrame >= [cursorFrames count])
            cursorFrame = 0;
        [self setCursor];

        frame = [cursorFrames objectAtIndex:cursorFrame];
        duration = [[frame objectForKey:@"duration"] doubleValue];
        date = [[theTimer fireDate] dateByAddingTimeInterval:duration];
        [cursorTimer setFireDate:date];
    }

    - (void) setCursorWithFrames:(NSArray*)frames
    {
        if (self.cursorFrames == frames)
            return;

        self.cursorFrames = frames;
        cursorFrame = 0;
        [cursorTimer invalidate];
        self.cursorTimer = nil;

        if ([frames count])
        {
            if ([frames count] > 1)
            {
                NSDictionary* frame = [frames objectAtIndex:0];
                NSTimeInterval duration = [[frame objectForKey:@"duration"] doubleValue];
                NSDate* date = [NSDate dateWithTimeIntervalSinceNow:duration];
                self.cursorTimer = [[[NSTimer alloc] initWithFireDate:date
                                                             interval:1000000
                                                               target:self
                                                             selector:@selector(nextCursorFrame:)
                                                             userInfo:nil
                                                              repeats:YES] autorelease];
                [[NSRunLoop currentRunLoop] addTimer:cursorTimer forMode:NSRunLoopCommonModes];
            }

            [self setCursor];
        }
    }

    - (void) setApplicationIconFromCGImageArray:(NSArray*)images
    {
        NSImage* nsimage = nil;

        if ([images count])
        {
            NSSize bestSize = NSZeroSize;
            id image;

            nsimage = [[[NSImage alloc] initWithSize:NSZeroSize] autorelease];

            for (image in images)
            {
                CGImageRef cgimage = (CGImageRef)image;
                NSBitmapImageRep* imageRep = [[NSBitmapImageRep alloc] initWithCGImage:cgimage];
                if (imageRep)
                {
                    NSSize size = [imageRep size];

                    [nsimage addRepresentation:imageRep];
                    [imageRep release];

                    if (MIN(size.width, size.height) > MIN(bestSize.width, bestSize.height))
                        bestSize = size;
                }
            }

            if ([[nsimage representations] count] && bestSize.width && bestSize.height)
                [nsimage setSize:bestSize];
            else
                nsimage = nil;
        }

        self.applicationIcon = nsimage;
    }

    - (void) handleCommandTab
    {
        if ([NSApp isActive])
        {
            NSRunningApplication* thisApp = [NSRunningApplication currentApplication];
            NSRunningApplication* app;
            NSRunningApplication* otherValidApp = nil;

            if ([originalDisplayModes count] || displaysCapturedForFullscreen)
            {
                NSNumber* displayID;
                for (displayID in originalDisplayModes)
                {
                    CGDisplayModeRef mode = CGDisplayCopyDisplayMode([displayID unsignedIntValue]);
                    [latentDisplayModes setObject:(id)mode forKey:displayID];
                    CGDisplayModeRelease(mode);
                }

                CGRestorePermanentDisplayConfiguration();
                CGReleaseAllDisplays();
                [originalDisplayModes removeAllObjects];
                displaysCapturedForFullscreen = FALSE;
            }

            for (app in [[NSWorkspace sharedWorkspace] runningApplications])
            {
                if (![app isEqual:thisApp] && !app.terminated &&
                    app.activationPolicy == NSApplicationActivationPolicyRegular)
                {
                    if (!app.hidden)
                    {
                        // There's another visible app.  Just hide ourselves and let
                        // the system activate the other app.
                        [NSApp hide:self];
                        return;
                    }

                    if (!otherValidApp)
                        otherValidApp = app;
                }
            }

            // Didn't find a visible GUI app.  Try the Finder or, if that's not
            // running, the first hidden GUI app.  If even that doesn't work, we
            // just fail to switch and remain the active app.
            app = [[NSRunningApplication runningApplicationsWithBundleIdentifier:@"com.apple.finder"] lastObject];
            if (!app) app = otherValidApp;
            [app unhide];
            [app activateWithOptions:0];
        }
    }

    /*
     * ---------- Cursor clipping methods ----------
     *
     * Neither Quartz nor Cocoa has an exact analog for Win32 cursor clipping.
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
            oldLocation = NSPointToCGPoint([self flippedMouseLocation:[NSEvent mouseLocation]]);

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
            *newLocation = NSPointToCGPoint([self flippedMouseLocation:[NSEvent mouseLocation]]);

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
        }

        return FALSE;
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

        cursorLocation = NSPointToCGPoint([self flippedMouseLocation:[NSEvent mouseLocation]]);

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
            lastSetCursorPositionTime = lastEventTapEventTime;

        [self warpCursorTo:&synthesizedLocation from:&cursorLocation];
        if (!CGPointEqualToPoint(eventLocation, synthesizedLocation))
            CGEventSetLocation(event, synthesizedLocation);

        return event;
    }

    CGEventRef WineAppEventTapCallBack(CGEventTapProxy proxy, CGEventType type,
                                       CGEventRef event, void *refcon)
    {
        WineApplicationController* controller = refcon;
        return [controller eventTapWithProxy:proxy type:type event:event];
    }

    - (BOOL) installEventTap
    {
        ProcessSerialNumber psn;
        OSErr err;
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
        void* appServices;
        OSErr (*pGetCurrentProcess)(ProcessSerialNumber* PSN);

        if (cursorClippingEventTap)
            return TRUE;

        // We need to get the Mac GetCurrentProcess() from the ApplicationServices
        // framework with dlsym() because the Win32 function of the same name
        // obscures it.
        appServices = dlopen("/System/Library/Frameworks/ApplicationServices.framework/ApplicationServices", RTLD_LAZY);
        if (!appServices)
            return FALSE;

        pGetCurrentProcess = dlsym(appServices, "GetCurrentProcess");
        if (!pGetCurrentProcess)
        {
            dlclose(appServices);
            return FALSE;
        }

        err = pGetCurrentProcess(&psn);
        dlclose(appServices);
        if (err != noErr)
            return FALSE;

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

        if ([windowsBeingDragged count])
            ret = FALSE;
        else if (clippingCursor)
        {
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
                lastSetCursorPositionTime = lastEventTapEventTime;
            }
        }
        else
        {
            ret = (CGWarpMouseCursorPosition(pos) == kCGErrorSuccess);
            if (ret)
            {
                lastSetCursorPositionTime = [[NSProcessInfo processInfo] systemUptime];

                // Annoyingly, CGWarpMouseCursorPosition() effectively disassociates
                // the mouse from the cursor position for 0.25 seconds.  This means
                // that mouse movement during that interval doesn't move the cursor
                // and events carry a constant location (the warped-to position)
                // even though they have delta values.  This screws us up because
                // the accumulated deltas we send to Wine don't match any eventual
                // absolute position we send (like with a button press).  We can
                // work around this by simply forcibly reassociating the mouse and
                // cursor position.
                CGAssociateMouseAndMouseCursorPosition(true);
            }
        }

        if (ret)
        {
            WineEventQueue* queue;

            // Discard all pending mouse move events.
            [eventQueuesLock lock];
            for (queue in eventQueues)
            {
                [queue discardEventsMatchingMask:event_mask_for_type(MOUSE_MOVED) |
                                                 event_mask_for_type(MOUSE_MOVED_ABSOLUTE)
                                       forWindow:nil];
                [queue resetMouseEventPositions:pos];
            }
            [eventQueuesLock unlock];
        }

        return ret;
    }

    - (void) activateCursorClipping
    {
        if (cursorClippingEventTap && !CGEventTapIsEnabled(cursorClippingEventTap))
        {
            CGEventTapEnable(cursorClippingEventTap, TRUE);
            [self setCursorPosition:NSPointToCGPoint([self flippedMouseLocation:[NSEvent mouseLocation]])];
        }
    }

    - (void) deactivateCursorClipping
    {
        if (cursorClippingEventTap && CGEventTapIsEnabled(cursorClippingEventTap))
        {
            CGEventTapEnable(cursorClippingEventTap, FALSE);
            [warpRecords removeAllObjects];
            lastSetCursorPositionTime = [[NSProcessInfo processInfo] systemUptime];
        }
    }

    - (void) updateCursorClippingState
    {
        if (clippingCursor && [NSApp isActive] && ![windowsBeingDragged count])
            [self activateCursorClipping];
        else
            [self deactivateCursorClipping];
    }

    - (void) updateWindowsForCursorClipping
    {
        WineWindow* window;
        for (window in [NSApp windows])
        {
            if ([window isKindOfClass:[WineWindow class]])
                [window updateForCursorClipping];
        }
    }

    - (BOOL) startClippingCursor:(CGRect)rect
    {
        CGError err;

        if (!cursorClippingEventTap && ![self installEventTap])
            return FALSE;

        if (clippingCursor && CGRectEqualToRect(rect, cursorClipRect) &&
            CGEventTapIsEnabled(cursorClippingEventTap))
            return TRUE;

        err = CGAssociateMouseAndMouseCursorPosition(false);
        if (err != kCGErrorSuccess)
            return FALSE;

        clippingCursor = TRUE;
        cursorClipRect = rect;
        [self updateCursorClippingState];
        [self updateWindowsForCursorClipping];

        return TRUE;
    }

    - (BOOL) stopClippingCursor
    {
        CGError err = CGAssociateMouseAndMouseCursorPosition(true);
        if (err != kCGErrorSuccess)
            return FALSE;

        clippingCursor = FALSE;
        [self updateCursorClippingState];
        [self updateWindowsForCursorClipping];

        return TRUE;
    }

    - (BOOL) isKeyPressed:(uint16_t)keyCode
    {
        int bits = sizeof(pressedKeyCodes[0]) * 8;
        int index = keyCode / bits;
        uint32_t mask = 1 << (keyCode % bits);
        return (pressedKeyCodes[index] & mask) != 0;
    }

    - (void) noteKey:(uint16_t)keyCode pressed:(BOOL)pressed
    {
        int bits = sizeof(pressedKeyCodes[0]) * 8;
        int index = keyCode / bits;
        uint32_t mask = 1 << (keyCode % bits);
        if (pressed)
            pressedKeyCodes[index] |= mask;
        else
            pressedKeyCodes[index] &= ~mask;
    }

    - (void) handleMouseMove:(NSEvent*)anEvent
    {
        WineWindow* targetWindow;
        BOOL drag = [anEvent type] != NSMouseMoved;

        if ([windowsBeingDragged count])
            targetWindow = nil;
        else if (mouseCaptureWindow)
            targetWindow = mouseCaptureWindow;
        else if (drag)
            targetWindow = (WineWindow*)[anEvent window];
        else
        {
            /* Because of the way -[NSWindow setAcceptsMouseMovedEvents:] works, the
               event indicates its window is the main window, even if the cursor is
               over a different window.  Find the actual WineWindow that is under the
               cursor and post the event as being for that window. */
            CGPoint cgpoint = CGEventGetLocation([anEvent CGEvent]);
            NSPoint point = [self flippedMouseLocation:NSPointFromCGPoint(cgpoint)];
            NSInteger windowUnderNumber;

            windowUnderNumber = [NSWindow windowNumberAtPoint:point
                                  belowWindowWithWindowNumber:0];
            targetWindow = (WineWindow*)[NSApp windowWithWindowNumber:windowUnderNumber];
            if (!NSMouseInRect(point, [targetWindow contentRectForFrameRect:[targetWindow frame]], NO))
                targetWindow = nil;
        }

        if ([targetWindow isKindOfClass:[WineWindow class]])
        {
            CGPoint point = CGEventGetLocation([anEvent CGEvent]);
            macdrv_event* event;
            BOOL absolute;

            // If we recently warped the cursor (other than in our cursor-clipping
            // event tap), discard mouse move events until we see an event which is
            // later than that time.
            if (lastSetCursorPositionTime)
            {
                if ([anEvent timestamp] <= lastSetCursorPositionTime)
                    return;

                lastSetCursorPositionTime = 0;
                forceNextMouseMoveAbsolute = TRUE;
            }

            if (forceNextMouseMoveAbsolute || targetWindow != lastTargetWindow)
            {
                absolute = TRUE;
                forceNextMouseMoveAbsolute = FALSE;
            }
            else
            {
                // Send absolute move events if the cursor is in the interior of
                // its range.  Only send relative moves if the cursor is pinned to
                // the boundaries of where it can go.  We compute the position
                // that's one additional point in the direction of movement.  If
                // that is outside of the clipping rect or desktop region (the
                // union of the screen frames), then we figure the cursor would
                // have moved outside if it could but it was pinned.
                CGPoint computedPoint = point;
                CGFloat deltaX = [anEvent deltaX];
                CGFloat deltaY = [anEvent deltaY];

                if (deltaX > 0.001)
                    computedPoint.x++;
                else if (deltaX < -0.001)
                    computedPoint.x--;

                if (deltaY > 0.001)
                    computedPoint.y++;
                else if (deltaY < -0.001)
                    computedPoint.y--;

                // Assume cursor is pinned for now
                absolute = FALSE;
                if (!clippingCursor || CGRectContainsPoint(cursorClipRect, computedPoint))
                {
                    const CGRect* rects;
                    NSUInteger count, i;

                    // Caches screenFrameCGRects if necessary
                    [self primaryScreenHeight];

                    rects = [screenFrameCGRects bytes];
                    count = [screenFrameCGRects length] / sizeof(rects[0]);

                    for (i = 0; i < count; i++)
                    {
                        if (CGRectContainsPoint(rects[i], computedPoint))
                        {
                            absolute = TRUE;
                            break;
                        }
                    }
                }
            }

            if (absolute)
            {
                if (clippingCursor)
                    [self clipCursorLocation:&point];

                event = macdrv_create_event(MOUSE_MOVED_ABSOLUTE, targetWindow);
                event->mouse_moved.x = point.x;
                event->mouse_moved.y = point.y;

                mouseMoveDeltaX = 0;
                mouseMoveDeltaY = 0;
            }
            else
            {
                /* Add event delta to accumulated delta error */
                /* deltaY is already flipped */
                mouseMoveDeltaX += [anEvent deltaX];
                mouseMoveDeltaY += [anEvent deltaY];

                event = macdrv_create_event(MOUSE_MOVED, targetWindow);
                event->mouse_moved.x = mouseMoveDeltaX;
                event->mouse_moved.y = mouseMoveDeltaY;

                /* Keep the remainder after integer truncation. */
                mouseMoveDeltaX -= event->mouse_moved.x;
                mouseMoveDeltaY -= event->mouse_moved.y;
            }

            if (event->type == MOUSE_MOVED_ABSOLUTE || event->mouse_moved.x || event->mouse_moved.y)
            {
                event->mouse_moved.time_ms = [self ticksForEventTime:[anEvent timestamp]];
                event->mouse_moved.drag = drag;

                [targetWindow.queue postEvent:event];
            }

            macdrv_release_event(event);

            lastTargetWindow = targetWindow;
        }
        else
            lastTargetWindow = nil;

        [self updateCursor:FALSE];
    }

    - (void) handleMouseButton:(NSEvent*)theEvent
    {
        WineWindow* window = (WineWindow*)[theEvent window];
        NSEventType type = [theEvent type];
        BOOL broughtWindowForward = FALSE;

        if ([window isKindOfClass:[WineWindow class]] &&
            !window.disabled && !window.noActivate &&
            type == NSLeftMouseDown &&
            (([theEvent modifierFlags] & (NSShiftKeyMask | NSControlKeyMask| NSAlternateKeyMask | NSCommandKeyMask)) != NSCommandKeyMask))
        {
            NSWindowButton windowButton;

            broughtWindowForward = TRUE;

            /* Any left-click on our window anyplace other than the close or
               minimize buttons will bring it forward. */
            for (windowButton = NSWindowCloseButton;
                 windowButton <= NSWindowMiniaturizeButton;
                 windowButton++)
            {
                NSButton* button = [window standardWindowButton:windowButton];
                if (button)
                {
                    NSPoint point = [button convertPoint:[theEvent locationInWindow] fromView:nil];
                    if ([button mouse:point inRect:[button bounds]])
                    {
                        broughtWindowForward = FALSE;
                        break;
                    }
                }
            }
        }

        if ([windowsBeingDragged count])
            window = nil;
        else if (mouseCaptureWindow)
            window = mouseCaptureWindow;

        if ([window isKindOfClass:[WineWindow class]])
        {
            BOOL pressed = (type == NSLeftMouseDown || type == NSRightMouseDown || type == NSOtherMouseDown);
            CGPoint pt = CGEventGetLocation([theEvent CGEvent]);
            BOOL process;

            if (clippingCursor)
                [self clipCursorLocation:&pt];

            if (pressed)
            {
                if (mouseCaptureWindow)
                    process = TRUE;
                else
                {
                    // Test if the click was in the window's content area.
                    NSPoint nspoint = [self flippedMouseLocation:NSPointFromCGPoint(pt)];
                    NSRect contentRect = [window contentRectForFrameRect:[window frame]];
                    process = NSMouseInRect(nspoint, contentRect, NO);
                    if (process && [window styleMask] & NSResizableWindowMask)
                    {
                        // Ignore clicks in the grow box (resize widget).
                        HIPoint origin = { 0, 0 };
                        HIThemeGrowBoxDrawInfo info = { 0 };
                        HIRect bounds;
                        OSStatus status;

                        info.kind = kHIThemeGrowBoxKindNormal;
                        info.direction = kThemeGrowRight | kThemeGrowDown;
                        if ([window styleMask] & NSUtilityWindowMask)
                            info.size = kHIThemeGrowBoxSizeSmall;
                        else
                            info.size = kHIThemeGrowBoxSizeNormal;

                        status = HIThemeGetGrowBoxBounds(&origin, &info, &bounds);
                        if (status == noErr)
                        {
                            NSRect growBox = NSMakeRect(NSMaxX(contentRect) - bounds.size.width,
                                                        NSMinY(contentRect),
                                                        bounds.size.width,
                                                        bounds.size.height);
                            process = !NSMouseInRect(nspoint, growBox, NO);
                        }
                    }
                }
                if (process)
                    unmatchedMouseDowns |= NSEventMaskFromType(type);
            }
            else
            {
                NSEventType downType = type - 1;
                NSUInteger downMask = NSEventMaskFromType(downType);
                process = (unmatchedMouseDowns & downMask) != 0;
                unmatchedMouseDowns &= ~downMask;
            }

            if (process)
            {
                macdrv_event* event;

                event = macdrv_create_event(MOUSE_BUTTON, window);
                event->mouse_button.button = [theEvent buttonNumber];
                event->mouse_button.pressed = pressed;
                event->mouse_button.x = pt.x;
                event->mouse_button.y = pt.y;
                event->mouse_button.time_ms = [self ticksForEventTime:[theEvent timestamp]];

                [window.queue postEvent:event];

                macdrv_release_event(event);
            }
            else if (broughtWindowForward)
            {
                [[window ancestorWineWindow] postBroughtForwardEvent];
                if (![window isKeyWindow])
                    [self windowGotFocus:window];
            }
        }

        // Since mouse button events deliver absolute cursor position, the
        // accumulating delta from move events is invalidated.  Make sure
        // next mouse move event starts over from an absolute baseline.
        // Also, it's at least possible that the title bar widgets (e.g. close
        // button, etc.) could enter an internal event loop on a mouse down that
        // wouldn't exit until a mouse up.  In that case, we'd miss any mouse
        // dragged events and, after that, any notion of the cursor position
        // computed from accumulating deltas would be wrong.
        forceNextMouseMoveAbsolute = TRUE;
    }

    - (void) handleScrollWheel:(NSEvent*)theEvent
    {
        WineWindow* window;

        if (mouseCaptureWindow)
            window = mouseCaptureWindow;
        else
            window = (WineWindow*)[theEvent window];

        if ([window isKindOfClass:[WineWindow class]])
        {
            CGEventRef cgevent = [theEvent CGEvent];
            CGPoint pt = CGEventGetLocation(cgevent);
            BOOL process;

            if (clippingCursor)
                [self clipCursorLocation:&pt];

            if (mouseCaptureWindow)
                process = TRUE;
            else
            {
                // Only process the event if it was in the window's content area.
                NSPoint nspoint = [self flippedMouseLocation:NSPointFromCGPoint(pt)];
                NSRect contentRect = [window contentRectForFrameRect:[window frame]];
                process = NSMouseInRect(nspoint, contentRect, NO);
            }

            if (process)
            {
                macdrv_event* event;
                CGFloat x, y;
                BOOL continuous = FALSE;

                event = macdrv_create_event(MOUSE_SCROLL, window);
                event->mouse_scroll.x = pt.x;
                event->mouse_scroll.y = pt.y;
                event->mouse_scroll.time_ms = [self ticksForEventTime:[theEvent timestamp]];

                if (CGEventGetIntegerValueField(cgevent, kCGScrollWheelEventIsContinuous))
                {
                    continuous = TRUE;

                    /* Continuous scroll wheel events come from high-precision scrolling
                       hardware like Apple's Magic Mouse, Mighty Mouse, and trackpads.
                       For these, we can get more precise data from the CGEvent API. */
                    /* Axis 1 is vertical, axis 2 is horizontal. */
                    x = CGEventGetDoubleValueField(cgevent, kCGScrollWheelEventPointDeltaAxis2);
                    y = CGEventGetDoubleValueField(cgevent, kCGScrollWheelEventPointDeltaAxis1);
                }
                else
                {
                    double pixelsPerLine = 10;
                    CGEventSourceRef source;

                    /* The non-continuous values are in units of "lines", not pixels. */
                    if ((source = CGEventCreateSourceFromEvent(cgevent)))
                    {
                        pixelsPerLine = CGEventSourceGetPixelsPerLine(source);
                        CFRelease(source);
                    }

                    x = pixelsPerLine * [theEvent deltaX];
                    y = pixelsPerLine * [theEvent deltaY];
                }

                /* Mac: negative is right or down, positive is left or up.
                   Win32: negative is left or down, positive is right or up.
                   So, negate the X scroll value to translate. */
                x = -x;

                /* The x,y values so far are in pixels.  Win32 expects to receive some
                   fraction of WHEEL_DELTA == 120.  By my estimation, that's roughly
                   6 times the pixel value. */
                event->mouse_scroll.x_scroll = 6 * x;
                event->mouse_scroll.y_scroll = 6 * y;

                if (!continuous)
                {
                    /* For non-continuous "clicky" wheels, if there was any motion, make
                       sure there was at least WHEEL_DELTA motion.  This is so, at slow
                       speeds where the system's acceleration curve is actually reducing the
                       scroll distance, the user is sure to get some action out of each click.
                       For example, this is important for rotating though weapons in a
                       first-person shooter. */
                    if (0 < event->mouse_scroll.x_scroll && event->mouse_scroll.x_scroll < 120)
                        event->mouse_scroll.x_scroll = 120;
                    else if (-120 < event->mouse_scroll.x_scroll && event->mouse_scroll.x_scroll < 0)
                        event->mouse_scroll.x_scroll = -120;

                    if (0 < event->mouse_scroll.y_scroll && event->mouse_scroll.y_scroll < 120)
                        event->mouse_scroll.y_scroll = 120;
                    else if (-120 < event->mouse_scroll.y_scroll && event->mouse_scroll.y_scroll < 0)
                        event->mouse_scroll.y_scroll = -120;
                }

                if (event->mouse_scroll.x_scroll || event->mouse_scroll.y_scroll)
                    [window.queue postEvent:event];

                macdrv_release_event(event);

                // Since scroll wheel events deliver absolute cursor position, the
                // accumulating delta from move events is invalidated.  Make sure next
                // mouse move event starts over from an absolute baseline.
                forceNextMouseMoveAbsolute = TRUE;
            }
        }
    }

    // Returns TRUE if the event was handled and caller should do nothing more
    // with it.  Returns FALSE if the caller should process it as normal and
    // then call -didSendEvent:.
    - (BOOL) handleEvent:(NSEvent*)anEvent
    {
        BOOL ret = FALSE;
        NSEventType type = [anEvent type];

        if (type == NSFlagsChanged)
            self.lastFlagsChanged = anEvent;
        else if (type == NSMouseMoved || type == NSLeftMouseDragged ||
                 type == NSRightMouseDragged || type == NSOtherMouseDragged)
        {
            [self handleMouseMove:anEvent];
            ret = mouseCaptureWindow && ![windowsBeingDragged count];
        }
        else if (type == NSLeftMouseDown || type == NSLeftMouseUp ||
                 type == NSRightMouseDown || type == NSRightMouseUp ||
                 type == NSOtherMouseDown || type == NSOtherMouseUp)
        {
            [self handleMouseButton:anEvent];
            ret = mouseCaptureWindow && ![windowsBeingDragged count];
        }
        else if (type == NSScrollWheel)
        {
            [self handleScrollWheel:anEvent];
            ret = mouseCaptureWindow != nil;
        }
        else if (type == NSKeyUp)
        {
            uint16_t keyCode = [anEvent keyCode];
            if ([self isKeyPressed:keyCode])
            {
                WineWindow* window = (WineWindow*)[anEvent window];
                [self noteKey:keyCode pressed:FALSE];
                if ([window isKindOfClass:[WineWindow class]])
                    [window postKeyEvent:anEvent];
            }
        }
        else if (type == NSAppKitDefined)
        {
            short subtype = [anEvent subtype];

            // These subtypes are not documented but they appear to mean
            // "a window is being dragged" and "a window is no longer being
            // dragged", respectively.
            if (subtype == 20 || subtype == 21)
            {
                WineWindow* window = (WineWindow*)[anEvent window];
                if ([window isKindOfClass:[WineWindow class]])
                {
                    macdrv_event* event;
                    int eventType;

                    if (subtype == 20)
                    {
                        [windowsBeingDragged addObject:window];
                        eventType = WINDOW_DRAG_BEGIN;
                    }
                    else
                    {
                        [windowsBeingDragged removeObject:window];
                        eventType = WINDOW_DRAG_END;
                    }
                    [self updateCursorClippingState];

                    event = macdrv_create_event(eventType, window);
                    [window.queue postEvent:event];
                    macdrv_release_event(event);
                }
            }
        }

        return ret;
    }

    - (void) didSendEvent:(NSEvent*)anEvent
    {
        NSEventType type = [anEvent type];

        if (type == NSKeyDown && ![anEvent isARepeat] && [anEvent keyCode] == kVK_Tab)
        {
            NSUInteger modifiers = [anEvent modifierFlags];
            if ((modifiers & NSCommandKeyMask) &&
                !(modifiers & (NSControlKeyMask | NSAlternateKeyMask)))
            {
                // Command-Tab and Command-Shift-Tab would normally be intercepted
                // by the system to switch applications.  If we're seeing it, it's
                // presumably because we've captured the displays, preventing
                // normal application switching.  Do it manually.
                [self handleCommandTab];
            }
        }
    }

    - (void) setupObservations
    {
        NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];
        NSNotificationCenter* wsnc = [[NSWorkspace sharedWorkspace] notificationCenter];
        NSDistributedNotificationCenter* dnc = [NSDistributedNotificationCenter defaultCenter];

        [nc addObserverForName:NSWindowDidBecomeKeyNotification
                        object:nil
                         queue:nil
                    usingBlock:^(NSNotification *note){
            NSWindow* window = [note object];
            [keyWindows removeObjectIdenticalTo:window];
            [keyWindows insertObject:window atIndex:0];
        }];

        [nc addObserverForName:NSWindowWillCloseNotification
                        object:nil
                         queue:[NSOperationQueue mainQueue]
                    usingBlock:^(NSNotification *note){
            NSWindow* window = [note object];
            if ([window isKindOfClass:[WineWindow class]] && [(WineWindow*)window isFakingClose])
                return;
            [keyWindows removeObjectIdenticalTo:window];
            if (window == lastTargetWindow)
                lastTargetWindow = nil;
            if (window == self.mouseCaptureWindow)
                self.mouseCaptureWindow = nil;
            if ([window isKindOfClass:[WineWindow class]] && [(WineWindow*)window isFullscreen])
            {
                dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 0), dispatch_get_main_queue(), ^{
                    [self updateFullscreenWindows];
                });
            }
            [windowsBeingDragged removeObject:window];
            [self updateCursorClippingState];
        }];

        [nc addObserver:self
               selector:@selector(keyboardSelectionDidChange)
                   name:NSTextInputContextKeyboardSelectionDidChangeNotification
                 object:nil];

        /* The above notification isn't sent unless the NSTextInputContext
           class has initialized itself.  Poke it. */
        [NSTextInputContext self];

        [wsnc addObserver:self
                 selector:@selector(activeSpaceDidChange)
                     name:NSWorkspaceActiveSpaceDidChangeNotification
                   object:nil];

        [nc addObserver:self
               selector:@selector(releaseMouseCapture)
                   name:NSMenuDidBeginTrackingNotification
                 object:nil];

        [dnc        addObserver:self
                       selector:@selector(releaseMouseCapture)
                           name:@"com.apple.HIToolbox.beginMenuTrackingNotification"
                         object:nil
             suspensionBehavior:NSNotificationSuspensionBehaviorDrop];

        [dnc addObserver:self
                selector:@selector(enabledKeyboardInputSourcesChanged)
                    name:(NSString*)kTISNotifyEnabledKeyboardInputSourcesChanged
                  object:nil];
    }

    - (BOOL) inputSourceIsInputMethod
    {
        if (!inputSourceIsInputMethodValid)
        {
            TISInputSourceRef inputSource = TISCopyCurrentKeyboardInputSource();
            if (inputSource)
            {
                CFStringRef type = TISGetInputSourceProperty(inputSource, kTISPropertyInputSourceType);
                inputSourceIsInputMethod = !CFEqual(type, kTISTypeKeyboardLayout);
                CFRelease(inputSource);
            }
            else
                inputSourceIsInputMethod = FALSE;
            inputSourceIsInputMethodValid = TRUE;
        }

        return inputSourceIsInputMethod;
    }

    - (void) releaseMouseCapture
    {
        // This might be invoked on a background thread by the distributed
        // notification center.  Shunt it to the main thread.
        if (![NSThread isMainThread])
        {
            dispatch_async(dispatch_get_main_queue(), ^{ [self releaseMouseCapture]; });
            return;
        }

        if (mouseCaptureWindow)
        {
            macdrv_event* event;

            event = macdrv_create_event(RELEASE_CAPTURE, mouseCaptureWindow);
            [mouseCaptureWindow.queue postEvent:event];
            macdrv_release_event(event);
        }
    }

    - (void) unminimizeWindowIfNoneVisible
    {
        if (![self frontWineWindow])
        {
            for (WineWindow* window in [NSApp windows])
            {
                if ([window isKindOfClass:[WineWindow class]] && [window isMiniaturized])
                {
                    [window deminiaturize:self];
                    break;
                }
            }
        }
    }


    /*
     * ---------- NSApplicationDelegate methods ----------
     */
    - (void)applicationDidBecomeActive:(NSNotification *)notification
    {
        NSNumber* displayID;
        NSDictionary* modesToRealize = [latentDisplayModes autorelease];

        latentDisplayModes = [[NSMutableDictionary alloc] init];
        for (displayID in modesToRealize)
        {
            CGDisplayModeRef mode = (CGDisplayModeRef)[modesToRealize objectForKey:displayID];
            [self setMode:mode forDisplay:[displayID unsignedIntValue]];
        }

        [self updateCursorClippingState];

        [self updateFullscreenWindows];
        [self adjustWindowLevels:YES];

        if (beenActive)
            [self unminimizeWindowIfNoneVisible];
        beenActive = TRUE;

        // If a Wine process terminates abruptly while it has the display captured
        // and switched to a different resolution, Mac OS X will uncapture the
        // displays and switch their resolutions back.  However, the other Wine
        // processes won't have their notion of the desktop rect changed back.
        // This can lead them to refuse to draw or acknowledge clicks in certain
        // portions of their windows.
        //
        // To solve this, we synthesize a displays-changed event whenever we're
        // activated.  This will provoke a re-synchronization of Wine's notion of
        // the desktop rect with the actual state.
        [self sendDisplaysChanged:TRUE];

        // The cursor probably moved while we were inactive.  Accumulated mouse
        // movement deltas are invalidated.  Make sure the next mouse move event
        // starts over from an absolute baseline.
        forceNextMouseMoveAbsolute = TRUE;
    }

    - (void)applicationDidChangeScreenParameters:(NSNotification *)notification
    {
        primaryScreenHeightValid = FALSE;
        [self sendDisplaysChanged:FALSE];
        [self adjustWindowLevels];

        // When the display configuration changes, the cursor position may jump.
        // Accumulated mouse movement deltas are invalidated.  Make sure the next
        // mouse move event starts over from an absolute baseline.
        forceNextMouseMoveAbsolute = TRUE;
    }

    - (void)applicationDidResignActive:(NSNotification *)notification
    {
        macdrv_event* event;
        WineEventQueue* queue;

        [self updateCursorClippingState];

        [self invalidateGotFocusEvents];

        event = macdrv_create_event(APP_DEACTIVATED, nil);

        [eventQueuesLock lock];
        for (queue in eventQueues)
            [queue postEvent:event];
        [eventQueuesLock unlock];

        macdrv_release_event(event);

        [self releaseMouseCapture];
    }

    - (void) applicationDidUnhide:(NSNotification*)aNotification
    {
        [self adjustWindowLevels];
    }

    - (BOOL) applicationShouldHandleReopen:(NSApplication*)theApplication hasVisibleWindows:(BOOL)flag
    {
        // Note that "flag" is often wrong.  WineWindows are NSPanels and NSPanels
        // don't count as "visible windows" for this purpose.
        [self unminimizeWindowIfNoneVisible];
        return YES;
    }

    - (NSApplicationTerminateReply) applicationShouldTerminate:(NSApplication *)sender
    {
        NSApplicationTerminateReply ret = NSTerminateNow;
        NSAppleEventManager* m = [NSAppleEventManager sharedAppleEventManager];
        NSAppleEventDescriptor* desc = [m currentAppleEvent];
        macdrv_event* event;
        WineEventQueue* queue;

        event = macdrv_create_event(APP_QUIT_REQUESTED, nil);
        event->deliver = 1;
        switch ([[desc attributeDescriptorForKeyword:kAEQuitReason] int32Value])
        {
            case kAELogOut:
            case kAEReallyLogOut:
                event->app_quit_requested.reason = QUIT_REASON_LOGOUT;
                break;
            case kAEShowRestartDialog:
                event->app_quit_requested.reason = QUIT_REASON_RESTART;
                break;
            case kAEShowShutdownDialog:
                event->app_quit_requested.reason = QUIT_REASON_SHUTDOWN;
                break;
            default:
                event->app_quit_requested.reason = QUIT_REASON_NONE;
                break;
        }

        [eventQueuesLock lock];

        if ([eventQueues count])
        {
            for (queue in eventQueues)
                [queue postEvent:event];
            ret = NSTerminateLater;
        }

        [eventQueuesLock unlock];

        macdrv_release_event(event);

        return ret;
    }

    - (void)applicationWillResignActive:(NSNotification *)notification
    {
        [self adjustWindowLevels:NO];
    }

/***********************************************************************
 *              PerformRequest
 *
 * Run-loop-source perform callback.  Pull request blocks from the
 * array of queued requests and invoke them.
 */
static void PerformRequest(void *info)
{
    WineApplicationController* controller = [WineApplicationController sharedController];

    for (;;)
    {
        __block dispatch_block_t block;

        dispatch_sync(controller->requestsManipQueue, ^{
            if ([controller->requests count])
            {
                block = (dispatch_block_t)[[controller->requests objectAtIndex:0] retain];
                [controller->requests removeObjectAtIndex:0];
            }
            else
                block = nil;
        });

        if (!block)
            break;

        block();
        [block release];
    }
}

/***********************************************************************
 *              OnMainThreadAsync
 *
 * Run a block on the main thread asynchronously.
 */
void OnMainThreadAsync(dispatch_block_t block)
{
    WineApplicationController* controller = [WineApplicationController sharedController];

    block = [block copy];
    dispatch_sync(controller->requestsManipQueue, ^{
        [controller->requests addObject:block];
    });
    [block release];
    CFRunLoopSourceSignal(controller->requestSource);
    CFRunLoopWakeUp(CFRunLoopGetMain());
}

@end

/***********************************************************************
 *              LogError
 */
void LogError(const char* func, NSString* format, ...)
{
    va_list args;
    va_start(args, format);
    LogErrorv(func, format, args);
    va_end(args);
}

/***********************************************************************
 *              LogErrorv
 */
void LogErrorv(const char* func, NSString* format, va_list args)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

    NSString* message = [[NSString alloc] initWithFormat:format arguments:args];
    fprintf(stderr, "err:%s:%s", func, [message UTF8String]);
    [message release];

    [pool release];
}

/***********************************************************************
 *              macdrv_window_rejected_focus
 *
 * Pass focus to the next window that hasn't already rejected this same
 * WINDOW_GOT_FOCUS event.
 */
void macdrv_window_rejected_focus(const macdrv_event *event)
{
    OnMainThread(^{
        [[WineApplicationController sharedController] windowRejectedFocusEvent:event];
    });
}

/***********************************************************************
 *              macdrv_get_input_source_info
 *
 * Returns the keyboard layout uchr data, keyboard type and input source.
 */
void macdrv_get_input_source_info(CFDataRef* uchr, CGEventSourceKeyboardType* keyboard_type, int* is_iso, TISInputSourceRef* input_source)
{
    OnMainThread(^{
        TISInputSourceRef inputSourceLayout;

        inputSourceLayout = TISCopyCurrentKeyboardLayoutInputSource();
        if (inputSourceLayout)
        {
            CFDataRef data = TISGetInputSourceProperty(inputSourceLayout,
                                kTISPropertyUnicodeKeyLayoutData);
            *uchr = CFDataCreateCopy(NULL, data);
            CFRelease(inputSourceLayout);

            *keyboard_type = [WineApplicationController sharedController].keyboardType;
            *is_iso = (KBGetLayoutType(*keyboard_type) == kKeyboardISO);
            *input_source = TISCopyCurrentKeyboardInputSource();
        }
    });
}

/***********************************************************************
 *              macdrv_beep
 *
 * Play the beep sound configured by the user in System Preferences.
 */
void macdrv_beep(void)
{
    OnMainThreadAsync(^{
        NSBeep();
    });
}

/***********************************************************************
 *              macdrv_set_display_mode
 */
int macdrv_set_display_mode(const struct macdrv_display* display,
                            CGDisplayModeRef display_mode)
{
    __block int ret;

    OnMainThread(^{
        ret = [[WineApplicationController sharedController] setMode:display_mode forDisplay:display->displayID];
    });

    return ret;
}

/***********************************************************************
 *              macdrv_set_cursor
 *
 * Set the cursor.
 *
 * If name is non-NULL, it is a selector for a class method on NSCursor
 * identifying the cursor to set.  In that case, frames is ignored.  If
 * name is NULL, then frames is used.
 *
 * frames is an array of dictionaries.  Each dictionary is a frame of
 * an animated cursor.  Under the key "image" is a CGImage for the
 * frame.  Under the key "duration" is a CFNumber time interval, in
 * seconds, for how long that frame is presented before proceeding to
 * the next frame.  Under the key "hotSpot" is a CFDictionary encoding a
 * CGPoint, to be decoded using CGPointMakeWithDictionaryRepresentation().
 * This is the hot spot, measured in pixels down and to the right of the
 * top-left corner of the image.
 *
 * If the array has exactly 1 element, the cursor is static, not
 * animated.  If frames is NULL or has 0 elements, the cursor is hidden.
 */
void macdrv_set_cursor(CFStringRef name, CFArrayRef frames)
{
    SEL sel;

    sel = NSSelectorFromString((NSString*)name);
    if (sel)
    {
        OnMainThreadAsync(^{
            WineApplicationController* controller = [WineApplicationController sharedController];
            [controller setCursorWithFrames:nil];
            controller.cursor = [NSCursor performSelector:sel];
            [controller unhideCursor];
        });
    }
    else
    {
        NSArray* nsframes = (NSArray*)frames;
        if ([nsframes count])
        {
            OnMainThreadAsync(^{
                [[WineApplicationController sharedController] setCursorWithFrames:nsframes];
            });
        }
        else
        {
            OnMainThreadAsync(^{
                WineApplicationController* controller = [WineApplicationController sharedController];
                [controller setCursorWithFrames:nil];
                [controller hideCursor];
            });
        }
    }
}

/***********************************************************************
 *              macdrv_get_cursor_position
 *
 * Obtains the current cursor position.  Returns zero on failure,
 * non-zero on success.
 */
int macdrv_get_cursor_position(CGPoint *pos)
{
    OnMainThread(^{
        NSPoint location = [NSEvent mouseLocation];
        location = [[WineApplicationController sharedController] flippedMouseLocation:location];
        *pos = NSPointToCGPoint(location);
    });

    return TRUE;
}

/***********************************************************************
 *              macdrv_set_cursor_position
 *
 * Sets the cursor position without generating events.  Returns zero on
 * failure, non-zero on success.
 */
int macdrv_set_cursor_position(CGPoint pos)
{
    __block int ret;

    OnMainThread(^{
        ret = [[WineApplicationController sharedController] setCursorPosition:pos];
    });

    return ret;
}

/***********************************************************************
 *              macdrv_clip_cursor
 *
 * Sets the cursor cursor clipping rectangle.  If the rectangle is equal
 * to or larger than the whole desktop region, the cursor is unclipped.
 * Returns zero on failure, non-zero on success.
 */
int macdrv_clip_cursor(CGRect rect)
{
    __block int ret;

    OnMainThread(^{
        WineApplicationController* controller = [WineApplicationController sharedController];
        BOOL clipping = FALSE;

        if (!CGRectIsInfinite(rect))
        {
            NSRect nsrect = NSRectFromCGRect(rect);
            NSScreen* screen;

            /* Convert the rectangle from top-down coords to bottom-up. */
            [controller flipRect:&nsrect];

            clipping = FALSE;
            for (screen in [NSScreen screens])
            {
                if (!NSContainsRect(nsrect, [screen frame]))
                {
                    clipping = TRUE;
                    break;
                }
            }
        }

        if (clipping)
            ret = [controller startClippingCursor:rect];
        else
            ret = [controller stopClippingCursor];
    });

    return ret;
}

/***********************************************************************
 *              macdrv_set_application_icon
 *
 * Set the application icon.  The images array contains CGImages.  If
 * there are more than one, then they represent different sizes or
 * color depths from the icon resource.  If images is NULL or empty,
 * restores the default application image.
 */
void macdrv_set_application_icon(CFArrayRef images)
{
    NSArray* imageArray = (NSArray*)images;

    OnMainThreadAsync(^{
        [[WineApplicationController sharedController] setApplicationIconFromCGImageArray:imageArray];
    });
}

/***********************************************************************
 *              macdrv_quit_reply
 */
void macdrv_quit_reply(int reply)
{
    OnMainThread(^{
        [NSApp replyToApplicationShouldTerminate:reply];
    });
}

/***********************************************************************
 *              macdrv_using_input_method
 */
int macdrv_using_input_method(void)
{
    __block BOOL ret;

    OnMainThread(^{
        ret = [[WineApplicationController sharedController] inputSourceIsInputMethod];
    });

    return ret;
}

/***********************************************************************
 *              macdrv_set_mouse_capture_window
 */
void macdrv_set_mouse_capture_window(macdrv_window window)
{
    WineWindow* w = (WineWindow*)window;

    [w.queue discardEventsMatchingMask:event_mask_for_type(RELEASE_CAPTURE) forWindow:w];

    OnMainThread(^{
        [[WineApplicationController sharedController] setMouseCaptureWindow:w];
    });
}

const CFStringRef macdrv_input_source_input_key = CFSTR("input");
const CFStringRef macdrv_input_source_type_key = CFSTR("type");
const CFStringRef macdrv_input_source_lang_key = CFSTR("lang");

/***********************************************************************
 *              macdrv_create_input_source_list
 */
CFArrayRef macdrv_create_input_source_list(void)
{
    CFMutableArrayRef ret = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);

    OnMainThread(^{
        CFArrayRef input_list;
        CFDictionaryRef filter_dict;
        const void *filter_keys[2] = { kTISPropertyInputSourceCategory, kTISPropertyInputSourceIsSelectCapable };
        const void *filter_values[2] = { kTISCategoryKeyboardInputSource, kCFBooleanTrue };
        int i;

        filter_dict = CFDictionaryCreate(NULL, filter_keys, filter_values, sizeof(filter_keys)/sizeof(filter_keys[0]),
                                         &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        input_list = TISCreateInputSourceList(filter_dict, false);

        for (i = 0; i < CFArrayGetCount(input_list); i++)
        {
            TISInputSourceRef input = (TISInputSourceRef)CFArrayGetValueAtIndex(input_list, i);
            CFArrayRef source_langs = TISGetInputSourceProperty(input, kTISPropertyInputSourceLanguages);
            CFDictionaryRef entry;
            const void *input_keys[3] = { macdrv_input_source_input_key,
                                          macdrv_input_source_type_key,
                                          macdrv_input_source_lang_key };
            const void *input_values[3];

            input_values[0] = input;
            input_values[1] = TISGetInputSourceProperty(input, kTISPropertyInputSourceType);
            input_values[2] = CFArrayGetValueAtIndex(source_langs, 0);

            entry = CFDictionaryCreate(NULL, input_keys, input_values, sizeof(input_keys) / sizeof(input_keys[0]),
                                       &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

            CFArrayAppendValue(ret, entry);
            CFRelease(entry);
        }
        CFRelease(input_list);
        CFRelease(filter_dict);
    });

    return ret;
}

int macdrv_select_input_source(TISInputSourceRef input_source)
{
    __block int ret = FALSE;

    OnMainThread(^{
        ret = (TISSelectInputSource(input_source) == noErr);
    });

    return ret;
}
