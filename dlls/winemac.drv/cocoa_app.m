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

#import "cocoa_app.h"
#import "cocoa_event.h"
#import "cocoa_window.h"


int macdrv_err_on;


@interface WineApplication ()

@property (readwrite, copy, nonatomic) NSEvent* lastFlagsChanged;

@end


@implementation WineApplication

    @synthesize keyboardType, lastFlagsChanged;
    @synthesize orderedWineWindows;

    - (id) init
    {
        self = [super init];
        if (self != nil)
        {
            eventQueues = [[NSMutableArray alloc] init];
            eventQueuesLock = [[NSLock alloc] init];

            keyWindows = [[NSMutableArray alloc] init];
            orderedWineWindows = [[NSMutableArray alloc] init];

            originalDisplayModes = [[NSMutableDictionary alloc] init];

            if (!eventQueues || !eventQueuesLock || !keyWindows || !orderedWineWindows ||
                !originalDisplayModes)
            {
                [self release];
                return nil;
            }
        }
        return self;
    }

    - (void) dealloc
    {
        [originalDisplayModes release];
        [orderedWineWindows release];
        [keyWindows release];
        [eventQueues release];
        [eventQueuesLock release];
        [super dealloc];
    }

    - (void) transformProcessToForeground
    {
        if ([self activationPolicy] != NSApplicationActivationPolicyRegular)
        {
            NSMenu* mainMenu;
            NSMenu* submenu;
            NSString* bundleName;
            NSString* title;
            NSMenuItem* item;

            [self setActivationPolicy:NSApplicationActivationPolicyRegular];
            [self activateIgnoringOtherApps:YES];

            mainMenu = [[[NSMenu alloc] init] autorelease];

            submenu = [[[NSMenu alloc] initWithTitle:@"Wine"] autorelease];
            bundleName = [[NSBundle mainBundle] objectForInfoDictionaryKey:(NSString*)kCFBundleNameKey];
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

            submenu = [[[NSMenu alloc] initWithTitle:@"Window"] autorelease];
            [submenu addItemWithTitle:@"Minimize" action:@selector(performMiniaturize:) keyEquivalent:@""];
            [submenu addItemWithTitle:@"Zoom" action:@selector(performZoom:) keyEquivalent:@""];
            [submenu addItem:[NSMenuItem separatorItem]];
            [submenu addItemWithTitle:@"Bring All to Front" action:@selector(arrangeInFront:) keyEquivalent:@""];
            item = [[[NSMenuItem alloc] init] autorelease];
            [item setTitle:@"Window"];
            [item setSubmenu:submenu];
            [mainMenu addItem:item];

            [self setMainMenu:mainMenu];
            [self setWindowsMenu:submenu];
        }
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
        macdrv_event event;

        [NSApp invalidateGotFocusEvents];

        event.type = WINDOW_GOT_FOCUS;
        event.window = (macdrv_window)[window retain];
        event.window_got_focus.serial = windowFocusSerial;
        if (triedWindows)
            event.window_got_focus.tried_windows = [triedWindows retain];
        else
            event.window_got_focus.tried_windows = [[NSMutableSet alloc] init];
        [window.queue postEvent:&event];
    }

    - (void) windowRejectedFocusEvent:(const macdrv_event*)event
    {
        if (event->window_got_focus.serial == windowFocusSerial)
        {
            triedWindows = (NSMutableSet*)event->window_got_focus.tried_windows;
            [triedWindows addObject:(WineWindow*)event->window];
            for (NSWindow* window in [keyWindows arrayByAddingObjectsFromArray:[self orderedWindows]])
            {
                if (![triedWindows containsObject:window] && [window canBecomeKeyWindow])
                {
                    [window makeKeyWindow];
                    break;
                }
            }
            triedWindows = nil;
        }
    }

    - (void) keyboardSelectionDidChange
    {
        TISInputSourceRef inputSource;

        inputSource = TISCopyCurrentKeyboardLayoutInputSource();
        if (inputSource)
        {
            CFDataRef uchr;
            uchr = TISGetInputSourceProperty(inputSource,
                    kTISPropertyUnicodeKeyLayoutData);
            if (uchr)
            {
                macdrv_event event;
                WineEventQueue* queue;

                event.type = KEYBOARD_CHANGED;
                event.window = NULL;
                event.keyboard_changed.keyboard_type = self.keyboardType;
                event.keyboard_changed.iso_keyboard = (KBGetLayoutType(self.keyboardType) == kKeyboardISO);
                event.keyboard_changed.uchr = CFDataCreateCopy(NULL, uchr);

                if (event.keyboard_changed.uchr)
                {
                    [eventQueuesLock lock];

                    for (queue in eventQueues)
                    {
                        CFRetain(event.keyboard_changed.uchr);
                        [queue postEvent:&event];
                    }

                    [eventQueuesLock unlock];

                    CFRelease(event.keyboard_changed.uchr);
                }
            }

            CFRelease(inputSource);
        }
    }

    - (CGFloat) primaryScreenHeight
    {
        if (!primaryScreenHeightValid)
        {
            NSArray* screens = [NSScreen screens];
            if ([screens count])
            {
                primaryScreenHeight = NSHeight([[screens objectAtIndex:0] frame]);
                primaryScreenHeightValid = TRUE;
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

    - (void) wineWindow:(WineWindow*)window
                ordered:(NSWindowOrderingMode)order
             relativeTo:(WineWindow*)otherWindow
    {
        NSUInteger index;

        switch (order)
        {
            case NSWindowAbove:
                [window retain];
                [orderedWineWindows removeObjectIdenticalTo:window];
                if (otherWindow)
                {
                    index = [orderedWineWindows indexOfObjectIdenticalTo:otherWindow];
                    if (index == NSNotFound)
                        index = 0;
                }
                else
                {
                    index = 0;
                    for (otherWindow in orderedWineWindows)
                    {
                        if ([otherWindow levelWhenActive] <= [window levelWhenActive])
                            break;
                        index++;
                    }
                }
                [orderedWineWindows insertObject:window atIndex:index];
                [window release];
                break;
            case NSWindowBelow:
                [window retain];
                [orderedWineWindows removeObjectIdenticalTo:window];
                if (otherWindow)
                {
                    index = [orderedWineWindows indexOfObjectIdenticalTo:otherWindow];
                    if (index == NSNotFound)
                        index = [orderedWineWindows count];
                }
                else
                {
                    index = 0;
                    for (otherWindow in orderedWineWindows)
                    {
                        if ([otherWindow levelWhenActive] < [window levelWhenActive])
                            break;
                        index++;
                    }
                }
                [orderedWineWindows insertObject:window atIndex:index];
                [window release];
                break;
            case NSWindowOut:
            default:
                break;
        }
    }

    - (void) sendDisplaysChanged:(BOOL)activating
    {
        macdrv_event event;
        WineEventQueue* queue;

        event.type = DISPLAYS_CHANGED;
        event.window = NULL;
        event.displays_changed.activating = activating;

        [eventQueuesLock lock];
        for (queue in eventQueues)
            [queue postEvent:&event];
        [eventQueuesLock unlock];
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
        CGDisplayModeRef currentMode, originalMode;

        currentMode = CGDisplayCopyDisplayMode(displayID);
        if (!currentMode) // Invalid display ID
            return FALSE;

        if ([self mode:mode matchesMode:currentMode]) // Already there!
        {
            CGDisplayModeRelease(currentMode);
            return TRUE;
        }

        mode = [self modeMatchingMode:mode forDisplay:displayID];
        if (!mode)
        {
            CGDisplayModeRelease(currentMode);
            return FALSE;
        }

        originalMode = (CGDisplayModeRef)[originalDisplayModes objectForKey:displayIDKey];
        if (!originalMode)
            originalMode = currentMode;

        if ([self mode:mode matchesMode:originalMode])
        {
            if ([originalDisplayModes count] == 1) // If this is the last changed display, do a blanket reset
            {
                CGRestorePermanentDisplayConfiguration();
                CGReleaseAllDisplays();
                [originalDisplayModes removeAllObjects];
                ret = TRUE;
            }
            else // ... otherwise, try to restore just the one display
            {
                if (CGDisplaySetDisplayMode(displayID, mode, NULL) == CGDisplayNoErr)
                {
                    [originalDisplayModes removeObjectForKey:displayIDKey];
                    ret = TRUE;
                }
            }
        }
        else
        {
            if ([originalDisplayModes count] || CGCaptureAllDisplays() == CGDisplayNoErr)
            {
                if (CGDisplaySetDisplayMode(displayID, mode, NULL) == CGDisplayNoErr)
                {
                    [originalDisplayModes setObject:(id)originalMode forKey:displayIDKey];
                    ret = TRUE;
                }
                else if (![originalDisplayModes count])
                {
                    CGRestorePermanentDisplayConfiguration();
                    CGReleaseAllDisplays();
                }
            }
        }

        CGDisplayModeRelease(currentMode);

        if (ret)
        {
            [orderedWineWindows enumerateObjectsWithOptions:NSEnumerationReverse usingBlock:^(id obj, NSUInteger idx, BOOL *stop){
                [(WineWindow*)obj adjustWindowLevel];
            }];
        }

        return ret;
    }

    - (BOOL) areDisplaysCaptured
    {
        return ([originalDisplayModes count] > 0);
    }


    /*
     * ---------- NSApplication method overrides ----------
     */
    - (void) sendEvent:(NSEvent*)anEvent
    {
        if ([anEvent type] == NSFlagsChanged)
            self.lastFlagsChanged = anEvent;

        [super sendEvent:anEvent];
    }


    /*
     * ---------- NSApplicationDelegate methods ----------
     */
    - (void)applicationDidBecomeActive:(NSNotification *)notification
    {
        [orderedWineWindows enumerateObjectsWithOptions:NSEnumerationReverse usingBlock:^(id obj, NSUInteger idx, BOOL *stop){
            WineWindow* window = obj;
            if ([window levelWhenActive] != [window level])
                [window setLevel:[window levelWhenActive]];
        }];

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
    }

    - (void)applicationDidChangeScreenParameters:(NSNotification *)notification
    {
        primaryScreenHeightValid = FALSE;
        [self sendDisplaysChanged:FALSE];
    }

    - (void)applicationDidResignActive:(NSNotification *)notification
    {
        macdrv_event event;
        WineEventQueue* queue;

        [self invalidateGotFocusEvents];

        event.type = APP_DEACTIVATED;
        event.window = NULL;

        [eventQueuesLock lock];
        for (queue in eventQueues)
            [queue postEvent:&event];
        [eventQueuesLock unlock];
    }

    - (void)applicationWillFinishLaunching:(NSNotification *)notification
    {
        NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];

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
            [keyWindows removeObjectIdenticalTo:window];
            [orderedWineWindows removeObjectIdenticalTo:window];
        }];

        [nc addObserver:self
               selector:@selector(keyboardSelectionDidChange)
                   name:NSTextInputContextKeyboardSelectionDidChangeNotification
                 object:nil];

        /* The above notification isn't sent unless the NSTextInputContext
           class has initialized itself.  Poke it. */
        [NSTextInputContext self];

        self.keyboardType = LMGetKbdType();
    }

    - (void)applicationWillResignActive:(NSNotification *)notification
    {
        [orderedWineWindows enumerateObjectsWithOptions:NSEnumerationReverse usingBlock:^(id obj, NSUInteger idx, BOOL *stop){
            WineWindow* window = obj;
            NSInteger level = window.floating ? NSFloatingWindowLevel : NSNormalWindowLevel;
            if ([window level] > level)
                [window setLevel:level];
        }];
    }

@end

/***********************************************************************
 *              OnMainThread
 *
 * Run a block on the main thread synchronously.
 */
void OnMainThread(dispatch_block_t block)
{
    dispatch_sync(dispatch_get_main_queue(), block);
}

/***********************************************************************
 *              OnMainThreadAsync
 *
 * Run a block on the main thread asynchronously.
 */
void OnMainThreadAsync(dispatch_block_t block)
{
    dispatch_async(dispatch_get_main_queue(), block);
}

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
    NSString* message = [[NSString alloc] initWithFormat:format arguments:args];
    fprintf(stderr, "err:%s:%s", func, [message UTF8String]);
    [message release];
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
        [NSApp windowRejectedFocusEvent:event];
    });
}

/***********************************************************************
 *              macdrv_get_keyboard_layout
 *
 * Returns the keyboard layout uchr data.
 */
CFDataRef macdrv_copy_keyboard_layout(CGEventSourceKeyboardType* keyboard_type, int* is_iso)
{
    __block CFDataRef result = NULL;

    OnMainThread(^{
        TISInputSourceRef inputSource;

        inputSource = TISCopyCurrentKeyboardLayoutInputSource();
        if (inputSource)
        {
            CFDataRef uchr = TISGetInputSourceProperty(inputSource,
                                kTISPropertyUnicodeKeyLayoutData);
            result = CFDataCreateCopy(NULL, uchr);
            CFRelease(inputSource);

            *keyboard_type = ((WineApplication*)NSApp).keyboardType;
            *is_iso = (KBGetLayoutType(*keyboard_type) == kKeyboardISO);
        }
    });

    return result;
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
        ret = [NSApp setMode:display_mode forDisplay:display->displayID];
    });

    return ret;
}
